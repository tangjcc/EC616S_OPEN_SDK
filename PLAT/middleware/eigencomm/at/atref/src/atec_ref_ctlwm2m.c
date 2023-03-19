/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    atec_ctlwm2m.c
 * Description:  at entry for ctlwm2m source file
 * History:      Rev1.0   2018-08-06
 *
 ****************************************************************************/
#ifdef  FEATURE_REF_AT_ENABLE
#include "bsp.h"
#include "at_util.h"
#include "ps_lib_api.h"
#include "atec_ref_ctlwm2m.h"
#include "at_ctlwm2m_task.h"
#include "ctiot_lwm2m_sdk.h"
#include "atec_ctlwm2m_cnf_ind.h"
#include "ec_ctlwm2m_api.h"

#include "ct_internals.h"

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
  \fn          CmsRetId ctm2mNCDP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mNCDP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR rspBuf[MAX_RSP_LEN] = {0};
    UINT8 serverIP[MAX_SERVER_LEN+1]={0};
    INT32 port = 0;
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+NCDP= */
        {
            if(atGetStrValue(pAtCmdReq->pParamList, 0, serverIP, MAX_SERVER_LEN+1,  &length, (CHAR *)NULL) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 1, &port, 0, 65535,5683) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            //1. No port is defined, before using default, first check is set before
            //2. not set before, use default
            //3. if 0, use 5683


            ctiot_funcv1_set_pm(NULL, (char *)serverIP, port, -1, reqHandle);

            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        case AT_READ_REQ:        /* AT+NCDP? */
        {
            ctiot_funcv1_get_pm(NULL, (char *)serverIP, (uint16_t*)&port, NULL);
            if (strlen((const CHAR*)serverIP) == 0)
            {
                rc = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            }
            else
            {
                snprintf(rspBuf, MAX_RSP_LEN, "+NCDP: %s,%d", serverIP, port);
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}




/**
  \fn          CmsRetId ctm2mQLWSREGIND(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mQLWSREGIND(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 type = 0;
    UINT8 regMode = 0;
    
    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+QLWSREGIND= */
        {
            ret = atGetNumValue(pAtCmdReq->pParamList, 0, &type, 0, 1, 0);
            if(ret == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }
            
            ctiot_funcv1_login_status_e loginStatus = ctiot_at_get_loginstatus();
            if(loginStatus == UE_NO_IP)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }
            
            ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }
            
            if (type == 0)
            {
            
                ctiot_funcv1_boot_flag_e bootflag = ctiot_at_get_bootflag();
                if(bootflag != BOOT_LOCAL_BOOTUP){
                    ret = ctiot_at_reg(NULL);
                    if(ret != CTIOT_NB_SUCCESS)
                    {
                        rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
                    }
                    else
                    {
                        rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                    }
                }else{
                    rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                }
            }
            else if (type == 1)
            {
                ctiot_funcv1_boot_flag_e bootflag = ctiot_at_get_bootflag();
                if(bootflag != BOOT_LOCAL_BOOTUP){
                    rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
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
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mQLWULDATA(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mQLWULDATA(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 length = 0;
    UINT8 * data = NULL;
    UINT16 atlength = 0;
    INT32 seq_num = 0;
    UINT8 status=0,seqNum = 0;
    UINT8 regMode = 0;

    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+QLWULDATA= */
        {
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &length, 1, 1024, 1)== AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }

            data = (UINT8*)ct_lwm2m_malloc(length*2+1);
            memset(data, 0, length*2+1);
            if(atGetStrValue(pAtCmdReq->pParamList, 1, data, length*2+1, &atlength, (CHAR *)NULL) == AT_PARA_ERR)
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            if (atlength % 2 != 0)
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            if(!atParserCheckHexstr(data))
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }

            if(atGetNumValue(pAtCmdReq->pParamList, 2, &seq_num, 0, 255, 0) == AT_PARA_ERR)
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            
            ctiot_funcv1_login_status_e loginStatus = ctiot_at_get_loginstatus();
            if(loginStatus == UE_NO_IP)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_NO_IP, NULL);
                break;
            }
            
            ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }
            
            ctiot_funcv1_boot_flag_e bootflag = ctiot_at_get_bootflag();
            if(bootflag != BOOT_LOCAL_BOOTUP){
                ct_lwm2m_free(data);
                ret = ctiot_at_reg(NULL);
                if(ret != CTIOT_NB_SUCCESS)
                {
                    rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
                }
                else
                {
                    rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                }
            }else{
                if(seq_num != 0){
                    ctiot_funcv1_get_data_status(NULL, &status, &seqNum);
                    if(seq_num == seqNum && status == SENT_WAIT_RESP)//last msg hasn't complete send
                    {
                        ct_lwm2m_free(data);
                        rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_UPLINK_BUSY, NULL);
                        break;
                    }
                }
                ret = ctlwm2m_client_create();
                if(ret != CTIOT_NB_SUCCESS)
                {
                    ct_lwm2m_free(data);
                    rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
                }
                else
                {
                    rc = ctlwm2m_client_send(reqHandle, (CHAR *)data, SENDMODE_NON, seq_num);
                }
            }

            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }
    return rc;
}


/**
  \fn          CmsRetId ctm2mQLWULDATAEX(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mQLWULDATAEX(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 length = 0;
    UINT8 * data = NULL;
    INT32 mode = 1;
    UINT16 atlength = 0;
    INT32 seq_num = 0;
    UINT8 status=0,seqNum = 0;
    UINT8 regMode = 0;

    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+QLWULDATAEX= */
        {
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &length, 1, 1024, 1)== AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }

            data = (UINT8*)ct_lwm2m_malloc(length*2+1);
            memset(data, 0, length*2+1);
            if(atGetStrValue(pAtCmdReq->pParamList, 1, data, length*2+1, &atlength, (CHAR *)NULL) == AT_PARA_ERR)
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            if (atlength % 2 != 0)
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            if(!atParserCheckHexstr(data))
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 2, &mode, 0, 0x101, 0) == AT_PARA_ERR)
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }

            ECOMM_TRACE(UNILOG_CTLWM2M, ctm2mQLWULDATAEX_1, P_INFO, 2, "data's length=%d mode=%x", length, mode);


            if(atGetNumValue(pAtCmdReq->pParamList, 3, &seq_num, 0, 255, 0) == AT_PARA_ERR)
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            
            ctiot_funcv1_login_status_e loginStatus = ctiot_at_get_loginstatus();
            if(loginStatus == UE_NO_IP)
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_NO_IP, NULL);
                break;
            }
            
            ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }

            if (mode == 0)
                mode = SENDMODE_NON;
            else if (mode == 0x1)
                mode = SENDMODE_NON_REL;
            else if (mode == 0x10)
                mode = SENDMODE_NON_REL;//SENDMODE_NON_RAR;
            else if (mode == 0x100)
                mode = SENDMODE_CON;
            else if (mode == 0x101)
                mode = SENDMODE_CON_REL;//SENDMODE_CON_RAR;
                
            ctiot_funcv1_boot_flag_e bootflag = ctiot_at_get_bootflag();
            if(bootflag != BOOT_LOCAL_BOOTUP)
            {
                ct_lwm2m_free(data);
                ret = ctiot_at_reg(NULL);
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
                if(seq_num != 0){
                    ctiot_funcv1_get_data_status(NULL, &status, &seqNum);
                    if(seq_num == seqNum && status == SENT_WAIT_RESP)//last msg hasn't complete send
                    {
                        ct_lwm2m_free(data);
                        rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_UPLINK_BUSY, NULL);
                        break;
                    }
                }
                ret = ctlwm2m_client_create();
                if(ret != CTIOT_NB_SUCCESS)
                {
                    ct_lwm2m_free(data);
                    rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
                }
                else
                {
                    rc = ctlwm2m_client_send(reqHandle, (CHAR *)data, mode, seq_num);
                }
            }

            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }
    return rc;
}

/**
  \fn          CmsRetId ctm2mQLWULDATASTATUS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mQLWULDATASTATUS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    AtCmdReqType operaType = (AtCmdReqType)(pAtCmdReq->operaType);
    CHAR rspBuf[MAX_RSP_LEN] = {0};
    UINT8 status=0,seqNum = 0;
    UINT8 regMode = 0;
    switch (operaType)
    {
        case AT_READ_REQ:              /* AT+QLWULDATASTATUS? */
        {
            ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }

            ctiot_funcv1_get_data_status(NULL, &status, &seqNum);
            if(seqNum == 0)
                snprintf(rspBuf, MAX_RSP_LEN, "+QLWULDATASTATUS: %d", status);
            else
                snprintf(rspBuf, MAX_RSP_LEN, "+QLWULDATASTATUS: %d,%d", status, seqNum);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mQLWFOTAIND(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mQLWFOTAIND(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 type = 0;
    UINT8 regMode = 0;

    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+QLWFOTAIND= */
        {
            ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }
            ret = atGetNumValue(pAtCmdReq->pParamList, 0, &type, 0, 5, 0);
            if(ret == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            ctiot_at_dfota_mode(type);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}



/**
  \fn          CmsRetId ctm2mQREGSWT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mQREGSWT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 type = 0;
    CHAR rspBuf[MAX_RSP_LEN] = {0};
    UINT8 regMode = 0;

    switch (operaType)
    {
        case AT_READ_REQ:         /* AT+QREGSWT? */
        {
            ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
            if(type == 3) type = 2;
            snprintf(rspBuf, MAX_RSP_LEN, "+QREGSWT: %d", regMode);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }
        case AT_SET_REQ:      /* AT+QREGSWT= */
        {
            ret = atGetNumValue(pAtCmdReq->pParamList, 0, &type, 0, 2, 0);
            if(ret == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            ctiot_funcv1_set_regswt_mode(NULL, type);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}


/**
  \fn          CmsRetId ctm2mNMGS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mNMGS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 length = 0;
    UINT8 * data = NULL;
    UINT16 atlength = 0;
    INT32 seq_num = 0;
    UINT8 status=0,seqNum = 0;
    UINT8 regMode = 0;

    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+NMGS= */
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, ctm2mNMGS_0, P_INFO, 1, "ctm2mNMGS paramNumb=%d", pAtCmdReq->paramRealNum);

            ret = atGetNumValue(pAtCmdReq->pParamList, 0, &length, 1, 1024, 1);
            if(ret == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }

            data = (UINT8*)ct_lwm2m_malloc(length*2 + 1);
            memset(data, 0, length + 1);
            if(atGetStrValue(pAtCmdReq->pParamList, 1, data, length*2 + 1, &atlength, (CHAR*)NULL)  == AT_PARA_ERR)
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }

            if (atlength%2 != 0)
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            if(!atParserCheckHexstr(data))
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }

            if(atGetNumValue(pAtCmdReq->pParamList, 2, &seq_num, 0, 255, 0) == AT_PARA_ERR)
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }

            ctiot_funcv1_login_status_e loginStatus = ctiot_at_get_loginstatus();
            if(loginStatus == UE_NO_IP)
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_NO_IP, NULL);
                break;
            }

            ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                ct_lwm2m_free(data);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }

            ctiot_funcv1_boot_flag_e bootflag = ctiot_at_get_bootflag();
            if(bootflag != BOOT_LOCAL_BOOTUP){
                ct_lwm2m_free(data);
                ret = ctiot_at_reg(NULL);
                if(ret != CTIOT_NB_SUCCESS)
                {
                    rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
                }
                else
                {
                    rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                }
            }else{
                if(seq_num != 0){
                    ctiot_funcv1_get_data_status(NULL, &status, &seqNum);
                    if(seqNum == seqNum && status == SENT_WAIT_RESP)//last msg hasn't complete send
                    {
                        ct_lwm2m_free(data);
                        rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_UPLINK_BUSY, NULL);
                        break;
                    }
                }
                ret = ctlwm2m_client_create();
                if(ret != CTIOT_NB_SUCCESS)
                {
                    ct_lwm2m_free(data);
                    rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, ret, NULL);
                }
                else
                {
                    rc = ctlwm2m_client_send(reqHandle, (CHAR *)data, SENDMODE_NON, seq_num);
                }
            }

            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }
    
    return rc;
}


/**
  \fn          CmsRetId ctm2mNSMI(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mNSMI(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 type = 0;
    CHAR rspBuf[32] = {0};
    UINT8 regMode = 0;

    switch (operaType)
    {
        case AT_READ_REQ:         /* AT+NSMI? */
        {
            ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }
            ctiot_funcv1_get_nsmi_mode(NULL, (uint8_t*)&type);
            snprintf(rspBuf, 32, "+NSMI: %d", type);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }
        
        case AT_SET_REQ:      /* AT+NSMI= */
        {
            ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }
            ret = atGetNumValue(pAtCmdReq->pParamList, 0, &type,0, 1, 0);
            if(ret == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            ctiot_funcv1_set_nsmi_mode(NULL, type);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}


/**
  \fn          CmsRetId ctm2mNNMI(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mNNMI(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 type = 0;
    CHAR rspBuf[32] = {0};
    UINT8 regMode = 0;

    switch (operaType)
    {
        case AT_READ_REQ:         /* AT+NNMI? */
        {
            ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }
            ctiot_funcv1_get_nnmi_mode(NULL, (uint8_t*)&type);
            snprintf(rspBuf, 32, "+NNMI: %d", type);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }
        case AT_SET_REQ:      /* AT+NNMI= */
        {
            ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }
            ret = atGetNumValue(pAtCmdReq->pParamList, 0, &type, 0, 2, 0);
            if(ret == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            ctiot_funcv1_set_nnmi_mode(NULL, type);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mNMGR(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mNMGR(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT16 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT8* data = NULL;
    UINT16 datalen = 0;
    CHAR* rspBuf = NULL;
    CHAR* dataBuf = NULL;
    UINT8 regMode = 0;

    switch (operaType)
    {
        case AT_EXEC_REQ:      /* AT+NMGR */
        {
            ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }
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
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mNMSTATUS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId  ctm2mNMSTATUS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR rspBuf[32] = {0};
    CHAR statBuf[16] = {0};
    UINT8 regMode = 0;
    ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
    
    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+NMSTATUS=? */
        {
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"UNINITIALISED\r\nREGISTERING\r\nMO_DATA_ENABLED\r\nDEREGISTERED\r\nREG_FAILED\r\nNO_UE_IP\r\n");
            break;
        }
        
        case AT_READ_REQ:      /* AT+NMSTATUS? */
        {
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;
            }
            ctiot_funcv1_login_status_e  reg_status = ctiot_at_get_loginstatus();
            switch(reg_status)
            {
            case UE_NOT_LOGINED:
                sprintf(statBuf, "UNINITIALISED");
                break;
            case UE_LOGINING:
                sprintf(statBuf, "REGISTERING");
                break;
            case UE_LOGINED:
                sprintf(statBuf, "MO_DATA_ENABLED");
                break;
            case UE_LOG_OUT:
                sprintf(statBuf, "DEREGISTERED");
                break;
            case UE_LOG_FAILED:
                sprintf(statBuf, "REG_FAILED");
                break;
            case UE_NO_IP:
                sprintf(statBuf, "NO_UE_IP");
                break;
            default:
                sprintf(statBuf, "UNINITIALISED");
                break;
            }
            snprintf(rspBuf, 32, "+NMSTATUS: %s", statBuf);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mQRESETDTLS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mQRESETDTLS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    switch (operaType)
    {
        case AT_EXEC_REQ:      /* AT+QRESETDTLS */
        {
            ctiot_at_reset_dtls(NULL);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mQSETPSK(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mQSETPSK(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR rspBuf[MAX_RSP_LEN] = {0};
    UINT16 ret = 0;

    UINT8 pskId[MAX_PSKID_LEN+1] = {0};
    UINT8 psk[MAX_PSK_LEN+1] = {0};
    UINT16 psklength = 0;
    UINT16 pskidlength = 0;


    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+QSETPSK= */
        {
            if(atGetStrValue(pAtCmdReq->pParamList, 0, pskId, MAX_PSKID_LEN, &pskidlength, (CHAR *)NULL) == AT_PARA_ERR)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, ctm2mQSETPSK_1, P_INFO, 1, "param0 error=%d", pskidlength);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            if(atGetStrValue(pAtCmdReq->pParamList, 1, psk, MAX_PSK_LEN+1, &psklength, (CHAR *)NULL) == AT_PARA_ERR)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, ctm2mQSETPSK_2, P_INFO, 1, "param1 error=%d", pskidlength);
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }

            if(strcmp((char*)pskId, "0") == 0)
            {
                appGetImeiNumSync((char*)pskId); 
            }
            char* pskBuf = malloc(psklength/2+1);
            memset(pskBuf, 0, psklength/2+1);
            cmsHexStrToHex((UINT8*)pskBuf, psklength/2, (CHAR *)psk, psklength);
            ret = ctiot_at_set_psk(NULL, 2, (uint8_t*)pskId, (uint8_t*)pskBuf);
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

        case AT_READ_REQ:        /* AT+QSETPSK? */
        {
            ctiot_at_get_pskid(NULL, (char*)pskId);
            snprintf(rspBuf, MAX_RSP_LEN, "+QSETPSK: %s,***", (char*)pskId);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mQDTLSSTAT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mQDTLSSTAT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT16 ret = 0;
    CHAR rspBuf[16] = {0};
    UINT8 status = 1;

    switch (operaType)
    {
        case AT_READ_REQ:      /* AT+QDTLSSTAT? */
        {
            ret = ctiot_at_get_dtls_status(NULL, &status);
            if(ret != CTIOT_NB_SUCCESS)
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, "+QDTLSSTAT: 1");
            }
            else
            {
                snprintf(rspBuf, 16, "+QDTLSSTAT: %d", status);
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mQLWSERVERIP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mQLWSERVERIP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR rspBuf[MAX_RSP_LEN] = {0};
    UINT8 typeStr[8] = {0};
    UINT8 serverIP[MAX_SERVER_LEN+1]={0};
    INT32 port = 0;
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+QLWSERVERIP= */
        {
            if(atGetStrValue(pAtCmdReq->pParamList, 0, typeStr, 8,  &length, (CHAR *)NULL) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            if(atGetStrValue(pAtCmdReq->pParamList, 1, serverIP, MAX_SERVER_LEN+1,  &length, (CHAR *)NULL) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 2, &port, 1, 65535,5683) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            //1. No port is defined, before using default, first check is set before
            //2. not set before, use default
            //3. if 0, use 5683


            if(strncasecmp((const CHAR*)typeStr, "LWM2M", strlen("LWM2M")) == 0){//set IoT server IP
                ctiot_funcv1_set_pm(NULL, (char *)serverIP, port, -1, reqHandle);
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }else if(strncasecmp((const CHAR*)typeStr, "BS", strlen("BS")) == 0){//set Bootstrap server IP
                ctiot_funcv1_set_bs_server(NULL, (char *)serverIP, port, reqHandle);
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }else if(strncasecmp((const CHAR*)typeStr, "DEL", strlen("DEL")) == 0){//delete Bootstrap/IoT server IP
                ctiot_funcv1_del_serverIP(NULL, (char *)serverIP, port);
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            
            break;
        }

        case AT_READ_REQ:        /* AT+QLWSERVERIP? */
        {
            ctiot_funcv1_get_pm(NULL, (char *)serverIP, (uint16_t*)&port, NULL);
            if (strlen((const CHAR*)serverIP) != 0)
            {
                snprintf(rspBuf, MAX_RSP_LEN, "+QLWSERVERIP: LWM2M,%s,%d", serverIP, port);
                rc = atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, (CHAR*)rspBuf);
            }
            ctiot_funcv1_get_bs_server(NULL,(char *)serverIP, (uint16_t*)&port);
            if (strlen((const CHAR*)serverIP) != 0)
            {
                snprintf(rspBuf, MAX_RSP_LEN, "+QLWSERVERIP: BS,%s,%d", serverIP, port);
                rc = atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, (CHAR*)rspBuf);
            }
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mQCFG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mQCFG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR rspBuf[MAX_RSP_LEN] = {0};
    UINT8 typeStr[20] = {0};
    UINT8 endpointname[ENDPOINT_NAME_SIZE+1]={0};
    INT32 lifeTime = 0;
    uint32_t contextLifetime = 0;
    CHAR bindingmode = 0;
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+QCFG= */
        {
            if(atGetStrValue(pAtCmdReq->pParamList, 0, typeStr, 20,  &length, (CHAR *)NULL) != AT_PARA_ERR)
            {
                if(strncasecmp((const CHAR*)typeStr, "LWM2M/Lifetime", strlen("LWM2M/Lifetime")) == 0)
                {
                    ret = atGetNumValue(pAtCmdReq->pParamList, 1, &lifeTime, 0, 30*86400, 86400);
                    if(ret == AT_PARA_DEFAULT){
                        ctiot_funcv1_get_lifetime(NULL, &contextLifetime);
                        snprintf(rspBuf, MAX_RSP_LEN, "+QCFG: LWM2M/Lifetime,%d", contextLifetime);
                        rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
                    }else if(ret == AT_PARA_OK){
                        ctiot_funcv1_set_lifetime(NULL, (UINT32)lifeTime);
                        rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                    }
                }
                if(strncasecmp((const CHAR*)typeStr, "LWM2M/EndpointName", strlen("LWM2M/EndpointName")) == 0)
                {
                    ret = atGetStrValue(pAtCmdReq->pParamList, 1, endpointname, ENDPOINT_NAME_SIZE, &length , (CHAR *)NULL);
                    if(ret == AT_PARA_DEFAULT){
                        ctiot_funcv1_get_ep(NULL, (CHAR*)endpointname);
                        snprintf(rspBuf, MAX_RSP_LEN, "+QCFG: LWM2M/EndpointName,%s", endpointname);
                        rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
                    }else if(ret == AT_PARA_OK){
                        ctiot_funcv1_set_ep(NULL,  (CHAR*)endpointname);
                        rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                    }else{
                        rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                    }
                }
                if(strncasecmp((const CHAR*)typeStr, "LWM2M/BindingMode", strlen("LWM2M/BindingMode")) == 0)
                {
                    ret = atGetNumValue(pAtCmdReq->pParamList, 1, (INT32*)&bindingmode, 1, 2, NULL);
                    if(ret == AT_PARA_DEFAULT){
                        ctiot_funcv1_get_mod(NULL, NULL, NULL, (uint8_t*)&bindingmode, NULL, NULL, NULL);
                        snprintf(rspBuf, MAX_RSP_LEN, "+QCFG: LWM2M/BindingMode,%d", bindingmode);
                        rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
                    }else if(ret == AT_PARA_OK){
                        ctiot_at_set_mod(NULL, 3, bindingmode);
                    }else{
                        rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                    }
                }
            }
            break;
        }

        case AT_READ_REQ:        /* AT+QCFG? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }
        
        case AT_TEST_REQ:        /* AT+QCFG=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mNQMGR(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mNQMGR(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR rspBuf[MAX_RSP_LEN] = {0};
    uint32_t buffered=0,received=0,dropped=0;
    UINT8 regMode = 0;

    switch (operaType)
    {
        case AT_EXEC_REQ:      /* AT+NQMGR */
        {
            ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_NO_IP, NULL);
                break;
            }
            ctiot_funcv1_get_dlpm(NULL,&buffered,&received,&dropped);
            snprintf(rspBuf, MAX_RSP_LEN, "BUFFERED=%d,RECEIVED=%d,DROPPED=%d", buffered,received,dropped);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mNQMGS(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValueList
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mNQMGS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR rspBuf[MAX_RSP_LEN] = {0};
    uint32_t pending=0,sent=0,error=0;
    UINT8 regMode = 0;

    switch (operaType)
    {
        case AT_EXEC_REQ:      /* AT+NQMGS */
        {
            ctiot_funcv1_get_regswt_mode(NULL,(uint8_t*)&regMode);
            if(regMode == 3)//at+qrerswt=2 and has reset
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_NO_IP, NULL);
                break;
            }
            ctiot_funcv1_get_ulpm(NULL,&pending,&sent,&error);
            snprintf(rspBuf, MAX_RSP_LEN, "PENDING=%d,SENT=%d,ERROR=%d", pending,sent,error);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}



/**
  \fn          CmsRetId ctm2mQSECSWT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mQSECSWT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR rspBuf[MAX_RSP_LEN] = {0};
    UINT8 type=0, nattype=0;
    
    switch (operaType)
    {
        case AT_READ_REQ:         /* AT+QSECSWT? */
        {
            ctiot_funcv1_get_dtlstype(NULL,&type,&nattype);
            snprintf(rspBuf, MAX_RSP_LEN, "+QSECSWT: %d,%d", type, nattype);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)rspBuf);
            break;
        }
        case AT_SET_REQ:      /* AT+QSECSWT= */
        {
            if(atGetNumValue(pAtCmdReq->pParamList, 0, (INT32*)&type, 0, 1, 0) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                break;
            }
            if(type == 1){
                if(atGetNumValue(pAtCmdReq->pParamList, 1, (INT32*)&nattype, 0, 1, 0) == AT_PARA_ERR)
                {
                    rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
                    break;
                }
            }
            ctiot_funcv1_set_dtlstype(NULL, type, nattype);
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ctm2mQCRITICALDATA(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   paramValuePtr
  \param[in]   paramNumb
  \param[in]   *infoPtr
  \param[in]   *xidPtr
  \param[in]   *argPtr
  \returns     CmsRetId
*/
CmsRetId  ctm2mQCRITICALDATA(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 type=0;
    
    switch (operaType)
    {
        case AT_SET_REQ:      /* AT+QCRITICALDATA= */
        {
            if(atGetNumValue(pAtCmdReq->pParamList, 0, (INT32*)&type, 1, 1, 1) == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CTIOT_EB_NO_IP, NULL);//bc28 also does not support
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_CTM2M_ERROR, CME_INCORRECT_PARAM, NULL);
            break;
        }
    }

    return rc;
}


#endif


