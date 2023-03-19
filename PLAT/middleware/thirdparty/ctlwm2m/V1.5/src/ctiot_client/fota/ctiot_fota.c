
#ifdef WITH_FOTA
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
//#include "rng_ec616.h"

#include "ctiot_fota.h"
#include "ctiot_NV_data.h"


#define MAX_PACKET_SIZE_1 1024

extern ctiotFotaManage fotaStateManagement;

CTIOT_URI fotaUrl;

static int messageID = 0;
static int sockfd;
struct sockaddr_in addr;
int addr_len = sizeof(struct sockaddr_in);

char* uri = NULL;
char* query = NULL;
uint32_t blockNumber = 0;
uint8_t resendCount = 5;
TimerHandle_t abup_reboot_timer;


/******************************************************************/
#if defined(CTIOT_ABUP_FOTA_ENABLE)
#include "mem_map.h"
#define ABUP_BLOCK_SIZE             (32*1024)
#define ABUP_FLAG_BLOCK_SIZE        (4*1024)

static uint8_t abup_update_result = 2;

uint32_t ct_abup_get_block_size(void)
{
    return ABUP_BLOCK_SIZE;
}

uint32_t ct_abup_get_delta_base(void)
{
    return FLASH_FOTA_REGION_START;
}

uint32_t ct_abup_get_delta_len(void)
{
    return (FLASH_FOTA_REGION_END - FLASH_FOTA_REGION_START - ct_abup_get_block_size());
}

uint32_t ct_abup_get_backup_base(void)
{
    return (FLASH_FOTA_REGION_END - ct_abup_get_block_size());
}

uint32_t ct_abup_get_data_base(void)
{
    return (FLASH_FOTA_REGION_END - 2*ct_abup_get_block_size());
}

void ct_abup_check_upgrade_result(void)
{
    uint8_t backup_data[4] = {0};

    ct_abup_read_backup(0, backup_data, 4);
    if (strncmp((char *)backup_data, "OK", 2) == 0)
    {
        abup_update_result=1;
        ECOMM_TRACE(UNILOG_CTLWM2M, UNILOG_ABUP_FLASH_LOG_2, P_INFO, 0, "update success");
    }
    else if (strncmp((char *)backup_data, "NO", 2) == 0)
    {
        abup_update_result=99;
        ECOMM_TRACE(UNILOG_CTLWM2M, UNILOG_ABUP_FLASH_LOG_3, P_INFO, 0, "update fail");
    }
    else
    {
        abup_update_result=0;
        ECOMM_TRACE(UNILOG_CTLWM2M, UNILOG_ABUP_FLASH_LOG_4, P_INFO, 0, "no update result");
    }
}

uint8_t ct_abup_get_upgrade_result(void)
{
    return abup_update_result;
}


uint32_t ct_abup_write_flash(uint32_t addr, uint8_t* data_ptr, uint32_t len)
{
    uint8_t retValue;
    if (len == 0) return LFS_ERR_OK;
    uint32_t address = ct_abup_get_delta_base() + addr;

    retValue = BSP_QSPI_Write_Safe((uint8_t *)data_ptr, address, len);

    ECOMM_TRACE(UNILOG_CTLWM2M, UNILOG_ABUP_FLASH_LOG_5, P_INFO, 3, "abup_write_flash-->address;%x,len:%x,ret:%d",address,len,retValue);
    return (retValue == QSPI_OK) ? LFS_ERR_OK: LFS_ERR_IO;
}

uint32_t ct_abup_read_backup(uint32_t addr,uint8_t* data_ptr, uint32_t len)
{
    uint8_t retValue;
    uint32_t address = ct_abup_get_backup_base() + addr;

    retValue = BSP_QSPI_Read_Safe((uint8_t *)data_ptr, address, len);

    ECOMM_TRACE(UNILOG_CTLWM2M, UNILOG_ABUP_FLASH_LOG_6, P_INFO, 3, "abup_read_backup-->address;%x,len:%x,ret:%d",address,len,retValue);
    return (retValue == QSPI_OK) ? LFS_ERR_OK: LFS_ERR_IO;
}

uint32_t ct_abup_erase_backup(void)
{
    uint8_t retValue;
    uint32_t address = ct_abup_get_backup_base();
    uint8_t DldStatStart[5] = {'A','D', 'U', 'P', 'S'};

    retValue = BSP_QSPI_Erase_Safe(address, ABUP_BLOCK_SIZE);
    ECOMM_TRACE(UNILOG_CTLWM2M, UNILOG_ABUP_FLASH_LOG_7, P_INFO, 2, "abup_erase_backup-->address;%x,ret:%d",address,retValue);
    if(retValue != QSPI_OK)
        return QSPI_ERROR;
    
    retValue=BSP_QSPI_Write_Safe(&DldStatStart[0],address,  sizeof(DldStatStart));
    ECOMM_TRACE(UNILOG_CTLWM2M, UNILOG_ABUP_FLASH_LOG_71, P_INFO, 2, "abup_write_start_flag szie;%d,ret:%d",sizeof(DldStatStart),retValue);
    return (retValue == QSPI_OK) ? LFS_ERR_OK: LFS_ERR_IO;
}

int8_t ct_abup_erase_delata(void)
{
    uint8_t retValue;
    uint32_t address = ct_abup_get_delta_base();
    uint32_t len=ct_abup_get_delta_len();
    uint32_t real_erase_size = len;
    uint32_t real_address = address;

    while(real_erase_size > 0)
    {
        retValue = BSP_QSPI_Erase_Safe(real_address,ABUP_BLOCK_SIZE);
        if (retValue != QSPI_OK)
        {                
            ECOMM_TRACE(UNILOG_CTLWM2M, UNILOG_ABUP_FLASH_LOG_8, P_INFO, 2, "abup_erase_delata error:real_address:%x,ret:%d",real_address,retValue);
            return LFS_ERR_IO;      
        }

        real_address += ABUP_BLOCK_SIZE;
        real_erase_size -= ABUP_BLOCK_SIZE;

        ECOMM_TRACE(UNILOG_CTLWM2M, UNILOG_ABUP_FLASH_ERASE_9, P_INFO, 2, "abup_erase_delata-->real_address:%x,ret:%d",real_address,retValue);
        vTaskDelay(300 / portTICK_RATE_MS);
    }

    return LFS_ERR_OK;
}

int32_t ct_abup_init_flash(void)
{
    uint16_t ret1,ret2;

    ret1 = ct_abup_erase_delata();
    ret2 = ct_abup_erase_backup();
    if(ret1 == 0 && ret2 == 0)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, UNILOG_ABUP_FLASH_LOG_10, P_INFO, 0, "abup_init_flash-->sucess");
        return 0;
    }
    else
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, UNILOG_ABUP_FLASH_LOG_11, P_INFO, 0, "abup_init_flash-->fail");
        return -1;
    }
}

void abup_check_version_callback(TimerHandle_t expiredTimer)
{
    ECOMM_TRACE(UNILOG_CTLWM2M, UNILOG_ABUP_FLASH_LOG_12, P_INFO, 0, "abup_check_version_ct_callback.....");
    //abup_send_reg_info("AT+M2MCLISEND=0001",18);
}

void abup_check_version(uint32_t ticks)
{
    TimerHandle_t timer;
    if(ticks < 10000)
    {
    ticks = 10000;
    }

    timer = xTimerCreate("abupctversion", ticks /portTICK_PERIOD_MS, false, NULL, abup_check_version_callback);
    if(NULL != timer)
    {
        xTimerStart(timer, 0);
    }
}

void ct_abup_system_reboot()
{   
    ECOMM_TRACE(UNILOG_CTLWM2M, UNILOG_ABUP_FLASH_LOG_13, P_INFO, 0, "abup_system_reboot-->");
    __disable_irq();

    #if defined CHIP_EC616S
    extern void PmuAonPorEn(void);
    PmuAonPorEn();
    #endif
    
    NVIC_SystemReset();
}

static void abup_system_reboot_timer(TimerHandle_t xTimer)
{
    xTimerStop(abup_reboot_timer, 0);
    ct_abup_system_reboot();
}

void abup_prepare_reboot(uint32_t ticks)
{
    if(ticks < 3000)
    {
        ticks = 3000;
    }

    abup_reboot_timer = xTimerCreate("abup_reboot_timer", ticks /portTICK_PERIOD_MS, false, NULL, abup_system_reboot_timer);
    ECOMM_TRACE(UNILOG_CTLWM2M, UNILOG_ABUP_FLASH_LOG_14, P_INFO, 1, "abup_reboot_timer:%d",abup_reboot_timer);
    if(NULL != abup_reboot_timer)
    {
        xTimerStart(abup_reboot_timer, 0);
    }
}
#endif
/******************************************************************/

void ctiot_funcv1_fota_state_changed(void)
{
    ctiot_funcv1_context_t* pContext;

    pContext = ctiot_funcv1_get_context();
    lwm2m_uri_t uri;
    ctlwm2m_stringToUri("/5/0", 4, &uri);
    osMutexAcquire(pContext->lwm2mContext->observe_mutex, osWaitForever);
    ct_lwm2m_resource_value_changed(pContext->lwm2mContext,&uri);
    osMutexRelease(pContext->lwm2mContext->observe_mutex);
    return;
}

void setUrl(void)
{
    char* queryTemp = NULL;
    uri = strtok(fotaUrl.uri,"?");
    queryTemp = strtok(NULL,"?");
    printf("uri is %s\n",uri);
    if(queryTemp != NULL)
    {
        query = (char*)ct_lwm2m_malloc(strlen(queryTemp) + 2);
        memset(query,0,strlen(queryTemp) + 2);
        strcpy(query,"?");
        strcat(query,queryTemp);

        printf("query is %s\n",query);
        ECOMM_STRING(UNILOG_CTLWM2M, setUrl_0, P_INFO, "setUrl:%s", (const uint8_t *)query);

    }
}

int send_resquest(uint32_t num, uint8_t more, uint16_t size)
{
    int result = 0;
    uint8_t * pktBuffer;
    size_t pktBufferLen = 0;
    size_t allocLen;
    static coap_packet_t message[1];

    printf("message ID is %d\n",messageID);
    ECOMM_TRACE(UNILOG_CTLWM2M, send_resquest_0, P_INFO, 1, "send_resquest message ID is %d", messageID);
    coap_init_message(message, COAP_TYPE_CON, COAP_GET, messageID);
    coap_set_header_uri_path(message, uri);

    if(query != NULL)
        coap_set_header_uri_query(message, query);

    coap_set_header_block2(message,num,more,size);
    allocLen = coap_serialize_get_size(message);
    if (allocLen == 0)
        return COAP_500_INTERNAL_SERVER_ERROR;

    pktBuffer = (uint8_t *)ct_lwm2m_malloc(allocLen);
    if (pktBuffer != NULL)
    {
        pktBufferLen = coap_serialize_message(message, pktBuffer);
        if (0 != pktBufferLen)
        {
            result = sendto(sockfd, pktBuffer, pktBufferLen, 0, (struct sockaddr *)&addr, addr_len);
            printf("sendto result %d\n",result);
            ECOMM_TRACE(UNILOG_CTLWM2M, send_resquest_1, P_INFO, 1, "sendto result %d",result);
        }
        ct_lwm2m_free(pktBuffer);
    }

    return result;
}

uint32_t ctiot_rand()
{
    uint32_t random=0;
    uint32_t a=0;
    uint32_t b=0;
    uint8_t i=0;
    uint8_t Rand[24];
    
    extern int32_t RngGenRandom(uint8_t Rand[24]);
    
    RngGenRandom(Rand);
    for(i=0; i<24; i++){
        a += Rand[i];
        b ^= Rand[i];
    }
    random = (a<<8) + b;
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_rand_1, P_SIG, 1, "ctiot_rand:%d",random);
    return random;
}

void firmware_fota(void* arg)
{
    uint32_t firmwareSize = 0;
    uint32_t address;
    bool runThread=1;
    ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_0_0, P_INFO, 2, "thread start...fotastate(%d) blockNumber(%d)",fotaStateManagement.fotaState,blockNumber);

    if(blockNumber == 0){
        ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_0_1, P_INFO, 0, "begin init abup flash first");
        if(ct_abup_init_flash() != 0)
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_0_2, P_ERROR, 0, "init flash failed");
            thread_exit(NULL);
            return;
        }
    }

    messageID = ctiot_rand();
    setUrl();
    firmwareSize = blockNumber * BLOCK_SIZE;
    send_resquest(blockNumber,START_MORE,BLOCK_SIZE);

    while(runThread)
    {
        struct timeval tv;
        int result;
        tv.tv_sec = 10;//读socket时间
        tv.tv_usec = 0;
        uint8_t buffer[MAX_PACKET_SIZE_1];
        int numBytes;
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        result = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);
        if(result > 0)
        {
            if (FD_ISSET(sockfd, &readfds))
            {
                struct sockaddr_storage addr;
                socklen_t addrLen;
                addrLen = sizeof(addr);
                numBytes = recvfrom(sockfd, buffer, MAX_PACKET_SIZE_1, 0, (struct sockaddr *)&addr, &addrLen);

                if (0 > numBytes)
                {
                    fprintf(stderr, "Error in recvfrom(): %d %s\r\n", errno, strerror(errno));
                }
                else if (0 < numBytes)
                {
                    uint8_t coap_error_code = COAP_NO_ERROR;
                    static coap_packet_t message[1];
                    coap_error_code = coap_parse_message(message, buffer, numBytes);
                    if (coap_error_code == COAP_NO_ERROR)
                    {
                        switch (message->type)
                        {
                            case COAP_TYPE_NON:
                            case COAP_TYPE_CON:
                            case COAP_TYPE_RST:
                                ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_0, P_INFO, 0, "fota no ack");
                                break;

                            case COAP_TYPE_ACK:
                            {
                                ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_1_0, P_INFO, 2, "message->block2_num:%d,blockNumber:%d ",message->block2_num,blockNumber);
                                if(blockNumber != message->block2_num)
                                {
                                    ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_1_1, P_INFO, 0, "not the block which current request, abandon it");
                                }
                                else
                                {
                                    int32_t ret = -1;
                                    ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_1_2, P_INFO, 2, "get the right ack, address:%x,block2_num:%d",firmwareSize,message->block2_num);
                                    ret=ct_abup_write_flash(firmwareSize, message->payload, message->payload_len);
                                    if(ret != 0)
                                    { 
                                        ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_1_3, P_INFO, 0, "write_flash error!");
                                        EC_ASSERT(FALSE,message->payload_len,0,0);
                                    }
                                    firmwareSize += message->payload_len;
                                    if(firmwareSize > FLASH_FOTA_REGION_END-FLASH_FOTA_REGION_START-ABUP_BLOCK_SIZE - BLOCK_SIZE)//diff package over size, stop downloading
                                    {
                                        ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_1_5, P_INFO, 0, "diff package over size,stop downloading notify server fail");
                                        fotaStateManagement.fotaState = FOTA_STATE_IDIL;
                                        fotaStateManagement.fotaResult = FOTA_RESULT_NOFREE;
                                        ctiot_funcv1_fota_state_changed();
                                        ctiot_funcv1_status_t wakeStatus[1]={0};
                                        wakeStatus->baseInfo=ct_lwm2m_strdup(FOTA1);
#ifndef FEATURE_REF_AT_ENABLE
                                        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, wakeStatus, 0);
                                        ct_lwm2m_free(wakeStatus->baseInfo);
#else       
                                        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_HSSTATUS, wakeStatus, 0);
#endif
                                        ctiot_funcv1_notify_event(MODULE_LWM2M, FOTA_DOWNLOAD_FAIL, NULL, 0);
                                        blockNumber = 0;//clear fota range
#ifdef WITH_FOTA_RESUME
                                        c2f_encode_fotarange_params();
#endif
                                        ctiot_funcv1_set_boot_flag(NULL, BOOT_LOCAL_BOOTUP);
                                        slpManFlushUsrNVMem();
                                        runThread=0;
                                        break;
                                    }
#ifdef WITH_FOTA_RESUME
                                    c2f_encode_fotarange_params();
#endif
                                    if(message->block2_more != 0)
                                    {
                                        ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_1_4, P_INFO, 0, "has more data continue request");
                                        resendCount = 5;
                                        blockNumber = message->block2_num+1;
                                        messageID++;
                                        send_resquest(message->block2_num+1, START_MORE, message->block2_size);
                                    }
                                    else
                                    {
                                        fotaStateManagement.fotaState = FOTA_STATE_DOWNLOADED;
                                        fotaStateManagement.fotaResult = FOTA_RESULT_INIT;
                                        ctiot_funcv1_fota_state_changed();
                                        ctiot_funcv1_status_t wakeStatus[1]={0};
#ifndef FEATURE_REF_AT_ENABLE
                                        wakeStatus->baseInfo=ct_lwm2m_strdup(FOTA2);
                                        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, wakeStatus, 0);
                                        ct_lwm2m_free(wakeStatus->baseInfo);
#else       
                                        wakeStatus->baseInfo=ct_lwm2m_strdup(FOTA2_BC28);
                                        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_HSSTATUS, wakeStatus, 0);
#endif
                                        ctiot_funcv1_notify_event(MODULE_LWM2M, FOTA_DOWNLOAD_SUCCESS, NULL, 0);
                                        ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_5, P_INFO, 0, "firmware download end with success. (SET fotaState->2)");

                                        //download sucess, erase start flag
                                        address = ct_abup_get_backup_base();
                                        ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_51, P_INFO, 1, "erase start flag addr %x",address);
                                        ret = BSP_QSPI_Erase_Safe(address, ABUP_FLAG_BLOCK_SIZE);
                                        if (ret != 0)
                                        {
                                            ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_52, P_INFO, 0, "erase start flag failed");
                                        }
                                        blockNumber = 0;//download finish clear fota range
#ifdef WITH_FOTA_RESUME
                                        c2f_encode_fotarange_params();
#endif
                                        runThread=0;
                                    }
                                }

                                break;
                            }

                            default:
                                break;
                        }
                    } /* Request or Response */
                }
            }

        }
        else
        {
            if(resendCount == 0)
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_8, P_INFO, 0, "runThread ended");
                
                if(fotaStateManagement.fotaState == FOTA_STATE_DOWNLOADING){
                    ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_8_2, P_INFO, 0, "no recv platform's ACK, download fail.");
#ifdef WITH_FOTA_RESUME
                    ctiot_funcv1_set_boot_flag(NULL, BOOT_FOTA_BREAK_RESUME);
                    slpManFlushUsrNVMem();
                    ctiot_funcv1_context_t* pContext = ctiot_funcv1_get_context();
                    pContext->inBreakFOTA = 0;
#else
                    fotaStateManagement.fotaResult = FOTA_RESULT_DISCONNECT;
                    ctiot_funcv1_fota_state_changed();
#endif                        
                    ctiot_funcv1_status_t wakeStatus[1]={0};
                    wakeStatus->baseInfo=ct_lwm2m_strdup(FOTA1);
#ifndef FEATURE_REF_AT_ENABLE
                    ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, wakeStatus, 0);
                    ct_lwm2m_free(wakeStatus->baseInfo);
#else       
                    ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_HSSTATUS, wakeStatus, 0);
#endif
                    ctiot_funcv1_notify_event(MODULE_LWM2M, FOTA_DOWNLOAD_FAIL, NULL, 0);
                }
                runThread=0;
            }
            else
            {
                if(fotaStateManagement.fotaState == FOTA_STATE_DOWNLOADING){
                    send_resquest(blockNumber,START_MORE,BLOCK_SIZE);
                    ECOMM_TRACE(UNILOG_CTLWM2M, firmware_fota_9, P_INFO, 1, "!!!!!!!!!!!!!!!resend packet!!!!!!!!!!!!!!!!resendCount=%d",resendCount);
                    resendCount--;
                }else if(fotaStateManagement.fotaState == FOTA_STATE_UPDATING){
                    runThread=0;
                }
            }
        }
    }

    /*
        if(fotaStateManagement.fotaState = FOTA_STATE_DOWNLOADING && (fotaStateManagement.fotaResult == FOTA_RESULT_DISCONNECT || fotaStateManagement.fotaResult == FOTA_RESULT_CHECKFAIL))
        {
            ReDownloadtimes++;
            if(ReDownloadtimes <= MAX_DOWNLOAD_TIMES)
                goto ReDownload;
        }
    */
    if(query != NULL){
        ct_lwm2m_free(query);
    }
    
    close(sockfd);
    sockfd = -1;
    thread_exit(NULL);
}

bool fota_create_socket(char * host,int port)
{
    bool result = true;
    uint32_t hwwdr = 1;
    /* 建立socket，注意必须是SOCK_DGRAM */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        result = false;
    }

    /* 填写sockaddr_in*/
    memset(&addr, 0x00, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(fotaUrl.port);
    addr.sin_addr.s_addr = inet_addr(fotaUrl.address);

    // Disable UDP high level delay feature
    ioctl(sockfd, FIOHWODR, &hwwdr);
    return result;
}

int fota_start(char* url)
{
    int err = 0;
    bool result = true;
    thread_handle_t tid;

    memset(&fotaUrl,0,sizeof(fotaUrl));
    result = ctiot_funcv1_parse_url(url,&fotaUrl);
    if(result == false)
    {
        fotaStateManagement.fotaState = FOTA_STATE_IDIL;
        fotaStateManagement.fotaResult = FOTA_RESULT_URIINVALID;
        ctiot_funcv1_fota_state_changed();
        ctiot_funcv1_status_t wakeStatus[1]={0};
        wakeStatus->baseInfo=ct_lwm2m_strdup(FOTA1);
#ifndef FEATURE_REF_AT_ENABLE
        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, wakeStatus, 0);
        ct_lwm2m_free(wakeStatus->baseInfo);
#else       
        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_HSSTATUS, wakeStatus, 0);
#endif
        ctiot_funcv1_notify_event(MODULE_LWM2M, FOTA_DOWNLOAD_FAIL, NULL, 0);
        goto exit;
    }
    if(strcmp(fotaUrl.protocol,"coap") != 0)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, fota_start_0, P_INFO, 0, "fota_start protocol is not coap");
        fotaStateManagement.fotaState = FOTA_STATE_IDIL;
        fotaStateManagement.fotaResult = FOTA_RESULT_PROTOCOLFAIL;
        ctiot_funcv1_fota_state_changed();
        ctiot_funcv1_status_t wakeStatus[1]={0};
        wakeStatus->baseInfo=ct_lwm2m_strdup(FOTA1);
#ifndef FEATURE_REF_AT_ENABLE
        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_STATUS, wakeStatus, 0);
        ct_lwm2m_free(wakeStatus->baseInfo);
#else       
        ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_HSSTATUS, wakeStatus, 0);
#endif
        ctiot_funcv1_notify_event(MODULE_LWM2M, FOTA_DOWNLOAD_FAIL, NULL, 0);
        goto exit;
    }
    
#ifdef WITH_FOTA_RESUME
    //save download url to nv
    c2f_encode_fotaurl_params();
    c2f_encode_fotarange_params();
#endif
    //create socket
    if(fota_create_socket(fotaUrl.address,fotaUrl.port) == false)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, fota_start_1, P_ERROR, 0, "failed to create socket");
        result = false;
        goto exit;
    }

    //create thread
    err = thread_create(&tid, NULL, firmware_fota, NULL, LWM2M_FOTA_DOWNLOAD_TASK);
    if (err != 0)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, fota_start_3, P_INFO, 0, "can't create thread");
        result = false;
        goto exit;
    }

exit:
    return result;
}

int fota_resume()
{
    int err = 0;
    bool result = true;
    thread_handle_t tid;

    //create socket
    if(fota_create_socket(fotaUrl.address,fotaUrl.port) == false)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, fota_resume_0, P_ERROR, 0, "failed to create socket");
        result = false;
        goto exit;
    }

    ECOMM_TRACE(UNILOG_CTLWM2M, fota_resume_1, P_INFO, 0, "to start thread: firmware_fota");
    //create thread
    err = thread_create(&tid, NULL, firmware_fota, NULL, LWM2M_FOTA_DOWNLOAD_TASK);
    if (err != 0)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, fota_resume_2, P_INFO, 0, "can't create thread");
        result = false;
        goto exit;
    }

exit:
    return result;
}

void notify_fotastate_con(lwm2m_transaction_t* transacP, void * message)
{
    coap_packet_t * packet = (coap_packet_t *)message;
    ctiot_funcv1_status_t atStatus[1]={0};
    ctiot_funcv1_context_t* pTmpContext=ctiot_funcv1_get_context();
    //coap_free_payload(transacP->message);
    if(packet == NULL)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, notify_fotastate_con_1, P_INFO, 0, "msg has no recv ack");
        return;
    }
    if(packet->type==COAP_TYPE_ACK)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, notify_fotastate_con_2, P_INFO, 0, "con msg has recv ack wait server's update cmd");
    }
    else if(packet->type==COAP_TYPE_RST)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, notify_fotastate_con_3, P_INFO, 0, "recv rst cmd!");
    }
}

#endif

