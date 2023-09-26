#include "nb-iot.h"
#include "rtthread.h"
#include "mqttclient.h"
#include "cJSON.h"
#include "at_device.h"
#include "netdev.h"
#include <board.h>
#include "config.h"

#define LOG_TAG              "nb.iot"
#include <ulog.h>

static mqtt_client_t *client = NULL;

static void sub_topic_handle1(void* client, message_data_t* msg)
{
    (void) client;
//    KAWAII_MQTT_LOG_I("%s:%d %s()...\ntopic: %s\nmessage:%s", __FILE__, __LINE__, __FUNCTION__, msg->topic_name, (char*)msg->message->payload);
	
//	if( msg->message->payloadlen ){
//		cJSON *item;
//		cJSON* root = cJSON_Parse((char *)msg->message->payload);
//		
//		cJSON *alarm_param = cJSON_GetObjectItem(root,"alarm_param"); //获取这个对象成员
//		item = cJSON_GetObjectItem(alarm_param,"temp_threshold");
//		config.temp_threshold = item->valueint;
//		item = cJSON_GetObjectItem(alarm_param,"smoke_threshold");
//		config.smoke_threshold = item->valueint;
//		item = cJSON_GetObjectItem(alarm_param,"poll_time");
//		config.poll_time = item->valueint;
//		
//		cJSON *control = cJSON_GetObjectItem(root,"control"); //获取这个对象成员
//		item = cJSON_GetObjectItem(control,"open");
//		extern uint8_t		remote_open;
//		remote_open = item->valueint;
//		
//		cJSON_Delete(root);
//	}
}

void nt_iot_publish_smoke(smoke_status_t *smoke_stus)
{
	cJSON * root =  cJSON_CreateObject();
    cJSON * floor =  cJSON_CreateObject();
    cJSON * building =  cJSON_CreateObject();
 
    cJSON_AddItemToObject(root, "floor_status", floor);//root节点下添加floor_status节点
    cJSON_AddItemToObject(floor, "floor", cJSON_CreateNumber(smoke_stus->floor));
	cJSON_AddItemToObject(floor, "room", cJSON_CreateNumber(smoke_stus->room));
	cJSON_AddItemToObject(floor, "temperature", cJSON_CreateNumber(smoke_stus->temperature));
	cJSON_AddItemToObject(floor, "smoke", cJSON_CreateNumber(smoke_stus->smoke));
	cJSON_AddItemToObject(floor, "smoke", cJSON_CreateNumber(smoke_stus->smoke));
	cJSON_AddItemToObject(floor, "temperature_alarm", cJSON_CreateNumber(smoke_stus->temperature_alarm));
	cJSON_AddItemToObject(floor, "smoke_alarm", cJSON_CreateNumber(smoke_stus->smoke_alarm));
	cJSON_AddItemToObject(floor, "battery", cJSON_CreateNumber(smoke_stus->battery));
	cJSON_AddItemToObject(floor, "connet", cJSON_CreateNumber(smoke_stus->connect));
	
	cJSON_AddItemToObject(root, "building_status", building);//root节点下添加building_status节点
	cJSON_AddItemToObject(building, "alarm", cJSON_CreateNumber(smoke_stus->alarm));
	cJSON_AddItemToObject(building, "door_open", cJSON_CreateNumber(smoke_stus->door_open));
	cJSON_AddItemToObject(building, "cause", cJSON_CreateNumber(smoke_stus->cause));
	
	char *buf = cJSON_Print(root);
	
	mqtt_message_t msg;
    memset(&msg, 0, sizeof(msg));

    msg.qos = QOS0;
    msg.payload = (void *)buf;
	mqtt_publish(client, config.mqtt.publish_topic, &msg);
	
	cJSON_free(buf);
	cJSON_Delete(root);
}

void nt_iot_publish_search(void)
{
	cJSON * root =  cJSON_CreateObject();
 
    cJSON_AddItemToObject(root, "search_status", root);//root节点下添加floor_status节点
	
	char *str = rt_malloc(5*SMOKE_NUM);
	if( str ){
		rt_memset(str,0,5*SMOKE_NUM);
		for( uint8_t i=0; i<config.smoke_num; i++ ){
			rt_sprintf(str+rt_strlen(str),"%02d%02d,",config.dev[i].floor,config.dev[i].room);
		}
	}
	cJSON_AddItemToObject(root, "num", cJSON_CreateNumber(config.smoke_num));
	cJSON_AddItemToObject(root, "result", cJSON_CreateString(str));
	
	char *buf = cJSON_Print(root);
	
	mqtt_message_t msg;
    memset(&msg, 0, sizeof(msg));

    msg.qos = QOS0;
    msg.payload = (void *)buf;
	mqtt_publish(client, config.mqtt.publish_topic, &msg);
	
	rt_free(str);
	cJSON_free(buf);
	cJSON_Delete(root);
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
	smoke_stus.alarm = 0;
	smoke_stus.door_open = 0;
	smoke_stus.cause = 0;
	
	nt_iot_publish_smoke(&smoke_stus);
}
MSH_CMD_EXPORT(mqtt_publish_smoke,"mqtt_publish_smoke");

void mqtt_publish_search(void)
{
	config.smoke_num = 8;
	for( uint8_t i=0; i<8; i++ ){
		config.dev[i].floor = i+1;
		config.dev[i].room = 01;
	}
	nt_iot_publish_search();
}
MSH_CMD_EXPORT(mqtt_publish_search,"mqtt_publish_search");

#define GREEN_LED_PIN    GET_PIN(C, 5)
void nb_iot_task(void *parameter)
{	
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
		LOG_I( "mqtt conect successful\r\n" );
		rt_pin_write(GREEN_LED_PIN, PIN_LOW);
	}
	else{
		LOG_E( "mqtt conect failed %d\r\n",sta );
		rt_pin_write(GREEN_LED_PIN, PIN_HIGH);
	}
    
    mqtt_subscribe(client, config.mqtt.subscribe_topic, QOS0, sub_topic_handle1);
	
	while(1)
	{
		rt_thread_delay(10);
	}
}

rt_err_t nb_iot_init(void)
{
	rt_err_t ret = RT_EOK;
	
	/* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("nb_iot", nb_iot_task, RT_NULL, 1024, 25, 10);
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
INIT_ENV_EXPORT(nb_iot_init);