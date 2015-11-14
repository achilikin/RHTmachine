/* Simple wrapper for reading serial port (similar to getch) on ATmega32L

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

#ifndef SERIAL_CHAR_H
#define SERIAL_CHAR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#if 0 // to trick VisualAssist
}
#endif
#endif

// non-character flag
#define EXTRA_KEY   0x0100

// support for arrow keys for very simple one command deep history
#define ARROW_UP    0x0141
#define ARROW_DOWN  0x0142
#define ARROW_RIGHT 0x0143
#define ARROW_LEFT  0x0144

#define KEY_HOME    0x0101
#define KEY_INS	    0x0102
#define KEY_DEL	    0x0103
#define KEY_END	    0x0104
#define KEY_PGUP    0x0105
#define KEY_PGDN    0x0106

// default baud rate
#define UART_BAUD_RATE 38400LL // 38400 at 8MHz gives only 0.2% errors

void     serial_init(long baud_rate);
int      serial_putchar(char c, FILE *stream);
void     serial_puts(char *str);
uint16_t serial_getc(void);

#define serial_putc(x) serial_putchar((uint8_t)x, NULL);
#define serial_wait_sending() for(uint8_t i = 0; UCSRB & _BV(UDRIE); i++)

#ifdef __cplusplus
}
#endif

#endif
