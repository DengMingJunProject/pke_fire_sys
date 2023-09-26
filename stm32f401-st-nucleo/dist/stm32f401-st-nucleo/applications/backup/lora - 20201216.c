#include "lora.h"
#include "crc8.h"
#include "rtthread.h"
#include <rtdevice.h>
#include "protocol.h"
#include "ulog.h"
#include "config.h"
#include "string.h"
#include <board.h>

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
const uint8_t ym401_setup[] = {0xff, 0x56, 0xae, 0x35, 0xa9, 0x55, 0x93, 0x06, 0x9c, 0x94, 0x02, 0x07, 0x03, 0x00, 0x00, 0x02};
																
const uint8_t ym401_setup_read[] =
{
    0xFF, 0x56, 0xAE, 0x35, 0xA9, 0x55, 0xF0
};

/* 用于接收消息的信号量 */
static struct rt_semaphore rx_sem;
static rt_device_t serial;
static smoke_dev_t smoke_dev[SMOKE_NUM];
lora_state_t lora_sta;
static rt_sem_t poll_sem = RT_NULL;
static uint8_t	wait_cmd;

static rt_err_t poll_sem_take(uint8_t cmd)
{
	if( poll_sem == RT_NULL )
		poll_sem = rt_sem_create("poll", 0, RT_IPC_FLAG_FIFO);
	
	wait_cmd = cmd;
	
	rt_sem_trytake(poll_sem);
	return rt_sem_take(poll_sem, 1000);
}

static rt_err_t poll_sem_release(uint8_t cmd)
{
	if( poll_sem == RT_NULL )
		poll_sem = rt_sem_create("poll", 0, RT_IPC_FLAG_FIFO);
	
	if( cmd == (wait_cmd|0x80) )
		return rt_sem_release (poll_sem);
}

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_sem_release(&rx_sem);

    return RT_EOK;
}

smoke_dev_t *lora_search_smoke(uint8_t floor, uint8_t room)
{
	smoke_dev_t *smoke_ptr = smoke_dev;
	for( uint8_t i=0; i<SMOKE_NUM; i++ ){
		if( smoke_ptr->floor == floor && smoke_ptr->room==room ){
			return smoke_ptr;
		}
	}
	return RT_NULL;
}

static void serial_thread_entry(void *parameter)
{
    uint8_t ch;
	uint8_t recv_step=0;
	uint8_t recv_buf[50];
	uint8_t	recv_len = 0;
	uint8_t	data_len=0;
	uint8_t data_cnt = 0;
    while (1)
    {
        /* 从串口读取一个字节的数据，没有读取到则等待接收信号量 */
        while (rt_device_read(serial, -1, &ch, 1) != 1)
        {
			
			switch( recv_step )
			{
				case 0:
					if( ch == HEADER1 ){
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
				case 2:
					recv_buf[recv_len++] = ch;
					if( recv_len==18 )
						recv_step ++;
					break;
				case 3:
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
				case 4:
					recv_buf[recv_len++] = ch;
					data_cnt++;
					if( data_cnt==data_len )
						recv_step ++;
					break;
				case 5:
					recv_buf[recv_len++] = ch;
					uint8_t crc = crc8_cal(recv_buf,recv_len-1);
					LOG_D("%02x ",crc);
					if( ch == crc ){
						for( uint8_t i=0; i<recv_len; i++ ){
							rt_kprintf("%02x ",recv_buf[i]);
						}
						rt_kprintf("\r\n");
						
						protocol_t *proto_ptr = (protocol_t *)recv_buf;
						
						if( strncmp(config.building_id,proto_ptr->building_id,11) ){
						
							smoke_dev_t *smoke_ptr = lora_search_smoke(proto_ptr->floor,proto_ptr->room);
							if( smoke_ptr != RT_NULL ){
								
								poll_sem_release(proto_ptr->cmd);
								
								switch( proto_ptr->cmd & 0x7f ){
									case CMD_SET:
										break;
									case CMD_QRE:
									{
										smoke_data_t *smoke_data = (smoke_data_t *)&proto_ptr->data;
										smoke_ptr->smoke = smoke_data->smoke;
										smoke_ptr->temperature = smoke_data->temperature;
										smoke_ptr->battery = smoke_data->battery;
										smoke_ptr->alarm_smoke = smoke_data->alarm.bit.smoke;
										smoke_ptr->alarm_temperature = smoke_data->alarm.bit.tempater;
										
										if( smoke_data->alarm.bit.smoke || smoke_data->alarm.bit.tempater ){
											extern uint8_t		alarm_open;
											alarm_open = 1;
											lora_sta.alarm_set = 1;
										}
										break;
									}
									case CMD_ARM:
										break;
									case CMD_SEH:
										break;
									default:
										break;
								}
							}
						}
					}
					recv_step = 0;
					recv_len = 0;
					break;
				default:
					break;
			}
			
            /* 阻塞等待接收信号量，等到信号量后再次读取数据 */
            rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        }
    }
}

void lora_send(uint8_t floor, uint8_t room, uint8_t cmd, uint8_t *data, uint8_t  len)
{
	uint8_t *buf = rt_malloc(proto_len(len));
	protocol_t *pro = (protocol_t *)buf;
	pro->header1 = 0xa5;
	pro->header2 = 0x5a;
	pro->msg_id = lora_sta.msgid++;
	pro->dir = DIR_TO_SMOKE;
	pro->floor = floor;
	pro->room = room;
	rt_memcpy(pro->building_id,config.building_id,11);
	pro->cmd = cmd;
	pro->len = len;
	if( len>0 )
		rt_memcpy(&pro->data,data,len);
	*(buf + proto_crc(len)) = crc8_cal(data,proto_len(len)-1);
	
	rt_device_write(serial, 0, buf, proto_len(len));
}

void lora_poll(uint8_t cmd, uint8_t *data, uint8_t len)
{
	if( config.smoke_num>0 ){
		for( uint8_t i=0; i<config.smoke_num;  i++ ){
			
			lora_send(config.dev[i].floor, config.dev[i].room, CMD_QRE, data, len);
			
			poll_sem_take(CMD_QRE);
		}
	}
}

void lora_search(void)
{
	set_blink_time(10);
	for( uint8_t i=1; i<=SEARCH_FLOOR;  i++ ){
		for( uint8_t j=1;j<=SEARCH_ROOM; j++ ){
			lora_send(i, j, CMD_SEH, RT_NULL, 0);
			
			poll_sem_take(CMD_SEH);
		}
	}
	set_blink_time(50);
}

extern uint8_t search_key;
void lora_protocol_task(void *parameter)
{
	while(1){
		if( search_key ){
			lora_search();
		}
		else{
			if( lora_sta.alarm_set ){
				
				lora_sta.alarm_set = 0;
				
				uint8_t data;
				lora_poll(CMD_ARM, &data, 1);
			}
			else if( lora_sta.app_set ){
				
				lora_sta.app_set = 0;
				
				uint8_t data[4];
				data[0] = (config.smoke_threshold>>8)&0xff;
				data[1] = config.smoke_threshold&0xff;
				data[2] = (config.temp_threshold>>8)&0xff;
				data[3] = config.temp_threshold&0xff;
				lora_poll(CMD_SET, data, 4);
			}
			else{
				lora_poll(CMD_QRE, RT_NULL, 0);
			}
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
	
	thread = rt_thread_create("lora_send", lora_protocol_task, RT_NULL, 1024, 10, 10);
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
    YM401_MODE3();

    rt_thread_mdelay( 100 );
	rt_device_write(serial, 0, ( uint8_t * )ym401_setup, sizeof( ym401_setup ));
//    rt_thread_mdelay( 400 );
//    DBG_DUMP( com_buf.slave_lora_recv_buf, 32 );
//    rt_device_write(serial, 0, ( uint8_t * )ym401_setup_read, sizeof( ym401_setup_read ) );
//    rt_thread_mdelay( 400 );
//    DBG_DUMP( com_buf.slave_lora_recv_buf, 32 );
    YM401_MODE0();
}
INIT_ENV_EXPORT(lora_init);