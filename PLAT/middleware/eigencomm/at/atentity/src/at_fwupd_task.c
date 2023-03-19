/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename:
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/

#include "cmsis_os2.h"
#include "bsp.h"
#include "cms_api.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "flash_ec616_rt.h"
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "flash_ec616s_rt.h"
#include "slpman_ec616s.h"
#endif

#include "ec_fwupd_api.h"
#include "at_fwupd_task.h"


#define FWUPD_DFU_TIMER_PERIOD    (180*1000)  /* 3min, unit: ms */

#define FWUPD_FOTA_DELTA_BASE      FLASH_FOTA_REGION_START
#define FWUPD_FOTA_BACKUP_BASE    (FLASH_FOTA_REGION_END - FWUPD_FOTA_BACKUP_SIZE)

#define FWUPD_ABUP_BLOCK_SIZE     (32*1024)
#define FWUPD_FOTA_BACKUP_SIZE     FWUPD_ABUP_BLOCK_SIZE
#define FWUPD_FOTA_DELTA_SIZE     (FWUPD_FOTA_TOTAL_SIZE - FWUPD_FOTA_BACKUP_SIZE)
#define FWUPD_FOTA_TOTAL_SIZE     (FLASH_FOTA_REGION_END - FLASH_FOTA_REGION_START)

#define FWUPD_DFU_CTX_RESET(ctx,ds)     \
        do\
        {\
            ctx.fwTotalSize = 0;\
            ctx.lastPkgSn   = 0;\
            ctx.errCode     = FWUPD_EC_UNDEF_ERROR;\
            ctx.dfuState    = ds;\
            ctx.dfuResult   = FWUPD_DFU_RESULT_UNDEF;\
            ctx.pkgStatus   = FWUPD_PS_PKG_UNFOUND;\
        }while(0)

typedef enum
{
    FWUPD_DS_STATE_UNDEF = 0,
    FWUPD_DS_FLASH_ERASED,
    FWUPD_DS_FW_DOWNLOADING,
    FWUPD_DS_FW_DOWNLOADED,
    FWUPD_DS_UPGRADE_DONE
}FwupdDfuState_e;

typedef enum
{
    FWUPD_DFU_RESULT_UNDEF = 0,
    FWUPD_DFU_RESULT_SUCC = 1,
    FWUPD_DFU_RESULT_FAIL = 99
}FwupdDfuResult_e;

typedef enum
{
    FWUPD_PS_BEGIN = 0,
    FWUPD_PS_PKG_UNFOUND = FWUPD_PS_BEGIN,
    FWUPD_PS_PKG_PARTIAL,
    FWUPD_PS_PKG_UNVERIFIED,
    FWUPD_PS_PKG_VERIFIED,
    FWUPD_PS_PKG_UPGRADED,
    FWUPD_PS_PKG_INVALID,
    FWUPD_PS_PKG_UNDEF,
    FWUPD_PS_END = FWUPD_PS_PKG_UNDEF,

    FWUPD_PS_MAXNUM
}FwupdPkgStatus_e;

/* text description of 'FwupdStatusCode_e' */
static const int8_t *g_fwupdPkgStatus[] =
{
    "FWUPD_PS_PKG_UNFOUND",
    "FWUPD_PS_PKG_PARTIAL",
    "FWUPD_PS_PKG_UNVERIFIED",
    "FWUPD_PS_PKG_VERIFIED",
    "FWUPD_PS_PKG_UPGRADED",
    "FWUPD_PS_PKG_INVALID",
    "FWUPD_PS_PKG_UNDEF"
};

typedef struct
{
    uint8_t    timerId;
    uint8_t    rsvd[3];
}FwupdTimerCtx_t;

typedef struct
{
    uint8_t             taskStack[FWUPD_TASK_STACK_SIZE];
    StaticTask_t        taskTcb;
    osThreadId_t        taskHandle;
    osTimerId_t         dfuTimer;
    osMessageQueueId_t  msgQHandle;
    uint8_t             slpHandle;

    int8_t              fwName[FWUPD_FW_NAME_MAXLEN];
    int8_t              fwVer[FWUPD_FW_VER_MAXLEN];
    uint32_t            fwTotalSize;
    uint16_t            rsvd;
    uint16_t            lastPkgSn;
    uint8_t             errCode;
    int8_t              dfuState;
    uint8_t             dfuResult;
    uint8_t             pkgStatus;
}FwupdServCtxMan_t;

static void FWUPD_mainTask(void *args);
static void FWUPD_dfuTimerExp(FwupdTimerCtx_t *timerCtx);

static int32_t FWUPD_clearFlash(uint8_t *respInfo);
static int32_t FWUPD_downloadFw(uint16_t pkgSn, uint16_t strLen, uint8_t *hexStr, uint8_t crc8, uint8_t *respInfo);
static int32_t FWUPD_verifyFw(uint8_t *respInfo);
static int32_t FWUPD_queryFwName(uint8_t *respInfo);
static int32_t FWUPD_queryFwVer(uint8_t *respInfo);
static int32_t FWUPD_upgradeFw(uint8_t *respInfo);
static int32_t FWUPD_downloadOver(uint8_t *respInfo);
static int32_t FWUPD_queryDfuStatus(uint8_t *respInfo);

static int32_t FWUPD_eraseFotaFlash(void);
static uint32_t FWUPD_writeFotaFlashDelta(uint32_t offset, uint8_t* data_ptr, uint32_t len);
static uint32_t FWUPD_readFotaFlashBackup(uint32_t offset,uint8_t* data_ptr, uint32_t len);
static uint32_t FWUPD_readFotaFlashBackup(uint32_t offset,uint8_t* data_ptr, uint32_t len);
static uint32_t FWUPD_checkDfuResult(void);

static uint8_t FWUPD_calcXor8Chksum(uint8_t* buf, int32_t nbytes);
static int32_t FWUPD_hexStrToBytes(uint8_t* outBuf, int32_t nbytes, uint8_t* inHexStr, int32_t strLen);


static FwupdTimerCtx_t    g_fwupdTimerCtx;
static FwupdServCtxMan_t  g_fwupdServCtxMan;


/******************************************************************************
 * @brief : FWUPD_initTask
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
int32_t FWUPD_initTask(void)
{
    osThreadAttr_t task_attr;

    if(g_fwupdServCtxMan.taskHandle) /* task is running!  */
    {
        if(osTimerIsRunning(g_fwupdServCtxMan.dfuTimer))
        {
            /* reset timer period! */
            osTimerStart(g_fwupdServCtxMan.dfuTimer, FWUPD_DFU_TIMER_PERIOD);
        }
        return 0;
    }

    do
    {
        ECOMM_TRACE(UNILOG_FWUPD, INIT_TASK_1, P_SIG, 0, "create a FWUPD task...");

        if(g_fwupdServCtxMan.dfuTimer == NULL)
        {
            g_fwupdServCtxMan.dfuTimer = osTimerNew((osTimerFunc_t)FWUPD_dfuTimerExp, osTimerOnce, &g_fwupdTimerCtx, NULL);
            if(g_fwupdServCtxMan.dfuTimer == NULL)
            {
                break;
            }
        }

        if(g_fwupdServCtxMan.msgQHandle == NULL)
        {
            g_fwupdServCtxMan.msgQHandle = osMessageQueueNew(FWUPD_MQUE_MSG_MAXNUM, sizeof(FwupdReqMsg_t), NULL);
            if(g_fwupdServCtxMan.msgQHandle == NULL)
            {
                break;
            }
        }

        memset(&task_attr, 0, sizeof(osThreadAttr_t));
        task_attr.name       = "atFwupd";
        task_attr.stack_mem  = g_fwupdServCtxMan.taskStack;
        task_attr.stack_size = FWUPD_TASK_STACK_SIZE;
        task_attr.priority   = osPriorityNormal;

        task_attr.cb_mem  = &g_fwupdServCtxMan.taskTcb;  //task control block
        task_attr.cb_size = sizeof(StaticTask_t);        //size of task control block
        memset(g_fwupdServCtxMan.taskStack, 0xA5, FWUPD_TASK_STACK_SIZE);

        g_fwupdServCtxMan.taskHandle = osThreadNew(FWUPD_mainTask, NULL, &task_attr);
        if(g_fwupdServCtxMan.taskHandle == NULL)
        {
            break;
        }
        else
        {
            ECOMM_TRACE(UNILOG_FWUPD, INIT_TASK_2, P_SIG, 1, "And succ! tid(0x%x)", g_fwupdServCtxMan.taskHandle);
        }

        if(RET_TRUE != slpManApplyPlatVoteHandle("FWUPD", &g_fwupdServCtxMan.slpHandle))
        {
            ECOMM_TRACE(UNILOG_FWUPD, INIT_TASK_3, P_INFO, 0, "apply slp vote handle failed!");
            break;
        }
        slpManPlatVoteDisableSleep(g_fwupdServCtxMan.slpHandle, SLP_SLP1_STATE);

        osTimerStart(g_fwupdServCtxMan.dfuTimer, FWUPD_DFU_TIMER_PERIOD);

        /* set fota pkg name&version with running fw info ... */
        snprintf((CHAR*)g_fwupdServCtxMan.fwName, FWUPD_FW_NAME_MAXLEN, "EC61x-ABUP-FOTA-DFWPKG");
        snprintf((CHAR*)g_fwupdServCtxMan.fwVer, FWUPD_FW_VER_MAXLEN, "%s", SOFTVERSION);

        /* set flags per to DFU result... */
        g_fwupdServCtxMan.dfuResult = (uint8_t)FWUPD_checkDfuResult();
        switch(g_fwupdServCtxMan.dfuResult)
        {
            case FWUPD_DFU_RESULT_SUCC:
                g_fwupdServCtxMan.dfuState  = FWUPD_DS_UPGRADE_DONE;
                g_fwupdServCtxMan.pkgStatus = FWUPD_PS_PKG_UPGRADED;
                break;
            case FWUPD_DFU_RESULT_FAIL:
                g_fwupdServCtxMan.dfuState  = FWUPD_DS_UPGRADE_DONE;
                g_fwupdServCtxMan.pkgStatus = FWUPD_PS_PKG_INVALID;
                break;

            default:
                g_fwupdServCtxMan.dfuState  = FWUPD_DS_STATE_UNDEF;
                g_fwupdServCtxMan.pkgStatus = FWUPD_PS_PKG_UNDEF;
                break;
        }

        return 0;
    }while(0);

//FAILURE_CLEAR:
    ECOMM_TRACE(UNILOG_FWUPD, INIT_TASK_4, P_SIG, 0, "But failed, have to do some clearing...");
    FWUPD_deinitTask();

    return 1;
}

/******************************************************************************
 * @brief : FWUPD_deinitTask
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
int32_t FWUPD_deinitTask(void)
{
    osThreadId_t taskHandle = g_fwupdServCtxMan.taskHandle;
    slpManRet_t pmuRet;

    if(osTimerIsRunning(g_fwupdServCtxMan.dfuTimer))
    {
        OsaCheck(osOK == osTimerStop(g_fwupdServCtxMan.dfuTimer),0,0,0);
        OsaCheck(osOK == osTimerDelete(g_fwupdServCtxMan.dfuTimer),0,0,0);
    }
    else
    {
        OsaCheck(osOK == osTimerDelete(g_fwupdServCtxMan.dfuTimer),0,0,0);
    }

    if(g_fwupdServCtxMan.msgQHandle)
    {
        osMessageQueueDelete(g_fwupdServCtxMan.msgQHandle);
    }

    if(g_fwupdServCtxMan.slpHandle)
    {
        slpManPlatVoteEnableSleep(g_fwupdServCtxMan.slpHandle, SLP_SLP1_STATE);
        pmuRet = slpManGivebackPlatVoteHandle(g_fwupdServCtxMan.slpHandle);
        OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);
    }

    if(g_fwupdServCtxMan.taskHandle)
    {
        osThreadTerminate(g_fwupdServCtxMan.taskHandle);
    }

    /* BZERO all! */
    memset(&g_fwupdServCtxMan, 0, sizeof(FwupdServCtxMan_t));
    FWUPD_DFU_CTX_RESET(g_fwupdServCtxMan, FWUPD_DS_STATE_UNDEF);

    ECOMM_TRACE(UNILOG_FWUPD, DEINIT_TASK_1, P_SIG, 1, "FWUPD task(0x%x) terminated succ!", taskHandle);

    return 0;
}

/******************************************************************************
 * @brief : FWUPD_sendMsg
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
int32_t FWUPD_sendMsg(FwupdReqMsg_t *msg)
{
    osStatus_t status = osOK;

    status = osMessageQueuePut(g_fwupdServCtxMan.msgQHandle, (const void*)msg, 0, FWUPD_MQUE_SEND_TIMEOUT);
    if(osOK != status)
    {
        ECOMM_TRACE(UNILOG_FWUPD, SEND_MESSAGE_1, P_WARNING, 1, "fwupd send queue error(%d) ",status);
        return AT_PARA_ERR;
    }

    return AT_PARA_OK;
}

/******************************************************************************
 * @brief : FWUPD_mainTask
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
static void FWUPD_mainTask(void *args)
{
    int32_t           ret = 0;
    uint16_t       rcCode = APPL_RET_FAIL;
    FwupdReqMsg_t  reqMsg;
    FwupdCnfMsg_t  cnfMsg;
    osStatus_t status;

    while(1)
    {
        rcCode = APPL_RET_FAIL;
        FWUPD_REQMSG_INIT(reqMsg);
        FWUPD_CNFMSG_INIT(cnfMsg);

        /* recv msg (block mode) */
        status = osMessageQueueGet(g_fwupdServCtxMan.msgQHandle, &reqMsg, 0, osWaitForever);
        if(status == osOK)
        {
            switch(reqMsg.cmdCode)
            {
                case FWUPD_CMD_CLEAR_FLASH:
                    ret = FWUPD_clearFlash(NULL);
                    break;
                case FWUPD_CMD_DOWNLOAD_FW:
                    ret = FWUPD_downloadFw(reqMsg.pkgSn, reqMsg.strLen, reqMsg.hexStr, reqMsg.crc8, NULL);
                    break;
                case FWUPD_CMD_VERIFY_FW:
                    ret = FWUPD_verifyFw(cnfMsg.respStr);
                    break;
                case FWUPD_CMD_QUERY_FWNAME:
                    ret = FWUPD_queryFwName(cnfMsg.respStr);
                    break;
                case FWUPD_CMD_QUERY_FWVER:
                    ret = FWUPD_queryFwVer(cnfMsg.respStr);
                    break;
                case FWUPD_CMD_UPGRADE_FW:
                    ret = FWUPD_upgradeFw(NULL);
                    break;
                case FWUPD_CMD_DOWNLOAD_OVER:
                    ret = FWUPD_downloadOver(NULL);
                    break;
                case FWUPD_CMD_DFU_STATUS:
                    ret = FWUPD_queryDfuStatus(cnfMsg.respStr);
                    break;
    
                default:
                    break;
            }
    
            /*send confirm msg to at task*/
            cnfMsg.errCode = g_fwupdServCtxMan.errCode;
            if(ret == 0)
            {
                rcCode = APPL_RET_SUCC;
                cnfMsg.strLen  = strlen((CHAR*)cnfMsg.respStr);
            }
    
            #ifdef FEATURE_AT_ENABLE
            applSendCmsCnf(reqMsg.atHandle, rcCode, APPL_FWUPD, APPL_FWUPD_CNF, sizeof(cnfMsg), &cnfMsg);
            #endif
        }

    }
}

/******************************************************************************
 * @brief : FWUPD_dfuTimerExp
 * @author: Xu.Wang
 * @note  : timer ticking through the entire upgrading procedure
 ******************************************************************************/
static void FWUPD_dfuTimerExp(FwupdTimerCtx_t *timerCtx)
{
    ECOMM_TRACE(UNILOG_FWUPD, DFU_TIMER_EXPIRED_1, P_SIG, 1, "DFU timer(%d) expired!", timerCtx->timerId);

    FWUPD_deinitTask();
}

/******************************************************************************
 * @brief : FWUPD_clearFlash
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
static int32_t FWUPD_clearFlash(uint8_t *respInfo)
{
    int32_t         ret = 0;
    int8_t   *pkgStatus = NULL;

    if(0 != FWUPD_eraseFotaFlash())
    {
        ret = -1;
        g_fwupdServCtxMan.errCode = FWUPD_EC_FLERASE_ERROR;
    }
    else
    {
        /* reset state & flags of pkg processing! */
        FWUPD_DFU_CTX_RESET(g_fwupdServCtxMan, FWUPD_DS_FLASH_ERASED);
    }

    pkgStatus = (int8_t*)g_fwupdPkgStatus[g_fwupdServCtxMan.pkgStatus];

    if(respInfo)
    {
        snprintf((CHAR*)respInfo, FWUPD_RESP_BUF_MAXLEN, "+NFWUPD: %s\r\n", pkgStatus);
    }

    return ret;
}

/******************************************************************************
 * @brief : FWUPD_downloadFw
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
static int32_t FWUPD_downloadFw(uint16_t pkgSn, uint16_t strLen, uint8_t *hexStr, uint8_t crc8, uint8_t *respInfo)
{
    int32_t         ret = -1;
    uint8_t       csum8 = 0;
    int8_t   *pkgStatus = NULL;
    int32_t     dataLen = 0;
    uint8_t        data[FWUPD_DATA_BYTES_MAXNUM] = {0};

    ECOMM_TRACE(UNILOG_FWUPD, DOWNLOAD_FW_1, P_INFO, 3, "downloading FW: sn(%d), nbytes(%d), rcvd bytes(%d) before",
                                                                    pkgSn, strLen/2, g_fwupdServCtxMan.fwTotalSize);

    if((g_fwupdServCtxMan.dfuState == FWUPD_DS_STATE_UNDEF) || \
       (g_fwupdServCtxMan.dfuState == FWUPD_DS_UPGRADE_DONE))
    {
        g_fwupdServCtxMan.errCode = FWUPD_EC_FLERASE_UNDONE;
        ECOMM_TRACE(UNILOG_FWUPD, DOWNLOAD_FW_2, P_INFO, 0, "Do erase the flash first!");
        return -1;
    }

    g_fwupdServCtxMan.dfuState  = FWUPD_DS_FW_DOWNLOADING;

    do
    {
        if(!strLen || (FWUPD_HEXBYTE_NUM(strLen) % 4))
        {
            g_fwupdServCtxMan.errCode = FWUPD_EC_PKGSZ_ERROR;
            ECOMM_TRACE(UNILOG_FWUPD, DOWNLOAD_FW_3, P_INFO, 1, "Invalid pkg nbytes(%d)!", FWUPD_HEXBYTE_NUM(strLen));
            break;
        }

        /* pkgSn ?= lastPkgSn + 1 when pkg is not first one! */
        if(g_fwupdServCtxMan.fwTotalSize)
        {
            if(pkgSn != g_fwupdServCtxMan.lastPkgSn + 1)
            {
                /* including repeated pkg scenario as a result that flash cannot be rewrited! */
                g_fwupdServCtxMan.errCode = FWUPD_EC_PKGSN_ERROR;
                ECOMM_TRACE(UNILOG_FWUPD, DOWNLOAD_FW_4, P_INFO, 2, "PkgSn(%d) is not consecutive! expected is (%d)",
                                                                                 pkgSn, g_fwupdServCtxMan.lastPkgSn + 1);
                break;
            }
        }
        else /* pkgSn ?= 0 */
        {
            if(FWUPD_FW_PSN_MINNUM != pkgSn)
            {
                g_fwupdServCtxMan.errCode = FWUPD_EC_PKGSN_ERROR;
                ECOMM_TRACE(UNILOG_FWUPD, DOWNLOAD_FW_5, P_INFO, 1, "PkgSn(%d) is not started with zero!", pkgSn);
                break;
            }
            /* TODO: is there some extra work to be done with the first pkg segment? */
        }

        dataLen = FWUPD_hexStrToBytes(data, FWUPD_DATA_BYTES_MAXNUM, hexStr, strLen);
        csum8 = FWUPD_calcXor8Chksum(data, dataLen);
        if(csum8 != crc8)
        {
            g_fwupdServCtxMan.errCode = FWUPD_EC_CRC8_ERROR;
            ECOMM_TRACE(UNILOG_FWUPD, DOWNLOAD_FW_6, P_INFO, 2, "Pkg crc8 check error! real_cs8(%x), in_cs8(%x)", csum8, crc8);
            break;
        }

        if(g_fwupdServCtxMan.fwTotalSize + dataLen > FWUPD_FOTA_DELTA_SIZE)
        {
            g_fwupdServCtxMan.errCode = FWUPD_EC_PKGSZ_ERROR;
            ECOMM_TRACE(UNILOG_FWUPD, DOWNLOAD_FW_7, P_INFO, 1, "delta overflow! recvBytes(%d), pkgBytes(%d)!", \
                                                                       g_fwupdServCtxMan.fwTotalSize, dataLen);
            break;
        }

        ret = FWUPD_writeFotaFlashDelta(g_fwupdServCtxMan.fwTotalSize, data, dataLen);
        if(0 != ret)
        {
            g_fwupdServCtxMan.errCode = FWUPD_EC_FLWRITE_ERROR;
            ECOMM_TRACE(UNILOG_FWUPD, DOWNLOAD_FW_8, P_INFO, 0, "Pkg write flash error!");
            break;
        }

        /* refresh the pkg Sn & totalSize! */
        g_fwupdServCtxMan.fwTotalSize += dataLen;
        g_fwupdServCtxMan.lastPkgSn    = pkgSn;
        g_fwupdServCtxMan.pkgStatus    = FWUPD_PS_PKG_PARTIAL;
    }while(0);

    pkgStatus = (int8_t*)g_fwupdPkgStatus[g_fwupdServCtxMan.pkgStatus];

    if(respInfo)
    {
        snprintf((CHAR*)respInfo, FWUPD_RESP_BUF_MAXLEN, "+NFWUPD: %s\r\n", pkgStatus);
    }

    return ret;
}

/******************************************************************************
 * @brief : FWUPD_verifyFw
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
static int32_t FWUPD_verifyFw(uint8_t *respInfo)
{
    int8_t  *pkgStatus = NULL;

    do
    {
        if((g_fwupdServCtxMan.dfuState == FWUPD_DS_FLASH_ERASED) || \
           (g_fwupdServCtxMan.dfuState == FWUPD_DS_UPGRADE_DONE))
        {
            break;
        }

        if((g_fwupdServCtxMan.dfuState == FWUPD_DS_STATE_UNDEF) && \
           (g_fwupdServCtxMan.pkgStatus != FWUPD_PS_PKG_UNDEF))
        {
            break;
        }

        /* TODO: do verification job, but no means to do it yet! */
        if(0)
        {
            g_fwupdServCtxMan.pkgStatus = FWUPD_PS_PKG_INVALID;
        }
        else
        {
            g_fwupdServCtxMan.pkgStatus = FWUPD_PS_PKG_VERIFIED;
        }

    }while(0);

    pkgStatus = (int8_t*)g_fwupdPkgStatus[g_fwupdServCtxMan.pkgStatus];

    if(respInfo)
    {
        snprintf((CHAR*)respInfo, FWUPD_RESP_BUF_MAXLEN, "+NFWUPD: %s\r\n", pkgStatus);
    }

    return 0;
}

/******************************************************************************
 * @brief : FWUPD_queryFwName
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
static int32_t FWUPD_queryFwName(uint8_t *respInfo)
{
    OsaDebugBegin(respInfo != NULL, 0, 0, 0);
    return -1;
    OsaDebugEnd();

    FWUPD_verifyFw(NULL);

    if((g_fwupdServCtxMan.pkgStatus == FWUPD_PS_PKG_VERIFIED) || \
       (g_fwupdServCtxMan.pkgStatus == FWUPD_PS_PKG_UPGRADED))
    {
        snprintf((CHAR*)respInfo, FWUPD_RESP_BUF_MAXLEN, "+NFWUPD: %s\r\n", \
                                                 g_fwupdServCtxMan.fwName);
    }
    else
    {
        snprintf((CHAR*)respInfo, FWUPD_RESP_BUF_MAXLEN, "+NFWUPD: %s\r\n", \
                            g_fwupdPkgStatus[g_fwupdServCtxMan.pkgStatus]);
    }

    return 0;
}

/******************************************************************************
 * @brief : FWUPD_queryFwVer
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
static int32_t FWUPD_queryFwVer(uint8_t *respInfo)
{
    OsaDebugBegin(respInfo != NULL, 0, 0, 0);
    return -1;
    OsaDebugEnd();

    FWUPD_verifyFw(NULL);

    if((g_fwupdServCtxMan.pkgStatus == FWUPD_PS_PKG_VERIFIED) || \
       (g_fwupdServCtxMan.pkgStatus == FWUPD_PS_PKG_UPGRADED))
    {
        snprintf((CHAR*)respInfo, FWUPD_RESP_BUF_MAXLEN, "+NFWUPD: %s\r\n", \
                                                  g_fwupdServCtxMan.fwVer);
    }
    else
    {
        snprintf((CHAR*)respInfo, FWUPD_RESP_BUF_MAXLEN, "+NFWUPD: %s\r\n", \
                            g_fwupdPkgStatus[g_fwupdServCtxMan.pkgStatus]);
    }

    return 0;
}

void FWUPD_rebootSystem(void)
{
    ECOMM_TRACE(UNILOG_FWUPD, UPGRADE_FW_1, P_SIG, 0, "fota_system_reboot-->");

    __disable_irq();
    
    #if defined CHIP_EC616S
    extern void PmuAonPorEn(void);
    PmuAonPorEn();
    #endif
    
    NVIC_SystemReset();
}

/******************************************************************************
 * @brief : FWUPD_upgradeFw
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
static int32_t FWUPD_upgradeFw(uint8_t *respInfo)
{
    if(g_fwupdServCtxMan.dfuState == FWUPD_DS_FW_DOWNLOADING)
    {
        FWUPD_downloadOver(NULL);
    }

    if(osTimerIsRunning(g_fwupdServCtxMan.dfuTimer))
    {
        OsaCheck(osOK == osTimerStop(g_fwupdServCtxMan.dfuTimer),0,0,0);
    }

    FWUPD_verifyFw(NULL);

    FWUPD_rebootSystem();

    return 0;
}

/******************************************************************************
 * @brief : FWUPD_downloadOver
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
static int32_t FWUPD_downloadOver(uint8_t *respInfo)
{
    g_fwupdServCtxMan.dfuState  = FWUPD_DS_FW_DOWNLOADED;
    g_fwupdServCtxMan.pkgStatus = FWUPD_PS_PKG_UNVERIFIED;

    ECOMM_TRACE(UNILOG_FWUPD, DOWNLOAD_FW_10, P_INFO, 1, "All pkg downloaded! fw totalSize(%d)", g_fwupdServCtxMan.fwTotalSize);

    /* TODO: clear flags in flash if any? */

    return 0;
}

/******************************************************************************
 * @brief : FWUPD_queryDfuStatus
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
static int32_t FWUPD_queryDfuStatus(uint8_t *respInfo)
{
    /* +NFWUPD:<totalSize>,<lastPkgSn>,<dfuResult>,<dfuState>,<pkgStatus>,<errCode> */
    snprintf((CHAR*)respInfo, FWUPD_RESP_BUF_MAXLEN, "+NFWUPD: %d, %d, %d, %d, %d, %d\r\n",
                                                             g_fwupdServCtxMan.fwTotalSize,
                                                             g_fwupdServCtxMan.lastPkgSn,
                                                             g_fwupdServCtxMan.dfuResult,
                                                             g_fwupdServCtxMan.dfuState,
                                                             g_fwupdServCtxMan.pkgStatus,
                                                             g_fwupdServCtxMan.errCode);

    return 0;
}

static int8_t FWUPD_eraseFotaFlashDelta(void)
{
    uint8_t  retValue;
    uint32_t address = FWUPD_FOTA_DELTA_BASE;
    uint32_t len = FWUPD_FOTA_DELTA_SIZE;
    uint32_t real_erase_size = len;
    uint32_t real_address = address;

    while(real_erase_size > 0)
    {
        retValue = BSP_QSPI_Erase_Safe(real_address,FWUPD_ABUP_BLOCK_SIZE);
        if (retValue != QSPI_OK)
        {
            ECOMM_TRACE(UNILOG_FWUPD, ERASE_FLASH_1, P_INFO, 2, "fota_erase_delta error:real_address:%x,ret:%d",real_address,retValue);
            break;
        }

        real_address += FWUPD_ABUP_BLOCK_SIZE;
        real_erase_size -= FWUPD_ABUP_BLOCK_SIZE;

        ECOMM_TRACE(UNILOG_FWUPD, ERASE_FLASH_2, P_INFO, 2, "fota_erase_delata-->real_address:%x,ret:%d",real_address,retValue);
        osDelay(300 / portTICK_RATE_MS);
    }

    return retValue;
}

static uint32_t FWUPD_eraseFotaFlashBackup(void)
{
    uint8_t retValue;
    uint32_t address = FWUPD_FOTA_BACKUP_BASE;

    retValue = BSP_QSPI_Erase_Safe(address, FWUPD_ABUP_BLOCK_SIZE);
    ECOMM_TRACE(UNILOG_FWUPD, ERASE_FLASH_3, P_INFO, 2, "fota_erase_backup-->address:%x,ret:%d",address,retValue);

    return retValue;
}

static int32_t FWUPD_eraseFotaFlash(void)
{
    uint16_t ret1,ret2;

    ret1 = FWUPD_eraseFotaFlashDelta();
    ret2 = FWUPD_eraseFotaFlashBackup();
    if(ret1 == 0 && ret2 == 0)
    {
        ECOMM_TRACE(UNILOG_FWUPD, ERASE_FLASH_10, P_INFO, 0, "FWUPD_eraseFotaFlash-->succ");
        return 0;
    }
    else
    {
        g_fwupdServCtxMan.errCode = FWUPD_EC_FLWRITE_ERROR;
        ECOMM_TRACE(UNILOG_FWUPD, ERASE_FLASH_11, P_INFO, 0, "FWUPD_eraseFotaFlash-->fail");
        return -1;
    }
}

static uint32_t FWUPD_writeFotaFlashDelta(uint32_t offset, uint8_t* data_ptr, uint32_t len)
{
    uint8_t   retValue;
    uint32_t   address = FWUPD_FOTA_DELTA_BASE + offset;

    if (len == 0) return 0;

    retValue = BSP_QSPI_Write_Safe((uint8_t *)data_ptr, address, len);
    ECOMM_TRACE(UNILOG_FWUPD, WRITE_FLASH_1, P_INFO, 3, "fota_write_delta-->address:%x,len:%x,ret:%d",address,len,retValue);

    return (retValue == QSPI_OK) ? 0: -1;
}
#if 0 /* unused yet! */
static uint32_t FWUPD_readFotaFlashDelta(uint32_t offset,uint8_t* data_ptr, uint32_t len)
{
    uint8_t retValue;
    uint32_t address = FWUPD_FOTA_DELTA_BASE + offset;

    retValue = BSP_QSPI_Read_Safe((uint8_t *)data_ptr, address, len);
    ECOMM_TRACE(UNILOG_FWUPD, READ_FLASH_1, P_INFO, 3, "fota_read_delta-->address:%x,len:%x,ret:%d",address,len,retValue);

    return (retValue == QSPI_OK) ? 0: -1;
}
#endif

static uint32_t FWUPD_readFotaFlashBackup(uint32_t offset,uint8_t* data_ptr, uint32_t len)
{
    uint8_t retValue;
    uint32_t address = FWUPD_FOTA_BACKUP_BASE + offset;

    retValue = BSP_QSPI_Read_Safe((uint8_t *)data_ptr, address, len);
    ECOMM_TRACE(UNILOG_FWUPD, READ_FLASH_2, P_INFO, 3, "fota_read_backup-->address:%x,len:%x,ret:%d",address,len,retValue);

    return (retValue == QSPI_OK) ? 0: -1;
}

static uint32_t FWUPD_checkDfuResult(void)
{
    uint32_t     dfuResult = FWUPD_DFU_RESULT_UNDEF;
    uint8_t backup_data[4] = {0};

    if(0 != FWUPD_readFotaFlashBackup(0, backup_data, 4))
    {
        g_fwupdServCtxMan.errCode = FWUPD_EC_FLREAD_ERROR;
        return FWUPD_DFU_RESULT_UNDEF;
    }

    if (strncmp((char *)backup_data, "OK", 2) == 0)
    {
        dfuResult = FWUPD_DFU_RESULT_SUCC;
        ECOMM_TRACE(UNILOG_FWUPD, CHECK_DFU_RESULT_1, P_INFO, 0, "DFU success");
    }
    else if (strncmp((char *)backup_data, "NO", 2) == 0)
    {
        dfuResult = FWUPD_DFU_RESULT_FAIL;
        ECOMM_TRACE(UNILOG_FWUPD, CHECK_DFU_RESULT_2, P_INFO, 0, "DFU failed");
    }
    else
    {
        ECOMM_TRACE(UNILOG_FWUPD, CHECK_DFU_RESULT_3, P_INFO, 0, "no DFU result");
    }

    return dfuResult;
}

static uint8_t FWUPD_calcXor8Chksum(uint8_t* buf, int32_t nbytes)
{
    uint8_t    csum8 = 0;
    int32_t     loop = 0;

    for(loop = 0; loop < nbytes; loop++)
    {
        csum8 ^= buf[loop];
    }

    return csum8;
}

static int32_t FWUPD_hexStrToBytes(uint8_t* outBuf, int32_t nbytes, uint8_t* inHexStr, int32_t strLen)
{
    int32_t loop;

    for (loop = 0; loop < nbytes && (loop << 1) < strLen; loop ++)
    {
        if(strLen == ((loop << 1) + 1))
        {
            outBuf[loop] = (int8_t)(FWUPD_HEXCHAR_TO_INTEGER(inHexStr[loop << 1]) << 4);
        }
        else
        {
            outBuf[loop] = (int8_t)((FWUPD_HEXCHAR_TO_INTEGER(inHexStr[loop << 1]) << 4) | \
                                     FWUPD_HEXCHAR_TO_INTEGER(inHexStr[(loop << 1) + 1]));
        }
    }

    return loop;
}


