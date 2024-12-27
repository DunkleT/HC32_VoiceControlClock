#include "ds3231.h"
#include "i2c.h"
#include "bsp_delay.h"


// BCD(8421)תDEC
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

// DECתBCD(8421)
uint8_t DEC_BCD(uint8_t val)
{
  uint8_t i,j,k;
  i=val/10;
  j=val%10;
  k=j+(i<<4);
  return k;
}

//DS3231��ʼ��
void DS3231_Init(void)
{
    I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_CONTROL,0x04);    	//�����ж�������ʼ����������1����2
    I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
}

// DS3231д�������ֻ����д�������
en_result_t I2C_DS3231_WriteCmd(M0P_I2C_TypeDef* I2CX,DS3231_CMD pu8Addr, uint8_t pu8Data)
{
		en_result_t enRet = Error;
		//��ʱ����ֵ
		uint32_t u32TimeOut = 0xFFFFFFu;
    uint8_t sendCount = 0, u8State = 0;
		I2C_SetFunc(I2CX,I2cStop_En);          ///����ֹͣ����
		I2C_ClearIrq(I2CX);
    I2C_SetFunc(I2CX,I2cStart_En);										 ///������ʼ����
		while(1)
		{

				while(0 == I2C_GetIrq(I2CX))
				{		
						//��ʱI2C��δ����
						if(0 == u32TimeOut--)
						{
								//������
								//NVIC_SystemReset();

								//��������������Ƶ��������ȡ�������û�����
								enRet = ErrorTimeout;
								return enRet;
						}
				}
				u8State = I2C_GetState(I2CX);
				switch(u8State)
				{
						case 0x08:                                 ///�ѷ�����ʼ����
								I2C_ClearFunc(I2CX, I2cStart_En);
								I2C_WriteByte(I2CX, DS3231_ADDR_WRITE);  ///�����豸��ַ+Wд��־0
								break;
						case 0x18:                                 ///�ѷ���SLW+W���ѽ���ACK
						case 0x28:                                 ///�ѷ���I2Cx_DATA�е����ݣ��ѽ���ACK
								switch(sendCount)
								{
										case 0:
												I2C_WriteByte(I2CX,pu8Addr);		 ///���͵�ַ
												break;
										case 1:
												I2C_WriteByte(I2CX,pu8Data);		 ///��������
												break;
								}
								sendCount++;
								break;
						case 0x20:                                 ///�ѷ���SLW+W���ѽ��շ�ACK
								break;
						case 0x30:                                 ///�ѷ���I2Cx_DATA�е����ݣ��ѽ��շ�ACK��������һ��STOP����
								I2C_SetFunc(I2CX,I2cStop_En);          ///����ֹͣ����
								break;
						case 0x58:                                 ///< �ѽ��յ����һ�����ݣ�NACK�ѷ���
                I2C_SetFunc(I2CX,I2cStop_En);          ///< ����ֹͣ����
                break;
            case 0x38:                                 ///< �ڷ��͵�ַ������ʱ���ٲö�ʧ
                I2C_SetFunc(I2CX,I2cStart_En);         ///< �����߿���ʱ������ʼ����
                break;
            case 0x48:                                 ///< ����SLA+R���յ�һ��NACK
                I2C_SetFunc(I2CX,I2cStop_En);          ///< ����ֹͣ����
                I2C_SetFunc(I2CX,I2cStart_En);         ///< ������ʼ����
                break;
						default:
								break;
				}            
				if(sendCount>2)
				{
						I2C_SetFunc(I2CX,I2cStop_En);              ///��˳���ܵ�������ֹͣ����
						I2C_ClearIrq(I2CX);
						break;
				}
				I2C_ClearIrq(I2CX);                            ///����ж�״̬��־λ
		}
		enRet = Ok;
    return enRet;
}
 
// ������ȡ���ݺ�����ֻ���ж����ݲ���
en_result_t I2C_MasterRead_DS3231Data(M0P_I2C_TypeDef* I2CX,DS3231_CMD pu8Addr,uint8_t *pu8Data,uint32_t u32Len)
{
		en_result_t enRet = Error;
		//��ʱ����ֵ
		uint32_t u32TimeOut = 0xFFFFFFu;
    uint8_t u8State=0;
    uint8_t receiveCount=0;
    uint8_t sendAddrCount=0;
		I2C_SetFunc(I2CX,I2cStop_En);          ///����ֹͣ����
		I2C_ClearIrq(I2CX);
    I2C_SetFunc(I2CX,I2cStart_En);										 ///������ʼ����

    while(1)
    {
        while(0 == I2C_GetIrq(I2CX))
        {		
						//��ʱI2C��δ����
						if(0 == u32TimeOut--)
						{
								//������
								//NVIC_SystemReset();

								//��������������Ƶ��������ȡ�������û�����
								enRet = ErrorTimeout;
								return enRet;
						}
				}
        u8State = I2C_GetState(I2CX);
        switch(u8State)
        {
            case 0x08:                                 ///< �ѷ�����ʼ������������SLA+W
								sendAddrCount++;
								I2C_ClearFunc(I2CX,I2cStart_En);
								I2C_WriteByte(I2CX,DS3231_ADDR_WRITE); 
                break;
            case 0x18:                                 ///< �ѷ���SLA+W,�����յ�ACK
                I2C_WriteByte(I2CX, pu8Addr); 					 ///< ��ȡ�Ĵ���λ��
                break;
            case 0x28:                                 ///< �ѷ������ݣ����յ�ACK, �˴����ѷ��ʹӻ��ڴ��ַu8Addr�����յ�ACK
								I2C_SetFunc(I2CX,I2cStart_En);
                break;
            case 0x10:                                 ///< �ѷ����ظ���ʼ����
                I2C_ClearFunc(I2CX,I2cStart_En);
								I2C_WriteByte(I2CX,DS3231_ADDR_READ);///< ����SLA+R����ʼ�Ӵӻ���ȡ����	
                break;
            case 0x40:                                 ///< �ѷ���SLA+R�������յ�ACK
                if(u32Len>=1)
                {
                    I2C_SetFunc(I2CX,I2cAck_En);       ///< ʹ������Ӧ����
                }
                break;
            case 0x50:                                 ///< �ѽ��������ֽڣ����ѷ���ACK�ź�
                pu8Data[receiveCount] = I2C_ReadByte(I2CX);
								receiveCount++;
                if(receiveCount==u32Len)
                {
                    I2C_ClearFunc(I2CX,I2cAck_En);     ///< �ѽ��յ������ڶ����ֽڣ��ر�ACKӦ����
                }
                break;
            case 0x58:                                 ///< �ѽ��յ����һ�����ݣ�NACK�ѷ���
								I2C_ClearFunc(I2CX,I2cStart_En);
                I2C_SetFunc(I2CX,I2cStop_En);          ///< ����ֹͣ����
								I2C_SetFunc(I2CX,I2cStart_En);
                break;
            case 0x38:                                 ///< �ڷ��͵�ַ������ʱ���ٲö�ʧ
                I2C_SetFunc(I2CX,I2cStart_En);         ///< �����߿���ʱ������ʼ����
                break;
            case 0x48:                                 ///< ����SLA+R���յ�һ��NACK
                I2C_SetFunc(I2CX,I2cStop_En);          ///< ����ֹͣ����
                I2C_SetFunc(I2CX,I2cStart_En);         ///< ������ʼ����
                break;
            default:
                I2C_SetFunc(I2CX,I2cStart_En);         ///< ��������״̬�����·�����ʼ����
                break;
        }
        I2C_ClearIrq(I2CX);                            ///< ����ж�״̬��־λ
        if(receiveCount==u32Len)                                ///< ����ȫ����ȡ��ɣ�����whileѭ��
        {
                break;
        }
    }
    enRet = Ok;
    return enRet;
}
 

// DS3231����ʱ������
void I2C_DS3231_SetTime(uint8_t year,uint8_t month,uint8_t day,uint8_t week,uint8_t hour,uint8_t minute,uint8_t second)
{
		uint8_t readData = 0x04;
		// DS3231æ��ȴ�
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

// DS3231��ȡʱ������
void I2C_DS3231_ReadTime()
{
		uint8_t readTimeList[7]= {0};
    
		uint8_t readData = 0x04;
		// DS3231æ��ȴ�
		while((readData&0x04) > 0)
		{
			I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_STATUS,&readData,1);
		}

		I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_SECOND,readTimeList,7);
		
		calendar.yearH  = 20;			//��ǧ��λ
		calendar.yearL  = BCD_DEC(readTimeList[6]);			//�� 
		calendar.month  = BCD_DEC(readTimeList[5]);			//�� 			
		calendar.date   = BCD_DEC(readTimeList[4]);		  //�� 
		calendar.hour   = BCD_DEC(readTimeList[2]);		  //ʱ 
		calendar.minute = BCD_DEC(readTimeList[1]);			//��
		calendar.second = BCD_DEC(readTimeList[0]);			//��
		calendar.week   = BCD_DEC(readTimeList[3]);		  //�� 
}	

//DS3231����Alarm_1
void I2C_DS3231_SetAlarm_1(boolean_t en,uint8_t date,uint8_t hour,uint8_t minute,uint8_t second)
{
		uint8_t readData = 0x04;
		// DS3231æ��ȴ�
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
				I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_CONTROL,readData);    	//�����������������ж�
		}
		else
		{
				I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_CONTROL,&readData,1);
				readData &= 0xFE;
				I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_CONTROL,readData);    	//�������Ž��������ж�
		}
}	

//DS3231����Alarm_2
void I2C_DS3231_SetAlarm_2(boolean_t en, uint8_t hour, uint8_t minute)
{
		uint8_t readData = 0x04;
		// DS3231æ��ȴ�
		while((readData&0x04) > 0)
		{
			I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_STATUS,&readData,1);
		}
		
		//��������ƥ��Ĵ���A2M4λΪ1,��ÿ�ս���ʱ��ƥ��
		I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_ALARM2DATE,0x80);
		//д��ʱ��ƥ������
		I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_ALARM2MINUTE,DEC_BCD(minute));
		I2C_DS3231_WriteCmd(M0P_I2C0, DS3231_ALARM2HOUR,DEC_BCD(hour));

		if(en)
		{
				I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_CONTROL,&readData,1);
				readData |= 0x02;
				I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_CONTROL,readData);    	//�����������������ж�
		}
		else
		{
				I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_CONTROL,&readData,1);
				readData &= 0xFD;
				I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_CONTROL,readData);    	//�������Ž��������ж�
		}
}	

// DS3231��ȡСʱ����
uint8_t I2C_DS3231_ReadTime_Hour()
{
		uint8_t readData = 0x04;
		// DS3231æ��ȴ�
		while((readData&0x04) > 0)
		{
			I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_STATUS,&readData,1);
		}

		I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_HOUR,&readData,1);
		
		return readData;
}	

//DS3231��ȡ��������
uint8_t I2C_DS3231_ReadTime_Minute()
{   
		uint8_t readData = 0x04;
		// DS3231æ��ȴ�
		while((readData&0x04) > 0)
		{
			I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_STATUS,&readData,1);
		}

		I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_MINUTE,&readData,1);
		
		return readData;
}	

// ��ȡ�¶���������
uint8_t I2C_DS3231_getTemperature(void)
{
		uint8_t readData = 0x04;
		// DS3231æ��ȴ�
		while((readData&0x04) > 0)
		{
			I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_STATUS,&readData,1);
		}

		I2C_MasterRead_DS3231Data(M0P_I2C0,DS3231_TEMPERATUREH,&readData,1);

		return readData;
}

// ���㹫�������ó�����
uint8_t GregorianDay(uint8_t year,uint8_t month,uint8_t date)
{
		int leapsToDate = 0;
		int day = 0;
		int MonthOffset[] = {0,31,59,90,120,151,181,212,243,273,304,334};

		if(year > 0)
		{
				/*�����2000�굽������ǰһ��֮��һ�������˶��ٸ�����*/
				leapsToDate = (year-1)/4 + 1; 
		}
		else
		{
				leapsToDate = 0;
		}
		     
	 
		/*������������һ��Ϊ���꣬�Ҽ������·���2��֮����������1�����򲻼�1*/
		if((year%4==0)&&(month>2))
		{
			day=1;
		} else {
			day=0;
		}
	 
		day += year*365 + leapsToDate + MonthOffset[month-1] + date; /*�����2000��Ԫ������������һ���ж�����*/
		day += 5;	/*1999�����һ���������� */
		return day%7; //�������
}
