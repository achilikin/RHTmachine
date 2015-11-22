/* Command line parser for data node
  
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
#include <Arduino.h>

#include "main.h"
#include "ds3231.h"
#include "serial.h"
#include "ossd_i2c.h"
#include "serial_cli.h"

// list of supported commands 
static const char ps_help[] PROGMEM =
	"  mem\n"             // show available memory
	"  status\n"          // show current settings
	"  version\n"         // show version info
	"  print now\n"       // print current data
	"  print log [n]\n"   // print data history (12/24h), n records
	"  config on|off\n"   // turn on configuration mode
	"  set gauge 0-255\n" // calibrate gauge in configuration mode
	"  set trigger 1|0\n" // turn trigger on/off
	"  set contrast 0-255\n"
	"  set time hh:mm:ss\n"
	"  echo rht|thist|extra|verbose on|off"; // debug echo on/off
static const char ps_on[] PROGMEM = "on";
static const char ps_off[] PROGMEM = "off";
static const char ps_config[] PROGMEM = "config";
const char ps_version[] PROGMEM = "version";
const char ps_sensors[] PROGMEM = "%s L %u\n";
const char ps_time[] PROGMEM = "%02u:%02u:%02u ";
const char ps_verstr[] PROGMEM = "2015-11-22, 22,266/1339 (72/65%) bytes";

static int8_t set_rtc_time(char *str);

const char *is_on(uint8_t bit)
{
	if (bit)
		return "on";
	return "off";
}

void print_status(uint8_t echo_only)
{
	static const char *disp[3] = { PSTR("temperature"), PSTR("humidity"), PSTR("light") };

	if (!echo_only) {
		printf_P(PSTR("Uptime    : "));
		print_time(uptime, 1);
		printf_P(PSTR("\nRTC time  : "));
		print_time(rtctime, 0);
		printf_P(PSTR("\nmachine   : "));
		printf_P((pins & CD_ATTACHED) ? PSTR("at") : PSTR("de"));
		printf_P(PSTR("tached\n"));
		printf_P(PSTR("display   : "));
		printf_P(disp[get_disp_mode()]);
		printf_P(PSTR("\nalt mode  : "));
		printf_P(disp[1 + (pins & ALT_PIN)]);
		printf_P(PSTR("\n24 hour   : %s\n"), is_on(pins & HIST_24H));
		printf_P(PSTR("config mod: %s\n"), is_on(flags & CONFIG_MODE));
	}
	printf_P(PSTR("verbose   : %s\n"), is_on(flags & ECHO_VERBOSE));
	printf_P(PSTR("extra echo: %s\n"), is_on(flags & ECHO_EXTRA));
	printf_P(PSTR("thist echo: %s\n"), is_on(flags & ECHO_THIST));
	printf_P(PSTR("rht echo  : %s\n"), is_on(flags & ECHO_RHT));
}

int8_t cli_proc(char *buf, void *ptr)
{
	char *arg, *val;
	char cmd[CMD_LEN + 1];

	memcpy(cmd, buf, sizeof(cmd));
	arg = get_arg(cmd);
	val = get_arg(arg);

	if (str_is(cmd, PSTR("help"))) {
		puts_P(ps_help);
		return 0;
	}

	if (str_is(cmd, ps_version)) {
		puts_P(ps_verstr);
		return 0;
	}

	if (str_is(cmd, ps_config)) {
		if (str_is(arg, ps_on))
			flags |= CONFIG_MODE;
		else if (str_is(arg, ps_off))
			flags &= ~CONFIG_MODE;
		printf_P(ps_config);
		printf_P(PSTR(" is %s\n"), is_on(flags & CONFIG_MODE));
		return 0;
	}

	if (str_is(cmd, PSTR("status"))) {
		print_status(0);
		return 0;
	}

	if (str_is(cmd, PSTR("print"))) {
		if (str_is(arg, PSTR("log")))
			print_hist(atoi(val), 1);
		else if (str_is(arg, PSTR("now"))) {
			uint8_t h, m, s;
			if (ds3231_get_time(&h, &m, &s) == 0)
				printf_P(ps_time, h, m, s);
			char buf[24];
			get_rht_data(buf);
			printf_P(ps_sensors, buf, light);
		}
		else
			return CLI_EARG;
		return 0;
	}

	if (str_is(cmd, PSTR("set"))) {
		if (str_is(arg, PSTR("gauge"))) {
			if (flags & CONFIG_MODE) {
				uint8_t pwm = atoi(val);
				set_gauge(pwm);
			}
		} else if (str_is(arg, PSTR("contrast"))) {
			uint8_t contrast = atoi(val);
			ossd_set_contrast(contrast);
		} else if (str_is(arg, PSTR("time")))
			return set_rtc_time(val);
		else if (str_is(arg, PSTR("trigger")))
			set_trigger(atoi(val));
		else
			return CLI_EARG;
		return 0;
	}

	if (str_is(cmd, PSTR("echo"))) {
		uint8_t set = 0;
		if (*arg == '\0')
			goto print_stat;
		if (str_is(arg, ps_off)) {
			flags &= ~(ECHO_RHT | ECHO_THIST | ECHO_EXTRA | ECHO_VERBOSE);
			goto print_stat;
		}
		if (str_is(arg, PSTR("rht")))
			set = ECHO_RHT;
		if (str_is(arg, PSTR("thist")))
			set = ECHO_THIST;
		if (str_is(arg, PSTR("extra")))
			set = ECHO_EXTRA;
		if (str_is(arg, PSTR("verbose")))
			set = ECHO_VERBOSE;
		if (!set)
			return CLI_EARG;
		if (str_is(val, ps_on))
			flags |= set;
		else if (str_is(val, ps_off))
			flags &= ~set;
		else
			return CLI_EARG;
print_stat:
		print_status(1);
		return 0;
	}

	if (str_is(cmd, PSTR("mem"))) {
		printf_P(PSTR("memory %u\n"), free_mem());
		return 0;
	}
	return CLI_ENOTSUP;
}

int8_t set_rtc_time(char *str)
{
	uint8_t h, m, s;
	
	h = (uint8_t)strtoul(str, &str, 10);
	if (h > 24 || *str != ':')
		return CLI_EARG;
	m = (uint8_t)strtoul(++str, &str, 10);
	if (m > 59 || *str != ':')
		return CLI_EARG;
	s = (uint8_t)strtoul(++str, &str, 10);
	if (s > 59)
		return CLI_EARG;
	return ds3231_set_time(h, m, s);
}
