#include "config.h"
#include "string.h"
#include "rtthread.h"
#include "macro_def.h"
#include <dfs_posix.h> /* 当需要使用文件操作时，需要包含这个头文件 */
#include "hal.h"
#include "fal.h"

#define LOG_TAG              "config"
#include <ulog.h>

config_t config;

void config_save(void)
{
	config.crc = crc8_cal(&config,sizeof(config_t)-1);
	
	const struct fal_partition *part;
	part = fal_partition_find("config");
	if (part == RT_NULL)
	{		
		LOG_D("Partition[config] not found.");
		return ;
	}
	
	if (fal_partition_erase(part, 0, 128*1024) < 0)
	{
		LOG_D("Partition[config] erase failed!");
		return;
	}
	
	if (fal_partition_write(part, 0, (const uint8_t*)&config, sizeof(config_t)) < 0)
	{
		LOG_D("Partition[config] write failed!");
		return;
	}
}

#define MQTT_HOST				"post-cn-zvp255j8c01.mqtt.aliyuncs.com"
#define MQTT_USER				"Signature|LTAI5tCymxkCDJzyhnisq9G4|post-cn-zvp255j8c01"
#define MQTT_PASSWORD			"hCRfYt/oTH9zhcQps8y/1Fz7Oh8="
#define MQTT_ID					"GID_osomqtt@@@1_1_v1_440309010100001"
#define MQTT_PUBLISH_TOPIC		"ser"
#define MQTT_SUBSCRIBE_TOPIC	"dev/"
#define BUILDING_ID				"440309010100001"
#define UPDATE_URI				"http://8.129.145.22:8080/pke_fire"
#define VERSION					2021041512

void config_default(void)
{
	rt_memset(&config, 0, sizeof(config_t)-1 );
	
	uint8_t *un_id = hal_read_uniqueid();
	uint32_t crc = crc32_cal(un_id,STM32_UNIQUE_ID_SIZE);
	
	snprintf(config.building_id,15,"%08X",crc);
	//strcpy(config.building_id,BUILDING_ID);
	
	config.poll_time = 30;
	config.temp_threshold = 500;
	config.smoke_threshold = 10;
	
	config.lora_freq = 	433300;
	config.ver =		VERSION;
	
	strcpy(config.update_uri,UPDATE_URI);
	
	strcpy(config.mqtt.host,MQTT_HOST);
	strcpy(config.mqtt.user,MQTT_USER);
	strcpy(config.mqtt.password,MQTT_PASSWORD);
	strcpy(config.mqtt.id,MQTT_ID);
	strcpy(config.mqtt.publish_topic,MQTT_PUBLISH_TOPIC);
	strcpy(config.mqtt.subscribe_topic,MQTT_SUBSCRIBE_TOPIC);
	
	config_save();
}

void config_timming_save(void)
{
	uint8_t crc = crc8_cal(&config,sizeof(config_t)-1);
	if( crc != config.crc ){
//		int fd = open("/flash/config.ini", O_WRONLY | O_CREAT);
//		if( fd>0 ){
//			config.crc = crc;
//			write(fd,&config,sizeof(config_t));
//			close(fd);
//		}
		config.crc = crc;
		
		const struct fal_partition *part;
		part = fal_partition_find("config");
		if (part == RT_NULL)
		{		
			LOG_D("Partition[config] not found.");
			return ;
		}
		
		if (fal_partition_erase(part, 0, 128*1024) < 0)
		{
			LOG_D("Partition[config] erase failed!");
			return;
		}
		
		if (fal_partition_write(part, 0, (const uint8_t*)&config, sizeof(config_t)) < 0)
		{
			LOG_D("Partition[config] write failed!");
			return;
		}
	}
}

static rt_sem_t config_sem = RT_NULL;
rt_err_t config_sem_init(void)
{
	config_sem = rt_sem_create("config", 0, RT_IPC_FLAG_FIFO);
	if (config_sem == RT_NULL)
    {
        LOG_I("create config semaphore failed.\n");
        return RT_ERROR;
    }
    else
    {
        LOG_I("create done. config semaphore value = 0.\n");
		return RT_EOK;
    }
}
INIT_ENV_EXPORT(config_sem_init);

rt_err_t config_post_config_sem(void)
{
	return rt_sem_release(config_sem);
}

rt_err_t config_wait_config_sem(void)
{
	rt_sem_take(config_sem, RT_WAITING_FOREVER);
	rt_sem_delete(config_sem);
}

rt_err_t config_init(void)
{
	uint8_t cnt=0;
	const struct fal_partition *part;
	part = fal_partition_find("config");
	if (part == RT_NULL)
	{		
		LOG_D("Partition[config] not found.");
		return RT_ERROR;
	}
	
	if (fal_partition_read(part, 0, (uint8_t*)&config, sizeof(config_t)) < 0)
	{
		LOG_D("Partition[config] read failed!");
		LOG_D("config set the default!");
		config_default();
	}
	else{
		uint8_t crc = crc8_cal(&config,sizeof(config_t)-1);
		if( crc != config.crc ){
			config_default();
		}
	}
	return config_post_config_sem();
}
INIT_APP_EXPORT(config_init);

void config_set_host(int argc, char *argv[])
{
	if( argc == 1 ){
		rt_kprintf("mqtt host name:%s\r\n",config.mqtt.host);
	}
	else if( argc==2 ){
		rt_memset(config.mqtt.host,0,sizeof(((config_mqtt_t*)0)->host));
		strncpy(config.mqtt.host,argv[1],sizeof(((config_mqtt_t*)0)->host));
		rt_kprintf("set new mqtt host name:%s\r\n",config.mqtt.host);
	}
}
MSH_CMD_EXPORT(config_set_host,"set mqtt host name -- config_set_host host_name");

void config_set_user(int argc, char *argv[])
{
	if( argc == 1 ){
		rt_kprintf("mqtt host user:%s\r\n",config.mqtt.user);
	}
	else if( argc==2 ){
		rt_memset(config.mqtt.user,0,sizeof(((config_mqtt_t*)0)->user));
		strncpy(config.mqtt.user,argv[1],sizeof(((config_mqtt_t*)0)->user));
		rt_kprintf("set new mqtt user name:%s\r\n",config.mqtt.user);
	}
}
MSH_CMD_EXPORT(config_set_user,"set mqtt user name -- config_set_user user_name");

void config_set_passwd(int argc, char *argv[])
{
	if( argc == 1 ){
		rt_kprintf("mqtt host password:%s\r\n",config.mqtt.password);
	}
	else if( argc==2 ){
		rt_memset(config.mqtt.password,0,sizeof(((config_mqtt_t*)0)->password));
		strncpy(config.mqtt.password,argv[1],sizeof(((config_mqtt_t*)0)->password));
		rt_kprintf("set new mqtt passwrod:%s\r\n",config.mqtt.password);
	}
}
MSH_CMD_EXPORT(config_set_passwd,"set mqtt password -- config_set_passwd password");

void config_set_id(int argc, char *argv[])
{
	if( argc == 1 ){
		rt_kprintf("mqtt host id:%s\r\n",config.mqtt.id);
	}
	else if( argc==2 ){
		rt_memset(config.mqtt.id,0,sizeof(((config_mqtt_t*)0)->id));
		strncpy(config.mqtt.id,argv[1],sizeof(((config_mqtt_t*)0)->id));
		rt_kprintf("set new mqtt client id:%s\r\n",config.mqtt.id);
	}
}
MSH_CMD_EXPORT(config_set_id,"set mqtt id -- config_set_id id name");

void config_set_publish_topic(int argc, char *argv[])
{
	if( argc == 1 ){
		rt_kprintf("mqtt publish topic:%s\r\n",config.mqtt.publish_topic);
	}
	else if( argc==2 ){
		rt_memset(config.mqtt.publish_topic,0,sizeof(((config_mqtt_t*)0)->publish_topic));
		strncpy(config.mqtt.publish_topic,argv[1],sizeof(((config_mqtt_t*)0)->publish_topic));
		rt_kprintf("set new mqtt passwrod:%s\r\n",config.mqtt.publish_topic);
	}
}
MSH_CMD_EXPORT(config_set_publish_topic,"set mqtt publish topic -- config_set_publish_topic topic name");

void config_set_subscribe_topic(int argc, char *argv[])
{
	if( argc == 1 ){
		rt_kprintf("mqtt subsrcibe topic:%s\r\n",config.mqtt.subscribe_topic);
	}
	else if( argc==2 ){
		rt_memset(config.mqtt.subscribe_topic,0,sizeof(((config_mqtt_t*)0)->subscribe_topic));
		strncpy(config.mqtt.subscribe_topic,argv[1],sizeof(((config_mqtt_t*)0)->subscribe_topic));
		rt_kprintf("set new mqtt passwrod:%s\r\n",config.mqtt.subscribe_topic);
	}
}
MSH_CMD_EXPORT(config_set_subscribe_topic,"set mqtt subscribe topic -- config_set_subscribe_topic topic name");

void config_set_buildingid(int argc, char *argv[])
{
	if( argc == 1 ){
		rt_kprintf("building id:%s\r\n",config.building_id);
	}
	else if( argc==2 ){
		rt_memset(config.building_id,0,sizeof(((config_t*)0)->building_id));
		strncpy(config.building_id,argv[1],sizeof(((config_t*)0)->building_id));
		rt_kprintf("set new building id:%s\r\n",config.building_id);
	}
}
MSH_CMD_EXPORT(config_set_buildingid,"set building id -- config_set_buildingid ");

void config_mqtt_list(int argc, char *argv[])
{
	if( argc == 1 ){
		rt_kprintf("mqtt host name:			%s\r\n",config.mqtt.host);
		rt_kprintf("mqtt host user:			%s\r\n",config.mqtt.user);
		rt_kprintf("mqtt host password:		%s\r\n",config.mqtt.password);
		rt_kprintf("mqtt host id:			%s\r\n",config.mqtt.id);
		rt_kprintf("mqtt publish topic:		%s\r\n",config.mqtt.publish_topic);
		rt_kprintf("mqtt subsrcibe topic:		%s\r\n",config.mqtt.subscribe_topic);
	}
}
MSH_CMD_EXPORT(config_mqtt_list,"list all the mqtt info -- config_mqtt_list ");

void config_set_connect(int argc, char *argv[])
{
	if( argc==5 ){
		
		//设置楼栋ID
		rt_memset(config.building_id,0,sizeof(((config_t*)0)->building_id));
		strncpy(config.building_id,argv[1],sizeof(((config_t*)0)->building_id));
		//设置client id
		rt_memset(config.mqtt.id,0,sizeof(((config_mqtt_t*)0)->id));
		strncpy(config.mqtt.id,argv[2],sizeof(((config_mqtt_t*)0)->id));
		//设置user
		rt_memset(config.mqtt.user,0,sizeof(((config_mqtt_t*)0)->user));
		strncpy(config.mqtt.user,argv[3],sizeof(((config_mqtt_t*)0)->user));
		//设置密码
		rt_memset(config.mqtt.password,0,sizeof(((config_mqtt_t*)0)->password));
		strncpy(config.mqtt.password,argv[4],sizeof(((config_mqtt_t*)0)->password));
		
		rt_kprintf("set the building id:		%s\r\n",config.building_id);
		rt_kprintf("set the client id:		%s\r\n",config.mqtt.id);
		rt_kprintf("set the user name:		%s\r\n",config.mqtt.user);
		rt_kprintf("set the passwrod:		%s\r\n",config.mqtt.password);
	}
}
MSH_CMD_EXPORT(config_set_connect,"set the connect info -- config_set_connect ");

void config_set_smoke(int argc, char *argv[])
{
	if( argc == 1 ){
		rt_kprintf("tempature threshold:	%d\r\n",config.temp_threshold);
		rt_kprintf("smoke threshold:	%d\r\n",config.smoke_threshold);
		rt_kprintf("smoke up time:		%d seconds\r\n",config.poll_time);
	}
	else if( argc==4 ){
		config.temp_threshold = atoi(argv[1]);
		config.smoke_threshold = atoi(argv[2]);
		config.poll_time = atoi(argv[3]);
		
		rt_kprintf("set new smoke paramter:\r\n");
		rt_kprintf("tempature threshold:	%d\r\n",config.temp_threshold);
		rt_kprintf("smoke threshold:	%d\r\n",config.smoke_threshold);
		rt_kprintf("smoke up time:		%d seconds\r\n",config.poll_time);
	}
}
MSH_CMD_EXPORT(config_set_smoke,"set smoke parameter -- config_set_smoke  temp smoke time");

void config_set_lora_freq(int argc, char *argv[])
{
	if( argc == 1 ){
		rt_kprintf("lora frequency:	%d\r\n",config.lora_freq);
	}
	else if( argc==2 ){
		config.lora_freq = atoi(argv[1]);
		
		rt_kprintf("set new lora frequency:%d KHz\r\n",config.lora_freq);
	}
}
MSH_CMD_EXPORT(config_set_lora_freq,"set lora frequency -- config_set_lora_freq  KHz");

void config_set_lora_updatetime(int argc, char *argv[])
{
	if( argc == 1 ){
		rt_kprintf("lora update time:	%d\r\n",config.poll_time);
	}
	else if( argc==2 ){
		config.poll_time = atoi(argv[1]);
		
		rt_kprintf("set new update time:%d second\r\n",config.poll_time);
	}
}
MSH_CMD_EXPORT(config_set_lora_updatetime,"set lora update time -- config_set_lora_updatetime second");

void config_set_update_uri(int argc, char *argv[])
{
	if( argc == 1 ){
		rt_kprintf("update uri:	%s\r\n",config.update_uri);
	}
	else if( argc==2 ){
		strcpy(config.update_uri,argv[1]);
		
		rt_kprintf("set new update uri:%s \r\n",config.update_uri);
	}
}
MSH_CMD_EXPORT(config_set_update_uri,"set update uri -- config_set_update_uri <uri>");
