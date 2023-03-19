/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: at_api.c
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "at_api.h"
#include "cms_util.h"
#include "at_entity.h"
#include "atc_decoder.h"
#include "atc_reply.h"
#include "ostask.h"

/******************************************************************************
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_VARIABLES__ //just for easy to find this position in SS

/******************************************************************************
 ******************************************************************************
 * INERNAL/STATIC FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position in SS




/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS


/**
  \fn          INT32 atRegisterAtChannel(AtChanRegInfo *pAtRegInfo)
  \brief       APP/task register a AT channel
  \param[in]   pAtRegInfo
  \returns     chanId, if >= 0, else CmsRetId
*/
INT32 atRegisterAtChannel(AtChanRegInfo *pAtRegInfo)
{
    INT32   atChanId = -1;
    INT32   retCode = CMS_RET_SUCC;

    retCode = cmsHighPriSynApiCall(atcRegisterAtChannelCallback, sizeof(AtChanRegInfo), pAtRegInfo, sizeof(INT32), &atChanId);

    if (retCode == CMS_RET_SUCC)
    {
        if (atChanId > CMS_CHAN_RSVD && atChanId < CMS_CHAN_NUM)
        {
            return atChanId;
        }
        else
        {
            return CMS_NO_RESOURCE;
        }
    }
    else if (retCode > 0)
    {
        return CMS_FAIL;
    }

    return retCode;
}


/**
  \fn          void atSendAtcmdStrSig(UINT8 *pCmdStr, UINT32 len)
  \brief       send the "SIG_AT_CMD_STR_REQ" to CMS_TASK
  \param[in]   pCmdStr      AT command string
  \param[in]   len          input length
  \returns
*/
void atSendAtcmdStrSig(UINT8 atChanId, const UINT8 *pCmdStr, UINT32 len)
{
    AtCmdStrReq *pAtCmdReq = PNULL;
    SignalBuf   *pSig = PNULL;
    CHAR    *pHeapStr = PNULL;
    osStatus_t  retStatus = osOK;

    OsaDebugBegin(pCmdStr != PNULL && len > 0 && len < 4096 && atChanId > 0 && atChanId < CMS_CHAN_NUM,
                  pCmdStr, len, atChanId);
    return;
    OsaDebugEnd();

    pHeapStr = (CHAR *)OsaAllocMemory(len + 1);  //+1 means '\0'
    OsaDebugBegin(pHeapStr != PNULL, pHeapStr, len, 0);
    return;
    OsaDebugEnd();

    memcpy(pHeapStr, pCmdStr, len);
    pHeapStr[len] = '\0';

    OsaCreateZeroSignal(SIG_AT_CMD_STR_REQ, sizeof(AtCmdStrReq), &pSig);
    pAtCmdReq = (AtCmdStrReq *)(pSig->sigBody);

    pAtCmdReq->atChanId = atChanId;
    pAtCmdReq->atStrLen = len;
    pAtCmdReq->pAtStr   = pHeapStr;

    //OsaSendSignal(CMS_TASK_ID, &pSig);
    retStatus = OsaSendSignalNoCheck(CMS_TASK_ID, &pSig);

    if (retStatus != osOK)
    {
        ECOMM_TRACE(UNILOG_ATCMD, atSendAtcmdStrSig_1, P_WARNING, 0,
                    "AT CMD Signal send to CMS task failed");

        /* free the memory */
        OsaFreeMemory(&(pHeapStr));

        /*destory signal*/
        OsaDestroySignal(&pSig);
    }

    return;
}

/**
  \fn           atUartPrintCallback(UINT16 paramSize, void *pParam)
  \brief        Callback function, called in: atUartPrint()
  \param[in]    paramSize
  \param[in]    pParam
  \returns
*/
void atUartPrintCallback(UINT16 paramSize, void *pParam)
{
    AtChanEntity    *pAtChanEty = atcGetEntityById(CMS_CHAN_UART);
    CHAR    *pStr = (CHAR *)(*(UINT32 *)pParam);

    OsaCheck(paramSize == 4 && pParam != PNULL && pStr != PNULL, paramSize, pParam, pStr);

    OsaDebugBegin(pAtChanEty != PNULL, CMS_CHAN_UART, 0, 0);
    OsaFreeMemory(&pStr);
    OsaDebugEnd();

    atcSendRawRespStr(pAtChanEty, pStr, strlen(pStr));

    OsaFreeMemory(&pStr);
    return;
}

/**
  \fn           CmsRetId atRilAtCmdReq(const CHAR *pAtCmdLine, UINT32 cmdLen, AtRespFunctionP respCallback, void *respData, UINT32 timeOutMs)
  \brief        AT RIL API, request the AT command service. It a blocked API, and will returned in force in "timeOutMs" milliseconds
  \param[in]    pAtCmdLine      AT command, should end with "\r\n"
  \param[in]    cmdLen          AT command string length
  \param[in]    respCallback    AT response callback function
  \param[in]    respData        passed as a parameter in callback function
  \param[in]    timeOutMs       AT guard time
  \returns      CmsRetId
*/
CmsRetId atRilAtCmdReq(const CHAR *pAtCmdLine, UINT32 cmdLen, AtRespFunctionP respCallback, void *respData, UINT32 timeOutMs)
{
    AtRilAtCmdReqData   atRilReqData;
    UINT16  atLineLen = cmdLen;     /* end with '\r' */

    /*
     * check parameters
    */
    OsaDebugBegin(pAtCmdLine != PNULL && pAtCmdLine[0] != '\0' && cmdLen >= 2 && cmdLen <= AT_CMD_STR_MAX_LEN && respCallback != PNULL,
                  pAtCmdLine, cmdLen, respCallback);
    return CMS_INVALID_PARAM;
    OsaDebugEnd();

    if (timeOutMs == 0)
    {
        OsaDebugBegin(FALSE, timeOutMs, pAtCmdLine[0], pAtCmdLine[1]);
        OsaDebugEnd();
        timeOutMs = CMS_MAX_DELAY_MS;
    }

    /*
     * AT CMD should end with "\r"
    */
    if (pAtCmdLine[cmdLen-1] != '\r' &&
        pAtCmdLine[cmdLen-2] != '\r')
    {
        OsaDebugBegin(FALSE, pAtCmdLine[cmdLen-1], pAtCmdLine[cmdLen-2], cmdLen);
        OsaDebugEnd();
        atLineLen += 2; //add "\r\n"
    }

    memset(&atRilReqData, 0x00, sizeof(AtRilAtCmdReqData));

    atRilReqData.pCmdLine   = (CHAR *)OsaAllocMemory(atLineLen + 1);  // end with '\0'

    OsaDebugBegin(atRilReqData.pCmdLine != PNULL, atRilReqData.pCmdLine, atLineLen, 0);
    return CMS_NO_MEM;
    OsaDebugEnd();

    memcpy(atRilReqData.pCmdLine,
           pAtCmdLine,
           cmdLen);

    if (atLineLen > cmdLen)
    {
        atRilReqData.pCmdLine[cmdLen] = '\r';
        atRilReqData.pCmdLine[cmdLen+1] = '\n';
    }

    atRilReqData.pCmdLine[atLineLen] = '\0';

    atRilReqData.cmdLen     = atLineLen;
    atRilReqData.respCallback = respCallback;
    atRilReqData.respData   = respData;

    /*
     * call ATC process func
    */
    return cmsBlockApiCall(atcRilAtCmdApiCallback,
                           sizeof(AtRilAtCmdReqData),
                           &(atRilReqData),
                           0,
                           PNULL,
                           timeOutMs);

}

/**
  \fn           CmsRetId atRilRegisterUrcCallback(AtUrcFunctionP urcCallback)
  \brief        AT RIL API, register RIL channel URC call back
  \param[in]    urcCallback     URC callback
  \returns      CmsRetId
*/
CmsRetId atRilRegisterUrcCallback(AtUrcFunctionP urcCallback)
{
    OsaDebugBegin(urcCallback != PNULL, urcCallback, 0, 0);
    OsaDebugEnd();

    return cmsSynApiCall(atcRilRegisterUrcSynCallback,
                         4,
                         &(urcCallback),
                         0,
                         PNULL);
}

/**
  \fn           CmsRetId atRilUnRegisterUrcCallback(void)
  \brief        AT RIL API, unregister RIL channel URC call back
  \param[in]    void
  \returns      CmsRetId
*/
CmsRetId atRilUnRegisterUrcCallback(void)
{
    return cmsSynApiCall(atcRilUnRegisterUrcSynCallback,
                         0,
                         PNULL,
                         0,
                         PNULL);
}


