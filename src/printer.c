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
#include <string.h>
#include "printer.h"
#include "thermalhead.h"
#include "led.h"
#include "button.h"
#include "dip-sw.h"
#include "motor.h"
#include "titleimg.h"
#include "font_6x10.h"
#include "font_8x16_t1.h"
#include "font_8x16_t2.h"
#include "font_9x16_t1.h"
//#include "font_9x16_t2.h"
#include "font_10x20.h"
#include "font_12x24.h"

/* Typedefs */

typedef struct {
	int width;
	int height;
	int columns;
	const uint8_t (*pointer)[];
} Font;

/* Constants */

static const Font fonts[] = {

	{
		.width = 6,
		.height = 10,
		.columns = 64,
		.pointer = &font_6x10,
	},
	{
		.width = 8,
		.height = 16,
		.columns = 48,
		.pointer = &font_8x16_t1,
	},
	{
		.width = 8,
		.height = 16,
		.columns = 48,
		.pointer = &font_8x16_t2,
	},
	{
		.width = 9,
		.height = 16,
		.columns = 42,
		.pointer = &font_9x16_t1,
	},
/*	{
		.width = 9,
		.height = 16,
		.columns = 42,
		.pointer = &font_9x16_t2,
	},*/
	{
		.width = 10,
		.height = 20,
		.columns = 38,
		.pointer = &font_10x20,
	},
	{
		.width = 12,
		.height = 24,
		.columns = 32,
		.pointer = &font_12x24,
	},
};

static const int max_fonts = sizeof(fonts) / sizeof(Font);

/* Enums */

enum lpt_state {
	LS_CHAR = 0,
	LS_ESC,
	LS_ESC_PARAM,
};

enum esc_state {
	ES_NONE = 0,
	ES_IGNORE,
	ES_WAIT_NULL,
	ES_SECOND_PARAM,
	ES_SELECT_FONT,
	ES_PAGE_LENGTH,
	ES_HORIZONTAL_TABS,
	ES_VERTICAL_TABS,
	ES_LINE_SPACING,
	ES_FORWARD_FEED,
	ES_PRINT_DATA,
	ES_GRAPH_MODE,
};

/* Global variables */

int steps_by_line = DEFAULT_STEPS_BY_LINE_TEXT;
bool bold = false;

/* Local variables */

static int column = 0;
static int line = 1;
static int line_spacing;
static int default_font_number = 0;
static int font_number;
static int default_page_length = 40;
static int page_length;
static bool default_bold = false;
static enum lpt_state lpt_state = LS_CHAR;
static enum esc_state esc_state = ES_NONE;
static bool graph_mode = false;
static int graph_bytes_per_column = 0;
static int horizontal_tabs[32];
static int vertical_tabs[16];
static uint8_t graph_buffer[(384/8)*48];			// 48 lines of graphics

/* Local functions */

/*****************************************************************************/
static void print_buffer(void) {

	const int font_height = fonts[font_number].height;
	int number_lines, step_lines;
	int lines_stepped = 0;

	led_set_printing(true);
	if (graph_mode) {
		steps_by_line = DEFAULT_STEPS_BY_LINE_GRAPH;
		number_lines = step_lines = 8 * graph_bytes_per_column;
		graph_mode = false;
	} else {
		number_lines = font_height;
		step_lines = line_spacing;
	}
	// Send buffer to Thermal Head, one line at a time
	for (int i = 0; i < number_lines; i++) {
		if (!th_print(graph_buffer + i * 48)) {
			// If out of paper, wait user to press button to continue
			while (!button_get_press()) {
				__asm("NOP");
			}
		}
		++lines_stepped;
	}
	int diff = step_lines - lines_stepped;
	if (diff) {
		motor_pulse(diff);
	}
	steps_by_line = DEFAULT_STEPS_BY_LINE_TEXT;
	if (++line == page_length) {
		line = 1;
	}
	// Clear buffer
	memset(graph_buffer, 0, sizeof(graph_buffer));
	column = 0;
	led_set_printing(false);
}

/*****************************************************************************/
static void buffer_char(const uint8_t c) {

	const int font_width = fonts[font_number].width;
	const int font_height = fonts[font_number].height;
	const uint8_t (*pointer)[] = fonts[font_number].pointer;
	const int font_byte_width = (int)((font_width+7) / 8);

	if (column + font_width >= 384) {
		print_buffer();
		column = 0;
	}
	for (int y = 0; y < font_height; y++) {
		uint16_t font_data = 0;
		// Get all bytes from font and combine in one variable
		int font_offs = c * font_height * font_byte_width + (font_byte_width * y);
		for (int t = 0; t < font_byte_width; t++) {
			font_data <<= 8;
			font_data |= (*pointer)[font_offs++];
		}
		font_data >>= 8 * font_byte_width - font_width;
		// Calculate Byte and bit position
		int byte_pos = (int)(column / 8);
		int bit_pos = 8-(column % 8);
		// Algorithm to put font data in correct position of line_buffer
		int font_bits = font_width;
		int byte_offs = byte_pos + (y * 48);
		while (font_bits > 0) {
			if (font_bits < bit_pos) {
				graph_buffer[byte_offs] |= (font_data << (bit_pos-font_bits) & 0xFF);
			} else {
				graph_buffer[byte_offs] |= (font_data >> (font_bits - bit_pos) & 0xFF);
			}
			font_bits -= bit_pos;
			bit_pos = 8;
			++byte_offs;
		}
	}
	column += font_width;
}

/*****************************************************************************/
static void set_default(void) {
	if (column > 0) {
		print_buffer();
	}
	column = 0;
	font_number = default_font_number;
	bold = default_bold;
	steps_by_line = DEFAULT_STEPS_BY_LINE_TEXT;
	page_length = default_page_length;
	line_spacing = fonts[font_number].height;
	for (int i = 0; i < 32; i++) {
		int temp = i*fonts[font_number].width * 8;
		if (temp >= 384) {
			temp = 0;
		}
		horizontal_tabs[i] = temp;
	}
	for (int i = 0; i < 16; i++) {
		vertical_tabs[i] = 0;
	}
}

/* Global functions */

/*****************************************************************************/
void printer_init() {

	// Define default parameters
	uint8_t ds = dipsw_read();
	default_font_number = (ds & 0x01) != 0 ? 5    : 2;
	default_page_length = (ds & 0x02) != 0 ? 80   : 40;
	default_bold        = (ds & 0x04) != 0 ? true : false;
	set_default();
}

/*****************************************************************************/
void printer_title() {
	led_set_printing(true);
	steps_by_line = DEFAULT_STEPS_BY_LINE_GRAPH;
	for (int i = 0; i < 128; i++) {
		th_print(title_img+i*48);
	}
	steps_by_line = DEFAULT_STEPS_BY_LINE_TEXT;
	motor_pulse(9 * steps_by_line);
	char temp[24];
	sprintf(temp, "Head temperature: %d%cC", (int)th_get_temp(), 0xF8);
	printer_text(temp);
	motor_pulse(9 * steps_by_line);
	for (int c = 0; c < 256; c++) {
		printer_char((uint8_t)c);
	}
	if (column > 0) {
		print_buffer();
	}
	motor_pulse(12 * steps_by_line);
	motor_off();
	line = 1;
	led_set_printing(false);
}

/*****************************************************************************/
void printer_char(uint8_t c) {
	buffer_char(c);
}

/*****************************************************************************/
void printer_text(char* text) {
	for (size_t i = 0; i < strlen(text); i++) {
		buffer_char((uint8_t)text[i]);
	}
	if (column > 0) {
		print_buffer();
	}
}

/*****************************************************************************/
void printer_parse(uint8_t c) {

	const int font_width  = fonts[font_number].width;
	const int font_height  = fonts[font_number].height;
	static int byte_count = 0;
	static int temp_count = 0;
	static int ignore_chars = 0;
	static int graph_type = 0;
	static int graph_length = 0;
	static bool CR = false;

	switch (lpt_state) {
		case LS_CHAR:
			if (c > 31) {
				buffer_char(c);
			} else {
				switch (c) {

					case '\x8':				// BS: Backspace
						column -= font_width;
						if (column < 0) {
							column = 0;
						}
						break;

					case '\x9':				// HT: Perform horizontal tab
						for (temp_count = 0; temp_count < 32; temp_count++) {
							if (horizontal_tabs[temp_count] > column) {
								column = horizontal_tabs[temp_count];
								break;
							}
						}
						break;

					case '\xA':				// LF: Line feed
						if (!CR) {
							print_buffer();
						}
						CR = false;
						break;

					case '\xB':				// VT: Perform vertical tab
						for (temp_count = 0; temp_count < 16; temp_count++) {
							if (vertical_tabs[temp_count] > line) {
								temp_count = line;
								line = vertical_tabs[temp_count];
								if (line > page_length) {
									line = page_length;
								}
								motor_pulse(line_spacing * (line-temp_count) * steps_by_line);
								break;
							}
						}
						break;

					case '\xC':				// FF: Page feed
						temp_count = line_spacing * (page_length - line) * steps_by_line;
						print_buffer();
						motor_pulse(temp_count);
						break;

					case '\xD':				// CR: Carriage return
						print_buffer();
						CR = true;
						break;

					case '\x1B':			// ESC: ESC/P mode
						lpt_state = LS_ESC;
						break;

					case '\xE':				// SO: Specify auto canceling stretched characters
					case '\xF':				// SI: Specify compressed characters
					case '\x12':			// DC2: Cancel compressed characters
					case '\x14':			// DC4: Cancel auto canceling double width characters
					default:
						break;
				}
			}
			break;

		case LS_ESC:
			switch (c) {
				case '@':					// Initialize
					lpt_state = LS_CHAR;
					set_default();
					break;

				case 'C':					// Set page length in lines or inches
					esc_state = ES_PAGE_LENGTH;
					lpt_state = LS_ESC_PARAM;
					break;

				case 'D':					// Specify horizontal tab position
					esc_state = ES_HORIZONTAL_TABS;
					lpt_state = LS_ESC_PARAM;
					temp_count = 0;
					for (int i = 0; i < 32; i++) {
						horizontal_tabs[i] = 0;
					}
					break;

				case 'B':					// Specify vertical tab position
					esc_state = ES_VERTICAL_TABS;
					lpt_state = LS_ESC_PARAM;
					temp_count = 0;
					for (int i = 0; i < 16; i++) {
						vertical_tabs[i] = 0;
					}
					break;

				case '0':					// Specify line feed of 1/8 inch
					line_spacing = (int)(font_height / 1.3);
					lpt_state = LS_CHAR;
					break;

				case '2':					// Specify line feed of 1/6 inch
					line_spacing = font_height;
					lpt_state = LS_CHAR;
					break;

				case 'A':					// Specify line feed of n/60 inch
					esc_state = ES_LINE_SPACING;
					lpt_state = LS_ESC_PARAM;
					temp_count = 60;
					break;

				case '3':					// Specify line feed of n/180 inch
					esc_state = ES_LINE_SPACING;
					lpt_state = LS_ESC_PARAM;
					temp_count = 180;
					break;

				case 'J':					// Forward paper feed
					esc_state = ES_FORWARD_FEED;
					lpt_state = LS_ESC_PARAM;
					break;

				case 'E':					// Apply bold style
					bold = true;
					lpt_state = LS_CHAR;
					break;

				case 'F':					// Cancel bold style
					bold = false;
					lpt_state = LS_CHAR;
					break;

				case 'R':					// Select international character set
				case 'q':					// Select character style
				case 't':					// Select character code set
				case '-':					// Apply/cancel underlining
				case '!':					// Global formatting
				case ' ':					// Specify character spacing
				case 'l':					// Specify left margin
				case 'Q':					// Specify right margin
				case 'a':					// Specify alignment
				case 'W':					// Specify double width characters
				case 'w':					// Specify double height characters
				case 'x':					// Select NLQ or draft
					esc_state = ES_IGNORE;
					lpt_state = LS_ESC_PARAM;
					ignore_chars = 1;
					break;

				case '\\':					// Specify relative horizontal position
				case '$':					// Specify absolute horizontal position
					esc_state = ES_IGNORE;
					lpt_state = LS_ESC_PARAM;
					ignore_chars = 2;
					break;

				case 'X':					// Specify character size
					esc_state = ES_IGNORE;
					lpt_state = LS_ESC_PARAM;
					ignore_chars = 3;
					break;

				case '(':					// (need read next char)
					esc_state = ES_SECOND_PARAM;
					lpt_state = LS_ESC_PARAM;
					break;

				case '*':					// Select bit image
					esc_state = ES_GRAPH_MODE;
					lpt_state = LS_ESC_PARAM;
					byte_count = 0;
					break;
				
				case 'K':					// 8 dot single density bit image
					esc_state = ES_GRAPH_MODE;
					lpt_state = LS_ESC_PARAM;
					byte_count = 1;
					graph_type = 0;
					break;

				case 'L':					// 8 dot double density bit image
					esc_state = ES_GRAPH_MODE;
					lpt_state = LS_ESC_PARAM;
					byte_count = 1;
					graph_type = 1;
					break;

				case 'Y':					// 8 dot double speed double density bit image
					esc_state = ES_GRAPH_MODE;
					lpt_state = LS_ESC_PARAM;
					byte_count = 1;
					graph_type = 2;
					break;

				case 'Z':					// 8 dot quadruple density bit image
					esc_state = ES_GRAPH_MODE;
					lpt_state = LS_ESC_PARAM;
					byte_count = 1;
					graph_type = 3;
					break;

				case 'k':					// Select font
					esc_state = ES_SELECT_FONT;
					lpt_state = LS_ESC_PARAM;
					break;

				case '4':					// Apply italic style
				case '5':					// Cancel italic style
				case 'G':					// Apply double strike printing
				case 'H':					// Cancel double strike printing
				case 'P':					// Select 10 cpi
				case 'M':					// Select 12 cpi
				case 'g':					// Select 15 cpi
				case 'p':					// Specify proportional characters
				case '\xE':					// SO: Specify auto canceling stretched characters
				case '\xF':					// SI: Specify compressed characters
				default:
					lpt_state = LS_CHAR;
			}
			break;

		case LS_ESC_PARAM:
			switch (esc_state) {
				case ES_IGNORE:
					if (--ignore_chars == 0) {
						lpt_state = LS_CHAR;
					}
					break;

				case ES_WAIT_NULL:
					if (c == 0) {
						lpt_state = LS_CHAR;
					}
					break;

				case ES_SECOND_PARAM:
					switch (c) {
						case 'U':			// Set Unit
							esc_state = ES_IGNORE;
							ignore_chars = 3;
							break;

						case 'V':			// Specify absolute vertical position
						case 'v':			// Specify relative vertical position
						case 'C':			// Specify page length
							esc_state = ES_IGNORE;
							ignore_chars = 4;
							break;

						case 'c':			// Specify page format
							esc_state = ES_IGNORE;
							ignore_chars = 6;
							break;

						case '^':			// Print data as character
							temp_count = 0;
							esc_state = ES_PRINT_DATA;
							break;

						default:
							lpt_state = LS_CHAR;
					}
					break;
				
				case ES_SELECT_FONT:
					if (c < max_fonts) {
						font_number = c;
					} else {
						font_number = default_font_number;
					}
					line_spacing = fonts[font_number].height;
					lpt_state = LS_CHAR;
					break;

				case ES_PAGE_LENGTH:
					temp_count = 0;
					if (c == 0) {
						temp_count = 1;
						break;
					}
					if (c > 0 && c <= 127) {
						page_length = c;
					}
					lpt_state = LS_CHAR;
					break;

				case ES_LINE_SPACING:
					line_spacing = (int)(c / temp_count * font_height * 7.5f);
					if (line_spacing == 0) {
						line_spacing = 1;
					}
					lpt_state = LS_CHAR;
					break;

				case ES_FORWARD_FEED:
					temp_count = (int)(c / 180 * font_height * 7.5f);
					if (temp_count == 0) {
						temp_count = 1;
					}
					line =+ (int)(temp_count / font_height)+1;
					if (line > page_length) {
						line -= page_length;
					}
					motor_pulse(temp_count * steps_by_line);
					lpt_state = LS_CHAR;
					break;

				case ES_HORIZONTAL_TABS:
					if (c == 0 || temp_count == 32) {
						lpt_state = LS_CHAR;
						break;
					}
					byte_count = (c-1) * font_width;
					if (byte_count >= 384 || (temp_count > 0 && byte_count <= horizontal_tabs[temp_count-1])) {
						lpt_state = LS_CHAR;
						break;
					}
					horizontal_tabs[temp_count++] = byte_count;
					break;

				case ES_VERTICAL_TABS:
					if (c == 0 || temp_count == 16) {
						lpt_state = LS_CHAR;
						break;
					}
					if (temp_count > 0 && c <= vertical_tabs[temp_count-1]) {
						lpt_state = LS_CHAR;
						break;
					}
					vertical_tabs[temp_count++] = c;
					break;

				case ES_PRINT_DATA:
					if (temp_count == 0) {
						graph_length = c;
						++temp_count;
					} else if (temp_count == 1) {
						graph_length |= c << 8;
						++temp_count;
					} else {
						printer_char(c);
						--graph_length;
						if (graph_length == 0) {
							lpt_state = LS_CHAR;
						}
					}
					break;

				case ES_GRAPH_MODE:
					if (byte_count == 0) {
						graph_type = c;
						++byte_count;
					} else if (byte_count == 1) {
						graph_length = c;
						++byte_count;
					} else if (byte_count == 2) {
						graph_length |= c << 8;
						if (graph_type <= 31) {
							graph_bytes_per_column = 1;
						}
						if (graph_type > 31 && graph_type <= 70) {
							graph_bytes_per_column = 3;
						} else if (graph_type > 70) {
							graph_bytes_per_column = 6;
						}
						++byte_count;
						temp_count = 0;
					} else {
						// Buffer graph byte
						if (column < 384) {
							for (int b = 0; b < 8; b++) {
								int mask = (1 << (7-b));
								bool pixel = (c & mask) != 0;
								if (pixel) {
									int byte_offs = (int)(column / 8) + (48 * 8 * temp_count) + (48 * b);
									int bit_offs = 7-(column % 8);
									graph_buffer[byte_offs] |= 1 << bit_offs;
								}
							}
						}
						if (++temp_count == graph_bytes_per_column) {
							temp_count = 0;
							++column;
							--graph_length;
						}
						if (graph_length == 0) {
							// End of graphics
							graph_mode = true;
							lpt_state = LS_CHAR;
						}
						break;
					}
					break;

				default:
					lpt_state = LS_CHAR;
			}
			break;
	}
}

/*****************************************************************************/
void printer_flush(void) {
	if (column > 0) {
		print_buffer();
	}
}
