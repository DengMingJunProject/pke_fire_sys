#ifndef __NB_IOT_H__
#define __NB_IOT_H__

#include "stdint.h"
#include "stdbool.h"

#define		DEVICE_TYPE		1

#define 	CMD_UP_SMOKE	0		//上报烟感
#define		CMD_UP_BUILDING	1		//上报门状态
#define 	CMD_UP_POWER	2		//上报掉电状态
#define		CMD_UP_START	3		//上报启动信息
#define		CMD_UP_STATUS	4		//上报主控状态等信息

enum{
	STUS_OK=0,
	STUS_RST,
	STUS_OTA,
};

#define		CMD_DOWN_ALL_SMOKE	0		//下发配置所有烟感参数
#define		CMD_DOWN_DOOR		1		//下发开门
#define		CMD_DOWN_GET_SMOKE	2		//下发获取指定烟感参数
#define		CMD_DOWN_SET_SMOKE	3		//下发设置指定烟感参数
#define		CMD_DOWN_UPDATE		4		//下发请求主控升级
#define		CMD_DOWN_SETTING	5		//下发配置用户名密码ID楼栋ID等
#define		CMD_DOWN_RESET		6		//下发复位主控命令
#define		CMD_DOWN_STATUS		7		//下发获取状态
#define     CMD_DOWN_STOP       8       //下发停机命令
#define     CMD_DOWN_RUN        9       //下发运行命令

typedef struct _smoke_status
{
	uint8_t		floor;
	uint8_t		room;
	uint16_t	temperature;
	uint16_t	smoke;
	uint8_t		temperature_alarm;
	uint8_t		smoke_alarm;
	uint16_t	battery;
	uint8_t		connect;
	
	uint16_t	temp_threshold;
	uint16_t	smoke_threshold;
	uint8_t		poll_time;
}smoke_status_t;

typedef struct _building_status
{
	uint8_t		alarm;
	uint8_t		door_open;
	uint8_t		cause;
	
	uint8_t		floor;
	uint8_t		room;
	uint16_t	temperature;
	uint16_t	smoke;
	uint8_t		smoke_alarm;
	uint8_t		temp_alarmm;
}building_status_t;

extern int nb_iot_publish_building(building_status_t *building_stus);
extern int nb_iot_publish_smoke(smoke_status_t *smoke_stus);
extern int nb_iot_publish_powerdown(uint8_t powerdown);
#endif

