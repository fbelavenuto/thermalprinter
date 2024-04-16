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
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include "led.h"
#include "pins.h"
#include "thermalhead.h"

/* Constants */

static const bool patterns[][6] = {
	// Stand-by
	{
		true, true, true, true, true, true, 
	},
	// Out of Paper
	{
		true, false, true, false, true, false,
	},
	// Printing
	{
		true, false, false, false, false, false, 
	},
	// Over temperature
	{
		true, true, true, false, true, false,
	},
};

/* Enumerates */

enum led_patterns {
    LP_STANDBY = 0,
    LP_OUTOFPAPER,
    LP_PRINTING,
	LP_OVERTEMP,
};

/* Local variables */

static volatile uint8_t counter = 0;
static volatile enum led_patterns actual_pattern = LP_STANDBY;
static bool printing = false;

/* Local functions */

/*****************************************************************************/
static void led_status_off(void) {
	gpio_clear(GPIO_PORT(LED), GPIO_PIN(LED));
}

/*****************************************************************************/
static void led_status_on(void) {
	gpio_set(GPIO_PORT(LED), GPIO_PIN(LED));
}

/*****************************************************************************/
static void update(void) {
	// Check states and define pattern
	bool overheat = th_get_temp() > MAX_HEAD_TEMP;
	bool have_paper = th_have_paper();
	if (overheat) {
		actual_pattern = LP_OVERTEMP;
	} else if (!have_paper) {
		actual_pattern = LP_OUTOFPAPER;
	} else if (printing) {
		actual_pattern = LP_PRINTING;
	} else {
		actual_pattern = LP_STANDBY;
	}
}

/* Global functions */

/*****************************************************************************/
void led_init(void) {
	/* LED status */
	// Configure GPIO to output
	gpio_set_mode(GPIO_PORT(LED), GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO_PIN(LED));
	/* Enable TIMER 3 clock. */
	rcc_periph_clock_enable(RCC_TIM3);
	/* Enable TIMER 3 interrupt. */
	nvic_enable_irq(NVIC_TIM3_IRQ);
	// Configure priority
	nvic_set_priority(NVIC_TIM3_IRQ, 1);
	/* Reset TIMER peripheral to defaults. */
	rcc_periph_reset_pulse(RST_TIM3);
	/* Enable trigger IRQ */
	timer_enable_irq(TIM3, TIM_DIER_UIE);
	/* Timer frequency */
	timer_set_prescaler(TIM3, rcc_apb1_frequency / (1/*ms*/ * 1000));
	/* Initial couter */
	timer_set_counter(TIM3, 0);
	/* Timer period */
	timer_set_period(TIM3, 150);    // units of timer frequency: 150ms
	/* Start timer */
	timer_enable_counter(TIM3);
	update();
}

/*****************************************************************************/
void led_set_printing(bool v) {
	printing = v;
}

/*****************************************************************************/
void tim3_isr(void) {
	// Clear interrupt
	timer_clear_flag(TIM3, TIM_SR_UIF);
	bool d = patterns[actual_pattern][counter++];
	if (counter == 6) {
		counter = 0;
		update();
	}
	if (d) {
		led_status_on();
	} else {
		led_status_off();
	}
}

/*****************************************************************************/
void hard_fault_handler(void) {
	uint32_t i;
	while(1)  {
#define LOOPVAL 500000
		led_status_on();
		for (i = 0; i < LOOPVAL; i++) {
			__asm("NOP");
		}
		led_status_off();
		for (i = 0; i < LOOPVAL; i++) {
			__asm("NOP");
		}
	}
}

/*****************************************************************************/
void bus_fault_handler(void) {
	hard_fault_handler();
}

/*****************************************************************************/
void usage_fault_handler(void) {
	hard_fault_handler();
}
