#ifndef __BSP_GPIO_H__
#define __BSP_GPIO_H__

#include "gpio.h"

#define HARD_KEY_EMPTY					0		//�ް���
#define HARD_KEY_UP							1		//�ϼ�
#define HARD_KEY_DOWN						2		//�¼�
#define HARD_KEY_CANCEL					3		//ȡ����
#define HARD_KEY_SET						4		//ȷ����

void Bsp_Key_Gpio_Init(void);
void Bsp_TGS_Gpio_Init(void);
void Bsp_Alarm_Gpio_Init(void);
void Bsp_Horn_Gpio_Init(void);
void Bsp_Voice_Gpio_Init(void);
void Bsp_Ctrl_GPIO_Init(void);
void Bsp_Page_Ctrl_Init(void);

#endif
