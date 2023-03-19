/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_lwm2m_cnf_ind_api.c
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "atec_lwm2m_cnf_ind.h"
#include "at_lwm2m_task.h"
#include "at_util.h"


#define _DEFINE_AT_CNF_IND_INTERFACE_

extern uint8_t lwm2mSlpHandler;

static CmsRetId lwm2mCreateCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[16];
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mCreateCnf_0, P_INFO, 1, "rc : %d", rc);

    lwm2m_cnf_msg     *pConf = (lwm2m_cnf_msg *)paras;
    
    if(rc == APPL_RET_SUCC)
    {
        sprintf(RspBuf, "+LWM2MCREATE: %d",pConf->id);
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)RspBuf);
    }
    else
    {
        //lwm2mRemoveClient(pConf->id);
        ret = atcReply(reqHandle, AT_RC_LWM2M_ERROR, pConf->ret, NULL);
    }
    slpManPlatVoteEnableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
    return ret;
}

static CmsRetId lwm2mAddObjCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mAddObjCnf_0, P_INFO, 1, "rc : %d", rc);

    lwm2m_cnf_msg     *pConf = (lwm2m_cnf_msg *)paras;
    
    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        ECOMM_TRACE(UNILOG_LWM2M, lwm2mAddObjCnf_1, P_INFO, 1, "pConf->ret : %d", pConf->ret);
        ret = atcReply(reqHandle, AT_RC_LWM2M_ERROR, pConf->ret, NULL);
    }
    slpManPlatVoteEnableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
    return ret;
}

static CmsRetId lwm2mDelObjCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mDelObjCnf_0, P_INFO, 1, "rc : %d", rc);

    lwm2m_cnf_msg     *pConf = (lwm2m_cnf_msg *)paras;
    
    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_LWM2M_ERROR, pConf->ret, NULL);
    }
    slpManPlatVoteEnableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
    return ret;
}

static CmsRetId lwm2mNotifyCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mNotfyCnf_0, P_INFO, 1, "rc : %d", rc);

    lwm2m_cnf_msg     *pConf = (lwm2m_cnf_msg *)paras;
    
    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_LWM2M_ERROR, pConf->ret, NULL);
    }
    //slpManPlatVoteEnableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
    return ret;
}

static CmsRetId lwm2mUpdateCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mUpdateCnf_0, P_INFO, 1, "rc : %d", rc);

    lwm2m_cnf_msg     *pConf = (lwm2m_cnf_msg *)paras;
    
    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_LWM2M_ERROR, pConf->ret, NULL);
    }
    slpManPlatVoteEnableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
    return ret;
}

static CmsRetId lwm2mDelCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mDelCnf_0, P_INFO, 1, "rc : %d", rc);
    
    lwm2m_cnf_msg     *pConf = (lwm2m_cnf_msg *)paras;

    if(rc == APPL_RET_SUCC)
    {
        //lwm2mRemoveClient(pConf->id);
        //lwm2mClearFile();
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_LWM2M_ERROR, pConf->ret, NULL);
    }
    slpManPlatVoteEnableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
    return ret;
}

static CmsRetId lwm2mInd(UINT16 reqhandler, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_LWM2M, lwm2mRECVInd_1, P_INFO, 0, "recv indication");

    if(paras == NULL)
    {
       ECOMM_TRACE(UNILOG_LWM2M, lwm2mRECVInd_2, P_ERROR, 0, "invalid paras");
       return ret;
    }
    UINT32 chanId = AT_GET_HANDLER_CHAN_ID(reqhandler);

    ret = atcURC(chanId, (CHAR*)paras);
    return ret;
}


/******************************************************************************
 * lwm2mCmsCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplCnfFuncMapList lwm2mCmsCnfHdrList[] =
{
    {APPL_LWM2M_CREATE_CNF,                lwm2mCreateCnf},
    {APPL_LWM2M_ADDOBJ_CNF,                lwm2mAddObjCnf},
    {APPL_LWM2M_DELOBJ_CNF,                lwm2mDelObjCnf},
    {APPL_LWM2M_NOTIFY_CNF,                lwm2mNotifyCnf},
    {APPL_LWM2M_UPDATE_CNF,                lwm2mUpdateCnf},
    {APPL_LWM2M_DEL_CNF,                   lwm2mDelCnf},

    {APPL_LWM2M_PRIM_ID_END, PNULL} /* this should be the last */
};

/******************************************************************************
 * lwm2mCmsIndHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplIndFuncMapList lwm2mCmsIndHdrList[] =
{
    {APPL_LWM2M_IND, lwm2mInd},
    {APPL_LWM2M_PRIM_ID_END, PNULL} /* this should be the last */
};

void atApplLwm2mProcCmsCnf(CmsApplCnf *pCmsCnf)
{
    UINT8 hdrIndex = 0;
    ApplCnfHandler applCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsCnf->header.primId);

    while (lwm2mCmsCnfHdrList[hdrIndex].primId != 0)
    {
        if (lwm2mCmsCnfHdrList[hdrIndex].primId == primId)
        {
            applCnfHdr = lwm2mCmsCnfHdrList[hdrIndex].applCnfHdr;
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

void atApplLwm2mProcCmsInd(CmsApplInd *pCmsInd)
{
    UINT8 hdrIndex = 0;
    ApplIndHandler applIndHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsInd != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsInd->header.primId);

    while (lwm2mCmsIndHdrList[hdrIndex].primId != 0)
    {
        if (lwm2mCmsIndHdrList[hdrIndex].primId == primId)
        {
            applIndHdr = lwm2mCmsIndHdrList[hdrIndex].applIndHdr;
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



