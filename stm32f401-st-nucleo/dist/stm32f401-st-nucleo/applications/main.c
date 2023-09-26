/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "hal.h"
#include "stdio.h"

#define LOG_TAG              "main"
#include <ulog.h>

static int rt_flash_init(void)
{
#include <spi_flash.h>
#include <drv_spi.h>
    extern rt_spi_flash_device_t rt_sfud_flash_probe(const char *spi_flash_dev_name, const char *spi_dev_name);
    extern int fal_init(void);

    rt_hw_spi_device_attach("spi1", "spi10", GPIOA, GPIO_PIN_4);

    /* initialize SPI Flash device */
    rt_sfud_flash_probe("norflash0", "spi10");

    fal_init();

    return 0;
}
INIT_ENV_EXPORT(rt_flash_init);

int rt_bc28_reset(void)
{
	rt_pin_mode(8, PIN_MODE_OUTPUT);
	rt_pin_write(8, PIN_LOW);
	
	rt_pin_mode(29, PIN_MODE_OUTPUT);
	rt_pin_write(29, PIN_LOW);
	rt_thread_mdelay(100);
	rt_pin_write(29, PIN_HIGH);
}

/* defined the LED0 pin: PB1 */
#define LED0_PIN    GET_PIN(C, 4)
uint8_t	blink_time = 50;

void set_blink_time( uint8_t time )
{
	blink_time = time;
}

/* 定时器 1 超时函数 */
static void led_blink_timeout(void *parameter)
{
	static uint8_t flag=0,cnt=0;
	if( cnt++>=blink_time ){
		cnt = 0;
		if( flag ){
			flag = 0;
			rt_pin_write(LED0_PIN, PIN_HIGH);
		}
		else{
			flag = 1;
			rt_pin_write(LED0_PIN, PIN_LOW);
		}
	}
}

void boot_loader(void)
{
	#ifdef BOOTLOADER
	SCB->VTOR = FLASH_BASE | 0x10000;
	#endif
}
INIT_BOARD_EXPORT(boot_loader);

void led_blink_init(void)
{
	static rt_timer_t timer1;
	timer1 = rt_timer_create("blink", led_blink_timeout,
                             RT_NULL, 10,
                             RT_TIMER_FLAG_PERIODIC);
	/* 启动定时器 1 */
    if (timer1 != RT_NULL) rt_timer_start(timer1);
}

///* 定时器 1 超时函数 */
//static void reboot_timeout(void *parameter)
//{
//	rt_hw_cpu_reset();
//}

//void reboot_test_init(void)
//{
//	static rt_timer_t timer1;
//	timer1 = rt_timer_create("reboot", reboot_timeout,
//                             RT_NULL, 100000,
//                             RT_TIMER_FLAG_PERIODIC);
//	/* 启动定时器 1 */
//    if (timer1 != RT_NULL) rt_timer_start(timer1);
//}

#define MAJOR_VERSION		"1.2.6"

const char *main_version(void)
{
	return MAJOR_VERSION;
}

const char *main_compile(void)
{
	static char buf[30];
	rt_memset(buf,0,30);
	rt_snprintf(buf,30,"%s %s",__DATE__,__TIME__);
	return buf;
}

void easyflash_init2(void)
{
	easyflash_init();
}

INIT_ENV_EXPORT(easyflash_init2);

int main(void)
{
//	easyflash_init();
//	ulog_ef_backend_init();
    /* set LED0 pin mode to output */
    rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);
	led_blink_init();
	
	char id[10]={0};
	uint8_t *un_id = hal_read_uniqueid();
	uint32_t crc = crc32_cal(un_id,STM32_UNIQUE_ID_SIZE);
	
	snprintf(id,10,"%08X",crc);
//	reboot_test_init();
	LOG_D("====================================");
	LOG_D("The pkefire version:	%s",MAJOR_VERSION);
	LOG_D( "last compile : %s--%s", __DATE__, __TIME__ );
	LOG_D( "product serial numner : %s", id );
	LOG_D("====================================");
	
	LOG_D("start successful");

    return RT_EOK;
}
