#include "sht3x.h"
#include "gpio.h"
#include "i2c.h"
#include "pama.h"

extern MainConfig		g_tMainConfig;		// ϵͳ����

///< I2C�˿�����
void I2C_Port_Init(void)
{
    stc_gpio_cfg_t stcGpioCfg;
    
    DDL_ZERO_STRUCT(stcGpioCfg);
    
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE);   //����GPIOʱ���ſ� 
    
    stcGpioCfg.enDir = GpioDirOut;                           ///< �˿ڷ�������->���    
    stcGpioCfg.enOD = GpioOdEnable;                          ///< ��©���
    stcGpioCfg.enPu = GpioPuEnable;                          ///< �˿���������->ʹ��
    stcGpioCfg.enPd = GpioPdDisable;                         ///< �˿���������->��ֹ
    stcGpioCfg.bOutputVal = TRUE;
    
    Gpio_Init(GpioPortB,GpioPin6,&stcGpioCfg);               ///< �˿ڳ�ʼ��
    Gpio_Init(GpioPortB,GpioPin7,&stcGpioCfg);
    
    Gpio_SetAfMode(GpioPortB,GpioPin6,GpioAf1);              ///< ����PB13ΪSCL
    Gpio_SetAfMode(GpioPortB,GpioPin7,GpioAf1);              ///< ����PB14ΪSDA
}

///< I2C ģ������
void I2C_Cfg_Init(void)
{
    stc_i2c_cfg_t stcI2cCfg;
    
    DDL_ZERO_STRUCT(stcI2cCfg);                            ///< ��ʼ���ṹ�������ֵΪ0
    
    Sysctrl_SetPeripheralGate(SysctrlPeripheralI2c0,TRUE); ///< ����I2C0ʱ���ſ�
    
    stcI2cCfg.u32Pclk = Sysctrl_GetPClkFreq();             ///< ��ȡPCLKʱ��
    stcI2cCfg.u32Baud = 100000;                            ///< 100kHz
    stcI2cCfg.enMode = I2cMasterMode;                      ///< ����ģʽ
    stcI2cCfg.u8SlaveAddr = 0x55;                          ///< �ӵ�ַ����ģʽ��Ч
    stcI2cCfg.bGc = FALSE;                                 ///< �㲥��ַӦ��ʹ�ܹر�
    I2C_Init(M0P_I2C0,&stcI2cCfg);                         ///< ģ���ʼ��
}

// CRC-8У��
uint8_t Crc_Check_8(uint8_t* const message, uint8_t initialValue)
{
    uint8_t  remainder;        //����
    uint8_t  i = 0, j = 0;  //ѭ������
 
    /* ��ʼ�� */
    remainder = initialValue;
 
    for(j = 0; j < 2;j++)
    {
        remainder ^= message[j];
 
        /* �����λ��ʼ���μ���  */
        for (i = 0; i < 8; i++)
        {
            if (remainder & 0x80)
            {
                remainder = (remainder << 1)^CRC8_POLYNOMIAL;
            }
            else
            {
                remainder = (remainder << 1);
            }
        }
    }

    /* ���ؼ����CRC�� */
    return remainder;
}

// SHT30д�������ֻ����д�������
en_result_t I2C_SHT30_WriteCmd(M0P_I2C_TypeDef* I2CX,uint8_t *pu8Data)
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
                I2C_WriteByte(I2CX, SHT30_ADDR_WRITE);  ///�����豸��ַ+Wд��־0
                break;
            case 0x18:                                 ///�ѷ���SLW+W���ѽ���ACK
            case 0x28:                                 ///�ѷ���I2Cx_DATA�е����ݣ��ѽ���ACK
								if(sendCount<2)
								{
										I2C_WriteByte(I2CX,pu8Data[sendCount++]);		 ///��������
								}else
								{
										//�������ֽ�������յ�ACK,�˳�
										sendCount++;
								}
                break;
            case 0x20:                                 ///�ѷ���SLW+W���ѽ��շ�ACK
            case 0x38:                                 ///��һ����SLA+д����ʱ��ʧ�ٲ�
                I2C_SetFunc(I2CX,I2cStart_En);         ///��I2C���߿���ʱ������ʼ����
                break;
            case 0x30:                                 ///�ѷ���I2Cx_DATA�е����ݣ��ѽ��շ�ACK��������һ��STOP����
                I2C_SetFunc(I2CX,I2cStop_En);          ///����ֹͣ����
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
en_result_t I2C_MasterRead_SHT3xData(M0P_I2C_TypeDef* I2CX,uint16_t u8Cmd,uint8_t *pu8Data,uint32_t u32Len)
{
    en_result_t enRet = Error;
		//��ʱ����ֵ
		uint32_t u32TimeOut = 0xFFFFFFu;
    uint8_t u8State=0;
    uint8_t receiveCount=0;
    uint8_t sendAddrCount=0;
	  uint8_t sendCmdCount=0;
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
								if(sendAddrCount<=1)
								{
										I2C_ClearFunc(I2CX,I2cStart_En);
										I2C_WriteByte(I2CX,SHT30_ADDR_WRITE); 
								}
								if(sendAddrCount>1)
								{
										I2C_ClearFunc(I2CX,I2cStart_En);
										I2C_WriteByte(I2CX,SHT30_ADDR_READ);///< ����SLA+R����ʼ�Ӵӻ���ȡ����						
								}
                break;
            case 0x18:                                 ///< �ѷ���SLA+W,�����յ�ACK
                I2C_WriteByte(I2CX,(uint8_t)(u8Cmd>>8)); ///<�����8λ    
                break;
            case 0x28:                                 ///< �ѷ������ݣ����յ�ACK, �˴����ѷ��ʹӻ��ڴ��ַu8Addr�����յ�ACK
                sendCmdCount++;
                I2C_WriteByte(I2CX,(uint8_t)u8Cmd);    ///<�����8λ
                if(sendCmdCount>1)
                    I2C_SetFunc(I2CX,I2cStart_En);     ///< �����ظ���ʼ����
                break;
            case 0x10:                                 ///< �ѷ����ظ���ʼ����
                I2C_ClearFunc(I2CX,I2cStart_En);
                I2C_WriteByte(I2CX,SHT30_ADDR_WRITE);	 ///< ����SLA+R����ʼ�Ӵӻ���ȡ����
                break;
            case 0x40:                                 ///< �ѷ���SLA+R�������յ�ACK
                if(u32Len>1)
                {
                    I2C_SetFunc(I2CX,I2cAck_En);       ///< ʹ������Ӧ����
                }
                break;
            case 0x50:                                 ///< �ѽ��������ֽڣ����ѷ���ACK�ź�
                pu8Data[receiveCount++] = I2C_ReadByte(I2CX);
                if(receiveCount==u32Len-1)
                {
                    I2C_ClearFunc(I2CX,I2cAck_En);     ///< �ѽ��յ������ڶ����ֽڣ��ر�ACKӦ����
                }
                break;
            case 0x58:                                 ///< �ѽ��յ����һ�����ݣ�NACK�ѷ���
								if(receiveCount!=0)
								{
										pu8Data[receiveCount++] = I2C_ReadByte(I2CX);
								}
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

// ��SHT30����һ��ָ��(16bit)
uint8_t SHT30_Send_Cmd(SHT30_CMD cmd)
{
    uint8_t cmdBufferList[2];
    cmdBufferList[0] = cmd >> 8;
    cmdBufferList[1] = cmd;
    return I2C_SHT30_WriteCmd(M0P_I2C0, cmdBufferList);
}

// ��λSHT30
void SHT30_Reset(void)
{
    SHT30_Send_Cmd(SOFT_RESET_CMD);
    delay1us(20);
}

// ��ʼ��SHT30 ���ڲ���ģʽ0x2220
uint8_t SHT30_Init(void)
{
    return SHT30_Send_Cmd(MEDIUM_2_CMD);
}

// ��SHT30��ȡһ������
uint8_t SHT30_Read_Data(uint8_t* data)
{
		//׼����ȡ������ϵͳ��ʱ�ж�
		SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

    return I2C_MasterRead_SHT3xData(M0P_I2C0,READOUT_FOR_PERIODIC_MODE, data, 6);
}

// ��SHT30���յ�6���ֽ����ݽ���CRCУ�飬��ת��Ϊ�¶�ֵ��ʪ��ֵ
uint8_t SHT30_Data_To_Float(uint8_t* const data, float* temperature, float* humidity)
{
    uint16_t recv_temperature = 0;
    uint16_t recv_humidity = 0;
 
    /* У���¶����ݺ�ʪ�������Ƿ������ȷ */
    if(Crc_Check_8(data, 0xFF) != data[2] || Crc_Check_8(&data[3], 0xFF) != data[5])
        return 1;
 
    /* ת���¶����� */
    recv_temperature = ((uint16_t)data[0]<<8)|data[1];
    *temperature = -45 + 175*((float)recv_temperature/65535);
 
    /* ת��ʪ������ */
    recv_humidity = ((uint16_t)data[3]<<8)|data[4];
    *humidity = 100 * ((float)recv_humidity / 65535);
 
		*temperature = *temperature +g_tMainConfig.offsetTemperature;
		*humidity = *humidity +g_tMainConfig.offsetHumidity;

		//��ȡ��ϣ��ر�ϵͳ��ʱ�ж�
		SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

    return 0;
}

