/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    at_util.c
 * Description:
 * History:
 *
 ****************************************************************************/
#include "at_util.h"
#ifdef FEATURE_MBEDTLS_ENABLE
#include "sha1.h"
#include "sha256.h"
#include "md5.h"
#endif

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
  \fn          void atByteToBitString(UINT8 *outString, UINT8 n, UINT8 strlen)
  \brief       Convert byte (n) to bit string out put
  \param[out]  *outString     output string buffer, end with '\0'
  \param[in]   byte           byte
  \param[in]   outStrLen      how many bits (from LSB) need to output
  \returns     void
*/
void atByteToBitString(UINT8 *outString, UINT8 byte, UINT8 outStrLen)
{
    int     i = 0;

    /*
     * example:
     * atByteToBitString(out, 0x26, 8) => out = "00100110",
     * atByteToBitString(out, 0x26, 4) => out = "0110",
    */

    OsaCheck(outString != PNULL && outStrLen > 0 && outStrLen <= 8,
             outString, outStrLen, 0);

    for (i = 0; i < outStrLen; i++)
    {
        outString[i] = (((byte) >> (outStrLen-i-1)) & 0x01) + '0';
    }

    outString[outStrLen] = '\0';

    return;
}


/**
  \fn          BOOL atDataToHexString(UINT8 *outString, UINT8 *rawData, INT32 rawDataLen)
  \brief       Convert data (raw data) to HEX string out put
  \param[out]  *outString     output string buffer
  \param[in]   *rawData       raw data
  \param[in]   rawDataLen     raw data size in byte
  \returns     BOOL
*/
BOOL atDataToHexString(UINT8 *outString, UINT8 *rawData, INT32 rawDataLen)
{
    UINT8 hbit = 0, lbit = 0;
    UINT32 i = 0;
    CHAR *outP = (CHAR *)outString;

    /*
     * example:
     * rawData = 0x3456AD, rawDataLen = 3; => outString = "3456AD\0"
    */

    OsaDebugBegin(outString != PNULL && rawData != PNULL, outString, rawData, 0);
    return FALSE;
    OsaDebugEnd();

    for (i = 0; i < rawDataLen; i++)
    {
        hbit = ((*(rawData + i))&0xf0) >> 4;
        lbit = (*(rawData + i))&0x0f;

        if (hbit > 9)
        {
            outP[2*i] = 'A'+hbit-10;
        }
        else
        {
            outP[2*i] = '0'+hbit;
        }

        if (lbit > 9)
        {
            outP[2*i + 1] = 'A'+lbit-10;
        }
        else
        {
            outP[2*i + 1] = '0'+lbit;
        }
    }

    outP[2*i] = 0;

    return TRUE;
}


/**
  \fn   AtParaRet atGetNumValue(AtParamValueCP pAtParaList,
  \                             UINT32 index,
  \                             INT32 *pOutValue,
  \                             INT32 minValue,
  \                             INT32 maxValue,
  \                             INT32 defaultValue)
  \brief       Get INT value from AT PARAMETER list
  \param[in]   pAtParaList      AT PARAMETER list
  \param[in]   index            Which AT parameter in the list
  \param[out]  *pOutValue       output INT32 value
  \param[in]   minValue         acceptable minimum value
  \param[in]   maxValue         acceptable maximum value
  \param[in]   defaultValue     if not present in AT command, set this default value
  \returns     INT32
*/
AtParaRet atGetNumValue(AtParamValueCP pAtParaList,
                        UINT32 index,
                        INT32 *pOutValue,
                        INT32 minValue,
                        INT32 maxValue,
                        INT32 defaultValue)
{
    INT32 inValue;

    OsaDebugBegin(pAtParaList != PNULL && pOutValue != PNULL && minValue <= maxValue && index < AT_CMD_PARAM_MAX_NUM,
                  index, minValue, maxValue);
    return AT_PARA_ERR;
    OsaDebugEnd();


    OsaDebugBegin(pAtParaList[index].type == AT_DEC_VAL || pAtParaList[index].type == AT_HEX_VAL ||
                  pAtParaList[index].type == AT_BIN_VAL || pAtParaList[index].type == AT_MIX_VAL,
                  index, pAtParaList[index].type, 0);
    return AT_PARA_ERR;
    OsaDebugEnd();

    /*
     * whether default value
    */
    if (pAtParaList[index].bDefault)
    {
        *pOutValue = defaultValue;
        return AT_PARA_DEFAULT;
    }

    inValue = pAtParaList[index].value.numValue;

    /*
     * whether out of range
    */
    if (inValue > maxValue || inValue < minValue)
    {
        return AT_PARA_ERR;
    }
    else
    {
        *pOutValue = inValue;
    }

    return AT_PARA_OK;
}


/**
  \fn          AtParaRet atGetStrValue(AtParamValueCP pAtParaList,
  \                                    UINT32 index,
  \                                    UINT8  *pOutStr,
  \                                    UINT16 maxOutBufLen,
  \                                    UINT16 *pOutStrLen,
  \                                    const CHAR *pDefaultStr)
  \brief       Get string value from AT PARAMETER list
  \param[in]   pAtParaList      AT PARAMETER list
  \param[in]   index            PARM index
  \param[in]   pOutStr          output buffer
  \param[in]   maxOutBufLen     output buffer size (including string end byte: \0)
  \param[in]   pOutStrLen       output string size
  \param[in]   pDefaultStr      default string value
  \returns     INT32
*/
AtParaRet atGetStrValue(AtParamValueCP pAtParaList,
                        UINT32 index,
                        UINT8  *pOutStr,
                        UINT16 maxOutBufLen,
                        UINT16 *pOutStrLen,
                        const CHAR *pDefaultStr)
{
    INT16 length = 0;

    OsaCheck(pOutStr != PNULL && maxOutBufLen > 0 && pAtParaList != PNULL, index, pOutStr, maxOutBufLen);

    OsaDebugBegin(index < AT_CMD_PARAM_MAX_NUM && (pAtParaList[index].type == AT_STR_VAL || pAtParaList[index].type == AT_MIX_VAL),
                  index, pAtParaList[index].type, 0);
    return AT_PARA_ERR;
    OsaDebugEnd();

    *pOutStr = '\0';

    if (pOutStrLen != PNULL)
    {
        *pOutStrLen = 0;
    }

    /* check if it uses default values */
    if (pAtParaList[index].bDefault)
    {
        if (pDefaultStr != PNULL)
        {
            length = strlen(pDefaultStr);

            if (length + 1 > maxOutBufLen)  //strlen not contain '\0', so here should "+1"
            {
                return AT_PARA_ERR;
            }
            memmove(pOutStr, pDefaultStr, length);
            pOutStr[length] = '\0';

            if (pOutStrLen != PNULL)
            {
                *pOutStrLen = length;
            }
        }

        return AT_PARA_DEFAULT;
    }

    OsaDebugBegin(pAtParaList[index].value.pStr != PNULL, index, pAtParaList[index].value.pStr, 0);
    return AT_PARA_ERR;
    OsaDebugEnd();

    length = strlen((CHAR *)pAtParaList[index].value.pStr);

    if (length + 1 > maxOutBufLen)  //strlen not contain '\0', so here should "+1"
    {
        return AT_PARA_ERR;
    }
    memmove(pOutStr, pAtParaList[index].value.pStr, length);
    pOutStr[length] = '\0';

    if (pOutStrLen != PNULL)
    {
        *pOutStrLen = length;
    }

    return AT_PARA_OK;
}

/**
  \fn          AtParaRet atGetStrValue(AtParamValueCP pAtParaList,
  \                                    UINT32 index,
  \                                    UINT8  *pOutStr,
  \                                    UINT16 maxOutBufLen,
  \                                    UINT16 *pOutStrLen,
  \                                    const CHAR *pDefaultStr)
  \brief       Get JSON string value from AT PARAMETER list
  \param[in]   pAtParaList      AT PARAMETER list
  \param[in]   index            PARM index
  \param[in]   pOutStr          output buffer
  \param[in]   maxOutBufLen     output buffer size (including string end byte: \0)
  \param[in]   pOutStrLen       output string size
  \param[in]   pDefaultStr      default string value
  \returns     AtParaRet
*/
AtParaRet atGetJsonStrValue(AtParamValueCP pAtParaList,
                            UINT32 index,
                            UINT8  *pOutStr,
                            UINT16 maxOutBufLen,
                            UINT16 *pOutStrLen,
                            const CHAR *pDefaultStr)
{
    INT16 length = 0;

    OsaCheck(pOutStr != PNULL && maxOutBufLen > 0 && pAtParaList != PNULL, index, pOutStr, maxOutBufLen);

    OsaDebugBegin(index < AT_CMD_PARAM_MAX_NUM && pAtParaList[index].type == AT_JSON_VAL, index, pAtParaList[index].type, 0);
    return AT_PARA_ERR;
    OsaDebugEnd();

    *pOutStr = '\0';

    if (pOutStrLen != PNULL)
    {
        *pOutStrLen = 0;
    }

    /* check if it uses default values */
    if (pAtParaList[index].bDefault)
    {
        if (pDefaultStr != PNULL)
        {
            length = strlen(pDefaultStr);

            if (length + 1 > maxOutBufLen)  //strlen not contain '\0', so here should "+1"
            {
                return AT_PARA_ERR;
            }
            memmove(pOutStr, pDefaultStr, length);
            pOutStr[length] = '\0';

            if (pOutStrLen != PNULL)
            {
                *pOutStrLen = length;
            }
        }

        return AT_PARA_DEFAULT;
    }

    OsaDebugBegin(pAtParaList[index].value.pStr != PNULL, index, pAtParaList[index].value.pStr, 0);
    return AT_PARA_ERR;
    OsaDebugEnd();

    length = strlen((CHAR *)pAtParaList[index].value.pStr);

    if (length + 1 > maxOutBufLen)  //strlen not contain '\0', so here should "+1"
    {
        return AT_PARA_ERR;
    }
    memmove(pOutStr, pAtParaList[index].value.pStr, length);
    pOutStr[length] = '\0';

    if (pOutStrLen != PNULL)
    {
        *pOutStrLen = length;
    }

    return AT_PARA_OK;
}



/**
  \fn          AtParaRet atGetMixValue(AtParamValueCP pAtParaList,
  \                                    UINT32  index,
  \                                    UINT32  *extValType,
  \                                    UINT8   *pOutStr,
  \                                    UINT16  maxOutBufLen,
  \                                    UINT16  *pOutStrLen,
  \                                    const CHAR *pDefaultStr,
  \                                    INT32  *pOutValue,
  \                                    INT32  minValue,
  \                                    INT32  maxValue,
  \                                    INT32  defaultValue)
  \brief       Get string value, or UINT value from AT PARAMETER list
  \param[in]
  \returns     INT32
*/
AtParaRet atGetMixValue(AtParamValueCP pAtParaList,
                        UINT32  index,
                        UINT32  *extValType,
                        UINT8   *pOutStr,
                        UINT16  maxOutBufLen,
                        UINT16  *pOutStrLen,
                        const CHAR *pDefaultStr,
                        INT32  *pOutValue,
                        INT32  minValue,
                        INT32  maxValue,
                        INT32  defaultValue)
{
    UINT32  length = 0;
    OsaDebugBegin(pAtParaList != PNULL && extValType != PNULL && pOutStr != PNULL && pOutStrLen != PNULL &&
                  pOutValue != PNULL && minValue <= maxValue, extValType, minValue, maxValue);
    return AT_PARA_ERR;
    OsaDebugEnd();

    *extValType = (UINT32)pAtParaList[index].type;

    if (*extValType == AT_DEC_VAL)
    {
        return atGetNumValue(pAtParaList, index, pOutValue, minValue, maxValue, defaultValue);
    }
    else if (*extValType == AT_STR_VAL)
    {
        return atGetStrValue(pAtParaList, index, pOutStr, maxOutBufLen, pOutStrLen, pDefaultStr);
    }
    else if (*extValType == AT_MIX_VAL) //must be default value
    {
        OsaDebugBegin(pAtParaList[index].bDefault == TRUE, pAtParaList[index].bDefault, index, 0);
        return AT_PARA_ERR;
        OsaDebugEnd();

        /* set the default value */
        *pOutValue = defaultValue;

        if (pDefaultStr != PNULL)
        {
            length = strlen(pDefaultStr);

            if (length + 1 > maxOutBufLen)  //strlen not contain '\0', so here should "+1"
            {
                return AT_PARA_ERR;
            }
            memmove(pOutStr, pDefaultStr, length);
            pOutStr[length] = '\0';

            if (pOutStrLen != PNULL)
            {
                *pOutStrLen = length;
            }
        }

        return AT_PARA_DEFAULT;
    }

    OsaDebugBegin(FALSE, *extValType, index, 0);
    OsaDebugEnd();

    return AT_PARA_ERR;
}

/**
  \fn          AtParaRet atGetLastMixStrValue(AtParamValueCP pAtParaList,
  \                                           UINT32 index,
  \                                           UINT8  *pOutStr,
  \                                           UINT16 maxOutBufLen,
  \                                           UINT16 *pOutStrLen,
  \                                           const CHAR *pDefaultStr)
  \brief       Get LAST MIX string value from AT PARAMETER list
  \param[in]   pAtParaList      AT PARAMETER list
  \param[in]   index            PARM index
  \param[in]   pOutStr          output buffer
  \param[in]   maxOutBufLen     output buffer size (including string end byte: \0)
  \param[in]   pOutStrLen       output string size
  \param[in]   pDefaultStr      default string value
  \returns     AtParaRet
*/
AtParaRet atGetLastMixStrValue(AtParamValueCP pAtParaList,
                               UINT32 index,
                               UINT8  *pOutStr,
                               UINT16 maxOutBufLen,
                               UINT16 *pOutStrLen,
                               const CHAR *pDefaultStr)
{
    INT16 length = 0;

    OsaCheck(pOutStr != PNULL && maxOutBufLen > 0 && pAtParaList != PNULL, index, pOutStr, maxOutBufLen);

    OsaDebugBegin(index < AT_CMD_PARAM_MAX_NUM && pAtParaList[index].type == AT_LAST_MIX_STR_VAL, index, pAtParaList[index].type, 0);
    return AT_PARA_ERR;
    OsaDebugEnd();

    *pOutStr = '\0';

    if (pOutStrLen != PNULL)
    {
        *pOutStrLen = 0;
    }

    /* check if it uses default values */
    if (pAtParaList[index].bDefault)
    {
        if (pDefaultStr != PNULL)
        {
            length = strlen(pDefaultStr);

            if (length + 1 > maxOutBufLen)  //strlen not contain '\0', so here should "+1"
            {
                return AT_PARA_ERR;
            }
            memmove(pOutStr, pDefaultStr, length);
            pOutStr[length] = '\0';

            if (pOutStrLen != PNULL)
            {
                *pOutStrLen = length;
            }
        }

        return AT_PARA_DEFAULT;
    }

    OsaDebugBegin(pAtParaList[index].value.pStr != PNULL, index, pAtParaList[index].value.pStr, 0);
    return AT_PARA_ERR;
    OsaDebugEnd();

    length = strlen((CHAR *)pAtParaList[index].value.pStr);

    if (length + 1 > maxOutBufLen)  //strlen not contain '\0', so here should "+1"
    {
        return AT_PARA_ERR;
    }
    memmove(pOutStr, pAtParaList[index].value.pStr, length);
    pOutStr[length] = '\0';

    if (pOutStrLen != PNULL)
    {
        *pOutStrLen = length;
    }

    return AT_PARA_OK;
}



/**
  \fn          AtParaRet atGetStrLength(AtParamValueCP pAtParaList,
  \                                     UINT32 index,
  \                                     UINT16 *pOutStrLen)
  \brief       Get string value from AT PARAMETER list
  \param[in]   pAtParaList      AT PARAMETER list
  \param[in]   index            PARM index
  \param[in]   pOutStrLen       output string size
  \returns     INT32
*/
AtParaRet atGetStrLength(AtParamValueCP pAtParaList,
                         UINT32 index,
                         UINT16 *pOutStrLen)
{
    INT16 length = 0;

    OsaDebugBegin(index < AT_CMD_PARAM_MAX_NUM && pAtParaList[index].type != AT_DEC_VAL &&
                  pAtParaList[index].type != AT_HEX_VAL && pAtParaList[index].type != AT_BIN_VAL,
                  index, pAtParaList[index].type, 0);
    return AT_PARA_ERR;
    OsaDebugEnd();

    if (pAtParaList[index].value.pStr != PNULL)
    {
        length = strlen((CHAR *)pAtParaList[index].value.pStr);
    }

    if (pOutStrLen != PNULL)
    {
        *pOutStrLen = length;
    }

    return AT_PARA_OK;
}

/**
  \fn           BOOL atBeNumericString(const UINT8 *pStr)
  \brief        whether the input string only contain numeric value
  \param[in]    pStr    input string
  \returns      BOOL
*/
BOOL atBeNumericString(const UINT8 *pStr)
{
    INT16   count = 0;

    /*
     * example:
     * "\0", return: FALSE
     * "1293", return: TRUE
     * "1293A", return: FALSE
    */

    OsaDebugBegin(pStr != PNULL || *pStr == '\0', pStr, 0, 0);
    return FALSE;
    OsaDebugEnd();

    while (TRUE)
    {
        if (pStr[count] == '\0')    //end
        {
            break;
        }
        if (pStr[count] < '0' ||
            pStr[count] > '9')
        {
            return FALSE;
        }

        count++;
    }

    return TRUE;
}

/**
  \fn           INT32 atCheckParaDefaultFlag(const AtCmdInputContext *pAtCmdReqParaPtr, INT32 index)
  \brief        whether the parameter (indexed by "index") in the input AT CMD is default value
  \param[in]    pAtCmdReqParaPtr        input ATCMD info
  \param[in]    index                   parameter index
  \returns      INT32
*/
AtParaRet atCheckParaDefaultFlag(const AtCmdInputContext *pAtCmdReqParaPtr, UINT32 index)
{
    OsaDebugBegin(pAtCmdReqParaPtr != PNULL && index < pAtCmdReqParaPtr->paramMaxNum,
                  pAtCmdReqParaPtr, index, pAtCmdReqParaPtr->paramMaxNum);
    return  AT_PARA_ERR;
    OsaDebugEnd();

    /*
     * if default value, return: AT_PARA_DEFAULT
    */

    /* check if it uses default values */
    if (pAtCmdReqParaPtr->pParamList[index].bDefault)
    {
        return AT_PARA_DEFAULT;
    }

    return AT_PARA_OK;
}

/**
  \fn           void atStrnCat(CHAR *pDest, CHAR *pNew, UINT32 destBufSize, UINT32 catLen)
  \brief        safe strncat
  \param[in/out]    pDest        dest string
  \param[in]        destBufSize  dest string buffer size
  \param[in]        pNew         new string
  \param[in]        catLen       new string lenght
  \returns      BOOL
*/
BOOL atStrnCat(CHAR *pDest, UINT32 destBufSize, CHAR *pNew, UINT32 catLen)
{
    UINT32  oldStrLen = 0;

    OsaCheck(pDest != PNULL && pNew != PNULL && destBufSize > 0,
             pDest, pNew, destBufSize);

    if (catLen == 0)
    {
        OsaDebugBegin(FALSE, catLen, destBufSize, 0);
        OsaDebugEnd();
        return TRUE;
    }

    oldStrLen = strlen(pDest);

    if (destBufSize >= (oldStrLen + catLen))
    {
        strncat(pDest, pNew, catLen);

        return TRUE;
    }

    OsaDebugBegin(FALSE, destBufSize, oldStrLen, catLen);
    OsaDebugEnd();

    return FALSE;
}



/****************************************************************************************
*  FUNCTION:     atCheckBitFormat
*
*  PARAMETERS:
*
*  DESCRIPTION:
*
*  RETURNS:
*
****************************************************************************************/
BOOL atCheckBitFormat(const UINT8 *input)
{
    INT8    inputLen = 0;
    INT8    count = 0;
    BOOL    result = TRUE;

    inputLen = strlen((const CHAR*)input);

    if (inputLen > 0)
    {
        /* The input should be only '0' or '1'  */
        for (count = 0; count < inputLen; count++)
        {
            if ((input[count] != '0') && (input[count] != '1'))
            {
                result = FALSE;
                break;
            }
        }
    }

    return (result);
}




/**
  \fn   AtParaRet atCheckApnName(const CHAR *apn)
  \brief       check the apn type whether is  invalid
  \param[in]   (const CHAR *apn)     the input apn name string
  \returns     BOOL ,TRUE or FALSE
*/
BOOL atCheckApnName(UINT8 *apn,UINT16  len)
{
    CHAR   *pstrBegin = NULL;
    CHAR    *ptemp    = NULL;
    UINT16  NetworkIdLen = 0;

    if(apn == NULL)
    {
        return FALSE;
    }

    /*allow to configurate a NULL apnname ,such as default eps APN*/
    if(*apn == 0)
    {
       return  TRUE;
    }

    pstrBegin = (CHAR *)apn;
    while(pstrBegin  && *pstrBegin != '\0')
    {
        /*invalid aipha exclude that '.' and '-' according 3GPP 23.003 9.1 CHAPTER*/
        if( (*pstrBegin != 0x2E && *pstrBegin != 0x2D && *pstrBegin != 0x5F)  &&
            (*pstrBegin <= 0x2F   ||
            (*pstrBegin >= 0X5B  && *pstrBegin <= 0X60 )  ||
            (*pstrBegin >= 0X7B  && *pstrBegin <= 0X7E )))
         {
            return FALSE;
         }
         pstrBegin++;
    }

    /*according 23.003 CHAPTER 9.1.1 APN network identifier, it should not including fellow contents*/
    pstrBegin = NULL;
    if((pstrBegin  = strstr(( CHAR *)apn,"MNC"))   != NULL ||
        (pstrBegin = strstr(( CHAR *)apn,"mnc"))   != NULL  )
    {
        NetworkIdLen =  (UINT16)((UINT8 *)pstrBegin-(UINT8 *)apn -1);
        if(NetworkIdLen > 0)
        {
            if(((ptemp  = strstr(( CHAR *)apn,"RAC"))   != NULL ||
                (ptemp = strstr(( CHAR *)apn,"rac"))    != NULL ||
                (ptemp = strstr(( CHAR *)apn,"SGSN"))   != NULL ||
                (ptemp = strstr(( CHAR *)apn,"sgsn"))   != NULL ||
                (ptemp = strstr(( CHAR *)apn,"RNC"))    != NULL ||
                (ptemp = strstr(( CHAR *)apn,"rnc"))    != NULL ||
                (ptemp = strstr(( CHAR *)apn,"GPRS"))   != NULL
                ) &&( ptemp    < pstrBegin))
            {
                return FALSE;
            }
        }
    }

    return TRUE;

}


#ifdef FEATURE_MBEDTLS_ENABLE

char atHb2Hex(unsigned char hb)
{
    hb = hb&0xF;
    return (char)(hb<10 ? '0'+hb : hb-10+'a');
}

/*
 * output = SHA-1( input buffer )
 */
void atAliHmacSha1(const unsigned char *input, int ilen, unsigned char *output,const unsigned char *key, int keylen)
{
    int i;
    mbedtls_sha1_context ctx;
    unsigned char k_ipad[ALI_SHA1_KEY_IOPAD_SIZE] = {0};
    unsigned char k_opad[ALI_SHA1_KEY_IOPAD_SIZE] = {0};
    unsigned char tempbuf[ALI_SHA1_DIGEST_SIZE];

    memset(k_ipad, 0x36, ALI_SHA1_KEY_IOPAD_SIZE);
    memset(k_opad, 0x5C, ALI_SHA1_KEY_IOPAD_SIZE);

    for(i=0; i<keylen; i++)
    {
        if(i>=ALI_SHA1_KEY_IOPAD_SIZE)
        {
            break;
        }
        k_ipad[i] ^=key[i];
        k_opad[i] ^=key[i];
    }
    mbedtls_sha1_init(&ctx);

    mbedtls_sha1_starts(&ctx);
    mbedtls_sha1_update(&ctx, k_ipad, ALI_SHA1_KEY_IOPAD_SIZE);
    mbedtls_sha1_update(&ctx, input, ilen);
    mbedtls_sha1_finish(&ctx, tempbuf);

    mbedtls_sha1_starts(&ctx);
    mbedtls_sha1_update(&ctx, k_opad, ALI_SHA1_KEY_IOPAD_SIZE);
    mbedtls_sha1_update(&ctx, tempbuf, ALI_SHA1_DIGEST_SIZE);
    mbedtls_sha1_finish(&ctx, tempbuf);

    for(i=0; i<ALI_SHA1_DIGEST_SIZE; ++i)
    {
        output[i*2] = atHb2Hex(tempbuf[i]>>4);
        output[i*2+1] = atHb2Hex(tempbuf[i]);
    }
    mbedtls_sha1_free(&ctx);
}
/*
 * output = SHA-256( input buffer )
 */
void atAliHmacSha256(const unsigned char *input, int ilen, unsigned char *output,const unsigned char *key, int keylen)
{
    int i;
    mbedtls_sha256_context ctx;
    unsigned char k_ipad[ALI_SHA256_KEY_IOPAD_SIZE] = {0};
    unsigned char k_opad[ALI_SHA256_KEY_IOPAD_SIZE] = {0};

    memset(k_ipad, 0x36, 64);
    memset(k_opad, 0x5C, 64);

    if ((NULL == input) || (NULL == key) || (NULL == output)) {
        return;
    }

    if (keylen > ALI_SHA256_KEY_IOPAD_SIZE) {
        return;
    }

    for(i=0; i<keylen; i++)
    {
        if(i>=ALI_SHA256_KEY_IOPAD_SIZE)
        {
            break;
        }
        k_ipad[i] ^=key[i];
        k_opad[i] ^=key[i];
    }
    mbedtls_sha256_init(&ctx);

    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, k_ipad, ALI_SHA256_KEY_IOPAD_SIZE);
    mbedtls_sha256_update(&ctx, input, ilen);
    mbedtls_sha256_finish(&ctx, output);

    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, k_opad, ALI_SHA256_KEY_IOPAD_SIZE);
    mbedtls_sha256_update(&ctx, output, ALI_SHA256_DIGEST_SIZE);
    mbedtls_sha256_finish(&ctx, output);

    mbedtls_sha256_free(&ctx);
}
/*
 * output = MD-5( input buffer )
 */
void atAliHmacMd5(const unsigned char *input, int ilen, unsigned char *output,const unsigned char *key, int keylen)
{
    int i;
    mbedtls_md5_context ctx;
    unsigned char k_ipad[ALI_MD5_KEY_IOPAD_SIZE] = {0};
    unsigned char k_opad[ALI_MD5_KEY_IOPAD_SIZE] = {0};
    unsigned char tempbuf[ALI_MD5_DIGEST_SIZE];

    memset(k_ipad, 0x36, ALI_MD5_KEY_IOPAD_SIZE);
    memset(k_opad, 0x5C, ALI_MD5_KEY_IOPAD_SIZE);

    for(i=0; i<keylen; i++)
    {
        if(i>=ALI_MD5_KEY_IOPAD_SIZE)
        {
            break;
        }
        k_ipad[i] ^=key[i];
        k_opad[i] ^=key[i];
    }
    mbedtls_md5_init(&ctx);

    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, k_ipad, ALI_MD5_KEY_IOPAD_SIZE);
    mbedtls_md5_update(&ctx, input, ilen);
    mbedtls_md5_finish(&ctx, tempbuf);

    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, k_opad, ALI_MD5_KEY_IOPAD_SIZE);
    mbedtls_md5_update(&ctx, tempbuf, ALI_MD5_DIGEST_SIZE);
    mbedtls_md5_finish(&ctx, tempbuf);

    for(i=0; i<ALI_MD5_DIGEST_SIZE; ++i)
    {
        output[i*2] = atHb2Hex(tempbuf[i]>>4);
        output[i*2+1] = atHb2Hex(tempbuf[i]);
    }
    mbedtls_md5_free(&ctx);
}
int ali_hmac_falg = ALI_HMAC_USED;
#else
int ali_hmac_falg = ALI_HMAC_NOT_USED;
#endif

