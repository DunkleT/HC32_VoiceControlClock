#include "gpio.h"
#include "ddl.h"
#include "i2c.h"

#include "bsp_gpio.h"
#include "bsp_delay.h"
#include "bsp_uart.h"
#include "bsp_flash.h"
#include "bsp_adc.h"
#include "bsp_timer.h"

#include "tm1652.h"
#include "ds3231.h"
#include "sht3x.h"

#include "segment.h"
#include "pama.h"

#define ENABLE_SHOW			Gpio_SetIO(GpioPortB,GpioPin11);delay_ms(10);
#define DISABLE_SHOW		delay_ms(10);Gpio_ClrIO(GpioPortB,GpioPin11);

//��ȡ��ʪ��
extern uint8_t recv_dat_list[6];
//��ȡ����ʪ��ֵ
extern float temperature;
extern float humidity;

/*
*               ����������������趨
*
*		1��ȫ�ֺ궨��		����ȫ����д��ĸ��ʹ��_���е��ʼ��
*   2��ȫ�ֱ�����		����ȫ��Сд��ĸ��ʹ��_���е��ʼ��
*		3���ֲ�������		������������ĸ��һ��Сд��������д���շ�������
*   4��������				������������ĸ��д��ʹ��_���е��ʼ��
*		5��ȫ���б���		����β׺_list
*		6���ֲ��б���		����β׺List
*   7���½��ṹ��		������������ĸ��һ����д��������д���շ�������
*
*
*/

int main(void)
{

		//ϵͳʱ�ӳ�ʼ��
		bsp_clk_init();

		//�ⲿ�������ų�ʼ��
		Bsp_Ctrl_GPIO_Init();

		//�������ų�ʼ��
		Bsp_Key_Gpio_Init();
	
		//�������͵����ų�ʼ��
		Bsp_TGS_Gpio_Init();

		//�����������ų�ʼ��
		Bsp_Voice_Gpio_Init();
		
		//ʱ�ӱ����ж����ų�ʼ��
		Bsp_Alarm_Gpio_Init();

		//�������������ų�ʼ��
		Bsp_Horn_Gpio_Init();

		//TM1652��ʼ��
		Tm1652_Init();

		//I2C��ʼ��
		I2C_Cfg_Init();
		I2C_Port_Init();

    ///< ��I2C���߷���ʼ�ź�
    I2C_SetFunc(M0P_I2C0,I2cStart_En); 

		//SHT30�����Բ�����ʼ��
		SHT30_Init();

		//DS3231��ʼ��
		DS3231_Init();

		//uart1��ʼ��
		Uart1_Port_Init();
		Uart1_Port_Cfg();

		//flash��ʼ��
		flash_module_init();
		
		//������������
		MainConfigLoadAtBoot();

		//ADC��ʼ��
		ADC_Init();

		//ͨ�ö�ʱ��3��ʼ��
		Timer3_Init();

		//��ʱ1����һ���ж�
		Tim3_Reset(200);

		//ҳ�����̳�ʼ��
		Bsp_Page_Ctrl_Init();

		//����ͬʱ�ϵ�
		ENABLE_SHOW
		
		//��ʾ����
		Tm1652_Show_Updata('d', 'u', 'n', 'K', ' ');
		delay_ms(1000);
		DISABLE_SHOW

		while (1)
		{
				Main_Page_Show();
		}
}

