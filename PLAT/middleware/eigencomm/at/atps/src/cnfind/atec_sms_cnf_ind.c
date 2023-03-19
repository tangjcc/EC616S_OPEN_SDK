/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_sms.c
*
*  Description: Process SMS related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdio.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "cmisms.h"
#include "atec_sms.h"
#include "atec_sms_cnf_ind.h"
#include "ps_sms_if.h"
#include "mw_config.h"

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


/******************************************
*  FUNCTION:  smsNewMsgAck
*
*  PARAMETERS:
*
*  DESCRIPTION: Interface to do new message acknowledgement, implementation of AT+CNMA (txt mode) or *CNMA=<n> (PDU mode)
*
*  RETURNS: CmsRetId
***************************************************/
CmsRetId smsNewMsgAck(UINT32 reqHandle, UINT32 reply)
{
    CmiSmsNewMsgRsp newMsgRsp;
    CmsRetId ret = CMS_FAIL;

    memset(&newMsgRsp, 0, sizeof(CmiSmsNewMsgRsp));
    if (reply == 0 )
    {
        newMsgRsp.bRPAck = TRUE;
    }
    else if ( reply == 1 )
    {
        newMsgRsp.bRPAck = FALSE;
    }
    newMsgRsp.smsId = 0;
    CacCmiRequest((UINT16)reqHandle, CAC_SMS, CMI_SMS_NEW_MSG_RSP, sizeof(CmiSmsNewMsgRsp), (UINT8 *)&newMsgRsp);

    ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    return ret;

}


/****************************************************************************************
*  Process CMI cnf msg of AT+CMGS
*
****************************************************************************************/
CmsRetId smsCMGSCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[16];
    CmiSmsSendMsgCnf *sendSmsConf = (CmiSmsSendMsgCnf *)paras;

    memset(RspBuf, 0, sizeof(RspBuf));

    if (rc == CMS_SMS_SUCC)
    {
        sprintf((char*)RspBuf, "+CMGS: %d", sendSmsConf->msgReference);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        UINT16 Error;
        Error = errorCauseofSendSms(rc, sendSmsConf->rpCause, sendSmsConf->tpCause);
        ret = atcReply(reqHandle, AT_RC_CMS_ERROR, Error, NULL);
    }
    return ret;
}

/****************************************************************************************
*  Process CMI cnf msg of AT+CSCA?
*
****************************************************************************************/
CmsRetId smsCSCAGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId                ret = CMS_FAIL;
    CHAR                    AtRspBuf[64], TempBuf[48];
    CmiSmsGetSmscAddrCnf    *getSmsCenterAddrCnf = (CmiSmsGetSmscAddrCnf*)paras;

    memset(AtRspBuf, 0, sizeof(AtRspBuf));
    memset(TempBuf, 0, sizeof(TempBuf));

    if (rc == CMS_SMS_SUCC)
    {
        const char *plus = NULL;

        if (getSmsCenterAddrCnf->smscAddr.addrType.numType == CMI_SMS_NUMTYPE_INTERNATIONAL)
        {
            plus = "+";
        }
        else
        {
            plus = "";
        }

        memcpy(TempBuf, getSmsCenterAddrCnf->smscAddr.digits, getSmsCenterAddrCnf->smscAddr.len);
        TempBuf[getSmsCenterAddrCnf->smscAddr.len] = '\0';

        sprintf((char*)AtRspBuf, "+CSCA: \"%s%s\",%d", plus, TempBuf,
                ((0x80 | ((getSmsCenterAddrCnf->smscAddr.addrType.numType) << 4)) | (getSmsCenterAddrCnf->smscAddr.addrType.numPlan)));

        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)AtRspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CMS_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atSmsCSCASetCnf
 * Description: Process CMI cnf msg of AT+CSCA
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId smsCSCASetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if (rc == CMS_SMS_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CMS_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atSmsCSMSSetCnf
 * Description: Process CMI cnf msg of AT+CSMS
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId smsCSMSSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if (rc == CMS_SMS_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CMS_ERROR, rc, NULL);
    }
    return ret;
}

/****************************************************************************************
*  Process CMI cnf msg of AT+CSMS?
*
****************************************************************************************/
CmsRetId smsCSMSCurrentGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR AtRspBuf[64];
    CmiSmsGetCurrentSmsServiceCnf *getCurrentSmsServiceCnf = (CmiSmsGetCurrentSmsServiceCnf*)paras;

    memset(AtRspBuf, 0, sizeof(AtRspBuf));

    if (rc == CMS_SMS_SUCC)
    {
        sprintf((char*)AtRspBuf, "+CSMS: %d,%d,%d,%d", getCurrentSmsServiceCnf->smsService,
                getCurrentSmsServiceCnf->msgType.mtSupported,
                getCurrentSmsServiceCnf->msgType.moSupported,
                getCurrentSmsServiceCnf->msgType.bmSupported);

        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)AtRspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CMS_ERROR, rc, NULL);
    }
    return ret;
}

/****************************************************************************************
*  Process CMI cnf msg of AT+CSMS=?
*
****************************************************************************************/
CmsRetId smsCSMSSupportedGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR AtRspBuf[100], tempBuf[30];
    UINT8 i;

    CmiSmsGetSupportedSmsServiceCnf *getSupportedSmsServiceCnf = (CmiSmsGetSupportedSmsServiceCnf*)paras;

    memset(AtRspBuf, 0, sizeof(AtRspBuf));

    if (rc == CMS_SMS_SUCC)
    {
        sprintf((char*)AtRspBuf, "+CSMS: (%d", getSupportedSmsServiceCnf->serviceList[0]);

        for (i = 1; i < getSupportedSmsServiceCnf->length; i++ )
        {
            sprintf((CHAR *)tempBuf, ",%d", getSupportedSmsServiceCnf->serviceList[i]);
            strcat((CHAR *)AtRspBuf, (CHAR *)tempBuf);
        }
        strcat((CHAR *)AtRspBuf, (CHAR *)")");
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)AtRspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CMS_ERROR, rc, NULL);
    }
    return ret;
}

/****************************************************************************************
*  Process CMI cnf msg of AT+CMMS=<mode>
*
****************************************************************************************/
CmsRetId smsCMMSSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if (rc == CMS_SMS_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CMS_ERROR, rc, NULL);
    }
    return ret;
}


/****************************************************************************************
*  Process CMI cnf msg of AT+CMMS?
*
****************************************************************************************/
CmsRetId smsCMMSGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR AtRspBuf[64];
    CmiSmsGetMoreMessageModeCnf *getSmsMoreMsgModeCnf = (CmiSmsGetMoreMessageModeCnf*)paras;
    memset(AtRspBuf, 0, sizeof(AtRspBuf));

    if (rc == CMS_SMS_SUCC)
    {
        sprintf((char*)AtRspBuf, "+CMMS: %d", getSmsMoreMsgModeCnf->smsMoreMsgMode);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)AtRspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CMS_ERROR, rc, NULL);
    }
    return ret;
}


static BOOL SmsDeliverPduToText(CHAR *pRspStr, UINT16 rspBufSize, CmiSmsPdu *pPdu)
{
    UINT8   idx = 0;
    AtecMtDeliverSmsInfo *pDeliverInfo = PNULL;
    CHAR    tmpStr[ATEC_IND_RESP_256_STR_LEN] = {0};
    UINT32  rspLen = strlen(pRspStr);
    BOOL    hdrpresent = FALSE;

    OsaDebugBegin(pRspStr != PNULL && rspLen < rspBufSize && pPdu != PNULL, pRspStr, rspBufSize, pPdu);
    return FALSE;
    OsaDebugEnd();

    pDeliverInfo = (AtecMtDeliverSmsInfo *)OsaAllocZeroMemory(sizeof(AtecMtDeliverSmsInfo));

    OsaDebugBegin(pDeliverInfo != PNULL, sizeof(AtecMtDeliverSmsInfo), 0, 0);
    return FALSE;
    OsaDebugEnd();

    idx = 0;

    if (!(pPdu->pduData[idx] & 0x04))
    {
        pDeliverInfo->msgFlag |= 0x01;
    }

    /* TS23.040 9.2.3.4 */
    if (pPdu->pduData[idx]  & 0x20)
    {
        pDeliverInfo->msgFlag |= 0x02;
    }

    /* TS23.040 9.2.3.23 */
    if (pPdu->pduData[idx]  & 0x40)
    {
        pDeliverInfo->msgFlag |= 0x04;
        hdrpresent = TRUE;
    }

    /* TS23.040 9.2.3.17 */
    if (pPdu->pduData[idx]  & 0x80)
    {
        pDeliverInfo->msgFlag |= 0x08;
    }

    /* SRC address */
    idx++;
    smsPduDecodeAddress(pPdu->pduData, &idx, pDeliverInfo->srcAddr, PSIL_MSG_MAX_ADDR_LEN+1);

    OsaDebugBegin(idx < pPdu->len, idx, pPdu->len, 0);
        OsaFreeMemory(&pDeliverInfo);
        return FALSE;
    OsaDebugEnd();

    /* get protocol ID */
    smsPduDecodeProtocolId(pPdu->pduData, &idx, &pDeliverInfo->protocolId);

    OsaDebugBegin(idx < pPdu->len, idx, pPdu->len, pDeliverInfo->protocolId);
        OsaFreeMemory(&pDeliverInfo);
        return FALSE;
    OsaDebugEnd();

    /* get DCS */
    smsPduDecodeDcs(pPdu->pduData, &idx, &pDeliverInfo->dcsInfo);

    OsaDebugBegin(idx < pPdu->len, idx, pPdu->len, 2);
        OsaFreeMemory(&pDeliverInfo);
        return FALSE;
    OsaDebugEnd();

    /* get timestamp */
    smsPduDecodeTimeStamp(pPdu->pduData, &idx, &pDeliverInfo->timeStamp);

    OsaDebugBegin(idx < pPdu->len, idx, pPdu->len, 2);
        OsaFreeMemory(&pDeliverInfo);
        return FALSE;
    OsaDebugEnd();

    /* PRINT Time info */
    snprintf(tmpStr, ATEC_IND_RESP_256_STR_LEN, "\"%s\",,\"%02d/%02d/%02d,%02d:%02d:%02d %c%d\"\r\n",
             pDeliverInfo->srcAddr, pDeliverInfo->timeStamp.year,
             pDeliverInfo->timeStamp.month, pDeliverInfo->timeStamp.day,
             pDeliverInfo->timeStamp.hour, pDeliverInfo->timeStamp.minute, pDeliverInfo->timeStamp.second,
             pDeliverInfo->timeStamp.tzSign, pDeliverInfo->timeStamp.tz);

    strncat(pRspStr, tmpStr, rspBufSize - rspLen - 1);

    /*
     * decode PDU data
    */
    smsPduDecodeUserData(pPdu->pduData+idx,
                         pPdu->len - idx - 1,
                         (PsilMsgCodingType)pDeliverInfo->dcsInfo.alphabet,
                         hdrpresent,
                         pDeliverInfo->smsBuf,
                         (UINT8 *)&(pDeliverInfo->smsLength),
                         0xFF);

    if (pDeliverInfo->smsLength > 0)
    {
        rspLen = strlen(pRspStr);

        if (rspBufSize > rspLen + 1) // 1 for '\0'
        {
            strncat(pRspStr, (CHAR *)pDeliverInfo->smsBuf, rspBufSize - rspLen - 1);
        }
        else
        {
            OsaDebugBegin(FALSE, rspBufSize, rspLen, 0);
            OsaDebugEnd();
        }

        OsaFreeMemory(&pDeliverInfo);
        return TRUE;
    }

    OsaDebugBegin(FALSE, pDeliverInfo->smsLength, strlen(pRspStr), rspBufSize);
    OsaDebugEnd();

    OsaFreeMemory(&pDeliverInfo);
    return FALSE;
}


CmsRetId smsNewSMSInd(void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CmiSmsNewMsgInd *pCmiMsgInd = (CmiSmsNewMsgInd*)paras;

    UINT16 atRspBufMaxlen = pCmiMsgInd->pdu.len * 2 + 30 + PSIL_MSG_MAX_ADDR_LEN * 2 + CMS_NULL_CHAR_LEN;
    CHAR *pAtRspBuf = PNULL, *pPduBuf = PNULL;
    CHAR    smscBuf[PSIL_MSG_MAX_ADDR_LEN * 2 + CMS_NULL_CHAR_LEN];
    UINT16  smscLen = 0;
    PsilSmsFormatMode gSmsFormatMode = PSIL_SMS_FORMAT_PDU_MODE;

    /*
     * check the MSG input
    */
    OsaDebugBegin(pCmiMsgInd->pdu.len > 0 && pCmiMsgInd->pdu.len <= CMI_SMS_MAX_PDU_SIZE,
                  pCmiMsgInd->pdu.len, CMI_SMS_MAX_PDU_SIZE, 0);
    return CMS_FAIL;
    OsaDebugEnd();


    pAtRspBuf = (CHAR *)OsaAllocZeroMemory(atRspBufMaxlen);

    if (pAtRspBuf == PNULL)
    {
        OsaDebugBegin(FALSE, atRspBufMaxlen, pCmiMsgInd->pdu.len, 0);
        OsaDebugEnd();

        return CMS_FAIL;
    }

    pPduBuf = (CHAR *)OsaAllocZeroMemory(pCmiMsgInd->pdu.len * 2 + 10);

    if (pPduBuf == PNULL)
    {
        OsaDebugBegin(FALSE, pCmiMsgInd->pdu.len * 2 + 10, 0, 0);
        OsaDebugEnd();

        OsaFreeMemory(&pAtRspBuf);
        return CMS_FAIL;
    }

    gSmsFormatMode = (PsilSmsFormatMode)mwGetAtChanConfigItemValue(CMS_CHAN_DEFAULT, MID_WARE_AT_CHAN_SMS_MODE_CFG);

    /*
     * PDU format
    */
    if (gSmsFormatMode == PSIL_SMS_FORMAT_PDU_MODE)
    {
        memset(smscBuf, 0x00, sizeof(smscBuf));

        /* store the smsc address in smscBuf */
        smsSMSCToHexStrPdu(&(pCmiMsgInd->smscAddr), smscBuf, &smscLen);

        /* TPDU, hex -> hexstring */
        if (cmsHexToHexStr(pPduBuf, pCmiMsgInd->pdu.len * 2 + 10, pCmiMsgInd->pdu.pduData, pCmiMsgInd->pdu.len) >0)
        {
            switch (pCmiMsgInd->smsType)
            {
                case CMI_SMS_TYPE_STATUS_REPORT:
                case CMI_SMS_TYPE_DELIVER_REPORT:
                {
                    /* Status report */
                    snprintf(pAtRspBuf, atRspBufMaxlen, "+CDS: %d\r\n", pCmiMsgInd->pdu.len);

                    /* SMSC address */
                    strcat(pAtRspBuf, smscBuf);

                    /* TPDU */
                    strcat(pAtRspBuf, pPduBuf);

                    /* print to UART by default */
                    ret = atcURC(AT_CHAN_DEFAULT, pAtRspBuf);
                    break;
                }

                case CMI_SMS_TYPE_DELIVER:
                default:
                {
                    /* SCA add TPDU from CP msg composite sms*/
                    snprintf(pAtRspBuf, atRspBufMaxlen, "+CMT:,%d\r\n", pCmiMsgInd->pdu.len);

                    /* SMSC address */
                    strcat(pAtRspBuf, smscBuf);

                    /* TPDU */
                    strcat(pAtRspBuf, pPduBuf);

                    ret = atcURC(AT_CHAN_DEFAULT, pAtRspBuf);
                    break;
                }
            }
        }
    }
    else if (gSmsFormatMode == PSIL_SMS_FORMAT_TXT_MODE)    //TXT format
    {
        /* Only MT SMS need to reply */
        if ((pCmiMsgInd->pdu.pduData[0] & ATEC_MT_SMS_TYPE_MASK)  == ATEC_MT_DELIVER_SMS)
        {
            snprintf(pAtRspBuf, atRspBufMaxlen, "+CMT: ");

            if (SmsDeliverPduToText(pAtRspBuf, atRspBufMaxlen, &(pCmiMsgInd->pdu)))
            {
                ret = atcURC(AT_CHAN_DEFAULT, pAtRspBuf);
            }
            else
            {
                OsaDebugBegin(FALSE, pCmiMsgInd->pdu.pduData[0], pCmiMsgInd->pdu.pduData[1], pCmiMsgInd->pdu.pduData[2]);
                OsaDebugEnd();
            }
        }
        else
        {
            OsaDebugBegin(FALSE, pCmiMsgInd->pdu.pduData[0], ATEC_MT_DELIVER_SMS, 0);
            OsaDebugEnd();
        }
    }

    OsaFreeMemory(&pAtRspBuf);
    OsaFreeMemory(&pPduBuf);

    return ret;
}



/******************************************************************************
 ******************************************************************************
 * Staic func table
 ******************************************************************************
******************************************************************************/
#define __DEFINE_STATIC_VARIABLE__ //just for easy to find this position in SS


/****************************************************************************
*smsCmiCnfHdrList
* const table
****************************************************************************/
static const CmiCnfFuncMapList smsCmiCnfHdrList[] =
{
    {CMI_SMS_SEND_MSG_CNF, smsCMGSCnf},
    {CMI_SMS_GET_SMSC_ADDR_CNF, smsCSCAGetCnf},
    {CMI_SMS_SET_SMSC_ADDR_CNF, smsCSCASetCnf},
    {CMI_SMS_SELECT_SMS_SERVICE_CNF, smsCSMSSetCnf},
    {CMI_SMS_GET_CURRENT_SMS_SERVICE_CNF, smsCSMSCurrentGetCnf},
    {CMI_SMS_GET_SUPPORTED_SMS_SERVICE_CNF, smsCSMSSupportedGetCnf},
    {CMI_SMS_SET_MORE_MESSAGE_MODE_CNF, smsCMMSSetCnf},
    {CMI_SMS_GET_MORE_MESSAGE_MODE_CNF, smsCMMSGetCnf},
    {CMI_SMS_PRIM_BASE, PNULL}  /* this should be the last */
};

/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS


/******************************************************************************
 * atSmsProcCmiCnf
 * Description: Process CMI cnf msg for SMS sub-module
 * input: CacCmiCnf *pCmiCnf;  //ptr of cmi cnf
 * output: void
 * Comment:
******************************************************************************/
void atSmsProcCmiCnf(CacCmiCnf *pCmiCnf)
{
    UINT8 hdrIndex = 0;
    CmiCnfHandler cmiCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmiCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    while (smsCmiCnfHdrList[hdrIndex].primId != CMI_SMS_PRIM_BASE &&
           smsCmiCnfHdrList[hdrIndex].primId != CMI_SMS_PRIM_END)
    {
        if (smsCmiCnfHdrList[hdrIndex].primId == primId)
        {
            cmiCnfHdr = smsCmiCnfHdrList[hdrIndex].cmiCnfHdr;
            break;
        }
        hdrIndex++;
    }

    if (cmiCnfHdr != PNULL)
    {
        (*cmiCnfHdr)(pCmiCnf->header.srcHandler, pCmiCnf->header.rc, pCmiCnf->body);
    }

    return;
}


/******************************************************************************
 * atSmsProcCmiInd
 * Description: Process CMI Ind msg for SMS sub-module
 * input: CacCmiInd *pCmiInd;  //ptr of cmi Ind
 * output: void
 * Comment:
******************************************************************************/
void atSmsProcCmiInd(CacCmiInd *pCmiInd)
{
    UINT16 primId = 0;

    OsaDebugBegin(pCmiInd != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiInd->header.indId);

    switch(primId)
    {
        case CMI_SMS_NEW_MSG_IND:
            smsNewSMSInd(pCmiInd->body);
            break;

        default:
            break;
    }

}



