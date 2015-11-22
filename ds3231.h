/* Copyright (c) 2015 Andrey Chilikin (https://github.com/achilikin)

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

#ifndef DS3231_RTC_HEADER
#define DS3231_RTC_HEADER

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#if 0
} // dummy bracket for Visual Assist
#endif
#endif

#define DS3231_32KEN 0x01 /* enable 32kHz signal (POR enabled) */
#define DS3231_SQWEN 0x40 /* enable squire wave signal (POR disabled) */
#define DS3231_SQW1H 0x00 /* 1Hz */
#define DS3231_SQW1K 0x08 /* 1kHz */
#define DS3231_SQW4K 0x10 /* 4kHz */
#define DS3231_SQW8K 0x18 /* 8kHz (POR) */

int8_t ds3231_set_cfg(uint8_t flags);

int8_t ds3231_get_time(uint8_t *hour, uint8_t *day, uint8_t *sec);
int8_t ds3231_set_time(uint8_t hour, uint8_t day, uint8_t sec);

int8_t ds3231_get_date(uint8_t *year, uint8_t *month, uint8_t *day);
int8_t ds3231_set_date(uint8_t year, uint8_t month, uint8_t day);

int8_t ds3231_get_temperature(int8_t *tval, uint8_t *dec);
#ifdef __cplusplus
}
#endif
#endif