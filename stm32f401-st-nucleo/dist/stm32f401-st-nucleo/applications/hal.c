
#include "hal.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

static uint8_t	stm32_uniqueId[STM32_UNIQUE_ID_SIZE] = {0}; 
uint8_t* hal_read_uniqueid(void) 
{        
    volatile uint8_t* addr = (volatile uint8_t*)(UID_BASE);   //UID首地址
    for(uint8_t i = 0; i < STM32_UNIQUE_ID_SIZE; ++i)
    {
        uint8_t id= *addr;
        stm32_uniqueId[i] = id;
        ++addr;
    }
    return stm32_uniqueId;
}

typedef void (*rt_fota_app_func)(void);
static rt_fota_app_func app_func = RT_NULL;
void hal_app_reset(void)
{
		//用户代码区第二个字为程序开始地址(复位地址)
	app_func = (rt_fota_app_func)*(__IO uint32_t *)(0x08010000 + 4);
	/* Configure main stack */ 
	__set_MSP(*(__IO uint32_t *)0x08010000);       
           
	/* jump to application */
	app_func();
}