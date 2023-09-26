#ifndef __PORTOCOL_H__
#define __PORTOCOL_H__

#include "stdint.h"
#include "stdbool.h"

#define HEADER1		0xa5
#define HEADER2		0x5a

//烟感上报命令
#define CMD_REPORT	0x02
#define CMD_SET		0x03

//主控下发命令
#define CMD_BUILDING_ID	0x83

#define DIR_TO_SMOKE	0x01
#define DIR_TO_MAIN		0x02

#define offsetof(TYPE, MEMBER) ((int)(&((TYPE *)0)->MEMBER))

#pragma pack(1)

typedef struct _protocol{
	uint8_t		header1;
	uint8_t		header2;
	uint8_t		floor;
	uint8_t		room;
	uint8_t		msg_id;
	uint8_t		dir;
	char		building_id[15];
	uint8_t		cmd;
	uint8_t		len;
	uint8_t		data;
}protocol_t;

#pragma pack()

#define proto_len(l)		(sizeof(protocol_t)+l)
#define proto_data(l)		(sizeof(protocol_t)-1)
#define proto_crc(l)		(sizeof(protocol_t)+l-1)

#endif
