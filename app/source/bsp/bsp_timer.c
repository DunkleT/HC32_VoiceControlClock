#include "bsp_timer.h"
#include "bsp_uart.h"
#include "bsp_delay.h"

#include "tm1652.h"

//100ms���ش���
uint32_t reload_count = 0;
//��ʱʱ�� = x��/100ms
uint32_t duration_count = 0;
//�Ƿ�ʱ
bool is_out_time = false;

#define ENABLE_SHOW			Gpio_SetIO(GpioPortB,GpioPin11);delay_ms(10);
#define DISABLE_SHOW		delay_ms(10);Gpio_ClrIO(GpioPortB,GpioPin11);


// Timer3 ����
void Timer3_Init()
{
		uint16_t u16Period = 56250;

    uint16_t                    u16ArrValue;
    uint16_t                    u16CntValue;
    stc_tim3_mode0_cfg_t     stcTim3BaseCfg;
    
    //�ṹ���ʼ������
    DDL_ZERO_STRUCT(stcTim3BaseCfg);
    
    Sysctrl_SetPeripheralGate(SysctrlPeripheralTim3, TRUE); //Base Timer����ʱ��ʹ��
    
    stcTim3BaseCfg.enWorkMode = Tim3WorkMode0;              //��ʱ��ģʽ
    stcTim3BaseCfg.enCT       = Tim3Timer;                  //��ʱ�����ܣ�����ʱ��Ϊ�ڲ�PCLK
    stcTim3BaseCfg.enPRS      = Tim3PCLKDiv256;             //PCLK/256
    stcTim3BaseCfg.enCntMode  = Tim316bitArrMode;           //�Զ�����16λ������/��ʱ��
    stcTim3BaseCfg.bEnTog     = FALSE;
    stcTim3BaseCfg.bEnGate    = FALSE;
    stcTim3BaseCfg.enGateP    = Tim3GatePositive;
    
    Tim3_Mode0_Init(&stcTim3BaseCfg);                       //TIM3 ��ģʽ0���ܳ�ʼ��
    
		//Timer3 ����; 16��Ƶ,����u16Period = 56250-->56250*(1/48M) * 256 = 300000us = 300ms=0.3s
    u16ArrValue = 0x10000 - u16Period ;
    
    Tim3_M0_ARRSet(u16ArrValue);                            //��������ֵ(ARR = 0x10000 - ����)
    
    u16CntValue = 0x10000 - u16Period;
    
    Tim3_M0_Cnt16Set(u16CntValue);                          //���ü�����ֵ
    
    Tim3_ClearIntFlag(Tim3UevIrq);                          //���жϱ�־
    Tim3_Mode0_EnableIrq();                                 //ʹ��TIM3�ж�(ģʽ0ʱֻ��һ���ж�)
    EnableNvic(TIM3_IRQn, IrqLevel3, TRUE);                 //TIM3 ���ж� 
}

// TIM3�жϷ�����
void Tim3_IRQHandler(void)
{
    //Timer3 ģʽ0 ��ʱ����ж�
    if(TRUE == Tim3_GetIntFlag(Tim3UevIrq))
    {
        reload_count ++;
				if(reload_count >= duration_count)
				{
						is_out_time = true;
						reload_count = 0;
				}
        Tim3_ClearIntFlag(Tim3UevIrq);  //Timer3ģʽ0 �жϱ�־���
    }
}

// TIM3��ʱ��������
void Tim3_Reset(uint32_t duration100Ms)
{
		reload_count = 0;
		duration_count = duration100Ms;
		Tim3_M0_Run();   //TIM3 ���С�
}

