/*
 *  cgminer SPI driver for Bitmine.ch D12 devices
 *
 *  Copyright 2013 Zefir Kurtisi <zefir.kurtisi@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 3 of the License, or (at your option)
 *  any later version.  See COPYING for more details.
 *  Tachibana Kin @Jingda Tech Co..,Ltd
 */

#include "config.h"

#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <stdbool.h>
#include <termios.h>
#ifdef HAVE_CURSES
#include <curses.h>
#endif
#include "logging.h"
#include "miner.h"
#include "CgConfig.h"
#include "jdasic.h"

//features
#define tty_PLL_CLOCK_EN     //config D12's chip PLL clock
#define API_STATE_EN         //support show extra ASIC information
#define tty_SPI_SPEED_EN     //support change Master SPI speed
#define tty_CHK_CMD_TM       //check command timeout

#define tty_MAX_SPEED      200
#define MAX_ASIC_BOARD            8
#define MAX_JOB                   4
#define MAX_CORE_NUMS             63
#define MAX_ASIC_NUM_PER_BOARD    48
#define MAX_ASIC_NUMS             (MAX_BOARDS * MAX_ASIC_NUM_PER_BOARD)

/********** chip and chain context structures */
#define ASIC_BOARD_OK        0x00000000
#define ERROR_CHIP           0x00000001
#define ERROR_CORE           0x00000002
#define ERROR_TEMP_SENSOR    0x00000004
#define ERROR_BOARD          0x00000008
#define ERROR_OVERHEAD       0x00000010

#define CHIP_NUM             48
#define CORE_NUM             63 

#define ARRAY_NUM(x)    (sizeof((x)) / sizeof((x)[0]))
#define PLL_CLOCK_OUT_MIN        (250)
#define PLL_CLOCK_OUT_MAX        (1200)
#define PLL_CLOCK_OUT_STEP       (5)
#define PLL_CLOCK_OUT_DEFAULT    (275)

#define SET_PLL_CLK(idx)                     \
    (gPllClk = PllTbl_250_1200[idx].pll_fb | \
               (PllTbl_250_1200[idx].pll_pr << 10))
#define PLL_DEFAULT_IDX \
    ((PLL_CLOCK_OUT_DEFAULT - PLL_CLOCK_OUT_MIN) / PLL_CLOCK_OUT_STEP)

enum {
    PLL_CFG_BIT_LEN = 1, 
    PLL_PR_BIT_LEN = 6,
    PLL_FB_BIT_LEN = 10,
    PLL_OD_BIT_LEN  = 3,
    PLL_UPDATE_BIT_LEN = 1,
    PLL_BP_BIT_LEN = 1,
};

typedef struct pll_tbl {
    uint16_t pll_fb;            /*13-22,  2^10-1>=x>= 64*/
    uint8_t  pll_pr;            /*23-28 1<= x<=63*/
                                /*totally 13-28 16bits*/
} PllTbl;

typedef struct D12AppREG {
    uint16_t pllReg;
    uint8_t  volt;
    //read only  part
    uint8_t  goodCores;
    int8_t   temp;
    int8_t   padding;
} D12AppAsicReg;

struct tty_chip {
    int            id;
    int            num_cores;
    int            last_queued_id;
    struct work    *work[MAX_JOB];
    /* stats */
    int            hw_errors;
    int            stales;
    int            nonces_found;
    int            nonce_ranges_done;
    D12AppAsicReg  reg;
    struct timeval job_start_time;
    uint32_t       lastnonce;
    //uint8_t reg[4]; //latest update from ASIC
};

enum tty_command {
    tty_BIST_START   = 0x01,
    tty_BIST_FIX     = 0x03,
    tty_RESET        = 0x04,
    tty_WRITE_JOB    = 0x07,
    tty_READ_RESULT  = 0x08,
    tty_WRITE_REG    = 0x09,
    tty_READ_REG     = 0x0a,
    tty_READ_REG_RESP= 0x1a,
    tty_READ_LKSN    = 0x0c,
};

static char multiple = 1; 
extern int  opt_jobtimeout;
extern int opt_diff;
uint32_t lastnonce = 0;
int lastchipid = -1;
uint8_t LKSN[16] = { 0 };
uint16_t gPllClk;
PllTbl   PllTbl_250_1200[] =
{
    /*250-270 MHz*/
    {  250, 6 }, {  255, 6 }, {  260, 6 }, {  265, 6 }, {  270, 6 },
    /*275-295 MHz*/
    {  275, 6 }, {  280, 6 }, {  285, 6 }, {  290, 6 }, {  295, 6 },
    /*300-320 MHz*/
    {  300, 6 }, {  305, 6 }, {  310, 6 }, {  315, 6 }, {  320, 6 },
    /*325-345 MHz*/
    {  325, 6 }, {  330, 6 }, {  335, 6 }, {  340, 6 }, {  345, 6 },
    /*350-370 MHz*/
    {  350, 6 }, {  355, 6 }, {  360, 6 }, {  365, 6 }, {  370, 6 },
    /*375- 395 MHz*/
    {  375, 6 }, {  380, 6 }, {  385, 6 }, {  390, 6 }, {  395, 6 },
    /*400-420 MHz*/
    {  400, 6 }, {  405, 6 }, {  410, 6 }, {  415, 6 }, {  420, 6 },
    /*425-445 MHz*/
    {  425, 6 }, {  430, 6 }, {  435, 6 }, {  440, 6 }, {  445, 6 },
    /*450-470 MHz*/
    {  450, 6 }, {  455, 6 }, {  460, 6 }, {  465, 6 }, {  470, 6 },
    /*475-495 MHz*/
    {  475, 6 }, {  480, 6 }, {  485, 6 }, {  490, 6 }, {  495, 6 },
    /*500-520 MHz*/
    {  500, 6 }, {  505, 6 }, {  510, 6 }, {  515, 6 }, {  520, 6 },
    /*525-545 MHz*/
    {  525, 6 }, {  530, 6 }, {  535, 6 }, {  540, 6 }, {  545, 6 },
    /*550-570 MHz*/
    {  550, 6 }, {  555, 6 }, {  560, 6 }, {  565, 6 }, {  570, 6 },
    /*575-595 MHz*/
    {  575, 6 }, {  580, 6 }, {  585, 6 }, {  590, 6 }, {  595, 6 },
    /*600-620 MHz*/
    {  600, 6 }, {  605, 6 }, {  610, 6 }, {  615, 6 }, {  620, 6 },
    /*625-645 MHz*/
    {  625, 6 }, {  630, 6 }, {  635, 6 }, {  640, 6 }, {  645, 6 },
    /*650-670 MHz*/
    {  650, 6 }, {  655, 6 }, {  660, 6 }, {  665, 6 }, {  670, 6 },
    /*675-695 MHz*/
    {  675, 6 }, {  680, 6 }, {  685, 6 }, {  690, 6 }, {  695, 6 },
    /*700-720 MHz*/
    {  700, 6 }, {  705, 6 }, {  710, 6 }, {  715, 6 }, {  720, 6 },
    /*725-745 MHz*/
    {  725, 6 }, {  730, 6 }, {  735, 6 }, {  740, 6 }, {  745, 6 },
    /*750-770 MHz*/
    {  750, 6 }, {  755, 6 }, {  760, 6 }, {  765, 6 }, {  770, 6 },
    /*775-795 MHz*/
    {  775, 6 }, {  780, 6 }, {  785, 6 }, {  790, 6 }, {  795, 6 },
    /*800-820 MHz*/
    {  800, 6 }, {  805, 6 }, {  810, 6 }, {  815, 6 }, {  820, 6 },
    /*825-845 MHz*/
    {  825, 6 }, {  830, 6 }, {  835, 6 }, {  840, 6 }, {  845, 6 },
    /*850-870 MHz*/
    {  850, 6 }, {  855, 6 }, {  860, 6 }, {  865, 6 }, {  870, 6 },
    /*875-895 MHz*/
    {  875, 6 }, {  880, 6 }, {  885, 6 }, {  890, 6 }, {  895, 6 },
    /*900-920 MHz*/
    {  900, 6 }, {  905, 6 }, {  910, 6 }, {  915, 6 }, {  920, 6 },
    /*925-945 MHz*/
    {  925, 6 }, {  930, 6 }, {  935, 6 }, {  940, 6 }, {  945, 6 },
    /*950-970 MHz*/
    {  950, 6 }, {  955, 6 }, {  960, 6 }, {  965, 6 }, {  970, 6 },
    /*975-995 MHz*/
    {  975, 6 }, {  980, 6 }, {  985, 6 }, {  990, 6 }, {  995, 6 },
    /*1000-1020 MHz*/
    { 1000, 6 }, { 1005, 6 }, { 1010, 6 }, { 1015, 6 }, { 1020, 6 },
    /*1025-1045 MHz*/
    { 1025, 6 }, {  515, 3 }, { 1035, 6 }, {  520, 3 }, { 1045, 6 },
    /*1050-1070 MHz*/
    {  175, 1 }, { 1055, 6 }, {  530, 3 }, { 1065, 6 }, {  535, 3 },
    /*1075-1095 MHz*/
    { 1075, 6 }, {  180, 1 }, { 1085, 6 }, {  545, 3 }, { 1095, 6 },
    /*1100-1120 MHz*/
    {  550, 3 }, { 1105, 6 }, {  555, 3 }, { 1115, 6 }, {  560, 3 },
    /*1125-1145 MHz*/
    { 1125, 6 }, {  565, 3 }, { 1135, 6 }, {  190, 1 }, { 1145, 6 },
    /*1150-1170 MHz*/
    {  575, 3 }, {  385, 2 }, {  580, 3 }, { 1165, 6 }, {  195, 1 },
    /*1175-1195 MHz*/
    { 1175, 6 }, {  590, 3 }, {  395, 2 }, {  595, 3 }, { 1195, 6 },
    /*1200 MHz*/
    {  200, 1 },
};


static void tty_reinit_device(struct cgpu_info *cgpu);
static void tty_flush_work(struct cgpu_info *cgpu);
void tty_reinit(void);

void pabort ( const char *s )
{
    perror ( s );
    abort();
}

static inline bool wq_enqueue(struct work_queue *wq, struct work *work)
{
    if (work == NULL)
        return false;
    struct work_ent *we = malloc(sizeof(*we));
    if (we == NULL)
        return false;
    we->work = work;
    INIT_LIST_HEAD(&we->head);
    list_add_tail(&we->head, &wq->head);
    wq->num_elems++;
    return true;
}


static inline struct work *wq_dequeue(struct work_queue *wq)
{
    if (wq == NULL)
        return NULL;
    if (wq->num_elems == 0)
        return NULL;
    struct work_ent *we;
    we = list_entry(wq->head.next, struct work_ent, head);
    struct work *work = we->work;
    list_del(&we->head);
    free(we);
    wq->num_elems--;
    return work;
}


/********** temporary helper for hexdumping SPI traffic */
#define DEBUG_HEXDUMP    1
static void hexdump(char *prefix, uint8_t *buff, int len)
{
#if DEBUG_HEXDUMP
    static char line[2048];
    char        *pos = line;
    int         i;
    if (len < 1)
        return;
    pos += sprintf(pos, "%s: %d bytes:", prefix, len);
    for (i = 0; i < len; i++) {
        if ((i % 32) == 0)
            pos += sprintf(pos, "\n");
        pos += sprintf(pos, "%.2X ", buff[i]);
    }
    applog(LOG_WARNING, "%s", line);
#endif
}

void tty_ConfigureD12PLLClock(int optPll)
{
    int i;
    /* default pll */
    int D12PllIdx = PLL_DEFAULT_IDX;

    if (optPll > 0)
    {
        for (i = 1; i < ARRAY_NUM(PllTbl_250_1200); ++i)
        {
            /* i-1 <= val <= i */
            if ((optPll <= (PLL_CLOCK_OUT_MIN + i * PLL_CLOCK_OUT_STEP)) &&
                (optPll >= PLL_CLOCK_OUT_MIN + (i - 1) * PLL_CLOCK_OUT_STEP))
            {
                if (PLL_CLOCK_OUT_MIN + i * PLL_CLOCK_OUT_STEP - optPll >
                    (PLL_CLOCK_OUT_STEP >> 1))
                    D12PllIdx = i - 1;
                else
                    D12PllIdx = i;
                break;
            }
        }
    }
    if (PllTbl_250_1200[D12PllIdx].pll_fb > 1023)
        D12PllIdx = PLL_DEFAULT_IDX;
    SET_PLL_CLK(D12PllIdx);
    applog(LOG_WARNING,
           "fb %04x pr %02x",
           PllTbl_250_1200[D12PllIdx].pll_fb,
           PllTbl_250_1200[D12PllIdx].pll_pr);
}

#define WRITE_JOB_LENGTH    57

/********** D12 low level functions */
static void tty_hw_reset(void)
{
    //applog(LOG_NOTICE, "hardware reset start");
    //h3_gpio_reset_mcu();
    //applog(LOG_NOTICE, "hardware reset done");
}


/********** job creation and result evaluation */
static uint8_t *create_job(uint8_t chip_id, uint8_t job_id, struct work *work)
{
    static uint8_t job[WRITE_JOB_LENGTH] =
    {
        /* command */
        0x3c, 0xff, 0x3f, 0x20,/*jobid<<5, offset 2,3*/
        0x00, 0x00, 0x00, 0x00,/*0 init nonce offset 4,5,6,7*/
        0xff, 0xff, 0x00, 0x1d, //1 difficulty offset 8 9 10 11
        /* midstate offset 12-43 */
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00,
        /* wdata offset 44-55*/
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00,
        0xc3//offset 56
    };
    uint8_t        *midstate = work->midstate;
    uint8_t        *wdata    = work->data + 64;
    job[1] = chip_id;
    job[3] = (job_id<<5);
    memcpy(&job[8], &work->target[24], 4);
    memcpy(&job[12], midstate, 32);
    memcpy(&job[44], wdata, 12);
    uint8_t        temp[10] = {0};
    int i = 0;
    int k = 0;
    for (i = 0; i < WRITE_JOB_LENGTH; i++)
    {
        if (i%4 == 0)
        {
            sprintf(temp+k, "\n");
            applog(LOG_WARNING, "%s\n", temp);
            k = 0;
        }
        k += sprintf(temp+k, "%02x ", job[i]); 
    }
    sprintf(temp+k, "\n");
    applog(LOG_WARNING, "%s", temp); 
    return job;

}


/* set work for given chip, returns true if a nonce range was finished */
static inline bool set_work(struct tty_chain *d12, uint8_t chip_id, struct work *work)
{
    uint8_t job[WRITE_JOB_LENGTH] =
    {
        /* command */
        0x3c, 0xff, 0x3f, 0x20,/*jobid<<5, offset 2,3*/
        0x00, 0x00, 0x00, 0x00,/*0 init nonce offset 4,5,6,7*/
        0xff, 0xff, 0x00, 0x1d, //1   difficulty offset 8 9 10 11
        /* midstate offset 12-43 */
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00,
        /* wdata offset 44-55*/
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00,
        0xc3//offset 56
    };
    
    uint8_t        *midstate = work->midstate;
    uint8_t        *wdata    = work->data + 64;
    struct tty_chip *chip;
    bool            retval = false;

    if (!d12 || (d12->chips == NULL) || (chip_id < 1))
        return false;
    chip = &d12->chips[chip_id - 1];
    int job_id = chip->last_queued_id;
    
    applog(LOG_NOTICE, "chip id %d, job id % d", chip_id, job_id);

    if (chip->work[job_id] != NULL)
    {
        work_completed(d12->cgpu, chip->work[job_id]);
        chip->work[job_id] = NULL;
        retval = true;
    }

    work->chipid = chip_id;
    applog(LOG_DEBUG, "target 6 %08x", ((uint32_t*)work->target)[6]);
    job[1] = chip_id;
    job[3] = (job_id<<5);
    
    memcpy(&job[8], &work->target[24], 4);
    memcpy(&job[12], midstate, 32);
    memcpy(&job[44], wdata, 12);

    applog(LOG_DEBUG, "writejob: d12:%p, ctx:%p, fd:%d", d12, d12->ctx, d12->ctx->fd);
    
    if (!write_job(d12->ctx->fd, job, WRITE_JOB_LENGTH))
    {
        /* give back work */
        work_completed(d12->cgpu, work);
        applog(LOG_ERR, "error set work for chip %d.%d", chip_id, job_id);
    }
    else
    {
        cgtime(&chip->job_start_time);
        chip->work[job_id] = work;
        chip->last_queued_id++;
        chip->last_queued_id %= MAX_JOB;
        applog(LOG_ERR, "set work for chip %d.%d\n", chip_id, job_id);
    }
    return retval;
}



/* reset input work queues in chip chain */
static bool abort_work(struct tty_chain *d12)
{
    /*
     * for now, the proposed input queue reset does not seem to work
     * TODO: implement reliable abort method
     * NOTE: until then, we are completing queued work => stales
     */
    return true;
    int     i;
    applog(LOG_WARNING, "abort_work...");
    uint8_t reg[6];
    memset(reg, 0, 6);
    reg[0] = (gPllClk >> 8) & 0xff;
    reg[1] = (gPllClk & 0xff);
    reg[2] = setting.volt;
    reg[3] = 0;
    reg[4] = 0;
    reg[5] = 0;
    if (!cmd_WRITE_REG(d12, 0, reg))
        applog(LOG_ERR, "Failed to write reg");
    applog(LOG_WARNING, "%s %04x", __func__, gPllClk);
    return cmd_RESET_BCAST(d12, 0);
}


/********** driver interface */
static void tty_exit(tty_ctx * ctx)
{
    if (ctx->fd != -1)
        close(ctx->fd);
    free(ctx);
}

static tty_ctx *tty_init(const char *devname, speed_t baud)
{
    tty_ctx * ctx = 0;
    struct termios	my_termios;
    bool ret = false;
    do
    {
        ctx = malloc(sizeof(tty_ctx));
        if(!ctx) break;
        ctx->baud = baud;
        ctx->fd = jdasic_open(devname, baud);
        applog(LOG_NOTICE, "tty_init:devname:%s ctx :%p, fd:%d\n", devname, ctx, ctx->fd);
        if (ctx->fd == -1) break;
        strcpy(ctx->devname,devname);
        ret = true;
    }while(0);
    if (ret)
        return ctx;
    if (ctx)
    {
        free(ctx);
        return (void*)0;
    }
}

struct tty_chain *init_tty_chain(tty_ctx *ctx)
{
    struct tty_chain *d12 = 0;
    bool ret = false;
    applog(LOG_DEBUG, "D12 init chain\n");
    do
    {
        d12 = calloc(1, sizeof(*d12));
        if(!d12) {
            applog(LOG_ERR, "tty_chain allocation error");
            break;
        }
        d12->ctx  = ctx;
        //d12->status   = ASIC_BOARD_OK;
        d12->shutdown = 0;

        ret = detect_board(ctx->fd,ctx->devname);
        if(!ret) 
            break;

        applog(LOG_NOTICE, "detected board on %s interface\n", ctx->devname);

        if(!jdasic_config_btc(ctx->fd, setting.currentFrequency/*freq*/)) 
            break;
        
        d12->num_chips = setting.chipNumber;//MAX_ASIC_NUM_PER_BOARD;
        d12->num_cores = MAX_CORE_NUMS;
        d12->accept_time = 0;
        d12->chips = calloc(d12->num_chips, sizeof(struct tty_chip));
        if (!d12->chips)
        {
            applog(LOG_ERR, "tty chips allocation error");
            break;
        }
        mutex_init(&d12->lock);
        INIT_LIST_HEAD(&d12->active_wq.head);
        ret = true;
    }while(0);
    if (ret)
        return d12;
    if (d12)
    {
        free(d12);
        return 0;
    }
}


static bool tty_detect_one_chain(const char * tty_devname, int buad)
{
    struct cgpu_info *cgpu = 0;
    tty_ctx   *ctx = 0;
    struct tty_chain *d12 = 0;
    bool ret = false;

    do
    {
        applog(LOG_NOTICE,"devname = %s,baud = %d\n",tty_devname,buad);
        ctx = tty_init(tty_devname, buad);
        if (!ctx)
            break;
        d12 = init_tty_chain(ctx);
        if (!d12)
           break;
        cgpu = malloc(sizeof(*cgpu));
        applog(LOG_NOTICE, "detect_one_chain:devname:%s,ctx:%p(%p), d12:%p, cgpu:%p  fd:%d(%d)\n", tty_devname, ctx, d12->ctx, d12, cgpu, d12->ctx->fd, ctx->fd);
        if(cgpu == NULL)
           break;
        memset(cgpu, 0, sizeof(*cgpu));
        cgpu->drv         = &bitmineB11S_drv;
        cgpu->name        = "bitmineB11S";
        cgpu->threads     = 1;
        cgpu->device_data = d12;
        cgpu->deven = DEV_ENABLED;
        d12->cgpu   = cgpu;
        d12->led_val = 0;
        d12->status = BOARD_RUNNING;
        cgtime(&d12->sendlast48_t);
        cgtime(&d12->last_get_nonce_t);
        cgtime(&d12->last_flash_t);
        add_cgpu(cgpu);
        ret = true;
    }while(0);
    if (ret)
        return ret;
    if (d12)
    {
        if (d12->chips) free(d12->chips);
        free(d12);
    }
    if(ctx)
    {
        tty_exit(ctx);
    }
    return ret;

}

static bool CheckForAnotherInstance()
{
    int fd;
    struct flock fl;
    fd = open ( "/tmp/cgminer.lock", O_RDWR | O_CREAT, 0662 );

    if ( fd == -1 )
    {
        return false;
    }

    fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
    fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
    fl.l_start  = 0;        /* Offset from l_whence         */
    fl.l_len    = 0;        /* length, 0 = to EOF           */
    fl.l_pid    = getpid(); /* our PID                      */

    // try to create a file lock
    if ( fcntl ( fd, F_SETLK, &fl ) == -1 ) /* F_GETLK, F_SETLK, F_SETLKW */
    {
        // we failed to create a file lock, meaning it's already locked //
        if ( errno == EACCES || errno == EAGAIN )
        {
            return false;
        }
    }
    return true;
}


#ifdef  USE_SPI
void tty_detect(bool hotplug)
{
    uint8_t  no_cgpu = 0;
    const char devname[] ={"/dev/spidev1.0"};
    const int diff_table[16] = {
        1, 2, 4, 8, 16, 32, 52, 64, 86, 103, 128, 256, 512, 1024, 2048, 4096
    };
    if (!CheckForAnotherInstance())
    {
        pabort("Another cgminer is running");
        return;
    }

    applog(LOG_DEBUG, "D12S detect");

    //tty_ConfigureD12PLLClock(opt_D12Pll);
    if ((opt_diff >= 1) && (opt_diff <= 16))
        applog(LOG_NOTICE, "ASIC Difficulty =  %d", diff_table[opt_diff - 1]);
    else
        applog(LOG_NOTICE, "ASIC Difficulty =  %d", 2048);

    if (tty_detect_one_chain(devname, SPI_RATE)) 
        no_cgpu++;

    if (no_cgpu == 0) {
        applog(LOG_ERR, "No any board");
        exit(0);
    }
    else
        applog(LOG_NOTICE, "%d board[s] Found\n", no_cgpu);

}

#else
void tty_detect(bool hotplug)
{
    uint8_t  no_cgpu = 0;
    const char devname[][20] ={"/dev/ttyO2", 0};
    int kk = 0;

    if (!CheckForAnotherInstance())
    {
        pabort("Another cgminer is running");
        return;
    }

    applog(LOG_DEBUG, "D12S detect\n");

    //tty_ConfigureD12PLLClock(opt_D12Pll);
    const int diff_table[16] = {
        1, 2, 4, 8, 16, 32, 52, 64, 86, 103, 128, 256, 512, 1024, 2048, 4096
    };
    if ((opt_diff >= 1) && (opt_diff <= 16))
        applog(LOG_NOTICE, "ASIC Difficulty =  %d\n", diff_table[opt_diff - 1]);
    else
        applog(LOG_NOTICE, "ASIC Difficulty =  %d\n", 2048);

    while(devname[kk][0]) 
    {
        kk++;
        if (!tty_detect_one_chain(devname[kk - 1], B230400)) 
            continue;
        no_cgpu++;
    }
    if (no_cgpu == 0) {
        applog(LOG_ERR, "No any board");
        exit(0);
    }
    else
        applog(LOG_NOTICE, "%d board[s] Found\n", no_cgpu);

}
#endif

#if 0
void flush_work_by_board(struct tty_chain *d12, uint32_t board_id)
{

    int i = (board_id)*setting.chipNumber; 
    int j;
    for (; (i < d12->num_chips && i < (board_id + 1)*setting.chipNumber); i++)
    {
        //clean up works of chips
        struct tty_chip *chip = &d12->chips[i];
        for (j = 0; j < MAX_JOB; j++)
        {
            struct work *work = chip->work[j];
            if (work == NULL)
                continue;
            applog(LOG_DEBUG, "flushing chip %d, work %d: 0x%p", i, j + 1,
                   work);
            work_completed(d12->cgpu, work);
            chip->work[j] = NULL;
        }
        //cgsleep_ms(10000);
    }
}

bool has_board_workable2(void)
{
   int i;
   for (i = 0; i < bh.board_num; i++)
   {
       if (bh.board_status[i] == BOARD_OK || bh.board_status[i] == BOARD_RESETTING)
           break;
   }
   return (i < bh.board_num);
}

bool has_board_workable(void)
{
   int i;
   for (i = 0; i < bh.board_num; i++)
   {
       if (bh.board_status[i] == BOARD_OK)
           break;
   }
   return (i < bh.board_num);
}

bool is_chip_workable(int chip_id)
{
    int m = (chip_id - 1) / setting.chipNumber;
    return (bh.board_status[m] == BOARD_OK);
}

void reset_board_by_chipid(struct tty_chain * d12, uint32_t chip_id)
{
    int m = (chip_id - 1)/setting.chipNumber;
    applog(LOG_WARNING, "enter %s(%d:(%d:%d))", __func__, m, chip_id, setting.chipNumber);
    if (!is_chip_workable(chip_id))
    {
        applog(LOG_WARNING, "chip(%d) already in resetting status, just return");
        return;
    }
    bh.board_status[m] = BOARD_RESETTING;
    cgtime(&bh.board_reset_time[m]);
    if (!cmd_RESET_BCAST(d12, m + 1))
    {
        applog(LOG_ERR, "reset board %d failed", m);
    }
    applog(LOG_WARNING, "leave %s", __func__);
}

bool check_board_timeout(void)
{
    bool bcontinue = false;
    int i;
    for (i = 0; i < bh.board_num; i++)
    {
        struct timeval  check_board_to;
        cgtime(&check_board_to);
        if (bh.board_status[i] == BOARD_RESETTING && tdiff(&check_board_to, &bh.board_reset_time[i]) >= (setting.board_reset_waittime?: 40))
        {
            bh.board_status[i] = BOARD_OK;
            memset(&bh.board_reset_time[i], 0, sizeof(bh.board_reset_time[i]));
        }
        else if (bh.board_status[i] == BOARD_DISABLED && tdiff(&check_board_to, &bh.board_reenable_time[i]) >= (setting.board_reenable_waittime?:14))
        {
            bh.board_status[i] = BOARD_OK;
            memset(&bh.board_reenable_time[i], 0, sizeof(bh.board_reenable_time[i]));
        }
    }
    bcontinue = has_board_workable2();
    return bcontinue;
}


bool temperature_protect(struct tty_chain *d12, int chip_id, struct tty_chip *chip)
{
    bool bcontinue = false;
    do
    {
        if (!is_chip_workable(chip_id))
            break;
        if((chip_id%setting.chipNumber) != 0) 
        {
            bcontinue = true;
            break;
        }
        if(!cmd_READ_REG(d12, chip_id))
        {
            applog(LOG_ERR, "read chip(%d) reg failed", chip_id);
            break;
        }
        memcpy(&chip->reg, d12->spi_rx + 2, 6);
        //applog(LOG_WARNING, "reg: %02x %02x %02x %02x %02x %02x",
        //        d12->spi_rx[2], d12->spi_rx[3], d12->spi_rx[4],d12->spi_rx[5],
        //        d12->spi_rx[6], d12->spi_rx[7]);

        int bid = (chip_id - 1)/setting.chipNumber + 1;
        bh.temp[bid - 1] = chip->reg.temp;
        if (chip->reg.temp >= (setting.temp_threshold? : 80))
        {
            applog(LOG_WARNING, "trying to disable board %d", bid); 
            if(!cmd_RESET_BCAST(d12, 0x80|bid))
            {
                applog(LOG_ERR, "disable board %d failed", bid); 
            }
            else
            {
                applog(LOG_WARNING, "disable board %d ok", bid); 
            }
            bh.board_status[bid - 1] = BOARD_DISABLED;
            cgtime(&bh.board_reenable_time[bid-1]);
        }
        else
            bcontinue = has_board_workable();
    }while(0);
    return bcontinue;
}

static bool check_chipid_repeated(struct tty_chain* d12, int chipid)
{
    static int lastchipid = 0;
    static int  repeated_times = 0;
    bool bcontinue = true;
    if (lastchipid == chipid)
    {
        repeated_times++;
    }
    else
    {
        repeated_times = 1;
        lastchipid = chipid;
    }
    if (repeated_times > setting.invalid_cnt)
    {
        repeated_times = 0;
        applog(LOG_WARNING, "chipid_occurrence(%d) > %d", chipid, setting.invalid_cnt);
        reset_board_by_chipid(d12, chipid);
        bcontinue = has_board_workable2();
    }
    return bcontinue;
}
#endif


void *reset_board_thread( void *userdata )
{
    struct thr_info *mythr = userdata;
    struct cgpu_info *cgpu = mythr->cgpu;
    struct tty_chain *d12   = cgpu->device_data;
    char threadname[24];
    char cmd_buf[64] = {0};
    snprintf ( threadname, 24, "resetboard/%d", basename(d12->ctx->devname));
    RenameThread ( threadname );
    pthread_detach ( pthread_self() );
    if (d12->status ==  BOARD_HW_RESET)
    {
        applog(LOG_WARNING,"\e[48;5;22mreset done %s\e[0m", basename(d12->ctx->devname));
        jdasic_config_btc(d12->ctx->fd, setting.currentFrequency);
        applog(LOG_WARNING,"\e[48;5;2mboard running %s\e[0m", basename(d12->ctx->devname));
    }
    pull_up_payload(d12->ctx->fd);	
    sleep(1);
    d12->status = BOARD_RUNNING;
    return;
}

static int64_t tty_scanwork(struct thr_info *thr)
{
    int i, j;
    struct cgpu_info *cgpu  = thr->cgpu;
    struct tty_chain *d12   = cgpu->device_data;
    bool flag = false;
    int64_t accept = 0;
    uint32_t nonce;
    uint8_t chip_id, job_id;
    bool work_updated = false;
    struct timeval now_t;
    int time_diff;

    mutex_lock(&d12->lock);
    do 
    {
        if ((d12->chips == NULL) || d12->shutdown)
            break;
        
        if(read_nonce(d12->ctx->fd, &nonce, &chip_id, &job_id))
        {
            //handle the result
            cgtime(&d12->last_get_nonce_t);
            set_led(d12->ctx->devname[11]-48,1);
            work_updated = true;

            //wrong chip id or job id case
            if ((chip_id == 0) || (chip_id > d12->num_chips) || (job_id < 0) || (job_id >= MAX_JOB))
            {
                d12->invalid_chipid_cnt++;
                continue;
            }
            
            struct tty_chip *chip = &d12->chips[chip_id - 1];
            memset(&chip->job_start_time, 0, sizeof(chip->job_start_time));

            //duplicate nonce case
            if(nonce == chip->lastnonce)
                continue;

            // keep the nonce
            chip->lastnonce = nonce;           

            // pack the nonce into the work
            struct work *work = chip->work[job_id];
            if (work == NULL)
            {
                applog(LOG_WARNING, "stale nonce 0x%08x(%s:%d)",  nonce, d12->ctx->devname, chip_id);
                chip->stales++;
                continue;
            }
            strncpy(work->ttydev_name, basename(d12->ctx->devname),sizeof(work->ttydev_name));
            work->nonce = nonce; 

            if (!submit_nonce(thr, work, nonce))
            {
                //handle the wrong case
                unsigned char data[10] = {0};
                chip->hw_errors++;
                applog(LOG_ERR, "invalid nonce [%s:%d:%d(%d):%d:%d] 0x%08x", basename(d12->ctx->devname), chip_id,job_id, chip->hw_errors, \
                                                d12->invalid_jobid_cnt,d12->invalid_chipid_cnt, nonce);
                if(d12->status == BOARD_RUNNING)
                {
                    d12->invalid_nonce_cnt++;
                    //check if need reset board 
                    if (d12->invalid_nonce_cnt > setting.invalid_cnt)
                    {
                        applog(LOG_WARNING,"\e[48;5;0mrestart [invalid=%d]%s\e[0m",d12->invalid_nonce_cnt, basename(d12->ctx->devname));
                        //avoid others change status to HW_RESET again 
                        d12->invalid_nonce_cnt = 0;
                        d12->status = BOARD_HW_RESET;
                        disable_core_bystep(d12->ctx->fd);
                        set_linux_uart_baudrate(d12->ctx->fd, B230400);
                        hw_init(d12->ctx->devname);
                        if ( unlikely ( thr_info_create ( thr, NULL, reset_board_thread, thr ) ) )
                            applog(LOG_ERR,"\e[48;5;9m%screate reset board thread failed\e[0m", basename(d12->ctx->devname));
                    }
                }
                inc_hw_errors(thr);
                break;
            }
            
            accept += (int64_t)work->sdiff;
        }
        else
        {
            cgtime(&now_t);
            time_diff = tdiff(&now_t, &d12->last_get_nonce_t);
        }

        cgtime(&now_t);
        if (ms_tdiff(&now_t,&d12->sendlast48_t) > opt_jobtimeout)
        {
            struct work *work;
            cgtime(&d12->sendlast48_t);
            
            for (i = d12->num_chips; i > 0; i--)
            {
                if (d12->status != BOARD_RUNNING)
                    break;

                work_updated = true;
                work = wq_dequeue(&d12->active_wq);

                if (work != NULL)
                {
                    cgsleep_us(setting.task_interval ? : 360);
                    set_work(d12, i, work);
                }
            }
            
        /*
            * some pool (f2pool etc.) the work difficulty is so large so it will takes more time for the machine to get a nonce.
            * therefore the "multiple" should be changed to prevent the machine restart frequently
           */
              
            if (work && work->work_difficulty > 1024*6)
                multiple = 4;
        }
    }while(0);
    
    if (!work_updated)
        cgsleep_ms(setting.scanwork_sleeptime? : 6);

    mutex_unlock(&d12->lock);

    if (accept < 0)
        accept = 0;

    return ((int64_t) accept<< 32);
}

/* queue one work per chip in chain */

static inline bool tty_queue_full(struct cgpu_info *cgpu)
{
    struct tty_chain *d12       = cgpu->device_data;
    int    queue_full = false;
    mutex_lock(&d12->lock);
    if (d12->active_wq.num_elems >= d12->num_chips)
        queue_full = true;
    else
      wq_enqueue(&d12->active_wq, get_queued(cgpu));
    mutex_unlock(&d12->lock);
    return queue_full;
}

static void tty_flush_work(struct cgpu_info *cgpu)
{
    struct tty_chain *d12 = cgpu->device_data;

    applog(LOG_DEBUG, "D12 running flushwork");
    int i;
    if (d12->chips == NULL)
        return;
    mutex_lock(&d12->lock);
    /* stop chips hashing current work */
    if (!abort_work(d12))
        applog(LOG_ERR, "failed to abort work in chip chain!");
    /* flush the work chips were currently hashing */
    for (i = 0; i < d12->num_chips; i++)
    {
        int             j;
        struct tty_chip *chip = &d12->chips[i];
        for (j = 0; j < MAX_JOB; j++)
        {
            struct work *work = chip->work[j];
            if (work == NULL)
                continue;
            applog(LOG_DEBUG, "flushing chip %d, work %d: 0x%p", i, j + 1,work);
            work_completed(cgpu, work);
            chip->work[j] = NULL;
        }
    }
    mutex_unlock(&d12->lock);
}


#ifdef API_STATE_EN
static void tty_reinit_device(struct cgpu_info *cgpu)
{
    struct tty_chain *d12 = cgpu->device_data;
    struct thr_info  *thr = cgpu->thr[0];
    //d12->status   = ASIC_BOARD_OK;
    d12->shutdown = 0;
    //  cgtime(&thr->last);
    int i, j, num_chips = 0;
    applog(LOG_WARNING, "%s %04x", __func__, gPllClk);
    if (!cmd_RESET_BCAST(d12, 0))
        goto reinit_failure;
    if (!cmd_BIST_START(d12))
        goto reinit_failure;
    // Set zero cores before check num chips
    for (i = 0; i < d12->num_chips; i++)
    {
        d12->chips[i].num_cores         = 0;
    }
    d12->num_cores = 0;
    //d12->num_chips = (d12->tty_rx[2]<<8)|(d12->spi_rx[3]);
    applog(LOG_WARNING,
           "ttydev%s: Found %d D12 chips",
           d12->ctx->devname,
           d12->num_chips);
    if (d12->num_chips == 0)
        goto reinit_failure;
    if (d12->chips)
        free(d12->chips);
    d12->chips = NULL;
    d12->chips = calloc(d12->num_chips, sizeof(struct tty_chip));
    if (d12->chips == NULL)
    {
        applog(LOG_ERR, "tty_chips allocation error");
        goto reinit_failure;
    }
    //if(!cmd_BIST_FIX_BCAST(d12))
    //    goto reinit_failure;
    //if (d12->num_chips < MAX_ASIC_NUMS)
    //    d12->status |= ERROR_CHIP;
    for (i = 0; i < d12->num_chips; i++)
    {
        int chip_id = i + 1;
        if (!cmd_READ_REG(d12, chip_id))
        {
            applog(LOG_WARNING,
                   "Failed to read register for " "chip %d -> disabling",
                   chip_id);
            d12->chips[i].num_cores = 0;
            continue;
        }
        num_chips++;
#if 0
        memcpy(&d12->chips[i].reg, d12->spi_rx + 2, 6);
        d12->chips[i].num_cores = d12->chips[i].reg.goodCores;
        if (d12->chips[i].reg.goodCores > MAX_CORE_NUMS)
            d12->chips[i].num_cores = MAX_CORE_NUMS;
        d12->num_cores += d12->chips[i].num_cores;
#endif
        applog(LOG_WARNING,
               "Found chip %d with %d active cores",
               chip_id,
               d12->chips[i].num_cores);
        for (j = 0; j < MAX_JOB; j++)
            d12->chips[i].work[j] = NULL;
        //assign chip id
        //start from 0 .. chip_num -1
        d12->chips[i].id = i;
    }
    applog(LOG_WARNING,
           "Found %d chips with total %d active cores",
           d12->num_chips,
           d12->num_cores);
    if (d12->num_cores == 0)     // The D12 board  haven't any cores

        goto reinit_failure;
    cgpu->deven = DEV_ENABLED;
    return;
 reinit_failure:
    // Set zero cores before check num chips
    for (i = 0; i < d12->num_chips; i++)
        d12->chips[i].num_cores = 0;
    if (d12->chips)
        free(d12->chips);
    d12->chips  = NULL;
    cgpu->deven = DEV_DISABLED;
    //  cgpu->deven = DEV_ENABLED;
    //d12->status = ERROR_BOARD;
}


void tty_reinit(void)
{
    int              i;
    struct tty_chain *d12;
    struct cgpu_info *cgpu;

    applog(LOG_WARNING, "tty_reinit");
    rd_lock(&devices_lock);
    for (i = 0; i < total_devices; ++i)
    {
        cgpu = devices[i];
        d12  = cgpu->device_data;
        applog(LOG_WARNING, "%d,%d,%d", cgpu->deven, cgpu->status, d12->status);
        cgpu->deven = DEV_RECOVER;
        //      d12->reinit=1;
    }
    rd_unlock(&devices_lock);
}
void tty_shutdown(void)
{
    int              i;
    struct tty_chain *d12;
    struct cgpu_info *cgpu;

    applog(LOG_WARNING, "tty_shutdown");
    rd_lock(&devices_lock);
    for (i = 0; i < total_devices; ++i)
    {
        cgpu          = devices[i];
        d12           = cgpu->device_data;
        d12->shutdown = 1;
    }
    rd_unlock(&devices_lock);
}


static void tty_get_statline(char *buf, size_t bufsiz, struct cgpu_info *cgpu)
{
    struct tty_chain *d12 = cgpu->device_data;
    int              i;
    char             strT[100], *pStrT;
    float            avgTemp  = 0, maxTemp = 0;
    bool             overheat = false;

    // Warning, access to these is not locked - but we don't really
    // care since hashing performance is way more important than
    // locking access to displaying API debug 'stats'
    // If locking becomes an issue for any of them, use copy_data=true also
    if (d12->chips == NULL)
        //      tailsprintf(buf, bufsiz, " | T:--C");
        return;
    if (cgpu->deven != DEV_ENABLED)
        return;
    avgTemp = d12->chips[0].reg.temp;
    pStrT   = strT;
    sprintf(pStrT, "%02d", d12->chips[0].reg.temp);
    pStrT += 2;
    for (i = 1; i < d12->num_chips; i++)
    {
        if (maxTemp < d12->chips[i].reg.temp)
            maxTemp = d12->chips[i].reg.temp;
        if (d12->cutTemp < d12->chips[i].reg.temp)
            overheat = true;
        avgTemp += d12->chips[i].reg.temp;
        sprintf(pStrT, "-%02d", d12->chips[i].reg.temp);
        pStrT += 3;
    }
    avgTemp /= d12->num_chips;
    if (d12->shutdown)
    {
        tailsprintf(buf,
                    bufsiz,
                    " | T:%2.0fC(%s)(shutdown=%d)",
                    maxTemp,
                    strT,
                    d12->shutdown);
    }
    else if (overheat)
    {
        tailsprintf(buf, bufsiz, " | T:%2.0fC*Hot*(%s)", maxTemp, strT);
    }
    else
    {
        tailsprintf(buf, bufsiz, " | T:%2.0fC     (%s)", maxTemp, strT);
    }
}


static void tty_statline_before(char *buf, size_t bufsiz,
                                struct cgpu_info *cgpu)
{
    struct tty_chain *d12 = cgpu->device_data;
    char             strT[100], *pStrT;
    int              i;

    if (d12->chips == NULL)
        return;
    if (cgpu->deven != DEV_ENABLED)
        return;

    tailsprintf(buf, bufsiz, "A:%d-C:%03d | ", d12->num_chips, d12->num_cores);
}


#ifdef API_STATE_EN
static struct api_data *tty_api_stats(struct cgpu_info *cgpu)
{
    struct api_data  *root = NULL;
    struct tty_chain *d12  = cgpu->device_data;
    int              i;
    char             strT[100], *pStrT;
    float            avgTemp = 0;

    if (d12->chips == NULL)
    {
        root = api_add_string(root, "Temperature", strT, false);
        return root;
    }
    // Warning, access to these is not locked - but we don't really
    // care since hashing performance is way more important than
    // locking access to displaying API debug 'stats'
    // If locking becomes an issue for any of them, use copy_data=true also
    //root = api_add_int(root, "ASIC", &(d12->num_chips), false);
    //root = api_add_int(root, "CORES(TOTAL)", &(d12->num_cores), false);
    pStrT = strT;
    sprintf(pStrT, "%02d", d12->chips[0].num_cores);
    pStrT += 2;
    for (i = 1; i < d12->num_chips; i++)
    {
        if (d12->chips[i].num_cores)
            sprintf(pStrT, "-%02d", d12->chips[i].num_cores);
        else
            sprintf(pStrT, "- F");
        pStrT += 3;
    }
    //root = api_add_string(root, "CORES(SOLO)", strT, true);
    pStrT = strT;
    sprintf(pStrT, "%02d", d12->chips[0].reg.temp);
    pStrT += 2;
    for (i = 1; i < d12->num_chips; i++)
    {
        if (d12->chips[i].reg.temp)
        {
            avgTemp += d12->chips[i].reg.temp;
            sprintf(pStrT, "-%02d", d12->chips[i].reg.temp);
        }
        else
        {
            sprintf(pStrT, "- F");
        } pStrT += 3;
    }
    avgTemp /= d12->num_chips;
    root     = api_add_temp(root, "TEMP(AVG)", &avgTemp, true);
    root     = api_add_string(root, "TEMP(SOLO)", strT, true);
    return root;
}
#endif

#ifdef HAVE_CURSES

#define CURBUFSIZ    256
#define cg_mvwprintw(win, y, x, fmt, ...)                    \
    do {                                                     \
        char tmp42[CURBUFSIZ];                               \
        snprintf(tmp42, sizeof(tmp42), fmt, ## __VA_ARGS__); \
        mvwprintw(win, y, x, "%s", tmp42);                   \
    } while (0)
#define cg_wprintw(win, fmt, ...)                            \
    do {                                                     \
        char tmp42[CURBUFSIZ];                               \
        snprintf(tmp42, sizeof(tmp42), fmt, ## __VA_ARGS__); \
        wprintw(win, "%s", tmp42);                           \
    } while (0)


extern WINDOW *mainwin, *statuswin, *logwin;
void tty_curses_print_status(int y)
{
      applog(LOG_WARNING, "tty_curses_print_status");
    struct tty_chain *d12;
    struct cgpu_info *cgpu;
    int              i, testing = 0, errHw = 0, errBoard = 0, errOverheat = 0,
                     errChip = 0, errCore = 0, errSenor = 0, numChips = 0;
    char             strT[100], *pStrT;

    //Note: not allow write any data to  devices[i]
    rd_lock(&devices_lock);
    for (i = 0; i < total_devices; ++i)
    {
        cgpu = devices[i];
        d12  = cgpu->device_data;
        if (cgpu->deven == DEV_RECOVER)
        {
            testing = 1;
            break;
        }
#if 0
        if (d12->status &
            (ERROR_BOARD | ERROR_CHIP | ERROR_CORE | ERROR_TEMP_SENSOR))
        {
            errHw = 1;
            if (d12->status & ERROR_CHIP)
                errChip = 1;
            if (d12->status & ERROR_CORE)
                errCore = 1;
            if (d12->status & ERROR_TEMP_SENSOR)
                errSenor = 1;
            if (d12->status & ERROR_BOARD)
                errBoard = 1;
            if (d12->status & ERROR_OVERHEAD)
                errOverheat = 1;
        }
        if (d12->status == ASIC_BOARD_OK)
            numChips++;
#endif
    }
    rd_unlock(&devices_lock);
    if (numChips < MAX_ASIC_BOARD)
    {
        //      errHw=1;
        //  errBoard=1;
    }
    ++y;
    mvwhline(statuswin, y, 0, '-', 80);
    if (total_devices)
    {
        if (testing)
        {
            cg_mvwprintw(statuswin, ++y, 1, "Testing .....");
        }
        else if (errHw)
        {
            pStrT = strT;
            sprintf(pStrT, "Hardware: ");
            pStrT += strlen("Hardware: ");
            if (errBoard)
            {
                sprintf(pStrT, "ASIC board error,");
                pStrT += strlen("ASIC board error,");
            }
            if (errChip)
            {
                sprintf(pStrT, "Chip error,");
                pStrT += strlen("Chip error,");
            }
            if (errCore)
            {
                sprintf(pStrT, "Core error,");
                pStrT += strlen("Core error,");
            }
            if (errSenor)
            {
                sprintf(pStrT, "Senor error,");
                pStrT += strlen("Senor error,");
            }
            if (errOverheat)
            {
                sprintf(pStrT, "Overheat");
                pStrT += strlen("Overheat");
            }
            cg_mvwprintw(statuswin, ++y, 1, "%s", strT);
        }
        else
        {
            cg_mvwprintw(statuswin, ++y, 1, "Hardware testing: Passed");
        }
    }
    else
    {
        cg_mvwprintw(statuswin, ++y, 1, "Test finished: No any ASIC board");
    } wclrtoeol(statuswin);
    ++y;
    mvwhline(statuswin, y, 0, '-', 80);
    //wattroff(statuswin, A_BOLD);
    wclrtoeol(statuswin);
}
#endif

void tty_stop_test(struct cgpu_info *cgpu)
{
    struct tty_chain *d12 = cgpu->device_data;
    //    d12->status  = (d12->status & (~ASIC_BOARD_TESTING));
}


static bool tty_get_stats(struct cgpu_info *cgpu)
{
    struct tty_chain *d12 = cgpu->device_data;
    int              i, overheatCnt;
    char             strT[100], *pStrT;
    float            avgTemp  = 0, maxTemp = 0;
    bool             overheat = false;

    // Warning, access to these is not locked - but we don't really
    // care since hashing performance is way more important than
    // locking access to displaying API debug 'stats'
    // If locking becomes an issue for any of them, use copy_data=true also
    if (d12->chips == NULL)
    {
        //      cgpu->deven=DEV_DISABLED;
        //cgpu->status= LIFE_NOSTART;
        //      d12->status=NO_ASIC_BOARD;
        return true;
    }
    if (d12->num_chips < MAX_ASIC_NUMS)
    {
        //      cgpu->status = LIFE_DEAD;
        //d12->status |= ERROR_CHIP;
    }
    else
    {
        if (d12->num_cores < (MAX_ASIC_NUMS * MAX_CORE_NUMS))
            //          cgpu->status = LIFE_SICK;
            //d12->status |= ERROR_CORE;
        for (i = 0; i < d12->num_chips; i++)
        {
            //if (d12->chips[i].reg.temp == 0)
                //              cgpu->status = LIFE_SICK;
                //d12->status |= ERROR_TEMP_SENSOR;
        }
    }
    /*  Check temperature, then stop if
     *  2) For two or more asic chip has temperature higher than
     *      the argu value for 5 seconds, it stop submitting job
     *      for the whole board forever.
     *
     *  3) For only one chip has temperature 10 degree higher than
     *     the argu value for 5 seconds, it stop submitting job for
     *     the whole board forever.
     *
     *  4) even only one chip suddenly 15 degree higher than the
     *     argu value, it stop immediately.
     */
    return true;
}
#endif

struct device_drv bitmineB11S_drv =
{
    .drv_id = DRIVER_bitmineB11S,
    .dname = "bitmineB11S",
    .name = "D12S",
    .drv_detect = tty_detect,
    .hash_work = hash_queued_work,
    .scanwork = tty_scanwork,
    .queue_full = tty_queue_full,
    //.flush_work = tty_flush_work,
    //.reinit_device = tty_reinit_device,
};



