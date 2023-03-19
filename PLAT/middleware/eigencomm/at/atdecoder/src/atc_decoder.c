/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atc_parser.c
*
*  Description:
 *
 * DESCRIPTION
 *
 *  History:
 *
 *  Notes:
 *
 ******************************************************************************/
#include <ctype.h>
#include "at_util.h"
#include "atc_decoder.h"
#include "debug_log.h"
#include "ostask.h"
#include "bsp.h"
#include "atec_cust_cmd_table.h"
#include "atec_cmd_table.h"
#include "atec_sms.h"
#include "atec_mqtt.h"
#include "atec_coap.h"
#include "atec_tcpip.h"

#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "ecpm_ec616_external.h"
#elif defined CHIP_EC616S
#include "ecpm_ec616s_external.h"
#endif

#ifdef  FEATURE_REF_AT_ENABLE
#include "atec_ref_cmd_table.h"
#endif


/******************************************************************************
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_VARIABLES__ //just for easy to find this position in SS

/******************************************************************************
 * gAtChanEtyList
 *  AT channel entity list
 *  1> CHAN 0 is reserved for IND
 *  2> CHAN 1 is default channel
******************************************************************************/
static AtChanEntityP    gAtChanEtyList[CMS_CHAN_NUM];

/*
 * ATC_CHAN_REAL_ENTITY_NUM = CMS_CHAN_NUM - 1
 *
 * gAtChanEtyList[1] = &gAtChanDefaultEty[0]
 * ...
 * gAtChanEtyList[N] = &gAtChanDefaultEty[N-1]
*/
static AtChanEntity     gAtChanEty[ATC_CHAN_REAL_ENTITY_NUM];


extern ARM_DRIVER_USART *UsartAtCmdHandle;

/******************************************************************************
 ******************************************************************************
 * STATIC FUNCTION
 ******************************************************************************
******************************************************************************/

#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position----Below for internal use

#if 0   //All AT channel need to be registered firstly
/**
  \fn          atcDefaultChanRespFun(const CHAR *pStr, UINT32 strLen, void *pArg)
  \brief
  \param[in]
  \returns
*/
static void atcDefaultChanRespFun(const CHAR *pStr, UINT32 strLen, void *pArg)
{
    INT32   status = 0;

    OsaDebugBegin(UsartAtCmdHandle != PNULL && pStr != PNULL && strLen > 0,
                  UsartAtCmdHandle, pStr, strLen);
    return;
    OsaDebugEnd();

    //int32_t USART_Send(const void *data, uint32_t num, USART_RESOURCES *usart)
    status = UsartAtCmdHandle->Send(pStr, strLen);

    //#define ARM_DRIVER_OK                 0
    OsaDebugBegin(status == 0, status, 0, 0);
    OsaDebugEnd();

    return;
}


/**
  \fn          void atcDefaultChanUrcFun(const CHAR *pStr, UINT32 strLen)
  \brief
  \param[in]
  \returns
*/
static void atcDefaultChanUrcFun(const CHAR *pStr, UINT32 strLen)
{
    INT32   status = 0;

    OsaDebugBegin(UsartAtCmdHandle != PNULL && pStr != PNULL && strLen > 0,
                  UsartAtCmdHandle, pStr, strLen);
    return;
    OsaDebugEnd();

    //int32_t USART_Send(const void *data, uint32_t num, USART_RESOURCES *usart)
    status = UsartAtCmdHandle->Send(pStr, strLen);

    //#define ARM_DRIVER_OK                 0
    OsaDebugBegin(status == 0, status, 0, 0);
    OsaDebugEnd();

    return;
}
#endif

/**
  \fn           CmsRetId atcCheckPreDefinedTable(AtCmdPreDefInfoC *pCmdTbl, UINT32 tblSize)
  \brief        Check whether all pre-defined AT command are all valid
  \param[in]    pCmdTbl     pre-defined AT command table
  \param[in]    tblSize     table size
  \returns      CmsRetId
*/
static CmsRetId atcCheckPreDefinedTable(AtCmdPreDefInfoC *pCmdTbl, UINT32 tblSize)
{
    CmsRetId    ret = CMS_RET_SUCC;
    AtCmdPreDefInfoC    *pPreCmd = pCmdTbl;
    UINT32      tmpIdx = 0, charIdx = 0;
    UINT32      strLen = 0;
    CHAR        tmpChr = 0;

    OsaCheck(pCmdTbl != PNULL && tblSize > 0, pCmdTbl, tblSize, 0);

    for (tmpIdx = 0; tmpIdx < tblSize; tmpIdx++)
    {
        pPreCmd = pCmdTbl + tmpIdx;

        /*
         * check AT name
        */
        if (pPreCmd->pName == PNULL)
        {
            ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcCheckPreDefinedTable_1, P_WARNING, 2,
                        "AT CMD, invalid pre-defined AT, no AT name, pPreCmd->cmdType is %d, pPreCmd->paramMaxNum is %d",
                        pPreCmd->cmdType, pPreCmd->paramMaxNum);

            ret = CMS_FAIL;
            continue;
        }

        strLen = strlen(pPreCmd->pName);

        if (strLen <= 0 || strLen > AT_CMD_MAX_NAME_LEN)
        {
            ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcCheckPreDefinedTable_2, P_WARNING, 1,
                        "AT CMD, invalid pre-defined AT, AT name strLen is %d", strLen);

            ECOMM_STRING(UNILOG_ATCMD_PARSER, atcCheckPreDefinedTable_warning_1, P_WARNING,
                         "AT CMD: %s, name len is 0, or greater than 32", (UINT8 *)(pPreCmd->pName));
            ret = CMS_FAIL;
            continue;
        }

        /*
         * check time out value
        */
        OsaDebugBegin(pPreCmd->timeOutS > 0, pPreCmd->timeOutS, 0, 0);
        ECOMM_STRING(UNILOG_ATCMD_PARSER, atcCheckPreDefinedTable_warning_2, P_WARNING,
                     "AT CMD: %s, time out can't set to 0", (UINT8 *)(pPreCmd->pName));
        ret = CMS_FAIL;
        OsaDebugEnd();

        if (pPreCmd->cmdType == AT_BASIC_CMD)
        {
            if (!(pPreCmd->pName[0] == ' ' && strLen == 1))    /* 'AT' is a right basic command */
            {
                for (charIdx = 0; charIdx < strLen; charIdx++)
                {
                    tmpChr = pPreCmd->pName[charIdx];

                    /* A ~ Z, & */
                    if ((tmpChr < 'A' || tmpChr > 'Z') &&
                        (tmpChr < 'a' || tmpChr > 'z') &&
                        (tmpChr < '0' || tmpChr > '9') &&
                        (tmpChr != '&'))    //AT&Z
                    {
                        OsaDebugBegin(FALSE, pPreCmd->timeOutS, pPreCmd->paramMaxNum, pPreCmd->cmdType);
                        ECOMM_STRING(UNILOG_ATCMD_PARSER, atcCheckPreDefinedTable_warning_3, P_WARNING,
                                     "AT CMD: %s, basic AT name not right", (UINT8 *)(pPreCmd->pName));
                        ret = CMS_FAIL;
                        break;
                        OsaDebugEnd();
                    }
                }
            }
        }
        else    //extended AT command
        {
            // first byte should be extended byte
            if (AT_BE_EXT_CMD_CHAR(pPreCmd->pName[0]) != TRUE)
            {
                OsaDebugBegin(FALSE, pPreCmd->pName[0], pPreCmd->pName[1], pPreCmd->pName[2]);
                ECOMM_STRING(UNILOG_ATCMD_PARSER, atcCheckPreDefinedTable_warning_invalid_ext_1, P_WARNING,
                             "AT CMD: %s, extended CHAR is not right", (UINT8 *)pPreCmd->pName);
                ret = CMS_FAIL;
                OsaDebugEnd();
            }
            else
            {
                for (charIdx = 1; charIdx < strLen; charIdx++)
                {
                    tmpChr = pPreCmd->pName[charIdx];

                    /* A ~ Z, & */
                    if ((tmpChr < 'A' || tmpChr > 'Z') &&
                        (tmpChr < 'a' || tmpChr > 'z') &&
                        (tmpChr < '0' || tmpChr > '9') &&
                        (tmpChr != '!') && (tmpChr != '-') &&
                        (tmpChr != '.') && (tmpChr != '/') &&
                        (tmpChr != ':') && (tmpChr != '_'))
                    {
                        OsaDebugBegin(FALSE, pPreCmd->pName[0], pPreCmd->pName[1], pPreCmd->pName[2]);
                        ECOMM_STRING(UNILOG_ATCMD_PARSER, atcCheckPreDefinedTable_warning_4, P_WARNING,
                                     "AT CMD: %s, extended AT name not right", (UINT8 *)pPreCmd->pName);
                        ret = CMS_FAIL;
                        break;
                        OsaDebugEnd();
                    }

                }
            }

        }

        /*
         * func
        */
        OsaDebugBegin(pPreCmd->atProcFunc != PNULL, pPreCmd->paramMaxNum, tmpIdx, 0);
        ECOMM_STRING(UNILOG_ATCMD_PARSER, atcCheckPreDefinedTable_warning_5, P_WARNING,
                     "AT CMD: %s, proc func is NULL", (UINT8 *)pPreCmd->pName);
        ret = CMS_FAIL;
        OsaDebugEnd();

        /*
         * param num : AT_CMD_PARAM_MAX_NUM
        */
        OsaDebugBegin(pPreCmd->paramMaxNum < AT_CMD_PARAM_MAX_NUM, pPreCmd->paramMaxNum, tmpIdx, 0);
        ECOMM_STRING(UNILOG_ATCMD_PARSER, atcCheckPreDefinedTable_warning_6, P_WARNING,
                     "AT CMD: %s, too many param >= 32", (UINT8 *)pPreCmd->pName);
        ret = CMS_FAIL;
        OsaDebugEnd();

        /*
         * param list: pParamList
        */
        if (pPreCmd->paramMaxNum > 0)
        {
            OsaDebugBegin(pPreCmd->pParamList != PNULL, pPreCmd->paramMaxNum, tmpIdx, 0);
            ECOMM_STRING(UNILOG_ATCMD_PARSER, atcCheckPreDefinedTable_warning_7, P_WARNING,
                         "AT CMD: %s, pre-defined param list is NULL ", (UINT8 *)pPreCmd->pName);
            ret = CMS_FAIL;
            OsaDebugEnd();
        }
    }

    return ret;
}


/**
  \fn           void atcFreeAtInputNodeAndAtStr(AtCmdInputNode **pNode)
  \brief
  \param[in]
  \returns      void
*/
static void atcFreeAtInputNodeAndAtStr(AtCmdInputNode **pNode)
{
    OsaCheck(pNode != PNULL && *pNode != PNULL, pNode, 0, 0);

    /*Free string memory*/
    if ((*pNode)->pStart != PNULL)
    {
        OsaFreeMemory(&((*pNode)->pStart));
    }
    else
    {
        OsaDebugBegin(FALSE, (*pNode)->pStart, pNode, 0);
        OsaDebugEnd();
    }

    /*Free the node*/
    OsaFreeMemory(pNode);

    return;
}

/**
  \fn           void atcFreeAtInputNode(AtCmdInputNode **pNode)
  \brief
  \param[in]
  \returns      void
*/
static void atcFreeAtInputNode(AtCmdInputNode **pNode)
{
    OsaCheck(pNode != PNULL && *pNode != PNULL, pNode, 0, 0);

    /*Free the node*/
    OsaFreeMemory(pNode);

    return;
}

/**
  \fn           atcFreeAtInputNodeHeader(AtInputInfo *pAtInput, BOOL freeStr)
  \brief
  \param[in]
  \returns      void
*/
static void atcFreeAtInputNodeHeader(AtInputInfo *pAtInput, BOOL freeStr)
{
    AtCmdInputNode  *pFreeNode = PNULL;

    OsaCheck(pAtInput->input.cmdInput.pHdr != PNULL && pAtInput->input.cmdInput.pTailer != PNULL && pAtInput->input.cmdInput.pTailer->pNextNode == PNULL,
             pAtInput->input.cmdInput.pHdr, pAtInput->input.cmdInput.pTailer, freeStr);

    pFreeNode = pAtInput->input.cmdInput.pHdr;

    /* check  */
    OsaCheck(pAtInput->pendingNodeNum > 0 && pAtInput->pendingLen >= (pFreeNode->pEnd - pFreeNode->pNextDec + 1),
             pAtInput->pendingNodeNum, pAtInput->pendingLen, (pFreeNode->pEnd - pFreeNode->pNextDec + 1));

    pAtInput->pendingNodeNum--;
    pAtInput->pendingLen -= (pFreeNode->pEnd - pFreeNode->pNextDec + 1);

    pAtInput->input.cmdInput.pHdr = pAtInput->input.cmdInput.pHdr->pNextNode;

    pFreeNode->pNextNode = PNULL;

    if (freeStr)
    {
        atcFreeAtInputNodeAndAtStr(&pFreeNode);
    }
    else
    {
        atcFreeAtInputNode(&pFreeNode);
    }

    if (pAtInput->input.cmdInput.pHdr == PNULL)
    {
        pAtInput->input.cmdInput.pTailer = 0;
        pAtInput->pendingLen = 0;
        pAtInput->pendingNodeNum = 0;
    }

    return;
}

/**
  \fn           atcFreeAtInputNodeHeader(AtInputInfo *pAtInput)
  \brief
  \param[in]
  \returns      void
*/
static void atcFreeAtApiInputNodeHeader(AtInputInfo *pAtInput)
{
    AtApiInputNode  *pFreeNode = PNULL;

    OsaCheck(pAtInput->input.apiInput.pHdr != PNULL && pAtInput->input.apiInput.pTailer != PNULL && pAtInput->input.apiInput.pTailer->pNextNode == PNULL,
             pAtInput->input.apiInput.pHdr, pAtInput->input.apiInput.pTailer, pAtInput);

    pFreeNode = pAtInput->input.apiInput.pHdr;

    /* check  */
    OsaCheck(pAtInput->pendingNodeNum > 0 && pAtInput->pendingLen >= pFreeNode->pEnd - pFreeNode->pStart + 1,
             pAtInput->pendingNodeNum, pAtInput->pendingLen, (pFreeNode->pEnd - pFreeNode->pStart + 1));

    pAtInput->pendingNodeNum--;
    pAtInput->pendingLen -= (pFreeNode->pEnd - pFreeNode->pStart + 1);

    pAtInput->input.apiInput.pHdr = pAtInput->input.apiInput.pHdr->pNextNode;

    pFreeNode->pNextNode = PNULL;

    OsaFreeMemory(&pFreeNode);

    if (pAtInput->input.apiInput.pHdr == PNULL)
    {
        pAtInput->input.apiInput.pTailer = 0;
        pAtInput->pendingLen = 0;
        pAtInput->pendingNodeNum = 0;
    }

    return;
}


/**
  \fn           void atcInitSetAtCmdLineInfo(AtCmdLineInfo *pAtLine, CHAR *pLine, CHAR *pStart, CHAR *pEnd, UINT32 timeOutMs)
  \brief
  \param[in]
  \returns      void
*/
static void atcInitSetAtCmdLineInfo(AtCmdLineInfo *pAtLine, CHAR *pLine, CHAR *pStart, CHAR *pEnd, UINT32 timeOutMs)
{
    OsaCheck(pAtLine != PNULL && pStart != PNULL && pEnd != PNULL && pEnd >= pStart && pStart >= pLine,
             pAtLine, pStart, pEnd);

    pAtLine->pLine      = pLine;
    pAtLine->pEnd       = pEnd;
    pAtLine->pNextHdr   = pStart;
    pAtLine->startLine  = TRUE;
    pAtLine->startAt    = TRUE;
    pAtLine->timeOutMs  = timeOutMs;

    return;
}


/**
  \fn           void atcFreeAtCmdLine(AtChanEntity *pAtChanEty)
  \brief
  \param[in]
  \returns      void
*/
static void atcFreeAtCmdLine(AtChanEntity *pAtChanEty)
{
    AtCmdLineInfo   *pLineInfo = &(pAtChanEty->atLineInfo);

    if (pLineInfo->pLine != PNULL)
    {
        OsaFreeMemory(&(pLineInfo->pLine));
    }

    memset(pLineInfo, 0x00, sizeof(AtCmdLineInfo));

    return;
}

/**
  \fn           void atcMoveToNextAtCmdLine(AtChanEntity *pAtChanEty, CHAR *pFromChar)
  \brief        Current AT line is done (SUCC or ERROR), need to move to next line
  \param[in]    pAtChanEty          AT channel entity
  \param[in]    pFromChar           point to decoded character
  \returns      void
*/
static void atcMoveToNextAtCmdLine(AtChanEntity *pAtChanEty, CHAR *pFromChar)
{
    AtCmdLineInfo   *pLineInfo = &(pAtChanEty->atLineInfo);
    CHAR    *pNextLine = pFromChar;
    BOOL    firEndFound = FALSE;


    OsaCheck(pFromChar != PNULL && pFromChar >= pLineInfo->pNextHdr && pFromChar <= pLineInfo->pEnd,
             pFromChar, pLineInfo->pNextHdr, pLineInfo->pEnd);

    if (pLineInfo->startLine == TRUE) //already move to next line
    {
        OsaCheck(pLineInfo->pNextHdr != PNULL && pFromChar <= pLineInfo->pNextHdr, pLineInfo->pNextHdr, pFromChar, 0);
        return;
    }

    /*
     * ATxxxxxx<S3>ATxxxxxx<S3>
    */
    for (; pNextLine < pLineInfo->pEnd; pNextLine++)
    {
        if (firEndFound == FALSE)
        {
            if ((*pNextLine) == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX])
            {
                firEndFound = TRUE;
            }
        }
        else
        {
            /*skip <SP>/<S3>/<S4> */
            if ((*pNextLine) != ' ' &&
                (*pNextLine) != pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] &&
                (*pNextLine) != pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX])
            {
                break;
            }
        }
    }

    /* no new line */
    if (pNextLine >= pLineInfo->pEnd)
    {
        pLineInfo->pNextHdr     = PNULL;
        pLineInfo->startLine    = FALSE;
        pLineInfo->startAt      = FALSE;
    }
    else
    {
        pLineInfo->pNextHdr     = pNextLine;
        pLineInfo->startLine    = TRUE;
        pLineInfo->startAt      = TRUE;
    }

    return;
}


/**
  \fn           void atcFoundAtCmdLineEnd(AtChanEntity *pAtChanEty, CHAR *pDecEndLine, CHAR **pDecNextChar)
  \brief        Found ending character of one AT command line: <S3>
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    pDecEndLine     ending character position
  \param[out]   pDecNextChar    output, point to next decode charcater
  \returns      void
*/
static void atcFoundAtCmdLineEnd(AtChanEntity *pAtChanEty, CHAR *pDecEndLine, CHAR **pDecNextChar)
{
    AtCmdLineInfo   *pLineInfo = &(pAtChanEty->atLineInfo);
    CHAR    *pDecChar = pDecEndLine;

    OsaCheck(pDecChar > pLineInfo->pNextHdr && pDecChar <= pLineInfo->pEnd && pDecNextChar != PNULL,
             pDecChar, pLineInfo->pNextHdr, pLineInfo->pEnd);

    /*
     * AT line end with: <S3>
    */
    if (pDecChar < pLineInfo->pEnd)
    {
        pDecChar++; //as <S3> maybe changed to '\0' for string type of parameter, move to next
    }

    for (; pDecChar < pLineInfo->pEnd; pDecChar++)
    {
        /*skip <SP>/<S3>/<S4> */
        if ((*pDecChar) != ' ' &&
            (*pDecChar) != pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] &&
            (*pDecChar) != pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX])
        {
            break;
        }
    }

    /* no new line */
    if (pDecChar >= pLineInfo->pEnd)
    {
        pLineInfo->pNextHdr     = PNULL;
        pLineInfo->startLine    = FALSE;
        pLineInfo->startAt      = FALSE;
    }
    else
    {
        pLineInfo->pNextHdr     = pDecChar;
        pLineInfo->startLine    = TRUE;
        pLineInfo->startAt      = TRUE;
    }

    *pDecNextChar = pDecChar;

    return;
}


/**
  \fn           void atcFoundAtCmdEnd(AtChanEntity *pAtChanEty, CHAR *pDecEndAt, CHAR **pDecNextChar)
  \brief        Found ending character of one AT command, for extended AT: ';', for basic AT: ' '
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    pDecEndAt       ending character position
  \param[out]   pDecNextChar    output, point to next decode charcater
  \returns      void
*/
static void atcFoundAtCmdEnd(AtChanEntity *pAtChanEty, CHAR *pDecEndAt, CHAR **pDecNextChar)
{
    AtCmdLineInfo   *pLineInfo = &(pAtChanEty->atLineInfo);
    CHAR    *pDecChar = pDecEndAt;

    OsaCheck(pDecEndAt > pLineInfo->pNextHdr && pDecEndAt < pLineInfo->pEnd && pDecNextChar != PNULL,
             pDecEndAt, pLineInfo->pNextHdr, pLineInfo->pEnd);

    OsaCheck(pLineInfo->startLine == FALSE && pLineInfo->startAt == FALSE,
             pLineInfo->pNextHdr, pDecEndAt, pLineInfo->startAt);

    /*
     * AT CMD end with: ';', or ' '
    */
    pDecChar++; //as ';' maybe changed to '\0' for string type of parameter, move to next

    //skip space
    AT_IGNORE_SPACE_CHAR(pDecChar, pLineInfo->pEnd);

    /* if end of one line */
    if (*pDecChar == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX])
    {
        atcFoundAtCmdLineEnd(pAtChanEty, pDecChar, pDecNextChar);
        return;
    }
    else
    {
        pLineInfo->pNextHdr     = pDecChar;
        pLineInfo->startAt      = TRUE;
        pLineInfo->startLine    = FALSE;

        *pDecNextChar = pDecChar;
    }

    return;
}



/**
  \fn           atcStartAsynTimer(AtChanEntityP pAtChanEty, UINT32 timeValueMs)
  \brief        start AT asynchronoous response guard timer
  \param[in]
  \returns      TID: guard timer index
*/
static UINT8 atcStartAsynTimer(AtChanEntityP pAtChanEty, UINT32 timeValueMs)
{
    UINT16  timerId = 0;
    UINT8   tid = 0;

    OsaCheck(pAtChanEty->nextTid <= AT_MAX_ASYN_GUARD_TIMER_TID &&
             pAtChanEty->asynTimer == OSA_TIMER_NOT_CREATE &&
             timeValueMs > 0,
             pAtChanEty->nextTid, pAtChanEty->asynTimer, timeValueMs);

    ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcStartAsynTimer_1, P_VALUE, 1,
                "AT CMD, start guard time: %d ms", timeValueMs);

    tid = pAtChanEty->nextTid;

    timerId = AT_SET_ASYN_TIMER_ID(pAtChanEty->chanId, tid);

    /*
     * create the timer
    */
    pAtChanEty->curTid      = tid;
    pAtChanEty->asynTimer   = OsaTimerNew(CMS_TASK_ID, timerId, osTimerOnce);
    OsaCheck(pAtChanEty->asynTimer != OSA_TIMER_NOT_CREATE, tid, timerId, 0);

    /*
     * start timer
    */
    OsaTimerStart(pAtChanEty->asynTimer, MILLISECONDS_TO_TICKS(timeValueMs));

    /*
     * nextTid++
    */
    pAtChanEty->nextTid++;
    if (pAtChanEty->nextTid > AT_MAX_ASYN_GUARD_TIMER_TID)
    {
        pAtChanEty->nextTid = 0;
    }

    return tid;
}


/**
  \fn           void atcChanEntityInit(AtChanEntityP pAtChanEty, UINT8 chanId, BOOL bApiMode)
  \brief        Init a AT channl entity
  \param[in]
  \returns     void
*/
static void atcChanEntityInit(AtChanEntityP pAtChanEty, UINT8 chanId, BOOL bApiMode)
{
    OsaCheck(pAtChanEty != PNULL && chanId > CMS_CHAN_RSVD && chanId < CMS_CHAN_NUM, pAtChanEty, chanId, 0);

    if (pAtChanEty->bInited)
    {
        return;
    }

    memset(pAtChanEty, 0x00, sizeof(AtChanEntity));

    pAtChanEty->chanId      = chanId;
    pAtChanEty->bApiMode    = bApiMode;

    /*
     * config
    */
    pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] = AT_S3_CMD_LINE_TERM_CHAR_DEFAULT;
    pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX]   = AT_S4_RESP_FORMAT_CHAR_DEFAULT;
    pAtChanEty->cfg.S[AT_S5_CMD_LINE_EDIT_CHAR_IDX] = AT_S5_CMD_LINE_EDIT_CHAR_DEFAULT;

    pAtChanEty->cfg.echoFlag            = mwGetAtChanConfigItemValue((UINT8)chanId, MID_WARE_AT_CHAN_ECHO_CFG);
    pAtChanEty->cfg.respFormat          = 1;    //ATV1
    pAtChanEty->cfg.suppressValue       = mwGetAtChanConfigItemValue((UINT8)chanId, MID_WARE_AT_CHAN_SUPPRESS_CFG);


    /*
     * AT LIST
    */
    pAtChanEty->pPreDefCmdList          = atcGetATCommandsSeqPointer();
    pAtChanEty->preDefCmdNum            = atcGetATCommandsSeqNumb();
    pAtChanEty->pPreDefCustCmdList      = atcGetATCustCommandsSeqPointer();
    pAtChanEty->preDefCustCmdNum        = atcGetATCustCommandsSeqNumb();
    #ifdef  FEATURE_REF_AT_ENABLE
    pAtChanEty->pPreDefRefCmdList       = atcGetRefAtCmdSeqPointer();
    pAtChanEty->preDefRefCmdNum         = (UINT16)atcGetRefAtCmdSeqNumb();
    #else
    pAtChanEty->pPreDefRefCmdList       = PNULL;
    pAtChanEty->preDefRefCmdNum         = 0;
    #endif

    if (bApiMode)
    {
        /* not echo  */
        pAtChanEty->cfg.echoFlag        = 0;    //ATE0
        pAtChanEty->cfg.respFormat      = 1;    //ATV1
        pAtChanEty->cfg.suppressValue   = 0;    //ATQ0

        strncpy(pAtChanEty->chanName, AT_RIL_API_CHAN_NAME, AT_CHAN_NAME_SIZE);
        pAtChanEty->chanName[AT_CHAN_NAME_SIZE-1] = '\0';
    }

    /*
     * INIT done
    */
    pAtChanEty->bInited = TRUE;

    return;
}

/**
  \fn           void atcChanEntityRegister(AtChanEntityP *pAtChanEty, AtChanRegInfo *pRegInfo)
  \brief        register the at channel callback
  \param[in]
  \returns      void
*/
static void atcChanEntityRegister(AtChanEntityP pAtChanEty, AtChanRegInfo *pRegInfo)
{
    OsaCheck(pAtChanEty != PNULL && pRegInfo != PNULL, pAtChanEty, pRegInfo, 0);
    OsaCheck(pAtChanEty->bInited, pAtChanEty->chanId, 0, 0);

    /*
     * this channel should new created
    */
    if (pAtChanEty->chanName[0] != '\0')
    {
        OsaDebugBegin(FALSE, pAtChanEty->chanId, 0, 0);
        OsaDebugEnd();
    }

    if (strlen(pRegInfo->chanName) > 0)
    {
        strncpy(pAtChanEty->chanName, pRegInfo->chanName, AT_CHAN_NAME_SIZE);
    }
    else
    {
        OsaDebugBegin(FALSE, 0, 0, 0);
        OsaDebugEnd();

        strncpy(pAtChanEty->chanName, "TEMPAT", AT_CHAN_NAME_SIZE);
    }

    pAtChanEty->chanName[AT_CHAN_NAME_SIZE-1] = '\0';

    if (pRegInfo->atRespFunc != PNULL)
    {
        pAtChanEty->callBack.respFunc   = pRegInfo->atRespFunc;
        pAtChanEty->callBack.pArg       = pRegInfo->pRespArg;
    }
    else
    {
        OsaDebugBegin(FALSE, 0, 0, 0);
        OsaDebugEnd();
    }

    if (pRegInfo->atUrcFunc != PNULL)
    {
        pAtChanEty->callBack.urcFunc   = pRegInfo->atUrcFunc;
    }
    else
    {
        OsaDebugBegin(FALSE, 0, 0, 0);
        OsaDebugEnd();
    }

    return;
}



/**
  \fn           AtCmdPreDefInfoC *atcFindPreDefinedAtInfo(AtChanEntityP pAtChanEty, const CHAR *pAtName)
  \brief        find the pre-defined AT info according to AT NAME
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    pAtName         AT command name, end with '\0'
  \returns      AtCmdPreDefInfoC
*/
static AtCmdPreDefInfoC *atcFindPreDefinedAtInfo(AtChanEntityP pAtChanEty, const CHAR *pAtName, BOOL bBasicAt)
{
    AtCmdPreDefInfoC    *pAtPreDefInfo = PNULL;
    UINT32  nameLen = strlen(pAtName);
    UINT32  index = 0;
    CHAR    upChar = toupper(pAtName[1]);

    OsaCheck(pAtName != PNULL && nameLen <= AT_CMD_MAX_NAME_LEN && nameLen > 0 &&
             pAtChanEty->pPreDefCmdList != PNULL && pAtChanEty->pPreDefCustCmdList != PNULL,
             pAtName, nameLen, pAtChanEty->pPreDefCmdList);

    if (bBasicAt != TRUE)
    {
        /* find the PS AT CMD table firstly */
        while (index < pAtChanEty->preDefCmdNum)
        {
            pAtPreDefInfo = pAtChanEty->pPreDefCmdList + index;

            if ((pAtPreDefInfo->cmdType != AT_BASIC_CMD) &&
                (upChar == toupper(pAtPreDefInfo->pName[1])) &&
                (strcasecmp(pAtPreDefInfo->pName, pAtName) == 0))

            {
                return pAtPreDefInfo;
            }

            index++;
        }

        /* CUST AT CMD table */
        index = 0;
        while (index < pAtChanEty->preDefCustCmdNum)
        {
            pAtPreDefInfo = pAtChanEty->pPreDefCustCmdList + index;

            if ((pAtPreDefInfo->cmdType != AT_BASIC_CMD) &&
                (upChar == toupper(pAtPreDefInfo->pName[1])) &&
                (strcasecmp(pAtPreDefInfo->pName, pAtName) == 0))

            {
                return pAtPreDefInfo;
            }

            index++;
        }

        /* REF AT CMD table */
        index = 0;
        while (index < pAtChanEty->preDefRefCmdNum)
        {
            pAtPreDefInfo = pAtChanEty->pPreDefRefCmdList + index;

            if ((pAtPreDefInfo->cmdType != AT_BASIC_CMD) &&
                (upChar == toupper(pAtPreDefInfo->pName[1])) &&
                (strcasecmp(pAtPreDefInfo->pName, pAtName) == 0))

            {
                return pAtPreDefInfo;
            }

            index++;
        }
    }
    else    //basic AT
    {
        upChar = toupper(pAtName[0]);

        /* find the PS AT CMD table firstly */
        while (index < pAtChanEty->preDefCmdNum)
        {
            pAtPreDefInfo = pAtChanEty->pPreDefCmdList + index;

            if ((pAtPreDefInfo->cmdType == AT_BASIC_CMD) &&
                (upChar == toupper(pAtPreDefInfo->pName[0])) &&
                (strcasecmp(pAtPreDefInfo->pName, pAtName) == 0))

            {
                return pAtPreDefInfo;
            }

            index++;
        }

        /* CUST AT CMD table */
        index = 0;
        while (index < pAtChanEty->preDefCustCmdNum)
        {
            pAtPreDefInfo = pAtChanEty->pPreDefCustCmdList + index;

            if ((pAtPreDefInfo->cmdType == AT_BASIC_CMD) &&
                (upChar == toupper(pAtPreDefInfo->pName[0])) &&
                (strcasecmp(pAtPreDefInfo->pName, pAtName) == 0))

            {
                return pAtPreDefInfo;
            }

            index++;
        }

        /* REF AT CMD table */
        index = 0;
        while (index < pAtChanEty->preDefRefCmdNum)
        {
            pAtPreDefInfo = pAtChanEty->pPreDefRefCmdList + index;

            if ((pAtPreDefInfo->cmdType == AT_BASIC_CMD) &&
                (upChar == toupper(pAtPreDefInfo->pName[0])) &&
                (strcasecmp(pAtPreDefInfo->pName, pAtName) == 0))

            {
                return pAtPreDefInfo;
            }

            index++;
        }
    }

    return PNULL;
}

/**
  \fn           AtcDecRetCode atcDecJsonParam(AtChanEntityP pAtChanEty,
  \                                           CHAR   *pDecParam,
  \                                           CHAR   **pJsonStart,
  \                                           CHAR   **pJsonEnd,
  \                                           CHAR   **pDecNextChar,
  \                                           BOOL   *pParamEnd)
  \brief        decode JSON parameter
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    pDecParam       CHAR point to start to JSON <parameter_value>
  \param[out]   pJsonStart      output, point to start of JSON parameter, if failed, point to PNULL
  \param[out]   pJsonEnd        output, point to end of JSON parameter
  \param[out]   pDecNextChar    output, point to next decode charcater
  \param[out]   pParamEnd       output, whether parameter ending
  \returns      AtcDecRetCode
*/
static AtcDecRetCode atcDecJsonParam(AtChanEntityP pAtChanEty,
                                     CHAR   *pDecParam,
                                     CHAR   **pJsonStart,
                                     CHAR   **pJsonEnd,
                                     CHAR   **pDecNextChar,
                                     BOOL   *pParamEnd)
{
    AtCmdLineInfo       *pAtLine = &(pAtChanEty->atLineInfo);
    CHAR    *pDecChar = pDecParam;
    UINT32  quoteNum = 0;           // " number, not include beginning/ending quote
    UINT32  openSquareNum = 0;      // [ number
    UINT32  closeSquareNum = 0;     // [ number
    UINT32  openBraceNum = 0;       // { number
    UINT32  closeBraceNum = 0;      // { number
    BOOL    bQuoteBegin = FALSE;    //whether this parameter begin with: "
    BOOL    bQuoteEnd = FALSE;      //whether ending '"' meet
    //BOOL    bJsonBegin  = FALSE;    //whether JSON begin, begins with "{" or "["
    BOOL    validParam  = TRUE;


    OsaCheck(pDecChar != PNULL && pDecChar > pAtLine->pNextHdr && pDecChar <= pAtLine->pEnd,
             pDecChar, pAtLine->pNextHdr, pAtLine->pEnd);
    OsaCheck(pJsonStart != PNULL && pJsonEnd != PNULL && pDecNextChar != PNULL && pParamEnd != PNULL,
             pJsonStart, pDecNextChar, pParamEnd);

    /*
     * init the output value
    */
    *pJsonStart = *pJsonEnd = PNULL;
    *pDecNextChar = pDecParam;
    *pParamEnd  = FALSE;

    /*
     * AT_JSON_VAL,
     * 1> if input: "{abcdefghijklmnopqrst}", then
     *               ^pJsonStart          ^pJsonEnd
     * 2> if input: {abcdefghijklmnopqrst}, then:
     *              ^pJsonStart          ^pJsonEnd
     * 3> if input: "    {abcdefghijklmnopqrst}   ", then
     *                   ^pJsonStart          ^pJsonEnd
     * 4> if input:     {abcdefghijklmnopqrst}    , then
     *                  ^pJsonStart          ^pJsonEnd
    */

    /*
     * JSON is built on two structure
     * 1> Object, begins with: {, ends with: }
     * 2> Array, begins with: [, ends with: ]
    */
    AT_IGNORE_SPACE_CHAR(pDecChar, pAtLine->pEnd);

    if (*pDecChar == '"')
    {
        bQuoteBegin = TRUE;
        pDecChar++;

        AT_IGNORE_SPACE_CHAR(pDecChar, pAtLine->pEnd);
    }

    /*
     * 1> Following characters are allowed (ignored) in JSON format
     *   a) ' ' (0x20); b) 0x09; b) 0x0A; b) 0x0D
     * 2> JSON should begin with '{' or '[', and end with '}' or ']'
     *
    */
    for (; pDecChar <= pAtLine->pEnd; pDecChar++)
    {
        /*
         * CHAR need to handle: '{'  '}'  '['  ']'  ' '  ';'  <S3>  '"'  ','
        */
        if ((*pDecChar == '{') ||
            (*pDecChar == '['))   //object, array
        {
            if ((quoteNum&0x01) == 0x01)
            {
                /*
                 * str: {"{asd": 5}
                 *        ^
                 */
                continue;
            }
            else
            {
                if (*pDecChar == '{')
                {
                    openBraceNum++;
                }
                else
                {
                    openSquareNum++;
                }
            }

            if (*pJsonStart == PNULL)
            {
                OsaCheck(*pJsonEnd == PNULL, *pJsonEnd, **pJsonEnd, quoteNum);
                *pJsonStart = pDecChar;
            }

            if (*pJsonEnd != PNULL)
            {
                OsaCheck(*pJsonStart != PNULL, *pJsonStart, *pJsonEnd, quoteNum);

                /*
                 * str: "{abc}"{    - NOK
                 * str: {abc} {     - NOK
                */
                OsaDebugBegin(FALSE, *pJsonStart, *pJsonEnd, pDecParam);
                OsaDebugEnd();

                validParam = FALSE;
                break;
            }

            if (bQuoteBegin && bQuoteEnd)
            {
                /* str: ""{ */
                OsaDebugBegin(FALSE, *pJsonStart, *pJsonEnd, pDecParam);
                OsaDebugEnd();

                validParam = FALSE;
                break;
            }
        }
        else if ((*pDecChar == '}') ||
                 (*pDecChar == ']'))
        {
            if ((quoteNum&0x01) == 0x01)
            {
                /*
                 * str: {"}asd": 5}
                 *        ^
                 */
                continue;
            }

            if (*pDecChar == '}')
            {
                closeBraceNum++;
            }
            else
            {
                closeSquareNum++;
            }

            if ((closeBraceNum > openBraceNum) ||
                (closeSquareNum > openSquareNum))
            {
                OsaDebugBegin(FALSE, *pDecChar, closeBraceNum, openBraceNum);
                OsaDebugEnd();
                validParam = FALSE;
                break;
            }
            else    //closeBraceNum <= openBraceNum && closeSquareNum <= openSquareNum
            {
                /* { must found before */
                OsaCheck(*pJsonStart != PNULL, *pJsonStart, *pJsonEnd, quoteNum);

                /*
                 * case: {[], []},
                 * But can't check case: {[}]   - let caller check it
                 */
                if ((closeBraceNum == openBraceNum) &&
                    (closeSquareNum == openSquareNum))
                {
                    OsaCheck(*pJsonEnd == PNULL, *pJsonStart, *pJsonEnd, quoteNum);
                    *pJsonEnd = pDecChar;
                }
            }

        }
        else if (*pDecChar == '"')
        {
            if (*(pDecChar-1) == '\\')
            {
                /*
                 * "{"abc": "\"asd\"abc"}"
                 *            ^
                */
                continue;
            }

            if ((openSquareNum > closeSquareNum) ||
                (openBraceNum > closeBraceNum))
            {
                /*
                 * {"abc" : 5}
                 *  ^
                 * "{"abc" : 5}"
                 *   ^
                */
                quoteNum++;
                continue;
            }

            /*
             * openSquareNum == closeSquareNum && openBraceNum == closeBraceNum
            */

            /*
             * "{"abc" : 5}"
             *             ^
            */
            if (bQuoteBegin && bQuoteEnd == FALSE)
            {
                bQuoteEnd = TRUE;
            }
            else
            {
                /*
                 * example:
                 * 1> {abc}"def
                 *         ^
                 * 2> "abc""
                 *         ^
                 * 3> """
                 *      ^
                 * ERROR
                */
                OsaDebugBegin(FALSE, bQuoteBegin, bQuoteEnd, pDecParam);
                OsaDebugEnd();

                validParam = FALSE;
                break;
            }
        }
        else
        {
            /* other CHAR, except: '{'  '}'  '['  ']' '"' */

            /* CHAR in JSON */
            if ((openSquareNum > closeSquareNum) ||
                (openBraceNum > closeBraceNum))
            {
                OsaCheck(*pJsonStart != PNULL, *pJsonStart, *pJsonEnd, quoteNum);

                /*
                 *1> Following characters are allowed (ignored) in JSON format
                 *   a) ' ' (0x20); b) 0x09; b) 0x0A; b) 0x0D
                */
                if (AT_BE_VALID_CHAR(*pDecChar) ||
                    ((*pDecChar) == ' ') || ((*pDecChar) == 0x09) ||
                    ((*pDecChar) == 0x0A) || ((*pDecChar) == 0x0D))
                {
                    continue;
                }
                else
                {
                    OsaDebugBegin(FALSE, *pDecChar, bQuoteBegin, bQuoteEnd);
                    OsaDebugEnd();

                    validParam = FALSE;
                    break;
                }
            }

            /* CHAR between: "   " */
            if (bQuoteBegin && bQuoteEnd == FALSE)
            {
                /*
                 *1> Following characters are allowed (ignored) in JSON format
                 *   a) ' ' (0x20); b) 0x09; b) 0x0A; b) 0x0D
                 *  example: " \r\n {abc} \r\n " - OK
                */
                if (((*pDecChar) == ' ') || ((*pDecChar) == 0x09) ||
                    ((*pDecChar) == 0x0A) || ((*pDecChar) == 0x0D))
                {
                    continue;
                }
                else
                {
                    OsaDebugBegin(FALSE, *pDecChar, bQuoteBegin, bQuoteEnd);
                    OsaDebugEnd();

                    validParam = FALSE;
                    break;
                }
            }

            /*
             * CHAR not in JSON string, only accept AT CMD CHAR:
             *  a) ' '(0x20); b) ','; c) ';' d) <S3>
            */
            if ((*pDecChar) == ' ')
            {
                continue;
            }
            else if ((*pDecChar) == ',')
            {
                /* JSON parameter done */
                /* ',' decode done, point to next char, and current ',' maybe change to '\0' later in case: abc, */
                pDecChar++;
                break;
            }
            else if ((*pDecChar) == ';')
            {
                /* current ';' maybe change to '\0' later in case: abc; */
                *pParamEnd = TRUE;
                atcFoundAtCmdEnd(pAtChanEty, pDecChar, pDecNextChar);
                pDecChar = *pDecNextChar;

                break;
            }
            else if ((*pDecChar) == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX])
            {
                *pParamEnd = TRUE;
                atcFoundAtCmdLineEnd(pAtChanEty, pDecChar, pDecNextChar);
                pDecChar = *pDecNextChar;
                break;
            }
            else
            {
                // other, should not valid JSON parameter
                OsaDebugBegin(FALSE, *pDecChar, bQuoteBegin, bQuoteEnd);
                OsaDebugEnd();

                validParam = FALSE;
                break;
            }
        }
    }

    /*
     * in case: pDecChar == pAtLine->pEnd
    */
    if (validParam == TRUE)
    {
        /*
         * case 1: {[<S3>
        */
        if (*pJsonStart != PNULL && *pJsonEnd == PNULL)
        {
            OsaDebugBegin(FALSE, *pJsonStart, **pJsonStart, *pDecChar);
            OsaDebugEnd();
            validParam = FALSE;
        }

        /*
         * case 2: "   <S3>
        */
        if (bQuoteBegin && bQuoteEnd == FALSE)
        {
            OsaDebugBegin(FALSE, bQuoteBegin, bQuoteEnd, *pDecChar);
            OsaDebugEnd();
            validParam = FALSE;
        }
    }

    if (validParam == FALSE)
    {
        *pJsonStart = *pJsonEnd = PNULL;
        *pDecNextChar = pAtLine->pEnd;

        ECOMM_STRING(UNILOG_ATCMD_PARSER, atcDecJsonParam_warning_1, P_WARNING,
                     "AT CMD, invalid JSON param: %s", (UINT8 *)pDecParam);

        return ATC_DEC_SYNTAX_ERR;
    }

    /*
     * JSON PARAM is OK
    */
    if (*pJsonStart != PNULL)
    {
        OsaCheck(*pJsonEnd != PNULL && *pJsonEnd < pDecChar, *pJsonStart, *pJsonEnd, pDecChar);

        *(*pJsonEnd+1) = '\0';

        /*
         * example:
         * 1>  "{abcedfghigklmnopqrst}",
         *      ^                    ^
         *      *pJsonStart          *pJsonEnd
         *                            ^ ^
         *                            \0pDecChar
        */
    }
    else
    {
        OsaCheck(*pJsonEnd == PNULL, *pJsonStart, **pJsonStart, pAtLine->pEnd);
    }

    *pDecNextChar = pDecChar;

    /*
     * Case:
     * 1> "  ",
     * 2>    ,
     * In such cases, *pJsonStart = *pJsonEnd = PNULL, but return: ATC_DEC_OK
     *
    */

    return ATC_DEC_OK;
}


/**
  \fn           AtcDecRetCode atcDecAtParam(AtChanEntityP pAtChanEty,
  \                                         AtCmdPreDefInfoC *pAtPreDefInfo,
  \                                         CHAR *pDecParam,
  \                                         AtParamValue *pOutParamList,
  \                                         UINT32 *pOutParamNum,
  \                                         CHAR **pDecNextChar)
  \brief        decode input AT parameters
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    pAtPreDefInfo   AT pre-defined info
  \param[in]    pDecParam       CHAR point to start to <parameter_value>
  \param[in]    bBasicAt        whether basic AT command
  \param[out]   pOutParamList   output, parameter list
  \param[out]   pOutParamNum    output, parameter number
  \param[out]   pDecNextChar    output, point to next decode charcater
  \returns      AtcDecRetCode
*/
static AtcDecRetCode atcDecAtParam(AtChanEntityP pAtChanEty,
                                   AtCmdPreDefInfoC *pAtPreDefInfo,
                                   CHAR *pDecParam,
                                   BOOL bBasicAt,
                                   AtParamValue *pOutParamList,
                                   UINT32 *pOutParamNum,
                                   CHAR **pDecNextChar)
{
    AtCmdLineInfo       *pAtLine = &(pAtChanEty->atLineInfo);
    CHAR    *pDecChar = pDecParam;
    UINT32  paramNum = 0, paramLen = 0, quoteNum = 0;
    //const AtValueAttr   *pParamAttr = PNULL;
    UINT32  numValue = 0;
    CHAR    *pStrValueStart = PNULL;
    CHAR    *pStrValueEnd   = PNULL;
    BOOL    validParam = TRUE;
    BOOL    paramEnd = FALSE;   /* end of parameter: ";", <S3> */

    OsaCheck(pDecChar != PNULL && pDecChar > pAtLine->pNextHdr && pDecChar <= pAtLine->pEnd,
             pDecChar, pAtLine->pNextHdr, pAtLine->pEnd);

    OsaCheck(pAtPreDefInfo->paramMaxNum < AT_CMD_PARAM_MAX_NUM, pAtPreDefInfo->paramMaxNum, pAtPreDefInfo->pName[1], pAtPreDefInfo->pName[2]);

    *pOutParamNum = 0;

    /*
     * AT+CMD=<param1>,<param2>[,<param3>];/<S3>
     *        ^
     *        |pDecChar
    */

    /*
     * BASIC AT:
     * AT<command>[<number>]
     *             ^
     *             |pDecChar
     * ATS<parameter_number>= [<value>]
     *                         ^
     *                         pDecChar
     * 1> Not support ATD now
     * 2> Only one parameter
     * 3> ' ' should not be ignored, and ' ' just act as a separate char between two AT CMD
     *  example:
     *  ATCMD1 CMD2=12; +CMD1; +CMD2=,,15; +CMD2?; +CMD2=?<CR>
     *        ^
    */
    if (bBasicAt)
    {
        OsaDebugBegin(pAtPreDefInfo->paramMaxNum <= 1, pAtPreDefInfo->paramMaxNum, pAtPreDefInfo->pName[0], pAtPreDefInfo->pName[1]);
        OsaDebugEnd();
    }

    while (paramNum < pAtPreDefInfo->paramMaxNum)
    {
        // no matter the value type firstly, decode as the string type
        pStrValueStart = pStrValueEnd = PNULL;
        quoteNum = 0;
        validParam = TRUE;
        paramLen = 0;

        if (pAtPreDefInfo->pParamList[paramNum].type == AT_JSON_VAL)
        {
            /*JSON format need special process*/
            if (atcDecJsonParam(pAtChanEty, pDecChar, &pStrValueStart, &pStrValueEnd, pDecNextChar, &paramEnd) == ATC_DEC_OK)
            {
                OsaCheck(*pDecNextChar != PNULL && *pDecNextChar >= pDecChar && *pDecNextChar <= pAtLine->pEnd,
                         *pDecNextChar, pDecChar, pAtLine->pEnd);
                pDecChar = *pDecNextChar;
            }
            else
            {
                OsaCheck(*pDecNextChar != PNULL && *pDecNextChar >= pDecChar && *pDecNextChar <= pAtLine->pEnd,
                         *pDecNextChar, pDecChar, pAtLine->pEnd);
                pDecChar = *pDecNextChar;
                validParam = FALSE;
            }
        }
        else if (pAtPreDefInfo->pParamList[paramNum].type == AT_LAST_MIX_STR_VAL)
        {
            /*
             * AT CMD last parameter, maybe a MIX value, contain special characters: ','';','\r\n',
             *  AT decoder don't care it, just pass the whole string to the AT processor
            */
            AT_IGNORE_SPACE_CHAR(pDecChar, pAtLine->pEnd);

            if (pDecChar < pAtLine->pEnd)
            {
                pStrValueStart = pDecChar;

                /* pAtLine->pEnd => '\r' */
                pStrValueEnd   = pAtLine->pEnd - 1;

                pDecChar = pAtLine->pEnd;   //point to the end
            }
            //else: pDecChar == pAtLine->pEnd

            paramEnd = TRUE;
            atcFoundAtCmdLineEnd(pAtChanEty, pDecChar, pDecNextChar);
            pDecChar = *pDecNextChar;
        }
        else
        {
            for (; pDecChar <= pAtLine->pEnd; pDecChar++)
            {
                /*
                 * special characters: ' '/';'/<S3>/'"'/','
                */
                if (*pDecChar == ' ')
                {
                    if (bBasicAt)
                    {
                        /*
                         * str: ATE1 Q0
                         *          ^
                         *          |
                         * ' ' should not be ignored, and ' ' just act as a separate char between two AT CMD
                        */
                        if (quoteNum == 1)
                        {
                            /*str: " abc def" - OK*/
                            if (pStrValueStart == PNULL)
                            {
                                pStrValueStart = pDecChar;
                            }
                            continue;
                        }

                        if (pStrValueStart != PNULL && pStrValueEnd == PNULL)
                        {
                            /*str: abc  - OK */
                            pStrValueEnd = pDecChar - 1;
                        }
                        /*
                         * case:
                         * 1> str: "";     - OK
                         * 2> str: ,;      - OK
                         * pStrValueStart = pStrValueEnd = PNULL
                        */

                        /* current ';' maybe change to '\0' later in case: abc; */
                        paramEnd = TRUE;
                        atcFoundAtCmdEnd(pAtChanEty, pDecChar, pDecNextChar);
                        pDecChar = *pDecNextChar;

                        break;
                    }
                    else
                    {
                        if (quoteNum == 0)
                        {
                            if (pStrValueStart == PNULL) /* before param, str:    abc, - OK*/
                            {
                                continue;
                            }
                            else
                            {
                                if (pStrValueEnd == PNULL)  /*after param: str: abc   , - OK */
                                {
                                    pStrValueEnd = pDecChar - 1;
                                }
                                continue;
                            }
                        }
                        else if (quoteNum == 1)
                        {
                            /*
                             * str: "abc def" - OK
                             * str: " abcdef" - OK
                            */
                            if (pStrValueStart == PNULL)
                            {
                                pStrValueStart = pDecChar;
                            }
                            continue;
                        }
                        else //quoteNum == 2
                        {
                            /*str: "abc"   , - OK*/
                            continue;
                        }
                    }
                }
                else if (*pDecChar == ';')
                {
                    if (quoteNum == 1)
                    {
                        /*str: "abc;def" - OK*/
                        if (pStrValueStart == PNULL)
                        {
                            pStrValueStart = pDecChar;
                        }
                        continue;
                    }

                    /*
                     * str: abc ;    - OK
                     * str: "abc" ; - OK
                    */
                    if (pStrValueStart != PNULL &&
                        pStrValueEnd == PNULL)
                    {
                        /*str: abc; - OK */
                        pStrValueEnd = pDecChar - 1;
                    }
                    /*
                     * case:
                     * 1> str: "";     - OK
                     * 2> str: ,;      - OK
                     * pStrValueStart = pStrValueEnd = PNULL
                    */

                    /* current ';' maybe change to '\0' later in case: abc; */
                    paramEnd = TRUE;
                    atcFoundAtCmdEnd(pAtChanEty, pDecChar, pDecNextChar);
                    pDecChar = *pDecNextChar;

                    break;
                }
                else if (*pDecChar == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX])
                {
                    /* <S3>, line end */
                    if (quoteNum == 1)  /*str: "abc<S3>  - NOK*/
                    {
                        validParam = FALSE;
                    }
                    else
                    {
                        if (pStrValueStart != PNULL && pStrValueEnd == PNULL)
                        {
                            /*str: abc<S3> */
                            pStrValueEnd = pDecChar - 1;
                        }
                        //else: /*str: abc <S3>  or  "abc"<S3> */
                    }

                    /* current <S3> maybe change to '\0' later in case: abc<S3> */
                    paramEnd = TRUE;
                    atcFoundAtCmdLineEnd(pAtChanEty, pDecChar, pDecNextChar);
                    pDecChar = *pDecNextChar;
                    break;
                }
                else if (*pDecChar == '"')
                {
                    quoteNum++;

                    if (quoteNum == 1)
                    {
                        if (pStrValueStart != PNULL)    /*case: 123"456, -NOK */
                        {
                            validParam = FALSE;
                            break;
                        }
                    }
                    else if (quoteNum == 2)
                    {
                        if (pStrValueStart != PNULL)
                        {
                            pStrValueEnd = pDecChar - 1;    /* "abc", point to: c */
                        }
                        //else  /* case: "" */
                    }
                    else    //quoteNum > 2
                    {
                        validParam = FALSE;
                        break;
                    }

                    continue;
                }
                else if (*pDecChar == ',')
                {
                    if (quoteNum == 1)
                    {
                        /* str: "abc,ef" - OK */
                        continue;
                    }

                    if (pStrValueStart != PNULL && pStrValueEnd == PNULL)
                    {
                        /* str: abc,  - OK*/
                        pStrValueEnd = pDecChar - 1;
                    }
                    //else: /* str: abc ,"abc","",, - all OK */

                    /* ',' decode done, point to next char, and current ',' maybe change to '\0' later in case: abc, */
                    pDecChar++;
                    break;
                }
                else if (AT_BE_VALID_CHAR(*pDecChar))
                {
                    /* other valid characters, exclude: ' '/';'/<S3>/'"'/',' */
                    if (quoteNum >= 2)  //
                    {
                        /* str: "abc"def  - NOK */
                        validParam = FALSE;
                        break;
                    }

                    if (pStrValueStart == PNULL)
                    {
                        OsaCheck(pStrValueEnd == PNULL, pStrValueStart, pStrValueEnd, quoteNum);
                        pStrValueStart = pDecChar;
                    }

                    if (pStrValueEnd != PNULL)
                    {
                        OsaCheck(pStrValueStart != PNULL, pStrValueStart, pStrValueEnd, quoteNum);

                        /*
                         * str: "abc"def  - NOK
                         * str: abc def   - NOK
                        */
                        validParam = FALSE;
                        break;
                    }

                    /* check the length here? - OK*/
                    OsaCheck(pDecChar >= pStrValueStart, pDecChar, pStrValueStart, pStrValueEnd);
                    paramLen = pDecChar - pStrValueStart + 1;

                    if (pAtPreDefInfo->pParamList[paramNum].type == AT_DEC_VAL)
                    {
                        if (quoteNum > 0 || paramLen >= AT_DEC_NUM_STR_MAX_LEN)
                        {
                            OsaDebugBegin(FALSE, quoteNum, paramLen, AT_DEC_NUM_STR_MAX_LEN);
                            OsaDebugEnd();

                            /*
                             * str: "123" - NOK
                             * str: 123445634666445 - NOK
                            */
                            validParam = FALSE;
                            break;
                        }
                    }
                    else if (pAtPreDefInfo->pParamList[paramNum].type == AT_HEX_VAL)
                    {
                        if (quoteNum > 0 || paramLen >= AT_HEX_NUM_STR_MAX_LEN)
                        {
                            OsaDebugBegin(FALSE, quoteNum, paramLen, AT_HEX_NUM_STR_MAX_LEN);
                            OsaDebugEnd();

                            /*
                             * str: "123" - NOK
                             * str: FEFFFFFFFF - NOK
                            */
                            validParam = FALSE;
                            break;
                        }
                    }
                    else if (pAtPreDefInfo->pParamList[paramNum].type == AT_BIN_VAL)
                    {
                        if (quoteNum > 0 || paramLen >= AT_BIN_NUM_STR_MAX_LEN)
                        {
                            OsaDebugBegin(FALSE, quoteNum, paramLen, AT_BIN_NUM_STR_MAX_LEN);
                            OsaDebugEnd();

                            /*
                             * str: "123" - NOK
                             * str: 11111111 11111111 11111111 11111111 111 - NOK
                            */
                            validParam = FALSE;
                            break;
                        }
                    }

                    continue;
                }
                else    //invalid CHAR
                {
                    OsaDebugBegin(FALSE, quoteNum, *pDecChar, 0);
                    OsaDebugEnd();

                    validParam = FALSE;
                    break;
                }
            }
        }


        /* end of one parameter */
        if (validParam == FALSE)
        {
            OsaDebugBegin(FALSE, paramNum, pAtPreDefInfo->paramMaxNum, 0);
            OsaDebugEnd();

            *pDecNextChar = pDecChar;

            return ATC_DEC_SYNTAX_ERR;
        }

        /*
         * Set parameter value
         * 1> "abcdefghijklmnopqrstuvwxyz"
         *     ^                        ^
         *     |pStrValueStart          |pStrValueEnd
         * 2> abcdefghijklmnopqrstuvwxyz
         *    ^                        ^
         *    |pStrValueStart          |pStrValueEnd
        */
        paramLen = 0;
        if (pStrValueStart == PNULL)
        {
            OsaCheck(pStrValueEnd == PNULL, pStrValueStart, pStrValueEnd, quoteNum);
        }
        else    //pStrValueStart != PNULL
        {
            OsaCheck(pStrValueEnd != PNULL && pStrValueStart <= pStrValueEnd && pStrValueEnd < pAtLine->pEnd,
                     pStrValueStart, pStrValueEnd, pAtLine->pEnd);

            *(pStrValueEnd + 1) = '\0';     //string end
            paramLen = pStrValueEnd - pStrValueStart + 1;
        }

        switch (pAtPreDefInfo->pParamList[paramNum].type)
        {
            case AT_DEC_VAL:
            case AT_HEX_VAL:
            case AT_BIN_VAL:
            {
                if (quoteNum > 0)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecAtParam_num_warn_1, P_WARNING, 2,
                                "AT CMD, the %d param is numeric type, but contain: %d \", invalid", paramNum, quoteNum);

                    *pDecNextChar = pDecChar;
                    return ATC_DEC_SYNTAX_ERR;
                }

                if (pStrValueStart == PNULL)    //not input
                {
                    if (pAtPreDefInfo->pParamList[paramNum].presentType == AT_MUST_VAL)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecAtParam_num_warn_2, P_WARNING, 1,
                                    "AT CMD, the %d param is must, can't missing", paramNum);

                        *pDecNextChar = pDecChar;
                        return ATC_DEC_SYNTAX_ERR;
                    }

                    pOutParamList[paramNum].type    = pAtPreDefInfo->pParamList[paramNum].type;
                    pOutParamList[paramNum].bDefault= TRUE;
                }
                else    //input
                {
                    // check whether input value, numValue
                    /*
                     * V.250:
                     * 1> For extended AT command:
                     *   In all numeric constants, the most significant digit is specified first.
                     *    Leading "0" characters shall be ignored by the DCE;
                     *   So: AT+CEREG=05   => OK
                     * 2> For basic AT command: AT<command>[<number>]
                     *  a) <number> may be a string of one or more characters from "0" through "9" representing a decimal
                     *    integer value;
                     *  b) If a command expects <number> and it is missing (<command> is immediately followed in the command
                     *    line by another <command> or the termination character), the value "0" is assumed.
                     *   So: ATE    => ATE0
                     *  c) All leading "0"s in <number> are ignored by the DCE;
                     *   So: ATE01  => ATE1
                    */
                    if ((pAtPreDefInfo->pParamList[paramNum].type == AT_DEC_VAL &&
                         cmsDecStrToUInt(&numValue, (const UINT8 *)pStrValueStart, paramLen) == TRUE) ||
                        (pAtPreDefInfo->pParamList[paramNum].type == AT_HEX_VAL &&
                         cmsHexStrToUInt(&numValue, (const UINT8 *)pStrValueStart, paramLen) == TRUE) ||
                        (pAtPreDefInfo->pParamList[paramNum].type == AT_BIN_VAL &&
                         cmsBinStrToUInt(&numValue, (const UINT8 *)pStrValueStart, paramLen) == TRUE))
                    {
                        pOutParamList[paramNum].type    = pAtPreDefInfo->pParamList[paramNum].type;
                        pOutParamList[paramNum].bDefault        = FALSE;
                        pOutParamList[paramNum].value.numValue  = numValue;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecAtParam_num_warn_3, P_WARNING, 3,
                                    "AT CMD, the %d param is numeric type: %d, invlaid input: %d",
                                    paramNum, pAtPreDefInfo->pParamList[paramNum].type, numValue);

                        *pDecNextChar = pDecChar;
                        return ATC_DEC_SYNTAX_ERR;
                    }
                }

                break;
            }

            case AT_JSON_VAL:
            {
                //JSON format
                if (pStrValueStart == PNULL)    //no input
                {
                    if (pAtPreDefInfo->pParamList[paramNum].presentType == AT_MUST_VAL)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecAtParam_warn_json_1, P_WARNING, 1,
                                    "AT CMD, the %d param JSON is must, can't missing", paramNum);

                        *pDecNextChar = pDecChar;
                        return ATC_DEC_SYNTAX_ERR;
                    }

                    pOutParamList[paramNum].type    = pAtPreDefInfo->pParamList[paramNum].type;
                    pOutParamList[paramNum].bDefault= TRUE;
                }
                else
                {
                    pOutParamList[paramNum].inputQuote  = 0;
                    pOutParamList[paramNum].type        = pAtPreDefInfo->pParamList[paramNum].type;
                    pOutParamList[paramNum].bDefault    = FALSE;
                    pOutParamList[paramNum].value.pStr  = pStrValueStart;
                }
                break;
            }

            case AT_STR_VAL:
            {
                if (pStrValueStart == PNULL && quoteNum == 0)    //no input
                {
                    if (pAtPreDefInfo->pParamList[paramNum].presentType == AT_MUST_VAL)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecAtParam_warn_str_1, P_WARNING, 1,
                                    "AT CMD, the %d param is must, can't missing", paramNum);

                        *pDecNextChar = pDecChar;
                        return ATC_DEC_SYNTAX_ERR;
                    }

                    pOutParamList[paramNum].type    = pAtPreDefInfo->pParamList[paramNum].type;
                    pOutParamList[paramNum].bDefault= TRUE;
                }
                else
                {
                    pOutParamList[paramNum].inputQuote  = (quoteNum > 0) ? TRUE : FALSE;
                    pOutParamList[paramNum].type        = pAtPreDefInfo->pParamList[paramNum].type;
                    pOutParamList[paramNum].bDefault    = FALSE;
                    pOutParamList[paramNum].value.pStr  = pStrValueStart;   //if input: "", then: bDefault = FALSE, pStr = PNULL;
                }
                break;
            }

            case AT_LAST_MIX_STR_VAL:
            {
                OsaCheck(paramEnd == TRUE, paramEnd, pStrValueStart, pStrValueEnd);
                //LAST STR format
                if (pStrValueStart == PNULL)    //no input
                {
                    if (pAtPreDefInfo->pParamList[paramNum].presentType == AT_MUST_VAL)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecAtParam_warn_last_mix_1, P_WARNING, 1,
                                    "AT CMD, the %d param LAST MIX STR is must, can't missing", paramNum);

                        *pDecNextChar = pDecChar;
                        return ATC_DEC_SYNTAX_ERR;
                    }

                    pOutParamList[paramNum].type    = pAtPreDefInfo->pParamList[paramNum].type;
                    pOutParamList[paramNum].bDefault= TRUE;
                }
                else
                {
                    pOutParamList[paramNum].inputQuote  = 0;
                    pOutParamList[paramNum].type        = pAtPreDefInfo->pParamList[paramNum].type;
                    pOutParamList[paramNum].bDefault    = FALSE;
                    pOutParamList[paramNum].value.pStr  = pStrValueStart;
                }
                break;
            }

            default:    //mix type
            {
                if (pStrValueStart == PNULL && quoteNum == 0)    //not input
                {
                    if (pAtPreDefInfo->pParamList[paramNum].presentType == AT_MUST_VAL)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecAtParam_warn_mix_1, P_WARNING, 1,
                                    "AT CMD, the %d param is must, can't missing", paramNum);

                        *pDecNextChar = pDecChar;
                        return ATC_DEC_SYNTAX_ERR;
                    }

                    pOutParamList[paramNum].type    = pAtPreDefInfo->pParamList[paramNum].type;
                    pOutParamList[paramNum].bDefault= TRUE;
                }
                else
                {
                    if (quoteNum == 0 &&
                        cmsDecStrToUInt(&numValue, (const UINT8 *)pStrValueStart, paramLen) == TRUE)  //check whether DEC type is OK
                    {
                        pOutParamList[paramNum].type            = AT_DEC_VAL;
                        pOutParamList[paramNum].bDefault        = FALSE;
                        pOutParamList[paramNum].value.numValue  = numValue;
                    }
                    else
                    {
                        pOutParamList[paramNum].type            = AT_STR_VAL;
                        pOutParamList[paramNum].bDefault        = FALSE;
                        pOutParamList[paramNum].value.pStr      = pStrValueStart;
                        pOutParamList[paramNum].inputQuote      = (quoteNum > 0) ? TRUE : FALSE;
                    }
                }

                break;
            }
        }

        /*
         * current parameter decode done
        */
        paramNum++;

        /*
         * check whether: paramEnd, end of parameter: ";", <S3>
        */
        if (paramEnd)
        {
            break;
        }

        /*
         * decode next parameter value
        */
    }

    /*
     * if paramEnd, end of parameter: ";", <S3> - most case
    */
    if (paramEnd)
    {
        *pDecNextChar = pDecChar;
        *pOutParamNum = paramNum;

        /*
         * check any must parameters not present
        */
        if (paramNum < pAtPreDefInfo->paramMaxNum &&
            pAtPreDefInfo->pParamList[paramNum].presentType == AT_MUST_VAL)
        {
            ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecAtParam_warn_must_1, P_WARNING, 1,
                        "AT CMD, the %d param is must, can't missing", paramNum);

            return ATC_DEC_SYNTAX_ERR;
        }

        return ATC_DEC_OK;
    }

    /*
     * if (paramNum < pAtPreDefInfo->paramMaxNum), just be already returned: FAILED or AT END with: ";"/<S3>
    */

    OsaCheck(paramNum == pAtPreDefInfo->paramMaxNum && pDecChar <= pAtLine->pEnd,
             paramNum, pDecChar, pAtLine->pEnd);

    /*
     * example: AT+CEREG=5,  <S3>  - REFER is OK
     *                     ^
     *                     |pDecChar
    */
    AT_IGNORE_SPACE_CHAR(pDecChar, pAtLine->pEnd);

    if (*pDecChar == ';')
    {
        atcFoundAtCmdEnd(pAtChanEty, pDecChar, pDecNextChar);

        /*
         * *pDecNextChar point to next AT CMD
         * example: AT+CEREG=5,  ;+CEREG?
         *                        ^
         *                        |*pDecNextChar
        */
        *pOutParamNum = paramNum;

        return ATC_DEC_OK;
    }
    else if (*pDecChar == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX])
    {
        atcFoundAtCmdLineEnd(pAtChanEty, pDecChar, pDecNextChar);

        /*
         * *pDecNextChar point to next AT CMD LINE
         * example: AT+CEREG=5,  <S3>AT+CEREG?
         *                           ^
         *                           |*pDecNextChar
        */
        *pOutParamNum = paramNum;

        return ATC_DEC_OK;
    }

    *pDecNextChar = pDecChar;

    /*
     * example: AT+CEREG=5,6
     *                     ^
     *                     |pDecChar
    */
    return ATC_DEC_SYNTAX_ERR;
}



/**
  \fn           AtcDecRetCode atcProcAtCmd(AtChanEntityP pAtChanEty,
  \                                        AtCmdReqType reqType,
  \                                        const CHAR *pAtName,
  \                                        CHAR *pDecParam,
  \                                        BOOL bBasicAt,
  \                                        BOOL paramFollowed,
  \                                        CHAR **pDecNextChar)
  \brief        execute one AT CMD
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    reqType         AT request type: set/read/test
  \param[in]    pAtName         AT command name, end with '\0'
  \param[in]    pDecParam       CHAR point to start of <parameter_value>
  \param[in]    bBasicAt        whether this is a basic AT command
  \param[in]    needParam       whether <parameter_value> is needed.
  \param[out]   pDecNextChar    output, point to next decode charcater
  \returns      AtcDecRetCode
*/
static AtcDecRetCode atcProcAtCmd(AtChanEntityP pAtChanEty,
                                  AtCmdReqType reqType,
                                  const CHAR *pAtName,
                                  CHAR *pDecParam,
                                  BOOL bBasicAt,
                                  BOOL needParam,
                                  CHAR **pDecNextChar)
{
    AtCmdLineInfo       *pAtLine = &(pAtChanEty->atLineInfo);
    AtCmdPreDefInfoC    *pAtPreDefInfo = PNULL;
    CHAR    *pDecChar = pDecParam;
    AtCmdInputContext   atInputCtx;
    AtParamValue        paramList[AT_CMD_PARAM_MAX_NUM];    //32*8 = 256 bytes, still acceptable
    UINT32              paramNum = 0;
    AtcDecRetCode       atcRetCode = ATC_DEC_OK;
    CmsRetId            retCode = CMS_RET_SUCC;
    UINT32              timeOutMs = 0;

    /*
     * AT format:
     * ATCMD1 CMD2=12; +CMD1; +CMD2=,,15; +CMD2?; +CMD2=?<CR>
    */

    /*
     * Example:
     * Basic AT:
     * 1. ATE0
     *       ^
     *       |pDecParam, AT_SET_REQ, needParam = TRUE
     * 2. AT
     *       ^
     *       |pDecParam, AT_EXEC_REQ, needParam = FALSE
     * 3. ATI<S3>
     *       ^
     *       |pDecParam, AT_EXEC_REQ, needParam = FALSE
     * 4. ATS3?
     *         ^
     *         |pDecParam, AT_READ_REQ, needParam = FALSE
     * 5. ATS3=13
     *         ^
     *         |pDecParam, AT_SET_REQ, needParam = TRUE
     * 6. AT&<command>[number]
     *                 ^
     *                 |pDecParam, AT_SET_REQ, needParam = TRUE
     * 7. AT&<command>=?    //test
     *                  ^
     *                  |pDecParam, AT_TEST_REQ, needParam = FALSE
     * 8. AT&<command>?     //read
     *                 ^
     *                 |pDecParam, AT_READ_REQ, needParam = FALSE
     * 9. AT&<command>=[param]
     *                  ^
     *                  |pDecParam, AT_BASIC_EXT_SET_REQ, needParam = TRUE
     *
     * Extened AT:
     * 1. AT+CMD1
     *           ^
     *           |pDecParam, AT_EXEC_REQ, needParam = FALSE
     * 2. AT+CMD1?  <S3>
     *            ^
     *            |pDecParam, AT_READ_REQ, needParam = FALSE
     * 3. AT+CMD1=?  <S3>
     *             ^
     *             |pDecParam, AT_TEST_REQ, needParam = FALSE
     * 4. AT+CMD1=<param1>
     *             ^
     *             |pDecParam, AT_SET_REQ, needParam = TRUE
    */

    OsaCheck(pDecChar != PNULL && pDecChar > pAtLine->pNextHdr && pDecChar <= pAtLine->pEnd && reqType != AT_INVALID_REQ_TYPE,
             pDecChar, pAtLine->pNextHdr, pAtLine->pEnd);

    // check format
    if (needParam == FALSE)
    {
        /* ATS? / AT+CMD1? / AT+CMD2=? */

        if (bBasicAt)
        {
            /*
             * BASIC AT
             *
             * Extended syntax commands may appear on the same command line after a basic syntax command without a separator ";"
             * example: ATS3?
             *               ^
             *               |pDecChar
            */
            if (*pDecChar == ' ')   //at end
            {
                atcFoundAtCmdEnd(pAtChanEty, pDecChar, pDecNextChar);
                pDecChar = *pDecNextChar;
            }
            else if (*pDecChar == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX])
            {
                atcFoundAtCmdLineEnd(pAtChanEty, pDecChar, pDecNextChar);
                pDecChar = *pDecNextChar;
            }
            else
            {
                OsaDebugBegin(FALSE, pAtName[0], pAtName[1], *pDecChar);
                OsaDebugEnd();

                *pDecNextChar = pDecChar;

                return ATC_DEC_SYNTAX_ERR;
            }
        }
        else
        {
            /*
             * Extended AT
             * Additional commands may follow an extended syntax command on the same command line if a
             *  semicolon (";", IA5 3/11) is inserted after the preceding extended command as a separator;
             * example: AT+CEREG?;+COPS?<S3>  ==> OK
             *                   ^
             *                   |pDecChar
            */
            AT_IGNORE_SPACE_CHAR(pDecChar, pAtLine->pEnd);

            if (*pDecChar == ';')
            {
                atcFoundAtCmdEnd(pAtChanEty, pDecChar, pDecNextChar);
                pDecChar = *pDecNextChar;
            }
            else if (*pDecChar == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX])
            {
                atcFoundAtCmdLineEnd(pAtChanEty, pDecChar, pDecNextChar);
                pDecChar = *pDecNextChar;
            }
            else
            {
                OsaDebugBegin(FALSE, pAtName[0], pAtName[1], *pDecChar);
                OsaDebugEnd();

                *pDecNextChar = pDecChar;

                return ATC_DEC_SYNTAX_ERR;
            }
        }
    }

    pAtPreDefInfo = atcFindPreDefinedAtInfo(pAtChanEty, pAtName, bBasicAt);

    if (pAtPreDefInfo == PNULL || pAtPreDefInfo->atProcFunc == PNULL)
    {
        ECOMM_STRING(UNILOG_ATCMD_PARSER, atcProcAtCmd_check_1, P_WARNING,
                     "AT CMD, can't find handler/procFunc for AT: %s", (UINT8 *)pAtName);

        *pDecNextChar = pDecChar;
        return ATC_DEC_SYNTAX_ERR;
    }

    /*
     * basic check: extended parameter command/action command;
     * "action commands" maybe: "executed" or "tested"; not read command
    */
    if (pAtPreDefInfo->cmdType == AT_EXT_ACT_CMD && reqType == AT_READ_REQ)
    {
        ECOMM_STRING(UNILOG_ATCMD_PARSER, atcProcAtCmd_check_2, P_WARNING,
                     "AT CMD, AT: %s, is action command, not support READ operation", (UINT8 *)pAtName);

        *pDecNextChar = pDecChar;
        return ATC_DEC_SYNTAX_ERR;
    }


    memset(paramList, 0x00, sizeof(paramList));
    for (paramNum = 0; paramNum < AT_CMD_PARAM_MAX_NUM; paramNum++)
    {
        paramList[paramNum].bDefault = TRUE;
        if (paramNum < pAtPreDefInfo->paramMaxNum)
        {
            paramList[paramNum].type = pAtPreDefInfo->pParamList[paramNum].type;
        }
    }

    paramNum = 0;

    if (needParam) //set/exec
    {
        // need to decode the parameters;
        OsaCheck(reqType == AT_SET_REQ || reqType == AT_BASIC_EXT_SET_REQ, reqType, pAtName[1], pAtName[2]);

        atcRetCode = atcDecAtParam(pAtChanEty,
                                   pAtPreDefInfo,
                                   pDecChar,
                                   bBasicAt,
                                   paramList,
                                   &paramNum,
                                   pDecNextChar);
        pDecChar = *pDecNextChar;
    }

    /*
     * call the FUNC to process it
    */
    if (atcRetCode != ATC_DEC_OK)
    {
        ECOMM_STRING(UNILOG_ATCMD_PARSER, atcProcAtCmd_param_warn_1, P_WARNING,
                     "AT CMD: %s, decode param_value failed", (UINT8 *)pAtName);

        *pDecNextChar = pDecChar;
        return atcRetCode;
    }

    /*
     * !!!!! before call procFunc, should be point to next AT CMD !!!!
     * 1> Current AT command decode should be OK, so "pNextHdr" should be point to next AT command;
     * 2> In the "procFunc" API, the AT response maybe sent, and in the send function, need to check whether current AT line is done by
     *    atcBePreAtLineDone();
    */
    OsaCheck(pAtLine->pNextHdr == PNULL || pAtLine->startAt == TRUE, pAtLine->pNextHdr, pAtLine->startAt, pAtLine->startLine);


    memset(&atInputCtx, 0x00, sizeof(AtCmdInputContext));
    atInputCtx.operaType    = reqType;
    atInputCtx.chanId       = pAtChanEty->chanId;
    atInputCtx.paramMaxNum  = pAtPreDefInfo->paramMaxNum;
    atInputCtx.paramRealNum = paramNum;
    atInputCtx.pParamList   = paramList;

    /* before call the procFunc, start the guard timer */
    if (pAtChanEty->bApiMode)
    {
        if (pAtLine->timeOutMs > 0 && pAtLine->timeOutMs < CMS_MAX_DELAY_MS)
        {
            timeOutMs = pAtLine->timeOutMs;
        }
        else
        {
            ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcProcAtCmd_warning_no_timeout_1, P_WARNING, 1,
                        "AT CMD, RIL AT, no valid time out set, use the AT CMD default value: %d seconds", pAtPreDefInfo->timeOutS);
            timeOutMs = pAtPreDefInfo->timeOutS * 1000;
        }
    }
    else
    {
        timeOutMs = pAtPreDefInfo->timeOutS * 1000;
    }

    if (timeOutMs == 0)
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcProcAtCmd_warning_no_timeout_2, P_WARNING, 1,
                    "AT CMD, no time out set, use the default value: %d seconds", AT_DEFAULT_TIMEOUT_SEC);
        timeOutMs = AT_DEFAULT_TIMEOUT_SEC * 1000;
    }

    atInputCtx.tid          = atcStartAsynTimer(pAtChanEty, timeOutMs);

    /*
     * !!!!! AT FUNC !!!!!!
    */
    retCode = pAtPreDefInfo->atProcFunc(&atInputCtx);

    if (retCode != CMS_RET_SUCC)
    {
        /* proc failed, need to stop the guard timer */
        ECOMM_STRING(UNILOG_ATCMD_PARSER, atcProcAtCmd_proc_warn_1, P_WARNING,
                     "AT CMD: %s, funcProc failed", (UINT8 *)pAtName);

        atcStopAsynTimer(pAtChanEty, atInputCtx.tid);

        *pDecNextChar = pDecChar;
        return ATC_DEC_PROC_ERR;
    }

    /*
     * else, let procFunc to stop the guard timer, when AT done
    */

    return ATC_DEC_OK;
}


/**
  \fn           AtcDecRetCode atcDecBasicAtCmd(AtChanEntityP pAtChanEty, CHAR *pAtName, CHAR **pNextChar)
  \brief        decode one basic AT command
  \param[in]    pAtChanEty          AT channel entity
  \param[in]    pDecAtName          CHAR point to start of <name>
  \param[out]   pDecNextChar        output, point to next decode charcater
  \returns      AtcDecRetCode
*/
static AtcDecRetCode atcDecBasicAtCmd(AtChanEntityP pAtChanEty, CHAR *pDecAtName, CHAR **pDecNextChar)
{
    AtCmdLineInfo   *pAtLine = &(pAtChanEty->atLineInfo);
    CHAR    atName[AT_CMD_MAX_NAME_LEN+1] = {0};
    CHAR    *pDecChar = pDecAtName;
    UINT32  atNameLen = 0;

    AtCmdReqType    reqType = AT_INVALID_REQ_TYPE;
    BOOL            needParam = FALSE;

    /*
     * Basic AT command format:
     * 1> AT<command>[<number>]
     *      ^
     *      |pAtName
     * 2> ATS<parameter_number>?
     *      ^
     *      |pAtName
     * 3> ATS<parameter_number>=[<value>]
     *      ^
     *      |pAtName
     * 4> AT&<command>[number]
     *      ^
     *      |pAtName
     * 5> AT&<command>=?    //test
     *      ^
     *      |pAtName
     * 6> AT&<command>?     //read
     *      ^
     *      |pAtName
    */

    /*
     * ATE1 Q0
     *      ^
     *      |pDecAtName = pAtLine->pNextHdr
    */
    OsaCheck(pDecAtName != PNULL && pDecAtName >= pAtLine->pNextHdr && pDecAtName <= pAtLine->pEnd && pDecNextChar!= PNULL,
             pDecAtName, pAtLine->pNextHdr, pAtLine->pEnd);

    if (pDecChar == pAtLine->pEnd ||
        (*pDecChar) == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX])  //AT only
    {
        atName[0]   = ' ';
        atNameLen   = 1;

        reqType     = AT_EXEC_REQ;
    }
    else if (*pDecChar == ' ')
    {
        /*
         * str: AT   <S3> - OK
        */
        AT_IGNORE_SPACE_CHAR(pDecChar, pAtLine->pEnd);
        if (*pDecChar == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX])
        {
            atName[0]   = ' ';
            atNameLen   = 1;
            reqType     = AT_EXEC_REQ;
        }
    }
    else if (*pDecChar == 'S' || *pDecChar == 's')
    {
        /* ATS<parameter_number>?/ATS<parameter_number>=[<value>] */
        atName[atNameLen++] = 'S';

        for (pDecChar += 1; pDecChar < pAtLine->pEnd; pDecChar++)
        {
            /*<parameter_number>*/
            if (*pDecChar >= '0' && *pDecChar <= '9')
            {
                atName[atNameLen++] = *pDecChar;

                if (atNameLen > AT_CMD_MAX_NAME_LEN)
                {
                    break;
                }
            }
            else if (*pDecChar == '?')
            {
                reqType = AT_READ_REQ;
                pDecChar++;
                break;
            }
            else if (*pDecChar == '=')
            {
                reqType = AT_SET_REQ;
                needParam = TRUE;
                pDecChar++;
                break;
            }
            else
            {
                atNameLen = AT_CMD_MAX_NAME_LEN+1;  // invalid basic AT
                break;
            }
        }
    }
    else
    {
        /*
         * AT<command>[<number>]
         * AT&<command>[number]
        */
        for (; pDecChar <= pAtLine->pEnd; pDecChar++)
        {
            if ((*pDecChar >= 'A' && *pDecChar <= 'Z') ||
                (*pDecChar >= 'a' && *pDecChar <= 'z') ||
                (*pDecChar == '&'))
            {
                atName[atNameLen++] = *pDecChar;

                if (atNameLen > AT_CMD_MAX_NAME_LEN)
                {
                    break;
                }
            }
            else if (*pDecChar >= '0' && *pDecChar <= '9')
            {
                if (atNameLen == 0) //AT0 - invalid
                {
                    atNameLen = AT_CMD_MAX_NAME_LEN+1;
                    break;
                }
                else
                {
                    needParam = TRUE;
                    reqType = AT_SET_REQ;
                    break;
                }
            }
            else if (((*pDecChar) == ' ') ||
                     ((*pDecChar) == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX]))
            {
                /*
                 * AT<command>[<number>], number is not input
                 * example: ATI <S3>
                 *             ^
                 *             |
                */
                reqType     = AT_EXEC_REQ;
                break;
            }
            else if ((*pDecChar) == '?')
            {
                /*
                 * AT&<command>?     //read
                 *             ^
                 *             |
                */
                if (atNameLen == 0) //AT? - invalid
                {
                    atNameLen = AT_CMD_MAX_NAME_LEN+1;
                    break;
                }
                else
                {
                    reqType = AT_READ_REQ;
                    pDecChar++;
                    break;
                }
            }
            else if ((*pDecChar) == '=')
            {
                /*
                 * AT&<command>=?       //test
                 *             ^
                 *             |
                */
                if (atNameLen == 0) //AT=? - invalid
                {
                    atNameLen = AT_CMD_MAX_NAME_LEN+1;
                    break;
                }
                else
                {
                    pDecChar++;
                    if ((*pDecChar) == '?')
                    {
                        reqType = AT_TEST_REQ;
                        pDecChar++;
                    }
                    else /*if (*pDecChar >= '0' && *pDecChar <= '9')*/
                    {
                        /*
                         * AT&<command>=0
                         *              ^
                         * Some customer need to support such type of input and act as: AT&<command>0.
                         * In fact, this is not a standard input. So here, we involve a new type: AT_BASIC_EXT_SET_REQ (Basic AT extended set request).
                        */
                        needParam   = TRUE;
                        reqType     = AT_BASIC_EXT_SET_REQ;
                    }
                    #if 0
                    else
                    {
                        /*
                         * AT&<command>= ?
                         *              ^ - invalid
                        */
                        atNameLen = AT_CMD_MAX_NAME_LEN+1;
                    }
                    #endif
                    break;
                }
            }
            else
            {
                atNameLen = AT_CMD_MAX_NAME_LEN+1;  // invalid basic AT
                break;
            }
        }
    }

    if (reqType == AT_INVALID_REQ_TYPE || atNameLen > AT_CMD_MAX_NAME_LEN)
    {
        OsaDebugBegin(FALSE, atName[0], reqType, *pDecChar);
        OsaDebugEnd();

        *pDecNextChar = pDecChar;

        return ATC_DEC_SYNTAX_ERR;
    }

    return atcProcAtCmd(pAtChanEty,
                        reqType,
                        atName,
                        pDecChar,
                        TRUE,
                        needParam,
                        pDecNextChar);
}


/**
  \fn           CmsRetId atcDecExtendedAtCmd(AtChanEntityP pAtChanEty, CHAR *pAtName)
  \brief        decode one basic AT command
  \param[in]    pAtChanEty          AT channel entity
  \param[in]    pDecAtName          CHAR point to start of <name>
  \param[out]   pDecNextChar        output, point to next decode charcater
  \returns      AtcDecRetCode
*/
static AtcDecRetCode atcDecExtendedAtCmd(AtChanEntityP pAtChanEty, CHAR *pDecAtName, CHAR **pDecNextChar)
{
    AtCmdLineInfo   *pAtLine = &(pAtChanEty->atLineInfo);
    CHAR    atName[AT_CMD_MAX_NAME_LEN+1] = {0};
    CHAR    *pDecChar = pDecAtName;
    UINT32  atNameLen = 0;

    AtCmdReqType    reqType     = AT_INVALID_REQ_TYPE;
    BOOL            needParam   = FALSE;

    /*
     * AT+CEREG=5;+CEREG?
     *            ^
     *            |pDecAtName = pAtLine->pNextHdr
    */
    OsaCheck(pDecAtName != PNULL && pDecAtName >= pAtLine->pNextHdr && pDecAtName < pAtLine->pEnd && pDecNextChar!= PNULL,
             pDecAtName, pAtLine->pNextHdr, pAtLine->pEnd);

    /*
     * AT+CMD1
     *   ^
     *   |pDecChar
     * AT+CMD1=<param1>[...]
     *   ^
     *   |pDecChar
     * AT+CMD1?
     *   ^
     *   |pDecChar
     * AT+CMD1=?
     *   ^
     *   |pDecChar
    */
    atName[atNameLen++] = *pDecChar;    /* '+'/'*'/'^' */
    pDecChar++;
    for (; pDecChar <= pAtLine->pEnd; pDecChar++)
    {
        AT_IGNORE_SPACE_CHAR(pDecChar, pAtLine->pEnd);

        if (*pDecChar == '=')
        {
            /*
             * AT+CMD=?         //test
             * or
             * AT+CMD=<param>   //set
            */
            pDecChar++;

            if (*pDecChar == '?')
            {
                reqType = AT_TEST_REQ;
                pDecChar++;     /* this '?' don't need to re-decode */
                break;
            }
            else
            {
                reqType     = AT_SET_REQ;
                needParam   = TRUE;
                break;
            }
        }
        else if (*pDecChar == '?')
        {
            /* AT+CMD?  - READ */
            reqType = AT_READ_REQ;
            pDecChar++;     /* this '?' don't need to re-decode */
            break;
        }
        else if (*pDecChar == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] ||
                 *pDecChar == ';')
        {
            /*
             * AT+CMD; / AT+CMD<S3>
             *      ^           ^
            */
            reqType = AT_EXEC_REQ;
            /* here: don't need: pDecChar++, let next func: atcProcAtCmd() to decode this ';'/<S3>  */
            break;

        }
        else if ((*pDecChar >= 'a' && *pDecChar <= 'z') ||
                 (*pDecChar >= 'A' && *pDecChar <= 'Z') ||
                 (*pDecChar >= '0' && *pDecChar <= '9') ||
                 (*pDecChar == '!') || (*pDecChar == '%') || (*pDecChar == '-') ||
                 (*pDecChar == '.') || (*pDecChar == '/') || (*pDecChar == ':') ||
                 (*pDecChar == '_')) //valid name
        {
            /*
             * Following the "+", from one to sixteen (16) additional characters appear in the command name.
             * These characters shall be selected from the following set:
             *  A through Z (IA5 4/1 through 5/10)
             *  0 through 9 (IA5 3/0 through 3/9)
             *  ! (IA5 2/1)
             *  % (IA5 2/5)
             *  ? (IA5 2/13)
             *  . (IA5 2/14)
             *  ?? (IA5 2/15)
             *  : (IA5 3/10)
             *  _ (IA5 5/15)
            */
            atName[atNameLen++] = *pDecChar;

            if (atNameLen > AT_CMD_MAX_NAME_LEN)
            {
                break;
            }
        }
        else    //invalid AT CHAR
        {
            break;
        }
    }

    if (reqType == AT_INVALID_REQ_TYPE || atNameLen <= 1 || atNameLen > AT_CMD_MAX_NAME_LEN)
    {
        /* AT+=<param>  - NOK */
        OsaDebugBegin(FALSE, atName[1], atName[2], *pDecChar);
        OsaDebugEnd();

        *pDecNextChar = pDecChar;

        return ATC_DEC_SYNTAX_ERR;
    }

    return atcProcAtCmd(pAtChanEty,
                        reqType,
                        atName,
                        pDecChar,
                        FALSE,
                        needParam,
                        pDecNextChar);
}


/**
  \fn           static void atcGetApiAtLine(AtChanEntity *pAtChanEty)
  \brief        Get AT line info from AT API input node
  \param[in]
  \returns      void
*/
static void atcGetApiAtLine(AtChanEntity *pAtChanEty)
{
    AtCmdLineInfo   *pAtLine    = &(pAtChanEty->atLineInfo);
    AtInputInfo     *pAtInput   = &(pAtChanEty->atInputInfo);
    AtApiInputNode  *pAtApiHdr  = pAtInput->input.apiInput.pHdr;
    CHAR    *pStart = PNULL, *pEnd = PNULL;

    /* line should be empty */
    OsaCheck(pAtLine->pLine == PNULL && pAtChanEty->bApiMode, pAtLine->pLine, pAtLine->startAt, pAtLine->pNextHdr);

    /*
     * reset the AT channel response API
    */
    pAtChanEty->callBack.respFunc   = PNULL;
    pAtChanEty->callBack.pArg       = PNULL;
    pAtChanEty->callBack.apiSem     = PNULL;

    if (pAtApiHdr == PNULL)
    {
        OsaCheck(pAtInput->input.apiInput.pTailer == PNULL && pAtInput->pendingNodeNum == 0 && pAtInput->pendingLen == 0,
                 pAtInput->input.apiInput.pTailer, pAtInput->pendingNodeNum, pAtInput->pendingLen);

        return; //no AT pending
    }

    /*
     * AT should end with <S3>
    */
    OsaCheck(pAtApiHdr->respFunc != PNULL && pAtApiHdr->sem != PNULL, pAtApiHdr->respFunc, pAtApiHdr->sem, 0);

    if ((*(pAtApiHdr->pEnd)) == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX])
    {
        pEnd = pAtApiHdr->pEnd;
    }
    else if ((*(pAtApiHdr->pEnd - 1)) == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX])
    {
        pEnd = pAtApiHdr->pEnd - 1;
    }
    else
    {
        OsaCheck(FALSE, (*(pAtApiHdr->pEnd)), (*(pAtApiHdr->pEnd - 1)), pAtApiHdr->pEnd - pAtApiHdr->pStart + 1);
    }


    /*
     * set Line info, find the start & end
    */
    pStart = pAtApiHdr->pStart;

    AT_IGNORE_SPACE_S3_S4_CHAR(pStart,
                               pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX],
                               pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX],
                               pEnd);

    OsaCheck(pStart < pEnd, pStart, pEnd, pAtApiHdr->pStart);


    /* set line info */
    atcInitSetAtCmdLineInfo(pAtLine, pAtApiHdr->pStart, pStart, pEnd, pAtApiHdr->timeOutMs);

    /*
     * set the respone API, and ARG
    */
    pAtChanEty->callBack.respFunc   = pAtApiHdr->respFunc;
    pAtChanEty->callBack.pArg       = pAtApiHdr->pArg;
    pAtChanEty->callBack.apiSem     = pAtApiHdr->sem;


    //free header node, AT command sting in "pAtLine", when AT decode done, not here
    atcFreeAtApiInputNodeHeader(pAtInput);

    /*
     * Done
    */
    return;
}


/**
  \fn           void atcGetAtLine(AtChanEntity *pAtChanEty)
  \brief        Get AT line info from AT CMD node
  \param[in]
  \returns      void
*/
static void atcGetAtLine(AtChanEntity *pAtChanEty)
{
    AtCmdLineInfo   *pAtLine = &(pAtChanEty->atLineInfo);
    AtInputInfo     *pAtInfo = &(pAtChanEty->atInputInfo);
    AtCmdInputNode  *pInputNode = PNULL, *pTermCharNode = PNULL;
    BOOL            termEnding = TRUE; // whether input string ending with "termination char": <S3>
    CHAR            *pTerm = PNULL;     //<S3> "termination char"
    CHAR            *pStart = PNULL;    //comand start char
    UINT16          lineLen = 0;
    CHAR            *pLine = PNULL, *pLineOffset = PNULL, *pLineEnd = PNULL;   /*store at command line*/

    /* line should be empty */
    OsaCheck(pAtLine->pLine == PNULL, pAtLine->pLine, pAtLine->startAt, pAtLine->pNextHdr);

    if (pAtChanEty->bApiMode)
    {
        atcGetApiAtLine(pAtChanEty);
        return;
    }

    if (pAtInfo->input.cmdInput.pHdr == PNULL)
    {
        OsaCheck(pAtInfo->input.cmdInput.pTailer == PNULL && pAtInfo->pendingNodeNum == 0 && pAtInfo->pendingLen == 0,
                 pAtInfo->input.cmdInput.pTailer, pAtInfo->pendingNodeNum, pAtInfo->pendingLen);

        return; //no AT pending
    }

    pInputNode = pAtInfo->input.cmdInput.pHdr;

    /*
     * find the termination char: <S3>
    */
    while (TRUE)
    {
        termEnding = TRUE;

        if (pInputNode == PNULL)
        {
            break;
        }

        /* find the valid start firstly */
        if (pStart == PNULL)
        {
            for (; pInputNode->pNextDec <= pInputNode->pEnd; pInputNode->pNextDec++)
            {
                if (AT_BE_VALID_CHAR(*(pInputNode->pNextDec)) == TRUE &&
                    *(pInputNode->pNextDec) != ' ' &&
                    *(pInputNode->pNextDec) != pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] &&
                    *(pInputNode->pNextDec) != pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX])
                {
                    pStart = pInputNode->pNextDec;
                    break;
                }
            }
        }

        if (pStart == PNULL)    //no valid char in header node
        {
            OsaCheck(pInputNode == pAtInfo->input.cmdInput.pHdr, pInputNode, pAtInfo->input.cmdInput.pHdr, pInputNode->pNextDec);

            atcFreeAtInputNodeHeader(pAtInfo, TRUE);
            pInputNode = pAtInfo->input.cmdInput.pHdr;
            continue;
        }


        /*
         * find the termination character: <S3>, Start from end
        */
        for (pTerm = pInputNode->pEnd; (UINT32)pTerm >= (UINT32)(pInputNode->pNextDec); pTerm--)
        {
            if (*pTerm == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX])
            {
                //termFound = TRUE;
                break;
            }

            if (*pTerm != ' ' && *pTerm != pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX])   //<SP> & S4(\n) should ignored
            {
                termEnding = FALSE;
            }
        }

        /* <S3>XXXXXX */
        if ((UINT32)pTerm == (UINT32)(pInputNode->pNextDec))
        {
            /* <S3> only*/
            if (termEnding == TRUE)
            {
                if (pInputNode == pAtInfo->input.cmdInput.pHdr)    //just means, header contain only <S3>, just ignore
                {
                    atcFreeAtInputNodeHeader(pAtInfo, TRUE);

                    pInputNode = pAtInfo->input.cmdInput.pHdr;
                    //continue;
                }
                else
                {
                    //not the header, this node should not decoded before
                    OsaCheck(pInputNode->pStart == pInputNode->pNextDec && pInputNode->pEnd >= pInputNode->pStart,
                             pInputNode->pStart, pInputNode->pNextDec, pInputNode->pEnd);

                    OsaCheck(lineLen > 0, lineLen, 0, 0);
                    lineLen += 1;   //inculding this <S3>
                    pTermCharNode = pInputNode;
                    break;
                }
            }
            else // <S3>XXXXX
            {
                if (pInputNode == pAtInfo->input.cmdInput.pHdr)    //just means, header start with <S3>
                {
                    // skip this <S3>
                    AT_IGNORE_SPACE_S3_S4_CHAR(pInputNode->pNextDec,
                                               pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX],
                                               pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX],
                                               pInputNode->pEnd);

                    OsaCheck(pInputNode->pEnd >= pInputNode->pNextDec, pInputNode->pStart, pInputNode->pEnd, pInputNode->pNextDec);

                    lineLen += (pInputNode->pEnd - pInputNode->pNextDec + 1);
                    pInputNode = pInputNode->pNextNode;
                    //continue;
                }
                else
                {
                    OsaCheck(lineLen > 0, lineLen, 0, 0);
                    lineLen += 1;   //inculding this <S3>
                    pTermCharNode = pInputNode;
                    break;
                }
            }
        }
        else if ((UINT32)pTerm > (UINT32)(pInputNode->pNextDec))
        {
            /*
             * XXXX<S3>XXXX
             * XXXX<S3>     -- most case
            */
            lineLen += (pTerm - pInputNode->pNextDec + 1);
            pTermCharNode = pInputNode;
            break;
        }
        else    //not found
        {
            OsaCheck(pInputNode->pEnd >= pInputNode->pNextDec, pInputNode->pStart, pInputNode->pEnd, pInputNode->pNextDec);
            lineLen += (pInputNode->pEnd - pInputNode->pNextDec + 1);

            pInputNode = pInputNode->pNextNode;
        }
    }

    if (pAtInfo->input.cmdInput.pHdr == PNULL)
    {
        OsaCheck(pTermCharNode == PNULL, pTermCharNode, lineLen, 0);
        pAtInfo->input.cmdInput.pTailer    = PNULL;
        pAtInfo->pendingLen = 0;
        pAtInfo->pendingNodeNum = 0;

        return;
    }

    if (pTermCharNode == PNULL)
    {
        // no <S3> found, just pending
        return;
    }

    OsaCheck(lineLen > 0, lineLen, AT_CMD_MAX_PENDING_STR_LEN, 0);

    /*
     * If one AT line > AT_CMD_MAX_PENDING_STR_LEN, don't parse/decode it, maybe AT CMD not right
    */
    if (lineLen > AT_CMD_MAX_PENDING_STR_LEN)
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcGetAtLine_w_1, P_WARNING, 2,
                    "AT CMD, input AT CMD too long: %d > %d, not support, discard it",
                    lineLen, AT_CMD_MAX_PENDING_STR_LEN);

        /* will free/discard this AT in caller: atcDecOneNode() */

        return;
    }


    /*
     * check whether need to allocate addicational buffer to store the AT command line string
    */
    if (pTermCharNode == pAtInfo->input.cmdInput.pHdr &&
        termEnding == TRUE)
    {
        /*
         * most case: one AT node, contain one line, don't need to allocate new memory
         * XXXXXXX<S3>[<S4><SP><SP>...]
        */
        OsaCheck(pTermCharNode->pStart <= pTermCharNode->pNextDec && pTerm > pTermCharNode->pNextDec && pTerm <= pTermCharNode->pEnd,
                 pTerm, pTermCharNode->pNextDec, pTermCharNode->pEnd);

        atcInitSetAtCmdLineInfo(pAtLine, pTermCharNode->pStart, pTermCharNode->pNextDec, pTerm, 0);

        //free header node
        atcFreeAtInputNodeHeader(pAtInfo, FALSE);   //not need to free the AT string

    }
    else
    {
        /*
         * (pTermCharNode != pAtInfo->input.cmdInput.pHdr || termEnding != TRUE)
         * AT command line, not in one input node, need a copy here
         * example:
         * AT\r         => lineLen = 3
         * ^  ^
         *    |pLineEnd
        */
        pLine = (CHAR *)OsaAllocMemory(lineLen + 1);  /* include '\0'*/
        pLineOffset = pLine;
        pLineEnd    = pLine + lineLen - 1;

        while (pAtInfo->input.cmdInput.pHdr != pTermCharNode)
        {
            OsaCheck(pAtInfo->input.cmdInput.pHdr != PNULL, lineLen, (pLineOffset - pLine), 0);

            if (pLine != PNULL)
            {
                OsaCheck((pLineEnd - pLineOffset + 1) > (pAtInfo->input.cmdInput.pHdr->pEnd - pAtInfo->input.cmdInput.pHdr->pNextDec + 1),
                         (pLineEnd - pLineOffset + 1), (pAtInfo->input.cmdInput.pHdr->pEnd - pAtInfo->input.cmdInput.pHdr->pNextDec + 1), lineLen);

                memcpy(pLineOffset, pAtInfo->input.cmdInput.pHdr->pNextDec, (pAtInfo->input.cmdInput.pHdr->pEnd - pAtInfo->input.cmdInput.pHdr->pNextDec + 1));

                pLineOffset += (pAtInfo->input.cmdInput.pHdr->pEnd - pAtInfo->input.cmdInput.pHdr->pNextDec + 1);
            }

            //free header node, and move to next node
            atcFreeAtInputNodeHeader(pAtInfo, TRUE);
        }

        //(pAtInfo->input.cmdInput.pHdr == pTermCharNode)
        OsaCheck((pTermCharNode->pNextDec >= pTermCharNode->pStart) && (pTerm >= pTermCharNode->pNextDec) && (pTerm <= pTermCharNode->pEnd),
                 pTerm, pTermCharNode->pNextDec, pTermCharNode->pEnd);

        /*
         * XXXXX<S3>XXXX
         *       ^pTerm
        */
        if (pLine != PNULL)
        {
            OsaCheck((pLineEnd - pLineOffset + 1) >= (pTerm - pAtInfo->input.cmdInput.pHdr->pNextDec + 1),
                     (pLineEnd - pLineOffset + 1), (pTerm - pAtInfo->input.cmdInput.pHdr->pNextDec + 1), lineLen);

            memcpy(pLineOffset, pAtInfo->input.cmdInput.pHdr->pNextDec, (pTerm - pAtInfo->input.cmdInput.pHdr->pNextDec + 1));

            pLineOffset += (pTerm - pAtInfo->input.cmdInput.pHdr->pNextDec + 1);

            *pLineOffset = '\0';    //ending; pLineEnd point to '\r'

            OsaCheck(pLineEnd+1 == pLineOffset, pLineEnd, pLineOffset, pLine);
        }

        if (termEnding == TRUE)
        {
            /*XXXX<S3>*/
            //free header node
            atcFreeAtInputNodeHeader(pAtInfo, TRUE);
        }
        else
        {
            /*XXXX<S3>XXXX*/
            OsaCheck(pAtInfo->input.cmdInput.pHdr->pNextDec >= pAtInfo->input.cmdInput.pHdr->pStart &&
                     pAtInfo->pendingNodeNum > 0 &&
                     pAtInfo->pendingLen > (pTerm - pAtInfo->input.cmdInput.pHdr->pNextDec + 1),
                     pAtInfo->pendingNodeNum, pAtInfo->pendingLen, (pTerm - pAtInfo->input.cmdInput.pHdr->pNextDec + 1));

            // move to next AT header
            pAtInfo->input.cmdInput.pHdr->pNextDec = pTerm;

            // skip this <S3>
            AT_IGNORE_SPACE_S3_S4_CHAR(pAtInfo->input.cmdInput.pHdr->pNextDec,
                                       pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX],
                                       pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX],
                                       pAtInfo->input.cmdInput.pHdr->pEnd);

            OsaCheck(pAtInfo->input.cmdInput.pHdr->pNextDec > pAtInfo->input.cmdInput.pHdr->pStart &&
                     pAtInfo->input.cmdInput.pHdr->pNextDec <= pAtInfo->input.cmdInput.pHdr->pEnd &&
                     pAtInfo->pendingNodeNum > 0 &&
                     pAtInfo->pendingLen > (pAtInfo->input.cmdInput.pHdr->pNextDec - pAtInfo->input.cmdInput.pHdr->pStart),
                     pAtInfo->input.cmdInput.pHdr->pNextDec, pAtInfo->input.cmdInput.pHdr->pEnd, (pAtInfo->input.cmdInput.pHdr->pNextDec - pAtInfo->input.cmdInput.pHdr->pStart));

            //pAtInfo->pendingLen -= (pAtInfo->input.cmdInput.pHdr->pNextDec - pAtInfo->input.cmdInput.pHdr->pStart + 1);
            /* "pAtInfo->input.cmdInput.pHdr->pNextDec" is the next header, not decoded this time, current decoded len should be:  */
            pAtInfo->pendingLen -= (pAtInfo->input.cmdInput.pHdr->pNextDec - pAtInfo->input.cmdInput.pHdr->pStart);
        }

        if (pLine != PNULL)
        {
            atcInitSetAtCmdLineInfo(pAtLine, pLine, pLine, pLineEnd, 0);
        }
        else
        {
            //memory allocated failed
            OsaDebugBegin(FALSE, lineLen, 0, 0);
            OsaDebugEnd();

            /*
             * here, need to send a signal to let AT decoder to decode next AT CMD
             * - TBD
            */
        }
    }

    /*
     * DONE
    */
    return;
}



/**
  \fn           void atcDiscardAllPendingAtNode(AtChanEntity *pAtChanEty)
  \brief
  \param[in]
  \returns      void
*/
static void atcDiscardAllPendingAtNode(AtChanEntity *pAtChanEty)
{
    AtInputInfo     *pAtInfo = &(pAtChanEty->atInputInfo);
    AtCmdInputNode  *pFreeNode = PNULL;

    while (pAtInfo->input.cmdInput.pHdr != PNULL)
    {
        pFreeNode = pAtInfo->input.cmdInput.pHdr;
        pAtInfo->input.cmdInput.pHdr = pAtInfo->input.cmdInput.pHdr->pNextNode;

        /*Free string memory*/
        if (pFreeNode->pStart != PNULL)
        {
            OsaFreeMemory(&(pFreeNode->pStart));
        }
        else
        {
            OsaDebugBegin(FALSE, pFreeNode->pEnd, pFreeNode->pNextDec, 0);
            OsaDebugEnd();
        }

        /*Free node memory*/
        OsaFreeMemory(&(pFreeNode));
    }

    pAtInfo->input.cmdInput.pHdr = pAtInfo->input.cmdInput.pTailer = PNULL;
    pAtInfo->pendingLen     = 0;
    pAtInfo->pendingNodeNum = 0;

    return;
}

/**
  \fn           AtcDecRetCode atcDecOneAt(AtChanEntityP pAtChanEty)
  \brief        Decode one AT command in current line. Just from start of one AT, example:
  \             1> AT+CEREG=5;
  \             2> ;+COPS?
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    pDecNextChar    output, point to next decode charcater
  \returns      CmsRetId
  \             ATC_DEC_OK:     just means AT command deocde OK,
  \                             and let the processor func to return the AT result code. caller shound return directly.
  \             ATC_DEC_NO_AT   No valid AT in line (pAtChanEty->atLineInfo), caller should get the next line input from
  \                             pending input list.
  \             OTHER           just means AT command deocde NOK, and the caller should response ERROR result code, skip current line,
  \                             and decode next line.
*/
static AtcDecRetCode atcDecOneAt(AtChanEntityP pAtChanEty, CHAR **pDecNextChar)
{
    AtCmdLineInfo   *pAtLine = &(pAtChanEty->atLineInfo);
    CHAR    *pDecChar   = pAtLine->pNextHdr;
    BOOL    prefixFound = FALSE;
    AtcDecRetCode   atcRet = ATC_DEC_OK;

    OsaCheck(pAtLine->pLine != PNULL && pAtLine->pNextHdr != PNULL && pAtLine->pEnd != PNULL && pAtLine->pEnd > pAtLine->pNextHdr,
             pAtLine->pLine, pAtLine->pNextHdr, pAtLine->pEnd);

    /* should be the start of one AT, but maybe not the start of one line */
    OsaCheck(pAtLine->startAt == TRUE, pAtLine->startLine, pAtLine->pLine, pAtLine->pNextHdr);

    AT_IGNORE_SPACE_S3_S4_CHAR(pDecChar,
                               pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX],
                               pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX],
                               pAtLine->pEnd);

    if (pDecChar >= pAtLine->pEnd || *pDecChar == '\0')
    {
        OsaDebugBegin(FALSE, pDecChar, pAtLine->pEnd, *pDecChar);
        OsaDebugEnd();

        /*
         * Maybe the first byte is: 0x00, not the right input, current line done
        */
        pAtLine->pNextHdr = PNULL;
        pAtLine->startLine  = FALSE;
        pAtLine->startAt    = FALSE;

        *pDecNextChar = pDecChar;

        return ATC_DEC_NO_AT;
    }

    ECOMM_STRING(UNILOG_ATCMD, atcDecOneAt_1, P_SIG, "ATCMD, decode AT: %s", (UINT8 *)pDecChar);

    /* Start with "AT"/"at"/"At"/"aT" */
    if (pAtLine->startLine)
    {
        pAtLine->startLine  = FALSE;
        pAtLine->startAt    = FALSE;

        if ((*pDecChar) == 'A' || (*pDecChar) == 'a')
        {
            pDecChar++;
            AT_IGNORE_SPACE_CHAR(pDecChar, pAtLine->pEnd);

            if (pDecChar < pAtLine->pEnd &&
                ((*pDecChar) == 'T' || (*pDecChar) == 't'))
            {
                prefixFound = TRUE;
                pDecChar++;
            }
        }

        if (prefixFound == FALSE)
        {
            OsaDebugBegin(FALSE, *(pAtLine->pNextHdr), *(pAtLine->pNextHdr+1), *(pAtLine->pNextHdr+2));
            OsaDebugEnd();
            //OsaCheck(FALSE, *(pAtLine->pNextHdr), *(pAtLine->pNextHdr+1), *(pAtLine->pNextHdr+2));

            *pDecNextChar = pDecChar;
            return ATC_DEC_SYNTAX_ERR;
        }

        AT_IGNORE_SPACE_CHAR(pDecChar, pAtLine->pEnd);
    }

    pAtLine->startAt    = FALSE;

    /*
     * Basic AT command format:
     * 1> AT<command>[<number>]
     * 2> ATS<parameter_number>?
     * 3> ATS<parameter_number>=[<value>]
     *
     * Extended AT command format:
     * 1> action command:
     *   a) AT+<name>
     *   b) AT+<name>[=<value>]
     *   c) AT+<name>[=<compound_value>]
     *   d) AT+<name>=?
     * 2> parameter command
     *   a) AT+<name>=[<value>]
     *   b) AT+<name>=[<compound_value>]
     *   c) AT+<name>?
     *   d) AT+<name>=?
    */
    AT_IGNORE_SPACE_CHAR(pDecChar, pAtLine->pEnd);

    if (pDecChar >= pAtLine->pEnd)  //AT only
    {
        //basic AT
        atcRet = atcDecBasicAtCmd(pAtChanEty, pDecChar, pDecNextChar);
    }
    else
    {
        /* if extended AT command */
        if (AT_BE_EXT_CMD_CHAR(*pDecChar))
        {
            atcRet = atcDecExtendedAtCmd(pAtChanEty, pDecChar, pDecNextChar);
        }
        else
        {
            //basic AT
            atcRet = atcDecBasicAtCmd(pAtChanEty, pDecChar, pDecNextChar);
        }
    }

    return atcRet;
}


/**
  \fn          CmsRetId atcDecAtLine(AtChanEntityP pAtChanEty)
  \brief
  \param[in]
  \returns     CmsRetId
  \            ATC_DEC_OK:      just means AT command deocde OK,
  \                             and let the processor func to return the AT result code. caller shound return directly
  \            ATC_DEC_NO_AT    No valid AT in line (pAtChanEty->atLineInfo), caller should get the next line input from
  \                             pending input list.
*/
static AtcDecRetCode atcDecAtLine(AtChanEntityP pAtChanEty)
{
    AtCmdLineInfo   *pLineInfo = &(pAtChanEty->atLineInfo);
    AtcDecRetCode   atcRet = ATC_DEC_OK;
    CHAR    *pDecNextChar = PNULL;

    /*
     * 1. A command line is made up of three elements: the prefix, the body, and the termination character.
     * 2. The command line prefix consists of the characters "AT" (IA5 4/1, 5/4) or "at" (IA5 6/1, 7/4), or,
     *    to repeat the execution of the previous command line, the characters "A/" (IA5 4/1, 2/15) or "a/"
     *    (IA5 6/1, 2/15).
     * 3. The body is made up of individual commands, Space characters (IA5 2/0) are ignored and may be used
     *    freely for formatting purposes, unless they are embedded in numeric or string constants (see 5.4.2.1 or 5.4.2.2).
     *    The termination character may not appear in the body.
     * 4. The termination character may be selected by a user option (parameter S3),
     *    the default being CR (IA5 0/13).
     * ==========================================================================
     * pLine should be:
     * 1> ATxxxxxx<S3>
     * 2> atxxxxxx<S3>
     * 3> Atxxxxxx<S3>        ==> REFER is OK
     * 4> <SP>/<SP>/<S3>/<S4>ATxxxxxx<S3>
     * 5> ATxxxxxx<S3>ATxxxxxxx<S3>
     *
    */

    if (pLineInfo->pLine == PNULL || pLineInfo->pNextHdr == PNULL)  //AT LINE done
    {
        return ATC_DEC_NO_AT;
    }

    OsaCheck(pLineInfo->pEnd != PNULL && pLineInfo->pEnd > pLineInfo->pNextHdr,
             pLineInfo->pLine, pLineInfo->pNextHdr, pLineInfo->pEnd);


    //decode one line
    while (TRUE)
    {
        atcRet = atcDecOneAt(pAtChanEty, &pDecNextChar);

        if (atcRet == ATC_DEC_OK ||
            atcRet == ATC_DEC_NO_AT)
        {
            if (pLineInfo->pNextHdr == PNULL) //all AT decoded
            {
                OsaCheck(pLineInfo->startLine == FALSE && pLineInfo->startAt == FALSE,
                         pLineInfo->startLine, pLineInfo->startAt, pLineInfo->pLine);
                atcFreeAtCmdLine(pAtChanEty);
            }

            break;
        }
        else
        {
            OsaDebugBegin(FALSE, atcRet, 0, pLineInfo->pNextHdr);
            OsaDebugEnd();

            /* send the error result code */
            atcSendResultCode(pAtChanEty, AT_RC_ERROR, PNULL);

            /* skip current line */
            atcMoveToNextAtCmdLine(pAtChanEty, pDecNextChar);

            if (pLineInfo->pNextHdr == PNULL)   //just means current line all done
            {
                OsaCheck(pLineInfo->startLine == FALSE && pLineInfo->startAt == FALSE,
                         pLineInfo->startLine, pLineInfo->startAt, pLineInfo->pLine);

                atcFreeAtCmdLine(pAtChanEty);

                atcRet = ATC_DEC_NO_AT;
                break;
            }
            else    //proc next line
            {
                OsaCheck(pLineInfo->startLine == TRUE && pLineInfo->startAt == TRUE,
                         pLineInfo->startLine, pLineInfo->startAt, pLineInfo->pLine);
                //continue to run
            }
        }
    }

    return atcRet;
}

/**
  \fn           void atcDecOneNode(AtChanEntity *pAtChanEty, AtCmdInputNode *pNewNode, UINT32 inputLen)
  \brief
  \param[in]
  \returns
  \
  \
*/
static void atcDecOneNode(AtChanEntity *pAtChanEty, AtCmdInputNode *pNewNode, UINT32 inputLen)
{
    AtInputInfo     *pAtInfo = &(pAtChanEty->atInputInfo);
    AtcDecRetCode   atcRet = ATC_DEC_OK;

    OsaCheck(pAtChanEty->bApiMode == FALSE, pAtChanEty->bApiMode, pAtChanEty->chanId, inputLen);

    /*
     * insert into the node list
    */
    if (pAtInfo->input.cmdInput.pHdr == PNULL)
    {
        OsaCheck(pAtInfo->input.cmdInput.pTailer == PNULL && pAtInfo->pendingNodeNum == 0 && pAtInfo->pendingLen == 0,
                 pAtInfo->input.cmdInput.pTailer, pAtInfo->pendingNodeNum, pAtInfo->pendingLen);

        pAtInfo->input.cmdInput.pHdr = pAtInfo->input.cmdInput.pTailer = pNewNode;
        pAtInfo->input.cmdInput.pTailer->pNextNode = PNULL;

        pAtInfo->pendingNodeNum++;
        pAtInfo->pendingLen += inputLen;
    }
    else
    {
        OsaCheck(pAtInfo->input.cmdInput.pTailer != PNULL && pAtInfo->input.cmdInput.pTailer->pNextNode == PNULL && pAtInfo->pendingNodeNum != 0 && pAtInfo->pendingLen != 0,
                 pAtInfo->input.cmdInput.pTailer, pAtInfo->pendingNodeNum, pAtInfo->pendingLen);

        pAtInfo->input.cmdInput.pTailer->pNextNode = pNewNode;
        pAtInfo->input.cmdInput.pTailer = pNewNode;
        pAtInfo->input.cmdInput.pTailer->pNextNode = PNULL;

        pAtInfo->pendingNodeNum++;
        pAtInfo->pendingLen += inputLen;
    }

    /*
     * if current one AT is ongoing, just pending
    */
    if (pAtChanEty->atLineInfo.pLine != PNULL ||
        OsaTimerIsRunning(pAtChanEty->asynTimer))
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecodeOneNode_pending_1, P_WARNING, 1,
                    "AT CMD, one AT is ongoing, pending current one, input len: %d", inputLen);

        // check whether pending too much
        if (pAtInfo->pendingLen > AT_CMD_MAX_PENDING_STR_LEN || pAtInfo->pendingNodeNum > AT_CMD_MAX_PENDING_NODE_NUM)
        {
            OsaDebugBegin(FALSE, pAtInfo->pendingLen, pAtInfo->pendingNodeNum, 0);
            OsaDebugEnd();
            ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecodeOneNode_1, P_ERROR, 2,
                        "AT CMD, too much pending AT input, pendingLen: %d/pendingNodeNum: %d, discard all",
                        pAtInfo->pendingLen, pAtInfo->pendingNodeNum);

            atcDiscardAllPendingAtNode(pAtChanEty);
        }

        return;
    }

    /*
     * No AT command is ongoing, get one line
    */
    while (TRUE)
    {
        atcGetAtLine(pAtChanEty);

        if (pAtChanEty->atLineInfo.pLine != PNULL)
        {
            atcEchoString(pAtChanEty, pAtChanEty->atLineInfo.pLine, (pAtChanEty->atLineInfo.pEnd - pAtChanEty->atLineInfo.pLine + 1));

            atcRet = atcDecAtLine(pAtChanEty);

            if (atcRet == ATC_DEC_OK)
            {
                //current AT ongoing, waiting for response, the re-decode next line
                break;
            }
            else
            {
                // no AT ongoing, process next node
                ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecOneNode_next_1, P_INFO, 1,
                            "AT CMD, chan Id: %d, current line done, check next..", pAtChanEty->chanId);

                OsaCheck(atcRet == ATC_DEC_NO_AT, atcRet, pAtChanEty->atLineInfo.pLine, pAtChanEty->atLineInfo.pNextHdr);
                continue;
            }
        }
        else
        {
            //no AT need to proc
            break;
        }
    }

    // check whether pending too much
    if (pAtInfo->pendingLen > AT_CMD_MAX_PENDING_STR_LEN || pAtInfo->pendingNodeNum > AT_CMD_MAX_PENDING_NODE_NUM)
    {
        OsaDebugBegin(FALSE, pAtInfo->pendingLen, pAtInfo->pendingNodeNum, 0);
        OsaDebugEnd();
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecodeOneNode_warn_1, P_ERROR, 2,
                    "AT CMD, too much pending AT input, pendingLen: %d/pendingNodeNum: %d, discard all",
                    pAtInfo->pendingLen, pAtInfo->pendingNodeNum);

        atcDiscardAllPendingAtNode(pAtChanEty);

        /* send the error result code, if no AT is ongoing */
        atcSendResultCode(pAtChanEty, AT_RC_ERROR, PNULL);
    }

    return;
}


/**
  \fn           void atcDecOneNode(AtChanEntity *pAtChanEty, AtCmdInputNode *pNewNode, UINT32 inputLen)
  \brief
  \param[in]
  \returns      AtcDecRetCode
  \             ATC_DEC_OK - AT command line processed OK, semaphore released when AT response send.
  \             OTHER      - AT command line not processed normally, "AT command string" & "AT API node" &
  \                          "semaphone" should be freed/released in caller
*/
static AtcDecRetCode atcDecOneApiNode(AtChanEntity *pAtChanEty, AtApiInputNode *pNewNode, UINT32 inputLen)
{
    AtInputInfo     *pAtInfo = &(pAtChanEty->atInputInfo);
    AtcDecRetCode   atcRet = ATC_DEC_OK;

    OsaCheck(pAtChanEty->bApiMode == TRUE, pAtChanEty->bApiMode, pAtChanEty->chanId, inputLen);

    /*
     * check if pending too much
    */
    if ((pAtInfo->pendingLen + inputLen) > AT_CMD_MAX_PENDING_STR_LEN ||
        (pAtInfo->pendingNodeNum + 1) > AT_CMD_MAX_PENDING_NODE_NUM)
    {
        OsaDebugBegin(FALSE, pAtInfo->pendingLen, pAtInfo->pendingNodeNum, inputLen);
        OsaDebugEnd();
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecOneApiNode_warning_1, P_WARNING, 3,
                    "AT CMD, too much pending RIL AT input, pendingLen: %d/currentLen: %d/pendingNodeNum: %d, discard current one",
                    pAtInfo->pendingLen, inputLen, pAtInfo->pendingNodeNum);

        return ATC_DEC_PROC_ERR;
    }


    /*
     * insert into the node list
    */
    if (pAtInfo->input.apiInput.pHdr == PNULL)
    {
        OsaCheck(pAtInfo->input.apiInput.pTailer == PNULL && pAtInfo->pendingNodeNum == 0 && pAtInfo->pendingLen == 0,
                 pAtInfo->input.apiInput.pTailer, pAtInfo->pendingNodeNum, pAtInfo->pendingLen);

        pAtInfo->input.apiInput.pHdr = pAtInfo->input.apiInput.pTailer = pNewNode;
        pAtInfo->input.apiInput.pTailer->pNextNode = PNULL;

        pAtInfo->pendingNodeNum++;
        pAtInfo->pendingLen += inputLen;
    }
    else
    {
        OsaCheck(pAtInfo->input.apiInput.pTailer != PNULL && pAtInfo->input.apiInput.pTailer->pNextNode == PNULL && pAtInfo->pendingNodeNum != 0 && pAtInfo->pendingLen != 0,
                 pAtInfo->input.apiInput.pTailer, pAtInfo->pendingNodeNum, pAtInfo->pendingLen);

        pAtInfo->input.apiInput.pTailer->pNextNode = pNewNode;
        pAtInfo->input.apiInput.pTailer = pNewNode;
        pAtInfo->input.apiInput.pTailer->pNextNode = PNULL;

        pAtInfo->pendingNodeNum++;
        pAtInfo->pendingLen += inputLen;
    }

    /*
     * if current one AT is ongoing, just pending
    */
    if (pAtChanEty->atLineInfo.pLine != PNULL ||
        OsaTimerIsRunning(pAtChanEty->asynTimer))
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecOneApiNode_pending_1, P_WARNING, 1,
                    "AT CMD, one RIL AT is ongoing, pending current one, input len: %d", inputLen);

        return ATC_DEC_OK;
    }

    /*
     * No AT command is ongoing, get one line
    */
    while (TRUE)
    {
        atcGetAtLine(pAtChanEty);

        if (pAtChanEty->atLineInfo.pLine != PNULL)
        {
            // RIL API, don't need to ECHO
            //atcEchoString(pAtChanEty, pAtChanEty->atLineInfo.pLine, (pAtChanEty->atLineInfo.pEnd - pAtChanEty->atLineInfo.pLine + 1));

            atcRet = atcDecAtLine(pAtChanEty);

            if (atcRet == ATC_DEC_OK)
            {
                //current AT ongoing, waiting for response, the re-decode next line
                break;
            }
            else
            {
                // no AT ongoing, process next node
                ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcDecOneNode_next_1, P_INFO, 1,
                            "AT CMD, chan Id: %d, current line done, check next..", pAtChanEty->chanId);

                OsaCheck(atcRet == ATC_DEC_NO_AT, atcRet, pAtChanEty->atLineInfo.pLine, pAtChanEty->atLineInfo.pNextHdr);
                continue;
            }
        }
        else
        {
            //no AT need to proc
            break;
        }
    }

    return ATC_DEC_OK;
}




/**
  \fn           void atcProcAtInDataState(AtChanEntity *pAtChanEty, CHAR *pAtStr, UINT16 strLen)
  \brief        Process the signal: SIG_AT_CMD_STR_REQ, while AT channel in data state
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    pAtData         AT input data       //allocated in heap
  \param[in]    dataLen         AT input data len
  \returns      void
*/
static void atcProcAtInDataState(AtChanEntity *pAtChanEty, CHAR *pAtData, UINT16 dataLen)
{
    CmsRetId cmsRet = CMS_RET_SUCC;

    OsaCheck(pAtChanEty->chanState != ATC_ONLINE_CMD_STATE, pAtChanEty->chanState, ATC_ONLINE_CMD_STATE, dataLen);

    switch (pAtChanEty->chanState)
    {
        case ATC_SMS_CMGS_CMGC_DATA_STATE:
        {
            atcEchoString(pAtChanEty, pAtData, dataLen);

            //call CMGS API
            cmsRet = smsCMGSCMGCInputData(pAtChanEty->chanId, (UINT8 *)pAtData, dataLen);

            if (cmsRet != CMS_RET_SUCC)
            {
                OsaDebugBegin(FALSE, cmsRet, ATC_SMS_CMGS_CMGC_DATA_STATE, 0);
                OsaDebugEnd();

                /* need change back to command state*/
                atcChangeChannelState(pAtChanEty->chanId, ATC_ONLINE_CMD_STATE);
            }
            break;
        }
#if defined(FEATURE_MQTT_ENABLE) || defined(FEATURE_MQTT_TLS_ENABLE)
        case ATC_MQTT_PUB_DATA_STATE:
        {
            atcEchoString(pAtChanEty, pAtData, dataLen);

            //call mqtt API
            cmsRet = mqttPUBInputData(pAtChanEty->chanId, (UINT8 *)pAtData, dataLen);

            if (cmsRet != CMS_RET_SUCC)
            {
                OsaDebugBegin(FALSE, cmsRet, ATC_SMS_CMGS_CMGC_DATA_STATE, 0);
                OsaDebugEnd();

                /* need change back to command state*/
                atcChangeChannelState(pAtChanEty->chanId, ATC_ONLINE_CMD_STATE);
            }
            break;
        }
#endif

#ifdef FEATURE_LIBCOAP_ENABLE
        case ATC_COAP_SEND_DATA_STATE:
        {
            atcEchoString(pAtChanEty, pAtData, dataLen);

            //call coap API
            cmsRet = coapSENDInputData(pAtChanEty->chanId, (UINT8 *)pAtData, dataLen);

            if (cmsRet != CMS_RET_SUCC)
            {
                OsaDebugBegin(FALSE, cmsRet, ATC_SMS_CMGS_CMGC_DATA_STATE, 0);
                OsaDebugEnd();

                /* need change back to command state*/
                atcChangeChannelState(pAtChanEty->chanId, ATC_ONLINE_CMD_STATE);
            }
            break;
        }
#endif
        case ATC_SOCKET_SEND_DATA_STATE:
        {
            atcEchoString(pAtChanEty, pAtData, dataLen);

            //call
            cmsRet = nmSocketInputData(pAtChanEty->chanId, (UINT8 *)pAtData, dataLen);

            if (cmsRet != CMS_RET_SUCC)
            {
                OsaDebugBegin(FALSE, cmsRet, ATC_SOCKET_SEND_DATA_STATE, 0);
                OsaDebugEnd();

                /* need change back to command state*/
                atcChangeChannelState(pAtChanEty->chanId, ATC_ONLINE_CMD_STATE);
            }
            break;
        }
        case ATC_SMS_CMGC_DATA_STATE:
        default:
            OsaDebugBegin(FALSE, pAtChanEty->chanState, pAtChanEty->chanId, dataLen);
            OsaDebugEnd();
            break;
    }

    // free the pAtData;
    OsaFreeMemory(&pAtData);

    return;
}


/**
  \fn           BOOL atcBeHandshakeInput(AtChanEntity *pAtChanEty, CHAR *pAtData, UINT16 dataLen)
  \brief        Check whether input is handshake string, if so, reply the handshake response
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    pAtData         AT input data       //allocated in heap
  \param[in]    dataLen         AT input data len
  \returns      BOOL
*/
static BOOL atcBeHandshakeInput(AtChanEntity *pAtChanEty, CHAR *pAtData, UINT16 dataLen)
{
    if (strncasecmp(pAtData, AT_HANDSHAKE_REQ_STR, strlen(AT_HANDSHAKE_REQ_STR)) == 0)
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcBeHandshakeInput_1, P_INFO, 0,
                    "AT CMD, Handshake RESP: KSDH");

        atcSendRawRespStr(pAtChanEty, AT_HANDSHAKE_RESP_STR, AT_HANDSHAKE_RESP_STR_LEN);

        if (dataLen > AT_HANDSHAKE_REQ_STR_LEN)
        {
            /*
             * Maybe a normal AT command followed handshake, example:
             *   HDSK\r\nAT+CEREG?\r\n
             * In such case, normal AT command should be processed, here, return FALSE
            */
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }

    return FALSE;
}

/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS

/**
  \fn           BOOL atcBePreAtLineDone(AtChanEntity *pAtChanEty)
  \brief        Check whether previous AT command all performed/decoded,
  \             Called when one AT done, and before sending the result code.
  \             If return TRUE, result code: "OK" is needed, else need to pend this
  \             "OK" result code until all CMDs in a command line has been performed
  \             successfull.
  \param[in]    pAtChanEty      AT channel entity
  \returns      BOOL
*/
BOOL atcBePreAtLineDone(AtChanEntity *pAtChanEty)
{
    /*
     * whether previous AT line finished
    */
    AtCmdLineInfo   *pLineInfo = &(pAtChanEty->atLineInfo);

    if (pLineInfo->pLine == PNULL || pLineInfo->pNextHdr == PNULL)
    {
        return TRUE;
    }

    //already point to next line header, just means previous line already done
    if (pLineInfo->startLine)
    {
        return TRUE;
    }

    return FALSE;
}

/**
  \fn           BOOL atcAnyPendingAt(AtChanEntity *pAtChanEty)
  \brief        Check whether all AT commands are performed
  \param[in]
  \returns      BOOL
*/
BOOL atcAnyPendingAt(AtChanEntity *pAtChanEty)
{
    AtCmdLineInfo   *pLineInfo = &(pAtChanEty->atLineInfo);

    if (pLineInfo->pNextHdr != PNULL)
    {
        return TRUE;
    }

    if (pAtChanEty->bApiMode)
    {
        /* check the at input node */
        if (pAtChanEty->atInputInfo.input.apiInput.pHdr != PNULL)
        {
            OsaCheck(pAtChanEty->atInputInfo.input.apiInput.pTailer != PNULL && pAtChanEty->atInputInfo.input.apiInput.pTailer->pNextNode == PNULL,
                     pAtChanEty->atInputInfo.input.apiInput.pTailer, pAtChanEty->atInputInfo.pendingLen, pAtChanEty->atInputInfo.pendingNodeNum);

            return TRUE;
        }
        else
        {
            OsaCheck(pAtChanEty->atInputInfo.input.apiInput.pTailer == PNULL &&
                     pAtChanEty->atInputInfo.pendingLen == 0 &&
                     pAtChanEty->atInputInfo.pendingNodeNum == 0,
                     pAtChanEty->atInputInfo.input.apiInput.pTailer, pAtChanEty->atInputInfo.pendingLen, pAtChanEty->atInputInfo.pendingNodeNum);
        }
    }
    else
    {
        /* check the at input node */
        if (pAtChanEty->atInputInfo.input.cmdInput.pHdr != PNULL)
        {
            OsaCheck(pAtChanEty->atInputInfo.input.cmdInput.pTailer != PNULL && pAtChanEty->atInputInfo.input.cmdInput.pTailer->pNextNode == PNULL,
                     pAtChanEty->atInputInfo.input.cmdInput.pTailer, pAtChanEty->atInputInfo.pendingLen, pAtChanEty->atInputInfo.pendingNodeNum);

            return TRUE;
        }
        else
        {
            OsaCheck(pAtChanEty->atInputInfo.input.cmdInput.pTailer == PNULL &&
                     pAtChanEty->atInputInfo.pendingLen == 0 &&
                     pAtChanEty->atInputInfo.pendingNodeNum == 0,
                     pAtChanEty->atInputInfo.input.cmdInput.pTailer, pAtChanEty->atInputInfo.pendingLen, pAtChanEty->atInputInfo.pendingNodeNum);
        }
    }


    return FALSE;
}



/**
  \fn           void atcProcAtCmdStrReqSig(AtCmdStrReq *pAtReq)
  \brief        Process the signal: SIG_AT_CMD_STR_REQ
  \param[in]
  \returns      void
*/
void atcProcAtCmdStrReqSig(AtCmdStrReq *pAtReq)
{
    AtChanEntity    *pAtChanEty = PNULL;
    AtCmdInputNode  *pAtNode = PNULL;
    UINT8   chanId = pAtReq->atChanId;

    pAtChanEty = atcGetEntityById(chanId);

    OsaDebugBegin(pAtChanEty != PNULL && pAtReq->atStrLen > 0 && pAtReq->pAtStr != PNULL, chanId, pAtReq->atStrLen, pAtReq->pAtStr);
        if (pAtReq->pAtStr != PNULL)
        {
            OsaFreeMemory(&(pAtReq->pAtStr));
        }

        return;
    OsaDebugEnd();

    if (pAtChanEty->chanState != ATC_ONLINE_CMD_STATE)
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcProcAtCmdStrReqSig_state_1, P_INFO, 2,
                    "AT CMD, chanId: %d, state is: %d, not CMD state, don't need to decode AT", pAtChanEty->chanId, pAtChanEty->chanState);

        atcProcAtInDataState(pAtChanEty, pAtReq->pAtStr, pAtReq->atStrLen);

        return;
    }

    if (pAtReq->atStrLen < 100)
    {
        ECOMM_DUMP(UNILOG_ATCMD_PARSER, atcProcAtCmdStrReqSig_info_1, P_INFO,
                   "AT CMD, RECV dump: ", pAtReq->atStrLen, (UINT8 *)pAtReq->pAtStr);
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcProcAtCmdStrReqSig_info_2, P_INFO, 2,
                    "AT chanId: %d, recv long AT, length: %d",
                    chanId, pAtReq->atStrLen);

        ECOMM_DUMP(UNILOG_ATCMD_PARSER, atcProcAtCmdStrReqSig_info_3, P_INFO,
                   "AT CMD, RECV dump: ", 100, (UINT8 *)pAtReq->pAtStr);
    }

    /*
     * if handshake wake string
    */
    if (atcBeHandshakeInput(pAtChanEty, pAtReq->pAtStr, pAtReq->atStrLen))
    {
        OsaFreeMemory(&(pAtReq->pAtStr));
        return;
    }

    //create a input string node
    pAtNode = (AtCmdInputNode *)OsaAllocZeroMemory(sizeof(AtCmdInputNode));
    OsaDebugBegin(pAtNode != PNULL, sizeof(AtCmdInputNode), pAtReq->atStrLen, 0);
        OsaFreeMemory(&(pAtReq->pAtStr));
        return;
    OsaDebugEnd();

    /*
     * example:
     * "AT+CEREG?\r\n\0"
     *  ^           ^
     *  pStart      pEnd
     *
     * atStrLen = 11
     *
    */
    pAtNode->pStart = pAtReq->pAtStr;
    pAtNode->pEnd   = pAtReq->pAtStr + pAtReq->atStrLen - 1;
    pAtNode->pNextDec   = pAtReq->pAtStr;

    atcDecOneNode(pAtChanEty, pAtNode, pAtReq->atStrLen);

    return;
}

/**
  \fn           void atcProcAtCmdContinueReqSig(AtCmdStrReq *pAtReq)
  \brief        Process the signal: SIG_AT_CMD_CONTINUE_REQ
  \param[in]
  \returns      void
*/
void atcProcAtCmdContinueReqSig(AtCmdContinueReq *pContReq)
{
    /*
     * when one AT command responsed, need to process next one
    */
    AtChanEntity    *pAtChanEty = PNULL;
    UINT8   chanId = pContReq->atChanId;
    AtcDecRetCode   atcRet = ATC_DEC_OK;

    pAtChanEty = atcGetEntityById(chanId);

    OsaDebugBegin(pAtChanEty != PNULL, chanId, 0, 0);
    return;
    OsaDebugEnd();

    /*
     * Abnormal case:
     * 1> PS task -> SIG_CAC_CMI_CNF -> AT task;
     * 2> During AT task process "SIG_CAC_CMI_CNF", an other AT CMS string comes:
     *     UART Rx task -> SIG_AT_CMD_STR_REQ -> AT task    //in fact, UART Rx task prorioty should lower than AT task
     * 3> AT task continue processing "SIG_CAC_CMI_CNF", and send the signal: "SIG_AT_CMD_CONTINUE_REQ"
     * 4> Than AT task should process the "SIG_AT_CMD_STR_REQ" firstly, and decode the AT CMD.
     * 5> When process the "SIG_AT_CMD_CONTINUE_REQ", the "pAtChanEty->asynTimer" timer should be running
    */
    if (OsaTimerIsRunning(pAtChanEty->asynTimer))
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcProcAtCmdContinueReqSig_w_1, P_WARNING, 0,
                    "AT CHAN: %d, proc SIG_AT_CMD_CONTINUE_REQ, but another AT is ongoing, and guard timer is runing");
        return;
    }
    //else

    /* Guard timer should not running */
    OsaCheck(pAtChanEty->asynTimer == OSA_TIMER_NOT_CREATE,
             pAtChanEty->asynTimer, pAtChanEty->curTid, 0);

    /* decode the pending AT in current pending line */
    atcRet = atcDecAtLine(pAtChanEty);

    if (atcRet == ATC_DEC_OK)
    {
        //current AT ongoing, waiting for response, then re-decode next line
        return;
    }

    OsaCheck(atcRet == ATC_DEC_NO_AT, atcRet, pAtChanEty->atLineInfo.pLine, pAtChanEty->atLineInfo.pNextHdr);

    /*
     * No AT command is ongoing, get one line
    */
    while (TRUE)
    {
        atcGetAtLine(pAtChanEty);

        if (pAtChanEty->atLineInfo.pLine != PNULL)
        {
            atcEchoString(pAtChanEty, pAtChanEty->atLineInfo.pLine, (pAtChanEty->atLineInfo.pEnd - pAtChanEty->atLineInfo.pLine + 1));

            atcRet = atcDecAtLine(pAtChanEty);

            if (atcRet == ATC_DEC_OK)
            {
                //current AT ongoing, waiting for response, the re-decode next line
                break;
            }
            else
            {
                // no AT ongoing, process next node
                ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcProcAtCmdContinueReqSig_next_1, P_INFO, 1,
                            "AT CMD, chan Id: %d, current line done, check next..", pAtChanEty->chanId);

                OsaCheck(atcRet == ATC_DEC_NO_AT, atcRet, pAtChanEty->atLineInfo.pLine, pAtChanEty->atLineInfo.pNextHdr);
                continue;
            }
        }
        else
        {
            //no AT need to proc
            break;
        }
    }

    return;
}


/**
  \fn          void atcDecInit(void)
  \brief
  \param[in]
  \returns     void
*/
void atcDecInit(void)
{
    //AtChanRegInfo   defRegInfo;
    CmsRetId        ret = CMS_RET_SUCC;
    UINT32          tmpIdx = 0;

    memset(gAtChanEtyList, 0x00, sizeof(void *)*CMS_CHAN_NUM);
    memset(&gAtChanEty, 0x00, sizeof(gAtChanEty));

    /*
     * gAtChanEtyList[0] always reserved to PNULL
    */

    for (tmpIdx = 0; tmpIdx < ATC_CHAN_REAL_ENTITY_NUM; tmpIdx++)
    {
        gAtChanEtyList[tmpIdx+1] = &(gAtChanEty[tmpIdx]);
    }

    /*
     * check the AT CMD list, in fact, if wakeup from HIB/Sleep2, in order to save time, not need to check again, -TBD
    */
    if (pmuBWakeupFromHib() != TRUE &&
        pmuBWakeupFromSleep2() != TRUE)
    {
        ret = atcCheckPreDefinedTable(atcGetATCommandsSeqPointer(), atcGetATCommandsSeqNumb());
        OsaDebugBegin(ret == CMS_RET_SUCC, ret, atcGetATCommandsSeqNumb(), 0);
        OsaDebugEnd();

        ret = atcCheckPreDefinedTable(atcGetATCustCommandsSeqPointer(), atcGetATCustCommandsSeqNumb());
        OsaDebugBegin(ret == CMS_RET_SUCC, ret, atcGetATCustCommandsSeqNumb(), 0);
        OsaDebugEnd();

        #ifdef  FEATURE_REF_AT_ENABLE
        ret = atcCheckPreDefinedTable(atcGetRefAtCmdSeqPointer(), atcGetRefAtCmdSeqNumb());
        OsaDebugBegin(ret == CMS_RET_SUCC, ret, atcGetRefAtCmdSeqNumb(), 0);
        OsaDebugEnd();
        #endif
    }

    #if 0
    /*
     * init default AT CHAN
    */
    atcChanEntityInit(gAtChanEtyList[CMS_CHAN_DEFAULT], CMS_CHAN_DEFAULT, FALSE);

    /*
     * register the callback
    */
    memset(&defRegInfo, 0x00, sizeof(AtChanRegInfo));
    strncpy(defRegInfo.chanName, AT_DEFAULT_CHAN_NAME, AT_CHAN_NAME_SIZE);
    defRegInfo.chanName[AT_CHAN_NAME_SIZE-1] = '\0';

    defRegInfo.atRespFunc   = atcDefaultChanRespFun;
    defRegInfo.atUrcFunc    = atcDefaultChanUrcFun;

    atcChanEntityRegister(gAtChanEtyList[CMS_CHAN_DEFAULT], &defRegInfo);
    #endif

    /*
     * init AT RIL API CHAN
    */
    atcChanEntityInit(gAtChanEtyList[AT_RIL_API_CHAN], AT_RIL_API_CHAN, TRUE);

    return;
}


/**
  \fn          CmsRetId atcRegisterAtChannelCallback(UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutput)
  \brief
  \param[in]
  \returns     CmsRetId
*/
CmsRetId atcRegisterAtChannelCallback(UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutput)
{
    AtChanRegInfo *pAtRegInfo = (AtChanRegInfo *)pInput;
    INT32  *pChanId = (INT32 *)pOutput;
    AtChanEntity    *pAtChanEty = PNULL;
    UINT32  tmpIdx = 0, validChanId = 0;

    OsaDebugBegin(inputSize == sizeof(AtChanRegInfo) && pInput != PNULL && outputSize == 4 && pOutput != PNULL,
               inputSize, outputSize, pInput);
    return CMS_INVALID_PARAM;
    OsaDebugEnd();

    *pChanId = 0;

    /*
     * check input parameters, "chanName" should be exist
    */
    OsaDebugBegin(pAtRegInfo->chanName[0] != 0 && pAtRegInfo->atRespFunc != PNULL,
                  pAtRegInfo->chanName[0], pAtRegInfo->atRespFunc, 0);
    return CMS_INVALID_PARAM;
    OsaDebugEnd();

    #if 0
    /*
     * AT_RIL_API_CHAN_NAME: RILAPI, is reserved
    */
    if (strcasecmp(pAtRegInfo->chanName, AT_DEFAULT_CHAN_NAME) == 0 ||
        strcasecmp(pAtRegInfo->chanName, AT_RIL_API_CHAN_NAME) == 0)
    {
        OsaDebugBegin(FALSE, pAtRegInfo->chanName[0], pAtRegInfo->chanName[1], pAtRegInfo->chanName[2]);
        return CMS_INVALID_PARAM;
        OsaDebugEnd();
    }
    #endif

    pAtRegInfo->chanName[AT_CHAN_NAME_SIZE-1] = '\0';

    /*
     * AT_RIL_API_CHAN_NAME: RILAPI, is reserved
    */
    if (strcasecmp(pAtRegInfo->chanName, AT_RIL_API_CHAN_NAME) == 0)
    {
        OsaDebugBegin(FALSE, pAtRegInfo->chanName[0], pAtRegInfo->chanName[1], pAtRegInfo->chanName[2]);
        return CMS_INVALID_PARAM;
        OsaDebugEnd();
    }

    /*
     * AT_DEFAULT_CHAN_NAME: "UARTAT", is acted as default channel
    */
    if (strcasecmp(pAtRegInfo->chanName, AT_DEFAULT_CHAN_NAME) == 0)
    {
        ECOMM_STRING(UNILOG_ATCMD_PARSER, atcRegisterAtChannelCallback_info_1, P_INFO,
                     "AT CMD, register default AT channel: %s", (const UINT8*)pAtRegInfo->chanName);

        pAtChanEty = gAtChanEtyList[AT_CHAN_DEFAULT];

        if (pAtChanEty != PNULL && pAtChanEty->bInited)
        {
            ECOMM_STRING(UNILOG_ATCMD_PARSER, atcRegisterAtChannelCallback_default_warning_1, P_WARNING,
                         "AT CMD, default chan already registered: %s, re-init", (const UINT8*)pAtChanEty->chanName);
        }
        validChanId = AT_CHAN_DEFAULT;
    }
    else
    {
        /*
         * Find a valid empty channel, "AT_CHAN_DEFAULT" is reserved
        */
        for (tmpIdx = AT_CHAN_DEFAULT+1; tmpIdx < CMS_CHAN_NUM; tmpIdx++)
        {
            if (gAtChanEtyList[tmpIdx] != PNULL)
            {
                if (strcasecmp(gAtChanEtyList[tmpIdx]->chanName, pAtRegInfo->chanName) == 0)
                {
                    ECOMM_STRING(UNILOG_ATCMD_PARSER, atcRegisterAtChannelCallback_warning_1, P_WARNING,
                                 "AT CMD, Chan name already registered: %s", (const UINT8*)pAtRegInfo->chanName);

                    validChanId     = tmpIdx;
                    pAtChanEty      = gAtChanEtyList[tmpIdx];
                    break;
                }

                if (gAtChanEtyList[tmpIdx]->chanName[0] == 0 &&
                    pAtChanEty == PNULL)
                {
                    validChanId     = tmpIdx;
                    pAtChanEty      = gAtChanEtyList[tmpIdx];
                }
            }
            else
            {
                break;
            }
        }
    }

    if (pAtChanEty == PNULL)    // no valid
    {
        ECOMM_STRING(UNILOG_ATCMD_PARSER, atcRegisterAtChannelCallback_warning_2, P_WARNING,
                     "AT CMD, No channel left for request channName: %s", (const UINT8*)pAtRegInfo->chanName);
        return CMS_NO_RESOURCE;
    }

    /*
     * free all pending, most case, none
    */
    atcDiscardAllPendingAtNode(pAtChanEty);
    pAtChanEty->bInited = FALSE;

    /*
     * init AT CHAN
    */
    atcChanEntityInit(pAtChanEty, validChanId, FALSE);

    /*
     * register the callback
    */
    atcChanEntityRegister(pAtChanEty, pAtRegInfo);

    /*
     * output
    */
    *pChanId = validChanId;

    return CMS_RET_SUCC;
}

/*
*/
AtChanEntityP atcGetEntityById(UINT32 chanId)
{
    OsaDebugBegin(chanId > CMS_CHAN_RSVD && chanId < CMS_CHAN_NUM, chanId, CMS_CHAN_RSVD, CMS_CHAN_NUM);
    return PNULL;
    OsaDebugEnd();

    OsaDebugBegin(gAtChanEtyList[chanId] != PNULL, chanId, 0, 0);
    return PNULL;
    OsaDebugEnd();

    return gAtChanEtyList[chanId];
}

/*
*/
AtChanEntityP atcGetCheckEntityById(UINT32 chanId)
{
    OsaCheck(chanId > CMS_CHAN_RSVD && chanId < CMS_CHAN_NUM, chanId, CMS_CHAN_RSVD, CMS_CHAN_NUM);

    OsaCheck(gAtChanEtyList[chanId] != PNULL, chanId, 0, 0);

    return gAtChanEtyList[chanId];
}


/**
  \fn           atcStopAsynTimer(AtChanEntityP pAtChanEty, UINT8 tid)
  \brief        stop AT asynchronoous response guard timer
  \param[in]
  \returns      void
*/
void atcStopAsynTimer(AtChanEntityP pAtChanEty, UINT8 tid)
{
    osStatus_t osStatus = osOK;

    OsaCheck(pAtChanEty != PNULL && tid <= AT_MAX_ASYN_GUARD_TIMER_TID, pAtChanEty, tid, 0);

    OsaDebugBegin(pAtChanEty->asynTimer != OSA_TIMER_NOT_CREATE && pAtChanEty->curTid == tid,
                  tid, pAtChanEty->curTid, pAtChanEty->asynTimer);
    return;
    OsaDebugEnd();

    osStatus = OsaTimerStop(pAtChanEty->asynTimer);

    OsaDebugBegin(osStatus == osOK, tid, 0, 0);
    OsaDebugEnd();

    /*
     * pAtChanEty->asynTimerlist[tid].osaTimer set to OSA_TIMER_NOT_CREATE in delete API
    */
    OsaTimerDelete(&(pAtChanEty->asynTimer));

    return;
}


/**
  \fn           void atcSendAtCmdContinueReqSig(AtChanEntityP pAtChanEty)
  \brief        send "SIG_AT_CMD_CONTINUE_REQ" to continue to decode next AT command
  \param[in]    pAtChanEty      AT channel entity
  \returns      void
*/
void atcSendAtCmdContinueReqSig(AtChanEntityP pAtChanEty)
{
    SignalBuf   *pSig = PNULL;
    AtCmdContinueReq    *pContReq = PNULL;

    OsaCreateZeroSignal(SIG_AT_CMD_CONTINUE_REQ, sizeof(AtCmdContinueReq), &pSig);
    pContReq = (AtCmdContinueReq *)(pSig->sigBody);

    pContReq->atChanId = pAtChanEty->chanId;

    OsaSendSignal(CMS_TASK_ID, &pSig);

    return;
}


/**
  \fn           void atcAbortAtCmdLine(AtChanEntity *pAtChanEty)
  \brief        When one AT not accepted by the TA (ERROR), subsequent commands in the same line should not
  \             processed (aborted)
  \param[in]    pAtChanEty          AT channel entity
  \returns      void
*/
void atcAbortAtCmdLine(AtChanEntity *pAtChanEty)
{
    AtCmdLineInfo   *pLineInfo = &(pAtChanEty->atLineInfo);

    // no other AT in the same line
    if (pLineInfo->pNextHdr == PNULL)
    {
        //OsaCheck(pLineInfo->pLine == PNULL, pLineInfo->pNextHdr, pLineInfo->startAt, pLineInfo->pLine);

        return;
    }
    else
    {
        /* discard current line, and move to next line*/
        atcMoveToNextAtCmdLine(pAtChanEty, pLineInfo->pNextHdr);

        if (pLineInfo->pNextHdr == PNULL)   //just means current line all done
        {
            OsaCheck(pLineInfo->startLine == FALSE && pLineInfo->startAt == FALSE,
                     pLineInfo->startLine, pLineInfo->startAt, pLineInfo->pLine);

            atcFreeAtCmdLine(pAtChanEty);
        }
    }

    return;
}

/**
  \fn           void atcAsynTimerExpiry(UINT16 timeId)
  \brief        AT asyn guard timer expiry
  \param[in]    timeId          timer ID
  \returns      void
*/
void atcAsynTimerExpiry(UINT16 timeId)
{
    UINT32  chanId  = AT_GET_ASYN_TIMER_CHAN_ID(timeId);
    UINT32  tid     = AT_GET_ASYN_TIMER_TID(timeId);
    AtChanEntityP   pAtChanEty = atcGetEntityById(chanId);

    ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcAsynTimerExpiry_warning_1, P_WARNING, 2,
                "AT CMD. channId: %d, tid: %d, guard timer expiry", chanId, tid);

    OsaDebugBegin(pAtChanEty != PNULL, chanId, tid, 0);
    return;
    OsaDebugEnd();

    /*
     * check TID, if not the same, just means this guard timer should already stop before (expiry & stop at same time)
    */
    OsaDebugBegin(pAtChanEty->curTid == tid && pAtChanEty->asynTimer != OSA_TIMER_NOT_CREATE,
                  pAtChanEty->curTid, tid, pAtChanEty->asynTimer);
    return;
    OsaDebugEnd();

    OsaTimerDelete(&(pAtChanEty->asynTimer));

    /* send the error result */
    atcSendResultCode(pAtChanEty, AT_RC_ERROR, PNULL);

    /*
     * if AT channel is CGMS data channel, need to notify SMS to cancel SMS sending
    */
    if (pAtChanEty->chanState == ATC_SMS_CMGS_CMGC_DATA_STATE)
    {
        smsCMGSCMGCCancel();
        // change back to AT command state
        atcChangeChannelState(chanId, ATC_ONLINE_CMD_STATE);
    }

#if defined(FEATURE_MQTT_ENABLE) || defined(FEATURE_MQTT_TLS_ENABLE)
    if (pAtChanEty->chanState == ATC_MQTT_PUB_DATA_STATE)
    {
        mqttPUBCancel();
        // change back to AT command state
        atcChangeChannelState(chanId, ATC_ONLINE_CMD_STATE);
    }
#endif

#ifdef FEATURE_LIBCOAP_ENABLE
    if (pAtChanEty->chanState == ATC_COAP_SEND_DATA_STATE)
    {
        coapSENDCancel();
        // change back to AT command state
        atcChangeChannelState(chanId, ATC_ONLINE_CMD_STATE);
    }
#endif

    if (pAtChanEty->chanState == ATC_SOCKET_SEND_DATA_STATE)
    {
        nmSocketFreeSendInfo();
        // change back to AT command state
        atcChangeChannelState(chanId, ATC_ONLINE_CMD_STATE);
    }

    /* abort current line */
    atcAbortAtCmdLine(pAtChanEty);

    /* if any still pending */
    if (atcAnyPendingAt(pAtChanEty))
    {
        atcSendAtCmdContinueReqSig(pAtChanEty);
    }

    return;
}

/**
  \fn           CmsRetId atcChangeChannelState(UINT8 chanId, AtcState newState)
  \brief        AT channel state change
  \param[in]    newState            new channel state
  \returns      CmsRetId
*/
CmsRetId atcChangeChannelState(UINT8 chanId, AtcState newState)
{
    AtChanEntityP   pAtChanEty = atcGetEntityById(chanId);
    AtCmdLineInfo   *pLineInfo = &(pAtChanEty->atLineInfo);

    OsaDebugBegin(pAtChanEty != PNULL, chanId, newState, 0);
    return CMS_FAIL;
    OsaDebugEnd();

    ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcChangeChannelState_1, P_SIG, 3,
                "AT CMD. chanId: %d, channel state change from: %d to %d", chanId, pAtChanEty->chanState, newState);

    if (pAtChanEty->chanState == newState)
    {
        return CMS_RET_SUCC;
    }

    /*
     *  For easy: need to clear pending/followed AT string
    */
    // current line, move to ending
    if (pLineInfo->pNextHdr != PNULL)   //still has pending AT CMD in same line
    {
        pLineInfo->pNextHdr     = PNULL;
        pLineInfo->startLine    = FALSE;
        pLineInfo->startAt      = FALSE;

        /* pLineInfo->pLine will be freed in function: atcDecAtLine() */
    }

    atcDiscardAllPendingAtNode(pAtChanEty);

    pAtChanEty->chanState = newState;

    /*
     * if AT comes from UART, need to check whether allow power off UART (enter sleep1 PMU state)
    */
    if (chanId == CMS_CHAN_UART)
    {
        if (newState != ATC_ONLINE_CMD_STATE)   //enter data state
        {
            CMS_SUB_MOD_NOT_ALLOW_DEEP_PMU(CMS_AT_UART_CHAN_PMU_MOD);   //not allow to enter Sleep2/HIB

            #if defined CHIP_EC616 || defined CHIP_EC616_Z0     //EC616 not allow to power off UART (enter sleep1 PMU state)
            pmuVoteToSleep1State(PMU_SLEEP_AT_UART_MOD, FALSE);
            #endif

            ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcChangeChannelState_pmu_1, P_SIG, 1,
                        "AT CMD, UART channel enter state: %d, wait for more input, not allow enter SLEEP2/HIB",
                        newState);
        }
        else
        {
            CMS_SUB_MOD_ALLOW_DEEP_PMU(CMS_AT_UART_CHAN_PMU_MOD);

            #if defined CHIP_EC616 || defined CHIP_EC616_Z0
            pmuVoteToSleep1State(PMU_SLEEP_AT_UART_MOD, TRUE);
            #endif

            ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcChangeChannelState_pmu_2, P_SIG, 0,
                        "AT CMD, UART channel back to online state, allow enter SLEEP2/HIB");
        }
    }


    return CMS_RET_SUCC;
}

/**
  \fn           CmsRetId atcSetConfigValue(UINT8 chanId, AtcChanCfgItem item, UINT32 value)
  \brief        AT channel config parameter setting
  \param[in]    chanId          Channel ID
  \param[in]    item            which parameter
  \param[in]    value           value
  \returns      CmsRetId
*/
CmsRetId atcSetConfigValue(UINT8 chanId, AtcChanCfgItem item, UINT32 value)
{
    AtChanEntityP   pAtChanEty = atcGetEntityById(chanId);
    AtChanConfig    *pCfg = PNULL;

    OsaDebugBegin(pAtChanEty != PNULL && item < ATC_CFG_PARAM_NUM, chanId, item, value);
    return CMS_FAIL;
    OsaDebugEnd();

    pCfg = &(pAtChanEty->cfg);

    switch (item)
    {
        case ATC_CFG_S3_PARAM:
            if (value <= 255)
            {
                pCfg->S[AT_S3_CMD_LINE_TERM_CHAR_IDX] = value;

                return CMS_RET_SUCC;
            }
            break;

        case ATC_CFG_S4_PARAM:
            if (value <= 255)
            {
                pCfg->S[AT_S4_RESP_FORMAT_CHAR_IDX] = value;

                return CMS_RET_SUCC;
            }
            break;

        case ATC_CFG_S5_PARAM:
            if (value <= 255)
            {
                pCfg->S[AT_S5_CMD_LINE_EDIT_CHAR_IDX] = value;

                return CMS_RET_SUCC;
            }
            break;

        case ATC_CFG_ECHO_PARAM:    //ATE
            if (value <= 1)
            {
                pCfg->echoFlag = value;

                return CMS_RET_SUCC;
            }
            break;

        case ATC_CFG_SUPPRESS_PARAM:    //ATQ
            if (value <= 1)
            {
                pCfg->suppressValue = value;

                return CMS_RET_SUCC;
            }
            break;

        case ATC_CFG_VERBOSE_PARAM:     //ATV
            if (value <= 1)
            {
                pCfg->respFormat = value;

                return CMS_RET_SUCC;
            }
            break;

        default:
            break;
    }

    OsaDebugBegin(FALSE, chanId, item, value);
    OsaDebugEnd();

    return CMS_FAIL;
}


/**
  \fn          void atcRilAtCmdApiCallback(void *pArg)
  \brief
  \param[in]
  \returns     void
*/
void atcRilAtCmdApiCallback(void *pArg)
{
    CmsBlockApiInfo     *pApiInfo   = (CmsBlockApiInfo *)pArg;
    AtRilAtCmdReqData   *pAtRilData = (AtRilAtCmdReqData *)(pApiInfo->pInput);
    AtChanEntityP       pAtChanEty  = atcGetEntityById(AT_RIL_API_CHAN);
    AtApiInputNode      *pAtApiNode = PNULL;
    UINT32  tmpIdx = 0;
    CHAR    *pStartChar = 0;
    CHAR    *pS3End = PNULL;

    // check the input
    OsaCheck(pApiInfo->sem != PNULL && pApiInfo->inputSize == sizeof(AtRilAtCmdReqData) &&
             pAtRilData != PNULL && pApiInfo->timeOutMs != 0,
             pApiInfo, pApiInfo->inputSize, pAtRilData);


    /* Len >= 2: AT, && MAX LEN: 4096 */
    OsaDebugBegin(pAtRilData->pCmdLine != PNULL && pAtRilData->cmdLen >= 2 && pAtRilData->cmdLen < 4096 && pAtRilData->respCallback != PNULL,
                  pAtRilData->pCmdLine, pAtRilData->cmdLen, pAtRilData->respCallback);
        if (pAtRilData->pCmdLine != PNULL)
        {
            OsaFreeMemory(&(pAtRilData->pCmdLine));
        }
        pApiInfo->retCode = CMS_INVALID_PARAM;
        osSemaphoreRelease(pApiInfo->sem);
        return;
    OsaDebugEnd();

    if (pAtRilData->cmdLen < 100)
    {
        ECOMM_DUMP(UNILOG_ATCMD_PARSER, atcRilAtCmdApiCallback_dump_1, P_INFO,
                   "AT CMD, RIL AT dump: ", pAtRilData->cmdLen, (UINT8 *)pAtRilData->pCmdLine);
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcRilAtCmdApiCallback_info_1, P_INFO, 1,
                    "AT RIL, recv long AT, length: %d", pAtRilData->cmdLen);

        ECOMM_DUMP(UNILOG_ATCMD_PARSER, atcRilAtCmdApiCallback_dump_2, P_INFO,
                   "AT CMD, RIL AT dump: ", 100, (UINT8 *)pAtRilData->pCmdLine);
    }


    /*
     * check AT channel entity
    */
    if (pAtChanEty == PNULL || pAtChanEty->bInited == FALSE || pAtChanEty->bApiMode == FALSE)
    {
        OsaDebugBegin(pAtChanEty != PNULL, 0, 0, 0);
        OsaDebugEnd();

        OsaFreeMemory(&(pAtRilData->pCmdLine));
        pApiInfo->retCode = CMS_FAIL;
        osSemaphoreRelease(pApiInfo->sem);
        return;
    }

    /*
     * check whether valid AT command string header
    */
    for (tmpIdx = 0; tmpIdx < pAtRilData->cmdLen; tmpIdx++)
    {
        pStartChar = pAtRilData->pCmdLine + tmpIdx;

        /*
         * if ' ', <S3>, <S4> ignore
        */
        if (*pStartChar == ' ' ||
            *pStartChar == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] ||
            *pStartChar == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX])
        {
            continue;
        }
        else
        {
            break;
        }
        //if (AT_BE_VALID_CHAR(tmpChar))
    }

    if (tmpIdx >= pAtRilData->cmdLen || AT_BE_VALID_CHAR(*pStartChar) == FALSE)
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcRilAtCmdApiCallback_warning_1, P_WARNING, 3,
                    "AT CMD, RIL input AT not valid, startIdx: %d, len: %d, CHAR: 0x%x",
                    tmpIdx, pAtRilData->cmdLen, *pStartChar);

        OsaFreeMemory(&(pAtRilData->pCmdLine));
        pApiInfo->retCode = CMS_INVALID_PARAM;
        osSemaphoreRelease(pApiInfo->sem);
        return;
    }


    #if 0
    /*
     * check AT input should end with "\r\n"/ <S3>
    */
    if (pAtRilData->pCmdLine[pAtRilData->cmdLen-1] != pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] &&
        pAtRilData->pCmdLine[pAtRilData->cmdLen-2] != pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX])
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcRilAtCmdApiCallback_warning_2, P_WARNING, 2,
                    "AT CMD, RIL input AT not right, not end with <S3>(\\r\\n), ending: %d, %d",
                    pAtRilData->pCmdLine[pAtRilData->cmdLen-1], pAtRilData->pCmdLine[pAtRilData->cmdLen-2]);

        OsaFreeMemory(&(pAtRilData->pCmdLine));
        pApiInfo->retCode = CMS_INVALID_PARAM;
        osSemaphoreRelease(pApiInfo->sem);
        return;
    }
    #endif
    /*
     * not support multi-line input
    */
    pS3End = strchr(pStartChar, pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX]);
    if (pS3End == PNULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcRilAtCmdApiCallback_warning_2, P_WARNING, 2,
                    "AT CMD, RIL input AT not right, not end with <S3>(\\r\\n), ending: %d, %d",
                    pAtRilData->pCmdLine[pAtRilData->cmdLen-1], pAtRilData->pCmdLine[pAtRilData->cmdLen-2]);

        OsaFreeMemory(&(pAtRilData->pCmdLine));
        pApiInfo->retCode = CMS_INVALID_PARAM;
        osSemaphoreRelease(pApiInfo->sem);
        return;
    }
    else if (pS3End != (pAtRilData->pCmdLine + pAtRilData->cmdLen - 1) &&
             pS3End != (pAtRilData->pCmdLine + pAtRilData->cmdLen - 2))
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcRilAtCmdApiCallback_warning_3, P_WARNING, 2,
                    "AT CMD, RIL input AT not right, not support multi-AT-line, ending: %d, %d",
                    pAtRilData->pCmdLine[pAtRilData->cmdLen-1], pAtRilData->pCmdLine[pAtRilData->cmdLen-2]);

        OsaFreeMemory(&(pAtRilData->pCmdLine));
        pApiInfo->retCode = CMS_INVALID_PARAM;
        osSemaphoreRelease(pApiInfo->sem);
        return;
    }


    /*
     * Alloc "pApiInput", and init
    */
    pAtApiNode = (AtApiInputNode *)OsaAllocZeroMemory(sizeof(AtApiInputNode));
    OsaDebugBegin(pAtApiNode != PNULL, sizeof(AtApiInputNode), 0, 0);
        OsaFreeMemory(&(pAtRilData->pCmdLine));
        pApiInfo->retCode = CMS_NO_MEM;
        osSemaphoreRelease(pApiInfo->sem);
        return;
    OsaDebugEnd();

    pAtApiNode->pStart = pAtRilData->pCmdLine;
    pAtApiNode->pEnd   = pAtRilData->pCmdLine + pAtRilData->cmdLen - 1;
    pAtApiNode->respFunc    = pAtRilData->respCallback;
    pAtApiNode->pArg        = pAtRilData->respData;
    pAtApiNode->sem         = pApiInfo->sem;
    pAtApiNode->timeOutMs   = pApiInfo->timeOutMs;

    /*
     * insert
    */
    if (atcDecOneApiNode(pAtChanEty, pAtApiNode, pAtRilData->cmdLen) != ATC_DEC_OK)
    {
        /* free the node, AT cmd string, and release semaphone */
        OsaDebugBegin(FALSE, pAtApiNode->pStart[0], pAtApiNode->pStart[1], 0);
        OsaDebugEnd();

        OsaFreeMemory(&(pAtRilData->pCmdLine));
        OsaFreeMemory(&(pAtApiNode));

        pApiInfo->retCode = CMS_BUSY;
        osSemaphoreRelease(pApiInfo->sem);
    }
    else
    {
        pApiInfo->retCode = CMS_RET_SUCC;

        /* !! semaphone should released in AT response function !! */
    }

    return;
}


/**
  \fn          CmsRetId atcRilRegisterUrcSynCallback(UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutput)
  \brief
  \param[in]
  \returns     CmsRetId
*/
CmsRetId atcRilRegisterUrcSynCallback(UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutput)
{
    AtUrcFunctionP  pUrcFunc = (AtUrcFunctionP)(*(UINT32 *)(pInput));
    AtChanEntity    *pAtChanEty = atcGetEntityById(AT_RIL_API_CHAN);

    OsaDebugBegin(inputSize == 4 && pInput != PNULL && outputSize == 0 && pOutput == PNULL && pUrcFunc != PNULL,
                  inputSize, outputSize, pInput);
    return CMS_INVALID_PARAM;
    OsaDebugEnd();

    OsaDebugBegin(pAtChanEty != PNULL && pAtChanEty->bInited == TRUE && pAtChanEty->bApiMode == TRUE,
                  pAtChanEty, pAtChanEty->bInited, pAtChanEty->bApiMode);
    return CMS_NO_RESOURCE;
    OsaDebugEnd();

    if (pAtChanEty->callBack.urcFunc != PNULL)
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcRilRegisterUrcCallback_warning_1, P_WARNING, 0,
                    "AT RIL channel, register URC channel, but already registered before, replace new one");
    }

    pAtChanEty->callBack.urcFunc = pUrcFunc;

    return CMS_RET_SUCC;
}

/**
  \fn          CmsRetId atcRilUnRegisterUrcSynCallback(UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutput)
  \brief
  \param[in]
  \returns     CmsRetId
*/
CmsRetId atcRilUnRegisterUrcSynCallback(UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutput)
{
    AtChanEntity    *pAtChanEty = atcGetEntityById(AT_RIL_API_CHAN);

    OsaDebugBegin(inputSize == 0 && pInput == PNULL && outputSize == 0 && pOutput == PNULL,
                  inputSize, outputSize, pInput);
    OsaDebugEnd();

    if (PNULL != pAtChanEty)
    {
        pAtChanEty->callBack.urcFunc = PNULL;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ATCMD_PARSER, atcRilUnRegisterUrcSynCallback_1, P_WARNING, 0,
                    "pAtChanEty is PNULL!");
        return CMS_FAIL;
    }

    return CMS_RET_SUCC;
}



