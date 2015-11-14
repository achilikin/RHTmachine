/*  Simple command line parser for ATmega32 or similar
 
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

#include "serial.h"
#include "serial_cli.h"

char *get_arg(char *str)
{
	char *arg;

	for(arg = str; *arg && *arg != ' '; arg++);

	while(*arg && *arg == ' ') {
		*arg = '\0';
		arg++;
	}

	return arg;
}

static uint8_t  cursor;
static char cmd[CMD_LEN + 1];
static char hist[CMD_LEN + 1];

void cli_init(void)
{
	cursor = 0;

	for(uint8_t i = 0; i <= CMD_LEN; i++) {
		cmd[i] = '\0';
		hist[i] = '\0';
	}
	serial_puts("> ");
}

int8_t cli_interact(cli_processor *process, void *ptr)
{
	uint16_t ch;

	if ((ch = serial_getc()) == 0)
		return 0;

	if (ch & EXTRA_KEY) {
		if (ch == ARROW_UP && (cursor == 0)) {
			// execute last successful command
			for(cursor = 0; ; cursor++) {
				cmd[cursor] = hist[cursor];
				if (cmd[cursor] == '\0')
					break;
			}
			serial_puts(cmd);
		}
		return 1;
	}

	if (ch == '\n') {
		serial_putc(ch);
		cmd[cursor] = '\0';
		if (*cmd) {
			// remove trailing spaces
			while (cursor) {
				if (cmd[cursor] > ' ')
					break;
				cmd[cursor--] = '\0';
			}
			int8_t ret = process(cmd, ptr);
			memcpy(hist, cmd, sizeof(cmd));
			if (ret == CLI_EARG)
				Serial.print(F("Invalid argument\n"));
			else if (ret == CLI_ENOTSUP)
				Serial.print(F("Unknown command\n"));
			else if (ret == CLI_ENODEV)
				Serial.print(F("Device error\n"));
		}
		for(uint8_t i = 0; i < cursor; i++)
			cmd[i] = '\0';
		cursor = 0;
		serial_puts("> ");
		return 1;
	}

	// backspace processing
	if (ch == '\b') {
		if (cursor) {
			cursor--;
			cmd[cursor] = '\0';
			serial_putc('\b');
			serial_putc(' ');
			serial_putc('\b');
		}
	}

	// skip control or damaged bytes
	if (ch < ' ')
		return 0;

	// echo
	serial_putc((uint8_t)ch);

	cmd[cursor++] = (uint8_t)ch;
	cursor &= CMD_LEN;
	// clean up in case of overflow (command too long)
	if (!cursor) {
		for(uint8_t i = 0; i <= CMD_LEN; i++)
			cmd[i] = '\0';
	}

	return 1;
}
