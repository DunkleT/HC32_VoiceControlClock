#include "tm1652.h"
#include "bsp_delay.h"

#define TM1652_SDA_PIN_LOW 		Gpio_ClrIO(TM1652_SDA_PORT,TM1652_SDA_PIN);
#define TM1652_SDA_PIN_HIGH 	Gpio_SetIO(TM1652_SDA_PORT,TM1652_SDA_PIN);

//TM1652显示控制调节命令
//位驱动占空比8/16，段驱动电流2/8，显示模式8段5位输出
uint8_t tm1652_show_mode=0x18;

//TM1652显示亮度设置
void Tm1652_Brightness_set(uint8_t level)
{
		switch(level)
		{
				case 1:
					tm1652_show_mode=0x88;
					break;
				case 2:
					tm1652_show_mode=0x28;
					break;
				case 3:
					tm1652_show_mode=0x18;
					break;
				case 4:
					tm1652_show_mode=0x1C;
					break;
				case 5:
					tm1652_show_mode=0x5C;
					break;
				case 6:
					tm1652_show_mode=0x3C;
					break;
				case 7:
					tm1652_show_mode=0x3E;
					break;
				case 8:
					tm1652_show_mode=0xFE;
					break;
		}
}

//TM1652GPIO初始化
void Tm1652_Init(void) 
{
		stc_gpio_cfg_t 	GpioInitStruct;
		DDL_ZERO_STRUCT(GpioInitStruct);

		///<使能GPIO外设时钟门控开关
    Sysctrl_SetPeripheralGate(SysctrlPeripheralGpio,TRUE);

		///<配置SDA引脚输出模式
    GpioInitStruct.enDir = GpioDirOut;
		GpioInitStruct.enDrv = GpioDrvL;//低驱动
		///<配置SDA引脚上拉
		GpioInitStruct.enPu = GpioPuEnable;
		GpioInitStruct.enPd = GpioPdDisable;

    Gpio_Init(TM1652_SDA_PORT,TM1652_SDA_PIN,&GpioInitStruct); 

		Tm1652_Show_Close();
}

//向TM1652发送1byte数据
void Tm1652_Send(uint8_t data)
{
		uint8_t nBit=0;
		uint8_t fParity =0;
		
		TM1652_SDA_PIN_LOW
		delay_us(52);

		for(nBit=0; nBit<8; nBit++)
		{
				if(data&0x01)
				{
						fParity ++;
						TM1652_SDA_PIN_HIGH
				}
				else
				{
						TM1652_SDA_PIN_LOW
				}
				delay_us(52);
				data>>=1;
		}

		if(fParity%2==0)
		{
				TM1652_SDA_PIN_HIGH
		}
		else
		{
				TM1652_SDA_PIN_LOW
		}
		        
		delay_us(52);
		TM1652_SDA_PIN_HIGH        
		delay_us(52);
		delay_us(52);
}

//数码管显示字符对应显示数据转换
uint8_t Tm1652_Transform(uint8_t showChar) 
{
		uint8_t data = 0xff;
		switch(showChar)
		{
				case 0:		data = 0x3F;	break;	// 0
				case 1:		data = 0x06;	break;	// 1
				case 2:		data = 0x5B;	break;	// 2
				case 3:		data = 0x4F;	break;	// 3
				case 4:		data = 0x66;	break;	// 4
				case 5:		data = 0x6D;	break;	// 5
				case 6:		data = 0x7D;	break;	// 6
				case 7:		data = 0x07;	break;	// 7
				case 8:		data = 0x7F;	break;	// 8
				case 9:		data = 0x6F;	break;	// 9

				case '0':		data = 0x3F;	break;	// 0
				case '1':		data = 0x06;	break;	// 1
				case '2':		data = 0x5B;	break;	// 2
				case '3':		data = 0x4F;	break;	// 3
				case '4':		data = 0x66;	break;	// 4
				case '5':		data = 0x6D;	break;	// 5
				case '6':		data = 0x7D;	break;	// 6
				case '7':		data = 0x07;	break;	// 7
				case '8':		data = 0x7F;	break;	// 8
				case '9':		data = 0x6F;	break;	// 9
				case 'A':		data = 0x77;	break;	// A
				case 'b':		data = 0x7C;	break;	// B
				case 'C':		data = 0x39;	break;	// C
				case 'd':		data = 0x5E;	break;	// D
				case 'E':		data = 0x79;	break;	// E
				case 'F':		data = 0x71;	break;	// F
				case 'H':		data = 0x76;	break;	// H
				case 'J':		data = 0x1E;	break;	// J
				case 'L':		data = 0x38;	break;	// L
				case 'O':		data = 0x3F;	break;	// O
				case 'P':		data = 0x73;	break;	// P
				case 'R':		data = 0x77;	break;	// R
				case 'S':		data = 0x6D;	break;	// S
				case 'T':		data = 0x31;	break;	// T
				case 'U':		data = 0x3E;	break;	// U
				case '-':   data = 0x40;	break;	// -
				case ' ':   data = 0x00;	break;	// 
				case ':':   data = 0xFF;	break;	// :
				case 'u':   data = 0x1C;	break;	// u
				case 'n':   data = 0x54;	break;	// n
				case 'K':   data = 0xF0;	break;	// k
				case 'k':   data = 0x70;	break;	// k
				case 'o':   data = 0x5C;	break;	// o
				case 'c':   data = 0x58;	break;	// c
		}
		return data;
};

//TM1652 5位数码管显示字符更新函数
void Tm1652_Show_Updata(uint8_t char1, uint8_t char2, uint8_t char3, uint8_t char4, uint8_t char5)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(char1));
		Tm1652_Send(Tm1652_Transform(char2));
		Tm1652_Send(Tm1652_Transform(char3));
		Tm1652_Send(Tm1652_Transform(char4));
		Tm1652_Send(Tm1652_Transform(char5));
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}

//TM1652 5位数码管显示字符更新函数
void Tm1652_Show_Number(uint16_t number)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(number/1000));
		Tm1652_Send(Tm1652_Transform(number%1000/100));
		Tm1652_Send(Tm1652_Transform(number%100/10));
		Tm1652_Send(Tm1652_Transform(number%10));
		Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}

//TM1652 正常显示时间 22：35
void Tm1652_Time_Show(uint8_t hour, uint8_t minute, uint8_t showColon)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(hour/10));
		Tm1652_Send(Tm1652_Transform(hour%10));
		Tm1652_Send(Tm1652_Transform(minute/10));
		Tm1652_Send(Tm1652_Transform(minute%10));
		if(showColon)
				Tm1652_Send(0xff);
		else
				Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}

//TM1652 显示设定年份 20xx
void Tm1652_Time_Show_Year(uint8_t year)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(2));
		Tm1652_Send(Tm1652_Transform(0));
		Tm1652_Send(Tm1652_Transform(year/10));
		Tm1652_Send(Tm1652_Transform(year%10));
		Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}

//TM1652 显示设定月份	_-11
void Tm1652_Time_Show_Month(uint8_t month)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(' '));
		Tm1652_Send(Tm1652_Transform('-'));
		Tm1652_Send(Tm1652_Transform(month/10));
		Tm1652_Send(Tm1652_Transform(month%10));
		Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}

//TM1652 显示设定日期	_-07
void Tm1652_Time_Show_Day(uint8_t day)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(' '));
		Tm1652_Send(Tm1652_Transform('-'));
		Tm1652_Send(Tm1652_Transform(day/10));
		Tm1652_Send(Tm1652_Transform(day%10));
		Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}

//TM1652 显示设定小时	22：__/22___
void Tm1652_Time_Show_Hour(uint8_t hour, uint8_t showColon)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(hour/10));
		Tm1652_Send(Tm1652_Transform(hour%10));
		Tm1652_Send(Tm1652_Transform(' '));
		Tm1652_Send(Tm1652_Transform(' '));
		if(showColon)
				Tm1652_Send(0xff);
		else
				Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}

//TM1652 显示设定分钟	22：__/22___
void Tm1652_Time_Show_Minute(uint8_t minute, uint8_t showColon)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(' '));
		Tm1652_Send(Tm1652_Transform(' '));
		Tm1652_Send(Tm1652_Transform(minute/10));
		Tm1652_Send(Tm1652_Transform(minute%10));
		if(showColon)
				Tm1652_Send(0xff);
		else
				Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}

//TM1652 显示设定小时和分钟十位	22：3_/22_3_
void Tm1652_Time_Show_Hour_HMinute(uint8_t hour, uint8_t minute, uint8_t showColon)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(hour/10));
		Tm1652_Send(Tm1652_Transform(hour%10));
		Tm1652_Send(Tm1652_Transform(minute/10));
		Tm1652_Send(Tm1652_Transform(' '));
		if(showColon)
				Tm1652_Send(0xff);
		else
				Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}
//TM1652 显示设定小时和分钟个位	22：_5/22__5
void Tm1652_Time_Show_Hour_LMinute(uint8_t hour, uint8_t minute, uint8_t showColon)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(hour/10));
		Tm1652_Send(Tm1652_Transform(hour%10));
		Tm1652_Send(Tm1652_Transform(' '));
		Tm1652_Send(Tm1652_Transform(minute%10));
		if(showColon)
				Tm1652_Send(0xff);
		else
				Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}
//TM1652 显示设定分钟和小时十位	2_：35/22_35
void Tm1652_Time_Show_HHour_Minute(uint8_t hour, uint8_t minute, uint8_t showColon)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(hour/10));
		Tm1652_Send(Tm1652_Transform(' '));
		Tm1652_Send(Tm1652_Transform(minute/10));
		Tm1652_Send(Tm1652_Transform(minute%10));
		if(showColon)
				Tm1652_Send(0xff);
		else
				Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}
//TM1652 显示设定分钟和小时个位	_2：35/_2_35
void Tm1652_Time_Show_LHour_Minute(uint8_t hour, uint8_t minute, uint8_t showColon)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(' '));
		Tm1652_Send(Tm1652_Transform(hour%10));
		Tm1652_Send(Tm1652_Transform(minute/10));
		Tm1652_Send(Tm1652_Transform(minute%10));
		if(showColon)
				Tm1652_Send(0xff);
		else
				Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}


//TM1652 显示设定秒钟 __：32/___32
void Tm1652_Time_Show_Second(uint8_t second, uint8_t showColon)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(' '));
		Tm1652_Send(Tm1652_Transform(' '));
		Tm1652_Send(Tm1652_Transform(second/10));
		Tm1652_Send(Tm1652_Transform(second%10));
		if(showColon)
				Tm1652_Send(0xff);
		else
				Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}

//TM1652 显示冒号 __:__
void Tm1652_Time_Show_Colon(uint8_t showColon)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(' '));
		Tm1652_Send(Tm1652_Transform(' '));
		Tm1652_Send(Tm1652_Transform(' '));
		Tm1652_Send(Tm1652_Transform(' '));
		if(showColon)
				Tm1652_Send(0xff);
		else
				Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}

//TM1652 温度显示
void Tm1652_Temperature_Show(float temperature)
{
		//显示帧数据
		Tm1652_Send(0x08);

		//大于零显示
		//大于100显示
		if(temperature >= 100)
		{
				Tm1652_Send(Tm1652_Transform((int)temperature/100));
				
				//不显示小数部分，四舍五入
				if((temperature-(int)temperature)*10 > 4)
				{
						if((int)temperature%10+1 == 10)
						{
								Tm1652_Send(Tm1652_Transform((int)temperature/10%10+1));
						}
						else
						{
								Tm1652_Send(Tm1652_Transform((int)temperature/10%10));
						}
						Tm1652_Send(Tm1652_Transform(((int)temperature%10+1)%10));
				}
				else
				{
						Tm1652_Send(Tm1652_Transform((int)temperature/10%10));
						Tm1652_Send(Tm1652_Transform((int)temperature%10));
				}
		}
		else
		{
				//大于10显示
				if(temperature >= 10)
				{
						Tm1652_Send(Tm1652_Transform(temperature/10));
						Tm1652_Send(Tm1652_Transform((int)temperature%10)+0x80);
						Tm1652_Send(Tm1652_Transform((temperature-(int)temperature)*10/1));
				}
				else
				{
						//小于10显示
						if(temperature < 10 && temperature >= 0)
						{
								Tm1652_Send(Tm1652_Transform(' '));
								Tm1652_Send(Tm1652_Transform((int)temperature%10)+0x80);
								Tm1652_Send(Tm1652_Transform((temperature-(int)temperature)*10/1));
						}
						else		//小于零显示
						{
								//大于-10显示
								if(temperature > -10 && temperature < 0)
								{
										temperature = -temperature;
										Tm1652_Send(Tm1652_Transform('-'));
										Tm1652_Send(Tm1652_Transform((int)temperature%10)+0x80);
										Tm1652_Send(Tm1652_Transform((temperature-(int)temperature)*10/1));
								}
								else
								{
										//小于-10显示
										if(temperature <= -10)
										{
												temperature = -temperature;
												Tm1652_Send(Tm1652_Transform('-'));
												
												//不显示小数部分，四舍五入
												if((temperature-(int)temperature)*10 > 4)
												{
														if((int)temperature%10+1 == 10)
														{
																Tm1652_Send(Tm1652_Transform((int)temperature/10+1));
														}
														else
														{
																Tm1652_Send(Tm1652_Transform((int)temperature/10));
														}
														Tm1652_Send(Tm1652_Transform(((int)temperature%10+1)%10));
												}
												else
												{
														Tm1652_Send(Tm1652_Transform((int)temperature/10));
														Tm1652_Send(Tm1652_Transform((int)temperature%10));
												}
												
										}
								}
								
						}
				}
		}
		
		Tm1652_Send(Tm1652_Transform('C'));
		Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}

//TM1652 湿度显示
void Tm1652_Humidity_Show(float humidity)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(Tm1652_Transform(' '));
		
		//不显示小数部分，四舍五入
		if((humidity-(int)humidity)*10 > 4)
		{
				if((int)humidity%10+1 == 10)
				{
						Tm1652_Send(Tm1652_Transform((int)humidity/10+1));
				}
				else
				{
						Tm1652_Send(Tm1652_Transform((int)humidity/10));
				}
				Tm1652_Send(Tm1652_Transform(((int)humidity%10+1)%10));
		}
		else
		{
				Tm1652_Send(Tm1652_Transform((int)humidity/10));
				Tm1652_Send(Tm1652_Transform((int)humidity%10));
		}
		Tm1652_Send(Tm1652_Transform('H'));
		Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}

//TM1652 关闭数码管显示
void Tm1652_Show_Close(void)
{
		//显示帧数据
		Tm1652_Send(0x08);
		Tm1652_Send(0x00);
		Tm1652_Send(0x00);
		Tm1652_Send(0x00);
		Tm1652_Send(0x00);
		Tm1652_Send(0x00);
		//命令间隔
		delay_ms(3);
		//控制帧数据
		Tm1652_Send(0x18);
		Tm1652_Send(tm1652_show_mode);
		//命令间隔
		delay_ms(5);
}

