#include "config.h"
#include "string.h"
#include "rtthread.h"
#include <dfs_posix.h> /* 当需要使用文件操作时，需要包含这个头文件 */

config_t config;

void config_save(void)
{
	config.crc = crc8_cal(&config,sizeof(config_t)-1);
	
	int fd = open("/flash/config.ini", O_WRONLY | O_CREAT);
	if( fd>0 ){
		write(fd,&config,sizeof(config_t));
		close(fd);
	}
}

#define MQTT_HOST				"post-cn-m7r1y1a7i0f.mqtt.aliyuncs.com"
#define MQTT_USER				"Signature|LTAI4GHiQYA8fE8z5ZpfKx7y|post-cn-m7r1y1a7i0f"
#define MQTT_PASSWORD			"gk5AfXablein+7usAxrKJPL5spU="
#define MQTT_ID					"GID_mymqttgruop@@@mqtt_9527_test1"
#define MQTT_PUBLISH_TOPIC		"ser/"
#define MQTT_SUBSCRIBE_TOPIC	"dev/"

void config_default(void)
{
	rt_memset(&config, 0, sizeof(config_t)-1 );
	
	strcpy(config.building_id,"12345678901");
	
	config.poll_time = 5;
	config.temp_threshold = 50;
	config.smoke_threshold = 20;
	
	config.smoke_num= 5;
	config.dev[0].floor  = 1;
	config.dev[0].room  = 1;
	config.dev[1].floor  = 2;
	config.dev[1].room  = 1;
	config.dev[2].floor  = 3;
	config.dev[2].room  = 1;
	config.dev[3].floor  = 4;
	config.dev[3].room  = 1;
	config.dev[4].floor  = 5;
	config.dev[4].room  = 1;
	
	strcpy(config.mqtt.host,MQTT_HOST);
	strcpy(config.mqtt.user,MQTT_USER);
	strcpy(config.mqtt.password,MQTT_PASSWORD);
	strcpy(config.mqtt.id,MQTT_ID);
	strcpy(config.mqtt.publish_topic,MQTT_PUBLISH_TOPIC);
	strcpy(config.mqtt.subscribe_topic,MQTT_SUBSCRIBE_TOPIC);
	
	config_save();
}

rt_err_t config_init(void)
{
	int fd = open("/flash/config.ini", O_RDWR);
	if( fd>=0 ){
		rt_memset(&config,0,sizeof(config_t));
		read(fd,&config,sizeof(config_t));
		if( config.crc == crc8_cal(&config,sizeof(config_t)-1) ){
			close(fd);
		}
		else{
			close(fd);
			config_default();
		}
	}
	else{
		config_default();
	}
}
INIT_ENV_EXPORT(config_init);