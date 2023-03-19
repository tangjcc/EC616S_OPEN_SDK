/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    atc_task.c
 * Description:  at command task source file
 * History:      Rev1.0   2018-08-06
 *
 ****************************************************************************/
#include "cms_comm.h"
#include "debug_log.h"
#include "ps_event_callback.h"
#include "ps_lib_api.h"
#include "atc_decoder.h"
#include "at_util.h"
#include "cmicomm.h"
#include "netmgr.h"
#include "ostask.h"
#include "PhyAtCmdDebug.h"
#include "atec_cust_cmd_table.h"
#include "atec_dev_cnf_ind.h"
#include "atec_mm_cnf_ind.h"
#include "atec_ps_cnf_ind.h"
#include "atec_sms_cnf_ind.h"
#include "atec_sim_cnf_ind.h"
#include "atec_tcpip_cnf_ind.h"
#include "atec_phy_cnf_ind.h"
#include "at_sock_entity.h"

#ifdef FEATURE_CISONENET_ENABLE
#include "atec_onenet_cnf_ind.h"
#endif
#ifdef  FEATURE_HTTPC_ENABLE
#include "atec_http_cnf_ind.h"
#endif
#ifdef FEATURE_WAKAAMA_ENABLE
#include "atec_lwm2m_cnf_ind.h"
#endif
#ifdef FEATURE_CTWINGV1_5_ENABLE
#include "atec_ctlwm2m_cnf_ind.h"
#endif
#if defined(FEATURE_MQTT_ENABLE) || defined(FEATURE_MQTT_TLS_ENABLE)
#include "atec_mqtt_cnf_ind.h"
#endif
#ifdef FEATURE_LIBCOAP_ENABLE
#include "atec_coap_cnf_ind.h"
#endif

#ifdef FEATURE_EXAMPLE_ENABLE
#include "atec_example_cnf_ind.h"
#endif

#ifdef  FEATURE_REF_AT_ENABLE
#include "at_ref_entity.h"
#endif

#ifdef FEATURE_ATADC_ENABLE
#include "atec_adc_cnf_ind.h"
#endif

#include "atec_fwupd_cnf_ind.h"

#ifdef SOFTSIM_CT_ENABLE
#include "atec_cust_softsim.h"
#include "esimmonitortask.h"

#endif
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

#ifdef  SOFTSIM_CT_ENABLE

/**
    \fn          atAppeSimRespMsg
    \brief       handle the SIG_ESIM_SIGNAL_SEND_CNF from esimproctask
    \param[in]   pSig: the cnf signal buf
    \returns     return 1
*/
static void atAppeSimRespMsg(void *pDataCnfMsg)
{
    esimProcessRespMsg(AT_CHAN_DEFAULT, pDataCnfMsg);

    return;
}
#endif

/**
  \fn
  \brief        AT process AT related signal
  \returns      void
  \Note
*/
static void atProcCmiCnfSig(CacCmiCnf *pCmiCnf)
{
    UINT16  sgId = 0;
    UINT16  primId = 0;
    UINT8   subAtId = AT_GET_HANDLER_SUB_ATID(pCmiCnf->header.srcHandler);

    /*
     * AT channel ID = 0, is reserved for internal using, not for AT CMD channel
    */
    if (AT_GET_HANDLER_CHAN_ID(pCmiCnf->header.srcHandler) == CMS_CHAN_RSVD)
    {
        return;
    }

    sgId    = AT_GET_CMI_SG_ID(pCmiCnf->header.cnfId);
    primId  = AT_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    ECOMM_TRACE(UNILOG_ATCMD, atPrcoCacCmiCnfSig_1, P_INFO, 3,
                "ATCMD, AT chanId: %d, CmiCnf sgId: %d(DEV-2/MM-3/PS-4/SIM-5/SMS-6), primId: %d",
                AT_GET_HANDLER_CHAN_ID(pCmiCnf->header.srcHandler), sgId, primId);

    #ifdef  FEATURE_REF_AT_ENABLE
    if (subAtId == CMS_REF_SUB_AT_1_ID || subAtId == CMS_REF_SUB_AT_2_ID || subAtId == CMS_REF_SUB_AT_3_ID)
    {
        atRefProcCmiCnf(pCmiCnf);
        return;
    }
    #endif

    OsaDebugBegin(subAtId <= CMS_EC_MAX_SUB_AT_ID, subAtId, sgId, primId);
    OsaDebugEnd();

    switch(sgId)
    {
        case CAC_DEV:
            atDevProcCmiCnf(pCmiCnf);
            break;

        case CAC_MM:
            atMmProcCmiCnf(pCmiCnf);
            break;

        case CAC_PS:
            atPsProcCmiCnf(pCmiCnf);
            break;

        case CAC_SIM:
            atSimProcCmiCnf(pCmiCnf);
            break;

        case CAC_SMS:
            atSmsProcCmiCnf(pCmiCnf);
            break;

        default:
            OsaDebugBegin(FALSE, sgId, primId, 0);
            OsaDebugEnd();
            break;
    }

    return;
}


/**
  \fn
  \brief        AT process AT related signal
  \returns      void
  \Note
*/
static void atProcCmiIndSig(CacCmiInd *pCmiInd)
{
    UINT16    sgId = 0;   //service group ID
    UINT16    primId = 0;

    sgId    = AT_GET_CMI_SG_ID(pCmiInd->header.indId);
    primId  = AT_GET_CMI_PRIM_ID(pCmiInd->header.indId);

    ECOMM_TRACE(UNILOG_ATCMD, atPrcoCacCmiIndSig_1, P_INFO, 2,
                "ATCMD NB protocol IND, sgId: %d(DEV-2/MM-3/PS-4/SIM-5/SMS-6), PRIM ID: %d", sgId, primId);

    switch(sgId)
    {
        case CAC_DEV:
            atDevProcCmiInd(pCmiInd);
            break;

        case CAC_MM:
            atMmProcCmiInd(pCmiInd);
            break;

        case CAC_PS:
            atPsProcCmiInd(pCmiInd);
            break;

        case CAC_SIM:
            atSimProcCmiInd(pCmiInd);
            break;

        case CAC_SMS:
            atSmsProcCmiInd(pCmiInd);
            break;

        default:
            OsaDebugBegin(FALSE, sgId, primId, 0);
            OsaDebugEnd();
            break;
    }

    #ifdef  FEATURE_REF_AT_ENABLE
    atRefProcCmiInd(pCmiInd);
    #endif

    return;
}

/**
  \fn
  \brief        AT process AT related signal
  \returns      void
  \Note
*/
static void atProcApplCnfSig(CmsApplCnf *pApplCnf)
{
    UINT8   subAtId = 0;

    if (pApplCnf == PNULL)
    {
        OsaDebugBegin(FALSE, pApplCnf->header.appId, pApplCnf->header.primId, pApplCnf->header.rc);
        OsaDebugEnd();

        return;
    }

    subAtId = AT_GET_HANDLER_SUB_ATID(pApplCnf->header.srcHandler);

    /*
     * AT channel ID = 0, is reserved for internal using, not for AT CMD channel
    */
    if (AT_GET_HANDLER_CHAN_ID(pApplCnf->header.srcHandler) == CMS_CHAN_RSVD)
    {
        return;
    }

    ECOMM_TRACE(UNILOG_ATCMD, atProcCmsApplCnfSig_1, P_INFO, 3, "ATCMD, AT chanId: %d, APP CNF appId: %d, primId: %d",
                AT_GET_HANDLER_CHAN_ID(pApplCnf->header.srcHandler), pApplCnf->header.appId, pApplCnf->header.primId);

    #ifdef  FEATURE_REF_AT_ENABLE
    if (subAtId == CMS_REF_SUB_AT_1_ID || subAtId == CMS_REF_SUB_AT_2_ID || subAtId == CMS_REF_SUB_AT_3_ID)
    {
        atRefProcApplCnf(pApplCnf);
        return;
    }
    #endif

    OsaDebugBegin(subAtId <= CMS_EC_MAX_SUB_AT_ID, subAtId, pApplCnf->header.appId, pApplCnf->header.primId);
    OsaDebugEnd();

    switch(pApplCnf->header.appId)
    {
        case APPL_NM:
            atTcpIpProcNmApplCnf(pApplCnf);
            break;

        case APPL_SOCKET:
            atTcpIpProcSktApplCnf(pApplCnf);
            break;


#if defined(FEATURE_MQTT_ENABLE) || defined(FEATURE_MQTT_TLS_ENABLE)
        case APPL_MQTT:
            atApplMqttProcCmsCnf(pApplCnf);
            break;
#endif
#ifdef FEATURE_LIBCOAP_ENABLE
        case APPL_COAP:
            atApplCoapProcCmsCnf(pApplCnf);
            break;
#endif
#ifdef FEATURE_WAKAAMA_ENABLE
        case APPL_LWM2M:
            atApplLwm2mProcCmsCnf(pApplCnf);
            break;
#endif
#ifdef  FEATURE_HTTPC_ENABLE
        case APPL_HTTP:
            atApplHttpProcCmsCnf(pApplCnf);
            break;
#endif
        case APPL_ECSOC:
            atTcpIpProcSocApplCnf(pApplCnf);
            break;
        case APPL_ECSRVSOC:
            atTcpIpProcSrvSocApplCnf(pApplCnf);
            break;

#ifdef FEATURE_CISONENET_ENABLE
        case APPL_ONENET:
            atApplOnenetProcCmsCnf(pApplCnf);
            break;
#endif

#ifdef FEATURE_CTWINGV1_5_ENABLE
        case APPL_CTLWM2M:
            atApplCtProcCmsCnf(pApplCnf);
            break;
#endif

#ifdef FEATURE_ATADC_ENABLE
        case APPL_ADC:
            atApplAdcProcCmsCnf(pApplCnf);
            break;
#endif

#ifdef FEATURE_EXAMPLE_ENABLE
        case APPL_EXAMPLE:
            atApplExampleProcCmsCnf(pApplCnf);
            break;
#endif

        case APPL_FWUPD:
            atApplFwupdProcCmsCnf(pApplCnf);
            break;

        default:
            break;
    }

    return;
}


/**
  \fn
  \brief        AT process AT related signal
  \returns      void
  \Note
*/
static void atProcApplIndSig(CmsApplInd *pApplInd)
{
    UINT8   subAtId = AT_GET_HANDLER_SUB_ATID(pApplInd->header.reqHandler);

    ECOMM_TRACE(UNILOG_ATCMD, atProcCmsApplIndSig_1, P_INFO, 2, "ATCMD, APP IND appId: %d, primId: %d",
                pApplInd->header.appId, pApplInd->header.primId);

    if (pApplInd == PNULL)
    {
        OsaDebugBegin(FALSE, pApplInd->header.appId, pApplInd->header.primId, 0);
        OsaDebugEnd();

        return;
    }


    if (pApplInd->header.reqHandler != BROADCAST_IND_HANDLER)
    {
        #ifdef  FEATURE_REF_AT_ENABLE
        if (subAtId == CMS_REF_SUB_AT_1_ID || subAtId == CMS_REF_SUB_AT_2_ID || subAtId == CMS_REF_SUB_AT_3_ID)
        {
            atRefProcApplInd(pApplInd);
            return;
        }
        #endif

        OsaDebugBegin(subAtId <= CMS_EC_MAX_SUB_AT_ID, subAtId, pApplInd->header.appId, pApplInd->header.primId);
        OsaDebugEnd();
    }

    switch(pApplInd->header.appId)
    {
        case APPL_NM:
            atTcpIpProcNmApplInd(pApplInd);
            break;

        case APPL_SOCKET:
            atTcpIpProcSktApplInd(pApplInd);
            break;

#if defined(FEATURE_MQTT_ENABLE) || defined(FEATURE_MQTT_TLS_ENABLE)
        case APPL_MQTT:
            atApplMqttProcCmsInd(pApplInd);
            break;
#endif
#ifdef FEATURE_LIBCOAP_ENABLE
        case APPL_COAP:
            atApplCoapProcCmsInd(pApplInd);
            break;
#endif

#ifdef FEATURE_WAKAAMA_ENABLE
        case APPL_LWM2M:
            atApplLwm2mProcCmsInd(pApplInd);
            break;
#endif
#ifdef  FEATURE_HTTPC_ENABLE
        case APPL_HTTP:
            atApplHttpProcCmsInd(pApplInd);
            break;
#endif

        case APPL_ECSOC:
            atTcpIpProcSocApplInd(pApplInd);
            break;
        case APPL_ECSRVSOC:
            atTcpIpProcSrvSocApplInd(pApplInd);
            break;

#ifdef FEATURE_CISONENET_ENABLE
        case APPL_ONENET:
            atApplOnenetProcCmsInd(pApplInd);
            break;
#endif

#ifdef FEATURE_CTWINGV1_5_ENABLE
        case APPL_CTLWM2M:
            atApplCtProcCmsInd(pApplInd);
            break;
#endif

#ifdef FEATURE_EXAMPLE_ENABLE
        case APPL_EXAMPLE:
            atApplExampleProcCmsInd(pApplInd);
            break;
#endif

        default:
            break;
    }

    #ifdef  FEATURE_REF_AT_ENABLE
    if (pApplInd->header.reqHandler == BROADCAST_IND_HANDLER)
    {
        atRefProcApplInd(pApplInd);
    }
    #endif

    return;
}

/**
  \fn       void atProcTimerExpirySig(OsaTimerExpiry *pExpiry)
  \brief
  \returns  void
  \Note
*/
static void atProcTimerExpirySig(OsaTimerExpiry *pExpiry)
{
    UINT16  timeId = pExpiry->timerEnum;

    if (CMS_GET_OSA_TIMER_SUB_MOD_ID(timeId) != CMS_TIMER_AT_SUB_MOD_ID)
    {
        return;
    }

    atcAsynTimerExpiry(timeId);

    return;
}



/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS

#if 0
/****************************************************************************************
*  FUNCTION:
*
*  PARAMETERS:
*
*  DESCRIPTION:
*
*  RETURNS:
*
****************************************************************************************/
CmsRetId ATCmdProcSeqSMS(const SignalBuf *smsSignalPtr)
{
    AtCmdStrReq *at_queue_msg = PNULL;
    switch(smsSignalPtr->sigId)
    {
        case SIG_AT_SMS_CTRL_REQ:
            at_queue_msg = (AtCmdStrReq *)(smsSignalPtr->sigBody);
            smsSendMsgAfterCtrlZ(at_queue_msg->at_cmd_buf_ptr, at_queue_msg->at_cmd_len);
            atcmd_ctrl[0].bSmsDataEntryModeOn = FALSE;

            break;

        case SIG_AT_SMS_ESC_REQ:
            at_queue_msg = (AtCmdStrReq *)(smsSignalPtr->sigBody);
            smsSendMsgAfterEsc(at_queue_msg->at_cmd_buf_ptr, at_queue_msg->at_cmd_len);
            atcmd_ctrl[0].bSmsDataEntryModeOn = FALSE;

            break;
    }

    return CMS_RET_SUCC;
}
#endif

/**
  \fn           void atInit()
  \brief        AT init
  \returns      void
  \Note
*/
void atInit()
{
    atcDecInit();
    //add at socket init
    atSocketInit();
}


/**
  \fn           void atProcSignal(const SignalBuf *pSig)
  \brief        AT process AT related signal
  \returns      void
  \Note         signal should not be destoryed in this func, as the signal maybe also needed in CMS task
*/
void atProcSignal(const SignalBuf *pSig)
{
    OsaDebugBegin(pSig != PNULL, 0, 0, 0);
    OsaDebugEnd();

    switch(pSig->sigId)
    {
        case SIG_AT_CMD_STR_REQ:
            atcProcAtCmdStrReqSig((AtCmdStrReq *)(pSig->sigBody));
            break;

        case SIG_CAC_CMI_CNF:
            atProcCmiCnfSig((CacCmiCnf *)(pSig->sigBody));
            break;

        case SIG_CAC_CMI_IND:
            atProcCmiIndSig((CacCmiInd *)(pSig->sigBody));
            break;

        case SIG_CMS_APPL_CNF:
            atProcApplCnfSig((CmsApplCnf *)(pSig->sigBody));
            break;

        case SIG_CMS_APPL_IND:
            atProcApplIndSig((CmsApplInd *)(pSig->sigBody));
            break;

        case SIG_AT_CMD_CONTINUE_REQ:
            atcProcAtCmdContinueReqSig((AtCmdContinueReq *)(pSig->sigBody));
            break;

        #if 0
        case SIG_NM_ATI_CNF:
            ECOMM_TRACE(UNILOG_ATCMD, atcmd_input6, P_INFO, 0, "atcmd.nm.cnf..");
            break;

        case SIG_NM_ATI_IND:
            ECOMM_TRACE(UNILOG_ATCMD, atcmd_input7, P_INFO, 0, "atcmd.nm.ind..");
            break;

        case SIG_AT_SMS_CTRL_REQ:
        case SIG_AT_SMS_ESC_REQ:
            /*
             * -TBD
            */
            ECOMM_TRACE(UNILOG_ATCMD, atcmd_input8, P_INFO, 0, "atcmd.sms.req..");
            break;
        #endif
        case SIG_CEPHY_DEBUG_CNF:
        {
            //UINT16    sgId = 0;   //service group ID
            UINT16    primId = 0;
            CePhyCnf   *pPhyCnf = PNULL;

            pPhyCnf = (CePhyCnf *)(pSig->sigBody);
            //sgId    = CEPHY_GET_SG_ID(pPhyCnf->header.cnfId);
            primId  = CEPHY_GET_PRIM_ID(pPhyCnf->header.cnfId);

            /*
             * -should OPT, -TBD
            */
            phyECPHYCFGcnf(primId, pPhyCnf->header.srcHandler, pPhyCnf->header.rc, pPhyCnf->body);

            ECOMM_TRACE(UNILOG_ATCMD, atcmd_input9, P_INFO, 0, "atcmd.phy.cnf..");
            break;
        }

        case SIG_CEPHY_DEBUG_IND:
            /*
             * - TBD
            */
            OsaDebugBegin(FALSE, SIG_CEPHY_DEBUG_IND, 0, 0);
            OsaDebugEnd();
            ECOMM_TRACE(UNILOG_ATCMD, atcmd_input10, P_INFO, 0, "atcmd.phy.ind..");
            break;

        case SIG_CUSTOMER_CNF:
            /*
             * - TBD
            */
            ECOMM_TRACE(UNILOG_ATCMD, atcmd_input11, P_INFO, 0, "atcmd.customer.cnf..");
            break;

        case SIG_CUSTOMER_IND:
            /*
             * - TBD
            */
            ECOMM_TRACE(UNILOG_ATCMD, atcmd_input12, P_INFO, 0, "atcmd.customer.ind..");
            break;

        case SIG_TIMER_EXPIRY:
            atProcTimerExpirySig((OsaTimerExpiry *)(pSig->sigBody));
            break;

   #ifdef  SOFTSIM_CT_ENABLE
        case SIG_ESIM_SIGNAL_SEND_CNF:
            atAppeSimRespMsg((void *)(pSig->sigBody));
            break;
   #endif

        default:
            break;
    }
    return;
}

