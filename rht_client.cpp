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

#ifndef INPUT_FAST
#define INPUT_FAST INPUT
#endif

#ifndef OUTPUT_FAST
#define OUTPUT_FAST OUTPUT
#endif

#include "rht_client.h"

RhtClient::RhtClient(int data, uint8_t start_delay)
{
	d_data = data;
	start  = start_delay;
	if (start && start < 2)
		start= 2;
}

void RhtClient::begin(void)
{
	pinMode(d_data, INPUT_FAST); // RHT sensor input
}

int8_t RhtClient::poll(uint8_t dbg)
{
	uint16_t rht[45];
	uint16_t state;
	uint16_t counter;
	uint32_t stamp;

	for (uint8_t ntry = 0; ntry < 2; ntry++) {
		if (dbg) {
			if (ntry)
				printf_P(PSTR("re-"));
			printf_P(PSTR("starting RHT poll... "));
		}
		pinMode(d_data, OUTPUT_FAST);
		digitalWrite(d_data, LOW);
		if (start > 0) {
			delay(start);
			counter = start * 1000l;
		}
		else {
			delayMicroseconds(2);
			counter = 60000l;
		}
		stamp = millis();
		pinMode(d_data, INPUT_FAST); // RHT sensor input
		// wait for the sensor to pull down
		for (state = 0; state < counter && digitalRead(d_data) == HIGH; state++);
		if (state < counter)
			goto rht_ack;
	}
	return -1;

rht_ack:
    // measure sensor start bit HIGH state interval
    for(state = 0; state < counter && digitalRead(d_data) == LOW; state++);
    if (state == counter) return -2;
    for(state = 0; state < counter && digitalRead(d_data) == HIGH; state++);
	if (state == counter) return -3;

	// read in 40 bits timing high state
    for(uint8_t i = 0; i < 40; i++) {
        for(state = 0; state < counter && digitalRead(d_data) == LOW; state++);
        for(state = 0; state < counter && digitalRead(d_data) == HIGH; state++);
        rht[i] = state;
    }
	stamp = millis() - stamp;

	state = 0;
	for(uint8_t i = 0; i < 40; i++) {
		if (rht[i] > state)
			state = rht[i];
	}

	uint8_t *pdata = (uint8_t *)&rht[40];
    pdata[0] = pdata[1] = pdata[2] = pdata[3] = pdata[4] = 0;

    uint16_t delta = state/2 + state/4;
	if (state > 2)
		delta++;

	if (dbg)
		printf_P(PSTR("took %lums, max %u, delta %u\n"), stamp, state, delta);

	// convert timings to bits
	for(uint8_t n = 0; n < 5; n++) {
		for (uint8_t i = 0; i < 8; i++) {
			uint16_t val = rht[n*8 + i];
			if (dbg)
				printf(" %u", val);
			if (val > delta)
				pdata[n] |= 0x80 >> i;
		}
		if (dbg)
			printf(" | %02X\n", pdata[n]);
    }

	// validate checksum
	if (pdata[4] == uint8_t(pdata[0] + pdata[1] + pdata[2] + pdata[3])) {
		int16_t val;
		// convert to humidity
		val = pdata[0];
		val <<= 8;
		val |= pdata[1];
		if (val == 0) // treat 0% as an error
			return -5;
		rh = val / 10.0;

		val = (int16_t)(pdata[2] & 0x7F);
		val <<= 8;
		val |= pdata[3];
		if (pdata[2] & 0x80)
			val *= -1;
		t = val/10.0;

		memcpy(data, pdata, 5);
		return 0;
	}

	return -4;
}

int8_t RhtClient::get_temp(uint8_t *dec)
{
	int8_t val = int8_t(t);
	*dec = uint8_t(int16_t(t*10)%10);
	return val;
}

uint8_t RhtClient::get_humidity(uint8_t *dec)
{
	uint8_t val = uint8_t(rh);
	*dec = uint8_t(int16_t(rh*10)%10);
	return val;
}
