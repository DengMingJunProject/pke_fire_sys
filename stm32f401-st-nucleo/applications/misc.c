#include "misc.h"
#include "rtthread.h"
#include "board.h"
#include "nb-iot.h"
//#include "lora.h"
#include "config.h"
#include "string.h"
#include "extio.h"

#define LOG_TAG              "misc"
#include <ulog.h>

uint8_t		remote_open = 0;
uint8_t		smoke_open = 0;
uint8_t		fire_machine_open=0;
uint8_t		key_open=0;
uint8_t		emergency_open=0;

uint8_t     run_stop_flag = 1;

smoke_report_t	smoke_report;

void misc_open_status(void)
{
	rt_kprintf("remote open %d\n",remote_open);
	rt_kprintf("smoke open %d\n",smoke_open);
	rt_kprintf("fire machine open %d\n",fire_machine_open);
	rt_kprintf("key open %d\n",key_open);
	rt_kprintf("emergency open %d\n",emergency_open);
}
MSH_CMD_EXPORT(misc_open_status,"debug open state");

rt_bool_t misc_smoke_target(void)
{
	rt_bool_t	have_smoke_target = RT_FALSE;
	for( uint8_t i=SMOKE_START; i<=SMOKE_NUM; i++ ){
		
		if( is_smoke(i) ){
			have_smoke_target = RT_TRUE;
		}
	}
	return have_smoke_target;
}

void misc_smoke_report(void)
{
	//检测烟感的报警
	if( smoke_report.poll_time++>10 ){
		smoke_report.poll_time = 0;

		if( misc_smoke_target() || is_fire_machine() ){
			//不处于掉电状态允许上报烟感状态
			if( smoke_report.powerdown_flag==0){
				smoke_report.powerdown_target = 1;
			}
		}
		else{
			smoke_report.powerdown_target = 0;
			smoke_report.powerdown_time  = 0;
		}
	}
	
	if( smoke_report.powerdown_target ){
		
		if( smoke_report.powerdown_time<255 )
			smoke_report.powerdown_time++;
		
		if(smoke_report.powerdown_time>50){

			smoke_open = 0;
			
//			for( uint8_t i=SMOKE_START; i<=SMOKE_NUM; i++ ){
//			
//				if( is_smoke(i) ){
//					
//					if( smoke_report.update[i-1]==0 ){
//						smoke_status_t smoke_stus;
//						smoke_stus.floor = i;
//						smoke_stus.room = 1;
//						smoke_stus.temperature = 0;
//						smoke_stus.smoke = 0;
//						smoke_stus.temperature_alarm = 0;
//						smoke_stus.smoke_alarm = 1;
//						smoke_stus.battery = 0;
//						smoke_stus.connect = 1;
//						
//						smoke_stus.temp_threshold = config.smoke[i-1].temp_threshold;
//						smoke_stus.smoke_threshold = config.smoke[i-1].smoke_threshold;
//						smoke_stus.poll_time = config.smoke[i-1].poll_time;
//						
//						smoke_report.update[i-1]=1;
//						
//						smoke_open = i;
//						
//						if(!nb_iot_publish_smoke(&smoke_stus))
//							LOG_I("smoke %d target publish",smoke_open);
//						
//						LOG_I("input 1 %02X",extio.input.save[0]);
//						LOG_I("input 2 %02X",extio.input.save[1]);
//						LOG_I("input 3 %02X",extio.input.save[2]);
//						
//						//一个循环周期只允许上报一个烟感状态
//						return;
//					}
//				}
//				else{
//					if( smoke_report.update[i-1] ){
//						
//						smoke_report.update[i-1]=0;
//						
//						smoke_status_t smoke_stus;
//						smoke_stus.floor = i;
//						smoke_stus.room = 1;
//						smoke_stus.temperature = 0;
//						smoke_stus.smoke = 0;
//						smoke_stus.temperature_alarm = 0;
//						smoke_stus.smoke_alarm = 0;
//						smoke_stus.battery = 0;
//						smoke_stus.connect = 1;
//						
//						smoke_stus.temp_threshold = config.smoke[i-1].temp_threshold;
//						smoke_stus.smoke_threshold = config.smoke[i-1].smoke_threshold;
//						smoke_stus.poll_time = config.smoke[i-1].poll_time;
//						
//						if(!nb_iot_publish_smoke(&smoke_stus))
//							LOG_I("smoke %d release publish",i);
//						
//						//一个循环周期只允许上报一个烟感状态
//						return;
//					}
//				}
//			}
			
			if( is_fire_machine() ){
				if( smoke_report.fire_machine_update==0 ){
					smoke_report.fire_machine_update = 1;
					fire_machine_open = 1;
				}
			}
			else{
				if( smoke_report.fire_machine_update ){
					smoke_report.fire_machine_update = 0;
					fire_machine_open = 0;
				}
			}
		}
		
		//发生掉电检测，取消上报烟感
//		if( smoke_report.powerdown_flag ){
//			smoke_report.powerdown_target = 0;
//			smoke_report.powerdown_time = 0;
//		}
	}
	else{
//		for( uint8_t i=SMOKE_START; i<=SMOKE_NUM; i++ ){
//			
//			if( smoke_report.update[i-1] ){
//				
//				smoke_report.update[i-1]=0;
//				
//				smoke_status_t smoke_stus;
//				smoke_stus.floor = i;
//				smoke_stus.room = 1;
//				smoke_stus.temperature = 0;
//				smoke_stus.smoke = 0;
//				smoke_stus.temperature_alarm = 0;
//				smoke_stus.smoke_alarm = 0;
//				smoke_stus.battery = 0;
//				smoke_stus.connect = 1;
//				
//				smoke_stus.temp_threshold = config.smoke[i-1].temp_threshold;
//				smoke_stus.smoke_threshold = config.smoke[i-1].smoke_threshold;
//				smoke_stus.poll_time = config.smoke[i-1].poll_time;
//				
//				if(!nb_iot_publish_smoke(&smoke_stus))
//					LOG_I("smoke %d release publish",i);
//				
//				//一个循环周期只允许上报一个烟感状态
//				return;
//			}
//		}
	}
}

//void emergency_isr(void * args)
//{
//	extio_main_alarm(1);
//}

void misc_task(void *parameter)
{	
	uint16_t	lock_time=0;
	uint8_t		door_open=0;
	uint16_t	check_smoke_time = 0;
	uint8_t		save_time = 0;
	
	uint8_t		last_cause = 0;
	uint8_t		last_smoke_open=0;
	while(1)
	{
        if( run_stop_flag==0 ){
            rt_thread_delay(1000);
            LOG_D("halt ");
            continue;
        }
		key_open = key_sta(EXTIO_OPEN_KEY)!=EXTIO_KEY_RELEASE;
		emergency_open = is_check(EXTIO_CHECK_EMERGENCY);
		
		key_clr(EXTIO_OPEN_KEY);
		
		uint8_t open = 0;
		open = key_open || emergency_open || remote_open || smoke_open || fire_machine_open;
		
		if( open ){
			
			lock_time = 0;
			
			if( door_open==0 ){
				
				door_open = 1;
				
				extio_door_open(1);
				
				building_status_t building_stus;
				if( smoke_open )
					building_stus.alarm = 1;
				else
					building_stus.alarm = 0;
				
				building_stus.door_open = 1;
				
				last_cause = building_stus.cause;
				
				if( smoke_open!=0 && smoke_open <= SMOKE_NUM ){
											
					building_stus.floor = smoke_open;
					building_stus.room = 1;
					building_stus.temperature = 26;
					building_stus.smoke = 10;
					building_stus.smoke_alarm = 1;
					building_stus.temp_alarmm = 0;
					
					last_smoke_open = smoke_open;
				}
				
				//只有烟感报警才会上报楼层
				if( last_cause != 4 ){
					building_stus.floor = 0;
					building_stus.room = 0;
				}
				
				if( key_open ){
					LOG_I("door open ");
					building_stus.cause = 1;
				}
				else if( emergency_open ){
					LOG_I("emergency open ");
					building_stus.cause = 2;
					set_all_alarm();
					extio_main_alarm(1);
				}
				else if( remote_open ){
					LOG_I("remote open ");
					building_stus.cause = 3;
				}
				else if( smoke_open ){
					LOG_I("smoke open ");
					building_stus.cause = 4;
					set_all_alarm();
					extio_main_alarm(1);
				}
				else if( fire_machine_open ){
					LOG_I("fire machine open ");
					building_stus.cause = 5;
					set_all_alarm();
					extio_main_alarm(1);
				}
				
				if(!nb_iot_publish_building(&building_stus)){
					LOG_I("building publish ok ");
				}
				
				key_open = 0;
				emergency_open = 0;
				remote_open = 0;
			}
		}
		else if( open==0 ){
			
			if( door_open && !misc_smoke_target() ){
			
				if( lock_time<0xfff )
					lock_time++;
				
				if( lock_time>100 ){
					extio_door_open(0);
					clr_all_alarm();
					extio_main_alarm(0);
					
					door_open = 0;
					
					key_open =0;
					emergency_open = 0; 
					remote_open = 0;
					fire_machine_open = 0;
					
					building_status_t building_stus;
					building_stus.alarm = smoke_open;
					building_stus.door_open = 0;
					building_stus.cause = 0;
										
					if( last_cause == 4 ){
						building_stus.floor = last_smoke_open;
						building_stus.room = 1;
					}
					else{
						building_stus.floor = 0;
						building_stus.room = 0;
					}
					building_stus.temperature = 0;
					building_stus.smoke = 0;
					building_stus.smoke_alarm = 0;
					building_stus.temp_alarmm = 0;
						
					if(!nb_iot_publish_building(&building_stus))
						LOG_I("door clsoe publish");
				}
			}
		}
		
		if( is_check(EXTIO_CHECK_POWERDOWN) && smoke_report.powerdown_flag==0 ){
			smoke_report.powerdown_flag = 1;
			
			if(!nb_iot_publish_powerdown(1))
				LOG_I("power down publish");
		}
		else if(!is_check(EXTIO_CHECK_POWERDOWN) && smoke_report.powerdown_flag==1){
			smoke_report.powerdown_flag = 0;
			
			if( !nb_iot_publish_powerdown(0))
				LOG_I("power up publish");
		}
		
		#ifdef FIRE_MACHINE
		misc_smoke_report();
		#endif
		
		if( save_time++ > 10 ){
			save_time = 0;
			config_timming_save();
		}
		
		if( key_sta(EXTIO_SET_KEY) == EXTIO_KEY_PRESS ){
			key_clr(EXTIO_SET_KEY);
			factory_testcase();
		}
		
		if( key_sta(EXTIO_SET_KEY)==EXTIO_KEY_KEEP ){
			if( key_sta(EXTIO_OPEN_KEY)==EXTIO_KEY_KEEP ){
				key_clr(EXTIO_SET_KEY);
				key_clr(EXTIO_OPEN_KEY);
				LOG_I("set to deault config");
			}
		}
		
//		for( uint8_t i=1; i<=24; i++ ){
//			if( is_smoke(i) )
//				LOG_I("smoke pin %d\r\n",i);
//		}
		
		rt_thread_delay(100);
	}
}

void misc_door_open(int argc, char *argv[])
{
	if( argc == 2 ){
		if( strcmp(argv[1],"open")==0 )
			extio_door_open(1);
		if( strcmp(argv[1],"close")==0 )
			extio_door_open(0);
	}
}

MSH_CMD_EXPORT(misc_door_open,"open the door -- misc_door_open open|close");

rt_err_t misc_init(void)
{	
	
	rt_memset(&smoke_report,0,sizeof(smoke_report_t));
	
	rt_err_t ret = RT_EOK;
	
	/* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("misc", misc_task, RT_NULL, 2048, 10, 10);
    /* 创建成功则启动线程 */
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
		factory_suspend_thread(thread);
    }
    else
    {
        ret = RT_ERROR;
    }
	return ret;
}
INIT_APP_EXPORT(misc_init);
