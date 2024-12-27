#include "bsp_delay.h"
#include "gpio.h"
#include "bt.h"
#include "flash.h"
#include "sysctrl.h"

uint8_t  fac_us = 0;
uint32_t SecondCount = 0;
uint32_t m_nSysClockTick = 0;

uint8_t new_ds1302_minute_1 = 0;
uint8_t old_ds1302_minute_1 = 0;
uint8_t new_ds1302_minute_2 = 0;
uint8_t old_ds1302_minute_2 = 0;

uint8_t now_ds1302_hour;
uint8_t now_ds1302_minute;

uint8_t blink_count;

void Systick_Deinit(void)
{
    Sysctrl_ClkDeInit();
}

void Systick_Init(void)
{
    stc_sysctrl_clk_cfg_t stcCfg;
    DDL_ZERO_STRUCT(stcCfg);
    ///< 选择PLL作为HCLK时钟源;
    stcCfg.enClkSrc  = SysctrlClkPLL;
    ///< HCLK SYSCLK/1
    stcCfg.enHClkDiv = SysctrlHclkDiv1;
    ///< PCLK 为HCLK/1
    stcCfg.enPClkDiv = SysctrlPclkDiv1;
    ///< 系统时钟初始化
    Sysctrl_ClkInit(&stcCfg);
    fac_us = Sysctrl_GetPClkFreq() / 1000000;
    SysTick_Config(Sysctrl_GetPClkFreq() / 1000);
}

void bsp_clk_init(void)
{
    stc_sysctrl_pll_cfg_t stcPLLCfg;

    ///< 因将要倍频的PLL作为系统时钟HCLK会达到48MHz：所以此处预先设置FLASH 读等待周期为1 cycle(默认值为0 cycle)
    Flash_WaitCycle(FlashWaitCycle1);

    ///< 时钟初始化前，优先设置要使用的时钟源：此处配置PLL
    Sysctrl_SetRCHTrim(SysctrlRchFreq4MHz);             //PLL使用RCH作为时钟源，因此需要先设置RCH

    stcPLLCfg.enInFreq    = SysctrlPllInFreq4_6MHz;     //RCH 4MHz
		//计时器最大值16777215，最大16MHz才能1s中断
    stcPLLCfg.enOutFreq   = SysctrlPllOutFreq36_48MHz;  //PLL 输出48MHz
    stcPLLCfg.enPllClkSrc = SysctrlPllRch;              //输入时钟源选择RCH
    stcPLLCfg.enPllMul    = SysctrlPllMul12;            	//4MHz x 12 = 48MHz
    Sysctrl_SetPLLFreq(&stcPLLCfg);

		Systick_Init();
}

//系统定时中断  1s
void SysTick_Handler(void)
{
	SecondCount ++;

	if (SecondCount >= HardClockTickGetTickFull())
			SecondCount = 0;

	HardClockTickUpdateTick();
}

void delay_us(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told,tnow,tcnt = 0;
    uint32_t reload = SysTick->LOAD;				/* LOAD的值	 */
    ticks = nus * fac_us; 						/* 需要的节拍数	  */
    tcnt = 0;
    told = SysTick->VAL;        				/* 刚进入时的计数器值 */
    while(1)
    {
        tnow = SysTick->VAL;
        if(tnow != told)
        {
            if(tnow < told)
                tcnt += told - tnow;			/* 这里注意一下SYSTICK是一个递减的计数器就可以了. */
            else
                tcnt += reload - tnow + told;
            told = tnow;
            if(tcnt >= ticks)
                break;						/* 时间超过/等于要延迟的时间,则退出. */
        }
    }
}

void delay_ms(uint16_t nms)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;				/* LOAD的值	 */
    ticks = nms * fac_us * 1000; 						/* 需要的节拍数	  */
    tcnt = 0;
    told = SysTick->VAL;        				/* 刚进入时的计数器值 */
    while(1)
    {
        tnow = SysTick->VAL;
        if(tnow != told)
        {
            if(tnow < told)
                tcnt += told-tnow;			/* 这里注意一下SYSTICK是一个递减的计数器就可以了. */
            else
                tcnt += reload - tnow + told;
            told = tnow;
            if(tcnt >= ticks)
                break;						/* 时间超过/等于要延迟的时间,则退出. */
        }
    }
}

void HardClockTickUpdateTick(void)	// 得到时钟周期，每个采样中断一次
{
    m_nSysClockTick++;
    if (m_nSysClockTick >= HardClockTickGetTickFull())
        m_nSysClockTick = 0;
}

uint32_t HardClockTickGetTick(void)
{
    return	m_nSysClockTick;
}

uint32_t HardClockTickGetTickFull(void)
{
    return 0xC0000000;	// 时钟溢出值
}

uint32_t	HardColckTickEclipse(uint32_t nClock)		// 得到自指定时钟到现在的周期数
{
    if (HardClockTickGetTick() < nClock)
        return	((HardClockTickGetTick() + HardClockTickGetTickFull()) - nClock);
    return	(HardClockTickGetTick() - nClock);
}

bool HardColckTickExpire(uint32_t nClock, uint32_t nExpire)	// 是否时钟周期已经超时(>0延时已经到, <=0还要延时)
{
    return (HardColckTickEclipse(nClock) >= nExpire) ? TRUE : FALSE;
}

uint32_t HardClockTickMilliSecond(uint32_t nMillSec)	// 得到与毫秒对应的时钟计数
{
    // 一个周波20ms，采样_HARD_ADC_POINT点，则两个点之间的间隔是20ms/_HARD_ADC_POINT；
    return	nMillSec;
}

uint32_t HardClockTickSecond(float fSecond)		// 得到与秒对应的时钟计数
{
    return	HardClockTickMilliSecond((uint32_t)(fSecond * 1000));
}

uint32_t GetSysTickSecCount(void)
{
    return SecondCount;
}

uint32_t GetSysTickMsInterval(uint32_t beginTick)
{
    uint32_t temptick;
    temptick = GetSysTickSecCount()*1000;
    if(beginTick <= temptick)
        return (temptick - beginTick);
    else
        return ((0xFFFFFFFF - beginTick)+temptick);
}

uint32_t	GetSysTickMsEclipse(uint32_t nClock)		// 得到自指定时钟到现在的周期数
{
    if (GetSysTickSecCount()*1000 < nClock)
        return	((GetSysTickSecCount()*1000 + HardClockTickGetTickFull()) - nClock);
    return	(GetSysTickSecCount()*1000 - nClock);
}

bool GetSysTickMsExpire(uint32_t nClock, uint32_t nExpire)	// 是否时钟周期已经超时(>0延时已经到, <=0还要延时)
{
    return (HardColckTickEclipse(nClock) >= nExpire) ? TRUE : FALSE;
}
