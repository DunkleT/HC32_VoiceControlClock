#include "segment.h"
#include "pama.h"

#include "bsp_gpio.h"
#include "bsp_delay.h"
#include "bsp_pwm.h"
#include "bsp_adc.h"

#include "tm1652.h"
#include "ds3231.h"
#include "sht3x.h"
#include "uart.h"
#include "i2c.h"
#include "lpm.h"

#define ENABLE_SHOW			Gpio_SetIO(GpioPortB,GpioPin11);delay_ms(10);
#define DISABLE_SHOW		delay_ms(10);Gpio_ClrIO(GpioPortB,GpioPin11);

//读写时间列表定义
uint8_t time_buf_list[8]; //设置时间
uint8_t read_time_list[14];//当前时间

//读取时间
uint8_t ds3231_hour = 0;
uint8_t ds3231_minute = 0;
uint8_t ds3231_second = 0;

//读取温湿度
uint8_t recv_dat_list[6] = {0};

//读取的温湿度值
float temperature = 0.0;
float humidity = 0.0;

extern DevicePama 		config;				  // 设备配置
extern DeviceState		state;					// 设备状态
extern MainConfig		g_tMainConfig;		// 系统配置

//报警时间
uint8_t alarm_yearL = 0;			//报警年份
uint8_t alarm_month = 0;			//报警月份
uint8_t alarm_date = 0;				//报警日期
uint8_t alarm_hour = 0;				//报警小时
uint8_t alarm_minute = 0;			//报警分钟
uint8_t alarm_second = 0;			//报警秒钟

_calendar_obj calendar;	//日历结构体

uint8_t i_index = 1;

//是否显示倒计时
bool	showDurationRemain_1 = false;
bool	showDurationRemain_2 = false;

//语音定时数据存储
extern uint8_t u8SetDurationData[5];
extern uint8_t durationSetStep;
//语音闹钟数据存储
extern uint8_t u8SetAlarmData[5];
extern uint8_t alarmSetStep;

//是否超时
extern bool is_out_time;

//发送语音控制命令
extern uint8_t u8TxData[10];
extern uint8_t u8TxCnt;
extern uint8_t uart_Send_Length;

//读取进入时刻
uint8_t beginDs3231Hour = 0;
uint8_t beginDs3231Minute = 0;
uint8_t beginDs3231Second = 0;

//显示计时时长
uint8_t timingHourCount = 0;
uint8_t timingMinuteCount = 0;
uint8_t timingSecondCount = 0;

//是否还在计时
bool isTiming = false;

//低电量提醒
bool low_power_remind_enable = true;
//低电量显示次数，因为每分钟显示则太频繁，因此计数，每10分钟显示一次
uint8_t low_power_show_count = 0;

//显示页面
void Main_Page_Show(void)
{
		switch(state.nPageShow)
		{
				case PAGE_MAIN:
						//主界面
						Page_Main_Function();
						break;
				case PAGE_SLEEP:
						//休眠界面
						Page_Sleep_Function();
						break;
				case PAGE_CTRL:
						//控制界面
						Page_Ctrl_Function();
						break;
				case PAGE_SET:
						//设置界面
						Page_Set_Function();
						break;
				case PAGE_TIMER_1:
						//定时1界面
						Page_Timer1_Function();
						break;
				case PAGE_TIMER_2:
						//定时2界面
						Page_Timer2_Function();
						break;
				case PAGE_ALARM:
						//报警界面
						Page_Alarm_Function();
						break;
				case PAGE_DURATION_SET:
						//语音定时设定界面
						Page_Duration_Set_Function();
						break;
				case PAGE_ALARM_SET:
						//语音闹钟设定界面
						Page_Alarm_Set_Function();
						break;
				case PAGE_TIMING:
						//计时界面
						Page_Timing_Function();
		}
}

//主循环功能
void Page_Main_Function(void)
{
		//定时总时长控制
		uint8_t delayCount = 30;
		uint8_t tempDelayCount = delayCount;

		ENABLE_SHOW

		//常亮模式不再显示当前时间
		if(!g_tMainConfig.dispalyMode)
		{
				//显示当前时间
				I2C_DS3231_ReadTime();
				ds3231_hour = calendar.hour;					//读取小时数据
				ds3231_minute = calendar.minute;			//读取分钟数据
				if(g_tMainConfig.timeMode)
				{
						if(ds3231_hour > 12)
						{
								ds3231_hour -= 12;
						}
				}
				Tm1652_Time_Show(ds3231_hour,ds3231_minute,1);
				while(tempDelayCount--)				//方便退出
				{
						delay_ms(100);
						if(state.nPageShow != PAGE_MAIN)
						{
								return;
						}
				}
				tempDelayCount = delayCount;
		}

		//读取温湿度数值
		if(SHT30_Read_Data(recv_dat_list) == Ok)
		{
				SHT30_Data_To_Float(recv_dat_list, &temperature, &humidity);
		}

		//显示温度
		Tm1652_Temperature_Show(temperature);
		while(tempDelayCount--)				//方便退出
		{
				delay_ms(100);
				if(state.nPageShow != PAGE_MAIN)
				{
						return;
				}
		}
		tempDelayCount = delayCount;

		//显示湿度
		Tm1652_Humidity_Show(humidity);
		while(tempDelayCount--)				//方便退出
		{
				delay_ms(100);
				if(state.nPageShow != PAGE_MAIN)
				{
						return;
				}
		}
		tempDelayCount = delayCount;
		
		//清空显示
		Tm1652_Show_Close();
		delay_ms(10);

		DISABLE_SHOW

		//页面状态切换
		if(state.nPageShow == PAGE_MAIN)
		{
				state.nPageShow = PAGE_SLEEP;
		}
}

//休眠唤醒功能
void Page_Sleep_Function(void)
{
		//剩余定时时长
		uint8_t remainDurationHour = 0;
		uint8_t remainDurationMinute = 0;
		uint8_t remainDurationSecond = 0;
		//延时总时长控制
		uint8_t tempDelayCount = 0;

		//按键播报退出
		if(state.nFuncIndex == 11)
		{
				state.nFuncIndex = 0;
				ENABLE_SHOW

				Tm1652_Show_Updata('b', 'o', '-', 'C', ' ');
				delay_ms(1000);

				//清屏幕
				Tm1652_Show_Close();
				delay_ms(10);
				
				DISABLE_SHOW
		}

		//按键唤醒
		if(state.nKeyPress != HARD_KEY_EMPTY)
		{
				ENABLE_SHOW

				switch (state.nKeyPress)
				{
						case HARD_KEY_UP:
								//显示定时1状态
								if(g_tMainConfig.tCtrl.alarm_duration_enable_1)
								{
										//定时开,显示剩余时长
										Tm1652_Show_Updata('F', '1', '-', 'O', ' ');
										tempDelayCount = 10;
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										//常亮模式则一直显示倒计时
										if(g_tMainConfig.dispalyMode)
										{
												showDurationRemain_1 = true;
												showDurationRemain_2 = false;
										}
										else
										{
												showDurationRemain_1 = false;
												showDurationRemain_2 = false;
										}
										//显示剩余时长
										if(GetDurationRemainTime(1, &remainDurationHour, &remainDurationMinute, &remainDurationSecond) && showDurationRemain_1 == false)
										{
												//大于1分钟则显示小时分钟，并闪烁冒号
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
										}
										else if(showDurationRemain_1 == false)
										{
												//小于1分钟则显示秒钟  ：32，并闪烁冒号
												Tm1652_Time_Show_Second(remainDurationSecond,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
										}
								}
								else
								{
										//定时关闭,显示设定时长
										Tm1652_Show_Updata('F', '1', '-', 'C', ' ');

										tempDelayCount = 10;
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										Tm1652_Time_Show(g_tMainConfig.tCtrl.alarm_duration_1.duration_hour,g_tMainConfig.tCtrl.alarm_duration_1.duration_minute,1);
										tempDelayCount = 10;
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
								}
								break;
						case HARD_KEY_DOWN:
								//显示定时2状态
								if(g_tMainConfig.tCtrl.alarm_duration_enable_2)
								{
										//定时开启
										Tm1652_Show_Updata('F', '2', '-', 'O', ' ');
										tempDelayCount = 10;
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										//常亮模式则一直显示倒计时
										if(g_tMainConfig.dispalyMode)
										{
												showDurationRemain_1 = false;
												showDurationRemain_2 = true;
										}
										else
										{
												showDurationRemain_1 = false;
												showDurationRemain_2 = false;
										}
										//显示剩余时长
										if(GetDurationRemainTime(2, &remainDurationHour, &remainDurationMinute, &remainDurationSecond) && showDurationRemain_2 == false)
										{
												//大于1分钟则显示小时分钟，并闪烁冒号
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
										}
										else if(showDurationRemain_2 == false)
										{
												//小于1分钟则显示秒钟  ：32，并闪烁冒号
												Tm1652_Time_Show_Second(remainDurationSecond,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
										}
								}
								else
								{
										//定时关闭,显示设定时长
										Tm1652_Show_Updata('F', '2', '-', 'C', ' ');

										tempDelayCount = 10;
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										Tm1652_Time_Show(g_tMainConfig.tCtrl.alarm_duration_2.duration_hour,g_tMainConfig.tCtrl.alarm_duration_2.duration_minute,1);
										tempDelayCount = 10;
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
								}
								break;
						case HARD_KEY_CANCEL:
								if(showDurationRemain_1 || showDurationRemain_2)
								{
										showDurationRemain_1 = false;
										showDurationRemain_2 = false;
								}
								else
								{
										//显示控制模式
										switch (g_tMainConfig.ctrlModeSet)
										{
												case 0:
														Tm1652_Show_Updata('E', 'R', 'E', 'S', ' ');
														break;
												case 1:
														Tm1652_Show_Updata('E', 'R', 'd', 'S', ' ');
														break;
												case 2:
														Tm1652_Show_Updata('d', 'R', 'E', 'S', ' ');
														break;
												case 3:
														Tm1652_Show_Updata('d', 'R', 'd', 'S', ' ');
														break;
										}
										tempDelayCount = 15;
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
								}
								break;
						case HARD_KEY_SET:
								state.nPageShow = PAGE_MAIN;
								break;
						default:
								DISABLE_SHOW
								break;
				}
				
				//清屏幕
				Tm1652_Show_Close();
				delay_ms(10);
				
				DISABLE_SHOW

				//清按键
				state.nKeyPress = HARD_KEY_EMPTY;
		}
		else
		{
				if(state.nVoicePage==0)
				{
						//常亮模式显示当前时间
						if(g_tMainConfig.dispalyMode)
						{
								// 定时器每分钟测量一次温度和电池电压
								if(is_out_time)
								{
										//读取温湿度数值
										if(SHT30_Read_Data(recv_dat_list) == Ok)
										{
												SHT30_Data_To_Float(recv_dat_list, &temperature, &humidity);
										}
										//获取电量  每十分钟提醒一次
										if(Get_Bat_Voltage()<29)
										{
												low_power_show_count++;
												//获取当前时间，夜间不再提醒
												I2C_DS3231_ReadTime();
												if(calendar.hour>=9 && calendar.hour<=21)
												{
														if(low_power_remind_enable && low_power_show_count>=9)
														{
																u8TxCnt = 1;
																u8TxData[2] = 0x0A;		//触发编号为10
																
																u8TxData[3] = 1;
																
																u8TxData[4] = 0x55;
																u8TxData[5] = 0xAA;
																uart_Send_Length = 6;
																Uart_SendDataIt(M0P_UART1, u8TxData[0]); //启动UART1发送第一个字
														}
												}
												if(low_power_show_count>=9)
												{
														ENABLE_SHOW
														tempDelayCount = 5;
														while(tempDelayCount--)				//方便退出
														{
																Tm1652_Show_Updata('L', 'O', 'P', 'R', ' ');
																delay_ms(1000);
																Tm1652_Show_Close();
																delay_ms(1000);
														}
														low_power_show_count = 0;
												}
										}
										is_out_time = false;
								}
								ENABLE_SHOW

								//常亮模式显示当前倒计时
								if(showDurationRemain_1 && !showDurationRemain_2)
								{
										//显示定时器1倒计时
										//显示剩余时长
										if(GetDurationRemainTime(1, &remainDurationHour, &remainDurationMinute, &remainDurationSecond))
										{
												//大于1分钟则显示小时分钟，并闪烁冒号
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
										}
										else
										{
												//小于1分钟则显示秒钟  ：32，并闪烁冒号
												Tm1652_Time_Show_Second(remainDurationSecond,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond-1,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond-1,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
										}
								}
								else if(!showDurationRemain_1 && showDurationRemain_2)
								{
										//显示定时器2倒计时
										//显示剩余时长
										if(GetDurationRemainTime(2, &remainDurationHour, &remainDurationMinute, &remainDurationSecond))
										{
												//大于1分钟则显示小时分钟，并闪烁冒号
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
										}
										else
										{
												//小于1分钟则显示秒钟  ：32，并闪烁冒号
												Tm1652_Time_Show_Second(remainDurationSecond,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond-1,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond-1,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
										}
								}
								else
								{
										//常亮模式，显示当前时间
										I2C_DS3231_ReadTime();
										ds3231_hour = calendar.hour;					//读取小时数据
										ds3231_minute = calendar.minute;			//读取分钟数据
										if(g_tMainConfig.timeMode)
										{
												if(ds3231_hour > 12)
												{
														ds3231_hour -= 12;
												}
										}
										Tm1652_Time_Show(ds3231_hour,ds3231_minute,1);
										//每100ms刷新
										delay_ms(100);
								}
						}
						else
						{
								//省电模式关闭显示
								Tm1652_Show_Close();
								DISABLE_SHOW

								//进入休眠模式
								Lpm_GotoSleep(false);
								
								// 定时器每分钟测量一次温度和电池电压
								if(is_out_time)
								{
										//读取温湿度数值
										if(SHT30_Read_Data(recv_dat_list) == Ok)
										{
												SHT30_Data_To_Float(recv_dat_list, &temperature, &humidity);
										}
										//获取电量  每十分钟提醒一次
										if(Get_Bat_Voltage()<29)
										{
												low_power_show_count++;
												//获取当前时间，夜间不再提醒
												I2C_DS3231_ReadTime();
												if(calendar.hour>=9 && calendar.hour<=21)
												{
														if(low_power_remind_enable && low_power_show_count>=9)
														{
																u8TxCnt = 1;
																u8TxData[2] = 0x0A;		//触发编号为10
																
																u8TxData[3] = 1;
																
																u8TxData[4] = 0x55;
																u8TxData[5] = 0xAA;
																uart_Send_Length = 6;
																Uart_SendDataIt(M0P_UART1, u8TxData[0]); //启动UART1发送第一个字
														}
												}
												if(low_power_show_count>=9)
												{
														ENABLE_SHOW
														tempDelayCount = 5;
														while(tempDelayCount--)				//方便退出
														{
																Tm1652_Show_Updata('L', 'O', 'P', 'R', ' ');
																delay_ms(1000);
																Tm1652_Show_Close();
																delay_ms(1000);
														}
														low_power_show_count = 0;
												}
										}
										is_out_time = false;
								}

						}
				}
				else
				{
						//语音控制唤醒
						ENABLE_SHOW

						switch(state.nVoicePage)
						{
								case 1:
										//开灯
										break;
								case 2:
										//关灯
										break;
								case 3:
										//亮度调亮一点
										config.pConfig = &g_tMainConfig;
										if(config.pConfig->displayLightSet < 8)
										{
												config.pConfig->displayLightSet += 1;
										}
										//显示亮度设置
										Tm1652_Brightness_set(config.pConfig->displayLightSet);
										//显示当前时间作为效果预览
										I2C_DS3231_ReadTime();
										ds3231_hour = calendar.hour;					//读取小时数据
										ds3231_minute = calendar.minute;			//读取分钟数据
										Tm1652_Time_Show(ds3231_hour,ds3231_minute,1);
										//保存参数
										if(MainConfigCorrect())
										{
												MainConfigSave();
										}
										tempDelayCount = 30;
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										//清空显示
										Tm1652_Show_Close();
										delay_ms(10);
										break;
								case 4:
										//亮度调暗一点
										config.pConfig = &g_tMainConfig;
										if(config.pConfig->displayLightSet > 1)
										{
												config.pConfig->displayLightSet -= 1;
										}
										//显示亮度设置
										Tm1652_Brightness_set(config.pConfig->displayLightSet);
										//显示当前时间作为效果预览
										I2C_DS3231_ReadTime();
										ds3231_hour = calendar.hour;					//读取小时数据
										ds3231_minute = calendar.minute;			//读取分钟数据
										Tm1652_Time_Show(ds3231_hour,ds3231_minute,1);
										//保存参数
										if(MainConfigCorrect())
										{
												MainConfigSave();
										}
										tempDelayCount = 30;
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										//清空显示
										Tm1652_Show_Close();
										delay_ms(10);
										break;
								case 5:
										//关闭闹铃
										break;
								case 6:
										//显示时间
										I2C_DS3231_ReadTime();
										ds3231_hour = calendar.hour;					//读取小时数据
										ds3231_minute = calendar.minute;			//读取分钟数据
										if(g_tMainConfig.timeMode)
										{
												if(ds3231_hour > 12)
												{
														ds3231_hour -= 12;
												}
										}
										Tm1652_Time_Show(ds3231_hour,ds3231_minute,1);
										tempDelayCount = 30;
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										//清空显示
										Tm1652_Show_Close();
										delay_ms(10);
										break;
								case 7:
										//显示温度
										Tm1652_Temperature_Show(temperature);
										tempDelayCount = 30;
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										//清空显示
										Tm1652_Show_Close();
										delay_ms(10);
										break;
								case 8:
										//显示湿度
										Tm1652_Humidity_Show(humidity);
										tempDelayCount = 30;
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										//清空显示
										Tm1652_Show_Close();
										delay_ms(10);
										break;
								case 9:
										//取消定时
										I2C_DS3231_SetAlarm_1(false,1,1,1,1);
										I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
										break;
								case 10:
										//取消闹钟
										I2C_DS3231_SetAlarm_2(false, 1, 1);
										I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
										break;
								case 13:
										//计时
										if(state.nPageShow == PAGE_SLEEP)
										{
												//计时功能，进入计时页面
												I2C_DS3231_ReadTime();
												beginDs3231Hour = calendar.hour;					//读取小时数据	
												beginDs3231Minute = calendar.minute;			//读取分钟数据	
												beginDs3231Second = calendar.second;			//读取秒钟数据	

												//显示计时时长
												timingHourCount = 0;
												timingMinuteCount = 0;
												timingSecondCount = 0;

												isTiming =true;
												state.nPageShow = PAGE_TIMING;
										}
										break;
								default:
										DISABLE_SHOW
										break;
						}
						//处理完毕，复位
						state.nVoicePage = 0;
				}
		}
}

//设置参数功能
void Page_Set_Function(void)
{
		bool pageExit = false;

		ENABLE_SHOW

		state.nFuncIndex = 0;
		
		while(!pageExit)
		{
				switch (state.nFuncIndex)
				{
						case 0:
								//计时
								Tm1652_Show_Updata('T', 'C', 'J', 'S', ' ');
								break;
						case 1:
								//闹钟1
								Tm1652_Show_Updata('A', 'L', '-', '1', ' ');
								break;
						case 2:
								//闹钟2
								Tm1652_Show_Updata('A', 'L', '-', '2', ' ');
								break;
						case 3:
								//闹钟3
								Tm1652_Show_Updata('A', 'L', '-', '3', ' ');
								break;
						case 4:
								//设置时分
								Tm1652_Show_Updata('S', 'E', 'H', 'F', ' ');
								break;
						case 5:
								//设置年月日，自动计算星期
								Tm1652_Show_Updata('S', 'E', 'E', 'E', ' ');
								break;
						case 6:
								//温度补偿
								Tm1652_Show_Updata('O', 'F', '-', 'P', ' ');
								break;
						case 7:
								//湿度补偿
								Tm1652_Show_Updata('O', 'F', '-', 'H', ' ');
								break;
						case 8:
								//常亮模式
								Tm1652_Show_Updata('S', 'E', '-', 'C', ' ');
								break;
						case 9:
								//参数恢复出厂
								Tm1652_Show_Updata('R', 'S', 'E', 'T', ' ');
								break;
						case 10:
								//日期格式
								Tm1652_Show_Updata('S', 'E', '-', 'T', ' ');
								break;
						case 11:
								//按键播报
								Tm1652_Show_Updata('k', 'b', 'o', 'c', ' ');
								break;
						default:
								Tm1652_Show_Updata('E', 'R', 'R', ' ', ' ');
								break;
				}
				delay_ms(300);
				if(state.nKeyPress != HARD_KEY_EMPTY)
				{
						switch (state.nKeyPress)
						{
								case HARD_KEY_UP:
										if(state.nFuncIndex == 11)
										{
												state.nFuncIndex = 0;
										}
										else
										{
												state.nFuncIndex++;
										}
										
										break;
								case HARD_KEY_DOWN:
										
										if(state.nFuncIndex == 0)
										{
												state.nFuncIndex = 11;
										}
										else
										{
												state.nFuncIndex--;
										}
										break;
								case HARD_KEY_CANCEL:
										pageExit = true;
										break;
								case HARD_KEY_SET:
										if(state.nFuncIndex == 0)
										{
												//计时功能，进入计时页面
												I2C_DS3231_ReadTime();
												beginDs3231Hour = calendar.hour;					//读取小时数据	
												beginDs3231Minute = calendar.minute;			//读取分钟数据	
												beginDs3231Second = calendar.second;			//读取秒钟数据	

												//显示计时时长
												timingHourCount = 0;
												timingMinuteCount = 0;
												timingSecondCount = 0;

												isTiming =true;
												state.nPageShow = PAGE_TIMING;
										}
										else
										{
												pageExit = Page_Set_Function_Lv2(state.nFuncIndex);
										}
										break;
								default:
										break;
						}
						state.nKeyPress = HARD_KEY_EMPTY;
				}
				if(state.nPageShow != PAGE_SET)
				{
						return;
				}
		}
		//清空显示
		Tm1652_Show_Close();
		delay_ms(10);

		DISABLE_SHOW
		
		//保存设置
		MainConfigSave();

		//页面状态切换
		state.nPageShow = PAGE_SLEEP;
}

//计时功能
void Page_Timing_Function(void)
{
		bool pageExit = false;

		//关闭显示
		bool isShowClose = false;

		//闪烁显示次数
		uint8_t twinkleCount = 0;

		//秒钟读取次数
		uint8_t secondReadCount = 0;

		//暂停时刻
		uint8_t pauseDs3231Hour = 0;
		uint8_t pauseDs3231Minute = 0;
		uint8_t pauseDs3231Second = 0;
		//是否被暂停
		bool isPause = false;

		//暂停时长
		uint32_t pastTime = 0;

		//计算临时变量
		uint32_t i,j;

		//定时总时长控制
		uint8_t delayCount = 4;
		uint8_t tempDelayCount = delayCount;

		ENABLE_SHOW
		
		state.nKeyPress = HARD_KEY_EMPTY;
		
		while(!pageExit)
		{
				// 定时器每分钟测量一次温度和电池电压
				if(is_out_time)
				{
						//读取温湿度数值
						if(SHT30_Read_Data(recv_dat_list) == Ok)
						{
								SHT30_Data_To_Float(recv_dat_list, &temperature, &humidity);
						}
						//获取电量  每十分钟提醒一次
						if(Get_Bat_Voltage()<29)
						{
								low_power_show_count++;
								if(low_power_remind_enable && low_power_show_count>=9)
								{
										u8TxCnt = 1;
										u8TxData[2] = 0x0A;		//触发编号为10

										u8TxData[3] = 1;
										
										u8TxData[4] = 0x55;
										u8TxData[5] = 0xAA;
										uart_Send_Length = 6;
										Uart_SendDataIt(M0P_UART1, u8TxData[0]); //启动UART1发送第一个字
								}
								if(low_power_show_count>=9)
								{
										//只播音不显示
										low_power_show_count = 0;
								}
						}
						is_out_time = false;
				}
				if(isPause)
				{
						//如果被暂停则显示计时时间
						if(timingHourCount>0 || timingMinuteCount>0)
						{
								//时长大于1分钟
								Tm1652_Time_Show(timingHourCount,timingMinuteCount,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(200);
										if(state.nPageShow != PAGE_TIMING)
										{
												return;
										}
										if(state.nKeyPress != HARD_KEY_EMPTY)
										{
												break;
										}
								}
								tempDelayCount = delayCount;
								Tm1652_Show_Close();
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(200);
										if(state.nPageShow != PAGE_TIMING)
										{
												return;
										}
										if(state.nKeyPress != HARD_KEY_EMPTY)
										{
												break;
										}
								}
								tempDelayCount = delayCount;
						}
						else
						{
								//时长小于1分钟
								Tm1652_Time_Show_Second(timingSecondCount,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nPageShow != PAGE_TIMING)
										{
												return;
										}
								}
								tempDelayCount = delayCount;
								Tm1652_Show_Close();
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nPageShow != PAGE_TIMING)
										{
												return;
										}
								}
								tempDelayCount = delayCount;

						}
				}
				else
				{
						//开始计时
						//读取当前时间
						I2C_DS3231_ReadTime();
						//计算小时
						if(timingHourCount>0 || timingMinuteCount>0 || timingSecondCount>0)
						{
								if(calendar.hour<beginDs3231Hour)
								{
										//过了午夜
										timingHourCount = (beginDs3231Hour + 24) - calendar.hour;
								}
								else
								{
										//没有过午夜
										timingHourCount = calendar.hour - beginDs3231Hour;
								}
						}
						//计算分钟
						if(calendar.minute<beginDs3231Minute)
						{
								//过小时了
								timingHourCount -= 1;
								timingMinuteCount = (calendar.minute + 60) - beginDs3231Minute;
						}
						else
						{
								//没有过小时
								timingMinuteCount = calendar.minute - beginDs3231Minute;
						}
						//计算秒钟
						if(calendar.second<beginDs3231Second)
						{
								//过分钟了
								if(timingMinuteCount>0)
								{
										timingMinuteCount -= 1;
								}
								else
								{
										if(timingHourCount>0)
										{
												timingHourCount -= 1;
												timingMinuteCount = 59;
										}
								}
								timingSecondCount = (calendar.second + 60) - beginDs3231Second;
						}
						else
						{
								//没有过分钟
								timingSecondCount = calendar.second - beginDs3231Second;
						}
						if(timingHourCount == 0 && timingMinuteCount == 0)
						{
								//小于1分钟则显示秒钟  ：32，并闪烁冒号
								if(secondReadCount<5)
								{
										Tm1652_Time_Show_Second(timingSecondCount,1);
								}
								else
								{
										Tm1652_Time_Show_Second(timingSecondCount,0);
								}
								//加速刷新确保秒数正确
								delay_ms(100);
								secondReadCount++;
								if(secondReadCount==10)
								{
										secondReadCount = 0;
								}
						}
						else
						{
								//大于1分钟则显示小时分钟，并闪烁冒号
								Tm1652_Time_Show(timingHourCount,timingMinuteCount,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nPageShow != PAGE_TIMING)
										{
												return;
										}
										if(state.nKeyPress != HARD_KEY_EMPTY)
										{
												break;
										}
								}
								tempDelayCount = delayCount;
								Tm1652_Time_Show(timingHourCount,timingMinuteCount,0);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nPageShow != PAGE_TIMING)
										{
												return;
										}
										if(state.nKeyPress != HARD_KEY_EMPTY)
										{
												break;
										}
								}
								tempDelayCount = delayCount;
						}
				}
				//超出最大计时时长则退出
				if(timingHourCount == 23 && timingMinuteCount == 59 && timingSecondCount>55)
				{
						if(!isPause)
						{
								//没有暂停计时则退出
								pageExit = true;
								//关闭计时
								isTiming = false;
						}
				}
				if(state.nKeyPress != HARD_KEY_EMPTY)
				{
						switch (state.nKeyPress)
						{
								case HARD_KEY_UP:
										if(isShowClose)
										{
												//打开显示
												ENABLE_SHOW
												isShowClose = false;
										}
										else
										{
												//上键按下则显示计时秒钟
												if(isPause)
												{
														if(timingHourCount>0 || timingMinuteCount>0)
														{
																//计时大于1分钟
																//如果暂停，则秒钟不跳动
																for(twinkleCount=0; twinkleCount<3; twinkleCount++)
																{
																		Tm1652_Time_Show_Second(timingSecondCount,1);
																		while(tempDelayCount--)				//方便退出
																		{
																				delay_ms(100);
																				if(state.nPageShow != PAGE_TIMING)
																				{
																						return;
																				}
																		}
																		tempDelayCount = delayCount;
																		Tm1652_Show_Close();
																		while(tempDelayCount--)				//方便退出
																		{
																				delay_ms(100);
																				if(state.nPageShow != PAGE_TIMING)
																				{
																						return;
																				}
																		}
																		tempDelayCount = delayCount;
																}
														}
												}
												else
												{
														//如果不暂停，则秒钟跳动
														if(timingHourCount>0 || timingMinuteCount>0 )
														{
																for(twinkleCount=0; twinkleCount<3; twinkleCount++)
																{
																		Tm1652_Time_Show_Second(timingSecondCount,1);
																		while(tempDelayCount--)				//方便退出
																		{
																				delay_ms(100);
																				if(state.nPageShow != PAGE_TIMING)
																				{
																						return;
																				}
																		}
																		tempDelayCount = delayCount;
																		Tm1652_Time_Show_Second(timingSecondCount,0);
																		while(tempDelayCount--)				//方便退出
																		{
																				delay_ms(100);
																				if(state.nPageShow != PAGE_TIMING)
																				{
																						return;
																				}
																		}
																		tempDelayCount = delayCount;
																		if(timingSecondCount+1 == 60)
																		{
																				timingSecondCount = 0;
																		}
																		else
																		{
																				timingSecondCount++;
																		}
																		if(state.nPageShow != PAGE_TIMING)
																		{
																				return;
																		}
																}
														}
												}
										}
										break;
								case HARD_KEY_DOWN:
										//下键按下则显示当前时间
										for(twinkleCount=0; twinkleCount<3; twinkleCount++)
										{
												I2C_DS3231_ReadTime();
												ds3231_hour = calendar.hour;					//读取小时数据
												ds3231_minute = calendar.minute;			//读取分钟数据
												if(g_tMainConfig.timeMode)
												{
														if(ds3231_hour > 12)
														{
																ds3231_hour -= 12;
														}
												}
												Tm1652_Time_Show(ds3231_hour,ds3231_minute,1);
												while(tempDelayCount--)				//方便退出
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_TIMING)
														{
																return;
														}
												}
												tempDelayCount = delayCount;
												if(state.nPageShow != PAGE_TIMING)
												{
														return;
												}
										}
										if(isShowClose)
										{
												//打开显示
												ENABLE_SHOW
												isShowClose = false;
										}
										else
										{
												//关闭显示
												Tm1652_Show_Close();
												delay_ms(10);

												DISABLE_SHOW
												isShowClose = true;
										}
										break;
								case HARD_KEY_CANCEL:
										//退出键按下则暂停计时，显示当前计时时长，再次按下退出
										if(isShowClose)
										{
												//打开显示
												ENABLE_SHOW
												isShowClose = false;
										}
										else
										{
												if(isPause)
												{
														//已经暂停计时则退出
														pageExit = true;
														//关闭计时
														isTiming = false;
												}
												else
												{
														//没有暂停则暂停计时
														I2C_DS3231_ReadTime();
														pauseDs3231Hour = calendar.hour;
														pauseDs3231Minute = calendar.minute;
														pauseDs3231Second = calendar.second;
														isPause = true;
												}
										}
										break;
								case HARD_KEY_SET:
										if(isShowClose)
										{
												//打开显示
												ENABLE_SHOW
												isShowClose = false;
										}
										else
										{
												if(isPause)
												{
														//已经暂停计时则恢复
														isPause = false;
														//恢复之前减掉暂停时间
														I2C_DS3231_ReadTime();

														//计算暂停时长
														i = pauseDs3231Second + pauseDs3231Minute*60 + pauseDs3231Hour*60*60;
														j = calendar.second + calendar.minute*60 + calendar.hour*60*60;
														if(j >= i)
														{
																pastTime = j - i;
														}
														else
														{
																pastTime = j + 86400 - i;
														}
														//暂停秒数
														i = pastTime%60;
														beginDs3231Second += i;
														//暂停分数
														i = pastTime/60%60;
														beginDs3231Minute += i;
														//暂停时数
														i = pastTime/3600;
														beginDs3231Hour += i;
														
														//修正秒数
														if(beginDs3231Second > 59)
														{
																beginDs3231Second -= 60;
																beginDs3231Minute += 1;
														}
														//修正分数
														if(beginDs3231Minute > 59)
														{
																beginDs3231Minute -= 60;
																beginDs3231Hour += 1;
														}
														//修正时数
														if(beginDs3231Hour > 23)
														{
																beginDs3231Second -= 24;
														}
												}
												else
												{
														//没有暂停则暂停计时
														I2C_DS3231_ReadTime();
														pauseDs3231Hour = calendar.hour;
														pauseDs3231Minute = calendar.minute;
														pauseDs3231Second = calendar.second;
														isPause = true;
												}
										}
										break;
								default:
										break;
						}
						state.nKeyPress = HARD_KEY_EMPTY;
				}
				//确认键按下则暂停计时，再次按下继续计时
		}
		//清空显示
		Tm1652_Show_Close();
		delay_ms(10);

		DISABLE_SHOW

		//页面状态切换
		state.nPageShow = PAGE_SLEEP;
}

//设置参数功能--二级菜单
bool Page_Set_Function_Lv2(uint8_t index)
{
		bool pageExit = false;
		bool returnUpper = false;
		uint8_t changeMode = 0;

		uint8_t tempSetYearL;
		uint8_t tempSetMonth;
		uint8_t tempSetDate;
		uint8_t tempSetHour;
		uint8_t tempSetMinute;
		
		float tempOffsetTemperature = 0;
		float tempOffsetHumidity = 0;
		float tempTemperature = temperature;
		float tempHumidity = humidity;


		bool tempDisplayMode;

		bool tempTimeMode;

		uint8_t broadcastKey = HARD_KEY_EMPTY;

		switch (index)
		{
				case 1:
					//闹钟1时间
					tempSetHour = g_tMainConfig.tCtrl.alarm_moment_1.moment_hour;
					tempSetMinute = g_tMainConfig.tCtrl.alarm_moment_1.moment_minute;
					break;
				case 2:
					//闹钟2时间
					tempSetHour = g_tMainConfig.tCtrl.alarm_moment_2.moment_hour;
					tempSetMinute = g_tMainConfig.tCtrl.alarm_moment_2.moment_minute;
					break;
				case 3:
					//闹钟3时间
					tempSetHour = g_tMainConfig.tCtrl.alarm_moment_3.moment_hour;
					tempSetMinute = g_tMainConfig.tCtrl.alarm_moment_3.moment_minute;
					break;
				case 4:
					//时间设定临时值
					//读取时间数据
					I2C_DS3231_ReadTime();
					tempSetHour = calendar.hour;
					tempSetMinute = calendar.minute;
					break;
				case 5:
					//日期设定临时值
					//读取时间数据
					I2C_DS3231_ReadTime();
					tempSetYearL = calendar.yearL;
					tempSetMonth = calendar.month;
					tempSetDate 	 = calendar.date;
					break;
				case 6:
					
					//读取温湿度数值
					if(SHT30_Read_Data(recv_dat_list) == Ok)
					{
							SHT30_Data_To_Float(recv_dat_list, &temperature, &humidity);
					}
					//温度再校正值
					tempOffsetTemperature = 0;
					break;
				case 7:
					//读取温湿度数值
					if(SHT30_Read_Data(recv_dat_list) == Ok)
					{
							SHT30_Data_To_Float(recv_dat_list, &temperature, &humidity);
					}
					//湿度再校正值
					tempOffsetHumidity = 0;
					break;
				case 8:
					//常亮模式
					tempDisplayMode = g_tMainConfig.dispalyMode;
					break;
				case 9:
					//恢复出厂参数
					//闪一下SET，如果按确认就恢复参数，如果按取消退出
					break;
				case 10:
					//12/24小时模式
					tempTimeMode = g_tMainConfig.timeMode;
					break;
				case 11:
					//按键播报
					//无需预处理
					break;
		}
		
		//定时总时长控制
		uint8_t delayCount = 4;
		uint8_t tempDelayCount = delayCount;
		
		//进入先清按键
		state.nKeyPress = HARD_KEY_EMPTY;
		
		while(!pageExit)
		{
				if(changeMode == 0)
				{
						switch (index)
						{
								case 1:
										//闹钟1
								case 2:
										//闹钟2
								case 3:
										//闹钟3
								case 4:
										//时间设定--时
										Tm1652_Time_Show_Minute(tempSetMinute,1);
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;

										Tm1652_Time_Show(tempSetHour,tempSetMinute,1);
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 5:
										//日期设定--年
										Tm1652_Time_Show_Year(tempSetYearL);
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;

										Tm1652_Show_Updata('2', '0', ' ', ' ', ' ');
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;

								case 6:
										//温度校准 输入当前温度
										//读取当前温度并显示
										tempTemperature = temperature + tempOffsetTemperature;
										Tm1652_Temperature_Show(tempTemperature);
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										//闪烁显示
										Tm1652_Show_Updata(' ', ' ', ' ', 'C', ' ');
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 7:
										//湿度校准
										//读取当前温度并显示
										tempHumidity = humidity + tempOffsetHumidity;
										Tm1652_Humidity_Show(tempHumidity);
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										//闪烁显示
										Tm1652_Show_Updata(' ', ' ', ' ', 'H', ' ');
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 8:
										//常亮模式
										//闪烁模式设置
										if(tempDisplayMode)
										{
												Tm1652_Show_Updata(' ', '-', 'C', 'L', ' ');
										}
										else
										{
												Tm1652_Show_Updata(' ', '-', 'P', 'S', ' ');
										}
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										//闪烁显示
										Tm1652_Show_Updata(' ', '-', ' ', ' ', ' ');
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 9:
										//恢复参数
										//闪烁SET
										Tm1652_Show_Updata(' ', 'S', 'E', 'T', ' ');
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										//闪烁显示
										Tm1652_Show_Updata(' ', ' ', ' ', ' ', ' ');
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 10:
										//时间模式
										//闪烁模式设置
										if(tempTimeMode)
										{
												Tm1652_Show_Updata(' ', '-', '1', '2', ' ');
										}
										else
										{
												Tm1652_Show_Updata(' ', '-', '2', '4', ' ');
										}
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										//闪烁显示
										Tm1652_Show_Updata(' ', '-', ' ', ' ', ' ');
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 11:
										//按键播报
										if(broadcastKey != HARD_KEY_EMPTY)
										{
												ENABLE_SHOW

												//不为空则发送对应按键信息，同时显示1秒按的按键
												switch(broadcastKey)
												{
														case HARD_KEY_UP:
																u8TxCnt = 1;
																u8TxData[2] = 0xFF;		//按键语音触发编号为255
																u8TxData[3]	= 11;			//上键
																u8TxData[4] = 0x55;
																u8TxData[5] = 0xAA;
																uart_Send_Length = 6;
																Uart_SendDataIt(M0P_UART1, u8TxData[0]); //启动UART1发送第一个字
																//显示按的按键
																Tm1652_Show_Updata('b', 'o', '-', '1', ' ');
																delay_ms(500);
																break;
														case HARD_KEY_DOWN:
																u8TxCnt = 1;
																u8TxData[2] = 0xFF;		//按键语音触发编号为255
																u8TxData[3]	= 22;			//下键
																u8TxData[4] = 0x55;
																u8TxData[5] = 0xAA;
																uart_Send_Length = 6;
																Uart_SendDataIt(M0P_UART1, u8TxData[0]); //启动UART1发送第一个字
																//显示按的按键
																Tm1652_Show_Updata('b', 'o', '-', '2', ' ');
																delay_ms(500);
																break;
														case HARD_KEY_CANCEL:
																u8TxCnt = 1;
																u8TxData[2] = 0xFF;		//按键语音触发编号为255
																u8TxData[3]	= 33;			//取消键
																u8TxData[4] = 0x55;
																u8TxData[5] = 0xAA;
																uart_Send_Length = 6;
																Uart_SendDataIt(M0P_UART1, u8TxData[0]); //启动UART1发送第一个字
																//显示按的按键
																Tm1652_Show_Updata('b', 'o', '-', '3', ' ');
																delay_ms(500);
																break;
														case HARD_KEY_SET:
																u8TxCnt = 1;
																u8TxData[2] = 0xFF;		//按键语音触发编号为255
																u8TxData[3]	= 44;			//确认键
																u8TxData[4] = 0x55;
																u8TxData[5] = 0xAA;
																uart_Send_Length = 6;
																Uart_SendDataIt(M0P_UART1, u8TxData[0]); //启动UART1发送第一个字
																//显示按的按键
																Tm1652_Show_Updata('b', 'o', '-', '4', ' ');
																delay_ms(500);
																break;
												}
												//处理完毕清空
												broadcastKey = HARD_KEY_EMPTY;
												
										}
										
										Tm1652_Show_Close();
										DISABLE_SHOW
										break;
								default:
										Tm1652_Show_Updata('E', 'R', 'R', ' ', ' ');
										break;
						}
				}
				else if(changeMode == 1)
				{
						switch (index)
						{
								case 1:
										//闹钟1
								case 2:
										//闹钟2
								case 3:
										//闹钟3
								case 4:
										//时间设定--时分
										Tm1652_Time_Show_Hour(tempSetHour,1);
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;

										Tm1652_Time_Show(tempSetHour,tempSetMinute,1);
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 5:
										//日期设定--年月日
										Tm1652_Time_Show_Month(tempSetMonth);
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;

										Tm1652_Show_Updata(' ', '-', ' ', ' ', ' ');
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 6:
										//温度校准
										//确定温度校准值，保存退出
										break;
								case 7:
										//湿度校准
										//确定湿度校准值，保存退出
										break;
								case 8:
										//常亮模式
										//确定显示模式，保存退出
										break;
								case 9:
										break;
								case 10:
										//时间模式
										//确定时间模式，保存退出
										break;
								default:
										Tm1652_Show_Updata('E', 'R', 'R', ' ', ' ');
										break;
						}
				}
				else if(changeMode == 2)
				{
						switch (index)
						{
								case 5:
										//日期设定--年月日
										Tm1652_Time_Show_Day(tempSetDate);
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;

										Tm1652_Show_Updata(' ', '-', ' ', ' ', ' ');
										while(tempDelayCount--)				//方便退出
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								default:
										Tm1652_Show_Updata('E', 'R', 'R', ' ', ' ');
										break;
						}
				}
				delay_ms(300);
				if(state.nKeyPress != HARD_KEY_EMPTY)
				{
						switch (state.nKeyPress)
						{
								case HARD_KEY_UP:
										switch(index)
										{
												case 1:
												case 2:
												case 3:
												case 4:
														if(changeMode == 0)
														{
																if(tempSetHour == 23)
																{
																		tempSetHour = 0;
																}
																else
																{
																		tempSetHour++;
																}
														}
														else if(changeMode == 1)
														{
																if(tempSetMinute == 59)
																{
																		tempSetMinute = 0;
																}
																else
																{
																		tempSetMinute++;
																}
														}
														break;
												case 5:
														if(changeMode == 0)
														{
																if(tempSetYearL == 99)
																{
																		tempSetYearL = 0;
																}
																else
																{
																		tempSetYearL++;
																}
														}
														else if(changeMode == 1)
														{
																if(tempSetMonth == 12)
																{
																		tempSetMonth = 1;
																}
																else
																{
																		tempSetMonth++;
																}
														}
														else if(changeMode == 2)
														{
																switch(tempSetMonth)
																{
																		case 1:
																		case 3:
																		case 5:
																		case 7:
																		case 8:
																		case 10:
																		case 12:
																				if(tempSetDate == 31)
																				{
																						tempSetDate = 1;
																				}
																				else
																				{
																						tempSetDate++;
																				}
																				break;
																		case 2:
																				if(tempSetYearL%4 == 0)
																				{
																						if(tempSetDate == 29)
																						{
																								tempSetDate = 1;
																						}
																						else
																						{
																								tempSetDate++;
																						}
																				}
																				else
																				{
																						if(tempSetDate == 28)
																						{
																								tempSetDate = 1;
																						}
																						else
																						{
																								tempSetDate++;
																						}
																				}
																				break;
																		case 4:
																		case 6:
																		case 9:
																		case 11:
																				if(tempSetDate == 30)
																				{
																						tempSetDate = 1;
																				}
																				else
																				{
																						tempSetDate++;
																				}
																				break;
																		
																}
														}
														break;
												case 6:
														if(tempTemperature < 124.5)
														{
																if(tempTemperature>-10 && tempTemperature<100)
																{
																		tempOffsetTemperature+=0.1;
																}
																else
																{
																		tempOffsetTemperature+=1;
																}
														}
														break;
												case 7:
														if(tempHumidity<=100)
														{
																tempOffsetHumidity+=0.5;
														}
														break;
												case 8:
														tempDisplayMode = !tempDisplayMode;
														break;
												case 9:
														break;
												case 10:
														tempTimeMode = !tempTimeMode;
														break;
												case 11:
														broadcastKey = HARD_KEY_UP;
														break;
										}
										break;
								case HARD_KEY_DOWN:
										switch(index)
										{
												case 1:
												case 2:
												case 3:
												case 4:
														if(changeMode == 0)
														{
																if(tempSetHour == 0)
																{
																		tempSetHour = 23;
																}
																else
																{
																		tempSetHour--;
																}
														}
														else if(changeMode == 1)
														{
																if(tempSetMinute == 0)
																{
																		tempSetMinute = 59;
																}
																else
																{
																		tempSetMinute--;
																}
														}
														break;
												case 5:
														if(changeMode == 0)
														{
																if(tempSetYearL == 0)
																{
																		tempSetYearL = 99;
																}
																else
																{
																		tempSetYearL--;
																}
														}
														else if(changeMode == 1)
														{
																if(tempSetMonth == 1)
																{
																		tempSetMonth = 12;
																}
																else
																{
																		tempSetMonth--;
																}
														}
														else if(changeMode == 2)
														{
																switch(tempSetMonth)
																{
																		case 1:
																		case 3:
																		case 5:
																		case 7:
																		case 8:
																		case 10:
																		case 12:
																				if(tempSetDate == 1)
																				{
																						tempSetDate = 31;
																				}
																				else
																				{
																						tempSetDate--;
																				}
																				break;
																		case 2:
																				if(tempSetYearL%4 == 0)
																				{
																						if(tempSetDate == 1)
																						{
																								tempSetDate = 29;
																						}
																						else
																						{
																								tempSetDate--;
																						}
																				}
																				else
																				{
																						if(tempSetDate == 1)
																						{
																								tempSetDate = 28;
																						}
																						else
																						{
																								tempSetDate--;
																						}
																				}
																				break;
																		case 4:
																		case 6:
																		case 9:
																		case 11:
																				if(tempSetDate == 1)
																				{
																						tempSetDate = 30;
																				}
																				else
																				{
																						tempSetDate--;
																				}
																				break;
																		
																}
														}
														break;
												case 6:
														if(tempTemperature>-39.5)
														{
																if(tempTemperature>-10 && tempTemperature<100)
																{
																		tempOffsetTemperature-=0.1;
																}
																else
																{
																		tempOffsetTemperature-=1;
																}
														}
														break;
												case 7:
														if(tempHumidity>0.5)
														{
																tempOffsetHumidity-=0.5;
														}
														break;
												case 8:
														tempDisplayMode = !tempDisplayMode;
														break;
												case 9:
														break;
												case 10:
														tempTimeMode = !tempTimeMode;
														break;
												case 11:
														broadcastKey = HARD_KEY_DOWN;
														break;
										}
										break;
								case HARD_KEY_CANCEL:
										if(index == 11)
										{
												broadcastKey = HARD_KEY_CANCEL;
										}
										else
										{
												if(changeMode > 0)
												{
														changeMode --;
												}
												else
												{
														returnUpper = true;
														pageExit = true;
												}
										}
										break;
								case HARD_KEY_SET:
										if(index == 11)
										{
												broadcastKey = HARD_KEY_SET;
										}
										else
										{
												if(changeMode == 0)
												{
														switch(index)
														{
																case 1:
																		//闹钟1
																		break;
																case 2:
																		//闹钟2
																		break;
																case 3:
																		//闹钟3
																		break;
																case 4:
																		//时间设定
																		break;
																case 5:
																		//日期设定
																		break;
																case 6:
																		//温度校准
																		//保存修正值并退出
																		g_tMainConfig.offsetTemperature += tempOffsetTemperature;
																		returnUpper = false;
																		pageExit = true;
																		break;
																case 7:
																		//湿度校准
																		//保存修正值并退出
																		g_tMainConfig.offsetHumidity += tempOffsetHumidity;
																		returnUpper = false;
																		pageExit = true;
																		break;
																case 8:
																		//常亮模式
																		g_tMainConfig.dispalyMode = tempDisplayMode;
																		returnUpper = false;
																		pageExit = true;
																		break;
																case 9:
																		//恢复参数
																		MainConfigReset();
																		returnUpper = false;
																		pageExit = true;
																		break;
																case 10:
																		//时间模式
																		g_tMainConfig.timeMode = tempTimeMode;
																		returnUpper = false;
																		pageExit = true;
																		break;
														}
												}
												if(changeMode == 1)
												{
														switch(index)
														{
																case 1:
																		//闹钟1
																		g_tMainConfig.tCtrl.alarm_moment_1.moment_hour = tempSetHour;
																		g_tMainConfig.tCtrl.alarm_moment_1.moment_minute = tempSetMinute;
																		break;
																case 2:
																		//闹钟2
																		g_tMainConfig.tCtrl.alarm_moment_2.moment_hour = tempSetHour;
																		g_tMainConfig.tCtrl.alarm_moment_2.moment_minute = tempSetMinute;
																		break;
																case 3:
																		//闹钟3
																		g_tMainConfig.tCtrl.alarm_moment_3.moment_hour = tempSetHour;
																		g_tMainConfig.tCtrl.alarm_moment_3.moment_minute = tempSetMinute;
																		break;
																case 4:
																		//时间设定
																		//DS3231写入时间数据
																		I2C_DS3231_SetTime(calendar.yearL,calendar.month,calendar.date,calendar.week,tempSetHour,tempSetMinute,0);
																		break;
																case 5:
																		//日期设定
																		break;
																case 6:
																		//温度校准
																		break;
																case 7:
																		//湿度校准
																		break;
																case 8:
																		//常亮模式
																		break;
																case 9:
																		//恢复参数
																		break;
																case 10:
																		//时间模式
																		break;
																default:
																		returnUpper = true;
																		pageExit = true;
																		break;
														}
														if(index <= 3)
														{
																//如果返回返回上层，则不结束循环，否则结束循环
																if(Page_Set_Function_Lv3(index))
																{
																		//需要结束循环
																		pageExit = true;
																		//清定时标志
																		I2C_DS3231_SetAlarm_2(false, 1, 1);
																		I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
																		//确定闹钟开启
																		switch(JudgeAlarmFirstArrive())
																		{
																				case 0:
																						//没有闹钟开启，这里关一下DS3231 Alarm2
																						I2C_DS3231_SetAlarm_2(false, 1, 1);
																						I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
																						break;
																				case 1:
																						//直接写入1的报警时刻（不需要判断闹钟，因为闹钟时间不变，和定时不一样）
																						I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_1.moment_hour, g_tMainConfig.tCtrl.alarm_moment_1.moment_minute);
																						break;
																				case 2:
																						//直接写入2的报警时刻（不需要判断闹钟，因为闹钟时间不变，和定时不一样）
																						I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_2.moment_hour, g_tMainConfig.tCtrl.alarm_moment_2.moment_minute);
																						break;
																				case 3:
																						//直接写入3的报警时刻（不需要判断闹钟，因为闹钟时间不变，和定时不一样）
																						I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_3.moment_hour, g_tMainConfig.tCtrl.alarm_moment_3.moment_minute);
																						break;
																		}
																}
																else
																{
																		changeMode--;
																}
														}
														else if(index != 5)
														{
																//非闹钟、非日期设定不需要处理，直接退出
																returnUpper = false;
																pageExit = true;
														}
												}
												if(changeMode == 2)
												{
														switch(index)
														{
																case 5:
																		//日期设定
																		I2C_DS3231_ReadTime();
																		I2C_DS3231_SetTime(tempSetYearL,tempSetMonth,tempSetDate,GregorianDay(tempSetYearL,tempSetMonth,tempSetDate),calendar.hour,calendar.minute,calendar.second);
																		break;
														}
														pageExit = true;
												}
												changeMode++;
										}
										break;
								default:
										returnUpper = true;
										pageExit = true;
										break;
						}
						state.nKeyPress = HARD_KEY_EMPTY;
				}
				if(state.nPageShow != PAGE_SET)
				{
						//返回上层为真，则上层不结束循环；如果为假，则流程往下走，结束循环
						if(returnUpper == true)
						{
								return false;
						}
						else
						{
								return true;
						}
				}
		}
		//返回上层为真，则上层不结束循环；如果为假，则流程往下走，结束循环
		if(returnUpper == true)
		{
				return false;
		}
		else
		{
				return true;
		}
}


//设置参数功能--三级菜单
bool Page_Set_Function_Lv3(uint8_t index)
{
		bool pageExit = false;
		bool returnUpper = false;
		bool tempAlarmMomentEnable;

		switch (index)
		{
				case 1:
					//闹钟1
					tempAlarmMomentEnable = g_tMainConfig.tCtrl.alarm_moment_enable_1;
					break;
				case 2:
					//闹钟2
					tempAlarmMomentEnable = g_tMainConfig.tCtrl.alarm_moment_enable_2;
					break;
				case 3:
					//闹钟3
					tempAlarmMomentEnable = g_tMainConfig.tCtrl.alarm_moment_enable_3;
					break;
				case 4:
					//时间设定
					break;
				case 5:
					//温度校正值
					break;
				case 6:
					//湿度校正值
					break;
		}
		
		//定时总时长控制
		uint8_t delayCount = 4;
		uint8_t tempDelayCount = delayCount;
		
		//进入先清按键
		state.nKeyPress = HARD_KEY_EMPTY;
		
		while(!pageExit)
		{
				switch (index)
				{
						case 1:
								//闹钟1
								if(!tempAlarmMomentEnable)
								{
										Tm1652_Show_Updata('A', '1', '-', 'O', ' ');
								}
								else
								{
										Tm1652_Show_Updata('A', '1', '-', 'C', ' ');
								}
								break;
						case 2:
								//闹钟2
								if(!tempAlarmMomentEnable)
								{
										Tm1652_Show_Updata('A', '2', '-', 'O', ' ');
								}
								else
								{
										Tm1652_Show_Updata('A', '2', '-', 'C', ' ');
								}
								break;
						case 3:
								//闹钟3
								if(!tempAlarmMomentEnable)
								{
										Tm1652_Show_Updata('A', '3', '-', 'O', ' ');
								}
								else
								{
										Tm1652_Show_Updata('A', '3', '-', 'C', ' ');
								}
								break;
						case 4:
								//时间设定
								break;
						case 5:
								//温度校准 输入当前温度
								break;
						case 6:
								//湿度校准
								break;
						default:
								Tm1652_Show_Updata('E', 'R', 'R', ' ', ' ');
								break;
				}
				while(tempDelayCount--)				//方便退出
				{
						delay_ms(100);
						if(state.nKeyPress != HARD_KEY_EMPTY)
								break;
				}
				tempDelayCount = delayCount;
				//闪烁
				Tm1652_Show_Updata(' ', ' ', ' ', ' ', ' ');
				while(tempDelayCount--)				//方便退出
				{
						delay_ms(100);
						if(state.nKeyPress != HARD_KEY_EMPTY)
								break;
				}
				tempDelayCount = delayCount;
				
				//按键处理
				if(state.nKeyPress != HARD_KEY_EMPTY)
				{
						switch (state.nKeyPress)
						{
								case HARD_KEY_UP:
										tempAlarmMomentEnable = !tempAlarmMomentEnable;
										break;
								case HARD_KEY_DOWN:
										tempAlarmMomentEnable = !tempAlarmMomentEnable;
										break;
								case HARD_KEY_CANCEL:
										//返回上一层
										returnUpper = true;
										pageExit = true;
										break;
								case HARD_KEY_SET:
										switch(index)
										{
												case 1:
														//闹钟1
														//如果没有该闹钟才启用，如果有，返回上层修改
														if(!tempAlarmMomentEnable)
														{
																//判断对应时刻是否有闹钟
																if(g_tMainConfig.tCtrl.alarm_moment_enable_2)
																{
																		if(g_tMainConfig.tCtrl.alarm_moment_1.moment_hour == g_tMainConfig.tCtrl.alarm_moment_2.moment_hour)
																		{
																				if(g_tMainConfig.tCtrl.alarm_moment_1.moment_minute == g_tMainConfig.tCtrl.alarm_moment_2.moment_minute)
																				{
																						//返回上层修改
																						returnUpper = true;
																				}
																		}
																}
																if(g_tMainConfig.tCtrl.alarm_moment_enable_3)
																{
																		if(g_tMainConfig.tCtrl.alarm_moment_1.moment_hour == g_tMainConfig.tCtrl.alarm_moment_3.moment_hour)
																		{
																				if(g_tMainConfig.tCtrl.alarm_moment_1.moment_minute == g_tMainConfig.tCtrl.alarm_moment_3.moment_minute)
																				{
																						//返回上层修改
																						returnUpper = true;
																				}
																		}
																}
														}
														if(!returnUpper)
														{
																//没有问题进行赋值
																g_tMainConfig.tCtrl.alarm_moment_enable_1 = !tempAlarmMomentEnable;
														}
														break;
												case 2:
														//闹钟2
														//如果没有该闹钟才启用，如果有，返回上层修改
														if(!tempAlarmMomentEnable)
														{
																//判断对应时刻是否有闹钟
																if(g_tMainConfig.tCtrl.alarm_moment_enable_1)
																{
																		if(g_tMainConfig.tCtrl.alarm_moment_1.moment_hour == g_tMainConfig.tCtrl.alarm_moment_2.moment_hour)
																		{
																				if(g_tMainConfig.tCtrl.alarm_moment_1.moment_minute == g_tMainConfig.tCtrl.alarm_moment_2.moment_minute)
																				{
																						//返回上层修改
																						returnUpper = true;
																				}
																		}
																}
																if(g_tMainConfig.tCtrl.alarm_moment_enable_3)
																{
																		if(g_tMainConfig.tCtrl.alarm_moment_2.moment_hour == g_tMainConfig.tCtrl.alarm_moment_3.moment_hour)
																		{
																				if(g_tMainConfig.tCtrl.alarm_moment_2.moment_minute == g_tMainConfig.tCtrl.alarm_moment_3.moment_minute)
																				{
																						//返回上层修改
																						returnUpper = true;
																				}
																		}
																}
														}
														if(!returnUpper)
														{
																//没有问题进行赋值
																g_tMainConfig.tCtrl.alarm_moment_enable_2 = !tempAlarmMomentEnable;
														}
														break;
												case 3:
														//闹钟3
														//如果没有该闹钟才启用，如果有，返回上层修改
														if(!tempAlarmMomentEnable)
														{
																//判断对应时刻是否有闹钟
																if(g_tMainConfig.tCtrl.alarm_moment_enable_1)
																{
																		if(g_tMainConfig.tCtrl.alarm_moment_1.moment_hour == g_tMainConfig.tCtrl.alarm_moment_3.moment_hour)
																		{
																				if(g_tMainConfig.tCtrl.alarm_moment_1.moment_minute == g_tMainConfig.tCtrl.alarm_moment_3.moment_minute)
																				{
																						//返回上层修改
																						returnUpper = true;
																				}
																		}
																}
																if(g_tMainConfig.tCtrl.alarm_moment_enable_2)
																{
																		if(g_tMainConfig.tCtrl.alarm_moment_2.moment_hour == g_tMainConfig.tCtrl.alarm_moment_3.moment_hour)
																		{
																				if(g_tMainConfig.tCtrl.alarm_moment_2.moment_minute == g_tMainConfig.tCtrl.alarm_moment_3.moment_minute)
																				{
																						//返回上层修改
																						returnUpper = true;
																				}
																		}
																}
														}
														if(!returnUpper)
														{
																//没有问题进行赋值
																g_tMainConfig.tCtrl.alarm_moment_enable_3 = !tempAlarmMomentEnable;
														}
														break;
												case 4:
														//时间设定
														break;
												case 5:
														//温度校准 输入当前温度
														
														break;
												case 6:
														//湿度校准

														break;
												default:
														break;
										}
										//检测到错误，返回上层
										if(returnUpper == true)
										{
												state.nKeyPress = HARD_KEY_EMPTY;
												return false;
										}
										//按确定键则不返回上层
										returnUpper = false;
										pageExit = true;
								default:
										break;
						}
						state.nKeyPress = HARD_KEY_EMPTY;
				}
				if(state.nPageShow != PAGE_SET)
				{
						//返回上层为真，则上层不结束循环；如果为假，则流程往下走，结束循环
						if(returnUpper == true)
						{
								return false;
						}
						else
						{
								return true;
						}
				}
		}
		//返回上层为真，则上层不结束循环；如果为假，则流程往下走，结束循环
		if(returnUpper == true)
		{
				return false;
		}
		else
		{
				return true;
		}
}

//控制模式功能
void Page_Ctrl_Function(void)
{
		bool pageExit = false;

		//定时总时长控制
		uint8_t delayCount = 5;
		uint8_t tempDelayCount = delayCount;

		ENABLE_SHOW
		
		state.nKeyPress = HARD_KEY_EMPTY;
		state.nFuncIndex = g_tMainConfig.ctrlModeSet;
		
		while(!pageExit)
		{
				switch (state.nFuncIndex)
				{
						case 0:
								Tm1652_Show_Updata('E', 'R', 'E', 'S', ' ');
								break;
						case 1:
								Tm1652_Show_Updata('E', 'R', 'd', 'S', ' ');
								break;
						case 2:
								Tm1652_Show_Updata('d', 'R', 'E', 'S', ' ');
								break;
						case 3:
								Tm1652_Show_Updata('d', 'R', 'd', 'S', ' ');
								break;
						default:
								Tm1652_Show_Updata('E', 'R', 'R', ' ', ' ');
								break;
				}
				while(tempDelayCount--)				//方便退出
				{
						delay_ms(100);
						if(state.nKeyPress != HARD_KEY_EMPTY)
								break;
				}
				tempDelayCount = delayCount;
				Tm1652_Show_Updata(' ', ' ', ' ', ' ', ' ');
				while(tempDelayCount--)				//方便退出
				{
						delay_ms(100);
						if(state.nKeyPress != HARD_KEY_EMPTY)
								break;
				}
				tempDelayCount = delayCount;
				
				if(state.nKeyPress != HARD_KEY_EMPTY)
				{
						switch (state.nKeyPress)
						{
								case HARD_KEY_UP:
										if(state.nFuncIndex == 3)
										{
												state.nFuncIndex = 0;
										}
										else
										{
												state.nFuncIndex++;
										}
										
										break;
								case HARD_KEY_DOWN:
										
										if(state.nFuncIndex == 0)
										{
												state.nFuncIndex = 3;
										}
										else
										{
												state.nFuncIndex--;
										}
										break;
								case HARD_KEY_CANCEL:
										//显示设置失败
										Tm1652_Show_Updata('F', 'A', 'L', 'S', ' ');
										delay_ms(2000);

										pageExit = 1;
										break;
								case HARD_KEY_SET:
										g_tMainConfig.ctrlModeSet = state.nFuncIndex;
										//显示设置成功
										Tm1652_Show_Updata('S', 'U', 'C', 'C', ' ');
										delay_ms(2000);

										pageExit = 1;
										//进行控制
										//红外感应使能判断
										if(config.pConfig->ctrlModeSet==0||config.pConfig->ctrlModeSet==1)
										{
												//高电平打开
												Gpio_SetIO(GpioPortA,GpioPin10);
										}
										else
										{
												Gpio_ClrIO(GpioPortA,GpioPin10);
										}
										//语音播报使能判断
										if(config.pConfig->ctrlModeSet==1||config.pConfig->ctrlModeSet==3)
										{
												//高电平关闭
												Gpio_SetIO(GpioPortB,GpioPin15);
										}
										else
										{
												Gpio_ClrIO(GpioPortB,GpioPin15);
										}
										break;
								default:
										break;
						}
						state.nKeyPress = HARD_KEY_EMPTY;
				}
				if(state.nPageShow != PAGE_CTRL)
				{
						return;
				}
		}
		//清空显示
		Tm1652_Show_Close();
		delay_ms(10);

		DISABLE_SHOW

		//保存设置
		MainConfigSave();

		//页面状态切换
		state.nPageShow = PAGE_SLEEP;
}

//定时1设置功能
void Page_Timer1_Function(void)
{
		//定时总时长控制
		uint8_t delayCount = 4;
		uint8_t tempDelayCount = delayCount;

		bool pageExit = false;
		uint8_t changeMode = 0;
		uint8_t tempDurationHour = g_tMainConfig.tCtrl.alarm_duration_1.duration_hour;
		uint8_t tempDurationMinute = g_tMainConfig.tCtrl.alarm_duration_1.duration_minute;

		state.nKeyPress = HARD_KEY_EMPTY;

		ENABLE_SHOW

		Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
		while(tempDelayCount--)				//方便退出
		{
				delay_ms(100);
		}
		tempDelayCount = delayCount;

		while(!pageExit)
		{
				if(g_tMainConfig.tCtrl.alarm_duration_enable_1)
				{
						//显示一下原定时时长
						Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
						delay_ms(800);
						//显示清屏
						Tm1652_Show_Updata(' ', ' ', ' ', ' ', ' ');
						delay_ms(10);
						
						//关闭定时1
						switch(JudgeDurationFirstArrive())
						{
								case 0:
										//没有定时开启
										//不会出现该情况
										break;
								case 1:
										//定时1正在计时
										//定时1关闭计时，查看定时2是否需要计时，需要计时则开启定时2计时
										I2C_DS3231_SetAlarm_1(false,1,1,1,1);
										I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零

										g_tMainConfig.tCtrl.alarm_duration_enable_1 = false;

										if(g_tMainConfig.tCtrl.alarm_duration_enable_2)
										{
												alarm_date = g_tMainConfig.tCtrl.alarm_duration_2.alarm_date;
												alarm_hour = g_tMainConfig.tCtrl.alarm_duration_2.alarm_hour;
												alarm_minute = g_tMainConfig.tCtrl.alarm_duration_2.alarm_minute;
												alarm_second = g_tMainConfig.tCtrl.alarm_duration_2.alarm_second;
												I2C_DS3231_SetAlarm_1(true,alarm_date,alarm_hour,alarm_minute,alarm_second);
										}
										break;
								case 2:
										//定时2正在计时
										//直接调整定时1开启标志位，无需更改实时时钟报警计时
										g_tMainConfig.tCtrl.alarm_duration_enable_1 = false;
										break;
						}

						//显示定时已关闭
						Tm1652_Show_Updata('F', '1', '-', 'C', ' ');
						delay_ms(1500);

						//显示倒计时关
						showDurationRemain_1 = false;

						pageExit = true;
				}
				else
				{
						//进入定时设定
						if(changeMode == 0)
						{
								Tm1652_Time_Show_LHour_Minute(tempDurationHour, tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;

								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
						}
						else if(changeMode == 1)
						{	
								Tm1652_Time_Show_HHour_Minute(tempDurationHour, tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
						}
						else if(changeMode == 2)
						{	
								Tm1652_Time_Show_Hour_LMinute(tempDurationHour, tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
						}
						else if(changeMode == 3)
						{	
								Tm1652_Time_Show_Hour_HMinute(tempDurationHour, tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
						}
				}
				if(state.nKeyPress != HARD_KEY_EMPTY)
				{
						if(changeMode < 3)
						{
								switch (state.nKeyPress)
								{
										case HARD_KEY_UP:
												if(changeMode == 0)
												{
														if(tempDurationHour/10 == 9)
														{
																tempDurationHour -= 90;
														}
														else
														{
																tempDurationHour += 10;
														}
												}
												if(changeMode == 1)
												{
														if(tempDurationHour%10 == 9)
														{
																tempDurationHour -= 9;
														}
														else
														{
																tempDurationHour += 1;
														}
												}
												if(changeMode == 2)
												{
														if(tempDurationMinute/10 == 5)
														{
																tempDurationMinute -= 50;
														}
														else
														{
																tempDurationMinute += 10;
														}
												}
												break;
										case HARD_KEY_DOWN:
												if(changeMode == 0)
												{
														if(tempDurationHour/10 == 0)
														{
																tempDurationHour += 90;
														}
														else
														{
																tempDurationHour -= 10;
														}
												}
												if(changeMode == 1)
												{
														if(tempDurationHour%10 == 0)
														{
																tempDurationHour += 9;
														}
														else
														{
																tempDurationHour -= 1;
														}
												}
												if(changeMode == 2)
												{
														if(tempDurationMinute/10 == 0)
														{
																tempDurationMinute += 50;
														}
														else
														{
																tempDurationMinute -= 10;
														}
												}
												break;
										case HARD_KEY_CANCEL:
												if(changeMode>0)
												{
														changeMode --;
												}
												else
												{
														pageExit = true;
												}
												break;
										case HARD_KEY_SET:
												changeMode ++;
												break;
										default:
												break;
								}
						}
						else if(changeMode == 3)
						{
								switch (state.nKeyPress)
								{
										case HARD_KEY_UP:
												if(tempDurationMinute%10 == 9)
												{
														tempDurationMinute -= 9;
												}
												else
												{
														tempDurationMinute += 1;
												}
												break;
										case HARD_KEY_DOWN:
												
												if(tempDurationMinute%10 == 0)
												{
														tempDurationMinute += 9;
												}
												else
												{
														tempDurationMinute -= 1;
												}
												break;
										case HARD_KEY_CANCEL:
												changeMode --;
												break;
										case HARD_KEY_SET:
												if(tempDurationHour==0 && tempDurationMinute==0)
												{
														Tm1652_Show_Updata(' ', 'E', 'R', 'R', ' ');
												}
												else
												{
														//保存定时1数据
														g_tMainConfig.tCtrl.alarm_duration_1.duration_hour = tempDurationHour;
														g_tMainConfig.tCtrl.alarm_duration_1.duration_minute = tempDurationMinute;
														g_tMainConfig.tCtrl.alarm_duration_enable_1 = true;
														I2C_DS3231_ReadTime();
														alarm_month = calendar.month;
														if(calendar.minute+tempDurationMinute>=60)
														{
																tempDurationHour+=1;
																alarm_minute = calendar.minute+tempDurationMinute-60;
														}
														else
														{
																alarm_minute = calendar.minute+tempDurationMinute;
														}
														if(calendar.hour+tempDurationHour>=24)
														{
																alarm_date = calendar.date + ((calendar.hour+tempDurationHour)/24);
																switch(calendar.month)
																{
																		case 1:
																		case 3:
																		case 5:
																		case 7:
																		case 8:
																		case 10:
																		case 12:
																				if(alarm_date>31)
																				{
																						alarm_date-=31;
																						alarm_month+=1;
																				}
																				break;
																		case 2:
																				if(calendar.yearL%4 == 0)
																				{
																						if(alarm_date>29)
																						{
																								alarm_date -= 29;
																								alarm_month += 1;
																						}
																				}
																				else
																				{
																						if(alarm_date>28)
																						{
																								alarm_date -= 28;
																								alarm_month += 1;
																						}
																				}
																				break;
																		case 4:
																		case 6:
																		case 9:
																		case 11:
																				if(alarm_date>30)
																				{
																						alarm_date -= 30;
																						alarm_month += 1;
																				}
																				break;

																}
																alarm_hour = (calendar.hour+tempDurationHour)%24;
														}
														else
														{
																alarm_date = calendar.date;
																alarm_hour = calendar.hour+tempDurationHour;
														}
														if(alarm_month>12)
														{
																alarm_month -= 12;
																alarm_yearL = calendar.yearL+1;
														}
														else
														{
																alarm_yearL = calendar.yearL;
														}
														alarm_second = calendar.second;
														//保存定时结束时刻
														g_tMainConfig.tCtrl.alarm_duration_1.alarm_yearL = alarm_yearL;
														g_tMainConfig.tCtrl.alarm_duration_1.alarm_month = alarm_month;
														g_tMainConfig.tCtrl.alarm_duration_1.alarm_date = alarm_date;
														g_tMainConfig.tCtrl.alarm_duration_1.alarm_hour = alarm_hour;
														g_tMainConfig.tCtrl.alarm_duration_1.alarm_minute = alarm_minute;
														g_tMainConfig.tCtrl.alarm_duration_1.alarm_second = alarm_second;
														//判断定时2是否在定时，然后进行比较确定哪一个先定时结束，先结束的写入实时时钟报警1
														switch(JudgeDurationFirstArrive())
														{
																case 0:
																		//没有定时开启
																		//不会出现该情况
																		break;
																case 1:
																		//定时1开始计时
																		I2C_DS3231_SetAlarm_1(false,1,1,1,1);
																		I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
																		I2C_DS3231_SetAlarm_1(true,alarm_date,alarm_hour,alarm_minute,alarm_second);
																		break;
																case 2:
																		//定时2正在计时
																		//无需操作等待定时2结束
																		break;
														}

														delay_ms(200);
														Tm1652_Show_Updata('F', '1', '-', 'O', ' ');
												}
												delay_ms(1500);
												pageExit = true;
												break;
										default:
												break;
								}
						}
						state.nKeyPress = HARD_KEY_EMPTY;
				}
				if(state.nPageShow != PAGE_TIMER_1)
				{
						return;
				}
		}
		//清空显示
		Tm1652_Show_Close();
		delay_ms(10);

		DISABLE_SHOW

		//保存设置
		MainConfigSave();

		//页面状态切换
		state.nPageShow = PAGE_SLEEP;
}

//定时2设置功能
void Page_Timer2_Function(void)
{
		//定时总时长控制
		uint8_t delayCount = 4;
		uint8_t tempDelayCount = delayCount;

		bool pageExit = false;
		uint8_t changeMode = 0;
		uint8_t tempDurationHour = g_tMainConfig.tCtrl.alarm_duration_2.duration_hour;
		uint8_t tempDurationMinute = g_tMainConfig.tCtrl.alarm_duration_2.duration_minute;

		state.nKeyPress = HARD_KEY_EMPTY;

		ENABLE_SHOW

		Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
		while(tempDelayCount--)				//方便退出
		{
				delay_ms(100);
		}
		tempDelayCount = delayCount;

		while(!pageExit)
		{
				if(g_tMainConfig.tCtrl.alarm_duration_enable_2)
				{
						//显示一下原定时时长
						Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
						delay_ms(800);
						//显示清屏
						Tm1652_Show_Updata(' ', ' ', ' ', ' ', ' ');
						delay_ms(10);
						
						//关闭定时2
						switch(JudgeDurationFirstArrive())
						{
								case 0:
										//没有定时开启
										//不会出现该情况
										break;
								case 1:
										//定时1正在计时
										//直接调整定时2开启标志位，无需更改实时时钟报警计时
										g_tMainConfig.tCtrl.alarm_duration_enable_2 = false;
										break;
								case 2:
										//定时2正在计时
										//定时2关闭计时，查看定时1是否需要计时，需要计时则开启定时1计时
										I2C_DS3231_SetAlarm_1(false,1,1,1,1);
										I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
										g_tMainConfig.tCtrl.alarm_duration_enable_2 = false;

										if(g_tMainConfig.tCtrl.alarm_duration_enable_1)
										{
												alarm_date = g_tMainConfig.tCtrl.alarm_duration_1.alarm_date;
												alarm_hour = g_tMainConfig.tCtrl.alarm_duration_1.alarm_hour;
												alarm_minute = g_tMainConfig.tCtrl.alarm_duration_1.alarm_minute;
												alarm_second = g_tMainConfig.tCtrl.alarm_duration_1.alarm_second;
												I2C_DS3231_SetAlarm_1(true,alarm_date,alarm_hour,alarm_minute,alarm_second);
										}
										break;
						}

						//显示定时已关闭
						Tm1652_Show_Updata('F', '2', '-', 'C', ' ');
						delay_ms(1500);
						
						//显示倒计时关
						showDurationRemain_2 = false;
					
						pageExit = true;
				}
				else
				{
						//进入定时设定
						if(changeMode == 0)
						{
								Tm1652_Time_Show_LHour_Minute(tempDurationHour, tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;

								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
						}
						else if(changeMode == 1)
						{	
								Tm1652_Time_Show_HHour_Minute(tempDurationHour, tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
						}
						else if(changeMode == 2)
						{	
								Tm1652_Time_Show_Hour_LMinute(tempDurationHour, tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
						}
						else if(changeMode == 3)
						{	
								Tm1652_Time_Show_Hour_HMinute(tempDurationHour, tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
						}
				}
				if(state.nKeyPress != HARD_KEY_EMPTY)
				{
						if(changeMode < 3)
						{
								switch (state.nKeyPress)
								{
										case HARD_KEY_UP:
												if(changeMode == 0)
												{
														if(tempDurationHour/10 == 9)
														{
																tempDurationHour -= 90;
														}
														else
														{
																tempDurationHour += 10;
														}
												}
												if(changeMode == 1)
												{
														if(tempDurationHour%10 == 9)
														{
																tempDurationHour -= 9;
														}
														else
														{
																tempDurationHour += 1;
														}
												}
												if(changeMode == 2)
												{
														if(tempDurationMinute/10 == 5)
														{
																tempDurationMinute -= 50;
														}
														else
														{
																tempDurationMinute += 10;
														}
												}
												break;
										case HARD_KEY_DOWN:
												if(changeMode == 0)
												{
														if(tempDurationHour/10 == 0)
														{
																tempDurationHour += 90;
														}
														else
														{
																tempDurationHour -= 10;
														}
												}
												if(changeMode == 1)
												{
														if(tempDurationHour%10 == 0)
														{
																tempDurationHour += 9;
														}
														else
														{
																tempDurationHour -= 1;
														}
												}
												if(changeMode == 2)
												{
														if(tempDurationMinute/10 == 0)
														{
																tempDurationMinute += 50;
														}
														else
														{
																tempDurationMinute -= 10;
														}
												}
												break;
										case HARD_KEY_CANCEL:
												if(changeMode>0)
												{
														changeMode --;
												}
												else
												{
														pageExit = true;
												}
												break;
										case HARD_KEY_SET:
												changeMode ++;
												break;
										default:
												break;
								}
						}
						else if(changeMode == 3)
						{
								switch (state.nKeyPress)
								{
										case HARD_KEY_UP:
												if(tempDurationMinute%10 == 9)
												{
														tempDurationMinute -= 9;
												}
												else
												{
														tempDurationMinute += 1;
												}
												break;
										case HARD_KEY_DOWN:
												
												if(tempDurationMinute%10 == 0)
												{
														tempDurationMinute += 9;
												}
												else
												{
														tempDurationMinute -= 1;
												}
												break;
										case HARD_KEY_CANCEL:
												changeMode --;
												break;
										case HARD_KEY_SET:
												if(tempDurationHour==0 && tempDurationMinute==0)
												{
														Tm1652_Show_Updata(' ', 'E', 'R', 'R', ' ');
												}
												else
												{
														//保存定时2数据
														g_tMainConfig.tCtrl.alarm_duration_2.duration_hour = tempDurationHour;
														g_tMainConfig.tCtrl.alarm_duration_2.duration_minute = tempDurationMinute;
														g_tMainConfig.tCtrl.alarm_duration_enable_2 = true;
														I2C_DS3231_ReadTime();
														alarm_month = calendar.month;
														if(calendar.minute+tempDurationMinute>=60)
														{
																tempDurationHour+=1;
																alarm_minute = calendar.minute+tempDurationMinute-60;
														}
														else
														{
																alarm_minute = calendar.minute+tempDurationMinute;
														}
														if(calendar.hour+tempDurationHour>=24)
														{
																alarm_date = calendar.date + ((calendar.hour+tempDurationHour)/24);
																switch(calendar.month)
																{
																		case 1:
																		case 3:
																		case 5:
																		case 7:
																		case 8:
																		case 10:
																		case 12:
																				if(alarm_date>31)
																				{
																						alarm_date-=31;
																						alarm_month+=1;
																				}
																				break;
																		case 2:
																				if(calendar.yearL%4 == 0)
																				{
																						if(alarm_date>29)
																						{
																								alarm_date -= 29;
																								alarm_month += 1;
																						}
																				}
																				else
																				{
																						if(alarm_date>28)
																						{
																								alarm_date -= 28;
																								alarm_month += 1;
																						}
																				}
																				break;
																		case 4:
																		case 6:
																		case 9:
																		case 11:
																				if(alarm_date>30)
																				{
																						alarm_date -= 30;
																						alarm_month += 1;
																				}
																				break;

																}
																alarm_hour = (calendar.hour+tempDurationHour)%24;
														}
														else
														{
																alarm_date = calendar.date;
																alarm_hour = calendar.hour+tempDurationHour;
														}
														if(alarm_month>12)
														{
																alarm_month -= 12;
																alarm_yearL = calendar.yearL+1;
														}
														else
														{
																alarm_yearL = calendar.yearL;
														}
														alarm_second = calendar.second;
														//保存定时结束时刻
														g_tMainConfig.tCtrl.alarm_duration_2.alarm_yearL = alarm_yearL;
														g_tMainConfig.tCtrl.alarm_duration_2.alarm_month = alarm_month;
														g_tMainConfig.tCtrl.alarm_duration_2.alarm_date = alarm_date;
														g_tMainConfig.tCtrl.alarm_duration_2.alarm_hour = alarm_hour;
														g_tMainConfig.tCtrl.alarm_duration_2.alarm_minute = alarm_minute;
														g_tMainConfig.tCtrl.alarm_duration_2.alarm_second = alarm_second;
														//判断定时2是否在定时，然后进行比较确定哪一个先定时结束，先结束的写入实时时钟报警1
														switch(JudgeDurationFirstArrive())
														{
																case 0:
																		//没有定时开启
																		//不会出现该情况
																		break;
																case 1:
																		//定时1正在计时
																		//无需操作等待定时1结束
																		break;
																case 2:
																		//定时2开始计时
																		I2C_DS3231_SetAlarm_1(false,1,1,1,1);
																		I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
																		I2C_DS3231_SetAlarm_1(true,alarm_date,alarm_hour,alarm_minute,alarm_second);
																		break;
														}

														delay_ms(200);
														Tm1652_Show_Updata('F', '2', '-', 'O', ' ');
												}
												delay_ms(1500);
												pageExit = true;
												break;
										default:
												break;
								}
						}
						state.nKeyPress = HARD_KEY_EMPTY;
				}
				if(state.nPageShow != PAGE_TIMER_2)
				{
						return;
				}
		}
		//清空显示
		Tm1652_Show_Close();
		delay_ms(10);

		DISABLE_SHOW

		//保存设置
		MainConfigSave();

		//页面状态切换
		state.nPageShow = PAGE_SLEEP;
}

//报警功能
void Page_Alarm_Function(void)
{
		//定时总时长控制
		uint8_t delayCount = 10;
		uint8_t tempDelayCount = delayCount;

		uint8_t alarmType = 0;

		uint8_t readData = 0x04;


		//清设定数据
		durationSetStep = 0;
		u8SetDurationData[1] = 0;
		u8SetDurationData[3] = 0;
		alarmSetStep = 0;
		u8SetAlarmData[1] = 0;
		u8SetAlarmData[3] = 0;


		// DS3231忙则等待
		while((readData&0x04) > 0)
		{
			I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_STATUS,&readData,1);
		}
		if(readData & 0x01)
		{
				alarmType = JudgeDurationFirstArrive();
		}
		else if(readData & 0x02)
		{
				I2C_DS3231_ReadTime();
				if(calendar.hour == g_tMainConfig.tCtrl.alarm_moment_1.moment_hour && calendar.minute == g_tMainConfig.tCtrl.alarm_moment_1.moment_minute)
				{
						alarmType = 3;
				}
				else if(calendar.hour == g_tMainConfig.tCtrl.alarm_moment_2.moment_hour && calendar.minute == g_tMainConfig.tCtrl.alarm_moment_2.moment_minute)
				{
						alarmType = 4;
				}
				else if(calendar.hour == g_tMainConfig.tCtrl.alarm_moment_3.moment_hour && calendar.minute == g_tMainConfig.tCtrl.alarm_moment_3.moment_minute)
				{
						alarmType = 5;
				}
		}

		ENABLE_SHOW
			
		for(int i=0; i < 90; i++)
		{
				//通知定时1时间到
				
				if(i%2 == 0)
				{
						switch(alarmType)
						{
								case 0:
										Tm1652_Show_Updata(' ', 'E', 'R', 'R', ' ');
										break;
								case 1:
										Tm1652_Show_Updata('F', '1', '-', 'A', ' ');
										break;
								case 2:
										Tm1652_Show_Updata('F', '2', '-', 'A', ' ');
										break;

								case 3:
										Tm1652_Show_Updata('A', '1', '-', 'A', ' ');
										break;
								case 4:
										Tm1652_Show_Updata('A', '2', '-', 'A', ' ');
										break;
								case 5:
										Tm1652_Show_Updata('A', '3', '-', 'A', ' ');
										break;
						}
				}
				else
				{
						
						switch(alarmType)
						{
								case 0:
										Tm1652_Show_Updata(' ', 'E', 'R', 'R', ' ');
										break;
								case 1:
										Tm1652_Time_Show(g_tMainConfig.tCtrl.alarm_duration_1.duration_hour,g_tMainConfig.tCtrl.alarm_duration_1.duration_minute,1);
										break;
								case 2:
										Tm1652_Time_Show(g_tMainConfig.tCtrl.alarm_duration_2.duration_hour,g_tMainConfig.tCtrl.alarm_duration_2.duration_minute,1);
										break;

								case 3:
										
								case 4:
										
								case 5:
										I2C_DS3231_ReadTime();
										Tm1652_Time_Show(calendar.hour, calendar.minute, 1);
										break;
						}
				}
				for(int j=0; j < 4; j++)
				{
						bsp_adt_pwm_ctrl_output(1);
						delay_ms(60);
						bsp_adt_pwm_ctrl_output(0);
						delay_ms(60);
				}
				delay_ms(20);
				Tm1652_Show_Updata(' ', ' ', ' ', ' ', ' ');
				tempDelayCount = delayCount;
				while(tempDelayCount--)				//方便退出
				{
						delay_ms(100);
						if(state.nPageShow != PAGE_ALARM)
						{
								break;
						}
				}
				
				if(state.nPageShow != PAGE_ALARM)
				{
						break;
				}
		}

		//如果有其他定时，则关闭报警定时写入新定时
		switch(alarmType)
		{
				case 1:
						//清理报警位
						I2C_DS3231_SetAlarm_1(false,1,1,1,1);
						I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
						g_tMainConfig.tCtrl.alarm_duration_enable_1 = false;
						showDurationRemain_1 = false;
						
						if(JudgeDurationFirstArrive() == 2)
						{
								alarm_date = g_tMainConfig.tCtrl.alarm_duration_2.alarm_date;
								alarm_hour = g_tMainConfig.tCtrl.alarm_duration_2.alarm_hour;
								alarm_minute = g_tMainConfig.tCtrl.alarm_duration_2.alarm_minute;
								alarm_second = g_tMainConfig.tCtrl.alarm_duration_2.alarm_second;
								//读取确定定时2到达时刻有没有过期，如果过期则说明同一时刻到达
								if(JudgeAlarmOutline(2))
								{
										//已经过期，则关闭定时开启标志
										g_tMainConfig.tCtrl.alarm_duration_enable_2 = false;
								}
								else
								{
										//没有过期则写入结束时间
										I2C_DS3231_SetAlarm_1(true,alarm_date,alarm_hour,alarm_minute,alarm_second);
								}
						}
						break;
				case 2:
						//清理报警位
						I2C_DS3231_SetAlarm_1(false,1,1,1,1);
						I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
						g_tMainConfig.tCtrl.alarm_duration_enable_2 = false;
						showDurationRemain_2 = false;

						if(JudgeDurationFirstArrive() == 1)
						{
								alarm_date = g_tMainConfig.tCtrl.alarm_duration_1.alarm_date;
								alarm_hour = g_tMainConfig.tCtrl.alarm_duration_1.alarm_hour;
								alarm_minute = g_tMainConfig.tCtrl.alarm_duration_1.alarm_minute;
								alarm_second = g_tMainConfig.tCtrl.alarm_duration_1.alarm_second;
								//读取确定定时1到达时刻有没有过期，如果过期则说明同一时刻到达
								if(JudgeAlarmOutline(1))
								{
										//已经过期，则关闭定时开启标志
										g_tMainConfig.tCtrl.alarm_duration_enable_1 = false;
								}
								else
								{
										//没有过期则写入结束时间
										I2C_DS3231_SetAlarm_1(true,alarm_date,alarm_hour,alarm_minute,alarm_second);
								}
						}	
						break;
				case 3:
						//清理报警位
						I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
						OpenMomentNextArrive(1);
						break;
				case 4:
						//清理报警位
						I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
						OpenMomentNextArrive(2);
						break;
				case 5:
						//清理报警位
						I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
						OpenMomentNextArrive(3);
						break;
		}

		DISABLE_SHOW

		//页面状态切换
		if(isTiming)
		{
				//还在计时则返回
				state.nPageShow = PAGE_TIMING;
		}
		else
		{
				state.nPageShow = PAGE_SLEEP;
		}
}

//语音定时功能
void Page_Duration_Set_Function(void)
{
		bool isSetDuration = true;
		//定时总时长控制
		uint8_t delayCount = 3;
		uint8_t tempDelayCount = delayCount;

		ENABLE_SHOW

		while(1)
		{
				switch(durationSetStep)
				{
						case 0:
								//设置定时异常，退出
								state.nPageShow = PAGE_SLEEP;
								break;
						case 1:
								//进入定时设定
								//时分同时闪烁
								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(durationSetStep != 1)
										{
												break;
										}
								}
								
								Tm1652_Time_Show_Colon(1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(durationSetStep != 1)
										{
												break;
										}
								}
								break;
						case 2:
								//输入第一个数  时分同时闪烁
								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(durationSetStep != 2)
										{
												break;
										}
								}

								Tm1652_Time_Show_Colon(1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(durationSetStep != 2)
										{
												break;
										}
								}
								break;
						case 3:
								//输入第二个数  时分同时闪烁
								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(durationSetStep != 3)
										{
												break;
										}
								}

								Tm1652_Time_Show_Colon(1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(durationSetStep != 3)
										{
												break;
										}
								}
								break;
						case 4:
								//确定小时   时停止闪烁 分闪烁
								Tm1652_Time_Show_Hour(u8SetDurationData[1], 1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(durationSetStep != 4)
										{
												break;
										}
								}

								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(durationSetStep != 4)
										{
												break;
										}
								}
								break;
						case 5:
								//设定分钟 	时停止闪烁 分闪烁
								Tm1652_Time_Show_Hour(u8SetDurationData[1], 1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(durationSetStep != 5)
										{
												break;
										}
								}

								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(durationSetStep != 5)
										{
												break;
										}
								}
								break;
						case 6:
								//收到分钟提示
								//显示定时时间
								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],0);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(durationSetStep != 6)
										{
												break;
										}
								}

								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(durationSetStep != 6)
										{
												break;
										}
								}
								break;
						case 7:
								//收到定时确认
								//检查定时数值是否正确
								//定时数值正确设置定时1	
								//显示定时时间
								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],1);
								delay_ms(1000);
								if(u8SetDurationData[1]!=0)
								{
										if(u8SetDurationData[3]>=60)
										{
												//参数错误，取消定时
												isSetDuration = false;
										}
								}
								else if(u8SetDurationData[1]==0)
								{
										//小时等于零没有小于60分钟限制
										if( u8SetDurationData[3]==0)
										{
												//参数错误，取消定时
												isSetDuration = false;
										}
										else if(u8SetDurationData[3]>=60)
										{
												u8SetDurationData[1]=u8SetDurationData[3]/60;
												u8SetDurationData[3]=u8SetDurationData[3]-60; 
										}
				
								}
								if(isSetDuration)
								{
										//设置定时
										//保存定时1数据
										g_tMainConfig.tCtrl.alarm_duration_1.duration_hour = u8SetDurationData[1];
										g_tMainConfig.tCtrl.alarm_duration_1.duration_minute = u8SetDurationData[3];
										g_tMainConfig.tCtrl.alarm_duration_enable_1 = true;
										I2C_DS3231_ReadTime();
										alarm_month = calendar.month;
										if(calendar.minute+u8SetDurationData[3]>=60)
										{
												u8SetDurationData[1]+=1;
												alarm_minute = calendar.minute+u8SetDurationData[3]-60;
										}
										else
										{
												alarm_minute = calendar.minute+u8SetDurationData[3];
										}
										if(calendar.hour+u8SetDurationData[1]>=24)
										{
												alarm_date = calendar.date + ((calendar.hour+u8SetDurationData[1])/24);
												switch(calendar.month)
												{
														case 1:
														case 3:
														case 5:
														case 7:
														case 8:
														case 10:
														case 12:
																if(alarm_date>31)
																{
																		alarm_date-=31;
																		alarm_month+=1;
																}
																break;
														case 2:
																if(calendar.yearL%4 == 0)
																{
																		if(alarm_date>29)
																		{
																				alarm_date -= 29;
																				alarm_month += 1;
																		}
																}
																else
																{
																		if(alarm_date>28)
																		{
																				alarm_date -= 28;
																				alarm_month += 1;
																		}
																}
																break;
														case 4:
														case 6:
														case 9:
														case 11:
																if(alarm_date>30)
																{
																		alarm_date -= 30;
																		alarm_month += 1;
																}
																break;

												}
												alarm_hour = (calendar.hour+u8SetDurationData[1])%24;
										}
										else
										{
												alarm_date = calendar.date;
												alarm_hour = calendar.hour+u8SetDurationData[1];
										}
										if(alarm_month>12)
										{
												alarm_month -= 12;
												alarm_yearL = calendar.yearL+1;
										}
										else
										{
												alarm_yearL = calendar.yearL;
										}
										alarm_second = calendar.second;
										//保存定时结束时刻
										g_tMainConfig.tCtrl.alarm_duration_1.alarm_yearL = alarm_yearL;
										g_tMainConfig.tCtrl.alarm_duration_1.alarm_month = alarm_month;
										g_tMainConfig.tCtrl.alarm_duration_1.alarm_date = alarm_date;
										g_tMainConfig.tCtrl.alarm_duration_1.alarm_hour = alarm_hour;
										g_tMainConfig.tCtrl.alarm_duration_1.alarm_minute = alarm_minute;
										g_tMainConfig.tCtrl.alarm_duration_1.alarm_second = alarm_second-1;
										//判断定时2是否在定时，然后进行比较确定哪一个先定时结束，先结束的写入实时时钟报警1
										switch(JudgeDurationFirstArrive())
										{
												case 0:
														//没有定时开启
														//不会出现该情况
														break;
												case 1:
														//定时1开始计时
														I2C_DS3231_SetAlarm_1(false,1,1,1,1);
														I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
														I2C_DS3231_SetAlarm_1(true,alarm_date,alarm_hour,alarm_minute,alarm_second-1);
														break;
												case 2:
														//定时2正在计时
														//无需操作等待定时2结束
														break;
										}
										//显示成功
										Tm1652_Time_Show_Colon(0);
										delay_ms(500);

										Tm1652_Show_Updata('S', 'U', 'C', 'C', ' ');
										delay_ms(2000);

								}
								else
								{

										Tm1652_Time_Show_Colon(0);
										delay_ms(500);

										Tm1652_Show_Updata('F', 'A', 'L', 'S', ' ');
										delay_ms(2000);

								}
								//流程完成，清数据
								durationSetStep = 0;
								u8SetDurationData[1] = 0;
								u8SetDurationData[3] = 0;
								state.nPageShow = PAGE_SLEEP;
								break;
				}
				if(state.nPageShow != PAGE_DURATION_SET)
				{
						break;
				}
		}

		Tm1652_Show_Close();
		DISABLE_SHOW
}

//语音闹钟功能
void Page_Alarm_Set_Function(void)
{
		bool isSetAlarm = true;
		//定时总时长控制
		uint8_t delayCount = 3;
		uint8_t tempDelayCount = delayCount;

		ENABLE_SHOW

		while(1)
		{
				switch(alarmSetStep)
				{
						case 0:
								//设置定时异常，退出
								state.nPageShow = PAGE_SLEEP;
								break;
						case 1:
								//进入定时设定
								//时分同时闪烁
								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(alarmSetStep != 1)
										{
												break;
										}
								}
								
								Tm1652_Time_Show_Colon(1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(alarmSetStep != 1)
										{
												break;
										}
								}
								break;
						case 2:
								//输入第一个数  时分同时闪烁
								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(alarmSetStep != 2)
										{
												break;
										}
								}

								Tm1652_Time_Show_Colon(1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(alarmSetStep != 2)
										{
												break;
										}
								}
								break;
						case 3:
								//输入第二个数  时分同时闪烁
								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(alarmSetStep != 3)
										{
												break;
										}
								}

								Tm1652_Time_Show_Colon(1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(alarmSetStep != 3)
										{
												break;
										}
								}
								break;
						case 4:
								//确定小时   时停止闪烁 分闪烁
								Tm1652_Time_Show_Hour(u8SetAlarmData[1], 1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(alarmSetStep != 4)
										{
												break;
										}
								}

								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(alarmSetStep != 4)
										{
												break;
										}
								}
								break;
						case 5:
								//设定分钟 	时停止闪烁 分闪烁
								Tm1652_Time_Show_Hour(u8SetAlarmData[1], 1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(alarmSetStep != 5)
										{
												break;
										}
								}

								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(alarmSetStep != 5)
										{
												break;
										}
								}
								break;
						case 6:
								//收到分钟提示
								//显示定时时间
								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],0);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(alarmSetStep != 6)
										{
												break;
										}
								}

								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//方便退出
								{
										delay_ms(100);
										if(alarmSetStep != 6)
										{
												break;
										}
								}
								break;
						case 7:
								//收到定时确认
								//检查定时数值是否正确
								//定时数值正确设置定时1	
								//显示定时时间
								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],1);
								delay_ms(1000);
								if(u8SetAlarmData[1]>=24)
								{
										//参数错误，取消
										isSetAlarm = false;
								}
								if(u8SetAlarmData[3]>=60)
								{
										//参数错误，取消定时
										isSetAlarm = false;
								}

								if(isSetAlarm)
								{
										//设置定时
										g_tMainConfig.tCtrl.alarm_moment_1.moment_hour = u8SetAlarmData[1];
										g_tMainConfig.tCtrl.alarm_moment_1.moment_minute = u8SetAlarmData[3];
										g_tMainConfig.tCtrl.alarm_moment_enable_1 = TRUE;
										//清定时标志
										I2C_DS3231_SetAlarm_2(false, 1, 1);
										I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
										//确定闹钟开启
										switch(JudgeAlarmFirstArrive())
										{
												case 0:
														//没有闹钟开启，这里关一下DS3231 Alarm2
														I2C_DS3231_SetAlarm_2(false, 1, 1);
														I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
														break;
												case 1:
														//直接写入1的报警时刻（不需要判断闹钟，因为闹钟时间不变，和定时不一样）
														I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_1.moment_hour, g_tMainConfig.tCtrl.alarm_moment_1.moment_minute);
														break;
												case 2:
														//直接写入2的报警时刻（不需要判断闹钟，因为闹钟时间不变，和定时不一样）
														I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_2.moment_hour, g_tMainConfig.tCtrl.alarm_moment_2.moment_minute);
														break;
												case 3:
														//直接写入3的报警时刻（不需要判断闹钟，因为闹钟时间不变，和定时不一样）
														I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_3.moment_hour, g_tMainConfig.tCtrl.alarm_moment_3.moment_minute);
														break;
										}
										//显示成功
										Tm1652_Time_Show_Colon(0);
										delay_ms(500);

										Tm1652_Show_Updata('S', 'U', 'C', 'C', ' ');
										delay_ms(2000);
								}
								else
								{

										Tm1652_Time_Show_Colon(0);
										delay_ms(500);

										Tm1652_Show_Updata('F', 'A', 'L', 'S', ' ');
										delay_ms(2000);
								}
								//流程完成，清数据
								alarmSetStep = 0;
								u8SetAlarmData[1] = 0;
								u8SetAlarmData[3] = 0;
								state.nPageShow = PAGE_SLEEP;
								break;
				}
				if(state.nPageShow != PAGE_ALARM_SET)
				{
						break;
				}
		}

		Tm1652_Show_Close();
		DISABLE_SHOW
}
