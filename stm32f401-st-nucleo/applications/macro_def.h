#ifndef __MACRO_DEF_H__
#define __MACRO_DEF_H__

/** ����ṹ���Աƫ�Ƶ�ַ */  
#define offsetof(TYPE, MEMBER) ((int)(&((TYPE *)0)->MEMBER))

/** ���ݳ�Ա��ַ��ȡ�ṹ��ָ�� */  
#define container_of(ptr, type, member) ({   \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - offsetof(type,member) );})

typedef enum 
{
	FUN_OK       = 0x00U,
	FUN_ERROR    = 0x01U,
	FUN_BUSY     = 0x02U,
	FUN_TIMEOUT  = 0x03U,
	FUN_END,
	FUN_NEND,
	FUN_NONE,
} FUN_STATUS_T;

enum{
	STA_CLR=0,
	STA_SET,
};

#define REPEAT_EXE(fun)		\
({							\
	uint8_t cnt=0;			\
	while( (fun != FUN_OK) )		\
	{						\
		vTaskDelay( 10 );	\
		if( cnt++>=3 )		\
			break;			\
	}						\
	if( cnt==3 ){			\
		repeat_exe_result = FUN_OK;				\
	}						\
	else{					\
		repeat_exe_result = FUN_ERROR;			\
	}						\
})

#define repeat_exe_state()	(repeat_exe_result)

#define set_bit(x,y) 	x|=(1<<y) 	//<��X�ĵ�Yλ��1
#define clr_bit(x,y) 	x&=~(1<<y) 	//<��X�ĵ�Yλ��0
#define is_bit(x,y)		(((x)&(1<<y))!=0) 	//<�ж��Ƿ���λ

#define swap_int16(val) ((val) >> 8)|((val) << 8)

#define CONECT_STR(str1,str2)    (str1##str2)

///<��־λ��31-24��Ϊ�ֿصĵ�ַ
#define TASK_COM_OK		0x00000001
#define TASK_COM_BUSY	0x00000002

extern unsigned char repeat_exe_result;

#endif
