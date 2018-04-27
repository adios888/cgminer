#ifndef SYSTEM_DETECTH_H_
#define SYSTEM_DETECTH_H_

#define TEMPERATURE_CHANNEL_COUNT 8
#define TEMPERATURE_LEVEL_COUNT 25

//
//index value should be:5,2,3,4,6,7 
//dev/ttymxc5:   5
//dev/ttymxc2:   2
//dev/ttymxc3:   3
//dev/ttymxc4:   4
extern int tempera_val[3];
extern float input_current_val[3];
extern float input_volt_val[3];
extern float output_volt_val[3];

int open_temperature(int index);
int close_temperature(int fd);
int get_temperature(int fd);
void *temperature_thread(void *userdata);


#endif
