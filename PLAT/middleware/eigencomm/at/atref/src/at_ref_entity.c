/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: at_ref_entity.c
*
*  Description: Process protocol stack related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifdef  FEATURE_REF_AT_ENABLE
#include "at_ref_entity.h"
#include "atec_ref_ps_cnf_ind.h"
#include "atec_ref_tcpip_cnf_ind.h"


#define _DEFINE_AT_REQ_FUNCTION_LIST_

/*
 * process: SIG_CAC_CMI_CNF
*/
CmsRetId atRefProcCmiCnf(CacCmiCnf *pCmiCnf)
{
    UINT16  sgId = 0;
    UINT16  primId = 0;
    UINT8   subAtId = AT_GET_HANDLER_SUB_ATID(pCmiCnf->header.srcHandler);

    sgId    = AT_GET_CMI_SG_ID(pCmiCnf->header.cnfId);
    primId  = AT_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    ECOMM_TRACE(UNILOG_ATCMD, atRefProcCmiCnf_1, P_INFO, 1, "ATCMD, AT REF SUB ID: %d", subAtId);

    switch(sgId)
    {
        case CAC_DEV:
            atRefDevProcCmiCnf(pCmiCnf);
            break;

        case CAC_MM:
            atRefMmProcCmiCnf(pCmiCnf);
            break;

        case CAC_PS:
            atRefPsProcCmiCnf(pCmiCnf);
            break;

        case CAC_SIM:
            atRefSimProcCmiCnf(pCmiCnf);
            break;

        case CAC_SMS:
            atRefSmsProcCmiCnf(pCmiCnf);
            break;

        default:
            OsaDebugBegin(FALSE, sgId, primId, 0);
            OsaDebugEnd();
            break;
    }

    return CMS_RET_SUCC;
}

/**
  \fn
  \brief        AT process AT related signal
  \returns      void
  \Note
*/
CmsRetId atRefProcCmiInd(CacCmiInd *pCmiInd)
{
    UINT16    sgId = 0;   //service group ID
    UINT16    primId = 0;

    sgId    = AT_GET_CMI_SG_ID(pCmiInd->header.indId);
    primId  = AT_GET_CMI_PRIM_ID(pCmiInd->header.indId);

    switch(sgId)
    {
        case CAC_DEV:
            atRefDevProcCmiInd(pCmiInd);
            break;

        case CAC_MM:
            atRefMmProcCmiInd(pCmiInd);
            break;

        case CAC_PS:
            atRefPsProcCmiInd(pCmiInd);
            break;

        case CAC_SIM:
            atRefSimProcCmiInd(pCmiInd);
            break;

        case CAC_SMS:
            atRefSmsProcCmiInd(pCmiInd);
            break;

        default:
            OsaDebugBegin(FALSE, sgId, primId, 0);
            OsaDebugEnd();
            break;
    }

    return CMS_RET_SUCC;
}


/*
 * process: SIG_CMS_APPL_CNF
*/
CmsRetId atRefProcApplCnf(CmsApplCnf *pApplCnf)
{
    UINT8   subAtId = AT_GET_HANDLER_SUB_ATID(pApplCnf->header.srcHandler);

    /*
     * AT channel ID = 0, is reserved for internal using, not for AT CMD channel
    */
    if (AT_GET_HANDLER_CHAN_ID(pApplCnf->header.srcHandler) == CMS_CHAN_RSVD)
    {
        return CMS_FAIL;
    }

    ECOMM_TRACE(UNILOG_ATCMD, atRefProcApplCnf_1, P_INFO, 3, "ATCMD, AT chanId: %d, APP CNF appId: %d, primId: %d",
                AT_GET_HANDLER_CHAN_ID(pApplCnf->header.srcHandler), pApplCnf->header.appId, pApplCnf->header.primId);

    if (pApplCnf == PNULL)
    {
        OsaDebugBegin(FALSE, pApplCnf->header.appId, pApplCnf->header.primId, pApplCnf->header.rc);
        OsaDebugEnd();

        return CMS_FAIL;
    }

    if (subAtId != CMS_REF_SUB_AT_1_ID && subAtId != CMS_REF_SUB_AT_2_ID && subAtId != CMS_REF_SUB_AT_3_ID)
    {
        ECOMM_TRACE(UNILOG_ATCMD, atRefProcApplCnf_2, P_ERROR, 1, "ATCMD, AT chanId: %d",
            AT_GET_HANDLER_CHAN_ID(pApplCnf->header.srcHandler));    
        return CMS_FAIL;
    }

    switch(pApplCnf->header.appId)
    {
        case APPL_NM:
            atTcpIpProcRefNmApplCnf(pApplCnf);
            break;
        default:
            break;
    }

    return CMS_RET_SUCC;
}


/*
 * process: SIG_CMS_APPL_IND
*/
CmsRetId atRefProcApplInd(CmsApplInd *pApplInd)
{
    UINT8   subAtId = AT_GET_HANDLER_SUB_ATID(pApplInd->header.reqHandler);

    ECOMM_TRACE(UNILOG_ATCMD, atRefProcApplInd_1, P_INFO, 2, "ATCMD, APP IND appId: %d, primId: %d",
                pApplInd->header.appId, pApplInd->header.primId);

    if (pApplInd == PNULL)
    {
        OsaDebugBegin(FALSE, pApplInd->header.appId, pApplInd->header.primId, 0);
        OsaDebugEnd();

        return CMS_FAIL;
    }


    if (pApplInd->header.reqHandler != BROADCAST_IND_HANDLER)
    {
        if (subAtId != CMS_REF_SUB_AT_1_ID && subAtId != CMS_REF_SUB_AT_2_ID && subAtId != CMS_REF_SUB_AT_3_ID)
        {
            ECOMM_TRACE(UNILOG_ATCMD, atRefProcApplInd_2, P_ERROR, 1, "ATCMD, AT chanId: %d",
                AT_GET_HANDLER_CHAN_ID(pApplInd->header.reqHandler));        
            return CMS_FAIL;
        }

    }

    switch(pApplInd->header.appId)
    {
        case APPL_NM:
            atTcpIpProcRefNmApplInd(pApplInd);
            break;
        default:
            break;
    }

    return CMS_RET_SUCC;
}



#endif

