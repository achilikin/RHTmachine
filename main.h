#ifndef RHT_MACHINE_HEADER
#define RHT_MACHINE_HEADER

#define ECHO_RHT     0x01 // echo RHT poll data
#define ECHO_THIST   0x02 // echo T history
#define VERBOSE_MODE 0x04

#define CONFIG_MODE  0x40
#define HIST_INIT    0x80
extern uint8_t  flags;

#define HIST_24H     0x01
#define DISP_RH      0x02 // show humidly graph instead of temperature
#define DBG_PIN      0x04 // reserved
#define CD_ATTACHED  0x08 // cd-machine attached
extern uint8_t  pins;

#define HIST_SIZE 120

extern uint8_t tday[];
extern uint8_t hday[];

extern uint32_t uptime;

void disp_hist(void);
void print_hist(void);
void print_time(uint32_t time);
void print_status(uint8_t echo_only);
void set_gauge(uint8_t pwm);

#endif