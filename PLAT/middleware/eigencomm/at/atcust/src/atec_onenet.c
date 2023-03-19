/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_onenet.c
*
*  Description: Process onenet related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

#include "at_util.h"

#include "atec_onenet.h"
#include "at_onenet_task.h"

#include "cis_api.h"
#include "cis_if_sys.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif

#include "netmgr.h"
#include "ps_lib_api.h"

extern OnenetSIMOTAContext gOnenetSimOtaCtx;

CHAR *defaultCisLocalPort = "49250";

#define _DEFINE_AT_REQ_FUNCTION_LIST_



static BOOL onenetRaiStringToRaiFlag(INT32 *raiflag, UINT8 *RaiString)
{
    UINT32 tmp;

    if(RaiString[0]=='\0')
    {
        *raiflag = 0;
        return true;
    }

    if(cmsHexStrToUInt(&tmp, RaiString, 5) == true)
    {
        #ifdef FEATURE_REF_AT_ENABLE
        if(tmp == 0x200)
        {
            *raiflag = 1;
            return true;
        }
        else if(tmp == 0x400)
        {
            *raiflag = 2;
            return true;
        }
        else if(tmp == 0)
        {
            *raiflag = 0;
            return true;
        }
        else
        {
            *raiflag = 0;
            return false;
        }
        #else
        if(tmp == 0x1)
        {
            *raiflag = 1;
            return true;
        }
        else if(tmp == 0x2)
        {
            *raiflag = 2;
            return true;
        }
        else if(tmp == 0)
        {
            *raiflag = 0;
            return true;
        }
        else
        {
            *raiflag = 0;
            return false;
        }
        #endif
    }

    *raiflag = 0;
    return false;
}



/**
  \fn          CmsRetId onenetCONFIG(const AtCmdInputContext *pAtCmdReq)
  \brief       config bootstrap mode, server(bootstrap server) ip and port
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetCONFIG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 mode;
    UINT8 ip[16] = {0};
    UINT8 port[6] = {0};
    UINT8 psk[17] = {0};
    UINT8 auth_code[17] = {0};
    INT32 cisError=CIS_ERRID_PARAMETER_ERROR;
    CHAR respStr[64];
    UINT16 length = 0;

    switch (operaType)
    {

        case AT_TEST_REQ:         /* AT+MIPLCONFIG=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+MIPLCONFIG: (0,7),<parameter1>[,<parameter2>]");
            break;
        }

        case AT_SET_REQ:              /* AT+MIPLCONFIG= */
        {
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &mode, 0, 7, 1) == AT_PARA_OK)
            {
                switch(mode)
                {
                    case 0:
                    case 1:
                    {
                        if(atGetStrValue(pAtCmdReq->pParamList, 1, ip, 16, &length, (CHAR*)NULL) == AT_PARA_OK)
                        {
                            ip_addr_t   ipAddr;
                            if (ipaddr_aton((CHAR* )ip, &ipAddr)){
                                if(atGetStrValue(pAtCmdReq->pParamList, 2, port, 6, &length, (CHAR*)NULL) == AT_PARA_OK)
                                {
                                    if(atCheckForNumericOnlyChars(port)){
                                    cisError = onenetServerIPConfig(0, (BOOL)mode, (CHAR*)ip, (CHAR*)port);
                                    }
                                }
                            }
                        }
                        break;
                    }

                    case 2:
                    {
                        INT32 rsp_timeout = 2;
                        INT32 parameter1 = 0;
              
                        if(atGetNumValue(pAtCmdReq->pParamList, 1, &parameter1, 1, 1, 0) == AT_PARA_OK)
                        {
                            if(atGetNumValue(pAtCmdReq->pParamList, 2, &rsp_timeout, 2, 20, 2) == AT_PARA_OK)
                            {
                                cisError = onenetRspTimeoutConfig(0, (UINT8)rsp_timeout);
                            }
                        }
                        break;
                    }

                    case 3:
                    {
                        INT32 obs_autoack = 1;
                        
                        if(atGetNumValue(pAtCmdReq->pParamList, 1, &obs_autoack, 0, 1, 1) == AT_PARA_OK)
                        {
                            cisError = onenetObsAutoackConfig(0, (UINT8)obs_autoack);
                        }
                        break;
                    }

                    case 4:
                    {
                        INT32 auth_enable;
                        length = 0;

                        if(atGetNumValue(pAtCmdReq->pParamList, 1, &auth_enable, 0, 1, 0) == AT_PARA_OK)
                        {
                            if(atGetStrValue(pAtCmdReq->pParamList, 2, auth_code, 17, &length, (CHAR*)NULL) != AT_PARA_ERR)
                            {
                                cisError = onenetObsAuthConfig(0, (BOOL)auth_enable, auth_code);
                            }
                        }
                        break;

                    }
                    case 5:
                    {
                        INT32 dtls_enable;
                        memset(psk, 0, sizeof(psk));
                        length = 0;
                        
                        if(atGetNumValue(pAtCmdReq->pParamList, 1, &dtls_enable, 0, 1, 0) == AT_PARA_OK)
                        {
                            if(atGetStrValue(pAtCmdReq->pParamList, 2, psk, 17, &length, (CHAR*)NULL) != AT_PARA_ERR)
                            {
                                cisError = onenetObsDTLSConfig(0, (BOOL)dtls_enable, psk);
                            }
                        }
                        break;
                    }
                    case 6:
                    {
                        INT32 write_format = 0;

                        if(atGetNumValue(pAtCmdReq->pParamList, 1, &write_format, 0, 1, 0) == AT_PARA_OK)
                        {
                            cisError = onenetWriteFormatConfig(0, (UINT8)write_format);
                        }
                        break;
                    }

                    case 7:
                    {
                        INT32 buf_cfg = 0;
                        INT32 buf_urc_mode = 0;
                        
                        if(atGetNumValue(pAtCmdReq->pParamList, 1, &buf_cfg, 0, 3, 0) == AT_PARA_OK)
                        {
                            if(atGetNumValue(pAtCmdReq->pParamList, 2, &buf_urc_mode, 0, 1, 1) == AT_PARA_OK)
                            {
                                cisError = onenetMIPLRDConfig(0, (UINT8)buf_cfg, (UINT8)buf_urc_mode);
                            }
                        }

                        break;
                    }

                    default:
                        OsaCheck(0, mode, 0, 0);
                    

                }


            }

            if(cisError == CIS_ERRID_OK)
            {
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( cisError), NULL);
            }
            break;
        }

        case AT_READ_REQ:              /* AT+MIPLCONFIG? */
        {
            BOOL bsmode = false;
            
            memset(ip,0,sizeof(ip));
            memset(port,0,sizeof(port));
            onenetServerIPGet(0, &bsmode, (CHAR *)ip, (CHAR *)port);
            
            memset(respStr, 0, sizeof(respStr));
            snprintf(respStr, 64, "+MIPLCONFIG: %d,%s,%s\r\n", bsmode, ip, port);
            ret = atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, (CHAR*)respStr);
            
            for(UINT8 mode=2; mode<8; mode++)
            {
                if(mode == 2)
                {
                    UINT8 rsp_timeout;
                    memset(respStr, 0, sizeof(respStr));
                    onenetRspTimeoutGet(0, &rsp_timeout);
                    snprintf(respStr, 64, "+MIPLCONFIG: 2,%d\r\n", rsp_timeout);
                    atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, (CHAR*)respStr);
                }
                else if(mode == 3)
                {
                    UINT8 obs_autoack;
                    memset(respStr, 0, sizeof(respStr));
                    onenetObsAutoackGet(0, &obs_autoack);
                    snprintf(respStr, 64, "+MIPLCONFIG: 3,%d\r\n", obs_autoack);
                    atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, (CHAR*)respStr);
                }
                else if(mode == 4)
                {
                    UINT8 auth_enable;
                    memset(auth_code, 0, sizeof(auth_code));
                    memset(respStr, 0, sizeof(respStr));
                    onenetObsAuthGet(0, &auth_enable, auth_code);
                    auth_code[16] = '\0';
                    if(auth_enable == 0)
                        snprintf(respStr, 64, "+MIPLCONFIG: 4,0\r\n");
                    else
                        snprintf(respStr, 64, "+MIPLCONFIG: 4,%s\r\n", auth_code);
                    atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, (CHAR*)respStr);
                }
                else if(mode == 5)
                {
                    UINT8 dtls_enable;
                    memset(psk, 0, sizeof(psk));
                    memset(respStr, 0, sizeof(respStr));

                    onenetObsDTLSGet(0, &dtls_enable, psk);
                    
                    psk[16] = '\0';
                    if(dtls_enable == 0)
                        snprintf(respStr, 64, "+MIPLCONFIG: 5,0\r\n");
                    else
                        snprintf(respStr, 64, "+MIPLCONFIG: 5,%s\r\n", psk);
                    atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, (CHAR*)respStr);
                }
                else if(mode == 6)
                {
                    UINT8 write_format;
                    memset(respStr, 0, sizeof(respStr));
                    onenetObsFormatGet(0, &write_format);
                    snprintf(respStr, 64, "+MIPLCONFIG: 6,%d\r\n", write_format);
                    atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, (CHAR*)respStr);

                }
                else if(mode == 7)
                {
                    UINT8 buf_cfg;
                    UINT8 buf_urc_mode;
                    memset(respStr, 0, sizeof(respStr));
                    onenetMIPLRDCfgGet(0, &buf_cfg, &buf_urc_mode);
                    snprintf(respStr, 64, "+MIPLCONFIG: 7,%d,%d\r\n", buf_cfg, buf_urc_mode);
                    atcReply(reqHandle, AT_RC_CONTINUE, ATC_SUCC_CODE, (CHAR*)respStr);
                }

            }

            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId onenetCREATE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetCREATE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    UINT32 ref;
    cis_ret_t cis_ret;
    CHAR RspBuf[16];
    onenetContext *onenet;

    switch (operaType)
    {
        case AT_EXEC_REQ:        /* AT+MIPLCREATE */
        {
            if(cisnet_attached_state(NULL) == false)
            {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NO_NETWORK), NULL);
                break;
            }
            onenet = onenetCreateInstance();
            if (onenet == NULL) {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            } else {
                onenet->configLen = createConfigBuf(onenet->onenetId, onenet->configBuf);
                ECOMM_TRACE(UNILOG_ONENET, onenetCREATE_1, P_VALUE, 1, "ConfigBuf Len = %d", onenet->configLen);
                ECOMM_DUMP(UNILOG_ONENET, onenetCREATE_2, P_VALUE, "ConfigBuf Dump: ", onenet->configLen, onenet->configBuf);
                cis_ret = cis_init(&onenet->cis_context, onenet->configBuf, onenet->configLen, NULL, defaultCisLocalPort);
                if (cis_ret != CIS_RET_OK)
                {
                    onenetDeleteInstance(onenet);
                    ECOMM_TRACE(UNILOG_ONENET, onenetCREATE_3, P_INFO, 1, "cis_init ERROR=%d", cis_ret);
                    ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_SDK_ERROR), NULL);
                } else {
                    onenetSleepVote(SYSTEM_BUSY);
                    ref = onenet->onenetId;
                    sprintf(RspBuf, "+MIPLCREATE: %d",(INT32)ref);
                    onenet->bRestoreFlag = 1;
                    onenet->reqhandle = reqHandle;
                    onenet_athandle_create();
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)RspBuf);
                }

            }

            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId onenetDELETE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetDELETE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    INT32 ref;
    INT32 cisError;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+MIPLDELETE=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+MIPLDELETE: (0)");
            break;
        }

        case AT_SET_REQ:              /* AT+MIPLDELETE= */
        {
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &ref, 0, 0, 0) != AT_PARA_ERR)
            {
                cisError = onenetDelete(ref);
                if(cisError == CIS_ERRID_OK)
                {
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
                    onenetClearFlash(false);
                    onenetSleepVote(SYSTEM_FREE);
                }
                else
                {
                    ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( cisError), NULL);
                }
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_PARAMETER_ERROR), NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }
    return ret;
}

/**
  \fn          CmsRetId onenetOPEN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetOPEN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 ref, lifetime, timeout;
    onenetContext *pContex = NULL;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+MIPLOPEN=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+MIPLOPEN: (0),(0xF-0xFFFFFFF),(30-0xFFFF)");
            break;
        }

        case AT_SET_REQ:              /* AT+MIPLOPEN= */
        {
            if(cisnet_attached_state(NULL) == false)
            {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NO_NETWORK), NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &ref, 0, 0, 0) != AT_PARA_ERR)
            {
                if(atGetNumValue(pAtCmdReq->pParamList, 1, &lifetime, LIFETIME_LIMIT_MIN, LIFETIME_LIMIT_MAX, 86400) != AT_PARA_ERR)
                {
                    if(atGetNumValue(pAtCmdReq->pParamList, 2, &timeout, 30, 65535, 30) != AT_PARA_ERR)
                    {
                        pContex = onenetSearchInstance(ref);
                        if(pContex == NULL)
                        {
                            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NO_INSTANCE), NULL);
                            break;
                        }
                        if(pContex->bConnected == true)
                        {
                            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_STATUS_ERROR), NULL);
                            break;
                        }
                        onenetSleepVote(SYSTEM_BUSY);
                        ret = onenet_client_open(reqHandle, ref, lifetime, timeout);
                    }
                }
            }
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId onenetCLOSE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetCLOSE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 ref;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+MIPLCLOSE=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+MIPLCLOSE: (0)");
            break;
        }

        case AT_SET_REQ:              /* AT+MIPLCLOSE= */
        {
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &ref, 0, 0, 0) != AT_PARA_ERR)
            {
                onenetContext *pContex = NULL;
                pContex = onenetSearchCisInstance(ref);
                if(pContex == NULL)
                {
                    ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NO_INSTANCE), NULL);
                    break;
                }

                if(pContex->bConnected == false)
                {
                    ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_STATUS_ERROR), NULL);
                    break;
                }
                pContex->closeHandle = reqHandle;
                onenetSleepVote(SYSTEM_BUSY);
                ret = onenet_client_close(reqHandle, ref);
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_PARAMETER_ERROR), NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId onenetADDOBJ(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetADDOBJ(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 ref,objectId,instanceCount,attributeCount,actionCount;
    UINT8* instanceBitmap = NULL;;
    INT32 cisError = CIS_ERRID_PARAMETER_ERROR;
    UINT16 length = 0;
    
    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+MIPLADDOBJ=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+MIPLADDOBJ: (0),(0-0xFFFF),(0-0xFFFF),\"<instanceBitmap>\",(0-0xFFFF),(0-0xFFFF)");
            break;
        }

        case AT_SET_REQ:              /* AT+MIPLADDOBJ= */
        {
            do{
                if(atGetNumValue(pAtCmdReq->pParamList, 0, &ref, 0, 0, 0) == AT_PARA_ERR)
                {
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 1, &objectId, 0, 65535, 0) == AT_PARA_ERR)
                {
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 2, &instanceCount, 0, 100, 1) == AT_PARA_ERR)
                {
                    break;
                }

                instanceBitmap = (UINT8*)malloc(instanceCount+1);
                memset(instanceBitmap, 0, instanceCount+1);
                if(atGetStrValue(pAtCmdReq->pParamList, 3, instanceBitmap, instanceCount+1, &length, (CHAR*)NULL) == AT_PARA_ERR)
                {
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 4, &attributeCount, 0, 65535, 1) == AT_PARA_ERR)
                {
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 5, &actionCount, 0, 65535, 1) == AT_PARA_ERR)
                {
                    break;
                }

                cisError = onenetAddObj(ref, objectId, instanceCount, instanceBitmap, attributeCount, actionCount);
            }while(0);

            if(cisError == CIS_ERRID_OK)
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            else
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( cisError), NULL);
            if(instanceBitmap)
                free(instanceBitmap);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }
    
    return ret;
}

/**
  \fn          CmsRetId onenetDELOBJ(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetDELOBJ(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 ref,objectId;
    INT32 cisError= CIS_ERRID_PARAMETER_ERROR;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+MIPLDELOBJ=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+MIPLDELOBJ: (0),(0-0xFFFF)");
            break;
        }

        case AT_SET_REQ:              /* AT+MIPLDELOBJ= */
        {
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &ref, 0, 0, 0) != AT_PARA_ERR)
            {
                if(atGetNumValue(pAtCmdReq->pParamList, 1, &objectId, 0, 65535, 0) != AT_PARA_ERR)
                {
                    cisError = onenetDelObj(ref, objectId);
                }
            }

            if(cisError == CIS_ERRID_OK){
                onenetSaveFlash();//save the del object info to flash
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( cisError), NULL);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId onenetNOTIFY(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetNOTIFY(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 ref,msgId,objectId,instanceId,resourceId,valueType,valueLen,index,flag,ackId,raiflag=0;
    UINT8* value = NULL;
    INT32 cisError = CIS_ERRID_PARAMETER_ERROR;
    UINT16 length=0;
    UINT8 raiString[6];
    UINT16 raiStringLen = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+MIPLNOTIFY=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+MIPLNOTIFY: (0),(0-0xFFFFFFF),(0-0xFFFF),(0-0xFFFF),(0-0xFFFF),(1-5),(1-0xFF),\"<value>\",(0-10),(0-2),(0-0xFFFF),(0-2)");
            break;
        }

        case AT_SET_REQ:              /* AT+MIPLNOTIFY= */
        {
            do{
                if(cisnet_attached_state(NULL) == false)
                {
                    ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NO_NETWORK), NULL);
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 0, &ref, 0, 0, 0) == AT_PARA_ERR)
                {
                    break;
                }
                if(onenetSearchCisInstance(ref) == NULL)
                {
                    ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NO_INSTANCE), NULL);
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 1, &msgId, 0, 0xFFFFFFF,0) == AT_PARA_ERR)
                {
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 2, &objectId, 0, 65535, 0) == AT_PARA_ERR)
                {
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 3, &instanceId, 0, 65535, 0) == AT_PARA_ERR)
                {
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 4, &resourceId, 0, 65535, 0) == AT_PARA_ERR)
                {
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 5, &valueType, 1, 5, 1) == AT_PARA_ERR)
                {
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 6, &valueLen, 1, 0x400, 1) == AT_PARA_ERR)
                {
                    break;
                }
                if((valueType == 4) && ((valueLen%4 != 0) || (valueLen > 8) ))
                {
                    break;
                }
                if((valueType == 3) && ((valueLen%2 != 0) || (valueLen > 8) ))
                {
                    break;
                }
                if(valueType == 4)
                {
                    valueLen = 0xFF;
                }

                if(valueType == 2)  // opaque type
                {
                    valueLen = valueLen*2;
                }
                
                value = (UINT8*)malloc(valueLen+1);
                memset(value, 0, valueLen+1);
                if(atGetStrValue(pAtCmdReq->pParamList, 7, value, valueLen+1, &length, (CHAR*)NULL) == AT_PARA_ERR)
                {
                    break;
                }

                if((valueType == 2) && ((valueLen != length) || ((length % 2) != 0)))
                {
                    ECOMM_TRACE(UNILOG_ONENET, onenetNOTIFY_1, P_ERROR, 2, "Opaque Check: valueLen=%d, Actual length=%d", valueLen, length);
                    break;
                }
                
                valueLen = length;
                
                if(atGetNumValue(pAtCmdReq->pParamList, 8, &index, 0, 10, 0) == AT_PARA_ERR)
                {
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 9, &flag, 0, 2, 0) == AT_PARA_ERR)
                {
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 10, &ackId, 0, 65535, 0) == AT_PARA_ERR)
                {
                    break;
                }
                memset(raiString,0,sizeof(raiString));
                if(atGetStrValue(pAtCmdReq->pParamList, 11, raiString, 6, &raiStringLen, (CHAR*)NULL) == AT_PARA_ERR)
                {
                    break;
                }

                if(onenetRaiStringToRaiFlag(&raiflag, raiString) == false)
                    break;

                #ifdef FEATURE_REF_AT_ENABLE
                if((ackId != 0) && (raiflag == 1))
                    break;
                #endif

                onenetResuourcCmd* pCmd;
                pCmd = malloc(sizeof(onenetResuourcCmd));
                memset(pCmd,0,sizeof(onenetResuourcCmd));
                pCmd->msgid = msgId;
                pCmd->objectid = objectId;
                pCmd->instanceid = instanceId;
                pCmd->resourceid = resourceId;
                pCmd->valuetype = valueType;
                pCmd->len = valueLen;
                pCmd->value = value;
                pCmd->ackid = ackId;
                pCmd->raiflag = raiflag;
                if(index > 0)
                {
                    pCmd->result = CIS_NOTIFY_CONTINUE;
                }
                else if(index == 0)
                {
                    pCmd->result = CIS_NOTIFY_CONTENT;
                }
                onenetSleepVote(SYSTEM_BUSY);
                ret = onenet_client_notify(reqHandle, ref, pCmd);
                cisError = CIS_ERRID_OK;
            }while(0);

            if(cisError != CIS_ERRID_OK){
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( cisError), NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId onenetREADRSP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetREADRSP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 ref,msgId,result,objectId,instanceId,resourceId,valueType,valueLen,index,flag,raiflag;
    UINT8* value = NULL;
    cis_coapret_t coapret;
    BOOL bContinue = FALSE;
    INT32 cisError = CIS_ERRID_PARAMETER_ERROR;
    UINT16 length = 0;
    UINT8 raiString[6];
    UINT16 raiStringLen = 0;
    
    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+MIPLREADRSP=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+MIPLREADRSP: (0),(0-0xFFFFFFF),(1-15),(0-0xFFFF),(0-0xFFFF),(0-0xFFFF),(1-5),(1-0xFF),\"<value>\",(0-10),(0-2),(0-2)");
            break;
        }

        case AT_SET_REQ:              /* AT+MIPLREADRSP= */
        {
            if(cisnet_attached_state(NULL) == false)
            {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NO_NETWORK), NULL);
                break;
            }

            if(atGetNumValue(pAtCmdReq->pParamList, 0, &ref, 0, 0, 0) != AT_PARA_ERR)
            {
                if(atGetNumValue(pAtCmdReq->pParamList, 1, &msgId, 0, 0xFFFFFFF, 0) != AT_PARA_ERR)
                {
                    if(atGetNumValue(pAtCmdReq->pParamList, 2, &result, 1, 15, 1) != AT_PARA_ERR)
                    {
                        bContinue = TRUE;
                    }
                }
            }
            do{
                if(!bContinue)
                {
                    break;
                }
                else
                {
                    if(result == ONENET_OP_RESULT_205_CONTENT)
                    {
                        if(pAtCmdReq->pParamList[3].bDefault || pAtCmdReq->pParamList[4].bDefault || pAtCmdReq->pParamList[5].bDefault || pAtCmdReq->pParamList[6].bDefault || pAtCmdReq->pParamList[7].bDefault || pAtCmdReq->pParamList[8].bDefault)
                        {
                            break;
                        }
                        if(atGetNumValue(pAtCmdReq->pParamList, 3, &objectId, 0, 65535, 0) == AT_PARA_ERR)
                        {
                            break;
                        }
                        if(atGetNumValue(pAtCmdReq->pParamList, 4, &instanceId, 0, 65535, 0)== AT_PARA_ERR)
                        {
                            break;
                        }
                        if(atGetNumValue(pAtCmdReq->pParamList, 5, &resourceId, 0, 65535, 0) == AT_PARA_ERR)
                        {
                            break;
                        }
                        if(atGetNumValue(pAtCmdReq->pParamList, 6, &valueType, 1, 5, 1) == AT_PARA_ERR)
                        {
                            break;
                        }
                        if(atGetNumValue(pAtCmdReq->pParamList, 7, &valueLen, 1, 0x400, 1) == AT_PARA_ERR)
                        {
                            break;
                        }
                        if((valueType == 4) && ((valueLen%4 != 0) || (valueLen > 8) ))
                        {
                            break;
                        }
                        if(valueType == 4)
                        {
                            valueLen = 0xFF;
                        }

                        if(valueType == 2)  // opaque type
                        {
                            valueLen = valueLen*2;
                        }

                        value = (UINT8*)malloc(valueLen+1);
                        memset(value, 0, valueLen);
                        if(atGetStrValue(pAtCmdReq->pParamList, 8, value, valueLen+1, &length, (CHAR*)NULL) == AT_PARA_ERR)
                        {
                            break;
                        }

                        if((valueType == 2) && ((valueLen != length) || ((length % 2) != 0)))
                        {
                            ECOMM_TRACE(UNILOG_ONENET, onenetREADRSP_2, P_ERROR, 2, "Opaque Check: valueLen=%d, Actual length=%d", valueLen, length);
                            break;
                        }

                        valueLen = length;
                        if(atGetNumValue(pAtCmdReq->pParamList, 9, &index, 0, 10, 0) == AT_PARA_ERR)
                        {
                            break;
                        }
                        if(atGetNumValue(pAtCmdReq->pParamList, 10, &flag, 0, 2, 0) == AT_PARA_ERR)
                        {
                            break;
                        }

                        memset(raiString,0,sizeof(raiString));
                        if(atGetStrValue(pAtCmdReq->pParamList, 11, raiString, 6, &raiStringLen, (CHAR*)NULL) == AT_PARA_ERR)
                        {
                            break;
                        }
                        
                        if(onenetRaiStringToRaiFlag(&raiflag, raiString) == false)
                            break;
                        
                        if(index > 0)
                        {
                            cisError = onenetReadResp(ref, msgId, objectId, instanceId, resourceId, valueType, valueLen, value, CIS_NOTIFY_CONTINUE, raiflag);
                            ECOMM_TRACE(UNILOG_ONENET, onenetREADRSP_0, P_INFO, 2, "index=%d, onenetReadResp return %d", index, cisError);
                        }
                        else if(index == 0)
                        {
                            cisError = onenetReadResp(ref, msgId, objectId, instanceId, resourceId, valueType, valueLen, value, CIS_RESPONSE_READ, raiflag);
                            ECOMM_TRACE(UNILOG_ONENET, onenetREADRSP_1, P_INFO, 2, "index=%d, onenetReadResp return %d", index, cisError);
                        }
                        else
                        {
                            cisError = CIS_ERRID_PARAMETER_ERROR;
                        }
                    }
                    else
                    {
                        if(atGetStrValue(pAtCmdReq->pParamList, 11, raiString, 6, &raiStringLen, (CHAR*)NULL) == AT_PARA_ERR)
                        {
                            break;
                        }
                        
                        if(onenetRaiStringToRaiFlag(&raiflag, raiString) == false)
                            break;
                        if(onenetResult2Coapret(ONENET_CMD_READRESP, (onenet_at_result_t)result, &coapret) == true)
                        {
                            cisError = onenetResponse(ref, msgId, NULL, NULL, coapret, raiflag);
                        }
                        else
                        {
                            cisError = CIS_ERRID_PARAMETER_ERROR;
                        }
                    }
                }
            }while(0);

            if(cisError == CIS_ERRID_OK)
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            else
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( cisError), NULL);
            if(value)
                free(value);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId onenetWRITERSP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetWRITERSP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 ref,msgId,result,raiflag=0;
    INT32 cisError = CIS_ERRID_PARAMETER_ERROR;
    cis_coapret_t coapret;
    UINT8 raiString[6];
    UINT16 raiStringLen = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+MIPLWRITERSP=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+MIPLWRITERSP: (0),(0-0xFFFFFFF),(2-14),(0-2)");
            break;
        }

        case AT_SET_REQ:              /* AT+MIPLWRITERSP= */
        {
            if(cisnet_attached_state(NULL) == false)
            {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NO_NETWORK), NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &ref, 0, 0, 0) != AT_PARA_ERR)
            {
                if(atGetNumValue(pAtCmdReq->pParamList, 1, &msgId, 0, 0xFFFFFFF, 0) != AT_PARA_ERR)
                {
                    if(atGetNumValue(pAtCmdReq->pParamList, 2, &result, 2, 14, 2) != AT_PARA_ERR)
                    {
                        memset(raiString,0,sizeof(raiString));
                        if(atGetStrValue(pAtCmdReq->pParamList, 3, raiString, 6, &raiStringLen, (CHAR*)NULL) != AT_PARA_ERR)
                        {
                            if(onenetRaiStringToRaiFlag(&raiflag, raiString) == true)
                            {
                                if(result == ONENET_OP_RESULT_204_CHANGED) {
                                    cisError = onenetResponse(ref, msgId, NULL, NULL, CIS_RESPONSE_WRITE, raiflag);
                                } else {
                                    if(onenetResult2Coapret(ONENET_CMD_WRITERESP, (onenet_at_result_t)result, &coapret) == true)
                                    {
                                        cisError = onenetResponse(ref, msgId, NULL, NULL, coapret, raiflag);
                                    }
                                    else
                                    {
                                        cisError = CIS_ERRID_PARAMETER_ERROR;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if(cisError == CIS_ERRID_OK)
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            else
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( cisError), NULL);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId onenetEXECUTERSP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetEXECUTERSP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 ref,msgId,result,raiflag=0;
    INT32 cisError = CIS_ERRID_PARAMETER_ERROR;
    cis_coapret_t coapret;
    UINT8 raiString[6];
    UINT16 raiStringLen = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+MIPLEXECUTERSP=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+MIPLEXECUTERSP: (0),(0-0xFFFFFFF),(2-14),(0-2)");
            break;
        }

        case AT_SET_REQ:              /* AT+MIPLEXECUTERSP= */
        {
            if(cisnet_attached_state(NULL) == false)
            {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NO_NETWORK), NULL);
                break;
            }
           if(atGetNumValue(pAtCmdReq->pParamList, 0, &ref, 0, 0, 0) != AT_PARA_ERR)
            {
                if(atGetNumValue(pAtCmdReq->pParamList, 1, &msgId, 0, 0xFFFFFFF, 0) != AT_PARA_ERR)
                {
                    if(atGetNumValue(pAtCmdReq->pParamList, 2, &result, 2, 14, 4) != AT_PARA_ERR)
                    {
                        memset(raiString,0,sizeof(raiString));
                        if(atGetStrValue(pAtCmdReq->pParamList, 3, raiString, 6, &raiStringLen, (CHAR*)NULL) != AT_PARA_ERR)
                        {
                            if(onenetRaiStringToRaiFlag(&raiflag, raiString) == true)
                            {
                                if(result == ONENET_OP_RESULT_204_CHANGED) {
                                    cisError = onenetResponse(ref, msgId, NULL, NULL, CIS_RESPONSE_EXECUTE, raiflag);
                                }
                                else
                                {
                                    if(onenetResult2Coapret(ONENET_CMD_EXECUTERESP, (onenet_at_result_t)result, &coapret) == true)
                                    {
                                        cisError = onenetResponse(ref, msgId, NULL, NULL, coapret, raiflag);
                                    }
                                    else
                                    {
                                        cisError = CIS_ERRID_PARAMETER_ERROR;
                                    }
                                }
                            }   
                        }
                    }
                }
            }

            if(cisError == CIS_ERRID_OK)
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            else
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( cisError), NULL);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId onenetOBSERVERSP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetOBSERVERSP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 ref,msgId,result,raiflag=0;
    INT32 cisError = CIS_ERRID_PARAMETER_ERROR;
    cis_coapret_t coapret;
    UINT8 raiString[6];
    UINT16 raiStringLen = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+MIPLOBSERVERSP=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+MIPLOBSERVERSP: (0),(0-0xFFFFFFF),(1-15),(0-2)");
            break;
        }

        case AT_SET_REQ:              /* AT+MIPLOBSERVERSP= */
        {
            if(cisnet_attached_state(NULL) == false)
            {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NO_NETWORK), NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &ref, 0, 0, 0) != AT_PARA_ERR)
            {
                if(atGetNumValue(pAtCmdReq->pParamList, 1, &msgId, 0, 0xFFFFFFF, 0) != AT_PARA_ERR)
                {
                    if(atGetNumValue(pAtCmdReq->pParamList, 2, &result, 1, 15, 1) != AT_PARA_ERR)
                    {
                        memset(raiString,0,sizeof(raiString));
                        if(atGetStrValue(pAtCmdReq->pParamList, 3, raiString, 6, &raiStringLen, (CHAR*)NULL) != AT_PARA_ERR)
                        {
                            if(onenetRaiStringToRaiFlag(&raiflag, raiString) == true)
                            {
                                if(result == ONENET_OP_RESULT_205_CONTENT) {
                                    cisError = onenetResponse(ref, msgId, NULL, NULL, CIS_RESPONSE_OBSERVE, raiflag);
                                } else {
                                    if(onenetResult2Coapret(ONENET_CMD_OBSERVERESP, (onenet_at_result_t)result, &coapret) == true)
                                    {
                                        cisError = onenetResponse(ref, msgId, NULL, NULL, coapret, raiflag);
                                    }
                                    else
                                    {
                                        cisError = CIS_ERRID_PARAMETER_ERROR;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if(cisError == CIS_ERRID_OK)
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            else
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( cisError), NULL);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId onenetDISCOVERRSP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetDISCOVERRSP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 ref,msgId,result,length,raiflag=0;
    UINT8* valueString = NULL;
    INT32 cisError = CIS_ERRID_PARAMETER_ERROR;
    cis_coapret_t coapret;
    char* buffer = NULL;
    UINT16 atlength = 0;
    UINT8 raiString[6];
    UINT16 raiStringLen = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+MIPLDISCOVERRSP=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+MIPLDISCOVERRSP: (0),(0-0xFFFFFFF),(1-15),(1-0xFF),\"<valuestring>\",(0-2)");
            break;
        }

        case AT_SET_REQ:              /* AT+MIPLDISCOVERRSP= */
        {
            do{
                if(cisnet_attached_state(NULL) == false)
                {
                    ret = CIS_ERRID_NO_NETWORK;
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 0, &ref, 0, 0, 0) == AT_PARA_ERR)
                {
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 1, &msgId, 0, 0xFFFFFFF, 0) == AT_PARA_ERR)
                {
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 2, &result, 1, 15, 1) == AT_PARA_ERR)
                {
                    break;
                }
                if(atGetNumValue(pAtCmdReq->pParamList, 3, &length, 1, 255, 1) == AT_PARA_ERR)
                {
                    break;
                }
                valueString = (UINT8*)malloc(length+1);
                memset(valueString, 0, length+1);
                if(atGetStrValue(pAtCmdReq->pParamList, 4, valueString, length+1, &atlength, (CHAR*)NULL) == AT_PARA_ERR)
                {
                    break;
                }

                memset(raiString,0,sizeof(raiString));
                if(atGetStrValue(pAtCmdReq->pParamList, 5, raiString, 6, &raiStringLen, (CHAR*)NULL) == AT_PARA_ERR)
                {
                    break;
                }
                
                if(onenetRaiStringToRaiFlag(&raiflag, raiString) == false)
                    break;
                
                if(result == ONENET_OP_RESULT_205_CONTENT)
                {
                    UINT32 i;
                    cis_uri_t uri;
                    char *attrList[ONENET_ATTR_NUM + 1];
                    buffer = malloc(length+10);
                    UINT32 attrNum = onenetParseAttr((const char*)valueString, buffer, NULL, attrList, ONENET_ATTR_NUM + 1, ";");
                    if(attrNum > ONENET_ATTR_NUM)
                    {
                        ECOMM_TRACE(UNILOG_ONENET, onenetDISCOVERRSP_1, P_INFO, 1, "parameter error");
                        cisError = CIS_ERRID_PARAMETER_ERROR;
                    }
                    else
                    {
                        for(i = 0; i < attrNum; i++)
                        {
                            uri.objectId = URI_INVALID;
                            uri.instanceId = URI_INVALID;
                            uri.resourceId = (UINT16)atoi(attrList[i]);
                            cis_uri_update(&uri);
                            cisError = onenetResponse(ref, msgId, &uri, NULL, CIS_RESPONSE_CONTINUE, raiflag);
                        }
                        cisError = onenetResponse(ref, msgId, NULL, NULL, CIS_RESPONSE_DISCOVER, raiflag);
                    }
                }
                else
                {
                    if(onenetResult2Coapret(ONENET_CMD_DISCOVERRESP, (onenet_at_result_t)result, &coapret) == true)
                    {
                        cisError = onenetResponse(ref, msgId, NULL, NULL, coapret, raiflag);
                    }
                    else
                    {
                        cisError = CIS_ERRID_PARAMETER_ERROR;
                    }
                }
            }while(0);


            if(cisError == CIS_ERRID_OK)
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            else
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( cisError), NULL);
            if(valueString)
                free(valueString);
            if(buffer)
                free(buffer);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId onenetPARAMETERRSP(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetPARAMETERRSP(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 ref,msgId,result,raiflag=0;
    INT32 cisError = CIS_ERRID_PARAMETER_ERROR;
    cis_coapret_t coapret;
    UINT8 raiString[6];
    UINT16 raiStringLen = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+MIPLPARAMETERRSP=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+MIPLPARAMETERRSP: (0),(0-0xFFFFFFF),(2-14),(0-2)");
            break;
        }

        case AT_SET_REQ:              /* AT+MIPLPARAMETERRSP= */
        {
            if(cisnet_attached_state(NULL) == false)
            {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NO_NETWORK), NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &ref, 0, 0, 0) != AT_PARA_ERR)
            {
                if(atGetNumValue(pAtCmdReq->pParamList, 1, &msgId, 0, 0xFFFFFFF, 0) != AT_PARA_ERR)
                {
                    if(atGetNumValue(pAtCmdReq->pParamList, 2, &result, 2, 14, 2) != AT_PARA_ERR)
                    {
                        memset(raiString,0,sizeof(raiString));
                        if(atGetStrValue(pAtCmdReq->pParamList, 3, raiString, 6, &raiStringLen, (CHAR*)NULL) != AT_PARA_ERR)
                        {
                            if(onenetRaiStringToRaiFlag(&raiflag, raiString) == true)
                            {
                                if(result == ONENET_OP_RESULT_204_CHANGED)
                                {
                                    cisError = onenetResponse(ref, msgId, NULL, NULL, CIS_RESPONSE_OBSERVE_PARAMS, raiflag);
                                }
                                else
                                {
                                    if(onenetResult2Coapret(ONENET_CMD_PARAMRESP, (onenet_at_result_t)result, &coapret) == true)
                                    {
                                        cisError = onenetResponse(ref, msgId, NULL, NULL, coapret, raiflag);
                                    }
                                    else
                                    {
                                        cisError = CIS_ERRID_PARAMETER_ERROR;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if(cisError == CIS_ERRID_OK)
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            else
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( cisError), NULL);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId onenetUPDATE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetUPDATE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 ref,lifetime,oflag, raiflag=0;
    INT32 cisError = CIS_ERRID_PARAMETER_ERROR;
    UINT8 raiString[6];
    UINT16 raiStringLen = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+MIPLUPDATE=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+MIPLUPDATE: (0),(0x0-0x0FFFFFFF),(0-1),(0-2)");
            break;
        }

        case AT_SET_REQ:              /* AT+MIPLUPDATE= */
        {
            if(cisnet_attached_state(NULL) == false)
            {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NO_NETWORK), NULL);
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &ref, 0, 0, 0) != AT_PARA_ERR)
            {
                if(atGetNumValue(pAtCmdReq->pParamList, 1, &lifetime, 0, LIFETIME_LIMIT_MAX, 0) != AT_PARA_ERR)
                {
                    if(atGetNumValue(pAtCmdReq->pParamList, 2, &oflag, 0, 1, 0) != AT_PARA_ERR)
                    {
                        memset(raiString,0,sizeof(raiString));
                        if(atGetStrValue(pAtCmdReq->pParamList, 3, raiString, 6, &raiStringLen, (CHAR*)NULL) != AT_PARA_ERR)
                        {
                            if(onenetRaiStringToRaiFlag(&raiflag, raiString) == true)
                            {
                                if(onenetSearchCisInstance(ref) == NULL)
                                {
                                    ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NO_INSTANCE), NULL);
                                    break;
                                }
                                if(lifetime == 0)
                                {
                                    lifetime = 86400;
                                }
                                onenetSleepVote(SYSTEM_BUSY);
                                ret = onenet_client_update(reqHandle, ref, lifetime, oflag, raiflag);
                                cisError = CIS_ERRID_OK;

                                onenetContext *onenet = onenetSearchInstance(ref);
                                cissys_assert(onenet != NULL);
                                if(onenet->lifeTime != lifetime)
                                {
                                    onenet->lifeTime = lifetime;
                                    onenetSaveFlash();
                                }
                            }
                        }
                    }
                }
            }

            if(cisError != CIS_ERRID_OK){
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( cisError), NULL);
            }

            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId onenetVERSION(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetVERSION(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    cis_version_t version;
    char RspBuf[ONENET_AT_RESPONSE_DATA_LEN] = {0};

    switch (operaType)
    {
        case AT_READ_REQ:              /* AT+MIPLVER? */
        {
            cis_version(&version);
            sprintf(RspBuf, "+MIPLVER: %d.%d.%d", (INT32)version.major, (INT32)version.minor, (INT32)version.micro);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)RspBuf);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId onenetOTASTART(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetOTASTART(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 timeoutValue = 0;
    CHAR RspBuf[32];

    switch (operaType)
    {

        case AT_SET_REQ:         /* AT+OTASTART=t */
        {
            if(cisnet_attached_state(NULL) == false)
            {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NO_NETWORK), NULL);
                ECOMM_TRACE(UNILOG_ONENET, onenetOTASTART_3, P_INFO, 0, "SIM OTA START enable slp2 because network not ok");
                break;
            }
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &timeoutValue, 0, 65535, 0) != AT_PARA_ERR)
            {
                //check if any onenet connection created, if so, notify user to stop
                if(onenetSearchInstance(0) != NULL)
                {
                    ECOMM_TRACE(UNILOG_ONENET, onenetOTASTART_1, P_INFO, 0, "SIM OTA failed as already active onenet connection");
                    ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_CONFLICT_FAIL), NULL);
                    break;
                }

                //get the SIM OTA finished history state from flash, check if already done.
                memset(&gOnenetSimOtaCtx, 0, sizeof(OnenetSIMOTAContext));
                onenetRestoreContext();
                if (gOnenetSimOtaCtx.otaFinishState == OTA_HISTORY_STATE_FINISHED)
                {
                    ECOMM_TRACE(UNILOG_ONENET, onenetOTASTART_2, P_INFO, 0, "SIM OTA has history finish state, no new OTA started.");
                    sprintf(RspBuf, "OK\r\n\r\n+OTAFINISH: %d",(INT32)OTA_FINISH_AT_RETURN_REPEATED);
                    ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)RspBuf);
                    break;
                }

                gOnenetSimOtaCtx.timeoutValue = timeoutValue;
                gOnenetSimOtaCtx.reqhandle = reqHandle;
                ECOMM_TRACE(UNILOG_ONENET, onenetOTASTART_5, P_WARNING, 0, "create onenet SIM OTA task");
                xTaskCreate(onenetSimOtaTaskEntry,
                             "onenet_simota_task",
                             1024*3 / sizeof(portSTACK_TYPE),
                             NULL,
                             osPriorityBelowNormal7,
                             NULL);
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_PARAMETER_ERROR), NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId onenetOTASTATE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  onenetOTASTATE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    char RspBuf[ONENET_AT_RESPONSE_DATA_LEN] = {0};
    INT32 otaFlag = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:             /* AT+OTASTATE=? */
        case AT_READ_REQ:              /* AT+OTASTATE? */
        {
            onenetRestoreContext();
            sprintf(RspBuf, "+OTASTATE: %d", (INT32)gOnenetSimOtaCtx.otaFinishState);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)RspBuf);
            break;
        }

        case AT_SET_REQ://AT+OTASTATE=<ota_flag>
        {
            if(atGetNumValue(pAtCmdReq->pParamList, 0, &otaFlag, 0, 1, 0) != AT_PARA_ERR)
            {
                gOnenetSimOtaCtx.otaFinishState = otaFlag;
                onenetSaveFlash();
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, PNULL);
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_PARAMETER_ERROR), NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, ( CIS_ERRID_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}



/**
  \fn          CmsRetId onenetMIPLRD(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId onenetMIPLRD(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    INT32   operaType = (UINT32)pAtCmdReq->operaType;
    char RspBuf[ONENET_AT_RESPONSE_DATA_LEN] = {0};

    switch (operaType)
    {
        case AT_READ_REQ:              /* AT+MIPLRD? */
        {
            uint8_t mBuffered;
            uint16_t mBufferedSize;
            uint32_t mRecvCnt, mDropCnt;

            onenetDLBufferInfoCheck(&mBuffered, &mBufferedSize, &mRecvCnt, &mDropCnt);
            snprintf(RspBuf,ONENET_AT_RESPONSE_DATA_LEN, "+MIPLRD: %d,%d,%d,%d", mBuffered, mBufferedSize, mRecvCnt, mDropCnt);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)RspBuf);

            break;
        }
        case AT_EXEC_REQ:               /* AT+MIPLRD */
        {
            onenetDLPopout();
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }
        default:
        {
            ret = atcReply(reqHandle, AT_RC_CIS_ERROR, CIS_ERRID_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;

}


