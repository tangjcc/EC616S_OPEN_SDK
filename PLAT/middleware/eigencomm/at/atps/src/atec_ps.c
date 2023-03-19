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
//#include "atdial.h"
#include "atec_ps_cnf_ind.h"

//extern BOOL cgpaddrActCmd;

extern BOOL psDialGetPsConnStatus(void);

#define _DEFINE_AT_REQ_FUNCTION_LIST_
/**
  \fn          CmsRetId psCGCMOD(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGCMOD(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 cid;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CGCMOD=? */
        {
            /* Send the CI Request */
            ret = psGetGPRSActiveCidList(atHandle);

            break;
        }

        case AT_SET_REQ:      /* AT+CGCMOD= */
        {
            //Get context identifier - multiple cids is not supported
            if ((pAtCmdReq->paramRealNum == 1) &&
                (atGetNumValue(pParamList, 0, &cid,
                               ATC_CGCMOD_0_CID_VAL_MIN, ATC_CGCMOD_0_CID_VAL_MAX, ATC_CGCMOD_0_CID_VAL_DEFAULT) == AT_PARA_OK))
            {
                /* Send the CI Request */
                ret = psModGPRSContext(atHandle, cid);
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        #if 0
        case AT_EXEC_REQ:        /* AT+CGCMOD */
        {
            ret = psModGPRSContext(atHandle, 5);
            break;
        }
        #endif

        case AT_READ_REQ:           /* AT+CGCMOD?  */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCGREG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGREG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32 cgreg;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CGREG=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CGREG: (0-2)\r\n");
            break;
        }

        case AT_READ_REQ:    /* AT+CGREG?  */
        {
            ret = psGetGPRSRegStatus(atHandle);
            break;
        }

        case AT_SET_REQ:   /* AT+CGREG=  */
        {
            /*
            **  Parse the state variable.
            */
            if(atGetNumValue(pParamList, 0, &cgreg, ATC_CGREG_0_VAL_MIN, ATC_CGREG_0_VAL_MAX, ATC_CGREG_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                ret = psSetGPRSRegStatus(atHandle, (UINT32)cgreg);
            }

            if(ret == CMS_RET_SUCC)
            {
                //CGREG cnf will call atcReply
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCGATT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGATT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32 cgatt;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CGATT=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CGATT: (0,1)\r\n");
            break;
        }

        case AT_READ_REQ:    /* AT+CGATT?  */
        {
            ret = psGetGPRSAttached(atHandle);
            break;
        }

        case AT_SET_REQ:   /* AT+CGATT=  */
        {
            /*
            **  Parse the state variable.
            */
            if(atGetNumValue(pParamList, 0, &cgatt, ATC_CGATT_0_STATE_VAL_MIN, ATC_CGATT_0_STATE_VAL_MAX, ATC_CGATT_0_STATE_VAL_DEFAULT) != AT_PARA_ERR)
            {
                /*
                **  Check if this is an attach or detach request.
                */
                switch (cgatt)
                {
                    case CMI_PS_ATTACHED:       /* perform GPRS attach */
                    {
                        ret = psSetGPRSAttached(atHandle, TRUE);
                        break;
                    }

                    case CMI_PS_DETACHED:       /* perform GPRS detach and flag all the CIDs as inactive */
                    {
                        ret = psSetGPRSAttached(atHandle, FALSE);
                        break;
                    }

                    default:
                    {
                        break;
                    }

                }   /* switch */
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCGACT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGACT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32 pdpState;
    INT32 cid;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CGACT=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CGACT: (0,1)");
            break;
        }

        case AT_READ_REQ:    /* AT+CGACT?  */
        {
            ret = psGetGPRSContextActivatedList(atHandle);
            break;
        }

        case AT_SET_REQ:      /* AT+CGACT= */
        {
            /*
            **  Parse the state.
            */
            if(atGetNumValue(pParamList, 0, &pdpState, ATC_CGACT_0_STATE_VAL_MIN, ATC_CGACT_0_STATE_VAL_MAX, ATC_CGACT_0_STATE_VAL_DEFAULT) != AT_PARA_ERR)
            {
                /* get the <cid> parameter */
                if(atGetNumValue(pParamList, 1, &cid, ATC_CGACT_1_CID_VAL_MIN, ATC_CGACT_1_CID_VAL_MAX, ATC_CGACT_1_CID_VAL_DEFAULT) != AT_PARA_ERR)
                {
                    ret = psSetGPRSContextActivated(atHandle, cid, pdpState);
                }
            }

            if(ret == CMS_RET_SUCC)
            {
                //CGACT cnf will call atcReply
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:   /* AT+CGACT */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCGDATA(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGDATA(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT8   l2pTypeStr[ATC_CGDATA_0_L2P_STR_MAX_LEN] = { 0 };
    UINT16  l2pTypeStrLen = 0;
    INT32   cid = 0;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32     operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP    pParamList = pAtCmdReq->pParamList;
    MtErrorResultCode   cmeError = CME_SUCC;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CGDATA=? */
        {
            /* +CGDATA: (list of supported <L2P>s) */
            ret = atcReply(atHandle, AT_RC_OK, 0, "+CGDATA: (\"M-PT\")");
            break;
        }

        case AT_SET_REQ:      /* AT+CGDATA= */
        {
            /*AT+CGDATA=[<L2P>],<cid>*/
            if (atGetStrValue(pParamList, 0, l2pTypeStr,
                               ATC_CGDATA_0_L2P_STR_MAX_LEN, &l2pTypeStrLen, ATC_CGDATA_0_L2P_STR_DEFAULT) == AT_PARA_ERR)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, psCGDATA_l2p_1, P_WARNING, 0,
                            "ATCMD: CGDATA, invalid param: <l2p>");
                cmeError = CME_INCORRECT_PARAM;
            }

            if (cmeError == CME_SUCC)
            {
                /*only support "M-PT" type*/
                if ((strlen("M-PT") != l2pTypeStrLen) ||
                    (strncasecmp((CHAR* )(l2pTypeStr), "M-PT", l2pTypeStrLen) != 0))
                {
                    ECOMM_STRING(UNILOG_ATCMD_EXEC, psCGDATA_l2p_2, P_WARNING,
                                 "ATCMD: CGDATA, not support <l2p> : %s", l2pTypeStr);
                    cmeError = CME_INCORRECT_PARAM;
                }
            }

            /*<cid>, is must*/
            if (cmeError == CME_SUCC)
            {
                if (atGetNumValue(pParamList, 1, &cid,
                                  ATC_CGDATA_1_CID_VAL_MIN, ATC_CGDATA_1_CID_VAL_MAX, ATC_CGDATA_1_CID_VAL_DEFAULT) != AT_PARA_OK)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_EXEC, psCGDATA_cid_1, P_WARNING, 0,
                                "ATCMD: CGDATA, invalid param: <cid>");
                    cmeError = CME_INCORRECT_PARAM;
                }
            }

            if (cmeError == CME_SUCC)
            {
                ret = psEnterDataState(atHandle, (UINT8)cid);

                if (ret != CMS_RET_SUCC)
                {
                    cmeError = CME_UNKNOWN;
                }
            }

            if (ret != CMS_RET_SUCC)
            {
                OsaDebugBegin(cmeError != CME_SUCC, cmeError, ret, 0);
                cmeError = CME_UNKNOWN;
                OsaDebugEnd();

                ret = atcReply(atHandle, AT_RC_CME_ERROR, cmeError, PNULL);
            }

            break;
        }

        case AT_READ_REQ:           /* AT+CGDATA?  */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCGDCONT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGDCONT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    DefineBearerCtxInfo pdpCtxInfo;
    INT32   primCid=0;
    UINT8   tmpStr[ATC_CGDCONT_2_APN_STR_MAX_LEN + CMS_NULL_CHAR_LEN] = {0};   //101 bytes
    UINT16  tmpStrLen = 0;
    INT32   tmpParam = 0;
    CHAR    tmpAtRspStr[ATEC_IND_RESP_128_STR_LEN] = {0};   //128 bytes


    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CGDCONT=? */
        {
            /* +CGDCONT: (range of supported<cid>s),<PDP_type>,,,(list of supported <d_comp>s),
             *           (list of supported <h_comp>s),(list of supported <IPv4AddrAlloc>s),
             *           (list of supported <request_type>s),(list of supported <PCSCF_discovery>s),
             *           (list of supported <IM_CN_Signalling_Flag_Ind>s),(list of supported <NSLPI>s),
             *           (list of supported <securePCO>s),(list of supported <IPv4_MTU_discovery>s),
             *           (list of supported <Local_Addr_Ind>s),(list of supported <NonIP_MTU_discovery>s),
             *           (list of supported <Reliable_Data_Service>s)
             * [<CR><LF>
             * +CGDCONT: (range of supported<cid>s),<PDP_type>,,,(list of supported <d_comp>s),
             *           (list of supported <h_comp>s),(list of supported <IPv4AddrAlloc>s),
             *           (list of supported <request_type>s),(list of supported <PCSCF_discovery>s),
             *           (list of supported <IM_CN_Signalling_Flag_Ind>s),(list of supported <NSLPI>s),
             *           (list of supported <securePCO>s),(list of supported <IPv4_MTU_discovery>s),
             *           (list of supported <Local_Addr_Ind>s),(list of supported <NonIP_MTU_discovery>s),
             *           (list of supported <Reliable_Data_Service>s) [...]]
             *
             * If the MT supports several PDP types, <PDP_type>, the parameter value ranges for each <PDP_type>
             *   are returned on a separate line
             */
            snprintf(tmpAtRspStr, ATEC_IND_RESP_128_STR_LEN,
                     "\r\n+CGDCONT: (0-10),\"IP\",,,,,(0),(0,2),(0),(0),(0,1),(0),(0,1),(0),(0)");
            /*                   cids, type, ,,, d,  h, allo,ty, pc, im,nsl,sec,i4mtu,loc,nmtu,rel */
            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)tmpAtRspStr);

            snprintf(tmpAtRspStr, ATEC_IND_RESP_128_STR_LEN,
                     "\r\n+CGDCONT: (0-10),\"IPV6\",,,,,(0),(0,2),(0),(0),(0,1),(0),(0),(0),(0)");
            /*                   cid,  type,   ,,, d,  h, allo,ty, pc, im,nsl,sec,i4mtu,loc,nmtu,rel */
            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)tmpAtRspStr);

            snprintf(tmpAtRspStr, ATEC_IND_RESP_128_STR_LEN,
                     "\r\n+CGDCONT: (0-10),\"IPV4V6\",,,,,(0),(0,2),(0),(0),(0,1),(0),(0,1),(0),(0)");
            /*                   cid,  type,     ,,, d,  h, allo,ty, pc, im,nsl,sec,i4mtu,loc,nmtu,rel */
            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)tmpAtRspStr);

            snprintf(tmpAtRspStr, ATEC_IND_RESP_128_STR_LEN,
                     "\r\n+CGDCONT: (0-10),\"Non-IP\",,,,,(0),(0,2),(0),(0),(0,1),(0),(0),(0),(0,1)\r\n");
            /*                   cid,  type,     ,,, d,  h, allo,ty, pc, im,nsl,sec,i4mtu,loc,nmtu,rel */

            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)tmpAtRspStr);

            break;
        }

        case AT_READ_REQ:    /* AT+CGDCONT?  */
        {
            ret = psGetCGDCONTCid(atHandle);
            break;
        }

        case AT_SET_REQ:      /* AT+CGDCONT= */
        {
            memset(&pdpCtxInfo, 0, sizeof(pdpCtxInfo));

            /* 1  Extract the arguments starting with the <0:CID> */
            if (atGetNumValue(pParamList, 0, &primCid, ATC_CGDCONT_0_CID_VAL_MIN, ATC_CGDCONT_0_CID_VAL_MAX, ATC_CGDCONT_0_CID_VAL_DEFAULT) == AT_PARA_ERR)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, 0);
                break;
            }
            pdpCtxInfo.cid = primCid;

            /* If only the CID is included, we should undefine this PDP Context */
            if(pParamList[1].bDefault && pParamList[2].bDefault && pParamList[3].bDefault
               && pParamList[4].bDefault && pParamList[5].bDefault )
            {
                ret = psDeleteCGDCONTContext(atHandle, primCid);
                break;
            }

            /* 2.  Determine the PDP type. <1:pdp_type>     */
            if(atGetStrValue(pParamList, 1, tmpStr, 10, &tmpStrLen, (CHAR *)ATC_CGDCONT_1_PDPTYPE_STR_DEFAULT) != AT_PARA_ERR)
            {
                if(tmpStrLen == 0)
                {
                    /* default value */
                    pdpCtxInfo.pdnType = CMI_PS_PDN_TYPE_IP_V4;
                }
                else
                {
                    if(strcasecmp("IP", (char *)tmpStr) == 0)
                    {
                        pdpCtxInfo.pdnType = CMI_PS_PDN_TYPE_IP_V4;
                    }
                    else if(strcasecmp("IPV6", (char *)tmpStr) == 0)
                    {
                        pdpCtxInfo.pdnType = CMI_PS_PDN_TYPE_IP_V6;
                    }
                    else if(strcasecmp("IPV4V6", (char *)tmpStr) == 0)
                    {
                        pdpCtxInfo.pdnType = CMI_PS_PDN_TYPE_IP_V4V6;
                    }
                    else if(strcasecmp("NON-IP", (char *)tmpStr) == 0)
                    {
                        pdpCtxInfo.pdnType = CMI_PS_PDN_TYPE_NON_IP;
                    }
                    else
                    {
                        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, 0);
                        break;
                    }
                }
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, 0);
                break;
            }

            /*   ** 3 Assign the APN string. <2:APN>                */
            if(atGetStrValue(pParamList, 2, tmpStr, ATC_CGDCONT_2_APN_STR_MAX_LEN, &tmpStrLen, ATC_CGDCONT_2_APN_STR_DEFAULT) != AT_PARA_ERR)
            {
                if(tmpStrLen != 0)
                {
                    if(atCheckApnName(tmpStr,tmpStrLen) == FALSE)
                    {
                        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_ALLOW, 0);
                        break;
                    }
                    pdpCtxInfo.apnLength = tmpStrLen;
                    memcpy( pdpCtxInfo.apnStr, tmpStr, tmpStrLen );
                }
            }
            else if(pParamList[2].value.pStr == PNULL)
            {
                 //let empty Apn continue.
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, 0);
                break;
            }
            /* 4.  Extract the PDP Address. <3:PDP address>                  */
            /* "PDP_addr" is ignored with the set command */
            /* 5.  Now get the D_COMP.  <4:d_comp>           */
            /* 6.  Now get the H_COMP.  <5:h_comp>   */
            /* "d_comp" & "h_comp" is for 2/3G, don't need */
            /*     ** 7 IPv4AddrAlloc .   <6:ipv4 address alloc>               */
            if(atGetNumValue(pParamList, 6, &tmpParam, ATC_CGDCONT_6_IPV4ADDRALLOC_VAL_MIN, ATC_CGDCONT_6_IPV4ADDRALLOC_VAL_MAX, ATC_CGDCONT_6_IPV4ADDRALLOC_VAL_DEFAULT) != AT_PARA_ERR)
            {
                pdpCtxInfo.ipv4AlloTypePresent = (tmpParam == ATC_CGDCONT_6_IPV4ADDRALLOC_VAL_DEFAULT ? FALSE:TRUE);
                pdpCtxInfo.ipv4AlloType = tmpParam;
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, 0);
                break;
            }

            /*     ** 8 request type.only support 0,2    <7:request type>               */
            if(atGetNumValue(pParamList, 7, &tmpParam, ATC_CGDCONT_7_PARA_VAL_MIN, ATC_CGDCONT_7_PARA_VAL_MAX, ATC_CGDCONT_7_PARA_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if(tmpParam == 1)
                {
                    ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, 0);
                    break;
                }
                pdpCtxInfo.reqTypePresent = TRUE;
                pdpCtxInfo.reqType = tmpParam;
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, 0);
                break;
            }

            /*     ** 9 how the MT requests to get the P-CSCF address.  <8:P-CSCF address>               */
            if(atGetNumValue(pParamList, 8, &tmpParam, ATC_CGDCONT_8_PARA_VAL_MIN, ATC_CGDCONT_8_PARA_VAL_MAX, ATC_CGDCONT_8_PARA_VAL_DEFAULT) != AT_PARA_ERR)
            {
              pdpCtxInfo.pcscfDisTypePresent = (tmpParam == ATC_CGDCONT_8_PARA_VAL_DEFAULT ? FALSE:TRUE);
              pdpCtxInfo.pcscfDisType = tmpParam;
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, 0);
                break;
            }

            /*     ** 10 whether the PDP context is for IM CN subsystem-related signalling only or not. <9:IM CN  signalling  flag ind>                  */
            if(atGetNumValue(pParamList, 9, &tmpParam, ATC_CGDCONT_9_PARA_VAL_MIN, ATC_CGDCONT_9_PARA_VAL_MAX, ATC_CGDCONT_9_PARA_VAL_DEFAULT) != AT_PARA_ERR)
            {
              pdpCtxInfo.imCnSigFlagPresent = (tmpParam == ATC_CGDCONT_9_PARA_VAL_DEFAULT ? FALSE:TRUE);
              pdpCtxInfo.imCnSigFlag = tmpParam;
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, 0);
                break;
            }

            /*     ** 11 NAS signalling priority requested for this PDP context. <10:NSLPI>               */
            if(atGetNumValue(pParamList, 10, &tmpParam, ATC_CGDCONT_10_PARA_VAL_MIN, ATC_CGDCONT_10_PARA_VAL_MAX, ATC_CGDCONT_10_PARA_VAL_DEFAULT) != AT_PARA_ERR)
            {
              pdpCtxInfo.NSLPIPresent = (tmpParam == ATC_CGDCONT_10_PARA_VAL_DEFAULT ? FALSE:TRUE);
              pdpCtxInfo.NSLPI = tmpParam;
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, 0);
                break;
            }

            /*     ** 12 security protected transmission of PCO.        <11:security PCO>             */
            /*     ** 13 how the MT requests to get the IPv4 MTU size.  <12:ipv4 MTU discovery>            */
            if(atGetNumValue(pParamList, 12, &tmpParam, ATC_CGDCONT_12_PARA_VAL_MIN, ATC_CGDCONT_12_PARA_VAL_MAX, ATC_CGDCONT_12_PARA_VAL_DEFAULT) != AT_PARA_ERR)
            {
              pdpCtxInfo.ipv4MtuDisTypePresent = (tmpParam == ATC_CGDCONT_12_PARA_VAL_DEFAULT ? FALSE:TRUE);
              pdpCtxInfo.ipv4MtuDisByNas = tmpParam;
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, 0);
                break;
            }

            /*     ** 14 whether or not MS supports local IP address.  <13:local address ind>              */
            /*     ** 15 how the MT requests to get the Non-IP MTU size.  <14:Non-IP MTU size>               */
            if(atGetNumValue(pParamList, 14, &tmpParam, ATC_CGDCONT_14_PARA_VAL_MIN, ATC_CGDCONT_14_PARA_VAL_MAX, ATC_CGDCONT_14_PARA_VAL_DEFAULT) != AT_PARA_ERR)
            {
              pdpCtxInfo.nonIpMtuDisTypePresent = (tmpParam == ATC_CGDCONT_14_PARA_VAL_DEFAULT ? FALSE:TRUE);
              pdpCtxInfo.nonIpMtuDisByNas = tmpParam;
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, 0);
                break;
            }
            ret = psSetCdgcont(atHandle, &pdpCtxInfo);
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCGDSCONT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGDSCONT(const AtCmdInputContext *pAtCmdReq)
{
    return CMS_FAIL;
}

/**
  \fn          CmsRetId psCEQOS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCEQOS(const AtCmdInputContext *pAtCmdReq)
{
    #if 0
    CmsRetId ret = CMS_FAIL;
    BOOL cmdValid = FALSE;
    INT32 cid;
       //cmi not define the data struct

    CiPs3GQosProfile qosProfile;
    UINT32 atHandle = MAKE_AT_HANDLE( *(ATCDecoderID *)arg_p );

    *xidPtr = atHandle;

    //DBGMSG("%s: atHandle = %d.\n", __FUNCTION__, atHandle);

    switch (operaType)
    {
    case AT_READ_REQ:    /* AT+CEQOS?  */
    {
        ret = PS_GetCEQOSQualityOfServiceList(atHandle, CI_PS_3G_QOSTYPE_REQ);
        break;
    }

    case AT_TEST_REQ:    /* AT+CEQOS=? */
    {
        ret = PS_GetCEQOSCapsQos(atHandle, CI_PS_3G_QOSTYPE_REQ);
        break;
    }

    case AT_SET_REQ:      /* AT+CEQOS= */
    {
        /*
        **  Extract the arguments starting with the CID.
        */
        if (( atGetNumValue(parameter_values_p, 0, &cid, ATC_CGEQREQ_0_CID_VAL_MIN, ATC_CGEQREQ_0_CID_VAL_MAX, ATC_CGEQREQ_0_CID_VAL_DEFAULT) != AT_PARA_ERR)  &&
            ( cid > 0 ) && ( cid <= CI_PS_MAX_PDP_CTX_NUM ))
        {

            cmdValid = getAtQosParams(parameter_values_p, &qosProfile);

            if ( cmdValid == TRUE )
                ret = PS_SetCEQOSQualityOfService(atHandle, cid - 1, &qosProfile, CI_PS_3G_QOSTYPE_REQ);

        }       /*  if (( atGetNumValue(parameter_values_p, X, &cid */
        break;
    }

    case AT_EXEC_REQ:   /* AT+CEQOS */
    default:
    {
        break;
    }
    }


    /* handle the return value */

    #endif
    return CMS_FAIL;
}

/**
  \fn          CmsRetId psCGEQOS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGEQOS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 cid = 0;
    INT32 qci = 0;
    INT32 dlgbr = 0;
    INT32 ulgbr = 0;
    INT32 dlmbr = 0;
    INT32 ulmbr = 0;
    CHAR RspBuf[64]={0};
    CmiPsEpsQosParams params;
    memset(&params,0,sizeof(CmiPsEpsQosParams));

    switch (operaType)
    {
        case AT_TEST_REQ:     /* AT+CGEQOS=? */
            //CMI_PS_GET_EPS_QOS_SETTING_CAPA_REQ
            sprintf((char*)RspBuf, "+CGEQOS: (0-10),(0,1-4,5-9,75,79)");
            ret = atcReply(atHandle, AT_RC_OK, 0, (char*)RspBuf);

            break;
        case AT_READ_REQ:      /* AT+CGEQOS? */
        {
            ret = psGetCGEQOSQualityService(atHandle);
            break;
        }

        case AT_SET_REQ:      /* AT+CGEQOS= */
        {
            /*
            **  Extract the arguments starting with the CID.
            */
            if(atGetNumValue(pParamList, 0, &cid, ATC_CGEQOS_0_CID_VAL_MIN, ATC_CGEQOS_0_CID_VAL_MAX, ATC_CGEQOS_0_CID_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if(pParamList[1].bDefault && pParamList[2].bDefault && pParamList[3].bDefault && pParamList[4].bDefault && pParamList[5].bDefault)
                {
                    params.cid = cid;
                    ret = psDeleteCGEQOSQualityService(atHandle, cid);
                }
                else
                {
                    if(atGetNumValue(pParamList, 1, &qci, ATC_CGEQOS_1_QCI_VAL_MIN, ATC_CGEQOS_1_QCI_VAL_MAX, ATC_CGEQOS_1_QCI_VAL_DEFAULT) != AT_PARA_ERR)
                    {
                        if(((qci == 0)||(qci >= 5 && qci <= 9)||(qci == 79))&&(pParamList[2].bDefault && pParamList[3].bDefault && pParamList[4].bDefault && pParamList[5].bDefault))
                        {
                            params.cid = cid;
                            params.qci = qci;
                            params.gbrMbrPresent = FALSE;
                            ret = psSetCGEQOSQualityService(atHandle, &params);
                        }
                        else if((qci > 0 && qci < 5)||(qci == 75))
                        {
                            if(atGetNumValue(pParamList, 2, &dlgbr, ATC_CGEQOS_2_DLGBR_VAL_MIN, ATC_CGEQOS_2_DLGBR_VAL_MAX, ATC_CGEQOS_2_DLGBR_VAL_DEFAULT) != AT_PARA_ERR)
                            {
                                if(atGetNumValue(pParamList, 3, &ulgbr, ATC_CGEQOS_3_ULGBR_VAL_MIN, ATC_CGEQOS_3_ULGBR_VAL_MAX, ATC_CGEQOS_3_ULGBR_VAL_DEFAULT) != AT_PARA_ERR)
                                {
                                    if(atGetNumValue(pParamList, 4, &dlmbr, ATC_CGEQOS_4_DLMBR_VAL_MIN, ATC_CGEQOS_4_DLMBR_VAL_MAX, ATC_CGEQOS_4_DLMBR_VAL_DEFAULT) != AT_PARA_ERR)
                                    {
                                        if(atGetNumValue(pParamList, 5, &ulmbr, ATC_CGEQOS_5_ULMBR_VAL_MIN, ATC_CGEQOS_5_ULMBR_VAL_MAX, ATC_CGEQOS_5_ULMBR_VAL_DEFAULT) != AT_PARA_ERR)
                                        {
                                            params.cid = cid;
                                            params.qci = qci;
                                            params.gbrMbrPresent = TRUE;
                                            params.dlGBR = dlgbr;
                                            params.dlMBR = dlmbr;
                                            params.ulGBR = ulgbr;
                                            params.ulMBR = ulmbr;
                                            ret = psSetCGEQOSQualityService(atHandle, &params);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if(ret == CMS_RET_SUCC)
            {
                //CGEQOS cnf will call atcReply
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCGEQOSRDP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGEQOSRDP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    UINT8   cid = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CGEQOSRDP=? */
        {
            ret = psGetCGEQOSRDPCapsQos(atHandle);
            break;
        }

        case AT_SET_REQ:      /* AT+CGEQOSRDP= */
        {
            if (atGetNumValue(pParamList, 0, (INT32*)&cid,
                              ATC_CGEQOSRDP_0_CID_VAL_MIN, ATC_CGEQOSRDP_0_CID_VAL_MAX, ATC_CGEQOSRDP_0_CID_VAL_DEFAULT) == AT_PARA_OK)
            {
                ret = psSetCGEQOSRDPQualityService(atHandle, cid, FALSE);
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:   /* AT+CGEQOSRDP */
        {
            /*
             * If the parameter <cid> is omitted, the Quality of Service parameters for all secondary and non secondary active PDP
             *  contexts are returned.
            */
            ret = psSetCGEQOSRDPQualityService(atHandle, 0, TRUE);
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId psCGCONTRDP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGCONTRDP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT8 cid;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CGCONTRDP=? */
        {
            ret = psGetCGCONTRDPCaps(atHandle);
            break;
        }

        case AT_SET_REQ:      /* AT+CGCONTRDP= */
        {
            if (pParamList[0].bDefault == FALSE &&
                atGetNumValue(pParamList, 0, (INT32*)&cid, ATC_CGCONTRDP_0_CID_VAL_MIN, ATC_CGCONTRDP_0_CID_VAL_MAX, ATC_CGCONTRDP_0_CID_VAL_DEFAULT) != AT_PARA_ERR)
            {
                ret = psGetCGCONTRDPParam(atHandle, cid);
            }

            if(ret == CMS_RET_SUCC)
            {
                //CGCONTRDP cnf will call atcReply
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:   /* AT+CGCONTRDP */
        {
            ret = psGetAllCGCONTRDPParam(atHandle);
            break;
        }
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCEREG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCEREG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 regParam;

    switch (operaType)
    {
        case AT_TEST_REQ:              /* AT+CEREG=? */
        {
            ret = psQueryERegOption(atHandle);
            break;
        }

        case AT_READ_REQ:              /* AT+CEREG? */
        {
            ret = psGetERegStatus(atHandle);
            break;
        }

        case AT_SET_REQ:              /* AT+CEREG= */
        {
            if (atGetNumValue(pParamList, 0, &regParam, ATC_CEREG_0_VAL_MIN, ATC_CEREG_0_VAL_MAX, ATC_CEREG_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CEREG_RPT_CFG, regParam);

                ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
           break;
        }

        case AT_EXEC_REQ:              /* AT+CEREG */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCSCON(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCSCON(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   rptMode = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:              /* AT+CSCON=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, "+CSCON: (0,1)");
            break;
        }

        case AT_READ_REQ:              /* AT+CSCON? */
        {
            ret = psGetCSCON(atHandle);
            break;
        }

        case AT_SET_REQ:              /* AT+CSCON= */
        {
            /*AT+CSCON=[<n>]*/
            if (pParamList[0].bDefault == FALSE &&
                atGetNumValue(pParamList, 0, &rptMode,
                              ATC_CSCON_0_STATE_VAL_MIN, ATC_CSCON_0_STATE_VAL_MAX, ATC_CSCON_0_STATE_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if (rptMode <= ATC_CSCON_RPT_MODE)   //only 0 - 1 support now
                {
                    mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CSCON_RPT_CFG, rptMode);

                    ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                }
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }

            break;
        }

        case AT_EXEC_REQ:              /* AT+CSCON */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCGTFT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGTFT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CmiPsTftFilter tftFilter, *pFilter=NULL;
    PsilPsAddrTempInfo pdp_addr;
    UINT8 add_len=0;
    UINT8 mask_len=0;
    UINT8 strAddress[ATC_CGTFT_3_ADDRESS_STR_MAX_LEN + CMS_NULL_CHAR_LEN];
    UINT8 strDestPort[ATC_CGTFT_5_PORT_STR_MAX_LEN + CMS_NULL_CHAR_LEN];
    UINT8 strSrcPort[ATC_CGTFT_6_PORT_STR_MAX_LEN + CMS_NULL_CHAR_LEN];
    UINT8 strIpsec[ATC_CGTFT_7_IPSEC_STR_MAX_LEN + CMS_NULL_CHAR_LEN];
    UINT8 strTos[ATC_CGTFT_8_TOS_STR_MAX_LEN + CMS_NULL_CHAR_LEN];
    UINT8 strFlowLable[ATC_CGTFT_9_FLOWLABLE_STR_MAX_LEN + CMS_NULL_CHAR_LEN];
    INT32 cid = 0, pfid = 0, epindex = 0, protocolNum = 0, iNumOfDots = 0, index = 0, minVal = 0, maxVal = 0;
    UINT16 addressLen = 0, destPortLen = 0, srcPortLen = 0, ipsecLen = 0, tosLen = 0, flowLableLen = 0;
    BOOL bFoundDot = FALSE, bContinue = TRUE;
    INT32 direction = 0;

    memset((CHAR*)&pdp_addr, 0, sizeof(pdp_addr));
    memset((CHAR*)strAddress, 0, sizeof(strAddress));
    memset((CHAR*)strDestPort, 0, sizeof(strDestPort));
    memset((CHAR*)strSrcPort, 0, sizeof(strSrcPort));
    memset((CHAR*)strIpsec, 0, sizeof(strIpsec));
    memset((CHAR*)strTos, 0, sizeof(strTos));
    memset((CHAR*)strFlowLable, 0, sizeof(strFlowLable));

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CGTFT=? */
        {
            ret = atcReply(atHandle,AT_RC_CONTINUE,0,(char *)"+CGTFT:\"IP\",(1-16),(0-255),(\"0.0.0.0.0.0.0.0\"-\"255.255.255.255.255.255.255.255\"),(0-255),(\"0.0\"-\"65535.65535\"),(\"0.0\"-\"65535.65535\"),(0-FFFFFFFF),(\"0.0\"-\"255.255\"),,(0-3)");
            ret = atcReply(atHandle,AT_RC_OK,0,(char *)"+CGTFT:\"IPV6\",(1-16),(0-255),(\"0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0\"-\"255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255.255\"),(0-255),(\"0.0\"-\"65535.65535\"),(\"0.0\"-\"65535.65535\"),(0-FFFFFFFF),(\"0.0\"-\"255.255\"),(0-FFFFF),(0-3)");
            break;
        }

        case AT_READ_REQ:    /* AT+CGTFT? */
        {
            ret = psGetCGTFT(atHandle);
            break;
        }

        case AT_SET_REQ:      /* AT+CGTFT= */
        {
            if(atGetNumValue(pParamList, 0, &cid, ATC_CGTFT_0_CID_VAL_MIN, ATC_CGTFT_0_CID_VAL_MAX, ATC_CGTFT_0_CID_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if(pParamList[1].bDefault && pParamList[2].bDefault && pParamList[3].bDefault && pParamList[4].bDefault && pParamList[5].bDefault && pParamList[6].bDefault && pParamList[7].bDefault && pParamList[8].bDefault && pParamList[9].bDefault)
                {//CGTFT = <cid> -> delete reqest
                    ret = psDeleteTft(atHandle, cid);
                }
                else //set reqest process user parameter to ci reqest
                {
                    if(pParamList[1].bDefault == FALSE)
                    {
                        if(atGetNumValue(pParamList, 1, &pfid, ATC_CGTFT_1_PFID_VAL_MIN, ATC_CGTFT_1_PFID_VAL_MAX, ATC_CGTFT_1_PFID_VAL_DEFAULT) != AT_PARA_ERR)
                        {
                            if(atGetNumValue(pParamList, 2,&epindex , ATC_CGTFT_2_EPINDEX_VAL_MIN, ATC_CGTFT_2_EPINDEX_VAL_MAX, ATC_CGTFT_2_EPINDEX_VAL_DEFAULT) != AT_PARA_ERR)
                            {
                                pFilter = &tftFilter;
                                pFilter->cid = cid;
                                pFilter->pfId = (UINT8)pfid;            /*Packet Filter ID*/
                                pFilter->epIndex = (UINT8)epindex;      /*Evaluation precedence index*/

                                if(atGetStrValue(pParamList, 3, strAddress, ATC_CGTFT_3_ADDRESS_STR_MAX_LEN, &addressLen , (CHAR *)ATC_CGTFT_3_ADDRESS_STR_DEFAULT) != AT_PARA_ERR)
                                {
                                    /*source address and sub netmask*/
                                    /********************************/
                                    if(pParamList[3].bDefault == FALSE)
                                    {
                                        UINT8 * strAddressMask = &strAddress[index];
                                        for(index = 0; index < addressLen; index++)
                                        {
                                            if(strAddress[index] == '.')
                                            {
                                                iNumOfDots++;
                                                if(iNumOfDots == 4)
                                                strAddressMask = &strAddress[index + 1];
                                                else if(iNumOfDots == 16)
                                                strAddressMask = &strAddress[index + 1];
                                            }
                                        }

                                        switch (iNumOfDots)
                                        {
                                            case 3: //IPv4 Src Addres only
                                            {
                                                pFilter->remoteAddrAndMask.subnetLength = 0;
                                                pFilter->remoteAddrAndMask.addrType = CMI_IPV4_ADDR;
                                                add_len = ((UINT8)(addressLen) < 64 ? (UINT8)(addressLen) : 63);
                                                memcpy(pdp_addr.valData, strAddress, add_len);
                                                pdp_addr.valData[add_len] = 0;
                                                if(psIpStrAddrToHexAddr(&pdp_addr))
                                                    memcpy(pFilter->remoteAddrAndMask.addrData, pdp_addr.valData, pdp_addr.len);
                                                else
                                                    bContinue = FALSE;
                                                break;
                                            }
                                            case 15: //IPv6 Src Addres only
                                            {
                                                pFilter->remoteAddrAndMask.subnetLength = 0;
                                                pFilter->remoteAddrAndMask.addrType = CMI_FULL_IPV6_ADDR;
                                                add_len = ((UINT8)(addressLen) < 64 ? (UINT8)(addressLen) : 63);
                                                memcpy(pdp_addr.valData, strAddress, add_len);
                                                pdp_addr.valData[add_len] = 0;
                                                if(psIpStrAddrToHexAddr(&pdp_addr))
                                                    memcpy(pFilter->remoteAddrAndMask.addrData, pdp_addr.valData, pdp_addr.len);
                                                else
                                                    bContinue = FALSE;
                                                break;
                                            }
                                            case 7: //IPv4 Src Addres and Sub net mask
                                            {
                                                pFilter->remoteAddrAndMask.addrType = CMI_IPV4_ADDR;
                                                add_len = ((UINT8)(strAddressMask - strAddress - 1) < 64 ? (UINT8)(strAddressMask - strAddress - 1) : 63);
                                                memcpy(pdp_addr.valData, strAddress, add_len);
                                                pdp_addr.valData[add_len] = 0;
                                                if(psIpStrAddrToHexAddr(&pdp_addr))
                                                    memcpy(pFilter->remoteAddrAndMask.addrData, pdp_addr.valData, pdp_addr.len);
                                                else
                                                    bContinue = FALSE;
                                                if((mask_len = (UINT8)psNetMaskStrToLen((char* )strAddressMask)) > 0)
                                                    pFilter->remoteAddrAndMask.subnetLength = mask_len;
                                                else
                                                    bContinue = FALSE;
                                                break;
                                            }
                                            case 31: //IPv6 Src Addres and Sub net mask
                                                {
                                                    pFilter->remoteAddrAndMask.addrType = CMI_FULL_IPV6_ADDR;
                                                    add_len = ((UINT8)(strAddressMask - strAddress - 1) < 64 ? (UINT8)(strAddressMask - strAddress - 1) : 63);
                                                    memcpy(pdp_addr.valData, strAddress, add_len);
                                                    pdp_addr.valData[add_len] = 0;
                                                    if(psIpStrAddrToHexAddr(&pdp_addr))
                                                        memcpy(pFilter->remoteAddrAndMask.addrData, pdp_addr.valData, pdp_addr.len);
                                                    else
                                                        bContinue = FALSE;
                                                    if((mask_len = (UINT8)psNetMaskStrToLen((char* )strAddressMask)) > 0)
                                                        pFilter->remoteAddrAndMask.subnetLength = mask_len;
                                                    else
                                                        bContinue = FALSE;
                                                    break;
                                                }
                                            default:
                                                bContinue = FALSE;
                                                break;
                                        }
                                    }
                                }
                                if(bContinue)
                                {
                                    if(atGetNumValue(pParamList, 4, &protocolNum, ATC_CGTFT_4_PROTOCOLNUM_VAL_MIN, ATC_CGTFT_4_PROTOCOLNUM_VAL_MAX, ATC_CGTFT_4_PROTOCOLNUM_VAL_DEFAULT) != AT_PARA_ERR)
                                    {
                                        /*protocol number (ipv4) / next header (ipv6)*/
                                        if(pParamList[4].bDefault == FALSE)
                                        {
                                            pFilter->pIdNextHdr = (UINT8) protocolNum;
                                            pFilter->pIdNextHdrPresent = TRUE;
                                        }
                                        else
                                            pFilter->pIdNextHdrPresent = FALSE;
                                    }
                                    else
                                        bContinue = FALSE;
                                }


                                /*source port range*/
                                /*******************/
                                if(bContinue)
                                {
                                    if(atGetStrValue(pParamList, 5, strSrcPort, ATC_CGTFT_5_PORT_STR_MAX_LEN, &srcPortLen , (CHAR *)ATC_CGTFT_5_PORT_STR_DEFAULT) != AT_PARA_ERR)
                                    {

                                        if(pParamList[5].bDefault == FALSE)
                                        {
                                            bFoundDot = FALSE;
                                            for(index = 0; index < srcPortLen && bFoundDot == FALSE; index++)
                                            {
                                                if(strSrcPort[index] == '.')
                                                {
                                                    if(index + 1 < srcPortLen)
                                                    {
                                                        minVal = atoi((char*)strSrcPort);
                                                        maxVal = atoi((char*)&strSrcPort[index+1]);
                                                        bFoundDot = TRUE;
                                                    }
                                                }
                                            }
                                            if(bFoundDot)
                                            {
                                                if(minVal >= 0 && minVal <= MAX_PORT_SUPPORT && maxVal >= 0 && maxVal <= MAX_PORT_SUPPORT)
                                                {
                                                    pFilter->srcPortRange.min = (UINT32)minVal;
                                                    pFilter->srcPortRange.max = (UINT32)maxVal;
                                                    pFilter->srcPortRangePresent = TRUE;
                                                }
                                                else
                                                    bContinue = FALSE;
                                            }
                                            else
                                                bContinue = FALSE;
                                        }
                                        else
                                            pFilter->srcPortRangePresent = FALSE;
                                    }
                                }

                                /*destination port range*/
                                /************************/
                                if(bContinue)
                                {
                                    if(atGetStrValue(pParamList, 6, strDestPort, ATC_CGTFT_6_PORT_STR_MAX_LEN, &destPortLen , (CHAR *)ATC_CGTFT_6_PORT_STR_DEFAULT) != AT_PARA_ERR)
                                    {
                                        if(pParamList[6].bDefault == FALSE)
                                        {
                                            bFoundDot = FALSE;
                                            for(index = 0; index < destPortLen && bFoundDot == FALSE; index++)
                                            {
                                                if(strDestPort[index] == '.')
                                                {
                                                    if(index + 1 < destPortLen)
                                                    {
                                                        minVal = atoi((char*)strDestPort);
                                                        maxVal = atoi((char*)&strDestPort[index+1]);
                                                        bFoundDot = TRUE;
                                                    }
                                                }
                                            }
                                            if(bFoundDot)
                                            {
                                                if(minVal >= 0 && minVal <= MAX_PORT_SUPPORT && maxVal >= 0 && maxVal <= MAX_PORT_SUPPORT)
                                                {
                                                    pFilter->dstPortRange.min = (UINT32)minVal ;
                                                    pFilter->dstPortRange.max = (UINT32)maxVal;
                                                    pFilter->dstPortRangePresent = TRUE;
                                                }
                                                else
                                                    bContinue = FALSE;
                                            }
                                            else
                                                bContinue = FALSE;
                                        }
                                        else
                                            pFilter->dstPortRangePresent = FALSE;
                                    }
                                    else
                                        bContinue = FALSE;
                                }

                                /*ipsec security parameter index (spi)*/
                                /**************************************/
                                if(bContinue)
                                {
                                    if(atGetStrValue(pParamList, 7, strIpsec, ATC_CGTFT_7_IPSEC_STR_MAX_LEN, &ipsecLen , (CHAR *)ATC_CGTFT_7_IPSEC_STR_DEFAULT) != AT_PARA_ERR)
                                    {
                                        if(pParamList[7].bDefault == FALSE)
                                        {
                                            if(cmsHexStrToUInt(&pFilter->ipSecSPI, strIpsec, ipsecLen))
                                            {
                                                pFilter->ipSecSPIPresent = TRUE;
                                            }
                                            else
                                                bContinue = FALSE;
                                        }
                                        else
                                            pFilter->ipSecSPIPresent = FALSE;
                                    }
                                    else
                                        bContinue = FALSE;
                                }

                                /*type of service (tos) and mask (ipv4) / traffic class (ipv4) and mask*/
                                /***********************************************************************/
                                if(bContinue)
                                {
                                    if(atGetStrValue(pParamList, 8, strTos, ATC_CGTFT_8_TOS_STR_MAX_LEN, &tosLen , (CHAR *)ATC_CGTFT_8_TOS_STR_DEFAULT) != AT_PARA_ERR)
                                    {

                                        if(pParamList[8].bDefault == FALSE)
                                        {
                                            bFoundDot = FALSE;
                                            for(index = 0; index < tosLen && bFoundDot == FALSE; index++)
                                            {
                                                if(strTos[index] == '.')
                                                {
                                                    if(index + 1 < tosLen)
                                                    {
                                                        minVal = atoi((char*)strTos);
                                                        maxVal = atoi((char*)&strTos[index+1]);
                                                        bFoundDot = TRUE;
                                                    }
                                                }
                                            }
                                            if(bFoundDot)
                                            {
                                                if(minVal >= 0 && minVal <= 255 && maxVal >= 0 && maxVal <= 255)
                                                {
                                                    pFilter->tosTc = (UINT8)minVal ;
                                                    pFilter->tosTcMask = (UINT8)maxVal;
                                                    pFilter->tosPresent = TRUE;
                                                }
                                                else
                                                    bContinue = FALSE;
                                            }
                                            else
                                                bContinue = FALSE;
                                        }
                                        else
                                            pFilter->tosPresent = FALSE;
                                    }
                                    else
                                        bContinue = FALSE;
                                }

                                /*flow label (ipv6)*/
                                /*******************/
                                if(bContinue)
                                {
                                    if(atGetStrValue(pParamList, 9, strFlowLable, ATC_CGTFT_9_FLOWLABLE_STR_MAX_LEN, &flowLableLen , (CHAR *)ATC_CGTFT_9_FLOWLABLE_STR_DEFAULT) != AT_PARA_ERR)
                                    {
                                        if(pParamList[9].bDefault == FALSE)
                                        {
                                            if(cmsHexStrToUInt(&pFilter->flowLabel, strFlowLable, flowLableLen))
                                            {
                                                pFilter->flowLabelPresent = TRUE;
                                            }
                                            else
                                                bContinue = FALSE;
                                        }
                                        else
                                            pFilter->flowLabelPresent = FALSE;
                                    }
                                    else
                                        bContinue = FALSE;
                                }
                                if(bContinue)
                                {
                                    if(atGetNumValue(pParamList, 10, &direction, ATC_CGTFT_10_DIRECTION_VAL_MIN, ATC_CGTFT_10_DIRECTION_VAL_MAX, ATC_CGTFT_10_DIRECTION_VAL_DEFAULT) != AT_PARA_ERR)
                                    {
                                        /* mark command as valid */
                                        if(pParamList[10].bDefault == FALSE)
                                        {
                                            pFilter->direction = (UINT8)direction;
                                        }
                                        else
                                        {
                                            pFilter->direction = 3;
                                        }
                                    }
                                    else
                                        bContinue = FALSE;
                                }
                                /* Send CI reqest*/
                                if(bContinue)
                                {
                                    ret = psSetTftFilter(atHandle, pFilter);
                                }
                            }
                        }
                    }
                }
            }

            if(ret == CMS_RET_SUCC)
            {
                //CGTFT cnf will call atcReply
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:   /* AT+CGTFT */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCSODCP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCSODCP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   cid = 0, cpdLen = 0, cpdBufSize = 0, rai = 0, type = 0;
    UINT16  cpDataStrLen = 0;
    UINT8   *strCpdata = PNULL;
    MtErrorResultCode   errCode = CME_SUCC;

    switch (operaType)
    {
        case AT_TEST_REQ:   /* AT+CSODCP=? */
        {
            /*
             * +CSODCP: (range of supported <cid>s),(maximum number of octets of user data indicated by <cpdata_length>),
             *          (list of supported <RAI>s),(list of supported <type_of_user_data>s)
            */
            ret = atcReply(atHandle, AT_RC_OK, CME_SUCC, "+CSODCP: (0-10),(950),(0-2),(0,1)");
            break;
        }

        case AT_SET_REQ:      /* AT+CSODCP= */
        {
            /* +CSODCP=<cid>,<cpdata_length>,<cpdata>[,<RAI>[,<type_of_user_data>]] */
            if (atGetNumValue(pParamList, 0, &cid,
                              ATC_CSODCP_0_CID_VAL_MIN, ATC_CSODCP_0_CID_VAL_MAX, ATC_CSODCP_0_CID_VAL_DEFAULT) != AT_PARA_OK)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, psCSODCP_cid_1, P_WARNING, 0, "AT CMD: CSODCP, invalid input CID");
                errCode = CME_INCORRECT_PARAM;
            }

            /*<cpdata_length>*/
            if (errCode == CME_SUCC &&
                atGetNumValue(pParamList, 1, &cpdLen,
                              ATC_CSODCP_1_CPD_VAL_MIN, ATC_CSODCP_1_CPD_VAL_MAX, ATC_CSODCP_1_CPD_VAL_DEFAULT) != AT_PARA_OK)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, psCSODCP_length_1, P_WARNING, 0, "AT CMD: CSODCP, invalid input cpdata_length");
                errCode = CME_INCORRECT_PARAM;
            }

            /*
             * Calc string data buffer size, and alloc memory
             * example: cpdata_length = 2, cpdata = "34A8", so here, data string length = 4;
            */
            cpdBufSize = (cpdLen*2 + 1 + 3)&0xFFFFFFFC;     /*+1, for '\0', and +3 is for 4 bytes aligned*/
            strCpdata = (UINT8 *)OsaAllocMemory(cpdBufSize);

            if (strCpdata == PNULL)
            {
                OsaDebugBegin(FALSE, cpdLen, cid, cpdBufSize);
                OsaDebugEnd();
                errCode = CME_MEMORY_FULL;
            }

            /*<cpdata>*/
            if (errCode == CME_SUCC &&
                atGetStrValue(pParamList, 2, strCpdata,
                               cpdBufSize, &cpDataStrLen, ATC_CSODCP_2_CPDATA_STR_DEFAULT) != AT_PARA_OK)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, psCSODCP_cpdate_1, P_WARNING, 0, "AT CMD: CSODCP, invalid input cpdata");
                errCode = CME_INCORRECT_PARAM;
            }

            /*check the data length*/
            if (cpDataStrLen != cpdLen*2)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, psCSODCP_cpdate_2, P_WARNING, 0, "AT CMD: CSODCP, data length mismatched");
                errCode = CME_INCORRECT_PARAM;
            }

            /*
             * +CSODCP=<cid>,<cpdata_length>,<cpdata>[,<RAI>[,<type_of_user_data>]]
             * <RAI>, opt
            */
            if (errCode == CME_SUCC &&
                atGetNumValue(pParamList, 3, &rai,
                              ATC_CSODCP_3_RAI_VAL_MIN, ATC_CSODCP_3_RAI_VAL_MAX, ATC_CSODCP_3_RAI_VAL_DEFAULT) == AT_PARA_ERR)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, psCSODCP_RAI_1, P_WARNING, 0, "AT CMD: CSODCP, invalid input RAI");
                errCode = CME_INCORRECT_PARAM;
            }

            /*<type_of_user_data>*/
            if (errCode == CME_SUCC &&
                atGetNumValue(pParamList, 4, &type,
                              ATC_CSODCP_4_TYPE_VAL_MIN, ATC_CSODCP_4_TYPE_VAL_MAX, ATC_CSODCP_4_TYPE_VAL_DEFAULT) == AT_PARA_ERR)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, psCSODCP_type_1, P_WARNING, 0, "AT CMD: CSODCP, invalid input: <type_of_user_data>");
                errCode = CME_INCORRECT_PARAM;
            }

            if (errCode == CME_SUCC)
            {
                ret = psSendUserData(atHandle, cid, cpDataStrLen, strCpdata, rai, type, TRUE);

                if (ret != CMS_RET_SUCC)
                {
                    errCode = CME_MEMORY_FAILURE;   /*?*/
                }
            }

            /* free "strCpdata" */
            if (strCpdata != PNULL)
            {
                OsaFreeMemory(&strCpdata);
            }

            if (ret != CMS_RET_SUCC)
            {
                OsaDebugBegin(errCode != CME_SUCC, errCode, ret, 0);
                errCode = CME_UNKNOWN;
                OsaDebugEnd();

                ret = atcReply(atHandle, AT_RC_CME_ERROR, errCode, PNULL);
            }

            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psECATTBEARER(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psECATTBEARER(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   pdnType = 0, eitf = 0, ipv4alloc = 0, nslpi = 0, ipv4mtu = 0, noipmtu = 0, authPro = 0;
    UINT8   apnStr[CMI_PS_MAX_APN_LEN+1] = {0};
    UINT8   authUserName[CMI_PS_MAX_AUTH_STR_LEN+1] ={0};
    UINT8   authPassword[CMI_PS_MAX_AUTH_STR_LEN+1] ={0};
    UINT16  length = 0;
    AttachedBearCtxPreSetParam attBrParam;
    MtErrorResultCode cmeErr = CME_SUCC;

    memset(&attBrParam, 0, sizeof(AttachedBearCtxPreSetParam));

    switch (operaType)
    {
        case AT_READ_REQ:    /* AT+ECATTBEARER? */
        {
            ret = psGetATTBEAR(atHandle);
            break;
        }

        case AT_TEST_REQ:   /* AT+ECATTBEARER=? */
        {
            /*
             * +ECATTBEARER: (list of supported <PDP_type>s),(list of supported <eitf>s),(list of supported <IPv4AddrAlloc>s),
             *               (list of supported <NSLPI>s),(list of supported <IPv4_MTU_discovery>s),(list of supported <NonIP_MTU_discovery>s)
            */
            ret = atcReply(atHandle, AT_RC_OK, CME_SUCC, "+ECATTBEARER: (1,2,3,5),(0,1),(0),(0,1),(0,1),(0,1),(0,1),(""),("")");
            break;
        }

        case AT_SET_REQ:    /* AT+ECATTBEARER= */
        {
            /*
             * AT+ECATTBEARER=<PDP_type>[,<eitf>[,<apn>[,<IPv4AddrAlloc>[,<NSLPI>[,<IPv4_MTU_discovery>[,<NonIP_MTU_discovery>[,PAP,Auth_User_Name,Auth_Password]]]]]]]
            */
            if (atGetNumValue(pParamList, 0, &pdnType,
                              ATC_ECATTBEARER_0_PDN_VAL_MIN, ATC_ECATTBEARER_0_PDN_VAL_MAX, ATC_ECATTBEARER_0_PDN_VAL_DEFAULT) != AT_PARA_OK)
            {
                cmeErr = CME_INCORRECT_PARAM;
            }
            else
            {
                if (pdnType != CMI_PS_PDN_TYPE_IP_V4 && pdnType != CMI_PS_PDN_TYPE_IP_V6 &&
                    pdnType != CMI_PS_PDN_TYPE_IP_V4V6 && pdnType != CMI_PS_PDN_TYPE_NON_IP)
                {
                    OsaDebugBegin(FALSE, pdnType, 0, 0);
                    OsaDebugEnd();

                    cmeErr = CME_INCORRECT_PARAM;
                }
                else
                {
                    attBrParam.pdnType = (UINT8)pdnType;
                }
            }

            /*eitf*/
            if (cmeErr == CME_SUCC)
            {
                if (pParamList[1].bDefault)
                {
                    attBrParam.eitfPresent = FALSE;
                }
                else if (atGetNumValue(pParamList, 1, &eitf,
                                       ATC_ECATTBEARER_1_EITF_VAL_MIN, ATC_ECATTBEARER_1_EITF_VAL_MAX, ATC_ECATTBEARER_1_EITF_VAL_DEFAULT) == AT_PARA_OK)
                {
                    attBrParam.eitfPresent = TRUE;
                    attBrParam.eitf = (UINT8)eitf;
                }
                else
                {
                    cmeErr = CME_INCORRECT_PARAM;
                }
            }

            /*<apn>*/
            if (cmeErr == CME_SUCC)
            {
                if (pParamList[2].bDefault)
                {
                    attBrParam.apnLength = 0;
                }
                else if (atGetStrValue(pParamList, 2, apnStr,
                                        ATC_ECATTBEARER_2_APN_STR_MAX_LEN, &length, ATC_ECATTBEARER_2_APN_STR_DEFAULT) == AT_PARA_OK)
                {
                    attBrParam.apnLength = (length < CMI_PS_MAX_APN_LEN ? length : CMI_PS_MAX_APN_LEN);
                    strncpy((CHAR *)(attBrParam.apnStr), (CHAR *)apnStr, attBrParam.apnLength);
                }
                else
                {
                    cmeErr = CME_INCORRECT_PARAM;
                }

            }

            /*
             * AT+ECATTBEARER=<PDP_type>[,<eitf>[,<apn>[,<IPv4AddrAlloc>[,<NSLPI>[,<IPv4_MTU_discovery>[,<NonIP_MTU_discovery>]]]]]]
             * <IPv4AddrAlloc>
             */
            if (cmeErr == CME_SUCC)
            {
                if (pParamList[3].bDefault)
                {
                    attBrParam.ipv4AlloTypePresent = FALSE;
                }
                else if (atGetNumValue(pParamList, 3, &ipv4alloc,
                                       ATC_ECATTBEARER_3_IPV4ALLOC_VAL_MIN, ATC_ECATTBEARER_3_IPV4ALLOC_VAL_MAX, ATC_ECATTBEARER_3_IPV4ALLOC_VAL_DEFAULT)
                                       == AT_PARA_OK)
                {
                    attBrParam.ipv4AlloTypePresent = TRUE;
                    attBrParam.ipv4AlloType = (UINT8)ipv4alloc;
                }
                else
                {
                    cmeErr = CME_INCORRECT_PARAM;
                }
            }

            /* <NSLPI> */
            if (cmeErr == CME_SUCC)
            {
                if ((pParamList[4].bDefault))
                {
                    attBrParam.NSLPIPresent = FALSE;
                }
                else if (atGetNumValue(pParamList, 4, &nslpi,
                                       ATC_ECATTBEARER_4_NSLPI_VAL_MIN, ATC_ECATTBEARER_4_NSLPI_VAL_MAX, ATC_ECATTBEARER_4_NSLPI_VAL_DEFAULT)
                                       == AT_PARA_OK)
                {
                    attBrParam.NSLPIPresent = TRUE;
                    attBrParam.NSLPI = (UINT8)nslpi;
                }
                else
                {
                    cmeErr = CME_INCORRECT_PARAM;
                }
            }

            /* <IPv4_MTU_discovery> */
            if (cmeErr == CME_SUCC)
            {
                if ((pParamList[5].bDefault))
                {
                    attBrParam.ipv4MtuDisTypePresent = FALSE;
                }
                else if (atGetNumValue(pParamList, 5, &ipv4mtu,
                                       ATC_ECATTBEARER_5_IPV4MTU_VAL_MIN, ATC_ECATTBEARER_5_IPV4MTU_VAL_MAX, ATC_ECATTBEARER_5_IPV4MTU_VAL_DEFAULT)
                                       == AT_PARA_OK)
                {
                    attBrParam.ipv4MtuDisTypePresent = TRUE;
                    attBrParam.ipv4MtuDisByNas = (BOOL)ipv4mtu;
                }
                else
                {
                    cmeErr = CME_INCORRECT_PARAM;
                }
            }

            /*<NonIP_MTU_discovery>*/
            if (cmeErr == CME_SUCC)
            {
                if ((pParamList[6].bDefault))
                {
                    attBrParam.nonIpMtuDisTypePresent = FALSE;
                }
                else if (atGetNumValue(pParamList, 6, &noipmtu,
                                       ATC_ECATTBEARER_6_NOIPMTU_VAL_MIN, ATC_ECATTBEARER_6_NOIPMTU_VAL_MAX, ATC_ECATTBEARER_6_NOIPMTU_VAL_DEFAULT)
                                       == AT_PARA_OK)
                {
                    attBrParam.nonIpMtuDisTypePresent = TRUE;
                    attBrParam.nonIpMtuDisByNas = (BOOL)noipmtu;
                }
                else
                {
                    cmeErr = CME_INCORRECT_PARAM;
                }
            }


            /*<PAP,Auth_User_Name,Auth_Password>*/
            if (cmeErr == CME_SUCC)
            {
                if ((pParamList[7].bDefault))
                {
                    attBrParam.authProPresent = FALSE;
                }
                else if (atGetNumValue(pParamList, 7, &authPro,
                                       ATC_ECATTBEARER_7_AUTH_PROTOCOL_MIN, ATC_ECATTBEARER_7_AUTH_PROTOCOL_MAX, ATC_ECATTBEARER_7_AUTH_PROTOCOL_DEFAULT)
                                       == AT_PARA_OK)
                {
                    if(authPro == 1 && attBrParam.eitf == 0)
                    {
                        /*if PAP is TRUE, eitf must be TRUE */
                        cmeErr = CME_PDP_PAP_AND_EITF_NOT_MATCHED;
                    }
                    else
                    {
                        attBrParam.authProPresent = TRUE;
                        attBrParam.authProtocol = (BOOL)authPro;
                    }
                }
                else
                {
                    cmeErr = CME_INCORRECT_PARAM;
                }



                if(attBrParam.authProtocol == CMI_SECURITY_PROTOCOL_PAP)
                {
                   /*auth user name*/
                    if (atGetStrValue(pParamList, 8, authUserName,
                                            ATC_ECATTBEARER_8_AUTH_USERNAME_MAX_LEN + 1 , &length, ATC_ECATTBEARER_8_AUTH_USERNAME_DEFAULT) == AT_PARA_OK)
                    {
                        attBrParam.authUserNameLength = (length < CMI_PS_MAX_AUTH_STR_LEN ? length : CMI_PS_MAX_AUTH_STR_LEN);
                        strncpy((CHAR *)(attBrParam.authUserName), (CHAR *)authUserName, attBrParam.authUserNameLength);
                    }

                    else
                    {
                        cmeErr = CME_INCORRECT_PARAM;
                    }

                    /*auth password*/
                    if (atGetStrValue(pParamList, 9, authPassword,
                                            ATC_ECATTBEARER_9_AUTH_PASSWORD_MAX_LEN + 1, &length, ATC_ECATTBEARER_9_AUTH_PASSWORD_DEFAULT) == AT_PARA_OK)
                    {
                        attBrParam.authPasswordLength = (length < CMI_PS_MAX_AUTH_STR_LEN ? length : CMI_PS_MAX_AUTH_STR_LEN);
                        strncpy((CHAR *)(attBrParam.authPassword), (CHAR *)authPassword, attBrParam.authPasswordLength);
                    }

                    else
                    {
                        cmeErr = CME_INCORRECT_PARAM;
                    }

                }
            }



            if (cmeErr == CME_SUCC)
            {
                ret = psSetAttBear(atHandle, &attBrParam);

                if (ret != CMS_RET_SUCC)
                {
                    cmeErr = CME_INCORRECT_PARAM;
                }
            }

            if (ret != CMS_RET_SUCC)
            {
                OsaDebugBegin(cmeErr != CME_SUCC, cmeErr, ret, 0);
                cmeErr = CME_UNKNOWN;
                OsaDebugEnd();

                ret = atcReply(atHandle, AT_RC_CME_ERROR, cmeErr, NULL);
            }

            break;
        }

        case AT_EXEC_REQ:
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCRTDCP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCRTDCP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 reporting;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CRTDCP=? */
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CRTDCP: (0-1),(0-10),(1600)");
            break;

        case AT_READ_REQ:     /* AT+CRTDCP? */
            ret = psGetCurRTDCP(atHandle);
            break;


        case AT_SET_REQ:      /* AT+CRTDCP= */
        {
            if(atGetNumValue(pParamList, 0, &reporting, 0, 1, 0) != AT_PARA_ERR)
            {
                ret = psSetRTDCP(atHandle, reporting);
            }

            if(ret == CMS_RET_SUCC)
            {
                //CRTDCP cnf will call atcReply
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCGAUTH(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGAUTH(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 cid, authProt;
    UINT8 userId[CMI_PS_MAX_AUTH_STR_LEN+1] = {0};
    UINT8 password[CMI_PS_MAX_AUTH_STR_LEN+1] = {0};
    UINT16 length;
    UINT8 userIdlength = 0;
    UINT8 passwordLength = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CGAUTH=? */
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CGAUTH: (0-10),(0-1),(20),(20)\r\n");
            break;

        case AT_READ_REQ:     /* AT+CGAUTH? */
            ret = psGetCGAUTH(atHandle);
            break;

        case AT_SET_REQ:      /* AT+CGAUTH= */
        {
            if(atGetNumValue(pParamList, 0, &cid, ATC_CGAUTH_0_CID_VAL_MIN, ATC_CGAUTH_0_CID_VAL_MAX, ATC_CGAUTH_0_CID_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if(atGetNumValue(pParamList, 1, &authProt, ATC_CGAUTH_1_AUTHPROT_VAL_MIN, ATC_CGAUTH_1_AUTHPROT_VAL_MAX, ATC_CGAUTH_1_AUTHPROT_VAL_DEFAULT) != AT_PARA_ERR)
                {
                    if(authProt == 1)
                    {
                        if(atGetStrValue(pParamList, 2, userId, ATC_CGAUTH_2_USER_STR_MAX_LEN +1, &length, ATC_CGAUTH_2_USER_STR_DEFAULT) != AT_PARA_ERR)
                        {
                            userIdlength = (length < CMI_PS_MAX_AUTH_STR_LEN ? length : CMI_PS_MAX_AUTH_STR_LEN);
                            if(atGetStrValue(pParamList, 3, password, ATC_CGAUTH_3_PASSWD_STR_MAX_LEN +1, &length, ATC_CGAUTH_3_PASSWD_STR_DEFAULT) != AT_PARA_ERR)
                            {
                                passwordLength = (length < CMI_PS_MAX_AUTH_STR_LEN ? length : CMI_PS_MAX_AUTH_STR_LEN);
                                if(userIdlength == 0 || passwordLength == 0)
                                {
                                    ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                                    break;
                                }
                                ret = psSetCGAUTH(atHandle,cid,authProt,userId,userIdlength,password,passwordLength);
                            }
                        }
                    }
                    else//delete current PAP setting
                    {
                        ret = psSetCGAUTH(atHandle,cid,authProt,userId,userIdlength,password,passwordLength);
                    }
                }
            }

            if(ret == CMS_RET_SUCC)
            {
                //CGAUTH cnf will call atcReply
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCIPCA(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCIPCA(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 n, attach;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CIPCA=? */
#ifdef  FEATURE_REF_AT_ENABLE
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CIPCA: (3),(0,1)\r\n");
#else
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CIPCA: (3),(0-1)\r\n");
#endif
            break;

        case AT_READ_REQ:     /* AT+CIPCA? */
            ret = psGetCIPCA(atHandle);
            break;

        case AT_SET_REQ:      /* AT+CIPCA= */
        {
            if(atGetNumValue(pParamList, 0, &n, ATC_CIPCA_0_VAL_MIN, ATC_CIPCA_0_VAL_MAX, ATC_CIPCA_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if(atGetNumValue(pParamList, 1, &attach, ATC_CIPCA_1_ATT_VAL_MIN, ATC_CIPCA_1_ATT_VAL_MAX, ATC_CIPCA_1_ATT_VAL_DEFAULT) != AT_PARA_ERR)
                {
                    ret = psSetCIPCA(atHandle,n,attach);
                }
            }

            if(ret == CMS_RET_SUCC)
            {
                //CIPCA cnf will call atcReply
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId psCGAPNRC(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGAPNRC(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   cid = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CGAPNRC=? */
        {
            ret = psGetCGAPNRCCid(atHandle);
            break;
        }

        case AT_SET_REQ:      /* AT+CGAPNRC= */
        {
            /* +CGAPNRC[=<cid>] */
            if (atGetNumValue(pParamList, 0, &cid,
                              ATC_CGAPNRC_0_CID_VAL_MIN, ATC_CGAPNRC_0_CID_VAL_MAX, ATC_CGAPNRC_0_CID_VAL_DEFAULT) == AT_PARA_OK)
            {
                ret = psGetCGAPNRC(atHandle, cid, FALSE);
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:      /* AT+CGAPNRC */
        {
            ret = psGetCGAPNRC(atHandle, 0, TRUE);
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          psCGEREP psCGAPNRC(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGEREP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   mode = 0, bfr = 0;
    CHAR    respStr[ATEC_IND_RESP_128_STR_LEN] = {0};

    /*
     * +CGEREP=[<mode>[,<bfr>]]
     * <mode>: integer type
     *   0 buffer unsolicited result codes in the MT; if MT result code buffer is full, the oldest ones can be discarded.
     *     No codes are forwarded to the TE.
     *   1 discard unsolicited result codes when MT-TE link is reserved (e.g. in on-line data mode); otherwise forward
     *     them directly to the TE
     *   2 buffer unsolicited result codes in the MT when MT-TE link is reserved (e.g. in on-line data mode) and flush
     *     them to the TE when MT-TE link becomes available; otherwise forward them directly to the TE; - NOT SUPPORT
     * <bfr>: integer type
     *   0 MT buffer of unsolicited result codes defined within this command is cleared when <mode> 1 or 2 is entered
     *   1 MT buffer of unsolicited result codes defined within this command is flushed to the TE when <mode> 1 or 2
     *     is entered (OK response shall be given before flushing the codes) - NOT SUPPORT
    */
    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CGEREP=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+CGEREP: (0,1),(0)\r\n");
            break;
        }

        case AT_SET_REQ:      /* AT+CGEREP= */
        {
            if (atGetNumValue(pParamList, 0, &mode,
                              ATC_CGEREP_0_VAL_MIN, ATC_CGEREP_0_VAL_MAX, ATC_CGEREP_0_VAL_DEFAULT) == AT_PARA_OK)
            {
                /* check the second parameter */
                if (atGetNumValue(pParamList, 1, &bfr,
                                  ATC_CGEREP_1_VAL_MIN, ATC_CGEREP_1_VAL_MAX, ATC_CGEREP_0_VAL_DEFAULT) != AT_PARA_ERR)
                {
                    /* BFR, only support 0, by now */
                    if (mode == ATC_CGEREP_DISCARD_OLD_CGEV ||
                        mode == ATC_CGEREP_FWD_CGEV)
                    {
                        mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CGEREP_MODE_CFG, mode);
                        ret = CMS_RET_SUCC;
                    }
                }
            }

            if (ret == CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_OK, 0, PNULL);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_READ_REQ:      /* AT+CGEREP? */
        {
            /*+CGEREP: <mode>,<bfr>*/
            snprintf(respStr, ATEC_IND_RESP_128_STR_LEN, "+CGEREP: %d,0",
                     mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_CGEREP_MODE_CFG));

            ret = atcReply(atHandle, AT_RC_OK, 0, respStr);

            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId psECSENDDATA(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psECSENDDATA(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   cid = 0, dataLen = 0, dataBufSize = 0, rai = 0, type = 0;
    UINT16  dataStrLen = 0;
    UINT8   *pStrdata = PNULL;
    MtErrorResultCode   errCode = CME_SUCC;

    switch (operaType)
    {
        case AT_TEST_REQ:   /* AT+ECSENDDATA=? */
            /*
             * +ECSENDDATA: (range of supported <cid>s),(maximum number of octets of user data indicated by <data_length>),
             *              (list of supported <RAI>s),(list of supported <type_of_user_data>s)
            */
            ret = atcReply(atHandle, AT_RC_OK, CME_SUCC, "+ECSENDDATA: (0-10),(950),(0-2),(0,1)");
            break;

        case AT_SET_REQ:      /* AT+ECSENDDATA= */
        {
            /* +ECSENDDATA=<cid>,<data_length>,<data>[,<RAI>[,<type_of_user_data>]] */
            if (atGetNumValue(pParamList, 0, &cid,
                              ATC_ECSENDDATA_0_CID_VAL_MIN, ATC_ECSENDDATA_0_CID_VAL_MAX, ATC_ECSENDDATA_0_CID_VAL_DEFAULT) != AT_PARA_OK)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, psECSENDDATA_cid_1, P_WARNING, 0, "AT CMD: ECSENDDATA, invalid input CID");
                errCode = CME_INCORRECT_PARAM;
            }

            /*<cpdata_length>*/
            if (errCode == CME_SUCC &&
                atGetNumValue(pParamList, 1, &dataLen,
                              ATC_ECSENDDATA_1_DATALEN_VAL_MIN, ATC_ECSENDDATA_1_DATALEN_VAL_MAX, ATC_ECSENDDATA_1_DATALEN_VAL_DEFAULT) != AT_PARA_OK)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, psECSENDDATA_length_1, P_WARNING, 0, "AT CMD: ECSENDDATA, invalid input data_length");
                errCode = CME_INCORRECT_PARAM;
            }

            /*
             * Calc string data buffer size, and alloc memory
             * example: data_length = 2, data = "34A8", so here, data string length = 4;
            */
            dataBufSize = (dataLen*2 + 1 + 3)&0xFFFFFFFC;     /*+1, for '\0', and +3 is for 4 bytes aligned*/
            pStrdata = (UINT8 *)OsaAllocMemory(dataBufSize);

            if (pStrdata == PNULL)
            {
                OsaDebugBegin(FALSE, dataLen, cid, dataBufSize);
                OsaDebugEnd();
                errCode = CME_MEMORY_FULL;
            }

            /*<data>*/
            if (errCode == CME_SUCC &&
                atGetStrValue(pParamList, 2, pStrdata,
                               dataBufSize, &dataStrLen, ATC_ECSENDDATA_2_DATA_STR_DEFAULT) != AT_PARA_OK)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, psECSENDDATA_date_1, P_WARNING, 0, "AT CMD: ECSENDDATA, invalid input data");
                errCode = CME_INCORRECT_PARAM;
            }

            /*check the data length*/
            if (dataStrLen != dataLen*2)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, psECSENDDATA_date_2, P_WARNING, 0, "AT CMD: ECSENDDATA, data length mismatched");
                errCode = CME_INCORRECT_PARAM;
            }

            /*
             * +CSODCP=<cid>,<cpdata_length>,<cpdata>[,<RAI>[,<type_of_user_data>]]
             * <RAI>, opt
            */
            if (errCode == CME_SUCC &&
                atGetNumValue(pParamList, 3, &rai,
                              ATC_ECSENDDATA_3_RAI_VAL_MIN, ATC_ECSENDDATA_3_RAI_VAL_MAX, ATC_ECSENDDATA_3_RAI_VAL_DEFAULT) == AT_PARA_ERR)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, psECSENDDATA_RAI_1, P_WARNING, 0, "AT CMD: ECSENDDATA, invalid input RAI");
                errCode = CME_INCORRECT_PARAM;
            }

            /*<type_of_user_data>*/
            if (errCode == CME_SUCC &&
                atGetNumValue(pParamList, 4, &type,
                              ATC_ECSENDDATA_4_TYPE_VAL_MIN, ATC_ECSENDDATA_4_TYPE_VAL_MAX, ATC_ECSENDDATA_4_TYPE_VAL_DEFAULT) == AT_PARA_ERR)
            {
                ECOMM_TRACE(UNILOG_ATCMD_EXEC, psECSENDDATA_type_1, P_WARNING, 0, "AT CMD: ECSENDDATA, invalid input: <type_of_user_data>");
                errCode = CME_INCORRECT_PARAM;
            }

            if (errCode == CME_SUCC)
            {
                ret = psSendUserData(atHandle, cid, dataStrLen, pStrdata, rai, type, FALSE);

                if (ret != CMS_RET_SUCC)
                {
                    errCode = CME_MEMORY_FAILURE;
                }
            }

            /* free "strCpdata" */
            if (pStrdata != PNULL)
            {
                OsaFreeMemory(&pStrdata);
            }

            if (ret != CMS_RET_SUCC)
            {
                OsaDebugBegin(errCode != CME_SUCC, errCode, ret, 0);
                errCode = CME_UNKNOWN;
                OsaDebugEnd();

                ret = atcReply(atHandle, AT_RC_CME_ERROR, errCode, PNULL);
            }

            break;
        }

        default:
            ret = atcReply(atHandle, AT_RC_ERROR, CME_OPERATION_NOT_ALLOW, PNULL);
            break;
    }

    return ret;
}

/**
  \fn          CmsRetId psECCIOTPLANE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psECCIOTPLANE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 plane;

    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+ECCIOTPLANE= */
        {
            if(atGetNumValue(pParamList, 0, &plane, ATC_ECCIOTPLANE_0_PLANE_VAL_MIN, ATC_ECCIOTPLANE_0_PLANE_VAL_MAX, ATC_ECCIOTPLANE_0_PLANE_VAL_DEFAULT) != AT_PARA_ERR)
            {
                ret = psSetECCIOTPLANE(atHandle,plane);
            }

            if(ret == CMS_RET_SUCC)
            {
                //ECCIOTPLANE cnf will call atcReply
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId psECNBIOTRAI(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psECNBIOTRAI(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32   rai = 0;
    UINT16  cpDataStrLen = 2;
    UINT8   *strCpdata = "00";

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+ECNBIOTRAI=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECNBIOTRAI: (0-1)\r\n");
            break;
        }

        case AT_SET_REQ:      /* AT+ECNBIOTRAI= */
        {
            if(atGetNumValue(pParamList, 0, &rai, ATC_ECNBIOTRAI_0_RAI_VAL_MIN, ATC_ECNBIOTRAI_0_RAI_VAL_MAX, ATC_ECNBIOTRAI_0_RAI_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if (psDialGetPsConnStatus() == TRUE)
                {
                    ret = psSendUserData(atHandle, 0, cpDataStrLen, strCpdata, rai, 0, TRUE);
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                }
            }

            if(ret == CMS_RET_SUCC)
            {
                //ECNBIOTRAI cnf will call atcReply
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }

            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId psCGPADDR(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCGPADDR(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32   cid = 0;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32     operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP    pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:    /* AT+CGPADDR=? */
        {
            ret = psGetPDPAddrCid(atHandle);
            break;
        }

        case AT_SET_REQ:      /* AT+CGPADDR=<cid> */
        {
            //cgpaddrActCmd = FALSE;
            if (atGetNumValue(pParamList, 0, &cid,
                              ATC_CGPADDR_0_CID_VAL_MIN, ATC_CGPADDR_0_CID_VAL_MAX, ATC_CGPADDR_0_CID_VAL_DEFAULT) == AT_PARA_OK)
            {
                ret = psGetPDPAddr(atHandle, cid, FALSE);
            }

            if (ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:        /* AT+CGPADDR */
        {
            //cgpaddrActCmd = TRUE;
            ret = psGetPDPAddr(atHandle, 0, TRUE);
            break;
        }

        case AT_READ_REQ:           /* AT+CGPADDR?  */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId psCNMPSD(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  psCNMPSD(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CNMPSD=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_EXEC_REQ:         /* AT+CNMPSD */
        {
            ret = psSendNoMorePsDataReq(atHandle);
            break;
        }

        default:         /* AT+CNMPSD?, AT+CNMPSD= */
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId psCEER(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   *pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId  psCEER(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CEER=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        case AT_EXEC_REQ:         /* AT+CEER */
        {
            ret = psGetCEER(atHandle);
            break;
        }

        default:         /* AT+CEER? AT+CEER= */
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


