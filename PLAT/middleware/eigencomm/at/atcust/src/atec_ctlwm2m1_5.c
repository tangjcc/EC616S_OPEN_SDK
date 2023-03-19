/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    atec_ctlwm2m.c
 * Description:  at entry for ctlwm2m source file
 * History:      Rev1.0   2018-08-06
 *
 ****************************************************************************/
#include "bsp.h"
#include "at_util.h"
#include "ps_lib_api.h"
#include "atec_ctlwm2m1_5.h"
#include "ctiot_lwm2m_sdk.h"
#include "atec_ctlwm2m_cnf_ind.h"
#include "ec_ctlwm2m_api.h"
#include "ct_internals.h"
#include "at_ctlwm2m_task.h"
#include "ctiot_NV_data.h"

INT32 g_seq_num = 0;

static bool atParserCheckHexstr(const UINT8* hexStr)
{
    int len = strlen((const char *)hexStr);
    int i = 0;
    if(len % 2 != 0)
        return 0;
    for(i = 0; i < len; i ++)
    {
        if(!((hexStr[i] >= '0' && hexStr[i] <= '9')
            ||(hexStr[i] >= 'a' && hexStr[i] <= 'f')
            || (hexStr[i] >= 'A' && hexStr[i] <= 'F')))
            return 0;
    }
    return 1;
}

/**
  \fn          CmsRetId ctm2mVERSION(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mVERSION(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    CHAR rspBuf[MAX_CTM2M_VER_LEN] = {0};

    switch (operaType)
    {
        case AT_READ_REQ:              /* AT+CTM2MVER? */
        {
            sprintf(rspBuf, "+CTM2MVER: %s,%s,%s,%s", LWM2M_VERSION, CTM2M_VERSION, BOARD_NAME, SOFTVERSION);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mSETMOD(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mSETMOD(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR rspBuf[MAX_RSP_LEN] = {0};
    INT32 mod_id = 0,mod_value = 0;
    UINT8 idAuthMode = 0;
    UINT8 natType = 0;
    UINT8 onUQMode = 0;
    UINT8 autoHeartb = 0;
    UINT8 wakeupPolicy = 0;
    UINT8 protocolMode = 0;


    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CTM2MSETMOD =? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+CTM2MSETMOD: (1-6), (0-5)");
            break;
        }
        case AT_READ_REQ:              /* AT+CTM2MSETMOD? */
        {
            ctiot_funcv1_get_mod(NULL, &idAuthMode, &natType, &onUQMode, &autoHeartb, &wakeupPolicy, &protocolMode);
            snprintf(rspBuf, MAX_RSP_LEN, "+CTM2MSETMOD: %d,%d,%d,%d,%d,%d", idAuthMode, natType, onUQMode, autoHeartb, wakeupPolicy,protocolMode);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }

        case AT_SET_REQ:              /* AT+CTM2MSETMOD= */
        {
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &mod_id, IDAUTH_MODE, PROTOCOL_MODE, IDAUTH_MODE) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            ECOMM_TRACE(UNILOG_ATCMD, ctm2mMODINIT_1, P_INFO, 1, "ctm2mMODINIT mod_id=%d", mod_id);

            switch(mod_id)
            {
                case IDAUTH_MODE:
                {
                    if(atGetNumValue(pAtCmdReq->pParamList, 1, &mod_value, AUTHMODE_IMEI, AUTHMODE_IMEI_IMSI, AUTHMODE_IMEI) == AT_PARA_ERR)
                    {
                        rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                        return rc;
                    }

                    ECOMM_TRACE(UNILOG_ATCMD, ctm2mMODINIT_2, P_INFO, 1, "ctm2mMODINIT idAuthMode=%d", mod_value);
                    break;
                }
                case NAT_TYPE:
                {
                    if(atGetNumValue(pAtCmdReq->pParamList, 1, &mod_value, ENABLE_NAT, DISABLE_NAT, ENABLE_NAT) == AT_PARA_ERR)
                    {
                        rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                        return rc;
                    }
                    break;
                }
                case ON_UQMODE:
                {
                    if(atGetNumValue(pAtCmdReq->pParamList, 1, &mod_value, DISABLE_UQ_MODE, ENABLE_UQ_MODE, DISABLE_UQ_MODE) == AT_PARA_ERR)
                    {
                        rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                        return rc;
                    }
                    ECOMM_TRACE(UNILOG_ATCMD, ctm2mMODINIT_4, P_INFO, 1, "ctm2mMODINIT onUQMode=%d", mod_value);
                    break;
                }
                case AUTO_HEARTBEAT_MODE:
                {
                    if(atGetNumValue(pAtCmdReq->pParamList, 1, &mod_value, AUTO_HEARTBEAT, NO_AUTO_HEARTBEAT, AUTO_HEARTBEAT) == AT_PARA_ERR)
                    {
                        rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                        return rc;
                    }
                    ECOMM_TRACE(UNILOG_ATCMD, ctm2mMODINIT_6, P_INFO, 1, "ctm2mMODINIT autoHeartb=%d", mod_value);
                    break;
                }
                case WAKEUP_NOTIFY:
                {
                    if(atGetNumValue(pAtCmdReq->pParamList, 1, &mod_value, NOTICE_MCU, NO_NOTICE_MCU, NOTICE_MCU) == AT_PARA_ERR)
                    {
                        rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                        return rc;
                    }
                    ECOMM_TRACE(UNILOG_ATCMD, ctm2mMODINIT_7, P_INFO, 1, "ctm2mMODINIT wakeupPolicy=%d", mod_value);
                    break;
                }
                case PROTOCOL_MODE:
                {
                    if(atGetNumValue(pAtCmdReq->pParamList, 1, &mod_value, PROTOCOL_MODE_NORMAL, PROTOCOL_MODE_ENHANCE, PROTOCOL_MODE_NORMAL) == AT_PARA_ERR)
                    {
                        rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                        return rc;
                    }
                    ECOMM_TRACE(UNILOG_ATCMD, ctm2mMODINIT_8, P_INFO, 1, "ctm2mMODINIT protocolMode=%d", mod_value);
                    break;
                }
                default:
                {
                    rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                    return rc;
                }

            }

            UINT16 ret = ctiot_at_set_mod(NULL, mod_id, mod_value);
            if(ret != CTIOT_NB_SUCCESS)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mSETPM(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mSETPM(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR rspBuf[MAX_RSP_LEN] = {0};
    UINT16 ret = 0, length = 0;
    UINT8 serverIP[MAX_SERVER_LEN+1]={0};
    INT32 lifeTime = 0, port = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CTM2MSETPM =? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+CTM2MSETPM: \"<Sever_IP>\",(1-65535),(300-2147483647)");
            break;
        }
        case AT_SET_REQ:      /* AT+CTM2MSETPM= */
        {
            if(atGetStrValue(pAtCmdReq->pParamList, 0, serverIP, MAX_SERVER_LEN+1, &length, (CHAR *)NULL) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 1, &port, CTM2MSETPM_1_PORT_MIN, CTM2MSETPM_1_PORT_MAX, CTM2MSETPM_1_PORT_DEF) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 2, &lifeTime, CTM2MSETPM_2_LIFETIME_MIN, CTM2MSETPM_2_LIFETIME_MAX, CTM2MSETPM_2_LIFETIME_DEF) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_ED_LIFETIME_ERROR, NULL);
                break;
            }
            
            ret = ctiot_at_set_pm(NULL, (CHAR *)serverIP, port, lifeTime, reqHandle);
            if(ret != CTIOT_NB_SUCCESS)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            break;
        }

        case AT_READ_REQ:        /* AT+CTM2MSETPM? */
        {
            ctiot_at_get_pm(NULL, (CHAR *)serverIP, (UINT16 *)&port, (uint32_t *)&lifeTime);

            if(strlen((const CHAR*)serverIP) == 0)
                snprintf(rspBuf, MAX_RSP_LEN, "+CTM2MSETPM: 0");
            else
                snprintf(rspBuf, MAX_RSP_LEN, "+CTM2MSETPM: %s,%d,%d", serverIP, port, lifeTime);

            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mSETPM(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mSETPSK(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR rspBuf[MAX_RSP_LEN] = {0};
    UINT16 ret = 0;

    INT32 Security_Mode=1;
    UINT8 pskId[MAX_PSKID_LEN+1] = {0};
    UINT8 psk[MAX_PSK_LEN+1] = {0};
    UINT16 psklength = 0;
    UINT16 pskidlength = 0;


    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CTM2MSETPSK =? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+CTM2MSETPSK: (1,2)[,\"<PSKID>\",\"<PSK>\"]");
            break;
        }
        case AT_SET_REQ:      /* AT+CTM2MSETPSK= */
        {
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &Security_Mode, 1, 2, 1) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            if(atGetStrValue(pAtCmdReq->pParamList, 1, pskId, MAX_PSKID_LEN, &pskidlength, (CHAR *)NULL) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            if(atGetStrValue(pAtCmdReq->pParamList, 2, psk, MAX_PSK_LEN+1, &psklength, (CHAR *)NULL) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }

            if(Security_Mode == 1)
            {
                if(psklength == 0 && pskidlength == 0)
                {
                    ret = ctiot_at_set_psk(NULL, Security_Mode, NULL, NULL);
                    if(ret != CTIOT_NB_SUCCESS)
                    {
                        rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
                    }
                    else
                    {
                        rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                    }
                }
                else
                {
                    rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                }
            }
            else
            {
                if(psklength != 0 && pskidlength != 0)
                {
                    char* pskBuf = malloc(psklength/2+1);
                    memset(pskBuf, 0, psklength/2+1);
                    cmsHexStrToHex((UINT8*)pskBuf, psklength/2, (CHAR *)psk, psklength);
                    ret = ctiot_at_set_psk(NULL, Security_Mode, (uint8_t*)pskId, (uint8_t*)pskBuf);
                    if(ret != CTIOT_NB_SUCCESS)
                    {
                        rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
                    }
                    else
                    {
                        rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                    }
                }
                else
                {
                    rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                }
            }
            break;
        }

        case AT_READ_REQ:        /* AT+CTM2MSETPSK? */
        {
            ctiot_at_get_sec_mode(NULL, (UINT8*)&Security_Mode);
            snprintf(rspBuf, MAX_RSP_LEN, "+CTM2MSETPSK:%d", Security_Mode);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mIDAUTHPM(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mIDAUTHPM(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 idAuthStr[MAX_AUTHSTR_LEN + 1] = {0};
    UINT16 ret = 0;
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CTM2MIDAUTHPM =? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+CTM2MIDAUTHPM: \"<IDAuth_STR>\"");
            break;
        }
        case AT_SET_REQ:              /* AT+CTM2MIDAUTHPM= */
        {
            if(atGetStrValue(pAtCmdReq->pParamList, 0, idAuthStr, MAX_AUTHSTR_LEN, &length, (CHAR *)NULL) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            ret = ctiot_funcv1_set_idauth_pm(NULL, (CHAR *)idAuthStr);
            if(ret != CTIOT_NB_SUCCESS)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mREG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mREG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 ret = 0;
    CHAR rspBuf[MAX_RSP_LEN] = {0};

    switch (operaType)
    {
        case AT_EXEC_REQ:           /* AT+CTM2MREG *///TODO AT_EXEC_REQ
        {
            ret = ctiot_at_reg(NULL);
            if(ret != CTIOT_NB_SUCCESS)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            break;
        }

        case AT_READ_REQ:
        {
            int login_status = ctiot_at_get_loginstatus();
            snprintf(rspBuf, MAX_RSP_LEN, "+CTM2MREG: %d", login_status);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mUPDATE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mUPDATE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    
    UINT16 ret = 0;
    
    switch (operaType)
    {
        case AT_EXEC_REQ:
        {
            ctiot_funcv1_boot_flag_e bootflag = ctiot_at_get_bootflag();
            if(bootflag != BOOT_LOCAL_BOOTUP){
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            }else{
                ret = ctlwm2m_client_create();
                if(ret != CTIOT_NB_SUCCESS)
                {
                    rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
                }
                else
                {
                    rc = ctlwm2m_client_update(reqHandle);
                }
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mDEREG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mDEREG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 ret = 0;

    switch (operaType)
    {
        case AT_EXEC_REQ:           /* AT+CTM2MDEREG *///TODO AT_EXEC_REQ
        {
            ctiot_funcv1_boot_flag_e bootflag = ctiot_at_get_bootflag();
            if(bootflag != BOOT_LOCAL_BOOTUP){
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EA_OPERATION_FAILED_NOSESSION, NULL);
            }else{
                ret = ctlwm2m_client_create();
                if(ret != CTIOT_NB_SUCCESS)
                {
                    rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
                }
                else
                {
                    rc = ctlwm2m_client_dereg(reqHandle);
                }
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mSEND(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mSEND(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 *data = NULL;
    INT32 mode = 1;
    UINT16 length = 0;
    UINT16 ret = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CTM2MSEND =? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+CTM2MSEND: \"<Data>\"[,(0-3)]");
            break;
        }
        case AT_SET_REQ:      /* AT+CTM2MSEND= */
        {
            atGetStrLength(pAtCmdReq->pParamList, 0, &length);
            if(length > MAX_CONTENT_LEN || length == 0)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_DATA_LENGTH_OVERRRUN, NULL);
                break;
            }
            if (length%2 != 0)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_DATA_LENGTH_NOT_EVEN, NULL);
                break;
            }
            data = malloc(length+1);
            memset(data,0,length+1);
            if(atGetStrValue(pAtCmdReq->pParamList, 0, data, MAX_CONTENT_LEN+1, &length, (CHAR *)NULL) == AT_PARA_ERR)
            {
                free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_DATA_LENGTH_OVERRRUN, NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 1, &mode, CTM2MSEND_1_MODE_MIN, CTM2MSEND_1_MODE_MAX, CTM2MSEND_1_MODE_DEF) == AT_PARA_ERR)
            {
                free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }

            if(!atParserCheckHexstr(data))
            {
                free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            ctiot_funcv1_boot_flag_e bootflag = ctiot_at_get_bootflag();
            if(bootflag != BOOT_LOCAL_BOOTUP){
                free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EA_OPERATION_FAILED_NOSESSION, NULL);
            }else{
                ret = ctlwm2m_client_create();
                if(ret != CTIOT_NB_SUCCESS)
                {
                    free(data);
                    rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
                }
                else
                {
                    rc = ctlwm2m_client_send(reqHandle, (CHAR *)data, mode, 0/* seqNum for extend later*/);
                }
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mCMDRSP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mCMDRSP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    INT32 msgId = 0;
    INT32 rspCode = 0;
    INT32 dataFormat = 0;
    INT32 observe = 0;
    UINT8 *data = NULL;
    UINT8 token[MAX_TOKEN_LEN + 1] = {0};
    UINT8 uriStr[MAX_URISTR_LEN + 1] = {0};
    UINT16 ret = 0;
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CTM2MCMDRSP =? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+CTM2MCMDRSP: \"<msgid>\",\"<token>\",(0-2147483647),\"<uri_str>\",(0,1,8,9)[,(1,2,7,8,9),\"<data>\"]");
            break;
        }
        case AT_SET_REQ:              /* AT+CTM2MCMDRSP= */
        {
            if(pAtCmdReq->paramRealNum != 5 && pAtCmdReq->paramRealNum != 7)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &msgId, 0, 65535, 0) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            if(atGetStrValue(pAtCmdReq->pParamList, 1, token, MAX_TOKEN_LEN+1, &length, (CHAR *)NULL) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 2, &rspCode, 0, 0x7FFFFFFF, 0) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            if(atGetStrValue(pAtCmdReq->pParamList, 3, uriStr, MAX_URISTR_LEN+1,  &length, (CHAR *)NULL) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 4, &observe, 0, 9, 0) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 5, &dataFormat, 0, 9, 0) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            data = malloc(MAX_CONTENT_LEN+1);
            memset(data,0,MAX_CONTENT_LEN+1);
            if(atGetStrValue(pAtCmdReq->pParamList, 6, data, MAX_CONTENT_LEN+1, &length, (CHAR *)NULL) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
            }
            ret = ctiot_funcv1_cmdrsp(NULL, msgId, (CHAR *)token, rspCode, (CHAR *)uriStr, observe, (UINT8)dataFormat,(CHAR *)data);
            if(ret != CTIOT_NB_SUCCESS)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    ct_lwm2m_free(data);
    return rc;
}

/**
  \fn          CmsRetId ctm2mGETSTATUS(const AtCmdInputContext *pAtCmdReq->pParamList)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mGETSTATUS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    INT32 msgId = 0, queryType = 0;
    UINT16 ret = 0;
    CHAR rspBuf[MAX_RSP_LEN] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CTM2MGETSTATUS =? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+CTM2MGETSTATUS: (0,1,2,3)[,(0-65535)]");
            break;
        }
        case AT_SET_REQ:              /* AT+CTM2MGETSTATUS= */
        {
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &queryType, 0, 4, 0) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            if(queryType == 2 && pAtCmdReq->paramRealNum != 2)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 1, &msgId, 0, 65535, 0) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
            }
            ret = ctiot_at_get_status(NULL, queryType, msgId);
            if(queryType != 2)
                snprintf(rspBuf, MAX_RSP_LEN, "+CTM2MGETSTATUS: %d,%d", queryType, ret);
            else
                snprintf(rspBuf, MAX_RSP_LEN, "+CTM2MGETSTATUS: %d,%d,%d", queryType, ret, msgId);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mRMODE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mRMODE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 type = 0;
    CHAR rspBuf[32] = {0};

    switch (operaType)
    {
        case AT_READ_REQ:         /* AT+CTM2MRMODE? */
        {
            ctiot_funcv1_get_nnmi_mode(NULL, (uint8_t*)&type);
            snprintf(rspBuf, 32, "+CTM2MRMODE: %d", type);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }
        case AT_SET_REQ:      /* AT+CTM2MRMODE= */
        {
            ret = atGetNumValue(pAtCmdReq->pParamList, 0, &type, 0, 2, 0);
            if(ret == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }
            ctiot_funcv1_set_nnmi_mode(NULL, type);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mREAD(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mREAD(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT16 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT8* data = NULL;
    UINT16 datalen = 0;
    CHAR* rspBuf = NULL;
    CHAR* dataBuf = NULL;

    switch (operaType)
    {
        case AT_EXEC_REQ:      /* AT+CTM2MREAD */
        {
            ret = ctiot_funcv1_get_recvData(NULL, &datalen, &data);
            if(ret == CTIOT_EB_NO_RECV_DATA)
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                rspBuf = malloc(datalen*2 + 10);
                dataBuf = malloc(datalen*2 + 1);
                memset(rspBuf, 0, datalen*2 + 10);
                memset(dataBuf, 0, datalen*2 + 1);
                
                BOOL result = FALSE;
                result = atDataToHexString((UINT8*)dataBuf,data,datalen);
                if(result){
                    snprintf(rspBuf, datalen*2 + 6, "%d,%s", datalen, dataBuf);
                    rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
                }else{
                    ret = CTIOT_EB_OTHER;
                    rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
                }
                free(rspBuf);
                free(dataBuf);
                free(data);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}


/**
\fn        CmsRetId ctm2mCLNV(const AtCmdInputContext *pAtCmdReq)
\brief     at cmd clean NV function
\param[in] pAtCmdReq
\returns   CmsRetId
*/
CmsRetId  ctm2mCLNV(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    INT32 clnv = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CTM2MCLNV =? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+CTM2MCLNV: (0,1,2)");
            break;
        }
        case AT_SET_REQ:      /* AT+CTM2MCLNV= */
        {
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &clnv, 0, 2, 0) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }

            if( clnv == 0 )
            {
                ctiot_at_clean_context();
                ctiot_at_clean_params();
            }
            else if( clnv == 1 )
            {
                ctiot_at_clean_params();
            }
            else if( clnv == 2 )
            {
                ctiot_at_clean_context();
            }

            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
        }break;

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}

/**
\fn        CmsRetId ctm2mSETIMSI(const AtCmdInputContext *pAtCmdReq)
\brief     at cmd set imsi and save it to NVm 
\param[in] pAtCmdReq
\returns   CmsRetId
*/
CmsRetId  ctm2mSETIMSI(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 ret = 0;
    UINT16 length = 0;
    char imsi[16];
    CHAR rspBuf[30] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CTM2MSETIMSI =? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+CTM2MSETIMSI: <imsi>");
            break;
        }
        case AT_SET_REQ:      /* AT+CTM2MSETIMSI= */
        {
            if(atGetStrValue(pAtCmdReq->pParamList, 0, (UINT8*)imsi, 16, &length, (CHAR *)NULL) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_PARAMETER_VALUE_ERROR, NULL);
                break;
            }

            ret = ctiot_funcv1_set_imsi(NULL,imsi);
            if(ret != CTIOT_NB_SUCCESS)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }

            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
        }break;

        case AT_READ_REQ:        /* AT+CTM2MSETIMSI? */
        {
            ctiot_funcv1_get_imsi(NULL, imsi);
            snprintf(rspBuf, 30, "+CTM2MSETIMSI:%s", imsi);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }
        
        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}

#ifdef WITH_FOTA_RESUME
/**
\fn        CmsRetId ctm2mCLFOTANV(const AtCmdInputContext *pAtCmdReq)
\brief     at cmd clean FOTA related NV function
\param[in] pAtCmdReq
\returns   CmsRetId
*/
CmsRetId  ctm2mCLFOTANV(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_EXEC_REQ:      /* AT+CTM2MCLFOTANV */
        {
            c2f_clear_fotaparam();
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
        }break;

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EE_OPERATOR_NOT_SUPPORTED, NULL);
            break;
        }
    }

    return rc;
}
#endif



