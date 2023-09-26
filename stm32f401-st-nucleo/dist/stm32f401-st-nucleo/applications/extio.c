
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "stm32_io.h"
#include "extio.h"

#define LOG_TAG              "extio"
#include <ulog.h>

#define EXTIO_CLK_PIN    	GET_PIN(B, 14)
#define EXTIO_PE_PIN     	GET_PIN(B, 15)
#define EXTIO_LE_PIN		GET_PIN(C, 6)
#define EXTIO_SMOKE_PIN		GET_PIN(C, 7)
#define EXTIO_ALARM_PIN		GET_PIN(C, 8)

#define EXTIO_POWERDOWN		GET_PIN(B, 9)
#define EXTIO_EMERGENCY		GET_PIN(C, 13)

#define EXTIO_DOOR_OPEN		GET_PIN(C, 0)
#define EXTIO_MAIN_ALARM	GET_PIN(A, 11)

#define EXTIO_DOOR_KEY_PIN		GET_PIN(B, 1)
#define EXTIO_SET_KEY_PIN		GET_PIN(B, 2)

extio_t	extio;

void extio_write(uint8_t byte)
{
	for( uint8_t i=0; i<8; i++ ){
		
		if( byte&(0x80) )
			rt_pin_write(EXTIO_ALARM_PIN, PIN_HIGH);
		else
			rt_pin_write(EXTIO_ALARM_PIN, PIN_LOW);
		
		rt_pin_write(EXTIO_CLK_PIN, PIN_HIGH);
		rt_pin_write(EXTIO_CLK_PIN, PIN_LOW);
		byte<<=1;
	}
}

void extio_output(void)
{
	for( uint8_t i=0; i<EXTIO_OUT_MAX/8; i++ ){
		extio_write(extio.output.byte[i]);
	}
	rt_pin_write(EXTIO_LE_PIN, PIN_HIGH);
	rt_pin_write(EXTIO_LE_PIN, PIN_LOW);
}

void extio_read(uint8_t *byte,uint8_t first)
{
	rt_base_t level;
	level = rt_hw_interrupt_disable();
	*byte = 0;
	if( first ){
		
		if( rt_pin_read(EXTIO_SMOKE_PIN) )
				(*byte) |= 0x01;
		
		for( uint8_t i=0; i<7; i++ ){
			
			rt_pin_write(EXTIO_CLK_PIN, PIN_HIGH);
			
			(*byte)<<=1;
			if( rt_pin_read(EXTIO_SMOKE_PIN) )
				(*byte) |= 0x01;
			
			rt_pin_write(EXTIO_CLK_PIN, PIN_LOW);
			
		}
	}
	else{
		for( uint8_t i=0; i<8; i++ ){
			
			rt_pin_write(EXTIO_CLK_PIN, PIN_HIGH);
			
			(*byte)<<=1;
			if( rt_pin_read(EXTIO_SMOKE_PIN) )
				(*byte) |= 0x01;
			
			rt_pin_write(EXTIO_CLK_PIN, PIN_LOW);
			
		}
	}
	rt_hw_interrupt_enable(level);
}

//void extio_read(uint8_t *byte)
//{
//	rt_base_t level;
//	level = rt_hw_interrupt_disable();
//	*byte = 0;
//	
//	rt_pin_write(EXTIO_CLK_PIN, PIN_HIGH);
//	rt_pin_write(EXTIO_CLK_PIN, PIN_LOW);
//	(rt_pin_read(EXTIO_SMOKE_PIN)==PIN_HIGH)?(*byte)|=0x80:0;
//	
//	rt_pin_write(EXTIO_CLK_PIN, PIN_HIGH);
//	rt_pin_write(EXTIO_CLK_PIN, PIN_LOW);
//	(rt_pin_read(EXTIO_SMOKE_PIN)==PIN_HIGH)?(*byte)|=0x40:0;
//	
//	rt_pin_write(EXTIO_CLK_PIN, PIN_HIGH);
//	rt_pin_write(EXTIO_CLK_PIN, PIN_LOW);
//	(rt_pin_read(EXTIO_SMOKE_PIN)==PIN_HIGH)?(*byte)|=0x20:0;
//	
//	rt_pin_write(EXTIO_CLK_PIN, PIN_HIGH);
//	rt_pin_write(EXTIO_CLK_PIN, PIN_LOW);
//	(rt_pin_read(EXTIO_SMOKE_PIN)==PIN_HIGH)?(*byte)|=0x10:0;
//	
//	rt_pin_write(EXTIO_CLK_PIN, PIN_HIGH);
//	rt_pin_write(EXTIO_CLK_PIN, PIN_LOW);
//	(rt_pin_read(EXTIO_SMOKE_PIN)==PIN_HIGH)?(*byte)|=0x08:0;
//	
//	rt_pin_write(EXTIO_CLK_PIN, PIN_HIGH);
//	rt_pin_write(EXTIO_CLK_PIN, PIN_LOW);
//	(rt_pin_read(EXTIO_SMOKE_PIN)==PIN_HIGH)?(*byte)|=0x04:0;
//	
//	rt_pin_write(EXTIO_CLK_PIN, PIN_HIGH);
//	rt_pin_write(EXTIO_CLK_PIN, PIN_LOW);
//	(rt_pin_read(EXTIO_SMOKE_PIN)==PIN_HIGH)?(*byte)|=0x02:0;
//	
//	rt_pin_write(EXTIO_CLK_PIN, PIN_HIGH);
//	rt_pin_write(EXTIO_CLK_PIN, PIN_LOW);
//	(rt_pin_read(EXTIO_SMOKE_PIN)==PIN_HIGH)?(*byte)|=0x01:0;
//	
//	rt_hw_interrupt_enable(level);
//}

void extio_input(void)
{
	rt_pin_write(EXTIO_PE_PIN, PIN_LOW);
	rt_pin_write(EXTIO_PE_PIN, PIN_HIGH);
//	rt_hw_us_delay(10);
	uint8_t *in_ptr =&extio.input.buf[EXTIO_IN_MAX/8-1];
	for( uint8_t i=0; i<EXTIO_IN_MAX/8; i++ ){
		//第一个芯片只需7个时钟
		if( i==0 )
			extio_read(in_ptr,1);
		else
			extio_read(in_ptr,0);
		in_ptr--;
	}
	
}

__inline void extio_door_open(rt_bool_t sta)
{
	rt_pin_write(EXTIO_DOOR_OPEN, sta?RT_TRUE:RT_FALSE);
}

__inline void extio_main_alarm(rt_bool_t sta)
{
	rt_pin_write(EXTIO_MAIN_ALARM, sta);
}

void extio_input_check( void )
{
	uint8_t i,io,k,num;

	for(  num=0; num<EXTIO_IN_MAX/8; num++ )
	{
		if( extio.input.buf[num]!=extio.input.save[num] )
		{
			io = extio.input.buf[num]^extio.input.save[num];
			for( i=0; i<8; i++ )
			{
				if( io&0x01 )
				{
					if( extio.input.timer[num*8+i]<30 )
					{
						extio.input.timer[num*8+i]++;	//每一个IO口对应一个计时器
					}
					else
					{
						extio.input.timer[num*8+i] = 0;
						k = extio.input.buf[num];
						k >>= i;
						if( k&0x01 )
						{
							extio.input.save[num] |= (1<<i);
						}
						else
						{
							extio.input.save[num] &= (~(1<<i));
						}
					}
				}
				else
				{
					extio.input.timer[num*8+i] = 0;
				}
				io >>= 1;
			}
		}
	}
}

void extio_key_scan(void)
{
	uint8_t sta[EXTIO_KEY_MAX];
	sta[0] = rt_pin_read(EXTIO_SET_KEY_PIN);
	sta[1] = rt_pin_read(EXTIO_DOOR_KEY_PIN);
	
	for( uint8_t i=0; i<EXTIO_KEY_MAX; i++ ){
		
		if( sta[i]==0 ){
			
			if( extio.key.time[i]<255 )
				extio.key.time[i]++;
			
			if( extio.key.time[i]>20 ){
				
				extio.key.sta[i] = EXTIO_KEY_KEEP;
				LOG_D("%d key keep",i);
			}
		}
		else{
			
			if( extio.key.sta[i] == EXTIO_KEY_RELEASE ){
				if( extio.key.time[i]>2 ){
					extio.key.sta[i] = EXTIO_KEY_PRESS;
					LOG_D("%d key pres",i);
				}
			}
			extio.key.time[i] = 0;
		}
	}
}

void extio_check(void)
{
	uint8_t sta[EXTIO_CHECK_AMX];
	sta[0] = !rt_pin_read(EXTIO_POWERDOWN);
	sta[1] = rt_pin_read(EXTIO_EMERGENCY);
	
	for( uint8_t i=0; i<EXTIO_CHECK_AMX; i++ ){
		
		if( sta[i]==0 ){
			
			if( extio.check.time[i]<255 )
				extio.check.time[i]++;
			
			if( extio.check.time[i]>10 ){
				
				extio.check.sta[i] = 1;
			}
		}
		else{
			extio.check.time[i] = 0;
			extio.check.sta[i] = 0;
		}
	}
}

void extio_task(void *parameter)
{
	while(1)
	{
		extio_output();
//		extio_input();
//		extio_input_check();
		extio_key_scan();
		extio_check();
		rt_thread_delay(100);
	}
}

rt_err_t extio_init(void)
{
	rt_memset(&extio,0,sizeof(extio_t));
	rt_memset(&extio.input.buf,0xff,EXTIO_IN_MAX/8);
	rt_memset(&extio.input.save,0xff,EXTIO_IN_MAX/8);
	
	//串行IO扩展
	rt_pin_mode(EXTIO_CLK_PIN, PIN_MODE_OUTPUT_OD);
	rt_pin_write(EXTIO_CLK_PIN, PIN_LOW);
	
	rt_pin_mode(EXTIO_PE_PIN, PIN_MODE_OUTPUT_OD);
	rt_pin_write(EXTIO_PE_PIN, PIN_LOW);
	
	rt_pin_mode(EXTIO_LE_PIN, PIN_MODE_OUTPUT_OD);
	rt_pin_write(EXTIO_LE_PIN, PIN_LOW);
	
	rt_pin_mode(EXTIO_SMOKE_PIN, PIN_MODE_INPUT);
	
	rt_pin_mode(EXTIO_ALARM_PIN, PIN_MODE_OUTPUT_OD);
	rt_pin_write(EXTIO_ALARM_PIN, PIN_LOW);
	
	//输出控制
	rt_pin_mode(EXTIO_MAIN_ALARM,PIN_MODE_OUTPUT);
	rt_pin_write(EXTIO_MAIN_ALARM, PIN_LOW);
	rt_pin_mode(EXTIO_DOOR_OPEN, PIN_MODE_OUTPUT_OD);
	rt_pin_write(EXTIO_DOOR_OPEN, PIN_LOW);
	
	//检测
	rt_pin_mode(EXTIO_POWERDOWN, PIN_MODE_INPUT);
	rt_pin_mode(EXTIO_EMERGENCY, PIN_MODE_INPUT);
	
//	rt_pin_mode(EXTIO_EMERGENCY, PIN_MODE_INPUT_PULLUP);   
//	/* 边沿触发（上升沿和下降沿都触发）*/
//	extern void emergency_isr(void * args);
//    rt_pin_attach_irq(EXTIO_EMERGENCY, PIN_IRQ_MODE_RISING_FALLING , emergency_isr, RT_NULL);
//    rt_pin_irq_enable(EXTIO_EMERGENCY, PIN_IRQ_ENABLE); 
	
	//按键
	rt_pin_mode(EXTIO_DOOR_KEY_PIN,  PIN_MODE_INPUT_PULLUP);
	rt_pin_mode(EXTIO_SET_KEY_PIN,   PIN_MODE_INPUT_PULLUP);
	
	rt_err_t ret = RT_EOK;
	
    rt_thread_t thread = rt_thread_create("extio", extio_task, RT_NULL, 1024, 8, 10);
    /* 创建成功则启动线程 */
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        ret = RT_ERROR;
    }
	return ret;
}
INIT_COMPONENT_EXPORT(extio_init);
