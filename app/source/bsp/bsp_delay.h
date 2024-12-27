#ifndef __BSP_DELAY_H__
#define __BSP_DELAY_H__

#include "sysctrl.h"
#include "stdbool.h"

void Systick_Deinit(void);
void bsp_clk_init(void);
void Systick_Init(void);

void delay_us(uint32_t nus);
void delay_ms(uint16_t nms);

uint32_t GetSysTickCount(void);
uint32_t GetSysTickInterval(uint32_t beginTick);
void HardClockTickUpdateTick(void);
uint32_t HardClockTickGetTickFull(void);
bool HardColckTickExpire(uint32_t nClock, uint32_t nExpire);
uint32_t	HardColckTickEclipse(uint32_t nClock);		// 得到自指定时钟到现在的周期数
uint32_t HardClockTickSecond(float fSecond);		// 得到与秒对应的时钟计数
uint32_t HardClockTickMilliSecond(uint32_t nMillSec);
uint32_t GetSysTickSecCount(void);
bool GetSysTickMsExpire(uint32_t nClock, uint32_t nExpire);
uint32_t HardClockTickGetTick(void);
uint32_t GetSysTickMsInterval(uint32_t beginTick);
void bsp_seg_timer_init(void);

#endif
