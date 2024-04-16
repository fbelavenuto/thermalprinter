/*

 Copyright (c) 2024 - Fabio Belavenuto <belavenuto@gmail.com>

 All rights reserved

 Redistribution and use in source and synthezised forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 Redistributions in synthesized form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

 Neither the name of the author nor the names of other contributors may
 be used to endorse or promote products derived from this software without
 specific prior written permission.

 THIS CODE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

 You are responsible for any legal issues arising from your use of this code.

*/

#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <math.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include "thermalhead.h"
#include "pins.h"
#include "printer.h"
#include "systick.h"
#include "motor.h"

/* Constants */

static const uint32_t latch_delay = 100;
static const uint32_t cooldown_delay = 5000;

/* Defines */

#define B_COEFFICENT 3950.0f
#define THERMISTOR_RESISTANCE 20000.0f
#define RTH_NOMINAL 30000.0f
#define ADC_RESOLUTION 4096.0f
#define TEMP_NOMINAL 25.0f

/* Global functions */

/*****************************************************************************/
void th_init() {
	// Configure DSTs and LATCH to output push-pull
	gpio_set_mode(GPIO_PORT(THERMALHEAD), GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO_PIN(THERMALHEAD_DSTA));
	gpio_set_mode(GPIO_PORT(THERMALHEAD), GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO_PIN(THERMALHEAD_DSTB));
	gpio_set_mode(GPIO_PORT(THERMALHEAD), GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO_PIN(THERMALHEAD_LATCH));
	// Configure initial state
	gpio_clear(GPIO_PORT(THERMALHEAD), GPIO_PIN(THERMALHEAD_DSTA));	// 0
	gpio_clear(GPIO_PORT(THERMALHEAD), GPIO_PIN(THERMALHEAD_DSTB));	// 0
	gpio_set(GPIO_PORT(THERMALHEAD), GPIO_PIN(THERMALHEAD_LATCH));	// 1
	// Configure paper sensor to input
	gpio_set_mode(GPIO_PORT(PAPER_SENSOR), GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_PIN(PAPER_SENSOR));
	// Configure thermistor sensor to analog
	gpio_set_mode(GPIO_PORT(THERMISTOR), GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO_PIN(THERMISTOR));
	// Turn on ADC1
	rcc_periph_clock_enable(RCC_ADC1);
	// Make sure the ADC doesn't run during config.
	adc_power_off(ADC1);
	// We configure everything for one single conversion.
	adc_disable_scan_mode(ADC1);
	adc_set_single_conversion_mode(ADC1);
	adc_disable_external_trigger_regular(ADC1);
	adc_set_right_aligned(ADC1);
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC);
	adc_power_on(ADC1);
	// Wait for ADC starting up.
	for (int i = 0; i < 800000; i++) __asm__("nop");	// Wait a bit.
	adc_reset_calibration(ADC1);
	adc_calibrate(ADC1);
	// Enable SPI1 Periph
	rcc_periph_clock_enable(RCC_SPI1);
	// Configure GPIOs: SCK=PA5 and MOSI=PA7
	gpio_set_mode(GPIO_PORT(THERMALHEAD), GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, 
		GPIO_PIN(THERMALHEAD_CLK) | GPIO_PIN(THERMALHEAD_DATA) );
	// Reset SPI, SPI_CR1 register cleared, SPI is disabled
	rcc_periph_reset_pulse(RST_SPI1);
	// Set up SPI in Master mode with:
	//  Clock baud rate: 1/8 of peripheral clock frequency
	//  Clock polarity: Idle High
	//  Clock phase: Data valid on 1nd clock pulse
	//  Data frame format: 16-bit
	//  Frame format: MSB First
	spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_8, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
		SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_16BIT, SPI_CR1_MSBFIRST);
	// Set NSS management to software.
	//  Note:
	//  Setting nss high is very important, even if we are controlling the GPIO
	//  ourselves this bit needs to be at least set to 1, otherwise the spi
	//  peripheral will not send any data out.
	spi_enable_software_slave_management(SPI1);
	spi_set_nss_high(SPI1);

	// Enable SPI1 periph.
	spi_enable(SPI1);
}

/*****************************************************************************/
bool th_have_paper() {
	return gpio_get(GPIO_PORT(PAPER_SENSOR), GPIO_PIN(PAPER_SENSOR)) != 0;
}

/*****************************************************************************/
float th_get_temp() {
	uint8_t channel_array[16] = {0};

	/* Select the channel we want to convert. */
	channel_array[0] = 8;	// 8 = B0
	adc_set_regular_sequence(ADC1, 1, channel_array);

	// Start the conversion directly (ie without a trigger).
	adc_start_conversion_direct(ADC1);

	// Wait for end of conversion.
	while (!(adc_eoc(ADC1))) __asm("NOP");
	float read_value = adc_read_regular(ADC1);
	float thermistor_r = THERMISTOR_RESISTANCE * ((ADC_RESOLUTION / read_value) - 1.0f);
	float temp_celsius = (1.0f/(1.0f/(TEMP_NOMINAL+273.0f) - 1.0f/B_COEFFICENT * log(RTH_NOMINAL / thermistor_r))) - 273.0f;
	return temp_celsius;
}

/*****************************************************************************/
bool th_print(const uint8_t data[(384 / 8)]) {
	if (!th_have_paper()) {
		return false;
	}
	while (th_get_temp() > MAX_HEAD_TEMP) {
		motor_off();
		// Wait to cooldown
		delay_ms(cooldown_delay);
	}
	// Send bits via SPI peripheral
	for (int idx = 0; idx < (384 / 8); idx += 2) {
		uint16_t word = (data[idx] << 8) | data[idx+1];
		spi_send(SPI1, word);
	}
	// Latch data
	gpio_clear(GPIO_PORT(THERMALHEAD), GPIO_PIN(THERMALHEAD_LATCH));
	delay_us(latch_delay);
	gpio_set(GPIO_PORT(THERMALHEAD), GPIO_PIN(THERMALHEAD_LATCH));
	delay_us(latch_delay);
	// Turn on resistances
	int delay = bold ? 3500 : 2500;
	// A group
	gpio_set(GPIO_PORT(THERMALHEAD), GPIO_PIN(THERMALHEAD_DSTA));
	delay_us(delay);
	gpio_clear(GPIO_PORT(THERMALHEAD), GPIO_PIN(THERMALHEAD_DSTA));
	// B group
	gpio_set(GPIO_PORT(THERMALHEAD), GPIO_PIN(THERMALHEAD_DSTB));
	delay_us(delay);
	gpio_clear(GPIO_PORT(THERMALHEAD), GPIO_PIN(THERMALHEAD_DSTB));

	motor_pulse(steps_by_line);
	return true;
}
