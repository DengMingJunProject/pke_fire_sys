#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "stdint.h"
#include "stdbool.h"
#include "lora.h"
#include "rtthread.h"

#pragma pack(1)

typedef struct _config_smoke
{
	uint8_t		floor;
	uint8_t		room;
	uint16_t	temp_threshold;
	uint16_t	smoke_threshold;
	uint8_t		poll_time;
}config_smoke_t;

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
	char building_id[20];
	config_mqtt_t	mqtt;
	char			update_uri[50];
	uint32_t		ver;
	
	uint16_t	temp_threshold;
	uint16_t	smoke_threshold;
	uint8_t		poll_time;
	uint32_t	lora_freq;
	
	config_smoke_t smoke[SMOKE_NUM];
	
//	config_no_t smoke_no[SMOKE_NUM];
//	uint8_t		smoke_num;
	
	uint8_t		crc;
}config_t;
#pragma pack()

extern config_t config;
extern void config_timming_save(void);
extern rt_err_t config_wait_config_sem(void);

#endif
