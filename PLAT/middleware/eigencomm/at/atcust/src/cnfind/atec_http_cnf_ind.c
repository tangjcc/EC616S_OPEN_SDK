/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_http_cnf_ind.c
*
*  Description: Process http(s) client related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "at_util.h"
#include "atc_reply.h"
#include "at_http_task.h"
#include "atec_http_cnf_ind.h"


//maybe used:if need to send response by signal and trigger proxy task
CmsRetId httpCreateCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32] = {0};
    httpCnfCmdMsg *cnfMsg = (httpCnfCmdMsg *)paras;

    if(rc == APPL_RET_SUCC)
    {
        sprintf(RspBuf, "+HTTPCREATE: %d", cnfMsg->clinetId);
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_HTTP_ERROR, ( HTTP_CREATE_FAILED), NULL);
    }
    return ret;
}

CmsRetId httpConnCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_HTTP_ERROR, ( HTTP_CONNECT_FAILED), NULL);
    }
    return ret;
}

CmsRetId httpSendCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_HTTP_ERROR, ( HTTP_SEND_FAILED), NULL);
    }
    return ret;
}

CmsRetId httpDestoryCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_HTTP_ERROR, ( HTTP_DESTORY_FAILED), NULL);
    }
    return ret;
}

CmsRetId httpInd(UINT16 reqhandler, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    httpIndMsg *pIndMsg = (httpIndMsg *)paras;
    UINT32 chanId = AT_GET_HANDLER_CHAN_ID(reqhandler);
    
    ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpInd_1, P_INFO, 0, "recv indication");

    if(paras == NULL)
    {
       ECOMM_TRACE(UNILOG_ATCMD_HTTP, httpInd_2, P_ERROR, 0, "invalid paras");
       return ret;
    }

    ret = atcURC(chanId, pIndMsg->pHttpInd);

    free(pIndMsg->pHttpInd);
    return ret;
}


/******************************************************************************
 * httpCmsCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplCnfFuncMapList httpCmsCnfHdrList[] =
{
    {APPL_HTTP_CREATE_CNF,              httpCreateCnf},
    {APPL_HTTP_CONNECT_CNF,             httpConnCnf},
    {APPL_HTTP_SEND_CNF,                httpSendCnf},
    {APPL_HTTP_DESTORY_CNF,             httpDestoryCnf},

    {APPL_HTTP_PRIM_ID_END, PNULL}  /* this should be the last */
};

/******************************************************************************
 * httpCmsIndHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplIndFuncMapList httpCmsIndHdrList[] =
{
    {APPL_HTTP_IND, httpInd},
    {APPL_HTTP_PRIM_ID_END, PNULL}  /* this should be the last */
};


#define _DEFINE_AT_CNF_IND_INTERFACE_

void atApplHttpProcCmsCnf(CmsApplCnf *pCmsCnf)
{
    UINT8 hdrIndex = 0;
    ApplCnfHandler applCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsCnf->header.primId);

    while (httpCmsCnfHdrList[hdrIndex].primId != 0)
    {
        if (httpCmsCnfHdrList[hdrIndex].primId == primId)
        {
            applCnfHdr = httpCmsCnfHdrList[hdrIndex].applCnfHdr;
            break;
        }
        hdrIndex++;
    }

    if (applCnfHdr != PNULL)
    {
        (*applCnfHdr)(pCmsCnf->header.srcHandler, pCmsCnf->header.rc, pCmsCnf->body);
    }
    else
    {
        OsaDebugBegin(applCnfHdr != NULL, 0, 0, 0);
        OsaDebugEnd();
    }

    return;
}

void atApplHttpProcCmsInd(CmsApplInd *pCmsInd)
{
    UINT8 hdrIndex = 0;
    ApplIndHandler applIndHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsInd != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsInd->header.primId);

    while (httpCmsIndHdrList[hdrIndex].primId != 0)
    {
        if (httpCmsIndHdrList[hdrIndex].primId == primId)
        {
            applIndHdr = httpCmsIndHdrList[hdrIndex].applIndHdr;
            break;
        }
        hdrIndex++;
    }

    if (applIndHdr != PNULL)
    {
        (*applIndHdr)(pCmsInd->header.reqHandler, pCmsInd->body);
    }
    else
    {
        OsaDebugBegin(applIndHdr != NULL, 0, 0, 0);
        OsaDebugEnd();
    }

    return;
}





