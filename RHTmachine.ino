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
#include <led.h>
#include <ticker.h>

#include "fdd.h"
#include "main.h"
#include "gauge.h"
#include "bmfont.h"
#include "serial.h"
#include "ossd_i2c.h"
#include "i2cmaster.h"
#include "serial_cli.h"
#include "rht_client.h"

static const uint8_t d_attached = 12;

static const uint8_t d_dir  = 4;
static const uint8_t d_eot  = 6;
static const uint8_t d_step = 2;
static const uint8_t trigger = 7;

static const uint8_t d_data = 3;
static const uint8_t d_gauge = 5;

static const uint8_t led_red = 9;
static const uint8_t led_blue = 11;
static const uint8_t led_green = 10;

static const uint8_t led_rht = 13;

static const uint8_t d_dbg = A0;
static const uint8_t d_hist_12 = A1;
static const uint8_t disp_t = A2;

#define TVAL_MIN 5
#define TVAL_MAX 15

led_t led; // debug led, using pin 13

int8_t rgb_led = 0;
int8_t rgb_val = 10;
uint8_t rgb[3] = { led_blue, led_green, led_red };

/* RH gauge calibration map */
static const uint8_t hmap[] = {
	0,  6,
	10, 22,
	20, 40,
	30, 59,
	40, 82,
	50, 107,
	60, 132,
	70, 153,
	80, 177,
	90, 209,
	100, 253
};

int8_t led_dimmer(void *data);
ticker_t tick_led(50, led_dimmer);

int8_t second(void *data);
ticker_t tick_sec(TICK_SEC(1), second); // 1 second ticker

int8_t rht_poll(void *data);
ticker_t tick_rht(TICK_SEC(15), rht_poll); // four polls per minute

int8_t calibrate(void *data);
ticker_t tick_fdd(TICK_HOUR(12), calibrate);

RhtClient rht(d_data, 1);
AnalogueGauge gauge(d_gauge, hmap, sizeof(hmap)/sizeof(hmap[0]));

#define FDD_RANGE 135 // number of steps for temperature needle
FddController fdd(d_dir, d_eot, d_step, FDD_RANGE);

// temperature for averaging
#define TAVR_LEN 40
uint32_t tidx;
float tavr[TAVR_LEN];

static int8_t get_tavr(float delta, uint8_t dbg);

uint32_t uptime;

uint8_t pins;
uint8_t flags;

uint8_t t6idx;  // last 6 min index
uint8_t t6[24]; // last 6 min of temp
uint8_t h6[24]; // last 6 min of humidity
uint8_t tdidx;
uint8_t tday[HIST_SIZE]; // last 12 (default) or 24 hour history
uint8_t hday[HIST_SIZE];

// setup all global variables
void setup()
{
	led.attach(led_rht);
	led.on();

	serial_init(UART_BAUD_RATE);
	// configuration pins
	pinMode(d_dbg, INPUT_PULLUP);
	pinMode(disp_t, INPUT_PULLUP);
	pinMode(d_hist_12, INPUT_PULLUP);
	pinMode(d_attached, INPUT_PULLUP);

	uptime = 0;
	pins = flags = 0;
	if (digitalRead(d_attached) == LOW)
		pins |= CD_ATTACHED;
	printf_P(PSTR("Starting at %luHz"), F_CPU);
	if (!(pins & CD_ATTACHED))
		printf_P(PSTR(" in standalone mode"));
	printf("\n");
	printf_P(PSTR("Timers: rht %lu, led %lu fdd %lu\n"),
		tick_rht.get_interval(), tick_led.get_interval(), tick_fdd.get_interval());

	i2c_init();
	delay(2);
	if (ossd_init(OSSD_NORMAL) == 0) {
		bmfont_select(BMFONT_8x8);
		ossd_putlx(0, -1, "RHT Machine", 0);
		bmfont_select(BMFONT_8x16);
	}

	rht.begin();
	fdd.begin(18.5, 25.25);
	gauge.begin(0.0, 100.0);

	tidx = 0;
	t6idx = 0;
	tdidx = 0;
	for (uint8_t i = 0; i < 120; i++) {
		tday[i] = 0;
		hday[i] = 0;
	}

	// PWM outputs for RGB LED
	pinMode(led_red, OUTPUT);
	pinMode(led_green, OUTPUT);
	pinMode(led_blue, OUTPUT);
	// trigger output
	pinMode(trigger, OUTPUT);
	// default values
	digitalWrite(led_red, LOW);
	digitalWrite(led_green, LOW);
	digitalWrite(led_blue, LOW);
	digitalWrite(trigger, LOW);

	// initialize console and print command prompt
	print_status(0);
	cli_init();
	led.off();
}

// main loop, never returns
void loop()
{
	uint8_t disp = ~(pins & DISP_RH);
	if (pins & CD_ATTACHED) {
		// seek to 0
		fdd.init();
		// set 20 C for scale calibration
		fdd.set(20.0);
		delay(2000);
		rht_poll(&rht);
	}

	uint32_t tms = millis();
	tick_sec.tick(tms, NULL);
	uptime = tms / 1000l;
	while(true) {
		if (!digitalRead(d_dbg))
			pins |= DBG_PIN;
		if (!digitalRead(d_hist_12))
			pins |= HIST_24H;
		if (!digitalRead(disp_t))
			pins |= DISP_RH;
		if (disp ^ (pins & DISP_RH)) {
			disp_hist();
			disp = pins & DISP_RH;
		}
		cli_interact(cli_proc, NULL);
		tms = millis();
		tick_led.tick(tms, NULL);
		if (!(flags & CONFIG_MODE)) {
			tick_rht.tick(tms, &rht);
			tick_fdd.tick(tms, &rht);
		}
		tick_sec.tick(tms, NULL);
		pins &= ~(HIST_24H | DISP_RH | DBG_PIN);
		delay(10);
	}
}

int8_t rht_poll(void *data)
{
	int8_t ok = 0;
	char disp[24];
	static uint8_t tick = 0;
	const uint8_t verbose = flags & VERBOSE_MODE;
	const uint8_t attached = pins & CD_ATTACHED;
	RhtClient *prht = (RhtClient *)data;

	if (verbose)
		print_time(uptime);
	led.on();
	if (attached)
		ok = prht->poll(flags & ECHO_RHT);
	led.off();
	if (ok == 0) {
		if (!(flags & HIST_INIT)) {
			for(uint8_t i = 0; i < TAVR_LEN; i++)
				tavr[i] = prht->get_temp();
			flags |= HIST_INIT;
		}
	}
	else if (verbose)
		printf_P(PSTR("Error %d, "), ok);

	uint8_t hdec;
	uint8_t hval = prht->get_humidity(&hdec);
	uint8_t tdec;
	int8_t  tval = prht->get_temp(&tdec);
	sprintf(disp, "h %u.%u t %d.%u", hval, hdec, tval, tdec);
	if (attached)
		ossd_putlx(0, -1, disp, TEXT_UNDERLINE);
	if (verbose)
		printf("%s\n", disp);

	// store to history
	uint8_t fpos = fdd.set(prht->get_temp());
	uint8_t gpos = gauge.set(prht->get_humidity());
	t6[t6idx] = fpos;
	h6[t6idx] = gpos;
	// for 24 hour mode store only every second reading
	tick++;
	if (pins & HIST_24H)
		t6idx += tick & 0x01;
	else
		t6idx++;
	if (t6idx == 24) {
		t6idx = 0;
		uint16_t tavr = 0;
		uint16_t havr = 0;
		for (uint8_t i = 0; i < 24; i++) {
			tavr += t6[i];
			havr += h6[i];
		}
		tdidx = (tdidx + 1) % HIST_SIZE;
		tday[tdidx] = tavr / 24;
		hday[tdidx] = havr / 24;
		if (attached)
			disp_hist();
	}

	// store for averaging
	tavr[tidx++] = prht->get_temp();
	tidx = tidx % TAVR_LEN;

	// get temperature gradient
	const char *tdir[3] = { PSTR(" (falling)"), PSTR(" (stable)"), PSTR(" (rising)") };
	int8_t tgrad = get_tavr(0.25, flags & ECHO_THIST);
	tgrad += 1; // translate -1, 0, 1 to 0,1,2
	if (rgb_led != tgrad) {
		digitalWrite(rgb[rgb_led], LOW);
		rgb_led = tgrad;
	}
	analogWrite(rgb[rgb_led], rgb_val);
	if (flags & ECHO_THIST)
		puts_P(tdir[rgb_led]);
	return ok;
}

int8_t calibrate(void *data)
{
	RhtClient *prht = (RhtClient *)data;
	static uint8_t count = 0;

	// do the first calibration after 12 hours
	// and then every 24 hours
	if (++count & 0x01) {
		print_time(uptime);
		puts_P(PSTR("calibrating"));
		fdd.init();
		fdd.set(prht->get_temp());
	}

	return 0;
}

// -1 - falling
//  0 - same
//  1 - rising
int8_t get_tavr(double delta, uint8_t dbg)
{
	double val[2];

	// accumulate temperature for the first half of the history
	// (current time - TAVR_LEN/2)
	val[0] = 0.0;
	for(uint8_t i = 0; i < TAVR_LEN/2; i++) {
		uint8_t idx = (tidx + i) % TAVR_LEN;
		val[0] += tavr[idx];
		if (dbg) {
			Serial.print(tavr[idx]);
			Serial.print(" ");
		}
	}
	val[0] = val[0]/(TAVR_LEN/2);
	if (dbg)
		Serial.println(val[0]);

	// accumulate temperature for the second half of the history
	// (TAVR_LEN/2 to the current time)
	val[1] = 0.0;
	for(uint8_t i = TAVR_LEN/2; i < TAVR_LEN; i++) {
		uint8_t idx = (tidx + i) % TAVR_LEN;
		val[1] += tavr[idx];
		if (dbg) {
			Serial.print(tavr[idx]);
			Serial.print(" ");
		}
	}
	val[1] = val[1]/(TAVR_LEN/2);
	if (dbg) {
		Serial.print("(");
		Serial.print(val[1]);
		Serial.println(")");
		Serial.print(F("temperature "));
		Serial.print(val[0]);
		Serial.print("->");
		Serial.print(val[1]);
	}
	if ((val[1] - val[0]) > delta)
		return 1;
	if ((val[0] - val[1]) > delta)
		return -1;

	return 0;
}

// RGB pulse for temperature gradient:
//  green - temperature stable
//  blue  - temperature falling
//  red   - temperature raising
int8_t led_dimmer(void *data)
{
	static int8_t dir = 1;

	rgb_val += dir;
	if (rgb_val == TVAL_MAX)
		dir = -1;
	if (rgb_val == TVAL_MIN)
		dir = 1;
	analogWrite(rgb[rgb_led], rgb_val);

	return 0;
}

// one second timer to track uptime
int8_t second(void *data)
{
	uptime++;
	return 0;
}

// calculate 48 bar graph for display
static void set_bar(uint8_t *bar, uint8_t val, uint8_t range)
{
	uint8_t n, nbits = uint8_t(48.*(float(val)/(float(range))));

	if (val && (flags & ECHO_THIST))
		printf_P(PSTR("set_bar %u %u:"), val, nbits);

	for (n = 0; nbits > 8; n++, nbits -= 8)
		bar[n] = 0xFF;
	if (n < 6) {
		val = 0xFF << (8 - nbits);
		bar[n++] = val;
		for (; n < 6; n++)
			bar[n] = 0;
	}

	if (val && (flags & ECHO_THIST)) {
		for (n = 0; n < 6; n++)
			printf(" %02X", bar[n]);
		printf("\n");
	}
}

void disp_hist(void)
{
	uint8_t hum = pins & DISP_RH;
	bmfont_select(BMFONT_8x8);
	ossd_putcx(2, 0, hum ? 'h' : 't', 0);
	ossd_putcx(3, 0, hum ? 'u' : 'e', 0);
	ossd_putcx(4, 0, 'm', 0);
	ossd_putcx(5, 0, hum ? 'i' : 'p', 0);
	ossd_putcx(6, 0, hum ? 'd' : ' ', 0);
	ossd_putcx(7, 0, hum ? '%' : 'C', 0);
	bmfont_select(BMFONT_8x16);

	uint8_t bar[6];
	uint8_t *day = hum ? hday : tday;
	uint8_t range = hum ? 255 : FDD_RANGE;
	for (uint8_t idx = tdidx, i = 0; i < HIST_SIZE; i++) {
		set_bar(bar, day[idx], range);
		for (uint8_t n = 0; n < 6; n++) {
			ossd_goto(2 + n, 127 - i);
			ossd_write(bar[5 - n]);
		}
		if (idx > 0)
			idx--;
		else
			idx = HIST_SIZE - 1;
	}
}

void print_hist(void)
{
	printf_P(PSTR("  T   H \n"));
	for (uint8_t idx = tdidx, i = 0; i < HIST_SIZE; i++) {
		if (hday[idx] == 0) // 0 is invalid, so we can use it as end-of-list
			break;

		uint8_t tdec, hdec;
		uint8_t tval = fdd.get(tday[idx], &tdec);
		uint8_t hval = gauge.get(hday[idx], &hdec);
		printf(" %2u.%u %2u.%u\n", tval, tdec, hval, hdec);
		if (idx > 0)
			idx--;
		else
			idx = HIST_SIZE - 1;
	}
}

void print_time(uint32_t sec)
{
	uint32_t d = (sec / 86400l);
	uint8_t  h = (sec / 3600l) % 24;
	uint8_t  m = (sec / 60) % 60;
	uint8_t  s = sec % 60;
	if (d)
		printf_P(PSTR("%lu days "), d);
	printf_P(PSTR("%02u:%02u:%02u "), h, m, s);
}

void set_gauge(uint8_t pwm)
{
	gauge.set(pwm);
}
