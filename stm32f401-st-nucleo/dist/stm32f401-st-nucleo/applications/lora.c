#include "lora.h"
#include "crc8.h"
#include "rtthread.h"
#include <rtdevice.h>
#include "protocol.h"
#include "ulog.h"
#include "config.h"
#include "string.h"
#include <board.h>
#include "lora_model.h"

#define LORA_UART_NAME       "uart6"

#define SETA_PIN		GET_PIN(C, 8)
#define SETB_PIN		GET_PIN(B, 14)

#define YM401_MODE3()           do{rt_pin_write(SETA_PIN, PIN_LOW);    rt_pin_write(SETB_PIN, PIN_LOW);}while(0)
#define YM401_MODE2()           do{rt_pin_write(SETA_PIN, PIN_LOW);    rt_pin_write(SETB_PIN, PIN_HIGH);}while(0)
#define YM401_MODE1()           do{rt_pin_write(SETA_PIN, PIN_HIGH);    rt_pin_write(SETB_PIN, PIN_LOW);}while(0)
#define YM401_MODE0()           do{rt_pin_write(SETA_PIN, PIN_HIGH);    rt_pin_write(SETB_PIN, PIN_HIGH);}while(0)

//前6字字符识别头，第7字节开始数据
//93 版本	
//06 9c 94 频点433.3	
//02 空中速率2.6K	
//07		20dbm
//03 串口波特率9600
//00 无校验
//00 唤醒时间50ms
//00 cad时间50ms

//const uint8_t ym401_setup[] = {0xff, 0x56, 0xae, 0x35, 0xa9, 0x55, 0x93, 0x06, 0x9c, 0x94, 0x02, 0x07, 0x03, 0x00, 0x00, 0x02};
const lora_setup_t lora_setup_default = 
{
	.header_id = LORA_ID,
	.version = LORA_VER,
	.frequency = lora_freq(433300),
	.air_rate = K0_81,
	.tx_power = tx_9db,
	.baudrate = br_9600,
	.verify = vf_none,
	.wakeup_time = wt_50ms,
	.cad_time = fs_50ms
};
																
const uint8_t ym401_setup_read[] =
{
    0xFF, 0x56, 0xAE, 0x35, 0xA9, 0x55, 0xF0
};

/* 用于接收消息的信号量 */
static struct rt_semaphore rx_sem;
static rt_device_t serial;
lora_state_t lora_sta;

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_sem_release(&rx_sem);

    return RT_EOK;
}

smoke_dev_t *lora_search_smoke(uint8_t floor, uint8_t room)
{
	if( floor==0 && room==0 )
		return RT_NULL;
	
	smoke_dev_t *smoke_ptr = lora_sta.smoke_dev;
	for( uint8_t i=0; i<SMOKE_NUM; i++ ){
		if( smoke_ptr->floor == floor && smoke_ptr->room==room ){
			return smoke_ptr;
		}
		//加入新烟感
		if( i>=lora_sta.smoke_num ){
			rt_memset(smoke_ptr,0,sizeof(smoke_dev_t));
			smoke_ptr->floor = floor;
			smoke_ptr->room = room;
			
			lora_sta.smoke_num++;
			
			return smoke_ptr;
		}
		smoke_ptr++;
	}
	return RT_NULL;
}

void lora_send(uint8_t floor, uint8_t room, uint8_t cmd, uint8_t *data, uint8_t  len)
{
	uint8_t *buf = rt_malloc(proto_len(len));
	protocol_t *pro = (protocol_t *)buf;
	pro->header1 = 0xa5;
	pro->header2 = 0x5a;
	pro->msg_id = lora_sta.msgid;
	pro->dir = DIR_TO_SMOKE;
	pro->floor = floor;
	pro->room = room;
	rt_memcpy(pro->building_id,config.building_id,sizeof(((protocol_t*)0)->building_id));
	pro->cmd = cmd;
	pro->len = len;
	if( len>0 )
		rt_memcpy(&pro->data,data,len);
	*(buf + proto_crc(len)) = crc8_cal(buf,proto_len(len)-1);
	
	if( lora_sta.debug ){
		LOG_HEX("send data", 16, buf, proto_len(len));
	}
	
	rt_device_write(serial, 0, buf, proto_len(len));
	
	rt_free(buf);
}

static void serial_thread_entry(void *parameter)
{
    uint8_t ch;
	uint8_t recv_step=0;
	uint8_t recv_buf[50];
	uint8_t	recv_len = 0;
	uint8_t	data_len=0;
	uint8_t data_cnt = 0;
	
	while( config.lora_freq==0 ){
		rt_thread_delay(10);
	}
	
	YM401_MODE3();
	
	uint8_t *buf = rt_malloc(11);
    rt_thread_mdelay( 100 );
	lora_setup_t lora_setup;
	rt_memcpy(&lora_setup,&lora_setup_default,sizeof( lora_setup_t ));
	lora_set_freq(lora_setup.frequency,config.lora_freq);
	rt_device_write(serial, 0, ( uint8_t * )&lora_setup, sizeof( lora_setup_t ));
	rt_thread_mdelay( 100 );
	rt_device_read(serial, 0, ( uint8_t * )buf, 11);
	LOG_HEX("lora step reply", 16, buf, 11);
	rt_free(buf);
	
    YM401_MODE0();
	
    while (1)
    {
        /* 从串口读取一个字节的数据，没有读取到则等待接收信号量 */
        while (rt_device_read(serial, -1, &ch, 1) != 1)
        {
			
			switch( recv_step )
			{
				case 0:
					if( ch == HEADER1 ){
						rt_memset(recv_buf,0,sizeof(recv_buf));
						recv_buf[recv_len++] = ch;
						recv_step ++;
					}
					break;
					
				case 1:
					if( ch == HEADER2 ){
						recv_buf[recv_len++] = ch;
						recv_step ++;
					}
					else{
						recv_step = 0;
						recv_len = 0;
					}
					break;
				case 2:	//接收到命令长度数据
					recv_buf[recv_len++] = ch;
					if( recv_len==offsetof(protocol_t,len) )
						recv_step ++;
					break;
				case 3:	//接收长度字节
					recv_buf[recv_len++] = ch;
					if( ch<30 ){
						data_len = ch;
						if( ch==0 )
							recv_step += 2;
						else{
							recv_step ++;
							data_cnt = 0;
						}
					}
					else{
						recv_step = 0;
						recv_len = 0;
					}
					break;
				case 4:	//接收数据区
					recv_buf[recv_len++] = ch;
					data_cnt++;
					if( data_cnt==data_len )
						recv_step ++;
					break;
				case 5:	//接上校验码
				{
					recv_buf[recv_len++] = ch;
					uint8_t crc = crc8_cal(recv_buf,recv_len-1);
					
					if( lora_sta.debug ){
						LOG_D("recv crc %02X\r\n",crc);

						LOG_HEX("recv data", 16, recv_buf, recv_len);
					}
					
					if( ch == crc )
					{
						protocol_t *proto_ptr = (protocol_t *)recv_buf;
						
						if( rt_memcmp(config.building_id,proto_ptr->building_id,sizeof(((protocol_t*)0)->building_id))==0 )
						{					
							smoke_dev_t *smoke_ptr = lora_search_smoke(proto_ptr->floor,proto_ptr->room);
						
							if( smoke_ptr != RT_NULL ){
								
								switch( proto_ptr->cmd ){
									case CMD_REPORT:
									case CMD_SET:
									{
										smoke_data_t *smoke_data = (smoke_data_t *)&proto_ptr->data;
										smoke_ptr->smoke = (smoke_data->smoke>>8)|((smoke_data->smoke&0xff)<<8);
										smoke_ptr->temperature = (smoke_data->temperature>>8)|((smoke_data->temperature&0xff)<<8);
										smoke_ptr->battery = (smoke_data->battery>>8)|((smoke_data->battery&0xff)<<8);
										smoke_ptr->alarm_smoke = smoke_data->alarm.bit.smoke;
										smoke_ptr->alarm_temperature = smoke_data->alarm.bit.tempater;
										smoke_ptr->active_time = 0xff;
										smoke_ptr->err_sta = ERR_STA_OK;
										lora_sta.msgid = proto_ptr->msg_id;
										
										uint8_t buf[6];
										buf[0] = (config.smoke_threshold>>8)&0xff;
										buf[1] = (config.smoke_threshold)&0xff;
										buf[2] = (config.temp_threshold>>8)&0xff;
										buf[3] = (config.temp_threshold)&0xff;
										buf[4] = (config.poll_time);
										buf[5] = (lora_sta.alarm_set);
										if( proto_ptr->cmd==CMD_REPORT )
											lora_send(proto_ptr->floor, proto_ptr->room, CMD_REPORT|0x80, buf, 6);
										break;
									}
									default:
										break;
								}
							}
						}
					}
					recv_step = 0;
					recv_len = 0;
					break;
				}
				default:
					break;
			}
			
            /* 阻塞等待接收信号量，等到信号量后再次读取数据 */
            rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        }
    }
}

rt_err_t lora_uart_init(void)
{
	rt_err_t ret = RT_EOK;
	
	rt_memset(&lora_sta,0,sizeof(lora_state_t));
	
	/* 查找系统中的串口设备 */
    serial = rt_device_find(LORA_UART_NAME);
    if (!serial)
    {
        rt_kprintf("find %s failed!\n", LORA_UART_NAME);
        return RT_ERROR;
    }
	
	struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;  /* 初始化配置参数 */
	/* step2：修改串口配置参数 */
	config.baud_rate = BAUD_RATE_9600;        //修改波特率为 9600
	config.data_bits = DATA_BITS_8;           //数据位 8
	config.stop_bits = STOP_BITS_1;           //停止位 1
	config.bufsz     = 128;                   //修改缓冲区 buff size 为 128
	config.parity    = PARITY_NONE;           //无奇偶校验位

	/* step3：控制串口设备。通过控制接口传入命令控制字，与控制参数 */
	rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);

    /* 初始化信号量 */
    rt_sem_init(&rx_sem, "lora_rx", 0, RT_IPC_FLAG_FIFO);
    /* 以中断接收及轮询发送模式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, uart_input);
	
	/* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("lora_recv", serial_thread_entry, RT_NULL, 1024, 10, 10);
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
rt_err_t lora_init(void)
{
	lora_uart_init();
	rt_pin_mode(SETA_PIN, PIN_MODE_OUTPUT);
	rt_pin_mode(SETB_PIN, PIN_MODE_OUTPUT);
}
//INIT_ENV_EXPORT(lora_init);

void lora_smoke_info(int argc, char *argv[])
{
	if( argc == 1 ){
		
		rt_kprintf("total smoke:%d\r\n",lora_sta.smoke_num);
		rt_kprintf("foor\t room\t smoke\t tempature\t battery\t smoke_alarm\t tempature_alarm\t connect\t active time\t error sta\r\n");
		
		smoke_dev_t *smoke = lora_sta.smoke_dev;
		for( uint8_t i=0; i<lora_sta.smoke_num; i++ ){
			rt_kprintf("%d\t %d\t %d\t %d\t %d\t %d\t %d\t %d\t %d\t %d\r\n",\
			smoke->floor,
			smoke->room,
			smoke->smoke,
			smoke->temperature,
			smoke->battery,
			smoke->alarm_smoke,
			smoke->alarm_temperature,
			smoke->connect,
			smoke->active_time,
			smoke->err_sta
			);
			smoke++;
		}
	}
}
MSH_CMD_EXPORT(lora_smoke_info,"list smoke info -- lora_smoke_info");

void lora_set_band(int argc, char *argv[])
{
	const uint32_t fb[]={
		412200,
		419900,
		426600,
		433300,
		440000,
		447700,
		454400,
		461100,
		467700,
		474400,
		482200,
		489900,
		496600,
		503300,
		510000,
		517700
	};
	
	if( argc == 1 ){
		
		rt_kprintf("lora frequency:%d\r\n",config.lora_freq);
	}
	else if( argc==2 ){
		uint8_t band = atoi(argv[1]);
		if( band >= 1 && band <= 16 ){
			config.lora_freq = fb[band-1];
			rt_kprintf("set lora new frequency:%d\r\n",config.lora_freq);
			
			YM401_MODE3();
			uint8_t *buf = rt_malloc(11);
			rt_thread_mdelay( 100 );
			lora_setup_t lora_setup;
			rt_memcpy(&lora_setup,&lora_setup_default,sizeof( lora_setup_t ));
			lora_set_freq(lora_setup.frequency,config.lora_freq);
			rt_device_write(serial, 0, ( uint8_t * )&lora_setup, sizeof( lora_setup_t ));
			rt_thread_mdelay( 100 );
			rt_device_read(serial, 0, ( uint8_t * )buf, 11);
			LOG_HEX("lora step reply", 16, buf, 11);
			rt_free(buf);
			
			YM401_MODE0();
		}
		else{
			
		}
	}
}
MSH_CMD_EXPORT(lora_set_band,"set lora frequency band -- lora_set_fb <1-16>");


void lora_set_building(void)
{
	uint8_t buf[6];
	buf[0] = (config.smoke_threshold>>8)&0xff;
	buf[1] = (config.smoke_threshold)&0xff;
	buf[2] = (config.temp_threshold>>8)&0xff;
	buf[3] = (config.temp_threshold)&0xff;
	buf[4] = (config.poll_time);
	buf[5] = (lora_sta.alarm_set);
	
	lora_send(0, 0, CMD_BUILDING_ID, buf, 6);
	
	rt_kprintf("set smoke building id %s\r\n",config.building_id);
}
MSH_CMD_EXPORT(lora_set_building,"set smoke building id");

void lora_debug_comm(int argc, char *argv[]) 
{
	if( argc == 1 ){
		
		if( lora_sta.debug==0 )
			rt_kprintf("lora communication debug: off\r\n");
		else if( lora_sta.debug==1 ){
			rt_kprintf("lora communication debug: on\r\n");
		}
	}
	else if( argc==2 ){
		if( strcmp(argv[1],"on") == 0 ){
			lora_sta.debug = 1;
		}
		else if( strcmp(argv[1],"off") == 0 ){
			lora_sta.debug = 0;
		}
		rt_kprintf("lora communication debug:%s\r\n",argv[1]);
	}
}
MSH_CMD_EXPORT(lora_debug_comm,"display lora communication receive and send data -- lora_debug_comm [on/off]");