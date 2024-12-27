#ifndef __SEGMENT_H__
#define __SEGMENT_H__

#include "ddl.h"
#include "stdbool.h"

#define PAGE_MAIN						0		// ������
#define PAGE_SLEEP					1		// ״̬����
#define PAGE_CTRL						2	  // ���ƽ���
#define PAGE_SET						3	  // ���ý���
#define PAGE_TIMER_1				4	  // ��ʱ1����
#define PAGE_TIMER_2				5	  // ��ʱ2����
#define PAGE_ALARM					6		// ��������
#define PAGE_DURATION_SET		7		// ��ʱ�趨����
#define PAGE_ALARM_SET			8		// �����趨����
#define PAGE_TIMING			    9		// ��ʱ����
 
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
