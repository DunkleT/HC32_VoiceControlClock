#ifndef __PAMA_H__
#define __PAMA_H__

#include "bsp_delay.h"

#define	DEV_INF_VER_SOFT				0x0101			// 硬件版本
#define DEV_INF_VER_HARD				0x0101			// 程序版本

//控制模式定义
#define CTRL_MODE_ERES					0				//开启红外，开启语音
#define CTRL_MODE_ERDS					1				//开启红外，关闭语音
#define CTRL_MODE_DRES					2				//关闭红外，开启语音
#define CTRL_MODE_DRDS					3				//关闭红外，关闭语音


typedef struct tagAlarmDuration
{
		//小时间隔值
		uint8_t duration_hour;
		//分钟间隔值
		uint8_t duration_minute;
		
		//结束定时年份
		uint8_t alarm_yearL;
		//结束定时月份
		uint8_t alarm_month;
		//结束定时日期
		uint8_t alarm_date;
		//结束定时小时
		uint8_t alarm_hour;
		//结束定时分钟
		uint8_t alarm_minute;
		//结束定时秒钟
		uint8_t alarm_second;
}AlarmDuration;

typedef struct tagAlarmMoment
{
		//小时间隔值
		uint8_t moment_hour;
		//分钟间隔值
		uint8_t moment_minute;
}AlarmMoment;

//配置参数类型定义
typedef	struct tagMainConfig
{
		struct
		{
				uint8_t time_yearH;			//年 
				uint8_t time_yearL;			//年 
				uint8_t time_month;			//月 			
				uint8_t time_date;		  //日 
				uint8_t time_hour;		  //时 
				uint8_t time_minute;		//分
				uint8_t time_second;		//秒
				uint8_t time_week;		  //周 
		}tTime;

    struct
    {
        AlarmDuration alarm_duration_1;		//定时时间1
        AlarmDuration alarm_duration_2;		//定时时间2

        bool alarm_duration_enable_1;			//定时1开启
        bool alarm_duration_enable_2;			//定时2开启
				
				AlarmMoment alarm_moment_1;				//闹钟时间1
				AlarmMoment alarm_moment_2;				//闹钟时间2
				AlarmMoment alarm_moment_3;				//闹钟时间3

				bool alarm_moment_enable_1;				//闹钟1开启
        bool alarm_moment_enable_2;				//闹钟2开启
				bool alarm_moment_enable_3;				//闹钟3开启
    } tCtrl;			// 控制参数

		float32_t  offsetTemperature;			//温度的修正值，因为温度有1位小数，所以温度修正存储值默认*10
		float32_t  offsetHumidity;				//湿度的修正值
		
		uint8_t  displayLightSet;		//显示亮度设置0~7等级
		uint8_t  ctrlModeSet;				//控制模式设置

		bool dispalyMode;						//显示模式，控制常亮模式
		bool timeMode;							//时间模式，0-24小时，1-12小时

		uint16_t	nDevVerSoft;			// 软件版本  DEV_INF_VER_SOFT
    uint16_t	nDevVerHard;			// 硬件版本  DEV_INF_VER_HARD

} MainConfig, *PMainConfig;

//设备参数类型定义
typedef struct tagDevicePama
{
    MainConfig*		pConfig;
		//参数正确标志位
    bool correct;
} DevicePama;

//设备状态
typedef struct tagDeviceState
{
    uint8_t  nKeyPress;			//当前按键触发
		uint8_t  nPageShow;			//当前页面显示
		uint8_t  nFuncIndex;		//当前功能索引
		uint8_t  nVoicePage;		//当前语音唤醒
} DeviceState;

#define DEV_CONFIG_DATA_LEN			sizeof(MainConfig)
#define DEV_CONFIG_TOTAL_LEN		(sizeof(MainConfig) + 2)

bool MainConfigReset(void);
bool MainConfigSave(void);
bool MainConfigCorrect(void);
bool MainConfigLoadAtBoot(void);

uint8_t JudgeDurationFirstArrive(void);
uint8_t JudgeAlarmFirstArrive(void);

bool JudgeAlarmOutline(uint8_t durationIndex);
bool GetDurationRemainTime(uint8_t durationIndex, uint8_t* remainHour, uint8_t* remainMinute, uint8_t* remainSecond);
void OpenMomentNextArrive(uint8_t momentIndex);

#endif
