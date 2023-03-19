
#ifndef  ABUP_FLASH_H
#define  ABUP_FLASH_H
#include "peer_type.h"

#include "adups_typedef.h"

#define BLOCK_SIZE_4

#ifdef BLOCK_SIZE_64
#define ADUPS_TRUE_BLOCK_SIZE (4*1024)
#define ADUPS_BLOCK_SIZE (64*1024)
#define ADUPS_DIFF_PARAM_SIZE    (64*1024)
#elif defined BLOCK_SIZE_32
#define ADUPS_TRUE_BLOCK_SIZE (4*1024)
#define ADUPS_BLOCK_SIZE (32*1024)
#define ADUPS_DIFF_PARAM_SIZE    (32*1024)
#elif defined BLOCK_SIZE_4
#define ADUPS_BLOCK_SIZE (4*1024)
#define ADUPS_DIFF_PARAM_SIZE    (32*1024)
#define ADUPS_TRUE_BLOCK_SIZE (32*1024) //real use to make delta file
#else
#define ADUPS_BLOCK_SIZE (4*1024)
#define ADUPS_DIFF_PARAM_SIZE    (4*1024)
#define ADUPS_TRUE_BLOCK_SIZE (4*1024)
#endif

typedef enum {
 
    FLASH_STATUS_ERROR = -1,                  /**< flash function error */
    FLASH_STATUS_OK = 0                       /**< flash function ok */
} flash_status_t;

//#define ABUP_DEBUG_PRINTF

#define abup_bl_main_printf      printf

extern void adups_bl_erase_backup_region(void);
extern adups_int32  adups_bl_read_backup_region(adups_uint8 *data_ptr, adups_uint32 len); 
extern	adups_int32 adups_bl_erase_block(adups_uint32 addr);
extern adups_int32 adups_bl_read_block(adups_uint8* dest, adups_uint32 start, adups_uint32 size);
extern adups_int32 adups_bl_write_block(adups_uint8 *src, adups_uint32 start, adups_uint32 size);
extern adups_int32 adups_bl_erase_delata(void);
extern adups_int32 adups_bl_write_backup_region(adups_uint8 *data_ptr, adups_uint32 len);
extern adups_uint32 AdupsGetFlashDiskSize(void);
extern adups_uint32 adups_bl_flash_block_size(void);
extern adups_uint32 adups_bl_flash_delta_size(void);
extern adups_uint32 adups_bl_flash_delta_base(void);
extern adups_uint32 adups_bl_flash_backup_base(void);
extern int32_t adups_bl_read_flash(uint32_t addr, uint8_t* data_ptr, uint32_t len);
extern int32_t adups_bl_write_flash(uint32_t addr, uint8_t* data_ptr, uint32_t len);
extern adups_uint32 adups_bl_flash_true_block_size(void);




#endif

