
#include "rtthread.h"
#include "board.h"
#include "ota.h"
#include "config.h"
#include "stdio.h"
#include "cJSON.h"
#include "dfs_posix.h"

#define LOG_TAG              "nb.iot"
#include <ulog.h>

#define UPDATE_OTA_PIN		GET_PIN(B, 0)

static uint8_t	ota_running=0;
uint32_t ota_compile_time(char const *time) { 
    char s_month[5];
    int month,day,year;
    struct tm t = {0};
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
 
    sscanf(time,"%s %d %d",s_month,&day,&year);
 
    month = (strstr(month_names,s_month)-month_names)/3;
 
    t.tm_mon = month;
    t.tm_mday = day;
    t.tm_year = year;
	
	uint32_t ver=0;
	ver = year*1000000;
	ver += (month+1)*10000;
	ver += day*100;
	ver += 99;
 
    return ver;
}

void ota_upload_file(void)
{
	webclient_post_file("http://8.129.145.22:8080/upload/test.ini", "/flash/config.ini","hello world");
}
MSH_CMD_EXPORT(ota_upload_file,"mqtt_publish_building");

static uint32_t		new_ver;
void ota_task(void *parameter)
{
//	while(1)
	{
		char ver_uri[50]={0};
		uint32_t ver=0;
		char fm_file[100]={0};
		snprintf(ver_uri,50,"%s/ver.json",config.update_uri);
		
		if ( webclient_get_file(ver_uri, "/flash/new_ver.json") ==0 ){
			
			int fd;
			fd = open("/flash/new_ver.json",O_RDONLY);
			if( fd>=0 ){
				char buf[100]={0};
				int size;
				size = read(fd, buf, sizeof(buf));
				if( size ){
					cJSON *item;
					cJSON* root = cJSON_Parse((char *)buf);
					
					if( root ){
						cJSON *date = cJSON_GetObjectItem(root,"ver"); //获取这个对象成员
						if( date ){
							ver =  (uint32_t)date->valueint;
						}
						
						if( ver>config.ver && ver>ota_compile_time(__DATE__) ){
							
							cJSON *file = cJSON_GetObjectItem(root,"file"); //获取这个对象成员
							if( file ){
								rt_thread_delay(1000);
								snprintf(fm_file,sizeof(fm_file),"%s/%s",config.update_uri,file->valuestring);
								new_ver = ver;
								http_ota_fw_download(fm_file);
							}
						}
						else{
							config.ver = ver;
							LOG_D( "no new version:%d\r\n",config.ver );
							rt_pin_write(UPDATE_OTA_PIN, PIN_HIGH);
						}
						
						cJSON_Delete(root);
					}
				}
				close(fd);
//				unlink("/flash/new_ver.json");
			}
		}
		else{
			LOG_E( "can not get the version file\r\n");
		}
	}
	rt_pin_write(UPDATE_OTA_PIN, PIN_HIGH);
	ota_running = 0;
}

void ota_save_ver(void)
{
	config.ver = new_ver;
	rt_thread_delay(1500);
}

rt_err_t ota_init(void)
{
	if( ota_running )
		return RT_ERROR;
	
	ota_running = 1;
	rt_err_t ret = RT_EOK;
	
	rt_pin_mode(UPDATE_OTA_PIN, PIN_MODE_OUTPUT);
	rt_pin_write(UPDATE_OTA_PIN, PIN_LOW);
	
	static time_t time;
	time = ota_compile_time(__DATE__);
	
	/* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("ota", ota_task, RT_NULL, 2048, 25, 10);
    /* 创建成功则启动线程 */
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        ret = RT_ERROR;
    }
}
MSH_CMD_EXPORT(ota_init,"set update uri -- config_set_update_uri <uri>");
