#include "nb-iot.h"
#include <rtthread.h>
#include <rtdevice.h>
#include "mqttclient.h"
#include "cJSON.h"
#include "at_device.h"
#include "netdev.h"
#include <board.h>
#include "config.h"
#include "lora.h"
#include "extio.h"
#include "hal.h"

#include <arpa/inet.h>
#include <netdev.h>

#define LOG_TAG              "nb.iot"
#include <ulog.h>

uint8_t mqtt_connect_sta = 0;
uint8_t	rssi = 0;

static mqtt_client_t *client = NULL;

time_t nb_iot_get_second(void)
{
	time_t now;
	now = time(RT_NULL);
	return now;
}

static void sub_topic_handle1(void* client, message_data_t* msg)
{
    (void) client;
    KAWAII_MQTT_LOG_I("%s:%d %s()...\ntopic: %s\nmessage:%s", __FILE__, __LINE__, __FUNCTION__, msg->topic_name, (char*)msg->message->payload);
	
	if( msg->message->payloadlen ){
		cJSON *item;
		cJSON* root = cJSON_Parse((char *)msg->message->payload);
		
		if( root ){
			cJSON *type = cJSON_GetObjectItem(root,"type"); //获取这个对象成员
			if( type ){
				if( type->valueint == CMD_DOWN_ALL_SMOKE ){
					item = cJSON_GetObjectItem(root,"temp_threshold");
					if( item )
						config.temp_threshold = item->valueint;
					item = cJSON_GetObjectItem(root,"smoke_threshold");
					if( item )
						config.smoke_threshold = item->valueint;
					item = cJSON_GetObjectItem(root,"poll_time");
					if( item )
						config.poll_time = item->valueint;
				}
				else if( type->valueint == CMD_DOWN_DOOR ){
					item = cJSON_GetObjectItem(root,"open");
					if( item ){
						extern uint8_t		remote_open;
						remote_open = item->valueint;
					}
				}
				else if( type->valueint == CMD_DOWN_GET_SMOKE ){
					
					item = cJSON_GetObjectItem(root,"smoke");
					if( item ){
						
						if( item->valueint == 1){
							smoke_status_t smoke_stus;
							
							for( uint8_t i=1; i<=SMOKE_NUM; i++ ){
								smoke_stus.floor = i;
								smoke_stus.room = 1;
								smoke_stus.smoke = 0;
								smoke_stus.temperature = 0;
								smoke_stus.battery = 0;
								smoke_stus.smoke_alarm = is_smoke(i);
								smoke_stus.temperature_alarm = 0;
								smoke_stus.connect = 1;
								
								smoke_stus.temp_threshold = config.smoke[i-1].temp_threshold;
								smoke_stus.smoke_threshold = config.smoke[i-1].smoke_threshold;
								smoke_stus.poll_time = config.smoke[i-1].poll_time;
//								nb_iot_publish_smoke(&smoke_stus);
								
								rt_thread_delay(250);
							}
						}
					}
				}
				else if( type->valueint == CMD_DOWN_SET_SMOKE ){
					item = cJSON_GetObjectItem(root,"temp_threshold");
					if( item ){
						config.temp_threshold = item->valueint;
					}
					item = cJSON_GetObjectItem(root,"smoke_threshold");
					if( item ){
						config.smoke_threshold = item->valueint;
					}
					item = cJSON_GetObjectItem(root,"poll_time");
					if( item ){
						config.poll_time = item->valueint;
					}
				}
				else if( type->valueint == CMD_DOWN_UPDATE ){
					item = cJSON_GetObjectItem(root,"update");
					if( item ){
						if( item->valueint==1 ){
							nb_iot_publish_status(STUS_OTA);
							rt_thread_delay(1000);
							ota_init();
						}
					}
				}
				else if( type->valueint == CMD_DOWN_SETTING ){
					item = cJSON_GetObjectItem(root,"user");
					if( item ){
						LOG_D("user %s",item->valuestring);
						rt_memset(config.mqtt.user,0,sizeof(((config_mqtt_t*)0)->user));
						strncpy(config.mqtt.user,item->valuestring,sizeof(((config_mqtt_t*)0)->user));
					}
					item = cJSON_GetObjectItem(root,"password");
					if( item ){
						LOG_D("password %s",item->valuestring);
						rt_memset(config.mqtt.password,0,sizeof(((config_mqtt_t*)0)->password));
						strncpy(config.mqtt.password,item->valuestring,sizeof(((config_mqtt_t*)0)->password));
					}
					item = cJSON_GetObjectItem(root,"id");
					if( item ){
						LOG_D("id %s",item->valuestring);
						rt_memset(config.mqtt.id,0,sizeof(((config_mqtt_t*)0)->id));
						strncpy(config.mqtt.id,item->valuestring,sizeof(((config_mqtt_t*)0)->id));
					}
					item = cJSON_GetObjectItem(root,"building_id");
					if( item ){
						LOG_D("building_id %s",item->valuestring);
						rt_memset(config.building_id,0,sizeof(((config_t*)0)->building_id));
						strncpy(config.building_id,item->valuestring,sizeof(((config_t*)0)->building_id));
					}
				}
				else if( type->valueint == CMD_DOWN_RESET ){
					item = cJSON_GetObjectItem(root,"reset");
					if( item ){
						if( item->valueint==1 ){
							LOG_D("reset cpu");
							nb_iot_publish_status(STUS_RST);
							rt_thread_delay(1000);
							extern void rt_hw_cpu_reset(void);
							rt_hw_cpu_reset();
						}
					}
				}
				else if( type->valueint == CMD_DOWN_STATUS ){
					item = cJSON_GetObjectItem(root,"status");
					if( item ){
						if( item->valueint==1 ){
							LOG_D("get status");
							nb_iot_publish_status(STUS_OK);
						}
					}
				}
			}
			
			cJSON_Delete(root);
		}
	}
}

int nb_iot_publish_smoke(smoke_status_t *smoke_stus)
{	
	int ret;
	if( mqtt_connect_sta==0 )
		return KAWAII_MQTT_CONNECT_FAILED_ERROR;
	
	cJSON * root =  cJSON_CreateObject();
 
	cJSON_AddItemToObject(root, "type", cJSON_CreateNumber(CMD_UP_SMOKE));
	cJSON_AddItemToObject(root, "device_type", cJSON_CreateNumber(DEVICE_TYPE));
	cJSON_AddItemToObject(root, "building_id", cJSON_CreateString(config.building_id));
    cJSON_AddItemToObject(root, "floor", cJSON_CreateNumber(smoke_stus->floor));
	cJSON_AddItemToObject(root, "room", cJSON_CreateNumber(smoke_stus->room));
	cJSON_AddItemToObject(root, "temperature", cJSON_CreateNumber(smoke_stus->temperature));
	cJSON_AddItemToObject(root, "smoke", cJSON_CreateNumber(smoke_stus->smoke));
	cJSON_AddItemToObject(root, "temp_alarm", cJSON_CreateNumber(smoke_stus->temperature_alarm));
	cJSON_AddItemToObject(root, "smoke_alarm", cJSON_CreateNumber(smoke_stus->smoke_alarm));
	cJSON_AddItemToObject(root, "battery", cJSON_CreateNumber(smoke_stus->battery));
	cJSON_AddItemToObject(root, "connet", cJSON_CreateNumber(smoke_stus->connect));
	
	cJSON_AddItemToObject(root, "temp_threshold", cJSON_CreateNumber(smoke_stus->temp_threshold));
	cJSON_AddItemToObject(root, "smoke_threshold", cJSON_CreateNumber(smoke_stus->smoke_threshold));
	cJSON_AddItemToObject(root, "poll_time", cJSON_CreateNumber(smoke_stus->poll_time));
	cJSON_AddItemToObject(root, "msg type", cJSON_CreateString("smoke"));

/*
	time_t now;
    now = time(RT_NULL);
	char timebuf[30]={0};
	sscanf(ctime(&now),"%[^\n]",timebuf);
	cJSON_AddItemToObject(root, "time", cJSON_CreateString(timebuf));
*/
	cJSON_AddItemToObject(root, "time", cJSON_CreateNumber(nb_iot_get_second()));
	
	char *buf = cJSON_Print(root);
	
	mqtt_message_t msg;
    memset(&msg, 0, sizeof(msg));

    msg.qos = QOS0;
    msg.payload = (void *)buf;
	
	ret = mqtt_publish(client, config.mqtt.publish_topic, &msg);
	
	cJSON_free(buf);
	cJSON_Delete(root);
	
	return ret;
}

int nb_iot_publish_building(building_status_t *building_stus)
{
	int ret;
	
	if( mqtt_connect_sta==0 )
		return KAWAII_MQTT_CONNECT_FAILED_ERROR;
	
	cJSON * root =  cJSON_CreateObject();
 
	cJSON_AddItemToObject(root, "type", cJSON_CreateNumber(CMD_UP_BUILDING));
	cJSON_AddItemToObject(root, "device_type", cJSON_CreateNumber(DEVICE_TYPE));
	cJSON_AddItemToObject(root, "building_id", cJSON_CreateString(config.building_id));
    cJSON_AddItemToObject(root, "alarm", cJSON_CreateNumber(building_stus->alarm));
	cJSON_AddItemToObject(root, "door_open", cJSON_CreateNumber(building_stus->door_open));
	cJSON_AddItemToObject(root, "cause", cJSON_CreateNumber(building_stus->cause));
	
  /*cJSON_AddItemToObject(root, "floor", cJSON_CreateNumber(building_stus->floor));
	cJSON_AddItemToObject(root, "room", cJSON_CreateNumber(building_stus->room));*/
	cJSON_AddItemToObject(root, "temperature", cJSON_CreateNumber(building_stus->temperature));
	cJSON_AddItemToObject(root, "smoke", cJSON_CreateNumber(building_stus->smoke));
	cJSON_AddItemToObject(root, "smoke_alarm", cJSON_CreateNumber(building_stus->smoke_alarm));
	cJSON_AddItemToObject(root, "temp_alarm", cJSON_CreateNumber(building_stus->temp_alarmm));
	
	cJSON_AddItemToObject(root, "msg type", cJSON_CreateString("building"));
	
/*
	time_t now;
    now = time(RT_NULL);
	char timebuf[30]={0};
	sscanf(ctime(&now),"%[^\n]",timebuf);
	cJSON_AddItemToObject(root, "time", cJSON_CreateString(timebuf));
*/

	cJSON_AddItemToObject(root, "time", cJSON_CreateNumber(nb_iot_get_second()));
	
	char *buf = cJSON_Print(root);
	
	mqtt_message_t msg;
    memset(&msg, 0, sizeof(msg));

    msg.qos = QOS1;
    msg.payload = (void *)buf;
	
	ret = mqtt_publish(client, config.mqtt.publish_topic, &msg);
	
	cJSON_free(buf);
	cJSON_Delete(root);
	
	return ret;
}

int nb_iot_publish_powerdown(uint8_t powerdown)
{
	int ret;
	
	if( mqtt_connect_sta==0 )
		return KAWAII_MQTT_CONNECT_FAILED_ERROR;
	
	cJSON * root =  cJSON_CreateObject();
 
	cJSON_AddItemToObject(root, "type", cJSON_CreateNumber(CMD_UP_POWER));
	cJSON_AddItemToObject(root, "device_type", cJSON_CreateNumber(DEVICE_TYPE));
	cJSON_AddItemToObject(root, "building_id", cJSON_CreateString(config.building_id));

	cJSON_AddItemToObject(root, "powerdown", cJSON_CreateNumber(powerdown));
	
	cJSON_AddItemToObject(root, "msg type", cJSON_CreateString("power down"));

/*
	time_t now;
    now = time(RT_NULL);
	char timebuf[30]={0};
	sscanf(ctime(&now),"%[^\n]",timebuf);
	cJSON_AddItemToObject(root, "time", cJSON_CreateString(timebuf));
*/

	cJSON_AddItemToObject(root, "time", cJSON_CreateNumber(nb_iot_get_second()));
	
	char *buf = cJSON_Print(root);
	
	mqtt_message_t msg;
    memset(&msg, 0, sizeof(msg));

    msg.qos = QOS1;
    msg.payload = (void *)buf;
	
	ret = mqtt_publish(client, config.mqtt.publish_topic, &msg);
	
	cJSON_free(buf);
	cJSON_Delete(root);
	
	return ret;
}

void nb_iot_set_rssi(uint8_t s)
{
	rssi = s;
}

int nb_iot_publish_startup(void)
{
	int ret;
	
	if( mqtt_connect_sta==0 )
		return KAWAII_MQTT_CONNECT_FAILED_ERROR;
	
	cJSON * root =  cJSON_CreateObject();
 
	cJSON_AddItemToObject(root, "type", cJSON_CreateNumber(CMD_UP_START));
	cJSON_AddItemToObject(root, "device_type", cJSON_CreateNumber(DEVICE_TYPE));
	cJSON_AddItemToObject(root, "building_id", cJSON_CreateString(config.building_id));

	extern const char *main_version(void);
	extern const char *main_compile(void);
	
	cJSON_AddItemToObject(root, "version", cJSON_CreateString(main_version()));
	cJSON_AddItemToObject(root, "compile", cJSON_CreateString(main_compile()));
	struct netdev *netdev = RT_NULL;
	netdev = netdev_get_first_by_flags(NETDEV_FLAG_UP);
	cJSON_AddItemToObject(root, "ip_addrress", cJSON_CreateString(inet_ntoa(netdev->ip_addr.addr)));
	
	cJSON_AddItemToObject(root, "rssi", cJSON_CreateNumber(rssi));
	
	char id[10]={0};
	uint8_t *un_id = hal_read_uniqueid();
	uint32_t crc = crc32_cal(un_id,STM32_UNIQUE_ID_SIZE);
	snprintf(id,10,"%08X",crc);
	cJSON_AddItemToObject(root, "serial_number", cJSON_CreateString(id));
	
	cJSON_AddItemToObject(root, "msg type", cJSON_CreateString("start up"));

/*
	time_t now;
    now = time(RT_NULL);
	char timebuf[30]={0};
	sscanf(ctime(&now),"%[^\n]",timebuf);
	cJSON_AddItemToObject(root, "time", cJSON_CreateString(timebuf));
*/

	cJSON_AddItemToObject(root, "time", cJSON_CreateNumber(nb_iot_get_second()));
	
	char *buf = cJSON_Print(root);
	
	mqtt_message_t msg;
    memset(&msg, 0, sizeof(msg));

    msg.qos = QOS1;
    msg.payload = (void *)buf;
	
	ret = mqtt_publish(client, config.mqtt.publish_topic, &msg);
	
	cJSON_free(buf);
	cJSON_Delete(root);
	
	return ret;
}

//1-OK
int nb_iot_publish_status(uint8_t status)
{
	int ret;
	
	if( mqtt_connect_sta==0 )
		return KAWAII_MQTT_CONNECT_FAILED_ERROR;
	
	cJSON * root =  cJSON_CreateObject();
 
	cJSON_AddItemToObject(root, "type", cJSON_CreateNumber(CMD_UP_STATUS));
	cJSON_AddItemToObject(root, "device_type", cJSON_CreateNumber(DEVICE_TYPE));
	cJSON_AddItemToObject(root, "building_id", cJSON_CreateString(config.building_id));
	cJSON_AddItemToObject(root, "status", cJSON_CreateNumber(status));

	extern const char *main_version(void);
	extern const char *main_compile(void);
	
	cJSON_AddItemToObject(root, "version", cJSON_CreateString(main_version()));
	cJSON_AddItemToObject(root, "compile", cJSON_CreateString(main_compile()));
	struct netdev *netdev = RT_NULL;
	netdev = netdev_get_first_by_flags(NETDEV_FLAG_UP);
	cJSON_AddItemToObject(root, "ip_addrress", cJSON_CreateString(inet_ntoa(netdev->ip_addr.addr)));
	
	cJSON_AddItemToObject(root, "rssi", cJSON_CreateNumber(rssi));
	
	char id[10]={0};
	uint8_t *un_id = hal_read_uniqueid();
	uint32_t crc = crc32_cal(un_id,STM32_UNIQUE_ID_SIZE);
	snprintf(id,10,"%08X",crc);
	cJSON_AddItemToObject(root, "serial_number", cJSON_CreateString(id));
	
	cJSON_AddItemToObject(root, "msg type", cJSON_CreateString("status"));
	
/*
	time_t now;
    now = time(RT_NULL);
	char timebuf[30]={0};
	sscanf(ctime(&now),"%[^\n]",timebuf);
	cJSON_AddItemToObject(root, "time", cJSON_CreateString(timebuf));
*/

	cJSON_AddItemToObject(root, "time", cJSON_CreateNumber(nb_iot_get_second()));
	
	char *buf = cJSON_Print(root);
	
	mqtt_message_t msg;
    memset(&msg, 0, sizeof(msg));

    msg.qos = QOS1;
    msg.payload = (void *)buf;
	
	ret = mqtt_publish(client, config.mqtt.publish_topic, &msg);
	
	cJSON_free(buf);
	cJSON_Delete(root);
	
	return ret;
}

void mqtt_publish_smoke(void)
{
	smoke_status_t smoke_stus;
	smoke_stus.floor = 1;
	smoke_stus.room = 1;
	smoke_stus.temperature = 25;
	smoke_stus.smoke = 12;
	smoke_stus.temperature_alarm = 0;
	smoke_stus.smoke_alarm = 0;
	smoke_stus.battery = 98;
	smoke_stus.connect = 1;
	
	nb_iot_publish_smoke(&smoke_stus);
}
MSH_CMD_EXPORT(mqtt_publish_smoke,"mqtt_publish_smoke");


void mqtt_publish_building(void)
{
	building_status_t building_stus;
	
	building_stus.alarm = 0;
	building_stus.door_open = 0;
	building_stus.cause = 0;
	
	nb_iot_publish_building(&building_stus);
}
MSH_CMD_EXPORT(mqtt_publish_building,"mqtt_publish_building");

#define GREEN_LED_PIN    GET_PIN(C, 5)
void nb_iot_task(void *parameter)
{	
	
	config_wait_config_sem();
	
	rt_pin_mode(GREEN_LED_PIN, PIN_MODE_OUTPUT);
	
	struct at_device *device = RT_NULL;
	device  = at_device_get_by_name(AT_DEVICE_NAMETYPE_NETDEV, "bc26");
	
	while( netdev_is_link_up(device->netdev)==0 ){
		rt_pin_write(GREEN_LED_PIN, PIN_LOW);
		rt_thread_delay(100);
		rt_pin_write(GREEN_LED_PIN, PIN_HIGH);
		rt_thread_delay(100);
	}
	
	for( uint8_t i=0; i<10; i++ ){
		rt_pin_write(GREEN_LED_PIN, PIN_LOW);
		rt_thread_delay(250);
		rt_pin_write(GREEN_LED_PIN, PIN_HIGH);
		rt_thread_delay(250);
	}
		
	mqtt_log_init();
	
	client = mqtt_lease();
	
	mqtt_set_host(client, config.mqtt.host);
    mqtt_set_port(client, "1883");
    mqtt_set_user_name(client, config.mqtt.user);
    mqtt_set_password(client, config.mqtt.password);
    mqtt_set_client_id(client, config.mqtt.id);
    mqtt_set_clean_session(client, 1);

	int sta = mqtt_connect(client);
    if( sta == KAWAII_MQTT_SUCCESS_ERROR){
		mqtt_connect_sta = 1;
		LOG_D( "mqtt conect successful\r\n" );
		rt_pin_write(GREEN_LED_PIN, PIN_LOW);
	}
	else{
		LOG_E( "mqtt conect failed %d\r\n",sta );
		rt_pin_write(GREEN_LED_PIN, PIN_HIGH);
	}
    
	char topic_buf[50]={0};
	rt_snprintf(topic_buf,50,"%s%s",config.mqtt.subscribe_topic,config.building_id);
    mqtt_subscribe(client, topic_buf, QOS0, sub_topic_handle1);
	
	uint8_t reconnect_time=0;
//	uint8_t	ota_check=1;
	
	uint8_t *un_id = hal_read_uniqueid();
	uint16_t crc = crc16_cal(un_id,STM32_UNIQUE_ID_SIZE);
	uint8_t check_minute,check_second,check_hour=0;
	check_minute = (float)(crc>>8)/255.0*59.0;
	check_second = (float)(crc&0xff)/255.0*59.0;;
	
	if( mqtt_connect_sta ){
		nb_iot_publish_startup();
	}
	while(1)
	{
		rt_thread_delay(1000);
		
//		time_t now;
        /* output current time */
//        now = time(RT_NULL);
//		struct tm*  tm=localtime(&now);
		
//		if( (tm->tm_year+1900)>2021 && tm->tm_hour==check_hour && tm->tm_min==check_minute && tm->tm_sec==check_second ){
//			ota_check = 1;
//		}
//		
//		if( ota_check ){
//			ota_check = 0;
//			ota_init();
//		}
		
		if( mqtt_connect_sta==0 ){
			if( reconnect_time++ > 250 ){
//				rt_hw_cpu_reset();
				hal_app_reset();
			}
		}
		else{
			reconnect_time = 0;
		}
	}
}

rt_err_t nb_iot_init(void)
{
	rt_err_t ret = RT_EOK;
	
	/* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("nb_iot", nb_iot_task, RT_NULL, 2048, 25, 10);
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
INIT_APP_EXPORT(nb_iot_init);