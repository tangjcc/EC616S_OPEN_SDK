#include <cis_sock.h>
#include <cis_if_net.h>
#include <cis_list.h>
#include <cis_log.h>
#include <cis_if_sys.h>
#include <cis_internals.h>
#include "netmgr.h"
#include "osasys.h"


#define MAX_PACKET_SIZE         (1080)
static NmNetifStatus cisNbStatus = NM_NETIF_ACTIVATED;

//const CHAR *defaultCisLocalPort = "49250";

//TaskHandle_t recv_taskhandle;
//TaskHandle_t attach_taskhandle;
//TaskHandle_t connect_taskhandle;

void callbackRecvThread(void* arg);
void callbackAttachThread(void* arg);
void callbackConnectThread(void* arg);


//////////////////////////////////////////////////////////////////////////
static int prvCreateSocket(const char * portStr, int ai_family)
{
    int s = -1;
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (0 != getaddrinfo(NULL, portStr, &hints, &res))
    {
        return -1;
    }

    for(p = res ; p != NULL && s == -1 ; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0)
        {
            if (-1 == bind(s, p->ai_addr, p->ai_addrlen))
            {
                close(s);
                s = -1;
            }
        }
    }

    freeaddrinfo(res);

    return s;
}


void cisnet_setNBStatus(uint8_t netStatus)
{
	cisNbStatus = (NmNetifStatus)netStatus;
    ECOMM_TRACE(UNILOG_ONENET, cisnet_setNBStatus_0, P_INFO, 1, "onenet netStatus = %d", netStatus);
}

bool  cisnet_attached_state(void * ctx)
{
	if(cisNbStatus == NM_NETIF_ACTIVATED)
		return true;
	else
		return false;
}

cis_ret_t cisnet_init(void *context,const cisnet_config_t* config,cisnet_callback_t cb)
{

    net_Init();
    LOGI("fall in cisnet_init\r\n");
    memcpy(&((struct st_cis_context *)context)->netConfig,config,sizeof(cisnet_config_t));
    ((struct st_cis_context *)context)->netCallback.onEvent = cb.onEvent;
    //cissys_sleepms(1000);
    return CIS_RET_OK;
}

cis_ret_t cisnet_create(cisnet_t* netctx,const char* host,void* context)
{
     st_context_t * ctx = ( struct st_cis_context *)context;
     
    if (cisnet_attached_state(NULL) != true)return CIS_RET_ERROR;
    (*netctx) = (cisnet_t)cis_malloc(sizeof(struct st_cisnet_context));
    (*netctx)->sock = 0;
    (*netctx)->port = atoi((const char *)(ctx->serverPort));
    (*netctx)->state = 0;
    (*netctx)->quit = 0;
    (*netctx)->g_packetlist=NULL;
    (*netctx)->context = ctx;
    (*netctx)->recv_taskhandle = NULL;//EC add
    strncpy((*netctx)->host,host,sizeof((*netctx)->host));
    strncpy((*netctx)->localPort,ctx->localPort,sizeof((*netctx)->localPort));//EC add
    (*netctx)->packetlistMutex = osMutexNew(NULL);

    return CIS_RET_OK;
}

void     cisnet_destroy(cisnet_t netctx)
{
    net_Close(netctx->sock);
    ECOMM_TRACE(UNILOG_ONENET, cisnet_destroy_1, P_INFO, 0, "stop recv_task");
    if (netctx->recv_taskhandle != NULL) {
        //EC add begin
        //vTaskDelete(recv_taskhandle);
        netctx->quit = 1;
        //EC add end
        netctx->recv_taskhandle = NULL;
    }
}


cis_ret_t cisnet_connect(cisnet_t netctx)
{
    int sock = -1;
    sock = prvCreateSocket(netctx->localPort,AF_INET);
    if (sock < 0)
    {
        LOGD("Failed to open socket: %d %s\r\n", errno, strerror(errno));
        free(netctx);
        ECOMM_TRACE(UNILOG_ONENET, cisnet_connect_0_1, P_ERROR, 1, "Failed to open socket,errno=%d free sessionH",errno);
        return CIS_RET_ERROR;
    }
    netctx->sock = sock;
    netctx->state = 1;
   ((struct st_cis_context *)(netctx->context))->netCallback.onEvent(netctx,cisnet_event_connected,NULL,netctx->context);
    if (netctx->recv_taskhandle != NULL) {
        ECOMM_TRACE(UNILOG_ONENET, cisnet_connect_0_2, P_ERROR, 0, "cisnet_connect already has recvThread");
        return CIS_RET_OK;
    }
    ECOMM_TRACE(UNILOG_ONENET, cisnet_connect_1, P_SIG, 0, "cisnet_connect start recvThread");

    xTaskCreate(callbackRecvThread,
                "cis_recv",
                2048 / sizeof(portSTACK_TYPE),
                netctx,
                osPriorityBelowNormal7,
                &(netctx->recv_taskhandle));

    return CIS_RET_OK;
}

cis_ret_t cisnet_disconnect(cisnet_t netctx)
{
    netctx->state = 0;
   ((struct st_cis_context *)(netctx->context))->netCallback.onEvent(netctx,cisnet_event_disconnect,NULL,netctx->context);
    return 1;
}

cis_ret_t cisnet_write(cisnet_t netctx,const uint8_t * buffer,uint32_t  length, uint8_t raiflag)
{
    int nbSent;
    size_t addrlen;
    size_t offset;
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(struct sockaddr_in));//EC add
    saddr.sin_len = sizeof(struct sockaddr_in);//EC add
    saddr.sin_family = AF_INET; 
    saddr.sin_port = htons(netctx->port);
    saddr.sin_addr.s_addr = inet_addr(netctx->host);
    
    addrlen = sizeof(saddr);
    offset = 0;
    ECOMM_DUMP(UNILOG_ONENET, cisnet_write_1, P_INFO, "send data:",4, (uint8_t*)buffer);
    while (offset != length)
    {
        nbSent = ps_sendto(netctx->sock, (const char*)buffer + offset, length - offset, 0,
                            (struct sockaddr *)&saddr, addrlen, raiflag, false);

        ECOMM_TRACE(UNILOG_ONENET, cisnet_write_2, P_INFO, 3, "(%x)%d has sent(raiflag=%d)",netctx->context,nbSent, raiflag);
        if (nbSent == -1){
            LOGE("socket sendto [%s:%d] failed.",netctx->host,ntohs(saddr.sin_port));
            return -1;
        }else{
            LOGI("socket sendto [%s:%d] %d bytes",netctx->host,ntohs(saddr.sin_port),nbSent);
        }
        offset += nbSent;
    }

    return CIS_RET_OK;
}

cis_ret_t cisnet_read(cisnet_t netctx,uint8_t** buffer,uint32_t *length)
{
    if(netctx->g_packetlist != NULL){
        struct st_net_packet* delNode;
        *buffer = netctx->g_packetlist->buffer;
        *length = netctx->g_packetlist->length;
         delNode =netctx->g_packetlist;
         netctx->g_packetlist = netctx->g_packetlist->next;
         cis_free(delNode);
         return CIS_RET_OK;
    }

    return CIS_RET_ERROR;
}

cis_ret_t cisnet_read_ex(cisnet_t netctx,uint8_t** buffer,uint32_t *length)//EC add
{
	osMutexAcquire(netctx->packetlistMutex, osWaitForever);
	struct st_net_packet* node;
	node = netctx->g_packetlist;
	
    if(node != NULL){
		ECOMM_TRACE(UNILOG_ONENET, cisnet_read_ex_1, P_INFO, 0, "netctx->g_packetlist has node");
        *buffer = node->buffer;
        *length = node->length;
        netctx->g_packetlist = netctx->g_packetlist->next;
    	cis_free(node);
    }else{
		osMutexRelease(netctx->packetlistMutex);
		return CIS_RET_ERROR;
   	}

    if(netctx->g_packetlist != NULL){
		ECOMM_TRACE(UNILOG_ONENET, cisnet_read_ex_2, P_INFO, 0, "netctx->g_packetlist still has node");
		osMutexRelease(netctx->packetlistMutex);
        return CIS_RET_CONTINUE;
	}else{
		osMutexRelease(netctx->packetlistMutex);
        return CIS_RET_EMPTY;
	}
}

cis_ret_t cisnet_free(cisnet_t netctx,uint8_t* buffer,uint32_t length)
{
    ECOMM_TRACE(UNILOG_ONENET, cisnet_free_1, P_INFO, 1, "buffer: 0x%x", buffer);
    cis_free(buffer);
    return CIS_RET_ERROR;
}

void callbackAttachThread(void* arg)
{
}

void callbackConnectThread(void* arg)
{
}


void callbackRecvThread(void* arg)
{
    
    cisnet_t netctx = (cisnet_t)arg;
    int sock = netctx->sock;
    struct st_net_packet* templist;
    ECOMM_TRACE(UNILOG_ONENET, callbackRecvThread_1, P_INFO, 1, "callbackRecvThread start---quit=%d",netctx->quit);
    while(0 == netctx->quit && netctx->state == 1)
    {
        struct timeval tv = {5,0};
        fd_set readfds;
        int result;

        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        /*
            * This part will set up an interruption until an event happen on SDTIN or the socket until "tv" timed out (set
            * with the precedent function)
            */
        result = select(FD_SETSIZE, &readfds, NULL, NULL, &tv);

        if (result < 0)
        {
            LOGE("Error in select(): %d %s", errno, strerror(errno));
            ECOMM_TRACE(UNILOG_ONENET, callbackRecvThread_2, P_INFO, 1, "Error in select(): %d",errno);
            goto TAG_END;
        }
        else if (result > 0)
        {
            uint8_t buffer[MAX_PACKET_SIZE];
            int numBytes;

            /*
                * If an event happens on the socket
                */
            if (FD_ISSET(sock, &readfds))
            {
                struct sockaddr_storage addr;
                socklen_t addrLen;

                addrLen = sizeof(addr);

                /*
                    * We retrieve the data received
                    */
                numBytes = recvfrom(sock, (char*)buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrLen);

                if (numBytes < 0)
                {
                    if(errno == 0)continue;
                    ECOMM_TRACE(UNILOG_ONENET, callbackRecvThread_3, P_INFO, 1, "Error in recvfrom(): %d",errno);
                    LOGE("Error in recvfrom(): %d %s", errno, strerror(errno));
                }
                else if (numBytes > 0)
                {
                    ECOMM_TRACE(UNILOG_ONENET, callbackRecvThread_4, P_INFO, 1, "recv %d",numBytes);
                    char s[INET_ADDRSTRLEN];
                    //uint16_t  port;  comments for eliminate warning
            
                    struct sockaddr_in *saddr = (struct sockaddr_in *)&addr;
                    inet_ntop(saddr->sin_family, &saddr->sin_addr, s, INET_ADDRSTRLEN);
                    //port = saddr->sin_port;

                    //LOGI("%d bytes received from [%s]:%hu", numBytes, s, ntohs(port));
                    
                    uint8_t* data = (uint8_t*)cis_malloc(numBytes);
                    cis_memcpy(data,buffer,numBytes);

                    struct st_net_packet *packet = (struct st_net_packet*)cis_malloc(sizeof(struct st_net_packet));
                    packet->next = NULL;
                    packet->buffer = data;
                    packet->length = numBytes;
                    //packet->netctx = netctx;
					osMutexAcquire(netctx->packetlistMutex, osWaitForever);
                    if(netctx->g_packetlist == NULL)
                    {
                        ECOMM_TRACE(UNILOG_ONENET, callbackRecvThread_4_1, P_INFO, 1, "packetlist is NULL add to head(0x%x)", data);
                        netctx->g_packetlist = packet;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ONENET, callbackRecvThread_4_2, P_INFO, 1, "packetlist not NULL add to tail(0x%x)", data);
                        templist = netctx->g_packetlist;
                        while (NULL != templist->next)
                        {
                            templist = templist->next;
                        }
                        templist->next = packet;
                    }
			    	osMutexRelease(netctx->packetlistMutex);
                }
            }
        }
    }
TAG_END:
    ECOMM_TRACE(UNILOG_ONENET, callbackRecvThread, P_INFO, 1, "callbackRecvThread exit.user control:%d",netctx->quit);
    
    struct st_net_packet* node;
    node = netctx->g_packetlist;
    while(node != NULL)
    {
        ECOMM_TRACE(UNILOG_ONENET, callbackRecvThread_5, P_INFO, 0, "free the packet still in g_packetlist");
        if(node->buffer != NULL)
        {
            cis_free(node->buffer);
        }
        netctx->g_packetlist = netctx->g_packetlist->next;
        cis_free(node);
        node = netctx->g_packetlist;
    }
    netctx->recv_taskhandle = NULL;
    osMutexDelete(netctx->packetlistMutex);
    cissys_free(netctx);
    vTaskDelete(NULL);
}
