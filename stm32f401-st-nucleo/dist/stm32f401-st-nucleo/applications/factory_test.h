
#ifndef __FACTORY_TEST_H__
#define __FACTORY_TEST_H__

#include "rtthread.h"
#include "stdint.h"
#include "stdbool.h"

#define	SUSPEND_THREAD_MAX		10

typedef struct _factory_sta{
	uint8_t		suspend_cnt;
	rt_thread_t suspend_thread[SUSPEND_THREAD_MAX];
}factory_sta_t;

extern void factory_suspend_thread(rt_thread_t thread);

#endif
