/*  Simple wrappers for reading serial port (similar to getch) on ATmega

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
#include <Arduino.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#include "serial.h"

// Escape sequence states
#define ESC_CHAR    0
#define ESC_BRACKET 1
#define ESC_BRCHAR  2
#define ESC_TILDA   3
#define ESC_CRLF    5

static FILE serial_stdout = { 0 };

int serial_putchar(char c, FILE *stream)
{
	Serial.write(c);
	return 0;
}

void serial_puts(char *str)
{
	while (*str)
		Serial.write(*str++);
}

void serial_init(long baud_rate)
{
	// all necessary initializations
	Serial.begin(baud_rate);

	// initialize printf()
	fdev_setup_stream(&serial_stdout, serial_putchar, NULL, _FDEV_SETUP_WRITE);
	stdout = &serial_stdout;
}

uint16_t serial_getc(void)
{
	static uint8_t esc = ESC_CHAR;
	static uint8_t idx = 0;
	int ch = 0;

	if (!Serial.available() || (ch = Serial.read()) < 0)
		return 0;
	
	// ESC sequence state machine
	if (ch == 27) {
		esc = ESC_BRACKET;
		return 0;
	}
	if (esc == ESC_BRACKET) {
		if (ch == '[') {
			esc = ESC_BRCHAR;
			return 0;
		}
	}
	if (esc == ESC_BRCHAR) {
		esc = ESC_CHAR;
		if (ch >= 'A' && ch <= 'D') {
			ch |= EXTRA_KEY;
			return ch;
		}
		if ((ch >= '1') && (ch <= '6')) {
			esc = ESC_TILDA;
			idx = ch - '0';
			return 0;
		}
		return ch;
	}
	if (esc == ESC_TILDA) {
		esc = ESC_CHAR;
		if (ch == '~') {
			ch = EXTRA_KEY | idx;
			return ch;
		}
		return 0;
	}

	// convert CR to LF 
	if (ch == '\r') {
		esc = ESC_CRLF;
		return '\n';
	}
	// do not return LF if it is part of CR+LF combination
	if (ch == '\n') {
		if (esc == ESC_CRLF) {
			esc = ESC_CHAR;
			return 0;
		}
	}
	esc = ESC_CHAR;
	return ch;
}
