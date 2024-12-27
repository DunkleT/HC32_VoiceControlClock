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

//��дʱ���б���
uint8_t time_buf_list[8]; //����ʱ��
uint8_t read_time_list[14];//��ǰʱ��

//��ȡʱ��
uint8_t ds3231_hour = 0;
uint8_t ds3231_minute = 0;
uint8_t ds3231_second = 0;

//��ȡ��ʪ��
uint8_t recv_dat_list[6] = {0};

//��ȡ����ʪ��ֵ
float temperature = 0.0;
float humidity = 0.0;

extern DevicePama 		config;				  // �豸����
extern DeviceState		state;					// �豸״̬
extern MainConfig		g_tMainConfig;		// ϵͳ����

//����ʱ��
uint8_t alarm_yearL = 0;			//�������
uint8_t alarm_month = 0;			//�����·�
uint8_t alarm_date = 0;				//��������
uint8_t alarm_hour = 0;				//����Сʱ
uint8_t alarm_minute = 0;			//��������
uint8_t alarm_second = 0;			//��������

_calendar_obj calendar;	//�����ṹ��

uint8_t i_index = 1;

//�Ƿ���ʾ����ʱ
bool	showDurationRemain_1 = false;
bool	showDurationRemain_2 = false;

//������ʱ���ݴ洢
extern uint8_t u8SetDurationData[5];
extern uint8_t durationSetStep;
//�����������ݴ洢
extern uint8_t u8SetAlarmData[5];
extern uint8_t alarmSetStep;

//�Ƿ�ʱ
extern bool is_out_time;

//����������������
extern uint8_t u8TxData[10];
extern uint8_t u8TxCnt;
extern uint8_t uart_Send_Length;

//��ȡ����ʱ��
uint8_t beginDs3231Hour = 0;
uint8_t beginDs3231Minute = 0;
uint8_t beginDs3231Second = 0;

//��ʾ��ʱʱ��
uint8_t timingHourCount = 0;
uint8_t timingMinuteCount = 0;
uint8_t timingSecondCount = 0;

//�Ƿ��ڼ�ʱ
bool isTiming = false;

//�͵�������
bool low_power_remind_enable = true;
//�͵�����ʾ��������Ϊÿ������ʾ��̫Ƶ������˼�����ÿ10������ʾһ��
uint8_t low_power_show_count = 0;

//��ʾҳ��
void Main_Page_Show(void)
{
		switch(state.nPageShow)
		{
				case PAGE_MAIN:
						//������
						Page_Main_Function();
						break;
				case PAGE_SLEEP:
						//���߽���
						Page_Sleep_Function();
						break;
				case PAGE_CTRL:
						//���ƽ���
						Page_Ctrl_Function();
						break;
				case PAGE_SET:
						//���ý���
						Page_Set_Function();
						break;
				case PAGE_TIMER_1:
						//��ʱ1����
						Page_Timer1_Function();
						break;
				case PAGE_TIMER_2:
						//��ʱ2����
						Page_Timer2_Function();
						break;
				case PAGE_ALARM:
						//��������
						Page_Alarm_Function();
						break;
				case PAGE_DURATION_SET:
						//������ʱ�趨����
						Page_Duration_Set_Function();
						break;
				case PAGE_ALARM_SET:
						//���������趨����
						Page_Alarm_Set_Function();
						break;
				case PAGE_TIMING:
						//��ʱ����
						Page_Timing_Function();
		}
}

//��ѭ������
void Page_Main_Function(void)
{
		//��ʱ��ʱ������
		uint8_t delayCount = 30;
		uint8_t tempDelayCount = delayCount;

		ENABLE_SHOW

		//����ģʽ������ʾ��ǰʱ��
		if(!g_tMainConfig.dispalyMode)
		{
				//��ʾ��ǰʱ��
				I2C_DS3231_ReadTime();
				ds3231_hour = calendar.hour;					//��ȡСʱ����
				ds3231_minute = calendar.minute;			//��ȡ��������
				if(g_tMainConfig.timeMode)
				{
						if(ds3231_hour > 12)
						{
								ds3231_hour -= 12;
						}
				}
				Tm1652_Time_Show(ds3231_hour,ds3231_minute,1);
				while(tempDelayCount--)				//�����˳�
				{
						delay_ms(100);
						if(state.nPageShow != PAGE_MAIN)
						{
								return;
						}
				}
				tempDelayCount = delayCount;
		}

		//��ȡ��ʪ����ֵ
		if(SHT30_Read_Data(recv_dat_list) == Ok)
		{
				SHT30_Data_To_Float(recv_dat_list, &temperature, &humidity);
		}

		//��ʾ�¶�
		Tm1652_Temperature_Show(temperature);
		while(tempDelayCount--)				//�����˳�
		{
				delay_ms(100);
				if(state.nPageShow != PAGE_MAIN)
				{
						return;
				}
		}
		tempDelayCount = delayCount;

		//��ʾʪ��
		Tm1652_Humidity_Show(humidity);
		while(tempDelayCount--)				//�����˳�
		{
				delay_ms(100);
				if(state.nPageShow != PAGE_MAIN)
				{
						return;
				}
		}
		tempDelayCount = delayCount;
		
		//�����ʾ
		Tm1652_Show_Close();
		delay_ms(10);

		DISABLE_SHOW

		//ҳ��״̬�л�
		if(state.nPageShow == PAGE_MAIN)
		{
				state.nPageShow = PAGE_SLEEP;
		}
}

//���߻��ѹ���
void Page_Sleep_Function(void)
{
		//ʣ�ඨʱʱ��
		uint8_t remainDurationHour = 0;
		uint8_t remainDurationMinute = 0;
		uint8_t remainDurationSecond = 0;
		//��ʱ��ʱ������
		uint8_t tempDelayCount = 0;

		//���������˳�
		if(state.nFuncIndex == 11)
		{
				state.nFuncIndex = 0;
				ENABLE_SHOW

				Tm1652_Show_Updata('b', 'o', '-', 'C', ' ');
				delay_ms(1000);

				//����Ļ
				Tm1652_Show_Close();
				delay_ms(10);
				
				DISABLE_SHOW
		}

		//��������
		if(state.nKeyPress != HARD_KEY_EMPTY)
		{
				ENABLE_SHOW

				switch (state.nKeyPress)
				{
						case HARD_KEY_UP:
								//��ʾ��ʱ1״̬
								if(g_tMainConfig.tCtrl.alarm_duration_enable_1)
								{
										//��ʱ��,��ʾʣ��ʱ��
										Tm1652_Show_Updata('F', '1', '-', 'O', ' ');
										tempDelayCount = 10;
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										//����ģʽ��һֱ��ʾ����ʱ
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
										//��ʾʣ��ʱ��
										if(GetDurationRemainTime(1, &remainDurationHour, &remainDurationMinute, &remainDurationSecond) && showDurationRemain_1 == false)
										{
												//����1��������ʾСʱ���ӣ�����˸ð��
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
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
												//С��1��������ʾ����  ��32������˸ð��
												Tm1652_Time_Show_Second(remainDurationSecond,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
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
										//��ʱ�ر�,��ʾ�趨ʱ��
										Tm1652_Show_Updata('F', '1', '-', 'C', ' ');

										tempDelayCount = 10;
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										Tm1652_Time_Show(g_tMainConfig.tCtrl.alarm_duration_1.duration_hour,g_tMainConfig.tCtrl.alarm_duration_1.duration_minute,1);
										tempDelayCount = 10;
										while(tempDelayCount--)				//�����˳�
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
								//��ʾ��ʱ2״̬
								if(g_tMainConfig.tCtrl.alarm_duration_enable_2)
								{
										//��ʱ����
										Tm1652_Show_Updata('F', '2', '-', 'O', ' ');
										tempDelayCount = 10;
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										//����ģʽ��һֱ��ʾ����ʱ
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
										//��ʾʣ��ʱ��
										if(GetDurationRemainTime(2, &remainDurationHour, &remainDurationMinute, &remainDurationSecond) && showDurationRemain_2 == false)
										{
												//����1��������ʾСʱ���ӣ�����˸ð��
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
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
												//С��1��������ʾ����  ��32������˸ð��
												Tm1652_Time_Show_Second(remainDurationSecond,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
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
										//��ʱ�ر�,��ʾ�趨ʱ��
										Tm1652_Show_Updata('F', '2', '-', 'C', ' ');

										tempDelayCount = 10;
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										Tm1652_Time_Show(g_tMainConfig.tCtrl.alarm_duration_2.duration_hour,g_tMainConfig.tCtrl.alarm_duration_2.duration_minute,1);
										tempDelayCount = 10;
										while(tempDelayCount--)				//�����˳�
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
										//��ʾ����ģʽ
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
										while(tempDelayCount--)				//�����˳�
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
				
				//����Ļ
				Tm1652_Show_Close();
				delay_ms(10);
				
				DISABLE_SHOW

				//�尴��
				state.nKeyPress = HARD_KEY_EMPTY;
		}
		else
		{
				if(state.nVoicePage==0)
				{
						//����ģʽ��ʾ��ǰʱ��
						if(g_tMainConfig.dispalyMode)
						{
								// ��ʱ��ÿ���Ӳ���һ���¶Ⱥ͵�ص�ѹ
								if(is_out_time)
								{
										//��ȡ��ʪ����ֵ
										if(SHT30_Read_Data(recv_dat_list) == Ok)
										{
												SHT30_Data_To_Float(recv_dat_list, &temperature, &humidity);
										}
										//��ȡ����  ÿʮ��������һ��
										if(Get_Bat_Voltage()<29)
										{
												low_power_show_count++;
												//��ȡ��ǰʱ�䣬ҹ�䲻������
												I2C_DS3231_ReadTime();
												if(calendar.hour>=9 && calendar.hour<=21)
												{
														if(low_power_remind_enable && low_power_show_count>=9)
														{
																u8TxCnt = 1;
																u8TxData[2] = 0x0A;		//�������Ϊ10
																
																u8TxData[3] = 1;
																
																u8TxData[4] = 0x55;
																u8TxData[5] = 0xAA;
																uart_Send_Length = 6;
																Uart_SendDataIt(M0P_UART1, u8TxData[0]); //����UART1���͵�һ����
														}
												}
												if(low_power_show_count>=9)
												{
														ENABLE_SHOW
														tempDelayCount = 5;
														while(tempDelayCount--)				//�����˳�
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

								//����ģʽ��ʾ��ǰ����ʱ
								if(showDurationRemain_1 && !showDurationRemain_2)
								{
										//��ʾ��ʱ��1����ʱ
										//��ʾʣ��ʱ��
										if(GetDurationRemainTime(1, &remainDurationHour, &remainDurationMinute, &remainDurationSecond))
										{
												//����1��������ʾСʱ���ӣ�����˸ð��
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
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
												//С��1��������ʾ����  ��32������˸ð��
												Tm1652_Time_Show_Second(remainDurationSecond,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond-1,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond-1,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
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
										//��ʾ��ʱ��2����ʱ
										//��ʾʣ��ʱ��
										if(GetDurationRemainTime(2, &remainDurationHour, &remainDurationMinute, &remainDurationSecond))
										{
												//����1��������ʾСʱ���ӣ�����˸ð��
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show(remainDurationHour,remainDurationMinute,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
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
												//С��1��������ʾ����  ��32������˸ð��
												Tm1652_Time_Show_Second(remainDurationSecond,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond-1,1);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
												{
														delay_ms(100);
														if(state.nPageShow != PAGE_SLEEP)
														{
																break;
														}
												}
												Tm1652_Time_Show_Second(remainDurationSecond-1,0);
												tempDelayCount = 5;
												while(tempDelayCount--)				//�����˳�
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
										//����ģʽ����ʾ��ǰʱ��
										I2C_DS3231_ReadTime();
										ds3231_hour = calendar.hour;					//��ȡСʱ����
										ds3231_minute = calendar.minute;			//��ȡ��������
										if(g_tMainConfig.timeMode)
										{
												if(ds3231_hour > 12)
												{
														ds3231_hour -= 12;
												}
										}
										Tm1652_Time_Show(ds3231_hour,ds3231_minute,1);
										//ÿ100msˢ��
										delay_ms(100);
								}
						}
						else
						{
								//ʡ��ģʽ�ر���ʾ
								Tm1652_Show_Close();
								DISABLE_SHOW

								//��������ģʽ
								Lpm_GotoSleep(false);
								
								// ��ʱ��ÿ���Ӳ���һ���¶Ⱥ͵�ص�ѹ
								if(is_out_time)
								{
										//��ȡ��ʪ����ֵ
										if(SHT30_Read_Data(recv_dat_list) == Ok)
										{
												SHT30_Data_To_Float(recv_dat_list, &temperature, &humidity);
										}
										//��ȡ����  ÿʮ��������һ��
										if(Get_Bat_Voltage()<29)
										{
												low_power_show_count++;
												//��ȡ��ǰʱ�䣬ҹ�䲻������
												I2C_DS3231_ReadTime();
												if(calendar.hour>=9 && calendar.hour<=21)
												{
														if(low_power_remind_enable && low_power_show_count>=9)
														{
																u8TxCnt = 1;
																u8TxData[2] = 0x0A;		//�������Ϊ10
																
																u8TxData[3] = 1;
																
																u8TxData[4] = 0x55;
																u8TxData[5] = 0xAA;
																uart_Send_Length = 6;
																Uart_SendDataIt(M0P_UART1, u8TxData[0]); //����UART1���͵�һ����
														}
												}
												if(low_power_show_count>=9)
												{
														ENABLE_SHOW
														tempDelayCount = 5;
														while(tempDelayCount--)				//�����˳�
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
						//�������ƻ���
						ENABLE_SHOW

						switch(state.nVoicePage)
						{
								case 1:
										//����
										break;
								case 2:
										//�ص�
										break;
								case 3:
										//���ȵ���һ��
										config.pConfig = &g_tMainConfig;
										if(config.pConfig->displayLightSet < 8)
										{
												config.pConfig->displayLightSet += 1;
										}
										//��ʾ��������
										Tm1652_Brightness_set(config.pConfig->displayLightSet);
										//��ʾ��ǰʱ����ΪЧ��Ԥ��
										I2C_DS3231_ReadTime();
										ds3231_hour = calendar.hour;					//��ȡСʱ����
										ds3231_minute = calendar.minute;			//��ȡ��������
										Tm1652_Time_Show(ds3231_hour,ds3231_minute,1);
										//�������
										if(MainConfigCorrect())
										{
												MainConfigSave();
										}
										tempDelayCount = 30;
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										//�����ʾ
										Tm1652_Show_Close();
										delay_ms(10);
										break;
								case 4:
										//���ȵ���һ��
										config.pConfig = &g_tMainConfig;
										if(config.pConfig->displayLightSet > 1)
										{
												config.pConfig->displayLightSet -= 1;
										}
										//��ʾ��������
										Tm1652_Brightness_set(config.pConfig->displayLightSet);
										//��ʾ��ǰʱ����ΪЧ��Ԥ��
										I2C_DS3231_ReadTime();
										ds3231_hour = calendar.hour;					//��ȡСʱ����
										ds3231_minute = calendar.minute;			//��ȡ��������
										Tm1652_Time_Show(ds3231_hour,ds3231_minute,1);
										//�������
										if(MainConfigCorrect())
										{
												MainConfigSave();
										}
										tempDelayCount = 30;
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										//�����ʾ
										Tm1652_Show_Close();
										delay_ms(10);
										break;
								case 5:
										//�ر�����
										break;
								case 6:
										//��ʾʱ��
										I2C_DS3231_ReadTime();
										ds3231_hour = calendar.hour;					//��ȡСʱ����
										ds3231_minute = calendar.minute;			//��ȡ��������
										if(g_tMainConfig.timeMode)
										{
												if(ds3231_hour > 12)
												{
														ds3231_hour -= 12;
												}
										}
										Tm1652_Time_Show(ds3231_hour,ds3231_minute,1);
										tempDelayCount = 30;
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										//�����ʾ
										Tm1652_Show_Close();
										delay_ms(10);
										break;
								case 7:
										//��ʾ�¶�
										Tm1652_Temperature_Show(temperature);
										tempDelayCount = 30;
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										//�����ʾ
										Tm1652_Show_Close();
										delay_ms(10);
										break;
								case 8:
										//��ʾʪ��
										Tm1652_Humidity_Show(humidity);
										tempDelayCount = 30;
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nPageShow != PAGE_SLEEP)
												{
														break;
												}
										}
										//�����ʾ
										Tm1652_Show_Close();
										delay_ms(10);
										break;
								case 9:
										//ȡ����ʱ
										I2C_DS3231_SetAlarm_1(false,1,1,1,1);
										I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
										break;
								case 10:
										//ȡ������
										I2C_DS3231_SetAlarm_2(false, 1, 1);
										I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
										break;
								case 13:
										//��ʱ
										if(state.nPageShow == PAGE_SLEEP)
										{
												//��ʱ���ܣ������ʱҳ��
												I2C_DS3231_ReadTime();
												beginDs3231Hour = calendar.hour;					//��ȡСʱ����	
												beginDs3231Minute = calendar.minute;			//��ȡ��������	
												beginDs3231Second = calendar.second;			//��ȡ��������	

												//��ʾ��ʱʱ��
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
						//������ϣ���λ
						state.nVoicePage = 0;
				}
		}
}

//���ò�������
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
								//��ʱ
								Tm1652_Show_Updata('T', 'C', 'J', 'S', ' ');
								break;
						case 1:
								//����1
								Tm1652_Show_Updata('A', 'L', '-', '1', ' ');
								break;
						case 2:
								//����2
								Tm1652_Show_Updata('A', 'L', '-', '2', ' ');
								break;
						case 3:
								//����3
								Tm1652_Show_Updata('A', 'L', '-', '3', ' ');
								break;
						case 4:
								//����ʱ��
								Tm1652_Show_Updata('S', 'E', 'H', 'F', ' ');
								break;
						case 5:
								//���������գ��Զ���������
								Tm1652_Show_Updata('S', 'E', 'E', 'E', ' ');
								break;
						case 6:
								//�¶Ȳ���
								Tm1652_Show_Updata('O', 'F', '-', 'P', ' ');
								break;
						case 7:
								//ʪ�Ȳ���
								Tm1652_Show_Updata('O', 'F', '-', 'H', ' ');
								break;
						case 8:
								//����ģʽ
								Tm1652_Show_Updata('S', 'E', '-', 'C', ' ');
								break;
						case 9:
								//�����ָ�����
								Tm1652_Show_Updata('R', 'S', 'E', 'T', ' ');
								break;
						case 10:
								//���ڸ�ʽ
								Tm1652_Show_Updata('S', 'E', '-', 'T', ' ');
								break;
						case 11:
								//��������
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
												//��ʱ���ܣ������ʱҳ��
												I2C_DS3231_ReadTime();
												beginDs3231Hour = calendar.hour;					//��ȡСʱ����	
												beginDs3231Minute = calendar.minute;			//��ȡ��������	
												beginDs3231Second = calendar.second;			//��ȡ��������	

												//��ʾ��ʱʱ��
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
		//�����ʾ
		Tm1652_Show_Close();
		delay_ms(10);

		DISABLE_SHOW
		
		//��������
		MainConfigSave();

		//ҳ��״̬�л�
		state.nPageShow = PAGE_SLEEP;
}

//��ʱ����
void Page_Timing_Function(void)
{
		bool pageExit = false;

		//�ر���ʾ
		bool isShowClose = false;

		//��˸��ʾ����
		uint8_t twinkleCount = 0;

		//���Ӷ�ȡ����
		uint8_t secondReadCount = 0;

		//��ͣʱ��
		uint8_t pauseDs3231Hour = 0;
		uint8_t pauseDs3231Minute = 0;
		uint8_t pauseDs3231Second = 0;
		//�Ƿ���ͣ
		bool isPause = false;

		//��ͣʱ��
		uint32_t pastTime = 0;

		//������ʱ����
		uint32_t i,j;

		//��ʱ��ʱ������
		uint8_t delayCount = 4;
		uint8_t tempDelayCount = delayCount;

		ENABLE_SHOW
		
		state.nKeyPress = HARD_KEY_EMPTY;
		
		while(!pageExit)
		{
				// ��ʱ��ÿ���Ӳ���һ���¶Ⱥ͵�ص�ѹ
				if(is_out_time)
				{
						//��ȡ��ʪ����ֵ
						if(SHT30_Read_Data(recv_dat_list) == Ok)
						{
								SHT30_Data_To_Float(recv_dat_list, &temperature, &humidity);
						}
						//��ȡ����  ÿʮ��������һ��
						if(Get_Bat_Voltage()<29)
						{
								low_power_show_count++;
								if(low_power_remind_enable && low_power_show_count>=9)
								{
										u8TxCnt = 1;
										u8TxData[2] = 0x0A;		//�������Ϊ10

										u8TxData[3] = 1;
										
										u8TxData[4] = 0x55;
										u8TxData[5] = 0xAA;
										uart_Send_Length = 6;
										Uart_SendDataIt(M0P_UART1, u8TxData[0]); //����UART1���͵�һ����
								}
								if(low_power_show_count>=9)
								{
										//ֻ��������ʾ
										low_power_show_count = 0;
								}
						}
						is_out_time = false;
				}
				if(isPause)
				{
						//�������ͣ����ʾ��ʱʱ��
						if(timingHourCount>0 || timingMinuteCount>0)
						{
								//ʱ������1����
								Tm1652_Time_Show(timingHourCount,timingMinuteCount,1);
								while(tempDelayCount--)				//�����˳�
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
								while(tempDelayCount--)				//�����˳�
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
								//ʱ��С��1����
								Tm1652_Time_Show_Second(timingSecondCount,1);
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(state.nPageShow != PAGE_TIMING)
										{
												return;
										}
								}
								tempDelayCount = delayCount;
								Tm1652_Show_Close();
								while(tempDelayCount--)				//�����˳�
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
						//��ʼ��ʱ
						//��ȡ��ǰʱ��
						I2C_DS3231_ReadTime();
						//����Сʱ
						if(timingHourCount>0 || timingMinuteCount>0 || timingSecondCount>0)
						{
								if(calendar.hour<beginDs3231Hour)
								{
										//������ҹ
										timingHourCount = (beginDs3231Hour + 24) - calendar.hour;
								}
								else
								{
										//û�й���ҹ
										timingHourCount = calendar.hour - beginDs3231Hour;
								}
						}
						//�������
						if(calendar.minute<beginDs3231Minute)
						{
								//��Сʱ��
								timingHourCount -= 1;
								timingMinuteCount = (calendar.minute + 60) - beginDs3231Minute;
						}
						else
						{
								//û�й�Сʱ
								timingMinuteCount = calendar.minute - beginDs3231Minute;
						}
						//��������
						if(calendar.second<beginDs3231Second)
						{
								//��������
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
								//û�й�����
								timingSecondCount = calendar.second - beginDs3231Second;
						}
						if(timingHourCount == 0 && timingMinuteCount == 0)
						{
								//С��1��������ʾ����  ��32������˸ð��
								if(secondReadCount<5)
								{
										Tm1652_Time_Show_Second(timingSecondCount,1);
								}
								else
								{
										Tm1652_Time_Show_Second(timingSecondCount,0);
								}
								//����ˢ��ȷ��������ȷ
								delay_ms(100);
								secondReadCount++;
								if(secondReadCount==10)
								{
										secondReadCount = 0;
								}
						}
						else
						{
								//����1��������ʾСʱ���ӣ�����˸ð��
								Tm1652_Time_Show(timingHourCount,timingMinuteCount,1);
								while(tempDelayCount--)				//�����˳�
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
								while(tempDelayCount--)				//�����˳�
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
				//��������ʱʱ�����˳�
				if(timingHourCount == 23 && timingMinuteCount == 59 && timingSecondCount>55)
				{
						if(!isPause)
						{
								//û����ͣ��ʱ���˳�
								pageExit = true;
								//�رռ�ʱ
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
												//����ʾ
												ENABLE_SHOW
												isShowClose = false;
										}
										else
										{
												//�ϼ���������ʾ��ʱ����
												if(isPause)
												{
														if(timingHourCount>0 || timingMinuteCount>0)
														{
																//��ʱ����1����
																//�����ͣ�������Ӳ�����
																for(twinkleCount=0; twinkleCount<3; twinkleCount++)
																{
																		Tm1652_Time_Show_Second(timingSecondCount,1);
																		while(tempDelayCount--)				//�����˳�
																		{
																				delay_ms(100);
																				if(state.nPageShow != PAGE_TIMING)
																				{
																						return;
																				}
																		}
																		tempDelayCount = delayCount;
																		Tm1652_Show_Close();
																		while(tempDelayCount--)				//�����˳�
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
														//�������ͣ������������
														if(timingHourCount>0 || timingMinuteCount>0 )
														{
																for(twinkleCount=0; twinkleCount<3; twinkleCount++)
																{
																		Tm1652_Time_Show_Second(timingSecondCount,1);
																		while(tempDelayCount--)				//�����˳�
																		{
																				delay_ms(100);
																				if(state.nPageShow != PAGE_TIMING)
																				{
																						return;
																				}
																		}
																		tempDelayCount = delayCount;
																		Tm1652_Time_Show_Second(timingSecondCount,0);
																		while(tempDelayCount--)				//�����˳�
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
										//�¼���������ʾ��ǰʱ��
										for(twinkleCount=0; twinkleCount<3; twinkleCount++)
										{
												I2C_DS3231_ReadTime();
												ds3231_hour = calendar.hour;					//��ȡСʱ����
												ds3231_minute = calendar.minute;			//��ȡ��������
												if(g_tMainConfig.timeMode)
												{
														if(ds3231_hour > 12)
														{
																ds3231_hour -= 12;
														}
												}
												Tm1652_Time_Show(ds3231_hour,ds3231_minute,1);
												while(tempDelayCount--)				//�����˳�
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
												//����ʾ
												ENABLE_SHOW
												isShowClose = false;
										}
										else
										{
												//�ر���ʾ
												Tm1652_Show_Close();
												delay_ms(10);

												DISABLE_SHOW
												isShowClose = true;
										}
										break;
								case HARD_KEY_CANCEL:
										//�˳�����������ͣ��ʱ����ʾ��ǰ��ʱʱ�����ٴΰ����˳�
										if(isShowClose)
										{
												//����ʾ
												ENABLE_SHOW
												isShowClose = false;
										}
										else
										{
												if(isPause)
												{
														//�Ѿ���ͣ��ʱ���˳�
														pageExit = true;
														//�رռ�ʱ
														isTiming = false;
												}
												else
												{
														//û����ͣ����ͣ��ʱ
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
												//����ʾ
												ENABLE_SHOW
												isShowClose = false;
										}
										else
										{
												if(isPause)
												{
														//�Ѿ���ͣ��ʱ��ָ�
														isPause = false;
														//�ָ�֮ǰ������ͣʱ��
														I2C_DS3231_ReadTime();

														//������ͣʱ��
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
														//��ͣ����
														i = pastTime%60;
														beginDs3231Second += i;
														//��ͣ����
														i = pastTime/60%60;
														beginDs3231Minute += i;
														//��ͣʱ��
														i = pastTime/3600;
														beginDs3231Hour += i;
														
														//��������
														if(beginDs3231Second > 59)
														{
																beginDs3231Second -= 60;
																beginDs3231Minute += 1;
														}
														//��������
														if(beginDs3231Minute > 59)
														{
																beginDs3231Minute -= 60;
																beginDs3231Hour += 1;
														}
														//����ʱ��
														if(beginDs3231Hour > 23)
														{
																beginDs3231Second -= 24;
														}
												}
												else
												{
														//û����ͣ����ͣ��ʱ
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
				//ȷ�ϼ���������ͣ��ʱ���ٴΰ��¼�����ʱ
		}
		//�����ʾ
		Tm1652_Show_Close();
		delay_ms(10);

		DISABLE_SHOW

		//ҳ��״̬�л�
		state.nPageShow = PAGE_SLEEP;
}

//���ò�������--�����˵�
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
					//����1ʱ��
					tempSetHour = g_tMainConfig.tCtrl.alarm_moment_1.moment_hour;
					tempSetMinute = g_tMainConfig.tCtrl.alarm_moment_1.moment_minute;
					break;
				case 2:
					//����2ʱ��
					tempSetHour = g_tMainConfig.tCtrl.alarm_moment_2.moment_hour;
					tempSetMinute = g_tMainConfig.tCtrl.alarm_moment_2.moment_minute;
					break;
				case 3:
					//����3ʱ��
					tempSetHour = g_tMainConfig.tCtrl.alarm_moment_3.moment_hour;
					tempSetMinute = g_tMainConfig.tCtrl.alarm_moment_3.moment_minute;
					break;
				case 4:
					//ʱ���趨��ʱֵ
					//��ȡʱ������
					I2C_DS3231_ReadTime();
					tempSetHour = calendar.hour;
					tempSetMinute = calendar.minute;
					break;
				case 5:
					//�����趨��ʱֵ
					//��ȡʱ������
					I2C_DS3231_ReadTime();
					tempSetYearL = calendar.yearL;
					tempSetMonth = calendar.month;
					tempSetDate 	 = calendar.date;
					break;
				case 6:
					
					//��ȡ��ʪ����ֵ
					if(SHT30_Read_Data(recv_dat_list) == Ok)
					{
							SHT30_Data_To_Float(recv_dat_list, &temperature, &humidity);
					}
					//�¶���У��ֵ
					tempOffsetTemperature = 0;
					break;
				case 7:
					//��ȡ��ʪ����ֵ
					if(SHT30_Read_Data(recv_dat_list) == Ok)
					{
							SHT30_Data_To_Float(recv_dat_list, &temperature, &humidity);
					}
					//ʪ����У��ֵ
					tempOffsetHumidity = 0;
					break;
				case 8:
					//����ģʽ
					tempDisplayMode = g_tMainConfig.dispalyMode;
					break;
				case 9:
					//�ָ���������
					//��һ��SET�������ȷ�Ͼͻָ������������ȡ���˳�
					break;
				case 10:
					//12/24Сʱģʽ
					tempTimeMode = g_tMainConfig.timeMode;
					break;
				case 11:
					//��������
					//����Ԥ����
					break;
		}
		
		//��ʱ��ʱ������
		uint8_t delayCount = 4;
		uint8_t tempDelayCount = delayCount;
		
		//�������尴��
		state.nKeyPress = HARD_KEY_EMPTY;
		
		while(!pageExit)
		{
				if(changeMode == 0)
				{
						switch (index)
						{
								case 1:
										//����1
								case 2:
										//����2
								case 3:
										//����3
								case 4:
										//ʱ���趨--ʱ
										Tm1652_Time_Show_Minute(tempSetMinute,1);
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;

										Tm1652_Time_Show(tempSetHour,tempSetMinute,1);
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 5:
										//�����趨--��
										Tm1652_Time_Show_Year(tempSetYearL);
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;

										Tm1652_Show_Updata('2', '0', ' ', ' ', ' ');
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;

								case 6:
										//�¶�У׼ ���뵱ǰ�¶�
										//��ȡ��ǰ�¶Ȳ���ʾ
										tempTemperature = temperature + tempOffsetTemperature;
										Tm1652_Temperature_Show(tempTemperature);
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										//��˸��ʾ
										Tm1652_Show_Updata(' ', ' ', ' ', 'C', ' ');
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 7:
										//ʪ��У׼
										//��ȡ��ǰ�¶Ȳ���ʾ
										tempHumidity = humidity + tempOffsetHumidity;
										Tm1652_Humidity_Show(tempHumidity);
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										//��˸��ʾ
										Tm1652_Show_Updata(' ', ' ', ' ', 'H', ' ');
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 8:
										//����ģʽ
										//��˸ģʽ����
										if(tempDisplayMode)
										{
												Tm1652_Show_Updata(' ', '-', 'C', 'L', ' ');
										}
										else
										{
												Tm1652_Show_Updata(' ', '-', 'P', 'S', ' ');
										}
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										//��˸��ʾ
										Tm1652_Show_Updata(' ', '-', ' ', ' ', ' ');
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 9:
										//�ָ�����
										//��˸SET
										Tm1652_Show_Updata(' ', 'S', 'E', 'T', ' ');
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										//��˸��ʾ
										Tm1652_Show_Updata(' ', ' ', ' ', ' ', ' ');
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 10:
										//ʱ��ģʽ
										//��˸ģʽ����
										if(tempTimeMode)
										{
												Tm1652_Show_Updata(' ', '-', '1', '2', ' ');
										}
										else
										{
												Tm1652_Show_Updata(' ', '-', '2', '4', ' ');
										}
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										//��˸��ʾ
										Tm1652_Show_Updata(' ', '-', ' ', ' ', ' ');
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 11:
										//��������
										if(broadcastKey != HARD_KEY_EMPTY)
										{
												ENABLE_SHOW

												//��Ϊ�����Ͷ�Ӧ������Ϣ��ͬʱ��ʾ1�밴�İ���
												switch(broadcastKey)
												{
														case HARD_KEY_UP:
																u8TxCnt = 1;
																u8TxData[2] = 0xFF;		//���������������Ϊ255
																u8TxData[3]	= 11;			//�ϼ�
																u8TxData[4] = 0x55;
																u8TxData[5] = 0xAA;
																uart_Send_Length = 6;
																Uart_SendDataIt(M0P_UART1, u8TxData[0]); //����UART1���͵�һ����
																//��ʾ���İ���
																Tm1652_Show_Updata('b', 'o', '-', '1', ' ');
																delay_ms(500);
																break;
														case HARD_KEY_DOWN:
																u8TxCnt = 1;
																u8TxData[2] = 0xFF;		//���������������Ϊ255
																u8TxData[3]	= 22;			//�¼�
																u8TxData[4] = 0x55;
																u8TxData[5] = 0xAA;
																uart_Send_Length = 6;
																Uart_SendDataIt(M0P_UART1, u8TxData[0]); //����UART1���͵�һ����
																//��ʾ���İ���
																Tm1652_Show_Updata('b', 'o', '-', '2', ' ');
																delay_ms(500);
																break;
														case HARD_KEY_CANCEL:
																u8TxCnt = 1;
																u8TxData[2] = 0xFF;		//���������������Ϊ255
																u8TxData[3]	= 33;			//ȡ����
																u8TxData[4] = 0x55;
																u8TxData[5] = 0xAA;
																uart_Send_Length = 6;
																Uart_SendDataIt(M0P_UART1, u8TxData[0]); //����UART1���͵�һ����
																//��ʾ���İ���
																Tm1652_Show_Updata('b', 'o', '-', '3', ' ');
																delay_ms(500);
																break;
														case HARD_KEY_SET:
																u8TxCnt = 1;
																u8TxData[2] = 0xFF;		//���������������Ϊ255
																u8TxData[3]	= 44;			//ȷ�ϼ�
																u8TxData[4] = 0x55;
																u8TxData[5] = 0xAA;
																uart_Send_Length = 6;
																Uart_SendDataIt(M0P_UART1, u8TxData[0]); //����UART1���͵�һ����
																//��ʾ���İ���
																Tm1652_Show_Updata('b', 'o', '-', '4', ' ');
																delay_ms(500);
																break;
												}
												//����������
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
										//����1
								case 2:
										//����2
								case 3:
										//����3
								case 4:
										//ʱ���趨--ʱ��
										Tm1652_Time_Show_Hour(tempSetHour,1);
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;

										Tm1652_Time_Show(tempSetHour,tempSetMinute,1);
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 5:
										//�����趨--������
										Tm1652_Time_Show_Month(tempSetMonth);
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;

										Tm1652_Show_Updata(' ', '-', ' ', ' ', ' ');
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;
										break;
								case 6:
										//�¶�У׼
										//ȷ���¶�У׼ֵ�������˳�
										break;
								case 7:
										//ʪ��У׼
										//ȷ��ʪ��У׼ֵ�������˳�
										break;
								case 8:
										//����ģʽ
										//ȷ����ʾģʽ�������˳�
										break;
								case 9:
										break;
								case 10:
										//ʱ��ģʽ
										//ȷ��ʱ��ģʽ�������˳�
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
										//�����趨--������
										Tm1652_Time_Show_Day(tempSetDate);
										while(tempDelayCount--)				//�����˳�
										{
												delay_ms(100);
												if(state.nKeyPress != HARD_KEY_EMPTY)
														break;
										}
										tempDelayCount = delayCount;

										Tm1652_Show_Updata(' ', '-', ' ', ' ', ' ');
										while(tempDelayCount--)				//�����˳�
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
																		//����1
																		break;
																case 2:
																		//����2
																		break;
																case 3:
																		//����3
																		break;
																case 4:
																		//ʱ���趨
																		break;
																case 5:
																		//�����趨
																		break;
																case 6:
																		//�¶�У׼
																		//��������ֵ���˳�
																		g_tMainConfig.offsetTemperature += tempOffsetTemperature;
																		returnUpper = false;
																		pageExit = true;
																		break;
																case 7:
																		//ʪ��У׼
																		//��������ֵ���˳�
																		g_tMainConfig.offsetHumidity += tempOffsetHumidity;
																		returnUpper = false;
																		pageExit = true;
																		break;
																case 8:
																		//����ģʽ
																		g_tMainConfig.dispalyMode = tempDisplayMode;
																		returnUpper = false;
																		pageExit = true;
																		break;
																case 9:
																		//�ָ�����
																		MainConfigReset();
																		returnUpper = false;
																		pageExit = true;
																		break;
																case 10:
																		//ʱ��ģʽ
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
																		//����1
																		g_tMainConfig.tCtrl.alarm_moment_1.moment_hour = tempSetHour;
																		g_tMainConfig.tCtrl.alarm_moment_1.moment_minute = tempSetMinute;
																		break;
																case 2:
																		//����2
																		g_tMainConfig.tCtrl.alarm_moment_2.moment_hour = tempSetHour;
																		g_tMainConfig.tCtrl.alarm_moment_2.moment_minute = tempSetMinute;
																		break;
																case 3:
																		//����3
																		g_tMainConfig.tCtrl.alarm_moment_3.moment_hour = tempSetHour;
																		g_tMainConfig.tCtrl.alarm_moment_3.moment_minute = tempSetMinute;
																		break;
																case 4:
																		//ʱ���趨
																		//DS3231д��ʱ������
																		I2C_DS3231_SetTime(calendar.yearL,calendar.month,calendar.date,calendar.week,tempSetHour,tempSetMinute,0);
																		break;
																case 5:
																		//�����趨
																		break;
																case 6:
																		//�¶�У׼
																		break;
																case 7:
																		//ʪ��У׼
																		break;
																case 8:
																		//����ģʽ
																		break;
																case 9:
																		//�ָ�����
																		break;
																case 10:
																		//ʱ��ģʽ
																		break;
																default:
																		returnUpper = true;
																		pageExit = true;
																		break;
														}
														if(index <= 3)
														{
																//������ط����ϲ㣬�򲻽���ѭ�����������ѭ��
																if(Page_Set_Function_Lv3(index))
																{
																		//��Ҫ����ѭ��
																		pageExit = true;
																		//�嶨ʱ��־
																		I2C_DS3231_SetAlarm_2(false, 1, 1);
																		I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
																		//ȷ�����ӿ���
																		switch(JudgeAlarmFirstArrive())
																		{
																				case 0:
																						//û�����ӿ����������һ��DS3231 Alarm2
																						I2C_DS3231_SetAlarm_2(false, 1, 1);
																						I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
																						break;
																				case 1:
																						//ֱ��д��1�ı���ʱ�̣�����Ҫ�ж����ӣ���Ϊ����ʱ�䲻�䣬�Ͷ�ʱ��һ����
																						I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_1.moment_hour, g_tMainConfig.tCtrl.alarm_moment_1.moment_minute);
																						break;
																				case 2:
																						//ֱ��д��2�ı���ʱ�̣�����Ҫ�ж����ӣ���Ϊ����ʱ�䲻�䣬�Ͷ�ʱ��һ����
																						I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_2.moment_hour, g_tMainConfig.tCtrl.alarm_moment_2.moment_minute);
																						break;
																				case 3:
																						//ֱ��д��3�ı���ʱ�̣�����Ҫ�ж����ӣ���Ϊ����ʱ�䲻�䣬�Ͷ�ʱ��һ����
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
																//�����ӡ��������趨����Ҫ����ֱ���˳�
																returnUpper = false;
																pageExit = true;
														}
												}
												if(changeMode == 2)
												{
														switch(index)
														{
																case 5:
																		//�����趨
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
						//�����ϲ�Ϊ�棬���ϲ㲻����ѭ�������Ϊ�٣������������ߣ�����ѭ��
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
		//�����ϲ�Ϊ�棬���ϲ㲻����ѭ�������Ϊ�٣������������ߣ�����ѭ��
		if(returnUpper == true)
		{
				return false;
		}
		else
		{
				return true;
		}
}


//���ò�������--�����˵�
bool Page_Set_Function_Lv3(uint8_t index)
{
		bool pageExit = false;
		bool returnUpper = false;
		bool tempAlarmMomentEnable;

		switch (index)
		{
				case 1:
					//����1
					tempAlarmMomentEnable = g_tMainConfig.tCtrl.alarm_moment_enable_1;
					break;
				case 2:
					//����2
					tempAlarmMomentEnable = g_tMainConfig.tCtrl.alarm_moment_enable_2;
					break;
				case 3:
					//����3
					tempAlarmMomentEnable = g_tMainConfig.tCtrl.alarm_moment_enable_3;
					break;
				case 4:
					//ʱ���趨
					break;
				case 5:
					//�¶�У��ֵ
					break;
				case 6:
					//ʪ��У��ֵ
					break;
		}
		
		//��ʱ��ʱ������
		uint8_t delayCount = 4;
		uint8_t tempDelayCount = delayCount;
		
		//�������尴��
		state.nKeyPress = HARD_KEY_EMPTY;
		
		while(!pageExit)
		{
				switch (index)
				{
						case 1:
								//����1
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
								//����2
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
								//����3
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
								//ʱ���趨
								break;
						case 5:
								//�¶�У׼ ���뵱ǰ�¶�
								break;
						case 6:
								//ʪ��У׼
								break;
						default:
								Tm1652_Show_Updata('E', 'R', 'R', ' ', ' ');
								break;
				}
				while(tempDelayCount--)				//�����˳�
				{
						delay_ms(100);
						if(state.nKeyPress != HARD_KEY_EMPTY)
								break;
				}
				tempDelayCount = delayCount;
				//��˸
				Tm1652_Show_Updata(' ', ' ', ' ', ' ', ' ');
				while(tempDelayCount--)				//�����˳�
				{
						delay_ms(100);
						if(state.nKeyPress != HARD_KEY_EMPTY)
								break;
				}
				tempDelayCount = delayCount;
				
				//��������
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
										//������һ��
										returnUpper = true;
										pageExit = true;
										break;
								case HARD_KEY_SET:
										switch(index)
										{
												case 1:
														//����1
														//���û�и����Ӳ����ã�����У������ϲ��޸�
														if(!tempAlarmMomentEnable)
														{
																//�ж϶�Ӧʱ���Ƿ�������
																if(g_tMainConfig.tCtrl.alarm_moment_enable_2)
																{
																		if(g_tMainConfig.tCtrl.alarm_moment_1.moment_hour == g_tMainConfig.tCtrl.alarm_moment_2.moment_hour)
																		{
																				if(g_tMainConfig.tCtrl.alarm_moment_1.moment_minute == g_tMainConfig.tCtrl.alarm_moment_2.moment_minute)
																				{
																						//�����ϲ��޸�
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
																						//�����ϲ��޸�
																						returnUpper = true;
																				}
																		}
																}
														}
														if(!returnUpper)
														{
																//û��������и�ֵ
																g_tMainConfig.tCtrl.alarm_moment_enable_1 = !tempAlarmMomentEnable;
														}
														break;
												case 2:
														//����2
														//���û�и����Ӳ����ã�����У������ϲ��޸�
														if(!tempAlarmMomentEnable)
														{
																//�ж϶�Ӧʱ���Ƿ�������
																if(g_tMainConfig.tCtrl.alarm_moment_enable_1)
																{
																		if(g_tMainConfig.tCtrl.alarm_moment_1.moment_hour == g_tMainConfig.tCtrl.alarm_moment_2.moment_hour)
																		{
																				if(g_tMainConfig.tCtrl.alarm_moment_1.moment_minute == g_tMainConfig.tCtrl.alarm_moment_2.moment_minute)
																				{
																						//�����ϲ��޸�
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
																						//�����ϲ��޸�
																						returnUpper = true;
																				}
																		}
																}
														}
														if(!returnUpper)
														{
																//û��������и�ֵ
																g_tMainConfig.tCtrl.alarm_moment_enable_2 = !tempAlarmMomentEnable;
														}
														break;
												case 3:
														//����3
														//���û�и����Ӳ����ã�����У������ϲ��޸�
														if(!tempAlarmMomentEnable)
														{
																//�ж϶�Ӧʱ���Ƿ�������
																if(g_tMainConfig.tCtrl.alarm_moment_enable_1)
																{
																		if(g_tMainConfig.tCtrl.alarm_moment_1.moment_hour == g_tMainConfig.tCtrl.alarm_moment_3.moment_hour)
																		{
																				if(g_tMainConfig.tCtrl.alarm_moment_1.moment_minute == g_tMainConfig.tCtrl.alarm_moment_3.moment_minute)
																				{
																						//�����ϲ��޸�
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
																						//�����ϲ��޸�
																						returnUpper = true;
																				}
																		}
																}
														}
														if(!returnUpper)
														{
																//û��������и�ֵ
																g_tMainConfig.tCtrl.alarm_moment_enable_3 = !tempAlarmMomentEnable;
														}
														break;
												case 4:
														//ʱ���趨
														break;
												case 5:
														//�¶�У׼ ���뵱ǰ�¶�
														
														break;
												case 6:
														//ʪ��У׼

														break;
												default:
														break;
										}
										//��⵽���󣬷����ϲ�
										if(returnUpper == true)
										{
												state.nKeyPress = HARD_KEY_EMPTY;
												return false;
										}
										//��ȷ�����򲻷����ϲ�
										returnUpper = false;
										pageExit = true;
								default:
										break;
						}
						state.nKeyPress = HARD_KEY_EMPTY;
				}
				if(state.nPageShow != PAGE_SET)
				{
						//�����ϲ�Ϊ�棬���ϲ㲻����ѭ�������Ϊ�٣������������ߣ�����ѭ��
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
		//�����ϲ�Ϊ�棬���ϲ㲻����ѭ�������Ϊ�٣������������ߣ�����ѭ��
		if(returnUpper == true)
		{
				return false;
		}
		else
		{
				return true;
		}
}

//����ģʽ����
void Page_Ctrl_Function(void)
{
		bool pageExit = false;

		//��ʱ��ʱ������
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
				while(tempDelayCount--)				//�����˳�
				{
						delay_ms(100);
						if(state.nKeyPress != HARD_KEY_EMPTY)
								break;
				}
				tempDelayCount = delayCount;
				Tm1652_Show_Updata(' ', ' ', ' ', ' ', ' ');
				while(tempDelayCount--)				//�����˳�
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
										//��ʾ����ʧ��
										Tm1652_Show_Updata('F', 'A', 'L', 'S', ' ');
										delay_ms(2000);

										pageExit = 1;
										break;
								case HARD_KEY_SET:
										g_tMainConfig.ctrlModeSet = state.nFuncIndex;
										//��ʾ���óɹ�
										Tm1652_Show_Updata('S', 'U', 'C', 'C', ' ');
										delay_ms(2000);

										pageExit = 1;
										//���п���
										//�����Ӧʹ���ж�
										if(config.pConfig->ctrlModeSet==0||config.pConfig->ctrlModeSet==1)
										{
												//�ߵ�ƽ��
												Gpio_SetIO(GpioPortA,GpioPin10);
										}
										else
										{
												Gpio_ClrIO(GpioPortA,GpioPin10);
										}
										//��������ʹ���ж�
										if(config.pConfig->ctrlModeSet==1||config.pConfig->ctrlModeSet==3)
										{
												//�ߵ�ƽ�ر�
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
		//�����ʾ
		Tm1652_Show_Close();
		delay_ms(10);

		DISABLE_SHOW

		//��������
		MainConfigSave();

		//ҳ��״̬�л�
		state.nPageShow = PAGE_SLEEP;
}

//��ʱ1���ù���
void Page_Timer1_Function(void)
{
		//��ʱ��ʱ������
		uint8_t delayCount = 4;
		uint8_t tempDelayCount = delayCount;

		bool pageExit = false;
		uint8_t changeMode = 0;
		uint8_t tempDurationHour = g_tMainConfig.tCtrl.alarm_duration_1.duration_hour;
		uint8_t tempDurationMinute = g_tMainConfig.tCtrl.alarm_duration_1.duration_minute;

		state.nKeyPress = HARD_KEY_EMPTY;

		ENABLE_SHOW

		Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
		while(tempDelayCount--)				//�����˳�
		{
				delay_ms(100);
		}
		tempDelayCount = delayCount;

		while(!pageExit)
		{
				if(g_tMainConfig.tCtrl.alarm_duration_enable_1)
				{
						//��ʾһ��ԭ��ʱʱ��
						Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
						delay_ms(800);
						//��ʾ����
						Tm1652_Show_Updata(' ', ' ', ' ', ' ', ' ');
						delay_ms(10);
						
						//�رն�ʱ1
						switch(JudgeDurationFirstArrive())
						{
								case 0:
										//û�ж�ʱ����
										//������ָ����
										break;
								case 1:
										//��ʱ1���ڼ�ʱ
										//��ʱ1�رռ�ʱ���鿴��ʱ2�Ƿ���Ҫ��ʱ����Ҫ��ʱ������ʱ2��ʱ
										I2C_DS3231_SetAlarm_1(false,1,1,1,1);
										I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����

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
										//��ʱ2���ڼ�ʱ
										//ֱ�ӵ�����ʱ1������־λ���������ʵʱʱ�ӱ�����ʱ
										g_tMainConfig.tCtrl.alarm_duration_enable_1 = false;
										break;
						}

						//��ʾ��ʱ�ѹر�
						Tm1652_Show_Updata('F', '1', '-', 'C', ' ');
						delay_ms(1500);

						//��ʾ����ʱ��
						showDurationRemain_1 = false;

						pageExit = true;
				}
				else
				{
						//���붨ʱ�趨
						if(changeMode == 0)
						{
								Tm1652_Time_Show_LHour_Minute(tempDurationHour, tempDurationMinute,1);
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;

								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//�����˳�
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
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//�����˳�
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
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//�����˳�
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
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//�����˳�
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
														//���涨ʱ1����
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
														//���涨ʱ����ʱ��
														g_tMainConfig.tCtrl.alarm_duration_1.alarm_yearL = alarm_yearL;
														g_tMainConfig.tCtrl.alarm_duration_1.alarm_month = alarm_month;
														g_tMainConfig.tCtrl.alarm_duration_1.alarm_date = alarm_date;
														g_tMainConfig.tCtrl.alarm_duration_1.alarm_hour = alarm_hour;
														g_tMainConfig.tCtrl.alarm_duration_1.alarm_minute = alarm_minute;
														g_tMainConfig.tCtrl.alarm_duration_1.alarm_second = alarm_second;
														//�ж϶�ʱ2�Ƿ��ڶ�ʱ��Ȼ����бȽ�ȷ����һ���ȶ�ʱ�������Ƚ�����д��ʵʱʱ�ӱ���1
														switch(JudgeDurationFirstArrive())
														{
																case 0:
																		//û�ж�ʱ����
																		//������ָ����
																		break;
																case 1:
																		//��ʱ1��ʼ��ʱ
																		I2C_DS3231_SetAlarm_1(false,1,1,1,1);
																		I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
																		I2C_DS3231_SetAlarm_1(true,alarm_date,alarm_hour,alarm_minute,alarm_second);
																		break;
																case 2:
																		//��ʱ2���ڼ�ʱ
																		//��������ȴ���ʱ2����
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
		//�����ʾ
		Tm1652_Show_Close();
		delay_ms(10);

		DISABLE_SHOW

		//��������
		MainConfigSave();

		//ҳ��״̬�л�
		state.nPageShow = PAGE_SLEEP;
}

//��ʱ2���ù���
void Page_Timer2_Function(void)
{
		//��ʱ��ʱ������
		uint8_t delayCount = 4;
		uint8_t tempDelayCount = delayCount;

		bool pageExit = false;
		uint8_t changeMode = 0;
		uint8_t tempDurationHour = g_tMainConfig.tCtrl.alarm_duration_2.duration_hour;
		uint8_t tempDurationMinute = g_tMainConfig.tCtrl.alarm_duration_2.duration_minute;

		state.nKeyPress = HARD_KEY_EMPTY;

		ENABLE_SHOW

		Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
		while(tempDelayCount--)				//�����˳�
		{
				delay_ms(100);
		}
		tempDelayCount = delayCount;

		while(!pageExit)
		{
				if(g_tMainConfig.tCtrl.alarm_duration_enable_2)
				{
						//��ʾһ��ԭ��ʱʱ��
						Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
						delay_ms(800);
						//��ʾ����
						Tm1652_Show_Updata(' ', ' ', ' ', ' ', ' ');
						delay_ms(10);
						
						//�رն�ʱ2
						switch(JudgeDurationFirstArrive())
						{
								case 0:
										//û�ж�ʱ����
										//������ָ����
										break;
								case 1:
										//��ʱ1���ڼ�ʱ
										//ֱ�ӵ�����ʱ2������־λ���������ʵʱʱ�ӱ�����ʱ
										g_tMainConfig.tCtrl.alarm_duration_enable_2 = false;
										break;
								case 2:
										//��ʱ2���ڼ�ʱ
										//��ʱ2�رռ�ʱ���鿴��ʱ1�Ƿ���Ҫ��ʱ����Ҫ��ʱ������ʱ1��ʱ
										I2C_DS3231_SetAlarm_1(false,1,1,1,1);
										I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
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

						//��ʾ��ʱ�ѹر�
						Tm1652_Show_Updata('F', '2', '-', 'C', ' ');
						delay_ms(1500);
						
						//��ʾ����ʱ��
						showDurationRemain_2 = false;
					
						pageExit = true;
				}
				else
				{
						//���붨ʱ�趨
						if(changeMode == 0)
						{
								Tm1652_Time_Show_LHour_Minute(tempDurationHour, tempDurationMinute,1);
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;

								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//�����˳�
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
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//�����˳�
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
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//�����˳�
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
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(state.nKeyPress != HARD_KEY_EMPTY)
												break;
								}
								tempDelayCount = delayCount;
								Tm1652_Time_Show(tempDurationHour,tempDurationMinute,1);
								while(tempDelayCount--)				//�����˳�
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
														//���涨ʱ2����
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
														//���涨ʱ����ʱ��
														g_tMainConfig.tCtrl.alarm_duration_2.alarm_yearL = alarm_yearL;
														g_tMainConfig.tCtrl.alarm_duration_2.alarm_month = alarm_month;
														g_tMainConfig.tCtrl.alarm_duration_2.alarm_date = alarm_date;
														g_tMainConfig.tCtrl.alarm_duration_2.alarm_hour = alarm_hour;
														g_tMainConfig.tCtrl.alarm_duration_2.alarm_minute = alarm_minute;
														g_tMainConfig.tCtrl.alarm_duration_2.alarm_second = alarm_second;
														//�ж϶�ʱ2�Ƿ��ڶ�ʱ��Ȼ����бȽ�ȷ����һ���ȶ�ʱ�������Ƚ�����д��ʵʱʱ�ӱ���1
														switch(JudgeDurationFirstArrive())
														{
																case 0:
																		//û�ж�ʱ����
																		//������ָ����
																		break;
																case 1:
																		//��ʱ1���ڼ�ʱ
																		//��������ȴ���ʱ1����
																		break;
																case 2:
																		//��ʱ2��ʼ��ʱ
																		I2C_DS3231_SetAlarm_1(false,1,1,1,1);
																		I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
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
		//�����ʾ
		Tm1652_Show_Close();
		delay_ms(10);

		DISABLE_SHOW

		//��������
		MainConfigSave();

		//ҳ��״̬�л�
		state.nPageShow = PAGE_SLEEP;
}

//��������
void Page_Alarm_Function(void)
{
		//��ʱ��ʱ������
		uint8_t delayCount = 10;
		uint8_t tempDelayCount = delayCount;

		uint8_t alarmType = 0;

		uint8_t readData = 0x04;


		//���趨����
		durationSetStep = 0;
		u8SetDurationData[1] = 0;
		u8SetDurationData[3] = 0;
		alarmSetStep = 0;
		u8SetAlarmData[1] = 0;
		u8SetAlarmData[3] = 0;


		// DS3231æ��ȴ�
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
				//֪ͨ��ʱ1ʱ�䵽
				
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
				while(tempDelayCount--)				//�����˳�
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

		//�����������ʱ����رձ�����ʱд���¶�ʱ
		switch(alarmType)
		{
				case 1:
						//������λ
						I2C_DS3231_SetAlarm_1(false,1,1,1,1);
						I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
						g_tMainConfig.tCtrl.alarm_duration_enable_1 = false;
						showDurationRemain_1 = false;
						
						if(JudgeDurationFirstArrive() == 2)
						{
								alarm_date = g_tMainConfig.tCtrl.alarm_duration_2.alarm_date;
								alarm_hour = g_tMainConfig.tCtrl.alarm_duration_2.alarm_hour;
								alarm_minute = g_tMainConfig.tCtrl.alarm_duration_2.alarm_minute;
								alarm_second = g_tMainConfig.tCtrl.alarm_duration_2.alarm_second;
								//��ȡȷ����ʱ2����ʱ����û�й��ڣ����������˵��ͬһʱ�̵���
								if(JudgeAlarmOutline(2))
								{
										//�Ѿ����ڣ���رն�ʱ������־
										g_tMainConfig.tCtrl.alarm_duration_enable_2 = false;
								}
								else
								{
										//û�й�����д�����ʱ��
										I2C_DS3231_SetAlarm_1(true,alarm_date,alarm_hour,alarm_minute,alarm_second);
								}
						}
						break;
				case 2:
						//������λ
						I2C_DS3231_SetAlarm_1(false,1,1,1,1);
						I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
						g_tMainConfig.tCtrl.alarm_duration_enable_2 = false;
						showDurationRemain_2 = false;

						if(JudgeDurationFirstArrive() == 1)
						{
								alarm_date = g_tMainConfig.tCtrl.alarm_duration_1.alarm_date;
								alarm_hour = g_tMainConfig.tCtrl.alarm_duration_1.alarm_hour;
								alarm_minute = g_tMainConfig.tCtrl.alarm_duration_1.alarm_minute;
								alarm_second = g_tMainConfig.tCtrl.alarm_duration_1.alarm_second;
								//��ȡȷ����ʱ1����ʱ����û�й��ڣ����������˵��ͬһʱ�̵���
								if(JudgeAlarmOutline(1))
								{
										//�Ѿ����ڣ���رն�ʱ������־
										g_tMainConfig.tCtrl.alarm_duration_enable_1 = false;
								}
								else
								{
										//û�й�����д�����ʱ��
										I2C_DS3231_SetAlarm_1(true,alarm_date,alarm_hour,alarm_minute,alarm_second);
								}
						}	
						break;
				case 3:
						//������λ
						I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
						OpenMomentNextArrive(1);
						break;
				case 4:
						//������λ
						I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
						OpenMomentNextArrive(2);
						break;
				case 5:
						//������λ
						I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
						OpenMomentNextArrive(3);
						break;
		}

		DISABLE_SHOW

		//ҳ��״̬�л�
		if(isTiming)
		{
				//���ڼ�ʱ�򷵻�
				state.nPageShow = PAGE_TIMING;
		}
		else
		{
				state.nPageShow = PAGE_SLEEP;
		}
}

//������ʱ����
void Page_Duration_Set_Function(void)
{
		bool isSetDuration = true;
		//��ʱ��ʱ������
		uint8_t delayCount = 3;
		uint8_t tempDelayCount = delayCount;

		ENABLE_SHOW

		while(1)
		{
				switch(durationSetStep)
				{
						case 0:
								//���ö�ʱ�쳣���˳�
								state.nPageShow = PAGE_SLEEP;
								break;
						case 1:
								//���붨ʱ�趨
								//ʱ��ͬʱ��˸
								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(durationSetStep != 1)
										{
												break;
										}
								}
								
								Tm1652_Time_Show_Colon(1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(durationSetStep != 1)
										{
												break;
										}
								}
								break;
						case 2:
								//�����һ����  ʱ��ͬʱ��˸
								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(durationSetStep != 2)
										{
												break;
										}
								}

								Tm1652_Time_Show_Colon(1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(durationSetStep != 2)
										{
												break;
										}
								}
								break;
						case 3:
								//����ڶ�����  ʱ��ͬʱ��˸
								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(durationSetStep != 3)
										{
												break;
										}
								}

								Tm1652_Time_Show_Colon(1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(durationSetStep != 3)
										{
												break;
										}
								}
								break;
						case 4:
								//ȷ��Сʱ   ʱֹͣ��˸ ����˸
								Tm1652_Time_Show_Hour(u8SetDurationData[1], 1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(durationSetStep != 4)
										{
												break;
										}
								}

								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(durationSetStep != 4)
										{
												break;
										}
								}
								break;
						case 5:
								//�趨���� 	ʱֹͣ��˸ ����˸
								Tm1652_Time_Show_Hour(u8SetDurationData[1], 1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(durationSetStep != 5)
										{
												break;
										}
								}

								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(durationSetStep != 5)
										{
												break;
										}
								}
								break;
						case 6:
								//�յ�������ʾ
								//��ʾ��ʱʱ��
								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],0);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(durationSetStep != 6)
										{
												break;
										}
								}

								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(durationSetStep != 6)
										{
												break;
										}
								}
								break;
						case 7:
								//�յ���ʱȷ��
								//��鶨ʱ��ֵ�Ƿ���ȷ
								//��ʱ��ֵ��ȷ���ö�ʱ1	
								//��ʾ��ʱʱ��
								Tm1652_Time_Show(u8SetDurationData[1],u8SetDurationData[3],1);
								delay_ms(1000);
								if(u8SetDurationData[1]!=0)
								{
										if(u8SetDurationData[3]>=60)
										{
												//��������ȡ����ʱ
												isSetDuration = false;
										}
								}
								else if(u8SetDurationData[1]==0)
								{
										//Сʱ������û��С��60��������
										if( u8SetDurationData[3]==0)
										{
												//��������ȡ����ʱ
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
										//���ö�ʱ
										//���涨ʱ1����
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
										//���涨ʱ����ʱ��
										g_tMainConfig.tCtrl.alarm_duration_1.alarm_yearL = alarm_yearL;
										g_tMainConfig.tCtrl.alarm_duration_1.alarm_month = alarm_month;
										g_tMainConfig.tCtrl.alarm_duration_1.alarm_date = alarm_date;
										g_tMainConfig.tCtrl.alarm_duration_1.alarm_hour = alarm_hour;
										g_tMainConfig.tCtrl.alarm_duration_1.alarm_minute = alarm_minute;
										g_tMainConfig.tCtrl.alarm_duration_1.alarm_second = alarm_second-1;
										//�ж϶�ʱ2�Ƿ��ڶ�ʱ��Ȼ����бȽ�ȷ����һ���ȶ�ʱ�������Ƚ�����д��ʵʱʱ�ӱ���1
										switch(JudgeDurationFirstArrive())
										{
												case 0:
														//û�ж�ʱ����
														//������ָ����
														break;
												case 1:
														//��ʱ1��ʼ��ʱ
														I2C_DS3231_SetAlarm_1(false,1,1,1,1);
														I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
														I2C_DS3231_SetAlarm_1(true,alarm_date,alarm_hour,alarm_minute,alarm_second-1);
														break;
												case 2:
														//��ʱ2���ڼ�ʱ
														//��������ȴ���ʱ2����
														break;
										}
										//��ʾ�ɹ�
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
								//������ɣ�������
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

//�������ӹ���
void Page_Alarm_Set_Function(void)
{
		bool isSetAlarm = true;
		//��ʱ��ʱ������
		uint8_t delayCount = 3;
		uint8_t tempDelayCount = delayCount;

		ENABLE_SHOW

		while(1)
		{
				switch(alarmSetStep)
				{
						case 0:
								//���ö�ʱ�쳣���˳�
								state.nPageShow = PAGE_SLEEP;
								break;
						case 1:
								//���붨ʱ�趨
								//ʱ��ͬʱ��˸
								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(alarmSetStep != 1)
										{
												break;
										}
								}
								
								Tm1652_Time_Show_Colon(1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(alarmSetStep != 1)
										{
												break;
										}
								}
								break;
						case 2:
								//�����һ����  ʱ��ͬʱ��˸
								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(alarmSetStep != 2)
										{
												break;
										}
								}

								Tm1652_Time_Show_Colon(1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(alarmSetStep != 2)
										{
												break;
										}
								}
								break;
						case 3:
								//����ڶ�����  ʱ��ͬʱ��˸
								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(alarmSetStep != 3)
										{
												break;
										}
								}

								Tm1652_Time_Show_Colon(1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(alarmSetStep != 3)
										{
												break;
										}
								}
								break;
						case 4:
								//ȷ��Сʱ   ʱֹͣ��˸ ����˸
								Tm1652_Time_Show_Hour(u8SetAlarmData[1], 1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(alarmSetStep != 4)
										{
												break;
										}
								}

								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(alarmSetStep != 4)
										{
												break;
										}
								}
								break;
						case 5:
								//�趨���� 	ʱֹͣ��˸ ����˸
								Tm1652_Time_Show_Hour(u8SetAlarmData[1], 1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(alarmSetStep != 5)
										{
												break;
										}
								}

								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(alarmSetStep != 5)
										{
												break;
										}
								}
								break;
						case 6:
								//�յ�������ʾ
								//��ʾ��ʱʱ��
								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],0);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(alarmSetStep != 6)
										{
												break;
										}
								}

								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],1);
								tempDelayCount = delayCount;
								while(tempDelayCount--)				//�����˳�
								{
										delay_ms(100);
										if(alarmSetStep != 6)
										{
												break;
										}
								}
								break;
						case 7:
								//�յ���ʱȷ��
								//��鶨ʱ��ֵ�Ƿ���ȷ
								//��ʱ��ֵ��ȷ���ö�ʱ1	
								//��ʾ��ʱʱ��
								Tm1652_Time_Show(u8SetAlarmData[1],u8SetAlarmData[3],1);
								delay_ms(1000);
								if(u8SetAlarmData[1]>=24)
								{
										//��������ȡ��
										isSetAlarm = false;
								}
								if(u8SetAlarmData[3]>=60)
								{
										//��������ȡ����ʱ
										isSetAlarm = false;
								}

								if(isSetAlarm)
								{
										//���ö�ʱ
										g_tMainConfig.tCtrl.alarm_moment_1.moment_hour = u8SetAlarmData[1];
										g_tMainConfig.tCtrl.alarm_moment_1.moment_minute = u8SetAlarmData[3];
										g_tMainConfig.tCtrl.alarm_moment_enable_1 = TRUE;
										//�嶨ʱ��־
										I2C_DS3231_SetAlarm_2(false, 1, 1);
										I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
										//ȷ�����ӿ���
										switch(JudgeAlarmFirstArrive())
										{
												case 0:
														//û�����ӿ����������һ��DS3231 Alarm2
														I2C_DS3231_SetAlarm_2(false, 1, 1);
														I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
														break;
												case 1:
														//ֱ��д��1�ı���ʱ�̣�����Ҫ�ж����ӣ���Ϊ����ʱ�䲻�䣬�Ͷ�ʱ��һ����
														I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_1.moment_hour, g_tMainConfig.tCtrl.alarm_moment_1.moment_minute);
														break;
												case 2:
														//ֱ��д��2�ı���ʱ�̣�����Ҫ�ж����ӣ���Ϊ����ʱ�䲻�䣬�Ͷ�ʱ��һ����
														I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_2.moment_hour, g_tMainConfig.tCtrl.alarm_moment_2.moment_minute);
														break;
												case 3:
														//ֱ��д��3�ı���ʱ�̣�����Ҫ�ж����ӣ���Ϊ����ʱ�䲻�䣬�Ͷ�ʱ��һ����
														I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_3.moment_hour, g_tMainConfig.tCtrl.alarm_moment_3.moment_minute);
														break;
										}
										//��ʾ�ɹ�
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
								//������ɣ�������
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
