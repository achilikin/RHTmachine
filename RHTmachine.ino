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
#include "ds3231.h"
#include "serial.h"
#include "ossd_i2c.h"
#include "i2cmaster.h"
#include "serial_cli.h"
#include "rht_client.h"

static const uint8_t d_attached = 12;

static const uint8_t d_dir  = 4;
static const uint8_t d_eot  = 6;
static const uint8_t d_step = 2;
static const uint8_t d_trigger = 7; // connected to a pull down npn

static const uint8_t d_data = 3;
static const uint8_t d_gauge = 5;
static const uint8_t d_busy = 8;

static const uint8_t led_red = 9;
static const uint8_t led_blue = 11;
static const uint8_t led_green = 10;

static const uint8_t led_rht = 13;

static const uint8_t d_alt = A0;
static const uint8_t d_hist_12 = A1;
static const uint8_t disp_t = A2;
static const uint8_t a_light = A7;

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

static int8_t led_dimmer(void *data);
ticker_t tick_led(50, led_dimmer);

static int8_t second(void *data);
ticker_t tick_sec(TICK_SEC(1), second); // 1 second ticker

static int8_t calibrate(void *data);
ticker_t tick_fdd(TICK_HOUR(12), calibrate);

// four polls per minute
static int8_t rht_poll(void);

uint8_t light;
RhtClient rht(d_data, 2);
AnalogueGauge gauge(d_gauge, hmap, sizeof(hmap)/sizeof(hmap[0]));

#define FDD_RANGE 135 // number of steps for temperature needle
FddController fdd(d_dir, d_eot, d_step, FDD_RANGE);

// temperature for averaging
#define TTREND_LEN 40
uint32_t ttrendidx;
float ttrend[TTREND_LEN];
static int8_t get_ttrend(float delta, uint8_t dbg);

uint32_t uptime; 
uint32_t rtctime; // real-time clock time (if DS3231 is connected)

static int8_t rtc_sync(void);

// rht poll stats
uint32_t estat[ESTAT_SIZE];

uint8_t pins;
uint8_t flags;

#define HAVR_SIZE 24
#define POLL_INTERVAL 15
#define HIST_INTERVAL (int16_t(HAVR_SIZE) * int16_t(POLL_INTERVAL))

uint8_t avridx;  // averaging index
uint8_t avrsize;
// averaging arrays
uint8_t tavr[HAVR_SIZE];
uint8_t havr[HAVR_SIZE];
uint8_t lavr[HAVR_SIZE];

uint8_t trange[2] = {  0, FDD_RANGE };
uint8_t hrange[2] = {  0, 255 };
uint8_t lrange[2] = {  0, 127 };

#define HIST_SIZE     120
int16_t hinterval;
uint8_t tday[HIST_SIZE]; // last 12 (default) or 24 hour history
uint8_t hday[HIST_SIZE];
uint8_t lday[HIST_SIZE];

#define INIT_EEPROM 0
uint8_t EEMEM em_tday[HIST_SIZE];
uint8_t EEMEM em_hday[HIST_SIZE];
uint8_t EEMEM em_lday[HIST_SIZE];
uint8_t EEMEM em_estat[sizeof(uint32_t)*ESTAT_SIZE];

// setup all global variables
void setup()
{
	led.attach(led_rht);
	led.on();

	serial_init(UART_BAUD_RATE);
	puts_P(ps_version);
	puts_P(ps_verstr);

#if INIT_EEPROM
	memset(tday, 0, HIST_SIZE);
	eeprom_write_block(tday, em_estat, sizeof(estat));
	eeprom_write_block(tday, em_tday, HIST_SIZE);
	eeprom_write_block(tday, em_hday, HIST_SIZE);
	eeprom_write_block(tday, em_lday, HIST_SIZE);
#endif

	eeprom_read_block((void *)estat, (const void *)em_estat, sizeof(estat));
	eeprom_read_block((void *)tday, (const void *)em_tday, HIST_SIZE);
	eeprom_read_block((void *)hday, (const void *)em_hday, HIST_SIZE);
	eeprom_read_block((void *)lday, (const void *)em_lday, HIST_SIZE);

	// PWM outputs for RGB LED
	pinMode(led_red, OUTPUT);
	pinMode(led_green, OUTPUT);
	pinMode(led_blue, OUTPUT);
	// RHT busy
	pinMode(d_busy, INPUT);

	// configuration pins
	pinMode(d_alt, INPUT_PULLUP);
	pinMode(disp_t, INPUT_PULLUP);
	pinMode(d_hist_12, INPUT_PULLUP);
	pinMode(d_attached, INPUT_PULLUP);
	analogRead(a_light);

	uptime = 0;
	rtctime = 0;
	pins = flags = 0;
	if (digitalRead(d_attached) == LOW)
		pins |= CD_ATTACHED;
	hinterval = HIST_INTERVAL;
	if (digitalRead(d_hist_12) == LOW) {
		pins |= HIST_24H;
		hinterval *= 2;
	}

	printf_P(PSTR("Running at %luHz"), F_CPU);
	i2c_init();
	delay(2);
	rtc_sync();

	if (!(pins & CD_ATTACHED))
		printf_P(PSTR(" in standalone mode"));
	else {
		if (ossd_init(OSSD_NORMAL) == 0) {
			bmfont_select(BMFONT_8x8);
			ossd_putlx(0, -1, "RHT Machine", 0);
			bmfont_select(BMFONT_8x16);
		}
	}
	printf("\n");

	rht.begin();
	fdd.begin(18.5, 25.25);
	gauge.begin(0.0, 100.0);

	avridx = avrsize = 0;
	ttrendidx = 0;

	// trigger output
	set_trigger(0);
	// default values
	digitalWrite(led_red, LOW);
	digitalWrite(led_green, LOW);
	digitalWrite(led_blue, LOW);

	// initialize console and print command prompt
	print_status(0);
	cli_init();
	led.off();
}

// main loop, never returns
void loop()
{
	uint8_t disp = ~(pins & DISP_ALT);
	if (pins & CD_ATTACHED) {
		// seek to 0
		fdd.init();
		// set 20 C for scale calibration
		fdd.set(20.0);
		delay(5000);
		rht_poll();
	}

	uint32_t tms = millis();
	tick_sec.tick(tms, NULL);
	uptime = tms / 1000l;

	while(true) {
		// pullup inputs
		if (!digitalRead(d_alt))
			pins |= ALT_PIN;
		if (!digitalRead(disp_t))
			pins |= DISP_ALT;
		if (disp ^ (pins & DISP_ALT)) {
			disp_hist();
			disp = pins & DISP_ALT;
		}
		cli_interact(cli_proc, NULL);
		tms = millis();
		tick_led.tick(tms, NULL);
		if (tick_sec.tick(tms, NULL)) {
			if (!(flags & CONFIG_MODE) && !(rtctime % POLL_INTERVAL))
				rht_poll();
			uint8_t sync = rtctime % 60l;
			if (sync == 5 || sync == 35)
				rtc_sync();
		}
		if (!(flags & CONFIG_MODE))
			tick_fdd.tick(tms, NULL);
		pins &= ~(DISP_ALT | ALT_PIN);
		delay(10);
	}
}

int8_t rht_poll(void)
{
	int8_t error = 0;
	char disp[24];
	static uint8_t tick = 0;
	const uint8_t verbose = flags & ECHO_VERBOSE;
	const uint8_t attached = pins & CD_ATTACHED;

	if (verbose)
		print_time(rtctime, 0);
	led.on();
	if (attached) {
		while(digitalRead(d_busy))
			delay(1);
		error = rht.poll(flags & ECHO_RHT);
	}
	led.off();

	if (!error) {
		if (!(flags & HIST_INIT)) {
			for(uint8_t i = 0; i < TTREND_LEN; i++)
				ttrend[i] = rht.get_temp();
			flags |= HIST_INIT;
		}
	}
	else if (verbose)
		printf_P(PSTR("Error %d, "), error);

	error = (-1 * error);
	if (error < ESTAT_SIZE)
		estat[error] += 1;
	// store statistic in the EEPROM
	uint8_t *pstat = (uint8_t *)&estat[0];
	for(uint8_t i = 0; i < sizeof(estat); i++)
		eeprom_update_byte(&em_estat[i], pstat[i]);

	get_rht_data(disp);
	if (attached)
		ossd_putlx(0, -1, disp, TEXT_UNDERLINE);

	light = uint8_t(analogRead(a_light) >> 2);
	if (verbose)
		printf_P(ps_sensors, disp, light);

	// store to history
	uint8_t fpos = fdd.set(rht.get_temp());
	uint8_t gpos = gauge.set(rht.get_humidity());
	tavr[avridx] = fpos;
	havr[avridx] = gpos;
	lavr[avridx] = light;
	if (avrsize < HAVR_SIZE)
		avrsize++;
	// for 24 hour mode store only every second reading
	tick++;
	if (pins & HIST_24H)
		avridx += tick & 0x01;
	else
		avridx++;
	avridx %= HAVR_SIZE;

	if (!(rtctime % hinterval)) {
		uint16_t avrt = 0;
		uint16_t avrh = 0;
		uint16_t avrl = 0;
		for (uint8_t i = 0; i < avrsize; i++) {
			avrt += tavr[i];
			avrh += havr[i];
			avrl += lavr[i];
		}
		uint8_t tdidx = uint8_t(uint32_t(rtctime / hinterval)) % HIST_SIZE;
		tday[tdidx] = avrt / avrsize;
		hday[tdidx] = avrh / avrsize;
		lday[tdidx] = avrl / avrsize;
		eeprom_update_byte(&em_tday[tdidx], tday[tdidx]);
		eeprom_update_byte(&em_hday[tdidx], hday[tdidx]);
		eeprom_update_byte(&em_lday[tdidx], lday[tdidx]);
		disp_hist();
		if (verbose)
			print_hist(1, 0);
	}

	// store for averaging
	ttrend[ttrendidx++] = rht.get_temp();
	ttrendidx = ttrendidx % TTREND_LEN;

	// get temperature gradient
	static const char *tdir[3] = { PSTR(" (falling)"), PSTR(" (stable)"), PSTR(" (rising)") };
	int8_t tgrad = get_ttrend(0.25, flags & ECHO_THIST);
	tgrad += 1; // translate -1, 0, 1 to 0,1,2
	if (rgb_led != tgrad) {
		digitalWrite(rgb[rgb_led], LOW);
		rgb_led = tgrad;
	}
	analogWrite(rgb[rgb_led], rgb_val);
	if (flags & ECHO_THIST)
		puts_P(tdir[rgb_led]);

	return error;
}

int8_t calibrate(void *data)
{
	static uint8_t count = 0;

	// do the first calibration after 12 hours
	// and then every 24 hours
	if (++count & 0x01) {
		print_time(rtctime, 0);
		puts_P(PSTR("calibrating"));
		fdd.init();
		fdd.set(rht.get_temp());
	}

	return 0;
}

// -1 - falling
//  0 - same
//  1 - rising
int8_t get_ttrend(double delta, uint8_t dbg)
{
	double val[2];

	// accumulate temperature for the first half of the history
	// (current time - TAVR_LEN/2)
	val[0] = 0.0;
	for(uint8_t i = 0; i < TTREND_LEN/2; i++) {
		uint8_t idx = (ttrendidx + i) % TTREND_LEN;
		val[0] += ttrend[idx];
		if (dbg) {
			Serial.print(ttrend[idx]);
			Serial.print(" ");
		}
	}
	val[0] = val[0]/(TTREND_LEN/2);
	if (dbg) {
		Serial.print("(");
		Serial.print(val[0]);
		Serial.println(")");
	}

	// accumulate temperature for the second half of the history
	// (TAVR_LEN/2 to the current time)
	val[1] = 0.0;
	for(uint8_t i = TTREND_LEN/2; i < TTREND_LEN; i++) {
		uint8_t idx = (ttrendidx + i) % TTREND_LEN;
		val[1] += ttrend[idx];
		if (dbg) {
			Serial.print(ttrend[idx]);
			Serial.print(" ");
		}
	}
	val[1] = val[1]/(TTREND_LEN/2);
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
//  red   - temperature rising
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
	rtctime = (rtctime + 1) % SEC_DAY;
#if 0
	uint8_t h, m, s;
	uint8_t Y, M, D;
	int8_t  tval;
	uint8_t tdec;
	if (ds3231_get_time(&h, &m, &s) == 0) {
		ds3231_get_date(&Y, &M, &D);
		ds3231_get_temperature(&tval, &tdec);
		uint8_t l = (analogRead(a_light) >> 2);
		printf_P(PSTR("%u/%02u/%02u %02u:%02u:%02u %3u %d.%u\n"),
			Y, M, D, h, m, s, l, tval, tdec);
#endif
	return 1;
}

// calculate 48 bar graph for display
static void set_bar(uint8_t *bar, uint8_t val, uint8_t range)
{
	uint8_t n, nbits = uint8_t(48.*(float(val)/(float(range))));

	if (val && (flags & ECHO_EXTRA))
		printf_P(PSTR("set_bar %u %u:"), val, nbits);

	for (n = 0; nbits > 8; n++, nbits -= 8)
		bar[n] = 0xFF;
	if (n < 6) {
		val = 0xFF << (8 - nbits);
		bar[n++] = val;
		for (; n < 6; n++)
			bar[n] = 0;
	}

	if (val && (flags & ECHO_EXTRA)) {
		for (n = 0; n < 6; n++)
			printf(" %02X", bar[n]);
		printf("\n");
	}
}

static inline uint8_t normilize(uint8_t val, const uint8_t *range)
{
	val = constrain(val, range[0], range[1]);
	return val - range[0];
}

void disp_hist(void)
{
	if (!(pins & CD_ATTACHED))
		return;
	uint8_t mode = get_disp_mode();
	static const char *disp[3] = { PSTR("Temp C"), PSTR("Humid%"), PSTR("Light%") };
	static const uint8_t *dhist[3] = { tday, hday, lday };
	static const uint8_t *drange[3] = { trange, hrange, lrange };

	bmfont_select(BMFONT_8x8);
	for (uint8_t i = 0; i < 6; i++)
		ossd_putcx(2 + i, 0, pgm_read_byte(disp[mode] + i), 0);
	bmfont_select(BMFONT_8x16);

	uint8_t bar[6];
	uint8_t range = drange[mode][1] - drange[mode][0];
	const uint8_t *day = dhist[mode];
	uint8_t idx = uint8_t(uint32_t(rtctime / hinterval)) % HIST_SIZE;
	for (uint8_t i = 0; i < HIST_SIZE; i++) {
		uint8_t val = normilize(day[idx], drange[mode]);
		set_bar(bar, val, range);
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

void print_time(uint32_t sec, uint8_t day)
{
	uint32_t d = (sec / SEC_DAY);
	uint8_t  h = (sec / SEC_HOUR) % 24;
	uint8_t  m = (sec / 60) % 60;
	uint8_t  s = sec % 60;
	if (day && d)
		printf_P(PSTR("%lu days "), d);
	printf_P(ps_time, h, m, s);
}

void reset_hist(void)
{
	memset(tday, 0, HIST_SIZE);
	memset(hday, 0, HIST_SIZE);
	memset(lday, 0, HIST_SIZE);
}

void print_hist(uint8_t nrec, uint8_t header)
{
	int32_t time = (rtctime / hinterval);
	uint8_t idx = uint8_t(time) % HIST_SIZE;
	time *= hinterval;
	if (!nrec || (nrec > HIST_SIZE))
		nrec = HIST_SIZE;
	if (header)
		printf_P(PSTR("    Time     T    H  L\n"));
	for (uint8_t i = 0; i < nrec; i++) {
		uint8_t tdec, hdec;
		uint8_t tval = fdd.get(tday[idx], &tdec);
		uint8_t hval = gauge.get(hday[idx], &hdec);
		print_time(time, 0);
		if (hday[idx] != 0) // 0 is invalid
			printf_P(PSTR(" %2u.%u %2u.%u %u"), tval, tdec, hval, hdec, lday[idx]);
		puts("");
		time -= hinterval;
		if (time < 0)
			time += SEC_DAY;
		if (idx > 0)
			idx--;
		else
			idx = HIST_SIZE - 1;
	}
}

void set_gauge(uint8_t pwm)
{
	gauge.set(pwm);
}

void get_rht_data(char *buf)
{
	uint8_t tdec, hdec;
	int8_t  tval = rht.get_temp(&tdec);
	uint8_t hval = rht.get_humidity(&hdec);
	sprintf_P(buf, PSTR("T %u.%u H %d.%u"), tval, tdec, hval, hdec);
}

int8_t rtc_sync(void)
{
	uint8_t h, m, s;
	if (ds3231_get_time(&h, &m, &s))
		return -1;
	rtctime = h * SEC_HOUR + m * 60l + s;
	return 0;
}

void set_trigger(uint8_t on)
{
	if (on) {
		pinMode(d_trigger, OUTPUT);
		digitalWrite(d_trigger, HIGH);
		return;
	}
	pinMode(d_trigger, INPUT);
}
