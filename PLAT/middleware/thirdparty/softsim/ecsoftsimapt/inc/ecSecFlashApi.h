/******************************************************************************
Copyright:      - 2020, All rights reserved by Eigencomm Ltd.
File name:      - ecsoftsimapt.c
Description:    - softsim adapter API.
Function List:  -
History:        - 09/10/2020, Originated by yjwang
******************************************************************************/

#ifndef _EC_SEC_FLASH_API_H
#define _EC_SEC_FLASH_API_H


#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#include "win32typedef.h"
#else
#include "commontypedef.h"
#include "debug_trace.h"
#include "debug_log.h"
#endif



/*------------sec store data-----------------*/
#define  ID_IMSI_SIZE_MAX               (9)
#define  ID_KEY_SIZE_MAX                (16)
#define  ID_OPC_SIZE_MAX                (16)
#define  ID_EUICC_SK_SIZE_MAX           (32)
#define  ID_OT_SK_SIZE_MAX              (32)


/*------------plain store data-----------------*/
#define  ID_ICCID_SIZE_MAX              (10)
#define  ID_DFNAME_SIZE_MAX             (16)
#define  ID_SMDPOID_SIZE_MAX            (16)
#define  ID_TRANSACTION_ID_SIZE_MAX     (16)
#define  ID_EUICC_CHALLENGE_SIZE_MAX    (16)
#define  ID_EID_SIZE_MAX                (16)
#define  ID_EFDIR_SIZE_MAX              (26)
#define  ID_OT_PK_SIZE_MAX              (64)
#define  ID_EUICC_PK_SIZE_MAX           (64)

#define  ID_CERT_CI_SIZE_MAX             (1024)
#define  ID_CERT_EUM_SIZE_MAX            (1024)
#define  ID_CERT_EUICC_SIZE_MAX          (1024)
#define  ID_CERT_DPPB_SIZE_MAX           (1024)

#define EC_FLASH_NV_FLAG	        (0x94898763)
#define EC_FLASH_VERSION_FLAG       (0x89696975)


#define EC_FLASH_DEFAULT_VAL        (0xFFFFFFFF)
typedef enum ec_softsim_data_id
{
    SFS_FILE_ID_SK=0,
    SFS_FILE_ID_OTSK,
    SFS_FILE_ID_PK,
    SFS_FILE_ID_CI_CERT,
    SFS_FILE_ID_EUM_CERT,
    SFS_FILE_ID_EUICC_CERT,
    SFS_FILE_ID_EUICC_CHALLENGE,
    SFS_FILE_ID_TRANSACTION_ID,
    SFS_FILE_ID_OTPK,
    SFS_FILE_ID_DPPB_CERT,
    SFS_FILE_ID_ICCID,
    SFS_FILE_ID_IMSI,
    SFS_FILE_ID_KEY,
    SFS_FILE_ID_OPC,
    SFS_FILE_ID_EF_DIR,
    SFS_FILE_ID_DFNAME,
    SFS_FILE_ID_ESIM_SWITCH,
    SFS_FILE_ID_SMDPOID,
    SFS_FILE_ID_6FE3,
    SFS_FILE_ID_6FE4,
    SFS_FILE_ID_EID,
    SFS_FILE_ID_MAX = 255,
}EC_SOFTSIM_DATA_ID;

typedef enum
{
    EC_FLASH_BLOCK_ID_HEADER = 0,
    EC_FLASH_BLOCK_ID_SWAP1,
    EC_FLASH_BLOCK_ID_DATA1,
    EC_FLASH_BLOCK_ID_DATA2,
    EC_FLASH_BLOCK_ID_DATA3,
    EC_FLASH_BLOCK_ID_DATA4,
    EC_FLASH_BLOCK_ID_SWAP2,
    EC_FLASH_BLOCK_ID_KEYDATA,
    EC_FLASH_BLOCK_ID_INVALID,
}EC_FLASH_BLOCK_ID;

#define EC_FLASH_STATISTIC_NUM_MAX     (EC_FLASH_BLOCK_ID_SWAP2 + 1)

typedef enum
{
    EC_FLASH_NO_ERASE,
    EC_FLASH_ERASED
}EC_FLASH_ID_STATE;

typedef enum _EC_FLASH_ERR
{
    EC_FLASH_OK = 0x0,
    EC_FLASH_ERR,
    EC_FLASH_FORMAT_ERR,
    EC_FLASH_ERASE_FLASH_ERR,
    EC_FLASH_WRITE_FLASH_ERR,
    EC_FLASH_CHECK_ITEM_ERR,
    EC_FLASH_CHECK_ITEM_NO_EXIST,
} EC_FLASH_ERR_E;


typedef struct
{
    UINT32       writeFlag;
    UINT32       writeCount;
    UINT8        sectorId;
    UINT16       chksum;        /*cal the main data sector checksum*/
}EC_FLASH_STORE_T;

typedef struct
{
    UINT16     sfs_id;
    UINT16     block_id;
    UINT32     offset;
    UINT16     maxsize;
}EC_CTCC_FLASH_RECORD_T;

typedef struct
{
    UINT16  sfs_id;
    UINT16  sfs_isEnc;
    UINT16  sfs_plainLen;
    UINT16  sfs_secLen;
}EC_CTCC_FLASH_ELEMENT_T;



typedef struct
{
    UINT32       work_flag;
    UINT32       nv_version;
    UINT16       used_size;
    UINT16       time_counter;
    UINT16       chksum;              /*cal this header checksum*/
    UINT16       reserved;
}EC_FLASH_HEADER_T;

typedef struct
{
    UINT32  write_count;
    UINT32  read_count;
    UINT32  erase_count;

}EC_FLASH_STATISTIC_T;


typedef struct
{
    UINT32   w_sector_chksum;
    UINT32   w_sector_id;
    UINT32   w_size;
    UINT32   w_counter;
    UINT32   swap_headchksum;
    UINT32   swap_datachksum;
    EC_FLASH_STATISTIC_T  statistic[EC_FLASH_STATISTIC_NUM_MAX];
    UINT32   reserved;
}EC_FLASH_SWAP_HEADER_T;



extern INT8 ecSoftSimFlashDataRead(UINT16 index,UINT16 outbuffMax,UINT8 *dataOut,  UINT32 *outlen,UINT8 isSec);
extern INT8 ecSoftSimFlashDataWrite(const UINT16 index, const UINT8  *dataIn, const UINT16 plainLen, const UINT16 encLen,const UINT8  isSec);
extern INT8 ecSoftSimFlashEncryDataWrite(const UINT16 index, const UINT8  *dataIn, const UINT16 inBuffLen);
extern UINT8 ecSoftSimFlashConstructAesData(const int mode,const UINT8  *dataIn, const UINT16 kvLen, UINT8 **EncData, UINT16 *outlen);
extern INT8 ecSoftSimFlashInit(void);


#ifdef __cplusplus
}
#endif

#endif
