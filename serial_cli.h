/* Simple command line parser for ATmega32
  
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

#ifndef SERIAL_CLI_H
#define SERIAL_CLI_H

#ifdef __cplusplus
extern "C" {
#if 0 // to trick VisualAssist
}
#endif
#endif

#ifndef CMD_LEN
#define CMD_LEN 0x7F // big enough for our needs
#endif

#define CLI_EOK      0 // success
#define CLI_EARG    -1 // invalid argument
#define CLI_ENOTSUP -2 // command not supported
#define CLI_ENODEV  -3 // device communication error

// command line processing, returns CLI_E* above
typedef int8_t cli_processor(char *buf, void *ptr);

void   cli_init(void);
int8_t cli_interact(cli_processor *process, void *ptr);

// helper functions
char *get_arg(char *str);

static inline int8_t str_is(const char *cmd, const char *str)
{
	return strcmp_P(cmd, str) == 0;
}

static inline uint16_t free_mem(void)
{
	extern int __heap_start, *__brkval; 
	unsigned val;
	val = (unsigned)((int)&val - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
	return val;
}

int8_t cli_proc(char *buf, void *ptr);

#ifdef __cplusplus
}
#endif
#endif
