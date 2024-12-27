#include "bsp_gpio.h"
#include "bsp_delay.h"
#include "bsp_uart.h"
#include "bsp_pwm.h"

#include "sht3x.h"
#include "ds3231.h"
#include "tm1652.h"

#include "pama.h"
#include "segment.h"

uint8_t flag=1;
bool isSleep = false;

//读取温湿度
extern uint8_t recv_dat_list[6];

//读取的温湿度值
extern float temperature;
extern float humidity;

//读取的当前时间
extern _calendar_obj calendar;	//日历结构体

//状态存储
extern DeviceState		state;					// 设备状态
//参数存储
extern MainConfig		g_tMainConfig;		// 系统配置

//发送语音控制命令
extern uint8_t u8TxData[10];
extern uint8_t u8TxCnt;
extern uint8_t uart_Send_Length;

uint8_t tempDs3231Hour;

///< PortA 中断服务函数
//按键中断，无信号高电平，有信号低电平
void PortA_IRQHandler(void)
{
		//上键
    if(TRUE == Gpio_GetIrqStatus(GpioPortA, GpioPin3))
    {
				//上键长按，定时1开关
				if(PAGE_MAIN == state.nPageShow||PAGE_SLEEP == state.nPageShow)
				{
						if(isSleep == false )
						{
								flag=1;
								for(int i=0; i<10; i++)
								{
										delay_ms(300);
										if(Gpio_GetInputIO(GpioPortA, GpioPin3)==1)
										{
												flag=0;
												break;
										}
								}
								if(flag==1)
								{
										state.nPageShow = PAGE_TIMER_1; 
										state.nKeyPress = HARD_KEY_EMPTY;		// 清按键
								}
								else
								{
										//未休眠主页，短按显示一遍
										state.nKeyPress = HARD_KEY_UP;	// 读按键
										state.nPageShow = PAGE_SLEEP;
								}
						}
				}
				//上键短按
				else
				{
						if(Gpio_GetInputIO(GpioPortA, GpioPin3)==0)
						{
								delay_ms(10);
								if(Gpio_GetInputIO(GpioPortA, GpioPin3)==0)
								{
										if(state.nPageShow == PAGE_ALARM)
										{
												state.nPageShow = PAGE_SLEEP;
										}
										else
										{
												state.nKeyPress = HARD_KEY_UP;	// 读按键
										}
								}
						}
				}
				Gpio_ClearIrq(GpioPortA, GpioPin3);
    }
		//下键
    if(TRUE == Gpio_GetIrqStatus(GpioPortA, GpioPin4))
    {
				//下键长按，定时2开关
				if(PAGE_MAIN == state.nPageShow||PAGE_SLEEP == state.nPageShow)
				{
						if(isSleep == false )
						{
								flag=1;
								for(int i=0; i<10; i++)
								{
										delay_ms(300);
										if(Gpio_GetInputIO(GpioPortA, GpioPin4)==1)
										{
												flag=0;
												break;
										}
								}
								if(flag==1)
								{
										state.nPageShow = PAGE_TIMER_2; 
										state.nKeyPress = HARD_KEY_EMPTY;		// 清按键
								}
								else
								{
										//未休眠主页，短按显示一遍
										state.nKeyPress = HARD_KEY_DOWN;	// 读按键
										state.nPageShow = PAGE_SLEEP;
								}
						}
				}
				else
				{
						if(Gpio_GetInputIO(GpioPortA, GpioPin4)==0)
						{
								delay_ms(10);
								if(Gpio_GetInputIO(GpioPortA, GpioPin4)==0)
								{
										if(state.nPageShow == PAGE_ALARM)
										{
												state.nPageShow = PAGE_SLEEP;
										}
										else
										{
												state.nKeyPress = HARD_KEY_DOWN;	// 读按键
										}
								}
						}
				}
				Gpio_ClearIrq(GpioPortA, GpioPin4);
    }
		//取消键
    if(TRUE == Gpio_GetIrqStatus(GpioPortA, GpioPin5))
    {
				//取消键长按控制模式调整
				if(PAGE_MAIN == state.nPageShow||PAGE_SLEEP == state.nPageShow||(state.nFuncIndex==11&&PAGE_SET == state.nPageShow))
				{
						if(isSleep == false )
						{
								flag=1;
								for(int i=0; i<10; i++)
								{
										delay_ms(300);
										if(Gpio_GetInputIO(GpioPortA, GpioPin5)==1)
										{
												flag=0;
												break;
										}
								}
								if(state.nFuncIndex==11&&PAGE_SET == state.nPageShow)
								{
										if(flag==1)
										{
												state.nPageShow = PAGE_SLEEP;  
												state.nKeyPress = HARD_KEY_EMPTY;		// 清按键
										}
										else
										{
												state.nKeyPress = HARD_KEY_CANCEL;	// 读按键
										}
								}
								else
								{
										if(flag==1)
										{
												state.nPageShow = PAGE_CTRL;  
												state.nKeyPress = HARD_KEY_EMPTY;		// 清按键
										}
										else
										{
												state.nKeyPress = HARD_KEY_CANCEL;	// 读按键
												state.nPageShow = PAGE_SLEEP;
										}
								}
						}
				}
				else
				{
						if(Gpio_GetInputIO(GpioPortA, GpioPin5)==0)
						{
								delay_ms(10);
								if(Gpio_GetInputIO(GpioPortA, GpioPin5)==0)
								{
										if(state.nPageShow == PAGE_ALARM)
										{
												state.nPageShow = PAGE_SLEEP;
										}
										else
										{
												state.nKeyPress = HARD_KEY_CANCEL;	// 读按键
										}
								}
						}
				}
				Gpio_ClearIrq(GpioPortA, GpioPin5);
    }
		//设置键
    if(TRUE == Gpio_GetIrqStatus(GpioPortA, GpioPin6))
    {
				//设置键长按，进入设置菜单
				if(PAGE_MAIN == state.nPageShow||PAGE_SLEEP == state.nPageShow)
				{
						if(isSleep == false )
						{
								flag=1;
								for(int i=0; i<10; i++)
								{
										delay_ms(300);
										if(Gpio_GetInputIO(GpioPortA, GpioPin6)==1)
										{
												flag=0;
												break;
										}
								}
								if(flag==1)
								{
										state.nPageShow = PAGE_SET;  
										state.nKeyPress = HARD_KEY_EMPTY;		// 清按键
								}
								else
								{
										state.nKeyPress = HARD_KEY_SET;	// 读按键
										state.nPageShow = PAGE_SLEEP;
								}
						}
				}
				else
				{
						if(Gpio_GetInputIO(GpioPortA, GpioPin6)==0)
						{
								delay_ms(10);
								if(Gpio_GetInputIO(GpioPortA, GpioPin6)==0)
								{
										if(state.nPageShow == PAGE_ALARM)
										{
												state.nPageShow = PAGE_SLEEP;
										}
										else
										{
												state.nKeyPress = HARD_KEY_SET;	// 读按键
										}
								}
						}
				}
				Gpio_ClearIrq(GpioPortA, GpioPin6);
    }
		//人体红外中断，无信号低电平，有信号高电平，封锁时间2.3s
		if(TRUE == Gpio_GetIrqStatus(GpioPortA, GpioPin9))
    {
				delay_ms(500);
				if(Gpio_GetInputIO(GpioPortA, GpioPin9)==1 && Gpio_GetInputIO(GpioPortB, GpioPin13)==0)
				{
						if(state.nPageShow == PAGE_SLEEP && state.nKeyPress == HARD_KEY_EMPTY)
						{
								state.nPageShow = PAGE_MAIN; 
						}
				}
				Gpio_ClearIrq(GpioPortA, GpioPin9);
    }
		//实时时钟闹钟中断，无信号高电平，有信号低电平
		if(TRUE == Gpio_GetIrqStatus(GpioPortA, GpioPin2))
    {
				if(Gpio_GetInputIO(GpioPortA, GpioPin2)==0)
				{
						state.nPageShow = PAGE_ALARM; 
				}
				Gpio_ClearIrq(GpioPortA, GpioPin2);
    }
}

///< PortB 中断服务函数
//语音模块中断，无信号低电平，有信号高电平
void PortB_IRQHandler(void)
{
		//SU-G1 播报控制
		if(TRUE == Gpio_GetIrqStatus(GpioPortB, GpioPin13))
    {
				delay_ms(100);                                                                                                                                                                                        
				//播报控制
				if(Gpio_GetInputIO(GpioPortB, GpioPin13))
				{
						if(!Gpio_GetInputIO(GpioPortB, GpioPin14))
						{
								u8TxCnt = 1;
								u8TxData[2] = 0x02;		//时钟串口触发编号为2
								if(temperature>=0)
								{
										u8TxData[3] = 0;
								}
								else
								{
										u8TxData[3] = 1;
										temperature = -temperature;
								}
								u8TxData[4] = (int)temperature;
								u8TxData[5] = (int)((temperature-(int)temperature)*10);
								
								if((humidity-(int)humidity)*10 > 4)
								{
										u8TxData[6] = (int)humidity+1;
								}
								else
								{
										u8TxData[6] = (int)humidity;
								}
								u8TxData[7] = 0x55;
								u8TxData[8] = 0xAA;
								uart_Send_Length = 9;
								Uart_SendDataIt(M0P_UART1, u8TxData[0]); //启动UART1发送第一个字   
						}
				}
				Gpio_ClearIrq(GpioPortB, GpioPin13);
		}
		//SU-G2 播报控制
		if(TRUE == Gpio_GetIrqStatus(GpioPortB, GpioPin14))
    {
				delay_ms(100); 
				//播报控制
				if(Gpio_GetInputIO(GpioPortB, GpioPin13))
				{
						if(Gpio_GetInputIO(GpioPortB, GpioPin14))
						{
								//播报时间
								I2C_DS3231_ReadTime();
								//播报时间
								u8TxCnt = 1;
								u8TxData[2] = 0x01;		//时钟串口触发编号为1
								tempDs3231Hour = calendar.hour;
								if(g_tMainConfig.timeMode)
								{
										if(tempDs3231Hour > 12)
										{
												tempDs3231Hour -= 12;
												if(tempDs3231Hour <= 6)
												{
														u8TxData[3] = 3;
												}
												else
												{
														u8TxData[3] = 4;
												}
										}
										else
										{
												if(tempDs3231Hour <= 5)
												{
														u8TxData[3] = 1;
												}
												else
												{
														u8TxData[3] = 2;
												}
										}
								}
								else
								{
										u8TxData[3] = 0;
								}
								u8TxData[4] = tempDs3231Hour;
								u8TxData[5] = calendar.minute;
								u8TxData[6] = 0x55;
								u8TxData[7] = 0xAA;
								uart_Send_Length = 8;
								Uart_SendDataIt(M0P_UART1, u8TxData[0]); //启动UART1发送第一个字
						}
				}
				else
				{
						if(Gpio_GetInputIO(GpioPortB, GpioPin14))
						{
								//播报日期
								I2C_DS3231_ReadTime();
								u8TxCnt = 1;
								u8TxData[2] = 0x03;		//时钟串口触发编号为3
								u8TxData[3] = calendar.yearL/10;	//年十位
								u8TxData[4] = calendar.yearL%10;	//年个位
								u8TxData[5] = calendar.month;			//月
								u8TxData[6] = calendar.date;			//日
								u8TxData[7]	= calendar.week;			//星期
								u8TxData[8] = 0x55;
								u8TxData[9] = 0xAA;
								uart_Send_Length = 10;
								Uart_SendDataIt(M0P_UART1, u8TxData[0]); //启动UART1发送第一个字
						}
				}
				Gpio_ClearIrq(GpioPortB, GpioPin14);
		}
}

//按键引脚初始化
void Bsp_Key_Gpio_Init(void)
{
		stc_gpio_cfg_t stcGpioCfg;
    DDL_ZERO_STRUCT(stcGpioCfg);

		///<使能GPIO外设时钟门控开关
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE);

    stcGpioCfg.enDir = GpioDirIn;
    stcGpioCfg.enOD = GpioOdDisable;
    stcGpioCfg.enPu = GpioPuEnable;
		stcGpioCfg.enPd = GpioPdDisable;
    stcGpioCfg.enDrv = GpioDrvL;
		
		//key1 上键
		Gpio_SetAfMode(GpioPortA, GpioPin3, GpioAf0);
    Gpio_Init(GpioPortA, GpioPin3, &stcGpioCfg); 							//初始化为输入模式
		//key2 下键
    Gpio_SetAfMode(GpioPortA, GpioPin4, GpioAf0);
    Gpio_Init(GpioPortA, GpioPin4, &stcGpioCfg); 							//初始化为输入模式
		//key3 确认键
    Gpio_SetAfMode(GpioPortA, GpioPin5, GpioAf0);
    Gpio_Init(GpioPortA, GpioPin5, &stcGpioCfg); 							//初始化为输入模式
		//key4 取消键
    Gpio_SetAfMode(GpioPortA, GpioPin6, GpioAf0);
    Gpio_Init(GpioPortA, GpioPin6, &stcGpioCfg); 							//初始化为输入模式
	
		Gpio_ClearIrq(GpioPortA, GpioPin3);
    Gpio_ClearIrq(GpioPortA, GpioPin4);
    Gpio_ClearIrq(GpioPortA, GpioPin5);
    Gpio_ClearIrq(GpioPortA, GpioPin6);

		//< 打开并配置按键端口为下降沿中断
		Gpio_EnableIrq(GpioPortA, GpioPin3, GpioIrqFalling);
		Gpio_EnableIrq(GpioPortA, GpioPin4, GpioIrqFalling);
		Gpio_EnableIrq(GpioPortA, GpioPin5, GpioIrqFalling);
    Gpio_EnableIrq(GpioPortA, GpioPin6, GpioIrqFalling);

    EnableNvic(PORTA_IRQn, IrqLevel2, TRUE);
}

//人体红外引脚初始化
void Bsp_TGS_Gpio_Init(void)
{
		stc_gpio_cfg_t stcGpioCfg;
    DDL_ZERO_STRUCT(stcGpioCfg);

		///<使能GPIO外设时钟门控开关
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE);

    stcGpioCfg.enDir = GpioDirIn;
    stcGpioCfg.enPu = GpioPuEnable;
		stcGpioCfg.enPd = GpioPdDisable;
    stcGpioCfg.enDrv = GpioDrvL;
		
		//人体感应
    Gpio_Init(GpioPortA, GpioPin9, &stcGpioCfg); 							//初始化为输入模式
		
		//< 人体感应中断，上升沿
		Gpio_EnableIrq(GpioPortA, GpioPin9, GpioIrqRising);
		
		EnableNvic(PORTA_IRQn, IrqLevel3, TRUE);
}

//实时时钟闹钟功能
void Bsp_Alarm_Gpio_Init(void)
{
		stc_gpio_cfg_t stcGpioCfg;
    DDL_ZERO_STRUCT(stcGpioCfg);

		///<使能GPIO外设时钟门控开关
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE);

    stcGpioCfg.enDir = GpioDirIn;
    stcGpioCfg.enOD = GpioOdDisable;
    stcGpioCfg.enPu = GpioPuEnable;
		stcGpioCfg.enPd = GpioPdDisable;
    stcGpioCfg.enDrv = GpioDrvL;
		
		//实时时钟闹钟中断
		Gpio_SetAfMode(GpioPortA, GpioPin2, GpioAf0);
    Gpio_Init(GpioPortA, GpioPin2, &stcGpioCfg); 							//初始化为输入模式
	
		Gpio_ClearIrq(GpioPortA, GpioPin2);

		//< 打开并配置闹钟中断，上升沿
		Gpio_EnableIrq(GpioPortA, GpioPin2, GpioIrqFalling);

    EnableNvic(PORTA_IRQn, IrqLevel2, TRUE);
}

//蜂鸣器引脚初始化
void Bsp_Horn_Gpio_Init(void)
{
		//蜂鸣器使用PWM驱动
		bsp_adt_pwm_init();
}


//语音控制引脚初始化
void Bsp_Voice_Gpio_Init(void)
{
		stc_gpio_cfg_t stcGpioCfg;
    DDL_ZERO_STRUCT(stcGpioCfg);

		///<使能GPIO外设时钟门控开关
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE);
		
    stcGpioCfg.enDir = GpioDirIn;
    stcGpioCfg.enPu = GpioPuDisable;
		stcGpioCfg.enPd = GpioPdEnable;
    stcGpioCfg.enDrv = GpioDrvL;

		///< SU_G1使能
		Gpio_Init(GpioPortB,GpioPin13,&stcGpioCfg);
		Gpio_ClrIO(GpioPortB, GpioPin13);

		///< SU_G2使能
		Gpio_Init(GpioPortB,GpioPin14,&stcGpioCfg);
		Gpio_ClrIO(GpioPortB, GpioPin14);


		Gpio_ClearIrq(GpioPortB, GpioPin13);
		Gpio_ClearIrq(GpioPortB, GpioPin14);

		//< 语音模块中断，上升沿
		Gpio_EnableIrq(GpioPortB, GpioPin13, GpioIrqRising);
		Gpio_EnableIrq(GpioPortB, GpioPin14, GpioIrqRising);

		EnableNvic(PORTB_IRQn, IrqLevel3, TRUE);
}


//外部控制引脚初始化
void Bsp_Ctrl_GPIO_Init(void)
{
		stc_gpio_cfg_t stcGpioCfg;	
		DDL_ZERO_STRUCT(stcGpioCfg);  

		///<使能GPIO外设时钟门控开关
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE);

		stcGpioCfg.enDrv = GpioDrvL;
    stcGpioCfg.enDir = GpioDirOut;
		stcGpioCfg.enPu = GpioPuDisable;
    stcGpioCfg.enPd = GpioPdEnable;
    
    ///< 红外使能引脚
		Gpio_Init(GpioPortA,GpioPin10,&stcGpioCfg);
		Gpio_ClrIO(GpioPortA, GpioPin10);

		///< 显示使能引脚
		Gpio_Init(GpioPortB,GpioPin11,&stcGpioCfg);
		Gpio_SetIO(GpioPortB, GpioPin11);
		
		///< SU_EN使能，静音控制
		Gpio_Init(GpioPortB,GpioPin15,&stcGpioCfg);
		Gpio_ClrIO(GpioPortB, GpioPin15);

		//语音开灯
    Gpio_Init(GpioPortA, GpioPin8, &stcGpioCfg);
		Gpio_ClrIO(GpioPortA, GpioPin8);
}

//页面流程初始化
void Bsp_Page_Ctrl_Init(void)
{		
		state.nKeyPress = HARD_KEY_EMPTY;
		state.nPageShow = PAGE_MAIN;		
		state.nFuncIndex = 0;
		state.nVoicePage = 0;
}

