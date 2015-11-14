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
#include <Arduino.h>

#include "gauge.h"

AnalogueGauge::AnalogueGauge(uint8_t pin, const uint8_t *pmap, uint8_t maplen)
{
	d_pin = pin;
	vmap = pmap;
	lmap = maplen;
	min_map = pmap[1];
	max_map = pmap[maplen - 1];
}

void AnalogueGauge::begin(float minimum, float maximin)
{
	pinMode(d_pin, OUTPUT);
	analogWrite(d_pin, 0);
	min_val = minimum;
	max_val = maximin;
}

uint8_t AnalogueGauge::set(float val)
{
	if (val < min_val) val = min_val;
	if (val > max_val) val = max_val;
	uint8_t pwm = int(val);

	for (uint8_t i = 0; i < lmap; i += 2) {
		if (pwm <= vmap[i]) {
			if (i == 0) {
				pwm = vmap[i + 1];
				break;
			}
			pwm = (int)map(double(pwm), double(vmap[i]), double(vmap[i-2]), double(vmap[i+1]), double(vmap[i-1]));
			break;
		}
	}
	set(pwm);
	return pwm;
}

uint8_t AnalogueGauge::get(uint8_t pos, uint8_t *dec)
{
	uint8_t val = 0;
	if (pos < min_map)
		pos = min_map;
	if (pos > max_map)
		pos = max_map;

	for (uint8_t i = 0; i < lmap; i += 2) {
		if (pos <= vmap[i+1]) {
			if (i == 0) {
				val = vmap[i];
				break;
			}
			double dval = map(double(pos), double(vmap[i-1]), double(vmap[i+1]), double(vmap[i-2]), double(vmap[i]));
			val = uint8_t(dval);
			*dec = uint8_t((uint32_t(dval*10.) % 10l));
			break;
		}
	}
	return val;
}
