/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_ps.c
*
*  Description: Process packet switched service related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdio.h>
#include "cmsis_os2.h"
#include "at_util.h"
//#include "atec_controller.h"
#include "cmips.h"
#include "atec_ps.h"
#include "ps_ps_if.h"
#include "atec_ps_cnf_ind.h"

/******************************************************************************
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_VARIABLES__ //just for easy to find this position in SS

/*
 * cmeIdStrOrderTbl
 *  CME ERROR ID -> ERROR STRING table.
 * !!!! NOTE: ERROR ID should in ascending order !!!!
*/
const AtcCeerCauseMapping  ceerEmmCauseIdStrOrderTbl[] =
{
    /* !!! should in ascending order !!! */
    {CMI_EMM_CAUSE_IMSI_UNKNOWN_IN_HSS,                                     "EMM_CAUSE_IMSI_UNKNOWN_IN_HSS"},
    {CMI_EMM_CAUSE_ILLEGAL_UE,                                              "EMM_CAUSE_ILLEGAL_UE"},
    {CMI_EMM_CAUSE_IMEI_NOT_ACCEPTED,                                       "EMM_CAUSE_IMEI_NOT_ACCEPTED"},
    {CMI_EMM_CAUSE_ILLEGAL_ME,                                              "EMM_CAUSE_ILLEGAL_ME"},
    {CMI_EMM_CAUSE_EPS_SERVICES_NOT_ALLOWED,                                "EMM_CAUSE_EPS_SERVICES_NOT_ALLOWED"},
    {CMI_EMM_CAUSE_EPS_AND_NON_EPS_SERVICES_NOT_ALLOWED,                    "EMM_CAUSE_EPS_AND_NON_EPS_SERVICES_NOT_ALLOWED"},
    {CMI_EMM_CAUSE_UE_ID_CAN_NOT_BE_DERIVED_IN_NETWORK,                     "EMM_CAUSE_UE_ID_CAN_NOT_BE_DERIVED_IN_NETWORK"},
    {CMI_EMM_CAUSE_IMPLICITLY_DETACHED,                                     "EMM_CAUSE_IMPLICITLY_DETACHED"},
    {CMI_EMM_CAUSE_PLMN_NOT_ALLOWED,                                        "EMM_CAUSE_PLMN_NOT_ALLOWED"},
    {CMI_EMM_CAUSE_TRACKING_AREA_NOT_ALLOWED,                               "EMM_CAUSE_TRACKING_AREA_NOT_ALLOWED"},
    {CMI_EMM_CAUSE_ROAMING_NOT_ALLOWED_IN_THIS_TRACKING_AREA,               "EMM_CAUSE_ROAMING_NOT_ALLOWED_IN_THIS_TRACKING_AREA"},
    {CMI_EMM_CAUSE_EPS_SERVICE_NOT_ALLOWED_IN_THIS_PLMN,                    "EMM_CAUSE_EPS_SERVICE_NOT_ALLOWED_IN_THIS_PLMN"},
    {CMI_EMM_CAUSE_NO_SUITABLE_CELLS_IN_TRACKING_AREA,                      "EMM_CAUSE_NO_SUITABLE_CELLS_IN_TRACKING_AREA"},
    {CMI_EMM_CAUSE_MSC_TEMPORARILY_NOT_REACHABLE,                           "EMM_CAUSE_MSC_TEMPORARILY_NOT_REACHABLE"},
    {CMI_EMM_CAUSE_NETWORK_FAILURE,                                         "EMM_CAUSE_NETWORK_FAILURE"},
    {CMI_EMM_CAUSE_CS_DOMAIN_NOT_AVAILABLE,                                 "EMM_CAUSE_CS_DOMAIN_NOT_AVAILABLE"},
    {CMI_EMM_CAUSE_ESM_FAILURE,                                             "EMM_CAUSE_ESM_FAILURE"},
    {CMI_EMM_CAUSE_MAC_FAILURE,                                             "EMM_CAUSE_MAC_FAILURE"},
    {CMI_EMM_CAUSE_SYNCH_FAILURE,                                           "EMM_CAUSE_SYNCH_FAILURE"},
    {CMI_EMM_CAUSE_CONGESTION,                                              "EMM_CAUSE_CONGESTION"},
    {CMI_EMM_CAUSE_UE_SECURITY_CAPAILITIES_MISMATCH,                        "EMM_CAUSE_UE_SECURITY_CAPAILITIES_MISMATCH"},
    {CMI_EMM_CAUSE_SECURITY_MODE_REJECTED_UNSPECIFIED,                      "EMM_CAUSE_SECURITY_MODE_REJECTED_UNSPECIFIED"},
    {CMI_EMM_CAUSE_NOT_AUTHORIZED_FOR_THIS_CSG,                             "EMM_CAUSE_NOT_AUTHORIZED_FOR_THIS_CSG"},
    {CMI_EMM_CAUSE_NON_EPS_AUTHENTICATION_UNACCEPTABLE,                     "EMM_CAUSE_NON_EPS_AUTHENTICATION_UNACCEPTABLE"},
    {CMI_EMM_CAUSE_REQUESTED_SERVICE_OPTION_NOT_AUTHORIZED_IN_THIS_PLMN,    "EMM_CAUSE_REQUESTED_SERVICE_OPTION_NOT_AUTHORIZED_IN_THIS_PLMN"},
    {CMI_EMM_CAUSE_CS_SERVICE_TEMPORARILY_NOT_AVAILABLE,                    "EMM_CAUSE_CS_SERVICE_TEMPORARILY_NOT_AVAILABLE"},
    {CMI_EMM_CAUSE_NO_EPS_BEARER_CONTEXT_ACTIVATED,                         "EMM_CAUSE_NO_EPS_BEARER_CONTEXT_ACTIVATED"},
    {CMI_EMM_CAUSE_SERVERE_NETWORK_FAILURE,                                 "EMM_CAUSE_SERVERE_NETWORK_FAILURE"},
    {CMI_EMM_CAUSE_SYMANTICALLY_INCORRECT_MESSAGE,                          "EMM_CAUSE_SYMANTICALLY_INCORRECT_MESSAGE"},
    {CMI_EMM_CAUSE_INVALID_MANDATORY_INFORMATION,                           "EMM_CAUSE_INVALID_MANDATORY_INFORMATION"},
    {CMI_EMM_CAUSE_MESSAGE_TYPE_NON_EXISTENT_OR_NOT_IMPLEMENTED,            "EMM_CAUSE_MESSAGE_TYPE_NON_EXISTENT_OR_NOT_IMPLEMENTED"},
    {CMI_EMM_CAUSE_MESSAGE_TYPE_NOT_COMPATIBLE_WITH_THE_PROTOCOL_STATE,     "EMM_CAUSE_MESSAGE_TYPE_NOT_COMPATIBLE_WITH_THE_PROTOCOL_STATE"},
    {CMI_EMM_CAUSE_INFORMATION_ELEMENT_NON_EXISTENT_OR_NOT_IMPLEMENTED,     "EMM_CAUSE_INFORMATION_ELEMENT_NON_EXISTENT_OR_NOT_IMPLEMENTED"},
    {CMI_EMM_CAUSE_CONDITIONAL_IE_ERROR,                                    "EMM_CAUSE_CONDITIONAL_IE_ERROR"},
    {CMI_EMM_CAUSE_MESSAGE_NOT_COMPATIBLE_WITH_THE_PROTOCOL_STATE,          "EMM_CAUSE_MESSAGE_NOT_COMPATIBLE_WITH_THE_PROTOCOL_STATE"},
    {CMI_EMM_CAUSE_PROTOCOL_ERROR_UNSPECIFIED,                              "EMM_CAUSE_PROTOCOL_ERROR_UNSPECIFIED"}

    /*
     * ADD here, in order !!!!
    */

};


/******************************************************************************
 ******************************************************************************
 * INERNAL/STATIC FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position in SS

/**
  \fn           const CHAR* atcGetCEEREmmCauseStr(UINT16 errId)
  \brief        Get CEER Emm Cause string, accord to the emm cause id
  \param[in]    UINT16 emm cause id
  \returns      const CHAR
*/
const CHAR* atcGetCEEREmmCauseStr(UINT16 emmCauseId)
{
    UINT32  startIdx = 0, endIdx = sizeof(ceerEmmCauseIdStrOrderTbl)/sizeof(AtcCeerCauseMapping) -1 ;
    UINT32  curIdx = 0;

    OsaDebugBegin(emmCauseId > 0 && emmCauseId <= CMI_EMM_CAUSE_PROTOCOL_ERROR_UNSPECIFIED, emmCauseId , 0, 0);
    return "INVALID_EMM_CAUSE";
    OsaDebugEnd();

    while (startIdx +1 < endIdx)
    {
        curIdx = (startIdx + endIdx) >> 1;
        if (emmCauseId == ceerEmmCauseIdStrOrderTbl[curIdx].causeId)
        {
            return ceerEmmCauseIdStrOrderTbl[curIdx].pStr;
        }
        else if (emmCauseId > ceerEmmCauseIdStrOrderTbl[curIdx].causeId)
        {
            startIdx = curIdx;
        }
        else    //errId < cmeIdStrOrderTbl[curIdx].errId
        {
            endIdx = curIdx;
        }
    }

    if (emmCauseId == ceerEmmCauseIdStrOrderTbl[startIdx].causeId)
    {
        return ceerEmmCauseIdStrOrderTbl[startIdx].pStr;
    }
    else if (emmCauseId == ceerEmmCauseIdStrOrderTbl[endIdx].causeId)
    {
        return ceerEmmCauseIdStrOrderTbl[endIdx].pStr;
    }

    OsaDebugBegin(FALSE, emmCauseId, 0, 0);
    OsaDebugEnd();

    return "INVALID_EMM_CAUSE";
}

#if 0
/******************************************************************************
 * atPsCGCONTRDPGetCnf
 * Description: Process CMI cnf msg of AT+CGCONTRDP=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
UINT8 psCheckCidCnf(void *paras)
{
    //UINT8 retOper = 0xff;
    CmiPsGetAllBearersCidsInfoCnf *Conf = (CmiPsGetAllBearersCidsInfoCnf *)paras;

    #if 0
    CMI_PS_CGDCONT_GET_CIDS_OPER =1,       /* CGDCONT */
    CMI_PS_CGCMOD_GET_CIDS_CNF,           /* CGCMOD */
    CMI_PS_CGPADDR_GET_CIDS_CNF,          /* CGPADDR */
    CMI_PS_CGCONTRDP_GET_CIDS_CNF,        /* CGCONTRDP */
    CMI_PS_CGSCONTRDP_GET_CIDS_CNF,       /* CGSCONTRDP */
    CMI_PS_CGTFTRDP_GET_CIDS_CNF,         /* CGTFTRDP */
    CMI_PS_CGEQOSRDP_GET_CIDS_CNF,        /* CGEQOSRDP */
    CMI_PS_CGAPNRC_GET_CIDS_CNF,          /* CGAPNRC */

    CMI_PS_GET_CIDS_NUM_OPERS
    #endif

    return Conf->getCidsCmd;
}
#endif

/******************************************************************************
 * atPsCGDCONTSetCnf
 * Description: AT+CGDCONT=[<>]
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGDCONTSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atPsCidListGetCnf
 * Description: AT+CGDCONT?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCidListGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    /*
     * logic is not right, - TBD
    */
    #if 0
    CHAR RspBuf[80], TempBuf[8];
    UINT16 cidflag = 0;
    CmiPsGetAllBearersCidsInfoCnf *Conf = (CmiPsGetAllBearersCidsInfoCnf *)paras;
    memset(RspBuf, 0, sizeof(RspBuf));
    memset(TempBuf, 0, sizeof(TempBuf));

    if(rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+CGDCONT: ");

        for(int i=0; i<16; i++)
        {
            if(2 == Conf->psCidInfo[i].state)    //invalid - 0/  defined - 1/  actived - 2/
            {
                sprintf(TempBuf, "%d", i+1);
                cidflag = 0x1;
                break;
            }
        }
        if(cidflag == 0x1)
        {
            strcat((char *)RspBuf,(char *)TempBuf);
        }
        else
        {
            strcat((char *)RspBuf,(char *)"invalid");
        }
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    #endif
    return ret;
}


/******************************************************************************
 * atPsCidContGetCnf
 * Description: AT+CGDCONT=<cid>
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCidContGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    rspBuf[ATEC_IND_RESP_256_STR_LEN] = {0};
    CHAR    tmpBuf[ATEC_IND_RESP_256_STR_LEN] = {0};
    //CHAR    *pTmpBuf = PNULL;
    BOOL    dualstack = FALSE;
    //UINT16  tmpBufSize = 0; //sizeof "pTmpBuf"
    UINT16  bufStrLen = 0, fixHdrIdx = 0;

    CmiPsGetDefinedBearerCtxCnf *pCmiCnf = (CmiPsGetDefinedBearerCtxCnf *)paras;

    /*
     * +CGDCONT: <cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp>[,<IPv4AddrAlloc>[,<request_type>
     *           [,<PCSCF_discovery>[,<IM_CN_Signalling_Flag_Ind>[,<NSLPI>[,<securePCO>[,<IPv4_MTU_discovery>
     *           [,<Local_Addr_Ind>[,<NonIP_MTU_discovery>[,<Reliable_Data_Service>[,<SSC_mode>[,<SNSSAI>
     *           [,<Pref_access_type>[,<RQoS_ind>[,<MH6-PDU>[,<Alwayson_req>]]]]]]]]]]]]]]]]
     * <CR><LF>
     * +CGDCONT: <cid>,<PDP_type>,<APN>,<PDP_addr>,<d_comp>,<h_comp>[,<IPv4AddrAlloc>[,<request_type>
     *           [,<PCSCF_discovery>[,<IM_CN_Signalling_Flag_Ind>[,<NSLPI>[,<securePCO>[,<IPv4_MTU_discovery>
     *           [,<Local_Addr_Ind>[,<NonIP_MTU_discovery>[,<Reliable_Data_Service>[,<SSC_mode>[,<SNSSAI>
     *           [,<Pref_access_type>[,<RQoS_ind>[,<MH6-PDU>[,<Alwayson_req>]]]]]]]]]]]]]]]]
     *
    */

    if (rc != CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, PNULL);
        return ret;
    }

    //if (rc == CME_SUCC)
    //{
    if (pCmiCnf->bCtxValid == FALSE)
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
        return ret;
    }

    memset(tmpBuf, 0, sizeof(tmpBuf));
    memset(rspBuf, 0, sizeof(rspBuf));

    /* cid */
    snprintf(rspBuf, ATEC_IND_RESP_256_STR_LEN, "\r\n+CGDCONT: %d,", pCmiCnf->definedBrCtx.cid);

    /* PDP type */
    psPdpTypeToStr(pCmiCnf->definedBrCtx.pdnType, tmpBuf);
    atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));


    /* ,APN, */
    OsaDebugBegin(pCmiCnf->definedBrCtx.apnLength <= CMI_PS_MAX_APN_LEN, pCmiCnf->definedBrCtx.apnLength, CMI_PS_MAX_APN_LEN, 0);
    pCmiCnf->definedBrCtx.apnLength = CMI_PS_MAX_APN_LEN;
    OsaDebugEnd();

    //memset(tmpBuf, 0, sizeof(tmpBuf));
    if (pCmiCnf->definedBrCtx.apnLength > 0)
    {
        //strncpy(tmpBuf, pCmiCnf->definedBrCtx.apnStr, pCmiCnf->definedBrCtx.apnLength);
        //tmpBuf[pCmiCnf->definedBrCtx.apnLength] = '\0';

        /* ," */
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, ",\"", 2);

        /* apn */
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, (CHAR *)(pCmiCnf->definedBrCtx.apnStr), pCmiCnf->definedBrCtx.apnLength);

        /* ", */
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, "\",", 2);
    }
    else
    {
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, ",,", strlen(",,"));
    }

    fixHdrIdx = strlen(rspBuf);

    if (pCmiCnf->definedBrCtx.pdnType == CMI_PS_PDN_TYPE_IP_V4V6 &&
        pCmiCnf->definedBrCtx.ipv4Addr.addrType == CMI_IPV4_ADDR &&
        pCmiCnf->definedBrCtx.ipv6Addr.addrType == CMI_FULL_IPV6_ADDR)  //only IPv6 full IP need to print
    {
        dualstack = TRUE;
    }

    if (!dualstack)     //FALSE
    {
        memset(tmpBuf, 0, sizeof(tmpBuf));
        if (pCmiCnf->definedBrCtx.ipv4Addr.addrType == CMI_IPV4_ADDR)
        {
            if (psBeAllZero(pCmiCnf->definedBrCtx.ipv4Addr.addrData, CMI_MAX_IP_ADDR_LEN) == FALSE) //IPV4 address valid
            {
                psUint8Ipv4AddrToStr(pCmiCnf->definedBrCtx.ipv4Addr.addrData, tmpBuf);

                /* \" */
                atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, "\"", 1);

                /* ip addr */
                atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));

                /*\"*/
                atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, "\"", 1);
            }
            else
            {
                OsaDebugBegin(FALSE, pCmiCnf->definedBrCtx.ipv4Addr.addrData[0], pCmiCnf->definedBrCtx.ipv4Addr.addrData[1], pCmiCnf->definedBrCtx.ipv4Addr.addrType);
                OsaDebugEnd();
            }
        }
        else if (pCmiCnf->definedBrCtx.ipv6Addr.addrType == CMI_FULL_IPV6_ADDR) //IPV6 interface not print
        {
            if (psBeAllZero(pCmiCnf->definedBrCtx.ipv6Addr.addrData, CMI_MAX_IP_ADDR_LEN) == FALSE)
            {
                psUint8Ipv6AddrToStr(CMI_FULL_IPV6_ADDR, pCmiCnf->definedBrCtx.ipv6Addr.addrData, tmpBuf);

                /* \" */
                atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, "\"", 1);

                /* ip addr */
                atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));

                /*\"*/
                atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, "\"", 1);
            }
            else
            {
                OsaDebugBegin(FALSE, pCmiCnf->definedBrCtx.ipv4Addr.addrData[0], pCmiCnf->definedBrCtx.ipv4Addr.addrData[1], pCmiCnf->definedBrCtx.ipv4Addr.addrType);
                OsaDebugEnd();
            }
        }
        else if (pCmiCnf->definedBrCtx.ipv4Addr.addrType == CMI_IPV6_INTERFACE_ADDR)
        {
            ECOMM_TRACE(UNILOG_ATCMD_EXEC, psCidContGetCnf_inter_ipv6_w_1, P_WARNING, 1,
                        "AT PS, CID: %d, only ipv6 interface address, not print out", pCmiCnf->definedBrCtx.cid);
        }

        /*,d_comp,h_comp,   //no d_comp & no h_comp  */
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, ",,,", 3);

        /*IPv4AddrAlloc*/
        if(pCmiCnf->definedBrCtx.ipv4AddrAllocPresent)
        {
            snprintf(tmpBuf, ATEC_IND_RESP_256_STR_LEN, "%d", pCmiCnf->definedBrCtx.ipv4AlloType);
            atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));
        }

        /*,request type,P-CSCF_discovery,IM_CN_Signalling_Flag_Ind, */
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, ",,,,", 4);

        /*NSLPI*/
        if(pCmiCnf->definedBrCtx.NSLPIPresent)
        {
            snprintf(tmpBuf, ATEC_IND_RESP_256_STR_LEN, "%d", pCmiCnf->definedBrCtx.NSLPI);
            atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));
        }

        /*,securePCO,*/
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, ",,", 2);

        /*IPv4 MTU discovery*/
        if(pCmiCnf->definedBrCtx.ipv4MTUDiscoveryPresent)
        {
            snprintf(tmpBuf, ATEC_IND_RESP_256_STR_LEN, "%d", pCmiCnf->definedBrCtx.ipv4MtuDisByNas);
            atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));
        }

        /*,local addr ind,*/
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, ",,", 2);

        /*non-ip MTU discovery*/
        if(pCmiCnf->definedBrCtx.nonIpMtuDiscoveryPresent)
        {
            snprintf(tmpBuf, ATEC_IND_RESP_256_STR_LEN, "%d", pCmiCnf->definedBrCtx.nonIpMtuDisByNas);
            atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));
        }

        /*
         * Following are all not need now
         * [,<Reliable_Data_Service>[,<SSC_mode>[,<SNSSAI>[,<Pref_access_type>[,<RQoS_ind>[,<MH6-PDU>[,<Alwayson_req>]]]]]]
        */


        /*clean all ',' behind the last valid value*/
        for (bufStrLen = ATEC_IND_RESP_256_STR_LEN; bufStrLen > 0; bufStrLen--)
        {
            if (rspBuf[bufStrLen] == ',')
            {
                rspBuf[bufStrLen] = 0;
            }
            else if(rspBuf[bufStrLen] != 0)
            {
                break;
            }
        }

        if (pCmiCnf->bContinue)
        {
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, rspBuf);
        }
        else
        {   //the last
            ret = atcReply(reqHandle, AT_RC_OK, 0, rspBuf);
        }

    }
    else    //dual IP
    {
        /*
         * need to print two line
        */
        //strcat((char *)RspBuf, (char *)preBuf);
        OsaCheck(pCmiCnf->definedBrCtx.ipv4Addr.addrType == CMI_IPV4_ADDR &&
                 pCmiCnf->definedBrCtx.ipv6Addr.addrType == CMI_FULL_IPV6_ADDR,
                 pCmiCnf->definedBrCtx.ipv4Addr.addrType, pCmiCnf->definedBrCtx.ipv6Addr.addrType, 0);

        memset(tmpBuf, 0, sizeof(tmpBuf));

        psUint8Ipv4AddrToStr(pCmiCnf->definedBrCtx.ipv4Addr.addrData, tmpBuf);
        /* \" */
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, "\"", 1);
        /* ip addr */
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));
        /*\"*/
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, "\"", 1);

        /*,d_comp,h_comp,*/
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, ",,,", 3);

        /*IPv4AddrAlloc*/
        if (pCmiCnf->definedBrCtx.ipv4AddrAllocPresent)
        {
            snprintf(tmpBuf, ATEC_IND_RESP_256_STR_LEN, "%d", pCmiCnf->definedBrCtx.ipv4AlloType);
            atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));
        }

        /*,request type,P-CSCF_discovery,IM_CN_Signalling_Flag_Ind,*/
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, ",,,,", 4);

        /*NSLPI*/
        if (pCmiCnf->definedBrCtx.NSLPIPresent)
        {
            snprintf(tmpBuf, ATEC_IND_RESP_256_STR_LEN, "%d", pCmiCnf->definedBrCtx.NSLPI);
            atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));
        }

        /*,securePCO,IPv4 MTU discovery,*/
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, ",,,", 3);

        /*IPv4 MTU discovery*/
        if(pCmiCnf->definedBrCtx.ipv4MTUDiscoveryPresent)
        {
            snprintf(tmpBuf, ATEC_IND_RESP_256_STR_LEN, "%d", pCmiCnf->definedBrCtx.ipv4MtuDisByNas);
            atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));
        }

        /*,local addr ind,*/
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, ",,", 2);

        /*non-ip MTU discovery*/
        if (pCmiCnf->definedBrCtx.nonIpMtuDiscoveryPresent)
        {
            snprintf(tmpBuf, ATEC_IND_RESP_256_STR_LEN, "%d", pCmiCnf->definedBrCtx.nonIpMtuDisByNas);
            atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));
        }

        /*
         * Following are all not need now
         * [,<Reliable_Data_Service>[,<SSC_mode>[,<SNSSAI>[,<Pref_access_type>[,<RQoS_ind>[,<MH6-PDU>[,<Alwayson_req>]]]]]]
        */

        /*clean all ',' behind the last valid value*/
        for (bufStrLen = ATEC_IND_RESP_256_STR_LEN; bufStrLen > 0; bufStrLen--)
        {
            if (rspBuf[bufStrLen] == ',')
            {
                rspBuf[bufStrLen] = 0;
            }
            else if(rspBuf[bufStrLen] != 0)
            {
                break;
            }
        }

        ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, (char*)rspBuf);

        /*********************************************
         * IPv6
        *********************************************/

        /* fix header: +CGDCONT: <cid>,<PDP_type>,<APN>, */
        memset(rspBuf+fixHdrIdx, 0x00, ATEC_IND_RESP_256_STR_LEN-fixHdrIdx);

        /* addr */
        memset(tmpBuf, 0x00, ATEC_IND_RESP_256_STR_LEN);
        psUint8Ipv6AddrToStr(CMI_FULL_IPV6_ADDR, pCmiCnf->definedBrCtx.ipv6Addr.addrData, tmpBuf);
        /* \" */
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, "\"", 1);
        /* ip addr */
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));
        /*\"*/
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, "\"", 1);

        /*,d_comp,h_comp,*/
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, ",,,", 3);

        /*IPv4AddrAlloc*/
        if (pCmiCnf->definedBrCtx.ipv4AddrAllocPresent)
        {
            snprintf(tmpBuf, ATEC_IND_RESP_256_STR_LEN, "%d", pCmiCnf->definedBrCtx.ipv4AlloType);
            atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));
        }

        /*,request type,P-CSCF_discovery,IM_CN_Signalling_Flag_Ind,*/
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, ",,,,", 4);

        /*NSLPI*/
        if (pCmiCnf->definedBrCtx.NSLPIPresent)
        {
            snprintf(tmpBuf, ATEC_IND_RESP_256_STR_LEN, "%d", pCmiCnf->definedBrCtx.NSLPI);
            atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));
        }

        /*,securePCO,IPv4 MTU discovery,*/
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, ",,,", 3);

        /*IPv4 MTU discovery*/
        if(pCmiCnf->definedBrCtx.ipv4MTUDiscoveryPresent)
        {
            snprintf(tmpBuf, ATEC_IND_RESP_256_STR_LEN, "%d", pCmiCnf->definedBrCtx.ipv4MtuDisByNas);
            atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));
        }

        /*,local addr ind,*/
        atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, ",,", 2);

        /*non-ip MTU discovery*/
        if (pCmiCnf->definedBrCtx.nonIpMtuDiscoveryPresent)
        {
            snprintf(tmpBuf, ATEC_IND_RESP_256_STR_LEN, "%d", pCmiCnf->definedBrCtx.nonIpMtuDisByNas);
            atStrnCat(rspBuf, ATEC_IND_RESP_256_STR_LEN, tmpBuf, strlen(tmpBuf));
        }

        /*
         * Following are all not need now
         * [,<Reliable_Data_Service>[,<SSC_mode>[,<SNSSAI>[,<Pref_access_type>[,<RQoS_ind>[,<MH6-PDU>[,<Alwayson_req>]]]]]]
        */

        /*clean all ',' behind the last valid value*/
        for (bufStrLen = ATEC_IND_RESP_256_STR_LEN; bufStrLen > 0; bufStrLen--)
        {
            if (rspBuf[bufStrLen] == ',')
            {
                rspBuf[bufStrLen] = 0;
            }
            else if(rspBuf[bufStrLen] != 0)
            {
                break;
            }
        }


        if (pCmiCnf->bContinue)
        {
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, (char*)rspBuf);
        }
        else
        {
            ret = atcReply(reqHandle, AT_RC_OK, 0, rspBuf);
        }
    }
    //}

    return ret;
}


/******************************************************************************
 * psCidContDeleteCnf
 * Description: AT+CGDCONT=<cid>
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCidContDeleteCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if (rc == CME_SUCC)
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
 * atPsCGATTSetConf
 * Description: Process CMI cnf msg of AT+CGPADDR=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGPADDRGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    pRspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CHAR    tmpStrBuf[80] = {0};
    CmiPsGetBearerIpAddrCnf     *pCmiConf = (CmiPsGetBearerIpAddrCnf *)paras;
    UINT16  rspStrLen = 0;

    /*
     * if CID valid, but no valid IP
     *  +CGPADDR: <cid>
     * if CID valid, and valid IP
     *  +CGPADDR: <cid>, <ip1>, <ip2>
    */

    /*
     * If more CID/PDP activated.
     * [+CGPADDR: <cid>[,<PDP_addr_1>[,<PDP_addr_2>]]]
     * [<CR><LF>+CGPADDR: <cid>,[<PDP_addr_1>[,<PDP_addr_2>]]
     * [...]]
     * ====================================
     * Just:
     * <\r><\n>+CGPADDR: <cid>[,<PDP_addr_1>[,<PDP_addr_2>]]
     * <\r><\n>+CGPADDR: <cid2>[,<PDP_addr_1>[,<PDP_addr_2>]]<\r\n>
     * <\r><\n>OK<\r><\n>
    */

    if (rc == CME_SUCC)
    {
        if (pCmiConf->cid < CMI_PS_CID_NUM) //CID VALID
        {
            snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "\r\n+CGPADDR: %d", pCmiConf->cid);

            if (pCmiConf->ipv4Addr.addrType == CMI_IPV4_ADDR)
            {
#ifdef  FEATURE_REF_AT_ENABLE
                snprintf(tmpStrBuf, 80, ",%d.%d.%d.%d",
                         pCmiConf->ipv4Addr.addrData[0], pCmiConf->ipv4Addr.addrData[1],
                         pCmiConf->ipv4Addr.addrData[2], pCmiConf->ipv4Addr.addrData[3]);

#else
                snprintf(tmpStrBuf, 80, ",\"%d.%d.%d.%d\"",
                         pCmiConf->ipv4Addr.addrData[0], pCmiConf->ipv4Addr.addrData[1],
                         pCmiConf->ipv4Addr.addrData[2], pCmiConf->ipv4Addr.addrData[3]);
#endif
                rspStrLen = strlen(pRspStr);
                OsaCheck(rspStrLen < ATEC_IND_RESP_128_STR_LEN, rspStrLen, ATEC_IND_RESP_128_STR_LEN, 0);
                strncat(pRspStr,
                        tmpStrBuf,
                        ATEC_IND_RESP_128_STR_LEN - rspStrLen - 1);
            }

            if (pCmiConf->ipv6Addr.addrType == CMI_FULL_IPV6_ADDR)
            {
#ifdef  FEATURE_REF_AT_ENABLE
                snprintf(tmpStrBuf, 80, ",%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d",
                         pCmiConf->ipv6Addr.addrData[0], pCmiConf->ipv6Addr.addrData[1],
                         pCmiConf->ipv6Addr.addrData[2], pCmiConf->ipv6Addr.addrData[3],
                         pCmiConf->ipv6Addr.addrData[4], pCmiConf->ipv6Addr.addrData[5],
                         pCmiConf->ipv6Addr.addrData[6], pCmiConf->ipv6Addr.addrData[7],
                         pCmiConf->ipv6Addr.addrData[8], pCmiConf->ipv6Addr.addrData[9],
                         pCmiConf->ipv6Addr.addrData[10], pCmiConf->ipv6Addr.addrData[11],
                         pCmiConf->ipv6Addr.addrData[12], pCmiConf->ipv6Addr.addrData[13],
                         pCmiConf->ipv6Addr.addrData[14], pCmiConf->ipv6Addr.addrData[15]);
#else

                snprintf(tmpStrBuf, 80, ",\"%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d\"",
                         pCmiConf->ipv6Addr.addrData[0], pCmiConf->ipv6Addr.addrData[1],
                         pCmiConf->ipv6Addr.addrData[2], pCmiConf->ipv6Addr.addrData[3],
                         pCmiConf->ipv6Addr.addrData[4], pCmiConf->ipv6Addr.addrData[5],
                         pCmiConf->ipv6Addr.addrData[6], pCmiConf->ipv6Addr.addrData[7],
                         pCmiConf->ipv6Addr.addrData[8], pCmiConf->ipv6Addr.addrData[9],
                         pCmiConf->ipv6Addr.addrData[10], pCmiConf->ipv6Addr.addrData[11],
                         pCmiConf->ipv6Addr.addrData[12], pCmiConf->ipv6Addr.addrData[13],
                         pCmiConf->ipv6Addr.addrData[14], pCmiConf->ipv6Addr.addrData[15]);
#endif
                rspStrLen = strlen(pRspStr);
                OsaCheck(rspStrLen < ATEC_IND_RESP_128_STR_LEN, rspStrLen, ATEC_IND_RESP_128_STR_LEN, 0);
                strncat(pRspStr,
                        tmpStrBuf,
                        ATEC_IND_RESP_128_STR_LEN - rspStrLen - 1);
            }
            //CMI_IPV6_INTERFACE_ADDR should not print out
        }

        if (pCmiConf->bContinue)
        {
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pRspStr);
        }
        else    // the last
        {
            /* Add ending \r\n */
            //rspStrLen = strlen(pRspStr);
            //OsaCheck(rspStrLen < ATEC_IND_RESP_128_STR_LEN, rspStrLen, ATEC_IND_RESP_128_STR_LEN, 0);
            //strncat(pRspStr, "\r\n", ATEC_IND_RESP_128_STR_LEN - 2 - 1);

            ret = atcReply(reqHandle, AT_RC_OK, 0, pRspStr);

            //ret = atcReply(reqHandle, AT_RC_OK, 0, PNULL);
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}


/******************************************************************************
 * atPsCGCMODGetConf
 * Description: AT+CGPADDR=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGPADDRGetCidCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    pRspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CHAR    pTmpStr[8] = {0};
    UINT16  idx = 0, tmpStrLen = 0;
    BOOL    bFirstBr = TRUE;
    CmiPsGetAllBearersCidsInfoCnf   *pCmiConf = (CmiPsGetAllBearersCidsInfoCnf *)paras;

    /*
     * +CGPADDR: (list of defined <cid>s)
    */
    if (rc == CME_SUCC)
    {
        /*
         * if not defined CID, just return:
         * +CGPADDR: \r\n
         *  \r\nOK\r\n
        */
        if(pCmiConf->validNum > 0)
        {
            snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "+CGPADDR: ");
        }
        for (idx = 0; idx < pCmiConf->validNum && idx < CMI_PS_CID_NUM; idx++)
        {
            if (pCmiConf->basicInfoList[idx].bearerState != CMI_PS_BEARER_INVALID)
            {
                if (bFirstBr)
                {
                    snprintf(pTmpStr, 8, "(%d", pCmiConf->basicInfoList[idx].cid);
                    bFirstBr = FALSE;
                }
                else
                {
                    snprintf(pTmpStr, 8, ",%d", pCmiConf->basicInfoList[idx].cid);
                }

                tmpStrLen = strlen(pRspStr);
                OsaCheck(ATEC_IND_RESP_128_STR_LEN > tmpStrLen, ATEC_IND_RESP_128_STR_LEN, tmpStrLen, 0);
                strncat(pRspStr,
                        pTmpStr,
                        (ATEC_IND_RESP_128_STR_LEN-tmpStrLen-1));
            }
        }

        if (bFirstBr != TRUE)
        {
            tmpStrLen = strlen(pRspStr);
            OsaCheck(ATEC_IND_RESP_128_STR_LEN > tmpStrLen, ATEC_IND_RESP_128_STR_LEN, tmpStrLen, 0);
            strncat(pRspStr,
                    ")",
                    (ATEC_IND_RESP_128_STR_LEN-tmpStrLen-1));
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, pRspStr);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atPsCGATTSetConf
 * Description: Process CMI cnf msg of AT+CGATT=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGATTSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atPsCGATTGetConf
 * Description: Process CMI cnf msg of AT+CGATT?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGATTGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[16];
    CmiPsGetAttachStateCnf *Conf = (CmiPsGetAttachStateCnf *)paras;
    memset(RspBuf, 0, sizeof(RspBuf));


    if(rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+CGATT: %d", Conf->state);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atPsCGACTSetConf
 * Description: AT+CGACT=STATE,CID
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGACTSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atPsCGACTSetConf
 * Description: AT+CGACT=STATE,CID
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGDATASetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;

    if(rc == CME_SUCC)
    {
        ret = atcReply(reqHandle, AT_RC_OK, 0, PNULL);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atPsCGACTGetConf
 * Description: AT+CGACT?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGACTGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    pRspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CHAR    pTmpStr[16] = {0};

    CmiPsGetAllBearersCidsInfoCnf   *pCmiConf = (CmiPsGetAllBearersCidsInfoCnf *)paras;
    UINT16  idx = 0, tmpStrLen = 0;
    UINT8   state = 0;

    /*
     * [+CGACT: <cid>,<state>]
     * [<CR><LF>+CGACT: <cid>,<state>[...]]
    */

    if (rc == CME_SUCC)
    {
        for(idx = 0; idx < pCmiConf->validNum && idx < CMI_PS_CID_NUM; idx++)
        {
            if ((pCmiConf->basicInfoList[idx].bearerState > CMI_PS_BEARER_INVALID) &&
                (pCmiConf->basicInfoList[idx].bearerState < CMI_PS_BEARER_MAX_STATE))
            {
                if ((pCmiConf->basicInfoList[idx].bearerState == CMI_PS_BEARER_DEFINED) ||
                    (pCmiConf->basicInfoList[idx].bearerState == CMI_PS_BEARER_ACTIVATING) ||
                    (pCmiConf->basicInfoList[idx].bearerState == CMI_PS_BEARER_DEACTIVATING))
                {
                    state = 0;
                }
                else
                {
                    state = 1;
                }

                if (pRspStr[0] == '\0')
                {
                    snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "+CGACT: %d,%d", pCmiConf->basicInfoList[idx].cid, state);
                }
                else
                {
                    snprintf(pTmpStr, 16, "\r\n+CGACT: %d,%d", pCmiConf->basicInfoList[idx].cid, state);

                    tmpStrLen = strlen(pRspStr);
                    OsaCheck(ATEC_IND_RESP_128_STR_LEN > tmpStrLen, ATEC_IND_RESP_128_STR_LEN, tmpStrLen, 0);
                    strncat(pRspStr,
                            pTmpStr,
                            (ATEC_IND_RESP_128_STR_LEN-tmpStrLen-1));
                }
            }
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, pRspStr);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * atPsCGCMODModConf
 * Description: AT+CGCMOD=CID
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGCMODModCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atPsCGCMODGetConf
 * Description: AT+CGCMOD=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGCMODGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    pRspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CHAR    pTmpStr[8] = {0};
    UINT16  idx = 0, tmpStrLen = 0;
    BOOL    bFirstBr = TRUE;
    CmiPsGetAllBearersCidsInfoCnf   *pCmiConf = (CmiPsGetAllBearersCidsInfoCnf *)paras;

    /*
     * +CGCMOD: (list of <cid>s associated with active contexts)
     * if not activate bearer, return: +CGCMOD:
    */
    if (rc == CME_SUCC)
    {
        if(pCmiConf->validNum > 0)
        {
            snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "+CGCMOD: ");
        }

        for (idx = 0; idx < pCmiConf->validNum && idx < CMI_PS_CID_NUM; idx++)
        {
            if (pCmiConf->basicInfoList[idx].bearerState == CMI_PS_BEARER_ACTIVATED)
            {
                if (bFirstBr)
                {
                    snprintf(pTmpStr, 8, "(%d", pCmiConf->basicInfoList[idx].cid);
                    bFirstBr = FALSE;
                }
                else
                {
                    snprintf(pTmpStr, 8, ",%d", pCmiConf->basicInfoList[idx].cid);
                }

                tmpStrLen = strlen(pRspStr);
                OsaCheck(ATEC_IND_RESP_128_STR_LEN > tmpStrLen, ATEC_IND_RESP_128_STR_LEN, tmpStrLen, 0);
                strncat(pRspStr,
                        pTmpStr,
                        (ATEC_IND_RESP_128_STR_LEN-tmpStrLen-1));
            }
        }

        if (bFirstBr != TRUE)
        {
            tmpStrLen = strlen(pRspStr);
            OsaCheck(ATEC_IND_RESP_128_STR_LEN > tmpStrLen, ATEC_IND_RESP_128_STR_LEN, tmpStrLen, 0);
            strncat(pRspStr,
                    ")",
                    (ATEC_IND_RESP_128_STR_LEN-tmpStrLen-1));
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, pRspStr);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, PNULL);
    }

    return ret;
}


/******************************************************************************
 * psCEREGGetCnf
 * Description: Process CMI cnf msg of AT+CEREG?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCEREGGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
#define ATEC_CEREG_GET_RESP_STR_LEN         100
#define ATEC_CEREG_GET_RESP_TEMP_STR_LEN    30

    CmsRetId ret = CMS_FAIL;
    CHAR     rspBuf[ATEC_CEREG_GET_RESP_STR_LEN], tempBuf[ATEC_CEREG_GET_RESP_TEMP_STR_LEN];
    UINT8    atChanId = AT_GET_HANDLER_CHAN_ID(reqHandle);
    UINT32   ceregRptMode = mwGetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CEREG_RPT_CFG);
    CmiPsGetCeregCnf *pCeregConf = (CmiPsGetCeregCnf *)paras;

    memset(rspBuf, 0, sizeof(rspBuf));

    if (ceregRptMode > CMI_PS_CEREG_LOC_PSM_INFO_EMM_CAUSE)
    {
        OsaDebugBegin(FALSE, ceregRptMode, CMI_PS_CEREG_LOC_PSM_INFO_EMM_CAUSE, 0);
        OsaDebugEnd();
        ceregRptMode = CMI_PS_CEREG_LOC_PSM_INFO_EMM_CAUSE;
    }

    /*
     * AT+CEREG?
     * when <n>=0, 1, 2 or 3 and command successful:
     *   +CEREG: <n>,<stat>[,[<tac>],[<ci>],[<AcT>[,<cause_type>,<reject_cause>]]]
     *   a> Location information elements <tac>, <ci> and <AcT>, if available, are returned
     *      only when <n>=2 and MT is registered in the network;
     *   b  The parameters[,<cause_type>,<reject_cause>], if available, are returned when <n>=3.
     * when <n>=4 or 5 and command successful:
     *   +CEREG: <n>,<stat>[,[<tac>],[<ci>],[<AcT>][,[<cause_type>],[<reject_cause>][,[<ActiveTime>],[<Periodic-TAU>]]]]
    */

    if (rc == CME_SUCC)
    {
#ifdef  FEATURE_REF_AT_ENABLE
        /*+CEREG: <n>,<stat>*/
        snprintf(rspBuf, ATEC_CEREG_GET_RESP_STR_LEN, "+CEREG: %d,%d", ceregRptMode, pCeregConf->state);

        /*,[<tac>],[<ci>],[<AcT>*/
        if (ceregRptMode >= CMI_PS_CEREG_LOC_INFO)
        {
            //For Ref AT: if locPresent is false, fill tac&cellId with all zero
            memset(tempBuf, 0, sizeof(tempBuf));
            snprintf(tempBuf, ATEC_CEREG_GET_RESP_TEMP_STR_LEN, ",%04X,%08X,%d",
                     pCeregConf->tac, pCeregConf->celId, pCeregConf->act);

            strcat(rspBuf, tempBuf);
        }

        /*<cause_type>,<reject_cause>*/
        //For Ref AT: only n=3 or n=5, AT+CEREG? will return <cause_type>,<reject_cause>
        if (ceregRptMode == CMI_PS_CEREG_LOC_EMM_CAUSE ||
            ceregRptMode == CMI_PS_CEREG_LOC_PSM_INFO_EMM_CAUSE)
        {
            if (pCeregConf->rejCausePresent)
            {
                memset(tempBuf, 0, sizeof(tempBuf));

                snprintf(tempBuf, ATEC_CEREG_GET_RESP_TEMP_STR_LEN, ",%d,%d",
                         pCeregConf->causeType, pCeregConf->rejCause);

                strcat(rspBuf, tempBuf);
            }
            else
            {
                //For Ref AT: always need comma when n>=3 before <cause_type> and <reject_cause>
                strcat(rspBuf, ",,");  //<cause_type>,<reject_cause>
            }
        }

        if (ceregRptMode == CMI_PS_CEREG_LOC_PSM_INFO)
        {
            //For Ref AT: when n==4, fill <cause_type> and <reject_cause> with null
            strcat(rspBuf, ",,");
        }

        /*[<ActiveTime>],[<Periodic-TAU>]*/
        if (ceregRptMode >= CMI_PS_CEREG_LOC_PSM_INFO)
        {
            /*
             * check whether need to fill: <tac>,<ci>,<AcT>,<cause_type>,<reject_cause>
            */

            //For Ref AT: always need comma when n>=4 before <Active-Time>
            strcat(rspBuf, ",");

            if (pCeregConf->activeTimePresent)
            {
                memset(tempBuf, 0, sizeof(tempBuf));
                atByteToBitString((UINT8 *)tempBuf, pCeregConf->activeTime, 8);
                //For Ref AT: no need double quote
                strcat(rspBuf, tempBuf);
            }

            //For Ref AT: always need comma when n>=4 before <Periodic-Tau>
            strcat(rspBuf, ",");

            if (pCeregConf->extTauTimePresent)
            {
                memset(tempBuf, 0, sizeof(tempBuf));
                atByteToBitString((UINT8 *)tempBuf, pCeregConf->extPeriodicTau, 8);
                //For Ref AT: no need double quote
                strcat(rspBuf, tempBuf);
            }
        }
#else
        /*+CEREG: <n>,<stat>*/
        snprintf(rspBuf, ATEC_CEREG_GET_RESP_STR_LEN, "+CEREG: %d,%d", ceregRptMode, pCeregConf->state);

        /*,[<tac>],[<ci>],[<AcT>*/
        if (ceregRptMode >= CMI_PS_CEREG_LOC_INFO)
        {
            if (pCeregConf->locPresent)
            {
                memset(tempBuf, 0, sizeof(tempBuf));
                snprintf(tempBuf, ATEC_CEREG_GET_RESP_TEMP_STR_LEN, ",\"%04X\",\"%08X\",%d",
                         pCeregConf->tac, pCeregConf->celId, pCeregConf->act);

                strcat(rspBuf, tempBuf);
            }
        }

        /*<cause_type>,<reject_cause>*/
        if (ceregRptMode >= CMI_PS_CEREG_LOC_EMM_CAUSE)
        {
            if (pCeregConf->rejCausePresent)
            {
                memset(tempBuf, 0, sizeof(tempBuf));

                if (pCeregConf->locPresent == FALSE)    // need fill empty: <tac>],[<ci>],[<AcT>]
                {
                    strcat(rspBuf, ",,,");
                }
                snprintf(tempBuf, ATEC_CEREG_GET_RESP_TEMP_STR_LEN, ",%d,%d",
                         pCeregConf->causeType, pCeregConf->rejCause);

                strcat(rspBuf, tempBuf);
            }
        }

        /*[<ActiveTime>],[<Periodic-TAU>]*/
        if (ceregRptMode >= CMI_PS_CEREG_LOC_PSM_INFO)
        {
            if (pCeregConf->activeTimePresent || pCeregConf->extTauTimePresent)
            {
                /*
                 * check whether need to fill: <tac>,<ci>,<AcT>,<cause_type>,<reject_cause>
                */
                if (pCeregConf->rejCausePresent == FALSE)
                {
                    if (pCeregConf->locPresent == FALSE)    //no such case
                    {
                        OsaDebugBegin(FALSE, pCeregConf->act, pCeregConf->activeTime, pCeregConf->extPeriodicTau);
                        OsaDebugEnd();
                        strcat(rspBuf, ",,,");  //<tac>,<ci>,<AcT>
                    }

                    strcat(rspBuf, ",,");  //<cause_type>,<reject_cause>
                }

                if (pCeregConf->activeTimePresent)
                {
                    memset(tempBuf, 0, sizeof(tempBuf));
                    atByteToBitString((UINT8 *)tempBuf, pCeregConf->activeTime, 8);
                    strcat(rspBuf, ",\"");
                    strcat(rspBuf, tempBuf);
                    strcat(rspBuf, "\"");
                }
                else
                {
                    strcat(rspBuf, ",");  //<Active-Time>
                }

                if (pCeregConf->extTauTimePresent)
                {
                    memset(tempBuf, 0, sizeof(tempBuf));
                    atByteToBitString((UINT8 *)tempBuf, pCeregConf->extPeriodicTau, 8);
                    strcat(rspBuf, ",\"");
                    strcat(rspBuf, tempBuf);
                    strcat(rspBuf, "\"");
                }
            }
        }
#endif
        OsaCheck(strlen(rspBuf) < ATEC_CEREG_GET_RESP_STR_LEN, strlen(rspBuf), ATEC_CEREG_GET_RESP_STR_LEN, 0);

        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)rspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * atPsCEREGGetCapaConf
 * Description: Process CMI cnf msg of AT+CEREG=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCEREGGetCapaCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32] = {0};
    CHAR tempBuf[4] = {0};
    CmiPsGetCeregCapCnf *Conf = (CmiPsGetCeregCapCnf *)paras;
    UINT8 bitmap = Conf->bBitMap;
    UINT8 i;
    BOOL first=TRUE;

    memset(RspBuf, 0, sizeof(RspBuf));

    if (rc == CME_SUCC)
    {
        /*+CEREG: (list of supported <n>s)*/
        sprintf(RspBuf, "+CEREG: (");

        for(i = 0; i < 8; i++)
        {
            memset(tempBuf, 0, sizeof(tempBuf));
            if ((bitmap >> i)&0x01)
            {
                if(first)
                {
                    sprintf(tempBuf, "%d", i);
                    first = FALSE;
                }
                else
                {
                    sprintf(tempBuf, ",%d", i);
                }
            }

            strcat(RspBuf, tempBuf);
        }

        strcat(RspBuf, ")");

        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * atPsCGCONTRDPReadCnf
 * Description: Process CMI cnf msg of AT+CGCONTRDP=
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGCONTRDPReadCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    int addrType,i;
    char* RspBuf=PNULL;
    char* tempBuf=PNULL;
    char *dnsBuf=PNULL;     //    char dnsBuf[CMI_PDN_MAX_NW_ADDR_NUM][PSIL_PS_PDP_IP_V6_SIZE];
    UINT16 tempBufLen = 0;
    UINT16 fixHdrIdx = 0;
    bool dualstack = FALSE;
    int tmpAddrType = CMI_INVALID_IP_ADDR;


/*
[+CGCONTRDP: <cid>,<bearer_id>,<apn>[,<local_addr andsubnet_mask>[,<gw_addr>[,<DNS_prim_addr>[,<DNS_sec_addr>[,<PCSCF_prim_addr>
           [,<PCSCF_sec_addr>[,<IM_CN_Signalling_Flag>[,<LIPA_indication>[,<IPv4_MTU>[,<WLAN_Offload>[,<Local_Addr_Ind>[,<NonIP_MTU>
           [,<Serving_PLMN_rate_control_value>[,<Reliable_Data_Service]]]]]]]]]]]]]]
]
[<CR><LF>+CGCONTRDP: <cid>,<bearer_id>,<apn>[,<local_addr andsubnet_mask>[,<gw_addr>[,<DNS_prim_addr>[,<DNS_sec_addr>[,<PCSCF_prim_addr>
           [,<PCSCF_sec_addr>[,<IM_CN_Signalling_Flag>[,<LIPA_indication>[,<IPv4_MTU>[,<WLAN_Offload>[,<Local_Addr_Ind>[,<NonIP_MTU>
           [,<Serving_PLMN_rate_control_value>[,<Reliable_Data_Service]]]]]]]]]]]]]]
]
[...]
*/

    CmiPsReadBearerDynCtxParamCnf *conf = (CmiPsReadBearerDynCtxParamCnf *)paras;

    RspBuf = (char*)malloc(ATEC_CGCONTRDP_RESP_BUF_SIZE);
    OsaCheck( RspBuf!= PNULL, 0, 0, 0);

    tempBufLen = 256;
    tempBuf = (char*)malloc(tempBufLen);
    OsaCheck( tempBuf!= PNULL, 0, 0, 0);


    memset(RspBuf, 0, ATEC_CGCONTRDP_RESP_BUF_SIZE);
    memset(tempBuf, 0, tempBufLen);

    if (rc == CME_SUCC)
    {
        if(conf->bearerCtxPresent)
        {
            /* cid,*/
            snprintf(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\r\n+CGCONTRDP: %d,", conf->ctxDynPara.cid);

            /* bid, */
            snprintf(tempBuf, tempBufLen, "%d,", conf->ctxDynPara.bid);
            atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));

            /* APN, */
            OsaDebugBegin(conf->ctxDynPara.apnLength <= CMI_PS_MAX_APN_LEN, conf->ctxDynPara.apnLength, CMI_PS_MAX_APN_LEN, 0);
            conf->ctxDynPara.apnLength = CMI_PS_MAX_APN_LEN;
            OsaDebugEnd();

            if (conf->ctxDynPara.apnLength > 0)
            {
                /* ," */
                atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);

                /* apn */
                atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, (CHAR *)(conf->ctxDynPara.apnStr), conf->ctxDynPara.apnLength);

                /* ", */
                atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\",", 2);
            }
            else
            {
                atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
            }

            fixHdrIdx = strlen(RspBuf);


            /*DNS pre handle*/
            if (conf->ctxDynPara.ipv4Addr.addrType == CMI_IPV4_ADDR &&
                conf->ctxDynPara.ipv6Addr.addrType == CMI_FULL_IPV6_ADDR)  //only IPv6 full IP need to print
            {
                dualstack = TRUE;
            }

            if(conf->ctxDynPara.dnsAddrNum > 0)
            {
                dnsBuf =  (char*)malloc(conf->ctxDynPara.dnsAddrNum * PSIL_PS_PDP_IP_V6_SIZE);
				OsaCheck( dnsBuf!= PNULL, 0, 0, 0);
				memset(dnsBuf,0x0,conf->ctxDynPara.dnsAddrNum * PSIL_PS_PDP_IP_V6_SIZE);
            }



            if (!dualstack)
            {
                if(conf->ctxDynPara.ipv4Addr.addrType == CMI_IPV4_ADDR)
                {
                    tmpAddrType = conf->ctxDynPara.ipv4Addr.addrType;
                }
                else if(conf->ctxDynPara.ipv6Addr.addrType == CMI_FULL_IPV6_ADDR)
                {
                    tmpAddrType = conf->ctxDynPara.ipv6Addr.addrType;
                }

            }
            for(i=0; i<conf->ctxDynPara.dnsAddrNum; i++)
            {
                 addrType = conf->ctxDynPara.dnsAddr[i].addrType;

                 if(addrType != CMI_INVALID_IP_ADDR && addrType != CMI_IPV6_INTERFACE_ADDR)
                 {
                     if(!dualstack && tmpAddrType != CMI_INVALID_IP_ADDR)
                     {
                         if((tmpAddrType == CMI_IPV4_ADDR && addrType != CMI_IPV4_ADDR)
                             || (tmpAddrType == CMI_FULL_IPV6_ADDR && addrType != CMI_FULL_IPV6_ADDR))
                             continue;
                     }

                     psUint8IpAddrToStr(addrType, conf->ctxDynPara.dnsAddr[i].addrData, &(dnsBuf[i*PSIL_PS_PDP_IP_V6_SIZE]));


                 }
             }

            if(!dualstack)
            {
                /*local_addr and subnet_mask*/
                memset(tempBuf, 0, tempBufLen);

                if(conf->ctxDynPara.ipv4Addr.addrType == CMI_IPV4_ADDR)
                {

                    if (psBeAllZero(conf->ctxDynPara.ipv4Addr.addrData, CMI_MAX_IP_ADDR_LEN) == FALSE) //IPV4 address valid
                    {
                        /* \" */
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);

                        /* ip addr */
                        psUint8IpAddrToStr(conf->ctxDynPara.ipv4Addr.addrType, conf->ctxDynPara.ipv4Addr.addrData, tempBuf);
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));

                        /*subnet mask*/
                        if(conf->ctxDynPara.ipv4Addr.subnetLength != 0)
                        {
                            memset(tempBuf, 0, tempBufLen);

                            atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ".", 1);

                            psSetNetMaskStr(conf->ctxDynPara.ipv4Addr.addrType, conf->ctxDynPara.ipv4Addr.subnetLength, tempBuf);
                            atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf,strlen(tempBuf));
                        }

                        /*\"*/
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);
                    }
                    else
                    {
                        OsaDebugBegin(FALSE, conf->ctxDynPara.ipv4Addr.addrData[0], conf->ctxDynPara.ipv4Addr.addrData[1], conf->ctxDynPara.ipv4Addr.addrType);
                        OsaDebugEnd();
                    }

                }
                else if(conf->ctxDynPara.ipv6Addr.addrType == CMI_FULL_IPV6_ADDR)
                {

                    if (psBeAllZero(conf->ctxDynPara.ipv6Addr.addrData, CMI_MAX_IP_ADDR_LEN) == FALSE)
                    {
                        /* \" */
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);

                        /* ip addr */
                        psUint8IpAddrToStr(conf->ctxDynPara.ipv6Addr.addrType, conf->ctxDynPara.ipv6Addr.addrData, tempBuf);
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));

                        /*subnet mask*/
                        if(conf->ctxDynPara.ipv6Addr.subnetLength != 0)
                        {
                            memset(tempBuf, 0, tempBufLen);

                            psSetNetMaskStr(conf->ctxDynPara.ipv6Addr.addrType, conf->ctxDynPara.ipv6Addr.subnetLength, tempBuf);
                            atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ".",1);
                            atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));
                        }
                        /*\"*/
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);
                    }
                    else
                    {
                        OsaDebugBegin(FALSE, conf->ctxDynPara.ipv6Addr.addrData[0], conf->ctxDynPara.ipv6Addr.addrData[1], conf->ctxDynPara.ipv6Addr.addrType);
                        OsaDebugEnd();
                    }

                }
                else if(conf->ctxDynPara.ipv6Addr.addrType == CMI_IPV6_INTERFACE_ADDR)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_EXEC, psCGCONTRDPReadCnf_1, P_WARNING, 1,
                            "AT PS, CID: %d, only ipv6 interface address, not print out", conf->ctxDynPara.cid);
                }
                atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);

                /*gw_addr*/
                if(conf->ctxDynPara.gwIpv4Addr.addrType  == CMI_IPV4_ADDR )
                {
                    if (psBeAllZero(conf->ctxDynPara.gwIpv4Addr.addrData, CMI_MAX_IP_ADDR_LEN) == FALSE) //IPV4 address valid
                    {
                        /* \" */
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);

                        /* ip addr */
                        psUint8IpAddrToStr(conf->ctxDynPara.gwIpv4Addr.addrType, conf->ctxDynPara.gwIpv4Addr.addrData, tempBuf);
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));

                        /*\"*/
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                    }
                    else
                    {
                        OsaDebugBegin(FALSE, conf->ctxDynPara.gwIpv4Addr.addrData[0], conf->ctxDynPara.gwIpv4Addr.addrData[1], conf->ctxDynPara.gwIpv4Addr.addrType);
                        OsaDebugEnd();
                    }
                }
                else
                {
                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                }

                if(conf->ctxDynPara.dnsAddr[0].addrType != CMI_INVALID_IP_ADDR)
                {

                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);
                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, &(dnsBuf[0]), strlen(&(dnsBuf[0])));
                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\",", 2);
                }
                else
                {
                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                }
                /*DNS2*/
                if(conf->ctxDynPara.dnsAddr[1].addrType != CMI_INVALID_IP_ADDR)
                {

                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);
                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, &(dnsBuf[PSIL_PS_PDP_IP_V6_SIZE]), strlen(&(dnsBuf[PSIL_PS_PDP_IP_V6_SIZE])));
                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\",", 2);
                }
                else
                {
                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                }

                /*pcscf 1,2  no need*/
                atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",,", 2);

                /*im_cn*/
                atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                /*LIPA*/
                atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                /*IPv4_MTU*/
                if(conf->ctxDynPara.ipv4MtuPresent == TRUE)
                {
                    snprintf(tempBuf, tempBufLen, "%d,", conf->ctxDynPara.ipv4Mtu);
                    atStrnCat((char *)RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));
                }
                else
                {
                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                }
                /*WLAN_Offload*/
                atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                /*Local_Addr_Ind*/
                atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                /*NonIP_MTU*/
                if(conf->ctxDynPara.nonIpMtuPresent == TRUE)
                {
                    snprintf (tempBuf, tempBufLen, "%d,", conf->ctxDynPara.nonIpMtu);
                    atStrnCat((char *)RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));
                }
                else
                {
                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                }

                /*clean all ',' behind the last valid value*/
                for(i=ATEC_IND_RESP_256_STR_LEN;i>0;i--)
                {
                    if(RspBuf[i] == ',')
                    {
                        RspBuf[i] = 0;
                    }
                    else if(RspBuf[i] != 0)
                    {
                        break;
                    }
                }

                if (conf->bContBearer)
                {
                    ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, RspBuf);
                }
                else
                {   //the last
                    ret = atcReply(reqHandle, AT_RC_OK, 0, RspBuf);
                }


            }
            else
            {
                int count = 0;

                /*local_addr and subnet_mask*/
                memset(tempBuf, 0, tempBufLen);

                if(conf->ctxDynPara.ipv4Addr.addrType == CMI_IPV4_ADDR)
                {

                    if (psBeAllZero(conf->ctxDynPara.ipv4Addr.addrData, CMI_MAX_IP_ADDR_LEN) == FALSE) //IPV4 address valid
                    {
                        /* \" */
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);

                        /* ip addr */
                        psUint8IpAddrToStr(conf->ctxDynPara.ipv4Addr.addrType, conf->ctxDynPara.ipv4Addr.addrData, tempBuf);
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));

                        /*subnet mask*/
                        if(conf->ctxDynPara.ipv4Addr.subnetLength != 0)
                        {
                            memset(tempBuf, 0, tempBufLen);

                            atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ".", 1);

                            psSetNetMaskStr(conf->ctxDynPara.ipv4Addr.addrType, conf->ctxDynPara.ipv4Addr.subnetLength, tempBuf);
                            atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf,strlen(tempBuf));
                        }

                        /*\"*/
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);
                    }
                    else
                    {
                        OsaDebugBegin(FALSE, conf->ctxDynPara.ipv4Addr.addrData[0], conf->ctxDynPara.ipv4Addr.addrData[1], conf->ctxDynPara.ipv4Addr.addrType);
                        OsaDebugEnd();
                    }

                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_EXEC, psCGCONTRDPReadCnf_2, P_WARNING, 1,
                            "AT PS, ipv4 addr should not be NULL :%d", conf->ctxDynPara.ipv4Addr.addrType);
                }
                atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                /*gw_addr*/
                if(conf->ctxDynPara.gwIpv4Addr.addrType  == CMI_IPV4_ADDR )
                {
                    if (psBeAllZero(conf->ctxDynPara.gwIpv4Addr.addrData, CMI_MAX_IP_ADDR_LEN) == FALSE) //IPV4 address valid
                    {
                        /* \" */
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);

                        /* ip addr */
                        psUint8IpAddrToStr(conf->ctxDynPara.gwIpv4Addr.addrType, conf->ctxDynPara.gwIpv4Addr.addrData, tempBuf);
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));

                        /*\"*/
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                    }
                    else
                    {
                        OsaDebugBegin(FALSE, conf->ctxDynPara.gwIpv4Addr.addrData[0], conf->ctxDynPara.gwIpv4Addr.addrData[1], conf->ctxDynPara.gwIpv4Addr.addrType);
                        OsaDebugEnd();
                    }
                }
                else
                {
                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                }

                for(i = 0; i < conf->ctxDynPara.dnsAddrNum; i++)
                {
                    if(conf->ctxDynPara.dnsAddr[i].addrType == CMI_IPV4_ADDR)
                    {
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, &(dnsBuf[i*PSIL_PS_PDP_IP_V6_SIZE]),strlen(&(dnsBuf[i*PSIL_PS_PDP_IP_V6_SIZE])));
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\",", 2);
                        count++;
                        if(count == 2)
                            break;
                    }
                }
                if(count == 0)
                {
                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",,", 2);
                }
                else if(count == 1)
                {
                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                }


                 /*pcscf 1&2 no need*/
                 atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",,", 2);

                /*im_cn*/
                 atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                 /*LIPA*/
                 atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                 /*IPv4_MTU*/
                 if(conf->ctxDynPara.ipv4MtuPresent == TRUE)
                 {
                     snprintf (tempBuf, tempBufLen, "%d,", conf->ctxDynPara.ipv4Mtu);
                     atStrnCat((char *)RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));
                 }
                 else
                 {
                     atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                 }
                 /*WLAN_Offload*/
                 strcat((char *)RspBuf,",");
                 /*Local_Addr_Ind*/
                 strcat((char *)RspBuf,",");
                 /*NonIP_MTU*/
                 if(conf->ctxDynPara.nonIpMtuPresent == TRUE)
                 {
                     snprintf (tempBuf, tempBufLen, "%d,", conf->ctxDynPara.nonIpMtu);
                     atStrnCat((char *)RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));
                 }
                 else
                 {
                     atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                 }

                 /*clean all ',' behind the last valid value*/
                 for(i=ATEC_IND_RESP_256_STR_LEN;i>0;i--)
                 {
                     if(RspBuf[i] == ',')
                     {
                         RspBuf[i] = 0;
                     }
                     else if(RspBuf[i] != 0)
                     {
                         break;
                     }
                 }

                ret = atcReply( reqHandle,AT_RC_CONTINUE,0,(char *)RspBuf);

               /*IPV6*/
                memset(RspBuf+fixHdrIdx,0,ATEC_CGCONTRDP_RESP_BUF_SIZE-fixHdrIdx);

                if(conf->ctxDynPara.ipv6Addr.addrType == CMI_FULL_IPV6_ADDR)
                {

                    if (psBeAllZero(conf->ctxDynPara.ipv6Addr.addrData, CMI_MAX_IP_ADDR_LEN) == FALSE)
                    {
                        /* \" */
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);

                        /* ip addr */
                        psUint8IpAddrToStr(conf->ctxDynPara.ipv6Addr.addrType, conf->ctxDynPara.ipv6Addr.addrData, tempBuf);
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));

                        /*subnet mask*/
                        if(conf->ctxDynPara.ipv6Addr.subnetLength != 0)
                        {
                            memset(tempBuf, 0, tempBufLen);
                            psSetNetMaskStr(conf->ctxDynPara.ipv6Addr.addrType, conf->ctxDynPara.ipv6Addr.subnetLength, tempBuf);
                            atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ".",1);
                            atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));
                        }

                        /*\"*/
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);
                    }
                    else
                    {
                        OsaDebugBegin(FALSE, conf->ctxDynPara.ipv6Addr.addrData[0], conf->ctxDynPara.ipv6Addr.addrData[1], conf->ctxDynPara.ipv6Addr.addrType);
                        OsaDebugEnd();
                    }

                }
                else
                {
                    ECOMM_TRACE(UNILOG_ATCMD_EXEC, psCGCONTRDPReadCnf_3, P_WARNING, 1,
                            "AT PS, ipv6 addr should not be NULL :%d", conf->ctxDynPara.ipv4Addr.addrType);
                }

                /*gw_addr -- no need*/
                atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);

                /*DNS V6*/
                atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",",1);
                count = 0;
                for(i = 0; i < conf->ctxDynPara.dnsAddrNum; i++)
                {
                    if(conf->ctxDynPara.dnsAddr[i].addrType == CMI_FULL_IPV6_ADDR)
                    {
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\"", 1);
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, &(dnsBuf[i*PSIL_PS_PDP_IP_V6_SIZE]), strlen(&(dnsBuf[i*PSIL_PS_PDP_IP_V6_SIZE])));
                        atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, "\",", 2);
                        count++;
                        if(count == 2)
                            break;
                    }
                }
                if(count == 0)
                {
                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",,", 2);
                }
                else if(count == 1)
                {
                    atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                }

                 /*pcscf 1&2 no need*/
                 atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",,", 2);

                /*im_cn*/
                 atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                 /*LIPA*/
                 atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                 /*IPv4_MTU*/
                 if(conf->ctxDynPara.ipv4MtuPresent == TRUE)
                 {
                     snprintf (tempBuf, tempBufLen, "%d,", conf->ctxDynPara.ipv4Mtu);
                     atStrnCat((char *)RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));
                 }
                 else
                 {
                     atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                 }
                 /*WLAN_Offload*/
                 strcat((char *)RspBuf,",");
                 /*Local_Addr_Ind*/
                 strcat((char *)RspBuf,",");
                 /*NonIP_MTU*/
                 if(conf->ctxDynPara.nonIpMtuPresent == TRUE)
                 {
                     snprintf (tempBuf, tempBufLen, "%d,", conf->ctxDynPara.nonIpMtu);
                     atStrnCat((char *)RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, tempBuf, strlen(tempBuf));
                 }
                 else
                 {
                     atStrnCat(RspBuf, ATEC_CGCONTRDP_RESP_BUF_SIZE, ",", 1);
                 }

                 /*clean all ',' behind the last valid value*/
                 for(i=ATEC_IND_RESP_256_STR_LEN;i>0;i--)
                 {
                     if(RspBuf[i] == ',')
                     {
                         RspBuf[i] = 0;
                     }
                     else if(RspBuf[i] != 0)
                     {
                         break;
                     }
                 }

                if (conf->bContBearer)
                {
                    ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, RspBuf);
                }
                else
                {   //the last
                    ret = atcReply(reqHandle, AT_RC_OK, 0, RspBuf);
                }

            }

        }
        else
        {
            ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
        }

    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    if(tempBuf != PNULL)
    {
        OsaFreeMemory(&tempBuf);
    }

    if(RspBuf != PNULL)
    {
        OsaFreeMemory(&RspBuf);
    }

    if(dnsBuf != PNULL)
    {
        OsaFreeMemory(&dnsBuf);
    }

    return ret;
}

/******************************************************************************
 * atPsCGCONTRDPGetCnf
 * Description: Process CMI cnf msg of AT+CGCONTRDP=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGCONTRDPGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{

    CmsRetId ret = CMS_FAIL;
    CHAR    pRspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CHAR    pTmpStr[8] = {0};
    UINT16  idx = 0, tmpStrLen = 0;
    BOOL    bFirstBr = TRUE;
    CmiPsGetAllBearersCidsInfoCnf   *pCmiConf = (CmiPsGetAllBearersCidsInfoCnf *)paras;


    if(rc == CME_SUCC)
    {
        if(pCmiConf->validNum > 0)
        {
            snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "+CGCONTRDP: ");
        }
        for (idx = 0; idx < pCmiConf->validNum && idx < CMI_PS_CID_NUM; idx++)
        {
            if (pCmiConf->basicInfoList[idx].bearerState == CMI_PS_BEARER_ACTIVATED)
            {
                if (bFirstBr)
                {
                    snprintf(pTmpStr, 8, "(%d", pCmiConf->basicInfoList[idx].cid);
                    bFirstBr = FALSE;
                }
                else
                {
                    snprintf(pTmpStr, 8, ",%d", pCmiConf->basicInfoList[idx].cid);
                }

                tmpStrLen = strlen(pRspStr);
                OsaCheck(ATEC_IND_RESP_128_STR_LEN > tmpStrLen, ATEC_IND_RESP_128_STR_LEN, tmpStrLen, 0);
                strncat(pRspStr,
                        pTmpStr,
                        (ATEC_IND_RESP_128_STR_LEN-tmpStrLen-1));
            }
        }

        if (bFirstBr != TRUE)
        {
            tmpStrLen = strlen(pRspStr);
            OsaCheck(ATEC_IND_RESP_128_STR_LEN > tmpStrLen, ATEC_IND_RESP_128_STR_LEN, tmpStrLen, 0);
            strncat(pRspStr,
                    ")",
                    (ATEC_IND_RESP_128_STR_LEN-tmpStrLen-1));
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, pRspStr);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}



/******************************************************************************
 * atPsCGTFTSetCnf
 * Description: Process CMI cnf msg of AT+CGTFT=cid,...
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGTFTSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atPsCGTFTGetCnf
 * Description: Process CMI cnf msg of AT+CGTFT?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGTFTGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    char* RspBuf;
    CHAR tempbuf[20] = {'\0'};
    unsigned char* valData;
    char* ip6Buf;
    char* mask6Buf;
    UINT8 addrType, subnetLen;
    RspBuf = (char*)malloc(1024);
    ip6Buf = (char*)malloc(PSIL_PS_PDP_IP_V6_SIZE);
    mask6Buf = (char*)malloc(PSIL_PS_PDP_IP_V6_SIZE+10);

    CmiPsGetDefineTFTParamCnf *Conf = (CmiPsGetDefineTFTParamCnf *)paras;

    OsaCheck(RspBuf != NULL && ip6Buf != NULL && mask6Buf != NULL , RspBuf, ip6Buf, mask6Buf);

    memset(RspBuf, 0, 1024);
    memset(ip6Buf, 0, PSIL_PS_PDP_IP_V6_SIZE);
    memset(mask6Buf, 0, PSIL_PS_PDP_IP_V6_SIZE+10);


    if (rc == CME_SUCC)
    {
        for (int i = 0; i < Conf->numFilters; i++)
        {
            sprintf((char*)RspBuf, "\r\n+CGTFT: %d,", Conf->filters[i].cid);
            sprintf(tempbuf, "%d,%d,",Conf->filters[i].pfId, Conf->filters[i].epIndex);
            strcat((char*)RspBuf, (char*)tempbuf);
            strcat((char*)RspBuf, "\"");
            addrType = Conf->filters[i].remoteAddrAndMask.addrType;
            if (addrType != CMI_INVALID_IP_ADDR)
            {
                valData = Conf->filters[i].remoteAddrAndMask.addrData;
                psUint8IpAddrToStr(addrType, valData, ip6Buf);
                strcat((char*)RspBuf,(char*)ip6Buf);
                subnetLen = Conf->filters[i].remoteAddrAndMask.subnetLength;
                if(subnetLen>0)
                {
                    psSetNetMaskStr(addrType,subnetLen,mask6Buf);
                    if (addrType == CMI_IPV4_ADDR)
                    {
                        strcat((char*)RspBuf, ".");
                    }
                    else
                    {
                        strcat((char*)RspBuf, " ");
                    }
                    strcat((char*)RspBuf, (char*)mask6Buf);
                }
            }
            strcat((char*)RspBuf, "\",");
            if(!Conf->filters[i].pIdNextHdrPresent)
            {
                sprintf((char*)tempbuf, ",");
            }
            else
            {
                sprintf((char*)tempbuf, "%d,",Conf->filters[i].pIdNextHdr);
            }
            strcat((char*)RspBuf, (char*)tempbuf);

            if(!Conf->filters[i].srcPortRangePresent)
            {
                sprintf((char*)tempbuf, "\"\",");
            }
            else
            {
                sprintf((char*)tempbuf, "\"%d.%d\",",Conf->filters[i].srcPortRange.min,Conf->filters[i].srcPortRange.max);
            }
            strcat((char*)RspBuf, (char*)tempbuf);

            if(!Conf->filters[i].dstPortRangePresent)
            {
                sprintf((char*)tempbuf, "\"\",");
            }
            else
            {
                sprintf((char*)tempbuf, "\"%d.%d\",",Conf->filters[i].dstPortRange.min,Conf->filters[i].dstPortRange.max);
            }
            strcat((char*)RspBuf, (char*)tempbuf);

            if(!Conf->filters[i].ipSecSPIPresent)
            {
                sprintf((char*)tempbuf, ",");
            }
            else
            {
                sprintf((char*)tempbuf, "%x,",Conf->filters[i].ipSecSPI);
            }
            strcat((char*)RspBuf, (char*)tempbuf);

            if(!Conf->filters[i].tosPresent)
            {
                sprintf((char*)tempbuf, "\"\",");
            }
            else
            {
                sprintf((char*)tempbuf, "\"%d.%d\",",Conf->filters[i].tosTc, Conf->filters[i].tosTcMask);
            }
            strcat((char*)RspBuf, (char*)tempbuf);

            if(!Conf->filters[i].flowLabelPresent)
            {
                sprintf((char*)tempbuf, ",");
            }
            else
            {
                sprintf((char*)tempbuf, "%d,",Conf->filters[i].flowLabel);
            }
            strcat((char*)RspBuf, (char*)tempbuf);

            sprintf((char*)tempbuf, "%d", Conf->filters[i].direction);
            strcat((char*)RspBuf, (char*)tempbuf);

            /*
             * whether last reply
            */
            if (Conf->bContinue == TRUE)
            {
                ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, (char*)RspBuf);
            }
            else    //Conf->bContinue == FALSE
            {   if (i == (Conf->numFilters - 1))
                {
                    ret = atcReply(reqHandle, AT_RC_OK, 0, RspBuf);
                }
                else
                {
                    ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, (char*)RspBuf);
                }
            }

            memset(RspBuf, 0, 1024);
            memset(ip6Buf, 0, PSIL_PS_PDP_IP_V6_SIZE);
            memset(mask6Buf, 0, PSIL_PS_PDP_IP_V6_SIZE+10);
        }

        if (Conf->numFilters == 0 &&
            Conf->bContinue == FALSE)  //no TFT config
        {
            ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    free(RspBuf);
    free(ip6Buf);
    free(mask6Buf);
    return ret;
}

/******************************************************************************
 * atPsCGTFTDelCnf
 * Description: Process CMI cnf msg of AT+CGTFT=cid
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGTFTDelCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atPsCGEQOSSetCnf
 * Description: Process CMI cnf msg of AT+CGEQOS=cid,qci,...
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGEQOSSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atPsCGEQOSDelCnf
 * Description: Process CMI cnf msg of AT+CGEQOS=cid,qci,...
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGEQOSDelCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atPsCGEQOSGetCnf
 * Description: Process CMI cnf msg of AT+CGEQOS=cid,qci,...
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGEQOSGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;
    CHAR    rspBuf[ATEC_IND_RESP_64_STR_LEN] = {0};
    CHAR    tempbuf[ATEC_IND_RESP_64_STR_LEN] = {0};
    CmiPsGetDefinedEpsQoSCnf *pCMiCnf = (CmiPsGetDefinedEpsQoSCnf *)paras;

    if (rc == CME_SUCC)
    {
        if (pCMiCnf->epsQosValid==TRUE)
        {
            snprintf(rspBuf, ATEC_IND_RESP_64_STR_LEN, "\r\n+CGEQOS: %d,%d", pCMiCnf->epsQosParam.cid, pCMiCnf->epsQosParam.qci);

            if (pCMiCnf->epsQosParam.gbrMbrPresent)
            {

                snprintf(tempbuf, ATEC_IND_RESP_64_STR_LEN, ",%d,%d,%d,%d",
                         pCMiCnf->epsQosParam.dlGBR, pCMiCnf->epsQosParam.ulGBR, pCMiCnf->epsQosParam.dlMBR, pCMiCnf->epsQosParam.ulMBR);

                //strcat((char*)RspBuf, (char*)tempbuf);
                atStrnCat(rspBuf, ATEC_IND_RESP_64_STR_LEN, tempbuf, strlen(tempbuf));
            }
        }

        if (pCMiCnf->bContinue)
        {
            if (rspBuf[0] != 0)
            {
                ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, rspBuf);
            }
        }
        else
        {
            if (rspBuf[0] != 0)
            {
                ret = atcReply(reqHandle, AT_RC_OK, 0, rspBuf);
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
            }
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * psCGEQOSGetCapaCnf
 * Description: Process CMI cnf msg of AT+CGEQOS=?,...
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGEQOSGetCapaCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[60]={0};

    if(rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+CGEQOS: (1-11),(0,5,6,7,8,9,79)");

        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * atPsCSODCPSetCnf
 * Description: Process CMI cnf msg of AT+CSODCP=cid,cpdata_len,...
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCSODCPSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * psCGEQOSRDPGetCnf
 * Description: Process CMI cnf msg of AT+CGEQOSRDP=cid,...
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGEQOSRDPGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[64] = {0};
    CHAR tempbuf[32] = {0};
    CmiPsReadDynEpsQoSCnf *Conf = (CmiPsReadDynEpsQoSCnf *)paras;;

    if (rc == CME_SUCC)
    {
        if (Conf->epsQosValid==TRUE)
        {
            sprintf((char*)RspBuf, "\r\n+CGEQOSRDP: %d,%d,", Conf->epsQosDynParams.cid, Conf->epsQosDynParams.qci);
            if(Conf->epsQosDynParams.gbrPresent)
            {
                sprintf((char*)tempbuf, "%d,%d,", Conf->epsQosDynParams.dlGBR, Conf->epsQosDynParams.ulGBR);
                strcat((char*)RspBuf, (char*)tempbuf);
            }
            else
            {
                sprintf((char*)tempbuf, ",,");
                strcat((char*)RspBuf, (char*)tempbuf);
            }
            memset(tempbuf, 0, 32);

            if(Conf->epsQosDynParams.mbrPresent)
            {
                sprintf((char*)tempbuf, "%d,%d",Conf->epsQosDynParams.dlMBR, Conf->epsQosDynParams.ulMBR);
                strcat((char*)RspBuf, (char*)tempbuf);
            }
            else
            {
                sprintf((char*)tempbuf, ",,");
                strcat((char*)RspBuf, (char*)tempbuf);
            }
            memset(tempbuf, 0, 32);

            if(Conf->epsQosDynParams.ambrPresent)
            {
                sprintf((char*)tempbuf, "%d,%d", Conf->epsQosDynParams.dlAMBR, Conf->epsQosDynParams.ulAMBR);
                strcat((char*)RspBuf, (char*)tempbuf);
            }
            else
            {
                sprintf((char*)tempbuf, ",");
                strcat((char*)RspBuf, (char*)tempbuf);
            }
            for(int i=63;i>0;i--)
            {
                if(RspBuf[i] == ',')
                {
                    RspBuf[i] = 0;
                }
                else if(RspBuf[i] != 0)
                {
                    break;
                }
            }
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, (char*)RspBuf);

        }

        if (Conf->bContinue != TRUE)
        {
            /* Add ending \r\n */
            //memset((char *)RspBuf,0,32);
            //strcat((char *)RspBuf,"\r\n");
            //atcReply(reqHandle, AT_RC_CONTINUE, 0, RspBuf);
            ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
        }


    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * psCGEQOSRDPGetCidCnf
 * Description: AT+CGEQOSRDP=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGEQOSRDPGetCidCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    pRspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CHAR    pTmpStr[8] = {0};
    UINT16  idx = 0, tmpStrLen = 0;
    BOOL    bFirstBr = TRUE;
    CmiPsGetAllBearersCidsInfoCnf   *pCmiConf = (CmiPsGetAllBearersCidsInfoCnf *)paras;


    if(rc == CME_SUCC)
    {
        if(pCmiConf->validNum > 0)
        {
            snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "+CGEQOSRDP: ");
        }

        for (idx = 0; idx < pCmiConf->validNum && idx < CMI_PS_CID_NUM; idx++)
        {
            if (pCmiConf->basicInfoList[idx].bearerState == CMI_PS_BEARER_ACTIVATED)
            {
                if (bFirstBr)
                {
                    snprintf(pTmpStr, 8, "(%d", pCmiConf->basicInfoList[idx].cid);
                    bFirstBr = FALSE;
                }
                else
                {
                    snprintf(pTmpStr, 8, ",%d", pCmiConf->basicInfoList[idx].cid);
                }

                tmpStrLen = strlen(pRspStr);
                OsaCheck(ATEC_IND_RESP_128_STR_LEN > tmpStrLen, ATEC_IND_RESP_128_STR_LEN, tmpStrLen, 0);
                strncat(pRspStr,
                        pTmpStr,
                        (ATEC_IND_RESP_128_STR_LEN-tmpStrLen-1));
            }
        }

        if (bFirstBr != TRUE)
        {
            tmpStrLen = strlen(pRspStr);
            OsaCheck(ATEC_IND_RESP_128_STR_LEN > tmpStrLen, ATEC_IND_RESP_128_STR_LEN, tmpStrLen, 0);
            strncat(pRspStr,
                    ")",
                    (ATEC_IND_RESP_128_STR_LEN-tmpStrLen-1));
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, pRspStr);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atPsATTBEARSetCnf
 * Description: Process CMI cnf msg of AT+CSODCP=cid,cpdata_len,...
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psATTBEARSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atPsATTBEARGetCnf
 * Description: Process CMI cnf msg of AT+ECATTBEARER?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psATTBEARGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR rspStrBuf[ATEC_IND_RESP_128_STR_LEN] = {0};    //128 bytes
    CHAR tempBuf[CMI_PS_MAX_APN_LEN+1] = {0}; //65 bytes


    CmiPsGetAttachedBearerCtxCnf *pCmiCnf = (CmiPsGetAttachedBearerCtxCnf *)paras;

    if (rc == CME_SUCC)
    {
        /*
         * +ECATTBEARER: <pdnType>,<eitf>,<apnStr>,<ipv4allocType>,<NSLPI>,<ipv4Mtu>,<nonIpMtu>,<pcoAuthProtocol,<authUserName,authPassword>>
        */
        snprintf(rspStrBuf, ATEC_IND_RESP_128_STR_LEN, "+ECATTBEARER: %d,%d", pCmiCnf->attBearCtxParam.pdnType, pCmiCnf->attBearCtxParam.eitf);

        if (pCmiCnf->attBearCtxParam.apnLength > 0)
        {
            OsaDebugBegin(pCmiCnf->attBearCtxParam.apnLength <= CMI_PS_MAX_APN_LEN, pCmiCnf->attBearCtxParam.apnLength, CMI_PS_MAX_APN_LEN, 0);
            OsaDebugEnd();

            snprintf(tempBuf, CMI_PS_MAX_APN_LEN+1, "%s", pCmiCnf->attBearCtxParam.apnStr);

            snprintf(rspStrBuf, ATEC_IND_RESP_128_STR_LEN, "+ECATTBEARER: %d,%d,\"%s\",%d,%d,%d,%d,%d,\"%s\",\"%s\"",
                     pCmiCnf->attBearCtxParam.pdnType, pCmiCnf->attBearCtxParam.eitf, tempBuf,
                     pCmiCnf->attBearCtxParam.ipv4AlloType, pCmiCnf->attBearCtxParam.NSLPI,
                     pCmiCnf->attBearCtxParam.ipv4MtuDisByNas, pCmiCnf->attBearCtxParam.nonIpMtuDisByNas,
                     pCmiCnf->attBearCtxParam.authProtocol,pCmiCnf->attBearCtxParam.authUserName,pCmiCnf->attBearCtxParam.authPassword);
        }
        else    //no APN
        {
            snprintf(rspStrBuf, ATEC_IND_RESP_128_STR_LEN, "+ECATTBEARER: %d,%d,,%d,%d,%d,%d,%d,\"%s\",\"%s\"",
                     pCmiCnf->attBearCtxParam.pdnType, pCmiCnf->attBearCtxParam.eitf,
                     pCmiCnf->attBearCtxParam.ipv4AlloType, pCmiCnf->attBearCtxParam.NSLPI,
                     pCmiCnf->attBearCtxParam.ipv4MtuDisByNas, pCmiCnf->attBearCtxParam.nonIpMtuDisByNas,
                     pCmiCnf->attBearCtxParam.authProtocol,pCmiCnf->attBearCtxParam.authUserName,pCmiCnf->attBearCtxParam.authPassword);
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, rspStrBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * atPsCRTDCPSetCnf
 * Description: Process CMI cnf msg of AT+CRTDCP=porting
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCRTDCPSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atPsCRTDCPGetCnf
 * Description: Process CMI cnf msg of AT+CRTDCP?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCRTDCPGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    char RspBuf[12] = {'\0'};

    CmiPsGetCpDataReportCfgCnf *conf = (CmiPsGetCpDataReportCfgCnf *)paras;

    if(rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+CRTDCP: %d", conf->reporting);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atPsCRTDCPGetCnf
 * Description: Process CMI cnf msg of AT+CRTDCP=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCRTDCPGetCapaCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    char RspBuf[12] = {'\0'};

    CmiPsGetCpDataReportCfgCnf *conf = (CmiPsGetCpDataReportCfgCnf *)paras;

    if (rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+CRTDCP: %d", conf->reporting);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atPsCGAUTHSetCnf
 * Description: Process CMI cnf msg of AT+CGAUTH=cid,auth_prot,userid,password
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGAUTHSetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atPsCGAUTHGetCnf
 * Description: Process CMI cnf msg of AT+CGAUTH?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGAUTHGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[128] = {0};
    CmiPsGetDefineAuthCtxCnf *Conf = (CmiPsGetDefineAuthCtxCnf *)paras;;

    if(rc == CME_SUCC)
    {
        if(Conf->authProtocol < CMI_SECURITY_PROTOCOL_NUM)
        {
            sprintf((char*)RspBuf, "\r\n+CGAUTH: %d,%d,\"%s\",\"%s\"", Conf->cid, Conf->authProtocol,Conf->authUserName,Conf->authPassword);
        }

        if (Conf->bContinue)
        {
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, (char*)RspBuf);
        }
        else
        {
            ret = atcReply(reqHandle, AT_RC_OK, 0, RspBuf);
        }

    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}


/******************************************************************************
 * atPsCIPCASetCnf
 * Description: Process CMI cnf msg of AT+CIPCA=n,attach
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCIPCASetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * atPsCIPCAGetCnf
 * Description: Process CMI cnf msg of AT+CIPCA?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCIPCAGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    char RspBuf[12] = {'\0'};

    CmiPsGetAttachWithOrWithoutPdnCnf *conf = (CmiPsGetAttachWithOrWithoutPdnCnf *)paras;

    if(rc == CME_SUCC)
    {
        sprintf((char*)RspBuf, "+CIPCA: %d,%d", conf->nflag, conf->attachedWithoutPdn);
        ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)RspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }
    return ret;
}

/******************************************************************************
 * atPsCGAPNRCGetCnf
 * Description: Process CMI cnf msg of AT+CGAPNRC?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGAPNRCGetCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    pRspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CmiPsGetAPNRateCtrlCnf *pCmiconf = (CmiPsGetAPNRateCtrlCnf *)paras;

    if (rc == CME_SUCC)
    {
        if (pCmiconf->cid < CMI_PS_CID_NUM && pCmiconf->apnRateValid)
        {
            snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "\r\n+CGAPNRC: %d,%d,%d,%d",
                     pCmiconf->cid, pCmiconf->apnRateParam.aer, pCmiconf->apnRateParam.ulTimeUnit, pCmiconf->apnRateParam.maxUlRate);
        }
        else if (pCmiconf->cid < CMI_PS_CID_NUM)
        {
            snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "\r\n+CGAPNRC: %d,0,0", pCmiconf->cid);
        }

        if (pCmiconf->bContinue)
        {
            ret = atcReply(reqHandle, AT_RC_CONTINUE, 0, pRspStr);
        }
        else
        {
            /* Add ending \r\n */
            //strcat((char *)pRspStr,"\r\n");
            //atcReply(reqHandle, AT_RC_RAW_INFO_CONTINUE, 0, (char*)pRspStr);

            ret = atcReply(reqHandle, AT_RC_OK, 0, pRspStr);
        }
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, PNULL);
    }

    return ret;
}

/******************************************************************************
 * atPsCGCMODGetConf
 * Description: AT+CGPADDR=?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCGAPNRCGetCidCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    pRspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CHAR    pTmpStr[8] = {0};
    CmiPsGetAllBearersCidsInfoCnf *pCmiConf = (CmiPsGetAllBearersCidsInfoCnf *)paras;
    UINT8   idx = 0, rspStrLen = 0;
    BOOL    bFirstBr = TRUE;

    if (rc == CME_SUCC)
    {
        /*
         * if not activated bearer, just return:
         * +CGAPNRC:\r\n
         * \r\nOK\r\n
        */
        //if(pCmiConf->validNum > 0)
        //{
        snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "+CGAPNRC: ");
        //}

        for (idx = 0; idx < pCmiConf->validNum && idx < CMI_PS_CID_NUM; idx++)
        {
            if (CMI_PS_IS_BEARER_ACTIVTED(pCmiConf->basicInfoList[idx].bearerState))
            {
                if (bFirstBr)
                {
                    snprintf(pTmpStr, 8, "(%d", pCmiConf->basicInfoList[idx].cid);
                    bFirstBr = FALSE;
                }
                else
                {
                    snprintf(pTmpStr, 8, ",%d", pCmiConf->basicInfoList[idx].cid);
                }

                rspStrLen = (UINT8)strlen(pRspStr);
                OsaCheck(rspStrLen < ATEC_IND_RESP_128_STR_LEN, rspStrLen, ATEC_IND_RESP_128_STR_LEN, 0);
                strncat(pRspStr,
                        pTmpStr,
                        ATEC_IND_RESP_128_STR_LEN-rspStrLen-1);
            }
        }

        if (bFirstBr == FALSE)
        {
            rspStrLen = (UINT8)strlen(pRspStr);
            OsaCheck(rspStrLen < ATEC_IND_RESP_128_STR_LEN, rspStrLen, ATEC_IND_RESP_128_STR_LEN, 0);
            strncat(pRspStr,
                    ")",
                    ATEC_IND_RESP_128_STR_LEN-rspStrLen-1);
        }

        ret = atcReply(reqHandle, AT_RC_OK, 0, pRspStr);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * atPsECSENDDATACnf
 * Description: Process CMI cnf msg of AT+ECSENDDATA=cid,cpdata_len,...
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psECSENDDATACnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * psECCIOTPLANECnf
 * Description: Process CMI cnf msg of AT+ECCIOTPLANE=plane
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psECCIOTPLANECnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * psCSCONReadCnf
 * Description: Process CMI cnf msg of +CSCON?
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
 * +CSCON?
 *  +CSCON: <n>,<mode>[,<state>]
******************************************************************************/
CmsRetId psCSCONReadCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmiPsGetConnStatusCnf *pGetCSCONCnf = (CmiPsGetConnStatusCnf *)paras;
    CmsRetId ret = CMS_FAIL;
    UINT8    atChanId = AT_GET_HANDLER_CHAN_ID(reqHandle);
    UINT32   csconRptMode = mwGetAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CSCON_RPT_CFG);
    CHAR     rspBuf[ATEC_IND_RESP_128_STR_LEN] = {0};

    //memset(rspBuf, 0x00, sizeof(rspBuf));

    if (rc == CME_SUCC)
    {
        /*+CSCON: <n>,<mode>[,<state>]*/
        if (csconRptMode > 1)   //by now only supprt: (0 - 1)
        {
            OsaDebugBegin(FALSE, csconRptMode, rc, 0);
            OsaDebugEnd();
            csconRptMode = 0;   //disable unsolicited result code

            mwSetAndSaveAtChanConfigItemValue(atChanId, MID_WARE_AT_CHAN_CSCON_RPT_CFG, csconRptMode);
        }
        snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "+CSCON: %d,%d", csconRptMode, pGetCSCONCnf->connMode);

        ret = atcReply(reqHandle, AT_RC_OK, 0, rspBuf);
    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
    }

    return ret;
}

/******************************************************************************
 * atPsCGCMODGetConf
 * Description: process CMI_PS_GET_ALL_BEARERS_CIDS_INFO_CNF
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psGetAllBearersCidInfCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmiPsGetAllBearersCidsInfoCnf   *pCmiConf = PNULL;

    OsaDebugBegin(paras != PNULL, 0, 0, 0);
    return CMS_FAIL;
    OsaDebugEnd();

    pCmiConf = (CmiPsGetAllBearersCidsInfoCnf *)paras;

    switch (pCmiConf->getCidsCmd)
    {
        case CMI_PS_CGACT_GET_CIDS_CMD:
            psCGACTGetCnf(reqHandle, rc, paras);
            break;

        case CMI_PS_CGCONTRDP_GET_CIDS_CMD:
            psCGCONTRDPGetCnf(reqHandle, rc, paras);
            break;

        case CMI_PS_CGCMOD_GET_CIDS_CMD:
            psCGCMODGetCnf(reqHandle, rc, paras);
            break;

        case CMI_PS_CGEQOSRDP_GET_CIDS_CMD:
            psCGEQOSRDPGetCidCnf(reqHandle, rc, paras);
            break;

        case CMI_PS_CGAPNRC_GET_CIDS_CMD:
            psCGAPNRCGetCidCnf(reqHandle, rc, paras);
            break;

        case CMI_PS_CGPADDR_GET_CIDS_REQ:
            psCGPADDRGetCidCnf(reqHandle, rc, paras);
            break;

        default:
            break;
    }

    return CMS_RET_SUCC;
}

#if 0
/****************************************************************************************
*  Process CI ind msg of +CRTDCP
*
****************************************************************************************/
CmsRetId psCRTDCPInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    UINT8* RespBuf = PNULL;
    UINT8* dataBuf = PNULL;
    CmiPsMtCpDataInd *Ind = (CmiPsMtCpDataInd*)paras;

    RespBuf = malloc(Ind->length*2 + 24);
    dataBuf = malloc(Ind->length*2+1);
    OsaCheck(RespBuf != PNULL && dataBuf != PNULL, RespBuf, dataBuf, 0);

    memset(RespBuf, 0, Ind->length *2 +24);
    memset(dataBuf, 0, Ind->length *2+1);

    BOOL result = FALSE;
    result = atDataToHexString(dataBuf,Ind->cpdata,Ind->length);
    if(!result)
    {
        free(RespBuf);
        free(dataBuf);
        if (Ind->cpdata != PNULL)
        {
            free(Ind->cpdata);
            Ind->cpdata = PNULL;
        }
        return ret;
    }

    sprintf((char *)RespBuf, "+CRTDCP: %d,%d,\"%s\"\r\n", Ind->cid, Ind->length,dataBuf);
    ret = atcURC(chanId, (char *)RespBuf );

    free(RespBuf);
    free(dataBuf);
    if (Ind->cpdata != PNULL)
    {
        free(Ind->cpdata);
        Ind->cpdata = PNULL;
    }
    return ret;
}
#endif

/******************************************************************************
 * psCRTDCPInd
 * Description: process CMI_PS_MT_CP_DATA_IND
 * input: UINT32 chanId
 *        void *paras       //CmiPsMtCpDataInd
 * output: void
 * Comment:
 * URC: +CRTDCP: <cid>,<data_length>,\"%s\"
******************************************************************************/
CmsRetId psCRTDCPInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    *pUrcBuf = PNULL;
    UINT32  bufLen = 0, tmpStrLen = 0;
    CmiPsMtCpDataInd *pCmiInd = (CmiPsMtCpDataInd*)paras;

    /*
     * 1600 -> NB PDCP max SDU length
    */
    if (pCmiInd->length == 0 || pCmiInd->cpdata == PNULL || pCmiInd->length > 1600)
    {
        OsaDebugBegin(FALSE, pCmiInd->length, pCmiInd->cpdata, 0);
        OsaDebugEnd();
        return CMS_FAIL;
    }

    /* "pCmiInd->length * 2 + 1" => hex need to convert to hex string */
    bufLen  = ATEC_IND_RESP_64_STR_LEN + pCmiInd->length * 2 + 1;
    pUrcBuf = (CHAR *)OsaAllocMemory(bufLen);

    if (pUrcBuf == PNULL)
    {
        OsaDebugBegin(FALSE, pCmiInd->length, bufLen, 0);
        OsaDebugEnd();
        return CMS_FAIL;
    }

    /* +RECVNONIP: <cid>,<data_length> */
    snprintf(pUrcBuf, bufLen, "\r\n+CRTDCP: %d,%d,\"", pCmiInd->cid, pCmiInd->length);
    tmpStrLen   = strlen(pUrcBuf);

    /* data */
    OsaCheck(bufLen > tmpStrLen, bufLen, tmpStrLen, 0);

    cmsHexToHexStr(pUrcBuf+tmpStrLen,
                   bufLen-tmpStrLen,
                   pCmiInd->cpdata,
                   pCmiInd->length);

    /* Ending: \r\n */
    atStrnCat(pUrcBuf, bufLen, "\"\r\n", 3);

    ret = atcURC(chanId, pUrcBuf);

    /*
     * Free URC string buffer, and non-ip data buffer
    */
    OsaFreeMemory(&pUrcBuf);
    OsaFreeMemory(&(pCmiInd->cpdata));

    return ret;
}


/****************************************************************************************
*  Process CI ind msg of +CEREG
*
****************************************************************************************/
CmsRetId psCEREGInd(UINT32 chanId, void *paras)
{
#define ATEC_CEREG_IND_STR_LEN  100
#define ATEC_CEREG_IND_TMP_LEN  30

    CmsRetId ret = CMS_FAIL;
    CHAR    indBuf[ATEC_CEREG_IND_STR_LEN], tempBuf[ATEC_CEREG_IND_TMP_LEN];
    CmiPsCeregInd *pCeregInd = (CmiPsCeregInd*)paras;
    UINT32  ceregRptMode = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_CEREG_RPT_CFG);
#ifndef  FEATURE_REF_AT_ENABLE
    BOOL    locPrint = FALSE, rejPrint = FALSE, actTimePrint = FALSE;
#endif

    if (ceregRptMode == CMI_PS_DISABLE_CEREG)
    {
        return CMS_RET_SUCC;  // not need report: +CEREG indication
    }

    if (ceregRptMode > CMI_PS_CEREG_LOC_PSM_INFO_EMM_CAUSE ||
        pCeregInd->changedType == CMI_CEREG_NONE_CHANGED ||
        pCeregInd->changedType > CMI_CEREG_PSM_INFO_CHANGED)
    {
        OsaDebugBegin(FALSE, pCeregInd->changedType, ceregRptMode, 0);
        OsaDebugEnd();
        return CMS_RET_SUCC;
    }

    if ((ceregRptMode == CMI_PS_ENABLE_CEREG && pCeregInd->changedType > CMI_CEREG_STATE_CHANGED) ||
        (ceregRptMode == CMI_PS_CEREG_LOC_INFO && pCeregInd->changedType > CMI_CEREG_LOC_INFO_CHANGED) ||
        (ceregRptMode == CMI_PS_CEREG_LOC_EMM_CAUSE && pCeregInd->changedType > CMI_CEREG_REJECT_INFO_CHANGED) ||
        (ceregRptMode == CMI_PS_CEREG_LOC_PSM_INFO && pCeregInd->changedType == CMI_CEREG_REJECT_INFO_CHANGED)) /*only reject info changed */
    {
        ECOMM_TRACE(UNILOG_ATCMD_EXEC, psCEREGInd_1, P_VALUE, 2,
                    "ATEC, CEREG report mode: %d, but PS info changed type: %d, don't need to report CEREG",
                    ceregRptMode, pCeregInd->changedType);
        return CMS_RET_SUCC;
    }

    if (ceregRptMode == CMI_PS_CEREG_LOC_PSM_INFO_EMM_CAUSE &&
        pCeregInd->changedType == CMI_CEREG_PSM_INFO_CHANGED &&
        pCeregInd->t3324AndT3412ExtCeregUrc == FALSE)
    {
        ECOMM_TRACE(UNILOG_ATCMD_EXEC, psCEREGInd_2, P_WARNING, 2,
                    "ATEC, CEREG report mode: %d, PS info changed type: %d, T3324_T3412_EXT_CHANGE_REPORT is disabled by user, don't need to report CEREG",
                    ceregRptMode, pCeregInd->changedType);
        return CMS_RET_SUCC;

    }

    /*
     * 1: +CEREG: <stat>
     * 2: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>]]
     * 3: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,<cause_type>,<reject_cause>]]
     * 4: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,,[,[<ActiveTime>],[<Periodic-TAU>]]]]
     * 5: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,[<cause_type>],[<reject_cause>][,[<Active-Time>],[<Periodic-TAU>]]]]
     */

    memset(indBuf, 0, sizeof(indBuf));

#ifdef  FEATURE_REF_AT_ENABLE
    snprintf(indBuf, ATEC_CEREG_IND_STR_LEN, "+CEREG: %d", pCeregInd->state);

    if (ceregRptMode >= CMI_PS_CEREG_LOC_INFO)
    {
        //For Ref AT: if locPresent is false, fill tac&cellId with all zero
        memset(tempBuf, 0, sizeof(tempBuf));
        //For Ref AT: no need double quote
        snprintf(tempBuf, ATEC_CEREG_IND_TMP_LEN, ",%04X,%08X,%d",
                 pCeregInd->tac, pCeregInd->celId, pCeregInd->act);

        strcat(indBuf, tempBuf);
    }

    /*
     * 3: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,<cause_type>,<reject_cause>]]
     * 5: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,[<cause_type>],[<reject_cause>][,[<Active-Time>],[<Periodic-TAU>]]]]
     */
    if (ceregRptMode == CMI_PS_CEREG_LOC_EMM_CAUSE ||
        ceregRptMode == CMI_PS_CEREG_LOC_PSM_INFO_EMM_CAUSE)
    {
        if (pCeregInd->rejCausePresent)
        {
            memset(tempBuf, 0, sizeof(tempBuf));
            snprintf(tempBuf, ATEC_CEREG_IND_TMP_LEN, ",%d,%d", pCeregInd->causeType, pCeregInd->rejCause);

            strcat(indBuf, tempBuf);
        }
        else
        {
            //For Ref AT: always need comma when n>=3 before <cause_type> and <reject_cause>
            strcat(indBuf, ",,");
        }
    }

    if (ceregRptMode == CMI_PS_CEREG_LOC_PSM_INFO)
    {
        //For Ref AT: when n==4, fill <cause_type> and <reject_cause> with null
        strcat(indBuf, ",,");
    }

    /*
     * 4: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,,[,[<ActiveTime>],[<Periodic-TAU>]]]]
     * 5: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,[<cause_type>],[<reject_cause>][,[<Active-Time>],[<Periodic-TAU>]]]]
     */
    if (ceregRptMode == CMI_PS_CEREG_LOC_PSM_INFO ||
        ceregRptMode == CMI_PS_CEREG_LOC_PSM_INFO_EMM_CAUSE)
    {
        //For Ref AT: always need comma when n>=4 before <Active-Time>
        strcat(indBuf, ",");

        if (pCeregInd->activeTimePresent)
        {
            memset(tempBuf, 0, sizeof(tempBuf));
            atByteToBitString((UINT8 *)tempBuf, pCeregInd->activeTime, 8);
            //For Ref AT: no need double quote
            strcat(indBuf, tempBuf);
        }

        //For Ref AT: always need comma when n>=4 before <Periodic-Tau>
        strcat(indBuf, ",");

        if (pCeregInd->extTauTimePresent)
        {
            memset(tempBuf, 0, sizeof(tempBuf));
            atByteToBitString((UINT8 *)tempBuf, pCeregInd->extPeriodicTau, 8);
            //For Ref AT: no need double quote
            strcat(indBuf, tempBuf);
        }
    }
#else
    snprintf(indBuf, ATEC_CEREG_IND_STR_LEN, "+CEREG: %d", pCeregInd->state);


    if (ceregRptMode >= CMI_PS_CEREG_LOC_INFO)
    {
        if (pCeregInd->locPresent)  /*[<tac>],[<ci>],[<AcT>]*/
        {
            memset(tempBuf, 0, sizeof(tempBuf));
            snprintf(tempBuf, ATEC_CEREG_IND_TMP_LEN, ",\"%04X\",\"%08X\",%d",
                     pCeregInd->tac, pCeregInd->celId, pCeregInd->act);

            strcat(indBuf, tempBuf);
            locPrint = TRUE;
        }
    }

    /*
     * 3: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,<cause_type>,<reject_cause>]]
     * 5: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,[<cause_type>],[<reject_cause>][,[<Active-Time>],[<Periodic-TAU>]]]]
     */
    if (ceregRptMode == CMI_PS_CEREG_LOC_EMM_CAUSE ||
        ceregRptMode == CMI_PS_CEREG_LOC_PSM_INFO_EMM_CAUSE)
    {
        if (pCeregInd->rejCausePresent)
        {
            // whether need to fill empty: [<tac>],[<ci>],[<AcT>]
            if (locPrint == FALSE)
            {
                strcat(indBuf, ",,,");
                locPrint = TRUE;
            }

            memset(tempBuf, 0, sizeof(tempBuf));
            snprintf(tempBuf, ATEC_CEREG_IND_TMP_LEN, ",%d,%d", pCeregInd->causeType, pCeregInd->rejCause);

            strcat(indBuf, tempBuf);
            rejPrint = TRUE;
        }
    }

    /*
     * 4: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,,[,[<ActiveTime>],[<Periodic-TAU>]]]]
     * 5: +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,[<cause_type>],[<reject_cause>][,[<Active-Time>],[<Periodic-TAU>]]]]
     */
    if (ceregRptMode == CMI_PS_CEREG_LOC_PSM_INFO ||
        ceregRptMode == CMI_PS_CEREG_LOC_PSM_INFO_EMM_CAUSE)
    {
        if (pCeregInd->activeTimePresent || pCeregInd->extTauTimePresent)
        {
            /*[<tac>],[<ci>],[<AcT>] should be already set*/
            if (locPrint == FALSE)
            {
                OsaDebugBegin(FALSE, pCeregInd->changedType, pCeregInd->tac, pCeregInd->act);
                OsaDebugEnd();

                strcat(indBuf, ",,,");
                locPrint = TRUE;
            }

            /* fill the empty reject cause*/
            if (rejPrint == FALSE)
            {
                strcat(indBuf, ",,");
                rejPrint = TRUE;
            }
            else    //if PSM parameters present, should no reject cause
            {
                OsaDebugBegin(FALSE, pCeregInd->rejCause, pCeregInd->rejCausePresent, ceregRptMode);
                OsaDebugEnd();
            }

            if (pCeregInd->activeTimePresent)
            {
                memset(tempBuf, 0, sizeof(tempBuf));
                atByteToBitString((UINT8 *)tempBuf, pCeregInd->activeTime, 8);
                strcat(indBuf, ",\"");
                strcat(indBuf, tempBuf);
                strcat(indBuf, "\"");

                actTimePrint = TRUE;
            }

            if (pCeregInd->extTauTimePresent)
            {
                if (actTimePrint == FALSE)
                {
                    strcat(indBuf, ",");
                    actTimePrint = TRUE;
                }
                memset(tempBuf, 0, sizeof(tempBuf));
                atByteToBitString((UINT8 *)tempBuf, pCeregInd->extPeriodicTau, 8);
                strcat(indBuf, ",\"");
                strcat(indBuf, tempBuf);
                strcat(indBuf, "\"");
            }
        }
    }
#endif

    OsaCheck(strlen(indBuf) < ATEC_CEREG_IND_STR_LEN, strlen(indBuf), ATEC_CEREG_IND_STR_LEN, 0);

    ret = atcURC(chanId, indBuf);

    return ret;
}

/****************************************************************************************
*  Process CI ind msg of +CGEV: xx DETACH
*
****************************************************************************************/
CmsRetId psDetachInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;

    if (mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_CGEREP_MODE_CFG) == ATC_CGEREP_FWD_CGEV)
    {
        ret = atcURC(chanId, "+CGEV: NW DETACH\r\n");
    }

    return ret;
}

/****************************************************************************************
*  Process CI ind msg of +CGEV: ME PDN ACT <cid>
*
****************************************************************************************/
CmsRetId psBearerActInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;
    CHAR    urcStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CmiPsBearerActedInd *pCmiInd = (CmiPsBearerActedInd*)paras;

    /*
     * Default bearer:
     *  +CGEV: ME PDN ACT <cid>[,<reason>[,<cid_other>]][,<WLAN_Offload>]
     * Dedicated bearer:
     *  +CGEV: NW ACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>]
     *  +CGEV: ME ACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>]
    */
    if (mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_CGEREP_MODE_CFG) == ATC_CGEREP_FWD_CGEV)
    {
        if (pCmiInd->bearerType == CMI_PS_BEARER_DEFAULT)
        {
            OsaDebugBegin(pCmiInd->cid < CMI_PS_CID_NUM, pCmiInd->pCid, pCmiInd->cid, 0);
            OsaDebugEnd();

            if (pCmiInd->pdnReason < CMI_PS_SINGLE_ADDR_ONLY_ALLOWED_BUT_ACTIVE_SECOND_BEARER_FAILED)
            {
                snprintf(urcStr, ATEC_IND_RESP_128_STR_LEN, "+CGEV: ME PDN ACT %d,%d",
                         pCmiInd->cid, pCmiInd->pdnReason);
            }
            else
            {
                snprintf(urcStr, ATEC_IND_RESP_128_STR_LEN, "+CGEV: ME PDN ACT %d", pCmiInd->cid);
            }
        }
        else    //dedicated bearer
        {
            /*For NB, should no dedicated bearer*/
            OsaDebugBegin(FALSE, pCmiInd->pCid, pCmiInd->cid, 0);
            OsaDebugEnd();

            if (pCmiInd->isMeInitial)
            {
                /*
                 * +CGEV: ME ACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>]
                 * <event_type>:
                 *  0 Informational event
                 *  1 Information request: Acknowledgement required. The acknowledgement can be accept or reject, see +CGANS.
                 * We only support 0;
                 * <WLAN_Offload>
                 *  NOT SUPPORT
                */
                snprintf(urcStr, ATEC_IND_RESP_128_STR_LEN, "+CGEV: ME ACT %d,%d,0",
                         pCmiInd->pCid, pCmiInd->cid);
            }
            else
            {
                /*
                 * +CGEV: NW ACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>]
                */
                snprintf(urcStr, ATEC_IND_RESP_128_STR_LEN, "+CGEV: NW ACT %d,%d,0",
                         pCmiInd->pCid, pCmiInd->cid);
            }
        }

        ret = atcURC(chanId, urcStr);
    }

    return ret;
}

/****************************************************************************************
*  Process CI ind msg of +CGEV: NW/MS PDN DEACT <cid>
*
****************************************************************************************/
CmsRetId psBearerDeaInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;
    CHAR    urcStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CmiPsBearerDeActInd *pCmiInd = (CmiPsBearerDeActInd*)paras;

    /*
     * Default bearer
     *  +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>]
     *  +CGEV: ME PDN DEACT <cid>
     * Dedicated bearer
     *  +CGEV: NW DEACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>]
     *  +CGEV: ME DEACT <p_cid>, <cid>, <event_type>
     * Note:
     * 1> <event_type>, only support 0 -  Informational event
     * 2> <WLAN_Offload>, not support
    */

    if (mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_CGEREP_MODE_CFG) == ATC_CGEREP_FWD_CGEV)
    {
        //memset(RespBuf, 0, sizeof(RespBuf));
        if(pCmiInd->bearerType == CMI_PS_BEARER_DEFAULT)
        {
            if (pCmiInd->isMeInitial)
            {
                snprintf(urcStr, ATEC_IND_RESP_128_STR_LEN, "+CGEV: ME PDN DEACT %d", pCmiInd->cid);
            }
            else
            {
                snprintf(urcStr, ATEC_IND_RESP_128_STR_LEN, "+CGEV: NW PDN DEACT %d", pCmiInd->cid);
            }
        }
        else //dedicated bearer
        {
            /* NB not support dedicated bearer */
            OsaDebugBegin(FALSE, pCmiInd->cid, pCmiInd->pCid, 0);
            OsaDebugEnd();

            if (pCmiInd->isMeInitial)
            {
                /* +CGEV: ME DEACT <p_cid>, <cid>, <event_type> */
                snprintf(urcStr, ATEC_IND_RESP_128_STR_LEN, "+CGEV: ME DEACT %d, %d, 0", pCmiInd->pCid, pCmiInd->cid);
            }
            else
            {
                /* +CGEV: NW DEACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>] */
                snprintf(urcStr, ATEC_IND_RESP_128_STR_LEN, "+CGEV: NW DEACT %d, %d, 0", pCmiInd->pCid, pCmiInd->cid);
            }
        }

        ret = atcURC(chanId, urcStr);
    }

    return ret;
}


/****************************************************************************************
*  Process CI ind msg of +CGEV: NW/ME MODIFY <cid>,
*
****************************************************************************************/
CmsRetId psBearerModInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_RET_SUCC;
    CHAR    urcStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CmiPsBearerModifyInd *pCmiInd = (CmiPsBearerModifyInd*)paras;

    /*
     * +CGEV: NW MODIFY <cid>, <change_reason>, <event_type>[,<WLAN_Offload>]
     * +CGEV: ME MODIFY <cid>, <change_reason>, <event_type>[,<WLAN_Offload>]
     * Note:
     * 1> <event_type>, only support 0 -  Informational event
     * 2> <WLAN_Offload>, not support
    */
    if (mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_CGEREP_MODE_CFG) == ATC_CGEREP_FWD_CGEV)
    {
        if (pCmiInd->bUeInited)
        {
            snprintf(urcStr, ATEC_IND_RESP_128_STR_LEN, "+CGEV: ME MODIFY %d,%d,0",
                     pCmiInd->cid, pCmiInd->changeReasonBitmap);
        }
        else
        {
            snprintf(urcStr, ATEC_IND_RESP_128_STR_LEN, "+CGEV: NW MODIFY %d,%d,0",
                     pCmiInd->cid, pCmiInd->changeReasonBitmap);
        }

        ret = atcURC(chanId, urcStr);
    }

    return ret;
}

#if 0
/****************************************************************************************
*  Process CI ind msg of +RECVNONIP:<cid>,<data_length>,<data>
*
****************************************************************************************/
CmsRetId psRECVNONIPInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    UINT8* RespBuf = PNULL;
    UINT8* dataBuf = PNULL;
    CmiPsRecvDlNonIpDataInd *Ind = (CmiPsRecvDlNonIpDataInd*)paras;

    RespBuf = malloc(Ind->length*2 + 20);
    dataBuf = malloc(Ind->length*2);
    OsaCheck(RespBuf != PNULL && dataBuf != PNULL, RespBuf, dataBuf, 0);
    memset(RespBuf, 0, Ind->length *2 +20);
    memset(dataBuf, 0, Ind->length *2);

    BOOL result = FALSE;
    result = atDataToHexString(dataBuf,Ind->pData,Ind->length);
    if(!result)
    {
        free(RespBuf);
        free(dataBuf);
        if (Ind->pData != PNULL)
        {
            free(Ind->pData);
            Ind->pData = PNULL;
        }
        return ret;
    }
    sprintf((char *)RespBuf, "+RECVNONIP: %d,%d,%s\r\n", Ind->cid, Ind->length,dataBuf);
    ret = atcURC(chanId, (char *)RespBuf );

    free(RespBuf);
    free(dataBuf);
    if (Ind->pData != PNULL)
    {
        free(Ind->pData);
        Ind->pData = PNULL;
    }
    return ret;
}
#endif

/******************************************************************************
 * psRECVNONIPInd
 * Description: process CMI_PS_RECV_DL_NON_IP_DATA_IND
 * input: UINT32 chanId
 *        void *paras       //CmiPsRecvDlNonIpDataInd
 * output: void
 * Comment:
 * URC: +RECVNONIP:<cid>,<data_length>,<data>
******************************************************************************/
CmsRetId psRECVNONIPInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR    *pUrcBuf = PNULL;
    UINT32  bufLen = 0, tmpStrLen = 0;
    CmiPsRecvDlNonIpDataInd *pCmiInd = (CmiPsRecvDlNonIpDataInd*)paras;

    /*
     * 1600 -> NB PDCP max SDU length
    */
    if (pCmiInd->length == 0 || pCmiInd->pData == PNULL || pCmiInd->length > 1600)
    {
        OsaDebugBegin(FALSE, pCmiInd->length, pCmiInd->pData, 0);
        OsaDebugEnd();
        return CMS_FAIL;
    }

    /* "pCmiInd->length * 2 + 1" => hex need to convert to hex string */
    bufLen  = ATEC_IND_RESP_64_STR_LEN + pCmiInd->length * 2 + 1;
    pUrcBuf = (CHAR *)OsaAllocMemory(bufLen);

    if (pUrcBuf == PNULL)
    {
        OsaDebugBegin(FALSE, pCmiInd->length, bufLen, 0);
        OsaDebugEnd();
        return CMS_FAIL;
    }

    /* +RECVNONIP: <cid>,<data_length> */
    snprintf(pUrcBuf, bufLen, "\r\n+RECVNONIP: %d,%d,\"", pCmiInd->cid, pCmiInd->length);
    tmpStrLen   = strlen(pUrcBuf);

    /* data */
    OsaCheck(bufLen > tmpStrLen, bufLen, tmpStrLen, 0);

    cmsHexToHexStr(pUrcBuf+tmpStrLen,
                   bufLen-tmpStrLen,
                   pCmiInd->pData,
                   pCmiInd->length);

    /* Ending: \r\n */
    atStrnCat(pUrcBuf, bufLen, "\"\r\n", 3);

    ret = atcURC(chanId, pUrcBuf);

    /*
     * Free URC string buffer, and non-ip data buffer
    */
    OsaFreeMemory(&pUrcBuf);
    OsaFreeMemory(&(pCmiInd->pData));

    return ret;
}



/****************************************************************************************
*  Process CI ind msg of +CSCON: <mode>
*
****************************************************************************************/
CmsRetId psCSCONRptInd(UINT32 chanId, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR indBuf[ATEC_IND_RESP_128_STR_LEN];
    CmiPsConnStatusInd *pConnStatusInd = (CmiPsConnStatusInd*)paras;
    UINT32   csconRptMode = mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_CSCON_RPT_CFG);

    if (csconRptMode == ATC_DISABLE_CSCON_RPT)
    {
        //0 disable unsolicited result code
        return CMS_RET_SUCC;
    }
    else if (csconRptMode == ATC_CSCON_RPT_MODE)  //1: +CSCON: <mode>
    {
        memset(indBuf, 0x00, sizeof(indBuf));
        snprintf(indBuf, ATEC_IND_RESP_128_STR_LEN, "+CSCON: %d", pConnStatusInd->connMode);

        ret = atcURC(chanId, indBuf);
    }
    else
    {
        OsaDebugBegin(FALSE, csconRptMode, 0, 0);
        OsaDebugEnd();

        mwSetAndSaveAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_CSCON_RPT_CFG, 0);
    }

    return ret;
}

/******************************************************************************
 * psCNMPSDCnf
 * Description: Process CMI cnf msg of AT+CNMPSD
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCNMPSDCnf(UINT16 reqHandle, UINT16 rc, void *paras)
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
 * psCEERCnf
 * Description: Process CMI cnf msg of AT+CEER
 * input: UINT16 primId;  //prim ID
 *        UINT16 reqHandle;   //req handler
          UINT16 rc;  //
 *        void *paras;
 * output: void
 * Comment:
******************************************************************************/
CmsRetId psCEERCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    CmsRetId ret = CMS_FAIL;
    CHAR     pRspStr[ATEC_IND_RESP_128_STR_LEN] = {0};
    CmiPsGetCeerCnf *pCeerCnf = (CmiPsGetCeerCnf *)paras;

    if (rc == CME_SUCC)
    {
        if (pCeerCnf->bEmmCausePresent)
        {
            /*+CEER: <report>*/
            snprintf(pRspStr, ATEC_IND_RESP_128_STR_LEN, "+CEER: %s", atcGetCEEREmmCauseStr(pCeerCnf->emmCause));

            ret = atcReply(reqHandle, AT_RC_OK, 0, (char*)pRspStr);
        }
        else
        {
            ret = atcReply(reqHandle, AT_RC_OK, 0, NULL);
        }

    }
    else
    {
        ret = atcReply(reqHandle, AT_RC_CME_ERROR, rc, NULL);
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
 * psCmiCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const CmiCnfFuncMapList psCmiCnfHdrList[] =
{
    {CMI_PS_GET_CEREG_CNF, psCEREGGetCnf},
    {CMI_PS_GET_CEREG_CAP_CNF, psCEREGGetCapaCnf},
    {CMI_PS_DEFINE_BEARER_CTX_CNF, psCGDCONTSetCnf},
    {CMI_PS_DEL_BEARER_CTX_CNF, psCidContDeleteCnf},
    {CMI_PS_GET_ALL_BEARERS_CIDS_INFO_CNF, psGetAllBearersCidInfCnf},

    {CMI_PS_GET_DEFINED_BEARER_CTX_CNF, psCidContGetCnf},
    {CMI_PS_SET_ATTACHED_BEARER_CTX_CNF, psATTBEARSetCnf},
    {CMI_PS_GET_ATTACHED_BEARER_CTX_CNF, psATTBEARGetCnf},
    {CMI_PS_SET_ATTACHED_AUTH_CTX_CNF, PNULL},
    {CMI_PS_GET_ATTACHED_AUTH_CTX_CNF, PNULL},
    {CMI_PS_DEFINE_DEDICATED_BEARER_CTX_CNF, PNULL},
    {CMI_PS_DEL_DEDICATED_BEARER_CTX_CNF, PNULL},
    {CMI_PS_GET_DEFINED_DEDICATED_BEARER_CTX_CNF, PNULL},
    {CMI_PS_SET_ATTACH_STATE_CNF, psCGATTSetCnf},
    {CMI_PS_GET_ATTACH_STATE_CNF, psCGATTGetCnf},
    {CMI_PS_GET_ATTACH_STATE_CAPA_CNF, PNULL},
    {CMI_PS_SET_BEARER_ACT_STATE_CNF, psCGACTSetCnf},
    {CMI_PS_GET_BEARER_ACT_CAPA_CNF, PNULL},
    {CMI_PS_MODIFY_BEARER_CTX_CNF, psCGCMODModCnf},
    {CMI_PS_ENTER_DATA_STATE_CNF, psCGDATASetCnf},
    {CMI_PS_SET_PS_EVENT_REPORT_CFG_CNF, PNULL},
    {CMI_PS_GET_PS_EVENT_REPORT_CFG_CNF, PNULL},
    {CMI_PS_GET_PS_EVENT_REPORT_CFG_CAPA_CNF, PNULL},
    {CMI_PS_READ_BEARER_DYN_CTX_CNF, psCGCONTRDPReadCnf},
    {CMI_PS_READ_DEDICATED_BEARER_DYN_CTX_CNF,PNULL},
    {CMI_PS_GET_BEARER_IPADDR_CNF, psCGPADDRGetCnf},
    {CMI_PS_SET_ATTACH_WITH_OR_WITHOUT_PDN_CNF, psCIPCASetCnf},
    {CMI_PS_GET_ATTACH_WITH_OR_WITHOUT_PDN_CNF, psCIPCAGetCnf},
    {CMI_PS_GET_APN_RATE_CTRL_PARM_CNF, psCGAPNRCGetCnf},
    {CMI_PS_DEFINE_TFT_PARM_CNF, psCGTFTSetCnf},
    {CMI_PS_DELETE_TFT_PARM_CNF, psCGTFTDelCnf},
    {CMI_PS_GET_DEFINED_TFT_PARM_CNF, psCGTFTGetCnf},
    {CMI_PS_DEFINE_EPS_QOS_CNF, psCGEQOSSetCnf},
    {CMI_PS_DEL_DEFINE_EPS_QOS_CNF, psCGEQOSDelCnf},
    {CMI_PS_GET_DEFINED_EPS_QOS_CNF, psCGEQOSGetCnf},
    {CMI_PS_READ_DYN_BEARER_EPS_QOS_CNF,psCGEQOSRDPGetCnf},
    {CMI_PS_SEND_CP_DATA_CNF, psCSODCPSetCnf},
    {CMI_PS_SET_MT_CP_DATA_REPORT_CFG_CNF, psCRTDCPSetCnf},
    {CMI_PS_GET_MT_CP_DATA_REPORT_CFG_CNF, psCRTDCPGetCnf},
    {CMI_PS_SEND_UL_DATA_CNF, psECSENDDATACnf},
    {CMI_PS_TRANS_CIOT_PLANE_CNF, psECCIOTPLANECnf},
    {CMI_PS_GET_CONN_STATUS_CNF, psCSCONReadCnf},
    {CMI_PS_DEFINE_AUTH_CTX_CNF,psCGAUTHSetCnf},
    {CMI_PS_GET_DEFINED_AUTH_CTX_CNF,psCGAUTHGetCnf},
    {CMI_PS_NO_MORE_PS_DATA_CNF,     psCNMPSDCnf},
    {CMI_PS_GET_CEER_CNF,            psCEERCnf},

    {CMI_PS_PRIM_BASE, PNULL}   /* this should be the last */

};

/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS

/******************************************************************************
 * atPsProcCmiCnf
 * Description: Process CMI cnf msg for PS sub-module
 * input: CacCmiCnf *pCmiCnf;  //ptr of cmi cnf
 * output: void
 * Comment:
******************************************************************************/
void atPsProcCmiCnf(CacCmiCnf *pCmiCnf)
{
    UINT8 hdrIndex = 0;
    CmiCnfHandler cmiCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmiCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    while (psCmiCnfHdrList[hdrIndex].primId != 0)
    {
        if (psCmiCnfHdrList[hdrIndex].primId == primId)
        {
            cmiCnfHdr = psCmiCnfHdrList[hdrIndex].cmiCnfHdr;
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
 * atPsProcCmiInd
 * Description: Process CMI Ind msg for PS sub-module
 * input: CacCmiInd *pCmiInd;  //ptr of cmi Ind
 * output: void
 * Comment:
******************************************************************************/
void atPsProcCmiInd(CacCmiInd *pCmiInd)
{
    UINT16 primId = 0;
    UINT32 chanId = 0;

    OsaDebugBegin(pCmiInd != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmiInd->header.indId);

    if (primId == CMI_PS_MT_CP_DATA_IND)
    {
        psCRTDCPInd(CMS_CHAN_DEFAULT, pCmiInd->body);
        return;
    }
    else if (primId == CMI_PS_RECV_DL_NON_IP_DATA_IND)
    {
        psRECVNONIPInd(CMS_CHAN_DEFAULT, pCmiInd->body);
        return;
    }


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
                case CMI_PS_DETACH_STATE_IND:
                    psDetachInd(chanId, pCmiInd->body);
                    break;

                case CMI_PS_BEARER_ACTED_IND:
                    psBearerActInd(chanId, pCmiInd->body);
                    break;

                case CMI_PS_BEARER_DEACT_IND:
                    psBearerDeaInd(chanId, pCmiInd->body);
                    break;

                case CMI_PS_BEARER_MODIFY_IND:
                    psBearerModInd(chanId, pCmiInd->body);
                    break;

                case CMI_PS_CEREG_IND:
                    psCEREGInd(chanId, pCmiInd->body);
                    break;

                case CMI_PS_CONN_STATUS_IND:
                    psCSCONRptInd(chanId, pCmiInd->body);
                    break;

                default:
                    break;
            }
        }
    }

}





