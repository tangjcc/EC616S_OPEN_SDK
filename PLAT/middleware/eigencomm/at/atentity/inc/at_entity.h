/******************************************************************************
 * (C) Copyright 2018 EIGENCOMM International Ltd.
 * All Rights Reserved
*******************************************************************************
 *  Filename: at_entity.h
 *
 *  Description: Entracne of AT CMD
 *
 *  History:
 *
 *  Notes:
 *
******************************************************************************/

#ifndef __AT_ENTITY_H__
#define __AT_ENTITY_H__
#include "osasys.h"
#include "mw_config.h"
#include "atc_decoder.h"

/******************************************************************************
 *****************************************************************************
 * MARCO
 *****************************************************************************
******************************************************************************/
#define AT_QUEUE_MAX_LEN                    16
#define UART_EVENT_RX_FLAG                  0x01
#define UART_EVENT_OPT                      0x00
#define UART_EVENT_TIME_OUT                 0xffffffffUL
#define UART_BUF_LEN                        16
#define READ_WAIT                           1
#define READ_DATA                           2
#define CMI_SYNC_MAX_REQUESTS               10
#define AT_CMD_FLAG_AT1                     0x1
#define AT_CMD_FLAG_AT2                     0x2
#define AT_CMD_FLAG_AT3                     0x3


/******************************************************************************
 *****************************************************************************
 * ENUM
 *****************************************************************************
******************************************************************************/
#if 0
typedef enum {
    SEQ_01  = 0,
    SEQ_02  = 1,
    SEQ_03  = 2,
    SEQ_04  = 3,
    SEQ_05  = 4,
    SEQ_06  = 5,
    SEQ_07  = 6,
    SEQ_08  = 7,
    SEQ_09  = 8,
    SEQ_10  = 9,
    SEQ_11 = 10,
    SEQ_12 = 11,
    SEQ_13,
    SEQ_14,
    SEQ_15,

    SEQ_MAX,
} AtSEQ;

typedef enum {
    ATC_REQ = 1,
    ATC_CNF,
    ATC_IND,
}AtFuncType;

/******************************************************************************
 *****************************************************************************
 * STRUCT
 *****************************************************************************
******************************************************************************/
typedef struct AtCmdProcSeqTbl_
{
    INT32 funcIndex;
    INT32 (* ProcFunc)(const SignalBuf *arg);
    //SignalBuf *ProcFuncArg;
    //INT32 ProcFuncRet;
    UINT32 nextFuncSeqNumb[2];
}AtCmdProcSeqTbl;

/*... Configuration ---------------------------------------------------------*/


typedef struct ATCmd_TimeOut{
    UINT32 index;
    const CHAR *pName;
    UINT32 time_second;
}ATCMD_TimeOut;

/*
 * AT Channel control info, seem useless, -TBD
*/
typedef struct atChanCtrlInfo_tag
{
    void *taskRef;
    INT32 iFd;
    CHAR *path;
    UINT32 index;
    BOOL bSmsDataEntryModeOn;
    BOOL bEnable;
}atChanCtrlInfo;
#endif

/*
 * AT APP to process the "SIG_CMS_APPL_CNF" signal
*/
typedef void (*AtProcApplCnfFunc)(CmsApplCnf *pApplCnf);

typedef struct AtApplCnfFuncTable_Tag
{
    UINT32  appId;
    AtProcApplCnfFunc   applCnfFunc;
}AtApplCnfFuncTable;

/*
 * AT APP to process the "SIG_CMS_APPL_IND" signal
*/
typedef void (*AtProcApplIndFunc)(CmsApplInd *pApplInd);

typedef struct AtApplIndFuncTable_Tag
{
    UINT32  appId;
    AtProcApplIndFunc   applIndFunc;
}AtApplIndFuncTable;


/******************************************************************************
 *****************************************************************************
 * API
 *****************************************************************************
******************************************************************************/

/*
 * process signal
*/
void atProcSignal(const SignalBuf *pSig);

/*
 * AT module init
*/
void atInit(void);

#endif

