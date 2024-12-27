#include "bsp_uart.h"
#include "bsp_gpio.h"
#include "gpio.h"
#include "pama.h"
#include "segment.h"

//״̬�洢
extern DeviceState	state;					// �豸״̬
//�͵�������
extern bool low_power_remind_enable;

uint8_t u8TxData[10] = {0xAA, 0x55, 0x00, 0x01, 0x0C, 0x1E, 0x2E, 0x1A,  0x55, 0xAA};
uint8_t u8RxData[3];

uint8_t u8SetDurationData[5] = {0xDD, 0x00, 0xAA, 0x00, 0xAD};
uint8_t u8SetAlarmData[5] = {0xDA, 0x00, 0xAA, 0x00, 0xAD};

uint8_t u8TxCnt=0,u8RxCnt=0;
uint8_t uart_Send_Length = 0;

uint8_t durationSetStep = 0;
uint8_t alarmSetStep = 0;

//UART1�жϺ���
void Uart1_IRQHandler(void)
{
    if(Uart_GetStatus(M0P_UART1, UartRC))         //UART1���ݽ���
    {
        Uart_ClrStatus(M0P_UART1, UartRC);        //���ж�״̬λ
				
        u8RxData[u8RxCnt] = Uart_ReceiveData(M0P_UART1);   //���������ֽ�
        u8RxCnt++;
				if(u8RxCnt >= 3 && u8RxData[0] == 0xFF)
        {
            u8RxCnt = 0;
						u8TxCnt = 0;
						switch(u8RxData[1])
						{
								case 0x00:
										//ȡ��
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
										//����
										state.nVoicePage = 1;
										Gpio_SetIO(GpioPortA, GpioPin8);
										break;
								case 0x22:
										//�ص�
										state.nVoicePage = 2;
										Gpio_ClrIO(GpioPortA, GpioPin8);
										break;
								case 0x33:
										//��ʾ����
										state.nVoicePage = 3;
										break;
								case 0x44:
										//��ʾ����
										state.nVoicePage = 4;
										break;
								case 0x55:
										//�ر�����
										state.nVoicePage = 5;
										//�жϲ��ܽ���
										if(state.nPageShow == PAGE_ALARM)
										{
												state.nPageShow = PAGE_SLEEP;
										}
										break;
								case 0x66:
										//��ʾʱ��
										state.nVoicePage = 6;
										break;
								case 0x77:
										//��ʾ�¶�
										state.nVoicePage = 7;
										break;
								case 0x88:
										//��ʾʪ��
										state.nVoicePage = 8;
										break;
								case 0x99:
										//ȡ����ʱ
										state.nVoicePage = 9;
										break;
								case 0xAA:
										//ȡ������
										state.nVoicePage = 10;
										break;
								case 0xBB:
										//ȷ��
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
										//�͵�������
										low_power_remind_enable = false;
										break;
								case 0xDD:
										//��ʱ
										state.nVoicePage = 13;
										break;
								default:
										state.nVoicePage = 0;
										break;
						}
        }
				//������ǿ������ݾ�ÿ�����
				if(u8RxData[0] != 0xFF)
				{
						u8RxCnt = 0;
						u8TxCnt = 0;
						switch(durationSetStep)
						{
								case 0:
										if(u8RxData[0] == 0xDD)
										{
												//�趨��ʱ1
												state.nPageShow = PAGE_DURATION_SET;
												durationSetStep = 1;
										}
										break;
								case 1:
										if(u8RxData[0]>10)
										{
												//�쳣
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
														//�쳣
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
												//�쳣
												state.nPageShow = PAGE_SLEEP;
												u8SetDurationData[1] = 0;
												u8SetDurationData[3] = 0;
												durationSetStep = 0;
										}
										break;
								case 4:
										if(u8RxData[0]>10)
										{
												//�쳣
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
														//�쳣
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
												//�趨����1
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
														//�쳣
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
														//�쳣
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
												//�쳣
												state.nPageShow = PAGE_SLEEP;
												u8SetAlarmData[1] = 0;
												u8SetAlarmData[3] = 0;
												alarmSetStep = 0;
										}
										break;
								case 4:
										if(u8RxData[0]>10)
										{
												//�쳣
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
														//�쳣
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
        Uart_ClrStatus(M0P_UART1, UartRC);              //����ж�״̬λ
    }
		if(Uart_GetStatus(M0P_UART1, UartFE)||Uart_GetStatus(M0P_UART1, UartPE))
    {
        //�û�������Ҫ�����޸�
        Uart_ClrStatus(M0P_UART1, UartFE);
        Uart_ClrStatus(M0P_UART1, UartPE);
    }
    if(Uart_GetStatus(M0P_UART1, UartTC))         //UART1���ݷ���
    {
				if(u8TxCnt < uart_Send_Length)
        {
            Uart_SendDataIt(M0P_UART1, u8TxData[u8TxCnt]);//��������
            u8TxCnt++;
        }
				else
        {
            u8TxCnt = 0;
            u8RxCnt = 0;
        }
        Uart_ClrStatus(M0P_UART1, UartTC);        //���ж�״̬λ
    }

}


void Uart1_Port_Init()
{
		stc_gpio_cfg_t stcGpioCfg;

    DDL_ZERO_STRUCT(stcGpioCfg);

    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE); //ʹ��GPIOģ��ʱ��

    stcGpioCfg.enDir = GpioDirOut;
    Gpio_Init(GpioPortD,GpioPin0,&stcGpioCfg);
    Gpio_SetAfMode(GpioPortD,GpioPin0,GpioAf3);             ///<����PD00 ΪUART1 TX
    stcGpioCfg.enDir = GpioDirIn;
    Gpio_Init(GpioPortD,GpioPin1,&stcGpioCfg);
    Gpio_SetAfMode(GpioPortD,GpioPin1,GpioAf3);             ///<����PD01 ΪUART1 RX
}

void Uart1_Port_Cfg()
{
	stc_uart_cfg_t    stcCfg;

    DDL_ZERO_STRUCT(stcCfg);

    ///< ��������ʱ��
    Sysctrl_SetPeripheralGate(SysctrlPeripheralUart1,TRUE);///<ʹ��uart1ģ��ʱ��

    ///<UART Init
    stcCfg.enRunMode        = UartMskMode1;          ///<ģʽ3
    stcCfg.enStopBit        = UartMsk1bit;           ///<1bitֹͣλ
    stcCfg.stcBaud.u32Baud  = 9600;                  ///<������9600
    stcCfg.stcBaud.enClkDiv = UartMsk8Or16Div;       ///<ͨ��������Ƶ����
    stcCfg.stcBaud.u32Pclk  = Sysctrl_GetPClkFreq(); ///<�������ʱ�ӣ�PCLK��Ƶ��ֵ
    Uart_Init(M0P_UART1, &stcCfg);                   ///<���ڳ�ʼ��

    ///<UART�ж�ʹ��
    Uart_ClrStatus(M0P_UART1,UartRC);                ///<���������
    Uart_ClrStatus(M0P_UART1,UartTC);                ///<���������
    Uart_EnableIrq(M0P_UART1,UartRxIrq);             ///<ʹ�ܴ��ڽ����ж�
    Uart_EnableIrq(M0P_UART1,UartTxIrq);             ///<ʹ�ܴ��ڽ����ж�
    EnableNvic(UART1_IRQn, IrqLevel0, TRUE);       ///<ϵͳ�ж�ʹ��
}
