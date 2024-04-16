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
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include "systick.h"
#include "pins.h"

/* Constants */

static volatile uint64_t millis_value = 0;
static volatile uint64_t cmicros_value = 0;

/* Global functions */

/*****************************************************************************/
void sys_tick_handler(void)
{
	if (++cmicros_value % 10 == 0) {
		++millis_value;
	}
}

/*****************************************************************************/
void systick_init(void) {
    /* 72MHz / 8 => 9_000_000 counts per second */
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
    /* 9_000_000/900 = 10_000 overflows per second - every 100us one interrupt */
	/* SysTick interrupt every N clock pulses: set reload to N-1 */
	systick_set_reload(900-1);
	systick_interrupt_enable();
	/* Start counting. */
	systick_counter_enable();
}

/*****************************************************************************/
uint64_t millis(void) {
	return millis_value;
}

/*****************************************************************************/
void delay_us(uint32_t us) {
	uint64_t end = cmicros_value + (us/100);
	while (cmicros_value < end) {
		__asm("NOP");
	}
}

/*****************************************************************************/
void delay_ms(uint32_t ms) {
	uint64_t end = millis() + ms;
	while (millis() < end) {
		__asm("NOP");
	}
}
