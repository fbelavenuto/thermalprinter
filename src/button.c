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
#include "button.h"
#include "pins.h"
#include "systick.h"

/* Constants */

static const int dbl_threshold = 200;
static const int long_threshold = 400;

/* Local variables */

static bool last_pressed = false;
static uint64_t last_ms = 0;

/* Global functions */

/*****************************************************************************/
void button_init() {
    // Configure GPIO to input pullup/down
	gpio_set_mode(GPIO_PORT(BUTTON), GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_PIN(BUTTON));
	// Pull-up
	gpio_set(GPIO_PORT(BUTTON), GPIO_PIN(BUTTON));
}

/*****************************************************************************/
bool button_get_press() {
	return gpio_get(GPIO_PORT(BUTTON), GPIO_PIN(BUTTON)) == 0;
}

/*****************************************************************************/
enum ButtonState button_get_state(void) {

	enum ButtonState result = BS_IDLE;
	bool pressed = gpio_get(GPIO_PORT(BUTTON), GPIO_PIN(BUTTON)) == 0;
	uint64_t now = millis();
	int ms_passed = now - last_ms;

	// Test single and long press
	if (pressed) {
		if (last_ms) {
			if (ms_passed > long_threshold) {
				result = BS_LONG_PRESS;
			}
		}
	} else {
		if (last_ms) {
			if (ms_passed > dbl_threshold && ms_passed < long_threshold) {
				result = BS_SINGLE_PRESS;
				last_ms = 0;
			} else if (ms_passed > long_threshold) {
				last_ms = 0;
			}
		}
	}
	// When button state changed
	if (last_pressed != pressed) {
		last_pressed = pressed;
		if (pressed) {					// Button pressed
			if (last_ms) {
				last_ms = 0;
				result = BS_DOUBLE_PRESS;
			} else {
				last_ms = now;
			}
		}
	}
	return result;
}
