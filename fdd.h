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
#ifndef FDD_CONTROLLER_HEADER
#define FDD_CONTROLLER_HEADER

class FddController {
private:
	uint8_t d_dir;
	uint8_t d_eot;
	int8_t d_step;

	uint8_t pos;
	uint8_t step;
	uint8_t range;
	float t_min;
	float t_max;

public:
	enum edir { dir_up, dir_down };

	FddController(uint8_t dir, uint8_t eot, int8_t step, uint8_t range);

	void set_dir(edir dir);
	void seek(uint8_t steps, bool limits = true);
	void begin(float minimum, float maximin);
	bool init(void);

	uint8_t set(float ft);
	uint8_t get(uint8_t pos, uint8_t *dec);
};

#endif