/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename:
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "ctype.h"
#include "cmicomm.h"
#include "PhyAtCmdDebug.h"
#include "atc_reply.h"


extern void mwSetPhyAtCmdValue(PhyDebugAtCmdInfo *pPhyDebugAtCmdInfo);


/******************************************************************************
 * phySetPhyInfoCnf
 * Description:
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId phySetPhyInfoCnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    PhyDebugAtCmdInfo *pPhyCfgInfo = (PhyDebugAtCmdInfo *)paras;
    CHAR RspBuf[88] = {0};

    if(rc == CME_SUCC)
    {
        mwSetPhyAtCmdValue(pPhyCfgInfo);

        ret = atcReply(reqHandle, AT_RC_OK, 0, RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}


/******************************************************************************
 * atPsCGDCONTSetCnf
 * Description:
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId phyGetPhyAllInfoCnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    PhyDebugAtCmdAllInfo *pPowerInfo = (PhyDebugAtCmdAllInfo *)paras;
    CHAR RspBuf[ATEC_IND_RESP_256_STR_LEN] = {0};

/*
    pPhyAllInfo.flag[0] = gpPhyDebugAtCmd->atCmdPowerInfo.powerTestFlag;
    pPhyAllInfo.flag[1] = gpPhyDebugAtCmd->atCmdPowerInfo.targetPower;
    pPhyAllInfo.flag[2] = gpPhyDebugAtCmd->atCmdOOSInfo.oosTestFlag;
    pPhyAllInfo.flag[3] = gpPhyDebugAtCmd->atCmdEscapeInfo.onOffFlag;
    pPhyAllInfo.flag[4] = gpPhyDebugAtCmd->atCmdDumpInfo.onOffFlag;
    pPhyAllInfo.flag[5] = gpPhyDebugAtCmd->atCmdDumpInfo.dumpTask;
    pPhyAllInfo.flag[6] = gpPhyDebugAtCmd->atCmdTxDpdInfo.onOffFlag;
    pPhyAllInfo.flag[7] = gpPhyDebugAtCmd->atCmdGraphInfo.onOffFlag;
    pPhyAllInfo.flag[8] = gpPhyDebugAtCmd->atCmdAlgoParam.algoType;
    pPhyAllInfo.flag[9] = gpPhyDebugAtCmd->atCmdAlgoParam.param[0];
    pPhyAllInfo.flag[10] = gpPhyDebugAtCmd->atCmdPhyLogCtrl.logLevel;
*/
    if(rc == CME_SUCC)
    {
        snprintf((char*)RspBuf, ATEC_IND_RESP_256_STR_LEN,
                 "\"Power\": %d, %d(power), \"Oos\": %d, \"Escape\": %d, \"Dump\": %d, %d, \"TxDpd(1 for Disable)\": %d, \"graph\": %d, \"algoParam\": %d, %d, \"logLevel\": %d",
                 pPowerInfo->flag[0], pPowerInfo->flag[1], pPowerInfo->flag[2], pPowerInfo->flag[3], pPowerInfo->flag[4], pPowerInfo->flag[5], pPowerInfo->flag[6],pPowerInfo->flag[7], pPowerInfo->flag[8], pPowerInfo->flag[9], pPowerInfo->flag[10]);

        ret = atcReply(reqHandle, AT_RC_OK, 0, RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/**
  \fn          CmsRetId phyECPHYCFGcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras)
  \brief       at cmd cnf function
  \param[in]   primId      hh
  \param[in]   reqHandle   hh
  \param[in]   rc          hh
  \param[in]   *paras      hh
  \returns     CmsRetId
*/
CmsRetId phyECPHYCFGcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    switch(primId)
    {
        case SIG_CEPHY_DEBUG_AT_CMD_SET_CNF:
            ret = phySetPhyInfoCnf(primId, reqHandle, rc, paras);
            break;

        case SIG_CEPHY_DEBUG_AT_CMD_GET_ALL_CNF:
            ret = phyGetPhyAllInfoCnf(primId, reqHandle, rc, paras);
            break;

        default:
            break;
    }

    return ret;
}


