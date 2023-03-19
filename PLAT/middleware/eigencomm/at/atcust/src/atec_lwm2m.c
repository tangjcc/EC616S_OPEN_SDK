/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_http.c
*
*  Description: Process http(s) client related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/


#include "atec_lwm2m.h"
#include "at_lwm2m_task.h"
#include "cms_util.h"
#include "netmgr.h"
#include "ps_lib_api.h"
#include "liblwm2m.h"

extern uint8_t lwm2mSlpHandler;
extern UINT8 lwm2mTaskStart;
lwm2mCmdContext lwm2mCmdCxt = {LWM2M_CMD_NONE,0,0,0,0,0};

#define _DEFINE_AT_REQ_FUNCTION_LIST_


/**
  \fn          CmsRetId lwm2mCREATE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  lwm2mCREATE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 operaType = (UINT32)pAtCmdReq->operaType;
    UINT8 server[MAX_SERVER_LEN+1]={0};
    UINT16 serverLen = 0;
    INT32 port = 0;
    INT32 localPort = 0;
    UINT8 enderpoint[MAX_NAME_LEN+1] = {0};
    UINT16 enderpointLen = 0;
    INT32 lifetime = 0;
    UINT8 pskId[MAX_NAME_LEN+1] = {0};
    UINT16 pskIdLen = 0;
    UINT8 psk[MAX_NAME_LEN+1] = {0};
    UINT16 pskLen = 0;
    lwm2mClientContext* lwm2m = NULL;
    slpManSlpState_t slpState = slpManGetLastSlpState();
    NmAtiGetNetInfoReq  getNetInfoReq;
    
    memset(&getNetInfoReq, 0, sizeof(NmAtiGetNetInfoReq));

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+LWM2MCREATE=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+LWM2MCREATE: \"<server>\",\"<port>\",\"<localPort>\",\"<endpoint>\",(0xF-0xFFFFFFF),\"<pskId>\",\"<psk>\"");
            break;
        }

        case AT_SET_REQ:      /* AT+LWM2MCREATE= */
        {
            NmAtiSyncRet netStatus;
            NetmgrAtiSyncReq(NM_ATI_SYNC_GET_NET_INFO_REQ, (void *)&getNetInfoReq, &netStatus);
            if(netStatus.body.netInfoRet.netifInfo.netStatus != NM_NETIF_ACTIVATED)
            {
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mCREATE_0, P_INFO, 0, "net status not activated");
                rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_NO_NETWORK), NULL);
                break;
            }

            if((ret = atGetStrValue(pAtCmdReq->pParamList, 0, server, MAX_SERVER_LEN, &serverLen , (CHAR *)LWM2MCREATE_0_SERVER_STR_DEF)) != AT_PARA_ERR)
            {
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &port, LWM2MCREATE_1_PORT_MIN, LWM2MCREATE_1_PORT_MAX, LWM2MCREATE_1_PORT_DEF)) != AT_PARA_ERR)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &localPort, LWM2MCREATE_2_LOCALPORT_MIN, LWM2MCREATE_2_LOCALPORT_MAX, LWM2MCREATE_2_LOCALPORT_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, enderpoint, MAX_NAME_LEN, &enderpointLen , (CHAR *)LWM2MCREATE_3_ENDPOINT_STR_DEF)) != AT_PARA_ERR)
                        {
                            if((ret = atGetNumValue(pAtCmdReq->pParamList, 4, &lifetime, LWM2MCREATE_4_LIFETIME_MIN, LWM2MCREATE_4_LIFETIME_MAX, LWM2MCREATE_4_LIFETIME_DEF)) != AT_PARA_ERR)
                            {
                                if((ret = atGetStrValue(pAtCmdReq->pParamList, 5, pskId, MAX_NAME_LEN, &pskIdLen , (CHAR *)LWM2MCREATE_5_PSKID_STR_DEF)) != AT_PARA_ERR)
                                {
                                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 6, psk, MAX_NAME_LEN, &pskLen , (CHAR *)LWM2MCREATE_6_PSK_STR_DEF)) != AT_PARA_ERR)
                                    {
                                        if(((pskIdLen==0) && (pskLen>0)) || ((pskIdLen>0) && (pskLen==0)))
                                        {
                                            ret = AT_PARA_ERR;
                                        }
                                        else
                                        {
                                            ret = AT_PARA_OK;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if(ret == AT_PARA_OK)
            {
                lwm2m = lwm2mGetFreeClient();
                if(lwm2m != NULL)
                {
                    lwm2m->server = malloc(serverLen+1);
                    memset(lwm2m->server, 0, serverLen+1);
                    memcpy(lwm2m->server, server, serverLen);

                    lwm2m->serverPort = port;
                    lwm2m->localPort = localPort;

                    lwm2m->enderpoint = malloc(enderpointLen+1);
                    memset(lwm2m->enderpoint, 0, enderpointLen+1);
                    memcpy(lwm2m->enderpoint, enderpoint, enderpointLen);

                    if(lifetime > 0)
                    {
                        lwm2m->lifetime = lifetime;
                    }
                    else
                    {
                        lwm2m->lifetime = DEFAULT_LIFE_TIME;
                    }
                    lwm2m->clientData = malloc(sizeof(lwm2mClientData));
                    if((pskIdLen>0) && (pskLen>0))
                    {
                        lwm2m->pskId = malloc(pskIdLen+1);
                        memset(lwm2m->pskId, 0, pskIdLen+1);
                        memcpy(lwm2m->pskId, pskId, pskIdLen);

                        lwm2m->psk = malloc(pskLen/2+1);
                        lwm2m->pskLen = pskLen/2;
                        memset(lwm2m->psk, 0, pskLen/2+1);
                        //getStrFromHex((UINT8*)lwm2m->psk, (CHAR*)psk, pskLen/2);
                        cmsHexStrToHex((UINT8*)lwm2m->psk, pskLen/2, (CHAR *)psk, pskLen);
                    }
                    else
                    {
                        lwm2m->pskId = 0;
                        lwm2m->psk = 0;
                    }
                    lwm2m->addressFamily = AF_INET;
                    lwm2m->readWriteSemph = xSemaphoreCreateBinary();
                    if(lwm2m->readWriteSemph == NULL)
                    {
                        lwm2mRemoveClient(lwm2m->lwm2mclientId);
                        rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_SEMPH_ERROR), NULL);
                        break;
                    }
                    lwm2m->reqHandle = reqHandle;
                    INT8 result = lwm2mCreate(lwm2m);
                    if(result < 0)
                    {
                        lwm2mRemoveClient(lwm2m->lwm2mclientId);
                        rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_CONFIG_ERROR), NULL);
                        break;
                    }
                    ECOMM_TRACE(UNILOG_LWM2M, lwm2mCREATE_1, P_SIG, 0, "CMD_CREATE disable slp2");
                    ECOMM_TRACE(UNILOG_LWM2M, lwm2mCREATE_2, P_INFO, 0, "create lwm2m mainloop");
                    slpManPlatVoteDisableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
                    ret = lwm2mCreateMainLoop();
                    if(ret != LWM2M_ERRID_OK){
                        rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_INTER_ERROR), NULL);
                    }else{
                        result = lwm2mAthandleCreate();
                        if(result != LWM2M_ERRID_OK){
                            rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_INTER_ERROR), NULL);
                        }else{
                            rc = CMS_RET_SUCC;
                        }
                    }
                }
                else
                {
                    rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_NO_FREE_CLIENT), NULL);
                    ECOMM_TRACE(UNILOG_LWM2M, lwm2mCREATE_3, P_SIG, 0, "no free client CMD_CREATE enable slp2");
                    slpManPlatVoteEnableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
                }
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_PARAM_ERROR), NULL);
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mCREATE_4, P_SIG, 0, "parameter error CMD_CREATE enable slp2");
                slpManPlatVoteEnableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
            }
            break;
        }

        case AT_READ_REQ:
        case AT_EXEC_REQ:
        default:
        {
            rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId lwm2mDELETE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  lwm2mDELETE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 lwm2mclientId;
    lwm2mClientContext* lwm2m = NULL;
    slpManSlpState_t slpState = slpManGetLastSlpState();

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+LWM2MDELETE=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+LWM2MDELETE: (0)");
            break;
        }

        case AT_SET_REQ:      /* AT+LWM2MDELETE                                                                                                                                                                             = */
        {
            if((rc = atGetNumValue(pAtCmdReq->pParamList, 0, &lwm2mclientId, LWM2MDELETE_0_CLIENTID_MIN, LWM2MDELETE_0_CLIENTID_MAX, LWM2MDELETE_0_CLIENTID_DEF)) != AT_PARA_ERR)
            {
                lwm2m = lwm2mFindClient(lwm2mclientId);
                if(lwm2m == NULL || lwm2m->context == NULL)
                {
                    ECOMM_TRACE(UNILOG_LWM2M, lwm2mDELETE_1, P_INFO, 0, "no find client");
                    rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_NO_FIND_CLIENT), NULL);
                    break;
                }
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mDELETE_2, P_SIG, 0, "CMD DELETE disable slp2");
                slpManPlatVoteDisableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
                lwm2m->reqHandle = reqHandle;
                
                INT32 result = lwm2mClientDelete(reqHandle, lwm2mclientId);
                if(result == CMS_RET_SUCC){
                    rc = CMS_RET_SUCC;
                }else{
                    rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_INTER_ERROR), NULL);
                }
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_PARAM_ERROR), NULL);
                break;
            }
            break;
        }

        case AT_READ_REQ:
        case AT_EXEC_REQ:
        default:
        {
            rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId lwm2mADDOBJ(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  lwm2mADDOBJ(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 lwm2mId,objectId,instanceId,resourceCount;
    UINT8 * resourceIds = NULL;
    UINT16 resourceIdsLen;
    lwm2mClientContext* lwm2m = NULL;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+LWM2MADDOBJ=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+LWM2MADDOBJ: (0),(0-0xFFFF),(0-0xFFFF),(0-0xFF),\"<resourceIds>\"");
            break;
        }

        case AT_SET_REQ:              /* AT+LWM2MADDOBJ= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &lwm2mId, LWM2MADDOBJ_0_ID_MIN, LWM2MADDOBJ_0_ID_MAX, LWM2MADDOBJ_0_ID_DEF)) != AT_PARA_ERR)
            {
                lwm2m = lwm2mFindClient(lwm2mId);
                if(lwm2m == NULL || lwm2m->context == NULL)
                {
                    ECOMM_TRACE(UNILOG_LWM2M, lwm2mADDOBJ_1, P_INFO, 0, "no find client");
                    rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_NO_FIND_CLIENT), NULL);
                    break;
                }
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &objectId, LWM2MADDOBJ_1_OBJID_MIN, LWM2MADDOBJ_1_OBJID_MAX, LWM2MADDOBJ_1_OBJID_DEF)) != AT_PARA_ERR)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &instanceId, LWM2MADDOBJ_2_INSTID_MIN, LWM2MADDOBJ_2_INSTID_MAX, LWM2MADDOBJ_2_INSTID_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 3, &resourceCount, LWM2MADDOBJ_3_RESOURCECOUNT_MIN, LWM2MADDOBJ_3_RESOURCECOUNT_MAX, LWM2MADDOBJ_3_RESOURCECOUNT_DEF)) != AT_PARA_ERR)
                        {
                            resourceIds = malloc(resourceCount*5+1);
                            memset(resourceIds, 0, resourceCount*5+1);

                            if((ret = atGetStrValue(pAtCmdReq->pParamList, 4, resourceIds, (resourceCount*6+1), &resourceIdsLen , (CHAR *)LWM2MADDOBJ_4_RESID_STR_DEF)) != AT_PARA_ERR)
                            {
                                ret = AT_PARA_OK;
                            }
                        }
                    }
                }
            }

            if(ret == AT_PARA_OK){
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mADDOBJ_2, P_SIG, 0, "CMD_ADDOBJ disable slp2");
                lwm2m->reqHandle = reqHandle;
                slpManPlatVoteDisableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
                lwm2mAddobjCmd* pCmd = malloc(sizeof(lwm2mAddobjCmd));
                memset(pCmd,0,sizeof(lwm2mAddobjCmd));
                pCmd->objectid = objectId;
                pCmd->instanceId = instanceId;
                pCmd->resourceIds = resourceIds;
                pCmd->resourceCount = resourceCount;
                rc = lwm2mClientAddobj(reqHandle, lwm2mId, pCmd);
            }
            break;
        }
        default:
        {
            rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId lwm2mDELOBJ(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  lwm2mDELOBJ(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 lwm2mclientId, objId;
    lwm2mClientContext* lwm2m = NULL;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+LWM2MDELOBJ=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+LWM2MDELOBJ: (0),(0-0xFFFF)");
            break;
        }

        case AT_SET_REQ:      /* AT+LWM2MDELOBJ                                                                                                                                                                             = */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &lwm2mclientId, LWM2MDELOBJ_0_ID_MIN, LWM2MDELOBJ_0_ID_MAX, LWM2MDELOBJ_0_ID_DEF)) != AT_PARA_ERR)
            {
                lwm2m = lwm2mFindClient(lwm2mclientId);
                if(lwm2m == NULL || lwm2m->context == NULL)
                {
                    rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_NO_FIND_CLIENT), NULL);
                    break;
                }
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &objId, LWM2MDELOBJ_1_OBJID_MIN, LWM2MDELOBJ_1_OBJID_MAX, LWM2MDELOBJ_1_OBJID_DEF)) != AT_PARA_ERR)
                {
                    if(objId == 0 || objId == 1 || objId == 3)
                    {
                        rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_OPERATION_NOT_SUPPORT), NULL);
                        break;
                    }
                    lwm2m->reqHandle = reqHandle;
                    slpManPlatVoteDisableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
                    rc = lwm2mClientDelobj(reqHandle, lwm2mclientId, objId);
                }
            }

            if(ret == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_PARAM_ERROR), NULL);
            }

            break;
        }

        case AT_READ_REQ:
        case AT_EXEC_REQ:
        default:
        {
            rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId lwm2mREADCONF(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  lwm2mREADCONF(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 lwm2mclientId, objId, instid, resId, valueType, valueLen;
    UINT8 * values = NULL;
    UINT16 valuesLen = 0;
    UINT8 * buffer = NULL;
    lwm2mClientContext* lwm2m = NULL;
    lwm2m_data_t* dataP = NULL;
    BOOL paramInvalid = FALSE;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+LWM2MREADCONF=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+LWM2MDELOBJ: (0),(0-0xFFFF),(0-0xFFFF),(0-0xFFFF),(0-4),(1-40),\"<value>\"");
            break;
        }

        case AT_SET_REQ:      /* AT+LWM2MREADCONF                                                                                                                                                                             = */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &lwm2mclientId, LWM2MREADCONF_0_ID_MIN, LWM2MREADCONF_0_ID_MAX, LWM2MREADCONF_0_ID_DEF)) != AT_PARA_ERR)
            {
                lwm2m = lwm2mFindClient(lwm2mclientId);
                if(lwm2m == NULL || lwm2m->context == NULL)
                {
                    rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_NO_FIND_CLIENT), NULL);
                    break;
                }
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &objId, LWM2MREADCONF_1_OBJID_MIN, LWM2MREADCONF_1_OBJID_MAX, LWM2MREADCONF_1_OBJID_DEF)) != AT_PARA_ERR)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &instid, LWM2MREADCONF_2_INSTID_MIN, LWM2MREADCONF_2_INSTID_MAX, LWM2MREADCONF_2_INSTID_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 3, &resId, LWM2MREADCONF_3_RESOURCEID_MIN, LWM2MREADCONF_3_RESOURCEID_MAX, LWM2MREADCONF_3_RESOURCEID_DEF)) != AT_PARA_ERR)
                        {
                            if((ret = atGetNumValue(pAtCmdReq->pParamList, 4, &valueType, LWM2MREADCONF_4_TYPE_MIN, LWM2MREADCONF_4_TYPE_MAX, LWM2MREADCONF_4_TYPE_DEF)) != AT_PARA_ERR)
                            {
                                if((ret = atGetNumValue(pAtCmdReq->pParamList, 5, &valueLen, LWM2MREADCONF_5_LEN_MIN, LWM2MREADCONF_5_LEN_MAX, LWM2MREADCONF_5_LEN_DEF)) != AT_PARA_ERR)
                                {
                                    if(valueType == VALUE_TYPE_OPAQUE && valueLen%2 == 1)
                                    {
                                        rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_PARAM_ERROR), NULL);
                                        break;
                                    }
                                    values = malloc(valueLen+1);
                                    memset(values, 0, valueLen+1);
                                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 6, values, (valueLen+1), &valuesLen , (CHAR *)LWM2MREADCONF_6_VALUE_DEF)) != AT_PARA_ERR)
                                    {

                                    }
                                }
                            }
                        }
                    }
                }
            }

            if(ret == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_PARAM_ERROR), NULL);
                break;
            }

            dataP = lwm2m_data_new(1);
            dataP->id = resId;
            switch (valueType) {
                case VALUE_TYPE_STRING:
                    lwm2m_data_encode_string((CHAR*)values, dataP);
                    break;
                case VALUE_TYPE_OPAQUE:
                    buffer = malloc(valueLen / 2);
                    //getStrFromHex(buffer, (CHAR*)value, valueLen / 2);
                    cmsHexStrToHex(buffer, valueLen/2, (CHAR *)values, valueLen);
                    lwm2m_data_encode_opaque(buffer, valueLen / 2, dataP);
                    free(buffer);
                    break;
                case VALUE_TYPE_INTEGER:
                    if(cmsBePureIntStr((CHAR*)values))
                    {
                        lwm2m_data_encode_int(atoi((CHAR*)values), dataP);
                    }
                    else
                    {
                        paramInvalid = TRUE;
                    }
                    break;
                case VALUE_TYPE_FLOAT:
                    lwm2m_data_encode_float(atof((CHAR*)values), dataP);
                    break;
                case VALUE_TYPE_BOOL:
                    lwm2m_data_encode_bool((atoi((CHAR*)values) == 0) ? FALSE : TRUE, dataP);
                    break;
                default:
                    dataP->type = LWM2M_TYPE_UNDEFINED;
                    break;
            }

            if(paramInvalid == FALSE)
            {
                if(lwm2m->readResultList != NULL)
                {
                    lwm2m_data_free(lwm2m->readResultCount, lwm2m->readResultList);
                }

                lwm2m->readResultList = dataP;
                lwm2m->readResultCount = 1;
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                if(dataP)
                {
                    lwm2m_data_free(1, dataP);
                    lwm2m->readResultCount = 0;
                    lwm2m->readResultList = NULL;
                }
                rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_PARAM_ERROR), NULL);
            }

            xSemaphoreGive(lwm2m->readWriteSemph);
            slpManPlatVoteEnableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
            break;
        }

        case AT_READ_REQ:
        case AT_EXEC_REQ:
        default:
        {
            rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }
    if(values)
        free(values);
    return rc;
}

/**
  \fn          CmsRetId lwm2mWRITECONF(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  lwm2mWRITECONF(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 lwm2mclientId, result;
    lwm2mClientContext* lwm2m = NULL;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+LWM2MWRITECONF=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+LWM2MWRITECONF: (0),(0-0xFF)");
            break;
        }

        case AT_SET_REQ:      /* AT+LWM2MWRITECONF                                                                                                                                                                             = */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &lwm2mclientId, LWM2MWRITECONF_0_ID_MIN, LWM2MWRITECONF_0_ID_MAX, LWM2MWRITECONF_0_ID_DEF)) != AT_PARA_ERR)
            {
                lwm2m = lwm2mFindClient(lwm2mclientId);
                if(lwm2m == NULL || lwm2m->context == NULL)
                {
                    rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_NO_FIND_CLIENT), NULL);
                    break;
                }

                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &result, LWM2MWRITECONF_1_RET_MIN, LWM2MWRITECONF_1_RET_MAX, LWM2MWRITECONF_1_RET_DEF)) != AT_PARA_ERR)
                {
                    lwm2m->writeResult = result;
                    xSemaphoreGive(lwm2m->readWriteSemph);
                    ret = AT_PARA_OK;
                }
            }

            if(ret == AT_PARA_OK)
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_PARAM_ERROR), NULL);
            }

            break;
        }

        case AT_READ_REQ:
        case AT_EXEC_REQ:
        default:
        {
            rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }
    return rc;
}

/**
  \fn          CmsRetId lwm2mEXECUTECONF(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  lwm2mEXECUTECONF(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 lwm2mclientId, result;
    lwm2mClientContext* lwm2m = NULL;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+LWM2MEXECUTECONF=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+LWM2MEXECUTECONF: (0),(0-0xFF)");
            break;
        }

        case AT_SET_REQ:      /* AT+LWM2MEXECUTECONF                                                                                                                                                                             = */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &lwm2mclientId, LWM2MEXECUTECONF_0_ID_MIN, LWM2MEXECUTECONF_0_ID_MAX, LWM2MEXECUTECONF_0_ID_DEF)) != AT_PARA_ERR)
            {
                lwm2m = lwm2mFindClient(lwm2mclientId);
                if(lwm2m == NULL || lwm2m->context == NULL)
                {
                    rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_NO_FIND_CLIENT), NULL);
                    break;
                }

                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &result, LWM2MEXECUTECONF_1_RET_MIN, LWM2MEXECUTECONF_1_RET_MAX, LWM2MEXECUTECONF_1_RET_DEF)) != AT_PARA_ERR)
                {
                    lwm2m->writeResult = result;
                    xSemaphoreGive(lwm2m->readWriteSemph);
                    ret = AT_PARA_OK;
                }
            }

            if(ret == AT_PARA_OK)
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_PARAM_ERROR), NULL);
            }

            break;
        }

        case AT_READ_REQ:
        case AT_EXEC_REQ:
        default:
        {
            rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }
    return rc;
}

/**
  \fn          CmsRetId lwm2mNOTIFY(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  lwm2mNOTIFY(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 lwm2mclientId, objId, instid, resId, valueType, valueLen;
    UINT8 * values = NULL;
    UINT8 * buffer = NULL;
    BOOL paramInvalid = FALSE;
    lwm2m_data_t* dataP = NULL;
    lwm2mClientContext* lwm2m = NULL;
    lwm2m_context_t* contextP = NULL;
    lwm2m_uri_t uri = {0};
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+LWM2MNOTIFY=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+LWM2MNOTIFY: (0),(0-0xFF),(0-0xFF),(0-0xFF),(0-4),(0-40),\"<value>\"");
            break;
        }

        case AT_SET_REQ:      /* AT+LWM2MNOTIFY                                                                                                                                                                             = */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &lwm2mclientId, LWM2MNOTIFY_0_ID_MIN, LWM2MNOTIFY_0_ID_MAX, LWM2MNOTIFY_0_ID_DEF)) != AT_PARA_ERR)
            {
                lwm2m = lwm2mFindClient(lwm2mclientId);
                if(lwm2m == NULL || lwm2m->context == NULL)
                {
                    ECOMM_TRACE(UNILOG_LWM2M, lwm2mNOTIFY_0, P_INFO, 0, "no find client");
                    rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_NO_FIND_CLIENT), NULL);
                    break;
                }
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &objId, LWM2MNOTIFY_1_OBJID_MIN, LWM2MNOTIFY_1_OBJID_MAX, LWM2MNOTIFY_1_OBJID_DEF)) != AT_PARA_ERR)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &instid, LWM2MNOTIFY_2_INSTID_MIN, LWM2MNOTIFY_2_INSTID_MAX, LWM2MNOTIFY_2_INSTID_DEF)) != AT_PARA_ERR)
                    {
                        if((ret = atGetNumValue(pAtCmdReq->pParamList, 3, &resId, LWM2MNOTIFY_3_RESOURCECOUNT_MIN, LWM2MNOTIFY_3_RESOURCECOUNT_MAX, LWM2MNOTIFY_3_RESOURCECOUNT_DEF)) != AT_PARA_ERR)
                        {
                            if((ret = atGetNumValue(pAtCmdReq->pParamList, 4, &valueType, LWM2MNOTIFY_4_TYPE_MIN, LWM2MNOTIFY_4_TYPE_MAX, LWM2MNOTIFY_4_TYPE_DEF)) != AT_PARA_ERR)
                            {
                                if((ret = atGetNumValue(pAtCmdReq->pParamList, 5, &valueLen, LWM2MNOTIFY_5_LEN_MIN, LWM2MNOTIFY_5_LEN_MAX, LWM2MNOTIFY_5_LEN_DEF)) != AT_PARA_ERR)
                                {
                                    if(valueType == VALUE_TYPE_OPAQUE && valueLen%2 == 1)
                                    {
                                        rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_PARAM_ERROR), NULL);
                                        break;
                                    }
                                    values = malloc(valueLen+1);
                                    memset(values, 0, valueLen+1);
                                    if((ret = atGetStrValue(pAtCmdReq->pParamList, 6, values, valueLen+1, &length , (CHAR *)LWM2MNOTIFY_6_VALUE_DEF)) != AT_PARA_ERR)
                                    {
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if(ret == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_PARAM_ERROR), NULL);
                break;
            }

            if(valueLen > 0){
                dataP = lwm2m_data_new(1);
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mNOTIFY_1, P_INFO, 1, "create new dataP=%x",dataP);
                dataP->id = resId;
                switch (valueType) {
                    case VALUE_TYPE_STRING:
                        lwm2m_data_encode_string((CHAR*)values, dataP);
                        break;
                    case VALUE_TYPE_OPAQUE:
                        buffer = malloc(valueLen / 2);
                        //getStrFromHex(buffer, (CHAR*)value, valueLen / 2);
                        cmsHexStrToHex(buffer, valueLen/2, (CHAR *)values, valueLen);
                        lwm2m_data_encode_opaque(buffer, valueLen / 2, dataP);
                        free(buffer);
                        break;
                    case VALUE_TYPE_INTEGER:
                        if(cmsBePureIntStr((CHAR*)values))
                        {
                            lwm2m_data_encode_int(atoi((CHAR*)values), dataP);
                        }
                        else
                        {
                            paramInvalid = TRUE;
                        }
                        break;
                    case VALUE_TYPE_FLOAT:
                        lwm2m_data_encode_float(atof((CHAR*)values), dataP);
                        break;
                    case VALUE_TYPE_BOOL:
                        lwm2m_data_encode_bool((atoi((CHAR*)values) == 0) ? FALSE : TRUE, dataP);
                        break;
                    default:
                        dataP->type = LWM2M_TYPE_UNDEFINED;
                        break;
                }
            }

            if(paramInvalid == FALSE)
            {
                contextP = lwm2m->context;
                uri.objectId = objId;
                uri.flag = LWM2M_URI_FLAG_OBJECT_ID;
                if(instid != -1)
                {
                    uri.instanceId = instid;
                    uri.flag = LWM2M_URI_FLAG_INSTANCE_ID;
                }
                if(resId != -1)
                {
                    uri.resourceId = resId;
                    uri.flag = LWM2M_URI_FLAG_RESOURCE_ID;
                }
                contextP->notifyDataP = dataP;
                ECOMM_TRACE(UNILOG_LWM2M, lwm2mNOTIFY_2, P_INFO, 0, "has prepare notifyDataP, disable slp2");
                slpManPlatVoteDisableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
                rc = lwm2mClientNotify(reqHandle, lwm2mclientId, &uri);
            }
            else
            {
                if(dataP)
                {
                    lwm2m_data_free(1, dataP);
                }
                rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_PARAM_ERROR), NULL);
                break;
            }

            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }
        case AT_READ_REQ:
        case AT_EXEC_REQ:
        default:
        {
            rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }
    if(values)
        free(values);
    return rc;
}

/**
  \fn          CmsRetId lwm2mUPDATE(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq       hh
  \returns     CmsRetId
*/
CmsRetId  lwm2mUPDATE(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 lwm2mclientId, withObj;
    lwm2mClientContext* lwm2m = NULL;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+LWM2MUPDATE=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR*)"+LWM2MUPDATE: (0),(0,1)");
            break;
        }

        case AT_SET_REQ:      /* AT+LWM2MUPDATE                                                                                                                                                                            = */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &lwm2mclientId, LWM2MUPDATE_0_ID_MIN, LWM2MUPDATE_0_ID_MAX, LWM2MUPDATE_0_ID_DEF)) != AT_PARA_ERR)
            {
                lwm2m = lwm2mFindClient(lwm2mclientId);
                if(lwm2m == NULL || lwm2m->context == NULL)
                {
                    rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_NO_FIND_CLIENT), NULL);
                    break;
                }
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &withObj, LWM2MUPDATE_1_WITHOBJ_MIN, LWM2MUPDATE_1_WITHOBJ_MAX, LWM2MUPDATE_1_WITHOBJ_DEF)) != AT_PARA_ERR)
                {
                    ECOMM_TRACE(UNILOG_LWM2M, lwm2mUPDATE_2, P_SIG, 0, "disable slp2");
                    slpManPlatVoteDisableSleep(lwm2mSlpHandler, SLP_SLP2_STATE);
                }
            }

            if(ret == AT_PARA_ERR)
            {
                rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_PARAM_ERROR), NULL);
                break;
            }
            lwm2m->reqHandle = reqHandle;
            INT32 result = lwm2mClientUpdate(reqHandle,lwm2mclientId,(UINT8)withObj);
            if(result == CMS_RET_SUCC){
                rc = CMS_RET_SUCC;
            }else{
                rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_INTER_ERROR), NULL);
            }
            break;
        }
        case AT_READ_REQ:
        case AT_EXEC_REQ:
        default:
        {
            rc = atcReply(reqHandle, AT_RC_LWM2M_ERROR, ( LWM2M_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }
    return rc;
}


