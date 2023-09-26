#ifndef __LORA_H__
#define __LORA_H__

#include "stdint.h"

#define SMOKE_NUM			20

#define SEARCH_FLOOR		20
#define SEARCH_ROOM			2

typedef struct _smoke_dev
{
	uint8_t		floor;
	uint8_t		room;
	uint16_t	smoke;
	uint16_t	temperature;
	uint16_t	battery;
	uint8_t		alarm_smoke;
	uint8_t		alarm_temperature;
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
	uint8_t		app_set;
	uint8_t		alarm_set;
}lora_state_t;

extern lora_state_t lora_sta;
#define lora_set_smoke()	(lora_sta.app_set = 1)

#endif
