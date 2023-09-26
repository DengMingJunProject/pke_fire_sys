#ifndef __LORA_H__
#define __LORA_H__

#include "stdint.h"

#define SMOKE_START			2
#define SMOKE_NUM			20

typedef enum _err_sta
{
	ERR_STA_OK=0,
	ERR_STA_CRC,
	ERR_STA_ID,
}err_sta_t;

typedef struct _smoke_dev
{
	uint8_t		floor;
	uint8_t		room;
	uint16_t	smoke;
	uint16_t	temperature;
	uint16_t	battery;
	uint8_t		alarm_smoke;
	uint8_t		alarm_temperature;
	uint8_t		connect;
	uint8_t		active_time;
	err_sta_t	err_sta;
}smoke_dev_t;

typedef struct _smoke_data
{
	uint16_t	smoke;
	uint16_t	temperature;
	uint16_t	battery;
	union{
		struct{
			uint8_t		smoke:1;
			uint8_t		tempater:1;
			uint8_t		reserved:6;
		}bit;
		uint8_t byte;
	}alarm;
}smoke_data_t;

typedef struct _lora_state
{
	uint8_t		msgid;
	uint8_t		alarm_set;
	smoke_dev_t smoke_dev[SMOKE_NUM];
	uint8_t		smoke_num;
	uint8_t		debug;
}lora_state_t;

extern lora_state_t lora_sta;

#endif
