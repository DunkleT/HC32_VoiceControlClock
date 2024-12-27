#include "bsp_pwm.h"

void tim0_irq_handler(void)
{
    if(TRUE == Bt_GetIntFlag(TIM0, BtUevIrq))
    {
        Bt_ClearIntFlag(TIM0,BtUevIrq);
    }
}

void bsp_bt_pwm_init(void)
{
    uint16_t                      u16ArrValue;
    uint16_t                      u16CompareAValue;
    uint16_t                      u16CntValue;
    uint8_t                       u8ValidPeriod;
    stc_bt_mode23_cfg_t        stcBtBaseCfg;
    stc_bt_m23_compare_cfg_t   stcBtPortCmpCfg;
    stc_gpio_cfg_t             stcTIM0Port;
    stc_gpio_cfg_t             stcLEDPort;

    DDL_ZERO_STRUCT(stcBtBaseCfg);
    DDL_ZERO_STRUCT(stcTIM0Port);
    DDL_ZERO_STRUCT(stcLEDPort);
    DDL_ZERO_STRUCT(stcBtPortCmpCfg);

    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);   //GPIO 外设时钟使能
    Sysctrl_SetPeripheralGate(SysctrlPeripheralBaseTim, TRUE);   //Base Timer外设时钟使能

    stcTIM0Port.enDir  = GpioDirOut;

    Gpio_Init(GpioPortA, GpioPin15, &stcTIM0Port);
    Gpio_SetAfMode(GpioPortA,GpioPin15,GpioAf7);            //PB12设置为TIM6_CHA

    stcBtBaseCfg.enWorkMode    = BtWorkMode2;              //锯齿波模式
    stcBtBaseCfg.enCT          = BtTimer;                  //定时器功能，计数时钟为内部PCLK
    stcBtBaseCfg.enPRS         = BtPCLKDiv1;               //PCLK
    stcBtBaseCfg.enCntDir      = BtCntUp;                  //向上计数，在三角波模式时只读
    stcBtBaseCfg.enPWMTypeSel  = BtIndependentPWM;         //独立输出PWM
    stcBtBaseCfg.enPWM2sSel    = BtSinglePointCmp;         //单点比较功能
    stcBtBaseCfg.bOneShot      = FALSE;                    //循环计数
    stcBtBaseCfg.bURSSel       = FALSE;                    //上下溢更新

		stcBtBaseCfg.pfnTim0Cb  = tim0_irq_handler;            //中断函数入口


    Bt_Mode23_Init(TIM0, &stcBtBaseCfg);                   //TIM0 的模式0功能初始化

    u16ArrValue = 0x4572;																	 //重载值是17778 PWM频率为2.7KHz
    Bt_M23_ARRSet(TIM0, u16ArrValue, TRUE);                //设置重载值,并使能缓存

    u16CompareAValue = 0x22B9;														 //PWM占空比为50%
    Bt_M23_CCR_Set(TIM0, BtCCR0A, u16CompareAValue);       //设置比较值A


    stcBtPortCmpCfg.enCH0ACmpCtrl   = BtPWMMode1;          //OCREFA输出控制OCMA:PWM模式2
    stcBtPortCmpCfg.enCH0APolarity  = BtPortOpposite;      //正常输出
    stcBtPortCmpCfg.bCh0ACmpBufEn   = TRUE;                //A通道缓存控制
    stcBtPortCmpCfg.enCh0ACmpIntSel = BtCmpIntNone;        //A通道比较控制:无

    Bt_M23_PortOutput_Cfg(TIM0, &stcBtPortCmpCfg);      //比较输出端口配置

    u8ValidPeriod = 0;                                     //事件更新周期设置，0表示锯齿波每个周期更新一次，每+1代表延迟1个周期
    Bt_M23_SetValidPeriod(TIM0,u8ValidPeriod);             //间隔周期设置

    u16CntValue = 0;
    Bt_M23_Cnt16Set(TIM0, u16CntValue);                    //设置计数初值

    Bt_ClearAllIntFlag(TIM0);                              //清中断标志
    EnableNvic(TIM0_IRQn, IrqLevel0, TRUE);                //TIM0中断使能
    Bt_Mode23_EnableIrq(TIM0,BtUevIrq);                    //使能TIM0 UEV更新中断

    Bt_M23_Run(TIM0);                                      //TIM0 运行
}

void bsp_bt_pwm_ctrl_output(bool en)
{
    if(en == true)
    {
        Gpio_SetAfMode(GpioPortA,GpioPin15,GpioAf5);            //开启TIM0 PWM输出
        Bt_M23_EnPWM_Output(TIM0, TRUE, FALSE);
    }
    else
    {
        Bt_M23_EnPWM_Output(TIM0, FALSE, FALSE);
        Gpio_SetAfMode(GpioPortA,GpioPin15,GpioAf0);            //关闭TIM0 PWM输出
        Gpio_ClrIO(GpioPortA,GpioPin15);
    }
}


void bsp_adt_pwm_init(void)
{
    en_adt_compare_t          enAdtCompareA;

    stc_adt_basecnt_cfg_t     stcAdtBaseCntCfg;
    stc_adt_CHxX_port_cfg_t   stcAdtTIM6ACfg;  
    
    DDL_ZERO_STRUCT(stcAdtBaseCntCfg);
    DDL_ZERO_STRUCT(stcAdtTIM6ACfg);
    
		stc_gpio_cfg_t         stcTIM6Port;
    
    DDL_ZERO_STRUCT(stcTIM6Port);
    
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE); 		 //端口外设时钟使能
    
    stcTIM6Port.enDir  = GpioDirOut;
    //PB12设置为TIM6_CHA
    Gpio_Init(GpioPortB, GpioPin12, &stcTIM6Port);
    Gpio_SetAfMode(GpioPortB,GpioPin12,GpioAf7);

    Sysctrl_SetPeripheralGate(SysctrlPeripheralAdvTim, TRUE);    //ADT外设时钟使能
    
    stcAdtBaseCntCfg.enCntMode = AdtSawtoothMode;              	 //锯齿波模式
    stcAdtBaseCntCfg.enCntDir = AdtCntUp;
    stcAdtBaseCntCfg.enCntClkDiv = AdtClkPClk0;
    
    Adt_Init(M0P_ADTIM6, &stcAdtBaseCntCfg);                  //ADT载波、计数模式、时钟配置
		
		//0x4572
    Adt_SetPeriod(M0P_ADTIM6, 0x5200);                        //周期设置 重载值是17778 PWM频率为2.7KHz
    
    enAdtCompareA = AdtCompareA;
		//0x22B9
    Adt_SetCompareValue(M0P_ADTIM6, enAdtCompareA, 0x3D80);   //通用比较基准值寄存器A设置(占空比50%)

    
    stcAdtTIM6ACfg.enCap = AdtCHxCompareOutput;            		//比较输出
    stcAdtTIM6ACfg.bOutEn = TRUE;                          		//CHA输出使能
    stcAdtTIM6ACfg.enPerc = AdtCHxPeriodLow;              		//计数值与周期匹配时CHA电平保持不变
    stcAdtTIM6ACfg.enCmpc = AdtCHxCompareHigh;             		//计数值与比较值A匹配时，CHA电平翻转
    stcAdtTIM6ACfg.enStaStp = AdtCHxStateSelSS;            		//CHA起始结束电平由STACA与STPCA控制
    stcAdtTIM6ACfg.enStaOut = AdtCHxPortOutLow;            		//CHA起始电平为低
    stcAdtTIM6ACfg.enStpOut = AdtCHxPortOutLow;            		//CHA结束电平为低
    Adt_CHxXPortCfg(M0P_ADTIM6, AdtCHxA, &stcAdtTIM6ACfg);    //端口CHA配置
}

void bsp_adt_pwm_ctrl_output(bool en)
{
    if(en == true)
    {
        Gpio_SetAfMode(GpioPortB,GpioPin12,GpioAf7);            //开启TIM0 PWM输出
        Adt_StartCount(M0P_ADTIM6); //AdvTimer6运行
    }
    else
    {
        Adt_StopCount(M0P_ADTIM6); //AdvTimer6停止
        Gpio_SetAfMode(GpioPortB,GpioPin12,GpioAf0);            //关闭TIM0 PWM输出
        Gpio_ClrIO(GpioPortB,GpioPin12);
    }
}
