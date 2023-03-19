/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_fwupd_cnf_ind.c
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/

#include "ec_fwupd_api.h"
#include "at_fwupd_task.h"
#include "atec_fwupd_cnf_ind.h"

CmsRetId ecNFWUPDCnf(UINT16 atHandle, UINT16 rc, void *paras);


/******************************************************************************
 * g_fwupdCmsCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplCnfFuncMapList g_fwupdCmsCnfHdrList[] =
{
    {APPL_FWUPD_CNF, ecNFWUPDCnf},

    {APPL_FWUPD_PRIM_ID_END, PNULL}   /* this should be the last */
};


/****************************************************************************************
*  FUNCTION:  ecNFWUPDCnf
*
*  PARAMETERS:
*
*  DESCRIPTION:
*
*  RETURNS: CmsRetId
*
****************************************************************************************/
CmsRetId ecNFWUPDCnf(UINT16 atHandle, UINT16 rc, void *paras)
{
    FwupdCnfMsg_t  *cnfMsgPtr = (FwupdCnfMsg_t*)paras;

    if(rc == APPL_RET_FAIL)
    {
        atcReply(atHandle, AT_RC_FWUPD_ERROR, cnfMsgPtr->errCode, NULL);
        return CMS_FAIL;
    }

    atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, (const CHAR*)cnfMsgPtr->respStr);

    return CMS_RET_SUCC;
}

void atApplFwupdProcCmsCnf(CmsApplCnf *pCmsCnf)
{
    UINT8 hdrIndex = 0;
    ApplCnfHandler applCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsCnf->header.primId);

    while (g_fwupdCmsCnfHdrList[hdrIndex].primId != 0)
    {
        if (g_fwupdCmsCnfHdrList[hdrIndex].primId == primId)
        {
            applCnfHdr = g_fwupdCmsCnfHdrList[hdrIndex].applCnfHdr;
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


