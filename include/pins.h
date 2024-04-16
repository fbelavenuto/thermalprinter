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

#ifndef __PINS_H__
#define __PINS_H__

/* Defines */

#define GPIO_PORT(G) G ## _PORT
#define GPIO_PIN(G) G ## _PIN

#define LED_PORT		 	GPIOC
#define LED_PIN 			GPIO13

#define MOT_EN_PORT 		GPIOB
#define MOT_EN_PIN	 		GPIO4

#define MOT_DIR_PORT		GPIOB
#define MOT_DIR_PIN 		GPIO3

#define MOT_STEP_PORT		GPIOA
#define MOT_STEP_PIN		GPIO15

#define BUTTON_PORT			GPIOB
#define BUTTON_PIN			GPIO5

#define PAPER_SENSOR_PORT	GPIOB
#define PAPER_SENSOR_PIN	GPIO1

#define LPT_DATA_PORT		GPIOB
#define LPT_STROBE_PIN		GPIO7
#define LPT_BUSY_PORT		GPIOB
#define LPT_BUSY_PIN		GPIO6
#define LPT_ACK_PORT		GPIOA
#define LPT_ACK_PIN			GPIO10

#define THERMISTOR_PORT		GPIOB
#define THERMISTOR_PIN		GPIO0

#define THERMALHEAD_PORT		GPIOA
#define THERMALHEAD_DSTB_PIN	GPIO1
#define THERMALHEAD_DSTA_PIN	GPIO0
#define THERMALHEAD_LATCH_PIN	GPIO4
#define THERMALHEAD_CLK_PIN		GPIO5
#define THERMALHEAD_DATA_PIN	GPIO7

#define DIPSW_PORT			GPIOA
#define DIPSW1_PIN			GPIO2
#define DIPSW2_PIN			GPIO3
#define DIPSW3_PIN			GPIO8

#endif //__PINS_H__
