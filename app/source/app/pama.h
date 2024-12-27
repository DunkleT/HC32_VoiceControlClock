#ifndef __PAMA_H__
#define __PAMA_H__

#include "bsp_delay.h"

#define	DEV_INF_VER_SOFT				0x0101			// Ӳ���汾
#define DEV_INF_VER_HARD				0x0101			// ����汾

//����ģʽ����
#define CTRL_MODE_ERES					0				//�������⣬��������
#define CTRL_MODE_ERDS					1				//�������⣬�ر�����
#define CTRL_MODE_DRES					2				//�رպ��⣬��������
#define CTRL_MODE_DRDS					3				//�رպ��⣬�ر�����


typedef struct tagAlarmDuration
{
		//Сʱ���ֵ
		uint8_t duration_hour;
		//���Ӽ��ֵ
		uint8_t duration_minute;
		
		//������ʱ���
		uint8_t alarm_yearL;
		//������ʱ�·�
		uint8_t alarm_month;
		//������ʱ����
		uint8_t alarm_date;
		//������ʱСʱ
		uint8_t alarm_hour;
		//������ʱ����
		uint8_t alarm_minute;
		//������ʱ����
		uint8_t alarm_second;
}AlarmDuration;

typedef struct tagAlarmMoment
{
		//Сʱ���ֵ
		uint8_t moment_hour;
		//���Ӽ��ֵ
		uint8_t moment_minute;
}AlarmMoment;

//���ò������Ͷ���
typedef	struct tagMainConfig
{
		struct
		{
				uint8_t time_yearH;			//�� 
				uint8_t time_yearL;			//�� 
				uint8_t time_month;			//�� 			
				uint8_t time_date;		  //�� 
				uint8_t time_hour;		  //ʱ 
				uint8_t time_minute;		//��
				uint8_t time_second;		//��
				uint8_t time_week;		  //�� 
		}tTime;

    struct
    {
        AlarmDuration alarm_duration_1;		//��ʱʱ��1
        AlarmDuration alarm_duration_2;		//��ʱʱ��2

        bool alarm_duration_enable_1;			//��ʱ1����
        bool alarm_duration_enable_2;			//��ʱ2����
				
				AlarmMoment alarm_moment_1;				//����ʱ��1
				AlarmMoment alarm_moment_2;				//����ʱ��2
				AlarmMoment alarm_moment_3;				//����ʱ��3

				bool alarm_moment_enable_1;				//����1����
        bool alarm_moment_enable_2;				//����2����
				bool alarm_moment_enable_3;				//����3����
    } tCtrl;			// ���Ʋ���

		float32_t  offsetTemperature;			//�¶ȵ�����ֵ����Ϊ�¶���1λС���������¶������洢ֵĬ��*10
		float32_t  offsetHumidity;				//ʪ�ȵ�����ֵ
		
		uint8_t  displayLightSet;		//��ʾ��������0~7�ȼ�
		uint8_t  ctrlModeSet;				//����ģʽ����

		bool dispalyMode;						//��ʾģʽ�����Ƴ���ģʽ
		bool timeMode;							//ʱ��ģʽ��0-24Сʱ��1-12Сʱ

		uint16_t	nDevVerSoft;			// ����汾  DEV_INF_VER_SOFT
    uint16_t	nDevVerHard;			// Ӳ���汾  DEV_INF_VER_HARD

} MainConfig, *PMainConfig;

//�豸�������Ͷ���
typedef struct tagDevicePama
{
    MainConfig*		pConfig;
		//������ȷ��־λ
    bool correct;
} DevicePama;

//�豸״̬
typedef struct tagDeviceState
{
    uint8_t  nKeyPress;			//��ǰ��������
		uint8_t  nPageShow;			//��ǰҳ����ʾ
		uint8_t  nFuncIndex;		//��ǰ��������
		uint8_t  nVoicePage;		//��ǰ��������
} DeviceState;

#define DEV_CONFIG_DATA_LEN			sizeof(MainConfig)
#define DEV_CONFIG_TOTAL_LEN		(sizeof(MainConfig) + 2)

bool MainConfigReset(void);
bool MainConfigSave(void);
bool MainConfigCorrect(void);
bool MainConfigLoadAtBoot(void);

uint8_t JudgeDurationFirstArrive(void);
uint8_t JudgeAlarmFirstArrive(void);

bool JudgeAlarmOutline(uint8_t durationIndex);
bool GetDurationRemainTime(uint8_t durationIndex, uint8_t* remainHour, uint8_t* remainMinute, uint8_t* remainSecond);
void OpenMomentNextArrive(uint8_t momentIndex);

#endif
