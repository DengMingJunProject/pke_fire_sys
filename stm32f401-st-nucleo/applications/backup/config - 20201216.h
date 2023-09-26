#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "stdint.h"
#include "stdbool.h"
#include "lora.h"

#pragma pack(1)

typedef struct _config_dev
{
	uint8_t		floor;
	uint8_t		room;
}config_dev_t;

typedef struct _config_mqtt
{
	char		host[100];
	char		user[100];
	char		password[100];
	char 		id[100];
	char		publish_topic[20];
	char		subscribe_topic[20];
}config_mqtt_t;

typedef struct _config
{
	char building_id[15];
	config_mqtt_t	mqtt;
	
	uint8_t		smoke_num;
	config_dev_t dev[SMOKE_NUM];
	
	uint16_t	temp_threshold;
	uint16_t	smoke_threshold;
	uint8_t		poll_time;
	
	uint8_t		crc;
}config_t;
#pragma pack()

extern config_t config;

#endif
