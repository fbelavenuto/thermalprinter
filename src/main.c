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
#include <stdio.h>
#include <memory.h>
#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "pins.h"
#include "thermalhead.h"
#include "led.h"
#include "systick.h"
#include "motor.h"
#include "button.h"
#include "lpt.h"
#include "dip-sw.h"
#include "printer.h"

/* Global functions */

/*****************************************************************************/
int main(void) {
	bool diagMode = false;

	// Max clock of 72MHz
	rcc_clock_setup_in_hse_8mhz_out_72mhz();
    // Turn on clock of peripherals
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_AFIO);
	AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON; // PB4,PB5,PA15 as GPIO

	// Initialize modules
	systick_init();
	th_init();
	motor_init();
	button_init();
	lpt_init();
	dipsw_init();
	printer_init();
	led_init();

	// Wait for the button capacitor to stabilize
	delay_ms(100);
	// If the button is ON at power-up
	if (button_get_press()) {
		printer_title();
	}
	// If the button remains pressed, enter in diagnostic mode
	if (button_get_press()) {
		diagMode = true;
		printer_text("Diagnostic mode:");
		// Wait button release
		while (button_get_press()) {
			__asm("NOP");
		}
	}

	// Main loop
	while (true) {
		switch(button_get_state()) {
			case BS_IDLE:
				break;
			
			case BS_SINGLE_PRESS:
				printer_flush();
				motor_pulse(16 * steps_by_line);
				break;

			case BS_DOUBLE_PRESS:
				printer_flush();
				motor_pulse(-16 * 5 * steps_by_line);
				break;

			case BS_LONG_PRESS:
				printer_flush();
				motor_pulse(16 * 10 * steps_by_line);
				break;
		}
		lpt_loop();
		int lpt_data = lpt_get_char();
		if (lpt_data >= 0) {
			if (diagMode) {
				uint8_t t[3];
				sprintf((char*)t, "%02X", lpt_data);
				printer_char(t[0]);
				printer_char(t[1]);
				printer_char(' ');
			} else {
				// Parse incoming byte
				printer_parse((uint8_t)lpt_data);
			}
		} else {
			motor_off();
		}
	}
}
