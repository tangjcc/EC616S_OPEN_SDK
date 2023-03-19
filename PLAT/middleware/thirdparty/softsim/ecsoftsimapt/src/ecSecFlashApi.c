/******************************************************************************
Copyright:      - 2020, All rights reserved by Eigencomm Ltd.
File name:      - ecsoftsimapt.c
Description:    - softsim adapter API.
Function List:  -
History:        - 09/10/2020, Originated by yjwang
******************************************************************************/


#ifdef SOFTSIM_FEATURE_ENABLE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "osasys.h"
#include "cmsis_os2.h"
#include "mem_map.h"

#if defined CHIP_EC616
#include "flash_ec616_rt.h"
#elif defined CHIP_EC616S
#include "flash_ec616s_rt.h"
#endif

#include "debug_trace.h"
#include "debug_log.h"
#include "ecSecFlashApi.h"
#include "ecSoftsimApt.h"
#include "ecSecCallback.h"

extern BOOL bSoftSimEnable;
#define SOFTSIM_FLASH_DEFLAUT_VAL    0xFFFFFFFF
#define SOFTSIM_FLASH_MAINDATA_FLAG  0x5A5A5A5A
#define SOFTSIM_FLASH_BKDATA_FLAG    0x86451256



/*-------------Feature support -----------*/
#define EC_SOFT_FLASH_SUP_RECOVERY        1
#define EC_SOFT_FLASH_SUP_KEYDATA_BACKUP  1


#define SOFTSIM_FLASH_ASSERT_TRACE(a)   do{   \
	ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashInit_0 , P_WARNING, 0, "Serious ERROR: flash checksum invalid!!!!!!"); 	 \
}while(a==0)

#define SOFTSIM_FLASH_BASE_ADDRESS     SOFTSIM_FLASH_PHYSICAL_BASEADDR
#define SOFTSIM_FLASH_SIZE             SOFTSIM_FLASH_MAX_SIZE

#define SOFTSIM_FLASH_RECORD_LENTH           16
#define SOFTSIM_FLASH_SECTOR_BASE_SIZE       0x1000

#define SOFTSIM_FLASH_HEADER_SIZE       (SOFTSIM_FLASH_SECTOR_BASE_SIZE)
#define SOFTSIM_FLASH_RECORD_SIZE       (SOFTSIM_FLASH_SECTOR_BASE_SIZE)
#define SOFTSIM_FLASH_DATA_ONE_SIZE     (SOFTSIM_FLASH_SECTOR_BASE_SIZE)
#define SOFTSIM_FLASH_DATA_TWO_SIZE     (SOFTSIM_FLASH_SECTOR_BASE_SIZE)
#define SOFTSIM_FLASH_DATA_BK_SIZE      (SOFTSIM_FLASH_SECTOR_BASE_SIZE)

#define  SOFTSIM_FLASH_RECORD_MAX_SIZE    (SOFTSIM_FLASH_RECORD_SIZE/SOFTSIM_FLASH_RECORD_LENTH)

#define  SOFTSIM_FLASH_RECORD_CAUTION_LEVEL  (SOFTSIM_FLASH_RECORD_MAX_SIZE/2 - 28)


/*-------------------FLASH MAPPING TABLE-----------------/
*
*      -----------------------  SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR: 0x344000
*      |                      |
*      |      HEADER (4k)     |
*      -----------------------  SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(1):  0x344000 +4k
*      |                      |
*      |      SWAP1 (4k)      |
*      ------------------------ SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(2):  0x344000 + 8k
*      |                      |
*      |      DATA1 (4k)      |
*      ------------------------ SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(2):  0x344000 + 12k
*      |                      |
*      |      DATA2 (4k)      |
*      ------------------------ SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(3) : 0x344000 + 16k
*      |                      |
*      |      DATA3(4K)       |
*      ------------------------ SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(4) :  0x344000 + 20K
*      |                      |
*      |      DATA4(4K)       |
*      ------------------------ SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(5) :  0x344000 + 24K
*      |                      |
*      |      SWAP2(4K)       |
*      ------------------------ SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(5) :  0x344000 + 28K
*      |                      |
*      |      RESERVED AREA   |
*      ------------------------

*------------------FLASH MAPPING END ----------------*/



#define   SECTOR_SWAP_ONE_OFFSET            (SOFTSIM_FLASH_BASE_ADDRESS + 1 * SOFTSIM_FLASH_SECTOR_BASE_SIZE)

#define   SFS_FILE_ID_CI_CERT_OFFSET     (SOFTSIM_FLASH_BASE_ADDRESS +  2 * SOFTSIM_FLASH_SECTOR_BASE_SIZE)
#define   SFS_FILE_ID_EUM_CERT_OFFSET    (SOFTSIM_FLASH_BASE_ADDRESS +  2 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 1024)

#define   SFS_FILE_ID_EUICC_CERT_OFFSET  (SOFTSIM_FLASH_BASE_ADDRESS +  3 * SOFTSIM_FLASH_SECTOR_BASE_SIZE)
#define   SFS_FILE_ID_DPPB_CERT_OFFSET   (SOFTSIM_FLASH_BASE_ADDRESS +  3 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 1024)


#define   SFS_FILE_ID_SK_OFFSET                 (SOFTSIM_FLASH_BASE_ADDRESS + 4 * SOFTSIM_FLASH_SECTOR_BASE_SIZE)
#define   SFS_FILE_ID_OTSK_OFFSET               (SOFTSIM_FLASH_BASE_ADDRESS + 4 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 1*128)
#define   SFS_FILE_ID_PK_OFFSET                 (SOFTSIM_FLASH_BASE_ADDRESS + 4 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 2*128)
#define   SFS_FILE_ID_EUICC_CHALLENGE_OFFSET    (SOFTSIM_FLASH_BASE_ADDRESS + 4 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 3*128)
#define   SFS_FILE_ID_TRANSACTION_ID_OFFSET     (SOFTSIM_FLASH_BASE_ADDRESS + 4 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 4*128)
#define   SFS_FILE_ID_OTPK_OFFSET               (SOFTSIM_FLASH_BASE_ADDRESS + 4* SOFTSIM_FLASH_SECTOR_BASE_SIZE  + 5*128)
#define   SFS_FILE_ID_ICCID_OFFSET              (SOFTSIM_FLASH_BASE_ADDRESS + 4 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 6*128)
#define   SFS_FILE_ID_IMSI_OFFSET               (SOFTSIM_FLASH_BASE_ADDRESS + 4 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 7*128)
#define   SFS_FILE_ID_6FE3_OFFSET               (SOFTSIM_FLASH_BASE_ADDRESS + 4 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 8*128)


#define   SFS_FILE_ID_SMDPOID_OFFSET            (SOFTSIM_FLASH_BASE_ADDRESS + 5 * SOFTSIM_FLASH_SECTOR_BASE_SIZE)
#define   SFS_FILE_ID_KEY_OFFSET                (SOFTSIM_FLASH_BASE_ADDRESS + 5 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 1*128)
#define   SFS_FILE_ID_OPC_OFFSET                (SOFTSIM_FLASH_BASE_ADDRESS + 5 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 2*128)
#define   SFS_FILE_ID_EF_DIR_OFFSET             (SOFTSIM_FLASH_BASE_ADDRESS + 5 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 3*128)
#define   SFS_FILE_ID_DFNAME_OFFSET             (SOFTSIM_FLASH_BASE_ADDRESS + 5 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 4*128)
#define   SFS_FILE_ID_6FE4_OFFSET               (SOFTSIM_FLASH_BASE_ADDRESS + 5 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 5*128)
#define   SFS_FILE_ID_EID_OFFSET                (SOFTSIM_FLASH_BASE_ADDRESS + 5 * SOFTSIM_FLASH_SECTOR_BASE_SIZE + 6*128)

#define   SECTOR_SWAP_TWO_OFFSET                (SOFTSIM_FLASH_BASE_ADDRESS + 6 * SOFTSIM_FLASH_SECTOR_BASE_SIZE)

#define   SECTOR_KEYDATA_AREA_BASE_OFFSET        (SOFTSIM_FLASH_BASE_ADDRESS + 7 * SOFTSIM_FLASH_SECTOR_BASE_SIZE)

#define   SECTOR_KEYDATA_ICCID_OFFSET            (SECTOR_KEYDATA_AREA_BASE_OFFSET)
#define   SECTOR_KEYDATA_IMSI_OFFSET             (SECTOR_KEYDATA_AREA_BASE_OFFSET + 1*128)
#define   SECTOR_KEYDATA_SMDPOID_OFFSET          (SECTOR_KEYDATA_AREA_BASE_OFFSET + 2*128)
#define   SECTOR_KEYDATA_KEY_OFFSET              (SECTOR_KEYDATA_AREA_BASE_OFFSET + 3*128)
#define   SECTOR_KEYDATA_OPC_OFFSET              (SECTOR_KEYDATA_AREA_BASE_OFFSET + 4*128)
#define   SECTOR_KEYDATA_EF_DIR_OFFSET           (SECTOR_KEYDATA_AREA_BASE_OFFSET + 5*128)
#define   SECTOR_KEYDATA_DFNAME_OFFSET           (SECTOR_KEYDATA_AREA_BASE_OFFSET + 6*128)


/**
  \brief       the flash mapping table
*/

static EC_CTCC_FLASH_RECORD_T  ctcc_flash_table[] = {

   /*setcor 1*/
   { SFS_FILE_ID_CI_CERT,          EC_FLASH_BLOCK_ID_DATA1 ,SFS_FILE_ID_CI_CERT_OFFSET,       1024},
   { SFS_FILE_ID_EUM_CERT,         EC_FLASH_BLOCK_ID_DATA1 ,SFS_FILE_ID_EUM_CERT_OFFSET,      1024},

   /*setcor 2*/
   { SFS_FILE_ID_EUICC_CERT,       EC_FLASH_BLOCK_ID_DATA2 ,SFS_FILE_ID_EUICC_CERT_OFFSET,    1024},
   { SFS_FILE_ID_DPPB_CERT,        EC_FLASH_BLOCK_ID_DATA2 ,SFS_FILE_ID_DPPB_CERT_OFFSET,     1024},

   /*setcor 3*/
   { SFS_FILE_ID_SK,               EC_FLASH_BLOCK_ID_DATA3 ,SFS_FILE_ID_SK_OFFSET,             128 },
   { SFS_FILE_ID_OTSK,             EC_FLASH_BLOCK_ID_DATA3 ,SFS_FILE_ID_OTSK_OFFSET,           128 },
   { SFS_FILE_ID_PK,               EC_FLASH_BLOCK_ID_DATA3 ,SFS_FILE_ID_PK_OFFSET,             128 },
   { SFS_FILE_ID_EUICC_CHALLENGE,  EC_FLASH_BLOCK_ID_DATA3 ,SFS_FILE_ID_EUICC_CHALLENGE_OFFSET,128 },
   { SFS_FILE_ID_TRANSACTION_ID,   EC_FLASH_BLOCK_ID_DATA3 ,SFS_FILE_ID_TRANSACTION_ID_OFFSET, 128 },
   { SFS_FILE_ID_OTPK,             EC_FLASH_BLOCK_ID_DATA3 ,SFS_FILE_ID_OTPK_OFFSET,           128 },
   { SFS_FILE_ID_ICCID,            EC_FLASH_BLOCK_ID_DATA3 ,SFS_FILE_ID_ICCID_OFFSET,          128 },
   { SFS_FILE_ID_IMSI,             EC_FLASH_BLOCK_ID_DATA3 ,SFS_FILE_ID_IMSI_OFFSET,           128 },
   { SFS_FILE_ID_6FE3,             EC_FLASH_BLOCK_ID_DATA3 ,SFS_FILE_ID_6FE3_OFFSET,           128 },

    /*setcor 4*/
    { SFS_FILE_ID_SMDPOID,        EC_FLASH_BLOCK_ID_DATA4 ,SFS_FILE_ID_SMDPOID_OFFSET,         128 },
    { SFS_FILE_ID_KEY,            EC_FLASH_BLOCK_ID_DATA4 ,SFS_FILE_ID_KEY_OFFSET,             128 },
    { SFS_FILE_ID_OPC,            EC_FLASH_BLOCK_ID_DATA4 ,SFS_FILE_ID_OPC_OFFSET,             128 },
    { SFS_FILE_ID_EF_DIR,         EC_FLASH_BLOCK_ID_DATA4 ,SFS_FILE_ID_EF_DIR_OFFSET,          128 },
    { SFS_FILE_ID_DFNAME,         EC_FLASH_BLOCK_ID_DATA4 ,SFS_FILE_ID_DFNAME_OFFSET,          128 },
    { SFS_FILE_ID_6FE4,           EC_FLASH_BLOCK_ID_DATA4 ,SFS_FILE_ID_6FE4_OFFSET,            128 },
    { SFS_FILE_ID_EID,            EC_FLASH_BLOCK_ID_DATA4 ,SFS_FILE_ID_EID_OFFSET,             128 }
};


static EC_CTCC_FLASH_RECORD_T  ctcc_keydata_table[] =
{
    { SFS_FILE_ID_ICCID,          EC_FLASH_BLOCK_ID_KEYDATA, SECTOR_KEYDATA_ICCID_OFFSET,           128 },
    { SFS_FILE_ID_IMSI,           EC_FLASH_BLOCK_ID_KEYDATA, SECTOR_KEYDATA_IMSI_OFFSET,            128 },
    { SFS_FILE_ID_SMDPOID,        EC_FLASH_BLOCK_ID_KEYDATA, SECTOR_KEYDATA_SMDPOID_OFFSET,         128 },
    { SFS_FILE_ID_KEY,            EC_FLASH_BLOCK_ID_KEYDATA, SECTOR_KEYDATA_KEY_OFFSET,             128 },
    { SFS_FILE_ID_OPC,            EC_FLASH_BLOCK_ID_KEYDATA, SECTOR_KEYDATA_OPC_OFFSET,             128 },
    { SFS_FILE_ID_EF_DIR,         EC_FLASH_BLOCK_ID_KEYDATA, SECTOR_KEYDATA_EF_DIR_OFFSET,          128 },
    { SFS_FILE_ID_DFNAME,         EC_FLASH_BLOCK_ID_KEYDATA, SECTOR_KEYDATA_DFNAME_OFFSET,          128 }
};



#define  ecSoftSimRecordSectorAddr    SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(5)

#define  INVALID_PLAIN_DATA(data)  \
        (data->sfs_isEnc != 0    ||  \
        (data->sfs_plainLen == 0 ||  \
         data->sfs_plainLen >= 0XFFFF) )

#define  INVALID_ENC_DATA(data)        \
       (data->sfs_isEnc != 1  ||   \
       (data->sfs_secLen == 0 ||   \
        data->sfs_secLen >= 0XFFFF ||   \
        data->sfs_plainLen == 0 ||      \
        data->sfs_plainLen >= 0XFFFF ))


INT8 ecSoftSimFlashRecovery(void);
UINT8  ecSoftSimFlashFindBlockSize(UINT16 block_id);
static UINT8 SoftsimFlashCheckSectorIsErased(UINT8 *pbuf,UINT32 size);
static UINT32  _ecSoftSimFlashChksum(void *dataptr, UINT16 len);
static EC_CTCC_FLASH_RECORD_T  *ecSoftSimFlashFindRecordTable(UINT16 element);

EC_CTCC_FLASH_RECORD_T  *ecSoftSimFlashFindKeyData(UINT8 index)
{
    UINT8 jloop = 0 ;
    EC_CTCC_FLASH_RECORD_T  *pRecordData =NULL;
    for(jloop = 0;jloop  < sizeof(ctcc_keydata_table) /sizeof(EC_CTCC_FLASH_RECORD_T);jloop++)
    {
        pRecordData = &ctcc_keydata_table[jloop];
        if(pRecordData && pRecordData->sfs_id == index)
        {
            return pRecordData;
        }
    }
    return  NULL;

}

INT8  SoftsimRecyMainFlashData(
     UINT16 block_id ,UINT16 writeLen, UINT8 *InData)
{
    INT8     ret    = -1;
    UINT8    jloop  = 0;
    UINT16   allLen  =0 ;
    UINT16   preLen  =0 ;
    UINT8    bFind   = 0;
    EC_CTCC_FLASH_RECORD_T    *pRecordData = NULL;
    EC_CTCC_FLASH_ELEMENT_T   *pElemData   = NULL;

    while(allLen < writeLen)
    {
         pElemData = NULL;
         pElemData = (EC_CTCC_FLASH_ELEMENT_T*)(InData + allLen);
         if(pElemData == NULL)
         {
            return  (0);
         }

         if(
             pElemData->sfs_id  >= 0xFFFF ||
             pElemData->sfs_plainLen == 0 ||
             pElemData->sfs_plainLen >= 0xFFFF ||
             (pElemData->sfs_plainLen == 0 && pElemData->sfs_secLen == 0))
         {
              allLen += preLen;
              continue;
         }

         bFind = 0;
         for(jloop = 0;jloop  < SFS_FILE_ID_EID + 1;jloop++)
         {
             pRecordData = NULL;
             pRecordData = &ctcc_flash_table[jloop];

             if(pRecordData->block_id == block_id  &&
                pRecordData->sfs_id   == pElemData->sfs_id)
             {
                bFind = 1;
                break;
             }
         }

         if(bFind)
         {
             preLen = (pElemData->sfs_isEnc ? pElemData->sfs_secLen : pElemData->sfs_plainLen ) + sizeof(EC_CTCC_FLASH_ELEMENT_T);
             if(preLen > sizeof(EC_CTCC_FLASH_ELEMENT_T) )
             {
                 ret = SoftsimWriteToRawFlash(pRecordData->offset,(UINT8 *)pElemData, preLen);
                 if(ret != 0)
                 {
                     return (ret);
                 }
             }
         }
         allLen += preLen;

    }

    ECOMM_TRACE(UNILOG_SOFTSIM, SoftsimRecyMainFlashData_2, P_INFO, 3,
                            "write :block_id %d len:%d,allLen:%d",block_id,writeLen,allLen);

    return 0;

}

INT8  SoftsimBackupDataToSwap(
        UINT8     sector_id,
        UINT16    dataLen,
        UINT8    *pdataIn,
        UINT8    *pDataDest,
        UINT16   *OutCount,
        UINT16   *OutLen)
{
    UINT8   iloop = 0;
    UINT16  findcount = 0;
    UINT16  AllLen = 0;
    UINT16  eleMaxSize = 0;
    EC_CTCC_FLASH_ELEMENT_T   *pElemData = NULL;

    eleMaxSize = ecSoftSimFlashFindBlockSize(sector_id);
    if(eleMaxSize == 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, SoftsimBackupDataToSwap_0, P_INFO, 1, "maxsize:%d error ",eleMaxSize);
        return (0);
    }

    findcount = 0;

    for(iloop = 0 ;iloop < SOFTSIM_FLASH_DATA_ONE_SIZE/eleMaxSize ;iloop++)
    {
         pElemData = NULL;
         pElemData = (EC_CTCC_FLASH_ELEMENT_T*)(pdataIn + iloop*eleMaxSize);

         if ( pElemData           != NULL   &&
              pElemData->sfs_id   != 0XFFFF &&
             ((pElemData->sfs_isEnc == 0 && pElemData->sfs_plainLen !=0 ) ||
              (pElemData->sfs_isEnc == 1 && pElemData->sfs_plainLen !=0  && pElemData->sfs_plainLen !=0)))
         {
            if(AllLen < dataLen)
            {
                if(pElemData->sfs_isEnc == 0)
                {
                    findcount++;
                    memcpy(pDataDest + AllLen, pElemData,
                                      sizeof(EC_CTCC_FLASH_ELEMENT_T) + pElemData->sfs_plainLen);
                    AllLen += sizeof(EC_CTCC_FLASH_ELEMENT_T) + pElemData->sfs_plainLen ;

                }
                else if(pElemData->sfs_isEnc == 1)
                {
                    findcount++;
                    memcpy(pDataDest + AllLen, pElemData,
                                      sizeof(EC_CTCC_FLASH_ELEMENT_T) + pElemData->sfs_secLen);
                    AllLen += sizeof(EC_CTCC_FLASH_ELEMENT_T) + pElemData->sfs_secLen ;
                }
            }
         }
    }

    if(OutLen)   *OutLen =   AllLen ;
    if(OutCount) *OutCount = findcount;

    ECOMM_TRACE(UNILOG_SOFTSIM, SoftsimBackupDataToSwap_2, P_INFO, 2,
                                        "finally, count:%d,len:%d",findcount,AllLen);
    return (0) ;

}

/**
  \fn          SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR
  \brief       get every sector phy address by sector index
  \param[in]   index : the sector index of softsim flash mapping table
  \returns     offset
*/


INT8  ecSoftSimFlashGetActivateSwap(
        UINT8                    *active_fdt,
        UINT8                    *inactive_fdt,
        UINT32                   *active_addr,
        UINT32                   *inactive_addr,
        EC_FLASH_SWAP_HEADER_T   *pswap_header
        )
{

    EC_FLASH_SWAP_HEADER_T swap_1_header = {0};
    EC_FLASH_SWAP_HEADER_T swap_2_header = {0};
    INT8                   ret            = -1;
    UINT32                 chksum         = 0;

    memset(&swap_1_header, 0x0, sizeof(EC_FLASH_SWAP_HEADER_T));
    ret = SoftsimReadRawFlash(SECTOR_SWAP_ONE_OFFSET,(UINT8 *)&swap_1_header ,sizeof(EC_FLASH_SWAP_HEADER_T));
    if(ret != 0)
    {
        return (ret);
    }

    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashGetActivateSwap_0, P_INFO, 5,
                                "swap1[ w_chksum:0x%x,w_sector_id %d,w_size:%d,w_count:%d,chksum:0x%x]",
                                                  swap_1_header.w_sector_chksum,swap_1_header.w_sector_id,
                                                  swap_1_header.w_size,swap_1_header.w_counter,
                                                  swap_1_header.swap_headchksum);

    if(swap_1_header.swap_headchksum != _ecSoftSimFlashChksum(&swap_1_header,16))
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashGetActivateSwap_1, P_INFO, 2,
                                    "cal chksum:0x%x,store chksum 0x%x",chksum,swap_1_header.swap_headchksum);
       // return (ret);
    }

    memset(&swap_2_header, 0x0, sizeof(EC_FLASH_SWAP_HEADER_T));
    ret = SoftsimReadRawFlash(SECTOR_SWAP_TWO_OFFSET,(UINT8 *)&swap_2_header ,sizeof(EC_FLASH_SWAP_HEADER_T));
    if(ret != 0)
    {
        return (ret);
    }

    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashGetActivateSwap_2, P_INFO, 5,
                                "swap2[ w_chksum:0x%x,w_sector_id %d,w_size:%d,w_count:%d,chksum:0x%x]",
                                                  swap_2_header.w_sector_chksum,swap_2_header.w_sector_id,
                                                  swap_2_header.w_size,swap_2_header.w_counter,
                                                  swap_2_header.swap_headchksum);

    if(swap_2_header.swap_headchksum != _ecSoftSimFlashChksum(&swap_2_header, 16))
    {
         ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashGetActivateSwap_3, P_INFO, 2,
                                     "cal chksum:0x%x,store chksum 0x%x",chksum,swap_2_header.swap_headchksum);
        // return (ret);
    }
    OsaCheck(swap_1_header.w_counter != 0 && swap_2_header.w_counter != 0, 0, 0, 0);

    if( swap_1_header.w_counter == swap_2_header.w_counter &&
        swap_1_header.w_counter != EC_FLASH_DEFAULT_VAL )
    {
         ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashGetActivateSwap_4, P_INFO, 0,"active is swap1");

        *active_fdt   = EC_FLASH_BLOCK_ID_SWAP1;
        *inactive_fdt = EC_FLASH_BLOCK_ID_SWAP2;
        *active_addr  = SECTOR_SWAP_ONE_OFFSET;
        *inactive_addr = SECTOR_SWAP_TWO_OFFSET;
         pswap_header->w_counter =  swap_1_header.w_counter;
         pswap_header->w_sector_chksum = swap_1_header.w_sector_chksum;
         pswap_header->w_sector_id     = swap_1_header.w_sector_id;
         pswap_header->w_size          = swap_1_header.w_size;
         pswap_header->swap_headchksum = swap_1_header.swap_headchksum;
         memcpy(pswap_header->statistic,swap_1_header.statistic,sizeof(EC_FLASH_STATISTIC_T) * EC_FLASH_STATISTIC_NUM_MAX);

         return (ret);
    }

    if(  swap_1_header.w_counter > swap_2_header.w_counter ||
        (swap_2_header.w_counter == EC_FLASH_DEFAULT_VAL &&  swap_1_header.w_counter == EC_FLASH_DEFAULT_VAL))
    {
         ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashGetActivateSwap_5, P_INFO, 0,"active is swap1");
        *active_fdt   = EC_FLASH_BLOCK_ID_SWAP1;
        *inactive_fdt = EC_FLASH_BLOCK_ID_SWAP2;
        *active_addr  = SECTOR_SWAP_ONE_OFFSET;
        *inactive_addr = SECTOR_SWAP_TWO_OFFSET;
         pswap_header->w_counter =  swap_1_header.w_counter;
         pswap_header->w_sector_chksum = swap_1_header.w_sector_chksum;
         pswap_header->w_sector_id     = swap_1_header.w_sector_id;
         pswap_header->w_size          = swap_1_header.w_size;
         pswap_header->swap_headchksum  = swap_1_header.swap_headchksum;
         memcpy(pswap_header->statistic,swap_1_header.statistic,sizeof(EC_FLASH_STATISTIC_T) * EC_FLASH_STATISTIC_NUM_MAX);
    }
    else
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashGetActivateSwap_6, P_INFO, 0,"active is swap2");

        *active_fdt   =   EC_FLASH_BLOCK_ID_SWAP2;
        *inactive_fdt =   EC_FLASH_BLOCK_ID_SWAP1;
        *active_addr   =  SECTOR_SWAP_TWO_OFFSET;
        *inactive_addr =  SECTOR_SWAP_ONE_OFFSET;
         pswap_header->w_counter       = swap_2_header.w_counter;
         pswap_header->w_sector_chksum = swap_2_header.w_sector_chksum;
         pswap_header->w_sector_id     = swap_2_header.w_sector_id;
         pswap_header->w_size          = swap_2_header.w_size;
         pswap_header->swap_headchksum  = swap_2_header.swap_headchksum;
         memcpy(pswap_header->statistic,swap_2_header.statistic,sizeof(EC_FLASH_STATISTIC_T) * EC_FLASH_STATISTIC_NUM_MAX);

    }

    if( pswap_header->w_counter == EC_FLASH_DEFAULT_VAL)
        pswap_header->w_counter = 1;
#if 0
    for(index = 0 ; index < EC_FLASH_STATISTIC_NUM_MAX; index++)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashGetActivateSwap_7, P_INFO, 3,
                                "sector:%d,statistic[erase:%d,write:%d]",index,
                                pswap_header->statistic[index].erase_count,
                                pswap_header->statistic[index].write_count);
    }
#endif
    return 0;
}




/**
  \fn          ecSoftSimFlashFetchAllSectorData
  \brief       read the softsim flash data according nameID
  \param[in]   index : nameID
  \param[in]   outbuffMax : output buf max size
  \param[out]  dataOut : the reading date of flash
  \param[out]  outlen :  the reading date actual Length
  \returns     offset
*/

INT8  ecSoftSimFlashFetchAllSectorData(UINT8 block_id,UINT32 phyaddr, UINT16 elementSize, UINT16 bufSize,UINT8 *saveBuf,
        UINT8   *sectorDataBuf,UINT16 *count,UINT16 *Len)
{
    INT8    ret    = -1;
    UINT8   iloop  = 0;
    UINT16  allLen = 0;
    UINT16  destLen = 0;
    UINT16  findcount = 0;
    EC_CTCC_FLASH_ELEMENT_T   *pElemData = NULL;

    *Len = 0 ;
    for(iloop = 0 ;iloop < SOFTSIM_FLASH_DATA_ONE_SIZE/elementSize ; iloop++)
    {
        memset(saveBuf, 0x0, elementSize);
        ret = SoftsimReadRawFlash(phyaddr + iloop*elementSize,(UINT8 *)saveBuf,elementSize);
        if(ret != 0)
        {
            return (ret);
        }
        pElemData = NULL;
        pElemData = (EC_CTCC_FLASH_ELEMENT_T*)saveBuf;
        if( pElemData          == NULL   ||
            pElemData->sfs_id  >= 0xFFFF ||
            pElemData->sfs_plainLen == 0 ||
            pElemData->sfs_plainLen >= 0xFFFF ||
            (pElemData->sfs_plainLen == 0 && pElemData->sfs_secLen == 0))
        {
             continue;
        }
        if(ecSoftSimFlashFindRecordTable(pElemData->sfs_id) == NULL)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashRecoveryData_2, P_INFO, 1,"invalid sfs_id, return error",pElemData->sfs_id);
            return (ret) ;
        }
        if(allLen < bufSize )
        {
            destLen  =  sizeof(EC_CTCC_FLASH_ELEMENT_T) +
                              (pElemData->sfs_isEnc == 0 ? pElemData->sfs_plainLen : pElemData->sfs_secLen );
            memcpy(sectorDataBuf +  allLen,  saveBuf, destLen);
            allLen += destLen;
            findcount++;
        }
    }

    if(Len)   *Len   = allLen;
    if(count) *count = findcount;
    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashFetchAllSectorData_1, P_INFO, 2,"finally fetch:count:%d,Len %d",findcount,allLen);
    return (0);
}

/**
  \fn          ecSoftSimFlashDataRead
  \brief       read the softsim flash data according nameID
  \param[in]   index : nameID
  \param[in]   outbuffMax : output buf max size
  \param[out]  dataOut : the reading date of flash
  \param[out]  outlen :  the reading date actual Length
  \returns     offset
*/

INT8 ecSoftSimFlashDataRead(UINT16 index,UINT16 outbuffMax,UINT8 *dataOut,  UINT32 *outlen,UINT8 isSec)
{
    INT8    ret        = -1;
    UINT32  phyAddr    = 0x0;
    UINT16  sectorId   = 0;
    UINT16  destLen    = 0;
    EC_CTCC_FLASH_ELEMENT_T   ElemData;
    EC_CTCC_FLASH_ELEMENT_T  *pelemData = &ElemData;
    EC_CTCC_FLASH_RECORD_T   *pRecordData = NULL;
    UINT8       *readBuf = NULL;

    if(outlen)  *outlen = 0;

    pRecordData = ecSoftSimFlashFindRecordTable(index);
    if(pRecordData == NULL)
    {
         ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataRead_1, P_WARNING, 1,
                "pElementData NULL index :%d",index);
         return (ret);
    }
    sectorId = pRecordData->block_id;

    if(sectorId == EC_FLASH_BLOCK_ID_HEADER ||
       sectorId == EC_FLASH_BLOCK_ID_INVALID  )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataRead_2, P_WARNING, 1, "sectorId :%d  invalid",sectorId);
        return (ret);
    }

    phyAddr = pRecordData->offset;
    if(phyAddr < SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(sectorId) ||
       phyAddr > SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(sectorId) + SOFTSIM_FLASH_MAX_SIZE )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataRead_3, P_WARNING, 1, "invalid   phyAddr 0x%x",phyAddr);
        return (ret);
    }
    memset(pelemData,0XFF,sizeof(EC_CTCC_FLASH_ELEMENT_T));

    if((ret = SoftsimReadRawFlash(phyAddr,(UINT8 *)pelemData,sizeof(EC_CTCC_FLASH_ELEMENT_T))) != QSPI_OK)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataRead_4, P_WARNING, 0, "SoftsimReadRawFlash  failure!!!!");
        return (ret);
    }

    if(pelemData->sfs_id    != index ||
       pelemData->sfs_isEnc != isSec)
    {
        if(EC_SOFT_FLASH_SUP_KEYDATA_BACKUP)
        {
            goto keydataRecyProc;
        }
        else
        {
            return (ret);
        }
    }

    if(isSec == 0)
    {
        if(INVALID_PLAIN_DATA(pelemData))
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataRead_8, P_WARNING, 0, "invalid !!!!");
            return (ret);
        }
        destLen =  pelemData->sfs_plainLen;
    }
    else
    {
        if(INVALID_ENC_DATA(pelemData))
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataRead_9, P_WARNING, 0, "invalid !!!!");
            return (ret);
        }
        destLen =  pelemData->sfs_secLen;
    }

    if(destLen >= 0XFFFF ||
       destLen == 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataRead_10, P_WARNING, 0, "invalid sfs_len !!!!");
        if(EC_SOFT_FLASH_SUP_KEYDATA_BACKUP)
        {
            goto keydataRecyProc;
        }
        else
        {
            return (ret);
        }
    }

    if((ret = SoftsimReadRawFlash(phyAddr + sizeof(EC_CTCC_FLASH_ELEMENT_T),dataOut,destLen)) != QSPI_OK)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataRead_11, P_WARNING, 0, "SoftsimReadRawFlash  failure!!!!");
        return (ret);
    }

    if(QSPI_OK == ret)
    {
        if(isSec == 0)
        {
            *outlen = pelemData->sfs_plainLen;
        }
        else
        {
            *outlen = (pelemData->sfs_plainLen << 16 | pelemData->sfs_secLen);
        }

    }

    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataRead_12,  P_INFO, 6,
                                    "Index:%d,isSec:%d,len:%d,plainLen:%d,seclen:%d,ret:%d",
                                    index,isSec,destLen,pelemData->sfs_plainLen ,pelemData->sfs_secLen,ret);
    ECOMM_DUMP(UNILOG_SOFTSIM,  ecSoftSimFlashDataRead_13, P_INFO,"READ->",destLen,dataOut);
    return (0);

keydataRecyProc:

    pRecordData = NULL;
    pelemData   = NULL;
    if((pRecordData = ecSoftSimFlashFindKeyData(index)) != NULL)
    {
        readBuf = malloc(128);
        if(readBuf)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataRead_14, P_INFO, 0, "read addr 0x%x",pRecordData->offset);
            if((ret = SoftsimReadRawFlash(pRecordData->offset,(UINT8 *)readBuf,sizeof(EC_CTCC_FLASH_ELEMENT_T))) != QSPI_OK)
            {
                ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataRead_15, P_WARNING, 0, "SoftsimReadRawFlash recy failure!!!!");
                free(readBuf);
                return (0);
            }
            pelemData = (EC_CTCC_FLASH_ELEMENT_T *)readBuf;
            if(0 == SoftsimFlashCheckSectorIsErased(readBuf,sizeof(EC_CTCC_FLASH_ELEMENT_T)))
            {
                ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataRead_16,  P_INFO, 4,
                                            "read backup[sfs_id:%d,isSec:%d,plainLen:%d,secLen:%d]",
                                            pelemData->sfs_id,pelemData->sfs_isEnc,pelemData->sfs_plainLen,pelemData->sfs_secLen);
                if(isSec != pelemData->sfs_isEnc ||
                   index != pelemData->sfs_id)
                {
                    free(readBuf);
                    return (0);
                }

                destLen =  (pelemData->sfs_isEnc == 0 ?   pelemData->sfs_plainLen : pelemData->sfs_secLen);
                if((ret = SoftsimReadRawFlash(pRecordData->offset + sizeof(EC_CTCC_FLASH_ELEMENT_T),dataOut,destLen)) != QSPI_OK)
                {
                    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataRead_17, P_WARNING, 0, "SoftsimReadRawFlash  failure!!!!");
                    free(readBuf);
                    return (0);
                }

                *outlen = (pelemData->sfs_isEnc == 0 ?
                           pelemData->sfs_plainLen : (pelemData->sfs_plainLen << 16 | pelemData->sfs_secLen) );

                free(readBuf);

            }

        }
    }


    return  (0);

}


/**
  \fn          ecSoftSimFlashFindRecordTable
  \brief       find corresponding mapping data of softsim flash
  \param[in]   element : nameID
  \param[out]  ctcc_flash_table[index] :  corresponding mapping data of softsim flash
*/
static EC_CTCC_FLASH_RECORD_T  *ecSoftSimFlashFindRecordTable(UINT16 element)
{
    UINT16 index = 0;
    for(index = 0;index  < SFS_FILE_ID_EID + 1;index++)
    {
        if(ctcc_flash_table[index].sfs_id == element)
        {
            return  &ctcc_flash_table[index];
        }
    }
    return NULL;
}

 UINT8  ecSoftSimFlashFindBlockSize(UINT16 block_id)
{
    UINT16 index = 0;
    for(index = 0;index  < SFS_FILE_ID_EID + 1;index++)
    {
        if(ctcc_flash_table[index].block_id == block_id)
        {
            return  ctcc_flash_table[index].maxsize;
        }
    }
    return 0;
}


/**
  \fn          SoftsimFlashCheckSectorIsErased
  \brief       check  the reading softsim flash is erased or not
  \param[in]   pbuf : reading softsim flash buffer data
  \param[in]   pbuf : reading softsim flash buffer data size
  \returns     1: Erased;0: not erased
*/

static UINT8 SoftsimFlashCheckSectorIsErased(UINT8 *pbuf,UINT32 size)
{
    UINT32 index = 0;
    for(index = 0 ;index < size; index++)
    {
        if(pbuf[index] != 0XFF)
        {
            return 0;
        }
    }
    return  1;

}

INT8 ecSoftSimFlashWriteSwap(UINT8 index ,UINT8 sector_id, UINT16 dataLen, UINT8 *dataIn,UINT16 preLen, UINT16 preCount)
{
    INT8     ret = -1;
    UINT8    active_swap_id      = 0;
    UINT8    inactive_swap_id    = 0;
    UINT32   active_swap_addr    = 0;
    UINT32   inactive_swap_addr  = 0;
    UINT32   phyAddr = 0;
    UINT16   findcount  = 0;
    UINT16   findeleLen = 0;
    UINT16   dyBufLen   = SOFTSIM_FLASH_DATA_ONE_SIZE/2 ;

    EC_FLASH_SWAP_HEADER_T     swap_header = {0};
    UINT8                     *pSectorBuf  = NULL;


    if(bSoftSimEnable == FALSE)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashWriteSwap_0, P_INFO, 0, "recovery need under softsim environment ");
        return (0);
    }

    memset(&swap_header,0x0,sizeof(EC_FLASH_SWAP_HEADER_T));
    if((ret = ecSoftSimFlashGetActivateSwap(&active_swap_id,&inactive_swap_id,&active_swap_addr,&inactive_swap_addr,&swap_header)) != 0  ||
              (active_swap_id   != EC_FLASH_BLOCK_ID_SWAP1 && active_swap_id   != EC_FLASH_BLOCK_ID_SWAP2)  ||
              (inactive_swap_id != EC_FLASH_BLOCK_ID_SWAP1 && inactive_swap_id != EC_FLASH_BLOCK_ID_SWAP2)  ||
              (swap_header.w_counter  == EC_FLASH_DEFAULT_VAL))
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashWriteSwap_1, P_INFO, 1,
                "ecSoftSimFlashGetActivateSwap %d error!!!",ret);
        return (0);
    }

    pSectorBuf = (UINT8 *)malloc(dyBufLen);
    if(PNULL  == pSectorBuf )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashWriteSwap_2, P_INFO, 0, "malloc failed!!!");
        return (0);
    }

    memset(pSectorBuf, 0x0, dyBufLen);

    if(SoftsimBackupDataToSwap(sector_id,dataLen,dataIn,pSectorBuf + sizeof(EC_FLASH_SWAP_HEADER_T),&findcount, &findeleLen) != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashWriteSwap_3, P_INFO, 1,
                        "read Mainsector 0x%x failed!!!",phyAddr);
        free(pSectorBuf);
        return (0);
    }

    if(findeleLen != preLen ||
       findcount  != preCount)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashWriteSwap_4, P_INFO, 0,"result is not expected");
        free(pSectorBuf);
        return (0);
    }

    if(SoftsimFlashCheckSectorIsErased(pSectorBuf + sizeof(EC_FLASH_SWAP_HEADER_T),(UINT32)findeleLen))
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashWriteSwap_5, P_INFO, 1,
                                    "sector:%d is first access, return",sector_id);
        free(pSectorBuf);
        return (0);
    }

    /*debug end*/
    swap_header.w_counter++;
    swap_header.w_sector_id = sector_id;
    swap_header.statistic[sector_id].write_count++;

    /*record total length that will be write into swap */
    swap_header.w_size      = findeleLen;
    /*record data checksum that will be write into swap */
    swap_header.w_sector_chksum = _ecSoftSimFlashChksum((void *)(pSectorBuf + sizeof(EC_FLASH_SWAP_HEADER_T)),findeleLen);
    /*finally should re-calculate the  data checksum include former data(such as
      w_counter,w_sector_id,w_size,w_sector_chksum, etc....),please note that it not included the reserved data  */
    swap_header.swap_headchksum = _ecSoftSimFlashChksum(&swap_header, 16);

    swap_header.swap_datachksum = _ecSoftSimFlashChksum(pSectorBuf + sizeof(EC_FLASH_SWAP_HEADER_T), findeleLen);
    /*step 5: success find the inactivate sector and store data on it before erase this sector,first erase this swap */
    if((ret = SoftsimEraseRawFlash(inactive_swap_addr,(UINT32)SOFTSIM_FLASH_DATA_ONE_SIZE)) != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashWriteSwap_6, P_INFO, 1, "ecSoftSimFlashGetActivateSwap %d error !!!",ret);
        free(pSectorBuf);
        return (0);
    }

    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashWriteSwap_7, P_INFO, 3,
             "write sector:%d,w_sector_chksum 0x%x,chksum:0x%x",sector_id,swap_header.w_sector_chksum,swap_header.swap_headchksum);

    /*write the swap area header:sizeof： EC_FLASH_SWAP_HEADER_T*/

    findeleLen += sizeof(EC_FLASH_SWAP_HEADER_T);
    memcpy((void*)pSectorBuf,&swap_header,sizeof(EC_FLASH_SWAP_HEADER_T));

    ret = SoftsimWriteToRawFlash(inactive_swap_addr,(UINT8 *)pSectorBuf, findeleLen);
    if(ret != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashWriteSwap_8, P_INFO, 1, "ecSoftSimFlashGetActivateSwap %d error!!!",ret);
        free(pSectorBuf);
        return (0);
    }

    free(pSectorBuf);
    return (0);

}


/**
  \fn          ecSoftSimFlashDataWrite
  \brief       write the flash data to corresponding flash position
  \param[in]   index : NameID
  \param[in]   dataIn : the writting data
  \param[in]   dataIn : the writting data  Length
  \returns     0: Write OK ;-1 :Write FAILED
*/
INT8 ecSoftSimFlashDataWrite(const UINT16 index, const UINT8  *dataIn, const UINT16 plainLen, const UINT16 encLen,const UINT8  isSec)
{
    INT8    ret = -1;
    UINT32  phyAddr   = 0x0;
    UINT8   sectorId  = EC_FLASH_BLOCK_ID_INVALID;
    UINT8  *saveBuf   = PNULL;
    UINT16  maxSize   = 0;
    UINT16  jloop,iloop = 0;
    UINT16  offlen    = 0;
    UINT8   bFind     = 0;
    UINT32  checkLen  = 0;
    UINT16  findcount = 0;
    UINT16  origLen   = 0;
    UINT16  destLen   = 0;
    UINT16  writeLen  = 0;
    UINT16  prepWLen  = 0;
    EC_CTCC_FLASH_RECORD_T    *pRecordData = NULL;
    EC_CTCC_FLASH_ELEMENT_T   *pElemData   = NULL;
    UINT8                     *TempBuf     = NULL;

    if(isSec == 0)
    {
        if(encLen  != 0 ||
           plainLen == 0 )
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_0, P_INFO, 0, "INVALID!!!");
            return (ret);
        }
        destLen = plainLen;
    }
    else
    {
        if(plainLen == 0 ||
           encLen   == 0 )
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_1, P_INFO, 0, "INVALID!!!");
            return (ret);
        }
        destLen = encLen;
    }

    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_2, P_INFO, 4,
                                "isSec:%d,origLen:%d,secLen:%d,destLen:%d",isSec,plainLen,encLen,destLen);
    pRecordData = ecSoftSimFlashFindRecordTable(index);
    if(pRecordData == NULL)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_3, P_INFO, 0, "pRecordData NULL ");
        return (ret);
    }

    maxSize  =  pRecordData->maxsize;
    sectorId =  pRecordData->block_id;
    if(sectorId  <= EC_FLASH_BLOCK_ID_HEADER ||
       sectorId  >= EC_FLASH_BLOCK_ID_INVALID  )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_4, P_INFO, 1, "sectorId :%d  invalid",sectorId);
        return (ret);
    }

    if( maxSize == 0  ||
        destLen > maxSize)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_5, P_INFO, 2,
                    " maxSize:%d, origLen:%d secLen%d INVALID!!!",maxSize,destLen);
        return (ret);
    }

    phyAddr = pRecordData->offset;
    if( phyAddr < SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(0) ||
        phyAddr > SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(0) + SOFTSIM_FLASH_MAX_SIZE )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_6, P_WARNING, 1, "invalid   phyAddr 0x%x",phyAddr);
        return (ret);
    }

    /*1st malloc the buffer for saving data*/
    saveBuf =  (UINT8 *)malloc(maxSize);
    if(PNULL == saveBuf )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_7, P_INFO, 0, "malloc failed!!!");
        return (ret);
    }

    memset(saveBuf,0x0,maxSize);
    ret = SoftsimReadRawFlash(phyAddr,saveBuf,(UINT32)maxSize);
    if(ret != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_8, P_INFO, 1, "read offAddr 0x%x error!!!",phyAddr);
        free(saveBuf);
        saveBuf = NULL;
        return (ret);
    }

    pElemData = NULL;
    pElemData = (EC_CTCC_FLASH_ELEMENT_T*)saveBuf;
    if(pElemData->sfs_id != 0XFFFF &&
       pElemData->sfs_id != index)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_9, P_INFO, 1, "read index %d invalid !!!",pElemData->sfs_id);
        free(saveBuf);
        saveBuf = NULL;
        return (-1);
    }

    if(isSec == 0)
    {
        if((pElemData->sfs_plainLen == destLen ) &&
            (memcmp(saveBuf + sizeof(EC_CTCC_FLASH_ELEMENT_T),(const void*)dataIn,destLen) == 0))
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_10, P_INFO, 0, "ORI data is equal to orginal,RETURN");
            free(saveBuf);
            saveBuf = NULL;
            return (0);
        }
    }
    else if((pElemData->sfs_secLen == destLen ) &&
            ( memcmp(saveBuf + sizeof(EC_CTCC_FLASH_ELEMENT_T),(const void*)dataIn,destLen) == 0))
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_11, P_INFO, 0, "SEC data is equal to orginal,RETURN");
        free(saveBuf);
        saveBuf = NULL;
        return (0);
    }

    if(SoftsimFlashCheckSectorIsErased(saveBuf,(UINT32)maxSize))
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_12, P_INFO, 0, " NO Need erase");
        memset(saveBuf, 0, maxSize);

        pElemData = (EC_CTCC_FLASH_ELEMENT_T*)saveBuf;
        pElemData->sfs_id  = index;
        if(isSec == 0)
        {
            pElemData->sfs_isEnc    = 0;
            pElemData->sfs_plainLen = destLen;
            pElemData->sfs_secLen   = 0;
        }
        else
        {
            pElemData->sfs_isEnc    = 1;
            pElemData->sfs_plainLen = plainLen;
            pElemData->sfs_secLen   = destLen;
        }

        memcpy(saveBuf +  sizeof(EC_CTCC_FLASH_ELEMENT_T),dataIn,destLen);
        ret = SoftsimWriteToRawFlash(phyAddr,saveBuf,(UINT32)(destLen + sizeof(EC_CTCC_FLASH_ELEMENT_T) ));
        if(ret != 0)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_13, P_WARNING, 3,
                   "write index:%d,finaAddr 0x%x,finaSize:%d FAIL!!!",index,phyAddr,destLen);
            free(saveBuf);
            saveBuf = NULL;
            return (ret);
        }

    }
    else
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_14, P_INFO, 0, "Need to erase");

        if(pElemData->sfs_id != index)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_15, P_INFO, 1, "read index %d invalid !!!",pElemData->sfs_id);
            free(saveBuf);
            saveBuf = NULL;
            return (-1);
        }

        TempBuf   = (UINT8 *)malloc(SOFTSIM_FLASH_DATA_ONE_SIZE/2);
        if(PNULL  == TempBuf )
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_16, P_INFO, 0, "malloc failed!!!");
            free(saveBuf);
            saveBuf = NULL;
            return (-1);
        }

        memset(TempBuf, 0X0, SOFTSIM_FLASH_DATA_ONE_SIZE/2);

        phyAddr = SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(sectorId);
        for(iloop = 0 ;iloop < SOFTSIM_FLASH_DATA_ONE_SIZE/maxSize ; iloop++)
        {
            memset(saveBuf,0x0,maxSize);
            ret = SoftsimReadRawFlash(phyAddr + iloop*maxSize,(UINT8 *)saveBuf,maxSize);
            if(ret != 0)
            {
                 free(saveBuf);
                 free(TempBuf);
                 return (ret);
            }

            pElemData = NULL;
            pElemData = (EC_CTCC_FLASH_ELEMENT_T*)saveBuf;
            if(pElemData          == NULL   ||
               pElemData->sfs_id  >= 0xFFFF ||
               pElemData->sfs_plainLen == 0 ||
               pElemData->sfs_plainLen >= 0xFFFF ||
               (pElemData->sfs_plainLen == 0 && pElemData->sfs_secLen == 0))
            {
                continue;
            }

            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_17, P_INFO, 5,
                        "read id %d isEnc:%d,at addr 0x%x plainLen:%d,secLen:%d",
                        pElemData->sfs_id,pElemData->sfs_isEnc,phyAddr + iloop*maxSize, pElemData->sfs_plainLen , pElemData->sfs_secLen );

            ECOMM_DUMP(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_18, P_INFO, "read->",
                        (pElemData->sfs_isEnc == 0 ? pElemData->sfs_plainLen : pElemData->sfs_secLen ),saveBuf + sizeof(EC_CTCC_FLASH_ELEMENT_T));

            if(pElemData->sfs_id == index)
            {
                 pElemData = NULL;
                 pElemData = (EC_CTCC_FLASH_ELEMENT_T*)(TempBuf + offlen);
                 pElemData->sfs_id  = index;
                 if(pElemData->sfs_isEnc == 0)
                 {
                     pElemData->sfs_plainLen = destLen;
                     pElemData->sfs_secLen   = 0;
                 }
                 else
                 {
                     pElemData->sfs_plainLen = plainLen;
                     pElemData->sfs_secLen   = destLen;
                 }
                 memcpy(TempBuf + sizeof(EC_CTCC_FLASH_ELEMENT_T) + offlen, dataIn,  maxSize);
                 findcount++;
                 ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_19, P_INFO, 4,
                        "find %d isEnc:%d,plainLen:%d,secLen:%d",
                        pElemData->sfs_id,pElemData->sfs_isEnc, pElemData->sfs_plainLen , pElemData->sfs_secLen );
                 prepWLen +=  destLen + (UINT16)(sizeof(EC_CTCC_FLASH_ELEMENT_T));

            }
            else
            {
                 memcpy(TempBuf  + offlen, saveBuf, maxSize);
                 findcount++;
                 prepWLen += (UINT16)(sizeof(EC_CTCC_FLASH_ELEMENT_T)) +
                             (pElemData->sfs_isEnc == 0 ? pElemData->sfs_plainLen : pElemData->sfs_secLen) ;
            }
            offlen += maxSize;

        }

        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_20, P_INFO, 3,
                 "sectorId:%d,find %d Len:%d records ", sectorId,findcount,prepWLen);

        /*here add the swap area backup*/
        if(EC_SOFT_FLASH_SUP_RECOVERY)
        {
            if(ecSoftSimFlashWriteSwap(index, sectorId,SOFTSIM_FLASH_DATA_ONE_SIZE/2,TempBuf,prepWLen,findcount) != 0)
            {
                ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_21, P_INFO, 1,
                                            "write %d at swap fail!!!", index);
            }
        }

        ret = SoftsimEraseRawFlash(SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(sectorId),(UINT32)SOFTSIM_FLASH_DATA_ONE_SIZE);
        if(ret != 0)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_22, P_INFO, 1, "erase sectorId %d error!!!",sectorId);
            free(saveBuf);
            free(TempBuf);
            return (ret);
        }

        findcount = 0;
        for(jloop = 0;jloop  < SFS_FILE_ID_EID + 1;jloop++)
        {
            pRecordData = NULL;
            pRecordData = &ctcc_flash_table[jloop];

            if(pRecordData->block_id == sectorId)
            {
                bFind = 0;
                for(iloop = 0 ;iloop < SOFTSIM_FLASH_DATA_ONE_SIZE/maxSize ;iloop++)
                {
                     pElemData = NULL;
                     pElemData = (EC_CTCC_FLASH_ELEMENT_T*)(TempBuf + iloop*maxSize);
                     if ( pElemData           != NULL   &&
                          pElemData->sfs_id   != 0XFFFF &&
                          pRecordData->sfs_id == pElemData->sfs_id)
                     {
                        if(pElemData->sfs_isEnc == 0)
                        {
                            if(pElemData->sfs_plainLen != 0 && pElemData->sfs_plainLen != 0XFFFF)
                            {
                                bFind = 1;
                                findcount++;
                                break;
                            }
                        }else if(pElemData->sfs_isEnc == 1)
                        {
                            if(pElemData->sfs_plainLen != 0 && pElemData->sfs_plainLen != 0XFFFF &&
                               pElemData->sfs_secLen != 0   && pElemData->sfs_secLen   != 0XFFFF)
                            {
                                bFind = 1;
                                findcount++;
                                break;
                            }
                        }
                     }
                }

                if(bFind == 1 && pElemData != NULL )
                {
                    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_23, P_INFO, 4, "write id %d isEnc:%d,at addr 0x%x len:%d",
                                pElemData->sfs_id,pElemData->sfs_isEnc,pRecordData->offset,
                                (pElemData->sfs_isEnc == 0 ? pElemData->sfs_plainLen : pElemData->sfs_secLen ));
                    if(pElemData->sfs_isEnc == 0)
                    {
                        offlen = pElemData->sfs_plainLen;
                    }
                    else
                    {
                        offlen = pElemData->sfs_secLen;
                    }
                    ret = SoftsimWriteToRawFlash(pRecordData->offset,
                                                 (UINT8 *)pElemData, (UINT32)(offlen + sizeof(EC_CTCC_FLASH_ELEMENT_T)));
                    if(ret != 0)
                    {
                        free(saveBuf);
                        free(TempBuf);
                        return (ret);
                    }
                    writeLen += (offlen + sizeof(EC_CTCC_FLASH_ELEMENT_T));
                }
            }
        }

        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_24, P_INFO, 2,
             "write %d len:%d,valid records", findcount,writeLen);
    }

    osDelay(5);

    memset(saveBuf,0x0,destLen);
    ecSoftSimFlashDataRead(index,maxSize,saveBuf,&checkLen,isSec);

    if(isSec == 0)
    {
        if((UINT16)checkLen != destLen)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_25, P_WARNING, 2,
                    " writeLen %d,readLen%d, INVALID !!!",destLen,checkLen);
            free(saveBuf);
            free(TempBuf);
            return (-1);
        }
    }
    else
    {
        origLen = (UINT16)(checkLen);
        if((UINT16)origLen != destLen)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_26, P_WARNING, 2,
                    " writeLen %d,readLen%d, INVALID !!!",destLen,checkLen);
            free(saveBuf);
            free(TempBuf);
            return (-1);
        }
    }


    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_27, P_INFO, 3, "index:%d,isSec:%d,read:%d",index,isSec,destLen);
    ECOMM_DUMP(UNILOG_SOFTSIM,  ecSoftSimFlashDataWrite_28, P_INFO, "read->",destLen,saveBuf);

    if(memcmp(saveBuf,(const void*)dataIn,destLen) == 0)
    {
        ret = 0;
    }
    else
    {
        ret  = 1;
    }

    /*support the feature of KEY DATA BACKUP*/
    if(EC_SOFT_FLASH_SUP_KEYDATA_BACKUP)
    {
        pRecordData = NULL;
        if((pRecordData = ecSoftSimFlashFindKeyData(index)) != NULL)
        {
            memset(saveBuf,0x0,maxSize);
            if((ret = SoftsimReadRawFlash(pRecordData->offset,(UINT8 *)saveBuf,destLen)) == QSPI_OK)
            {
                 if(SoftsimFlashCheckSectorIsErased(saveBuf,(UINT32)destLen))
                 {
                    pElemData = NULL;
                    memset(saveBuf,0x0,maxSize);
                    pElemData = (EC_CTCC_FLASH_ELEMENT_T *)saveBuf;
                    pElemData->sfs_id    = index ;
                    pElemData->sfs_isEnc = isSec;
                    if(isSec == 0)
                    {
                        pElemData->sfs_plainLen = destLen;
                        pElemData->sfs_secLen   = 0;
                    }
                    else
                    {
                        pElemData->sfs_plainLen = plainLen;
                        pElemData->sfs_secLen   = destLen;
                    }
                    memcpy(saveBuf + sizeof(EC_CTCC_FLASH_ELEMENT_T), dataIn,  destLen);

                    ECOMM_DUMP(UNILOG_SOFTSIM,  ecSoftSimFlashDataWrite_31, P_INFO,
                                                "write backup->",sizeof(EC_CTCC_FLASH_ELEMENT_T) +destLen ,saveBuf);
                    SoftsimWriteToRawFlash(pRecordData->offset,
                                              (UINT8 *)saveBuf, (UINT32)(destLen + sizeof(EC_CTCC_FLASH_ELEMENT_T)));

                 }
            }
        }
    }

    free(saveBuf);
    saveBuf = NULL;

    free(TempBuf);
    TempBuf = NULL;

    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashDataWrite_32, P_WARNING, 2, "write index %d result:%d",index,ret);
    return (ret);

}

/**
  \fn          ecSoftSimFlashConstructAesData
  \brief       construct the Encryption data by AES128 CBC mode
  \param[in]   dataIn :   the original plain data
  \param[in]   kvLen : the writting data  Len
  \param[out]  EncData :  the Encryption data pointer
  \param[out]  outlen :   the Encryption data length
  \returns     0: Write OK ;-1 :Write FAILED
*/
UINT8 ecSoftSimFlashConstructAesData(const int mode,const UINT8  *dataIn, const UINT16 kvLen, UINT8 **EncData, UINT16 *outlen)
{
    int  ret= -1;
    UINT16 modoffset    = 0;
    UINT16 remainoffset = 0;
    UINT16 encNum       = 0;
    UINT8  *pTempEncdata =  NULL;
    UINT8   iv_in[]      =  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    mbedtls_aes_context aes_context = {0};

    EC_mbedtls_aes_init(&aes_context );

    modoffset    = kvLen / 16 ;
    remainoffset = kvLen % 16;
    if(remainoffset)
    {
        modoffset += 1;
    }
    else
    {
        if(kvLen != modoffset *16)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashConstructAesData_0, P_INFO,0,
                        "unexpected error!");
            return (ECC_RESULTE_FAIL);
        }
    }

    if(modoffset >= 16)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashConstructAesData_00, P_INFO,0,
                    "unexpected error!");
        return (ECC_RESULTE_FAIL);
    }

    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashConstructAesData_1, P_INFO,2,
                "modoffset:%d,remainoffset:%d",modoffset,remainoffset);

    encNum = modoffset * 16;
    if((pTempEncdata = (UINT8 *)malloc(encNum)) == NULL)
    {
        return (ECC_RESULTE_FAIL);
    }

    memset(pTempEncdata,0x0,encNum);
    if((ret = (mbedtls_aes_crypt_cbc_hw(&aes_context, mode ,encNum ,iv_in,dataIn,pTempEncdata))) != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashConstructAesData_2, P_INFO,1, " ret:%d ,FAIL",ret);
        EC_mbedtls_aes_free(&aes_context);
        free(pTempEncdata);
        return (ECC_RESULTE_FAIL);
    }

    *EncData = pTempEncdata;
    *outlen  = encNum;

    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashConstructAesData_3, P_INFO,1, "read EncLen:%d",encNum);
    ECOMM_DUMP(UNILOG_SOFTSIM,  ecSoftSimFlashConstructAesData_4, P_INFO,  "ENC->",encNum,pTempEncdata);

    return  (ECC_RESULTE_SUCC);
}

/**
  \fn          ecSoftSimFlashEncryDataWrite
  \brief       write the Encryption data to corresponding flash position
  \param[in]   index : NameID
  \param[in]   dataIn : the writting data
  \param[in]   dataIn : the writting data  Length
  \returns     0: Write OK ;-1 :Write FAILED
*/

INT8 ecSoftSimFlashEncryDataWrite(const UINT16 index, const UINT8  *dataIn, const UINT16 inBuffLen)
{
    INT8       ret      = -1;
    UINT8     *encData  = NULL;
    UINT16     encLen  = 0;

    if(ecSoftSimFlashConstructAesData(0,dataIn,inBuffLen,&encData,&encLen) != ECC_RESULTE_SUCC)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashEncryDataWrite_0, P_INFO, 0, "construct AES data FAIL!!!");
        return (ret);
    }

    if(ecSoftSimFlashDataWrite(index,encData,inBuffLen,encLen,1) != 0)
    {
        free(encData);
        return (ret);
    }
    return 0;
}

/**
  \fn          _ecSoftSimFlashChksum
  \brief       checksum
  \param[in]   dataptr : the dats input into checksum
  \param[in]   len : the dats input into checksum length
  \returns     UINT16 src: checksum result
*/
static UINT32 _ecSoftSimFlashChksum(void *dataptr, UINT16 len)
{
  UINT32  acc;
  UINT32  src;
  UINT8   *octetptr;

  acc = 0;
  octetptr = (UINT8*)dataptr;
  while (len > 1) {
    src = (*octetptr) << 8;
    octetptr++;
    src |= (*octetptr);
    octetptr++;
    acc += src;
    len -= 2;
  }
  if (len > 0) {
    src = (*octetptr) << 8;
    acc += src;
  }

  acc = (acc >> 16) + (acc & 0x0000ffffUL);
  if ((acc & 0xffff0000UL) != 0) {
    acc = (acc >> 16) + (acc & 0x0000ffffUL);
  }

  src = acc;
  return ~src;
}

 INT8 ecSoftSimFlashRecovery(void)
{
    INT8     ret                = -1;
    UINT8    *sectorDataBuf     = NULL;
    UINT32   phyaddr            = 0x0;
    UINT16   sector_id          = 0XFFFF;
    UINT16   eleMaxSize = 0 ;
    UINT16   dynaBufLen   = 2048 ;
    UINT32   calchksum    = 0;
    UINT16   findcount = 0;
    UINT16   findwriteLen = 0;
    UINT8                     *saveBuf     = NULL;
    UINT8                      active_swap_id  = 0;
    UINT8                      inactive_swap_id = 0;
    UINT32                     active_swap_addr   = 0;
    UINT32                     inactive_swap_addr = 0;
    EC_FLASH_SWAP_HEADER_T     act_swap_header   =  {0};

    if(bSoftSimEnable == FALSE)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckAndRecovery_0, P_INFO, 0, "recovery need softsim Enable");
        return (0);
    }

    if((ret = ecSoftSimFlashGetActivateSwap(&active_swap_id, &inactive_swap_id,&active_swap_addr,&inactive_swap_addr,&act_swap_header)) != 0   ||
        (active_swap_addr   != SECTOR_SWAP_ONE_OFFSET && active_swap_addr != SECTOR_SWAP_TWO_OFFSET)    ||
        (inactive_swap_addr != SECTOR_SWAP_ONE_OFFSET && inactive_swap_addr != SECTOR_SWAP_TWO_OFFSET)  ||
        (act_swap_header.w_counter == EC_FLASH_DEFAULT_VAL))
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckAndRecovery_1, P_INFO, 1, "ecSoftSimFlashGetActivateSwap %d error!!!",ret);
        return (ret);
    }

    if(act_swap_header.w_size == 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckAndRecovery_4, P_INFO, 0,
                                "no write never,return");
        return (0);
    }

    eleMaxSize = ecSoftSimFlashFindBlockSize(act_swap_header.w_sector_id);
    if(eleMaxSize == 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckAndRecovery_5, P_INFO, 0,
                                "eleMaxSize 0，return");
        return (0);
    }

    sector_id = act_swap_header.w_sector_id;
    phyaddr =  SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(act_swap_header.w_sector_id);

    sectorDataBuf = (UINT8 *)malloc(dynaBufLen);
    if(PNULL  == sectorDataBuf )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_5, P_INFO, 0, "malloc failed!!!");
        return (ret);
    }

    memset(sectorDataBuf, 0x0, dynaBufLen);

    saveBuf =  (UINT8 *)malloc(eleMaxSize);
    if(PNULL == saveBuf )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_6, P_INFO, 0, "malloc failed!!!");
        free(sectorDataBuf);
        return (0);
    }

    memset(saveBuf, 0x0, eleMaxSize);
    if((ret = ecSoftSimFlashFetchAllSectorData(sector_id,phyaddr,eleMaxSize,dynaBufLen,saveBuf,sectorDataBuf,&findcount,&findwriteLen)) != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_7, P_INFO, 0, "fetch data failed!!!");

        free(sectorDataBuf);
        free(saveBuf);
        return (0);
    }

     if(findcount    == 0  ||
        findwriteLen == 0  ||
        findcount * sizeof(EC_CTCC_FLASH_ELEMENT_T)  > findwriteLen ||
        act_swap_header.w_size != findwriteLen)
     {
         free(sectorDataBuf);
         free(saveBuf);

         ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_78, P_INFO, 0,"error occured,return");
         return (0);
     }

    /*if data area is never write, it not need to roolback data*/
    if(SoftsimFlashCheckSectorIsErased(sectorDataBuf,(UINT32)findwriteLen))
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_8, P_INFO, 1,"sector:%d data is 0XFFFF, return",sector_id);
        free(sectorDataBuf);
        free(saveBuf);
        return (0);
    }

    if(act_swap_header.swap_headchksum == _ecSoftSimFlashChksum(&act_swap_header, 16))
    {
        calchksum = _ecSoftSimFlashChksum((void *)sectorDataBuf,findwriteLen);

        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_9, P_WARNING, 3,
                                "sector:%d calchksum 0x%x,w_chksum 0x%x",sector_id,calchksum ,act_swap_header.w_sector_chksum);
        if (act_swap_header.w_sector_chksum == calchksum)
        {
            free(sectorDataBuf);
            free(saveBuf);
            return (0);
        }
        else
        {
            memset(sectorDataBuf, 0x0, dynaBufLen);
            ret = SoftsimReadRawFlash(active_swap_addr + sizeof(EC_FLASH_SWAP_HEADER_T),(UINT8 *)sectorDataBuf,act_swap_header.w_size);
            if(ret != 0)
            {
                ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_11, P_INFO, 1,"sector:%d read fail!",sector_id);
                free(sectorDataBuf);
                free(saveBuf);
                return (0);
            }

            if(act_swap_header.w_size > 0)
            {
                /*here should check again for avoiding the mistake operations*/
                if((ret = SoftsimEraseRawFlash(SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(sector_id),
                                               (UINT32)SOFTSIM_FLASH_DATA_ONE_SIZE)) != 0 )
                {
                    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_13, P_INFO, 1, "ecSoftSimFlashGetActivateSwap %d error!!!",ret);
                    free(sectorDataBuf);
                    free(saveBuf);
                    return (0);
                }

                if((ret = SoftsimRecyMainFlashData(sector_id,(UINT16)act_swap_header.w_size,sectorDataBuf)) != 0)
                {
                    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_14, P_INFO, 0,"very serious issue,ASSERT!!!");
                    free(sectorDataBuf);
                    free(saveBuf);
                    return (-1);
                }

            }

        }
    }
    else
    {
        #if 0
        /*maybe this time active swap has been destory yet, should to check whether the inactive swap data sector is
        equal to the maindata, if that should recovery it again*/
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_15, P_INFO, 1,"recovery from inact_swap 0x%x",inactive_swap_addr);
        ret = (EC_FLASH_ERR_E)SoftsimReadRawFlash(inactive_swap_addr,(UINT8 *)&inact_swap_header,sizeof(EC_FLASH_SWAP_HEADER_T));
        if(ret != 0)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_16, P_INFO, 1, "read 0x%x error!!!",inactive_swap_addr);
            free(sectorDataBuf);
            free(saveBuf);
            return (0);
        }
        if(inact_swap_header.swap_headchksum == _ecSoftSimFlashChksum(&inact_swap_header, 16))
        {
            if(inact_swap_header.w_sector_id == sector_id )
            {
                 memset(sectorDataBuf, 0x0, dynaBufLen);
                 ret = SoftsimReadRawFlash(inactive_swap_addr + sizeof(EC_FLASH_SWAP_HEADER_T),(UINT8 *)sectorDataBuf ,inact_swap_header.w_size);
                 if(ret != 0)
                 {
                     ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_17, P_INFO, 1,"sector:%d read fail!",sector_id);
                     free(sectorDataBuf);
                     free(saveBuf);
                     return (ret);
                 }
                /*here should check again for avoiding the mistake operations*/
                if((SoftsimCheckSwapDataIsvalid(sector_id,inact_swap_header.w_size,sectorDataBuf)) != 0 )
                {
                    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_18, P_INFO, 1,"swap sector: 0x%x is dirty! ",inactive_swap_addr);
                    free(sectorDataBuf);
                    free(saveBuf);
                    return (0);
                }
                if((ret = SoftsimEraseRawFlash(SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(sector_id),
                                               (UINT32)SOFTSIM_FLASH_DATA_ONE_SIZE)) != 0 )
                {
                    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_19, P_INFO, 1, "ecSoftSimFlashGetActivateSwap %d error!!!",ret);
                    free(sectorDataBuf);
                    free(saveBuf);
                    return (-1);
                }
                if((ret = SoftsimRecyMainFlashData(sector_id,(UINT16)inact_swap_header.w_size,sectorDataBuf)) != 0)
                {
                    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_20, P_INFO, 0,"very serious issue,ASSERT!!!");
                    free(sectorDataBuf);
                    free(saveBuf);
                    return (-1);
                }
            }
        }
        else
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashCheckRecord_21, P_INFO, 0,"very serious issue,ASSERT!!!");
            free(sectorDataBuf);
            free(saveBuf);
            return (-1);
        }
        #endif
    }

    if(sectorDataBuf)
    {
        free(sectorDataBuf);
    }
    if(saveBuf)
    {
        free(saveBuf);
    }

    return (0);
}

/**
  \fn          ecSoftSimFlashFormat
  \brief       erased the flash data
  \returns     0: succ; 1: failed
*/

static INT8 ecSoftSimFlashFormat(void)
{
    INT8   ret          = -1;
    EC_FLASH_HEADER_T   nv_header ;
    UINT8      index    = 0;
    UINT16     maxblock = 8; //SOFTSIM_FLASH_SIZE/SOFTSIM_FLASH_SECTOR_BASE_SIZE;
    EC_FLASH_SWAP_HEADER_T  swapheader = {0};

    for(index = 0 ; index < maxblock ;index++)
    {
        ret = SoftsimEraseRawFlash(SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(index),(UINT32)SOFTSIM_FLASH_SECTOR_BASE_SIZE);
        if(ret != 0)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashFormat_0, P_INFO, 1, "Erase sector %d error",index);
            return (ret);
        }
        osDelay(5); //delay 5 ms
    }

    // write header
    memset(&nv_header,0xFF,sizeof(EC_FLASH_HEADER_T));

    //init nv header prepare to write
    nv_header.work_flag    =   EC_FLASH_NV_FLAG;
    nv_header.nv_version   =   EC_FLASH_VERSION_FLAG;
    nv_header.used_size    =   0;
    nv_header.time_counter =   1;
    nv_header.chksum       =   _ecSoftSimFlashChksum(&nv_header,sizeof(EC_FLASH_HEADER_T) - 4);

    ret = SoftsimWriteToRawFlash(SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(0),(UINT8*)&nv_header,(UINT32)(sizeof(EC_FLASH_HEADER_T)));
    if(ret != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashFormat_1, P_INFO, 0, "Write Header error");
        return (ret);
    }

    memset(&swapheader,0x0,sizeof(EC_FLASH_SWAP_HEADER_T));
    swapheader.w_counter = 1;
    swapheader.swap_headchksum    = _ecSoftSimFlashChksum(&swapheader, 16);

    swapheader.statistic[EC_FLASH_BLOCK_ID_HEADER].write_count++;
    swapheader.statistic[EC_FLASH_BLOCK_ID_HEADER].erase_count++;

    swapheader.statistic[EC_FLASH_BLOCK_ID_SWAP1].write_count++;
    swapheader.statistic[EC_FLASH_BLOCK_ID_SWAP2].erase_count++;


    ret = SoftsimWriteToRawFlash(SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(EC_FLASH_BLOCK_ID_SWAP1),
                                (UINT8*)&swapheader,(UINT32)(sizeof(EC_FLASH_SWAP_HEADER_T)));
    if(ret != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashFormat_4, P_INFO, 1, "Write sector:%d Header error",EC_FLASH_BLOCK_ID_SWAP1);
        return (ret);
    }

    swapheader.w_counter++;
    swapheader.swap_headchksum    = _ecSoftSimFlashChksum(&swapheader, 16 );

    ret = SoftsimWriteToRawFlash(SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(EC_FLASH_BLOCK_ID_SWAP2),
                                (UINT8*)&swapheader,(UINT32)(sizeof(EC_FLASH_SWAP_HEADER_T)));
    if(ret != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashFormat_6, P_INFO, 1, "Write sector:%d Header error",EC_FLASH_BLOCK_ID_SWAP2);
        return (ret);
    }

    ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashFormat_7, P_WARNING, 0, "format succ");
    return 0;

}

/**
  \fn          ecSoftSimFlashInit
  \brief       erased the flash data and write special flash header sector
  \returns     0: succ; 1: failed
*/

INT8 ecSoftSimFlashInit(void)
{
    UINT8              ret = EC_FLASH_OK;
    EC_FLASH_HEADER_T  nv_header;

    memset(&nv_header,0xFF,sizeof(EC_FLASH_HEADER_T));

    /*here add the flash area safeguard*/
    if(SOFTSIM_FLASH_BASE_ADDRESS != 0x340000 ||
       SOFTSIM_FLASH_SIZE  > 0x8000)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashInit_00, P_WARNING, 1, "FLASH ADDR(0x%x) ERROR!!!",SOFTSIM_FLASH_BASE_ADDRESS);
        EC_ASSERT(0, 0,0,0);
    }

    /*end*/
    SoftsimReadRawFlash(SOFTSIM_FLASH_BASE_ADDRESS,(UINT8 *)&nv_header, (UINT16)sizeof(EC_FLASH_HEADER_T));

    if(nv_header.work_flag    != EC_FLASH_NV_FLAG ||
       nv_header.nv_version   != EC_FLASH_VERSION_FLAG)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashInit_0, P_WARNING, 0, "Need format...");
        ret = ecSoftSimFlashFormat();
        EC_ASSERT(ret == EC_FLASH_OK, 0,0,0);

    }
    else
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, ecSoftSimFlashInit_2, P_WARNING, 1, "Check recovery,Enable:%d",EC_SOFT_FLASH_SUP_RECOVERY);
        if(EC_SOFT_FLASH_SUP_RECOVERY)
        {
            ret = ecSoftSimFlashRecovery();
            EC_ASSERT(ret == EC_FLASH_OK, 0,0,0);
        }
    }
    return ret;
}

#endif
