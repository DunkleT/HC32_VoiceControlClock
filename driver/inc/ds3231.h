#ifndef __DS3231_H__
#define __DS3231_H__


/* Includes ------------------------------------------------------------------*/

#include "gpio.h"


/* Exported types ------------------------------------------------------------*/

typedef struct
{
		uint8_t yearH;			//��ǧ��λ
		uint8_t yearL;			//�� 
		uint8_t month;			//�� 			
		uint8_t date;		  //�� 
		uint8_t hour;		  //ʱ 
		uint8_t minute;		//��
		uint8_t second;		//��
		uint8_t week;		  //�� 
}_calendar_obj;

extern _calendar_obj calendar;	//�����ṹ��

/* DS3231 ��ַ���� */
/* ADDR Pin Conect to VSS */
#define 	 DS3231_ADDR    		  0xD0
#define    DS3231_ADDR_WRITE    0xD0
#define    DS3231_ADDR_READ     0xD1

/* DS3231�Ĵ�����ַ */
typedef enum
{
    DS3231_SECOND = 0x00,    //��
		DS3231_MINUTE = 0x01,    //��
		DS3231_HOUR   = 0x02,    //ʱ
		DS3231_WEEK   = 0x03,    //����
		DS3231_DAY    = 0x04,    //��
		DS3231_MONTH  = 0x05,    //��
		DS3231_YEAR   = 0x06,    //��
		/* ����1 */          
		DS3231_ALARM1SECOND   = 0x07,    //��
		DS3231_ALARM1MINUTE   = 0x08,    //��
		DS3231_ALARM1HOUR     = 0x09,    //ʱ
		DS3231_ALARM1DATE  		= 0x0A,    //����/��
		/* ����2 */
		DS3231_ALARM2MINUTE 	= 0x0b,    //��
		DS3231_ALARM2HOUR     = 0x0c,    //ʱ
		DS3231_ALARM2DATE     = 0x0d,    //����/��
		
		DS3231_CONTROL        = 0x0e,    //���ƼĴ���
		DS3231_STATUS         = 0x0f,    //״̬�Ĵ���
		BSY                 	= 2,       //æ
		OSF                		= 7,       //����ֹͣ��־
		DS3231_XTAL         	= 0x10,    //�����ϻ��Ĵ���
		DS3231_TEMPERATUREH 	= 0x11,    //�¶ȼĴ������ֽ�(8λ)
		DS3231_TEMPERATUREL 	= 0x12,    //�¶ȼĴ������ֽ�(��2λ) 
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
