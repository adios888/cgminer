#ifndef __JDASIC_H_

#define __JDASIC_H_

#define USE_SPI
#define SPI_RATE   4000000

#define MAX_ASIC_PER_INTERFACE		48		//chip number of one calculate board(or uart)
#define MAGIC_HEADER 0x3c 
#define MAGIC_EOF 0xc3
#define ALL_CHIP 0xff
#define MODULE_CPM 0xf8
#define MODULE_BTC 0x3f
#define OSCILLATOR_CLK 12000000  //ASIC chip extern oscillator frequency
#define BAUD_RATE B1500000
#define DEFAULT_PLL 530


enum jdasic_cpm_reg_addr
{
    PLL_REG = 0x00,		//PLL, (init)
    PLL_REG_PRE = 0x30,	//PLL, (pre)
    PAD_REG = 0x02,	//no use
    BTC_CORE0 = 0x14,	//HCE clock enable, (init, bist)
    BTC_CORE1 = 0x15,
    BTC_CORE2 = 0x16,
    BTC_CORE3 = 0x17,	//HCE clock disable, (init, bist)
    SOFT_RESET = 0x1e,	//soft reset uart and HCE, (init, bist)
    UART_CFG = 0x20,	//baudrate and send interval setting, (init)
    STREAM_CFG = 0x22,
    DEVICE_ID = 0x26,  //code name
    AUTO_CFG = 0x65   //init address set connect mode, (init)
};

enum jdasic_btc_reg_addr
{
    BTC_INIT_NONCE = 0x00,		//init nonce, (init)
    BTC_DIFF_TARGET = 0x01,		//difficulty, (init)
    BTC_MIDSTATE = 0x02,
    BTC_DATA2 = 0x0a,       //data2(wdata), (bist)
    BTC_NTIME = 0x0b,
    BTC_BIST_RET = 0x1a,    //Test result of all HCE, (bist)
    BTC_CFG0 = 0x18,            //task enable, (bist)
    BTC_CFG1 = 0x1c		//HCE_TOP:bist enable/result,task delay/precul_time, (init, bist)
};




extern bool jdasic_write_register(int fd, int chip_id, int module, int reg_addr, int value);
extern bool jdasic_read_register(int fd, int chip_id, int moudle, int reg_addr, uint32_t *regval);
extern bool jdasic_set_baudrate(int fd, int baudrate, double uart_mode);
extern bool jdasic_disable_all_btc_core(int fd);
extern bool jdasic_reset_btc(int fd);
extern bool jdasic_set_pll(int fd, int freq, unsigned char chip_id);
extern bool jdasic_set_btc_mode(int fd, unsigned int mode);
extern bool jdasic_set_uart_mode(int fd, unsigned int chip_id, unsigned int bypass);
extern unsigned char calculate_good_core(unsigned int reg_val);
extern bool jdasic_auto_config(int fd, int init_id, int mode);
extern bool jdasic_btc_bist(int fd, unsigned char chip_id);
extern int jdasic_open(const char *devname, speed_t baud);
extern bool detect_board(int fd, unsigned char *devname);
extern bool jdasic_config_btc(int fd, int freq);
extern bool write_job(int fd, char *job_buf, int size);
extern bool read_nonce(int fd, unsigned int *nonce, unsigned char *chip_id, unsigned char *job_id);
extern void disable_core_bystep(int fd);
extern void shutdown_board(const char uart_port_num);
extern void set_linux_uart_baudrate(int fd,speed_t speed );
extern void set_led(int uart_port_num, int val);
extern void pull_up_payload(int fd);

#endif
