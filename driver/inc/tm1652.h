#ifndef __TM1652_H__
#define __TM1652_H__

#include "ddl.h"
#include "gpio.h"

//TM1652 GPIO∂®“Â
#define TM1652_SDA_PORT				GpioPortC
#define	TM1652_SDA_PIN				GpioPin14

void Tm1652_Brightness_set(uint8_t level);
void Tm1652_Init(void);
void Tm1652_Send(uint8_t data);
uint8_t Tm1652_Transform(uint8_t showChar) ;
void Tm1652_Show_Updata(uint8_t char1, uint8_t char2, uint8_t char3, uint8_t char4, uint8_t char5);
void Tm1652_Show_Number(uint16_t number);
void Tm1652_Time_Show(uint8_t hour, uint8_t minute, uint8_t showColon);
void Tm1652_Time_Show_Year(uint8_t year);
void Tm1652_Time_Show_Month(uint8_t month);
void Tm1652_Time_Show_Day(uint8_t day);
void Tm1652_Time_Show_Hour(uint8_t hour, uint8_t showColon);
void Tm1652_Time_Show_Minute(uint8_t minute, uint8_t showColon);
void Tm1652_Time_Show_LHour_Minute(uint8_t hour, uint8_t minute, uint8_t showColon);
void Tm1652_Time_Show_HHour_Minute(uint8_t hour, uint8_t minute, uint8_t showColon);
void Tm1652_Time_Show_Hour_LMinute(uint8_t hour, uint8_t minute, uint8_t showColon);
void Tm1652_Time_Show_Hour_HMinute(uint8_t hour, uint8_t minute, uint8_t showColon);
void Tm1652_Time_Show_Second(uint8_t second, uint8_t showColon);
void Tm1652_Time_Show_Colon(uint8_t showColon);
void Tm1652_Temperature_Show(float temperature);
void Tm1652_Humidity_Show(float humidity);
void Tm1652_Show_Close(void);
#endif /* __TM1652_H__ */
