/******************************************************************************
 * (C) Copyright 2018 EIGENCOMM International Ltd.
 * All Rights Reserved
*******************************************************************************
 *  Filename: at_api.h
 *
 *  Description: AT CMD entity/task (in CMS task) api
 *
 *  History:
 *
 *  Notes: All other tasks if need to use AT CMD functions, should ONLY include this header file
 *
******************************************************************************/
#ifndef __AT_API_H__
#define __AT_API_H__

#include <stdint.h>
#include "CommonTypedef.h"
#include "CMSIS_OS2.h"
#include "cms_api.h"
#include "cms_util.h"

/******************************************************************************
 *****************************************************************************
 * EXTERNAL COMMON MARCO
 *****************************************************************************
******************************************************************************/
#define AT_CHAN_NAME_SIZE   8
#define AT_UART_PRINT_MAX   256


/*
 * DEFAULT AT CHANNEL NAME, if registered, assign the fixed chanID: AT_CHAN_DEFAULT
*/
#define AT_DEFAULT_CHAN_NAME        "UARTAT"
#define AT_DEFAULT_CHAN_NAME_LEN    6

/*
 * AT CMD MAC string size
*/
#define AT_CMD_STR_MAX_LEN                  3072


typedef void (*AtRespFunctionP)(const CHAR *pStr, UINT32 strLen, void *pArg);
typedef void (*AtUrcFunctionP)(const CHAR *pStr, UINT32 strLen);



/******************************************************************************
 *****************************************************************************
 * EXTERNAL COMMON STRUCT
 *****************************************************************************
******************************************************************************/

/*
 * SigId: SIG_AT_CMD_STR_REQ
*/
typedef struct AtCmdStrReq_Tag
{
    UINT8       atChanId;       //AT channel ID
    UINT8       rsvd0;

    #if 0   //if AT STRING need to pass a copy in the signal, not suggest
    UINT16      atStrLen;
    UINT8       atStr[];
    #else
    UINT16      atStrLen;
    CHAR        *pAtStr;        //memory allocated in heap by OsaAllocateMemory()
    #endif
}AtCmdStrReq;


/*
*/
typedef struct AtChanRegInfo_tag
{
    CHAR    chanName[AT_CHAN_NAME_SIZE];

    AtRespFunctionP atRespFunc;
    void            *pRespArg;  //passed in atRespFunc(str, pRespArg);

    AtUrcFunctionP  atUrcFunc;
}AtChanRegInfo; //20 bytes


/*
 * SIGID: SIG_AT_CMD_CONTINUE_REQ
*/
typedef struct AtCmdContinueReq_tag
{
    UINT8       atChanId;       //AT channel ID
    UINT8       rsvd0;
    UINT16      rsvd1;
}AtCmdContinueReq;


/*
*/
typedef struct AtRilAtCmdReqData_Tag
{
    CHAR    *pCmdLine;      /* memory alloacted in heap by: OsaAllocateMemory() */

    UINT16  cmdLen;
    UINT16  rsvd;

    AtRespFunctionP     respCallback;
    void    *respData;
}AtRilAtCmdReqData; //16 bytes

/******************************************************************************
 *****************************************************************************
 * EXTERNAL API
 *****************************************************************************
******************************************************************************/

/*
 * register a AT channel;
 * 1> if succ, return the "channId" (>=0);
 * 2> else, return "CmsRetId" (< 0)
*/
INT32 atRegisterAtChannel(AtChanRegInfo *pAtRegInfo);

/*
 * send the AT command to AT CMD task: CMS_TASK_ID
*/
void atSendAtcmdStrSig(UINT8 atChanId, const UINT8 *pCmdStr, UINT32 len);

/*
 * at UART print callback
*/
void atUartPrintCallback(UINT16 paramSize, void *pParam);

/*
 * print LOG to AT UART port
*/
#define atUartPrint(fmt, ...)                                   \
do {                                                            \
    CHAR *PSTRUARTPRINT = (CHAR *)OsaAllocMemory(AT_UART_PRINT_MAX);     \
    if (PSTRUARTPRINT == PNULL)                                          \
    {                                                           \
        OsaDebugBegin(FALSE, AT_UART_PRINT_MAX, 0, 0);          \
        OsaDebugEnd();                                          \
    }                                                           \
    else                                                        \
    {                                                           \
        snprintf(PSTRUARTPRINT, AT_UART_PRINT_MAX, fmt, ##__VA_ARGS__);      \
                                                                \
        cmsNonBlockApiCall(atUartPrintCallback, sizeof(void *), &PSTRUARTPRINT); \
    }   \
}while(FALSE)


/*
 * AT RIL API
*/
CmsRetId atRilAtCmdReq(const CHAR *pAtCmdLine, UINT32 cmdLen, AtRespFunctionP respCallback, void *respData, UINT32 timeOutMs);

/*
 * register AT RIL URC callback
*/
CmsRetId atRilRegisterUrcCallback(AtUrcFunctionP urcCallback);

/*
 * unregister AT RIL URC callback
*/
CmsRetId atRilUnRegisterUrcCallback(void);

#endif

