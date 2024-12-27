#include "ds3231.h"
#include "i2c.h"
#include "bsp_delay.h"


// BCD(8421)转DEC
uint8_t BCD_DEC(uint8_t val)
{
	uint8_t i;
	i= val&0x0f;
	val >>= 4;
	val &= 0x0f;
	val *= 10;
	i += val;    
	return i;
}

// DEC转BCD(8421)
uint8_t DEC_BCD(uint8_t val)
{
  uint8_t i,j,k;
  i=val/10;
  j=val%10;
  k=j+(i<<4);
  return k;
}

//DS3231初始化
void DS3231_Init(void)
{
    I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_CONTROL,0x04);    	//闹钟中断允许，初始化禁用闹钟1闹钟2
    I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
}

// DS3231写命令函数，只进行写命令操作
en_result_t I2C_DS3231_WriteCmd(M0P_I2C_TypeDef* I2CX,DS3231_CMD pu8Addr, uint8_t pu8Data)
{
		en_result_t enRet = Error;
		//超时重启值
		uint32_t u32TimeOut = 0xFFFFFFu;
    uint8_t sendCount = 0, u8State = 0;
		I2C_SetFunc(I2CX,I2cStop_En);          ///发送停止条件
		I2C_ClearIrq(I2CX);
    I2C_SetFunc(I2CX,I2cStart_En);										 ///发送起始条件
		while(1)
		{

				while(0 == I2C_GetIrq(I2CX))
				{		
						//超时I2C还未启动
						if(0 == u32TimeOut--)
						{
								//软重启
								//NVIC_SystemReset();

								//由于软启动过于频繁，所以取消，由用户重启
								enRet = ErrorTimeout;
								return enRet;
						}
				}
				u8State = I2C_GetState(I2CX);
				switch(u8State)
				{
						case 0x08:                                 ///已发送起始条件
								I2C_ClearFunc(I2CX, I2cStart_En);
								I2C_WriteByte(I2CX, DS3231_ADDR_WRITE);  ///发送设备地址+W写标志0
								break;
						case 0x18:                                 ///已发送SLW+W，已接收ACK
						case 0x28:                                 ///已发送I2Cx_DATA中的数据，已接收ACK
								switch(sendCount)
								{
										case 0:
												I2C_WriteByte(I2CX,pu8Addr);		 ///发送地址
												break;
										case 1:
												I2C_WriteByte(I2CX,pu8Data);		 ///发送数据
												break;
								}
								sendCount++;
								break;
						case 0x20:                                 ///已发送SLW+W，已接收非ACK
								break;
						case 0x30:                                 ///已发送I2Cx_DATA中的数据，已接收非ACK，将传输一个STOP条件
								I2C_SetFunc(I2CX,I2cStop_En);          ///发送停止条件
								break;
						case 0x58:                                 ///< 已接收到最后一个数据，NACK已返回
                I2C_SetFunc(I2CX,I2cStop_En);          ///< 发送停止条件
                break;
            case 0x38:                                 ///< 在发送地址或数据时，仲裁丢失
                I2C_SetFunc(I2CX,I2cStart_En);         ///< 当总线空闲时发起起始条件
                break;
            case 0x48:                                 ///< 发送SLA+R后，收到一个NACK
                I2C_SetFunc(I2CX,I2cStop_En);          ///< 发送停止条件
                I2C_SetFunc(I2CX,I2cStart_En);         ///< 发送起始条件
                break;
						default:
								break;
				}            
				if(sendCount>2)
				{
						I2C_SetFunc(I2CX,I2cStop_En);              ///此顺序不能调换，出停止条件
						I2C_ClearIrq(I2CX);
						break;
				}
				I2C_ClearIrq(I2CX);                            ///清除中断状态标志位
		}
		enRet = Ok;
    return enRet;
}
 
// 主机读取数据函数，只进行读数据操作
en_result_t I2C_MasterRead_DS3231Data(M0P_I2C_TypeDef* I2CX,DS3231_CMD pu8Addr,uint8_t *pu8Data,uint32_t u32Len)
{
		en_result_t enRet = Error;
		//超时重启值
		uint32_t u32TimeOut = 0xFFFFFFu;
    uint8_t u8State=0;
    uint8_t receiveCount=0;
    uint8_t sendAddrCount=0;
		I2C_SetFunc(I2CX,I2cStop_En);          ///发送停止条件
		I2C_ClearIrq(I2CX);
    I2C_SetFunc(I2CX,I2cStart_En);										 ///发送起始条件

    while(1)
    {
        while(0 == I2C_GetIrq(I2CX))
        {		
						//超时I2C还未启动
						if(0 == u32TimeOut--)
						{
								//软重启
								//NVIC_SystemReset();

								//由于软启动过于频繁，所以取消，由用户重启
								enRet = ErrorTimeout;
								return enRet;
						}
				}
        u8State = I2C_GetState(I2CX);
        switch(u8State)
        {
            case 0x08:                                 ///< 已发送起始条件，将发送SLA+W
								sendAddrCount++;
								I2C_ClearFunc(I2CX,I2cStart_En);
								I2C_WriteByte(I2CX,DS3231_ADDR_WRITE); 
                break;
            case 0x18:                                 ///< 已发送SLA+W,并接收到ACK
                I2C_WriteByte(I2CX, pu8Addr); 					 ///< 读取寄存器位置
                break;
            case 0x28:                                 ///< 已发送数据，接收到ACK, 此处是已发送从机内存地址u8Addr并接收到ACK
								I2C_SetFunc(I2CX,I2cStart_En);
                break;
            case 0x10:                                 ///< 已发送重复起始条件
                I2C_ClearFunc(I2CX,I2cStart_En);
								I2C_WriteByte(I2CX,DS3231_ADDR_READ);///< 发送SLA+R，开始从从机读取数据	
                break;
            case 0x40:                                 ///< 已发送SLA+R，并接收到ACK
                if(u32Len>=1)
                {
                    I2C_SetFunc(I2CX,I2cAck_En);       ///< 使能主机应答功能
                }
                break;
            case 0x50:                                 ///< 已接收数据字节，并已返回ACK信号
                pu8Data[receiveCount] = I2C_ReadByte(I2CX);
								receiveCount++;
                if(receiveCount==u32Len)
                {
                    I2C_ClearFunc(I2CX,I2cAck_En);     ///< 已接收到倒数第二个字节，关闭ACK应答功能
                }
                break;
            case 0x58:                                 ///< 已接收到最后一个数据，NACK已返回
								I2C_ClearFunc(I2CX,I2cStart_En);
                I2C_SetFunc(I2CX,I2cStop_En);          ///< 发送停止条件
								I2C_SetFunc(I2CX,I2cStart_En);
                break;
            case 0x38:                                 ///< 在发送地址或数据时，仲裁丢失
                I2C_SetFunc(I2CX,I2cStart_En);         ///< 当总线空闲时发起起始条件
                break;
            case 0x48:                                 ///< 发送SLA+R后，收到一个NACK
                I2C_SetFunc(I2CX,I2cStop_En);          ///< 发送停止条件
                I2C_SetFunc(I2CX,I2cStart_En);         ///< 发送起始条件
                break;
            default:
                I2C_SetFunc(I2CX,I2cStart_En);         ///< 其他错误状态，重新发送起始条件
                break;
        }
        I2C_ClearIrq(I2CX);                            ///< 清除中断状态标志位
        if(receiveCount==u32Len)                                ///< 数据全部读取完成，跳出while循环
        {
                break;
        }
    }
    enRet = Ok;
    return enRet;
}
 

// DS3231设置时间日期
void I2C_DS3231_SetTime(uint8_t year,uint8_t month,uint8_t day,uint8_t week,uint8_t hour,uint8_t minute,uint8_t second)
{
		uint8_t readData = 0x04;
		// DS3231忙则等待
		while((readData&0x04) > 0)
		{
			I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_STATUS,&readData,1);
		}
		
		if(week == 0)
		{
				week = 7;
		}
	
		I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_SECOND,DEC_BCD(second));
		I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_MINUTE,DEC_BCD(minute));
		I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_HOUR,DEC_BCD(hour));
		I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_WEEK,DEC_BCD(week));
		I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_DAY,DEC_BCD(day));
		I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_MONTH,DEC_BCD(month));
		I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_YEAR,DEC_BCD(year));
}	

// DS3231读取时间日期
void I2C_DS3231_ReadTime()
{
		uint8_t readTimeList[7]= {0};
    
		uint8_t readData = 0x04;
		// DS3231忙则等待
		while((readData&0x04) > 0)
		{
			I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_STATUS,&readData,1);
		}

		I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_SECOND,readTimeList,7);
		
		calendar.yearH  = 20;			//年千百位
		calendar.yearL  = BCD_DEC(readTimeList[6]);			//年 
		calendar.month  = BCD_DEC(readTimeList[5]);			//月 			
		calendar.date   = BCD_DEC(readTimeList[4]);		  //日 
		calendar.hour   = BCD_DEC(readTimeList[2]);		  //时 
		calendar.minute = BCD_DEC(readTimeList[1]);			//分
		calendar.second = BCD_DEC(readTimeList[0]);			//秒
		calendar.week   = BCD_DEC(readTimeList[3]);		  //周 
}	

//DS3231设置Alarm_1
void I2C_DS3231_SetAlarm_1(boolean_t en,uint8_t date,uint8_t hour,uint8_t minute,uint8_t second)
{
		uint8_t readData = 0x04;
		// DS3231忙则等待
		while((readData&0x04) > 0)
		{
			I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_STATUS,&readData,1);
		}

		if(en)
		{
				I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_ALARM1SECOND,DEC_BCD(second));
				I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_ALARM1MINUTE,DEC_BCD(minute));
				I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_ALARM1HOUR,DEC_BCD(hour));
				I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_ALARM1DATE,DEC_BCD(date));

				I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_CONTROL,&readData,1);
				readData |= 0x01;
				I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_CONTROL,readData);    	//复用引脚设置闹钟中断
		}
		else
		{
				I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_CONTROL,&readData,1);
				readData &= 0xFE;
				I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_CONTROL,readData);    	//复用引脚禁用闹钟中断
		}
}	

//DS3231设置Alarm_2
void I2C_DS3231_SetAlarm_2(boolean_t en, uint8_t hour, uint8_t minute)
{
		uint8_t readData = 0x04;
		// DS3231忙则等待
		while((readData&0x04) > 0)
		{
			I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_STATUS,&readData,1);
		}
		
		//设置日期匹配寄存器A2M4位为1,即每日进行时分匹配
		I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_ALARM2DATE,0x80);
		//写入时分匹配数据
		I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_ALARM2MINUTE,DEC_BCD(minute));
		I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_ALARM2HOUR,DEC_BCD(hour));

		if(en)
		{
				I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_CONTROL,&readData,1);
				readData |= 0x02;
				I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_CONTROL,readData);    	//复用引脚设置闹钟中断
		}
		else
		{
				I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_CONTROL,&readData,1);
				readData &= 0xFD;
				I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_CONTROL,readData);    	//复用引脚禁用闹钟中断
		}
}	

// DS3231读取小时数据
uint8_t I2C_DS3231_ReadTime_Hour()
{
		uint8_t readData = 0x04;
		// DS3231忙则等待
		while((readData&0x04) > 0)
		{
			I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_STATUS,&readData,1);
		}

		I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_HOUR,&readData,1);
		
		return readData;
}	

//DS3231读取分钟数据
uint8_t I2C_DS3231_ReadTime_Minute()
{   
		uint8_t readData = 0x04;
		// DS3231忙则等待
		while((readData&0x04) > 0)
		{
			I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_STATUS,&readData,1);
		}

		I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_MINUTE,&readData,1);
		
		return readData;
}	

// 获取温度整数部分
uint8_t I2C_DS3231_getTemperature(void)
{
		uint8_t readData = 0x04;
		// DS3231忙则等待
		while((readData&0x04) > 0)
		{
			I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_STATUS,&readData,1);
		}

		I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_TEMPERATUREH,&readData,1);

		return readData;
}

// 计算公历天数得出星期
uint8_t GregorianDay(uint8_t year,uint8_t month,uint8_t date)
{
		int leapsToDate = 0;
		int day = 0;
		int MonthOffset[] = {0,31,59,90,120,151,181,212,243,273,304,334};

		if(year > 0)
		{
				/*计算从2000年到计数的前一年之中一共经历了多少个闰年*/
				leapsToDate = (year-1)/4 + 1; 
		}
		else
		{
				leapsToDate = 0;
		}
		     
	 
		/*如若计数的这一年为闰年，且计数的月份在2月之后，则日数加1，否则不加1*/
		if((year%4==0)&&(month>2))
		{
			day=1;
		} else {
			day=0;
		}
	 
		day += year*365 + leapsToDate + MonthOffset[month-1] + date; /*计算从2000年元旦到计数日期一共有多少天*/
		day += 5;	/*1999年最后一天是星期五 */
		return day%7; //算出星期
}
