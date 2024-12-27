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

//读取温湿度
extern uint8_t recv_dat_list[6];
//读取的温湿度值
extern float temperature;
extern float humidity;

/*
*               本程序代码有以下设定
*
*		1、全局宏定义		——全部大写字母，使用_进行单词间隔
*   2、全局变量名		——全部小写字母，使用_进行单词间隔
*		3、局部变量名		——单词首字母第一个小写，其他大写，驼峰命名法
*   4、函数名				——单词首字母大写，使用_进行单词间隔
*		5、全局列表名		——尾缀_list
*		6、局部列表名		——尾缀List
*   7、新建结构体		——单词首字母第一个大写，其他大写，驼峰命名法
*
*
*/

int main(void)
{

		//系统时钟初始化
		bsp_clk_init();

		//外部控制引脚初始化
		Bsp_Ctrl_GPIO_Init();

		//按键引脚初始化
		Bsp_Key_Gpio_Init();
	
		//人体热释电引脚初始化
		Bsp_TGS_Gpio_Init();

		//语音控制引脚初始化
		Bsp_Voice_Gpio_Init();
		
		//时钟报警中断引脚初始化
		Bsp_Alarm_Gpio_Init();

		//蜂鸣器控制引脚初始化
		Bsp_Horn_Gpio_Init();

		//TM1652初始化
		Tm1652_Init();

		//I2C初始化
		I2C_Cfg_Init();
		I2C_Port_Init();

    ///< 向I2C总线发起开始信号
    I2C_SetFunc(M0P_I2C0,I2cStart_En); 

		//SHT30周期性测量初始化
		SHT30_Init();

		//DS3231初始化
		DS3231_Init();

		//uart1初始化
		Uart1_Port_Init();
		Uart1_Port_Cfg();

		//flash初始化
		flash_module_init();
		
		//加载配置设置
		MainConfigLoadAtBoot();

		//ADC初始化
		ADC_Init();

		//通用定时器3初始化
		Timer3_Init();

		//定时1分钟一次中断
		Tim3_Reset(200);

		//页面流程初始化
		Bsp_Page_Ctrl_Init();

		//避免同时上电
		ENABLE_SHOW
		
		//显示作者
		Tm1652_Show_Updata('d', 'u', 'n', 'K', ' ');
		delay_ms(1000);
		DISABLE_SHOW

		while (1)
		{
				Main_Page_Show();
		}
}

