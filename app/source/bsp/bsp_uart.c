#include "bsp_uart.h"
#include "bsp_gpio.h"
#include "gpio.h"
#include "pama.h"
#include "segment.h"

//状态存储
extern DeviceState	state;					// 设备状态
//低电量提醒
extern bool low_power_remind_enable;

uint8_t u8TxData[10] = {0xAA, 0x55, 0x00, 0x01, 0x0C, 0x1E, 0x2E, 0x1A,  0x55, 0xAA};
uint8_t u8RxData[3];

uint8_t u8SetDurationData[5] = {0xDD, 0x00, 0xAA, 0x00, 0xAD};
uint8_t u8SetAlarmData[5] = {0xDA, 0x00, 0xAA, 0x00, 0xAD};

uint8_t u8TxCnt=0,u8RxCnt=0;
uint8_t uart_Send_Length = 0;

uint8_t durationSetStep = 0;
uint8_t alarmSetStep = 0;

//UART1中断函数
void Uart1_IRQHandler(void)
{
    if(Uart_GetStatus(M0P_UART1, UartRC))         //UART1数据接收
    {
        Uart_ClrStatus(M0P_UART1, UartRC);        //清中断状态位
				
        u8RxData[u8RxCnt] = Uart_ReceiveData(M0P_UART1);   //接收数据字节
        u8RxCnt++;
				if(u8RxCnt >= 3 && u8RxData[0] == 0xFF)
        {
            u8RxCnt = 0;
						u8TxCnt = 0;
						switch(u8RxData[1])
						{
								case 0x00:
										//取消
										if(state.nPageShow == PAGE_TIMING)
										{
												state.nKeyPress = HARD_KEY_CANCEL;
										}
										else
										{
												durationSetStep = 0;
												state.nPageShow = PAGE_SLEEP;
												u8SetDurationData[1] = 0;
												u8SetDurationData[3] = 0;
												alarmSetStep = 0;
												state.nPageShow = PAGE_SLEEP;
												u8SetAlarmData[1] = 0;
												u8SetAlarmData[3] = 0;
										}
										break;
								case 0x11:
										//开灯
										state.nVoicePage = 1;
										Gpio_SetIO(GpioPortA, GpioPin8);
										break;
								case 0x22:
										//关灯
										state.nVoicePage = 2;
										Gpio_ClrIO(GpioPortA, GpioPin8);
										break;
								case 0x33:
										//显示调亮
										state.nVoicePage = 3;
										break;
								case 0x44:
										//显示调暗
										state.nVoicePage = 4;
										break;
								case 0x55:
										//关闭闹铃
										state.nVoicePage = 5;
										//中断才能进入
										if(state.nPageShow == PAGE_ALARM)
										{
												state.nPageShow = PAGE_SLEEP;
										}
										break;
								case 0x66:
										//显示时间
										state.nVoicePage = 6;
										break;
								case 0x77:
										//显示温度
										state.nVoicePage = 7;
										break;
								case 0x88:
										//显示湿度
										state.nVoicePage = 8;
										break;
								case 0x99:
										//取消定时
										state.nVoicePage = 9;
										break;
								case 0xAA:
										//取消闹钟
										state.nVoicePage = 10;
										break;
								case 0xBB:
										//确定
										if(durationSetStep>0)
										{
												durationSetStep = 7;
										}
										if(alarmSetStep>0)
										{
												alarmSetStep = 7;
										}
										break;
								case 0xCC:
										//低电量提醒
										low_power_remind_enable = false;
										break;
								case 0xDD:
										//计时
										state.nVoicePage = 13;
										break;
								default:
										state.nVoicePage = 0;
										break;
						}
        }
				//如果不是控制数据就每次清空
				if(u8RxData[0] != 0xFF)
				{
						u8RxCnt = 0;
						u8TxCnt = 0;
						switch(durationSetStep)
						{
								case 0:
										if(u8RxData[0] == 0xDD)
										{
												//设定定时1
												state.nPageShow = PAGE_DURATION_SET;
												durationSetStep = 1;
										}
										break;
								case 1:
										if(u8RxData[0]>10)
										{
												//异常
												state.nPageShow = PAGE_SLEEP;
												durationSetStep = 0;
										}
										else
										{
												u8SetDurationData[1] = u8RxData[0];
												u8SetDurationData[3] = u8RxData[0];
												durationSetStep = 2;
										}
										break;
								case 2:
										if(u8RxData[0]>9)
										{
												if(u8RxData[0] == 0xAA)
												{
														u8SetDurationData[3] = 0;
														durationSetStep = 4;
												}
												else if(u8RxData[0] == 0xAD)
												{
														u8SetDurationData[1] = 0;
														durationSetStep = 6;
												}
												else if(u8RxData[0] == 0x0A && u8SetDurationData[1]<10)
												{
														u8SetDurationData[1] = u8SetDurationData[1]*10;
														u8SetDurationData[3] = u8SetDurationData[3]*10;
														durationSetStep = 2;
												}
												else
												{
														//异常
														state.nPageShow = PAGE_SLEEP;
														u8SetDurationData[1] = 0;
														u8SetDurationData[3] = 0;
														durationSetStep = 0;
												}
										}
										else
										{
												u8SetDurationData[1] += u8RxData[0];
												u8SetDurationData[3] += u8RxData[0];
												durationSetStep = 3;
										}
										break;
								case 3:
										if(u8RxData[0] == 0xAA)
										{
												u8SetDurationData[3] = 0;
												durationSetStep = 4;
										}
										else if(u8RxData[0] == 0xAD)
										{
												u8SetDurationData[1] = 0;
												durationSetStep = 6;
										}
										else
										{
												//异常
												state.nPageShow = PAGE_SLEEP;
												u8SetDurationData[1] = 0;
												u8SetDurationData[3] = 0;
												durationSetStep = 0;
										}
										break;
								case 4:
										if(u8RxData[0]>10)
										{
												//异常
												state.nPageShow = PAGE_SLEEP;
												u8SetDurationData[1] = 0;
												u8SetDurationData[3] = 0;
												durationSetStep = 0;
										}
										else
										{
												u8SetDurationData[3] = u8RxData[0];
												durationSetStep = 5;
										}
										break;
								case 5:
										if(u8RxData[0]>9)
										{
												if(u8RxData[0] == 0xAD)
												{
														u8SetDurationData[4] = 0xAD;
														durationSetStep = 6;
												}
												else if(u8RxData[0] == 0x0A && u8SetDurationData[3]<6)
												{
														u8SetDurationData[3] = u8SetDurationData[3]*10;
														durationSetStep = 5;
												}
												else
												{
														//异常
														state.nPageShow = PAGE_SLEEP;
														u8SetDurationData[1] = 0;
														u8SetDurationData[3] = 0;
														durationSetStep = 0;
												}
										}
										else
										{
												if(u8SetDurationData[3]%10==0)
												{
														u8SetDurationData[3] += u8RxData[0];
												}
												durationSetStep = 5;
										}
										break;
						}
						switch(alarmSetStep)
						{
								case 0:
										if(u8RxData[0] == 0xDA)
										{
												//设定闹钟1
												state.nPageShow = PAGE_ALARM_SET;
												alarmSetStep = 1;
										}
										break;
								case 1:
										if(u8RxData[0]>10)
										{
												if(u8RxData[0] == 0xAA)
												{
														u8SetAlarmData[1] = 0;
														alarmSetStep = 4;
												}
												else
												{
														//异常
														state.nPageShow = PAGE_SLEEP;
														alarmSetStep = 0;
												}
										}
										else
										{
												u8SetAlarmData[1] = u8RxData[0];
												alarmSetStep = 2;
										}
										break;
								case 2:
										if(u8RxData[0]>9)
										{
												if(u8RxData[0] == 0xAA)
												{
														u8SetAlarmData[3] = 0;
														alarmSetStep = 4;
												}
												else if(u8RxData[0] == 0x0A && u8SetAlarmData[1]<3)
												{
														u8SetAlarmData[1] = u8SetAlarmData[1]*10;
														alarmSetStep = 2;
												}
												else
												{
														//异常
														state.nPageShow = PAGE_SLEEP;
														u8SetAlarmData[1] = 0;
														u8SetAlarmData[3] = 0;
														alarmSetStep = 0;
												}
										}
										else
										{
												u8SetAlarmData[1] += u8RxData[0];
												alarmSetStep = 3;
										}
										break;
								case 3:
										if(u8RxData[0] == 0xAA)
										{
												u8SetAlarmData[3] = 0;
												alarmSetStep = 4;
										}
										else
										{
												//异常
												state.nPageShow = PAGE_SLEEP;
												u8SetAlarmData[1] = 0;
												u8SetAlarmData[3] = 0;
												alarmSetStep = 0;
										}
										break;
								case 4:
										if(u8RxData[0]>10)
										{
												//异常
												state.nPageShow = PAGE_SLEEP;
												u8SetAlarmData[1] = 0;
												u8SetAlarmData[3] = 0;
												alarmSetStep = 0;
										}
										else
										{
												u8SetAlarmData[3] = u8RxData[0];
												alarmSetStep = 5;
										}
										break;
								case 5:
										if(u8RxData[0]>9)
										{
												if(u8RxData[0] == 0xAD)
												{
														alarmSetStep = 6;
												}
												else if(u8RxData[0] == 0x0A && u8SetAlarmData[3]<6)
												{
														u8SetAlarmData[3] = u8SetAlarmData[3]*10;
														alarmSetStep = 5;
												}
												else
												{
														//异常
														state.nPageShow = PAGE_SLEEP;
														u8SetAlarmData[1] = 0;
														u8SetAlarmData[3] = 0;
														alarmSetStep = 0;
												}
										}
										else
										{
												if(u8SetAlarmData[3]%10==0)
												{
														u8SetAlarmData[3] += u8RxData[0];
												}
												alarmSetStep = 5;
										}
										break;
						}
				}
        Uart_ClrStatus(M0P_UART1, UartRC);              //清除中断状态位
    }
		if(Uart_GetStatus(M0P_UART1, UartFE)||Uart_GetStatus(M0P_UART1, UartPE))
    {
        //用户根据需要自行修改
        Uart_ClrStatus(M0P_UART1, UartFE);
        Uart_ClrStatus(M0P_UART1, UartPE);
    }
    if(Uart_GetStatus(M0P_UART1, UartTC))         //UART1数据发送
    {
				if(u8TxCnt < uart_Send_Length)
        {
            Uart_SendDataIt(M0P_UART1, u8TxData[u8TxCnt]);//发送数据
            u8TxCnt++;
        }
				else
        {
            u8TxCnt = 0;
            u8RxCnt = 0;
        }
        Uart_ClrStatus(M0P_UART1, UartTC);        //清中断状态位
    }

}


void Uart1_Port_Init()
{
		stc_gpio_cfg_t stcGpioCfg;

    DDL_ZERO_STRUCT(stcGpioCfg);

    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE); //使能GPIO模块时钟

    stcGpioCfg.enDir = GpioDirOut;
    Gpio_Init(GpioPortD,GpioPin0,&stcGpioCfg);
    Gpio_SetAfMode(GpioPortD,GpioPin0,GpioAf3);             ///<配置PD00 为UART1 TX
    stcGpioCfg.enDir = GpioDirIn;
    Gpio_Init(GpioPortD,GpioPin1,&stcGpioCfg);
    Gpio_SetAfMode(GpioPortD,GpioPin1,GpioAf3);             ///<配置PD01 为UART1 RX
}

void Uart1_Port_Cfg()
{
	stc_uart_cfg_t    stcCfg;

    DDL_ZERO_STRUCT(stcCfg);

    ///< 开启外设时钟
    Sysctrl_SetPeripheralGate(SysctrlPeripheralUart1,TRUE);///<使能uart1模块时钟

    ///<UART Init
    stcCfg.enRunMode        = UartMskMode1;          ///<模式3
    stcCfg.enStopBit        = UartMsk1bit;           ///<1bit停止位
    stcCfg.stcBaud.u32Baud  = 9600;                  ///<波特率9600
    stcCfg.stcBaud.enClkDiv = UartMsk8Or16Div;       ///<通道采样分频配置
    stcCfg.stcBaud.u32Pclk  = Sysctrl_GetPClkFreq(); ///<获得外设时钟（PCLK）频率值
    Uart_Init(M0P_UART1, &stcCfg);                   ///<串口初始化

    ///<UART中断使能
    Uart_ClrStatus(M0P_UART1,UartRC);                ///<清接收请求
    Uart_ClrStatus(M0P_UART1,UartTC);                ///<清接收请求
    Uart_EnableIrq(M0P_UART1,UartRxIrq);             ///<使能串口接收中断
    Uart_EnableIrq(M0P_UART1,UartTxIrq);             ///<使能串口接收中断
    EnableNvic(UART1_IRQn, IrqLevel0, TRUE);       ///<系统中断使能
}
