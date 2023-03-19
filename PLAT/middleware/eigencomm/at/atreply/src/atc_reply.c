/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atc_reply.c
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "at_util.h"
#include "atc_reply.h"
#include "cmicomm.h"
#include "at_sock_entity.h"
#include "atc_reply_err_map.h"
#if defined(FEATURE_MQTT_ENABLE) || defined(FEATURE_MQTT_TLS_ENABLE)
#include "ec_mqtt_api.h"
#include "at_mqtt_task.h"
#endif
#ifdef  FEATURE_LIBCOAP_ENABLE
#include "ec_coap_api.h"
//#include "at_coap_task.h"
#endif
#ifdef  FEATURE_EXAMPLE_ENABLE
#include "ec_example_api.h"
#include "at_example_task.h"
#endif
#if defined(FEATURE_CTCC_DM_ENABLE) || defined(FEATURE_CUCC_DM_ENABLE) || defined(FEATURE_CMCC_DM_ENABLE)
#include "ec_dm_api.h"
#endif

#ifdef FEATURE_ATADC_ENABLE
#include "ec_adc_api.h"
#endif

#include "ec_fwupd_api.h"


/******************************************************************************
 ******************************************************************************
 * VARIABLES / EXTERNAL FUNC
 ******************************************************************************
******************************************************************************/
#ifdef  FEATURE_REF_AT_ENABLE

/*
 * Some refer AT response don't need to clear blank char which afer colon char
*/
const static AtcRefIgnoreClearAt atRefIgnoreClearAtList[] =
{
    {"+CME ERROR",      10},    /*10 = strlen("+CME ERROR")*/
    {"+QCFG",           5},     /*5 = strlen("+QCFG")*/

    //...add here
    {PNULL,             0}      //end
};


#endif


/******************************************************************************
 ******************************************************************************
 * STATIC FUNCTION
 ******************************************************************************
******************************************************************************/

#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position----Below for internal use

#ifdef  FEATURE_REF_AT_ENABLE

/**
  \fn               INT32 atcNeedDelSpaceBeforeColon(const CHAR *pStr)
  \brief            whether need to clear space character before colon
  \param[in]        pStr
  \returns          const CHAR *
  \                 point to the space/blank character, which need to clear/delete
*/
static const CHAR *atcNeedDelSpaceBeforeColon(const CHAR *pStr)
{
    /*
     * Example:
     * EC:
     * +CEREG: 1
     * -->
     * REF:
     * +CEREG:1
    */
    INT32   startIdx = -1, colonIdx = -1, tmpIdx = 0, nameLen = 0;

    OsaDebugBegin(pStr != PNULL, pStr, 0, 0);
    return PNULL;
    OsaDebugEnd();

    /*
     * find the first colon ":"
    */
    for (tmpIdx = 0; tmpIdx < AT_CMD_MAX_NAME_LEN; tmpIdx++)
    {
        if (pStr[tmpIdx] == 0)
        {
            break;
        }

        if (pStr[tmpIdx] == ':')
        {
            colonIdx = tmpIdx;
            break;
        }

        if (startIdx == -1 &&
            pStr[tmpIdx] != '\r' && pStr[tmpIdx] != '\n')
        {
            startIdx = tmpIdx;
        }
    }

    if ((colonIdx == -1) || (pStr[colonIdx+1] != ' ') || (startIdx == -1))
    {
        return PNULL;    //no colon char found, or no blank after colon
    }

    /*
     * +CEREG: 1
     * ^     ^colonIdx
     * |startIdx
    */
    nameLen = colonIdx - startIdx;

    /* find whether not need to clear this blank */
    tmpIdx = 0;
    while (TRUE)
    {
        if (atRefIgnoreClearAtList[tmpIdx].pAtName == PNULL)
        {
            break;
        }

        if (atRefIgnoreClearAtList[tmpIdx].atNameStrLen == nameLen)
        {
            if (strncasecmp(pStr+startIdx, atRefIgnoreClearAtList[tmpIdx].pAtName, nameLen) == 0)
            {
                break;
            }
        }

        tmpIdx++;
    }

    if (atRefIgnoreClearAtList[tmpIdx].pAtName != PNULL)
    {
        //found, don't need to clear/delete "blank" char
        ECOMM_TRACE(UNILOG_ATCMD_EXEC, atcNeedDelSpaceBeforeColon_1, P_VALUE, 0,
                    "AT CMD, REF RESP, do not need to delete space char");
        return PNULL;
    }

    /********
     * should clear blank
     *
     * +CEREG: 1
     *        ^
     *        return
    ********/
    return (pStr+colonIdx+1);
}

#endif


/**
  \fn           CmsRetId atcRespStrOutput(const AtChanEntityP pAtChanEty, const CHAR *pStr, UINT32 strLen)
  \brief        Call the AT channel response function, to output the AT response
  \param[in]
  \returns      CmsRetId
*/
static CmsRetId atcRespStrOutput(const AtChanEntityP pAtChanEty, const CHAR *pStr, UINT32 strLen)
{
    OsaCheck(pAtChanEty != PNULL && pStr != PNULL, pAtChanEty, pStr, 0);

    if (pStr[0] == '\0')
    {
        OsaDebugBegin(FALSE, pStr[0], pStr[1], pAtChanEty->chanId);
        OsaDebugEnd();

        return CMS_RET_SUCC;
    }

    OsaDebugBegin(pAtChanEty->callBack.respFunc != PNULL, pAtChanEty->chanId, 0, 0);
    return CMS_FAIL;
    OsaDebugEnd();

    if (strLen == 0)
    {
        strLen = strlen(pStr);
    }


    (pAtChanEty->callBack.respFunc)(pStr, strLen, pAtChanEty->callBack.pArg);

    return CMS_RET_SUCC;
}




/**
  \fn           CmsRetId atcSendRespStr(const AtChanEntityP pAtChanEty, const CHAR *pStr, UINT32 strLen, BOOL bFinal)
  \brief        Send AT command response
  \param[in]
  \returns      void
*/
static CmsRetId atcSendRespStr(const AtChanEntityP pAtChanEty, const CHAR *pStr, UINT32 strLen, BOOL bFinalPrint)
{
    BOOL    bFailed = FALSE;
    CHAR    *pNewRespBuf = PNULL;

    OsaCheck(pAtChanEty != PNULL && pStr != PNULL, pAtChanEty, pStr, 0);

    if (pAtChanEty->bApiMode != TRUE)
    {
        OsaCheck(pAtChanEty->callBack.apiSem == PNULL && pAtChanEty->callBack.pBufResp == PNULL,
                 pAtChanEty->callBack.apiSem, pAtChanEty->callBack.pBufResp, pAtChanEty->callBack.respBufLen);

        return atcRespStrOutput(pAtChanEty, pStr, strLen);
    }

    /*
     * API mode
    */

    if (pStr[0] == '\0')
    {
        OsaDebugBegin(FALSE, pStr[0], pStr[1], pAtChanEty->chanId);
        OsaDebugEnd();

        return CMS_RET_SUCC;
    }

    OsaDebugBegin(pAtChanEty->callBack.respFunc != PNULL && pAtChanEty->callBack.apiSem != PNULL,
                  pAtChanEty->chanId, pAtChanEty->callBack.respFunc, pAtChanEty->callBack.apiSem);
    return CMS_FAIL;
    OsaDebugEnd();

    if (strLen == 0)
    {
        strLen = strlen(pStr);
    }

    if (bFinalPrint == FALSE)
    {
        /*
         * need to pending
        */
        if (pAtChanEty->callBack.pBufResp == PNULL)
        {
            if (strLen + ATC_EXTENDED_RESULT_STR_LEN > ATC_IND_RESP_MAX_STR_LEN)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, atcSendRespStr_len_warn_1, P_WARNING, 3,
                            "AT CMD, RIL response length too long, %d + %d > MAX: %d ",
                            strLen, ATC_EXTENDED_RESULT_STR_LEN, ATC_IND_RESP_MAX_STR_LEN);
                bFailed = TRUE;
            }
            else
            {
                pAtChanEty->callBack.pBufResp = (CHAR *)OsaAllocMemory(strLen + ATC_EXTENDED_RESULT_STR_LEN);

                OsaDebugBegin(pAtChanEty->callBack.pBufResp != PNULL, strLen, ATC_EXTENDED_RESULT_STR_LEN, 0);
                bFailed = TRUE;
                OsaDebugEnd();

                pAtChanEty->callBack.respBufLen     = strLen + ATC_EXTENDED_RESULT_STR_LEN;
                pAtChanEty->callBack.respBufOffset  = 0;
            }
        }
        else
        {
            OsaCheck(pAtChanEty->callBack.respBufLen > pAtChanEty->callBack.respBufOffset && pAtChanEty->callBack.respBufOffset > 0,
                     pAtChanEty->callBack.respBufLen, pAtChanEty->callBack.respBufOffset, strLen);
            // check whether the resp buffer is enough
            if (pAtChanEty->callBack.respBufOffset + strLen >= pAtChanEty->callBack.respBufLen)
            {
                /* reserved buffer is not enough */
                if (pAtChanEty->callBack.respBufOffset + strLen + ATC_EXTENDED_RESULT_STR_LEN > ATC_IND_RESP_MAX_STR_LEN)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_EXEC, atcSendRespStr_len_warn_2, P_WARNING, 4,
                                "AT CMD, RIL response length too long, %d + %d + %d > MAX: %d ",
                                pAtChanEty->callBack.respBufOffset, strLen, ATC_EXTENDED_RESULT_STR_LEN, ATC_IND_RESP_MAX_STR_LEN);
                    bFailed = TRUE;
                }
                else
                {
                    pNewRespBuf = (CHAR *)OsaAllocMemory(pAtChanEty->callBack.respBufOffset + strLen + ATC_EXTENDED_RESULT_STR_LEN);

                    if (pNewRespBuf != PNULL)
                    {
                        /*copy the old response*/
                        memcpy(pNewRespBuf,
                               pAtChanEty->callBack.pBufResp,
                               pAtChanEty->callBack.respBufOffset);

                        OsaFreeMemory(&(pAtChanEty->callBack.pBufResp));
                        pAtChanEty->callBack.pBufResp = pNewRespBuf;

                        pAtChanEty->callBack.respBufLen = pAtChanEty->callBack.respBufOffset + strLen + ATC_EXTENDED_RESULT_STR_LEN;
                        //respBufOffset not set
                    }
                    else
                    {
                        OsaDebugBegin(FALSE, strLen, ATC_EXTENDED_RESULT_STR_LEN, pAtChanEty->callBack.respBufOffset);
                        OsaDebugEnd();
                        bFailed = TRUE;
                    }
                }
            }
        }

        if (bFailed)
        {
            ECOMM_TRACE(UNILOG_ATCMD_EXEC, atcSendRespStr_warn_1, P_WARNING, 0,
                        "AT CMD, RIL response failed, send final: \\r\\nERROR\r\n");

            atcRespStrOutput(pAtChanEty, "\r\nERROR\r\n", strlen("\r\nERROR\r\n"));

            bFinalPrint = TRUE;
        }
        else
        {
            //pending it
            OsaCheck(pAtChanEty->callBack.pBufResp != PNULL && pAtChanEty->callBack.respBufOffset + strLen < pAtChanEty->callBack.respBufLen,
                     pAtChanEty->callBack.respBufOffset, strLen, pAtChanEty->callBack.respBufLen);

            memcpy(pAtChanEty->callBack.pBufResp + pAtChanEty->callBack.respBufOffset,
                   pStr,
                   strLen);

            pAtChanEty->callBack.respBufOffset += strLen;
        }
    }
    else //bFinalPrint == TRUE
    {
        /* check whether pending before */
        if (pAtChanEty->callBack.pBufResp == PNULL)
        {
            atcRespStrOutput(pAtChanEty, pStr, strLen);
        }
        else
        {
            OsaCheck(pAtChanEty->callBack.respBufLen > pAtChanEty->callBack.respBufOffset && pAtChanEty->callBack.respBufOffset > 0,
                     pAtChanEty->callBack.respBufLen, pAtChanEty->callBack.respBufOffset, strLen);

            if (pAtChanEty->callBack.respBufOffset + strLen >= pAtChanEty->callBack.respBufLen)
            {
                /* need to allocate new */
                if (pAtChanEty->callBack.respBufOffset + strLen + 1 > ATC_IND_RESP_MAX_STR_LEN)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_EXEC, atcSendRespStr_len_warn_3, P_WARNING, 3,
                                "AT CMD, RIL response length too long, %d + %d + 1 > MAX: %d ",
                                pAtChanEty->callBack.respBufOffset, strLen, ATC_IND_RESP_MAX_STR_LEN);
                    bFailed = TRUE;
                }
                else
                {
                    pNewRespBuf = (CHAR *)OsaAllocMemory(pAtChanEty->callBack.respBufOffset + strLen + 1);  /* +1, add '\0' */

                    if (pNewRespBuf != PNULL)
                    {
                        /*copy the old response*/
                        memcpy(pNewRespBuf,
                               pAtChanEty->callBack.pBufResp,
                               pAtChanEty->callBack.respBufOffset);

                        OsaFreeMemory(&(pAtChanEty->callBack.pBufResp));
                        pAtChanEty->callBack.pBufResp = pNewRespBuf;

                        pAtChanEty->callBack.respBufLen = pAtChanEty->callBack.respBufOffset + strLen + 1;
                        //respBufOffset not set
                    }
                    else
                    {
                        OsaDebugBegin(FALSE, strLen, pAtChanEty->callBack.respBufOffset, 1);
                        OsaDebugEnd();
                        bFailed = TRUE;
                    }
                }
            }
            //else: pAtChanEty->callBack.respBufOffset + strLen < pAtChanEty->callBack.respBufLen

            if (bFailed)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, atcSendRespStr_warn_2, P_WARNING, 0,
                            "AT CMD, RIL last response failed, send final: \\r\\nERROR\r\n");

                atcRespStrOutput(pAtChanEty, "\r\nERROR\r\n", strlen("\r\nERROR\r\n"));
            }
            else
            {
                OsaCheck(pAtChanEty->callBack.pBufResp != PNULL && pAtChanEty->callBack.respBufOffset + strLen < pAtChanEty->callBack.respBufLen,
                         pAtChanEty->callBack.respBufOffset, strLen, pAtChanEty->callBack.respBufLen);

                memcpy(pAtChanEty->callBack.pBufResp + pAtChanEty->callBack.respBufOffset,
                       pStr,
                       strLen);

                pAtChanEty->callBack.respBufOffset += strLen;

                pAtChanEty->callBack.pBufResp[pAtChanEty->callBack.respBufOffset] = '\0';   //end '\0'

                atcRespStrOutput(pAtChanEty, pAtChanEty->callBack.pBufResp, pAtChanEty->callBack.respBufOffset);
            }
        }
    }

    if (bFinalPrint)
    {
        // need to free RESP buffer
        if (pAtChanEty->callBack.pBufResp != PNULL)
        {
            OsaFreeMemory(&(pAtChanEty->callBack.pBufResp));
        }

        /* reset response callback parameters */
        pAtChanEty->callBack.respFunc   = PNULL;
        pAtChanEty->callBack.pArg       = PNULL;
        pAtChanEty->callBack.respBufLen = 0;
        pAtChanEty->callBack.respBufOffset = 0;

        /*release semaphone*/
        osSemaphoreRelease(pAtChanEty->callBack.apiSem);

        pAtChanEty->callBack.apiSem = PNULL;
    }

    return CMS_RET_SUCC;
}



/**
  \fn           CmsRetId atcSendUrcStr(const AtChanEntityP pAtChanEty, const CHAR *pStr, UINT32 strLen)
  \brief        Send AT command response
  \param[in]
  \returns      void
*/
static CmsRetId atcSendUrcStr(const AtChanEntityP pAtChanEty, const CHAR *pStr, UINT32 strLen)
{
    OsaCheck(pAtChanEty != PNULL && pStr != PNULL, pAtChanEty, pStr, 0);

    if (pStr[0] == '\0')
    {
        OsaDebugBegin(FALSE, pStr[0], pStr[1], pAtChanEty->chanId);
        OsaDebugEnd();

        return CMS_RET_SUCC;
    }

    OsaDebugBegin(pAtChanEty->callBack.urcFunc != PNULL, pAtChanEty->chanId, strLen, 0);
    return CMS_FAIL;
    OsaDebugEnd();

    if (strLen == 0)
    {
        strLen = strlen(pStr);
    }

    (pAtChanEty->callBack.urcFunc)(pStr, strLen);

    return CMS_RET_SUCC;
}

/**
  \fn           CmsRetId atcSendInfoText(const AtChanEntityP pAtChanEty, const CHAR *pStr, UINT32 strLen, BOOL bFinalPrint, BOOL bContinue)
  \brief        Send AT "information text" response
  \param[in]
  \returns      INT32
*/
static CmsRetId atcSendInfoText(const AtChanEntityP pAtChanEty, const CHAR *pStr, UINT32 strLen, BOOL bFinalPrint, BOOL bContinue)
{
    /*
     * "information text" three parts: a header, text, and a trailer
     *  a> ATV0: <text><S3><S4>            (normal: <text>\r\n)
     *  b> ATV1: <S3><S4><text><S3><S4>    (normal: \r\n<text>\r\n)
     *  c> information text returned in response to manufacturer-specific commands
     *     may contain multiple lines, may therefore include: CR,LF (\r\n)
    */

    /*
     * For some AT command response, maybe contained serveral lines, such as:
     * AT+CGPADDR
     * text = +CGPADDR: <cid>[,<PDP_addr_1>[,<PDP_addr_2>]]
     * <CR><LF>+CGPADDR: <cid>,[<PDP_addr_1>[,<PDP_addr_2>]]
     *
     * so:
     * a> ATV0:  return:
     *     +CGPADDR: <cid>[,<PDP_addr_1>[,<PDP_addr_2>]]
     *     <CR><LF>+CGPADDR: <cid>,[<PDP_addr_1>[,<PDP_addr_2>]]<S3><S4>
     * b> ATV1: return:
     *     <S3><S4>+CGPADDR: <cid>[,<PDP_addr_1>[,<PDP_addr_2>]]
     *     <CR><LF>+CGPADDR: <cid>,[<PDP_addr_1>[,<PDP_addr_2>]]<S3><S4>
     *
     * For easy, (not support: ATV & ATS command now), no matter the ATV & ATS setting, all return:
     *  <CR><LF>+CGPADDR: <cid>[,<PDP_addr_1>[,<PDP_addr_2>]]       ==> bContinue = TRUE
     *  <CR><LF>+CGPADDR: <cid>,[<PDP_addr_1>[,<PDP_addr_2>]]<S3><S4>   ==> bContiune = FALSE
    */


    const CHAR *pStart = pStr;
    UINT32  validStrLen = strLen;
    CHAR    *pBuf = PNULL, *pPrint = PNULL;
    UINT32  printLen = 0;
    CmsRetId    rcCode = CMS_RET_SUCC;

    #ifdef  FEATURE_REF_AT_ENABLE
    const CHAR *pDelSpace = PNULL;  /* for some ref AT, no space/blank character after colon (':') */
    #endif


    OsaDebugBegin(pStr != PNULL && strLen > 0 && pAtChanEty != PNULL, pAtChanEty, pStr, strLen);
    return CMS_RET_SUCC;
    OsaDebugEnd();

    /*
     * if suppressed, whether need to suppressed the "information text" response? - NO
     * 1) ATQ:
     *   a> 0  DCE transmits result codes.
     *   b> 1  Result codes are suppressed and not transmitted
     * 2) Two types of responses: "information text" and "result codes"
     *   so here, "information text" should not be suppressed by ATQ
    */
    #if 0
    if (pAtChanEty->cfg.suppressValue)
    {
    }
    #endif

    #ifdef  FEATURE_REF_AT_ENABLE
    pDelSpace = atcNeedDelSpaceBeforeColon(pStr);

    if (pDelSpace == PNULL)  //not need to clear space/blank char
    {
    #endif

    if (bContinue)
    {
        OsaCheck(bFinalPrint == FALSE, bFinalPrint, bContinue, 0);

        /* <CR><LF><text> */
        if (strLen > 2 &&
            pStr[0] == '\r' && pStr[1] == '\n' &&
            pStr[strLen-1] != '\r' && pStr[strLen-1] != '\n')
        {
            // already added <\r><\n>
            ECOMM_STRING(UNILOG_ATCMD_EXEC, atcSendInfoText_cont_1, P_SIG,
                         "AT CMD, RESP: %s", (const UINT8 *)(pStr+2));

            return atcSendRespStr(pAtChanEty, pStr, strLen, bFinalPrint);
        }
    }
    else if (pAtChanEty->cfg.respFormat) //ATV1, default value
    {
        /* <S3><S4><text><S3><S4> */
        if (strLen > 4 &&
            pStr[0] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] &&
            pStr[1] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX] &&
            pStr[strLen-2] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] &&
            pStr[strLen-1] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX])
        {
            // already added S3S4
            ECOMM_STRING(UNILOG_ATCMD_EXEC, atcSendInfoText_1, P_SIG,
                         "AT CMD, RESP: %s", (const UINT8 *)(pStr+2));

            return atcSendRespStr(pAtChanEty, pStr, strLen, bFinalPrint);
        }
    }
    else    //ATV0
    {
        /* <text><S3><S4> */
        if (strLen > 2 &&
            pStr[0] != pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] &&
            pStr[0] != pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX] &&
            pStr[strLen-2] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] &&
            pStr[strLen-1] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX])
        {
            // already added S3S4
            ECOMM_STRING(UNILOG_ATCMD_EXEC, atcSendInfoText_2, P_SIG,
                         "AT CMD, RESP: %s", (const UINT8 *)(pStr));

            return atcSendRespStr(pAtChanEty, pStr, strLen, bFinalPrint);
        }
    }
    #ifdef  FEATURE_REF_AT_ENABLE
    }
    #endif

    /* remove header & tailer: S3/S4 */
    if (pStr[0] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] ||
        pStr[0] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX])
    {
        pStart = pStart + 1;
        validStrLen -= 1;

        if (validStrLen > 0 &&
            (pStr[1] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] ||
             pStr[1] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX]))
        {
            pStart = pStart + 1;
            validStrLen -= 1;
        }
    }

    if (validStrLen > 0)
    {
        if (validStrLen > 0 &&
            (pStr[strLen-1] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] ||
             pStr[strLen-1] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX]))
        {
            validStrLen -= 1;

            if (validStrLen > 0 &&
                (pStr[strLen-2] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] ||
                 pStr[strLen-2] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX]))
            {
                validStrLen -= 1;
            }
        }
    }

    OsaDebugBegin(validStrLen > 0, *(UINT32 *)pStr, *(UINT32 *)(pStr+strLen-2), validStrLen);
    return CMS_RET_SUCC;
    OsaDebugEnd();

    /*
     * allo a new buffer
    */
    pBuf = (CHAR *)OsaAllocMemory(validStrLen + 4 + 1); /*4: <S3><S4><S3><S4>, 1: '\0'*/
    OsaDebugBegin(pBuf != PNULL, validStrLen, 0, 0);
    return CMS_RET_SUCC;
    OsaDebugEnd();

    /*
     * example:
     * pStr = "\n+CEREG: 1,"68CB","0C992A41",9\r\n"
     *           ^      ^                    ^
     *           |pStart|                    |
     *           |      |pDelSpace           |
     *           |<------------------------->| -> validStrLen
    */

    if (bContinue)
    {
        /* <\r><\n><text> */
        pBuf[0] = '\r';
        pBuf[1] = '\n';
        printLen = 2;
        pPrint = pBuf+2;

        #ifdef  FEATURE_REF_AT_ENABLE
        {
            if (pDelSpace != PNULL)
            {
                OsaCheck(pDelSpace > pStart && validStrLen >= (UINT32)(pDelSpace - pStart),
                         pDelSpace, pStart, validStrLen);

                /* first part: AT name */
                strncpy(pBuf+printLen, pStart, (UINT32)(pDelSpace - pStart));
                printLen += (UINT32)(pDelSpace - pStart);

                /* space/blank delete */

                /* end part: AT value */
                if (validStrLen > (UINT32)(pDelSpace + 1 - pStart))
                {
                    strncpy(pBuf+printLen, pDelSpace + 1, validStrLen - (UINT32)(pDelSpace+1-pStart));
                    printLen += (validStrLen - (UINT32)(pDelSpace+1-pStart));
                }
            }
            else
            {
                strncpy(pBuf+printLen, pStart, validStrLen);
                printLen += validStrLen;
            }
        }
        #else
        {
            strncpy(pBuf+printLen, pStart, validStrLen);
            printLen += validStrLen;
        }
        #endif
    }
    else if (pAtChanEty->cfg.respFormat) //ATV1, default value
    {
        /* <S3><S4><text><S3><S4> */
        pBuf[0] = pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX];
        pBuf[1] = pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX];
        printLen = 2;
        pPrint = pBuf+2;

        #ifdef  FEATURE_REF_AT_ENABLE
        {
            if (pDelSpace != PNULL)
            {
                OsaCheck(pDelSpace > pStart && validStrLen >= (UINT32)(pDelSpace - pStart),
                         pDelSpace, pStart, validStrLen);

                /* first part: AT name */
                strncpy(pBuf+printLen, pStart, (UINT32)(pDelSpace - pStart));
                printLen += (UINT32)(pDelSpace - pStart);

                /* space/blank delete */

                /* end part: AT value */
                if (validStrLen > (UINT32)(pDelSpace + 1 - pStart))
                {
                    strncpy(pBuf+printLen, pDelSpace + 1, validStrLen - (UINT32)(pDelSpace+1-pStart));
                    printLen += (validStrLen - (UINT32)(pDelSpace+1-pStart));
                }
            }
            else
            {
                strncpy(pBuf+printLen, pStart, validStrLen);
                printLen += validStrLen;
            }
        }
        #else
        strncpy(pBuf+printLen, pStart, validStrLen);
        printLen += validStrLen;
        #endif

        pBuf[printLen] = pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX];
        pBuf[printLen+1] = pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX];

        printLen += 2;
    }
    else
    {
        pPrint = pBuf;

        /* <text><S3><S4> */
        #ifdef  FEATURE_REF_AT_ENABLE
        printLen = 0;
        if (pDelSpace != PNULL)
        {
            OsaCheck(pDelSpace > pStart && validStrLen >= (UINT32)(pDelSpace - pStart),
                     pDelSpace, pStart, validStrLen);

            /* first part: AT name */
            strncpy(pBuf+printLen, pStart, (UINT32)(pDelSpace - pStart));
            printLen += (UINT32)(pDelSpace - pStart);

            /* space/blank delete */

            /* end part: AT value */
            if (validStrLen > (UINT32)(pDelSpace + 1 - pStart))
            {
                strncpy(pBuf+printLen, pDelSpace + 1, validStrLen - (UINT32)(pDelSpace+1-pStart));
                printLen += (validStrLen - (UINT32)(pDelSpace+1-pStart));
            }
        }
        else
        {
            strncpy(pBuf+printLen, pStart, validStrLen);
            printLen += validStrLen;
        }
        #else
        strncpy(pBuf, pStart, validStrLen);
        printLen = validStrLen;
        #endif

        pBuf[printLen] = pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX];
        pBuf[printLen+1] = pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX];

        printLen += 2;
    }

    pBuf[printLen] = '\0';

    ECOMM_STRING(UNILOG_ATCMD_EXEC, atcSendInfoText_3, P_SIG,
                 "AT CMD, RESP: %s", (const UINT8 *)(pPrint));


    rcCode = atcSendRespStr(pAtChanEty, pBuf, printLen, bFinalPrint);

    /* free memory */
    OsaFreeMemory(&pBuf);

    return rcCode;
}


/**
  \fn           CmsRetId atcSendInfoText(const AtChanEntityP pAtChanEty, const CHAR *pStr, UINT32 strLen)
  \brief        Send AT "information text" response
  \param[in]
  \returns      CmsRetId
*/
static CmsRetId atcSendUrcResult(const AtChanEntityP pAtChanEty, const CHAR *pStr, UINT32 strLen)
{
    /*
     * 1) Three types of result codes: "final", "intermediate", and "unsolicited".
     *    i> "final": indicates the completion of a full DCE action and a willingness
     *                to accept new commands from the DTE
     *    ii> "intermediate": suc as: the DCE moves from "command state" to
     *                        "online data state", and issues a: "CONNECT"
     *    iii> "Unsolicited": URC
     *
     * 2) In fact: URC is a type of result code, so the format better follow:
     *   a> ATV0: <numeric code><S3>        (normal: <numeric code>\r)
     *       example: 0\r (OK)
     *
     *   b> ATV1: <S3><S4><verbose code><S3><S4>    (normal: \r\n<verbose code>\r\n)
     *       example: \r\nOK\r\n
     *
     * 3) But URC don't has "numeric" format, then how to? - ignore the ATV value, all use ATV1 format.
     *
    */

    const CHAR *pStart = pStr;
    const CHAR *pTraceChr = pStr;
    UINT32  validStrLen = strLen;
    CHAR    *pBuf = PNULL;
    UINT32  printLen = 0;
    CmsRetId    rcCode = CMS_RET_SUCC;

    #ifdef  FEATURE_REF_AT_ENABLE
    const CHAR *pDelSpace = PNULL;  /* for some ref AT, no space/blank character after colon (':') */
    #endif

    OsaDebugBegin(pStr != PNULL && strLen > 0 && pAtChanEty != PNULL, pAtChanEty, pStr, strLen);
    return CMS_RET_SUCC;
    OsaDebugEnd();

    #ifdef  FEATURE_REF_AT_ENABLE
    pDelSpace = atcNeedDelSpaceBeforeColon(pStr);
    #endif

    /*
     * TRACE
    */
    if (pStr[0] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] ||
        pStr[0] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX])
    {
        pTraceChr = pTraceChr + 1;

        if ((pStr[1] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] ||
             pStr[1] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX]))
        {
            pTraceChr = pTraceChr + 1;
        }
    }

    /*
     * if suppressed, whether need to suppressed the "URC" response? - YE
     * 1) ATQ:
     *   a> 0  DCE transmits result codes.
     *   b> 1  Result codes are suppressed and not transmitted
     * 2) "Unsolicited"(URC) is one type of result codes.
    */
    if (pAtChanEty->cfg.suppressValue == 1)
    {
        ECOMM_STRING(UNILOG_ATCMD_EXEC, atcSendUrcResult_1, P_SIG,
                     "AT CMD, URC: %s", (const UINT8 *)pTraceChr);

        ECOMM_TRACE(UNILOG_ATCMD_EXEC, atcSendUrcResult_2, P_INFO, 2,
                    "AT CMD, chanId: %d is suppressed, not need to send the URC", pAtChanEty->chanId);
        return CMS_RET_SUCC;
    }

    #ifdef  FEATURE_REF_AT_ENABLE
    if (pDelSpace == PNULL)     //not need to delete space/blank char
    {
    #endif
    /*
     * ATV1: <S3><S4><verbose code><S3><S4>    (normal: \r\n<verbose code>\r\n)
     *       example: \r\n+CESQ: 99,99,99,99,27,45\r\n
    */
    if (strLen > 4 &&
        pStr[0] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] &&
        pStr[1] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX] &&
        pStr[strLen-2] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] &&
        pStr[strLen-1] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX])
    {
        // already added S3S4
        ECOMM_STRING(UNILOG_ATCMD_EXEC, atcSendUrcResult_str_1, P_SIG,
                     "AT CMD, URC: %s", (const UINT8 *)pTraceChr);

        return atcSendUrcStr(pAtChanEty, pStr, strLen);
    }
    #ifdef  FEATURE_REF_AT_ENABLE
    }
    #endif

    /* remove header & tailer: S3/S4 */
    if (pStr[0] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] ||
        pStr[0] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX])
    {
        pStart = pStart + 1;
        validStrLen -= 1;

        if (validStrLen > 0 &&
            (pStr[1] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] ||
             pStr[1] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX]))
        {
            pStart = pStart + 1;
            validStrLen -= 1;
        }
    }

    if (validStrLen > 0)
    {
        if (validStrLen > 0 &&
            (pStr[strLen-1] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] ||
             pStr[strLen-1] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX]))
        {
            validStrLen -= 1;

            if (validStrLen > 0 &&
                (pStr[strLen-2] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] ||
                 pStr[strLen-2] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX]))
            {
                validStrLen -= 1;
            }
        }
    }

    OsaDebugBegin(validStrLen > 0, *(UINT32 *)pStr, *(UINT32 *)(pStr+strLen-2), validStrLen);
    return CMS_RET_SUCC;
    OsaDebugEnd();

    /*
     * example:
     * pStr = "\n+CEREG: 1,"68CB","0C992A41",9\r\n"
     *           ^      ^                    ^
     *           |pStart|                    |
     *           |      |pDelSpace           |
     *           |<------------------------->| -> validStrLen
    */

    /*
     * allo a new buffer
    */
    pBuf = (CHAR *)OsaAllocMemory(validStrLen + 4 + 1); /*4: <S3><S4><S3><S4>, 1: '\0'*/
    OsaDebugBegin(pBuf != PNULL, validStrLen, 0, 0);
    return CMS_RET_SUCC;
    OsaDebugEnd();

    /* <S3><S4><text><S3><S4> */
    pBuf[0] = pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX];
    pBuf[1] = pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX];
    printLen = 2;

    #ifdef FEATURE_REF_AT_ENABLE
    {
        if (pDelSpace != PNULL)
        {
            OsaCheck(pDelSpace > pStart && validStrLen >= (UINT32)(pDelSpace - pStart),
                     pDelSpace, pStart, validStrLen);

            /* first part: AT name */
            strncpy(pBuf+printLen, pStart, (UINT32)(pDelSpace - pStart));
            printLen += (UINT32)(pDelSpace - pStart);

            /* space/blank delete */

            /* end part: AT value */
            if (validStrLen > (UINT32)(pDelSpace + 1 - pStart))
            {
                strncpy(pBuf+printLen, pDelSpace + 1, validStrLen - (UINT32)(pDelSpace+1-pStart));
                printLen += (validStrLen - (UINT32)(pDelSpace+1-pStart));
            }
        }
        else
        {
            strncpy(pBuf+printLen, pStart, validStrLen);
            printLen += validStrLen;
        }
    }
    #else
    strncpy(pBuf+printLen, pStart, validStrLen);
    printLen += validStrLen;
    #endif

    pBuf[printLen] = pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX];
    pBuf[printLen+1] = pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX];

    printLen += 2;

    pBuf[printLen] = '\0';

    ECOMM_STRING(UNILOG_ATCMD_EXEC, atcSendUrcResult_str_2, P_SIG,
                 "AT CMD, URC: %s", (const UINT8 *)(pBuf+2));

    rcCode = atcSendUrcStr(pAtChanEty, pBuf, printLen);

    /* free memory */
    OsaFreeMemory(&pBuf);

    return rcCode;
}


/**
  \fn           CmsRetId atcCMEECode(const AtChanEntityP pAtChanEty, UINT32 errCode)
  \brief        send extended result code: +CME ERROR x
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    resCode         type of result code
  \returns      CmsRetId
*/
static CmsRetId atcCMEECode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR        errStr[ATC_EXTENDED_RESULT_STR_LEN] = {0};
    CmsRetId    ret = CMS_RET_SUCC;

    UINT8       cmeModeType = mwGetAtChanConfigItemValue(pAtChanEty->chanId, MID_WARE_AT_CHAN_CMEE_CFG);   //AtcCMEERetType


    /* if fact, chanID should get from "atHandle", as now only one channel: CMS_CHAN_DEFAULT */
    OsaDebugBegin(cmeModeType <= ATC_CMEE_VERBOSE_ERROR_CODE, cmeModeType, ATC_CMEE_VERBOSE_ERROR_CODE, 0);
    cmeModeType = ATC_CMEE_NUM_ERROR_CODE;
    OsaDebugEnd();


    if (cmeModeType == ATC_CMEE_DISABLE_ERROR_CODE)
    {
        snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\nERROR\r\n");
    }
    else if (cmeModeType == ATC_CMEE_NUM_ERROR_CODE)
    {
        snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", errCode);
    }
    else
    {
        snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %s\r\n", atcGetCMEEStr(errCode));
    }

    ret = atcSendResultCode(pAtChanEty, AT_RC_CME_ERROR, errStr);

    return ret;
}


/**
  \fn           CmsRetId atcCMECode(const AtChanEntityP pAtChanEty, UINT32 errCode)
  \brief        send extended result code: +CMS ERROR x
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    resCode         type of result code
  \returns      CmsRetId
*/
static CmsRetId atcCMSECode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR        errStr[ATC_EXTENDED_RESULT_STR_LEN] = {0};
    CmsRetId    ret = CMS_RET_SUCC;

    UINT8       cmeModeType = mwGetAtChanConfigItemValue(pAtChanEty->chanId, MID_WARE_AT_CHAN_CMEE_CFG);   //AtcCMEERetType


    /* if fact, chanID should get from "atHandle", as now only one channel: CMS_CHAN_DEFAULT */
    OsaDebugBegin(cmeModeType <= ATC_CMEE_VERBOSE_ERROR_CODE, cmeModeType, ATC_CMEE_VERBOSE_ERROR_CODE, 0);
    cmeModeType = ATC_CMEE_NUM_ERROR_CODE;
    OsaDebugEnd();

    if(cmeModeType == ATC_CMEE_DISABLE_ERROR_CODE)
    {
        snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\nERROR\r\n");
        ret = atcSendResultCode(pAtChanEty, AT_RC_CMS_ERROR, errStr);
        return ret;
    }

    switch(errCode)
    {
        case CMS_SMS_ME_FAILURE:                                                                    //ME failure
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 300\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: ME failure\r\n");
            break;
        case CMS_SMS_SERVICE_OF_ME_RESV:                                                              //SMS service of ME reserved
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 301\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: SMS service of ME reserved\r\n");
            break;
        case CMS_SMS_OPERATION_NOT_ALLOWED:                                                 //operation not allowed
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 302\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: operation not allowed\r\n");
            break;
        case CMS_SMS_OPERATION_NOT_SUPPORTED:                                               //operation not supported
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 303\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: operation not supported\r\n");
            break;
        case CMS_SMS_INVALID_PDU_MODE_PARAMETER:                                                 //invalid PDU mode parameter
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 304\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: invalid PDU mode parameter\r\n");
            break;
        case CMS_SMS_INVALID_TEXT_MODE_PARAMETER:                                                //invalid text mode parameter
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 305\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: invalid text mode parameter\r\n");
            break;
        case CMS_SMS_USIM_NOT_INSERTED:                                         //(U)SIM not inserted
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 310\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: (U)SIM not inserted\r\n");
            break;
        case CMS_SMS_USIM_PIN_REQUIRED:                                                              //(U)SIM PIN required
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 311\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: (U)SIM PIN required\r\n");
            break;
        case CMS_SMS_PHSIM_PIN_REQUIRED:                                                   //PH-(U)SIM PIN required
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 312\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: PH-(U)SIM PIN required\r\n");
            break;
        case CMS_SMS_USIM_FAILURE:                                                                   //(U)SIM failure
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 313\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: (U)SIM failure\r\n");
            break;
        case CMS_SMS_USIM_BUSY:                                                                              //(U)SIM busy
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 314\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: (U)SIM busy\r\n");
            break;
        case CMS_SMS_USIM_WRONG:                                                                     //(U)SIM wrong
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 315\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: (U)SIM wrong\r\n");
            break;
        case CMS_SMS_USIM_PUK_REQUIRED:                                                              //(U)SIM PUK required
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 316\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: (U)SIM PUK required\r\n");
            break;
        case CMS_SMS_USIM_PIN2_REQUIRED:                                                     //(U)SIM PIN2 required
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 317\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: (U)SIM PIN2 required\r\n");
            break;
        case CMS_SMS_USIM_PUK2_REQUIRED:                                                     //(U)SIM PUK2 required
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 318\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: (U)SIM PUK2 required\r\n");
            break;
        case CMS_SMS_MEMORY_FAILURE:                                                                //memory failure
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 320\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: memory failure\r\n");
            break;
        case CMS_SMS_INVALID_MEM_INDEX:                                                  //invalid memory index
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 321\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: invalid memory index\r\n");
            break;
        case CMS_SMS_MEM_FULL:                                                                   //memory full
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 322\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: memory full\r\n");
            break;
        case CMS_SMS_SMSC_ADDR_UNKNOWN:                                                     //SMSC address unknown
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 330\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: SMSC address unknown\r\n");
            break;
        case CMS_SMS_NO_NETWORK_SERVICE:                                                                 //no network service
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 331\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: no network service\r\n");
            break;
        case CMS_SMS_NETWORK_TIMEOUT:                                                                    //network timeout
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 332\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: network timeout\r\n");
            break;
        case CMS_SMS_NO_CNMA_ACK_EXPECTED:                                                  //no +CNMA acknowledgement expected
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 340\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: no +CNMA acknowledgement expected\r\n");
            break;
        case CMS_SMS_UNKNOWN_ERROR:                                                                 //unknown error
        default:
            if(cmeModeType == 1)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: 500\r\n");
            else if(cmeModeType == 2)
                snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CMS ERROR: unknown error\r\n");
            break;
    }

    ret = atcSendResultCode(pAtChanEty, AT_RC_CMS_ERROR, errStr);

    return ret;
}


/**
  \fn           CmsRetId atcSOCKETECode(AtChanEntityP pAtChanEty, UINT32 errCode)
  \brief        send extended result code: +SOCKET ERROR x
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    resCode         type of result code
  \returns      CmsRetId
*/
static CmsRetId atcSOCKETECode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR        errStr[ATC_EXTENDED_RESULT_STR_LEN] = {0};
    CmsRetId    ret = 0;

    switch(errCode)
    {
        case CME_SOCK_MGR_ERROR_PARAM_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: PARAMETER ERROR\r\n");
            break;
        case CME_SOCK_MGR_ERROR_TOO_MUCH_INST:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: TOO MUCH SOCKET INSTANCE\r\n");
            break;
        case CME_SOCK_MGR_ERROR_CREATE_SOCK_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: CREATE SOCKET ERROR\r\n");
            break;
        case CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: OPERATION NO SUPPORT\r\n");
            break;
        case CME_SOCK_MGR_ERROR_NO_FIND_CLIENT:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: NO FIND CLIENT\r\n");
            break;
        case CME_SOCK_MGR_ERROR_CONNECT_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: CONNECT FAILED\r\n");
            break;
        case CME_SOCK_MGR_ERROR_BIND_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: BIND FAILED\r\n");
            break;
        case CME_SOCK_MGR_ERROR_SEND_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: SEND FAILED\r\n");
            break;
        case CME_SOCK_MGR_ERROR_NO_CONNECTED:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: SOCKET NOT CONNECTED\r\n");
            break;
        case CME_SOCK_MGR_ERROR_IS_CONNECTED:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: SOCKET ALREADY CONNECTED\r\n");
            break;
        case CME_SOCK_MGR_ERROR_INVALID_STATUS:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: SOCKET INVALID STATUS\r\n");
            break;
        case CME_SOCK_MGR_ERROR_CONNECT_TIMEOUT:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: SOCKET CONNECT TIMEOUT\r\n");
            break;
        case CME_SOCK_MGR_ERROR_DELETE_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: SOCKET DELETE FAILED\r\n");
            break;
        case CME_SOCK_MGR_ERROR_SOCK_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: SOCKET ACCOR ERROR \r\n");
            break;
        case CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: SOCKET SEND request fail \r\n");
            break;
        case CME_SOCK_MGR_ERROR_UNKNOWN:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: UNKNOWN\r\n");
            break;
        default:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+SOCKET ERROR: %d\r\n", errCode);
            break;
    }

    ret = atcSendResultCode(pAtChanEty, AT_RC_SOCKET_ERROR, errStr);

    return ret;
}


/**
  \fn           CmsRetId atcECSOCECode(AtChanEntityP pAtChanEty, UINT32 errCode)
  \brief        send extended result code: +CME ERROR x; for EC socket AT command
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    resCode         type of result code
  \returns      CmsRetId
*/
static CmsRetId atcECSOCECode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR        errStr[ATC_EXTENDED_RESULT_STR_LEN] = {0};
    CmsRetId    ret = 0;

    UINT8       cmeModeType = mwGetAtChanConfigItemValue(pAtChanEty->chanId, MID_WARE_AT_CHAN_CMEE_CFG);   //AtcCMEERetType

    if (cmeModeType == ATC_CMEE_DISABLE_ERROR_CODE)
    {
        snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\nERROR\r\n");

        ret = atcSendResultCode(pAtChanEty, AT_RC_SOCKET_ERROR, errStr);

        return ret;
    }

    switch(errCode)
    {
        case CME_SOCK_MGR_ERROR_PARAM_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_PARAM_ERROR);
            break;
        case CME_SOCK_MGR_ERROR_TOO_MUCH_INST:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_TOO_MUCH_INST);
            break;
        case CME_SOCK_MGR_ERROR_CREATE_SOCK_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_CREATE_SOCK_ERROR);
            break;
        case CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT);
            break;
        case CME_SOCK_MGR_ERROR_NO_FIND_CLIENT:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_NO_FIND_CLIENT);
            break;
        case CME_SOCK_MGR_ERROR_CONNECT_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_CONNECT_FAIL);
            break;
        case CME_SOCK_MGR_ERROR_BIND_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_BIND_FAIL);
            break;
        case CME_SOCK_MGR_ERROR_SEND_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_SEND_FAIL);
            break;
        case CME_SOCK_MGR_ERROR_NO_CONNECTED:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_NO_CONNECTED);
            break;
        case CME_SOCK_MGR_ERROR_IS_CONNECTED:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_IS_CONNECTED);
            break;
        case CME_SOCK_MGR_ERROR_INVALID_STATUS:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_INVALID_STATUS);
            break;
        case CME_SOCK_MGR_ERROR_CONNECT_TIMEOUT:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_CONNECT_TIMEOUT);
            break;
        case CME_SOCK_MGR_ERROR_DELETE_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_DELETE_FAIL);
            break;
        case CME_SOCK_MGR_ERROR_SOCK_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d \r\n", CME_SOCK_MGR_ERROR_SOCK_ERROR);
            break;
        case CME_SOCK_MGR_ERROR_NO_MEMORY:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d \r\n", CME_SOCK_MGR_ERROR_NO_MEMORY);
            break;
        case CME_SOCK_MGR_ERROR_NO_MORE_DL_BUFFER_RESOURCE:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d \r\n", CME_SOCK_MGR_ERROR_NO_MORE_DL_BUFFER_RESOURCE);
            break;
        case CME_SOCK_MGR_ERROR_CONNECT_IS_ONGOING:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d \r\n", CME_SOCK_MGR_ERROR_CONNECT_IS_ONGOING);
            break;
        case CME_SOCK_MGR_ERROR_UL_SEQUENCE_INVALID:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d \r\n", CME_SOCK_MGR_ERROR_UL_SEQUENCE_INVALID);
            break;
        case CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d \r\n", CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL);
            break;
        case CME_SOCK_MGR_ERROR_UNKNOWN:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_UNKNOWN);
            break;
        default:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", errCode);
            break;
    }

    ret = atcSendResultCode(pAtChanEty, AT_RC_ECSOC_ERROR, errStr);

    return ret;
}


/**
  \fn           CmsRetId atcECSRVSOCECode(AtChanEntityP pAtChanEty, UINT32 errCode)
  \brief        send extended result code: +CME ERROR x; for EC socket AT command
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    resCode         type of result code
  \returns      CmsRetId
*/
static CmsRetId atcECSRVSOCECode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR        errStr[ATC_EXTENDED_RESULT_STR_LEN] = {0};
    CmsRetId    ret = 0;

    UINT8       cmeModeType = mwGetAtChanConfigItemValue(pAtChanEty->chanId, MID_WARE_AT_CHAN_CMEE_CFG);   //AtcCMEERetType

    if (cmeModeType == ATC_CMEE_DISABLE_ERROR_CODE)
    {
        snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\nERROR\r\n");

        ret = atcSendResultCode(pAtChanEty, AT_RC_SOCKET_ERROR, errStr);

        return ret;
    }

    switch(errCode)
    {
        case CME_SOCK_MGR_ERROR_PARAM_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_PARAM_ERROR);
            break;
        case CME_SOCK_MGR_ERROR_TOO_MUCH_INST:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_TOO_MUCH_INST);
            break;
        case CME_SOCK_MGR_ERROR_CREATE_SOCK_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_CREATE_SOCK_ERROR);
            break;
        case CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_OPERATION_NOT_SUPPORT);
            break;
        case CME_SOCK_MGR_ERROR_NO_FIND_CLIENT:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_NO_FIND_CLIENT);
            break;
        case CME_SOCK_MGR_ERROR_CONNECT_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_CONNECT_FAIL);
            break;
        case CME_SOCK_MGR_ERROR_BIND_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_BIND_FAIL);
            break;
        case CME_SOCK_MGR_ERROR_SEND_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_SEND_FAIL);
            break;
        case CME_SOCK_MGR_ERROR_NO_CONNECTED:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_NO_CONNECTED);
            break;
        case CME_SOCK_MGR_ERROR_IS_CONNECTED:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_IS_CONNECTED);
            break;
        case CME_SOCK_MGR_ERROR_INVALID_STATUS:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_INVALID_STATUS);
            break;
        case CME_SOCK_MGR_ERROR_CONNECT_TIMEOUT:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_CONNECT_TIMEOUT);
            break;
        case CME_SOCK_MGR_ERROR_DELETE_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_DELETE_FAIL);
            break;
        case CME_SOCK_MGR_ERROR_SOCK_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d \r\n", CME_SOCK_MGR_ERROR_SOCK_ERROR);
            break;
        case CME_SOCK_MGR_ERROR_NO_MEMORY:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d \r\n", CME_SOCK_MGR_ERROR_NO_MEMORY);
            break;
        case CME_SOCK_MGR_ERROR_NO_MORE_DL_BUFFER_RESOURCE:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d \r\n", CME_SOCK_MGR_ERROR_NO_MORE_DL_BUFFER_RESOURCE);
            break;
        case CME_SOCK_MGR_ERROR_CONNECT_IS_ONGOING:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d \r\n", CME_SOCK_MGR_ERROR_CONNECT_IS_ONGOING);
            break;
        case CME_SOCK_MGR_ERROR_UL_SEQUENCE_INVALID:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d \r\n", CME_SOCK_MGR_ERROR_UL_SEQUENCE_INVALID);
            break;
        case CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d \r\n", CME_SOCK_MGR_ERROR_SEND_REQUEST_FAIL);
            break;
        case CME_SOCK_MGR_ERROR_UNKNOWN:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", CME_SOCK_MGR_ERROR_UNKNOWN);
            break;
        default:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", errCode);
            break;
    }

    ret = atcSendResultCode(pAtChanEty, AT_RC_ECSOC_ERROR, errStr);

    return ret;
}


#ifdef FEATURE_CISONENET_ENABLE
static CmsRetId atcCISECode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR errStr[32];
    INT32 ret = 0;

#ifdef  FEATURE_REF_AT_ENABLE
    UINT8 cmeModeType = mwGetAtChanConfigItemValue(pAtChanEty->chanId, MID_WARE_AT_CHAN_CMEE_CFG);   //AtcCMEERetType


    /* if fact, chanID should get from "atHandle", as now only one channel: CMS_CHAN_DEFAULT */
    OsaDebugBegin(cmeModeType <= ATC_CMEE_VERBOSE_ERROR_CODE, cmeModeType, ATC_CMEE_VERBOSE_ERROR_CODE, 0);
    cmeModeType = ATC_CMEE_NUM_ERROR_CODE;
    OsaDebugEnd();


    if (cmeModeType == ATC_CMEE_DISABLE_ERROR_CODE)
    {
        snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\nERROR\r\n");
    }
    else
    {
        snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", errCode);
    }
#else
    sprintf(errStr, "+CIS ERROR: %d\r\n", errCode);
#endif
    ret = atcSendResultCode(pAtChanEty, AT_RC_CIS_ERROR, errStr);
    return ret;
}
#endif
static CmsRetId atcCTM2MECode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR errStr[ATC_EXTENDED_RESULT_STR_LEN];
    INT32 ret = 0;

#ifdef  FEATURE_REF_AT_ENABLE
    UINT8       cmeModeType = mwGetAtChanConfigItemValue(pAtChanEty->chanId, MID_WARE_AT_CHAN_CMEE_CFG);   //AtcCMEERetType


    /* if fact, chanID should get from "atHandle", as now only one channel: CMS_CHAN_DEFAULT */
    OsaDebugBegin(cmeModeType <= ATC_CMEE_VERBOSE_ERROR_CODE, cmeModeType, ATC_CMEE_VERBOSE_ERROR_CODE, 0);
    cmeModeType = ATC_CMEE_NUM_ERROR_CODE;
    OsaDebugEnd();


    if (cmeModeType == ATC_CMEE_DISABLE_ERROR_CODE)
    {
        snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\nERROR\r\n");
    }
    else
    {
        snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "\r\n+CME ERROR: %d\r\n", errCode);
    }
#else
    snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+CTM2M ERROR: %d\r\n", errCode);
#endif
    ret = atcSendResultCode(pAtChanEty, AT_RC_CTM2M_ERROR, errStr);
    return ret;
}

#ifdef FEATURE_WAKAAMA_ENABLE
static CmsRetId atcLWM2MECode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR errStr[80];
    INT32 ret = 0;

    switch(errCode)
    {
        case LWM2M_PARAM_ERROR:
            sprintf(errStr, "+LWM2M ERROR: PARAMETER ERROR\r\n");
            break;
        case LWM2M_SEMPH_ERROR:
            sprintf(errStr, "+LWM2M ERROR: CANNOT CREATE SEMPH\r\n");
            break;
        case LWM2M_CONFIG_ERROR:
            sprintf(errStr, "+LWM2M ERROR: CONFIG ERROR\r\n");
            break;
        case LWM2M_NO_FREE_CLIENT:
            sprintf(errStr, "+LWM2M ERROR: NO FREE CLIENT\r\n");
            break;
        case LWM2M_OPERATION_NOT_SUPPORT:
            sprintf(errStr, "+LWM2M ERROR: OPERATION NO SUPPORT\r\n");
            break;
        case LWM2M_NO_FIND_CLIENT:
            sprintf(errStr, "+LWM2M ERROR: NO FIND CLIENT\r\n");
            break;
        case LWM2M_ADDOBJ_FAILED:
            sprintf(errStr, "+LWM2M ERROR: ADD OBJECT FAILED\r\n");
            break;
        case LWM2M_NO_FIND_OBJ:
            sprintf(errStr, "+LWM2M ERROR: NO FIND OBJECT\r\n");
            break;
        case LWM2M_DELOBJ_FAILED:
            sprintf(errStr, "+LWM2M ERROR: DELETE OBJECT FAILED\r\n");
            break;
        case LWM2M_NO_NETWORK:
            sprintf(errStr, "+LWM2M ERROR: NETWORK NOT READY\r\n");
            break;
        case LWM2M_INTER_ERROR:
            sprintf(errStr, "+LWM2M ERROR: INTERNAL ERROR\r\n");
            break;
        case LWM2M_REG_BAD_REQUEST:
            sprintf(errStr, "+LWM2M ERROR: REGISTER FAILED, BAD REQUEST\r\n");
            break;
        case LWM2M_REG_FORBIDEN:
            sprintf(errStr, "+LWM2M ERROR: SERVER REGISTER REJECT \r\n");
            break;
        case LWM2M_REG_PRECONDITION:
            sprintf(errStr, "+LWM2M ERROR: REGISTER FAILED, BAD PARAMETER\r\n");
            break;
        case LWM2M_REG_TIMEOUT:
            sprintf(errStr, "+LWM2M ERROR: REGISTER TIMEOUT\r\n");
            break;
        case LWM2M_SESSION_INVALID:
            sprintf(errStr, "+LWM2M ERROR: SESSION INVALID\r\n");
            break;
        case LWM2M_ALREADY_ADD:
            sprintf(errStr, "+LWM2M ERROR: OBJECT ALREADY ADD\r\n");
            break;
        default:
            sprintf(errStr, "+LWM2M ERROR: %d\r\n", errCode);
            break;
    }

    ret = atcSendResultCode(pAtChanEty, AT_RC_LWM2M_ERROR, errStr);
    return ret;
}
#endif
#ifdef  FEATURE_HTTPC_ENABLE
static CmsRetId atcHTTPCode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR errStr[80];
    INT32 ret = 0;

    switch(errCode)
    {
        case HTTP_PARAM_ERROR:
            sprintf(errStr, "+HTTP ERROR: PARAMETER ERROR\r\n");
            break;
        case HTTP_CLIENTS_EXHAOUST:
            sprintf(errStr, "+HTTP ERROR: NUMBER OF CLIENTS REACH MAXIMUM\r\n");
            break;
        case HTTP_URL_FORMAT_ERROR:
            sprintf(errStr, "+HTTP ERROR: URL FORMAT ERROR\r\n");
            break;
        case HTTP_URL_PARSE_ERROR:
            sprintf(errStr, "+HTTP ERROR: URL PARSE ERROR\r\n");
            break;
        case HTTP_OPERATION_NOT_SUPPORT:
            sprintf(errStr, "+HTTP ERROR: OPERATION NOT SUPPORT\r\n");
            break;
        case HTTP_NO_FIND_CLIENT_ID:
            sprintf(errStr, "+HTTP ERROR: NO FIND CLIENT ID\r\n");
            break;
        case HTTP_ALREADY_CONNECTED:
            sprintf(errStr, "+HTTP ERROR: ALREADY CONNECTED\r\n");
            break;
        case HTTP_CONNECT_FAILED:
            sprintf(errStr, "+HTTP ERROR: CONNECT FAILED\r\n");
            break;
        case HTTP_SEND_FAILED:
            sprintf(errStr, "+HTTP ERROR: SEND FAILED\r\n");
            break;
        case HTTP_CMD_CONTIUE:
            sprintf(errStr, "+HTTP CMD: CONTIUE ENTER CMD\r\n");
            break;
        case HTTP_NO_NETWORK:
            sprintf(errStr, "+HTTP ERROR: NETWORK NOT READY\r\n");
            break;
        case HTTP_CLIENT_NOT_OPEN:
            sprintf(errStr, "+HTTP ERROR: CLIENT NOT CONNECTED\r\n");
            break;
        case HTTP_DESTORY_FAILED:
            sprintf(errStr, "+HTTP ERROR: CLIENT DESTORY FAILED\r\n");
            break;
        case HTTP_NOT_SUPPORT_HTTPS:
            sprintf(errStr, "+HTTP ERROR: NOT SUPPORT HTTPS\r\n");
            break;
        default:
            sprintf(errStr, "+HTTP ERROR: %d\r\n", errCode);
            break;
    }

    ret = atcSendResultCode(pAtChanEty, AT_RC_HTTP_ERROR, errStr);
    return ret;
}
#endif

#if defined(FEATURE_MQTT_ENABLE) || defined(FEATURE_MQTT_TLS_ENABLE)
static CmsRetId atcMQTTCode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR errStr[80];
    INT32 ret = 0;

    switch(errCode)
    {
        case MQTT_PARAM_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: PARAMETER ERROR\r\n");
            break;
        case MQTT_CREATE_CLIENT_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: CREATE CLIENT ERROR\r\n");
            break;
        case MQTT_CREATE_SOCK_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: CREATE SOCKET ERROR\r\n");
            break;
        case MQTT_CONNECT_TCP_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: CONNECT TCP FAIL\r\n");
            break;
        case MQTT_CONNECT_MQTT_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: MQTT IS CONNECTING\r\n");
            break;
        case MQTT_SUB_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: SUB FAIL\r\n");
            break;
        case MQTT_UNSUB_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: UNSUB FAIL\r\n");
            break;
        case MQTT_SEND_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: PUB FAIL\r\n");
            break;
        case MQTT_DELETE_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: DELETE FAIL\r\n");
            break;
        case MQTT_FIND_CLIENT_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: FIND CLIENT FAIL\r\n");
            break;
        case MQTT_NOT_SUPPORT:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: NOT SUPPORT\r\n");
            break;
        case MQTT_NOT_CONNECTED:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: NOT CONNECTED\r\n");
            break;
        case MQTT_INFO_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: GET INFO FAIL\r\n");
            break;
        case MQTT_NETWORK_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: NETWORK NOT READY\r\n");
            break;
        case MQTT_PARAM_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: PARAM WRONG\r\n");
            break;
        case MQTT_TASK_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: CREATE TASK FAIL\r\n");
            break;
        case MQTT_RECV_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: RECV ERROR\r\n");
            break;
        case MQTT_ALI_ENCRYP_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: ALI ENCRYP NOT ENABLE\r\n");
            break;
        default:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+MQTT ERROR: %d\r\n", errCode);
            break;
    }

    ret = atcSendResultCode(pAtChanEty, AT_RC_MQTT_ERROR, errStr);
    return ret;
}
#endif

#ifdef  FEATURE_LIBCOAP_ENABLE
#if 0
static CmsRetId atcCOAPCode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR errStr[80];
    INT32 ret = 0;

    switch(errCode)
    {
        case COAP_PARAM_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: PARAMETER ERR\r\n");
            break;
        case COAP_CREATE_CLIENT_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: CREATE CLIENT ERR\r\n");
            break;
        case COAP_CREATE_SOCK_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: CREATE SOCKET ERR\r\n");
            break;
        case COAP_CONNECT_UDP_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: CONNECT UDP FAIL\r\n");
            break;
        case COAP_CONNECT_COAP_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: CONNECT COAP FAIL\r\n");
            break;
        case COAP_SEND_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: SEND FAIL\r\n");
            break;
        case COAP_DELETE_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: DELETE FAIL\r\n");
            break;
        case COAP_FIND_CLIENT_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: FIND CLIENT FAIL\r\n");
            break;
        case COAP_NOT_SUPPORT:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: NOT SUPPORT\r\n");
            break;
        case COAP_NOT_CONNECTED:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: NOT CONNECTED\r\n");
            break;
        case COAP_INFO_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: GET INFO FAIL\r\n");
            break;
        case COAP_NETWORK_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: NETWORK NOT READY\r\n");
            break;
        case COAP_URI_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: URI ERR\r\n");
            break;
        case COAP_TASK_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: CREATE TASK FAIL\r\n");
            break;
        case COAP_RECREATE_CLIENT_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: RECREATE CLIENT ERR\r\n");
            break;
        case COAP_SEND_NOT_END_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: NETWORK IS BUSY, WAIT FOR SEND\r\n");
            break;
        case COAP_RECV_FAIL:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: RECV ERROR\r\n");
            break;
        default:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+COAP ERROR: %d\r\n", errCode);
            break;
    }

    ret = atcSendResultCode(pAtChanEty, AT_RC_COAP_ERROR, errStr);
    return ret;
}
#endif
#endif

#ifdef  FEATURE_ATADC_ENABLE
static CmsRetId atcADCCode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR errStr[80];
    INT32 ret = 0;

    switch(errCode)
    {
        case ADC_PARAM_ERROR:
            sprintf(errStr, "+ADC ERROR: PARAMETER ERROR\r\n");
            break;
        case ADC_OPERATION_NOT_SUPPORT:
            sprintf(errStr, "+ADC ERROR: NOT SUPPORT\r\n");
            break;
        case ADC_TASK_NOT_CREATE:
            sprintf(errStr, "+ADC ERROR: TASK NOT CREATE\r\n");
            break;
        case ADC_CONVERSION_TIMEOUT:
            sprintf(errStr, "+ADC ERROR: CONVERSION TIMEOUT\r\n");
            break;
        default:
            sprintf(errStr, "+ADC ERROR: %d\r\n", errCode);
            break;
    }

    ret = atcSendResultCode(pAtChanEty, AT_RC_ADC_ERROR, errStr);
    return ret;
}
#endif

#ifdef  FEATURE_EXAMPLE_ENABLE
static CmsRetId atcEXAMPLECode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR errStr[80];
    INT32 ret = 0;

    switch(errCode)
    {
        case EXAMPLE_PARAM_ERROR:
            sprintf(errStr, "+EC ERROR: PARAMETER ERROR\r\n");
            break;
        case EXAMPLE_OPERATION_NOT_SUPPORT:
            sprintf(errStr, "+EC ERROR: NOT SUPPORT\r\n");
            break;
        case EXAMPLE_TASK_NOT_CREATE:
            sprintf(errStr, "+EC ERROR: TASK NOT CREATE\r\n");
            break;
        default:
            sprintf(errStr, "+EC ERROR: %d\r\n", errCode);
            break;
    }

    ret = atcSendResultCode(pAtChanEty, AT_RC_EXAMPLE_ERROR, errStr);
    return ret;
}
#endif

#if defined(FEATURE_CTCC_DM_ENABLE) || defined(FEATURE_CUCC_DM_ENABLE) || defined(FEATURE_CMCC_DM_ENABLE)
static CmsRetId atcDMCode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR errStr[80];
    INT32 ret = 0;

    switch(errCode)
    {
        case DM_PARAM_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+DM ERROR: PARAMETER ERR\r\n");
            break;
        case DM_GET_VAL_ERROR:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+DM ERROR: GET VALUE ERR\r\n");
            break;
        case DM_NOT_SUPPORT:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+DM ERROR: NOT SUPPORT\r\n");
            break;
        default:
            snprintf(errStr, ATC_EXTENDED_RESULT_STR_LEN, "+DM ERROR: %d\r\n", errCode);
            break;
    }

    ret = atcSendResultCode(pAtChanEty, AT_RC_DM_ERROR, errStr);
    return ret;
}
#endif

static CmsRetId atcFWUPDCode(AtChanEntityP pAtChanEty, UINT32 errCode)
{
    CHAR errStr[64];
    INT32 ret = 0;

    ECOMM_TRACE(UNILOG_ATCMD_EXEC, atcFWUPDResult_1, P_INFO, 2,
                "chanId[%d]: +NFWUPD ERROR: errno(%d)", pAtChanEty->chanId, errCode);

    switch(errCode)
    {
        case FWUPD_EC_PARAM_INVALID:
            sprintf(errStr, "+NFWUPD ERROR: PARAM INVALID\r\n");
            break;
        case FWUPD_EC_OPER_UNSUPP:
            sprintf(errStr, "+NFWUPD ERROR: OPER UNSUPPORTED\r\n");
            break;

        case FWUPD_EC_PKGSZ_ERROR: /* fall through */
        case FWUPD_EC_PKGSN_ERROR:
        case FWUPD_EC_CRC8_ERROR:
        case FWUPD_EC_FLERASE_UNDONE:
        case FWUPD_EC_FLERASE_ERROR:
        case FWUPD_EC_FLWRITE_ERROR:
        case FWUPD_EC_FLREAD_ERROR:
        case FWUPD_EC_UNDEF_ERROR:
        default:
            sprintf(errStr, "ERROR\r\n");
            break;
    }

    ret = atcSendResultCode(pAtChanEty, AT_RC_FWUPD_ERROR, errStr);
    return ret;
}

/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS


/**
  \fn           CmsRetId atcSendResultCode(AtChanEntity *pAtChanEty, AtResultCode resCode, const CHAR *pResultStr)
  \brief        send the AT result code
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    resCode         type of result code
  \param[in]    pResultStr      result string, if present, not constuct the result code according to "resCode" in this
  \                             API, but send this string directly
  \returns      CmsRetId
*/
CmsRetId atcSendResultCode(AtChanEntity *pAtChanEty, AtResultCode resCode, const CHAR *pResultStr)
{
    CHAR    basicResultStr[ATC_BASIC_RESULT_STR_LEN] = {0};

    /*
     * "result codes" three parts: a header, the result text, and a trailer
     *  a> ATV0: <numeric code><S3>        (normal: <numeric code>\r)
     *     example: 0\r (OK)
     *
     *  b> ATV1: <S3><S4><verbose code><S3><S4>    (normal: \r\n<verbose code>\r\n)
     *     example: \r\nOK\r\n
     *
     * "Extended syntax result codes"
     *  a> "extended syntax result codes" format is the same as "result codes" regard to
     *     headers and trailers; (Note: also controlled by ATV)
     *  b> "extended syntax result codes" have no numeric equivalent,
     *      and are always issued in alphabetic form, so only: ===  ATV1 ===?
     *  c> "Extended syntax result codes" may be: "final", "intermediate", "unsolicited"
     *  d> "Extended syntax result codes" shall be prefixed by the "+" character
     *     example: \r\n+<name>\r\n  => \r\n+CME ERROR: <err>\r\n
    */

    /*
     * Need to check whether suppressed by ATQ
     * a)   0  DCE transmits result codes.
     * b)   1  Result codes are suppressed and not transmitted
     *
    */
    #if 1
    if (pAtChanEty->cfg.suppressValue == 1)
    {
        ECOMM_TRACE(UNILOG_ATCMD_EXEC, atcSendResultCode_warning_1, P_WARNING, 2,
                    "AT CMD, chanId: %d, result code: %d is suppressed, not need to send the result code",
                    pAtChanEty->chanId, resCode);
        return CMS_RET_SUCC;
    }
    #endif

    if (pResultStr == PNULL)
    {
        /* construct result code according to: resCode */
        switch (resCode)
        {
            case AT_RC_OK:
            {
                if (pAtChanEty->cfg.respFormat == 0)    //ATV0: <numeric code><S3>
                {
                    basicResultStr[0] = '0';
                    basicResultStr[1] = pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX];
                    basicResultStr[2] = 0;
                }
                else    //ATV1, <S3><S4><verbose code><S3><S4>
                {
                    snprintf(basicResultStr, ATC_BASIC_RESULT_STR_LEN, "%c%cOK%c%c",
                             pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX], pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX],
                             pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX], pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX]);
                }
                break;
            }

            case AT_RC_ERROR:
            {
                if (pAtChanEty->cfg.respFormat == 0)    //ATV0: <numeric code><S3>
                {
                    basicResultStr[0] = '4';
                    basicResultStr[1] = pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX];
                    basicResultStr[2] = 0;
                }
                else    //ATV1, <S3><S4><verbose code><S3><S4>
                {
                    snprintf(basicResultStr, ATC_BASIC_RESULT_STR_LEN, "%c%cERROR%c%c",
                             pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX], pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX],
                             pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX], pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX]);
                }

                break;
            }

            case AT_RC_CONNECT:
            case AT_RC_RING:
            case AT_RC_NO_CARRIER:
            case AT_RC_NO_DIALTONE:
            case AT_RC_BUSY:
            case AT_RC_NO_ANSWER:
            {
                OsaDebugBegin(FALSE, resCode, 0, 0);
                OsaDebugEnd();
                //NOT SUPPORT NOW, act as ERROR
                if (pAtChanEty->cfg.respFormat == 0)    //ATV0: <numeric code><S3>
                {
                    basicResultStr[0] = '4';
                    basicResultStr[1] = pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX];
                    basicResultStr[2] = 0;
                }
                else    //ATV1, <S3><S4><verbose code><S3><S4>
                {
                    snprintf(basicResultStr, ATC_BASIC_RESULT_STR_LEN, "%c%cERROR%c%c",
                             pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX], pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX],
                             pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX], pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX]);
                }
                break;
            }



            /*
            case AT_RC_CONTINUE:
                break;

            case AT_RC_NO_RESULT:
                break;
            */
            case AT_RC_CME_ERROR:
            case AT_RC_CMS_ERROR:
            {
                OsaDebugBegin(FALSE, resCode, 0, 0);
                OsaDebugEnd();
                /*
                 * should by extended result type, and: "pResultStr" should not be NULL,
                 *  ACT an ERROR
                */
                if (pAtChanEty->cfg.respFormat == 0)    //ATV0: <numeric code><S3>
                {
                    basicResultStr[0] = '4';
                    basicResultStr[1] = pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX];
                    basicResultStr[2] = 0;
                }
                else    //ATV1, <S3><S4><verbose code><S3><S4>
                {
                    snprintf(basicResultStr, ATC_BASIC_RESULT_STR_LEN, "%c%cERROR%c%c",
                             pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX], pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX],
                             pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX], pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX]);
                }
                break;
            }



            default:
                OsaDebugBegin(FALSE, resCode, 0, 0);
                OsaDebugEnd();

                break;
        }

        if (pAtChanEty->cfg.respFormat == 0)    //ATV0: <numeric code><S3>
        {
            ECOMM_STRING(UNILOG_ATCMD_EXEC, atcSendResultCode_1, P_SIG,
                         "AT CMD, RESP: %s", (const UINT8 *)(basicResultStr));
        }
        else    //ATV1, <S3><S4><verbose code><S3><S4>
        {
            ECOMM_STRING(UNILOG_ATCMD_EXEC, atcSendResultCode_2, P_SIG,
                         "AT CMD, RESP: %s", (const UINT8 *)(basicResultStr+2));
        }

        return atcSendRespStr(pAtChanEty, basicResultStr, strlen(basicResultStr), TRUE);
    }

    //else    //pResultStr != PNULL
    /*
     * "Extended syntax result codes" format, just the same as "information text", here, we call the same API
    */
    return atcSendInfoText(pAtChanEty, pResultStr, strlen(pResultStr), TRUE, FALSE);
}

/**
  \fn           CmsRetId atcSendRawRespStr(const AtChanEntityP pAtChanEty, const CHAR *pStr, UINT32 strLen)
  \brief        Send raw string via response channel
  \param[in]    pAtChanEty      AT channel entity
  \param[in]    pStr            print string
  \param[in]    strLen          length
  \returns      CmsRetId
*/
CmsRetId atcSendRawRespStr(AtChanEntity *pAtChanEty, const CHAR *pStr, UINT32 strLen)
{
    return atcRespStrOutput(pAtChanEty, pStr, strLen);
}


/**
  \fn           CmsRetId atcEchoString(const AtChanEntityP pAtChanEty, const CHAR *pStr, UINT32 strLen)
  \brief        Send AT command response
  \param[in]
  \returns      void
*/
CmsRetId atcEchoString(AtChanEntityP pAtChanEty, const CHAR *pStr, UINT32 strLen)
{
    CHAR        *pFiltBuf = NULL;
    UINT32      pLen = 0;        /*print string length*/
    UINT32      tmpIdx = 0;
    CmsRetId    ret = CMS_RET_SUCC;

    OsaDebugBegin(pAtChanEty != PNULL && pStr != PNULL && strLen > 0, pAtChanEty, pStr, strLen);
    return CMS_RET_SUCC;
    OsaDebugEnd();

    /*
     * MCU and CP handshake
    */
    if (strncmp(pStr, "AT^SYSTEST=handshake", strlen("AT^SYSTEST=handshake")) == 0 ||
        strncmp(pStr, "AT+ECSYSTEST=handshake", strlen("AT+ECSYSTEST=handshake")) == 0)  /* for test only, ST+Ec616 test, weili&yr*/
    {
        return CMS_RET_SUCC;
    }

    if (pAtChanEty->cfg.echoFlag == 0)
    {
        return CMS_RET_SUCC;
    }

    OsaDebugBegin(pAtChanEty->callBack.respFunc != PNULL, pAtChanEty->chanId, 0, 0);
    return CMS_RET_SUCC;
    OsaDebugEnd();

    /*
     * V.250
     * 1> The DCE may echo characters received from the DTE during "command state" and "online command state" back to the DTE,
     *    depending on the setting of the E command. -- TBD
     *
     * 2> If echo enabled, characters received from the DTE are echoed at the same rate, parity, and format as received.
     *
     * 3> Echoing characters not recognized as valid in the command line or of incomplete or improperly-formed
     *    command line prefixes is manufacturer-specific;
     *    - if not recognized as a valid character, we just ignore/skip it to print back to the DTE.
     *
    */

    pFiltBuf = OsaAllocMemory(strLen + 1);  /* +1, end with '\0' */
    if (pFiltBuf == PNULL)
    {
        OsaDebugBegin(FALSE, strLen, 0, 0);
        OsaDebugEnd();

        return atcRespStrOutput(pAtChanEty, pStr, strLen);
    }

    while (tmpIdx < strLen)
    {
        /*
         * echo character should be displayable:
         * 1> in the range from 0x20 (Space or blank) to 0x7F (delete)
         * 2> <CR><LF> (S3/S4)  //note: \r\n should not echo out
        */
        if ((pStr[tmpIdx] >= 0x20 && pStr[tmpIdx] <= 0x7F)
            /*||
            pStr[tmpIdx] == pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] ||
            pStr[tmpIdx] == pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX]
            */)
        {
            pFiltBuf[pLen] = pStr[tmpIdx];
            pLen++;
        }
        else
        {
            if (pStr[tmpIdx] != AT_ASCI_CTRL_Z && pStr[tmpIdx] != AT_ASCI_ESC &&
                pStr[tmpIdx] != pAtChanEty->cfg.S[AT_S3_CMD_LINE_TERM_CHAR_IDX] &&
                pStr[tmpIdx] != pAtChanEty->cfg.S[AT_S4_RESP_FORMAT_CHAR_IDX])
            {
                OsaDebugBegin(FALSE, pStr[tmpIdx], tmpIdx, strLen);
                OsaDebugEnd();
            }

            if (pStr[tmpIdx] == '\0')   //ending
            {
                break;
            }
        }

        tmpIdx++;
    }

    pFiltBuf[pLen] = '\0';

    if (pLen > 0)
    {
        ret = atcRespStrOutput(pAtChanEty, pFiltBuf, pLen);
    }

    OsaFreeMemory(&pFiltBuf);

    return ret;
}


/**
  \fn           CmsRetId atcReply(UINT16 srcHandle, AtResultCode resCode, UINT32 errCode, const CHAR *pRespInfo)
  \brief        AT command response
  \param[in]
  \returns      CmsRetId
  \Note: Two types of responses: "information text" and "result codes"
*/
CmsRetId atcReply(UINT16 srcHandler, AtResultCode resCode, UINT32 errCode, const CHAR *pRespInfo)
{
    UINT8   chanId  = AT_GET_HANDLER_CHAN_ID(srcHandler);
    AtChanEntityP pAtChanEty = atcGetEntityById(chanId);
    UINT8   tid     = AT_GET_HANDLER_TID(srcHandler);
    CmsRetId    retCode = CMS_RET_SUCC;

    OsaDebugBegin(pAtChanEty != PNULL, chanId, srcHandler, resCode);
    return CMS_FAIL;
    OsaDebugEnd();

    /*
     * check whether guard timer is running, if not, just means: timer already expired
    */
    if (pAtChanEty->curTid != tid ||
        OsaTimerIsRunning(pAtChanEty->asynTimer) != TRUE)
    {
        OsaDebugBegin(FALSE, pAtChanEty->curTid, tid, chanId);
        OsaDebugEnd();

        ECOMM_TRACE(UNILOG_ATCMD_EXEC, atcReply_warning_1, P_WARNING, 3,
                    "AT CMD, chanId: %d, Tid:%d/%d, guarding timer is not running, maybe expired before.",
                    chanId, pAtChanEty->curTid, tid);
        return CMS_RET_SUCC;

    }

    /*
     * send the "information text"
    */
    if (pRespInfo != PNULL && pRespInfo[0] != '\0')
    {
        if (resCode == AT_RC_RAW_INFO_CONTINUE)
        {
            /* send RAW info, don't need to add prefix/tailed: <S3>/<S4>, not the final result */
            atcSendRespStr(pAtChanEty, pRespInfo, strlen(pRespInfo), FALSE);
        }
        else if (resCode == AT_RC_CONTINUE)
        {
            /* not the final result code */
            atcSendInfoText(pAtChanEty, pRespInfo, strlen(pRespInfo), FALSE, TRUE);
        }
        else
        {
            /* not the final result code */
            atcSendInfoText(pAtChanEty, pRespInfo, strlen(pRespInfo), FALSE, FALSE);
        }
    }

    /*
     * 1> if result code is: OK
     *  check whether current/previous line all decode, result code "OK" should send when last AT command performed
     * 2> else other ERROR result code,
     *  no subsequent commands in the command line are need processed
    */

    /*
     * check the result code
    */
    switch (resCode)
    {
        case AT_RC_OK:
        {
            OsaDebugBegin(errCode == ATC_SUCC_CODE, errCode, 0, 0);    //errCode == 0, just means succ
            OsaDebugEnd();

            /*
             * if result code is: OK
             *  check whether current/previous line all decode, result code "OK" should send when last AT command performed
            */
            if (atcBePreAtLineDone(pAtChanEty))
            {
                retCode = atcSendResultCode(pAtChanEty, resCode, PNULL);
            }

            break;
        }

        case AT_RC_CONNECT:
        case AT_RC_RING:
        case AT_RC_NO_CARRIER:
        case AT_RC_NO_DIALTONE:
        case AT_RC_BUSY:
        case AT_RC_NO_ANSWER:
        {
            OsaDebugBegin(FALSE, resCode, errCode, 0);
            OsaDebugEnd();
            //NOT SUPPORT NOW, act as ERROR

            retCode = atcSendResultCode(pAtChanEty, AT_RC_ERROR, PNULL);
            break;
        }

        case AT_RC_ERROR:
        {
            retCode = atcSendResultCode(pAtChanEty, AT_RC_ERROR, PNULL);
            break;
        }

        case AT_RC_CONTINUE:    //do nothing
            break;

        case AT_RC_NO_RESULT:   //do nothing
            break;

        case AT_RC_RAW_INFO_CONTINUE:   //do nothing
            break;

        case AT_RC_CME_ERROR:
        {
            retCode = atcCMEECode(pAtChanEty, errCode);
            break;
        }

        case AT_RC_CMS_ERROR:
        {
            retCode = atcCMSECode(pAtChanEty, errCode);
            break;
        }

        case AT_RC_SOCKET_ERROR:
        {
            retCode = atcSOCKETECode(pAtChanEty, errCode);
            break;
        }

        case AT_RC_ECSOC_ERROR:
        {
            retCode = atcECSOCECode(pAtChanEty, errCode);
            break;
        }

        case AT_RC_ECSRVSOC_ERROR:
        {
            retCode = atcECSRVSOCECode(pAtChanEty, errCode);
            break;
        }

#ifdef FEATURE_CISONENET_ENABLE
        case AT_RC_CIS_ERROR:
        {
            retCode = atcCISECode(pAtChanEty, errCode);
            break;
        }
#endif
        case AT_RC_CTM2M_ERROR:
        {
            retCode = atcCTM2MECode(pAtChanEty, errCode);
            break;
        }

#ifdef FEATURE_WAKAAMA_ENABLE
        case AT_RC_LWM2M_ERROR:
        {
            retCode = atcLWM2MECode(pAtChanEty, errCode);
            break;
        }
#endif
#ifdef FEATURE_HTTPC_ENABLE
        case AT_RC_HTTP_ERROR:
        {
            retCode = atcHTTPCode(pAtChanEty, errCode);
            break;
        }
#endif

#if defined(FEATURE_MQTT_ENABLE) || defined(FEATURE_MQTT_TLS_ENABLE)
        case AT_RC_MQTT_ERROR:
        {
            retCode = atcMQTTCode(pAtChanEty, errCode);
            break;
        }
#endif

#ifdef FEATURE_COAP_ENABLE
        case AT_RC_COAP_ERROR:
        {
            //retCode = atcCOAPCode(pAtChanEty, errCode);
            break;
        }
#endif

#if defined(FEATURE_CTCC_DM_ENABLE) || defined(FEATURE_CUCC_DM_ENABLE) || defined(FEATURE_CMCC_DM_ENABLE)
        case AT_RC_DM_ERROR:
        {
            retCode = atcDMCode(pAtChanEty, errCode);
            break;
        }
#endif

#ifdef FEATURE_ATADC_ENABLE
        case AT_RC_ADC_ERROR:
        {
            retCode = atcADCCode(pAtChanEty, errCode);
            break;
        }
#endif

#ifdef FEATURE_EXAMPLE_ENABLE
        case AT_RC_EXAMPLE_ERROR:
        {
            retCode = atcEXAMPLECode(pAtChanEty, errCode);
            break;
        }
#endif

        case AT_RC_FWUPD_ERROR:
        {
            retCode = atcFWUPDCode(pAtChanEty, errCode);
            break;
        }
        #if 0
        AT_RC_CUSTOM_ERROR = 20,
        AT_RC_SOCKET_ERROR,         //Timer will Stop, +SOCKET ERROR
        AT_RC_ECSOC_ERROR,          //Timer will Stop, +CME ERROR
        #endif

        default:
            OsaDebugBegin(FALSE, resCode, errCode, 0);
            OsaDebugEnd();

            break;
    }

    /*
     * after send the result code, need to stop the guard timer
    */
    if (resCode != AT_RC_CONTINUE &&
        resCode != AT_RC_RAW_INFO_CONTINUE)
    {
        atcStopAsynTimer(pAtChanEty, tid);
    }
    else
    {
        return retCode;
    }

    /*
     * check whether need to abort subsequent commands in the same line
    */
    if (resCode != AT_RC_OK &&
        resCode != AT_RC_CONTINUE &&
        resCode != AT_RC_NO_RESULT &&
        resCode != AT_RC_RAW_INFO_CONTINUE)
    {
        atcAbortAtCmdLine(pAtChanEty);
    }

    /*
     * check whether any AT is still pending, if so need to send a signal to decode it
    */
    if (atcAnyPendingAt(pAtChanEty))
    {
        atcSendAtCmdContinueReqSig(pAtChanEty);
    }

    return retCode;
}

/**
  \fn           CmsRetId atcReplyNoOK(UINT16 srcHandle, AtResultCode resCode, UINT32 errCode, const CHAR *pRespInfo)
  \brief        AT command response NO OK output
  \param[in]
  \returns      CmsRetId
  \Note: Two types of responses: "information text" and "result codes"
*/
CmsRetId atcReplyNoOK(UINT16 srcHandler, AtResultCode resCode, UINT32 errCode, const CHAR *pRespInfo)
{
    UINT8   chanId  = AT_GET_HANDLER_CHAN_ID(srcHandler);
    AtChanEntityP pAtChanEty = atcGetEntityById(chanId);
    UINT8   tid     = AT_GET_HANDLER_TID(srcHandler);
    CmsRetId    retCode = CMS_RET_SUCC;

    OsaDebugBegin(pAtChanEty != PNULL, chanId, srcHandler, resCode);
    return CMS_FAIL;
    OsaDebugEnd();

    /*
     * check whether guard timer is running, if not, just means: timer already expired
    */
    if (pAtChanEty->curTid != tid ||
        OsaTimerIsRunning(pAtChanEty->asynTimer) != TRUE)
    {
        OsaDebugBegin(FALSE, pAtChanEty->curTid, tid, chanId);
        OsaDebugEnd();

        ECOMM_TRACE(UNILOG_ATCMD_EXEC, atcReply_warning_1, P_WARNING, 3,
                    "AT CMD, chanId: %d, Tid:%d/%d, guarding timer is not running, maybe expired before.",
                    chanId, pAtChanEty->curTid, tid);
        return CMS_RET_SUCC;

    }

    /*
     * send the "information text"
    */
    if (pRespInfo != PNULL && pRespInfo[0] != '\0')
    {
        if (resCode == AT_RC_RAW_INFO_CONTINUE)
        {
            /* send RAW info, don't need to add prefix/tailed: <S3>/<S4>, not the final result */
            atcSendRespStr(pAtChanEty, pRespInfo, strlen(pRespInfo), FALSE);
        }
        else if (resCode == AT_RC_CONTINUE)
        {
            /* not the final result code */
            atcSendInfoText(pAtChanEty, pRespInfo, strlen(pRespInfo), FALSE, TRUE);
        }
        else
        {
            /* not the final result code */
            atcSendInfoText(pAtChanEty, pRespInfo, strlen(pRespInfo), FALSE, FALSE);
        }
    }

    /*
     * 1> if result code is: OK
     *  check whether current/previous line all decode, result code "OK" should send when last AT command performed
     * 2> else other ERROR result code,
     *  no subsequent commands in the command line are need processed
    */

    /*
     * check the result code
    */
    switch (resCode)
    {
        case AT_RC_OK:
        {
            OsaDebugBegin(errCode == ATC_SUCC_CODE, errCode, 0, 0);    //errCode == 0, just means succ
            OsaDebugEnd();

            /*
             * if result code is: OK
             *  check whether current/previous line all decode, result code "OK" should send when last AT command performed
            */
            if (atcBePreAtLineDone(pAtChanEty))
            {
                //retCode = atcSendResultCode(pAtChanEty, resCode, PNULL);
				retCode = CMS_RET_SUCC;
            }

            break;
        }

        case AT_RC_CONNECT:
        case AT_RC_RING:
        case AT_RC_NO_CARRIER:
        case AT_RC_NO_DIALTONE:
        case AT_RC_BUSY:
        case AT_RC_NO_ANSWER:
        {
            OsaDebugBegin(FALSE, resCode, errCode, 0);
            OsaDebugEnd();
            //NOT SUPPORT NOW, act as ERROR

            retCode = atcSendResultCode(pAtChanEty, AT_RC_ERROR, PNULL);
            break;
        }

        case AT_RC_ERROR:
        {
            retCode = atcSendResultCode(pAtChanEty, AT_RC_ERROR, PNULL);
            break;
        }

        case AT_RC_CONTINUE:    //do nothing
            break;

        case AT_RC_NO_RESULT:   //do nothing
            break;

        case AT_RC_RAW_INFO_CONTINUE:   //do nothing
            break;

        case AT_RC_CME_ERROR:
        {
            retCode = atcCMEECode(pAtChanEty, errCode);
            break;
        }

        case AT_RC_CMS_ERROR:
        {
            retCode = atcCMSECode(pAtChanEty, errCode);
            break;
        }

        case AT_RC_SOCKET_ERROR:
        {
            retCode = atcSOCKETECode(pAtChanEty, errCode);
            break;
        }

        case AT_RC_ECSOC_ERROR:
        {
            retCode = atcECSOCECode(pAtChanEty, errCode);
            break;
        }

        case AT_RC_ECSRVSOC_ERROR:
        {
            retCode = atcECSRVSOCECode(pAtChanEty, errCode);
            break;
        }

#ifdef FEATURE_CISONENET_ENABLE
        case AT_RC_CIS_ERROR:
        {
            retCode = atcCISECode(pAtChanEty, errCode);
            break;
        }
#endif
        case AT_RC_CTM2M_ERROR:
        {
            retCode = atcCTM2MECode(pAtChanEty, errCode);
            break;
        }

#ifdef FEATURE_WAKAAMA_ENABLE
        case AT_RC_LWM2M_ERROR:
        {
            retCode = atcLWM2MECode(pAtChanEty, errCode);
            break;
        }
#endif
#ifdef FEATURE_HTTPC_ENABLE
        case AT_RC_HTTP_ERROR:
        {
            retCode = atcHTTPCode(pAtChanEty, errCode);
            break;
        }
#endif

#if defined(FEATURE_MQTT_ENABLE) || defined(FEATURE_MQTT_TLS_ENABLE)
        case AT_RC_MQTT_ERROR:
        {
            retCode = atcMQTTCode(pAtChanEty, errCode);
            break;
        }
#endif

#ifdef FEATURE_COAP_ENABLE
        case AT_RC_COAP_ERROR:
        {
            //retCode = atcCOAPCode(pAtChanEty, errCode);
            break;
        }
#endif

#if defined(FEATURE_CTCC_DM_ENABLE) || defined(FEATURE_CUCC_DM_ENABLE) || defined(FEATURE_CMCC_DM_ENABLE)
        case AT_RC_DM_ERROR:
        {
            retCode = atcDMCode(pAtChanEty, errCode);
            break;
        }
#endif

#ifdef FEATURE_ATADC_ENABLE
        case AT_RC_ADC_ERROR:
        {
            retCode = atcADCCode(pAtChanEty, errCode);
            break;
        }
#endif

#ifdef FEATURE_EXAMPLE_ENABLE
        case AT_RC_EXAMPLE_ERROR:
        {
            retCode = atcEXAMPLECode(pAtChanEty, errCode);
            break;
        }
#endif

        case AT_RC_FWUPD_ERROR:
        {
            retCode = atcFWUPDCode(pAtChanEty, errCode);
            break;
        }
        #if 0
        AT_RC_CUSTOM_ERROR = 20,
        AT_RC_SOCKET_ERROR,         //Timer will Stop, +SOCKET ERROR
        AT_RC_ECSOC_ERROR,          //Timer will Stop, +CME ERROR
        #endif

        default:
            OsaDebugBegin(FALSE, resCode, errCode, 0);
            OsaDebugEnd();

            break;
    }

    /*
     * after send the result code, need to stop the guard timer
    */
    if (resCode != AT_RC_CONTINUE &&
        resCode != AT_RC_RAW_INFO_CONTINUE)
    {
        atcStopAsynTimer(pAtChanEty, tid);
    }
    else
    {
        return retCode;
    }

    /*
     * check whether need to abort subsequent commands in the same line
    */
    if (resCode != AT_RC_OK &&
        resCode != AT_RC_CONTINUE &&
        resCode != AT_RC_NO_RESULT &&
        resCode != AT_RC_RAW_INFO_CONTINUE)
    {
        atcAbortAtCmdLine(pAtChanEty);
    }

    /*
     * check whether any AT is still pending, if so need to send a signal to decode it
    */
    if (atcAnyPendingAt(pAtChanEty))
    {
        atcSendAtCmdContinueReqSig(pAtChanEty);
    }

    return retCode;
}

/**
  \fn           CmsRetId atcURC(UINT32 chanId, const CHAR *pUrcStr)
  \brief        unsolicited result code
  \param[in]
  \returns      CmsRetId
  \Note:
*/
CmsRetId atcURC(UINT32 chanId, const CHAR *pUrcStr)
{
    AtChanEntityP pAtChanEty = atcGetEntityById(chanId);

    OsaDebugBegin(pAtChanEty != PNULL && pUrcStr != PNULL, chanId, pUrcStr, 0);
    return CMS_RET_SUCC;
    OsaDebugEnd();

    return atcSendUrcResult(pAtChanEty, pUrcStr, strlen(pUrcStr));
}

/**
  \fn           BOOL atcBeURCConfiged(UINT32 chanId)
  \brief        Whether URC API configed
  \param[in]    chanId      AT channel ID
  \returns      BOOL
  \Note:
*/
BOOL atcBeURCConfiged(UINT32 chanId)
{
    AtChanEntityP pAtChanEty = atcGetEntityById(chanId);

    if (pAtChanEty == PNULL)
    {
        return FALSE;
    }

    return (pAtChanEty->callBack.urcFunc != PNULL);
}



