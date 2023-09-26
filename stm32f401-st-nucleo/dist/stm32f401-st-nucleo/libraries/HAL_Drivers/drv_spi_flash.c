/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#include "drv_spi.h" 
#include "drv_spi_flash.h"

#include "rtthread.h" 
#include "rtdevice.h" 
#include "spi_flash.h"
#include "spi_flash_sfud.h"

#define RT_SPI_CS0_PIN (4)		// 58 - 32 = 26

#define CHARACTER_LIBRARY_DEFAULT_SPI_CFG        \
{                                                \
    .mode = RT_SPI_MODE_0 | RT_SPI_MSB,          \
    .data_width = 8,                             \
    .max_hz = 45 * 1000 * 1000,                  \
}

//#define FLASH_DEFAULT_SPI_CFG        \
//{                                                \
//    .mode = RT_SPI_MODE_0 | RT_SPI_MSB,          \
//    .data_width = 8,                             \
//    .max_hz = 48 * 1000 * 1000,                  \
//}

void mx25lxx_enter_spi_mode(struct rt_spi_device *device)
{
	char write_enable = 0x06;
	char read_status = 0x05;
	char status = 0;
	char write_status_buf[2] = {0x01, 0x40};	
	
	rt_spi_send_then_send(device,&write_enable,1,write_status_buf,2);
	
	rt_spi_send_then_recv(device, &read_status, 1, &status, 1);
	//rt_kprintf("read status register = 0x%02x \n", status);
}

rt_err_t lpc_spi_character_library_configure(const char *name)
{
	rt_err_t result;
	struct rt_spi_configuration cfg = CHARACTER_LIBRARY_DEFAULT_SPI_CFG;
	struct rt_spi_device *spi_device = (struct rt_spi_device *) rt_device_find(name);;

	result = rt_spi_configure(spi_device, &cfg);
	return result;
}

int rt_hw_flash_init(void)
{
    rt_err_t result; 
    
    result = lpc_spi_bus_attach_device("spi2", "fg_font", RT_SPI_CS0_PIN); 
    if(result != RT_EOK) 
    {
        return result; 
    }

	result = lpc_spi_bus_attach_device("spi2", "cn_font", RT_SPI_CS1_PIN); 
    if(result != RT_EOK) 
    {
        return result; 
    }
	
	result = lpc_spi_bus_attach_device("spi2", "flash0_spi", RT_SPI_CS2_PIN); 
    if(result != RT_EOK) 
    {
        return result; 
    }

	result = lpc_spi_character_library_configure("fg_font");
	if(result != RT_EOK) 
    {
        return result; 
    }

	result = lpc_spi_character_library_configure("cn_font");
	if(result != RT_EOK) 
    {
        return result; 
    }
	
	if (RT_NULL == rt_sfud_flash_probe("flash0", "flash0_spi"))
    {
        return -RT_ERROR;
    }
	
//	{
//		rt_err_t result;
//		struct rt_spi_configuration cfg = FLASH_DEFAULT_SPI_CFG;
//		struct rt_spi_device *spi_device = (struct rt_spi_device *) rt_device_find("flash0_spi");;

//		result = rt_spi_configure(spi_device, &cfg);
//		return result;
//	}
	
//	while (1)
//	{        
//		
////		rt_pin_write(RT_SPI_CS1_PIN, !rt_pin_read(RT_SPI_CS1_PIN));
//		GPIO->B[1][27] = 1;
//		GPIO->B[1][27] = 0;
////		rt_thread_mdelay(1000);
//	}
		
    return RT_EOK; 
}
INIT_DEVICE_EXPORT(rt_hw_flash_init); 
