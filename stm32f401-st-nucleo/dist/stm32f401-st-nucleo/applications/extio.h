
#ifndef __EXTIO_H__
#define __EXTIO_H__

#include "stdint.h"
#include "stdbool.h"

#define EXTIO_IN_MAX	24
#define EXTIO_OUT_MAX	8
#define EXTIO_KEY_MAX	2
#define EXTIO_CHECK_AMX	2

#define EXTIO_SET_KEY	0
#define EXTIO_OPEN_KEY	1

#define EXTIO_KEY_RELEASE	0
#define EXTIO_KEY_PRESS		1
#define EXTIO_KEY_KEEP		2

#define EXTIO_CHECK_POWERDOWN	0
#define EXTIO_CHECK_EMERGENCY	1

typedef struct _input_t{
	uint8_t		buf[EXTIO_IN_MAX/8];
	uint8_t		save[EXTIO_IN_MAX/8];
	uint8_t		timer[EXTIO_IN_MAX];
}input_t;

typedef struct _output_t{
	uint8_t		byte[EXTIO_OUT_MAX/8];
}output_t;

typedef struct _key_t{
	uint8_t		sta[EXTIO_KEY_MAX];
	uint8_t		flag[EXTIO_KEY_MAX];
	uint8_t		time[EXTIO_KEY_MAX];
}key_t;

typedef struct _check_t{
	uint8_t		sta[EXTIO_CHECK_AMX];
	uint8_t		time[EXTIO_CHECK_AMX];
}check_t;

typedef struct _extio_t{
	input_t		input;
	output_t	output;
	key_t		key;
	check_t		check;
}extio_t;

extern extio_t	extio;
#define		is_smoke(num)			((extio.input.save[(num-1)/8]&(1<<((num-1)%8)))==0)
#define		set_alarm(num)			(extio.output.byte[(num-1)/8] |= 1<<((num-1)%8) )
#define		clr_alarm(num)			(extio.output.byte[(num-1)/8] &= ~(1<<((num-1)%8)) )
#define		set_all_alarm()			(extio.output.byte[0] = 0xff )
#define		clr_all_alarm()			(extio.output.byte[0] = 0x00 )
#define		key_sta(num)			(extio.key.sta[num])
#define		key_clr(num)			(extio.key.sta[num]=EXTIO_KEY_RELEASE)
#define 	is_check(num)			(extio.check.sta[num]==1)
#define		is_fire_machine()		((extio.input.save[(24-1)/8]&(1<<((24-1)%8)))==0)


extern 		__inline void extio_door_open(rt_bool_t sta);
extern 		__inline void extio_main_alarm(rt_bool_t sta);
extern		rt_bool_t extio_is_smoke_target(void);
#endif
