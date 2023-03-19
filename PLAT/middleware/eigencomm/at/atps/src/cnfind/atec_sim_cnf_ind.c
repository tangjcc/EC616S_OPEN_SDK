/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_sim_cnf_ind.c
*
*  Description: Process CMI CNF and reply or report URC for SIM
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdio.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "cmisim.h"
#include "atec_sim.h"
#include "ps_sim_if.h"

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



/******************************************************************************
 * simSwitchStateSetCnf
 * Description: Process CMI cnf of AT+esimswitch
 * input: UINT16 primId;      //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simSwitchStateSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR     RspBuf[16];
    memset(RspBuf, 0, sizeof(RspBuf));
    ret = atcReply(reqHandle, (rc == CME_SUCC ? AT_RC_OK : AT_RC_CME_ERROR), 0, (char*)RspBuf);

    return ret;

}


/******************************************************************************
 * simSwitchStateGetCnf
 * Description: Process CMI cnf of AT+esimswitch
 * input: UINT16 primId;      //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simSwitchStateGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR     RspBuf[16];

    CmiSimGetSimSwitchStateCnf *Conf = (CmiSimGetSimSwitchStateCnf *)paras;

    memset(RspBuf, 0, sizeof(RspBuf));

    if(rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+ESIMSWITCH:%d", Conf->mode);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);

    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;

}



/******************************************************************************
 * atSimCIMIGetConf
 * Description: Process CMI cnf msg of AT+CIMI
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simCIMIGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32];
    CmiSimGetSubscriberIdCnf *Conf = (CmiSimGetSubscriberIdCnf *)paras;

    memset(RspBuf, 0, sizeof(RspBuf));

    if(rc == CME_SUCC)
    {
        memcpy(RspBuf, Conf->imsiStr.contents, Conf->imsiStr.length);
        RspBuf[Conf->imsiStr.length] = '\0';
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atSimNCCIDGetConf
 * Description: Process CMI cnf msg of AT+ECICCID
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simECICCIDGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32], tempBuf[22];
    CmiSimGetIccIdCnf *Conf = (CmiSimGetIccIdCnf *)paras;

    memset(RspBuf, 0, sizeof(RspBuf));
    memset(RspBuf, 0, sizeof(tempBuf));

    if(rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+ECICCID: ");
        memcpy(tempBuf, Conf->iccidStr.data, CMI_SIM_ICCID_LEN);
        tempBuf[CMI_SIM_ICCID_LEN] = '\0';
        strcat((char *)RspBuf,(char *)tempBuf);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atSimNCCIDGetConf
 * Description: Process CMI cnf msg of AT+ECICCID
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simCPINGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32], tempBuf[22];
    CmiSimGetPinStateCnf *Conf = (CmiSimGetPinStateCnf *)paras;

    memset(RspBuf, 0, sizeof(RspBuf));
    memset(tempBuf, 0, sizeof(tempBuf));

    if(rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+CPIN: ");

        switch(Conf->pinState)
        {
            case CMI_SIM_PIN_STATE_READY:
                sprintf((char*)tempBuf, "READY");
                break;

            case CMI_SIM_PIN_STATE_PIN1_REQUIRED:
            case CMI_SIM_PIN_STATE_UNIVERSALPIN_REQUIRED:
                sprintf((char*)tempBuf, "SIM PIN");
                break;

            case CMI_SIM_PIN_STATE_UNBLOCK_PIN1_REQUIRED:
            case CMI_SIM_PIN_STATE_UNBLOCK_UNIVERSALPIN_REQUIRED:
                sprintf((char*)tempBuf, "SIM PUK");
                break;

            case CMI_SIM_PIN_STATE_PIN2_REQUIRED:
                sprintf((char*)tempBuf, "SIM PIN2");
                break;

            case CMI_SIM_PIN_STATE_UNBLOCK_PIN2_REQUIRED:
                sprintf((char*)tempBuf, "SIM PUK2");
                break;

            default:
                sprintf((char*)tempBuf, "ERROR");
                break;
        }
        strcat((char *)RspBuf,(char *)tempBuf);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simCPINSetCnf
 * Description: Process CMI cnf msg of AT+CPIN
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simCPINSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    //CmiSimOperatePinCnf *Conf = (CmiSimOperatePinCnf *)paras;

    if(rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simCLCKGetCnf
 * Description: Process CMI cnf msg of AT+CLCK
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simCLCKSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32];
    CmiSimFacilityLockCnf *Conf = (CmiSimFacilityLockCnf *)paras;

    memset(RspBuf, 0, sizeof(RspBuf));

    if(rc == CME_SUCC)
    {
        if(Conf->mode == CMI_SIM_FAC_LOCK_MODE_QUERY)
        {
            sprintf((char*)RspBuf, "+CLCK: %d", Conf->facStatus);
            ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
        }
        else
        {
            ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simCSIMSetCnf
 * Description: Process CMI cnf msg of AT+CSIM
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simCSIMSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR *RspBuf = PNULL;
    CHAR *strBuf = PNULL;
    CmiSimGenericAccessCnf *Conf = (CmiSimGenericAccessCnf *)paras;

    if(rc == CME_SUCC)
    {
        RspBuf = (CHAR *)OsaAllocZeroMemory(Conf->cmdRspLen*2 + 32);
        OsaCheck(RspBuf != PNULL, 0, 0, 0);
        strBuf = (CHAR *)OsaAllocZeroMemory(Conf->cmdRspLen*2 + 1);
        OsaCheck(strBuf != PNULL, 0, 0, 0);
        if (cmsHexToHexStr(strBuf, Conf->cmdRspLen*2+1, Conf->cmdRspData, Conf->cmdRspLen) > 0)
        {
            snprintf(RspBuf, Conf->cmdRspLen*2+32, "+CSIM: %d,\"%s\"", Conf->cmdRspLen*2, strBuf);
            ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
        }
        else
        {
            ECOMM_TRACE(UNILOG_ATCMD, simCSIMSetCnf_1, P_ERROR, 0, "simCSIMSetCnf atHexToString fail");
        }
        if (RspBuf != PNULL)
        {
            OsaFreeMemory(&RspBuf);
        }
        if (strBuf != PNULL)
        {
            OsaFreeMemory(&strBuf);
        }
        if (Conf->cmdRspData != PNULL)
        {
            OsaFreeMemory(&Conf->cmdRspData);
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simCRSMSetCnf
 * Description: Process CMI cnf msg of AT+CRSM
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simCRSMSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR *RspBuf = PNULL;
    CHAR *strBuf = PNULL;
    CmiSimRestrictedAccessCnf *Conf = (CmiSimRestrictedAccessCnf *)paras;

    if(rc == CME_SUCC)
    {
        RspBuf = (CHAR *)OsaAllocZeroMemory(Conf->cmdRspLen*2 + 32);
        OsaCheck(RspBuf != PNULL, 0, 0, 0);
        if (Conf->cmdRspLen > 0)
        {
            strBuf = (CHAR *)OsaAllocZeroMemory(Conf->cmdRspLen*2+1);
            OsaCheck(strBuf != PNULL, 0, 0, 0);

            if (cmsHexToHexStr(strBuf, Conf->cmdRspLen*2 + 1, Conf->cmdRspData, Conf->cmdRspLen) > 0)
            {
                snprintf(RspBuf, Conf->cmdRspLen*2+32, "+CRSM: %d,%d,\"%s\"", Conf->sw1, Conf->sw2, strBuf);
                ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
            }
            else
            {
                ECOMM_TRACE(UNILOG_ATCMD, simCRSMSetCnf_1, P_ERROR, 0, "simCRSMSetCnf atHexToString fail");
            }
        }
        else
        {
            snprintf(RspBuf, Conf->cmdRspLen*2+32, "+CRSM: %d,%d", Conf->sw1, Conf->sw2);
            ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);

        }

        if (RspBuf != PNULL)
        {
            OsaFreeMemory(&RspBuf);
        }
        if (strBuf != PNULL)
        {
            OsaFreeMemory(&strBuf);
        }
        if (Conf->cmdRspData != PNULL)
        {
            OsaFreeMemory(&Conf->cmdRspData);
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simCCHOSetCnf
 * Description: Process CMI cnf msg of AT+CCHO
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simCCHOSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32] = {0};
    CmiSimOpenLogicalChannelCnf *Conf = (CmiSimOpenLogicalChannelCnf *)paras;

    if(rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "%d", Conf->sessionId);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simCCHCSetCnf
 * Description: Process CMI cnf msg of AT+CCHC
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simCCHCSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32] = {0};
    //CmiSimCloseLogicalChannelCnf *Conf = (CmiSimCloseLogicalChannelCnf *)paras;

    if(rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+CCHC");
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simCGLASetCnf
 * Description: Process CMI cnf msg of AT+CGLA
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simCGLASetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR *RspBuf = PNULL;
    CmiSimGenLogicalChannelAccessCnf *Conf = (CmiSimGenLogicalChannelAccessCnf *)paras;

    if(rc == CME_SUCC)
    {
        RspBuf = (CHAR *)OsaAllocZeroMemory(Conf->cmdRspLen*2 + 32);
        OsaCheck(RspBuf != PNULL, 0, 0, 0);

        snprintf(RspBuf, Conf->cmdRspLen*2+32, "+CGLA: %d,\"", Conf->cmdRspLen*2);
        if (cmsHexToHexStr(RspBuf+strlen(RspBuf), Conf->cmdRspLen*2+1, Conf->cmdRspData, Conf->cmdRspLen) > 0)
        {
            strcat(RspBuf, "\"");
            ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
        }
        else
        {
            ECOMM_TRACE(UNILOG_ATCMD, simCGLASetCnf_1, P_ERROR, 0, "simCGLASetCnf atHexToString fail");
        }
        if (RspBuf != PNULL)
        {
            OsaFreeMemory(&RspBuf);
        }
        if (Conf->cmdRspData != PNULL)
        {
            OsaFreeMemory(&Conf->cmdRspData);
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simCCHCSetCnf
 * Description: Process CMI cnf msg of AT+ECSIMSLEEP=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simECSIMSLEEPSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simECSIMSLEEPGetCnf
 * Description: Process CMI cnf msg of AT+ECSIMSLEEP?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simECSIMSLEEPGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32];
    CmiSimGetSimSleepStateCnf *Conf = (CmiSimGetSimSleepStateCnf *)paras;

    memset(RspBuf, 0, sizeof(RspBuf));

    if(rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+ECSIMSLEEP: %d", Conf->mode);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simCPINRSetCnf
 * Description: Process CMI cnf msg of AT+CPINR
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simCPINRSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR rspBuf[64];
    CmiSimQueryRemianPinRetriesCnf *Conf = (CmiSimQueryRemianPinRetriesCnf *)paras;

    memset(rspBuf, 0, sizeof(rspBuf));

    if(rc == CME_SUCC)
    {
        switch (Conf->pinCode)
        {
            case CMI_SIM_PIN_1:
                sprintf((char*)rspBuf, "+CPINR: \"SIM PIN\",%d,%d", Conf->remianRetries[CMI_SIM_PIN_1], Conf->defaultRetries[CMI_SIM_PIN_1]);
                break;

            case CMI_SIM_PUK_1:
                sprintf((char*)rspBuf, "+CPINR: \"SIM PUK\",%d,%d", Conf->remianRetries[CMI_SIM_PUK_1], Conf->defaultRetries[CMI_SIM_PUK_1]);
                break;

            case CMI_SIM_PIN_ALL:
                sprintf((char*)rspBuf, "+CPINR: \"SIM PIN\",%d,%d\r\n+CPINR: \"SIM PUK\",%d,%d",
                    Conf->remianRetries[CMI_SIM_PIN_1], Conf->defaultRetries[CMI_SIM_PIN_1],
                    Conf->remianRetries[CMI_SIM_PUK_1], Conf->defaultRetries[CMI_SIM_PUK_1]);
                break;
        }
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)rspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simECSWCSetCnf
 * Description: Process CMI cnf msg of AT+ECSWC
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simECSWCSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR rspBuf[ATEC_IND_RESP_32_STR_LEN];
    UINT8 index = 0;
    CmiSimSetSimWriteCounterCnf *Conf = (CmiSimSetSimWriteCounterCnf *)paras;


    if(rc == CME_SUCC)
    {
        if(Conf->mode == CMI_SIM_WRITE_COUNTER_QUERY)
        {
            if (Conf->swcList.swcEfNum != 0)
            {
                for (index = 0; index < Conf->swcList.swcEfNum && index < CMI_MAX_SWC_EF_LIST_NUM; index++)
                {
                    memset(rspBuf, 0, sizeof(rspBuf));
                    switch (Conf->swcList.swcCnt[index].fileId)
                    {
                        case CMI_SIM_FID_KEYS_PS:
                            snprintf(rspBuf, ATEC_IND_RESP_32_STR_LEN, "+ECSWC: \"EF keysPS\",%d", Conf->swcList.swcCnt[index].writeCnt);
                            break;

                        case CMI_SIM_FID_EPSLOCI:
                            snprintf(rspBuf, ATEC_IND_RESP_32_STR_LEN, "+ECSWC: \"EF EPSLOCI\",%d", Conf->swcList.swcCnt[index].writeCnt);
                            break;

                        case CMI_SIM_FID_EPSNSC:
                            snprintf(rspBuf, ATEC_IND_RESP_32_STR_LEN, "+ECSWC: \"EF EPSNSC\",%d", Conf->swcList.swcCnt[index].writeCnt);
                            break;

                        case CMI_SIM_FID_FPLMN:
                            snprintf(rspBuf, ATEC_IND_RESP_32_STR_LEN, "+ECSWC: \"EF FPLMN\",%d", Conf->swcList.swcCnt[index].writeCnt);
                            break;

                        case CMI_SIM_FID_PLMN_ACT:
                            snprintf(rspBuf, ATEC_IND_RESP_32_STR_LEN, "+ECSWC: \"EF PLMNwACT\",%d", Conf->swcList.swcCnt[index].writeCnt);
                            break;

                        case CMI_SIM_FID_SMS:
                            snprintf(rspBuf, ATEC_IND_RESP_32_STR_LEN, "+ECSWC: \"EF SMS\",%d", Conf->swcList.swcCnt[index].writeCnt);
                            break;

                        case CMI_SIM_FID_SMSP:
                            snprintf(rspBuf, ATEC_IND_RESP_32_STR_LEN, "+ECSWC: \"EF SMSP\",%d", Conf->swcList.swcCnt[index].writeCnt);
                            break;

                        case CMI_SIM_FID_SMSS:
                            snprintf(rspBuf, ATEC_IND_RESP_32_STR_LEN, "+ECSWC: \"EF SMSS\",%d", Conf->swcList.swcCnt[index].writeCnt);
                            break;

                        case CMI_SIM_FID_SMSR:
                            snprintf(rspBuf, ATEC_IND_RESP_32_STR_LEN, "+ECSWC: \"EF SMSR\",%d", Conf->swcList.swcCnt[index].writeCnt);
                            break;

                        default:
                            snprintf(rspBuf, ATEC_IND_RESP_32_STR_LEN, "+ECSWC: \"unknown name fileId %04x\",%d", Conf->swcList.swcCnt[index].fileId, Conf->swcList.swcCnt[index].writeCnt);
                            break;
                    }
                    ret = atcReply( reqHandle,AT_RC_CONTINUE,0,(char *)rspBuf);
                }
            }
            else
            {
                snprintf(rspBuf, ATEC_IND_RESP_32_STR_LEN, "+ECSWC: \"null\",%d", Conf->swcList.swcCnt[index].writeCnt);
                ret = atcReply( reqHandle,AT_RC_CONTINUE,0,(char *)rspBuf);
            }

            ret = atcReply(reqHandle, AT_RC_OK, 0, PNULL);
        }
        else
        {
            ret = atcReply(reqHandle, AT_RC_OK, 0, PNULL);
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, PNULL);
    }

    return ret;
}

/******************************************************************************
 * simECSWCGetCnf
 * Description: Process CMI cnf msg of AT+ECSWC?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simECSWCGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR rspBuf[32];
    CmiSimGetSimWriteCounterModeCnf *Conf = (CmiSimGetSimWriteCounterModeCnf *)paras;

    memset(rspBuf, 0, sizeof(rspBuf));

    if(rc == CME_SUCC)
    {
        sprintf((char*)rspBuf, "+ECSWC: %d", Conf->mode);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)rspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simECSIMPDSetCnf
 * Description: Process CMI cnf msg of AT+ECSIMPD
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simECSIMPDSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}


/******************************************************************************
 * simECSIMPDGetCnf
 * Description: Process CMI cnf msg of AT+ECSIMPD
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simECSIMPDGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR rspBuf[32];
    CmiSimGetSimPresenceDetectModeCnf *Conf = (CmiSimGetSimPresenceDetectModeCnf *)paras;

    memset(rspBuf, 0, sizeof(rspBuf));

    if(rc == CME_SUCC)
    {
        sprintf((char*)rspBuf, "+ECSIMPD: %d", Conf->mode);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)rspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simCNUMGetCnf
 * Description: Process CMI cnf msg of AT+CNUM
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simCNUMGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    UINT8 index = 0;
    CHAR rspBuf[ATEC_IND_RESP_64_STR_LEN];
    CmiSimGetSubscriberNumCnf *Conf = (CmiSimGetSubscriberNumCnf *)paras;

    memset(rspBuf, 0, sizeof(rspBuf));

    if(rc == CME_SUCC)
    {
        if (Conf->totalNum != 0)
        {
            for (index = 0; index < Conf->totalNum && index < CMI_SIM_MAX_DIAL_NUMBER_LIST_SIZE; index++)
            {
                if (strlen(Conf->dialNumberList[index].dialNumStr) > 0)
                {
                    memset(rspBuf, 0, ATEC_IND_RESP_64_STR_LEN);
                    snprintf(rspBuf, ATEC_IND_RESP_64_STR_LEN, "+CNUM: \"%s\",\"%s\",%d",
                        Conf->dialNumberList[index].alphaId, Conf->dialNumberList[index].dialNumStr, Conf->dialNumberList[index].type);
                    ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, (char*)rspBuf);
                }
            }
        }
        ret = atcReply(reqHandle, AT_RC_OK, 0, PNULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simECUSATPSetCnf
 * Description: Process CMI cnf msg of AT+ECUSATP
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simECUSATPSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * simECUSATPSetCnf
 * Description: Process CMI cnf msg of AT+ECUSATP
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId simECUSATPGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR *RspBuf = PNULL;
    CHAR *strBuf = PNULL;
    CmiSimUsatGetTerminalProfileCnf *Conf = (CmiSimUsatGetTerminalProfileCnf *)paras;

    if(rc == CME_SUCC)
    {
        RspBuf = (CHAR *)OsaAllocZeroMemory(Conf->tpLength*2 + 32);
        OsaCheck(RspBuf != PNULL, 0, 0, 0);
        strBuf = (CHAR *)OsaAllocZeroMemory(Conf->tpLength*2 + 1);
        OsaCheck(strBuf != PNULL, 0, 0, 0);
        if (cmsHexToHexStr(strBuf, Conf->tpLength*2+1, Conf->tpData, Conf->tpLength) > 0)
        {
            snprintf(RspBuf, Conf->tpLength*2+32, "+ECUSATP: %d,\"%s\"", Conf->tpLength*2, strBuf);
            ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
        }
        else
        {
            ECOMM_TRACE(UNILOG_ATCMD, simECUSATPGetCnf_1, P_ERROR, 0, "simECUSATPGetCnf atHexToString fail");
        }
        if (RspBuf != PNULL)
        {
            OsaFreeMemory(&RspBuf);
        }
        if (strBuf != PNULL)
        {
            OsaFreeMemory(&strBuf);
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/****************************************************************************************
*  Process CI ind msg for +ECPIN:<code>
*
****************************************************************************************/
CmsRetId simECPINInd(UINT32 chanId, void *paras)
{
    CmsRetId    ret = CMS_RET_SUCC;
    CHAR        indBuf[32], tempBuf[22];
    CmiSimUiccPinStateInd *Ind = (CmiSimUiccPinStateInd *)paras;
    UINT32  rptMode = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_ECPIN_STATE_RPT_CFG);

    if (1 == rptMode)
    {
        memset(indBuf, 0, sizeof(indBuf));
        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf((char *)indBuf, "+ECPIN: ");

        switch(Ind->uiccPinState)
        {
            case CMI_SIM_PIN_STATE_READY:
                sprintf((char*)tempBuf, "READY");
                break;

            case CMI_SIM_PIN_STATE_PIN1_REQUIRED:
            case CMI_SIM_PIN_STATE_UNIVERSALPIN_REQUIRED:
                sprintf((char*)tempBuf, "SIM PIN");
                break;

            case CMI_SIM_PIN_STATE_UNBLOCK_PIN1_REQUIRED:
            case CMI_SIM_PIN_STATE_UNBLOCK_UNIVERSALPIN_REQUIRED:
                sprintf((char*)tempBuf, "SIM PUK");
                break;

            case CMI_SIM_PIN_STATE_UNBLOCK_PIN1_BLOCKED:
            case CMI_SIM_PIN_STATE_UNBLOCK_UNIVERSALPIN_BLOCKED:
                sprintf((char*)tempBuf, "SIM PUK BLOCKED");
                break;

            case CMI_SIM_PIN_STATE_SIM_NOT_READY:
                sprintf((char*)tempBuf, "SIM NOT READY");
                break;

            default:
                ECOMM_TRACE(UNILOG_ATCMD, simECPINInd_1, P_INFO, 1, "unexpected pin state %d", Ind->uiccPinState);
                return ret;
        }
        strcat((char *)indBuf,(char *)tempBuf);
        ret = atcURC(chanId, (char*)indBuf);
    }

    return ret;
}


/******************************************************************************
 ******************************************************************************
 * static func table
 ******************************************************************************
******************************************************************************/
#define __DEFINE_STATIC_VARIABLE__ //just for easy to find this position in SS

/******************************************************************************
 * simCmiCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const CmiCnfFuncMapList simCmiCnfHdrList[] =
{
    {CMI_SIM_GET_SUBSCRIBER_ID_CNF, simCIMIGetCnf},
    {CMI_SIM_OPERATE_PIN_CNF, simCPINSetCnf},
    {CMI_SIM_LOCK_FACILITY_CNF, simCLCKSetCnf},
    {CMI_SIM_GET_PIN_STATE_CNF, simCPINGetCnf},
    {CMI_SIM_GENERIC_ACCESS_CNF, simCSIMSetCnf},
    {CMI_SIM_RESTRICTED_ACCESS_CNF, simCRSMSetCnf},
    {CMI_SIM_OPEN_LOGICAL_CHANNEL_CNF, simCCHOSetCnf},
    {CMI_SIM_CLOSE_LOGICAL_CHANNEL_CNF, simCCHCSetCnf},
    {CMI_SIM_GENERIC_LOGICAL_CHANNEL_ACCESS_CNF, simCGLASetCnf},
    {CMI_SIM_SET_TERMINAL_PROFILE_CNF, simECUSATPSetCnf},
    {CMI_SIM_GET_TERMINAL_PROFILE_CNF, simECUSATPGetCnf},
    {CMI_SIM_GET_ICCID_CNF, simECICCIDGetCnf},
    {CMI_SIM_SET_SIM_SLEEP_CNF, simECSIMSLEEPSetCnf},
    {CMI_SIM_GET_SIM_SLEEP_STATE_CNF, simECSIMSLEEPGetCnf},
    {CMI_SIM_QUERY_REMIAN_PIN_RETRIES_CNF, simCPINRSetCnf},
    {CMI_SIM_SET_SIM_WRITE_COUNTER_CNF, simECSWCSetCnf},
    {CMI_SIM_GET_SIM_WRITE_COUNTER_MODE_CNF, simECSWCGetCnf},
    {CMI_SIM_SET_SIM_PRESENCE_DETECT_CNF, simECSIMPDSetCnf},
    {CMI_SIM_GET_SIM_PRESENCE_DETECT_MODE_CNF, simECSIMPDGetCnf},
    {CMI_SIM_GET_SUBSCRIBER_NUMBER_CNF, simCNUMGetCnf},
    {CMI_SIM_GET_SIM_SWITCH_STATE_CNF,  simSwitchStateGetCnf},
    {CMI_SIM_SET_SIM_SWITCH_STATE_CNF,  simSwitchStateSetCnf},

    {CMI_SIM_PRIM_BASE, PNULL}  /* this should be the last */
};

/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS

/******************************************************************************
 * atSimProcCmiCnf
 * Description: Process CMI cnf msg for SIM sub-module
 * input: CacCmiCnf *pCmiCnf;  //ptr of cmi cnf
 * output: void
 * Comment:
******************************************************************************/
void atSimProcCmiCnf(CacCmiCnf *pCmiCnf)
{
    UINT8 hdrIndex = 0;
    CmiCnfHandler cmiCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmiCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    while (simCmiCnfHdrList[hdrIndex].primId != 0)
    {
        if (simCmiCnfHdrList[hdrIndex].primId == primId)
        {
            cmiCnfHdr = simCmiCnfHdrList[hdrIndex].cmiCnfHdr;
            break;
        }
        hdrIndex++;
    }

    if (cmiCnfHdr != PNULL)
    {
        (*cmiCnfHdr)(pCmiCnf->header.srcHandler, pCmiCnf->header.rc, pCmiCnf->body);
    }
    else
    {
        OsaDebugBegin(FALSE, primId, pCmiCnf->header.srcHandler, AT_GET_CMI_SG_ID(pCmiCnf->header.cnfId));
        OsaDebugEnd();
    }

    return;
}

/******************************************************************************
 * atSimProcCmiInd
 * Description: Process CMI Ind msg for SIM sub-module
 * input: CacCmiInd *pCmiInd;  //ptr of cmi Ind
 * output: void
 * Comment:
******************************************************************************/
void atSimProcCmiInd(CacCmiInd *pCmiInd)
{
    UINT16 primId = 0;
    UINT32 chanId = 0;

    OsaDebugBegin(pCmiInd != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiInd->header.indId);

    if (pCmiInd->header.reqHandler != BROADCAST_IND_HANDLER)
    {
        //if not broadcast Ind, add here
    }
    else //broadcast Ind
    {
        for (chanId = 1; chanId < CMS_CHAN_NUM; chanId++)
        {
            if (atcBeURCConfiged(chanId) != TRUE)
            {
                continue;
            }

            switch(primId)
            {
                case CMI_SIM_UICC_PIN_STATE_IND:
#ifndef  FEATURE_REF_AT_ENABLE
                    simECPINInd(chanId, pCmiInd->body);
#endif
                    break;

                default:
                    break;
            }
        }
    }


}



