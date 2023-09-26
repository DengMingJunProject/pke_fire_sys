#include <rtthread.h>
#include "utest.h"
#include "rtthread.h"
#include <dfs_posix.h> /* 当需要使用文件操作时，需要包含这个头文件 */
#include "extio.h"
#include "factory_test.h"

factory_sta_t	factory_sta;

void factory_suspend_thread(rt_thread_t thread)
{
	if( factory_sta.suspend_cnt<SUSPEND_THREAD_MAX ){
		factory_sta.suspend_thread[factory_sta.suspend_cnt++] = thread;
	}
}

static void factory_filesystem(void)
{
	int fd = open("/flash/test.txt", O_WRONLY | O_CREAT);
	if( fd>2 ){
		
		write(fd, "1234567890", 10);
		close(fd);
	}
	else{
		uassert_true(0);
	}
	
	fd = open("/flash/test.txt", O_RDONLY);
	if( fd>2 ){
		
		char buf[20]={0};
		read(fd, buf, 10);
		close(fd);
		unlink("/flash/test.txt");
		
		if( strcmp(buf,"1234567890")==0 ){
			rt_kprintf("文件读写正常\n");
			uassert_true(1);
		}
		else{
			uassert_true(0);
		}
	}
	else{
		uassert_true(0);
	}
}

static void factory_nbiot(void)
{
	rt_kprintf("绿色网络指标灯常亮表示网络连接正常\n");
	time_t now;
        /* output current time */
	now = time(RT_NULL);
	rt_kprintf("%s", ctime(&now));
	
	rt_kprintf("请检查系统时间是否正确\n");
	rt_thread_delay(1000);
	
	uassert_true(1);
}

static void factory_alarm(void)
{
	rt_kprintf("观察10个继电器是否吸合断开正常\n");
	extio_main_alarm(1);
	rt_thread_delay(500);
	for( uint8_t i=1; i<=8; i++ ){
		set_alarm(i);
		rt_thread_delay(500);
	}
	extio_door_open(1);
	rt_thread_delay(1000);
	
	extio_main_alarm(0);
	for( uint8_t i=1; i<=8; i++ ){
		clr_alarm(i);
	}
	extio_door_open(0);
	
	uassert_true(1);
}

static void factory_input_test(void)
{
	uint8_t step=0;
	uint8_t	pin=1;
	rt_kprintf("按下设置键\n");
	while(1){
		switch(step){
			case 0:
				if( key_sta(EXTIO_SET_KEY)==EXTIO_KEY_PRESS ){
					key_clr(EXTIO_SET_KEY);
					
					rt_kprintf("设置键检测成功\n");
					rt_kprintf("按下开门键\n");
					step ++;
				}
				break;
		
			case 1:
				if( key_sta(EXTIO_OPEN_KEY)==EXTIO_KEY_PRESS ){
					key_clr(EXTIO_OPEN_KEY);
					
					rt_kprintf("开门键检测成功\n");
					rt_kprintf("关闭12V电源输入\n");
					step ++;
				}
				break;
			case 2:
				if( is_check(EXTIO_CHECK_POWERDOWN) ){
					rt_kprintf("掉电检测成功\n");
					rt_kprintf("重新接上12V电源输入\n");
					rt_kprintf("短接紧急报警开关端子\n");
					step ++;
				}
				break;
			case 3:
				if( is_check(EXTIO_CHECK_EMERGENCY) ){
					rt_kprintf("emergency checked\n");
					rt_kprintf("紧急报警开关端子检测成功\n");
					rt_kprintf("请放上测试排针\n");
					step ++;
				}
				break;
			case 4:
			{
				uint8_t i=0;
				while(i<24){
					if( is_smoke(i+1)==0 ){
						rt_kprintf("%d号没有信号正常\n",i+1);
						i++;
					}
					rt_thread_delay(1);
				}
				
				rt_kprintf("全部烟感信号检测正常\n");
				step ++;
				break;
			}
			case 5:
				uassert_true(1);
				return;
				break;
			default:
				break;
			
		}
		rt_thread_delay(100);
	}
}

static rt_err_t factory_test_tc_init(void)
{
	for( uint8_t i=0; i<factory_sta.suspend_cnt; i++ ){
		rt_thread_delete(factory_sta.suspend_thread[i]);
	}
    return RT_EOK;
}

static rt_err_t factory_test_tc_cleanup(void)
{
//	for( uint8_t i=0; i<factory_sta.suspend_cnt; i++ ){
//		rt_thread_resume(factory_sta.suspend_thread[i]);
//	}
    return RT_EOK;
}

void factory_testcase(void)
{
    UTEST_UNIT_RUN(factory_filesystem);
	UTEST_UNIT_RUN(factory_nbiot);
	UTEST_UNIT_RUN(factory_alarm);
	UTEST_UNIT_RUN(factory_input_test);
}
UTEST_TC_EXPORT(factory_testcase, "factory.test", factory_test_tc_init, factory_test_tc_cleanup, 10);