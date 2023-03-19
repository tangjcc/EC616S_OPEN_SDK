/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_example_cnf_ind.c
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "atec_example_cnf_ind.h"
#include "atec_cust_cmd_table.h"
#include "at_example_task.h"

/******************************************************************************
 * exampleCmsCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplCnfFuncMapList exampleCmsCnfHdrList[] =
{
    {APPL_EXAMPLE_ECTESTB_CNF,      ecTestbCnf},
    {APPL_EXAMPLE_ECTESTC_CNF,      ecTestcCnf},

    {APPL_EXAMPLE_PRIM_ID_END, PNULL}   /* this should be the last */
};

/******************************************************************************
 * exampleCmsIndHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplIndFuncMapList exampleCmsIndHdrList[] =
{
    {APPL_EXAMPLE_ECTESTC_IND,      ecTestcInd},
    {APPL_EXAMPLE_PRIM_ID_END, PNULL}   /* this should be the last */
};

#define _DEFINE_AT_CNF_FUNCTION_LIST_
/****************************************************************************************
*  FUNCTION:  ccCCTestbCnfApi
*
*  PARAMETERS:
*
*  DESCRIPTION: interface to set phone functionality status, implementation of AT+CFUN=
*
*  RETURNS: CmsRetId
*
****************************************************************************************/
CmsRetId ecTestbCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+TESTB: CNF");

    return CMS_RET_SUCC;
}

/****************************************************************************************
*  FUNCTION:  ccCCTestcCnfApi
*
*  PARAMETERS:
*
*  DESCRIPTION: interface to set phone functionality status, implementation of AT+CFUN=
*
*  RETURNS: CmsRetId
*
****************************************************************************************/
CmsRetId ecTestcCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+TESTC: CNF");
    return CMS_RET_SUCC;
}


#define _DEFINE_AT_IND_FUNCTION_LIST_
/****************************************************************************************
*  FUNCTION:  ecTestcIndApi
*
*  PARAMETERS:
*
*  DESCRIPTION: interface to set phone functionality status, implementation of AT+CFUN=
*
*  RETURNS: CmsRetId
*
****************************************************************************************/
CmsRetId ecTestcInd(UINT16 indHandle, void *paras)
{
    atcURC(AT_CHAN_DEFAULT, (CHAR *)"+TESTC: IND");
    return CMS_RET_SUCC;
}

#define _DEFINE_AT_CNF_IND_INTERFACE_

void atApplExampleProcCmsCnf(CmsApplCnf *pCmsCnf)
{
    UINT8 hdrIndex = 0;
    ApplCnfHandler applCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsCnf->header.primId);

    while (exampleCmsCnfHdrList[hdrIndex].primId != 0)
    {
        if (exampleCmsCnfHdrList[hdrIndex].primId == primId)
        {
            applCnfHdr = exampleCmsCnfHdrList[hdrIndex].applCnfHdr;
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

void atApplExampleProcCmsInd(CmsApplInd *pCmsInd)
{
    UINT8 hdrIndex = 0;
    ApplIndHandler applIndHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsInd != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsInd->header.primId);

    while (exampleCmsIndHdrList[hdrIndex].primId != 0)
    {
        if (exampleCmsIndHdrList[hdrIndex].primId == primId)
        {
            applIndHdr = exampleCmsIndHdrList[hdrIndex].applIndHdr;
            break;
        }
        hdrIndex++;
    }

    if (applIndHdr != PNULL)
    {
        (*applIndHdr)(AT_CHAN_DEFAULT, pCmsInd->body);
    }
    else
    {
        OsaDebugBegin(applIndHdr != NULL, 0, 0, 0);     
        OsaDebugEnd();
    }

    return;
}









