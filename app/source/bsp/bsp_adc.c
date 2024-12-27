#include "bsp_adc.h"
#include "bsp_delay.h"

///< ADC模块 初始化
void ADC_Init(void)
{
		stc_adc_cfg_t              stcAdcCfg;

    DDL_ZERO_STRUCT(stcAdcCfg);

    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio, TRUE);
    Sysctrl_SetPeripheralGate(SysctrlPeripheralAdcBgr, TRUE);

    //ADC配置
    Adc_Enable();
    M0P_BGR->CR_f.BGR_EN = 0x1u;                 //BGR必须使能
    M0P_BGR->CR_f.TS_EN = 0x0u;
    delay_us(100);

    stcAdcCfg.enAdcOpMode 			= AdcSglMode;         //扫描转换模式
    stcAdcCfg.enAdcClkDiv 			= AdcClkSysTDiv8;
    stcAdcCfg.enAdcSampTimeSel 	= AdcSampTime12Clk;
		stcAdcCfg.enAdcRefVolSel  	= RefVolSelInBgr1p5;
		stcAdcCfg.enAdcOpBuf				= AdcMskBufEnable;         	///<OP BUF配置-开
		
    Adc_Init(&stcAdcCfg);

    Adc_ConfigSglMode(&stcAdcCfg);
}

//获取输入电压
uint16_t Get_Bat_Voltage()
{
		uint16_t BATAdcRestult;

		//< 启动单次转换采样
    Adc_SGL_Start(); 

		//检测1/3输入电压
		Adc_ConfigSglChannel(AdcAVccDiV3Input);

		///< 获取采样值
		delay_ms(100);
		Adc_GetSglResult(&BATAdcRestult);

		//< 停止单次转换采样
		Adc_SGL_Stop();

		return BATAdcRestult/94;
}
