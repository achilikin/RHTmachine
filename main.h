#ifndef RHT_MACHINE_HEADER
#define RHT_MACHINE_HEADER

#define ECHO_RHT     0x01 // echo RHT poll data
#define ECHO_THIST   0x02 // echo T history
#define ECHO_EXTRA   0x04 // echo some extra stuff
#define ECHO_VERBOSE 0x08

#define CONFIG_MODE  0x40
#define HIST_INIT    0x80
extern uint8_t  flags;

#define ALT_PIN      0x01 // humidity/light alternative switch
#define DISP_ALT     0x02 // show alternative graph instead of temperature
#define HIST_24H     0x04
#define CD_ATTACHED  0x08 // cd-machine attached
extern uint8_t  pins;

static inline uint8_t get_disp_mode(void) {
	if (pins & DISP_ALT)
		return (1 << (pins & ALT_PIN));
	return 0;
}

#define HIST_SIZE 120
extern uint8_t tday[];
extern uint8_t hday[];

extern uint32_t uptime;
extern uint8_t light;
extern const char ps_version[];
extern const char ps_verstr[];

void disp_hist(void);
void print_hist(uint8_t nrec, uint8_t header);
void print_time(uint32_t time, uint8_t day);
void print_status(uint8_t echo_only);
void set_gauge(uint8_t pwm);
void get_rht_data(char *buf);

#endif