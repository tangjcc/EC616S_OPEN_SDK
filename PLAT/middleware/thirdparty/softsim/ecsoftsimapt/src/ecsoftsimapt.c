/******************************************************************************
Copyright:      - 2017, All rights reserved by Eigencomm Ltd.
File name:      - ecsoftsimapt.c
Description:    - softsim adapter API.
Function List:  -
History:        - 09/15/2020, Originated by xlhu
******************************************************************************/


/******************************************************************************
 * Include Files
*******************************************************************************/
#include "uiccdrvapi.h"
#include "debug_trace.h"
#include "debug_log.h"
#include "mem_map.h"

#if defined CHIP_EC616
#include "flash_ec616_rt.h"
#elif defined CHIP_EC616S
#include "flash_ec616s_rt.h"
#endif


#ifdef SOFTSIM_FEATURE_ENABLE
#include "esim_cos.h"
#endif


/*********************************************************************************
 * Macros
*********************************************************************************/
#define SOFTSIM_FLASH_SECTOR_SIZE 0x1000
#define SOFTSIM_MAX_SECTOR  ((SOFTSIM_FLASH_PHYSICAL_BASEADDR + SOFTSIM_FLASH_MAX_SIZE)/SOFTSIM_FLASH_SECTOR_SIZE)
/******************************************************************************
 * Extern global variables
*******************************************************************************/

/******************************************************************************
 * Extern functions
*******************************************************************************/

/******************************************************************************
 * Global variables
*******************************************************************************/

/******************************************************************************
 * Types
*******************************************************************************/

/******************************************************************************
 * Local variables
*******************************************************************************/


/******************************************************************************
 * Function Prototypes
*******************************************************************************/

/******************************************************************************
 * Function definition
*******************************************************************************/

/******************************************************************************
 * SoftSimReset
 * Description: softsim reset callback function
 * param[in]   null
 * param[out]   unsigned short *atrLen, the pointer to the length of ATR
 * param[out]   unsigned char *atrData, the pointer to the ATR data.
 * Comment:
******************************************************************************/
void SoftSimReset(UINT16 *atrLen, UINT8 *atrData)
{
#ifdef SOFTSIM_CT_ENABLE
    UINT16 res = 0;

    res = esim_reset_entry(atrData, atrLen);
    if (res != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, SoftSimReset_1, P_WARNING, 1, "esim_reset_entry return %d", res);
    }
#else
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, PNULL);
    osStatus_t osState = osOK;

    /*
    * create signal/msg
    * send signal/msg to softsim task
    * release sem after process done on softsim task
    */
    //softsim vender add:

    /*
     * wait for sem
    */
    if ((osState = osSemaphoreAcquire(sem, portMAX_DELAY)) != osOK)
    {
        OsaDebugBegin(FALSE, osState, 0, 0);
        OsaDebugEnd();
    }

    /*
     * Semaphore delete
    */
    osSemaphoreDelete(sem);
#endif

}

/******************************************************************************
 * SoftSimApduReq
 * Description: soft sim APDU request/response callback function
 * param[in]   unsigned short txDataLen, the length of tx data
 * param[in]   unsigned char *txData, the pointer to the tx data.
 * param[out]   unsigned short *rxDataLen, the pointer to the length of rx data
 * param[out]   unsigned char *rxData, the pointer to the rx data.
 * Comment:
******************************************************************************/
void SoftSimApduReq(UINT16 txDataLen, UINT8 *txData, UINT16 *rxDataLen, UINT8 *rxData)
{
#ifdef SOFTSIM_CT_ENABLE
    UINT16 res = 0;

    ECOMM_DUMP(UNILOG_SOFTSIM, SoftSimApduReq_0, P_VALUE,
                                "APDU command: ", txDataLen, txData);

    res = apdu_process_entry(txData, txDataLen, rxData, rxDataLen);
    osDelay(1);
    if (res != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, SoftSimApduReq_1, P_WARNING, 1, "apdu_process_entry return %d", res);
    }
    else
    {
        ECOMM_DUMP(UNILOG_SOFTSIM, SoftSimApduReq_2, P_VALUE,
                                    "APDU response: ", *rxDataLen, rxData);
    }

#else
    osSemaphoreId_t sem = osSemaphoreNew(1U, 0, PNULL);
    osStatus_t osState = osOK;

    /*
    * create signal/msg
    * send signal/msg to softsim task
    * release sem after process done on softsim task
    */
    //softsim vender add:


    /*
     * wait for sem
    */
    if ((osState = osSemaphoreAcquire(sem, portMAX_DELAY)) != osOK)
    {
        OsaDebugBegin(FALSE, osState, 0, 0);
        OsaDebugEnd();
    }

    /*
     * Semaphore delete
    */
    osSemaphoreDelete(sem);
#endif

}


#ifdef SOFTSIM_FEATURE_ENABLE


/******************************************************************************
 * SoftSimInit
 * Description: this api called by modem/SIM task to init configuration for softsim and start softsim task
 * input: void
 * output: void
 * Comment:
******************************************************************************/
void SoftSimInit(void)
{

    /*
    * register callback
    */
    EcSoftSimRegisterCallbackFunc(SoftSimReset, SoftSimApduReq);

    /*
    * start softsim task
    */
    ECOMM_TRACE(UNILOG_SOFTSIM, SoftSimInit_1, P_INFO, 0, "Start softsim task");
    //softsim vender add:

}

UINT32 SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(UINT8 index)
{
    return (SOFTSIM_FLASH_PHYSICAL_BASEADDR + index * SOFTSIM_FLASH_SECTOR_SIZE);
}


/******************************************************************************
 * SOFTSIM_RAWFLASH_SECTOR_1_ADDR
 * Description: softsim raw flash sector 1 (4KB) base address
 * input: void
 * output: void
 * Comment:
******************************************************************************/
UINT32 SOFTSIM_RAWFLASH_SECTOR_1_ADDR(void)
{
    return SOFTSIM_FLASH_PHYSICAL_BASEADDR;
}

/******************************************************************************
 * SOFTSIM_RAWFLASH_SECTOR_2_ADDR
 * Description: softsim raw flash sector 2 (4KB) base address
 * input: void
 * output: void
 * Comment:
******************************************************************************/
UINT32 SOFTSIM_RAWFLASH_SECTOR_2_ADDR(void)
{
    return (SOFTSIM_FLASH_PHYSICAL_BASEADDR + SOFTSIM_FLASH_SECTOR_SIZE);
}

/******************************************************************************
 * SOFTSIM_RAWFLASH_SECTOR_3_ADDR
 * Description: softsim raw flash sector 3 (4KB) base address
 * input: void
 * output: void
 * Comment:
******************************************************************************/
UINT32 SOFTSIM_RAWFLASH_SECTOR_3_ADDR(void)
{
    return (SOFTSIM_FLASH_PHYSICAL_BASEADDR + SOFTSIM_FLASH_SECTOR_SIZE * 2);
}

/******************************************************************************
 * SOFTSIM_RAWFLASH_SECTOR_4_ADDR
 * Description: softsim raw flash sector 4 (4KB) base address
 * input: void
 * output: void
 * Comment:
******************************************************************************/
UINT32 SOFTSIM_RAWFLASH_SECTOR_4_ADDR(void)
{
    return (SOFTSIM_FLASH_PHYSICAL_BASEADDR + SOFTSIM_FLASH_SECTOR_SIZE * 3);
}

UINT32 SOFTSIM_RAWFLASH_SECTOR_5_ADDR(void)
{
    return (SOFTSIM_FLASH_PHYSICAL_BASEADDR + SOFTSIM_FLASH_SECTOR_SIZE * 4);
}


UINT32 SOFTSIM_RAWFLASH_SECTOR_6_ADDR(void)
{
    return (SOFTSIM_FLASH_PHYSICAL_BASEADDR + SOFTSIM_FLASH_SECTOR_SIZE * 5);
}

UINT32 SOFTSIM_RAWFLASH_SECTOR_7_ADDR(void)
{
    return (SOFTSIM_FLASH_PHYSICAL_BASEADDR + SOFTSIM_FLASH_SECTOR_SIZE * 6);
}

UINT32 SOFTSIM_RAWFLASH_SECTOR_8_ADDR(void)
{
    return (SOFTSIM_FLASH_PHYSICAL_BASEADDR + SOFTSIM_FLASH_SECTOR_SIZE * 7);
}



/******************************************************************************
 * SoftsimReadRawFlash
 * Description: Erase to raw flash
 * param[in]   UINT32 addr, read flash address
 * param[out]   UINT8 *pBuffer, the pointer to the data buffer
 * param[in]   UINT32 length, the length of the data read
 * Comment:
******************************************************************************/
INT8 SoftsimReadRawFlash(UINT32 addr, UINT8 *pBuffer, UINT32 length)
{
    if((pBuffer == NULL) ||
       (length == 0) ||
       (addr < SOFTSIM_RAWFLASH_SECTOR_1_ADDR()) ||
       (addr >= (SOFTSIM_RAWFLASH_SECTOR_1_ADDR() + SOFTSIM_FLASH_MAX_SIZE)))
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, SoftsimReadRawFlash_1, P_WARNING, 3,
            "Save raw flash params error, pBuffer 0x%x, flash Addr 0x%x, bufferSize %d",
            pBuffer, addr, length);
        return -1;
    }

    if (BSP_QSPI_Read_Safe(pBuffer, addr, length) != QSPI_OK)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, SoftsimReadRawFlash_2, P_WARNING, 0, "read flash failure");
        return -1;
    }

    return 0;
}


/******************************************************************************
 * SoftsimEraseRawFlash
 * Description: Erase to raw flash
 * param[in]   UINT32 sectorAddr, the flash sector address
 * param[in]   UINT32 size, the size of erase flash
 * Comment:
******************************************************************************/
INT8 SoftsimEraseRawFlash(UINT32 sectorAddr, UINT32 size)
{
    UINT8   ret = 0;

    ECOMM_TRACE(UNILOG_SOFTSIM, SoftsimEraseRawFlash_0, P_INFO, 2, "SoftsimEraseRawFlash addr:0x%x,size:%d",  sectorAddr,size);

    if ((sectorAddr != SOFTSIM_RAWFLASH_SECTOR_1_ADDR()) &&
        (sectorAddr != SOFTSIM_RAWFLASH_SECTOR_2_ADDR()) &&
        (sectorAddr != SOFTSIM_RAWFLASH_SECTOR_3_ADDR()) &&
        (sectorAddr != SOFTSIM_RAWFLASH_SECTOR_4_ADDR()) &&
        (sectorAddr != SOFTSIM_RAWFLASH_SECTOR_5_ADDR()) &&
        (sectorAddr != SOFTSIM_RAWFLASH_SECTOR_6_ADDR()) &&
        (sectorAddr != SOFTSIM_RAWFLASH_SECTOR_7_ADDR()) &&
        (sectorAddr != SOFTSIM_RAWFLASH_SECTOR_8_ADDR()))
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, SoftsimEraseRawFlash_1, P_WARNING, 1, "Sector address 0x%x error", sectorAddr);
        return -1;
    }

    if ((size == 0 || size > SOFTSIM_FLASH_MAX_SIZE) ||
        ((size % SOFTSIM_FLASH_SECTOR_SIZE) != 0))
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, SoftsimEraseRawFlash_2, P_WARNING, 1, "Erase size %d error", size);
        return -1;
    }

    ret = BSP_QSPI_Erase_Safe(sectorAddr, size);
    if(ret != QSPI_OK)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, SoftsimEraseRawFlash_3, P_WARNING, 0, "Erase flash error");
        return -1;
    }

    return 0;
}


/******************************************************************************
 * SoftsimWriteToRawFlash
 * Description: write data to raw flash
 * param[in]   UINT32 addr, the flash address to be wrote
 * param[in]   UINT8 *pBuffer, the pointer to the buffer for write data
 * param[in]   UINT8 length, the data length to be wrote
 * Comment:
******************************************************************************/
INT8 SoftsimWriteToRawFlash(UINT32 addr, UINT8 *pBuffer, UINT32 length)
{
    UINT8   ret = 0;

    if((pBuffer == NULL) ||
       (length == 0)      ||
       (addr < SOFTSIM_RAWFLASH_SECTOR_1_ADDR()) ||
       (addr >= (SOFTSIM_RAWFLASH_SECTOR_1_ADDR() + SOFTSIM_FLASH_MAX_SIZE)))
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, SoftsimWriteToRawFlash_1, P_WARNING, 3,
            "Save raw flash params error, pBuffer 0x%x, flash Addr 0x%x, length %d",
            pBuffer, addr, length);
        return -1;
    }

    ret = BSP_QSPI_Write_Safe(pBuffer, addr, length) ;
    if(ret != QSPI_OK)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, SoftsimWriteToRawFlash_2, P_WARNING, 0, "Save raw flash error");
        return -1;
    }

    return 0;
}

#endif

