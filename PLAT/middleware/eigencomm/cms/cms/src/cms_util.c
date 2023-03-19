/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    cms_util.c
 * Description:
 * History:
 *
 ****************************************************************************/
#include "cms_util.h"
#include "ostask.h"

/******************************************************************************
 ******************************************************************************
  GLOBAL variables
 ******************************************************************************
******************************************************************************/




/******************************************************************************
 ******************************************************************************
  External API/Variables specification
 ******************************************************************************
******************************************************************************/



#define __DEFINE_INTERNAL_FUNCTION__ //just for easy to find this position----Below for internal use

/**
  \fn           CmsRetId cmsPriSynApiCall(CmsSynCallback synFunc, UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutPut, BOOL highPri)
  \brief        API called in CMS task
  \param[in]
  \returns      CmsRetId
*/
static CmsRetId cmsPriSynApiCall(CmsSynCallback synFunc, UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutPut, BOOL highPri)
{
    CmsSynApiReq    *pCmsSynReq = PNULL;
    SignalBuf       *pSig = PNULL;

    CmsSynApiInfo   *pApiInfo = PNULL;
    UINT16          synApiSize = 0;
    UINT16          synApiHdrSize = (sizeof(CmsSynApiInfo)+3)&0xFFFFFFFC;    /* 4 bytes aligned */
    UINT16          inputSize4 = (inputSize+3)&0xFFFFFFFC;      /* 4 bytes aligned */
    UINT16          outputSize4 = (outputSize+3)&0xFFFFFFFC;    /* 4 bytes aligned */
    UINT32          *pInMagic = PNULL, *pOutMagic = PNULL;
    UINT8           *pInAddr = PNULL, *pOutAddr = PNULL;
    osStatus_t      osState = osOK;
    CmsRetId        retErrCode = CMS_RET_SUCC;
    const CHAR      *pTaskName = PNULL;

    /*
     * check the input parameters
    */
    OsaDebugBegin(synFunc != PNULL, synFunc, inputSize, outputSize);
        return CMS_INVALID_PARAM;
    OsaDebugEnd();

    OsaDebugBegin((inputSize > 0 && pInput != PNULL) || (inputSize == 0 && pInput == PNULL), synFunc, inputSize, pInput);
        return CMS_INVALID_PARAM;
    OsaDebugEnd();

    OsaDebugBegin((outputSize > 0 && pOutPut != PNULL) || (outputSize == 0 && pOutPut == PNULL), synFunc, outputSize, pOutPut);
        return CMS_INVALID_PARAM;
    OsaDebugEnd();

    /*
     * this API can't be called in CMS task
    */
    pTaskName = osThreadGetName(osThreadGetId());

    if (pTaskName != PNULL)
    {
        if (strncasecmp(pTaskName, "CmsTask", strlen("CmsTask")) == 0)
        {
            OsaDebugBegin(FALSE, synFunc, outputSize, pOutPut);
            //return CMS_FAIL;
            OsaDebugEnd();

            return (*synFunc)(inputSize, pInput, outputSize, pOutPut);
        }
    }

    /*
     * +----------------------+--------------+---------+--------------+-------+
     * | CmsSynApiInfo        | input PARAM  | MAGIC   | output PARAM | MAGIC |
     * +----------------------+--------------+---------+--------------+-------+
     * ^                      ^              ^         ^              ^
     * pApiInfo               pInAddr        pInMagic  pOutAddr       pOutMagic
    */
    synApiSize = synApiHdrSize + inputSize4 + outputSize4 + 2*CMS_MAGIC_WORDS_SIZE;

    OsaDebugBegin(synApiSize < CMS_SYN_API_BODY_MAX, synApiSize, inputSize, outputSize);
    return CMS_INVALID_PARAM;
    OsaDebugEnd();

    /*
     * create SYNAPIINFO memory, and fill the struct
    */
    pApiInfo = (CmsSynApiInfo *)OsaAllocZeroMemory(synApiSize);

    OsaDebugBegin(pApiInfo != PNULL, synApiSize, inputSize, outputSize);
    return CMS_NO_MEM;
    OsaDebugEnd();

    pApiInfo->sem = osSemaphoreNew(1U, 0, PNULL);

    OsaDebugBegin(pApiInfo->sem != PNULL, synApiSize, inputSize, outputSize);
        OsaFreeMemory(&pApiInfo);
        return CMS_SEMP_ERROR;
    OsaDebugEnd();

    pApiInfo->cmsSynFunc    = synFunc;
    pApiInfo->inputSize     = inputSize;
    pApiInfo->outputSize    = outputSize;
    pApiInfo->hdrMagic      = CMS_MAGIC_WORDS;
    pApiInfo->retCode       = CMS_RET_SUCC;

    pInMagic    = (UINT32 *)(pApiInfo->param + inputSize4);
    pOutMagic   = (UINT32 *)(pApiInfo->param + inputSize4 + CMS_MAGIC_WORDS_SIZE + outputSize4);
    *pInMagic   = CMS_MAGIC_WORDS;
    *pOutMagic  = CMS_MAGIC_WORDS;

    /*
     * input param
    */
    pInAddr = pApiInfo->param;
    if (inputSize > 0)
    {
        memcpy(pInAddr, pInput, inputSize);
    }

    pOutAddr = pInAddr + inputSize4 + CMS_MAGIC_WORDS_SIZE;

    /*
     * create signal
    */
    if (highPri)
    {
        OsaCreateZeroSignal(SIG_CMS_HIGH_PRI_SYN_API_REQ, sizeof(CmsSynApiReq), &pSig);
        ECOMM_TRACE(UNILOG_CMS, cmsPriSynApiCall_1, P_DEBUG, 1, "SIG sent pointer:%x", pSig);
    }
    else
    {
        OsaCreateZeroSignal(SIG_CMS_SYN_API_REQ, sizeof(CmsSynApiReq), &pSig);
    }

    OsaCheck(pSig != PNULL, sizeof(CmsSynApiReq), 0, 0);

    pCmsSynReq = (CmsSynApiReq *)(pSig->sigBody);
    pCmsSynReq->pApiInfo = pApiInfo;

    /*
     * send signal to CMS task
    */
    OsaSendSignal(CMS_TASK_ID, &pSig);

    /*
     * wait for sem
    */
    if ((osState = osSemaphoreAcquire(pApiInfo->sem, portMAX_DELAY)) == osOK)
    {
        //OsaDebugBegin(FALSE, synFunc, inputSize, outputSize);
        //OsaDebugEnd();

        if (pApiInfo->retCode == CMS_RET_SUCC)
        {
            if (outputSize > 0 && pOutPut != PNULL)
            {
                memcpy(pOutPut, pOutAddr, outputSize);
            }
        }
        else
        {
            OsaDebugBegin(FALSE, pApiInfo->retCode, CMS_RET_SUCC, 0);
            OsaDebugEnd();
        }
    }
    else
    {
        OsaDebugBegin(FALSE, osState, pApiInfo->retCode, 0);
        OsaDebugEnd();

        pApiInfo->retCode = CMS_SEMP_ERROR;
    }

    retErrCode = pApiInfo->retCode;

    // check magic words
    OsaCheck(pApiInfo->hdrMagic == CMS_MAGIC_WORDS && *pInMagic == CMS_MAGIC_WORDS && *pOutMagic == CMS_MAGIC_WORDS,
             pApiInfo->hdrMagic, *pInMagic, *pOutMagic);

    /*
     * Semaphore delete, and memory free
    */
    osSemaphoreDelete(pApiInfo->sem);
    OsaFreeMemory(&pApiInfo);

    return retErrCode;
}


#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position---Below for external use


/**
  \fn           UINT8 cmsDigitCharToNum(UINT8 ch)
  \brief        Convert the input digit char value to digit value
  \param[in]    dChar       digit char value
  \returns      number value: 0-F; if failed, return -1
*/
INT8 cmsDigitCharToNum(UINT8 dChar)
{
    /*
     * example:
     * 1> cmsDigitCharToNum('2'), return: 2
     * 2> cmsDigitCharToNum('B'), return: 0xB
     * 3> cmsDigitCharToNum('c'), return: 0xC
     * 4> cmsDigitCharToNum('T'), return: -1, print a warning
    */
    UINT32  cValue = (UINT32)(dChar - '0');

    /* '0' = 0x30 = 48, 'A' = 0x41 = 65, "a" = 0x61 = 97 */
    if (cValue <= 9)
    {
        return (INT8)(cValue);
    }

    cValue = (UINT32)(dChar - 'a');
    if (cValue <= 5)
    {
        return (INT8)(cValue) + 0xA;
    }

    cValue = (UINT32)(dChar - 'A');
    if (cValue <= 5)
    {
        return (INT8)(cValue) + 0xA;
    }

    OsaDebugBegin(FALSE, dChar, 'a', 'A');
    OsaDebugEnd();

    return -1;
}


/**
  \fn           INT8 cmsNumToChar(UINT8 num)
  \brief        Convert the input digit number (<= 0xF) value to character
  \param[in]    num         digit number
  \returns      character value, if failed, return -1
*/
INT8 cmsNumToChar(UINT8 num)
{
    if (num <= 9)
    {
        return '0' + num;
    }

    if (num <= 0xF)
    {
        return 'A' + num - 10;
    }

    OsaDebugBegin(FALSE, num, 0, 0);
    OsaDebugEnd();

    return -1;
}


/**
  \fn           BOOL cmsHexStrToUInt(UINT32 *pOutput, const UINT8 *pHexStr, UINT32 strMaxLen)
  \brief        Convert the hex string to UINT value
  \param[out]   pOutput     output UINT32 value
  \param[in]    pHexStr     input hex string
  \param[in]    strMaxLen   hex string length
  \returns      TRUE/FALSE
*/
BOOL cmsHexStrToUInt(UINT32 *pOutput, const UINT8 *pHexStr, UINT32 strMaxLen)
{
    UINT32 tmpIdx = 0;

    /*
     * example:
     *  cmsHexStrToUInt(pOutput, "AB7", 3), return: TRUE, pOutput = 0x0AB7
     *  cmsHexStrToUInt(pOutput, "0xAB7", 5), return: TRUE, pOutput = 0x0AB7
     *  cmsHexStrToUInt(pOutput, "AB7", 6), return: TRUE, pOutput = 0x0AB7
     *  cmsHexStrToUInt(pOutput, "ABHL", 4), return: FALSE, pOutput = 0xAB
     *  cmsHexStrToUInt(pOutput, "ABHL", 2), return: TRUE, pOutput = 0xAB
     *  cmsHexStrToUInt(pOutput, "1234567890", 10), return: FALSE, pOutput = 0x12345678
     *  cmsHexStrToUInt(pOutput, "\0", 1), return: FALSE, pOutput = 0x0
     *  cmsHexStrToUInt(pOutput, "0x", 2), return: FALSE, pOutput = 0x0
     *  cmsHexStrToUInt(pOutput, "ABCD", 0), return: FALSE, pOutput = 0x0
    */

    if (pOutput != PNULL)
    {
        *pOutput = 0;
    }

    OsaDebugBegin(pHexStr != PNULL && pOutput != PNULL && strMaxLen > 0, pHexStr, strMaxLen, pOutput);
    return FALSE;
    OsaDebugEnd();

    /*
     * skip "0x", if input
    */
    if ((pHexStr[0] == '0' && pHexStr[1] == 'x') ||
        (pHexStr[0] == '0' && pHexStr[1] == 'X'))
    {
        OsaDebugBegin(strMaxLen > 2, strMaxLen, tmpIdx, 0);
        return FALSE;
        OsaDebugEnd();

        strMaxLen -= 2;
        pHexStr += 2;
    }

    if (*pHexStr == '\0')
    {
        OsaDebugBegin(FALSE, strMaxLen, 0, 0);
        OsaDebugEnd();

        return FALSE;
    }

    for (tmpIdx = 0; (tmpIdx < strMaxLen) && (pHexStr[tmpIdx] != '\0'); tmpIdx++)
    {
        /*
         * cmsHexStrToUInt("1234567890", 10, pOutput), return: FALSE, pOutput = 0x12345678
        */
        if (tmpIdx >= 8)
        {
            OsaDebugBegin(FALSE, tmpIdx, strMaxLen, *pOutput);
            OsaDebugEnd();

            return FALSE;
        }

        if (pHexStr[tmpIdx] >= '0' && pHexStr[tmpIdx] <= '9')
        {
            *pOutput = ((*pOutput) << 4) + (pHexStr[tmpIdx] - '0');
        }
        else if (pHexStr[tmpIdx] >= 'a' && pHexStr[tmpIdx] <= 'f')
        {
            *pOutput = ((*pOutput) << 4) + (pHexStr[tmpIdx] - 'a' + 0xA);
        }
        else if (pHexStr[tmpIdx] >= 'A' && pHexStr[tmpIdx] <= 'F')
        {
            *pOutput = ((*pOutput) << 4) + (pHexStr[tmpIdx] - 'A' + 0xA);
        }
        else
        {
            /*
             * other invalid CHAR:
             *   cmsHexStrToUInt("ABHL", 4, pOutput), return: FALSE, pOutput = 0xAB
            */
            OsaDebugBegin(FALSE, pHexStr[tmpIdx], tmpIdx, *pOutput);
            OsaDebugEnd();

            return FALSE;
        }
    }

    return TRUE;
}

/**
  \fn           BOOL cmsDecStrToUInt(UINT32 *pOutput, const UINT8 *pDecStr, UINT32 strMaxLen)
  \brief        Convert the DEC string to UINT value
  \param[out]   pOutput     output UINT32 value
  \param[in]    pDecStr     input dec string
  \param[in]    strMaxLen   hex string length
  \returns      TRUE/FALSE
*/
BOOL cmsDecStrToUInt(UINT32 *pOutput, const UINT8 *pDecStr, UINT32 strMaxLen)
{
    UINT32 tmpIdx = 0;

    /*
     * example:
     *  cmsDecStrToUInt(pOutput, "120", 3), return: TRUE, pOutput = 120
     *  cmsDecStrToUInt(pOutput, "120", 5), return: TRUE, pOutput = 120
     *  cmsDecStrToUInt(pOutput, "12AB", 4), return: FALSE, pOutput = 12
     *  cmsDecStrToUInt(pOutput, "12AB", 2), return: TRUE, pOutput = 12
     *  cmsDecStrToUInt(pOutput, "4294967295909", 13), return: FALSE, pOutput = 4294967295     //0xFFFFFFFF="4294967295"
     *  cmsDecStrToUInt(pOutput, "\0", 1), return: FALSE, pOutput = 0x0
     *  cmsDecStrToUInt(pOutput, "123", 0), return: FALSE, pOutput = 0x0
    */

    if (pOutput != PNULL)
    {
        *pOutput = 0;
    }

    OsaDebugBegin(pDecStr != PNULL && pOutput != PNULL && strMaxLen > 0 && *pDecStr != '\0', pDecStr, strMaxLen, pOutput);
    return FALSE;
    OsaDebugEnd();

    for (tmpIdx = 0; (tmpIdx < strMaxLen) && (pDecStr[tmpIdx] != '\0'); tmpIdx++)
    {
        /*
         * cmsHexStrToUInt(pOutput, "4294967295909", 13), return: FALSE, pOutput = 4294967295     //0xFFFFFFFF="4294967295"
        */
        if (tmpIdx >= 10)
        {
            OsaDebugBegin(FALSE, tmpIdx, strMaxLen, *pOutput);
            OsaDebugEnd();

            return FALSE;
        }

        if (pDecStr[tmpIdx] >= '0' && pDecStr[tmpIdx] <= '9')
        {
            *pOutput = ((*pOutput) * 10) + (pDecStr[tmpIdx] - '0');
        }
        else
        {
            /*
             * other invalid CHAR:
             *   cmsHexStrToUInt(pOutput, "12AB", 4), return: FALSE, pOutput = 12
            */
            OsaDebugBegin(FALSE, pDecStr[tmpIdx], tmpIdx, *pOutput);
            OsaDebugEnd();

            return FALSE;
        }
    }

    return TRUE;
}

/**
  \fn           BOOL cmsBinStrToUInt(UINT32 *pOutput, const UINT8 *pBinStr, UINT32 strMaxLen)
  \brief        Convert the DEC string to UINT value
  \param[out]   pOutput     output UINT32 value
  \param[in]    pBinStr     input bin string
  \param[in]    strMaxLen   hex string length
  \returns      TRUE/FALSE
*/
BOOL cmsBinStrToUInt(UINT32 *pOutput, const UINT8 *pBinStr, UINT32 strMaxLen)
{
    UINT32 tmpIdx = 0;

    /*
     * example:
     *  cmsBinStrToUInt(pOutput, "110", 3), return: TRUE, pOutput = 6
     *  cmsBinStrToUInt(pOutput, "110", 5), return: TRUE, pOutput = 6
     *  cmsBinStrToUInt(pOutput, "10AB", 4), return: FALSE, pOutput = 2
     *  cmsBinStrToUInt(pOutput, "10AB", 2), return: TRUE, pOutput = 2
     *  cmsBinStrToUInt(pOutput, "111111111111111111111111111111110000", 36), return: FALSE, pOutput = 0xFFFFFFFF
     *  cmsBinStrToUInt(pOutput, "\0", 1), return: FALSE, pOutput = 0x0
     *  cmsBinStrToUInt(pOutput, "1010", 0), return: FALSE, pOutput = 0x0
    */

    if (pOutput != PNULL)
    {
        *pOutput = 0;
    }

    OsaDebugBegin(pBinStr != PNULL && pOutput != PNULL && strMaxLen > 0 && *pBinStr != '\0', pBinStr, strMaxLen, pOutput);
    return FALSE;
    OsaDebugEnd();

    for (tmpIdx = 0; (tmpIdx < strMaxLen) && (pBinStr[tmpIdx] != '\0'); tmpIdx++)
    {
        /*
         * cmsBinStrToUInt(pOutput, "111111111111111111111111111111110000", 36), return: FALSE, pOutput = 0xFFFFFFFF
        */
        if (tmpIdx >= 32)
        {
            OsaDebugBegin(FALSE, tmpIdx, strMaxLen, *pOutput);
            OsaDebugEnd();

            return FALSE;
        }

        if (pBinStr[tmpIdx] == '0' || pBinStr[tmpIdx] == '1')
        {
            *pOutput = ((*pOutput) << 1) + (pBinStr[tmpIdx] - '0');
        }
        else
        {
            /*
             * other invalid CHAR:
             *   cmsHexStrToUInt(pOutput, "12AB", 4), return: FALSE, pOutput = 12
            */
            OsaDebugBegin(FALSE, pBinStr[tmpIdx], tmpIdx, *pOutput);
            OsaDebugEnd();

            return FALSE;
        }
    }

    return TRUE;
}

/**
  \fn           INT32 cmsHexStrToHex(UINT8 *pOutput, UINT32 outBufSize, const CHAR *pHexStr, UINT32 strMaxLen)
  \brief        Convert the hex string to hex array list
  \param[out]   pOutput     output HEX array list
  \param[in]    outBufSize  output buffer size
  \param[in]    pHexStr     input hex string
  \param[in]    strMaxLen   hex string length
  \returns      output hex array length, if failed, return -1
*/
INT32 cmsHexStrToHex(UINT8 *pOutput, UINT32 outBufSize, const CHAR *pHexStr, UINT32 strMaxLen)
{
    INT32   retLen = 0;
    UINT32  strIdx = 0;
    INT8    hc = 0, lc = 0;
    /*
     * example:
     *  cmsHexStrToHex(pOutput, 3, "123456", 6);     return 3, pOutput = {0x12, 0x34, 0x56}
     *  cmsHexStrToHex(pOutput, 3, "0x123456", 6);   return 2, pOutput = {0x12, 0x34}
     *  cmsHexStrToHex(pOutput, 3, "12345678", 8);   return 3, pOutput = {0x12, 0x34, 0x56}
     *  cmsHexStrToHex(pOutput, 3, "12345", 6);      return 2, pOutput = {0x12, 0x34}
     *  cmsHexStrToHex(pOutput, 3, "123MNP", 6);     return 1, pOutput = {0x12}
     *  cmsHexStrToHex(pOutput, 3, "OPQSRA", 6);     return 0, pOutput = {0x00}
     *  cmsHexStrToHex(PNULL, 3, PNULL, 6);          return -1
     *  cmsHexStrToHex(pOutput, 3, "0x", 2);         return 0
     *  cmsHexStrToHex(pOutput, 3, "", 6);           return 0
    */

    OsaDebugBegin(pOutput != PNULL && outBufSize > 0 && pHexStr != PNULL && strMaxLen > 0,
                  pOutput, outBufSize, strMaxLen);
    return -1;
    OsaDebugEnd();

    /*
     * skip "0x", if input
    */
    if ((pHexStr[0] == '0' && pHexStr[1] == 'x') ||
        (pHexStr[0] == '0' && pHexStr[1] == 'X'))
    {
        OsaDebugBegin(strMaxLen > 2, strMaxLen, outBufSize, 0);
        memset(pOutput, 0x00, outBufSize);
        return 0;
        OsaDebugEnd();

        strIdx += 2;
    }

    while ((strIdx + 1) < strMaxLen &&
           pHexStr[strIdx] != '\0' &&
           pHexStr[strIdx + 1] != '\0' &&
           retLen < outBufSize)
    {
        /*
         * "23" => 0x23
        */
        hc = cmsDigitCharToNum(pHexStr[strIdx]);
        lc = cmsDigitCharToNum(pHexStr[strIdx + 1]);

        if (hc < 0 || lc < 0)
        {
            OsaDebugBegin(FALSE, pHexStr[strIdx], pHexStr[strIdx + 1], 0);
            OsaDebugEnd();
            break;
        }

        pOutput[retLen] = (hc << 4) + lc;

        retLen += 1;
        strIdx += 2;
    }

    if (retLen < outBufSize)
    {
        memset(&(pOutput[retLen]), 0x00, outBufSize - retLen);
    }

    return retLen;
}

/**
  \fn           INT32 cmsHexToHexStr(CHAR *pOutStr, UINT32 outStrSize, const UINT8 *pHexData, UINT32 hexLen)
  \brief        Convert the hex array to hex string
  \param[out]   pOutStr     output string buffer
  \param[in]    outBufSize  output buffer size
  \param[in]    pHexData    input hex array
  \param[in]    hexLen      hex array size
  \returns      output hex string length (not include the length of '\0'), if failed, return -1
*/
INT32 cmsHexToHexStr(CHAR *pOutStr, UINT32 outBufSize, const UINT8 *pHexData, UINT32 hexLen)
{
    /*
     * example:
     *  cmsHexToHexStr(pOutStr, 4, 0x1234, 2);       return 4, pOutStr = "1234"
     *  cmsHexToHexStr(pOutStr, 4, 0x123456, 4);     return 4, pOutStr = "1234"
     *  cmsHexToHexStr(pOutStr, 8, 0x1234, 2);       return 4, pOutStr = "1234\0"
     *  cmsHexToHexStr(pOutStr, 3, 0x1234, 2);       return 3, pOutStr = "123"
     *  cmsHexToHexStr(PNULL, 0, PNULL, 0);          return -1
    */
    UINT32  hexIdx = 0, strIdx = 0;
    UINT8   hbit = 0, lbit = 0;

    OsaDebugBegin(pOutStr != PNULL && outBufSize > 0 && pHexData != PNULL && hexLen > 0,
                  pOutStr, outBufSize, hexLen);
    return -1;
    OsaDebugEnd();

    for (; (hexIdx < hexLen) && (strIdx+1 < outBufSize); hexIdx++)
    {
        hbit = (pHexData[hexIdx] >> 4) & 0x0F;
        lbit = pHexData[hexIdx] & 0x0F;

        pOutStr[strIdx++]   = hbit > 9 ? ((hbit - 0xA) + 'A') : (hbit + '0');
        pOutStr[strIdx++]   = lbit > 9 ? ((lbit - 0xA) + 'A') : (lbit + '0');
    }

    /*
     *  cmsHexToHexStr(pOutStr, 3, 0x1234, 2);       return 3, pOutStr = "123"
    */
    if ((hexIdx < hexLen) && (strIdx < outBufSize))
    {
        hbit = (pHexData[hexIdx] >> 4) & 0x0F;

        pOutStr[strIdx++]   = hbit > 9 ? ((hbit - 0xA) + 'A') : (hbit + '0');
    }

    /*
     * cmsHexToHexStr(pOutStr, 8, 0x1234, 2);       return 4, pOutStr = "1234\0"
    */
    if (strIdx < outBufSize)
    {
        pOutStr[strIdx] = '\0';
    }

    return strIdx;
}


/**
  \fn           BOOL cmsBePureIntStr(CHAR *str)
  \brief        Check whether the input string is pure number character
  \param[in]    str     input strings
  \returns      TRUE/FALSE
*/
BOOL cmsBePureIntStr(CHAR *str)
{
    if (str)
    {
        while (*str != '\0')
        {
            if (*str < '0' || *str > '9')
            {
                return FALSE;
            }
            str++;
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/**
  \fn           CmsRetId cmsSynApiCall(CmsSynCallback synFunc, UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutPut)
  \brief        API called in CMS task
  \param[in]
  \returns      CmsRetId
*/
CmsRetId cmsSynApiCall(CmsSynCallback synFunc, UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutPut)
{
    return cmsPriSynApiCall(synFunc, inputSize, pInput, outputSize, pOutPut, FALSE);
}

/**
  \fn           CmsRetId cmsHighPriSynApiCall(CmsSynCallback synFunc, UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutPut)
  \brief        API called in CMS task
  \param[in]
  \returns      CmsRetId
*/
CmsRetId cmsHighPriSynApiCall(CmsSynCallback synFunc, UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutPut)
{
    return cmsPriSynApiCall(synFunc, inputSize, pInput, outputSize, pOutPut, TRUE);
}



/**
  \fn           CmsRetId cmsBlockApiCall(CmsBlockCallback blockFunc, UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutPut, UINT32 timeOutMs)
  \brief        API called in CMS task
  \param[in]
  \returns      CmsRetId
*/
CmsRetId cmsBlockApiCall(CmsBlockCallback blockFunc, UINT16 inputSize, void *pInput, UINT16 outputSize, void *pOutPut, UINT32 timeOutMs)
{
    CmsBlockApiReq  *pCmsBlockReq = PNULL;
    SignalBuf       *pSig = PNULL;

    CmsBlockApiInfo blockApiInfo;   // 28 bytes, acceptable
    CmsBlockApiInfo *pBlockApiInfo = &blockApiInfo;
    osStatus_t      osState = osOK;
    CmsRetId        retErrCode = CMS_RET_SUCC;
    const CHAR      *pTaskName = PNULL;

    /*
     * check the input parameters
    */
    OsaDebugBegin(blockFunc != PNULL, blockFunc, inputSize, outputSize);
        return CMS_INVALID_PARAM;
    OsaDebugEnd();

    OsaDebugBegin((inputSize > 0 && pInput != PNULL) || (inputSize == 0 && pInput == PNULL), blockFunc, inputSize, pInput);
        return CMS_INVALID_PARAM;
    OsaDebugEnd();

    OsaDebugBegin((outputSize > 0 && pOutPut != PNULL) || (outputSize == 0 && pOutPut == PNULL), blockFunc, outputSize, pOutPut);
        return CMS_INVALID_PARAM;
    OsaDebugEnd();

    /*
     * this API can't be called in CMS task
    */
    pTaskName = osThreadGetName(osThreadGetId());

    if (pTaskName != PNULL)
    {
        if (strncasecmp(pTaskName, "CmsTask", strlen("CmsTask")) == 0)
        {
            OsaDebugBegin(FALSE, blockFunc, outputSize, pOutPut);
            return CMS_FAIL;
            OsaDebugEnd();
        }
    }

    /*
     * create BLOCKAPIINFO memory, and fill the struct, used the stack memory
    */
    #if 0
    pBlockApiInfo = (CmsBlockApiInfo *)OsaAllocZeroMemory(sizeof(CmsBlockApiInfo));

    OsaDebugBegin(pBlockApiInfo != PNULL, sizeof(CmsBlockApiInfo), inputSize, outputSize);
    return CMS_NO_MEM;
    OsaDebugEnd();
    #endif
    memset(pBlockApiInfo, 0x00, sizeof(CmsBlockApiInfo));

    pBlockApiInfo->sem = osSemaphoreNew(1U, 0, PNULL);

    OsaDebugBegin(pBlockApiInfo->sem != PNULL, 0, inputSize, outputSize);
        OsaFreeMemory(&pBlockApiInfo);
        return CMS_SEMP_ERROR;
    OsaDebugEnd();

    pBlockApiInfo->retCode      = CMS_RET_SUCC;
    pBlockApiInfo->cmsBlockFunc = blockFunc;
    pBlockApiInfo->timeOutMs    = timeOutMs;
    pBlockApiInfo->inputSize    = inputSize;
    pBlockApiInfo->outputSize   = outputSize;
    pBlockApiInfo->pInput       = pInput;
    pBlockApiInfo->pOutput      = pOutPut;

    /*
     * create signal
    */
    OsaCreateZeroSignal(SIG_CMS_BLOCK_API_REQ, sizeof(CmsBlockApiReq), &pSig);
    OsaCheck(pSig != PNULL, sizeof(CmsSynApiReq), 0, 0);

    pCmsBlockReq = (CmsBlockApiReq *)(pSig->sigBody);
    pCmsBlockReq->pApiInfo = pBlockApiInfo;

    /*
     * send signal to CMS task
    */
    OsaSendSignal(CMS_TASK_ID, &pSig);

    /*
     * wait for sem
    */
    if ((osState = osSemaphoreAcquire(pBlockApiInfo->sem, portMAX_DELAY)) == osOK)
    {
        if (pBlockApiInfo->retCode != CMS_RET_SUCC)
        {
            OsaDebugBegin(FALSE, pBlockApiInfo->retCode, CMS_RET_SUCC, 0);
            OsaDebugEnd();
        }
    }
    else
    {
        OsaDebugBegin(FALSE, osState, pBlockApiInfo->retCode, 0);
        OsaDebugEnd();

        pBlockApiInfo->retCode = CMS_SEMP_ERROR;
    }

    retErrCode = pBlockApiInfo->retCode;

    /*
     * Semaphore delete, and memory free
    */
    osSemaphoreDelete(pBlockApiInfo->sem);

    //OsaFreeMemory(&pBlockApiInfo);

    return retErrCode;
}


/**
  \fn           CmsRetId cmsNonBlockApiCall(CmsNonBlockCallback nonBlockFunc, UINT16 inputSize, void *pInput)
  \brief        API called in CMS task
  \param[in]
  \returns      CmsRetId
*/
CmsRetId cmsNonBlockApiCall(CmsNonBlockCallback nonBlockFunc, UINT16 inputSize, void *pInput)
{
    CmsNonBlockApiReq   *pCmsNonBlockReq = PNULL;
    SignalBuf           *pSig = PNULL;

    const CHAR      *pTaskName = PNULL;
    UINT16          inputSize4 = (inputSize+3)&0xFFFFFFFC;      /* 4 bytes aligned */

    /*
     * check the input parameters
    */
    OsaDebugBegin(nonBlockFunc != PNULL, nonBlockFunc, inputSize, pInput);
        return CMS_INVALID_PARAM;
    OsaDebugEnd();

    OsaDebugBegin((inputSize > 0 && pInput != PNULL) || (inputSize == 0 && pInput == PNULL),
                  nonBlockFunc, inputSize, pInput);
        return CMS_INVALID_PARAM;
    OsaDebugEnd();

    OsaDebugBegin(inputSize < CMS_SYN_API_BODY_MAX, nonBlockFunc, inputSize, pInput);
        return CMS_INVALID_PARAM;
    OsaDebugEnd();

    /*
     * this API can't be called in CMS task
    */
    pTaskName = osThreadGetName(osThreadGetId());

    if (pTaskName != PNULL)
    {
        if (strncasecmp(pTaskName, "CmsTask", strlen("CmsTask")) == 0)
        {
            //OsaDebugBegin(FALSE, nonBlockFunc, inputSize, pInput);
            //OsaDebugEnd();
            (*nonBlockFunc)(inputSize, pInput);

            return CMS_RET_SUCC;
        }
    }

    /*
     * create the signal
    */
    OsaCreateSignal(SIG_CMS_NON_BLOCK_API_REQ, sizeof(CmsNonBlockApiReq)+inputSize4, &pSig);
    OsaCheck(pSig != PNULL, sizeof(CmsNonBlockApiReq), inputSize4, 0);

    pCmsNonBlockReq = (CmsNonBlockApiReq *)(pSig->sigBody);
    pCmsNonBlockReq->cmsNonBlockFunc    = nonBlockFunc;

    pCmsNonBlockReq->paramSize  = inputSize;

    if (pCmsNonBlockReq->paramSize > 0)
    {
        memcpy(pCmsNonBlockReq->param,
               pInput,
               pCmsNonBlockReq->paramSize);
    }

    /*
     * send signal to CMS task
    */
    OsaSendSignal(CMS_TASK_ID, &pSig);

    return CMS_RET_SUCC;
}

