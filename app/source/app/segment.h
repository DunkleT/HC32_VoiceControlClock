#ifndef __SEGMENT_H__
#define __SEGMENT_H__

#include "ddl.h"
#include "stdbool.h"

#define PAGE_MAIN						0		// 主界面
#define PAGE_SLEEP					1		// 状态界面
#define PAGE_CTRL						2	  // 控制界面
#define PAGE_SET						3	  // 设置界面
#define PAGE_TIMER_1				4	  // 定时1界面
#define PAGE_TIMER_2				5	  // 定时2界面
#define PAGE_ALARM					6		// 报警界面
#define PAGE_DURATION_SET		7		// 定时设定界面
#define PAGE_ALARM_SET			8		// 闹钟设定界面
#define PAGE_TIMING			    9		// 计时界面
 
void Main_Page_Show(void);

void Page_Main_Function(void);
void Page_Sleep_Function(void);
void Page_Set_Function(void);
void Page_Timing_Function(void);
void Page_Ctrl_Function(void);
void Page_Timer1_Function(void);
void Page_Timer2_Function(void);
void Page_Alarm_Function(void);
void Page_Duration_Set_Function(void);
void Page_Alarm_Set_Function(void);

bool Page_Set_Function_Lv2(uint8_t index);
bool Page_Set_Function_Lv3(uint8_t index);

#endif
