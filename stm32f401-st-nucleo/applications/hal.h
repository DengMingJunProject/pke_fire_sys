
#ifndef __HAL_H__
#define __HAL_H__

#include "stdint.h"
#include "stdbool.h"

#define 	STM32_UNIQUE_ID_SIZE 	12

extern uint8_t* hal_read_uniqueid(void);
extern void hal_app_reset(void);

#endif
