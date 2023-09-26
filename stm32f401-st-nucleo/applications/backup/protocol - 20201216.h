#ifndef __PORTOCOL_H__
#define __PORTOCOL_H__

#include "stdint.h"
#include "stdbool.h"

#define HEADER1		0xa5
#define HEADER2		0x5a

#define CMD_SET		0x01
#define CMD_QRE		0x02
#define CMD_ARM		0x03
#define CMD_SEH		0x04

#define DIR_TO_SMOKE	0x01
#define DIR_TO_MAIN		0x02

#pragma pack(1)

typedef struct _protocol{
	uint8_t		header1;
	uint8_t		header2;
	uint8_t		msg_id;
	uint8_t		dir;
	uint8_t		floor;
	uint8_t		room;
	char		building_id[11];
	uint8_t		cmd;
	uint8_t		len;
	uint8_t		data;
}protocol_t;

#pragma pack()

#define proto_len(l)		(sizeof(protocol_t)+l)
#define proto_data(l)		(sizeof(protocol_t)-1)
#define proto_crc(l)		(sizeof(protocol_t)+l-1)

#endif
