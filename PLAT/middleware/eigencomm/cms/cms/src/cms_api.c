/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: cms_api.c
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "cms_api.h"
#include "osasys.h"
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



#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position---Below for external use



/**
  \fn          CmsRetId applSendCmsCnf(UINT16 reqHandler, UINT16 rcCode, UINT8 appId, UINT8 primId, UINT16 primSize, void *pPrimBody)
  \brief
  \returns     CmsRetId
*/
CmsRetId applSendCmsCnf(UINT16 reqHandler, UINT16 rcCode, UINT8 appId, UINT8 primId, UINT16 primSize, void *pPrimBody)
{
    SignalBuf   *pSig = PNULL;
    CmsApplCnf  *pApplCnf = PNULL;

    OsaDebugBegin(reqHandler != 0 && appId < APPL_END, reqHandler, appId, 0);
    return CMS_INVALID_PARAM;
    OsaDebugEnd();

    OsaDebugBegin((primSize > 0 && pPrimBody != PNULL) || (primSize == 0 && pPrimBody == PNULL),
                  primSize, appId, primId);
    return CMS_INVALID_PARAM;
    OsaDebugEnd();

    OsaDebugBegin(primSize < 1024, primSize, appId, primId);
    OsaDebugEnd();

    OsaCreateSignal(SIG_CMS_APPL_CNF, sizeof(CmsApplCnf)+primSize, &pSig);

    pApplCnf = (CmsApplCnf *)(pSig->sigBody);

    pApplCnf->header.appId  = appId;
    pApplCnf->header.primId = primId;
    pApplCnf->header.srcHandler = reqHandler;
    pApplCnf->header.rc     = rcCode;
    pApplCnf->header.rsvd   = 0;

    if (primSize > 0)
    {
        memcpy(pApplCnf->body, pPrimBody, primSize);
    }

    OsaSendSignal(CMS_TASK_ID, &pSig);

    return CMS_RET_SUCC;
}


/**
  \fn           CmsRetId applSendCmsInd(UINT16 reqHandler, UINT8 appId, UINT8 primId, UINT16 primSize, void *pPrimBody)
  \brief
  \returns      CmsRetId
*/
CmsRetId applSendCmsInd(UINT16 reqHandler, UINT8 appId, UINT8 primId, UINT16 primSize, void *pPrimBody)
{
    SignalBuf   *pSig = PNULL;
    CmsApplInd  *pApplInd = PNULL;

    OsaDebugBegin((primSize > 0 && pPrimBody != PNULL) || (primSize == 0 && pPrimBody == PNULL),
                  primSize, appId, primId);
    return CMS_INVALID_PARAM;
    OsaDebugEnd();

    OsaDebugBegin(primSize < 1024, primSize, appId, primId);
    OsaDebugEnd();

    OsaCreateSignal(SIG_CMS_APPL_IND, sizeof(CmsApplInd)+primSize, &pSig);

    pApplInd = (CmsApplInd *)(pSig->sigBody);

    pApplInd->header.appId  = appId;
    pApplInd->header.primId = primId;
    pApplInd->header.reqHandler = reqHandler;

    if (primSize > 0)
    {
        memcpy(pApplInd->body, pPrimBody, primSize);
    }

    OsaSendSignal(CMS_TASK_ID, &pSig);

    return CMS_RET_SUCC;

}


