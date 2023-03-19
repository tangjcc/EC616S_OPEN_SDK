#ifndef _PEER_TYPE_H
#define _PEER_TYPE_H

#include <stdint.h>

typedef unsigned int          ML_U32;
typedef int                   ML_S32;
typedef unsigned short int    ML_U16;
typedef short int             ML_S16;
typedef unsigned char         ML_U8;
typedef char                  ML_S8;

#if !defined(WIN32)
//typedef unsigned long long    uintptr_t;
#endif

typedef unsigned char        ML_BOOL;

//add wpf start
typedef unsigned char           uint8_t;

typedef signed char             int8_t;

typedef unsigned short int      uint16_t;

typedef signed short int        int16_t;

typedef unsigned long long   uint64_t;

typedef signed long long     int64_t;

//abup
typedef char                    abup_char;

typedef unsigned short          abup_wchar;

typedef unsigned char           abup_uint8;

typedef signed char             abup_int8;

typedef unsigned short int      abup_uint16;

typedef signed short int        abup_int16;

typedef unsigned int            abup_uint;

typedef signed int              abup_int;

typedef unsigned long            abup_uint32;

typedef signed long              abup_int32;

typedef unsigned long long   abup_uint64;

typedef signed long long     abup_int64;

typedef unsigned int abup_size_t;


typedef abup_uint8   abup_bool;
#define abup_true  ((abup_bool)1)
#define abup_false ((abup_bool)0)

#if 0 //debug log 
#define peer_logout  printf
#else
#define peer_logout(...)
#endif

#define MLMAX(a,b)    (((a) > (b)) ? (a) : (b))
#define MLMIN(a,b)    (((a) < (b)) ? (a) : (b))

#define FALSE   0
#define TRUE    1

#define ML_ALIGNSIZE( x , n )             ( ( (x)+(n)-1)  & (~((n)-1))  )
#define ML_ALIGNSIZE8( x  )               ML_ALIGNSIZE( x , 8 )
#define ML_ISALIGN(x, n)                  ( ( (ML_U32)(x) & (ML_U32)((n)-1) ) == 0)

#define ML_FILE_FLAG_INVALID  0
#define ML_FILE_FLAG_NORMAL   1
#define ML_FILE_FLAG_PUSH     2

typedef struct peer_file_stru_tag
{
    ML_U32 fd;
    ML_U16 type;
    ML_U16 flag;
	ML_S8* fname;
    ML_U32 param;
    ML_U32 base_position;
    ML_U32 position;
    ML_U32 length;
	ML_U32 errorcode;
}ML_PEER_FILE_STRU;

typedef ML_PEER_FILE_STRU* ML_PEER_FILE;

#define PEER_FILETYPE_ROM         1
#define PEER_FILETYPE_FLASH       2
#define PEER_FILETYPE_BACKUP      3
//#define PEER_FILETYPE_FAT         4


#define PATCH_TYPE_ZHUSUN          1
#define PATCH_TYPE_WOSUN           2

#define STREAM_TYPE_NONE             0
#define STREAM_TYPE_BZ               1
#define STREAM_TYPE_LZMA             2
typedef void* (* stream_init_decoder_ptr)(ML_PEER_FILE f);
typedef int (* stream_inf_stream_read_ptr)(void *stream, ML_U8* buf, int len);
typedef void (* stream_inf_stream_end_ptr)(void *stream);
typedef struct 
{
	stream_init_decoder_ptr init_fun; 
	stream_inf_stream_read_ptr read_fun;
	stream_inf_stream_end_ptr end_fun;
}STREAM_INF_FUN;
extern void *stream_init_bz_decoder(ML_PEER_FILE fp);
extern int stream_bz_read(void *stream, ML_U8* buf, int len); 
extern void stream_bz_end(void *strm);
extern void *stream_init_lzma_decoder(ML_PEER_FILE fp);
extern int stream_lzma_read(void *stream, ML_U8* buf, int len); 
extern void stream_lzma_end(void *strm);
extern void peer_setPrePatch(ML_BOOL flag);
extern void peer_setCheckOldbin(ML_BOOL flag);
#ifdef LINUX	
#define ML_INLINE __inline__
#else
#define ML_INLINE inline
#endif

#define restrict


#if defined(PEER_HAVING_MEM_TEST)
#include "peer_mem.h"
#endif

#if defined(WIN32)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <io.h>
#include <limits.h>
#include "windows.h"

#elif defined (LINUX)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>  
#include <sys/time.h>  
#include <assert.h>
#else
#include "adups_typedef.h"
#include "adups_bl_main.h"
#include "adups_bl_flash.h"
#include "stdarg.h"
#endif


#if !defined(WIN32) 
typedef struct _SYSTEMTIME
{
	ML_U16 wYear;
	ML_U16 wMonth;
	ML_U16 wDayOfWeek;
	ML_U16 wDay;
	ML_U16 wHour;
	ML_U16 wMinute;
	ML_U16 wSecond;
	ML_U16 wMilliseconds;
}SYSTEMTIME;
#endif


#define ML_SIZE_MAX 0xffffffff


/* 按平台需要细化返回值 */
/* 当RETCODE <0 时禁止删除差分包 */
/* 当RETCODE >=0 时可以删除差分包 */
typedef enum RETCODE_FOTA_ENUM 
    {
	    RETCODE_FOTA_WRITE_ERR = -2,
        RETCODE_FOTA_UPDATE_ERR = -1,
		RETCODE_FOTA_OK = 0,
	    RETCODE_FOTA_PROCESS_ERR   = 1,
		RETCODE_FOTA_PROCESS_ERRSYSTEM   = 2,
		RETCODE_FOTA_PROCESS_ERRFORMAT   = 3,
		RETCODE_FOTA_PROCESS_ERRMEM   = 4, 
		RETCODE_FOTA_PROCESS_ERRHEADER  = 5, 
	    RETCODE_FOTA_PROCESS_ERRDATA   = 6,
	    RETCODE_FOTA_PROCESS_ERRFILESPACE  = 7,
	    RETCODE_FOTA_PROCESS_PATCHED  = 8,
		RETCODE_FOTA_PROCESS_ERRPATCHFILE = 9
}RETCODE_FOTA;


#endif // _PEER_TYPE_H



