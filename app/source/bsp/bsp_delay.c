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
    ///< ѡ��PLL��ΪHCLKʱ��Դ;
    stcCfg.enClkSrc  = SysctrlClkPLL;
    ///< HCLK SYSCLK/1
    stcCfg.enHClkDiv = SysctrlHclkDiv1;
    ///< PCLK ΪHCLK/1
    stcCfg.enPClkDiv = SysctrlPclkDiv1;
    ///< ϵͳʱ�ӳ�ʼ��
    Sysctrl_ClkInit(&stcCfg);
    fac_us = Sysctrl_GetPClkFreq() / 1000000;
    SysTick_Config(Sysctrl_GetPClkFreq() / 1000);
}

void bsp_clk_init(void)
{
    stc_sysctrl_pll_cfg_t stcPLLCfg;

    ///< ��Ҫ��Ƶ��PLL��Ϊϵͳʱ��HCLK��ﵽ48MHz�����Դ˴�Ԥ������FLASH ���ȴ�����Ϊ1 cycle(Ĭ��ֵΪ0 cycle)
    Flash_WaitCycle(FlashWaitCycle1);

    ///< ʱ�ӳ�ʼ��ǰ����������Ҫʹ�õ�ʱ��Դ���˴�����PLL
    Sysctrl_SetRCHTrim(SysctrlRchFreq4MHz);             //PLLʹ��RCH��Ϊʱ��Դ�������Ҫ������RCH

    stcPLLCfg.enInFreq    = SysctrlPllInFreq4_6MHz;     //RCH 4MHz
		//��ʱ�����ֵ16777215�����16MHz����1s�ж�
    stcPLLCfg.enOutFreq   = SysctrlPllOutFreq36_48MHz;  //PLL ���48MHz
    stcPLLCfg.enPllClkSrc = SysctrlPllRch;              //����ʱ��Դѡ��RCH
    stcPLLCfg.enPllMul    = SysctrlPllMul12;            	//4MHz x 12 = 48MHz
    Sysctrl_SetPLLFreq(&stcPLLCfg);

		Systick_Init();
}

//ϵͳ��ʱ�ж�  1s
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
    uint32_t reload = SysTick->LOAD;				/* LOAD��ֵ	 */
    ticks = nus * fac_us; 						/* ��Ҫ�Ľ�����	  */
    tcnt = 0;
    told = SysTick->VAL;        				/* �ս���ʱ�ļ�����ֵ */
    while(1)
    {
        tnow = SysTick->VAL;
        if(tnow != told)
        {
            if(tnow < told)
                tcnt += told - tnow;			/* ����ע��һ��SYSTICK��һ���ݼ��ļ������Ϳ�����. */
            else
                tcnt += reload - tnow + told;
            told = tnow;
            if(tcnt >= ticks)
                break;						/* ʱ�䳬��/����Ҫ�ӳٵ�ʱ��,���˳�. */
        }
    }
}

void delay_ms(uint16_t nms)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;				/* LOAD��ֵ	 */
    ticks = nms * fac_us * 1000; 						/* ��Ҫ�Ľ�����	  */
    tcnt = 0;
    told = SysTick->VAL;        				/* �ս���ʱ�ļ�����ֵ */
    while(1)
    {
        tnow = SysTick->VAL;
        if(tnow != told)
        {
            if(tnow < told)
                tcnt += told-tnow;			/* ����ע��һ��SYSTICK��һ���ݼ��ļ������Ϳ�����. */
            else
                tcnt += reload - tnow + told;
            told = tnow;
            if(tcnt >= ticks)
                break;						/* ʱ�䳬��/����Ҫ�ӳٵ�ʱ��,���˳�. */
        }
    }
}

void HardClockTickUpdateTick(void)	// �õ�ʱ�����ڣ�ÿ�������ж�һ��
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
    return 0xC0000000;	// ʱ�����ֵ
}

uint32_t	HardColckTickEclipse(uint32_t nClock)		// �õ���ָ��ʱ�ӵ����ڵ�������
{
    if (HardClockTickGetTick() < nClock)
        return	((HardClockTickGetTick() + HardClockTickGetTickFull()) - nClock);
    return	(HardClockTickGetTick() - nClock);
}

bool HardColckTickExpire(uint32_t nClock, uint32_t nExpire)	// �Ƿ�ʱ�������Ѿ���ʱ(>0��ʱ�Ѿ���, <=0��Ҫ��ʱ)
{
    return (HardColckTickEclipse(nClock) >= nExpire) ? TRUE : FALSE;
}

uint32_t HardClockTickMilliSecond(uint32_t nMillSec)	// �õ�������Ӧ��ʱ�Ӽ���
{
    // һ���ܲ�20ms������_HARD_ADC_POINT�㣬��������֮��ļ����20ms/_HARD_ADC_POINT��
    return	nMillSec;
}

uint32_t HardClockTickSecond(float fSecond)		// �õ������Ӧ��ʱ�Ӽ���
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

uint32_t	GetSysTickMsEclipse(uint32_t nClock)		// �õ���ָ��ʱ�ӵ����ڵ�������
{
    if (GetSysTickSecCount()*1000 < nClock)
        return	((GetSysTickSecCount()*1000 + HardClockTickGetTickFull()) - nClock);
    return	(GetSysTickSecCount()*1000 - nClock);
}

bool GetSysTickMsExpire(uint32_t nClock, uint32_t nExpire)	// �Ƿ�ʱ�������Ѿ���ʱ(>0��ʱ�Ѿ���, <=0��Ҫ��ʱ)
{
    return (HardColckTickEclipse(nClock) >= nExpire) ? TRUE : FALSE;
}
