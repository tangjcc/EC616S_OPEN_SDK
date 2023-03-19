/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_sms_cnf_ind.h
*
*  Description: Process SMS related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef __ATEC_SMS_CNF_IND_H__
#define __ATEC_SMS_CNF_IND_H__

#include "at_util.h"
#include "atec_sms.h"
#include "ps_sms_if.h"


typedef struct AtecMtDeliverSmsInfo_Tag
{
    UINT8       msgFlag;
    UINT8       protocolId;
    UINT16      smsLength;          // length of body

    UINT8       srcAddr[PSIL_MSG_MAX_ADDR_LEN+1];
    PsilSmsTimeStampInfo    timeStamp;
    PsilSmsDcsInfo          dcsInfo;

    UINT8       smsBuf[512];        // msg body buffer
}AtecMtDeliverSmsInfo;




CmsRetId smsNewMsgAck(UINT32 reqHandle, UINT32 reply);
CmsRetId smsCMGSCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId smsCSCAGetCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId smsCSCASetCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId smsCSMSCurrentGetCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId smsCSMSSetCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId smsCSMSSupportedGetCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId smsCMMSSetCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId smsCMMSGetCnf(UINT16 reqHandle, UINT16 rc, void *paras);



CmsRetId smsNewSMSInd(void *paras);
void atSmsProcCmiCnf(CacCmiCnf *pCmiCnf);
void atSmsProcCmiInd(CacCmiInd *pCmiInd);

#endif

