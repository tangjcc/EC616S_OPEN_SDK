/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_onenet_cnf_ind.c
*
*  Description: Process onenet related AT commands RESP/IND
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "at_util.h"
#include "cms_api.h"
#include "at_onenet_task.h"

CmsRetId onenetInd(UINT16 reqhandler, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_ONENET, onenetInd_1, P_INFO, 0, "recv indication");

    if(paras == NULL)
    {
       ECOMM_TRACE(UNILOG_ONENET, onenetInd_2, P_ERROR, 0, "invalid paras");
       return ret;
    }

    UINT32 chanId = AT_GET_HANDLER_CHAN_ID(reqhandler);

    ret = atcURC(chanId, (CHAR*)paras);
    return ret;
}

#define __DEFINE_STATIC_VARIABLES__
static CmsRetId onenetNotifyCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_ONENET, onenetNotifyCnf_0, P_INFO, 1, "rc : %d", rc);
    onenet_cnf_msg     *pConf = (onenet_cnf_msg *)paras;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        onenetSleepVote(SYSTEM_FREE_ONE);
        ret = atcReply(reqHandle, AT_RC_CIS_ERROR, pConf->ret, NULL);
    }
    return ret;
}

static CmsRetId onenetOpenCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_ONENET, onenetOpenCnf_0, P_INFO, 1, "rc : %d", rc);
    onenet_cnf_msg     *pConf = (onenet_cnf_msg *)paras;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CIS_ERROR, pConf->ret, NULL);
    }
    return ret;
}

static CmsRetId onenetCloseCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_ONENET, onenetCloseCnf_0, P_INFO, 1, "rc : %d", rc);
    onenet_cnf_msg     *pConf = (onenet_cnf_msg *)paras;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        onenetSleepVote(SYSTEM_FREE);
        ret = atcReply(reqHandle, AT_RC_CIS_ERROR, pConf->ret, NULL);
    }
    return ret;
}

static CmsRetId onenetUpdateCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    ECOMM_TRACE(UNILOG_ONENET, onenetUpdateCnf_0, P_INFO, 1, "rc : %d", rc);
    onenet_cnf_msg     *pConf = (onenet_cnf_msg *)paras;

    if(rc == APPL_RET_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
    }
    else
    {
        onenetSleepVote(SYSTEM_FREE_ONE);
        ret = atcReply(reqHandle, AT_RC_CIS_ERROR, pConf->ret, NULL);
    }
    return ret;
}


/******************************************************************************
 * onenetCmsCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplCnfFuncMapList onenetCmsCnfHdrList[] =
{
    {APPL_ONENET_OPEN_CNF,           onenetOpenCnf},
    {APPL_ONENET_NOTIFY_CNF,           onenetNotifyCnf},
    {APPL_ONENET_CLOSE_CNF,            onenetCloseCnf},
    {APPL_ONENET_UPDATE_CNF,           onenetUpdateCnf},
    
    {APPL_ONENET_PRIM_ID_END, PNULL},   /* this should be the last */
};

/******************************************************************************
 * onenetCmsIndHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplIndFuncMapList onenetCmsIndHdrList[] =
{
    {APPL_ONENET_IND, onenetInd},

    {APPL_ONENET_PRIM_ID_END, PNULL},   /* this should be the last */
};



#define __DEFINE_GLOBAL_FUNCTION__

void atApplOnenetProcCmsCnf(CmsApplCnf *pCmsCnf)
{
    UINT8 hdrIndex = 0;
    ApplCnfHandler applCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsCnf->header.primId);

    while (onenetCmsCnfHdrList[hdrIndex].primId != 0)
    {
        if (onenetCmsCnfHdrList[hdrIndex].primId == primId)
        {
            applCnfHdr = onenetCmsCnfHdrList[hdrIndex].applCnfHdr;
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

void atApplOnenetProcCmsInd(CmsApplInd *pCmsInd)
{
    UINT8 hdrIndex = 0;
    ApplIndHandler applIndHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsInd != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsInd->header.primId);

    while (onenetCmsIndHdrList[hdrIndex].primId != 0)
    {
        if (onenetCmsIndHdrList[hdrIndex].primId == primId)
        {
            applIndHdr = onenetCmsIndHdrList[hdrIndex].applIndHdr;
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


