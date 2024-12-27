#ifndef __BSP_PWM_H__
#define __BSP_PWM_H__

#include "sysctrl.h"
#include "stdbool.h"
#include "gpio.h"
#include "bt.h"
#include "adt.h"

void bsp_bt_pwm_init(void);
void bsp_bt_pwm_ctrl_output(bool en);
void bsp_adt_pwm_init(void);
void bsp_adt_pwm_ctrl_output(bool en);

#endif
