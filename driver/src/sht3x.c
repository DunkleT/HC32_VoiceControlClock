#include "sht3x.h"
#include "gpio.h"
#include "i2c.h"
#include "pama.h"

extern MainConfig		g_tMainConfig;		// 系统配置

///< I2C端口配置
void I2C_Port_Init(void)
{
    stc_gpio_cfg_t stcGpioCfg;
    
    DDL_ZERO_STRUCT(stcGpioCfg);
    
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE);   //开启GPIO时钟门控 
    
    stcGpioCfg.enDir = GpioDirOut;                           ///< 端口方向配置->输出    
    stcGpioCfg.enOD = GpioOdEnable;                          ///< 开漏输出
    stcGpioCfg.enPu = GpioPuEnable;                          ///< 端口上拉配置->使能
    stcGpioCfg.enPd = GpioPdDisable;                         ///< 端口下拉配置->禁止
    stcGpioCfg.bOutputVal = TRUE;
    
    Gpio_Init(GpioPortB,GpioPin6,&stcGpioCfg);               ///< 端口初始化
    Gpio_Init(GpioPortB,GpioPin7,&stcGpioCfg);
    
    Gpio_SetAfMode(GpioPortB,GpioPin6,GpioAf1);              ///< 配置PB13为SCL
    Gpio_SetAfMode(GpioPortB,GpioPin7,GpioAf1);              ///< 配置PB14为SDA
}

///< I2C 模块配置
void I2C_Cfg_Init(void)
{
    stc_i2c_cfg_t stcI2cCfg;
    
    DDL_ZERO_STRUCT(stcI2cCfg);                            ///< 初始化结构体变量的值为0
    
    Sysctrl_SetPeripheralGate(SysctrlPeripheralI2c0,TRUE); ///< 开启I2C0时钟门控
    
    stcI2cCfg.u32Pclk = Sysctrl_GetPClkFreq();             ///< 获取PCLK时钟
    stcI2cCfg.u32Baud = 100000;                            ///< 100kHz
    stcI2cCfg.enMode = I2cMasterMode;                      ///< 主机模式
    stcI2cCfg.u8SlaveAddr = 0x55;                          ///< 从地址，主模式无效
    stcI2cCfg.bGc = FALSE;                                 ///< 广播地址应答使能关闭
    I2C_Init(M0P_I2C0,&stcI2cCfg);                         ///< 模块初始化
}

// CRC-8校验
uint8_t Crc_Check_8(uint8_t* const message, uint8_t initialValue)
{
    uint8_t  remainder;        //余数
    uint8_t  i = 0, j = 0;  //循环变量
 
    /* 初始化 */
    remainder = initialValue;
 
    for(j = 0; j < 2;j++)
    {
        remainder ^= message[j];
 
        /* 从最高位开始依次计算  */
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

    /* 返回计算的CRC码 */
    return remainder;
}

// SHT30写命令函数，只进行写命令操作
en_result_t I2C_SHT30_WriteCmd(M0P_I2C_TypeDef* I2CX,uint8_t *pu8Data)
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
                I2C_WriteByte(I2CX, SHT30_ADDR_WRITE);  ///发送设备地址+W写标志0
                break;
            case 0x18:                                 ///已发送SLW+W，已接收ACK
            case 0x28:                                 ///已发送I2Cx_DATA中的数据，已接收ACK
								if(sendCount<2)
								{
										I2C_WriteByte(I2CX,pu8Data[sendCount++]);		 ///发送数据
								}else
								{
										//发送两字节命令后收到ACK,退出
										sendCount++;
								}
                break;
            case 0x20:                                 ///已发送SLW+W，已接收非ACK
            case 0x38:                                 ///上一次在SLA+写数据时丢失仲裁
                I2C_SetFunc(I2CX,I2cStart_En);         ///当I2C总线空闲时发送起始条件
                break;
            case 0x30:                                 ///已发送I2Cx_DATA中的数据，已接收非ACK，将传输一个STOP条件
                I2C_SetFunc(I2CX,I2cStop_En);          ///发送停止条件
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
en_result_t I2C_MasterRead_SHT3xData(M0P_I2C_TypeDef* I2CX,uint16_t u8Cmd,uint8_t *pu8Data,uint32_t u32Len)
{
    en_result_t enRet = Error;
		//超时重启值
		uint32_t u32TimeOut = 0xFFFFFFu;
    uint8_t u8State=0;
    uint8_t receiveCount=0;
    uint8_t sendAddrCount=0;
	  uint8_t sendCmdCount=0;
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
								if(sendAddrCount<=1)
								{
										I2C_ClearFunc(I2CX,I2cStart_En);
										I2C_WriteByte(I2CX,SHT30_ADDR_WRITE); 
								}
								if(sendAddrCount>1)
								{
										I2C_ClearFunc(I2CX,I2cStart_En);
										I2C_WriteByte(I2CX,SHT30_ADDR_READ);///< 发送SLA+R，开始从从机读取数据						
								}
                break;
            case 0x18:                                 ///< 已发送SLA+W,并接收到ACK
                I2C_WriteByte(I2CX,(uint8_t)(u8Cmd>>8)); ///<命令高8位    
                break;
            case 0x28:                                 ///< 已发送数据，接收到ACK, 此处是已发送从机内存地址u8Addr并接收到ACK
                sendCmdCount++;
                I2C_WriteByte(I2CX,(uint8_t)u8Cmd);    ///<命令低8位
                if(sendCmdCount>1)
                    I2C_SetFunc(I2CX,I2cStart_En);     ///< 发送重复起始条件
                break;
            case 0x10:                                 ///< 已发送重复起始条件
                I2C_ClearFunc(I2CX,I2cStart_En);
                I2C_WriteByte(I2CX,SHT30_ADDR_WRITE);	 ///< 发送SLA+R，开始从从机读取数据
                break;
            case 0x40:                                 ///< 已发送SLA+R，并接收到ACK
                if(u32Len>1)
                {
                    I2C_SetFunc(I2CX,I2cAck_En);       ///< 使能主机应答功能
                }
                break;
            case 0x50:                                 ///< 已接收数据字节，并已返回ACK信号
                pu8Data[receiveCount++] = I2C_ReadByte(I2CX);
                if(receiveCount==u32Len-1)
                {
                    I2C_ClearFunc(I2CX,I2cAck_En);     ///< 已接收到倒数第二个字节，关闭ACK应答功能
                }
                break;
            case 0x58:                                 ///< 已接收到最后一个数据，NACK已返回
								if(receiveCount!=0)
								{
										pu8Data[receiveCount++] = I2C_ReadByte(I2CX);
								}
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

// 向SHT30发送一条指令(16bit)
uint8_t SHT30_Send_Cmd(SHT30_CMD cmd)
{
    uint8_t cmdBufferList[2];
    cmdBufferList[0] = cmd >> 8;
    cmdBufferList[1] = cmd;
    return I2C_SHT30_WriteCmd(M0P_I2C0, cmdBufferList);
}

// 复位SHT30
void SHT30_Reset(void)
{
    SHT30_Send_Cmd(SOFT_RESET_CMD);
    delay1us(20);
}

// 初始化SHT30 周期测量模式0x2220
uint8_t SHT30_Init(void)
{
    return SHT30_Send_Cmd(MEDIUM_2_CMD);
}

// 从SHT30读取一次数据
uint8_t SHT30_Read_Data(uint8_t* data)
{
		//准备读取，开启系统定时中断
		SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

    return I2C_MasterRead_SHT3xData(M0P_I2C0,READOUT_FOR_PERIODIC_MODE, data, 6);
}

// 将SHT30接收的6个字节数据进行CRC校验，并转换为温度值和湿度值
uint8_t SHT30_Data_To_Float(uint8_t* const data, float* temperature, float* humidity)
{
    uint16_t recv_temperature = 0;
    uint16_t recv_humidity = 0;
 
    /* 校验温度数据和湿度数据是否接收正确 */
    if(Crc_Check_8(data, 0xFF) != data[2] || Crc_Check_8(&data[3], 0xFF) != data[5])
        return 1;
 
    /* 转换温度数据 */
    recv_temperature = ((uint16_t)data[0]<<8)|data[1];
    *temperature = -45 + 175*((float)recv_temperature/65535);
 
    /* 转换湿度数据 */
    recv_humidity = ((uint16_t)data[3]<<8)|data[4];
    *humidity = 100 * ((float)recv_humidity / 65535);
 
		*temperature = *temperature +g_tMainConfig.offsetTemperature;
		*humidity = *humidity +g_tMainConfig.offsetHumidity;

		//读取完毕，关闭系统定时中断
		SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

    return 0;
}

