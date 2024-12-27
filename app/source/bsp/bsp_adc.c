#include "bsp_adc.h"
#include "bsp_delay.h"

///< ADCģ�� ��ʼ��
void ADC_Init(void)
{
		stc_adc_cfg_t              stcAdcCfg;

    DDL_ZERO_STRUCT(stcAdcCfg);

    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);
    Sysctrl_SetPeripheralGate(SysctrlPeripheralAdcBgr, TRUE);

    //ADC����
    Adc_Enable();
    M0P_BGR->CR_f.BGR_EN = 0x1u;                 //BGR����ʹ��
    M0P_BGR->CR_f.TS_EN = 0x0u;
    delay_us(100);

    stcAdcCfg.enAdcOpMode 			= AdcSglMode;         //ɨ��ת��ģʽ
    stcAdcCfg.enAdcClkDiv 			= AdcClkSysTDiv8;
    stcAdcCfg.enAdcSampTimeSel 	= AdcSampTime12Clk;
		stcAdcCfg.enAdcRefVolSel  	= RefVolSelInBgr1p5;
		stcAdcCfg.enAdcOpBuf				= AdcMskBufEnable;         	///<OP BUF����-��
		
    Adc_Init(&stcAdcCfg);

    Adc_ConfigSglMode(&stcAdcCfg);
}

//��ȡ�����ѹ
uint16_t Get_Bat_Voltage()
{
		uint16_t BATAdcRestult;

		//< ��������ת������
    Adc_SGL_Start(); 

		//���1/3�����ѹ
		Adc_ConfigSglChannel(AdcAVccDiV3Input);

		///< ��ȡ����ֵ
		delay_ms(100);
		Adc_GetSglResult(&BATAdcRestult);

		//< ֹͣ����ת������
		Adc_SGL_Stop();

		return BATAdcRestult/94;
}
