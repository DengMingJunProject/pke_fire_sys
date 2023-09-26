#ifndef __NB_IOT_H__
#define __NB_IOT_H__

#include "stdint.h"
#include "stdbool.h"

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
	
	uint8_t		alarm;
	uint8_t		door_open;
	uint8_t		cause;
}smoke_status_t;

typedef struct _building_status
{
	uint8_t		alarm;
	uint8_t		door_open;
	uint8_t		cause;
}building_status_t;

#endif

