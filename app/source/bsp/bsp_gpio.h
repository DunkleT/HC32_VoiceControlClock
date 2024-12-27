#ifndef __BSP_GPIO_H__
#define __BSP_GPIO_H__

#include "gpio.h"

#define HARD_KEY_EMPTY					0		//无按键
#define HARD_KEY_UP							1		//上键
#define HARD_KEY_DOWN						2		//下键
#define HARD_KEY_CANCEL					3		//取消键
#define HARD_KEY_SET						4		//确定键

void Bsp_Key_Gpio_Init(void);
void Bsp_TGS_Gpio_Init(void);
void Bsp_Alarm_Gpio_Init(void);
void Bsp_Horn_Gpio_Init(void);
void Bsp_Voice_Gpio_Init(void);
void Bsp_Ctrl_GPIO_Init(void);
void Bsp_Page_Ctrl_Init(void);

#endif
