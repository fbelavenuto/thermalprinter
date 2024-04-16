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
#include <libopencm3/stm32/gpio.h>
#include "motor.h"
#include "pins.h"
#include "systick.h"

/* Constants */

static const uint32_t delay = 600;

/* Global functions */

/*****************************************************************************/
void motor_init() {
    // Configure GPIO to output
	gpio_set_mode(GPIO_PORT(MOT_EN), GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO_PIN(MOT_EN));
	gpio_set_mode(GPIO_PORT(MOT_DIR), GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO_PIN(MOT_DIR));
	gpio_set_mode(GPIO_PORT(MOT_STEP), GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO_PIN(MOT_STEP));
	motor_off();
}

/*****************************************************************************/
void motor_off() {
	gpio_set(GPIO_PORT(MOT_EN), GPIO_PIN(MOT_EN));
}

/*****************************************************************************/
void motor_pulse(int steps) {	// >0 = forward
	int s = abs(steps);
	// turn on motor
	gpio_clear(GPIO_PORT(MOT_EN), GPIO_PIN(MOT_EN));
	if (steps > 0) {
		gpio_set(GPIO_PORT(MOT_DIR), GPIO_PIN(MOT_DIR));
	} else {
		gpio_clear(GPIO_PORT(MOT_DIR), GPIO_PIN(MOT_DIR));
	}
	while(s--) {
		gpio_set(GPIO_PORT(MOT_STEP), GPIO_PIN(MOT_STEP));
		delay_us(delay);
		gpio_clear(GPIO_PORT(MOT_STEP), GPIO_PIN(MOT_STEP));
		delay_us(delay);
	}
}
