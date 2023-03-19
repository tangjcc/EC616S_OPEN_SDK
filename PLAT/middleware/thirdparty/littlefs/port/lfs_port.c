#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "lfs_port.h"
#include "lfs_util.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "flash_ec616_rt.h"
#elif defined CHIP_EC616S
#include "flash_ec616s_rt.h"
#endif

#include "mem_map.h"
#include "debug_log.h"
#ifdef LFS_THREAD_SAFE_MUTEX
#include "cmsis_os2.h"
#endif
#include "plat_config.h"

/***************************************************
 ***************       MACRO      ******************
 ***************************************************/

#define LFS_BLOCK_DEVICE_READ_SIZE      (256)
#define LFS_BLOCK_DEVICE_PROG_SIZE      (256)
#define LFS_BLOCK_DEVICE_CACHE_SIZE     (256)
#define LFS_BLOCK_DEVICE_ERASE_SIZE     (4096)       // one sector 4KB
#define LFS_BLOCK_DEVICE_TOTOAL_SIZE    (FLASH_FS_REGION_SIZE)
#define LFS_BLOCK_DEVICE_LOOK_AHEAD     (16)

/***************************************************
 *******    FUNCTION FORWARD DECLARTION     ********
 ***************************************************/

// Read a block
static int block_device_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size);

// Program a block
//
// The block must have previously been erased.
static int block_device_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size);

// Erase a block
//
// A block must be erased before being programmed. The
// state of an erased block is undefined.
static int block_device_erase(const struct lfs_config *cfg, lfs_block_t block);

// Sync the block device
static int block_device_sync(const struct lfs_config *cfg);

// utility functions for traversals
static int lfs_statfs_count(void *p, lfs_block_t b);

/***************************************************
 ***************  GLOBAL VARIABLE  *****************
 ***************************************************/

// variables used by the filesystem
static lfs_t lfs;

#ifdef LFS_THREAD_SAFE_MUTEX
static osMutexId_t lfs_mutex;
#endif

static char lfs_read_buf[256];
static char lfs_prog_buf[256];
static __ALIGNED(4) char lfs_lookahead_buf[LFS_BLOCK_DEVICE_LOOK_AHEAD];

// configuration of the filesystem is provided by this struct
static struct lfs_config lfs_cfg =
{
    .context = NULL,
    // block device operations
    .read  = block_device_read,
    .prog  = block_device_prog,
    .erase = block_device_erase,
    .sync  = block_device_sync,

    // block device configuration
    .read_size = LFS_BLOCK_DEVICE_READ_SIZE,
    .prog_size = LFS_BLOCK_DEVICE_PROG_SIZE,
    .block_size = LFS_BLOCK_DEVICE_ERASE_SIZE,
    .block_count = LFS_BLOCK_DEVICE_TOTOAL_SIZE / LFS_BLOCK_DEVICE_ERASE_SIZE,
    .block_cycles = 200,
    .cache_size = LFS_BLOCK_DEVICE_CACHE_SIZE,
    .lookahead_size = LFS_BLOCK_DEVICE_LOOK_AHEAD,

    .read_buffer = lfs_read_buf,
    .prog_buffer = lfs_prog_buf,
    .lookahead_buffer = lfs_lookahead_buf,
    .name_max = 63,
    .file_max = 0,
    .attr_max = 0
};

#ifdef FS_FILE_OPERATION_STATISTIC

enum file_id_to_monitor
{
    FILE_CCMCONFIG,
    FILE_CEMMCONFIGEMM,
    FILE_CEMMCONFIGUE,
    FILE_CEMMESM,
    FILE_CEMMPLMN,
    FILE_CERRC,
    FILE_CISCONFIG,
    FILE_COAPNVM,
    FILE_DCXOFTBUFF,
    FILE_MIDWARECONFIG,
    FILE_NPICONFIG,
    FILE_PLATCONFIG,
    FILE_SLPNVM,
    FILE_UICCCTRL,
    FILE_TO_MONITOR_TOTAL_NUMBER
};

typedef struct file_to_monitor
{
    uint8_t fileId;
    const char* fileName;
} file_to_monitor_t;

const static file_to_monitor_t fileToMonitor[FILE_TO_MONITOR_TOTAL_NUMBER] =
{
    {FILE_CCMCONFIG,      "ccmconfig.nvm"},
    {FILE_CEMMCONFIGEMM,  "cemmconfigemminformation.nvm"},
    {FILE_CEMMCONFIGUE,   "cemmconfigueinformation.nvm"},
    {FILE_CEMMESM,        "cemmesmconfig.nvm"},
    {FILE_CEMMPLMN,       "cemmplmnconfig.nvm"},
    {FILE_CERRC,          "cerrcconfig.nvm"},
    {FILE_CISCONFIG,      "cisconfig.nvm"},
    {FILE_COAPNVM,        "coap_nvm"},
    {FILE_DCXOFTBUFF,     "dcxoFTBuff.nvm"},
    {FILE_MIDWARECONFIG,  "midwareconfig.nvm"},
    {FILE_NPICONFIG,      "npiconfig.nvm"},
    {FILE_PLATCONFIG,     "plat_config"},
    {FILE_SLPNVM,         "slp_nvm"},
    {FILE_UICCCTRL,       "uiccctrlconfig.nvm"},
};

#define STATISTIC_RESULT_FILE_NAME  "fileOpStatistic"

static uint32_t g_fileWriteBytesCount[FILE_TO_MONITOR_TOTAL_NUMBER] = {0};
static uint32_t g_fileWriteCount[FILE_TO_MONITOR_TOTAL_NUMBER] = {0};
static uint32_t g_blockEraseCount[LFS_BLOCK_DEVICE_TOTOAL_SIZE / LFS_BLOCK_DEVICE_ERASE_SIZE] = {0};

#define STATITIC_RESULT_FILE_BODY_SIZE    (sizeof(g_fileWriteBytesCount) + sizeof(g_fileWriteCount) + sizeof(g_blockEraseCount))
#define STATISTIC_RESULT_MAGIC_NUMBR  0x5AA5


typedef struct statistic_result_file_header
{
    uint16_t fileBodySize;
    uint16_t magicNumber;
} statistic_result_file_header_t;

static bool g_statisticResultLoaded = false;
static bool g_statisticResultChanged = false;

#endif

/***************************************************
 *******         INTERANL FUNCTION          ********
 ***************************************************/

static int block_device_read(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
    // Check if read is valid
    LFS_ASSERT(off  % cfg->read_size == 0);
    LFS_ASSERT(size % cfg->read_size == 0);
    LFS_ASSERT(block < cfg->block_count);

    uint8_t retValue;

#ifdef LFS_DEBUG_TRACE
    //ECOMM_TRACE(UNILOG_LFS, block_device_read, P_DEBUG, 3, "LFS block read, block: 0x%x, off: 0x%x, size: 0x%x", block, off, size);
#endif

    retValue = BSP_QSPI_Read_Safe((uint8_t *)buffer, (FLASH_FS_REGION_OFFSET + block * cfg->block_size + off), size);

    LFS_ASSERT(retValue == QSPI_OK);

    return (retValue == QSPI_OK) ? LFS_ERR_OK: LFS_ERR_IO;
}

static int block_device_prog(const struct lfs_config *cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{

    ECOMM_TRACE(UNILOG_LFS, block_device_prog_0, P_DEBUG, 1, "block_device_prog enter , SP: 0x%x", __current_sp());

    // Check if write is valid
    LFS_ASSERT(off  % cfg->prog_size == 0);
    LFS_ASSERT(size % cfg->prog_size == 0);
    LFS_ASSERT(block < cfg->block_count);

    uint8_t retValue;

#ifdef LFS_DEBUG_TRACE
    //ECOMM_TRACE(UNILOG_LFS, block_device_prog, P_DEBUG, 3, "LFS prog, block: 0x%x, off: 0x%x, size: 0x%x", block, off, size);
#endif

    // Program data
    retValue = BSP_QSPI_Write_Safe((uint8_t *)buffer, (FLASH_FS_REGION_OFFSET + block * cfg->block_size + off), size);

    LFS_ASSERT(retValue == QSPI_OK);

    return (retValue == QSPI_OK) ? LFS_ERR_OK: LFS_ERR_IO;
}

static int block_device_erase(const struct lfs_config *cfg, lfs_block_t block)
{
    // Check if erase is valid
    LFS_ASSERT(block < cfg->block_count);

#ifdef FS_FILE_OPERATION_STATISTIC
    g_blockEraseCount[block]++;
    g_statisticResultChanged = true;
#endif

    uint8_t retValue;

#ifdef LFS_DEBUG_TRACE
    //ECOMM_TRACE(UNILOG_LFS, block_device_erase, P_DEBUG, 1, "LFS erase block 0x%x", block);
#endif

    // Erase the block
    retValue = BSP_QSPI_Erase_Safe(FLASH_FS_REGION_OFFSET + block * cfg->block_size, LFS_BLOCK_DEVICE_ERASE_SIZE);

    LFS_ASSERT(retValue == QSPI_OK);

    return (retValue == QSPI_OK) ? LFS_ERR_OK: LFS_ERR_IO;
}

static int block_device_sync(const struct lfs_config *cfg)
{
    return 0;
}

static int lfs_statfs_count(void *p, lfs_block_t b)
{
    *(lfs_size_t *)p += 1;

    return 0;
}

/***************************************************
 *******      FUNCTION IMPLEMENTATION       ********
 ***************************************************/
#ifdef FS_FILE_OPERATION_STATISTIC

void LFS_LoadMonitorResult(void)
{
    int32_t err;

    lfs_file_t file;

    statistic_result_file_header_t fileHeader;

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    if(g_statisticResultLoaded)
    {
#ifdef LFS_THREAD_SAFE_MUTEX
        osMutexRelease(lfs_mutex);
#endif
        return;
    }

    err = lfs_file_open(&lfs, &file, STATISTIC_RESULT_FILE_NAME, LFS_O_RDWR);

    // file doesn't exist, create a new one and then write into default value
    if(err == LFS_ERR_NOENT)
    {
        err = lfs_file_open(&lfs, &file, STATISTIC_RESULT_FILE_NAME, LFS_O_CREAT | LFS_O_WRONLY);
        EC_ASSERT(err == 0, err, 0, 0);

        // fill file header
        fileHeader.fileBodySize = STATITIC_RESULT_FILE_BODY_SIZE;
        fileHeader.magicNumber = STATISTIC_RESULT_MAGIC_NUMBR;

        //write file header and check result
        err = lfs_file_write(&lfs, &file, &fileHeader, sizeof(statistic_result_file_header_t));
        EC_ASSERT(err == sizeof(statistic_result_file_header_t), 0, 0, 0);

        //write default value and check return value
        memset(g_fileWriteBytesCount, 0, sizeof(g_fileWriteBytesCount));
        err = lfs_file_write(&lfs, &file, g_fileWriteBytesCount, sizeof(g_fileWriteBytesCount));
        EC_ASSERT(err == sizeof(g_fileWriteBytesCount), err, sizeof(g_fileWriteBytesCount), 0);

        memset(g_fileWriteCount, 0, sizeof(g_fileWriteCount));
        err = lfs_file_write(&lfs, &file, g_fileWriteCount, sizeof(g_fileWriteCount));
        EC_ASSERT(err == sizeof(g_fileWriteCount), err, sizeof(g_fileWriteCount), 0);

        memset(g_blockEraseCount, 0, sizeof(g_blockEraseCount));
        err = lfs_file_write(&lfs, &file, g_blockEraseCount, sizeof(g_blockEraseCount));
        EC_ASSERT(err == sizeof(g_blockEraseCount), err, sizeof(g_blockEraseCount), 0);

        err = lfs_file_close(&lfs, &file);

        EC_ASSERT(err == 0, 0, 0, 0);
    }
    // file already exists
    else if(err == LFS_ERR_OK)
    {
        // fetch file header and perform validation check
        err = lfs_file_read(&lfs, &file, &fileHeader, sizeof(statistic_result_file_header_t));
        EC_ASSERT(err == sizeof(statistic_result_file_header_t), err, sizeof(statistic_result_file_header_t), 0);

        // file size and magicNumber check, if doest't match, write into default value
        if((fileHeader.fileBodySize != STATITIC_RESULT_FILE_BODY_SIZE) || (fileHeader.magicNumber != STATISTIC_RESULT_MAGIC_NUMBR))
        {
            // change position of the file to the beginning of the file
            err = lfs_file_rewind(&lfs, &file);
            EC_ASSERT(err == 0, 0, 0, 0);

            // fill file header
            fileHeader.fileBodySize = STATITIC_RESULT_FILE_BODY_SIZE;
            fileHeader.magicNumber = STATISTIC_RESULT_MAGIC_NUMBR;

            // write file header and check result
            err = lfs_file_write(&lfs, &file, &fileHeader, sizeof(statistic_result_file_header_t));
            EC_ASSERT(err == sizeof(statistic_result_file_header_t), err, sizeof(statistic_result_file_header_t), 0);

            // write default value and check return value
            memset(g_fileWriteBytesCount, 0, sizeof(g_fileWriteBytesCount));
            err = lfs_file_write(&lfs, &file, g_fileWriteBytesCount, sizeof(g_fileWriteBytesCount));
            EC_ASSERT(err == sizeof(g_fileWriteBytesCount), err, sizeof(g_fileWriteBytesCount), 0);

            memset(g_fileWriteCount, 0, sizeof(g_fileWriteCount));
            err = lfs_file_write(&lfs, &file, g_fileWriteCount, sizeof(g_fileWriteCount));
            EC_ASSERT(err == sizeof(g_fileWriteCount), err, sizeof(g_fileWriteCount), 0);

            memset(g_blockEraseCount, 0, sizeof(g_blockEraseCount));
            err = lfs_file_write(&lfs, &file, g_blockEraseCount, sizeof(g_blockEraseCount));
            EC_ASSERT(err == sizeof(g_blockEraseCount), err, sizeof(g_blockEraseCount), 0);
        }
        else
        {
            err = lfs_file_read(&lfs, &file, g_fileWriteBytesCount, sizeof(g_fileWriteBytesCount));
            EC_ASSERT(err == sizeof(g_fileWriteBytesCount), err, sizeof(g_fileWriteBytesCount), 0);

            err = lfs_file_read(&lfs, &file, g_fileWriteCount, sizeof(g_fileWriteCount));
            EC_ASSERT(err == sizeof(g_fileWriteCount), err, sizeof(g_fileWriteCount), 0);

            err = lfs_file_read(&lfs, &file, g_blockEraseCount, sizeof(g_blockEraseCount));
            EC_ASSERT(err == sizeof(g_blockEraseCount), err, sizeof(g_blockEraseCount), 0);
        }

        err = lfs_file_close(&lfs, &file);
        EC_ASSERT(err == 0, 0, 0, 0);

    }
    // handle left errors
    else
    {
        EC_ASSERT(err == LFS_ERR_OK, 0, 0, 0);
    }

    g_statisticResultLoaded = TRUE;

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return;
}

int LFS_GetFileWriteMonitorResult(file_operation_statistic_result_t* result)
{
    EC_ASSERT(result, result, 0, 0);

    LFS_LoadMonitorResult();

    static uint8_t pos = 0;

    if(pos < FILE_TO_MONITOR_TOTAL_NUMBER)
    {
        result->name = fileToMonitor[pos].fileName;
        result->writeCount = g_fileWriteCount[fileToMonitor[pos].fileId];
        result->writeBytesCount = g_fileWriteBytesCount[fileToMonitor[pos].fileId];
        pos++;
        return 1;
    }
    else
    {
        pos = 0;
        return 0;
    }
}

int LFS_GetBlockEraseCountResult(uint32_t* result)
{
    EC_ASSERT(result, result, 0, 0);

    LFS_LoadMonitorResult();

    static uint8_t index = 0;

    if(index < (LFS_BLOCK_DEVICE_TOTOAL_SIZE / LFS_BLOCK_DEVICE_ERASE_SIZE))
    {
        *result = g_blockEraseCount[index++];
        return 1;
    }
    else
    {
        index = 0;
        return 0;
    }
}


static void LFS_SaveMonitorResultNoLock(void)
{
    int err;

    lfs_file_t statisticFile;
    statistic_result_file_header_t fileHeader;

    if(g_statisticResultChanged)
    {
        err = lfs_file_open(&lfs, &statisticFile, STATISTIC_RESULT_FILE_NAME, LFS_O_WRONLY);
        EC_ASSERT(err == 0, 0, 0, 0);

        // fill file header
        fileHeader.fileBodySize = STATITIC_RESULT_FILE_BODY_SIZE;
        fileHeader.magicNumber = STATISTIC_RESULT_MAGIC_NUMBR;

        // write file header and check result
        err = lfs_file_write(&lfs, &statisticFile, &fileHeader, sizeof(statistic_result_file_header_t));
        EC_ASSERT(err == sizeof(statistic_result_file_header_t), err, sizeof(statistic_result_file_header_t), 0);

        // write filebody and check return value
        err = lfs_file_write(&lfs, &statisticFile, g_fileWriteBytesCount, sizeof(g_fileWriteBytesCount));
        EC_ASSERT(err == sizeof(g_fileWriteBytesCount), err, sizeof(g_fileWriteBytesCount), 0);

        err = lfs_file_write(&lfs, &statisticFile, g_fileWriteCount, sizeof(g_fileWriteCount));
        EC_ASSERT(err == sizeof(g_fileWriteCount), err, sizeof(g_fileWriteCount), 0);

        err = lfs_file_write(&lfs, &statisticFile, g_blockEraseCount, sizeof(g_blockEraseCount));
        EC_ASSERT(err == sizeof(g_blockEraseCount), err, sizeof(g_blockEraseCount), 0);

        err = lfs_file_close(&lfs, &statisticFile);
        EC_ASSERT(err == 0, 0, 0, 0);
    }

}

void LFS_SaveMonitorResult(void)
{
#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif


    LFS_SaveMonitorResultNoLock();


#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

}

int LFS_ResetMonitorResult(void)
{
#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    memset(g_fileWriteBytesCount, 0, sizeof(g_fileWriteBytesCount));
    memset(g_fileWriteCount, 0, sizeof(g_fileWriteCount));
    memset(g_blockEraseCount, 0, sizeof(g_blockEraseCount));

    g_statisticResultChanged = true;

    LFS_SaveMonitorResultNoLock();

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return 0;
}


#else

int LFS_GetFileWriteMonitorResult(file_operation_statistic_result_t* result)
{
    return 0;
}

int LFS_GetBlockEraseCountResult(uint32_t* result)
{
    return 0;
}

int LFS_SaveMonitorResult(void)
{
    return 0;
}

int LFS_ResetMonitorResult(void)
{
    return 0;
}

#endif

// Initialize
int LFS_Init(void)
{
    uint32_t fs_assert_count = BSP_GetFSAssertCount();

    if(fs_assert_count && ((fs_assert_count % EC_FS_ASSERT_REFORMAT_THRESHOLD) == 0))
    {
        ECOMM_TRACE(UNILOG_EXCEP_PRINT, LFS_Init_0, P_ERROR, 2, "FS region reformat threshold: %d, Current FS assert count:%d", EC_FS_ASSERT_REFORMAT_THRESHOLD, fs_assert_count);

        if(BSP_QSPI_Erase_Safe(FLASH_FS_REGION_OFFSET, FLASH_FS_REGION_SIZE) != 0)
        {
            ECOMM_TRACE(UNILOG_EXCEP_PRINT, LFS_Init_1, P_ERROR, 0, "FS region reformat failed!!!");
        }

        BSP_SetFSAssertCount(0);
    }

    // mount the filesystem
    int err = lfs_mount(&lfs, &lfs_cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err)
    {
        err = lfs_format(&lfs, &lfs_cfg);
        if(err)
            return err;

        err = lfs_mount(&lfs, &lfs_cfg);
        if(err)
            return err;
    }

#ifdef LFS_THREAD_SAFE_MUTEX
    // create mutex
    if(lfs_mutex == NULL)
    {
        lfs_mutex = osMutexNew(NULL);
        EC_ASSERT(lfs_mutex, lfs_mutex, 0, 0);
    }
#endif

#ifdef LFS_THREAD_SAFE_ALT_IMPLEMENT
    // create mutex
    if(lfs.rcache_mutex == NULL)
    {
        lfs.rcache_mutex = osMutexNew(NULL);
        EC_ASSERT(lfs.rcache_mutex, lfs.rcache_mutex, 0, 0);
    }

#endif
    return 0;
}

// Deinit
void LFS_Deinit(void)
{
    // release any resources we were using
    lfs_unmount(&lfs);

#ifdef LFS_THREAD_SAFE_MUTEX
    if(lfs_mutex)
    {
        osMutexDelete(lfs_mutex);
    }
#endif

#ifdef LFS_THREAD_SAFE_ALT_IMPLEMENT
    if(lfs.rcache_mutex)
    {
        osMutexDelete(lfs.rcache_mutex);
    }
#endif
}

int LFS_Format(void)
{
    int ret;

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret =  lfs_format(&lfs, &lfs_cfg);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}

int LFS_Stat(const char *path, struct lfs_info *info)
{
    int ret;

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret = lfs_stat(&lfs, path, info);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}

int LFS_Remove(const char *path)
{
    int ret;

#ifdef LFS_DEBUG_TRACE
    ECOMM_STRING(UNILOG_LFS, fs_remove, P_DEBUG, "LFS remove, path: %s", (uint8_t *)path);
#endif

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret =  lfs_remove(&lfs, path);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}

int LFS_FileOpen(lfs_file_t *file, const char *path, int flags)
{
    int ret;

#ifdef LFS_DEBUG_TRACE
    ECOMM_STRING(UNILOG_LFS, fs_file_open, P_DEBUG, "LFS file open, path: %s", (uint8_t *)path);
#endif

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret = lfs_file_open(&lfs, file, path, flags);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;

}

int LFS_FileClose(lfs_file_t *file)
{
    int ret;

#ifdef LFS_DEBUG_TRACE
    ECOMM_STRING(UNILOG_LFS, fs_file_close, P_DEBUG, "LFS file close, file name: %s", (uint8_t *)file->name);
#endif

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret = lfs_file_close(&lfs, file);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}

lfs_ssize_t LFS_FileRead(lfs_file_t *file, void *buffer, lfs_size_t size)
{
    int ret;

#ifdef LFS_DEBUG_TRACE
    ECOMM_STRING(UNILOG_LFS, fs_file_read, P_DEBUG, "LFS file read, file name: %s", (uint8_t *)file->name);
#endif

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret = lfs_file_read(&lfs, file, buffer, size);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}

lfs_ssize_t LFS_FileWrite(lfs_file_t *file, const void *buffer, lfs_size_t size)
{
    int ret;

#ifdef LFS_DEBUG_TRACE
    ECOMM_STRING(UNILOG_LFS, fs_file_write, P_DEBUG, "LFS file write, file name: %s", (uint8_t *)file->name);
#endif

#ifdef FS_FILE_OPERATION_STATISTIC
    LFS_LoadMonitorResult();
#endif

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

#ifdef FS_FILE_OPERATION_STATISTIC

    g_statisticResultChanged = true;

    for(int i = 0; i < FILE_TO_MONITOR_TOTAL_NUMBER; i++)
    {
        if(strcmp(file->name, fileToMonitor[i].fileName) == 0)
        {
            g_fileWriteBytesCount[fileToMonitor[i].fileId] += size;
            g_fileWriteCount[fileToMonitor[i].fileId] += 1;
        }
    }

#endif

    ret = lfs_file_write(&lfs, file, buffer, size);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}

lfs_soff_t LFS_FileSeek(lfs_file_t *file, lfs_soff_t off, int whence)
{
    int ret;

#ifdef LFS_DEBUG_TRACE
    ECOMM_STRING(UNILOG_LFS, fs_file_seek, P_DEBUG, "LFS file seek, file name: %s", (uint8_t *)file->name);
#endif

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret = lfs_file_seek(&lfs, file, off, whence);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}

int LFS_FileSync(lfs_file_t *file)
{
    int ret;

#ifdef LFS_DEBUG_TRACE
    ECOMM_STRING(UNILOG_LFS, fs_file_sync, P_DEBUG, "LFS file sync, file name: %s", (uint8_t *)file->name);
#endif

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret = lfs_file_sync(&lfs, file);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}


int LFS_FileTruncate(lfs_file_t *file, lfs_off_t size)
{
    int ret;

#ifdef LFS_DEBUG_TRACE
    ECOMM_STRING(UNILOG_LFS, fs_file_truncate, P_DEBUG, "LFS file truncate, file name: %s", (uint8_t *)file->name);
#endif

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret = lfs_file_truncate(&lfs, file, size);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}

lfs_soff_t LFS_FileTell(lfs_file_t *file)
{
    int ret;

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret = lfs_file_tell(&lfs, file);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}

int LFS_FileRewind(lfs_file_t *file)
{
    int ret;

#ifdef LFS_DEBUG_TRACE
    ECOMM_STRING(UNILOG_LFS, fs_file_rewind, P_DEBUG, "LFS file rewind, file name: %s", (uint8_t *)file->name);
#endif

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret = lfs_file_rewind(&lfs, file);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}

lfs_soff_t LFS_FileSize(lfs_file_t *file)
{
    int ret;

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret = lfs_file_size(&lfs, file);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}

int LFS_DirOpen(lfs_dir_t *dir, const char *path)
{
    int ret;

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret = lfs_dir_open(&lfs, dir, path);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}

int LFS_DirClose(lfs_dir_t *dir)
{
    int ret;

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret = lfs_dir_close(&lfs, dir);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}

int LFS_DirRead(lfs_dir_t *dir, struct lfs_info *info)
{
    int ret;

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret = lfs_dir_read(&lfs, dir, info);

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}

int LFS_Statfs(lfs_status_t *status)
{
    int ret;

    status->total_block = lfs_cfg.block_count;
    status->block_size = lfs_cfg.block_size;

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexAcquire(lfs_mutex, osWaitForever);
#endif

    ret = lfs_fs_traverse(&lfs, lfs_statfs_count, &(status->block_used));

#ifdef LFS_THREAD_SAFE_MUTEX
    osMutexRelease(lfs_mutex);
#endif

    return ret;
}


