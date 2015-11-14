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
#ifndef RH_GAUGE_HEADER
#define RH_GAUGE_HEADER

class AnalogueGauge {
private:
	const uint8_t *vmap;
	float   min_val, max_val;
	uint8_t d_pin;
	uint8_t lmap;
	uint8_t min_map, max_map;

public:
	AnalogueGauge(uint8_t pin, const uint8_t *pmap, uint8_t maplen);

	void begin(float minimum, float maximin);

	uint8_t set(float ft);
	void set(uint8_t pwm) {
		analogWrite(d_pin, pwm);
	}

	uint8_t get(uint8_t pos, uint8_t *dec);
};

#endif
