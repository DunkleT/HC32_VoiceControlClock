#include "bsp_timer.h"
#include "bsp_uart.h"
#include "bsp_delay.h"

#include "tm1652.h"

//100ms重载次数
uint32_t reload_count = 0;
//定时时长 = x秒/100ms
uint32_t duration_count = 0;
//是否超时
bool is_out_time = false;

#define ENABLE_SHOW			Gpio_SetIO(GpioPortB,GpioPin11);delay_ms(10);
#define DISABLE_SHOW		delay_ms(10);Gpio_ClrIO(GpioPortB,GpioPin11);


// Timer3 配置
void Timer3_Init()
{
		uint16_t u16Period = 56250;

    uint16_t                    u16ArrValue;
    uint16_t                    u16CntValue;
    stc_tim3_mode0_cfg_t     stcTim3BaseCfg;
    
    //结构体初始化清零
    DDL_ZERO_STRUCT(stcTim3BaseCfg);
    
    Sysctrl_SetPeripheralGate(SysctrlPeripheralTim3, TRUE); //Base Timer外设时钟使能
    
    stcTim3BaseCfg.enWorkMode = Tim3WorkMode0;              //定时器模式
    stcTim3BaseCfg.enCT       = Tim3Timer;                  //定时器功能，计数时钟为内部PCLK
    stcTim3BaseCfg.enPRS      = Tim3PCLKDiv256;             //PCLK/256
    stcTim3BaseCfg.enCntMode  = Tim316bitArrMode;           //自动重载16位计数器/定时器
    stcTim3BaseCfg.bEnTog     = FALSE;
    stcTim3BaseCfg.bEnGate    = FALSE;
    stcTim3BaseCfg.enGateP    = Tim3GatePositive;
    
    Tim3_Mode0_Init(&stcTim3BaseCfg);                       //TIM3 的模式0功能初始化
    
		//Timer3 配置; 16分频,周期u16Period = 56250-->56250*(1/48M) * 256 = 300000us = 300ms=0.3s
    u16ArrValue = 0x10000 - u16Period ;
    
    Tim3_M0_ARRSet(u16ArrValue);                            //设置重载值(ARR = 0x10000 - 周期)
    
    u16CntValue = 0x10000 - u16Period;
    
    Tim3_M0_Cnt16Set(u16CntValue);                          //设置计数初值
    
    Tim3_ClearIntFlag(Tim3UevIrq);                          //清中断标志
    Tim3_Mode0_EnableIrq();                                 //使能TIM3中断(模式0时只有一个中断)
    EnableNvic(TIM3_IRQn, IrqLevel3, TRUE);                 //TIM3 开中断 
}

// TIM3中断服务函数
void Tim3_IRQHandler(void)
{
    //Timer3 模式0 计时溢出中断
    if(TRUE == Tim3_GetIntFlag(Tim3UevIrq))
    {
        reload_count ++;
				if(reload_count >= duration_count)
				{
						is_out_time = true;
						reload_count = 0;
				}
        Tim3_ClearIntFlag(Tim3UevIrq);  //Timer3模式0 中断标志清除
    }
}

// TIM3定时参数重置
void Tim3_Reset(uint32_t duration100Ms)
{
		reload_count = 0;
		duration_count = duration100Ms;
		Tim3_M0_Run();   //TIM3 运行。
}

