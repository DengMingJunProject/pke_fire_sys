#include <rtthread.h>
#include "utest.h"
#include "rtthread.h"
#include <dfs_posix.h> /* ����Ҫʹ���ļ�����ʱ����Ҫ�������ͷ�ļ� */
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
			rt_kprintf("�ļ���д����\n");
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
	rt_kprintf("��ɫ����ָ��Ƴ�����ʾ������������\n");
	time_t now;
        /* output current time */
	now = time(RT_NULL);
	rt_kprintf("%s", ctime(&now));
	
	rt_kprintf("����ϵͳʱ���Ƿ���ȷ\n");
	rt_thread_delay(1000);
	
	uassert_true(1);
}

static void factory_alarm(void)
{
	rt_kprintf("�۲�10���̵����Ƿ����϶Ͽ�����\n");
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
	rt_kprintf("�������ü�\n");
	while(1){
		switch(step){
			case 0:
				if( key_sta(EXTIO_SET_KEY)==EXTIO_KEY_PRESS ){
					key_clr(EXTIO_SET_KEY);
					
					rt_kprintf("���ü����ɹ�\n");
					rt_kprintf("���¿��ż�\n");
					step ++;
				}
				break;
		
			case 1:
				if( key_sta(EXTIO_OPEN_KEY)==EXTIO_KEY_PRESS ){
					key_clr(EXTIO_OPEN_KEY);
					
					rt_kprintf("���ż����ɹ�\n");
					rt_kprintf("�ر�12V��Դ����\n");
					step ++;
				}
				break;
			case 2:
				if( is_check(EXTIO_CHECK_POWERDOWN) ){
					rt_kprintf("������ɹ�\n");
					rt_kprintf("���½���12V��Դ����\n");
					rt_kprintf("�̽ӽ����������ض���\n");
					step ++;
				}
				break;
			case 3:
				if( is_check(EXTIO_CHECK_EMERGENCY) ){
					rt_kprintf("emergency checked\n");
					rt_kprintf("�����������ض��Ӽ��ɹ�\n");
					rt_kprintf("����ϲ�������\n");
					step ++;
				}
				break;
			case 4:
			{
				uint8_t i=0;
				while(i<24){
					if( is_smoke(i+1)==0 ){
						rt_kprintf("%d��û���ź�����\n",i+1);
						i++;
					}
					rt_thread_delay(1);
				}
				
				rt_kprintf("ȫ���̸��źż������\n");
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