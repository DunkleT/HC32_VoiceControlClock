#include "pama.h"
#include "bsp_flash.h"
#include "bsp_gpio.h"
#include "tm1652.h"
#include "ds3231.h"
#include "gpio.h"

DevicePama 		config;				  // �豸����
DeviceState		state;					// �豸״̬
MainConfig		g_tMainConfig;	// ϵͳ����
//��ȡ�ĵ�ǰʱ��
extern _calendar_obj calendar;	//�����ṹ��

uint8_t buf[8];

//ϵͳ��������
bool MainConfigReset(void)
{
    config.pConfig = &g_tMainConfig;
		//��������ʱ��	����ʱ���ʼ��
		config.pConfig->tTime.time_yearH 	= 20;		//�� 		
		config.pConfig->tTime.time_yearL 	= 24;		//�� 
		config.pConfig->tTime.time_month 	= 10;		//��
		config.pConfig->tTime.time_date  	= 23;		//�� 
		config.pConfig->tTime.time_hour 	= 16;		//ʱ 
		config.pConfig->tTime.time_minute = 18;		//��
		config.pConfig->tTime.time_second = 20;		//��
		config.pConfig->tTime.time_week  	= 3;		//��

		config.pConfig->tCtrl.alarm_duration_1.duration_hour 		= 0;		//��ʱ1
		config.pConfig->tCtrl.alarm_duration_1.duration_minute	= 30;		//��ʱ1
		config.pConfig->tCtrl.alarm_duration_2.duration_hour 		= 2;		//��ʱ2
		config.pConfig->tCtrl.alarm_duration_2.duration_minute	= 0;		//��ʱ2
		
		config.pConfig->tCtrl.alarm_duration_enable_1 = false;		//��ʱ1ʹ��
		config.pConfig->tCtrl.alarm_duration_enable_2 = false;		//��ʱ2ʹ��

		config.pConfig->tCtrl.alarm_moment_1.moment_hour 		= 8;		//����1Сʱ
		config.pConfig->tCtrl.alarm_moment_1.moment_minute	= 0;		//����1����
		config.pConfig->tCtrl.alarm_moment_2.moment_hour 		= 8;		//����2Сʱ
		config.pConfig->tCtrl.alarm_moment_2.moment_minute	= 20;		//����2����
		config.pConfig->tCtrl.alarm_moment_3.moment_hour 		= 22;		//����3Сʱ
		config.pConfig->tCtrl.alarm_moment_3.moment_minute	= 30;		//����3����
		
		config.pConfig->tCtrl.alarm_moment_enable_1 = false;		//����1ʹ��
		config.pConfig->tCtrl.alarm_moment_enable_2 = false;		//����2ʹ��
		config.pConfig->tCtrl.alarm_moment_enable_3 = false;		//����3ʹ��

		config.pConfig->offsetTemperature = 0;			//�¶ȵ�����ֵ����Ϊ�¶���1λС���������¶������洢ֵĬ��*10
		config.pConfig->offsetHumidity = 0;					//ʪ�ȵ�����ֵ

		config.pConfig->displayLightSet = 5;						//��ʾ��������
		config.pConfig->ctrlModeSet = CTRL_MODE_ERES;		//����ģʽ����

		config.pConfig->dispalyMode = false;			//true����ģʽ��Ĭ�Ϲر�false
		config.pConfig->timeMode = false;					//Ĭ��24Сʱģʽfalse   12Сʱģʽtrue

		config.pConfig->nDevVerSoft = DEV_INF_VER_SOFT;		//����汾
		config.pConfig->nDevVerHard = DEV_INF_VER_HARD;		//Ӳ���汾

    return true;
}

//�������ñ����ַ�Ƿ���ȷ�ж�
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

//�������ñ����ַ�洢
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
//�����������
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
//���ز�������
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

//ϵͳ����������ȷ����
bool MainConfigCorrect(void)
{
    config.pConfig = &g_tMainConfig;
		uint8_t temp;
		float32_t tempOffset;

		temp = config.pConfig->tCtrl.alarm_duration_1.duration_hour;
		if(temp>99)
		{
				config.pConfig->tCtrl.alarm_duration_1.duration_hour = 0;		//��ʱ1Сʱ
		}
		temp = config.pConfig->tCtrl.alarm_duration_1.duration_minute;
		if(temp>59)
		{
				config.pConfig->tCtrl.alarm_duration_1.duration_minute	= 30;		//��ʱ1����
		}
		temp = config.pConfig->tCtrl.alarm_duration_2.duration_hour;
		if(temp>99)
		{
				config.pConfig->tCtrl.alarm_duration_2.duration_hour 		= 1;		//��ʱ2Сʱ
		}
		temp = config.pConfig->tCtrl.alarm_duration_2.duration_minute;
		if(temp>59)
		{
				config.pConfig->tCtrl.alarm_duration_2.duration_minute	= 0;		//��ʱ2����
		}

		temp = config.pConfig->tCtrl.alarm_moment_1.moment_hour;
		if(temp>23)
		{
				config.pConfig->tCtrl.alarm_moment_1.moment_hour 		= 8;		//����1Сʱ
		}
		temp = config.pConfig->tCtrl.alarm_moment_1.moment_minute;
		if(temp>59)
		{
				config.pConfig->tCtrl.alarm_moment_1.moment_minute	= 0;		//����1����
		}
		temp = config.pConfig->tCtrl.alarm_moment_2.moment_hour;
		if(temp>23)
		{
				config.pConfig->tCtrl.alarm_moment_2.moment_hour 		= 12;		//����2Сʱ
		}
		temp = config.pConfig->tCtrl.alarm_moment_2.moment_minute;
		if(temp>59)
		{
				config.pConfig->tCtrl.alarm_moment_2.moment_minute	= 0;		//����2����
		}
		temp = config.pConfig->tCtrl.alarm_duration_2.duration_hour;
		if(temp>23)
		{
				config.pConfig->tCtrl.alarm_moment_3.moment_hour 		= 22;		//����3Сʱ
		}
		temp = config.pConfig->tCtrl.alarm_duration_2.duration_minute;
		if(temp>59)
		{
				config.pConfig->tCtrl.alarm_moment_3.moment_minute	= 0;		//����3����
		}

		tempOffset = config.pConfig->offsetTemperature;
		if(tempOffset>165||tempOffset<-165)
		{
			config.pConfig->offsetTemperature = 0;			//�¶ȵ�����ֵ����Ϊ�¶���1λС���������¶������洢ֵĬ��*10
		}
		tempOffset = config.pConfig->offsetHumidity;
		if(tempOffset>100||tempOffset<-100)
		{
			config.pConfig->offsetHumidity = 0;			//ʪ�ȵ�����ֵ
		}

		if(config.pConfig->displayLightSet>8)
		{
				config.pConfig->displayLightSet = 4;		//��ʾ��������
		}
		if(config.pConfig->ctrlModeSet>3)
		{
				config.pConfig->ctrlModeSet = 0;				//����ģʽ����
		}

    return true;
}

//ϵͳ�������ز���
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
		//��ʱ1�����ж�
		if(config.pConfig->tCtrl.alarm_duration_enable_1)
		{
				//�жϽ���ʱ���Ƿ����
				if(JudgeAlarmOutline(1))
				{
						//��������ˣ���رն�ʱ������־
						config.pConfig->tCtrl.alarm_duration_enable_1 = false;
				}
				else
				{
						//���û�й��ڣ����趨��־
						duration1NotOutline = true;
				}
		}
		//��ʱ2�����ж�
		if(config.pConfig->tCtrl.alarm_duration_enable_2)
		{
				//�жϽ���ʱ���Ƿ����
				if(JudgeAlarmOutline(2))
				{
						//��������ˣ���رն�ʱ������־
						config.pConfig->tCtrl.alarm_duration_enable_2 = false;
				}
				else
				{
						//���û�й��ڣ����趨��־
						duration2NotOutline = true;
				}
		}
		//��ʱ����
		if(duration1NotOutline)
		{
				if(duration2NotOutline)
				{
						//������ʱ��û�й��ڣ���ȽϽ���ʱ�̿�ǰ��д��
						switch(JudgeDurationFirstArrive())
						{
								case 0:
										//û�ж�ʱ����
										//������ָ����
										break;
								case 1:
										//��ʱ1��ʼ��ʱ
										tempAlarmDate = g_tMainConfig.tCtrl.alarm_duration_1.alarm_date;
										tempAlarmHour = g_tMainConfig.tCtrl.alarm_duration_1.alarm_hour;
										tempAlarmMinute = g_tMainConfig.tCtrl.alarm_duration_1.alarm_minute;
										tempAlarmSecond = g_tMainConfig.tCtrl.alarm_duration_1.alarm_second;
										I2C_DS3231_SetAlarm_1(true,tempAlarmDate,tempAlarmHour,tempAlarmMinute,tempAlarmSecond);
										break;
								case 2:
										//��ʱ2��ʼ��ʱ
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
						//��ʱ1û�й��ڣ���ʱ2���ڣ���д�붨ʱ1
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
						//��ʱ1���ڣ���ʱ2û�й��ڣ���д�붨ʱ2
						tempAlarmDate = g_tMainConfig.tCtrl.alarm_duration_2.alarm_date;
						tempAlarmHour = g_tMainConfig.tCtrl.alarm_duration_2.alarm_hour;
						tempAlarmMinute = g_tMainConfig.tCtrl.alarm_duration_2.alarm_minute;
						tempAlarmSecond = g_tMainConfig.tCtrl.alarm_duration_2.alarm_second;
						I2C_DS3231_SetAlarm_1(true,tempAlarmDate,tempAlarmHour,tempAlarmMinute,tempAlarmSecond);
				}
		}

		//���ӿ���
		switch(JudgeAlarmFirstArrive())
		{
				case 0:
						//û�����ӿ����������һ��DS3231 Alarm2
						I2C_DS3231_SetAlarm_2(false, 1, 1);
						I2C_DS3231_WriteCmd(M0P_I2C0,DS3231_STATUS,0x00);     	//32KHZ�����ֹ�����ӱ�־λ����
						break;
				case 1:
						//ֱ��д��1�ı���ʱ�̣�����Ҫ�ж����ӣ���Ϊ����ʱ�䲻�䣬�Ͷ�ʱ��һ����
						I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_1.moment_hour, g_tMainConfig.tCtrl.alarm_moment_1.moment_minute);
						break;
				case 2:
						//ֱ��д��2�ı���ʱ�̣�����Ҫ�ж����ӣ���Ϊ����ʱ�䲻�䣬�Ͷ�ʱ��һ����
						I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_2.moment_hour, g_tMainConfig.tCtrl.alarm_moment_2.moment_minute);
						break;
				case 3:
						//ֱ��д��3�ı���ʱ�̣�����Ҫ�ж����ӣ���Ϊ����ʱ�䲻�䣬�Ͷ�ʱ��һ����
						I2C_DS3231_SetAlarm_2(true, g_tMainConfig.tCtrl.alarm_moment_3.moment_hour, g_tMainConfig.tCtrl.alarm_moment_3.moment_minute);
						break;
		}

		//�����Ӧʹ���ж�
		if(config.pConfig->ctrlModeSet==0||config.pConfig->ctrlModeSet==1)
		{
				//�ߵ�ƽ��
				Gpio_SetIO(GpioPortA,GpioPin10);
		}
		else
		{
				Gpio_ClrIO(GpioPortA,GpioPin10);
		}
		//��������ʹ���ж�
		if(config.pConfig->ctrlModeSet==1||config.pConfig->ctrlModeSet==3)
		{
				//�ߵ�ƽ�ر�
				Gpio_SetIO(GpioPortB,GpioPin15);
		}
		else
		{
				Gpio_ClrIO(GpioPortB,GpioPin15);
		}
		//��ʾ���ȳ�ʼ��
		Tm1652_Brightness_set(config.pConfig->displayLightSet);
    return true;
}

//�ж��Ǹ���ʱ�ȵ�
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


//�ж��Ǹ������ȵ�
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
		
		//�����뵱ǰʱ���ȵ�
		I2C_DS3231_ReadTime();
		nowMoment = calendar.hour*60 + calendar.minute;				//��ȡ��ǰʱ��
		if(alarmMoment_1 < nowMoment)
		{
				//�´α���ʱ��
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
		//ͬʱ����ʱȡ��ʹ��
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
								//û�����ӿ���
								return 0;
						}
				}
		}
}

//�ж϶�ʱ�Ƿ����
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
																						//�ϸ��������������С��5��Ľ��к���
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
		//�����ܴ�����������
		return true;
}

//�ж϶�ʱʣ��ʱ���Ƿ����1���ӣ�����ֵʣ��ʱ�������ʣ��ʱ������1��������д��0������д��ʣ�����ӡ�
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
				//����ֻ����12�·�������31,�������첹�����������ڵ��첻��
				dateCount = (31 - calendar.date) + tempAlarmDate -1;
				minuteCount = ((dateCount*24)+(24-calendar.hour-1)+tempAlarmHour)*60+(60-calendar.minute)+tempAlarmMinute;
				//�÷��ӽ��м��㣬����û���򲻶������ӵ������1
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
								//����ֻ����12�·�������31,�������첹�����������ڵ��첻��
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
								//�÷��ӽ��м��㣬����û���򲻶������ӵ������1
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
												//�÷��ӽ��м��㣬����û���򲻶������ӵ������1
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
																//�÷��ӽ��м��㣬����û���򲻶������ӵ������1
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
																				//�÷��ӽ��м��㣬����û���򲻶������ӵ������1
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
																				//����1����
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
		//�����ܴ�����������
		return false;
}

//������������
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
		//�鿴�Ƿ��к�������
		//��ʱ����1����
		if(momentEnable_1)
		{
				//��ʱ����2����
				if(momentEnable_2)
				{
						//�����뵱ǰʱ���ȵ�
						I2C_DS3231_ReadTime();
						nowMoment = calendar.hour*60 + calendar.minute;				//��ȡ��ǰʱ��
						if(tempAlarmMoment_1 < nowMoment)
						{
								//�´α���ʱ��
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
						//��ʱ����2����
						if(tempAlarmMoment_1 < tempAlarmMoment_2)
						{
								//д����ʱ����1
								I2C_DS3231_SetAlarm_2(true, tempMomentHour_1, tempMomentMinute_1);
						}
						else
						{
								//д����ʱ����2
								I2C_DS3231_SetAlarm_2(true, tempMomentHour_2, tempMomentMinute_2);
						}
				}
				else
				{
						//д����ʱ����1
						I2C_DS3231_SetAlarm_2(true, tempMomentHour_1, tempMomentMinute_1);
				}
		}
		else
		{
				//��ʱ����2����
				if(momentEnable_2)
				{
						//д����ʱ����2
						I2C_DS3231_SetAlarm_2(true, tempMomentHour_2, tempMomentMinute_2);
				}
				else
				{
						//û��	�������ӿ���
				}
		}
}
