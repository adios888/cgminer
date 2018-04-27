#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include "system_detect.h"
#include "miner.h"

#define FAN_CHANNEL_COUNT 2
#define FAN_SAMPLE_BURST 100
#define BUFFER_SIZE (4*FAN_SAMPLE_BURST) 

int node_start;

#define GPT1_SYS_CAPT_CH1   "/sys/devices/platform/soc/2000000.aips-bus/2098000.gpt/capture1_data"
#define GPT2_SYS_CAPT_CH1   "/sys/devices/platform/soc/2000000.aips-bus/20e8000.gpt/capture1_data"
typedef struct
{
	int fd;
	int speed;
	char fan_error;
}FAN_STRUCT;

FAN_STRUCT fan0_data, fan1_data;

unsigned char fan_buf[BUFFER_SIZE];

int tempera_val[3];
float input_current_val[3];
float input_volt_val[3];
float output_volt_val[3];


void read_electron(char *buf)
{
	sprintf(buf, "inputCurrent:%0.1f %0.1f %0.1f;inputVolt:%0.1f %0.1f %0.1f;outputVolt:%0.1f %0.1f %0.1f;temper:%d %d %d", \
								input_current_val[0], input_current_val[1],input_current_val[2],\
								input_volt_val[0], input_volt_val[1], input_volt_val[2],\
								output_volt_val[0], output_volt_val[1], output_volt_val[2],\
								tempera_val[0], tempera_val[1], tempera_val[2]\
								);
}

void echo_temperature(void)
{
	int tempera_fd[3];
	//int tempera_val[3];
	char i;
	char buf[8] = {0};
	char node_buf[64] = {0};
	int tmp_value;	

	//while(1)
	{
		sprintf(node_buf, "/sys/class/hwmon/hwmon%d/temp1_input", node_start);
		tempera_fd[0] = open(node_buf ,O_RDONLY);	

		sprintf(node_buf, "/sys/class/hwmon/hwmon%d/temp1_input", node_start+1);
		tempera_fd[1] = open(node_buf ,O_RDONLY);	
		
		sprintf(node_buf, "/sys/class/hwmon/hwmon%d/temp1_input", node_start+2);
		tempera_fd[2] = open(node_buf ,O_RDONLY);	
		
		for (i=0; i<3; i++)
		{
			if (tempera_fd[i]>0)
			{
				if (read(tempera_fd[i], buf, 8) > 0)
				{
					tmp_value =  atoi(buf)/1000;
					
					if (tmp_value > 0)
					{
						tempera_val[i] =  tmp_value;
					}
				}
			}

//			printf("board %d temperature:%d\n",i, tempera_val[i]);

		}

		for (i=0; i<3; i++)
		{
			if (tempera_fd[i]>0)
				close(tempera_fd[i]);	
		}

//		printf("--------------------------------------\n");
//		sleep(2);
	}

}

void echo_current(void)
{
	int input_current_fd[3];
	//float input_current_val[3];

	char i;
	char buf[8] = {0};
	char node_buf[64] = {0};
	float tmp_value;


	//while(1)
	{
		
		sprintf(node_buf, "/sys/class/hwmon/hwmon%d/curr1_input", node_start);
		input_current_fd[0] = open(node_buf ,O_RDONLY);	

		sprintf(node_buf, "/sys/class/hwmon/hwmon%d/curr1_input", node_start+1);
		input_current_fd[1] = open(node_buf ,O_RDONLY);	
		
		sprintf(node_buf, "/sys/class/hwmon/hwmon%d/curr1_input", node_start+2);
		input_current_fd[2] = open(node_buf ,O_RDONLY);	

		for (i=0; i<3; i++)
		{
			if (input_current_fd[i]>0)
			{
				if (read(input_current_fd[i], buf, 8)>0)
				{
					tmp_value  = atoi(buf)/10000.0;
					
					//if (tmp_value>0)
					{
						input_current_val[i] = tmp_value;
					}
				}
			}
			//printf("board %d current:%f, %s\n",i, input_current_val[i], buf);

		}

		for (i= 0; i<3; i++)
		{
			if (input_current_fd[i]>0)
				close(input_current_fd[i]);	
		}

//		sleep(2);
//		printf("-------------------------------\n");
	}

}

void echo_input_volt(void)
{
	int input_volt_fd[3];
	//float input_volt_val[3];


	char i;
	char buf[8] = {0};
	char node_buf[64] = {0};
	float tmp_value;

	//while(1)
	{
		sprintf(node_buf, "/sys/class/hwmon/hwmon%d/in1_input", node_start);
		input_volt_fd[0] = open(node_buf ,O_RDONLY);	

		sprintf(node_buf, "/sys/class/hwmon/hwmon%d/in1_input", node_start+1);
		input_volt_fd[1] = open(node_buf ,O_RDONLY);	
		
		sprintf(node_buf, "/sys/class/hwmon/hwmon%d/in1_input", node_start+2);
		input_volt_fd[2] = open(node_buf ,O_RDONLY);	
		
		for (i=0; i<3; i++)
		{
			if (input_volt_fd[i]>0)
			{
				if (read(input_volt_fd[i], buf, 8)>0)
				{
					tmp_value = atoi(buf)/1000.0;
					
					if (tmp_value)
					{
						input_volt_val[i] = tmp_value;
					}
				}
			}


			//printf("board %d, input_volt:%0.2f\n",i, input_volt_val[i]);

		}

		for (i=0; i<3; i++)
		{
			if (input_volt_fd[i] > 0)	
				close(input_volt_fd[i]);
		}

//		sleep(2);
//		printf("--------------------------\n");
	}

}

void echo_output_volt(void)
{
	int output_volt_fd[3];
	//float output_volt_val[3];

	char i;
	char buf[8] = {0};
	char node_buf[64]={0};
	float tmp_value; 

//	while(1)
	{
		
		sprintf(node_buf, "/sys/class/hwmon/hwmon%d/in2_input", node_start);
		output_volt_fd[0] = open(node_buf ,O_RDONLY);	

		sprintf(node_buf, "/sys/class/hwmon/hwmon%d/in2_input", node_start+1);
		output_volt_fd[1] = open(node_buf ,O_RDONLY);	

		sprintf(node_buf, "/sys/class/hwmon/hwmon%d/in2_input", node_start+2);
		output_volt_fd[2] = open(node_buf ,O_RDONLY);	

		for (i=0; i<3; i++)
		{
			if (output_volt_fd[i] > 0)
			{
				if (read(output_volt_fd[i], buf, 8)>0)
				{
					tmp_value = atoi(buf)/1000.0;
					
					if (tmp_value)
					{
						output_volt_val[i] = tmp_value;
					}
				}
			}
		}


		for (i=0; i<3; i++)
		{
			if (output_volt_fd[i] > 0)	
				close(output_volt_fd[i]);
		}


//		sleep(2);
	}

}

/*
 *tty4->hwmon3
 *tty6->hwmon4
 *tty7->hwmon4
 */
void set_output_volt(int board_id, int volt_value)
{
	char cmd_buf[64] = {0};
	
	if (board_id<1 || board_id>3)
		return ;

	sprintf(cmd_buf, "echo %d > /sys/class/hwmon/hwmon%d/in2_output", volt_value, board_id);	
	printf("%s", cmd_buf);

	system(cmd_buf);

}


unsigned int calc_fanspeed(unsigned int *data)
{
  int i = 0;
  uint value_1 = 0, value_2 = 0, value_pwm = 0;
  unsigned int min_val = 0xfffffff;

  for(i = 0; i < FAN_SAMPLE_BURST; i+=2)
  {
    value_1 = *(data + i);
    value_2 = *(data + i + 1); 
    value_pwm = value_2 - value_1;

	if ((min_val > value_pwm) && (value_pwm !=0))	
		min_val = value_pwm ;
	
#if 0	
	if(value_pwm != 0)
	{
	        printf("HZ:%d, speed:%d\n", value_pwm, (3000000/value_pwm), (22500000/min_val));
	}

	if(value_pwm != 0)
	{
	        printf("GPT1-CH1 :pwm_pre=0x%x,HZ:%d,speed:%d\n", value_pwm, (3000000/value_pwm), 15*(1500000/value_pwm));
	}
#endif 

  }

  return (22500000/min_val); 

}

#if 0
static void fan_sighandler(int sig)
{

	printf("--------fan timeout-----------\n");
	fan0_data.fan_error++;
	fan1_data.fan_error++;
}
#endif 

void fan_detect(FAN_STRUCT* fandata, char* buf)
{
	int ret;

	//alarm(5);
	lseek(fandata->fd, 0, SEEK_SET);
	ret = read(fandata->fd, buf, BUFFER_SIZE);
	//alarm(0);


	if (ret == BUFFER_SIZE)
	{
		fandata->fan_error = 0;
		  fandata->speed = calc_fanspeed((unsigned int *)buf);
	}
	else if(ret == 0)
	{
		fandata->speed = 0;	
		//if (fandata->fan_error)
			fandata->fan_error++;
	}
}

void fan_stop_handle(void)
{
	int i;
	int fd;
    struct tty_chain *d12;
	struct cgpu_info *cgpu;

	system("killall cg.sh");
	system("killall cg.sh");
	system("echo fan stop err > /usr/app/error_log.txt");

    applog(LOG_WARNING,"fan stop error!\n");
    applog(LOG_WARNING,"disable all core!\n");

	for (i = 0; i < total_devices; ++i) 
	{
			cgpu = devices[i];
			d12   = cgpu->device_data;
			fd = d12->ctx->fd;
			disable_core_bystep(fd);
	}
	/*set fan to low speed*/
	system("echo 10000 > /sys/class/pwm/pwmchip6/pwm0/duty_cycle");
	system("echo 10000 > /sys/class/pwm/pwmchip4/pwm0/duty_cycle");

	early_quit(0, "fan stop now quit");
}

void check_fan_status(FAN_STRUCT *fandata)
{
	if (fandata->fan_error > 10)
	{
		fan_stop_handle();
	}
	
}

#if 0
int main(void)
{

	fan0_data.fd = open(Fan_DevName[0], O_RDONLY, 0);
	fan1_data.fd = open(Fan_DevName[1], O_RDONLY, 0);

	if (fan0_data.fd < 0)
	{
		fan0_data.fd = open(Fan_DevName[0], O_RDONLY, 0);
		if (fan0_data.fd < 0)
		{
			fan_stop_handle();		
		}
	}

	if (fan1_data.fd < 0)
	{
		fan1_data.fd = open(Fan_DevName[0], O_RDONLY, 0);
		if (fan1_data.fd < 0)
		{
			fan_stop_handle();		
		}
	}

	struct sigaction handler;
	handler.sa_handler = &fan_sighandler;
	handler.sa_flags = 0;
	sigemptyset(&handler.sa_mask);
	sigaction(SIGALRM, &handler, NULL);

	while(1)
	{
		fan_detect(&fan0_data,fan_buf);
		check_fan_status(&fan0_data);
#if 0
		sleep(1);
		fan_detect(&fan1_data, fan_buf);
		check_fan_status(&fan1_data);
		sleep(1);
#endif		
	}
}
#endif 

/*normally the sensor node point shoulb be hwmon3/4/5 
 * but in some error conditin, it turn to hwmon2/3/4 or some others*/
int node_detect(void)
{
	int i;
	char node_buf[64]={0};
	for (i=0; i<4; i++)	
	{
		//tempera_fd[0] = open("/sys/class/hwmon/hwmon3/temp1_input" ,O_RDONLY);	

		sprintf(node_buf, "/sys/class/hwmon/hwmon%d/in2_input", i);

		if(!access(node_buf, F_OK))
		{
			return i;	
		}
	}

	return 3;
}

void *temperature_thread(void *args)
{
	fan0_data.fd = open(GPT1_SYS_CAPT_CH1, O_RDONLY, 0);
	fan1_data.fd = open(GPT2_SYS_CAPT_CH1, O_RDONLY, 0);

	if (fan0_data.fd < 0)
	{
		fan0_data.fd = open(GPT1_SYS_CAPT_CH1, O_RDONLY, 0);
		if (fan0_data.fd < 0)
		{
			fan_stop_handle();		
		}
	}

	if (fan1_data.fd < 0)
	{
		fan1_data.fd = open(GPT2_SYS_CAPT_CH1, O_RDONLY, 0);
		if (fan1_data.fd < 0)
		{
			fan_stop_handle();		
		}
	}

	node_start = node_detect();

#if 0 
	printf("==== exist node number:%d===========\n", node_start);

	struct sigaction handler;
	handler.sa_handler = &fan_sighandler;
	handler.sa_flags = 0;
	sigemptyset(&handler.sa_mask);
	sigaction(SIGALRM, &handler, NULL);
#endif

	pthread_detach(pthread_self());
	sleep(1);

	while(1)
	{
		echo_temperature();		
		echo_current();
		echo_input_volt();
		echo_output_volt();
		sleep(10);
#if 0		
		fan_detect(&fan0_data, fan_buf);
		check_fan_status(&fan0_data);
		sleep(9);
		fan_detect(&fan1_data, fan_buf);
		check_fan_status(&fan1_data);
		sleep(8);
		printf("speed:%d, %d\n", fan0_data.speed, fan1_data.speed);
#endif 
	}
}

#if 0
void main(void)
{
			
	echo_temperature();
	echo_current();
	echo_input_volt();
	echo_output_volt();

}
#endif
