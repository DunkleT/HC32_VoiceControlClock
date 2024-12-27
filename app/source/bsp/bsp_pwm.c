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

    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);   //GPIO ����ʱ��ʹ��
    Sysctrl_SetPeripheralGate(SysctrlPeripheralBaseTim, TRUE);   //Base Timer����ʱ��ʹ��

    stcTIM0Port.enDir  = GpioDirOut;

    Gpio_Init(GpioPortA, GpioPin15, &stcTIM0Port);
    Gpio_SetAfMode(GpioPortA,GpioPin15,GpioAf7);            //PB12����ΪTIM6_CHA

    stcBtBaseCfg.enWorkMode    = BtWorkMode2;              //��ݲ�ģʽ
    stcBtBaseCfg.enCT          = BtTimer;                  //��ʱ�����ܣ�����ʱ��Ϊ�ڲ�PCLK
    stcBtBaseCfg.enPRS         = BtPCLKDiv1;               //PCLK
    stcBtBaseCfg.enCntDir      = BtCntUp;                  //���ϼ����������ǲ�ģʽʱֻ��
    stcBtBaseCfg.enPWMTypeSel  = BtIndependentPWM;         //�������PWM
    stcBtBaseCfg.enPWM2sSel    = BtSinglePointCmp;         //����ȽϹ���
    stcBtBaseCfg.bOneShot      = FALSE;                    //ѭ������
    stcBtBaseCfg.bURSSel       = FALSE;                    //���������

		stcBtBaseCfg.pfnTim0Cb  = tim0_irq_handler;            //�жϺ������


    Bt_Mode23_Init(TIM0, &stcBtBaseCfg);                   //TIM0 ��ģʽ0���ܳ�ʼ��

    u16ArrValue = 0x4572;																	 //����ֵ��17778 PWMƵ��Ϊ2.7KHz
    Bt_M23_ARRSet(TIM0, u16ArrValue, TRUE);                //��������ֵ,��ʹ�ܻ���

    u16CompareAValue = 0x22B9;														 //PWMռ�ձ�Ϊ50%
    Bt_M23_CCR_Set(TIM0, BtCCR0A, u16CompareAValue);       //���ñȽ�ֵA


    stcBtPortCmpCfg.enCH0ACmpCtrl   = BtPWMMode1;          //OCREFA�������OCMA:PWMģʽ2
    stcBtPortCmpCfg.enCH0APolarity  = BtPortOpposite;      //�������
    stcBtPortCmpCfg.bCh0ACmpBufEn   = TRUE;                //Aͨ���������
    stcBtPortCmpCfg.enCh0ACmpIntSel = BtCmpIntNone;        //Aͨ���ȽϿ���:��

    Bt_M23_PortOutput_Cfg(TIM0, &stcBtPortCmpCfg);      //�Ƚ�����˿�����

    u8ValidPeriod = 0;                                     //�¼������������ã�0��ʾ��ݲ�ÿ�����ڸ���һ�Σ�ÿ+1�����ӳ�1������
    Bt_M23_SetValidPeriod(TIM0,u8ValidPeriod);             //�����������

    u16CntValue = 0;
    Bt_M23_Cnt16Set(TIM0, u16CntValue);                    //���ü�����ֵ

    Bt_ClearAllIntFlag(TIM0);                              //���жϱ�־
    EnableNvic(TIM0_IRQn, IrqLevel0, TRUE);                //TIM0�ж�ʹ��
    Bt_Mode23_EnableIrq(TIM0,BtUevIrq);                    //ʹ��TIM0 UEV�����ж�

    Bt_M23_Run(TIM0);                                      //TIM0 ����
}

void bsp_bt_pwm_ctrl_output(bool en)
{
    if(en == true)
    {
        Gpio_SetAfMode(GpioPortA,GpioPin15,GpioAf5);            //����TIM0 PWM���
        Bt_M23_EnPWM_Output(TIM0, TRUE, FALSE);
    }
    else
    {
        Bt_M23_EnPWM_Output(TIM0, FALSE, FALSE);
        Gpio_SetAfMode(GpioPortA,GpioPin15,GpioAf0);            //�ر�TIM0 PWM���
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
    
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE); 		 //�˿�����ʱ��ʹ��
    
    stcTIM6Port.enDir  = GpioDirOut;
    //PB12����ΪTIM6_CHA
    Gpio_Init(GpioPortB, GpioPin12, &stcTIM6Port);
    Gpio_SetAfMode(GpioPortB,GpioPin12,GpioAf7);

    Sysctrl_SetPeripheralGate(SysctrlPeripheralAdvTim, TRUE);    //ADT����ʱ��ʹ��
    
    stcAdtBaseCntCfg.enCntMode = AdtSawtoothMode;              	 //��ݲ�ģʽ
    stcAdtBaseCntCfg.enCntDir = AdtCntUp;
    stcAdtBaseCntCfg.enCntClkDiv = AdtClkPClk0;
    
    Adt_Init(M0P_ADTIM6, &stcAdtBaseCntCfg);                  //ADT�ز�������ģʽ��ʱ������
		
		//0x4572
    Adt_SetPeriod(M0P_ADTIM6, 0x5200);                        //�������� ����ֵ��17778 PWMƵ��Ϊ2.7KHz
    
    enAdtCompareA = AdtCompareA;
		//0x22B9
    Adt_SetCompareValue(M0P_ADTIM6, enAdtCompareA, 0x3D80);   //ͨ�ñȽϻ�׼ֵ�Ĵ���A����(ռ�ձ�50%)

    
    stcAdtTIM6ACfg.enCap = AdtCHxCompareOutput;            		//�Ƚ����
    stcAdtTIM6ACfg.bOutEn = TRUE;                          		//CHA���ʹ��
    stcAdtTIM6ACfg.enPerc = AdtCHxPeriodLow;              		//����ֵ������ƥ��ʱCHA��ƽ���ֲ���
    stcAdtTIM6ACfg.enCmpc = AdtCHxCompareHigh;             		//����ֵ��Ƚ�ֵAƥ��ʱ��CHA��ƽ��ת
    stcAdtTIM6ACfg.enStaStp = AdtCHxStateSelSS;            		//CHA��ʼ������ƽ��STACA��STPCA����
    stcAdtTIM6ACfg.enStaOut = AdtCHxPortOutLow;            		//CHA��ʼ��ƽΪ��
    stcAdtTIM6ACfg.enStpOut = AdtCHxPortOutLow;            		//CHA������ƽΪ��
    Adt_CHxXPortCfg(M0P_ADTIM6, AdtCHxA, &stcAdtTIM6ACfg);    //�˿�CHA����
}

void bsp_adt_pwm_ctrl_output(bool en)
{
    if(en == true)
    {
        Gpio_SetAfMode(GpioPortB,GpioPin12,GpioAf7);            //����TIM0 PWM���
        Adt_StartCount(M0P_ADTIM6); //AdvTimer6����
    }
    else
    {
        Adt_StopCount(M0P_ADTIM6); //AdvTimer6ֹͣ
        Gpio_SetAfMode(GpioPortB,GpioPin12,GpioAf0);            //�ر�TIM0 PWM���
        Gpio_ClrIO(GpioPortB,GpioPin12);
    }
}
