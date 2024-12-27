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

//��ȡ��ʪ��
extern uint8_t recv_dat_list[6];

//��ȡ����ʪ��ֵ
extern float temperature;
extern float humidity;

//��ȡ�ĵ�ǰʱ��
extern _calendar_obj calendar;	//�����ṹ��

//״̬�洢
extern DeviceState		state;					// �豸״̬
//�����洢
extern MainConfig		g_tMainConfig;		// ϵͳ����

//����������������
extern uint8_t u8TxData[10];
extern uint8_t u8TxCnt;
extern uint8_t uart_Send_Length;

uint8_t tempDs3231Hour;

///< PortA �жϷ�����
//�����жϣ����źŸߵ�ƽ�����źŵ͵�ƽ
void PortA_IRQHandler(void)
{
		//�ϼ�
    if(TRUE == Gpio_GetIrqStatus(GpioPortA, GpioPin3))
    {
				//�ϼ���������ʱ1����
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
										state.nKeyPress = HARD_KEY_EMPTY;		// �尴��
								}
								else
								{
										//δ������ҳ���̰���ʾһ��
										state.nKeyPress = HARD_KEY_UP;	// ������
										state.nPageShow = PAGE_SLEEP;
								}
						}
				}
				//�ϼ��̰�
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
												state.nKeyPress = HARD_KEY_UP;	// ������
										}
								}
						}
				}
				Gpio_ClearIrq(GpioPortA, GpioPin3);
    }
		//�¼�
    if(TRUE == Gpio_GetIrqStatus(GpioPortA, GpioPin4))
    {
				//�¼���������ʱ2����
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
										state.nKeyPress = HARD_KEY_EMPTY;		// �尴��
								}
								else
								{
										//δ������ҳ���̰���ʾһ��
										state.nKeyPress = HARD_KEY_DOWN;	// ������
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
												state.nKeyPress = HARD_KEY_DOWN;	// ������
										}
								}
						}
				}
				Gpio_ClearIrq(GpioPortA, GpioPin4);
    }
		//ȡ����
    if(TRUE == Gpio_GetIrqStatus(GpioPortA, GpioPin5))
    {
				//ȡ������������ģʽ����
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
												state.nKeyPress = HARD_KEY_EMPTY;		// �尴��
										}
										else
										{
												state.nKeyPress = HARD_KEY_CANCEL;	// ������
										}
								}
								else
								{
										if(flag==1)
										{
												state.nPageShow = PAGE_CTRL;  
												state.nKeyPress = HARD_KEY_EMPTY;		// �尴��
										}
										else
										{
												state.nKeyPress = HARD_KEY_CANCEL;	// ������
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
												state.nKeyPress = HARD_KEY_CANCEL;	// ������
										}
								}
						}
				}
				Gpio_ClearIrq(GpioPortA, GpioPin5);
    }
		//���ü�
    if(TRUE == Gpio_GetIrqStatus(GpioPortA, GpioPin6))
    {
				//���ü��������������ò˵�
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
										state.nKeyPress = HARD_KEY_EMPTY;		// �尴��
								}
								else
								{
										state.nKeyPress = HARD_KEY_SET;	// ������
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
												state.nKeyPress = HARD_KEY_SET;	// ������
										}
								}
						}
				}
				Gpio_ClearIrq(GpioPortA, GpioPin6);
    }
		//��������жϣ����źŵ͵�ƽ�����źŸߵ�ƽ������ʱ��2.3s
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
		//ʵʱʱ�������жϣ����źŸߵ�ƽ�����źŵ͵�ƽ
		if(TRUE == Gpio_GetIrqStatus(GpioPortA, GpioPin2))
    {
				if(Gpio_GetInputIO(GpioPortA, GpioPin2)==0)
				{
						state.nPageShow = PAGE_ALARM; 
				}
				Gpio_ClearIrq(GpioPortA, GpioPin2);
    }
}

///< PortB �жϷ�����
//����ģ���жϣ����źŵ͵�ƽ�����źŸߵ�ƽ
void PortB_IRQHandler(void)
{
		//SU-G1 ��������
		if(TRUE == Gpio_GetIrqStatus(GpioPortB, GpioPin13))
    {
				delay_ms(100);                                                                                                                                                                                        
				//��������
				if(Gpio_GetInputIO(GpioPortB, GpioPin13))
				{
						if(!Gpio_GetInputIO(GpioPortB, GpioPin14))
						{
								u8TxCnt = 1;
								u8TxData[2] = 0x02;		//ʱ�Ӵ��ڴ������Ϊ2
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
								Uart_SendDataIt(M0P_UART1, u8TxData[0]); //����UART1���͵�һ����   
						}
				}
				Gpio_ClearIrq(GpioPortB, GpioPin13);
		}
		//SU-G2 ��������
		if(TRUE == Gpio_GetIrqStatus(GpioPortB, GpioPin14))
    {
				delay_ms(100); 
				//��������
				if(Gpio_GetInputIO(GpioPortB, GpioPin13))
				{
						if(Gpio_GetInputIO(GpioPortB, GpioPin14))
						{
								//����ʱ��
								I2C_DS3231_ReadTime();
								//����ʱ��
								u8TxCnt = 1;
								u8TxData[2] = 0x01;		//ʱ�Ӵ��ڴ������Ϊ1
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
								Uart_SendDataIt(M0P_UART1, u8TxData[0]); //����UART1���͵�һ����
						}
				}
				else
				{
						if(Gpio_GetInputIO(GpioPortB, GpioPin14))
						{
								//��������
								I2C_DS3231_ReadTime();
								u8TxCnt = 1;
								u8TxData[2] = 0x03;		//ʱ�Ӵ��ڴ������Ϊ3
								u8TxData[3] = calendar.yearL/10;	//��ʮλ
								u8TxData[4] = calendar.yearL%10;	//���λ
								u8TxData[5] = calendar.month;			//��
								u8TxData[6] = calendar.date;			//��
								u8TxData[7]	= calendar.week;			//����
								u8TxData[8] = 0x55;
								u8TxData[9] = 0xAA;
								uart_Send_Length = 10;
								Uart_SendDataIt(M0P_UART1, u8TxData[0]); //����UART1���͵�һ����
						}
				}
				Gpio_ClearIrq(GpioPortB, GpioPin14);
		}
}

//�������ų�ʼ��
void Bsp_Key_Gpio_Init(void)
{
		stc_gpio_cfg_t stcGpioCfg;
    DDL_ZERO_STRUCT(stcGpioCfg);

		///<ʹ��GPIO����ʱ���ſؿ���
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE);

    stcGpioCfg.enDir = GpioDirIn;
    stcGpioCfg.enOD = GpioOdDisable;
    stcGpioCfg.enPu = GpioPuEnable;
		stcGpioCfg.enPd = GpioPdDisable;
    stcGpioCfg.enDrv = GpioDrvL;
		
		//key1 �ϼ�
		Gpio_SetAfMode(GpioPortA, GpioPin3, GpioAf0);
    Gpio_Init(GpioPortA, GpioPin3, &stcGpioCfg); 							//��ʼ��Ϊ����ģʽ
		//key2 �¼�
    Gpio_SetAfMode(GpioPortA, GpioPin4, GpioAf0);
    Gpio_Init(GpioPortA, GpioPin4, &stcGpioCfg); 							//��ʼ��Ϊ����ģʽ
		//key3 ȷ�ϼ�
    Gpio_SetAfMode(GpioPortA, GpioPin5, GpioAf0);
    Gpio_Init(GpioPortA, GpioPin5, &stcGpioCfg); 							//��ʼ��Ϊ����ģʽ
		//key4 ȡ����
    Gpio_SetAfMode(GpioPortA, GpioPin6, GpioAf0);
    Gpio_Init(GpioPortA, GpioPin6, &stcGpioCfg); 							//��ʼ��Ϊ����ģʽ
	
		Gpio_ClearIrq(GpioPortA, GpioPin3);
    Gpio_ClearIrq(GpioPortA, GpioPin4);
    Gpio_ClearIrq(GpioPortA, GpioPin5);
    Gpio_ClearIrq(GpioPortA, GpioPin6);

		//< �򿪲����ð����˿�Ϊ�½����ж�
		Gpio_EnableIrq(GpioPortA, GpioPin3, GpioIrqFalling);
		Gpio_EnableIrq(GpioPortA, GpioPin4, GpioIrqFalling);
		Gpio_EnableIrq(GpioPortA, GpioPin5, GpioIrqFalling);
    Gpio_EnableIrq(GpioPortA, GpioPin6, GpioIrqFalling);

    EnableNvic(PORTA_IRQn, IrqLevel2, TRUE);
}

//����������ų�ʼ��
void Bsp_TGS_Gpio_Init(void)
{
		stc_gpio_cfg_t stcGpioCfg;
    DDL_ZERO_STRUCT(stcGpioCfg);

		///<ʹ��GPIO����ʱ���ſؿ���
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE);

    stcGpioCfg.enDir = GpioDirIn;
    stcGpioCfg.enPu = GpioPuEnable;
		stcGpioCfg.enPd = GpioPdDisable;
    stcGpioCfg.enDrv = GpioDrvL;
		
		//�����Ӧ
    Gpio_Init(GpioPortA, GpioPin9, &stcGpioCfg); 							//��ʼ��Ϊ����ģʽ
		
		//< �����Ӧ�жϣ�������
		Gpio_EnableIrq(GpioPortA, GpioPin9, GpioIrqRising);
		
		EnableNvic(PORTA_IRQn, IrqLevel3, TRUE);
}

//ʵʱʱ�����ӹ���
void Bsp_Alarm_Gpio_Init(void)
{
		stc_gpio_cfg_t stcGpioCfg;
    DDL_ZERO_STRUCT(stcGpioCfg);

		///<ʹ��GPIO����ʱ���ſؿ���
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE);

    stcGpioCfg.enDir = GpioDirIn;
    stcGpioCfg.enOD = GpioOdDisable;
    stcGpioCfg.enPu = GpioPuEnable;
		stcGpioCfg.enPd = GpioPdDisable;
    stcGpioCfg.enDrv = GpioDrvL;
		
		//ʵʱʱ�������ж�
		Gpio_SetAfMode(GpioPortA, GpioPin2, GpioAf0);
    Gpio_Init(GpioPortA, GpioPin2, &stcGpioCfg); 							//��ʼ��Ϊ����ģʽ
	
		Gpio_ClearIrq(GpioPortA, GpioPin2);

		//< �򿪲����������жϣ�������
		Gpio_EnableIrq(GpioPortA, GpioPin2, GpioIrqFalling);

    EnableNvic(PORTA_IRQn, IrqLevel2, TRUE);
}

//���������ų�ʼ��
void Bsp_Horn_Gpio_Init(void)
{
		//������ʹ��PWM����
		bsp_adt_pwm_init();
}


//�����������ų�ʼ��
void Bsp_Voice_Gpio_Init(void)
{
		stc_gpio_cfg_t stcGpioCfg;
    DDL_ZERO_STRUCT(stcGpioCfg);

		///<ʹ��GPIO����ʱ���ſؿ���
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE);
		
    stcGpioCfg.enDir = GpioDirIn;
    stcGpioCfg.enPu = GpioPuDisable;
		stcGpioCfg.enPd = GpioPdEnable;
    stcGpioCfg.enDrv = GpioDrvL;

		///< SU_G1ʹ��
		Gpio_Init(GpioPortB,GpioPin13,&stcGpioCfg);
		Gpio_ClrIO(GpioPortB, GpioPin13);

		///< SU_G2ʹ��
		Gpio_Init(GpioPortB,GpioPin14,&stcGpioCfg);
		Gpio_ClrIO(GpioPortB, GpioPin14);


		Gpio_ClearIrq(GpioPortB, GpioPin13);
		Gpio_ClearIrq(GpioPortB, GpioPin14);

		//< ����ģ���жϣ�������
		Gpio_EnableIrq(GpioPortB, GpioPin13, GpioIrqRising);
		Gpio_EnableIrq(GpioPortB, GpioPin14, GpioIrqRising);

		EnableNvic(PORTB_IRQn, IrqLevel3, TRUE);
}


//�ⲿ�������ų�ʼ��
void Bsp_Ctrl_GPIO_Init(void)
{
		stc_gpio_cfg_t stcGpioCfg;	
		DDL_ZERO_STRUCT(stcGpioCfg);  

		///<ʹ��GPIO����ʱ���ſؿ���
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE);

		stcGpioCfg.enDrv = GpioDrvL;
    stcGpioCfg.enDir = GpioDirOut;
		stcGpioCfg.enPu = GpioPuDisable;
    stcGpioCfg.enPd = GpioPdEnable;
    
    ///< ����ʹ������
		Gpio_Init(GpioPortA,GpioPin10,&stcGpioCfg);
		Gpio_ClrIO(GpioPortA, GpioPin10);

		///< ��ʾʹ������
		Gpio_Init(GpioPortB,GpioPin11,&stcGpioCfg);
		Gpio_SetIO(GpioPortB, GpioPin11);
		
		///< SU_ENʹ�ܣ���������
		Gpio_Init(GpioPortB,GpioPin15,&stcGpioCfg);
		Gpio_ClrIO(GpioPortB, GpioPin15);

		//��������
    Gpio_Init(GpioPortA, GpioPin8, &stcGpioCfg);
		Gpio_ClrIO(GpioPortA, GpioPin8);
}

//ҳ�����̳�ʼ��
void Bsp_Page_Ctrl_Init(void)
{		
		state.nKeyPress = HARD_KEY_EMPTY;
		state.nPageShow = PAGE_MAIN;		
		state.nFuncIndex = 0;
		state.nVoicePage = 0;
}

