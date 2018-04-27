/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>

#include <signal.h>
#include <limits.h>
#include "arrays.h"
#include "CgConfig.h"

Setting setting;

/*
  *函数功能：从给定的文件中获取指定项的配置值
  *返回的值为堆内存，需要显示释放
  */
char *ReadConfig(const char *fileName, char *item) {
    FILE *fp;
    char *locate = NULL;
    char *pmove = NULL;
    char confLine[ CONLINELENGTH] = {};
    char context[CONLINELENGTH] = {};
    int result = 1;
    char *pline;
    int itl;

   // printf(item);
    if ((fp = fopen(fileName, "r")) == NULL) {
        printf("打开文件 : %s 失败!!\n", fileName);
        return NULL;
    }

    while (fgets(confLine, CONLINELENGTH, fp) != NULL) {
        pline = confLine;
        if (*pline == '#'||*pline == '/'||*pline == '*'||*pline == '{'||*pline == '}'||*pline == '['||*pline == ']') {
            memset(confLine, '0', CONLINELENGTH);
            continue;
        }
        while (isspace(*pline) != 0)pline++;
        locate = strchr(pline, ':');
        if (locate == NULL) {
            memset(confLine, '0', CONLINELENGTH);
            continue;
        }
        pmove = locate;
        pmove--;
        while (isspace(*pmove) != 0)pmove--;
        itl = pmove - pline + 1;
        if (itl == strlen(item)) {
            result = strncasecmp(pline, item, itl);
            if (result == 0) {
                locate++;
                while (isspace(*locate) != 0)locate++;
                pmove = locate;
                while (isspace(*pmove) == 0)pmove++;
                if (pmove - locate + 1 > 0) {
                    strncpy(context, locate, pmove - locate + 1);
                    break;
                } else {
                    fclose(fp);
                    return NULL;
                }
            } else {
                memset(confLine, '0', CONLINELENGTH);
                continue;
            }
        } else {
            memset(confLine, '0', CONLINELENGTH);
            continue;
        }
    }
    fclose(fp);
    char *rtnOld;
    char * rtn=(char *)malloc(strlen(context)+1);
    rtnOld=rtn;
    strcpy(rtn,context);

    char *tmp=rtn;
    
    
    char *ls=rtn;
    while(*ls==' ')
    {
        ls++;
        rtn++;
    }
    if(*ls=='"'){
        //trim "
        ls++;
        rtn++;
        while(*ls!='"'&&*ls!='\n'&&*ls!='\0')
            ls++;
        *ls='\0';
    }
    while(*tmp!=','&&*tmp!='\0')           //去掉字符串结束符'\0'前的'\n'    
        tmp++;  
     *tmp   =   '\0';
     char *rtnNew=(char *)malloc(strlen(rtn)+1);
     strcpy(rtnNew,rtn);
     free(rtnOld);
    return rtnNew;
}
double GetDouble(const char * confPath,char *name)
{
    double rtn=0;
    char *readname=ReadConfig(confPath,name);
    if(readname!=NULL)
    {
        char *pend = NULL;
        rtn=strtod(readname, &pend);
    }
    else
    {
        printf("配置文件中找不到叫:%s的项!",name);
    }
    free(readname);
    return rtn;
}
/*
 *获得配置文件的Int值
 */
int GetInt(const char * confPath,char *name)
{
    int rtn=0;
    char *readname=ReadConfig(confPath,name);
    if(readname!=NULL)
    {
        rtn=atol(readname);
    }
    else
    {
        printf("配置文件中找不到叫:%s的项!",name);
    }
    free(readname);
    return rtn;
}
int EqualString(char *s, char *s2) {
    while (*s2) {
        if (*s2++ != *s++)
            return 0;
    }
    if (*s == '\0' && *s2 == '\0')
        return 1;
    else
        return 0;
}
/*
 *获得配置文件的Int值
 */
bool GetBool(const char * confPath,char *name)
{
    bool rtn=0;
    char *readname=ReadConfig(confPath,name);
    int result=1;
    if(readname!=NULL)
    {
        
        result = EqualString(readname, "true");
        if(result)
            rtn=true;
        else 
            rtn=false;
    }
    else
    {
        printf("配置文件中找不到叫:%s的项!",name);
        rtn=false;
    }
    free(readname);
    return rtn;
}
Array * SplitSimple(char *str, char * str1) {
    int l1 = strlen(str);
    int l2 = strlen(str1);
    int matchOn = 0;
    char * cstr = str;
    char * cstr1 = str1;
    int begin = 0, end = 0;
    char * ls, *ls2;
    Array *arr = ArrayNew();
    while (*cstr) {
        if (*cstr == *cstr1) {/*如果有个字母与old第一个字母相同*/
            int j;
            for (j = 0; j < l2; j++) {/*循环old字母位数进行比较*/
                if (*(cstr + j) != *(cstr1 + j)) {
                    matchOn = 0;
                    break;
                } else if (j == l2 - 1)
                    matchOn = 1;
            }

        } else {
            end++;
            cstr++;
        }

        if (matchOn || !(*(cstr))) {/*如果匹配了就循环old位数项全部设为\0*/
            int k;
            char *s = (char *)malloc(end-begin+1);//(end - begin);
            for (k = 0; k < end - begin; k++)
                *s++ = *str++;
            if (!(*(cstr))) {
                *s++ = *str;
                s = s - end + begin - 1;
                AddArray(&arr, s);
            } else {

                *s = '\0';
                s = s - end + begin;
                AddArray(&arr, s);
                cstr += l2;
                str += l2;
                ;
                begin = end + l2;
                end = begin;
                matchOn = 0;
            }
        }

    }
    return arr;
}
void initFrequencyCurrentIndex(){
    int i;
    for(i=0;i<99;i++){
        if(setting.frequency[i].frequency==setting.currentFrequency){
         setting.currentFrequencyIndex=i;
          break;     
        }
    }
}
float getJobTime(){
    return setting.frequency[setting.currentFrequencyIndex].jobtime;
}
float getJobTime2(int frequency){
    int i;
    for(i=0;i<99;i++){
        if( setting.frequency[i].frequency==frequency)
            return  setting.frequency[i].jobtime;
    }
    return 0.08;
}

void initFrequency(){
    Array *arr=SplitSimple(setting.frequencySet,"|");
    int i;
    for(i=0;i<arr->size;i++){
      char *ls=(char *)ArrayGet(arr,i);
       Array *arrLs=SplitSimple(ls,"_");
       setting.frequency[i].frequency=atol((char *)ArrayGet(arrLs,0));
        setting.frequency[i].jobtime= atol((char *)ArrayGet(arrLs,1));
         ArrayFree(arrLs);
    }
    initFrequencyCurrentIndex();
    ArrayFree(arr);
    
}
void initPoolItem(int index){
    Array *arr=SplitSimple(setting.pool[index].allSetting,"|");
    setting.pool[index].url=(char *)ArrayGet(arr,0);
    setting.pool[index].user=(char *)ArrayGet(arr,1);
    setting.pool[index].pass=(char *)ArrayGet(arr,2);
     //ArrayFree(arr);
}
void double_check_str(int8_t str[])
{
    int i = 0;
    while(str[i])
    {
        if(str[i] == '|')
            str[i] = ',';
        i++;
    }
}

void initSetting(const  char * path){
    if (path)
    {
        printf("path:%s\n", path);
        setting.portWeb=GetInt(path, "\"HttpPort\"");
        setting.portApi=GetInt(path, "\"api-port\"");
        setting.volt=GetInt(path, "\"volt\"");
        setting.chipNumber=GetInt(path, "\"chipNumber\"");
        setting.currentFrequency=GetInt(path, "\"frequency\"");
        
        setting.failover_only=GetBool(path, "\"failover-only\"");
        setting.no_submit_stale=GetBool(path, "\"no-submit-stale\"");
        setting.autoNet=GetBool(path, "\"autoNet\"");
        setting.api_listen=GetBool(path, "\"api-listen\"");
        setting.autoFrequency=GetBool(path, "\"autoFrequency\"");
        setting.autoGetJobTimeOut=GetBool(path, "\"autoGetJobTimeOut\"");
        setting.isdebug=GetBool(path, "\"debug\"");
        
        setting.username=ReadConfig(path, "\"username\""); 
        setting.password=ReadConfig(path, "\"password\""); 
        setting.language=ReadConfig(path, "\"language\""); 
        setting.api_allow=ReadConfig(path, "\"api-allow\"");
        double_check_str(setting.api_allow);
        
        setting.defaultWebFolder=ReadConfig(path,"\"defaultWebFolder\"");
        setting.frequencySet=ReadConfig(path,"\"frequencySet\"");

        initFrequency();
        
        setting.pool[0].allSetting=ReadConfig(path,"\"pool1\"");
        setting.pool[1].allSetting=ReadConfig(path,"\"pool2\"");//
        setting.pool[2].allSetting=ReadConfig(path,"\"pool3\"");//
        initPoolItem(0);
	    initPoolItem(1);
	    initPoolItem(2);
        setting.ip=ReadConfig(path,"\"ip\"");
        setting.mask=ReadConfig(path,"\"mask\"");
        setting.gateway=ReadConfig(path,"\"gateway\"");
        setting.dns=ReadConfig(path,"\"dns\"");
        setting.invalid_cnt = GetInt(path,"\"invalid_cnt\"");
        setting.scanwork_sleeptime = GetInt(path, "\"scanwork_sleeptime\""); 
        setting.board_reset_waittime=GetDouble(path, "\"board_reset_waittime\""); 
        setting.board_reenable_waittime=GetDouble(path, "\"board_reenable_waittime\""); 
        setting.temp_threshold= GetInt(path,"\"temp_threshold\"");
        setting.task_interval = GetInt(path,"\"task_interval\"");
		setting.start_voltage = GetInt(path, "\"start_voltage\"");
		
		setting.running_voltage1 = GetInt(path, "\"running_voltage1\"");
		setting.running_voltage2 = GetInt(path, "\"running_voltage2\"");
		setting.running_voltage3 = GetInt(path, "\"running_voltage3\"");
		setting.fengru = GetInt(path, "\"fengru\"");
		setting.fengchu = GetInt(path, "\"fengchu\"");

		if (setting.start_voltage > 9000)
			 setting.start_voltage = 5800;
		if (setting.running_voltage1 > 9000)
			setting.running_voltage1 = 5650;
		if (setting.running_voltage2 > 9000)
			setting.running_voltage2 = 5650;
		if (setting.running_voltage3 > 9000)
			setting.running_voltage3 = 5650;
    }
}

int change_speed_to_param(int speed)
{
	if(speed >= 5000)
		return 40000;
	else if(speed >= 4000)	
		return 31000;
	else if(speed >= 3000) 
		return 22000;
	else if(speed >= 2000)
		return 15000;
	else
		return 7000;
}

extern int node_start;
void reset_param(const char *path)
{
		char cmd_buf[64] = {0};	

		setting.running_voltage1 = GetInt(path, "\"running_voltage1\"");
		setting.running_voltage2 = GetInt(path, "\"running_voltage2\"");
		setting.running_voltage3 = GetInt(path, "\"running_voltage3\"");
		setting.fengru = GetInt(path, "\"fengru\"");
		setting.fengchu = GetInt(path, "\"fengchu\"");

		if (setting.running_voltage1 > 7000)
			setting.running_voltage1 = 5650;
		if (setting.running_voltage2 > 7000)
			setting.running_voltage2 = 5650;
		if (setting.running_voltage3 > 7000)
			setting.running_voltage3 = 5650;

		sprintf(cmd_buf, "echo %d > /sys/class/hwmon/hwmon%d/in2_output",setting.running_voltage1, node_start);
		system(cmd_buf);
		
		sprintf(cmd_buf, "echo %d > /sys/class/hwmon/hwmon%d/in2_output",setting.running_voltage2, node_start+1);
		system(cmd_buf);

		sprintf(cmd_buf, "echo %d > /sys/class/hwmon/hwmon%d/in2_output",setting.running_voltage3, node_start+2);
		system(cmd_buf);

		sprintf(cmd_buf, "echo %d >/sys/class/pwm/pwmchip4/pwm0/duty_cycle", change_speed_to_param(setting.fengru));
		system(cmd_buf);

		sprintf(cmd_buf, "echo %d >/sys/class/pwm/pwmchip6/pwm0/duty_cycle", change_speed_to_param(setting.fengchu));
		system(cmd_buf);
}


#ifdef UNIT_TEST
int main(int argc, char** argv) {

    initSetting();
  
    return (EXIT_SUCCESS);
}
#endif
