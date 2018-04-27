#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <linux/spi/spidev.h>

#include "miner.h"
#include "logging.h"
#include "jdasic.h"
#include "CgConfig.h"

#define TIMEOUT 1000

#define CMD_DETECT_BOARD   0x0001
#define CMD_CONFIG_BTC   0x0002

#define ACK_DILE    0x0000
#define ACK_DETECT_BOARD    0x1000
#define ACK_CONFIG_BTC   0x2000

extern void pabort ( const char *s );

//use for pulling up payload,no nonce return.
unsigned char job3[] = 
{
    0x3C, 0xff, 0x3f, 0x20,
    0x00,0x00,0x00,0x00,
    /*0xFF,0xFF,0x1F,0x1B,*/
    0x96,0x4f,0x0b,0x00,
    0x5D,0x98,0xE2,0xA6,
    0xB4,0x95,0x0E,0xFA,
    0x08,0x28,0xA9,0xA2,
    0xA6,0x81,0x67,0xDD,
    0x82,0x80,0x90,0xEE,
    0x5D,0x23,0x5B,0x7F,
    0x0E,0x40,0xAE,0x38,
    0x84,0x76,0x97,0x48,
    0x26,0x73,0x3A,0x83,
    0x56,0xAB,0x23,0x2B,
    0x18,0x09,0x28,0xF0,
    0xc3
};

//test job,return nonce: F0 FC 1F 8E
unsigned char job2[] = 
{
    0x3C, 0xff, 0x3f, 0x20,
    0xf0, 0xfc, 0x1f, 0x8e, 
    0x96, 0x4f, 0x0b, 0x00,
    0xa6, 0x99, 0xff, 0xdd, 
    0x1b, 0xd8, 0x3e, 0xd6, 
    0x50, 0xa5, 0x10, 0xd1, 
    0x6f, 0xd2, 0x0f, 0x47, 
    0x47, 0x11, 0xd0, 0xf9, 
    0xb8, 0xef, 0xbf, 0xfd, 
    0x0c, 0x69, 0xdf, 0xd6, 
    0x0b, 0x70, 0xbe, 0x26, 
    0x53, 0x6e, 0x86, 0x4c, 
    0x54, 0x05, 0x89, 0xe6, 
    0x18, 0x28, 0x15, 0xee, 
    0xC3
};



//to protect the power source
void disable_core_bystep(int fd)
{
    char i;
    unsigned int reg_val = 0xffffffff;
    for (i=0;i<8;i++)
    {
        reg_val &= ~(0x0f << (i*4));		
        jdasic_write_register(fd, ALL_CHIP, 0xf8, 0x04, reg_val);
        usleep(30000);
        jdasic_write_register(fd, ALL_CHIP, 0xf8, 0x05, reg_val);
        usleep(30000);
    }
}

// after asic's baudrate changed,host must change subsequently.
void set_linux_uart_baudrate(int fd,speed_t speed)
{
    struct termios my_termios;
    tcgetattr(fd, &my_termios);
    cfsetispeed(&my_termios, speed);
    cfsetospeed(&my_termios, speed);
    tcsetattr(fd, TCSANOW, &my_termios);
    tcflush(fd, TCIOFLUSH);
}

/*
 *brief:disable power enalbe pin.
 *
 *to turn on calculate board again call hw_init(); 
 * 
 * uart_port_num:
 * 				/dev/ttymx3 -> uart_port_num=3;
 *				/dev/ttymx4 -> uart_port_num=4
 *				.......
 *
 */
void shutdown_board(const char uart_port_num)
{

	switch(uart_port_num)
	{
		case 5:
			//disable_core
			system("echo 0 > /sys/class/gpio/gpio88/value");
			break;

		case 2:
			//disable_core
			system("echo 0 > /sys/class/gpio/gpio9/value");
			break;
		
		case 3:
			//disable_core
			system("echo 0 > /sys/class/gpio/gpio27/value");
			break;

		case 4:
			//disable_core
			system("echo 0 > /sys/class/gpio/gpio23/value");
			break;
		
		case 6:
			//disable_core
			system("echo 0 > /sys/class/gpio/gpio44/value");
			break;
		case 7:
			//disable_core
			system("echo 0 > /sys/class/gpio/gpio22/value");
			break;
	}
	

}


/*
 *uart_port_num: /dev/ttymx3 -> 3
 */
inline void set_led(int uart_port_num, int val)
{

		if (val)
		{
			switch(uart_port_num)
			{
				case 5:
					system("echo 1 > /sys/class/gpio/gpio119/value");
					break;

				case 2:
					system("echo 1 > /sys/class/gpio/gpio120/value");
					break;
				
				case 3:
					system("echo 1 > /sys/class/gpio/gpio121/value");
					break;

				case 4:
					system("echo 1 > /sys/class/gpio/gpio122/value");
					break;
				
				case 6:
					system("echo 1 > /sys/class/gpio/gpio123/value");
					break;

				case 7:
					system("echo 1 > /sys/class/gpio/gpio124/value");
					break;
			}
		}
		else
		{
			
			switch(uart_port_num)
			{
				case 5: system("echo 0 > /sys/class/gpio/gpio119/value");
					break;

				case 2:
					system("echo 0 > /sys/class/gpio/gpio120/value");
					break;
				
				case 3:
					system("echo 0 > /sys/class/gpio/gpio121/value");
					break;

				case 4:
					system("echo 0 > /sys/class/gpio/gpio122/value");
					break;
				
				case 6:
					system("echo 0 > /sys/class/gpio/gpio123/value");
					break;

				case 7:
					system("echo 0 > /sys/class/gpio/gpio124/value");
					break;
			}
		}
}

void flash_status_led(struct timeval *now_t, struct tty_chain *d12)
{
	uint8_t uart_port_num;

	if (ms_tdiff(now_t, &d12->last_flash_t) > 500)
	{
		uart_port_num = d12->ctx->devname[11];
		uart_port_num -= 48; 
		cgtime(&d12->last_flash_t);	
		d12->led_val = !d12->led_val;
		set_led(uart_port_num, d12->led_val);
	}

}

static char ifNeedBist = 1;

//enable power_enalbe pin, hardware reset
void hw_init(const char * devname)
{
	static char run_once = 0;
	// /dev/ttymx3 -> '3' 
	char uart_port_num = devname[11];

	uart_port_num -= 48;


	shutdown_board(uart_port_num);
	usleep(100000);

	//vid1
	system("echo 1 > /sys/class/gpio/gpio117/value");	

	//vid2
	system("echo 1 > /sys/class/gpio/gpio118/value");

	switch(uart_port_num)
	{
		case 5:
			//en_core
			system("echo 1 > /sys/class/gpio/gpio88/value");
			usleep(20000);
			//reset
			system("echo 0 > /sys/class/gpio/gpio91/value");
			usleep(5000);
			system("echo 1 > /sys/class/gpio/gpio91/value");
			usleep(5000);
			system("echo 0 > /sys/class/gpio/gpio91/value");
			usleep(5000);
			break;

		case 2:
			//en_core
			system("echo 1 > /sys/class/gpio/gpio9/value");
			usleep(20000);
			//reset
			system("echo 0 > /sys/class/gpio/gpio26/value");
			usleep(5000);
			system("echo 1 > /sys/class/gpio/gpio26/value");
			usleep(5000);
			system("echo 0 > /sys/class/gpio/gpio26/value");
			usleep(5000);
			break;
		
		case 3:
			//en_core
			system("echo 1 > /sys/class/gpio/gpio27/value");
			usleep(20000);
			//reset
			system("echo 0 > /sys/class/gpio/gpio42/value");
			usleep(5000);
			system("echo 1 > /sys/class/gpio/gpio42/value");
			usleep(5000);
			system("echo 0 > /sys/class/gpio/gpio42/value");
			usleep(5000);
			break;

		case 4:
			//en_core
			system("echo 1 > /sys/class/gpio/gpio23/value");
			usleep(20000);
			//reset
			system("echo 0 > /sys/class/gpio/gpio19/value");
			usleep(5000);
			system("echo 1 > /sys/class/gpio/gpio19/value");
			usleep(5000);
			system("echo 0 > /sys/class/gpio/gpio19/value");
			usleep(5000);
			break;
		
		case 6:
			//en_core
			system("echo 1 > /sys/class/gpio/gpio44/value");
			usleep(20000);
			//reset
			system("echo 0 > /sys/class/gpio/gpio43/value");
			usleep(5000);
			system("echo 1 > /sys/class/gpio/gpio43/value");
			usleep(5000);
			system("echo 0 > /sys/class/gpio/gpio43/value");
			usleep(5000);
			break;
		case 7:
			//en_core
			system("echo 1 > /sys/class/gpio/gpio22/value");
			usleep(20000);
			//reset
			system("echo 0 > /sys/class/gpio/gpio18/value");
			usleep(5000);
			system("echo 1 > /sys/class/gpio/gpio18/value");
			usleep(5000);
			system("echo 0 > /sys/class/gpio/gpio18/value");
			usleep(5000);
			break;
	}
}
#ifdef USE_SPI
int spi_transferWords(int fd, uint16_t txbuf,uint16_t *rxbuf)
{
        int ret;
        uint16_t *tx = &txbuf;
        uint16_t *rx = rxbuf;

        if(fd<0)
            printf("failed open spi dev\n");

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
                .rx_buf = (unsigned long)rx,
		.len = 2,
	};
    	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret == 1)
		pabort("spi_test can't write spi message");   

        return (ret==0)?0:-1;
}

void spi_test(int fd)
{
        spi_transferWords(fd,0xaabb,NULL);
       // spi_transferWords(fd,0xccdd,NULL);
       // spi_transferWords(fd,0xeeff,NULL);
       // spi_transferWords(fd,0xabcd,NULL);
       // spi_transferWords(fd,0xef00,NULL);
       // spi_transferWords(fd,0xffff,NULL);
}

inline bool uart_write(int fd, const unsigned char *buffer, const int length)
{
    int ret;
    
    if(fd<0)
         applog(LOG_ERR, "wrong file desc(%d)",fd);
    
    applog(LOG_DEBUG,"%s addr(%p) len(%d)\n",__func__,buffer,length);

    struct spi_ioc_transfer tr = {
    	.tx_buf = (unsigned long)buffer,
        .len = length,
    };
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret == 1)
		pabort("uart_write can't write spi message");
    
    return (ret==0);

}

inline bool uart_read(int fd, unsigned char *buffer, const int length)
{
    int ret;
    
    if(fd<0)
         applog(LOG_ERR, "wrong file desc(%d)",fd);

    struct spi_ioc_transfer tr = {
    	.rx_buf = (unsigned long)buffer,
    	.len = length,
    };
    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret == 1)
		pabort("can't read spi message");   

    return (ret==0);
}
#else
inline bool uart_write(int fd, const unsigned char *buffer, const int length)
{
    bool ret = true;
    size_t size = 0;
    size_t count = 0;
    int repeat = 0;
    int i;
    while(count < length)
    {
        size = write(fd, buffer+count, length-count);	
        if (size < 0)
        {
            printf("%s %d\n", __func__, errno);
            ret = false;
            break;
        }
        count += size;
        if (repeat++ > 1)
        {
            printf("%s repeat %d\n", __func__, repeat);
            if (count != length)
            {
                ret = false;
                break;
            }
        }
    }
    for(i=0;i<length;i++)
    {
        printf("0x%02x",buffer[i]);
    }
    printf("\n\r");
    return ret;
}

inline bool uart_read(int fd, unsigned char *buffer, const int length)
{
    bool ret = true;
    size_t size = 0;
    size_t count = 0;
    int repeat = 0 ;
    while(count < length)
    {
        size = read(fd, buffer+count, length-count);
        if (size < 0)
        {
            printf("%s %d\n", __func__, errno);
            ret = false;
            break;
        }
        count += size;
        if (repeat++ > 1)
        {
            if (count != length)
            {
                ret = false;
                break;
            }
        }
    }
    return ret;
}

#endif


/*
*chipid:0-0xff, 0xff for broadcast
*module:0x00 for cpm module, 0xf8 for btc module
*/
bool jdasic_write_register(int fd, int chip_id, int module, int reg_addr, int value)
{
    bool ret = true;
    unsigned char write_reg_buf[12]={MAGIC_HEADER, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,MAGIC_EOF};

    write_reg_buf[1] = chip_id;
    write_reg_buf[2] = module;
    write_reg_buf[3] = 0x7f&reg_addr; //&0x7f->write bit[7]:1 read, 0 write
    write_reg_buf[4] = value&0xff;
    write_reg_buf[5] = (value>>8)&0xff;
    write_reg_buf[6] = (value>>16)&0xff;
    write_reg_buf[7] = (value>>24)&0xff;

    ret = uart_write(fd, write_reg_buf, 9);
    
    usleep(10000);
    return  ret;
}


/*
*chipid:0-0xff, 0xff for broadcast
*module:0x00 for cpm module, 0xf8 for btc module
*/
bool jdasic_read_register(int fd, int chip_id, int moudle, int reg_addr, uint32_t *pval)
{
    int i;
    bool ret = false;
    unsigned int *pValue;
    unsigned char read_reg_data[8]={0};
    unsigned char *p_data = read_reg_data;
    unsigned char  read_reg_cmd[12]={MAGIC_HEADER, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,MAGIC_EOF};

    read_reg_cmd[1] = chip_id;
    read_reg_cmd[2] = moudle;
    read_reg_cmd[3] = 0x80|reg_addr;

    do
    {
        if (!uart_write(fd, read_reg_cmd, 9))
            break;
        usleep(1000);
        if(!uart_read(fd, read_reg_data, 8))
            break;
        for (i=0; i<8; i++)
            applog(LOG_DEBUG, "0x%02x ", read_reg_data[i]);
        ret = true;
    }while(0);

    if(ret)
        *pval = *((unsigned int*)(&read_reg_data[4]));

    return ret;

}



//detect if there is asic connect to current uart port by broadcast a read_register cmd
bool detect_board(int fd, unsigned char *devname)
{

    
#ifdef USE_SPI
    struct timeval t1,t2;
    static uint16_t ack;
    
    gettimeofday(&t1,NULL);
    t2.tv_sec = t1.tv_sec;
    t2.tv_usec = t1.tv_usec+TIMEOUT;

    // send detect cmd
    spi_transferWords(fd,CMD_DETECT_BOARD,NULL);
    // wait for respond
    while(1)
    {       
        ack = ACK_DILE;
        spi_transferWords(fd,0,&ack);
        if(ack == ACK_DETECT_BOARD)
        {
            applog(LOG_NOTICE,"detect_board successfully with ack(0x%x)",ack);
            break;
        }       
        gettimeofday(&t1,NULL);
        if((t1.tv_usec == t2.tv_usec) )
        {
            applog(LOG_ERR,"detect_board timeout");            
            break;
        }
    }
    return (ack==ACK_DETECT_BOARD)?true:false;
   
#else    
    uint32_t  regval = 0;
    //here use register 0xf826,other register also work.
    if ( jdasic_read_register(fd, ALL_CHIP, 0xf8, 0x36, &regval) )	
    {
        ret = true;
        printf("find board\n");
    }
        return ret;
#endif

}

/*
 *init_id: init the first chip id, the following chip will auto set address orderly.
 *mode: 0 forward mode, 1 bypass mode (direct connect mode)
 */

bool jdasic_auto_config(int fd, int init_id, int mode)
{
    int value = 0x80000000;
    value |= (init_id & 0xff);
    if(mode)
        value |= 0x40000000;
    return jdasic_write_register(fd, ALL_CHIP, MODULE_CPM, AUTO_CFG, value);
}

/*
 *uart_mode: 4.0 4 times sample, 8.0 8 times sample 16.0 16 times sample
 *note: after setting the asic baudrate, you should call set_linux_uart_baudrate() to change host`s baudrate.
 */
bool jdasic_set_baudrate(int fd, int baudrate, double uart_mode)
{
    double clock_src = OSCILLATOR_CLK;
    double baudrate_divider	 = clock_src/uart_mode/baudrate;
    unsigned int divide_int = (unsigned int)baudrate_divider;
    unsigned int divide_fraction = (unsigned int)((baudrate_divider - divide_int)*1024);
    unsigned int uart_config = 0xd400001f;//1M:0xb400001f,1.5M:0xd400001f
    bool ret = false;
    do
    {
        if(baudrate == 1000000)
            uart_config = 0xb400001f;
        else if(baudrate == 1500000 )
            uart_config = 0xd400001f;
            uart_config = uart_config|((divide_int&0xff)<<8);
            uart_config = uart_config|((divide_fraction&0x3ff)<<16);
            //PRINTF("uart mode %d uart baudrate =%d config value=%08x\n",(unsigned int)uart_mode, baudrate, uart_config);
            if (!jdasic_write_register(fd, ALL_CHIP, MODULE_CPM, UART_CFG, uart_config))
            {
                printf("%s:%d\n", __FILE__, __LINE__);
                break;
            }
        //change host's baudrate too.
        set_linux_uart_baudrate(fd, B1500000);
            ret = true;
    }while(0);
        return ret;
}


bool jdasic_disable_all_btc_core(int fd)
{
    bool ret = false;
    do
    {
		if (!jdasic_write_register(fd, ALL_CHIP, MODULE_CPM, BTC_CORE0, 0))
		    break;
		usleep(1000);
		if(!jdasic_write_register(fd, ALL_CHIP, MODULE_CPM, BTC_CORE1, 0))
		    break;
		usleep(1000);
		if(!jdasic_write_register(fd, ALL_CHIP, MODULE_CPM, BTC_CORE2, 0))
		    break;
		usleep(1000);
		if(!jdasic_write_register(fd, ALL_CHIP, MODULE_CPM, BTC_CORE3, 0))
		    break;
		ret = true;
    }while(0);
    return ret;
}

//software reset
bool jdasic_reset_btc(int fd)
{
    bool ret = false;
    do
    {
		if(!jdasic_write_register(fd, ALL_CHIP, MODULE_CPM, SOFT_RESET, 0x00))
		    break;
		usleep(8000);
		if(!jdasic_write_register(fd, ALL_CHIP, MODULE_CPM, SOFT_RESET, 0x03))
		    break;
		ret = true;
    }while(0);
    return ret;
}

bool jdasic_set_pll(int fd, int freq, unsigned char chip_id)
{
		unsigned int Fin = 12; //25M
		unsigned int pll_F =0x40;/////64 <= pll_F <=0x3ff    [22:13]pll_fb
		unsigned int pll_R =0x10;/////1 <= pll_R <= 0x3f  [28:23]pll_pr
		unsigned int pll_OD =0x01;////0 <= pll_OD <=5	[31:29]pll_od
		unsigned int pll_config = 0x00001005;
		
		freq |= 0x1800;
		if(freq<50) pll_OD = 0x00;//res25M,min 25M
		pll_F = (freq*(0x01<<pll_OD)*pll_R)/Fin; //res 50M,min=50M
#if 1 //300M
		pll_F = 300;
		pll_R = 6;
#endif
		if(pll_F<0x40) pll_F = 0x40;
		if(pll_F>0x3ff)  pll_F = 0x3ff;

		pll_config =   pll_config | (freq << 13 ) | ( (pll_OD & 0x03 ) << 29 );
		return jdasic_write_register(fd, chip_id, MODULE_CPM, PLL_REG, pll_config);//gate disable

}

/*
 *mode:  work mode , 0: fifo mode 1: force start mode(usually use force start mode)
 */
bool jdasic_set_btc_mode(int fd, unsigned int mode)
{
		unsigned int work_mode = 0x09810000;
		if(mode ==0x00)
		{
				work_mode |=0x01000;
		}
		else
		{
		}
		return jdasic_write_register(fd, ALL_CHIP, MODULE_BTC, BTC_CFG1, work_mode);
}

/*
 *bypass: 1: bypass mode(Direct Connect) 0: forward mode
 */
bool jdasic_set_uart_mode(int fd, unsigned int chip_id, unsigned int bypass)
{
		unsigned int value = 0x22110002;
		value |=(bypass & 0x01);
		return jdasic_write_register(fd, chip_id, MODULE_CPM, STREAM_CFG, value);

}


unsigned char calculate_good_core(unsigned int reg_val)
{
		unsigned char goodCores=0;
		unsigned char i;
		unsigned int RegisterVal = reg_val;
		for(i=0; i<32; i++)
		{
				if (RegisterVal & 0x01)
						goodCores++;
				RegisterVal = RegisterVal>>1;
		}
		return goodCores;
}



/*******************************************************************************
Function name : jdasic_btc_bist
Description:
			 do jdasic btc bist(get good core number) 
********************************************************************************/
bool jdasic_btc_bist(int fd, unsigned char chip_id)
{
    unsigned int i;
    unsigned int bist_value = 0;
    unsigned int goodcores0=0;
    unsigned int goodcores1=0;
    unsigned int  total = 0;
    bool ret = false;
    //1. autoconfig : 3c ff F8 78 01 00 00 80 c3
    //2. reset BTC core: 3C FF F8 1e 00 00 00 00 C3 ; 3C FF F8 1e 03 00 00 00 C3
    //3. enable bist mode: 3C ff 3f 1C 20 00 00 00 C3
    //4. start bist test: 3C ff 3f 0c 18 28 15 ee C3
    //5. stop task : 3C ff 3f 18 02 00 00 00 C3
    //6. read btc bist result: 3C ff 00 9A 00 00 00 00 C3; 3C FF 3f 9C 00 00 00 00 C3
    do
    {
        if(!jdasic_write_register(fd, ALL_CHIP, 0x3f, 0x1c, 0x00000020))//bist enable
            break;
        if(!jdasic_write_register(fd, ALL_CHIP, 0x3f, 0x0c, 0xee152818))//bist start, data2
            break;
        //jdasic_write_register(fd, ALL_CHIP, 0x3f, 0x18, 0x00000002);//bist stop
        if(!jdasic_write_register(fd, ALL_CHIP, 0x3f, 0x18, 0x000000001))//bist stop
            break;
        for(i=1; i < (setting.chipNumber + 1); i++)
        {
            goodcores0 = 0;
            goodcores1 = 0;
            if(!jdasic_read_register(fd, i, 0x00, 0x1a, &bist_value))
            {
                printf("%s bist_val failed\n", __func__);
                break;
            }
            //printf("bist_value0 = %04x\n", bist_value);
            if (bist_value > 0)
            {
                goodcores0  = calculate_good_core(bist_value);
                printf("goodcores0 = %d\r\n", goodcores0);
            }
            if(!jdasic_read_register(fd, i, 0x00, 0x1a, &bist_value))
            {
                printf("%s bist_val failed\n", __func__);
                break;
            }
            //printf("bist_value1 = %04x\n", bist_value);
            if (bist_value > 0)
            {
                goodcores1  = calculate_good_core(bist_value);
                printf("goodcores1 = %d\r\n", goodcores1);
            }
            total += ( goodcores0+goodcores1);
            ret = true;
            printf("chip_%d cores=%d\n", i, total);
        }
    }while(0);
    return ret;
}

void pull_up_payload(int fd)
{
    char i;
    unsigned int regval;
//pull up payload by steps.
        for (i=0; i<16; i++)
        {
            regval  |= (0x03<<(2*i));
            if(!jdasic_write_register(fd, ALL_CHIP, 0xf8, 0x14, regval)) break;
            usleep(35000);
            if(!jdasic_write_register(fd, ALL_CHIP, 0xf8, 0x15, regval)) break;
            usleep(35000);
            if(!jdasic_write_register(fd, ALL_CHIP, 0xf8, 0x16, regval)) break;
            usleep(35000);
            if(!jdasic_write_register(fd, ALL_CHIP, 0xf8, 0x17, regval)) break;
            usleep(35000);
            if(!uart_write(fd, job3,57)) break;
            usleep(35000);
        }
        tcflush(fd, TCIOFLUSH);
}


/*******************************************************************************
Function name : jdasic_config_btc
Description:
			 config jdasic btc moudle
Parameter:
			unsigned int mode : work mode , 0: fifo mode 1: force start mode
			int init_id: first chip init id
Return value:
			void
Notes:
			NULL
********************************************************************************/
#ifdef USE_SPI
bool jdasic_config_btc(int fd, int freq)
{

    struct timeval t1,t2;
    static uint16_t ack;   
    gettimeofday(&t1,NULL);
    t2.tv_sec = t1.tv_sec;
    t2.tv_usec = t1.tv_usec+TIMEOUT;

    uint16_t cmd = (freq<<4) |CMD_CONFIG_BTC;
    spi_transferWords(fd,cmd,NULL);
    ack = 0;
    while(1)
    {       
        spi_transferWords(fd,0,&ack);
        if(ack ==ACK_CONFIG_BTC)
        {
            applog(LOG_NOTICE,"config_btc successfully with ack(0x%x)",ack);
            break;
        }       
        gettimeofday(&t2,NULL);
        
        if(t2.tv_usec== t1.tv_usec)
        {
            applog(LOG_ERR,"config_btc timeout\n");            
            break;
        }
    }
    return (ack==ACK_DETECT_BOARD)?true:false;
}

#else
bool jdasic_config_btc(int fd, int freq)
{
#ifdef DEBUG
	return 1;
#endif	

	unsigned char i;
	unsigned int regval=0xff;
	bool ret = false;

    do
    {
        if(!jdasic_auto_config(fd, 1, 1)) break;
        usleep(40000);		
        if(!jdasic_set_pll(fd,freq, ALL_CHIP)) break;
        usleep(40000);
        //if(!jdasic_set_baudrate(fd,1500000,8.0)) break;
        set_linux_uart_baudrate(fd,B230400);
        usleep(10000);
        //if(!jdasic_write_register(fd, ALL_CHIP, 0xf8, 0x22, 0x11090005)) break;//feed through
        //usleep(3000);
        if(!jdasic_set_btc_mode(fd, 1)) break;//force mode
        usleep(3000);
        if(!jdasic_disable_all_btc_core(fd)) break;
        usleep(3000);
        tcflush(fd, TCIOFLUSH);
        ifNeedBist = 0;
        if (ifNeedBist)
        {
            /*get good core number*/
            for (i=0; i<8; i++)
            {
                    regval = 0xf<<(4*i);
                    if (!jdasic_write_register(fd, 0xff, 0xf8, 0x04, regval))break;
                    usleep(5000);
                    if(!jdasic_write_register(fd, 0xff, 0xf8, 0x05, regval)) break;
                    usleep(5000);
                    if(!jdasic_btc_bist(fd, i)) break;
                    if(!jdasic_reset_btc(fd)) break;//retset
                    usleep(5000);
            }
        }
        else
        {
            if(!jdasic_write_register(fd, 0xff, 0x3f, 0x18, 0x000000001)) break;//bist stop
            if(!jdasic_reset_btc(fd)) break;//retset
        }
        ifNeedBist = 0;
#if 0
		for (i=1; i<=setting.chipNumber; i++)
		{
		
			jdasic_read_register(fd, i,  0xf8, 0x26, &regval);	
		}
#endif
        ret = true;
    }while(0);

	return ret;
}
#endif

/* open UART device */
#ifdef USE_SPI
int jdasic_open(const char *devname, speed_t baud)
{
    int fd = -1;
    int ret = 0;
    int bits = 16;   
    int mode = 0;;
    int speed = baud;

    do
    {
        fd = open(devname, O_RDWR | O_CLOEXEC | O_NOCTTY);
        if (fd == -1) 
            {
            if (errno == EACCES)
                printf("Do not have user privileges to open %s\n", devname);
            else
                printf("failed open device %s\n", devname);
            return  -1;
        }
         
        
        /* 
         * spi mode 
         */  
        ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);  
        if (ret == -1)  
                pabort("can't get spi mode");  
            
        /* 
         * bits per word 
         */  
        ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);  
        if (ret == -1)  
                pabort("can't set bits per word");  
  
        ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);  
        if (ret == -1)  
                pabort("can't get bits per word");  


       /* 
         * max speed hz 
         */  
        ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);  
        if (ret == -1)  
                pabort("can't set max speed hz");  
  
        ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);  
        if (ret == -1)  
                pabort("can't get max speed hz");  
 
        applog(LOG_NOTICE,"spi mode: %d\n", mode);  
        applog(LOG_NOTICE,"bits per word: %d\n", bits);  
        applog(LOG_NOTICE,"max speed: %d Hz (%d MHz)\n", speed, speed/1000000);  
        
    }while(0);
    return fd;
}

#else
int jdasic_open(const char *devname, speed_t baud)
{
    struct termios	my_termios;
    int uart_fd = -1;
    usleep(10000);

    do
    {
        uart_fd = open(devname, O_RDWR | O_CLOEXEC | O_NOCTTY);
        if (uart_fd == -1) 
            {
            if (errno == EACCES)
                printf("Do not have user privileges to open %s\n", devname);
            else
                printf("failed open device %s\n", devname);
            return  -1;
        }

        if(tcgetattr(uart_fd, &my_termios)) break;
        if(cfsetispeed(&my_termios, baud)) break;
        if(cfsetospeed(&my_termios, baud)) break;
        //cfsetspeed(&my_termios,  baud);

        my_termios.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
        my_termios.c_cflag |= CS8;
        my_termios.c_cflag |= CREAD;
        my_termios.c_cflag |= CLOCAL;

        my_termios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK |
                        ISTRIP | INLCR | IGNCR | ICRNL | IXON);
        my_termios.c_oflag &= ~OPOST;
        my_termios.c_lflag &= ~(ECHO | ECHOE | ECHONL | ICANON | ISIG | IEXTEN);

        // Code must specify a valid timeout value (0 means don't timeout)
        my_termios.c_cc[VTIME] = (cc_t)10;
        my_termios.c_cc[VMIN] = 0;

        if(tcsetattr(uart_fd, TCSANOW, &my_termios)) break;
        if(tcflush(uart_fd, TCIOFLUSH)) break;
    }while(0);

#ifdef DEBUG
	 set_linux_uart_baudrate(uart_fd, B1500000); //for debug
#endif

    return uart_fd;
}
#endif

#ifdef USE_SPI
inline bool write_job(int fd, char *job_buf, int size)
{
    uint16_t index; 
    uint16_t data;
    uint16_t *local_buf; 
    int local_size;
    local_size = size;
    local_buf = (uint16_t*)(job_buf);    
    if(local_size%2!=0)
        local_size+=1;
        
    for(index =0; index<local_size/2;index++)
    {
        data = *(local_buf+index);       
        spi_transferWords(fd,data,NULL);
    }
    return true;
}

#else
inline bool write_job(int fd, char *job_buf, int size)
{
	bool ret = true;
	ret = uart_write(fd, job_buf, size);	
	return ret;
}
#endif
/*
  * do read nonce operation through hardware bus
  * return true if success
  * return false if failed
  */
  #if 1
inline bool read_nonce(int fd, unsigned int *nonce, unsigned char *chip_id, unsigned char *job_id)
{
    #define BUF_SIZE 8
    bool ret = false;
    unsigned char buffer[BUF_SIZE] = {0};
    unsigned char nonce_buffer[BUF_SIZE] = {0};
    int iocount = 0;
    ioctl(fd, FIONREAD, &iocount);
    if(iocount == 0 || (iocount%8) != 0)
        return false;
    do
    {
        int i;
        ret = uart_read(fd, buffer, BUF_SIZE);
        if (!ret)
            break;
        if (buffer[0] != MAGIC_HEADER)
        {
            break;
        }
        if ((buffer[2]&1) == 0)	
            break;
        memcpy(nonce_buffer, buffer, BUF_SIZE);	
        *chip_id = nonce_buffer[1];
        *job_id = (0xc0 & nonce_buffer[3])>>6;
        for (i = 7; i > 3; i--)
        {
            (*nonce) = ((*nonce) << 8)|(nonce_buffer[i]) ;
        }
        ret = true;
    }while(0);

    if (*chip_id == 0 || *chip_id >setting.chipNumber || *nonce == 0)
		ret = false;
	return ret;
}
#else

bool read_nonce(int fd, unsigned int *nonce, unsigned char *chip_id, unsigned char *job_id)
{
	// a nonce package size=8
	#define BUF_SIZE 8	
	bool ret = true;
	char i = 0;
	bool get_header = true;
	unsigned char buffer[BUF_SIZE] = {0};
	unsigned char nonce_buffer[BUF_SIZE] = {0};

	ret = uart_read(fd, buffer, BUF_SIZE);

	if (!ret)
	{
		return false;	
	}
	
	if (buffer[0] != MAGIC_HEADER)
	{
		get_header = false;	
		for (i=1; i<BUF_SIZE; i++)	
		{
			if (buffer[i] == MAGIC_HEADER)
			{
				get_header = true;	
				memcpy(nonce_buffer, buffer+i, BUF_SIZE-i);
				
				if (!uart_read(fd, buffer, i))
				{
					return false;	
				}

				memcpy(nonce_buffer+BUF_SIZE-i, buffer, i);
			}
		}
	}
	else
	{
		memcpy(nonce_buffer, buffer, BUF_SIZE);	
	}


	if (!get_header)
	{
		return false;	
	}


#if 0	
	for (i=0; i<8; i++)
	{
		printf("0x%02x ", buffer[i]);	
	}
	printf("\r\n");
#endif
	
	for (i=0; i<8; i++)
	{
		printf("0x%02x ", nonce_buffer[i]);	
	}
	printf("\r\n");
	
	*chip_id = nonce_buffer[1];
	*job_id = (0xc0 & nonce_buffer[3])>>6;
	*nonce = *((unsigned int*)(&nonce_buffer[4]));

	return ret;
}
#endif

#if  0
int main(void)
{

		bool ret;
		int uart_fd;
		unsigned int nonce;
		unsigned char job_id, chip_id;

		uart_fd = jdasic_open("/dev/ttymxc3",B115200);		

		ret = detect_board(uart_fd, "/dev/ttymxc3");
		if (ret)
		{
			jdasic_config_btc(uart_fd, DEFAULT_PLL);				
		}
		else
		{
			printf("No board!\n");	
		}


		sleep(1);
		//now all you need to do is write job and get nonce.

		write_job(uart_fd, job2, 57);

		while(1)
		{   //nonce:F0 FC 1F 8E
			read_nonce(uart_fd, &nonce, &chip_id, &job_id);	
		}


	return 0;
}
#endif

