#include "pama.h"
#include "bsp_flash.h"
#include "bsp_gpio.h"
#include "tm1652.h"
#include "ds3231.h"
#include "gpio.h"

DevicePama 		config;				  // 设备配置
DeviceState		state;					// 设备状态
MainConfig		g_tMainConfig;	// 系统配置
//读取的当前时间
extern _calendar_obj calendar;	//日历结构体

uint8_t buf[8];

//系统参数重置
bool MainConfigReset(void)
{
    config.pConfig = &g_tMainConfig;
		//保存的最近时间	用于时间初始化
		config.pConfig->tTime.time_yearH 	= 20;		//年 		
		config.pConfig->tTime.time_yearL 	= 24;		//年 
		config.pConfig->tTime.time_month 	= 10;		//月
		config.pConfig->tTime.time_date  	= 23;		//日 
		config.pConfig->tTime.time_hour 	= 16;		//时 
		config.pConfig->tTime.time_minute = 18;		//分
		config.pConfig->tTime.time_second = 20;		//秒
		config.pConfig->tTime.time_week  	= 3;		//周

		config.pConfig->tCtrl.alarm_duration_1.duration_hour 		= 0;		//定时1
		config.pConfig->tCtrl.alarm_duration_1.duration_minute	= 30;		//定时1
		config.pConfig->tCtrl.alarm_duration_2.duration_hour 		= 2;		//定时2
		config.pConfig->tCtrl.alarm_duration_2.duration_minute	= 0;		//定时2
		
		config.pConfig->tCtrl.alarm_duration_enable_1 = false;		//定时1使能
		config.pConfig->tCtrl.alarm_duration_enable_2 = false;		//定时2使能

		config.pConfig->tCtrl.alarm_moment_1.moment_hour 		= 8;		//闹钟1小时
		config.pConfig->tCtrl.alarm_moment_1.moment_minute	= 0;		//闹钟1分钟
		config.pConfig->tCtrl.alarm_moment_2.moment_hour 		= 8;		//闹钟2小时
		config.pConfig->tCtrl.alarm_moment_2.moment_minute	= 20;		//闹钟2分钟
		config.pConfig->tCtrl.alarm_moment_3.moment_hour 		= 22;		//闹钟3小时
		config.pConfig->tCtrl.alarm_moment_3.moment_minute	= 30;		//闹钟3分钟
		
		config.pConfig->tCtrl.alarm_moment_enable_1 = false;		//闹钟1使能
		config.pConfig->tCtrl.alarm_moment_enable_2 = false;		//闹钟2使能
		config.pConfig->tCtrl.alarm_moment_enable_3 = false;		//闹钟3使能

		config.pConfig->offsetTemperature = 0;			//温度的修正值，因为温度有1位小数，所以温度修正存储值默认*10
		config.pConfig->offsetHumidity = 0;					//湿度的修正值

		config.pConfig->displayLightSet = 5;						//显示亮度设置
		config.pConfig->ctrlModeSet = CTRL_MODE_ERES;		//控制模式设置

		config.pConfig->dispalyMode = false;			//true常亮模式，默认关闭false
		config.pConfig->timeMode = false;					//默认24小时模式false   12小时模式true

		config.pConfig->nDevVerSoft = DEV_INF_VER_SOFT;		//软件版本
		config.pConfig->nDevVerHard = DEV_INF_VER_HARD;		//硬件版本

    return true;
}

//参数配置保存地址是否正确判断
uint8_t MainConfigGetCurrent(void)
{
    uint8_t buf[8];

    flash_read_data(PAMA_INFO_START_ADDR, (uint16_t *)&buf[0], 8);

    if(cal_crc16(buf, 6) != (uint16_t) buf[6] * 0x100 + buf[7])
        return 0xFF;
    if(buf[0] != 0xA5 || buf[1] != 0x5A || buf[5] != 0xA5 || buf[4] != 0x5A
            || buf[2] > 0x01 || buf[3] != 0xFF - buf[2])
        return 0xFF;

    return buf[2];
}

//参数配置保存地址存储
bool MainConfigSetCurAddr(uint8_t addr)
{
    uint16_t crc;

    buf[0] = 0xA5;
    buf[1] = 0x5A;
    buf[2] = addr;
    buf[3] = 0xFF - addr;
    buf[4] = 0x5A;
    buf[5] = 0xA5;

    crc = cal_crc16(buf, 6);
    buf[6] = crc / 0x100;
    buf[7] = crc % 0x100;
		
    flash_erase_sector(PAMA_INFO_START_ADDR);
    flash_write_data(PAMA_INFO_START_ADDR, (uint16_t *)&buf[0], 8);
		
    flash_read_data(PAMA_INFO_START_ADDR, (uint16_t *)&buf[0], 8);
    crc = cal_crc16(buf, 6);

    if(addr != MainConfigGetCurrent())
        return false;

    return true;
}
//保存参数配置
bool MainConfigSave(void)
{
    uint8_t cur_addr = MainConfigGetCurrent();
    uint8_t save_addr = 0;
    uint16_t crc = 0;
    uint8_t buf[DEV_CONFIG_DATA_LEN];
    bool ret = false;
    int8_t retry = 3;

    if(cur_addr == 0)
        save_addr = 1;
    else save_addr = 0;

    while(ret == false && (retry --) >= 0)
    {
        if(save_addr == 0)
        {
            flash_erase_sector(PAMA_ZONE_0_START_ADDR);
            flash_write_data(PAMA_ZONE_0_START_ADDR, (uint16_t *)&g_tMainConfig, DEV_CONFIG_DATA_LEN);
            crc = cal_crc16((uint8_t *)&g_tMainConfig, DEV_CONFIG_DATA_LEN);
            flash_write_data(PAMA_ZONE_0_START_ADDR + DEV_CONFIG_DATA_LEN, &crc, 2);
            flash_read_data(PAMA_ZONE_1_START_ADDR, (uint16_t *)buf, DEV_CONFIG_DATA_LEN);
        }
        else
        {
            flash_erase_sector(PAMA_ZONE_1_START_ADDR);
            flash_write_data(PAMA_ZONE_1_START_ADDR, (uint16_t *)&g_tMainConfig, DEV_CONFIG_DATA_LEN);
            crc = cal_crc16((uint8_t *)&g_tMainConfig, DEV_CONFIG_DATA_LEN);
            flash_write_data(PAMA_ZONE_1_START_ADDR + DEV_CONFIG_DATA_LEN, &crc, 2);
            flash_read_data(PAMA_ZONE_1_START_ADDR, (uint16_t *)buf, DEV_CONFIG_DATA_LEN);
        }
        if(MainConfigSetCurAddr(save_addr) == false || cal_crc16(buf, DEV_CONFIG_DATA_LEN) != crc)
            ret = false;
        ret = true;
    }

    return ret;
}
//加载参数配置
bool MainConfigLoad(void)
{
    uint8_t cur_addr = MainConfigGetCurrent();
    uint16_t crc = 0;
    uint8_t buf[DEV_CONFIG_DATA_LEN];

    if(cur_addr > 1)
    {
        flash_read_data(PAMA_ZONE_0_START_ADDR, (uint16_t *)buf, DEV_CONFIG_DATA_LEN);
        flash_read_data(PAMA_ZONE_0_START_ADDR + DEV_CONFIG_DATA_LEN, &crc, 2);
        if(cal_crc16(buf, DEV_CONFIG_DATA_LEN) != crc)
        {
            flash_read_data(PAMA_ZONE_1_START_ADDR, (uint16_t *)buf, DEV_CONFIG_DATA_LEN);
            flash_read_data(PAMA_ZONE_1_START_ADDR + DEV_CONFIG_DATA_LEN, &crc, 2);
            if(cal_crc16(buf, DEV_CONFIG_DATA_LEN) != crc)
                return false;
            else cur_addr = 1;
        }
        else cur_addr = 0;

        MainConfigSetCurAddr(cur_addr);
    }
    if(cur_addr > 1) return false;

    if(cur_addr == 0)
    {
        flash_read_data(PAMA_ZONE_0_START_ADDR, (uint16_t *)buf, DEV_CONFIG_DATA_LEN);
        flash_read_data(PAMA_ZONE_0_START_ADDR + DEV_CONFIG_DATA_LEN, &crc, 2);
        if(cal_crc16(buf, DEV_CONFIG_DATA_LEN) != crc)
        {
            flash_read_data(PAMA_ZONE_1_START_ADDR, (uint16_t *)buf, DEV_CONFIG_DATA_LEN);
            flash_read_data(PAMA_ZONE_1_START_ADDR + DEV_CONFIG_DATA_LEN, &crc, 2);
            if(cal_crc16(buf, DEV_CONFIG_DATA_LEN) != crc)
                return false;
            else
            {
                MainConfigSetCurAddr(1);
                flash_read_data(PAMA_ZONE_1_START_ADDR, (uint16_t *)&g_tMainConfig, DEV_CONFIG_DATA_LEN);
            }
        }
        else flash_read_data(PAMA_ZONE_0_START_ADDR, (uint16_t *)&g_tMainConfig, DEV_CONFIG_DATA_LEN);
    }
    else
    {
        flash_read_data(PAMA_ZONE_1_START_ADDR, (uint16_t *)buf, DEV_CONFIG_DATA_LEN);
        flash_read_data(PAMA_ZONE_1_START_ADDR + DEV_CONFIG_DATA_LEN, &crc, 2);
        if(cal_crc16(buf, DEV_CONFIG_DATA_LEN) != crc)
        {
            flash_read_data(PAMA_ZONE_0_START_ADDR, (uint16_t *)buf, DEV_CONFIG_DATA_LEN);
            flash_read_data(PAMA_ZONE_0_START_ADDR + DEV_CONFIG_DATA_LEN, &crc, 2);
            if(cal_crc16(buf, DEV_CONFIG_DATA_LEN) != crc)
                return false;
            else
            {
                MainConfigSetCurAddr(0);
                flash_read_data(PAMA_ZONE_0_START_ADDR, (uint16_t *)&g_tMainConfig, DEV_CONFIG_DATA_LEN);
            }
        }
        else flash_read_data(PAMA_ZONE_1_START_ADDR, (uint16_t *)&g_tMainConfig, DEV_CONFIG_DATA_LEN);
    }

    return true;
}

//系统参数配置正确修正
bool MainConfigCorrect(void)
{
    config.pConfig = &g_tMainConfig;
		uint8_t temp;
		float32_t tempOffset;

		temp = config.pConfig->tCtrl.alarm_duration_1.duration_hour;
		if(temp>99)
		{
				config.pConfig->tCtrl.alarm_duration_1.duration_hour = 0;		//定时1小时
		}
		temp = config.pConfig->tCtrl.alarm_duration_1.duration_minute;
		if(temp>59)
		{
				config.pConfig->tCtrl.alarm_duration_1.duration_minute	= 30;		//定时1分钟
		}
		temp = config.pConfig->tCtrl.alarm_duration_2.duration_hour;
		if(temp>99)
		{
				config.pConfig->tCtrl.alarm_duration_2.duration_hour 		= 1;		//定时2小时
		}
		temp = config.pConfig->tCtrl.alarm_duration_2.duration_minute;
		if(temp>59)
		{
				config.pConfig->tCtrl.alarm_duration_2.duration_minute	= 0;		//定时2分钟
		}

		temp = config.pConfig->tCtrl.alarm_moment_1.moment_hour;
		if(temp>23)
		{
				config.pConfig->tCtrl.alarm_moment_1.moment_hour 		= 8;		//闹钟1小时
		}
		temp = config.pConfig->tCtrl.alarm_moment_1.moment_minute;
		if(temp>59)
		{
				config.pConfig->tCtrl.alarm_moment_1.moment_minute	= 0;		//闹钟1分钟
		}
		temp = config.pConfig->tCtrl.alarm_moment_2.moment_hour;
		if(temp>23)
		{
				config.pConfig->tCtrl.alarm_moment_2.moment_hour 		= 12;		//闹钟2小时
		}
		temp = config.pConfig->tCtrl.alarm_moment_2.moment_minute;
		if(temp>59)
		{
				config.pConfig->tCtrl.alarm_moment_2.moment_minute	= 0;		//闹钟2分钟
		}
		temp = config.pConfig->tCtrl.alarm_duration_2.duration_hour;
		if(temp>23)
		{
				config.pConfig->tCtrl.alarm_moment_3.moment_hour 		= 22;		//闹钟3小时
		}
		temp = config.pConfig->tCtrl.alarm_duration_2.duration_minute;
		if(temp>59)
		{
				config.pConfig->tCtrl.alarm_moment_3.moment_minute	= 0;		//闹钟3分钟
		}

		tempOffset = config.pConfig->offsetTemperature;
		if(tempOffset>165||tempOffset<-165)
		{
			config.pConfig->offsetTemperature = 0;			//温度的修正值，因为温度有1位小数，所以温度修正存储值默认*10
		}
		tempOffset = config.pConfig->offsetHumidity;
		if(tempOffset>100||tempOffset<-100)
		{
			config.pConfig->offsetHumidity = 0;			//湿度的修正值
		}

		if(config.pConfig->displayLightSet>8)
		{
				config.pConfig->displayLightSet = 4;		//显示亮度设置
		}
		if(config.pConfig->ctrlModeSet>3)
		{
				config.pConfig->ctrlModeSet = 0;				//控制模式设置
		}

    return true;
}

//系统启动加载参数
bool MainConfigLoadAtBoot(void)
{
		bool duration1NotOutline = false;
		bool duration2NotOutline = false;

		uint8_t tempAlarmDate = 0;
		uint8_t tempAlarmHour = 0;
		uint8_t tempAlarmMinute = 0;
		uint8_t tempAlarmSecond = 0;
		

    config.pConfig = &g_tMainConfig;

    if(MainConfigLoad() == false)
    {
        MainConfigReset();
        if(MainConfigSave() == false)
        {
            config.correct = false;
            return false;
        }
        if(MainConfigLoad() == false)
        {
            config.correct = false;
            return false;
        }
    }

    MainConfigCorrect();
    config.correct = true;
		
		I2C_DS3231_ReadTime();
		//定时1过期判断
		if(config.pConfig->tCtrl.alarm_duration_enable_1)
		{
				//判断结束时间是否过期
				if(JudgeAlarmOutline(1))
				{
						//如果过期了，则关闭定时开启标志
						config.pConfig->tCtrl.alarm_duration_enable_1 = false;
				}
				else
				{
						//如果没有过期，则设定标志
						duration1NotOutline = true;
				}
		}
		//定时2过期判断
		if(config.pConfig->tCtrl.alarm_duration_enable_2)
		{
				//判断结束时间是否过期
				if(JudgeAlarmOutline(2))
				{
						//如果过期了，则关闭定时开启标志
						config.pConfig->tCtrl.alarm_duration_enable_2 = false;
				}
				else
				{
						//如果没有过期，则设定标志
						duration2NotOutline = true;
				}
		}
		//定时开启
		if(duration1NotOutline)
		{
				if(duration2NotOutline)
				{
						//两个定时都没有过期，则比较结束时刻靠前的写入
						switch(JudgeDurationFirstArrive())
						{
								case 0:
										//没有定时开启
										//不会出现该情况
										break;
								case 1:
										//定时1开始计时
										tempAlarmDate = g_tMainConfig.tCtrl.alarm_duration_1.alarm_date;
										tempAlarmHour = g_tMainConfig.tCtrl.alarm_duration_1.alarm_hour;
										tempAlarmMinute = g_tMainConfig.tCtrl.alarm_duration_1.alarm_minute;
										tempAlarmSecond = g_tMainConfig.tCtrl.alarm_duration_1.alarm_second;
										I2C_DS3231_SetAlarm_1(true,tempAlarmDate,tempAlarmHour,tempAlarmMinute,tempAlarmSecond);
										break;
								case 2:
										//定时2开始计时
										tempAlarmDate = g_tMainConfig.tCtrl.alarm_duration_2.alarm_date;
										tempAlarmHour = g_tMainConfig.tCtrl.alarm_duration_2.alarm_hour;
										tempAlarmMinute = g_tMainConfig.tCtrl.alarm_duration_2.alarm_minute;
										tempAlarmSecond = g_tMainConfig.tCtrl.alarm_duration_2.alarm_second;
										I2C_DS3231_SetAlarm_1(true,tempAlarmDate,tempAlarmHour,tempAlarmMinute,tempAlarmSecond);
										break;
						}
				}
				else
				{
						//定时1没有过期，定时2过期，则写入定时1
						tempAlarmDate = g_tMainConfig.tCtrl.alarm_duration_1.alarm_date;
						tempAlarmHour = g_tMainConfig.tCtrl.alarm_duration_1.alarm_hour;
						tempAlarmMinute = g_tMainConfig.tCtrl.alarm_duration_1.alarm_minute;
						tempAlarmSecond = g_tMainConfig.tCtrl.alarm_duration_1.alarm_second;
						I2C_DS3231_SetAlarm_1(true,tempAlarmDate,tempAlarmHour,tempAlarmMinute,tempAlarmSecond);
				}
		}
		else
		{
				if(duration2NotOutline)
				{
						//定时1过期，定时2没有过期，则写入定时2
						tempAlarmDate = g_tMainConfig.tCtrl.alarm_duration_2.alarm_date;
						tempAlarmHour = g_tMainConfig.tCtrl.alarm_duration_2.alarm_hour;
						tempAlarmMinute = g_tMainConfig.tCtrl.alarm_duration_2.alarm_minute;
						tempAlarmSecond = g_tMainConfig.tCtrl.alarm_duration_2.alarm_second;
						I2C_DS3231_SetAlarm_1(true,tempAlarmDate,tempAlarmHour,tempAlarmMinute,tempAlarmSecond);
				}
		}

		//闹钟开启
		switch(JudgeAlarmFirstArrive())
		{
				case 0:
						//没有闹钟开启，这里关一下DS3231 Alarm2
						I2C_DS3231_SetAlarm_2(false, 1, 1);
						I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ输出禁止，闹钟标志位清零
						break;
				case 1:
						//直接写入1的报警时刻（不需要判断闹钟，因为闹钟时间不变，和定时不一样）
						I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_1.moment_hour, g_tMainConfig.tCtrl.alarm_moment_1.moment_minute);
						break;
				case 2:
						//直接写入2的报警时刻（不需要判断闹钟，因为闹钟时间不变，和定时不一样）
						I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_2.moment_hour, g_tMainConfig.tCtrl.alarm_moment_2.moment_minute);
						break;
				case 3:
						//直接写入3的报警时刻（不需要判断闹钟，因为闹钟时间不变，和定时不一样）
						I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_3.moment_hour, g_tMainConfig.tCtrl.alarm_moment_3.moment_minute);
						break;
		}

		//红外感应使能判断
		if(config.pConfig->ctrlModeSet==0||config.pConfig->ctrlModeSet==1)
		{
				//高电平打开
				Gpio_SetIO(GpioPortA,GpioPin10);
		}
		else
		{
				Gpio_ClrIO(GpioPortA,GpioPin10);
		}
		//语音播报使能判断
		if(config.pConfig->ctrlModeSet==1||config.pConfig->ctrlModeSet==3)
		{
				//高电平关闭
				Gpio_SetIO(GpioPortB,GpioPin15);
		}
		else
		{
				Gpio_ClrIO(GpioPortB,GpioPin15);
		}
		//显示亮度初始化
		Tm1652_Brightness_set(config.pConfig->displayLightSet);
    return true;
}

//判断那个定时先到
uint8_t JudgeDurationFirstArrive(void)
{
		if(g_tMainConfig.tCtrl.alarm_duration_enable_1 && g_tMainConfig.tCtrl.alarm_duration_enable_2)
		{
				if(g_tMainConfig.tCtrl.alarm_duration_1.alarm_date < g_tMainConfig.tCtrl.alarm_duration_2.alarm_date)
				{
						return 1;
				}
				else if(g_tMainConfig.tCtrl.alarm_duration_1.alarm_date == g_tMainConfig.tCtrl.alarm_duration_2.alarm_date)
				{
						if(g_tMainConfig.tCtrl.alarm_duration_1.alarm_hour < g_tMainConfig.tCtrl.alarm_duration_2.alarm_hour)
						{
								return 1;
						}
						else if(g_tMainConfig.tCtrl.alarm_duration_1.alarm_hour == g_tMainConfig.tCtrl.alarm_duration_2.alarm_hour)
						{
								if(g_tMainConfig.tCtrl.alarm_duration_1.alarm_minute < g_tMainConfig.tCtrl.alarm_duration_2.alarm_minute)
								{
										return 1;
								}
								else if(g_tMainConfig.tCtrl.alarm_duration_1.alarm_minute == g_tMainConfig.tCtrl.alarm_duration_2.alarm_minute)
								{
										if(g_tMainConfig.tCtrl.alarm_duration_1.alarm_second < g_tMainConfig.tCtrl.alarm_duration_2.alarm_second)
										{
												return 1;
										}
								}
						}
				}
				return 2;
		}
		else
		{
				if(g_tMainConfig.tCtrl.alarm_duration_enable_1)
				{
						return 1;
				}
				else if(g_tMainConfig.tCtrl.alarm_duration_enable_2)
				{
						return 2;
				}
				else
				{
						return 0;
				}
		}
}


//判断那个闹钟先到
uint8_t JudgeAlarmFirstArrive(void)
{
		uint16_t nowMoment = 0;
	
		uint16_t alarmMoment_1 = 0;
		uint16_t alarmMoment_2 = 0;
		uint16_t alarmMoment_3 = 0;

		bool tempAlarmEnable_1 = g_tMainConfig.tCtrl.alarm_moment_enable_1;
		bool tempAlarmEnable_2 = g_tMainConfig.tCtrl.alarm_moment_enable_2;
		bool tempAlarmEnable_3 = g_tMainConfig.tCtrl.alarm_moment_enable_3;
		
		alarmMoment_1 = g_tMainConfig.tCtrl.alarm_moment_1.moment_hour*60;
		alarmMoment_1 += g_tMainConfig.tCtrl.alarm_moment_1.moment_minute;

		alarmMoment_2 = g_tMainConfig.tCtrl.alarm_moment_2.moment_hour*60;
		alarmMoment_2 += g_tMainConfig.tCtrl.alarm_moment_2.moment_minute;

		alarmMoment_3 = g_tMainConfig.tCtrl.alarm_moment_3.moment_hour*60;
		alarmMoment_3 += g_tMainConfig.tCtrl.alarm_moment_3.moment_minute;
		
		//处理，离当前时间先到
		I2C_DS3231_ReadTime();
		nowMoment = calendar.hour*60 + calendar.minute;				//读取当前时刻
		if(alarmMoment_1 < nowMoment)
		{
				//下次报警时长
				alarmMoment_1 = 24*60 - nowMoment + alarmMoment_1;
		}
		else
		{
				alarmMoment_1 -= nowMoment;
		}
		if(alarmMoment_2 < nowMoment)
		{
				alarmMoment_2 = 24*60 - nowMoment + alarmMoment_2;
		}
		else
		{
				alarmMoment_2 -= nowMoment;
		}
		if(alarmMoment_3 < nowMoment)
		{
				alarmMoment_3 = 24*60 - nowMoment + alarmMoment_3;
		}
		else
		{
				alarmMoment_3 -= nowMoment;
		}
		//同时则暂时取消使能
		if(alarmMoment_1 == 0)
		{
				tempAlarmEnable_1 = false;
		}
		if(alarmMoment_2 == 0)
		{
				tempAlarmEnable_2 = false;
		}
		if(alarmMoment_3 == 0)
		{
				tempAlarmEnable_3 = false;
		}

		if(tempAlarmEnable_1)
		{
				if(tempAlarmEnable_2)
				{
						if(tempAlarmEnable_3)
						{
								if(alarmMoment_1 == alarmMoment_2)
								{
										return 0;
								}
								if(alarmMoment_1 == alarmMoment_3)
								{
										return 0;
								}
								if(alarmMoment_2 == alarmMoment_3)
								{
										return 0;
								}
								
								if(alarmMoment_1 < alarmMoment_2)
								{
										if(alarmMoment_1 < alarmMoment_3)
										{
												return 1;
										}
										else
										{
												return 3;
										}
								}
								else
								{
										if(alarmMoment_2 < alarmMoment_3)
										{
												return 2;
										}
										else
										{
												return 3;
										}
								}
						}
						else
						{
								if(alarmMoment_1 == alarmMoment_2)
								{
										return 0;
								}
								if(alarmMoment_1 < alarmMoment_2)
								{
										return 1;
								}
								else
								{
										return 2;
								}
						}
				}
				else
				{
						if(tempAlarmEnable_3)
						{
								if(alarmMoment_1 == alarmMoment_3)
								{
										return 0;
								}
								if(alarmMoment_1 < alarmMoment_3)
								{
										return 1;
								}
								else
								{
										return 3;
								}
						}
						else
						{
								return 1;
						}
				}
		}
		else
		{
				if(tempAlarmEnable_2)
				{
						if(tempAlarmEnable_3)
						{
								if(alarmMoment_2 < alarmMoment_3)
								{
										return 2;
								}
								else
								{
										return 3;
								}
						}
						else
						{
								return 2;
						}
				}
				else
				{
						if(tempAlarmEnable_3)
						{
								return 3;
						}
						else
						{
								//没有闹钟开启
								return 0;
						}
				}
		}
}

//判断定时是否过期
bool JudgeAlarmOutline(uint8_t durationIndex)
{
		uint8_t tempAlarmYearL;
		uint8_t tempAlarmMonth;
		uint8_t tempAlarmDate;
		uint8_t tempAlarmHour;
		uint8_t tempAlarmMinute;
		uint8_t tempAlarmSecond;

		switch(durationIndex)
		{
				case 1:
						tempAlarmYearL = config.pConfig->tCtrl.alarm_duration_1.alarm_yearL;
						tempAlarmMonth = config.pConfig->tCtrl.alarm_duration_1.alarm_month;
						tempAlarmDate = config.pConfig->tCtrl.alarm_duration_1.alarm_date;
						tempAlarmHour = config.pConfig->tCtrl.alarm_duration_1.alarm_hour;
						tempAlarmMinute = config.pConfig->tCtrl.alarm_duration_1.alarm_minute;
						tempAlarmSecond = config.pConfig->tCtrl.alarm_duration_1.alarm_second;
						break;
				case 2:
						tempAlarmYearL = config.pConfig->tCtrl.alarm_duration_2.alarm_yearL;
						tempAlarmMonth = config.pConfig->tCtrl.alarm_duration_2.alarm_month;
						tempAlarmDate = config.pConfig->tCtrl.alarm_duration_2.alarm_date;
						tempAlarmHour = config.pConfig->tCtrl.alarm_duration_2.alarm_hour;
						tempAlarmMinute = config.pConfig->tCtrl.alarm_duration_2.alarm_minute;
						tempAlarmSecond = config.pConfig->tCtrl.alarm_duration_2.alarm_second;
						break;
		}

		I2C_DS3231_ReadTime();
		if(calendar.yearL < tempAlarmYearL)
		{
				return false;
		}
		else
		{
				if(calendar.yearL == tempAlarmYearL)
				{
						if(calendar.month < tempAlarmMonth)
						{
								return false;
						}
						else
						{
								if(calendar.month == tempAlarmMonth)
								{
										if(calendar.date < tempAlarmDate)
										{
												return false;
										}
										else
										{
												if(calendar.date == tempAlarmDate)
												{
														if(calendar.hour < tempAlarmHour)
														{
																return false;
														}
														else
														{
																if(calendar.hour == tempAlarmHour)
																{
																		if(calendar.minute < tempAlarmMinute)
																		{
																				return false;
																		}
																		else
																		{
																				if(calendar.minute == tempAlarmMinute)
																				{		
																						//上个报警结束后相距小于5秒的进行忽略
																						if(calendar.second + 5 < tempAlarmSecond)
																						{
																								return false;
																						}
																						else
																						{
																								return true;
																						}
																				}
																				else
																				{
																						return true;
																				}
																		}
																}
																else
																{
																		return true;
																}
														}
												}
												else
												{
														return true;
												}
										}
								}
						}
				}
		}
		//不可能触发，防报错
		return true;
}

//判断定时剩余时长是否大于1分钟，并赋值剩余时长，如果剩余时长大于1分钟秒钟写入0，否则写入剩余秒钟。
bool GetDurationRemainTime(uint8_t durationIndex, uint8_t* remainHour, uint8_t* remainMinute, uint8_t* remainSecond)
{
		uint8_t tempAlarmYearL;
		uint8_t tempAlarmMonth;
		uint8_t tempAlarmDate;
		uint8_t tempAlarmHour;
		uint8_t tempAlarmMinute;
		uint8_t tempAlarmSecond;

		uint8_t dateCount;
		uint16_t minuteCount;

		switch(durationIndex)
		{
				case 1:
						tempAlarmYearL = config.pConfig->tCtrl.alarm_duration_1.alarm_yearL;
						tempAlarmMonth = config.pConfig->tCtrl.alarm_duration_1.alarm_month;
						tempAlarmDate = config.pConfig->tCtrl.alarm_duration_1.alarm_date;
						tempAlarmHour = config.pConfig->tCtrl.alarm_duration_1.alarm_hour;
						tempAlarmMinute = config.pConfig->tCtrl.alarm_duration_1.alarm_minute;
						tempAlarmSecond = config.pConfig->tCtrl.alarm_duration_1.alarm_second;
						break;
				case 2:
						tempAlarmYearL = config.pConfig->tCtrl.alarm_duration_2.alarm_yearL;
						tempAlarmMonth = config.pConfig->tCtrl.alarm_duration_2.alarm_month;
						tempAlarmDate = config.pConfig->tCtrl.alarm_duration_2.alarm_date;
						tempAlarmHour = config.pConfig->tCtrl.alarm_duration_2.alarm_hour;
						tempAlarmMinute = config.pConfig->tCtrl.alarm_duration_2.alarm_minute;
						tempAlarmSecond = config.pConfig->tCtrl.alarm_duration_2.alarm_second;
						break;
		}

		I2C_DS3231_ReadTime();
		if(calendar.yearL < tempAlarmYearL)
		{
				//跨年只能是12月份所以是31,日历当天补满，结束日期当天不算
				dateCount = (31 - calendar.date) + tempAlarmDate -1;
				minuteCount = ((dateCount*24)+(24-calendar.hour-1)+tempAlarmHour)*60+(60-calendar.minute)+tempAlarmMinute;
				//用分钟进行计算，秒钟没到则不动，秒钟到了则减1
				if(calendar.second > tempAlarmSecond)
				{
						minuteCount -= 1;
				}
				*remainHour = minuteCount/60;
				*remainMinute = minuteCount%60;
				*remainSecond = 0;
				return true;
		}
		else
		{
				if(calendar.yearL == tempAlarmYearL)
				{
						if(calendar.month < tempAlarmMonth)
						{
								//跨年只能是12月份所以是31,日历当天补满，结束日期当天不算
								switch(calendar.month)
								{
										case 1:
										case 3:
										case 5:
										case 7:
										case 8:
										case 10:
										case 12:
												dateCount = (31 - calendar.date) + tempAlarmDate -1;
												break;
										case 2:
												if(calendar.yearL%4 == 0)
												{
														dateCount = (29 - calendar.date) + tempAlarmDate -1;
												}
												else
												{
														dateCount = (28 - calendar.date) + tempAlarmDate -1;
												}
												break;
										case 4:
										case 6:
										case 9:
										case 11:
												dateCount = (30 - calendar.date) + tempAlarmDate -1;
												break;
								}
								minuteCount = ((dateCount*24)+(24-calendar.hour-1)+tempAlarmHour)*60+(60-calendar.minute)+tempAlarmMinute;
								//用分钟进行计算，秒钟没到则不动，秒钟到了则减1
								if(calendar.second > tempAlarmSecond)
								{
										minuteCount -= 1;
								}
								*remainHour = minuteCount/60;
								*remainMinute = minuteCount%60;
								*remainSecond = 0;
								return true;
						}
						else
						{
								if(calendar.month == tempAlarmMonth)
								{
										if(calendar.date < tempAlarmDate)
										{
												dateCount = tempAlarmDate - calendar.date -1;
												minuteCount = ((dateCount*24)+(24-calendar.hour-1)+tempAlarmHour)*60+(60-calendar.minute)+tempAlarmMinute;
												//用分钟进行计算，秒钟没到则不动，秒钟到了则减1
												if(calendar.second > tempAlarmSecond)
												{
														minuteCount -= 1;
												}
												*remainHour = minuteCount/60;
												*remainMinute = minuteCount%60;
												*remainSecond = 0;
												return true;
										}
										else
										{
												if(calendar.date == tempAlarmDate)
												{
														if(calendar.hour < tempAlarmHour)
														{
																minuteCount = (tempAlarmHour-calendar.hour-1)*60+(60-calendar.minute)+tempAlarmMinute;
																//用分钟进行计算，秒钟没到则不动，秒钟到了则减1
																if(calendar.second > tempAlarmSecond)
																{
																		minuteCount -= 1;
																}
																*remainHour = minuteCount/60;
																*remainMinute = minuteCount%60;
																*remainSecond = 0;
																return true;
														}
														else
														{
																if(calendar.hour == tempAlarmHour)
																{
																		if(calendar.minute + 1 < tempAlarmMinute)
																		{
																				minuteCount = tempAlarmMinute-calendar.minute;
																				//用分钟进行计算，秒钟没到则不动，秒钟到了则减1
																				if(calendar.second > tempAlarmSecond)
																				{
																						minuteCount -= 1;
																				}
																				*remainHour = minuteCount/60;
																				*remainMinute = minuteCount%60;
																				*remainSecond = 0;
																				return true;
																		}
																		else
																		{
																				//相差不足1分钟
																				if(calendar.minute == tempAlarmMinute)
																				{		
																						*remainHour = 0;
																						*remainMinute = 0;
																						*remainSecond = tempAlarmSecond-calendar.second;
																						return false;
																				}
																				else
																				{
																						if(calendar.second<=tempAlarmSecond)
																						{
																								*remainHour = 0;
																								*remainMinute = 1;
																								*remainSecond = tempAlarmSecond-calendar.second;
																								return true;
																						}
																						else
																						{
																								*remainHour = 0;
																								*remainMinute = 0;
																								*remainSecond = (tempAlarmSecond+60)-calendar.second;
																								return false;
																						}
																						
																				}
																		}
																}
														}
												}
										}
								}
						}
				}
		}
		//不可能触发，防报错
		return false;
}

//开启后续闹钟
void OpenMomentNextArrive(uint8_t momentIndex)
{
		uint16_t nowMoment = 0;

		uint8_t tempMomentHour_1 =0;
		uint8_t tempMomentMinute_1 = 0;

		uint8_t tempMomentHour_2 =0;
		uint8_t tempMomentMinute_2 = 0;

		uint16_t tempAlarmMoment_1 = 0;
		uint16_t tempAlarmMoment_2 = 0;

		bool momentEnable_1 = false;
		bool momentEnable_2 = false;

		switch(momentIndex)
		{
				case 1:
						tempMomentHour_1 = g_tMainConfig.tCtrl.alarm_moment_2.moment_hour;
						tempMomentMinute_1 = g_tMainConfig.tCtrl.alarm_moment_2.moment_minute;

						tempAlarmMoment_1 = tempMomentHour_1*60 + tempMomentMinute_1;

						tempMomentHour_2 = g_tMainConfig.tCtrl.alarm_moment_3.moment_hour;
						tempMomentMinute_2 = g_tMainConfig.tCtrl.alarm_moment_3.moment_minute;

						tempAlarmMoment_2 = tempMomentHour_2*60 + tempMomentMinute_2;

						momentEnable_1 = g_tMainConfig.tCtrl.alarm_moment_enable_2;
						momentEnable_2 = g_tMainConfig.tCtrl.alarm_moment_enable_3;
						break;
				case 2:
						tempMomentHour_1 = g_tMainConfig.tCtrl.alarm_moment_1.moment_hour;
						tempMomentMinute_1 = g_tMainConfig.tCtrl.alarm_moment_1.moment_minute;

						tempAlarmMoment_1 = tempMomentHour_1*60 + tempMomentMinute_1;

						tempMomentHour_2 = g_tMainConfig.tCtrl.alarm_moment_3.moment_hour;
						tempMomentMinute_2 = g_tMainConfig.tCtrl.alarm_moment_3.moment_minute;

						tempAlarmMoment_2 = tempMomentHour_2*60 + tempMomentMinute_2;

						momentEnable_1 = g_tMainConfig.tCtrl.alarm_moment_enable_1;
						momentEnable_2 = g_tMainConfig.tCtrl.alarm_moment_enable_3;
						break;
				case 3:
						tempMomentHour_1 = g_tMainConfig.tCtrl.alarm_moment_1.moment_hour;
						tempMomentMinute_1 = g_tMainConfig.tCtrl.alarm_moment_1.moment_minute;

						tempAlarmMoment_1 = tempMomentHour_1*60 + tempMomentMinute_1;

						tempMomentHour_2 = g_tMainConfig.tCtrl.alarm_moment_2.moment_hour;
						tempMomentMinute_2 = g_tMainConfig.tCtrl.alarm_moment_2.moment_minute;
						
						tempAlarmMoment_2 = tempMomentHour_2*60 + tempMomentMinute_2;
						
						momentEnable_1 = g_tMainConfig.tCtrl.alarm_moment_enable_1;
						momentEnable_2 = g_tMainConfig.tCtrl.alarm_moment_enable_2;
						break;
		}
		//查看是否有后续闹钟
		//临时闹钟1开启
		if(momentEnable_1)
		{
				//临时闹钟2开启
				if(momentEnable_2)
				{
						//处理，离当前时间先到
						I2C_DS3231_ReadTime();
						nowMoment = calendar.hour*60 + calendar.minute;				//读取当前时刻
						if(tempAlarmMoment_1 < nowMoment)
						{
								//下次报警时长
								tempAlarmMoment_1 = 24*60 - nowMoment + tempAlarmMoment_1;
						}
						else
						{
								tempAlarmMoment_1 -= nowMoment;
						}
						if(tempAlarmMoment_2 < nowMoment)
						{
								tempAlarmMoment_2 = 24*60 - nowMoment + tempAlarmMoment_2;
						}
						else
						{
								tempAlarmMoment_2 -= nowMoment;
						}
						//临时闹钟2开启
						if(tempAlarmMoment_1 < tempAlarmMoment_2)
						{
								//写入临时闹钟1
								I2C_DS3231_SetAlarm_2(true, tempMomentHour_1, tempMomentMinute_1);
						}
						else
						{
								//写入临时闹钟2
								I2C_DS3231_SetAlarm_2(true, tempMomentHour_2, tempMomentMinute_2);
						}
				}
				else
				{
						//写入临时闹钟1
						I2C_DS3231_SetAlarm_2(true, tempMomentHour_1, tempMomentMinute_1);
				}
		}
		else
		{
				//临时闹钟2开启
				if(momentEnable_2)
				{
						//写入临时闹钟2
						I2C_DS3231_SetAlarm_2(true, tempMomentHour_2, tempMomentMinute_2);
				}
				else
				{
						//没有	其他闹钟开启
				}
		}
}
