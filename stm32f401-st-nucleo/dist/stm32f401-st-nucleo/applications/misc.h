#ifndef __MISC_H__
#define __MISC_H__

#include "stdint.h"

#define SMOKE_NUM			20

typedef struct _smoke_report{
	uint8_t		update[SMOKE_NUM];
	uint8_t		poll_time;
	uint8_t		powerdown_time;
	uint8_t		powerdown_target;
	uint8_t		powerdown_flag;
	
	uint8_t		fire_machine_update;
}smoke_report_t;

#endif
