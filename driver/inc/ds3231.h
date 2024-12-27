#ifndef __DS3231_H__
#define __DS3231_H__


/* Includes ------------------------------------------------------------------*/

#include "gpio.h"


/* Exported types ------------------------------------------------------------*/

typedef struct
{
		uint8_t yearH;			//年千百位
		uint8_t yearL;			//年 
		uint8_t month;			//月 			
		uint8_t date;		  //日 
		uint8_t hour;		  //时 
		uint8_t minute;		//分
		uint8_t second;		//秒
		uint8_t week;		  //周 
}_calendar_obj;

extern _calendar_obj calendar;	//日历结构体

/* DS3231 地址定义 */
/* ADDR Pin Conect to VSS */
#define 	 DS3231_ADDR    		  0xD0
#define    DS3231_ADDR_WRITE    0xD0
#define    DS3231_ADDR_READ     0xD1

/* DS3231寄存器地址 */
typedef enum
{
    DS3231_SECOND = 0x00,    //秒
		DS3231_MINUTE = 0x01,    //分
		DS3231_HOUR   = 0x02,    //时
		DS3231_WEEK   = 0x03,    //星期
		DS3231_DAY    = 0x04,    //日
		DS3231_MONTH  = 0x05,    //月
		DS3231_YEAR   = 0x06,    //年
		/* 闹铃1 */          
		DS3231_ALARM1SECOND   = 0x07,    //秒
		DS3231_ALARM1MINUTE   = 0x08,    //分
		DS3231_ALARM1HOUR     = 0x09,    //时
		DS3231_ALARM1DATE  		= 0x0A,    //星期/日
		/* 闹铃2 */
		DS3231_ALARM2MINUTE 	= 0x0b,    //分
		DS3231_ALARM2HOUR     = 0x0c,    //时
		DS3231_ALARM2DATE     = 0x0d,    //星期/日
		
		DS3231_CONTROL        = 0x0e,    //控制寄存器
		DS3231_STATUS         = 0x0f,    //状态寄存器
		BSY                 	= 2,       //忙
		OSF                		= 7,       //振荡器停止标志
		DS3231_XTAL         	= 0x10,    //晶体老化寄存器
		DS3231_TEMPERATUREH 	= 0x11,    //温度寄存器高字节(8位)
		DS3231_TEMPERATUREL 	= 0x12,    //温度寄存器低字节(高2位) 
} DS3231_CMD;																				
																																								
/* Exported functions ------------------------------------------------------- */

void DS3231_Init(void);		

en_result_t I2C_DS3231_WriteCmd(M0P_I2C_TypeDef* I2CX,DS3231_CMD pu8Addr, uint8_t pu8Data);
en_result_t I2C_MasterRead_DS3231Data(M0P_I2C_TypeDef* I2CX,DS3231_CMD pu8Addr,uint8_t*pu8Data,uint32_t u32Len);

void I2C_DS3231_SetTime(uint8_t year,uint8_t month,uint8_t day,uint8_t week,uint8_t hour,uint8_t minute,uint8_t second);
void I2C_DS3231_ReadTime(void);
void I2C_DS3231_SetAlarm_1(boolean_t en,uint8_t date,uint8_t hour,uint8_t minute,uint8_t second);
void I2C_DS3231_SetAlarm_2(boolean_t en,uint8_t hour,uint8_t minute);
uint8_t I2C_DS3231_ReadTime_Hour(void);
uint8_t I2C_DS3231_ReadTime_Minute(void);

uint8_t I2C_DS3231_getTemperature(void);
uint8_t GregorianDay(uint8_t year,uint8_t month,uint8_t date);

#endif /* __DS3231_H__ */
