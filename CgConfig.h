/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CgConfig.h
 * Author: will
 *
 * Created on February 27, 2016, 4:25 PM
 */

#ifndef CGCONFIG_H
#define CGCONFIG_H
#include "miner.h"
#ifdef __cplusplus
extern "C" {
#endif
    
#define CONLINELENGTH 1600
#define CONFIG_FILE_PATH "/etc/cgminer/conf.default"

    typedef struct _poolConf {
        char *url;
        char *user;
        char *pass;
        char *allSetting;
    }PoolConf;
    
     typedef struct _frequencyConf {
        int  frequency;
        int jobtime;
    }FrequencyConf;
    
     typedef struct _setting {
         PoolConf pool[3];
         int portWeb;
         int portApi;
         bool failover_only;
         bool no_submit_stale;
         bool api_listen;
         bool autoFrequency;
         bool autoGetJobTimeOut;
         bool isdebug;
         char *api_allow;
         int volt;
         char *defaultWebFolder;
         char *username;
         char *password;
         char *language;
         bool autoNet;
         char *ip;
         char *mask;
         char *gateway;
         char *dns;
         int chipNumber;
         int currentFrequencyIndex;
         int currentFrequency;
         char *frequencySet;
         FrequencyConf frequency[100];
         float board_reset_waittime;
         float board_reenable_waittime;
         int   invalid_cnt;
         int   temp_threshold;
         int   scanwork_sleeptime;
         int   task_interval;

         int start_voltage;//": "5800",
         int running_voltage1;//": "5630",
         int running_voltage2;//": "5630",
         int running_voltage3;//": "5630",
         int fengru;//": "4800",
         int fengchu;//": "6800"
    } Setting;

    extern void initSetting(const char *);
    extern Setting setting;
    extern int getFrequency();
    extern float getJobTimeout();
    extern float getJobTime2();
    extern float getJobTime();

#ifdef __cplusplus
}
#endif

#endif /* CGCONFIG_H */

