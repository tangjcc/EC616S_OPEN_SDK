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
#include "ctcc_dm.h"
#include "osasys.h"
#include "ostask.h"
#include "debug_log.h"
#include "netmgr.h"
#include "autoreg_common.h"
#include "ps_lib_api.h"
#include "at_coap_task.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif

extern int autoRegSuccFlag;
extern UINT8 ctccDmSlpHandler;

int autoRegcount = 0;
int autoRegTaskCount = 0;
char autoRegIpaddr[64] = {0}; 
coap_client autoregCoapClient;
extern  void coap_mutex_init_all(void);

extern  UINT32 mwGetSimCcidValue(UINT8 *ccid, UINT8 len);
extern  UINT32 mwSetSimCcidValue(UINT8 *ccid, UINT8 len);
extern  UINT8 mwGetCtccAutoRegFlag(void);
extern  void mwSetCtccAutoRegFlag(UINT8 autoReg);
extern  INT32 psSetGPRSAttached (UINT32 atHandle, BOOL fAttached);
extern  int mbedtls_base64_encode( unsigned char *dst, size_t dlen, size_t *olen,
                   const unsigned char *src, size_t slen );

BOOL checkNetworkReadyTimeOut(UINT8 times)
{
    UINT8 number = 0;
    NmAtiSyncRet netStatus;

    while(number < times)
    {
        appGetNetInfoSync(0, &netStatus);

        if(netStatus.body.netInfoRet.netifInfo.netStatus == NM_NETIF_ACTIVATED)
        {
            return TRUE;
        }
        else
        {
            osDelay(2000);
        }
        number++;
    }
    return FALSE;
}
void coapCheckRetAndResend(char *coapPayload)
{
    int coapPayloadLen = 0;

    coapPayloadLen = strlen(coapPayload);

    if(autoRegSuccFlag != 1)
    {
        for(int i=0; i<15; i++)
        {
            if(autoRegSuccFlag != 1)
            {
                //ECOMM_TRACE(UNILOG_PLA_APP, autoReg44, P_SIG, 0, "autoReg psSetGPRSAttached.....00");
                psSetGPRSAttached(0x1, FALSE);
                //ECOMM_TRACE(UNILOG_PLA_APP, autoReg45, P_SIG, 0, "autoReg psSetGPRSAttached.....01");
                osDelay(5000);
                //ECOMM_TRACE(UNILOG_PLA_APP, autoReg46, P_SIG, 0, "autoReg psSetGPRSAttached.....02");
                psSetGPRSAttached(0x1, TRUE);
                //ECOMM_TRACE(UNILOG_PLA_APP, autoReg47, P_SIG, 0, "autoReg psSetGPRSAttached.....03");
                //vTaskDelay(13000);
                BOOL result = checkNetworkReadyTimeOut(15);

                ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg09, P_SIG, 0, "autoReg check reg result");
                //coapClientSend(0, 0, "ct_coap", (CHAR *)PNULL, 2, 0, 0, 0, 0xff, (CHAR *)PNULL, coapPayload, 0, 0);
                //coap_client_send(0x1, 0, 0, 2, autoRegIpaddr, 5683, coapPayloadLen, coapPayload, COAP_SEND_AT);
                coap_autoreg_send(&autoregCoapClient, 0, 2, autoRegIpaddr, 5683, coapPayloadLen, coapPayload);
                /*check result*/
                for(int i=0; i<6; i++)
                {
                    if(autoRegSuccFlag != 1)
                    {
                        osDelay(2000);
                    }
                    else
                    {
                        break;
                    }
                }
                if(autoRegSuccFlag == 1)
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg20, P_SIG, 0, "autoReg recv register ack success");
                    return;
                }
            }
        }
    }

    if(autoRegSuccFlag == 1)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg10, P_SIG, 0, "autoReg recv register ack success");
    }
}

void coapCheckRetAndResendHour(char *coapPayload)
{
    int coapPayloadLen = 0;

    coapPayloadLen = strlen(coapPayload);
    while(autoRegSuccFlag != 1)
    {
        osDelay(30000); /* sleep 30 sec*/
        autoRegcount++;
        if(autoRegcount == 120)  /*wait for 1 hour*/
        {
            ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg11, P_SIG, 0, "autoReg retry one hour send register Data");
            //coapClientSend(0, 0, "ct_coap", (CHAR *)PNULL, 2, 0, 0, 0, 0xff, (CHAR *)PNULL, coapPayload, 0, 0);
            //coap_client_send(0x1, 0, 0, 2, autoRegIpaddr, 5683, coapPayloadLen, coapPayload, COAP_SEND_AT);
            coap_autoreg_send(&autoregCoapClient, 0, 2, autoRegIpaddr, 5683, coapPayloadLen, coapPayload);
            /*check result*/
            //vTaskDelay(4000);
            //coapCheckRetAndResend(coapPayload);
            break;
        }
    }
}

uint32_t startAutoRegCoap(uint8_t *IMEI, uint8_t *ICCID, uint8_t *IMSI)
{
    int ret = 1;
    char *coapPayload;
    char srcPayload[SRC_DATA_LEN] = {0};
    char buff[56] = {0};
    int srcLen = 0;
    int outLen = 0;
    int coapPayloadLen = 0;
    CoapCreateInfo coapInfo = {COAP_CREATE_FROM_AT, 0};
    struct addrinfo hints, *target_address = PNULL;
    char *model = NULL;
    char *swver = NULL;   
    UINT8 cfun = 0xff;

    ctccDeepSleepTimerCbRegister();
    coap_mutex_init_all();
    /*create client*/
    ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg58, P_SIG, 0, "autoReg create client");
    /*"coap://zzhc.vnet.cn" ip is "42.99.2.15", get by ecmdns cmd*/
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg200, P_SIG, 0, "autoReg start parse DNS");  
    osDelay(1500);
    sprintf((char*)buff, "coap://zzhc.vnet.cn");
    ECOMM_STRING(UNILOG_PLA_APP, ctcc_autoReg201, P_SIG, "autoReg parse DNS, url is: %s", (UINT8 *)buff);
    memset(buff, 0, 56);
    ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg202, P_SIG, 0, "autoReg parse DNS, lwip parseing DNS");
    
    if(getaddrinfo("zzhc.vnet.cn", NULL, &hints, &target_address) == 0)
    {
        appGetCFUN(&cfun);
        if(cfun == 0)
        {
            return 1;
        }
        sprintf(autoRegIpaddr, "%d.%d.%d.%d", target_address->ai_addr->sa_data[2],target_address->ai_addr->sa_data[3],target_address->ai_addr->sa_data[4],target_address->ai_addr->sa_data[5]);
        ECOMM_STRING(UNILOG_PLA_APP, ctcc_autoReg203, P_SIG, "autoReg parse DNS, the server ip is: %s", (UINT8 *)autoRegIpaddr);
        ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg204, P_SIG, 0, "autoReg parse DNS success!");
        //ret = coap_client_create(0, CTCC_COAP_LOCAL_PORT, coapInfo);
        memset(&autoregCoapClient, 0, sizeof(autoregCoapClient));
        ret = coap_autoreg_create(&autoregCoapClient, CTCC_COAP_LOCAL_PORT, coapInfo);
        
        //ret = coapClientCreate(0xff00, 0, "ct_coap", ipaddr, 5683, (CHAR *)PNULL, (CHAR *)PNULL, (CHAR *)PNULL, (CHAR *)PNULL, (CHAR *)PNULL, info);
    }
    else
    {
        ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg580, P_SIG, 0, "autoReg get dns fail");
        return ret;
    }

    if (ret == COAP_OK) //COAP create OK
    {
        /*send data*/
        coapPayload = malloc(DST_DATA_LEN);
        memset(coapPayload, 0, DST_DATA_LEN);
        model = (CHAR *)mwGetEcAutoRegModel();
        swver = (CHAR *)mwGetEcAutoRegSwver();
        /*ccid 89860435191890112882    */
        //sprintf((char*)srcPayload, "{\"REGVER\":\"2\",\"MEID\":\"%s\",\"MODEL\":\"eigen-Ec616\",\"SWVER\":\"SW_V10\",\"SIM1ICCID\":\"%s\",\"SIM1LTEIMSI\":\"%s\"}", IMEI, ICCID, IMSI);
        //sprintf((char*)srcPayload, "{\"REGVER\":\"2\",\"MEID\":\"356700082680757\",\"MODEL\":\"EiGENCOMM-EC616\",\"SWVER\":\"SW_V1.0\",\"SIM1ICCID\":\"89860435191890112842\",\"SIM1LTEIMSI\":\"460045500202842\"}");
        //sprintf((char*)srcPayload, "{\"REGVER\":\"2\",\"MEID\":\"356700082680757\",\"MODEL\":\"EiGENCOMM-EC616\",\"SWVER\":\"SW_V1.0\",\"SIM1ICCID\":\"%s\",\"SIM1LTEIMSI\":\"%s\"}", ICCID, IMSI);
        //sprintf((char*)srcPayload, "{\"REGVER\":\"2\",\"MEID\":\"%s\",\"MODEL\":\"EGN-YX EC616\",\"SWVER\":\"EC616_SW_V1.1\",\"SIM1ICCID\":\"%s\",\"SIM1LTEIMSI\":\"%s\"}", IMEI, ICCID, IMSI);
        //sprintf((char*)srcPayload, "{\"REGVER\":\"2\",\"MEID\":\"%s\",\"MODEL\":\"SC-MNF1P5A\",\"SWVER\":\"SC_MN_SW_V1.2\",\"SIM1ICCID\":\"%s\",\"SIM1LTEIMSI\":\"%s\"}", IMEI, ICCID, IMSI);//temp code for smartCore ctcc cert
        sprintf((char*)srcPayload, "{\"REGVER\":\"2\",\"MEID\":\"%s\",\"MODEL\":\"%s\",\"SWVER\":\"%s\",\"SIM1ICCID\":\"%s\",\"SIM1LTEIMSI\":\"%s\"}", IMEI, model, swver, ICCID, IMSI);//temp code for smartCore ctcc cert

        sprintf((char*)buff, "coap://zzhc.vnet.cn");
        ECOMM_STRING(UNILOG_PLA_APP, ctcc_autoReg50, P_SIG, "autoReg url is: %s", (UINT8 *)buff);

        memset(buff, 0, 56);
        sprintf((char*)buff, "5683");
        ECOMM_STRING(UNILOG_PLA_APP, ctcc_autoReg51, P_SIG, "autoReg port is: %s", (UINT8 *)buff);

        memset(buff, 0, 56);
        sprintf((char*)buff, "COAP");
        ECOMM_STRING(UNILOG_PLA_APP, ctcc_autoReg52, P_SIG, "autoReg register protocol is: %s", (UINT8 *)buff);

        memset(buff, 0, 56);
        sprintf((char*)buff, "POST");
        ECOMM_STRING(UNILOG_PLA_APP, ctcc_autoReg53, P_SIG, "autoReg register message type is: %s", (UINT8 *)buff);

        memset(buff, 0, 56);
        sprintf((char*)buff, "application/json (50)");
        ECOMM_STRING(UNILOG_PLA_APP, ctcc_autoReg54, P_SIG, "autoReg register message Content-Format is: %s", (UINT8 *)buff);


        srcLen = strlen(srcPayload);
        mbedtls_base64_encode((unsigned char *)coapPayload, (size_t)DST_DATA_LEN, (size_t*)&outLen, (unsigned char *)srcPayload,(size_t)srcLen);
        ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg56, P_SIG, 0, "prepare register data as JSON format");

        coapPayloadLen = strlen(coapPayload);

        ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg08, P_SIG, 0, "autoReg send register Data");
        //ret = coapClientSend(0, 0, "ct_coap", (CHAR *)NULL, 2, 0, 0, 0, 0xff, (CHAR *)NULL, coapPayload, 0, 0);
        //ret = coap_client_send(0x1, 0, 0, 2, autoRegIpaddr, 5683, coapPayloadLen, coapPayload, COAP_SEND_AT);
        ret = coap_autoreg_send(&autoregCoapClient, 0, 2, autoRegIpaddr, 5683, coapPayloadLen, coapPayload);

        if (ret == -1)
        {
            ECOMM_TRACE(UNILOG_PLA_APP, ctcc_startAutoRegCoap_1, P_WARNING, 1,
                        "AutoReg, COAP client send fail: %d", ret);
        }

        /*check result*/
        for(int i=0; i<100; i++)
        {
            if(autoRegSuccFlag != 1)
            {
                osDelay(200);
                appGetCFUN(&cfun);
                if(cfun == 0)
                {
                    free(coapPayload);
                    coap_autoreg_delete(&autoregCoapClient);
                    return 1;
                }
                coap_autoreg_recv(&autoregCoapClient);
            }
            else
            {
                break;
            }
        }
        
        #if CTCC_AUTO_REG_RETRY
        coapCheckRetAndResend(coapPayload);
        #endif
        
        if(autoRegSuccFlag != 1)
        {
            slpManDeepSlpTimerStart(CTCC_AUTO_REG_RETRY_TIMEOUT_ID, 60*60*1000); /*wait for 1 hour, and then resend reg packet */
        }
        //coapCheckRetAndResendHour(coapPayload);
        //coapCheckRetAndResendHour(coapPayload);
        //coapCheckRetAndResendHour(coapPayload);

        free(coapPayload);

        /*delete client*/
        //ret = coap_client_delete(0, 0);
        coap_autoreg_delete(&autoregCoapClient);
    }
    else
    {
        ECOMM_TRACE(UNILOG_PLA_APP, ctcc_startAutoRegCoap_2, P_WARNING, 1, "AutoReg, COAP client create faile: %d", ret);
    }

    return ret;
}

uint32_t startAutoRegHttp(uint8_t *IMEI, uint8_t *ICCID, uint8_t *IMSI)
{
    /*create client*/

    /*send data*/

    /*check result*/

    /*delete client*/
    return 1;
}

void ctccAutoRegisterTask(void *argument)
{
    UINT8 imei[IMEI_LEN+1] = {0};
    UINT8 imsi[IMSI_LEN+1] = {0};
    UINT8 flashCCID[CCID_LEN+1] = {0};
    UINT8 simCCID[CCID_LEN+1] = {0};
    UINT8 ctRegisterFlag = NON_REGISTER;
    NmAtiSyncRet netStatus;
    UINT8 cfun = 0xff;
    UINT32 ret = 0xff;

    slpManPlatVoteDisableSleep(ctccDmSlpHandler,SLP_SLP2_STATE);  //close sleep2 gate now, it can go to active or sleep1

    memset(imei, 0, IMEI_LEN+1);
    memset(imsi, 0, IMSI_LEN+1);
    memset(flashCCID, 0, CCID_LEN+1);

    //ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg01, P_SIG, 1, "autoReg..start..%d..",autoRegTaskCount);
    autoRegTaskCount++;

    appGetCFUN(&cfun);
    
    ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg0200, P_SIG, 1, "autoReg cfun is %d",cfun);
    if(cfun == 0) //at+cfun = 0
    {
        ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg0201, P_SIG, 0, "autoReg cfun is 0");
        slpManPlatVoteEnableSleep(ctccDmSlpHandler,SLP_SLP2_STATE); //open the sleep2 gate, it can go to sleep2 and hib state
        slpManGivebackPlatVoteHandle(ctccDmSlpHandler);
        ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg0202, P_SIG, 0, "autoReg will exit");
        
        osThreadExit();
    }
    else
    {
        for(int i=0; i<60; i++)
        {
            osDelay(1000);            
            appGetCFUN(&cfun);
            
            if(cfun == 0) //at+cfun = 0
            {
                ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg0203, P_SIG, 0, "autoReg cfun is 0");
                slpManPlatVoteEnableSleep(ctccDmSlpHandler,SLP_SLP2_STATE); //open the sleep2 gate, it can go to sleep2 and hib state
                slpManGivebackPlatVoteHandle(ctccDmSlpHandler);
                ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg0204, P_SIG, 0, "autoReg will exit");
                
                osThreadExit();
            }
            else
            {
                appGetNetInfoSync(0, &netStatus);
                if((netStatus.body.netInfoRet.netifInfo.netStatus == NM_NETIF_ACTIVATED)&&(mwGetTimeSyncFlag() == 1))
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg02, P_SIG, 0, "autoReg network is ready");
                    break;
                }
            }
            
        }
        
        if(netStatus.body.netInfoRet.netifInfo.netStatus != NM_NETIF_ACTIVATED)
        {
            ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg022, P_SIG, 0, "autoReg network is not ready");
        }
        
        //ECOMM_TRACE(UNILOG_PLA_APP, autoReg67, P_SIG, 1, "autoReg..ipType..%d..",netStatus.body.netInfoRet.netifInfo.ipType);
        
        if((netStatus.body.netInfoRet.netifInfo.ipType != NM_NET_TYPE_IPV6)&&(netStatus.body.netInfoRet.netifInfo.ipType != NM_NET_TYPE_INVALID))
        {
            osaGetImeiNumSync((CHAR *)imei);
        
            ECOMM_STRING(UNILOG_PLA_APP, ctcc_autoReg40, P_SIG, "autoReg get imei ok, imei is %s", (UINT8 *)imei);
            /* if imei is not set, use default imei, but the api will return false */
            {
                appGetImsiNumSync((CHAR *)imsi);
                appGetIccidNumSync((CHAR *)simCCID);
                mwGetSimCcidValue(flashCCID, CCID_LEN);
            }
        
            ECOMM_STRING(UNILOG_PLA_APP, ctcc_autoReg48, P_SIG, "autoReg get imsi is %s", (UINT8 *)imsi);
            ECOMM_STRING(UNILOG_PLA_APP, ctcc_autoReg41, P_SIG, "autoReg get current iccid is %s", (UINT8 *)simCCID);
            ECOMM_STRING(UNILOG_PLA_APP, ctcc_autoReg42, P_SIG, "autoReg get previous iccid is %s", (UINT8 *)flashCCID);
        
            /* check ccid, and start auto register */
            if(strcmp((CHAR *)flashCCID, (CHAR *)simCCID) != 0)
            {
                ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg03, P_SIG, 0, "autoReg find new sim iccid, start auto register");
                ret = startAutoRegCoap(imei, simCCID, imsi);
                if(ret == 1)
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg0205, P_SIG, 0, "autoReg cfun is 0");
                    slpManPlatVoteEnableSleep(ctccDmSlpHandler,SLP_SLP2_STATE); //open the sleep2 gate, it can go to sleep2 and hib state
                    slpManGivebackPlatVoteHandle(ctccDmSlpHandler);
                    ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg0206, P_SIG, 0, "autoReg will exit");
                    
                    osThreadExit();
                }
                if(autoRegSuccFlag == 1)
                {
                   mwSetSimCcidValue(simCCID, CCID_LEN);
                   mwSetCtccAutoRegFlag(ALREADY_REGISTER);
                   ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg04, P_SIG, 0, "autoReg done sucess,record current iccid to NV");
                }
            }
            else if(strcmp((CHAR *)flashCCID, (CHAR *)simCCID) == 0)
            {
                ctRegisterFlag = mwGetCtccAutoRegFlag();
                ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg05, P_SIG, 0, "autoReg find same sim iccid");
                if(ctRegisterFlag != ALREADY_REGISTER)
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg06, P_SIG, 0, "autoReg not register before, start register");
                    startAutoRegCoap(imei, simCCID, imsi);
                    if(autoRegSuccFlag == 1)
                    {
                        mwSetCtccAutoRegFlag(ALREADY_REGISTER);
                        ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg07, P_SIG, 0, "autoReg done sucess,record current iccid to NV");
                    }
                }
                else
                {
                    ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg37, P_SIG, 0, "autoReg has done before, ignore register");
                }
            }
        
        }
        else
        {
            //ECOMM_TRACE(UNILOG_PLA_APP, autoReg66, P_DEBUG, 0, "autoReg..ip..is..ipv6 only..disable..atuoreg..");
        }
        
        slpManPlatVoteEnableSleep(ctccDmSlpHandler,SLP_SLP2_STATE); //open the sleep2 gate, it can go to sleep2 and hib state
        slpManGivebackPlatVoteHandle(ctccDmSlpHandler);
        ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg0288, P_SIG, 0, "autoReg will exit");
        
        osThreadExit();
    }
}

/*ctcc task static stack and control block*/
osThreadId_t ctccAutoRegisterTaskHandle = NULL;

void ctccAutoregRetryTimeExpCallback(void)
{
    void *argument;
    
    ctccAutoRegisterTask(argument);
}

void ctccDeepSleepTimerCbRegister(void)
{
    slpManInternalDeepSlpTimerRegisterExpCb(ctccAutoregRetryTimeExpCallback,CTCC_AUTO_REG_RETRY_TIMEOUT_ID);
}

void ctccAutoRegisterInit(void)
{
    osThreadAttr_t task_attr;
    memset(&task_attr, 0, sizeof(task_attr));

    slpManApplyPlatVoteHandle("CTCC_DM",&ctccDmSlpHandler);

    task_attr.name = "ctccAutoReg";
    task_attr.stack_size = CTCC_AUTO_REG_TASK_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal5;

    ECOMM_TRACE(UNILOG_PLA_APP, ctcc_autoReg00, P_SIG, 1, "autoReg..starting task ");

    ctccAutoRegisterTaskHandle= osThreadNew(ctccAutoRegisterTask, NULL,&task_attr);
    if(ctccAutoRegisterTaskHandle == NULL)
    {
        if(slpManGivebackPlatVoteHandle(ctccDmSlpHandler) != RET_TRUE)
        {
            ECOMM_TRACE(UNILOG_DM, CTCC_DM0, P_ERROR, 0, "CTCC_DM vote handle give back failed");
        }
        else
        {
            ECOMM_TRACE(UNILOG_DM, CTCC_DM1, P_VALUE, 0, "CTCC_DM vote handle give back success");
        }
    }
}


