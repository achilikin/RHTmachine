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

#include "ds3231.h"
#include "i2cmaster.h"

#define I2C_RTC (0x68 << 1)

static uint8_t tobcd(const uint8_t val)
{
	return val + 6 * (val / 10);
}

static uint8_t todec(uint8_t val)
{
	return val - 6 * (val >> 4);
}

static int8_t ds3231_read(uint8_t addr, uint8_t *data, int8_t len)
{
	if (i2c_start_ex(I2C_RTC | I2C_WRITE, 200))
		return -1;
	i2c_write(addr);
	i2c_start_ex(I2C_RTC | I2C_READ, 200);
	int8_t i;
	for (--len, i = 0; i <= len; i++)
		data[i] = i2c_read(len-i);
	i2c_stop();
	return 0;
}

static int8_t ds3231_write(uint8_t addr, uint8_t *data, int8_t len)
{
	if (i2c_start_ex(I2C_RTC | I2C_WRITE, 200))
		return -1;
	i2c_write(addr);
	int8_t i;
	for (i = 0; i < len; i++)
		i2c_write(data[i]);
	i2c_stop();
	return 0;
}

int8_t ds3231_get_time(uint8_t *h, uint8_t *m, uint8_t *s)
{
	uint8_t data[3];
	if (ds3231_read(0, data, 3))
		return - 1;
	*s = todec(data[0]);
	*m = todec(data[1]);
	*h = todec(data[2]);
	return 0;
}

int8_t ds3231_set_time(uint8_t h, uint8_t m, uint8_t s)
{
	uint8_t data[3];
	data[0] = tobcd(s);
	data[1] = tobcd(m);
	data[2] = tobcd(h);
	return ds3231_write(0, data, 3);
}

int8_t ds3231_get_date(uint8_t *year, uint8_t *month, uint8_t *day)
{
	uint8_t data[3];
	if (ds3231_read(4, data, 3))
		return -1;
	*day   = todec(data[0]);
	*month = todec(data[1] & 0x1F);
	*year  = todec(data[2]);
	return 0;
}

int8_t ds3231_set_date(uint8_t year, uint8_t month, uint8_t day)
{
	uint8_t data[3];
	data[0] = tobcd(day);
	data[1] = tobcd(month);
	data[2] = tobcd(year);
	return ds3231_write(4, data, 3);
}

int8_t ds3231_get_temperature(int8_t *tval, uint8_t *dec)
{
	uint8_t data[2];
	if (ds3231_read(0x11, data, 2))
		return -1;
	*tval = data[0] & 0x7F;
	if (data[0] & 0x80)
		*tval *= -1;
	*dec = 25 * (data[1] >> 6);
	return 0;
}

int8_t ds3231_set_cfg(uint8_t flags)
{
	uint8_t data[2];
	if (ds3231_read(0x0E, data, 2))
		return -1;
	data[0] &= ~(DS3231_SQWEN | DS3231_SQW8K);
	data[1] &= 0xF7;
	if (flags & DS3231_SQWEN)
		data[0] |= flags & (DS3231_SQWEN | DS3231_SQW8K);
	if (flags & DS3231_32KEN)
		data[1] |= 0x08;
	ds3231_write(0x0E, data, 2);
	return 0;
}
