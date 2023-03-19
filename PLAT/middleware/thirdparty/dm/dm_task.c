/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of EigenComm Ltd.
 * File name:    plat_config.c
 * Description:  platform configuration source file
 * History:      Rev1.0   2019-01-18
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include "lfs_port.h"
#include "osasys.h"
#include "ostask.h"
#include "debug_log.h"
#include "netmgr.h"
#include "autoreg_common.h"
#include "ps_lib_api.h"
#include "ctcc_dm.h"
#include "cucc_dm.h"
#include "cmcc_dm.h"
#include "mw_config.h"
#include "queue.h"
#include "ps_event_callback.h"

typedef struct
{
    INT32 cmdType;
    UINT8 imsi[IMSI_LEN+1];
}dmInterMsg;

#define DM_MSG_NETWORK_READY   1 
#define DM_MSG_SIM_READY       2 
#define DM_MSG_MAX_NUMB        6 
#define DM_MSG_TIMEOUT         1000
#define DM_MSG_RECV_TIMEOUT    4000

UINT8 ctccDmSlpHandler = 0xff;
UINT8 cuccDmSlpHandler = 0xff;
UINT8 cmccDmSlpHandler = 0xFF;
QueueHandle_t dmMsgHandle = NULL;

INT32 dmNetWorkStatusCallback(urcID_t eventID, void *param, UINT32 paramLen)
{
    NmAtiNetifInfo *netif = NULL;
    dmInterMsg dmMsg;
    CmiSimImsiStr *imsi = NULL;

    memset(&dmMsg, 0, sizeof(dmInterMsg));
    switch(eventID)
    {
        case NB_URC_ID_PS_NETINFO:
        {
            netif = (NmAtiNetifInfo *)param;
            if (netif->netStatus == NM_NETIF_ACTIVATED)
            {
                dmMsg.cmdType = DM_MSG_NETWORK_READY;
                if(dmMsgHandle != NULL)
                {
                    xQueueSend(dmMsgHandle, &dmMsg, DM_MSG_TIMEOUT);
                    ECOMM_TRACE(UNILOG_DM, dmNetWorkStatusCallback_1, P_INFO, 0, "dm network is ready");
                }
                else
                {
                    ECOMM_TRACE(UNILOG_DM, dmNetWorkStatusCallback_2, P_ERROR, 0, "dm network dmMsgHandle is NULL");
                }
            }
            break;
        }
        
        case NB_URC_ID_SIM_READY:
        {
            imsi = (CmiSimImsiStr *)param;
            memcpy(dmMsg.imsi, imsi->contents, imsi->length);
            
            dmMsg.cmdType = DM_MSG_SIM_READY;
            if(dmMsgHandle != NULL)
            {
                xQueueSend(dmMsgHandle, &dmMsg, DM_MSG_TIMEOUT);
                ECOMM_TRACE(UNILOG_DM, dmNetWorkStatusCallback_3, P_INFO, 0, "dm sim is ready");
            }
            else
            {
                ECOMM_TRACE(UNILOG_DM, dmNetWorkStatusCallback_4, P_ERROR, 0, "dm sim dmMsgHandle is NULL");
            }
            break;
        }
        
        default:
            break;
    }
    return 0;
}

void ecAutoRegisterCheckTask(void *argument)
{
    UINT8 imsi[IMSI_LEN+1] = {0};
    UINT8 imsiReadyFlag = 0;
    UINT8 networkReadyFlag = 0;
    UINT8 cfun = 0xff;
    INT32 msgWaitCount = 60;
    dmInterMsg dmMsg;
    slpManSlpState_t slpState = slpManGetLastSlpState();
   
    memset(&dmMsg, 0, sizeof(dmInterMsg));
    dmMsgHandle = xQueueCreate(DM_MSG_MAX_NUMB, sizeof(dmInterMsg));

    registerPSEventCallback(NB_GROUP_ALL_MASK, dmNetWorkStatusCallback);

    while(msgWaitCount)
    {
        xQueueReceive(dmMsgHandle, &dmMsg, DM_MSG_RECV_TIMEOUT);

        switch(dmMsg.cmdType)
        {
            case DM_MSG_NETWORK_READY:
                networkReadyFlag = 1;
                appGetCFUN(&cfun);
                ECOMM_TRACE(UNILOG_DM, ec_autoReg00, P_INFO, 2, "dm network is ready %d %d", networkReadyFlag, cfun);
                break;

            case DM_MSG_SIM_READY:
                imsiReadyFlag = 1;
                memcpy(imsi, dmMsg.imsi, strlen((const char*)dmMsg.imsi));
                
                if(((imsi[3] == '0')&&(imsi[4] == '0'))||
                  ((imsi[3] == '0')&&(imsi[4] == '2'))||
                  ((imsi[3] == '0')&&(imsi[4] == '4'))||
                  ((imsi[3] == '0')&&(imsi[4] == '7')))
                 {
                    imsiReadyFlag = 2;   //for CMCC DM, it need not wait for network ready here
                 }
                else
                {
                    appGetCFUN(&cfun);   //for CTCC CUCC DM, it need wait for nerwork ready
                }
                ECOMM_TRACE(UNILOG_DM, ec_autoReg01, P_INFO, 2, "dm sim is ready %d %d", imsiReadyFlag, cfun);
                break;

            default:
                break;
        }
        msgWaitCount--;      

        if(imsiReadyFlag == 2)   //for CMCC DM, it do not wait for network ready here
        {
            break;
        }
        
        if((networkReadyFlag == 1)&&(imsiReadyFlag == 1))
        {
            break;
        }

        if((networkReadyFlag == 1)&&(imsiReadyFlag != 1))
        {
            if(cfun == 0) //at+cfun = 0
            {
                break;
            }
        }

        if((networkReadyFlag != 1)&&(imsiReadyFlag == 1))
        {
            if(cfun == 0) //at+cfun = 0
            {
                break;
            }
        }        
    }

    appGetCFUN(&cfun);

    if(imsiReadyFlag != 2)
    {
        if((networkReadyFlag != 1)||(imsiReadyFlag != 1)||(cfun == 0))
        {
            ECOMM_TRACE(UNILOG_DM, ec_autoReg02, P_INFO, 2, "dm network or sim is not ready %d %d", networkReadyFlag, cfun);
            if(dmMsgHandle != NULL)
            {
                deregisterPSEventCallback(dmNetWorkStatusCallback);
                vQueueDelete(dmMsgHandle);
                dmMsgHandle = NULL;
            }
            osThreadExit();
        }
    }

    if((imsi[0] == '4')&&(imsi[1] == '6')&&(imsi[2] == '0'))
    {
        if(((imsi[3] == '0')&&(imsi[4] == '3'))||
            ((imsi[3] == '0')&&(imsi[4] == '5'))||
            ((imsi[3] == '1')&&(imsi[4] == '1')))
        {
            ECOMM_TRACE(UNILOG_DM, ec_autoReg03, P_SIG, 0, "autoReg this is CTCC SIM card");
            #ifdef FEATURE_CTCC_DM_ENABLE
                UINT8 ctcc_autoreg_enable_flag = 0xff;
                ctcc_autoreg_enable_flag = mwGetCtccAutoRegEnableFlag();
                if(ENABLE_REGISTER == ctcc_autoreg_enable_flag)
                {
                    ctccAutoRegisterInit();
                }
                else
                {
                    ECOMM_TRACE(UNILOG_DM, ec_autoReg04, P_SIG, 0, "autoReg CTCC is disble");
                }
            #else
                ECOMM_TRACE(UNILOG_DM, ec_autoReg05, P_SIG, 0, "autoReg CTCC code is not enable");

            #endif

        }
        else if(((imsi[3] == '0')&&(imsi[4] == '1'))||
                 ((imsi[3] == '0')&&(imsi[4] == '6')))
        {
            ECOMM_TRACE(UNILOG_PLA_APP, ec_autoReg06, P_SIG, 0, "autoReg this is CUCC SIM card");
            #ifdef FEATURE_CUCC_DM_ENABLE
                UINT8 cucc_autoreg_enable_flag = 0xff;
                cucc_autoreg_enable_flag = mwGetCuccAutoRegEnableFlag();
                if(ENABLE_REGISTER == cucc_autoreg_enable_flag)
                {
                    cuccAutoRegisterInit();
                }
                else
                {
                    ECOMM_TRACE(UNILOG_DM, ec_autoReg07, P_SIG, 0, "autoReg CUCC is disble");
                }
            #else
                ECOMM_TRACE(UNILOG_DM, ec_autoReg08, P_SIG, 0, "autoReg CUCC code is not enable");

            #endif

        }
        else if(((imsi[3] == '0')&&(imsi[4] == '0'))||
                 ((imsi[3] == '0')&&(imsi[4] == '2'))||
                 ((imsi[3] == '0')&&(imsi[4] == '4'))||
                 ((imsi[3] == '0')&&(imsi[4] == '7')))
        {
            ECOMM_TRACE(UNILOG_PLA_APP, ec_autoReg09, P_SIG, 0, "autoReg this is CMCC SIM card");
            #ifdef FEATURE_CMCC_DM_ENABLE
                UINT8 cmcc_autoreg_enable_flag = 0xff;
                cmcc_autoreg_enable_flag = mwGetCmccAutoRegEnableFlag();
                if(ENABLE_CMCC_REGISTER == cmcc_autoreg_enable_flag)
                {
                    cmccAutoRegisterInit(slpState);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_DM, ec_autoReg10, P_SIG, 0, "autoReg CMCC is disble");
                }
            #else
                ECOMM_TRACE(UNILOG_DM, ec_autoReg11, P_SIG, 0, "autoReg CMCC code is not enable");

            #endif

        }
        else
        {
            ECOMM_TRACE(UNILOG_DM, ec_autoReg12, P_SIG, 0, "autoReg this MNC is not supported SIM card");
        }
    }
    else
    {
        if(dmMsgHandle != NULL)
        {
            deregisterPSEventCallback(dmNetWorkStatusCallback);
            vQueueDelete(dmMsgHandle);
            dmMsgHandle = NULL;
        }
        ECOMM_STRING(UNILOG_DM, ec_autoReg13, P_SIG, "autoReg the SIM card is %s, it is not '460' SIM card, exit now", (UINT8 *)imsi);
        osThreadExit();
    }
    
    if(dmMsgHandle != NULL)
    {
        deregisterPSEventCallback(dmNetWorkStatusCallback);
        vQueueDelete(dmMsgHandle);
        dmMsgHandle = NULL;
    }
    osThreadExit();
}

/*ctcc task static stack and control block*/
osThreadId_t ecAutoRegisterTaskHandle;

void ecAutoRegisterInit(void)
{
    slpManApplyPlatVoteHandle("CMCC_DM",&cmccDmSlpHandler);

    osThreadAttr_t task_attr;
    memset(&task_attr, 0, sizeof(task_attr));
    task_attr.name = "autoregCheck";
    task_attr.stack_size = EC_AUTO_REG_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;

    ECOMM_TRACE(UNILOG_DM, ec_autoReg14, P_SIG, 0, "autoReg..starting check imsi");

    ecAutoRegisterTaskHandle= osThreadNew(ecAutoRegisterCheckTask, NULL,&task_attr);
    if(ecAutoRegisterTaskHandle == NULL)
    {
        //add error assert
    }
}


