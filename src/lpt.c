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
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include "lpt.h"
#include "pins.h"
#include "systick.h"

/* Defines */

#define queue_size 16384	// 16K

/* Local variables */

static size_t queue_head = 0;
static size_t queue_tail = 0;
static uint8_t queue_data[queue_size];
static bool lpt_has_data = false;
static uint8_t lpt_data;

/* Local functions */

/*****************************************************************************/
static bool queue_write(uint8_t data) {
	if (((queue_head + 1) % queue_size) == queue_tail) {
		return false;	// No space
	}
	queue_data[queue_head] = data;
	queue_head = (queue_head + 1) % queue_size;
	return true;
}

/* Global functions */

/*****************************************************************************/
void exti9_5_isr(void) {
	exti_reset_request(EXTI7);
	gpio_set(GPIO_PORT(LPT_DATA), GPIO_PIN(LPT_BUSY));	// Set BUSY flag
	uint16_t raw = gpio_port_read(GPIO_PORT(LPT_DATA));	// Data in PB15~PB8
	lpt_data = raw >> 8;
	lpt_has_data = true;
	lpt_loop();
}

/*****************************************************************************/
void lpt_init(void) {
	// Set GPIO15~8 and STROBE to INPUT
	gpio_set_mode(GPIO_PORT(LPT_DATA), GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, 
		GPIO15|GPIO14|GPIO13|GPIO12|GPIO11|GPIO10|GPIO9|GPIO8|GPIO_PIN(LPT_STROBE));
	// Pull-up
	gpio_set(GPIO_PORT(LPT_DATA), 
		GPIO15|GPIO14|GPIO13|GPIO12|GPIO11|GPIO10|GPIO9|GPIO8|GPIO_PIN(LPT_STROBE));
	// BUSY is output LOW default
	gpio_set_mode(GPIO_PORT(LPT_DATA), GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO_PIN(LPT_BUSY));
	gpio_clear(GPIO_PORT(LPT_DATA), GPIO_PIN(LPT_BUSY));
	// ACK is output HIGH default
	gpio_set_mode(GPIO_PORT(LPT_ACK), GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO_PIN(LPT_ACK));
	gpio_set(GPIO_PORT(LPT_ACK), GPIO_PIN(LPT_ACK));
	// Interrupt (falling edge)
	nvic_enable_irq(NVIC_EXTI9_5_IRQ);
	exti_select_source(EXTI7, GPIO_PORT(LPT_DATA));	// EXTI7 = GPIO7
	exti_set_trigger(EXTI7, EXTI_TRIGGER_FALLING);
	exti_enable_request(EXTI7);
}

/*****************************************************************************/
void lpt_loop() {
	// Check if has data from LPT port
	if (lpt_has_data) {
		// Yes, try enqueue
		if (queue_write(lpt_data)) {
			// Ok, has space, then clear flag, pulse ACK and release BUSY signal
			lpt_has_data = false;
			gpio_clear(GPIO_PORT(LPT_ACK), GPIO_PIN(LPT_ACK));
			// Is not possible to use delay*() here
			for (int i = 0; i < 1000; i++) {
				__asm("NOP");	// Wait a little
			}
			gpio_set(GPIO_PORT(LPT_ACK), GPIO_PIN(LPT_ACK));
			gpio_clear(GPIO_PORT(LPT_DATA), GPIO_PIN(LPT_BUSY));
		}
	}
}

/*****************************************************************************/
int lpt_get_char() {
	if (queue_tail == queue_head) {
		// Empty queue, return error
		return -1;
	}
	uint8_t data = queue_data[queue_tail];
	queue_tail = (queue_tail + 1) % queue_size;
	return (int)data;
}
