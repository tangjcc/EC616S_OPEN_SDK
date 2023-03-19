#ifndef _PEER_H
#define _PEER_H

#include "peer_config.h"
#include "peer_type.h"
#include "abup_stdlib.h"
#include "Adups_bl_flash.h"


#define PNAME_LENGTH 8
#define DATETIME_LENGTH 8

typedef struct bspatch_header
{
	ML_S8  name[PNAME_LENGTH];
	ML_S8  datetime[DATETIME_LENGTH];
	ML_U32 oldsize;
	ML_U32 newsize;
	ML_U32 patchsize;
	ML_U32 frame_size;
	ML_U16 platform_id;        
	ML_U16 tools_version;
	ML_U8  type;                 // 1 : PATCH_TYPE_ZHUSUN  2:PATCH_TYPE_WOSUN
	ML_U8  serialNO;
	ML_U16 blocknum;  
	ML_U32 ad_op_data_size;
	ML_U32 ad_op_data_crc;
	ML_U32 request_mempool_size;
	ML_U32 patch_data_crc;
	ML_U8  stream_type;           // 1: BZ  2: LZMA
	ML_U8  badcrc;                // 1: crc32 old algorithm failed
	ML_U8  appended_patch_filecrc_flag;
	ML_U8  reserved_u8[1];
	ML_U32 reserved[3];
}BSPATCH_HEADER;

#define PEER_PLATFORM_NODEFINED         0
#define PEER_PLATFORM_1                 1
#define PEER_PLATFORM_2                 2
#define PEER_PLATFORM_3                 3

#ifdef __cplusplus
    extern "C" {
#endif

int updateFile(int platform_id, int tools_version, int type, char* oldfile, int oldfile_type,
	char* newfile, int newfile_type, char* patchfile, int patchfile_type, int stream_type, int filecrc_flag);
RETCODE_FOTA bspatchFile(char* oldfile, int oldfile_type, char* patchfile, int patchfile_type, ML_BOOL do_make_patchfile, int mempool_size, ML_BOOL do_pre_patch, char* olddatafile, int olddatafile_type);
void peer_getdatetime(SYSTEMTIME* psysTm);
void peer_assert( int expression );


// memory
#if defined(MEM_TEST)
#define peer_malloc(x)	    test_malloc_ptr(x, __FILE__,__LINE__)
#define peer_remalloc(x, y)	test_remalloc_ptr(x, y, __FILE__, __LINE__)
#define peer_free(x)		test_free_ptr(x)
#else
#define peer_malloc(x)	    peer_malloc_ptr(x)
#define peer_remalloc(x,y)	peer_remalloc_ptr(x,y)
#define peer_free(x)		peer_free_ptr(x)
#endif

void* peer_malloc_ptr(int nsize);
void* peer_remalloc_ptr(void* oldptr, int nsize);
int peer_mempool_total(void);
void peer_free_ptr(void *ptr);
void peer_malloc_status(void);

ML_BOOL frame_memory_initialize(void *in_start, ML_U32 in_len);
int frame_memory_finalize(void);
void * frame_memory_malloc(ML_U32 in_size);
void *frame_memory_remalloc(void *in_memory, ML_U32 in_size);
void frame_memory_get_free_size(ML_U32 *out_total_bytes, ML_U32 *out_longest_blocks);
int frame_memory_get_poolsize(void);
ML_BOOL frame_memory_free(void *in_memory);

// file
#define PEER_FILE_SEEK_SET	0	//SEEK_SET
#define PEER_FILE_SEEK_CUR	1	//SEEK_CUR
#define PEER_FILE_SEEK_END	2	//SEEK_END

ML_PEER_FILE peer_fopen(const char*file, ML_U16 type, const char *option, ML_U32 base_position);
int peer_fclose(ML_PEER_FILE fp);
int peer_fread(ML_U8* buf, int size, int times, ML_PEER_FILE fp);
int peer_fwrite(const ML_U8* buf, int size, int times, ML_PEER_FILE fp);
int peer_ftell(ML_PEER_FILE fp);
int peer_fseek(ML_PEER_FILE fp, int length, int base);
int peer_filesize(ML_PEER_FILE fp);
ML_BOOL peer_feof(ML_PEER_FILE fp);
int peer_ferror(ML_PEER_FILE fp);
int peer_fflush(ML_PEER_FILE fp);
int peer_remove(const char *file, int type);
int peer_ftruncate(ML_PEER_FILE fp, int len);
int peer_get_flashblocksize(void);
int peer_get_flashblocknum(int length);
int peer_get_flashblockpos(int block_num);
int peer_get_totalflashsize(void);

// misc
ML_BOOL peer_isCheckOldbin(void);
void peer_setCheckOldbin(ML_BOOL flag);
ML_BOOL peer_isReadbackCheck(void);
void peer_setReadbackCheck(ML_BOOL flag);
ML_BOOL peer_isPrePatch(void);
void peer_setPrePatch(ML_BOOL flag);

int32_t peer_offtin_sign_lowest(uint32_t* buffer);

int peer_str_ucs2_to_ascii(ML_S8 *pOutBuffer, ML_S8 *pInBuffer, int nsize);
int peer_str_ascii_to_ucs2(ML_S16 *pOutBuffer, ML_S8 *pInBuffer, int nsize);
void peer_update_length(ML_PEER_FILE fp, int length);
int peer_get_length(ML_PEER_FILE fp) ;

ML_BOOL readFromBackup(ML_U8* buf, int nsize, int pos);
ML_BOOL writeToBackup(ML_U8* buf, int nsize, int pos);

void peer_backupAlways(ML_U8* buf, int nsize);
ML_BOOL peer_copyFile(ML_S8* oldfile, ML_S8* newfile, int offset);
ML_BOOL peer_isOldDataExist(ML_U32 crc, int len, ML_S8* filename, ML_U16 type,  ML_U32 pos); 
ML_BOOL peer_fpush(ML_PEER_FILE fp);
ML_BOOL peer_fpop(ML_PEER_FILE fp);
void peer_watchdog_restart(void);

#define FILE_HANDLE_OLDFILE           100
#define FILE_HANDLE_PATCH             101
#define FILE_HANDLE_BACKUP            102
#define FILE_HANDLE_OLDDATA           103

#define FILE_NAME_OLDFILE            "$old$"
#define FILE_NAME_PATCH              "$patch$"
#define FILE_NAME_BACKUP             "$backup$"
#define FILE_NAME_OLDDATA            "$olddata$"

typedef struct flashdata_position_stru
{
	ML_U32 patchfile_base_pos;
	ML_U32 olddata_base_pos;
	ML_U32 backup_base_pos;
}FLASHDATA_POSITION;

#ifdef __cplusplus
    }
#endif

#endif // _PEER_H



