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
#include "dip-sw.h"
#include "pins.h"

/* Global functions */

/*****************************************************************************/
void dipsw_init() {
    // Configure GPIO to input pullup/down
	gpio_set_mode(GPIO_PORT(DIPSW), GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_PIN(DIPSW1));
	gpio_set_mode(GPIO_PORT(DIPSW), GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_PIN(DIPSW2));
	gpio_set_mode(GPIO_PORT(DIPSW), GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_PIN(DIPSW3));
	// Pull-up
	gpio_set(GPIO_PORT(DIPSW), GPIO_PIN(DIPSW1));
	gpio_set(GPIO_PORT(DIPSW), GPIO_PIN(DIPSW2));
	gpio_set(GPIO_PORT(DIPSW), GPIO_PIN(DIPSW3));
}

/*****************************************************************************/
uint8_t dipsw_read() {
	uint8_t result = 0;
	result |= gpio_get(GPIO_PORT(DIPSW), GPIO_PIN(DIPSW1)) == 0 ? 1 : 0;
	result |= gpio_get(GPIO_PORT(DIPSW), GPIO_PIN(DIPSW2)) == 0 ? 2 : 0;
	result |= gpio_get(GPIO_PORT(DIPSW), GPIO_PIN(DIPSW3)) == 0 ? 4 : 0;
	return result;
}
