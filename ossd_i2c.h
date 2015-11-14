/* LED display support for ATmega/Galilleo/Edison

   Copyright (c) 2015 Andrey Chilikin (https://github.com/achilikin)

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
   https://opensource.org/licenses/MIT
*/
#ifndef OLED_SSD1306_I2C_H
#define OLED_SSD1306_I2C_H

/**
	Limited set of functions for SSD1306 compatible OLED displays in text mode
	to minimize memory footprint if used on Atmel AVRs with low memory.
*/

#ifdef __cplusplus
extern "C" {
#if 0
} // dummy bracket for Visual Assist
#endif
#endif

/** Target platform */
#define OSSD_AVR	 1 /*< AVR compiler */
#define OSSD_RPI	 2 /*< Raspberry Pi */
#define OSSD_GALILEO 3 /*< Reserved for Intel Galileo */

#ifdef __AVR_ARCH__
	#define OSSD_TARGET OSSD_AVR
#else
	#define OSSD_TARGET OSSD_RPI
#endif

#if (OSSD_TARGET == OSSD_AVR)
	#define I2C_OSSD (0x3C << 1)
#else
	#include <stdint.h>
#endif

/** 
  flat cable connected at the top
  use ossd_init(OSSD_UPDOWN) to rotate screen
  */
#define OSSD_NORMAL 0x00
#define OSSD_UPDOWN 0x09

/** set default parameters */
int8_t ossd_init(uint8_t orientation);

/** fill screen with specified pattern */
void ossd_fill_screen(uint8_t data);

/** set display to sleep mode */
void ossd_sleep(uint8_t on_off);

/** set display contrast */
void ossd_set_contrast(uint8_t val);

void ossd_goto(uint8_t line, uint8_t x);
int8_t ossd_write(uint8_t data);

/**
 output string up to 64 chars in length
 line: 0-7
 x:    0-127, or -1 for centre of the line
 str:  output string
 atr:  OSSD_TEXT_*
 */
void ossd_putlx(uint8_t line, int8_t x, const char *str, uint8_t atr);
void ossd_putcx(uint8_t line, int8_t x, uint8_t ch, uint8_t atr);

/** void screen */
static inline void ossd_cls(void) {
	ossd_fill_screen(0);
}

#ifdef __cplusplus
}
#endif

#endif
