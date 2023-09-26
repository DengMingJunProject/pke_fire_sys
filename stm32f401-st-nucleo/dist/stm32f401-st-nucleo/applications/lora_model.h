#ifndef __LORA_MODEL_H__
#define __LORA_MODEL_H__

#include "stdint.h"
#include "rtthread.h"

#pragma pack(1)
//空间速率
typedef enum{
	K0_81,
	K1_41,
	K2_60,
	K4_56,
	K9_11,
	K18_23,
}air_rate_t;

//发射功率
typedef enum{
	tx_0db,
	tx_3db,
	tx_6db,
	tx_9db,
	tx_12db,
	tx_15db,
	tx_18db,
	tx_20db,
}tx_power_t;

//串口波特率
typedef enum{
	br_1200,
	br_2400,
	br_4800,
	br_9600,
	br_19200,
	br_38400,
	br_57600,
	br_115200,
}baudrate_t;

//串口校验方式
typedef enum{
	vf_none,
	vf_odd,
	vf_even,
}verify_t;

//唤醒时间
typedef enum{
	wt_50ms,
	wt_100ms,
	wt_200ms,
	wt_400ms,
	wt_600ms,
	wt_1s,
	wt_1_5s,
	wt_2s,
	wt_2_5s,
	wt_3s,
	wt_4s,
	wt_5s,
}wakeup_time_t;

typedef enum{
	fs_0ms,				//不需要cad
	fs_5ms, 			//超时5ms后，强制发送
	fs_50ms, 
	fs_100ms, 
	fs_200ms, 
	fs_400ms, 
	fs_600ms, 
	fs_1000ms, 
	fs_1500ms, 
	fs_2000ms, 
	fs_2500ms, 
	fs_3000ms, 
	fs_4000ms,
	cs_0ms	= 0x80, 	//不需要cad
	cs_5ms, 			//超时5ms后，取消发送
	cs_50ms, 
	cs_100ms, 
	cs_200ms, 
	cs_400ms, 
	cs_600ms, 
	cs_1000ms, 
	cs_1500ms, 
	cs_2000ms, 
	cs_2500ms, 
	cs_3000ms, 
	cs_4000ms
}cad_time_t;

#define LORA_VER		0x93
#define LORA_ID			{0xff, 0x56, 0xae, 0x35, 0xa9, 0x55}
#define lora_freq(freq)	{(freq>>16)&0xff,(freq>>8)&0xff,(freq)&0xff}

rt_inline void lora_set_freq(uint8_t *ptr, uint32_t freq)
{
	*ptr = (freq>>16)&0xff;
	*(ptr+1) = (freq>>8)&0xff;
	*(ptr+2) = (freq)&0xff;
}

typedef struct _lora_setup
{
	uint8_t		header_id[6];
	uint8_t		version;
	uint8_t		frequency[3];
	air_rate_t	air_rate;
	tx_power_t	tx_power;
	baudrate_t	baudrate;
	verify_t	verify;
	wakeup_time_t	wakeup_time;
	cad_time_t		cad_time;
}lora_setup_t;

#pragma pack()

#endif
