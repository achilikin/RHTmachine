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
#include "fdd.h"

#ifndef OUTPUT_FAST
#define OUTPUT_FAST OUTPUT
#endif

FddController::FddController(uint8_t dir, uint8_t eot, int8_t step, uint8_t vrange)
{
	d_dir  = dir;
	d_eot  = eot;
	d_step = step;
	range  = vrange;

	pos = 0;
	step = 0;
}

void FddController::set_dir(edir dir)
{
	if (dir == dir_up) {
		pinMode(d_dir, OUTPUT);
		digitalWrite(d_dir, LOW);
		step = 1;
		return;
	}
	step = -1;
	pinMode(d_dir, INPUT);
}

void FddController::seek(uint8_t steps, bool limits)
{
	for(uint8_t i = 0; i < steps; i++) {
		digitalWrite(d_step, HIGH);
		delayMicroseconds(3);
		digitalWrite(d_step, LOW);
		delayMicroseconds(3000);
		pos += step;
		if (limits && (pos == 0 || pos == range))
			break;
	}
}

void FddController::begin(float minimum, float maximin)
{
	// for a smooth movement we need 3ms between step signals
	pinMode(d_step, OUTPUT_FAST);
	digitalWrite(d_step, LOW);
	pinMode(d_dir, INPUT);
	pinMode(d_eot, INPUT);

	t_min = minimum;
	t_max = maximin;
}

bool FddController::init(void)
{
	uint8_t	eot = 1;

	set_dir(dir_up);  
	seek(1, false);
	seek(1, false);
	set_dir(dir_down);

	// DVD step motor has 135 steps, so just in case we use 140
	for(uint8_t i = 0; i < (range + 5); i++) {
		seek(1, false);
		// check for End of Track
		eot = digitalRead(d_eot);
		if (!eot) {
			set_dir(dir_up);
			break;
		}
	}

	pos = 0;
	return (eot == 0);
}

uint8_t FddController::set(float fset)
{
	if (fset < t_min)
		fset = t_min;
	if (fset > t_max)
		fset = t_max;

	uint8_t position = uint8_t(((fset - t_min)/(t_max - t_min)) * float(range));
	if (position != pos) {
		if (position < pos) {
			set_dir(dir_down);
			seek(pos - position);
			set_dir(dir_up); // turn direction led off
		}
		else {
			set_dir(dir_up);
			seek(position - pos);
		}
	}
	return pos;
}

uint8_t FddController::get(uint8_t pos, uint8_t *dec)
{
	uint8_t val = 0;
	if (pos <= range) {
		double fval = double(pos) / double(range);
		fval = fval*(t_max - t_min) + t_min;
		val  = uint8_t(fval);
		*dec = uint8_t((uint32_t(fval*10. + 0.5) % 10l));
	}
	return val;
}
