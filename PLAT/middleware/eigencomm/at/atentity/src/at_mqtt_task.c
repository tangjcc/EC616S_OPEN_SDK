/*
* OVERALL STRUCTURE
*                                                                    "mqtt socket connect ok"  "mqtt connect ok"
*  .-----------------.      .--------------.      .---------------.      .--------------.      .--------------.
*  | mqtt task start | ---> | mqtt config  | ---> | net/mqtt init | ---> | net connect  | ---> | mqtt connect |
*  `-----------------'      `--------------'      `---------------'      `--------------'      `--------------'
*                                                                                                      |
*                                                                                                      v
*                    .--------------.      .---------------.      .--------------------.      .----------------.
*                    | mqtt publish | <--- | mqtt recv msg | <--- | start mqtt_run task| <--- | mqtt topic sub |
*                    `--------------'      `---------------'      `--------------------'      `----------------'
*                                                                            |
*                                                                            v
*                                                          .----------------------------------.
*                                                          | while(1)                         |
*                                                          |   readPacket()                   | "mqtt try recv onenet Packet"
*                                                          |   check packet_type              |
*                                                          |      case:connack/puback/suback  | "mqtt recv timeout"
*                                                          |           publish/pubrec/pubcomp | "mqtt recv publish packet"
*                                                          |           pubrel                 | "mqtt recv pubrec/L packet"
*                                                          |           pingresp               | "mqtt recv keeplive ack Packet ok"
*                                                          |   check if send keeplive packet  |
*                                                          |      send keeplive packet        | "mqtt send keeplive Packet ok"
*                                                          `----------------------------------'
*/
#include "lwip/opt.h"
#include <stdio.h>
#include <string.h>
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "at_mqtt_task.h"
#include "debug_trace.h"
#include "FreeRTOS.h"
#include "cJSON.h"
#ifdef FEATURE_MBEDTLS_ENABLE
#include "sha1.h"
#include "sha256.h"
#include "md5.h"
#endif
#include "ec_mqtt_api.h"
#include "atec_mqtt.h"
#include "netmgr.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif
/*******************************************************************************
 * topic define
 ******************************************************************************/
#define EC_TOPIC_LENGTH                     128
#define EC_BUFF_LENGTH                      128
#ifdef FEATURE_MQTT_TLS_ENABLE        
#define MQTT_RECV_TASK_STACK_SIZE   6600
#define MQTT_SEND_TASK_STACK_SIZE   6600
#else
#define MQTT_RECV_TASK_STACK_SIZE   1800
#define MQTT_SEND_TASK_STACK_SIZE   1800
#endif
/*******************************************************************************
 * Variables
 ******************************************************************************/
#define ALI_RANDOM_TIMESTAMP   "20130219"

int keepalive_retry_time = 0;
int mqtt_task_recv_status_flag = MQTT_TASK_DELETE;
int mqtt_task_send_status_flag = MQTT_TASK_DELETE;
int mqtt_sempr_status_flag = MQTT_SEMPHR_NOT_CREATE;

uint8_t mqttSlpHandler = 0xff;

QueueHandle_t mqtt_send_msg_handle = NULL;
osThreadId_t mqtt_recv_task_handle = NULL;
osThreadId_t mqtt_send_task_handle = NULL;
Mutex mqtt_mutex1;

mqtt_context mqttContext[MQTT_CONTEXT_NUMB_MAX]={0};
mqtt_context *mqttCurrContext = NULL;

extern int ali_hmac_falg;
extern osThreadId_t mqtt_recv_task_handle;
extern  NBStatus_t appGetNetInfoSync(UINT32 cid, NmAtiSyncRet *result);
mqtt_context *getCurrMqttContext(void);

void mqttDefaultMessageArrived(MessageData* data)
{
    int primSize;
    mqtt_context *mqttContext = getCurrMqttContext();
    mqtt_message mqtt_msg;
    int mqtt_topic_len = 0;
    int mqtt_payload_len = 0;

    ECOMM_STRING(UNILOG_MQTT, mqtt_task_0, P_SIG, ".........MQTT_messageArrived is:%s", data->message->payload);
    memset(&mqtt_msg, 0, sizeof(mqtt_msg));
    mqtt_msg.tcp_id = mqttContext->tcp_id;
    mqtt_msg.msg_id = data->message->id;

    if(data->topicName->lenstring.data != NULL)
    {
        mqtt_topic_len = data->topicName->lenstring.len+1;
        mqtt_msg.mqtt_topic = malloc(mqtt_topic_len);
        memset(mqtt_msg.mqtt_topic, 0, mqtt_topic_len);
        memcpy(mqtt_msg.mqtt_topic, data->topicName->lenstring.data, (mqtt_topic_len-1));
        mqtt_msg.mqtt_topic_len = data->topicName->lenstring.len;
        memset(data->topicName->lenstring.data, 0, (mqtt_topic_len-1));
    }

    if(data->message->payload != NULL)
    {
        mqtt_payload_len = data->message->payloadlen+1;
        mqtt_msg.mqtt_payload = malloc(mqtt_payload_len);
        memset(mqtt_msg.mqtt_payload, 0, mqtt_payload_len);
        memcpy(mqtt_msg.mqtt_payload, data->message->payload, (mqtt_payload_len-1));
        mqtt_msg.mqtt_payload_len = data->message->payloadlen;
        memset(data->message->payload, 0, (mqtt_payload_len-1));
    }

    primSize = sizeof(mqtt_msg);

    applSendCmsInd(mqttContext->reqHandle, APPL_MQTT, APPL_MQTT_RECV_IND, primSize, (void *)&mqtt_msg);
}

int mqttDecodePacket(mqtt_context *mqttContext, int* value, int timeout)
{
    unsigned char i;
    int multiplier = 1;
    int len = 0;
    const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

    
    #ifdef FEATURE_MQTT_TLS_ENABLE
    *value = 0;
    do
    {
        int rc = MQTTPACKET_READ_ERROR;

        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
        {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            goto exit;
        }
        rc = mqttSslRead(mqttContext->mqtts_client, &i, 1, timeout);
        if (rc != 1)
            goto exit;
        *value += (i & 127) * multiplier;
        multiplier *= 128;
    } while ((i & 128) != 0);
    #else
    *value = 0;
    do
    {
        int rc = MQTTPACKET_READ_ERROR;

        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
        {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            goto exit;
        }
        rc = mqttContext->mqtt_client->ipstack->mqttread(mqttContext->mqtt_client->ipstack, &i, 1, timeout);       
        if (rc != 1)
            goto exit;
        *value += (i & 127) * multiplier;
        multiplier *= 128;
    } while ((i & 128) != 0);
    #endif

exit:
    return len;
}

int mqttGetNextPacketId(MQTTClient *c) {
    return c->next_packetid = (c->next_packetid == MAX_PACKET_ID) ? 1 : c->next_packetid + 1;
}

int mqttReadPacket(mqtt_context *mqttContext, Timer* timer)
{
    MQTTHeader header = {0};
    int len = 0;
    int rem_len = 0;
    int rc = 0;

    ECOMM_TRACE(UNILOG_MQTT, mqtt_task_read, P_INFO, 0, "...mqttReadPacket..");

    #ifdef FEATURE_MQTT_TLS_ENABLE
    if(mqttContext->is_mqtts != MQTT_SSL_NONE)
    {
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_tls0, P_INFO, 0, "...mqttReadPacket..0.");
        /* 1. read the header byte.  This has the packet type in it */
        rc = mqttSslRead(mqttContext->mqtts_client, mqttContext->mqtts_client->readBuf, 1, TimerLeftMS(timer));
        if (rc != 1)
        {
            goto exit;
        }
    
        len = 1;
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_tls1, P_INFO, 0, "...mqttReadPacket..1.");
        /* 2. read the remaining length.  This is variable in itself */
        mqttDecodePacket(mqttContext, &rem_len, TimerLeftMS(timer));
        len += MQTTPacket_encode(mqttContext->mqtts_client->readBuf + 1, rem_len); /* put the original remaining length back into the buffer */
    
        if (rem_len > (mqttContext->mqtts_client->readBufSize - len))
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_task_tls202, P_INFO, 0, "...mqttReadPacket..202.");
            rc = BUFFER_OVERFLOW;
            goto exit;
        }

        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_tls2, P_INFO, 0, "...mqttReadPacket..2.");
        /* 3. read the rest of the buffer using a callback to supply the rest of the data */
        if (rem_len > 0 && (mqttSslRead(mqttContext->mqtts_client, mqttContext->mqtts_client->readBuf + len, rem_len, TimerLeftMS(timer)) != rem_len)) 
        {
            rc = 0;
            ECOMM_TRACE(UNILOG_MQTT, mqtt_task_tls200, P_INFO, 0, "...mqttReadPacket..200.");
            goto exit;
        }
    
        header.byte = mqttContext->mqtts_client->readBuf[0];
        rc = header.bits.type;
        if (mqttContext->mqtt_client->keepAliveInterval > 0)
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_task_tls201, P_INFO, 0, "...mqttReadPacket..201.");
            TimerCountdown(&mqttContext->mqtt_client->last_received, mqttContext->mqtt_client->keepAliveInterval); // record the fact that we have successfully received a packet
        }
    }
    else
    #endif
    {
        /* 1. read the header byte.  This has the packet type in it */
        rc = mqttContext->mqtt_client->ipstack->mqttread(mqttContext->mqtt_client->ipstack, mqttContext->mqtt_client->readbuf, 1, TimerLeftMS(timer));
        if (rc != 1)
            goto exit;

        len = 1;
        /* 2. read the remaining length.  This is variable in itself */
        mqttDecodePacket(mqttContext, &rem_len, TimerLeftMS(timer));
        len += MQTTPacket_encode(mqttContext->mqtt_client->readbuf + 1, rem_len); /* put the original remaining length back into the buffer */

        if (rem_len > (mqttContext->mqtt_client->readbuf_size - len))
        {
            rc = BUFFER_OVERFLOW;
            goto exit;
        }

        /* 3. read the rest of the buffer using a callback to supply the rest of the data */
        if (rem_len > 0 && (mqttContext->mqtt_client->ipstack->mqttread(mqttContext->mqtt_client->ipstack, mqttContext->mqtt_client->readbuf + len, rem_len, TimerLeftMS(timer)) != rem_len)) {
            rc = 0;
            goto exit;
        }

        header.byte = mqttContext->mqtt_client->readbuf[0];
        rc = header.bits.type;
        if (mqttContext->mqtt_client->keepAliveInterval > 0)
        {
            TimerCountdown(&mqttContext->mqtt_client->last_received, mqttContext->mqtt_client->keepAliveInterval); // record the fact that we have successfully received a packet
        }
    }
    exit:
        return rc;
}

int mqttSendPacket(mqtt_context *mqttContext, int length, Timer* timer, int rai, bool exceptdata)
{
    int rc = FAILURE,
        sent = 0;

    #ifdef FEATURE_MQTT_TLS_ENABLE
    if(mqttContext->is_mqtts != MQTT_SSL_NONE)
    {
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_tls3, P_INFO, 0, "...mqttSendPacket..0.");
        mqttContext->mqtts_client->ssl->sslContext.rai = rai;
        rc = mqttSslSend(mqttContext->mqtts_client, &mqttContext->mqtts_client->sendBuf[sent], length);
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_tls4, P_INFO, 1, "...mqttSendPacket..=%d.", rc);
        TimerCountdown(&mqttContext->mqtt_client->last_sent, mqttContext->mqtt_client->keepAliveInterval); // record the fact that we have successfully sent the packet
        
        return rc;
    }
    else
    #endif
    {
        while (sent < length && !TimerIsExpired(timer))
        {
            #ifdef MQTT_RAI_OPTIMIZE
            rc = mqttContext->mqtt_client->ipstack->mqttwrite(mqttContext->mqtt_client->ipstack, &mqttContext->mqtt_client->buf[sent], length, TimerLeftMS(timer), rai, exceptdata);
            #else
            rc = mqttContext->mqtt_client->ipstack->mqttwrite(mqttContext->mqtt_client->ipstack, &mqttContext->mqtt_client->buf[sent], length, TimerLeftMS(timer));
            #endif
            
            if (rc < 0)  // there was an error writing the data
                break;
            sent += rc;
        }
        if (sent == length)
        {
            TimerCountdown(&mqttContext->mqtt_client->last_sent, mqttContext->mqtt_client->keepAliveInterval); // record the fact that we have successfully sent the packet
            rc = SUCCESS;
        }
        else
            rc = FAILURE;
        return rc;
    }
}


int mqttWaitForAck(mqtt_context *mqttContext, int packet_type, Timer* timer)
{
    int rc = FAILURE;

    do
    {
        if (TimerIsExpired(timer))
            break; // we timed out
        rc = mqttCycle(mqttContext, timer);
    }
    while (rc != packet_type && rc >= 0);

    return rc;
}


void mqttClientInit(mqtt_context *mqttContext, Network* network, unsigned int command_timeout_ms,
        unsigned char* sendbuf, size_t sendbuf_size, unsigned char* readbuf, size_t readbuf_size)
{
    int i;
    
    mqttContext->mqtt_client->ipstack = network;

    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
        mqttContext->mqtt_client->messageHandlers[i].topicFilter = 0;
    mqttContext->mqtt_client->command_timeout_ms = command_timeout_ms;
    mqttContext->mqtt_client->buf = sendbuf;
    mqttContext->mqtt_client->buf_size = sendbuf_size;
    mqttContext->mqtt_client->readbuf = readbuf;
    mqttContext->mqtt_client->readbuf_size = readbuf_size;
    mqttContext->mqtt_client->isconnected = 0;
    mqttContext->mqtt_client->cleansession = 0;
    mqttContext->mqtt_client->ping_outstanding = 0;
    mqttContext->mqtt_client->defaultMessageHandler = mqttDefaultMessageArrived;
    mqttContext->mqtt_client->next_packetid = 1;
    TimerInit(&mqttContext->mqtt_client->last_sent);
    TimerInit(&mqttContext->mqtt_client->last_received);

    #ifdef FEATURE_MQTT_TLS_ENABLE
    mqttContext->mqtts_client->sendBuf = sendbuf;
    mqttContext->mqtts_client->sendBufSize = sendbuf_size;
    mqttContext->mqtts_client->readBuf = readbuf;
    mqttContext->mqtts_client->readBufSize = readbuf_size;
    #endif    
#if defined(MQTT_TASK)
      MutexInit(&mqttContext->mqtt_client->mutex);
#endif
}

int mqttConnectWithResults(mqtt_context *mqttContext, MQTTPacket_connectData* options, MQTTConnackData* data)
{
    Timer connect_timer;
    int rc = FAILURE;
    MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
    int len = 0;

#if defined(MQTT_TASK)
      MutexLock(&mqttContext->mqtt_client->mutex);
#endif
      if (mqttContext->mqtt_client->isconnected) /* don't send connect packet again if we are already connected */
          goto exit;

    TimerInit(&connect_timer);
    TimerCountdownMS(&connect_timer, mqttContext->mqtt_client->command_timeout_ms);

    if (options == 0)
        options = &default_options; /* set default options if none were supplied */

    mqttContext->mqtt_client->keepAliveInterval = options->keepAliveInterval;
    mqttContext->mqtt_client->cleansession = options->cleansession;
    TimerCountdown(&mqttContext->mqtt_client->last_received, mqttContext->mqtt_client->keepAliveInterval);
    if ((len = MQTTSerialize_connect(mqttContext->mqtt_client->buf, mqttContext->mqtt_client->buf_size, options)) <= 0)
        goto exit;
    if ((rc = mqttSendPacket(mqttContext, len, &connect_timer, 0, false)) != SUCCESS)  // send the connect packet
        goto exit; // there was a problem
    #if 0
    // this will be a blocking call, wait for the connack
    if (mqttWaitForAck(mqttContext, CONNACK, &connect_timer) == CONNACK)
    {
        data->rc = 0;
        data->sessionPresent = 0;
        if (MQTTDeserialize_connack(&data->sessionPresent, &data->rc, mqttContext->mqtt_client->readbuf, mqttContext->mqtt_client->readbuf_size) == 1)
            rc = data->rc;
        else
            rc = FAILURE;
    }
    else
        rc = FAILURE;
    #endif
exit:
    if (rc == SUCCESS)
    {
        mqttContext->mqtt_client->isconnected = 1;
        mqttContext->mqtt_client->ping_outstanding = 0;
    }

#if defined(MQTT_TASK)
      MutexUnlock(&mqttContext->mqtt_client->mutex);
#endif

    return rc;
}

int mqttConnect(mqtt_context *mqttContext, MQTTPacket_connectData* options)
{
    MQTTConnackData data;
    return mqttConnectWithResults(mqttContext, options, &data);
}

int mqttReconnectTcp(void *context)
{
    int rc = -1;
    mqtt_context *mqttContext = (mqtt_context *)context;
    int socket_stat = -1;
    int socket_err = -1;

    socket_stat = sock_get_errno(mqttContext->mqtt_client->ipstack->my_socket);
    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_89, P_INFO, 1, "...get socket stat..socket_stat=%d..", socket_stat);

    {
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_90, P_INFO, 1, "...start tcp disconnect ..");
            rc = mqttContext->mqtt_client->ipstack->disconnect(mqttContext->mqtt_client->ipstack);
            if(rc != 0)
            {
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_91, P_INFO, 1, "...tcp disconnect fail!!!.rc=%d..", rc);
            }
        }

        {
            socket_err = socket_error_is_fatal(socket_stat);
            //if(socket_err == 1)
            {
                //ec_socket_delete_flag = 0xff;
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_92, P_INFO, 1, "...tcp disconnect ok!!!....%d.",socket_err);

                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_93, P_INFO, 1, "...start tcp connect ...");
                if ((rc = NetworkSetConnTimeout(mqttContext->mqtt_client->ipstack, MQTT_SEND_TIMEOUT, MQTT_RECV_TIMEOUT)) != 0)
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_94, P_INFO, 1, "...tcp socket set timeout fail...");
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_95, P_INFO, 1, "...tcp connect ok...");
                    mqttContext->mqtt_client->isconnected = 0;
                    if((rc = NetworkConnect(mqttContext->mqtt_client->ipstack, mqttContext->mqtt_uri, mqttContext->port)) < 0)
                    {
                        ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_96, P_INFO, 1, "...tcp reconnect fail!!!...\r\n");
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_97, P_INFO, 1, "...start mqtt connect ..");
                    }
                }
            }
        }
    }

    return rc;
}

int mqttReconnect(void *context)
{
    int rc = -1;
    mqtt_context *mqttContext = (mqtt_context *)context;
    int socket_stat = -1;

    socket_stat = sock_get_errno(mqttContext->mqtt_client->ipstack->my_socket);
    ECOMM_TRACE(UNILOG_MQTT, mqtt_task_1, P_INFO, 1, "...get socket stat..socket_stat=%d..", socket_stat);

    {
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_task_2, P_INFO, 1, "...start tcp disconnect ..");
            rc = mqttContext->mqtt_client->ipstack->disconnect(mqttContext->mqtt_client->ipstack);
            if(rc != 0)
            {
                ECOMM_TRACE(UNILOG_MQTT, mqtt_task_3, P_INFO, 1, "...tcp disconnect fail!!!.rc=%d..", rc);
            }
        }
        //vTaskDelay(10000);
        {
            //socket_stat = sock_get_errno(c->ipstack->my_socket);
            //ECOMM_TRACE(UNILOG_MQTT, mqtt_task_10, P_INFO, 1, "...get socket stat..socket_stat=%d..", socket_stat);
            //socket_err = socket_error_is_fatal(socket_stat);
            //if(socket_err == 1)
            {
                //ec_socket_delete_flag = 0xff;
                ECOMM_TRACE(UNILOG_MQTT, mqtt_task_11, P_INFO, 1, "...tcp disconnect ok!!!...");

                ECOMM_TRACE(UNILOG_MQTT, mqtt_task_12, P_INFO, 1, "...start tcp connect ...");
                if ((rc = NetworkSetConnTimeout(mqttContext->mqtt_client->ipstack, MQTT_SEND_TIMEOUT, MQTT_RECV_TIMEOUT)) != 0)
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_task_13, P_INFO, 1, "...tcp socket set timeout fail...");
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_task_14, P_INFO, 1, "...tcp connect ok...");
                    mqttContext->mqtt_client->isconnected = 0;
                    if((rc = NetworkConnect(mqttContext->mqtt_client->ipstack, mqttContext->mqtt_uri, mqttContext->port)) < 0)
                    {
                        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_15, P_INFO, 1, "...tcp reconnect fail!!!...\r\n");
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_16, P_INFO, 1, "...start mqtt connect ..");

                        if ((rc = mqttConnect(mqttContext, &mqttContext->mqtt_connect_data)) != 0)
                        {
                            ECOMM_TRACE(UNILOG_MQTT, mqtt_task_17, P_INFO, 1, "...mqtt reconnect fial!!!...");
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_MQTT, mqtt_task_18, P_INFO, 1, "...mqtt reconnect ok!!!...");
                        }
                    }
                }
            }
        }
    }

    return rc;
}

int mqttSetMessageHandler(MQTTClient* c, const char* topicFilter, messageHandler messageHandler)
{
    int rc = FAILURE;
    int i = -1;

    /* first check for an existing matching slot */
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        if (c->messageHandlers[i].topicFilter != NULL && strcmp(c->messageHandlers[i].topicFilter, topicFilter) == 0)
        {
            if (messageHandler == NULL) /* remove existing */
            {
                c->messageHandlers[i].topicFilter = NULL;
                c->messageHandlers[i].fp = NULL;
            }
            rc = SUCCESS; /* return i when adding new subscription */
            break;
        }
    }
    /* if no existing, look for empty slot (unless we are removing) */
    if (messageHandler != NULL) {
        if (rc == FAILURE)
        {
            for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
            {
                if (c->messageHandlers[i].topicFilter == NULL)
                {
                    rc = SUCCESS;
                    break;
                }
            }
        }
        if (i < MAX_MESSAGE_HANDLERS)
        {
            c->messageHandlers[i].topicFilter = topicFilter;
            c->messageHandlers[i].fp = messageHandler;
        }
    }
    return rc;
}


int mqttSubscribeWithResults(mqtt_context *mqttContext, int msgId, const char* topicFilter, enum QoS qos,
       messageHandler messageHandler, MQTTSubackData* data)
{
    int rc = FAILURE;
    Timer timer;
    int len = 0;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicFilter;

#if defined(MQTT_TASK)
      MutexLock(&mqttContext->mqtt_client->mutex);
#endif
      if (!mqttContext->mqtt_client->isconnected)
            goto exit;

    TimerInit(&timer);
    TimerCountdownMS(&timer, mqttContext->mqtt_client->command_timeout_ms);

    //len = MQTTSerialize_subscribe(c->buf, c->buf_size, 0, mqttGetNextPacketId(c), 1, &topic, (int*)&qos);
    len = MQTTSerialize_subscribe(mqttContext->mqtt_client->buf, mqttContext->mqtt_client->buf_size, 0, msgId, 1, &topic, (int*)&qos);    //modify for at cmd
    if (len <= 0)
        goto exit;
    if ((rc = mqttSendPacket(mqttContext, len, &timer, 0, false)) != SUCCESS) // send the subscribe packet
        goto exit;             // there was a problem
#if 0
    if (mqttWaitForAck(c, SUBACK, &timer) == SUBACK)      // wait for suback
    {
        int count = 0;
        unsigned short mypacketid;
        data->grantedQoS = QOS0;
        if (MQTTDeserialize_suback(&mypacketid, 1, &count, (int*)&data->grantedQoS, c->readbuf, c->readbuf_size) == 1)
        {
            if (data->grantedQoS != 0x80)
                rc = MQTTSetMessageHandler(c, topicFilter, messageHandler);
        }
    }
    else
        rc = FAILURE;
#endif
exit:
    if (rc == FAILURE)
        ;//MQTTCloseSession(c);
#if defined(MQTT_TASK)
      MutexUnlock(&mqttContext->mqtt_client->mutex);
#endif
    return rc;
}


int mqttSubscribe(mqtt_context *mqttContext, int msgId, const char* topicFilter, enum QoS qos,
       messageHandler messageHandler)
{
    MQTTSubackData data;
    int rc;
    rc = mqttSubscribeWithResults(mqttContext, msgId, topicFilter, qos, messageHandler, &data);
    if(rc == SUCCESS)
    {
        rc = MQTT_OK;
    }
    return rc;
}


int mqttUnsubscribe(mqtt_context *mqttContext, int msgID, const char* topicFilter)
{
    int rc = FAILURE;
    Timer timer;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicFilter;
    int len = 0;

#if defined(MQTT_TASK)
      MutexLock(&mqttContext->mqtt_client->mutex);
#endif
      if (!mqttContext->mqtt_client->isconnected)
          goto exit;

    TimerInit(&timer);
    TimerCountdownMS(&timer, mqttContext->mqtt_client->command_timeout_ms);

    //if ((len = MQTTSerialize_unsubscribe(mqttContext->mqtt_client->buf, mqttContext->mqtt_client->buf_size, 0, mqttGetNextPacketId(mqttContext->mqtt_client), 1, &topic)) <= 0)
    if ((len = MQTTSerialize_unsubscribe(mqttContext->mqtt_client->buf, mqttContext->mqtt_client->buf_size, 0, msgID, 1, &topic)) <= 0)
        goto exit;
    if ((rc = mqttSendPacket(mqttContext, len, &timer, 0, false)) != SUCCESS) // send the subscribe packet
        goto exit; // there was a problem
    #if 0
    if (mqttWaitForAck(mqttContext, UNSUBACK, &timer) == UNSUBACK)
    {
        unsigned short mypacketid;  // should be the same as the packetid above
        if (MQTTDeserialize_unsuback(&mypacketid, mqttContext->mqtt_client->readbuf, mqttContext->mqtt_client->readbuf_size) == 1)
        {
            /* remove the subscription message handler associated with this topic, if there is one */
            MQTTSetMessageHandler(mqttContext->mqtt_client, topicFilter, NULL);
        }
    }
    else
        rc = FAILURE;
    #endif
exit:
    if (rc == FAILURE)
        ;
        //mqttCloseSession(c);
#if defined(MQTT_TASK)
      MutexUnlock(&mqttContext->mqtt_client->mutex);
#endif
    if(rc == SUCCESS)
    {
        rc = MQTT_OK;
    }

    return rc;
}


int mqttPublish(mqtt_context *mqttContext, int msgID, const char* topicName, MQTTMessage* message, int rai, bool exceptdata)
{
    int rc = FAILURE;
    Timer timer;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicName;
    int len = 0;
    
      ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_88, P_INFO, 1, "..mqttPublish.len.%d",message->payloadlen);

#if defined(MQTT_TASK)
      MutexLock(&mqttContext->mqtt_client->mutex);
#endif
      if (!mqttContext->mqtt_client->isconnected)
            goto exit;

    TimerInit(&timer);
    TimerCountdownMS(&timer, mqttContext->mqtt_client->command_timeout_ms);

    if (message->qos == QOS1 || message->qos == QOS2)
        //message->id = mqttGetNextPacketId(c);
        message->id = msgID;

    len = MQTTSerialize_publish(mqttContext->mqtt_client->buf, mqttContext->mqtt_client->buf_size, 0, message->qos, message->retained, message->id,
              topic, (unsigned char*)message->payload, message->payloadlen);
    if (len <= 0)
        goto exit;
   
    if ((rc = mqttSendPacket(mqttContext, len, &timer, rai, exceptdata)) != SUCCESS) // send the subscribe packet
        goto exit; // there was a problem

#if 0
    if (message->qos == QOS1)
    {
        if (mqttWaitForAck(c, PUBACK, &timer) == PUBACK)
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
                rc = FAILURE;
        }
        else
            rc = FAILURE;
    }
    else if (message->qos == QOS2)
    {
        if (mqttWaitForAck(c, PUBCOMP, &timer) == PUBCOMP)
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
                rc = FAILURE;
        }
        else
            rc = FAILURE;
    }
#endif
exit:
    if (rc == FAILURE)
        ;//MQTTCloseSession(c);
#if defined(MQTT_TASK)
      MutexUnlock(&mqttContext->mqtt_client->mutex);
#endif
    if(rc == SUCCESS)
    {
        rc = MQTT_OK;
    }

    return rc;
}



int mqttClose(mqtt_context* mqttContext)
{
    mqtt_context* mqttNewContext = mqttContext;

    mqttNewContext->mqtt_network->disconnect(mqttNewContext->mqtt_network);

    return MQTT_OK;
}


int mqttDisconnect(mqtt_context *mqttContext)
{
    int rc = MQTT_OK;
    Timer timer;     // we might wait for incomplete incoming publishes to complete
    int len = 0;

#if defined(MQTT_TASK)
    MutexLock(&mqttContext->mqtt_client->mutex);
#endif
    TimerInit(&timer);
    TimerCountdownMS(&timer, mqttContext->mqtt_client->command_timeout_ms);

      len = MQTTSerialize_disconnect(mqttContext->mqtt_client->buf, mqttContext->mqtt_client->buf_size);
    if (len > 0)
        rc = mqttSendPacket(mqttContext, len, &timer, 0, false);            // send the disconnect packet
    //mqttCloseSession(c);

#if defined(MQTT_TASK)
      MutexUnlock(&mqttContext->mqtt_client->mutex);
#endif
    if(rc == SUCCESS)
    {
        rc = MQTT_OK;
    }
    return rc;
}
int mqttKeepalive(mqtt_context *mqttContext)
{
    int rc = SUCCESS;

    if (mqttContext->mqtt_client->keepAliveInterval == 0)
        goto exit;

    if (TimerIsExpired(&mqttContext->mqtt_client->last_sent) || TimerIsExpired(&mqttContext->mqtt_client->last_received))
    {
        if (mqttContext->mqtt_client->ping_outstanding)
            rc = FAILURE; /* PINGRESP not received in keepalive interval */
        else
        {
            Timer timer;
            TimerInit(&timer);
            TimerCountdownMS(&timer, 1000);
            int len = MQTTSerialize_pingreq(mqttContext->mqtt_client->buf, mqttContext->mqtt_client->buf_size);
            if (len > 0 && (rc = mqttSendPacket(mqttContext, len, &timer, 0, false)) == SUCCESS) // send the ping packet
                mqttContext->mqtt_client->ping_outstanding = 1;
        }
    }

exit:
    return rc;
}

static void mqttNewMessageData(MessageData* md, MQTTString* aTopicName, MQTTMessage* aMessage) {
    md->topicName = aTopicName;
    md->message = aMessage;
}

static char mqttIsTopicMatched(char* topicFilter, MQTTString* topicName)
{
    char* curf = topicFilter;
    char* curn = topicName->lenstring.data;
    char* curn_end = curn + topicName->lenstring.len;

    while (*curf && curn < curn_end)
    {
        if (*curn == '/' && *curf != '/')
            break;
        if (*curf != '+' && *curf != '#' && *curf != *curn)
            break;
        if (*curf == '+')
        {   // skip until we meet the next separator, or end of string
            char* nextpos = curn + 1;
            while (nextpos < curn_end && *nextpos != '/')
                nextpos = ++curn + 1;
        }
        else if (*curf == '#')
            curn = curn_end - 1;    // skip until end of string
        curf++;
        curn++;
    };

    return (curn == curn_end) && (*curf == '\0');
}

int mqttDeliverMessage(MQTTClient* c, MQTTString* topicName, MQTTMessage* message)
{
    int i;
    int rc = FAILURE;

    // we have to find the right message handler - indexed by topic
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        if (c->messageHandlers[i].topicFilter != 0 && (MQTTPacket_equals(topicName, (char*)c->messageHandlers[i].topicFilter) ||
                mqttIsTopicMatched((char*)c->messageHandlers[i].topicFilter, topicName)))
        {
            if (c->messageHandlers[i].fp != NULL)
            {
                MessageData md;
                mqttNewMessageData(&md, topicName, message);
                c->messageHandlers[i].fp(&md);
                rc = SUCCESS;
            }
        }
    }

    if (rc == FAILURE && c->defaultMessageHandler != NULL)
    {
        MessageData md;
        mqttNewMessageData(&md, topicName, message);
        c->defaultMessageHandler(&md);
        rc = SUCCESS;
    }

    return rc;
}

#define MQTT_CLIENT_AT_API

mqtt_context *getCurrMqttContext(void)
{
    return mqttCurrContext;
}
void mqttMessageArrived(mqtt_context *mqttContext, MessageData* data)
{
    int primSize;
    mqtt_message mqtt_msg;
    int mqtt_topic_len;
    int mqtt_payload_len;

    ECOMM_STRING(UNILOG_MQTT, mqtt_task_19, P_SIG, ".........MQTT_messageArrived is:%s", data->message->payload);

    mqtt_msg.mqtt_id = mqttContext->mqtt_id;

    mqtt_topic_len = strlen(data->topicName->cstring)+1;
    mqtt_payload_len = strlen(data->message->payload)+1;

    mqtt_msg.mqtt_topic = malloc(mqtt_topic_len);
    mqtt_msg.mqtt_payload = malloc(mqtt_payload_len);
    memset(mqtt_msg.mqtt_topic, 0, mqtt_topic_len);
    memset(mqtt_msg.mqtt_payload, 0, mqtt_payload_len);

    memcpy(mqtt_msg.mqtt_topic, data->topicName->cstring, (mqtt_topic_len-1));
    memcpy(mqtt_msg.mqtt_payload, data->message->payload, (mqtt_payload_len-1));

    primSize = sizeof(mqtt_msg);
    applSendCmsInd(mqttContext->reqHandle, APPL_MQTT, APPL_MQTT_RECV_IND, primSize, (void *)&mqtt_msg);
}
void mqttMessageHandler0(MessageData* data)
{
    mqtt_context *mqttContext = getCurrMqttContext();

    if(mqttContext->is_used == MQTT_CONTEXT_USED)
    {
        mqttMessageArrived(mqttContext, data);

    }
}

void mqttMessageHandler1(MessageData* data)
{
    mqtt_context *mqttContext = getCurrMqttContext();

    if(mqttContext->is_used == MQTT_CONTEXT_USED)
    {
        mqttMessageArrived(mqttContext, data);

    }
}

void mqttMessageHandler2(MessageData* data)
{
    mqtt_context *mqttContext = getCurrMqttContext();

    if(mqttContext->is_used == MQTT_CONTEXT_USED)
    {
        mqttMessageArrived(mqttContext, data);

    }
}

void mqttMessageHandler3(MessageData* data)
{
    mqtt_context *mqttContext = getCurrMqttContext();

    if(mqttContext->is_used == MQTT_CONTEXT_USED)
    {
        mqttMessageArrived(mqttContext, data);

    }
}

void mqttMessageHandler4(MessageData* data)
{
    mqtt_context *mqttContext = getCurrMqttContext();

    if(mqttContext->is_used == MQTT_CONTEXT_USED)
    {
        mqttMessageArrived(mqttContext, data);

    }
}

void *mqttMessageHandler[MQTT_CONTEXT_NUMB_MAX] =
{
    mqttMessageHandler0,
    //mqttMessageHandler1,
    //mqttMessageHandler2,
    //mqttMessageHandler3,
};

mqtt_context *mqttCreateContext(int tcpId, char *mqttUri, int mqttPort, int txBufLen, int rxBufLen, int mode)
{
    int i = 0;

    for(i=0; i<MQTT_CONTEXT_NUMB_MAX; i++)
    {
        if((mqttContext[i].is_used == MQTT_CONTEXT_USED)||(mqttContext[i].is_used == MQTT_CONTEXT_CONFIGED)||(mqttContext[i].is_used == MQTT_CONTEXT_OPENED))
        {
            if(tcpId == mqttContext[i].tcp_id)
            {
                return &mqttContext[i];
            }
        }
    }

    for(i=0; i<MQTT_CONTEXT_NUMB_MAX; i++)
    {
        if(mqttContext[i].is_used == MQTT_CONTEXT_NOT_USED)
        {
            mqttContext[i].mqtt_id    = MQTT_ID_DEFAULT;
            mqttContext[i].tcp_id     = MQTT_TCP_ID_DEFAULT;
            mqttContext[i].cloud_type = MQTT_CLOUD_DEFAULT;
        }
    }

    for(i=0; i<MQTT_CONTEXT_NUMB_MAX; i++)
    {
        if(mqttContext[i].is_used == MQTT_CONTEXT_NOT_USED)
        {
            memset(&mqttContext[i], 0, sizeof(mqtt_context));

            mqttContext[i].mqtt_network = malloc(sizeof(Network));          // mqtt_network
            if(mqttContext[i].mqtt_network != NULL)
            {
                memset(mqttContext[i].mqtt_network, 0, sizeof(Network));
            }
            else
            {

                if(mqttContext[i].mqtt_uri != NULL)
                {
                    free(mqttContext[i].mqtt_uri);
                    mqttContext[i].mqtt_uri = NULL;
                }
                return NULL;
            }

            mqttContext[i].mqtt_client = malloc(sizeof(MQTTClient));        // mqtt_client
            if(mqttContext[i].mqtt_client != NULL)
            {
                memset(mqttContext[i].mqtt_client, 0, sizeof(MQTTClient));
                mqttContext[i].mqtt_client->command_timeout_ms = MQTT_CMD_TIMEOUT_DEFAULT;
                mqttContext[i].mqtt_client->keepAliveInterval = MQTT_KEEPALIVE_DEFAULT;

            }
            else
            {
                if(mqttContext[i].mqtt_uri != NULL)
                {
                    free(mqttContext[i].mqtt_uri);
                    mqttContext[i].mqtt_uri = NULL;
                }
                if(mqttContext[i].mqtt_network != NULL)
                {
                    free(mqttContext[i].mqtt_network);
                    mqttContext[i].mqtt_network = NULL;
                }
                return NULL;
            }
            
            #ifdef FEATURE_MQTT_TLS_ENABLE            
            mqttContext[i].mqtts_client = malloc(sizeof(mqttsClientContext));        // mqtt_client
            if(mqttContext[i].mqtts_client != NULL)
            {
                memset(mqttContext[i].mqtts_client, 0, sizeof(mqttsClientContext));
                mqttContext[i].mqtts_client->timeout_s= MQTT_CMD_TIMEOUT_DEFAULT;
                mqttContext[i].mqtts_client->timeout_r = MQTT_CMD_TIMEOUT_DEFAULT;
            }
            #endif
            
            if((txBufLen != 0)&&(rxBufLen != 0))
            {
                mqttContext[i].mqtt_send_buf = malloc(txBufLen+1);          // mqtt_send_buf
                if(mqttContext[i].mqtt_send_buf != NULL)
                {
                    mqttContext[i].mqtt_send_buf_len = txBufLen+1;
                    memset(mqttContext[i].mqtt_send_buf, 0, txBufLen+1);
                }
                else
                {
                    if(mqttContext[i].mqtt_uri != NULL)
                    {
                        free(mqttContext[i].mqtt_uri);
                        mqttContext[i].mqtt_uri = NULL;
                    }
                    if(mqttContext[i].mqtt_network != NULL)
                    {
                        free(mqttContext[i].mqtt_network);
                        mqttContext[i].mqtt_network = NULL;
                    }
                    if(mqttContext[i].mqtt_client != NULL)
                    {
                        free(mqttContext[i].mqtt_client);
                        mqttContext[i].mqtt_client = NULL;
                    }
                    return NULL;
                }

                mqttContext[i].mqtt_read_buf = malloc(rxBufLen+1);          // mqtt_read_buf
                if(mqttContext[i].mqtt_read_buf != NULL)
                {
                    mqttContext[i].mqtt_read_buf_len = rxBufLen+1;
                    memset(mqttContext[i].mqtt_read_buf, 0, rxBufLen+1);
                }
                else
                {
                    if(mqttContext[i].mqtt_uri != NULL)
                    {
                        free(mqttContext[i].mqtt_uri);
                        mqttContext[i].mqtt_uri = NULL;
                    }
                    if(mqttContext[i].mqtt_network != NULL)
                    {
                        free(mqttContext[i].mqtt_network);
                        mqttContext[i].mqtt_network = NULL;
                    }
                    if(mqttContext[i].mqtt_client != NULL)
                    {
                        free(mqttContext[i].mqtt_client);
                        mqttContext[i].mqtt_client = NULL;
                    }
                    if(mqttContext[i].mqtt_send_buf != NULL)
                    {
                        free(mqttContext[i].mqtt_send_buf);
                        mqttContext[i].mqtt_send_buf = NULL;
                    }
                    return NULL;
                }
            }
            else
            {
                if(mqttContext[i].mqtt_uri != NULL)
                {
                    free(mqttContext[i].mqtt_uri);
                    mqttContext[i].mqtt_uri = NULL;
                }
                if(mqttContext[i].mqtt_network != NULL)
                {
                    free(mqttContext[i].mqtt_network);
                    mqttContext[i].mqtt_network = NULL;
                }
                if(mqttContext[i].mqtt_client != NULL)
                {
                    free(mqttContext[i].mqtt_client);
                    mqttContext[i].mqtt_client = NULL;
                }
                return NULL;
            }

            MQTTPacket_connectData mqttConnectData = MQTTPacket_connectData_initializer;
            memcpy((void *)(&mqttContext[i].mqtt_connect_data), (void *)&mqttConnectData, sizeof(MQTTPacket_connectData));
            mqttContext[i].mqtt_connect_data.MQTTVersion = 4;
            mqttContext[i].mqtt_connect_data.cleansession = 1;
            mqttContext[i].mqtt_connect_data.willFlag = 0;
            mqttContext[i].mqtt_connect_data.keepAliveInterval = MQTT_KEEPALIVE_DEFAULT;

            mqttContext[i].mqtt_msg_handler = (messageHandler)mqttMessageHandler[i];
            mqttContext[i].is_used = mode;
            mqttContext[i].mqtt_id = i;
            mqttContext[i].tcp_id = tcpId;
            mqttContext[i].port = mqttPort;
            mqttContext[i].send_data_format = MQTT_DATA_FORMAT_HEX;
            mqttContext[i].reconnect_count = 0;
            mqttContext[i].cloud_type = MQTT_CLOUD_DEFAULT;
            mqttContext[i].payloadType = MQTT_DATA_DEFAULT;

            return &mqttContext[i];
        }
    }
    return NULL;
}

int mqttDeleteContext(mqtt_context *mqttContext)
{
    int ret = MQTT_ERR;
    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_98, P_INFO, 0, ".....mqttDeleteContext client.......");

    mqttContext->is_used = MQTT_CONTEXT_NOT_USED;
    mqttContext->is_connected = MQTT_CONN_DEFAULT;
    mqttContext->mqtt_id = MQTT_ID_DEFAULT;
    mqttContext->tcp_id = MQTT_TCP_ID_DEFAULT;

    if(mqttContext->mqtt_uri != NULL)
    {
        free(mqttContext->mqtt_uri);
        mqttContext->mqtt_uri = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->mqtt_send_buf != NULL)
    {
        free(mqttContext->mqtt_send_buf);
        mqttContext->mqtt_send_buf = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->mqtt_read_buf != NULL)
    {
        free(mqttContext->mqtt_read_buf);
        mqttContext->mqtt_read_buf = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->mqtt_network != NULL)
    {
        free(mqttContext->mqtt_network);
        mqttContext->mqtt_network = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->mqtt_client != NULL)
    {
        free(mqttContext->mqtt_client);
        mqttContext->mqtt_client = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->mqtt_connect_data.clientID.cstring != NULL)
    {
        free(mqttContext->mqtt_connect_data.clientID.cstring);
        mqttContext->mqtt_connect_data.clientID.cstring = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->mqtt_connect_data.username.cstring != NULL)
    {
        free(mqttContext->mqtt_connect_data.username.cstring);
        mqttContext->mqtt_connect_data.username.cstring = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->mqtt_connect_data.password.cstring != NULL)
    {
        free(mqttContext->mqtt_connect_data.password.cstring);
        mqttContext->mqtt_connect_data.password.cstring = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->mqtt_connect_data.will.topicName.cstring != NULL)
    {
        free(mqttContext->mqtt_connect_data.will.topicName.cstring);
        mqttContext->mqtt_connect_data.will.topicName.cstring = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->mqtt_connect_data.will.message.cstring != NULL)
    {
        free(mqttContext->mqtt_connect_data.will.message.cstring);
        mqttContext->mqtt_connect_data.will.message.cstring = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->aliAuth.product_name != NULL)
    {
        free(mqttContext->aliAuth.product_name);
        mqttContext->aliAuth.product_name = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->aliAuth.product_key != NULL)
    {
        free(mqttContext->aliAuth.product_key);
        mqttContext->aliAuth.product_key = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->aliAuth.product_secret != NULL)
    {
        free(mqttContext->aliAuth.product_secret);
        mqttContext->aliAuth.product_secret = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->aliAuth.device_name != NULL)
    {
        free(mqttContext->aliAuth.device_name);
        mqttContext->aliAuth.device_name = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->aliAuth.device_secret != NULL)
    {
        free(mqttContext->aliAuth.device_secret);
        mqttContext->aliAuth.device_secret = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->aliAuth.device_token != NULL)
    {
        free(mqttContext->aliAuth.device_token);
        mqttContext->aliAuth.device_token = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->aliAuth.auth_type != NULL)
    {
        free(mqttContext->aliAuth.auth_type);
        mqttContext->aliAuth.auth_type = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->aliAuth.sign_method != NULL)
    {
        free(mqttContext->aliAuth.sign_method);
        mqttContext->aliAuth.sign_method = NULL;
        ret = MQTT_OK;
    }

    if(mqttContext->aliAuth.auth_mode != NULL)
    {
        free(mqttContext->aliAuth.auth_mode);
        mqttContext->aliAuth.auth_mode = NULL;
        ret = MQTT_OK;
    }
    
    if(mqttContext->aliAuth.secure_mode != NULL)
    {
        free(mqttContext->aliAuth.secure_mode);
        mqttContext->aliAuth.secure_mode = NULL;
        ret = MQTT_OK;
    }
    
    if(mqttContext->aliAuth.instance_id != NULL)
    {
        free(mqttContext->aliAuth.instance_id);
        mqttContext->aliAuth.instance_id = NULL;
        ret = MQTT_OK;
    }
    
    if(mqttContext->aliAuth.client_id != NULL)
    {
        free(mqttContext->aliAuth.client_id);
        mqttContext->aliAuth.client_id = NULL;
        ret = MQTT_OK;
    }    
    mqttContext->aliAuth.dynamic_register_used = 0;

    #ifdef FEATURE_MQTT_TLS_ENABLE
    if(mqttContext->mqtts_client->psk_key != NULL)
    {
        free(mqttContext->mqtts_client->psk_key);
        mqttContext->mqtts_client->psk_key = NULL;
        ret = MQTT_OK;
    }
    if(mqttContext->mqtts_client->psk_identity != NULL)
    {
        free(mqttContext->mqtts_client->psk_identity);
        mqttContext->mqtts_client->psk_identity = NULL;
        ret = MQTT_OK;
    }
    #endif    
    if(mqttContext->ecc_key != NULL)
    {
        free(mqttContext->ecc_key);
        mqttContext->ecc_key = NULL;
        ret = MQTT_OK;
    }
    
    if(mqttContext->ca_key != NULL)
    {
        free(mqttContext->ca_key);
        mqttContext->ca_key = NULL;
        ret = MQTT_OK;
    }
    
    mqttContext->cloud_type = MQTT_CLOUD_DEFAULT;
    mqttContext->payloadType = MQTT_DATA_DEFAULT;
    mqttContext->reconnect_count = 0;

    return ret;
}

mqtt_context *mqttFindContext(int tcpId)
{
    int i = 0;
    /* case1: mqttid + coap tag*/
    /* case2: NULL(def is 0xff) + mqtt tag*/
    /* case3: mqttid + NULL(def is NULL)*/

    if(tcpId != MQTT_TCP_ID_DEFAULT)
    {
        for(i=0; i<MQTT_CONTEXT_NUMB_MAX; i++)
        {
            if((mqttContext[i].is_used == MQTT_CONTEXT_USED)||(mqttContext[i].is_used == MQTT_CONTEXT_CONFIGED)||(mqttContext[i].is_used == MQTT_CONTEXT_OPENED))
            {
                if(tcpId == mqttContext[i].tcp_id)
                {
                    return &mqttContext[i];
                }
            }
        }
    }

    return NULL;
}


int mqttConfigContext(int cnfType, int tcpId, mqtt_cfg_data *cfgData)
{
    int i = 0;
    int strLen1 = 0;
    int strLen2 = 0;
    mqtt_context *mqttCurrContext = NULL;
    int aliPskLen = 64;
    int aliPskSignLen = 0;
    int aliPskIdentityLen = 0;
    char *ali_psk = NULL;
    char ali_psk_hex[34] = {0};
    char *ali_psk_sign = NULL;
    char *ali_psk_identity = NULL;

    for(i=0; i<MQTT_CONTEXT_NUMB_MAX; i++)
    {
        if((mqttContext[i].is_used == MQTT_CONTEXT_USED)||(mqttContext[i].is_used == MQTT_CONTEXT_CONFIGED)||(mqttContext[i].is_used == MQTT_CONTEXT_OPENED))
        {
            if(tcpId == mqttContext[i].tcp_id)
            {
                mqttCurrContext = &mqttContext[i];
            }
        }
    }
    if(mqttCurrContext != NULL)
    {
        switch(cnfType)
        {
            case MQTT_CONFIG_ECHOMODE:
                mqttCurrContext->echomode = cfgData->decParam1;
                break;

            case MQTT_CONFIG_DATAFORMAT:
                mqttCurrContext->send_data_format = cfgData->decParam1;
                mqttCurrContext->recv_data_format = cfgData->decParam2;
                break;

            case MQTT_CONFIG_KEEPALIVE:
                mqttCurrContext->keepalive = cfgData->decParam1;
                mqttCurrContext->mqtt_connect_data.keepAliveInterval = cfgData->decParam1;
                break;

            case MQTT_CONFIG_SEESION:
                mqttCurrContext->session = cfgData->decParam1;
                mqttCurrContext->mqtt_connect_data.cleansession = cfgData->decParam1;
                break;

            case MQTT_CONFIG_TIMEOUT:
                mqttCurrContext->timeout = cfgData->decParam1;
                if((0<cfgData->decParam1)&&(cfgData->decParam1<61))
                {
                    mqttCurrContext->pkt_timeout = cfgData->decParam1;//pkt_timeout d
                    mqttCurrContext->retry_time= cfgData->decParam2;//retry time
                    mqttCurrContext->timeout_notice= cfgData->decParam3;//timeout notice                 
                }
                else
                {
                    return MQTT_ERR;
                }
                break;

            case MQTT_CONFIG_WILL:
                mqttCurrContext->mqtt_connect_data.willFlag = cfgData->decParam1;
                mqttCurrContext->mqtt_connect_data.will.qos = cfgData->decParam2;
                mqttCurrContext->mqtt_connect_data.will.retained = cfgData->decParam3;
                if((cfgData->strParam1!=NULL)&&(cfgData->strParam2!=NULL))
                {
                    strLen1 = strlen(cfgData->strParam1); /*topic*/
                    strLen2 = strlen(cfgData->strParam2); /*message*/

                    mqttCurrContext->mqtt_connect_data.will.topicName.cstring = malloc(strLen1+1);
                    mqttCurrContext->mqtt_connect_data.will.message.cstring = malloc(strLen2+1);

                    memset(mqttCurrContext->mqtt_connect_data.will.topicName.cstring, 0, (strLen1+1));
                    memset(mqttCurrContext->mqtt_connect_data.will.message.cstring, 0, (strLen2+1));

                    memcpy(mqttCurrContext->mqtt_connect_data.will.topicName.cstring, cfgData->strParam1, strLen1);
                    memcpy(mqttCurrContext->mqtt_connect_data.will.message.cstring, cfgData->strParam2, strLen2);
                }
                else
                {
                    return MQTT_ERR;
                }
                break;

            case MQTT_CONFIG_VERSION:
                if((cfgData->decParam1 == 3)||(cfgData->decParam1 == 4))
                {
                    mqttCurrContext->version = cfgData->decParam1;
                    mqttCurrContext->mqtt_connect_data.MQTTVersion = cfgData->decParam1;
                }
                else
                {
                    return MQTT_ERR;
                }
                break;

            case MQTT_CONFIG_ALIAUTH:
                if(cfgData->strParam1 != NULL)
                {
                    if(mqttCurrContext->aliAuth.product_key != NULL)
                    {
                        free(mqttCurrContext->aliAuth.product_key);
                        mqttCurrContext->aliAuth.product_key = NULL;
                    }
                    strLen1 = strlen(cfgData->strParam1);
                    mqttCurrContext->aliAuth.product_key = malloc(strLen1+1);
                    memset(mqttCurrContext->aliAuth.product_key, 0, (strLen1+1));
                    memcpy(mqttCurrContext->aliAuth.product_key, cfgData->strParam1, strLen1);
                }
                if(cfgData->strParam2 != NULL)
                {
                    if(mqttCurrContext->aliAuth.device_name != NULL)
                    {
                        free(mqttCurrContext->aliAuth.device_name);
                        mqttCurrContext->aliAuth.device_name = NULL;
                    }
                    strLen1 = strlen(cfgData->strParam2);
                    mqttCurrContext->aliAuth.device_name = malloc(strLen1+1);
                    memset(mqttCurrContext->aliAuth.device_name, 0, (strLen1+1));
                    memcpy(mqttCurrContext->aliAuth.device_name, cfgData->strParam2, strLen1);
                }
                if(cfgData->strParam3 != NULL)
                {
                    if(mqttCurrContext->aliAuth.device_secret != NULL)
                    {
                        free(mqttCurrContext->aliAuth.device_secret);
                        mqttCurrContext->aliAuth.device_secret = NULL;
                    }
                    strLen1 = strlen(cfgData->strParam3);
                    mqttCurrContext->aliAuth.device_secret = malloc(strLen1+1);
                    memset(mqttCurrContext->aliAuth.device_secret, 0, (strLen1+1));
                    memcpy(mqttCurrContext->aliAuth.device_secret, cfgData->strParam3, strLen1);
                }
                if(cfgData->strParam4 != NULL)
                {
                    if(mqttCurrContext->aliAuth.product_name != NULL)
                    {
                        free(mqttCurrContext->aliAuth.product_name);
                        mqttCurrContext->aliAuth.product_name = NULL;
                    }
                    strLen1 = strlen(cfgData->strParam4);
                    mqttCurrContext->aliAuth.product_name = malloc(strLen1+1);
                    memset(mqttCurrContext->aliAuth.product_name, 0, (strLen1+1));
                    memcpy(mqttCurrContext->aliAuth.product_name, cfgData->strParam4, strLen1);
                }
                if(cfgData->strParam5 != NULL)
                {
                    if(mqttCurrContext->aliAuth.product_secret != NULL)
                    {
                        free(mqttCurrContext->aliAuth.product_secret);
                        mqttCurrContext->aliAuth.product_secret = NULL;
                    }
                    strLen1 = strlen(cfgData->strParam5);
                    mqttCurrContext->aliAuth.product_secret = malloc(strLen1+1);
                    memset(mqttCurrContext->aliAuth.product_secret, 0, (strLen1+1));
                    memcpy(mqttCurrContext->aliAuth.product_secret, cfgData->strParam5, strLen1);
                }
                if(cfgData->strParam6 != NULL)     /*auth_type: register, regnwl*/  
                {
                    if(mqttCurrContext->aliAuth.auth_type != NULL)
                    {
                        free(mqttCurrContext->aliAuth.auth_type);
                        mqttCurrContext->aliAuth.auth_type = NULL;
                    }
                    strLen1 = strlen(cfgData->strParam6);
                    mqttCurrContext->aliAuth.auth_type = malloc(strLen1+1);
                    memset(mqttCurrContext->aliAuth.auth_type, 0, (strLen1+1));
                    memcpy(mqttCurrContext->aliAuth.auth_type, cfgData->strParam6, strLen1);
                }
                if(cfgData->strParam7 != NULL)     /*sign_method: hmac_sha1, hmac_sha256, hmac_md5*/  
                {
                    if(mqttCurrContext->aliAuth.sign_method != NULL)
                    {
                        free(mqttCurrContext->aliAuth.sign_method);
                        mqttCurrContext->aliAuth.sign_method = NULL;
                    }
                    strLen1 = strlen(cfgData->strParam7);
                    mqttCurrContext->aliAuth.sign_method = malloc(strLen1+1);
                    memset(mqttCurrContext->aliAuth.sign_method, 0, (strLen1+1));
                    memcpy(mqttCurrContext->aliAuth.sign_method, cfgData->strParam7, strLen1);
                }
                if(cfgData->strParam8 != NULL)     /*auth_mode: tls-psk, tls-ca*/
                {
                    if(mqttCurrContext->aliAuth.auth_mode != NULL)
                    {
                        free(mqttCurrContext->aliAuth.auth_mode);
                        mqttCurrContext->aliAuth.auth_mode = NULL;
                    }
                    strLen1 = strlen(cfgData->strParam8);
                    mqttCurrContext->aliAuth.auth_mode = malloc(strLen1+1);
                    memset(mqttCurrContext->aliAuth.auth_mode, 0, (strLen1+1));
                    memcpy(mqttCurrContext->aliAuth.auth_mode, cfgData->strParam8, strLen1);
                }
                if(cfgData->strParam9 != NULL)     /*secure_mode: -2, 2*/
                {
                    if(mqttCurrContext->aliAuth.secure_mode != NULL)
                    {
                        free(mqttCurrContext->aliAuth.secure_mode);
                        mqttCurrContext->aliAuth.secure_mode = NULL;
                    }
                    strLen1 = strlen(cfgData->strParam9);
                    mqttCurrContext->aliAuth.secure_mode = malloc(strLen1+1);
                    memset(mqttCurrContext->aliAuth.secure_mode, 0, (strLen1+1));
                    memcpy(mqttCurrContext->aliAuth.secure_mode, cfgData->strParam9, strLen1);
                }
                if(cfgData->strParam10 != NULL)     /*instance_id:  */
                {
                    if(mqttCurrContext->aliAuth.instance_id != NULL)
                    {
                        free(mqttCurrContext->aliAuth.instance_id);
                        mqttCurrContext->aliAuth.instance_id = NULL;
                    }
                    strLen1 = strlen(cfgData->strParam10);
                    mqttCurrContext->aliAuth.instance_id = malloc(strLen1+1);
                    memset(mqttCurrContext->aliAuth.instance_id, 0, (strLen1+1));
                    memcpy(mqttCurrContext->aliAuth.instance_id, cfgData->strParam10, strLen1);
                }
                
                mqttCurrContext->cloud_type = CLOUD_TYPE_ALI;
                mqttCurrContext->aliAuth.dynamic_register_used = cfgData->decParam1;

                if((mqttCurrContext->aliAuth.auth_type != NULL)&&(mqttCurrContext->aliAuth.sign_method != NULL)&&(mqttCurrContext->aliAuth.product_key != NULL)
                    &&(mqttCurrContext->aliAuth.device_name != NULL)&&(mqttCurrContext->aliAuth.device_secret != NULL))
                {
                    /*=======for psk and psk_identity===================================*/
                    aliPskIdentityLen = strlen(mqttCurrContext->aliAuth.auth_type)+strlen(mqttCurrContext->aliAuth.sign_method)+strlen(mqttCurrContext->aliAuth.product_key)+strlen(mqttCurrContext->aliAuth.device_name)+strlen(ALI_RANDOM_TIMESTAMP)+32;
                    ali_psk_identity = malloc(aliPskIdentityLen);
                    memset(ali_psk_identity, 0, aliPskIdentityLen);
                    sprintf(ali_psk_identity,"devicename|%s|%s|&%s|%s", mqttCurrContext->aliAuth.sign_method, mqttCurrContext->aliAuth.product_key, mqttCurrContext->aliAuth.device_name, ALI_RANDOM_TIMESTAMP);
                    aliPskSignLen = strlen(mqttCurrContext->aliAuth.product_key)+strlen(mqttCurrContext->aliAuth.device_name)+strlen(ALI_RANDOM_TIMESTAMP)+32;
                    ali_psk_sign = malloc(aliPskSignLen);
                    memset(ali_psk_sign, 0, aliPskSignLen);
                    sprintf(ali_psk_sign,"id%s&%stimestamp%s", mqttCurrContext->aliAuth.product_key, mqttCurrContext->aliAuth.device_name, ALI_RANDOM_TIMESTAMP);
                    /*=======for psk and psk_identity===================================*/
                    
                    #ifdef FEATURE_MBEDTLS_ENABLE
                    if(strcmp((CHAR*)mqttCurrContext->aliAuth.sign_method, "hmacsha1") == 0)
                    {
                        atAliHmacSha1((unsigned char *)ali_psk_sign, strlen(ali_psk_sign), (unsigned char *)ali_psk_hex,(unsigned char *)mqttCurrContext->aliAuth.device_secret, strlen(mqttCurrContext->aliAuth.device_secret));
                    }
                    else if(strcmp((CHAR*)mqttCurrContext->aliAuth.sign_method, "hmacsha256") == 0)
                    {
                        atAliHmacSha256((unsigned char *)ali_psk_sign, strlen(ali_psk_sign), (unsigned char *)ali_psk_hex,(unsigned char *)mqttCurrContext->aliAuth.device_secret, strlen(mqttCurrContext->aliAuth.device_secret));
                    }
                    else if(strcmp((CHAR*)mqttCurrContext->aliAuth.sign_method, "hmacmd5") == 0)
                    {
                        atAliHmacMd5((unsigned char *)ali_psk_sign, strlen(ali_psk_sign), (unsigned char *)ali_psk_hex,(unsigned char *)mqttCurrContext->aliAuth.device_secret, strlen(mqttCurrContext->aliAuth.device_secret));
                    }
                    else
                    {
                        free(ali_psk_identity);
                        free(ali_psk_sign);
                        return MQTT_ALI_ENCRYP_ERR;
                    }
                    #endif

                    /*=======for psk and psk_identity===================================*/
                    ali_psk = malloc(aliPskLen+1);
                    memset(ali_psk, 0, (aliPskLen+1));
                    cmsHexToHexStr(ali_psk, aliPskLen, (const UINT8 *)ali_psk_hex, 32);
                    
                    #ifdef FEATURE_MQTT_TLS_ENABLE
                    if(mqttCurrContext->mqtts_client->psk_key != NULL)
                    {
                        free(mqttCurrContext->mqtts_client->psk_key);
                        mqttCurrContext->mqtts_client->psk_key = NULL;
                    }
                    if(mqttCurrContext->mqtts_client->psk_identity != NULL)
                    {
                        free(mqttCurrContext->mqtts_client->psk_identity);
                        mqttCurrContext->mqtts_client->psk_identity = NULL;
                    }
                    mqttCurrContext->mqtts_client->psk_key = ali_psk;
                    mqttCurrContext->mqtts_client->psk_identity = ali_psk_identity;                
                    mqttCurrContext->is_mqtts = MQTT_SSL_HAVE;
                    #endif
                    /*=======for psk and psk_identity===================================*/                                       
                }
                break;
                
            case MQTT_CONFIG_OPEN:
                mqttCurrContext->port = cfgData->decParam1;                
                #ifdef FEATURE_MQTT_TLS_ENABLE
                mqttCurrContext->mqtts_client->port = cfgData->decParam1;
                #endif
                if(cfgData->strParam1!=NULL)
                {
                    strLen1 = strlen(cfgData->strParam1); /*topic*/

                    mqttCurrContext->mqtt_uri = malloc(strLen1+1);

                    memset(mqttCurrContext->mqtt_uri, 0, (strLen1+1));

                    memcpy(mqttCurrContext->mqtt_uri, cfgData->strParam1, strLen1);
                }
                else
                {
                    return MQTT_ERR;
                }
                break;
                           
            case MQTT_CONFIG_CLOUD:
                mqttCurrContext->cloud_type = cfgData->decParam1;
                mqttCurrContext->payloadType = cfgData->decParam2;
                break;

            case MQTT_CONFIG_SSL:
                mqttCurrContext->ssl_type = cfgData->decParam1;
                #ifdef FEATURE_MQTT_TLS_ENABLE
                mqttCurrContext->mqtts_client->isMqtts = MQTT_SSL_HAVE;
                #endif
                mqttCurrContext->is_mqtts = MQTT_SSL_HAVE;
                if(mqttCurrContext->ssl_type == MQTT_SSL_PSK)
                {
                    #ifdef FEATURE_MQTT_TLS_ENABLE
                    if(mqttCurrContext->mqtts_client->psk_key != NULL)
                    {
                        free(mqttCurrContext->mqtts_client->psk_key);
                        mqttCurrContext->mqtts_client->psk_key = NULL;
                    }
                    if(mqttCurrContext->mqtts_client->psk_identity != NULL)
                    {
                        free(mqttCurrContext->mqtts_client->psk_identity);
                        mqttCurrContext->mqtts_client->psk_identity = NULL;
                    }
                    mqttCurrContext->mqtts_client->psk_key = cfgData->strParam1;
                    mqttCurrContext->mqtts_client->psk_identity = cfgData->strParam2;
                    #endif
                }
                else if(mqttCurrContext->ssl_type == MQTT_SSL_ECC)
                {
                    if(mqttCurrContext->ecc_key != NULL)
                    {
                        free(mqttCurrContext->ecc_key);
                        mqttCurrContext->ecc_key = NULL;
                    }
                    mqttCurrContext->ecc_key = cfgData->strParam1;
                }
                else if(mqttCurrContext->ssl_type == MQTT_SSL_CA)
                {
                    if(mqttCurrContext->ca_key != NULL)
                    {
                        free(mqttCurrContext->ca_key);
                        mqttCurrContext->ca_key = NULL;
                        #ifdef FEATURE_MQTT_TLS_ENABLE
                        mqttCurrContext->mqtts_client->caCert = NULL;
                        #endif
                    }
                    if(mqttCurrContext->host_name != NULL)
                    {
                        free(mqttCurrContext->host_name);
                        mqttCurrContext->host_name = NULL;
                        #ifdef FEATURE_MQTT_TLS_ENABLE
                        mqttCurrContext->mqtts_client->hostName = NULL;
                        #endif
                    }                   
                    mqttCurrContext->ca_key = cfgData->strParam1;
                    mqttCurrContext->host_name = cfgData->strParam2;
                    #ifdef FEATURE_MQTT_TLS_ENABLE
                    mqttCurrContext->mqtts_client->caCert = cfgData->strParam1;
                    mqttCurrContext->mqtts_client->caCertLen = strlen(cfgData->strParam1);
                    mqttCurrContext->mqtts_client->hostName = cfgData->strParam2;
                    #endif
                }               
                break;
                
            default:
                return MQTT_ERR;
                //break;
        }
    }
    else
    {
        return MQTT_ERR;
    }
    return MQTT_OK;
}

int mqttOpenClient(mqtt_context *mqttContext)
{
    int ret = MQTT_OK;
    //char hmac_source[256] = {0};
    //char signature[64] ={0};
    mqtt_context *mqttNewContext = mqttContext;

    NetworkInit(mqttNewContext->mqtt_network);
    #if 0
    MQTTClientInit(client, network, "183.230.40.39", 6002, &connectData, 40000,
                    (unsigned char *)sendbuf, sizeof(sendbuf), (unsigned char *)readbuf, sizeof(readbuf));
    #else
    mqttClientInit(mqttNewContext, mqttNewContext->mqtt_network, mqttNewContext->mqtt_client->command_timeout_ms,
                    (unsigned char *)mqttNewContext->mqtt_send_buf, mqttNewContext->mqtt_send_buf_len, (unsigned char *)mqttNewContext->mqtt_read_buf, mqttNewContext->mqtt_read_buf_len);
    #endif
    mqttNewContext->reconnect = mqttReconnect;//reconnect callback function
    if((ret = NetworkSetConnTimeout(mqttNewContext->mqtt_network, MQTT_SEND_TIMEOUT, MQTT_RECV_TIMEOUT)) != 0)
    {
        mqttNewContext->mqtt_client->keepAliveInterval = mqttNewContext->mqtt_connect_data.keepAliveInterval;
        mqttNewContext->mqtt_client->ping_outstanding = 1;
        mqttDeleteContext(mqttNewContext);
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_22, P_INFO, 1, "...mqtt socket connect fail!!!...\r\n");
        return MQTT_SOCKET_ERR;
    }
    else
    {
        if(mqttNewContext->is_mqtts == MQTT_SSL_NONE)
        {
            if((ret = NetworkConnect(mqttNewContext->mqtt_network, mqttNewContext->mqtt_uri, mqttNewContext->port)) != 0)
            {
                mqttNewContext->mqtt_network->disconnect(mqttNewContext->mqtt_network);
                mqttDeleteContext(mqttNewContext);
                ECOMM_TRACE(UNILOG_MQTT, mqtt_task_23, P_INFO, 1, "...mqtt socket connect fail...\r\n");
                return MQTT_SOCKET_ERR;
            }
            else
            {
                ECOMM_TRACE(UNILOG_MQTT, mqtt_task_24, P_INFO, 1, "...mqtt socket connect ok!!!...\r\n");
                ECOMM_TRACE(UNILOG_MQTT, mqtt_task_25, P_INFO, 1, "...mqtt connect ok!!!...\r\n");
                mqttNewContext->is_used = MQTT_CONTEXT_IS_CREATING;
                mqttNewContext->is_connected = MQTT_CONN_OPENED;
                ret = MQTT_OK;
            }
        }
        else
        {
            #ifdef FEATURE_MQTT_TLS_ENABLE        
            ret = mqttSslConn(mqttNewContext->mqtts_client, mqttNewContext->mqtt_uri);
            ret = MQTT_OK;
            #endif
        }
    }

    return ret;
}

int mqttCloseClient(mqtt_context *mqttContext)
{
    int ret = MQTT_OK;

    mqtt_context *mqttNewContext = mqttContext;

    if((ret = mqttClose(mqttNewContext)) != MQTT_OK)
    {
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_26, P_INFO, 0, "...mqtt close fail!!!...\r\n");
        return MQTT_MQTT_CONN_ERR;
    }
    else
    {
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_27, P_INFO, 0, "...mqtt close ok!!!...\r\n");
        ret = MQTT_OK;
    }

    return ret;
}

int mqttConnectClient(mqtt_context *mqttContext)
{
    int ret = MQTT_OK;
    //char hmac_source[256] = {0};
    //char signature[64] ={0};
    mqtt_context *mqttNewContext = mqttContext;

    if((ret = mqttConnect(mqttNewContext, &mqttNewContext->mqtt_connect_data)) != 0)
    {
        mqttReconnectTcp(mqttNewContext);
        if((ret = mqttConnect(mqttNewContext, &mqttNewContext->mqtt_connect_data)) != 0)
        {
            mqttNewContext->mqtt_client->ping_outstanding = 1;
            mqttNewContext->mqtt_network->disconnect(mqttNewContext->mqtt_network);
            //mqttDeleteContext(mqttNewContext);
            
            //mqttNewContext->is_used = MQTT_CONTEXT_NOT_USED;
            mqttNewContext->is_connected = MQTT_CONN_CONNECT_FAIL;
            ECOMM_TRACE(UNILOG_MQTT, mqtt_task_28, P_INFO, 1, "...mqtt connect fail!!!...\r\n");
            return MQTT_MQTT_CONN_ERR;
        }
        else
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_task_29hh, P_INFO, 1, "...mqtt connect ok!!!...\r\n");
            mqttNewContext->is_used = MQTT_CONTEXT_USED;
            mqttNewContext->is_connected = MQTT_CONN_CONNECTED;
            ret = MQTT_OK;
        }
    }
    else
    {
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_29, P_INFO, 1, "...mqtt connect ok!!!...\r\n");
        mqttNewContext->is_used = MQTT_CONTEXT_USED;
        mqttNewContext->is_connected = MQTT_CONN_CONNECTED;
        ret = MQTT_OK;
    }

    return ret;
}


int mqttDisconnectClient(mqtt_context *mqttContext)
{
    int ret = MQTT_OK;
    //char hmac_source[256] = {0};
    //char signature[64] ={0};
    mqtt_context *mqttNewContext = mqttContext;

    if((ret = mqttDisconnect(mqttNewContext)) != MQTT_OK)
    {
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_30, P_INFO, 1, "...mqtt disconnect fail!!!...\r\n");
        return MQTT_MQTT_CONN_ERR;
    }
    else
    {
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_31, P_INFO, 1, "...mqtt disconnect ok!!!...\r\n");
        ret = MQTT_OK;
    }

    return ret;
}


int keepaliveCheck(mqtt_context* context)
{
    int rc = MQTTSUCCESS;

    if(context->is_used != MQTT_CONTEXT_USED) // for open cmd the contxet is creating
    {
        return rc;
    }

    if (context->mqtt_client->keepAliveInterval == 0)
    {
        return rc;
    }

    //check if curr ticks > last_sent.xTimeOut.xTimeOnEntering + last_sent.xTicksToWait
    //check if curr ticks > last_received.xTimeOut.xTimeOnEntering + last_received.xTicksToWait
    if (TimerIsExpired(&context->mqtt_client->last_sent) || TimerIsExpired(&context->mqtt_client->last_received))
    {
        if (context->mqtt_client->ping_outstanding)
        {
            keepalive_retry_time++;
            ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_32, P_ERROR, 1, "...mqtt keep alive ping resp timeout...");
            rc = FAILURE; /* PINGRESP not received in keepalive interval */
        }
        else
        {
            rc = MQTTKEEPALIVE;
        }
    }

    return rc;
}
void mqttCleanSession(MQTTClient* c)
{
    int i = 0;

    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
        c->messageHandlers[i].topicFilter = NULL;
}

void mqttCloseSession(MQTTClient* c)
{
    c->ping_outstanding = 0;
    c->isconnected = 0;
    if (c->cleansession)
        mqttCleanSession(c);
}

int mqttCycle(mqtt_context* context, Timer* timer)
{
    int len = 0,rc = MQTTSUCCESS;
    mqtt_send_msg mqttMsg;
    MQTTSubackData data;
    MQTTConnackData connData;
    unsigned short mypacketid;
    unsigned char dup, type;
    int count = 0;
    int socket_stat = -1;
    int socket_err = -1;
    mqtt_context *mqttConextTemp = NULL;
    data.grantedQoS = QOS0;
    int primSize;
    mqtt_message mqtt_msg;
    char *data_start_ptr = NULL;
    char *data_end_ptr = NULL;
    int clientIDLen = 0;
    int userNameLen = 0;
    int hmacSourceLen = 0;
    char *hmac_source = NULL;
    char *ali_clientID = NULL;
    char *ali_username = NULL;
    char *ali_signature = NULL;
    
    int packet_type = mqttReadPacket(context, timer);     /* read the socket, see what work is due */

    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_33, P_INFO, 1, "...mqtt readPacket packet_type=%d...",packet_type);

    switch (packet_type)
    {
        case -1:
        case 0: /* timed out reading packet */
        default:
            {
                /* no more data to read, unrecoverable. Or read packet fails due to unexpected network error */
                socket_stat = sock_get_errno(context->mqtt_client->ipstack->my_socket);
                socket_err = socket_error_is_fatal(socket_stat);
                if(socket_err == 1)
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_34, P_INFO, 1, ".....now, need reconnect mqtt server, when read packet...err=%d....",socket_stat);
                    mqttConextTemp = getCurrMqttContext();
                    if(mqttConextTemp != NULL)
                    {
                        if(mqttConextTemp->is_used == MQTT_CONTEXT_USED)
                        {
                            if(mqttConextTemp->is_connected == MQTT_CONN_CONNECTED)
                            {
                                /* send  reconnect msg to send task */
                                memset(&mqttMsg, 0, sizeof(mqttMsg));
                                mqttMsg.cmd_type = MQTT_MSG_RECONNECT;
                                mqttMsg.context_ptr = getCurrMqttContext();
                                mqttMsg.client_ptr = (void *)context->mqtt_client;
                                mqttMsg.message.payloadlen = len;

                                xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
                            }
                        }
                        else
                        {

                        }
                    }
                    rc = MQTTSUCCESS;
                }
            }
            break;

        case CONNACK:
            ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_101, P_INFO, 0, ".....now, recv connack.......");
            if (MQTTDeserialize_connack(&connData.sessionPresent, &connData.rc, context->mqtt_client->readbuf, context->mqtt_client->readbuf_size) != 1)
            {
                rc = FAILURE;
            }
            /* send result to at task */
            {
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_102, P_INFO, 0, ".....send mqtt conn packet ok.......");
                primSize = sizeof(mqtt_msg);
                if(rc == FAILURE)
                {
                    mqtt_msg.conn_ret_code = 6;
                }
                else
                {
                    mqtt_msg.conn_ret_code = connData.rc;
                }
                mqtt_msg.ret = 0;
                mqtt_msg.tcp_id = context->tcp_id;
                if(connData.rc != 0)
                {
                    if(context->reconnect_count > MQTT_RECONN_MAX)
                    {
                        memset(&mqttMsg, 0, sizeof(mqttMsg));
                        mqttMsg.cmd_type = MQTT_MSG_CONNECT_DOWN;
                        mqttMsg.context_ptr = getCurrMqttContext();
                        //context->is_connected = MQTT_CONN_RECONNECTING_FAIL;

                        xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
                    }
                    context->reconnect_count++;
                }
                else
                {
                    context->reconnect_count = 0;
                }
                applSendCmsInd(context->reqHandle, APPL_MQTT, APPL_MQTT_CONN_IND, primSize, (void *)&mqtt_msg);
            }
            break;

        case PUBACK:
            ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_35, P_INFO, 0, ".....now, recv puback.......");
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, context->mqtt_client->readbuf, context->mqtt_client->readbuf_size) != 1)
            {
                rc = FAILURE;
            }
            /* send result to at task */
            {
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_36, P_INFO, 0, ".....send mqtt pub packet ok.......");
                primSize = sizeof(mqtt_msg);
                if(rc == FAILURE)
                {
                    mqtt_msg.ret = 1;
                }
                else
                {
                    mqtt_msg.ret = 0;
                }
                mqtt_msg.tcp_id = context->tcp_id;
                mqtt_msg.msg_id = mypacketid;
                mqtt_msg.pub_ret_value = dup;

                applSendCmsInd(context->reqHandle, APPL_MQTT, APPL_MQTT_PUB_IND, primSize, (void *)&mqtt_msg);
            }
            break;

        case SUBACK:
            ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_37, P_INFO, 0, ".....now, recv suback.......");
            if (MQTTDeserialize_suback(&mypacketid, 1, &count, (int*)&data.grantedQoS, context->mqtt_client->readbuf, context->mqtt_client->readbuf_size) == 1)
            {
                if (data.grantedQoS != 0x80)
                {
                    rc = MQTTSetMessageHandler(context->mqtt_client, context->sub_topic, context->mqtt_client->defaultMessageHandler);
                }
            }
            else
            {
                rc = FAILURE;

            }
            /* send result to at task */
            {
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_38, P_INFO, 0, ".....recv mqtt suback packet ok.......");
                primSize = sizeof(mqtt_msg);
                if(rc == FAILURE)
                {
                    mqtt_msg.ret = 1;
                }
                else
                {
                    mqtt_msg.ret = 0;
                }
                mqtt_msg.tcp_id = context->tcp_id;
                mqtt_msg.msg_id = mypacketid;
                mqtt_msg.sub_ret_value = count;
                applSendCmsInd(context->reqHandle, APPL_MQTT, APPL_MQTT_SUB_IND, primSize, (void *)&mqtt_msg);
            }

            break;

        case UNSUBACK:
            ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_39, P_INFO, 0, ".....now, recv unsuback.......");
            // should be the same as the packetid above
            if (MQTTDeserialize_unsuback(&mypacketid, context->mqtt_client->readbuf, context->mqtt_client->readbuf_size) == 1)
            {
                /* remove the subscription message handler associated with this topic, if there is one */
                MQTTSetMessageHandler(context->mqtt_client,  context->unsub_topic, NULL);
            }
            else
            {
                rc = FAILURE;

            }
            /* send result to at task */
            {
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_100, P_INFO, 0, ".....recv mqtt unsuback packet ok.......");
                primSize = sizeof(mqtt_msg);
                if(rc == FAILURE)
                {
                    mqtt_msg.ret = 1;
                }
                else
                {
                    mqtt_msg.ret = 0;
                }
                mqtt_msg.tcp_id = context->tcp_id;
                mqtt_msg.msg_id = mypacketid;
                applSendCmsInd(context->reqHandle, APPL_MQTT, APPL_MQTT_UNSUB_IND, primSize, (void *)&mqtt_msg);
            }
            break;

        case PUBLISH:
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_40, P_INFO, 0, ".....now, recv publish.......");
            MQTTString topicName;
            MQTTMessage msg;
            int intQoS;
            //char *ackPayload = NULL;
            //char aliMsgId[32] = {0};
            //int i=0;
            //int lenTemp = 0;
            //int lenTopic = 0;
            
            msg.payloadlen = 0; /* this is a size_t, but deserialize publish sets this as int */
            memset(&mqttMsg, 0, sizeof(mqttMsg));
            memset(&topicName, 0, sizeof(topicName));
            mqttMsg.context_ptr = getCurrMqttContext();
            
            if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName,
                                        (unsigned char**)&msg.payload, (int*)&msg.payloadlen, context->mqtt_client->readbuf, context->mqtt_client->readbuf_size) != 1)
                goto exit;
            msg.qos = (enum QoS)intQoS;
            if (msg.qos == QOS1)
            {
                /* send pub msg to mqtt send task*/
                mqttMsg.cmd_type = MQTT_MSG_PUBLISH_ACK;
                mqttMsg.server_ack_mode = PUBACK;
                mqttMsg.msg_id = msg.id;
                mqttMsg.qos = msg.qos;
                
                mqttDeliverMessage(context->mqtt_client, &topicName, &msg);
                xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
                rc = MQTTSUCCESS;
            }
            else if (msg.qos == QOS2)
            {
                /* send pub msg to mqtt send task*/
                mqttMsg.cmd_type = MQTT_MSG_PUBLISH_REC;
                mqttMsg.server_ack_mode = PUBREC;
                mqttMsg.msg_id = msg.id;
                mqttMsg.qos = msg.qos;
                
                mqttDeliverMessage(context->mqtt_client, &topicName, &msg);
                xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
                rc = MQTTSUCCESS;
            }
            else
            {
                #if 0
                if(context->cloud_type == CLOUD_TYPE_ALI)  /*for ali cloud rrpc mode*/
                {
                    /* send pub msg to mqtt send task*/
                    mqttMsg.cmd_type = MQTT_MSG_PUBLISH;
                    mqttMsg.server_ack_mode = PUBACK;

                    /*set cmd*/
                    mqttMsg.cmd_type = MQTT_MSG_PUBLISH;
                    mqttMsg.tcp_id = context->tcp_id;
                    mqttMsg.msg_id = msg.id;
                    
                    ackPayload = strstr(topicName.lenstring.data, "rrpc/request");
                    if(ackPayload != NULL)
                    {
                        /*set topic*/
                        mqttMsg.topic       = malloc(128);
                        memset(mqttMsg.topic, 0 , 128);
                        
                        ackPayload = strstr(topicName.lenstring.data, "/request");
                        memcpy(mqttMsg.topic, topicName.lenstring.data, (ackPayload - topicName.lenstring.data));
                        
                        sprintf(mqttMsg.topic,"%s/response", mqttMsg.topic);
                        lenTemp = topicName.lenstring.len - (ackPayload - topicName.lenstring.data) - 8;
                        lenTopic = strlen(mqttMsg.topic);
                        
                        for(i=0; i<lenTemp; i++)
                        {
                            mqttMsg.topic[lenTopic+i] = topicName.lenstring.data[(ackPayload - topicName.lenstring.data)+8+i];
                        }
                                                    
                        /*set payload*/
                        ackPayload = strstr(msg.payload, "id");
                        if(ackPayload != NULL)
                        {
                            for(i=0; ; i++)
                            {
                                if(ackPayload[5+i] == '"')
                                {
                                    break;
                                }
                                aliMsgId[i] = ackPayload[5+i];
                            }
                            mqttMsg.message.payload       = malloc(128);
                            memset(mqttMsg.message.payload, 0 , 128);
                            
                            sprintf(mqttMsg.message.payload,"{\"code\":200,\"data\":{},\"id\":\"%s\",\"message\":\"success\",\"version\":\"1.0\"}", aliMsgId);
                            mqttMsg.message.payloadlen = strlen(mqttMsg.message.payload);
                        }
                        
                        xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
                    }
                }
                #endif
                if((context->cloud_type == CLOUD_TYPE_ALI)&&(context->aliAuth.dynamic_register_used == ALI_DYNAMIC_REGISTER_IS_USED))
                {
                    if(((strstr(topicName.lenstring.data, "/register") != NULL)&&(strstr(msg.payload, "deviceSecret") != NULL))
                       ||((strstr(topicName.lenstring.data, "/regnwl") != NULL)&&(strstr(msg.payload, "deviceToken") != NULL)))
                    {
                        if(strcmp((CHAR*)context->aliAuth.auth_type, "register") == 0)
                        {
                            data_start_ptr = strstr(msg.payload, "deviceSecret") + 15;
                            data_end_ptr   = strstr(data_start_ptr, "\"");
                            if(context->aliAuth.device_secret != NULL)
                            {
                                free(context->aliAuth.device_secret);
                                context->aliAuth.device_secret = NULL;
                            }
                            len = data_end_ptr - data_start_ptr;
                            context->aliAuth.device_secret = malloc(len+1);
                            memset(context->aliAuth.device_secret, 0, len+1);
                            memcpy(context->aliAuth.device_secret, data_start_ptr, len);
                            ECOMM_STRING(UNILOG_MQTT, hh11tt, P_INFO, ".........device_secret......:%s",(uint8_t*)context->aliAuth.device_secret);
                        
                            clientIDLen = strlen(context->aliAuth.product_name)+strlen(ALI_RANDOM_TIMESTAMP)+64;
                            userNameLen = strlen(context->aliAuth.device_name)+strlen(context->aliAuth.product_key)+8;
                            hmacSourceLen = strlen(context->aliAuth.product_name)+strlen(context->aliAuth.device_name)+strlen(context->aliAuth.product_key)+strlen(ALI_RANDOM_TIMESTAMP)+64;
                           
                            ali_clientID = malloc(clientIDLen);
                            ali_username = malloc(userNameLen);
                            ali_signature = malloc(96);
                            hmac_source = malloc(hmacSourceLen); 
                            
                            memset(ali_clientID, 0, clientIDLen);
                            memset(ali_username, 0, userNameLen);
                            memset(ali_signature, 0, 96);
                            memset(hmac_source, 0, hmacSourceLen);
                            snprintf(hmac_source, hmacSourceLen, "clientId%s" "deviceName%s" "productKey%s" "timestamp%s", context->aliAuth.product_name, context->aliAuth.device_name, context->aliAuth.product_key, ALI_RANDOM_TIMESTAMP);
                            
                            #ifdef FEATURE_MBEDTLS_ENABLE
                            atAliHmacSha1((unsigned char *)hmac_source, strlen(hmac_source), (unsigned char *)ali_signature,(unsigned char *)context->aliAuth.device_secret, strlen(context->aliAuth.device_secret));
                            #endif
                            free(hmac_source);
                            
                            sprintf(ali_clientID,"%s|securemode=3,signmethod=hmacsha1,timestamp=%s|", context->aliAuth.product_name, ALI_RANDOM_TIMESTAMP);
                            sprintf(ali_username,"%s&%s",context->aliAuth.device_name,context->aliAuth.product_key);
                        }
                        else if(strcmp((CHAR*)context->aliAuth.auth_type, "regnwl") == 0)
                        {
                            data_start_ptr = strstr(msg.payload, "deviceToken") + 14;
                            data_end_ptr   = strstr(data_start_ptr, "\"");
                            if(context->aliAuth.device_token != NULL)
                            {
                                free(context->aliAuth.device_token);
                                context->aliAuth.device_token = NULL;
                            }
                            len = data_end_ptr - data_start_ptr;
                            context->aliAuth.device_token = malloc(len+1);
                            memset(context->aliAuth.device_token, 0, len+1);
                            memcpy(context->aliAuth.device_token, data_start_ptr, len);
                            ECOMM_STRING(UNILOG_MQTT, hh11tt1, P_INFO, ".........device_token......:%s",(uint8_t*)context->aliAuth.device_token);

                            data_start_ptr = strstr(msg.payload, "clientId") + 11;
                            data_end_ptr   = strstr(data_start_ptr, "\"");
                            if(context->aliAuth.client_id != NULL)
                            {
                                free(context->aliAuth.client_id);
                                context->aliAuth.client_id = NULL;
                            }
                            len = data_end_ptr - data_start_ptr;
                            context->aliAuth.client_id = malloc(len+1);
                            memset(context->aliAuth.client_id, 0, len+1);
                            memcpy(context->aliAuth.client_id, data_start_ptr, len);
                            ECOMM_STRING(UNILOG_MQTT, hh11tt11, P_INFO, ".........client_id......:%s",(uint8_t*)context->aliAuth.client_id);
                            clientIDLen = len+strlen(ALI_RANDOM_TIMESTAMP)+64;
                            userNameLen = strlen(context->aliAuth.device_name)+strlen(context->aliAuth.product_key)+8;
                           
                            ali_clientID = malloc(clientIDLen);
                            ali_username = malloc(userNameLen);
                            
                            memset(ali_clientID, 0, clientIDLen);
                            memset(ali_username, 0, userNameLen);
                        
                            sprintf(ali_clientID,"%s|securemode=-2,authType=connwl|", context->aliAuth.client_id);
                            sprintf(ali_username,"%s&%s",context->aliAuth.device_name,context->aliAuth.product_key);
                            ali_signature = context->aliAuth.device_token;
                        }
                        
                        if(context->mqtt_connect_data.clientID.cstring != NULL)
                        {
                            free(context->mqtt_connect_data.clientID.cstring);
                            context->mqtt_connect_data.clientID.cstring = NULL;
                        }
                        if(context->mqtt_connect_data.username.cstring != NULL)
                        {
                            free(context->mqtt_connect_data.username.cstring);
                            context->mqtt_connect_data.username.cstring = NULL;
                        }
                        if(context->mqtt_connect_data.password.cstring != NULL)
                        {
                            free(context->mqtt_connect_data.password.cstring);
                            context->mqtt_connect_data.password.cstring = NULL;
                        }
                        
                        context->mqtt_connect_data.clientID.cstring = ali_clientID;
                        context->mqtt_connect_data.username.cstring = ali_username;
                        context->mqtt_connect_data.password.cstring = ali_signature;
                        memset(&mqttMsg, 0, sizeof(mqttMsg));
                        mqttMsg.cmd_type = MQTT_MSG_ALI_DYN_CONNECT;
                        mqttMsg.context_ptr = (void *)context;
                        mqttMsg.reqhandle = context->reqHandle;
                        mqttMsg.tcp_id = context->tcp_id;

                        ECOMM_STRING(UNILOG_MQTT, hh1qq, P_INFO, ".........clientID......:%s",(uint8_t*)context->mqtt_connect_data.clientID.cstring);
                        ECOMM_STRING(UNILOG_MQTT, hh2qqq, P_INFO, ".........username......:%s",(uint8_t*)context->mqtt_connect_data.username.cstring);
                        ECOMM_STRING(UNILOG_MQTT, hh3qqqq, P_INFO, ".........password......:%s",(uint8_t*)context->mqtt_connect_data.password.cstring);
                        
                        xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);                        
                    }
                }
                
                mqttDeliverMessage(context->mqtt_client, &topicName, &msg);
                rc = MQTTSUCCESS;
            }
            break;
        }

        case PUBREC:
        case PUBREL:
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_41, P_INFO, 0, ".....now, recv pubrec or pubrel.......");
            
            memset(&mqttMsg, 0, sizeof(mqttMsg));
            mqttMsg.context_ptr = getCurrMqttContext();
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, context->mqtt_client->readbuf, context->mqtt_client->readbuf_size) != 1)
            {
                rc = FAILURE;
            }
            else
            {
                /* send msg to mqtt send task*/
                if(PUBREC == packet_type)
                {
                    mqttMsg.cmd_type = MQTT_MSG_PUBLISH_REC;
                    mqttMsg.server_ack_mode = PUBREL;
                    mqttMsg.msg_id = mypacketid;
                    xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
                }
                else if(PUBREL == packet_type)
                {
                    mqttMsg.cmd_type = MQTT_MSG_PUBLISH_COMP;
                    mqttMsg.server_ack_mode = PUBCOMP;
                    mqttMsg.msg_id = mypacketid;
                    xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
                }
                rc = MQTTSUCCESS;
            }

            if (rc == FAILURE)
            {
                goto exit; // there was a problem
            }
            break;
        }

        case PUBCOMP:
            ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_42, P_INFO, 0, ".....now, recv pubcomp.......");
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, context->mqtt_client->readbuf, context->mqtt_client->readbuf_size) != 1)
            {
                rc = FAILURE;
            }
            /* send result to at task */
            {
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_43, P_INFO, 0, ".....send mqtt pub packet ok.......");
                primSize = sizeof(mqtt_msg);
                if(rc == FAILURE)
                {
                    mqtt_msg.ret = 1;
                }
                else
                {
                    mqtt_msg.ret = 0;
                }
                mqtt_msg.tcp_id = context->tcp_id;
                mqtt_msg.msg_id = mypacketid;
                applSendCmsInd(context->reqHandle, APPL_MQTT, APPL_MQTT_PUB_IND, primSize, (void *)&mqtt_msg);
            }
            break;

        case PINGRESP:
        {
            context->mqtt_client->ping_outstanding = 0;
            keepalive_retry_time = 0;
            /* delete keepalive timer */
            ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_44, P_INFO, 1, ".....mqtt recv keeplive ack ok.......");
            break;
        }
    }

    rc = keepaliveCheck(context);

    if(rc == MQTTSUCCESS)
    {
        /* not need send keepalive packet */
    }
    else if(rc == MQTTKEEPALIVE)
    {
        ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_45, P_INFO, 0, ".....now, need keepalive.......");
        /* need send keepalive packet, send  keepalive msg to send task */
        len = MQTTSerialize_pingreq(context->mqtt_client->buf, context->mqtt_client->buf_size);
        memset(&mqttMsg, 0, sizeof(mqttMsg));
        mqttMsg.cmd_type = MQTT_MSG_KEEPALIVE;
        mqttMsg.context_ptr = getCurrMqttContext();
        mqttMsg.message.payloadlen = len;

        xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
        rc = MQTTSUCCESS;
    }
    else if(rc == FAILURE)
    {
        /* keepalive fail */
        int socket_stat = 0;

        socket_stat = sock_get_errno(context->mqtt_client->ipstack->my_socket);
        if((socket_stat == MQTT_ERR_ABRT)||(socket_stat == MQTT_ERR_RST)||(socket_stat == MQTT_ERR_CLSD)||(socket_stat == MQTT_ERR_BADE))
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_46, P_INFO, 0, ".....now, need reconnect.......");
            /* send  reconnect msg to send task */
            memset(&mqttMsg, 0, sizeof(mqttMsg));
            mqttMsg.cmd_type = MQTT_MSG_RECONNECT;
            mqttMsg.context_ptr = getCurrMqttContext();
            mqttMsg.message.payloadlen = len;

            xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
            rc = MQTTSUCCESS;
        }
        else
        {
            if(keepalive_retry_time > KEEPALIVE_RETRY_MAX)
            {
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_47, P_INFO, 0, ".....now, need reconnect.......");
                /* send  reconnect msg to send task */
                keepalive_retry_time = 0;
                memset(&mqttMsg, 0, sizeof(mqttMsg));
                mqttMsg.cmd_type = MQTT_MSG_RECONNECT;
                mqttMsg.context_ptr = getCurrMqttContext();
                mqttMsg.message.payloadlen = len;

                xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
                rc = MQTTSUCCESS;
            }
            else
            {
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_48, P_INFO, 0, ".....now, need keepalive.......");
                /* send  keepalive msg to send task */
                len = MQTTSerialize_pingreq(context->mqtt_client->buf, context->mqtt_client->buf_size);
                memset(&mqttMsg, 0, sizeof(mqttMsg));
                mqttMsg.cmd_type = MQTT_MSG_KEEPALIVE;
                mqttMsg.context_ptr = getCurrMqttContext();
                mqttMsg.message.payloadlen = len;

                xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
                rc = MQTTSUCCESS;
            }
        }
    }

exit:
    if (rc == MQTTSUCCESS)
    {
        rc = packet_type;
    }
    else
    {
        if (context->mqtt_client->isconnected)
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_49, P_ERROR, 1, "....mqtt close session");
            mqttCloseSession(context->mqtt_client);
        }
    }
    return rc;
}


int mqttYield(mqtt_context* context, int timeout_ms)
{
    int rc = MQTTSUCCESS;
    Timer timer;

    TimerInit(&timer);
    TimerCountdownMS(&timer, timeout_ms);

    do
    {
        if (mqttCycle(context, &timer) < 0)
        {
            rc = FAILURE;
            break;
        }
    } while (!TimerIsExpired(&timer));

    return rc;
}

void mqttTaskRecvProcess(void *argument)
{
    int i = 0;
    int hasBusyClient = 0;

    mqtt_task_recv_status_flag = MQTT_TASK_CREATE;

    while(1)
    {
        hasBusyClient = 0;
        for(i=0; i<MQTT_CONTEXT_NUMB_MAX; i++)
        {
            MutexLock(&mqtt_mutex1);
            if((mqttContext[i].is_used == MQTT_CONTEXT_USED)&&(mqttContext[i].is_connected != MQTT_CONN_DEFAULT))
            {
                mqttCurrContext = &mqttContext[i];
                hasBusyClient = 1;
                mqttYield(&mqttContext[i], MQTT_RECV_LOOP_TIMEOUT);
            }
            if((mqttContext[i].is_used == MQTT_CONTEXT_IS_CREATING)||(mqttContext[i].is_connected == MQTT_CONN_IS_CONNECTING))
            {
                hasBusyClient = 1;
                MutexUnlock(&mqtt_mutex1);
                osDelay(200);
            }
            else
            {
                MutexUnlock(&mqtt_mutex1);
            }
            osDelay(100);
        }

        if((hasBusyClient == 0))
        {
            break;
        }
    }
    mqtt_recv_task_handle = NULL;
    mqtt_task_recv_status_flag = MQTT_TASK_DELETE;
    osThreadExit();
}

void mqttTaskSendProcess(void *argument)
{
    int i = 0;
    int ret = MQTT_ERR;
    int hasBusyClient = 0;
    int msg_type = 0xff;
    int len = 0;
    mqtt_send_msg mqttMsg;
    Timer timer;
    mqtt_context *mqttNewContext;
    int primSize;
    mqtt_message mqtt_msg;
    int socket_stat = -1;
    int socket_err = -1;

    mqtt_task_send_status_flag = MQTT_TASK_CREATE;
    slpManRet_t vote_ret = slpManApplyPlatVoteHandle("ECMQTT", &mqttSlpHandler);

    if(RET_TRUE != vote_ret)
    {
        ECOMM_TRACE(UNILOG_MQTT, mqttTaskSendProcess_0, P_INFO, 1, "...mqttTaskSendProcess apply vote fail, vote_ret=%d...",vote_ret);
    }

    vote_ret = slpManPlatVoteDisableSleep(mqttSlpHandler,SLP_SLP2_STATE);

    if(RET_TRUE != vote_ret)
    {
        ECOMM_TRACE(UNILOG_MQTT, mqttTaskSendProcess_1, P_INFO, 1, "...mqttTaskSendProcess apply vote fail, vote_ret=%d...",vote_ret);
    }

    while(1)
    {
        /* recv msg (block mode) */
        xQueueReceive(mqtt_send_msg_handle, &mqttMsg, osWaitForever);
        msg_type = mqttMsg.cmd_type;
        mqttNewContext = (mqtt_context *)mqttMsg.context_ptr;

        switch(msg_type)
        {
            case MQTT_MSG_OPEN:
                /* create mqtt client */
                mqttNewContext->is_connected = MQTT_CONN_IS_OPENING;
                MutexLock(&mqtt_mutex1);
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_50, P_INFO, 0, ".....start open mqtt client.......");

                ret = mqttOpenClient(mqttNewContext);

                MutexUnlock(&mqtt_mutex1);

                /* send result to at task */
                if(ret == MQTT_OK)
                {
                    mqttNewContext->is_used = MQTT_CONTEXT_USED;
                    mqttNewContext->is_connected = MQTT_CONN_OPENED;
                    mqttNewContext->reqHandle = mqttMsg.reqhandle;
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_51, P_INFO, 0, ".....open mqtt client ok.......");
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.ret = 0;
                    applSendCmsInd(mqttNewContext->reqHandle, APPL_MQTT, APPL_MQTT_OPEN_IND, primSize, (void *)&mqtt_msg);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_52, P_INFO, 0, ".....open mqtt client fail.......");
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.ret = -1;
                    mqttNewContext->is_connected = MQTT_CONN_OPEN_FAIL;
                    mqttNewContext->reqHandle = mqttMsg.reqhandle;
                    mqttDeleteContext(mqttNewContext);
                    applSendCmsInd(mqttNewContext->reqHandle, APPL_MQTT, APPL_MQTT_OPEN_IND, primSize, (void *)&mqtt_msg);
                }
                break;
                
            case MQTT_MSG_CLOSE:
                if(mqttNewContext->is_connected == MQTT_CONN_CONNECTED)
                {
                    mqttNewContext->is_connected = MQTT_CONN_IS_CLOSING;
                    MutexLock(&mqtt_mutex1);
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_53, P_INFO, 0, ".....start close mqtt client.......");

                    ret = mqttDisconnectClient(mqttNewContext);
                    if(ret == MQTT_OK)
                    {
                        ret = mqttCloseClient(mqttNewContext);
                    }
                }
                else
                {
                    MutexLock(&mqtt_mutex1);
                    ret = mqttCloseClient(mqttNewContext);
                }

                /* send result to at task */
                if(ret == MQTT_OK)
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_54, P_INFO, 0, ".....close mqtt client ok.......");
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.ret = 0;
                    mqttNewContext->is_connected = MQTT_CONN_CLOSED;
                    mqttDeleteContext(mqttNewContext);
                    applSendCmsInd(mqttNewContext->reqHandle, APPL_MQTT, APPL_MQTT_CLOSE_IND, primSize, (void *)&mqtt_msg);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_55, P_INFO, 0, ".....close mqtt client fail.......");
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.ret = -1;
                    mqttNewContext->is_connected = MQTT_CONN_CLOSED_FAIL;
                    applSendCmsInd(mqttNewContext->reqHandle, APPL_MQTT, APPL_MQTT_CLOSE_IND, primSize, (void *)&mqtt_msg);
                }
                MutexUnlock(&mqtt_mutex1);
                break;

            case MQTT_MSG_CONNECT:
                /* create mqtt client */
                mqttNewContext->is_connected = MQTT_CONN_IS_CONNECTING;
                MutexLock(&mqtt_mutex1);
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_56, P_INFO, 0, ".....start connect mqtt client.......");

                mqtt_recv_task_Init();

                ret = mqttConnectClient(mqttNewContext);

                MutexUnlock(&mqtt_mutex1);

                /* send result to at task */
                if(ret == MQTT_OK)
                {
                    //mqttKeepalive(mqttNewContext->mqtt_client);
                    mqttNewContext->is_used = MQTT_CONTEXT_USED;
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_57, P_INFO, 0, ".....connect mqtt client ok.......");
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.ret = 0;
                    mqtt_msg.conn_ret_code = 0;
                    mqttNewContext->is_connected = MQTT_CONN_CONNECTED;
                    //applSendCmsInd(BROADCAST_IND_HANDLER, APPL_MQTT, APPL_MQTT_CONN_IND, primSize, (void *)&mqtt_msg);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_58, P_INFO, 0, ".....connect mqtt client fail.......");
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.ret = 2;
                    mqtt_msg.conn_ret_code = 2;
                    mqttNewContext->is_connected = MQTT_CONN_CONNECT_FAIL;
                    //mqttDeleteContext(mqttNewContext);
                    applSendCmsInd(mqttNewContext->reqHandle, APPL_MQTT, APPL_MQTT_CONN_IND, primSize, (void *)&mqtt_msg);
                }
                break;

            case MQTT_MSG_DISCONNECT:
                if(mqttNewContext->is_connected == MQTT_CONN_CONNECTED)
                {
                    mqttNewContext->is_connected = MQTT_CONN_IS_DISCONNECTING;
                    MutexLock(&mqtt_mutex1);
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_59, P_INFO, 0, ".....start disconnect mqtt client.......");

                    ret = mqttDisconnectClient(mqttNewContext);
                    if(ret == MQTT_OK)
                    {
                        ret = mqttCloseClient(mqttNewContext);
                    }
                }
                else if(mqttNewContext->is_connected == MQTT_CONN_CLOSED)
                {
                    MutexLock(&mqtt_mutex1);
                    ret = MQTT_OK;
                }
                else
                {
                    MutexLock(&mqtt_mutex1);
                    ret = MQTT_CONTEXT_ERR;
                }

                /* send result to at task */
                if(ret == MQTT_OK)
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_60, P_INFO, 0, ".....disconnect mqtt client ok.......");
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.ret = 0;
                    mqttNewContext->is_connected = MQTT_CONN_DISCONNECTED;
                    mqttDeleteContext(mqttNewContext);
                    applSendCmsInd(mqttNewContext->reqHandle, APPL_MQTT, APPL_MQTT_DISC_IND, primSize, (void *)&mqtt_msg);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_61, P_INFO, 0, ".....disconnect mqtt client fail.......");
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.ret = -1;
                    mqttNewContext->is_connected = MQTT_CONN_DISCONNECTED_FAIL;
                    applSendCmsInd(mqttNewContext->reqHandle, APPL_MQTT, APPL_MQTT_DISC_IND, primSize, (void *)&mqtt_msg);
                }
                MutexUnlock(&mqtt_mutex1);
                break;
            
            case MQTT_MSG_ALI_DYN_CONNECT:
                /* create mqtt client */
                mqttNewContext->is_connected = MQTT_CONN_IS_CONNECTING;
                MutexLock(&mqtt_mutex1);
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_ali_00, P_INFO, 0, ".....start connect mqtt client.......");

                ret = mqttDisconnectClient(mqttNewContext);
                if(ret == MQTT_OK)
                {
                    ret = mqttCloseClient(mqttNewContext);
                }
                ret = mqttOpenClient(mqttNewContext);
                if(ret == MQTT_OK)
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_ali_01, P_INFO, 0, ".....ali open ok.......");
                }

                ret = mqttConnectClient(mqttNewContext);

                MutexUnlock(&mqtt_mutex1);

                /* send result to at task */
                if(ret == MQTT_OK)
                {
                    //mqttKeepalive(mqttNewContext->mqtt_client);
                    mqttNewContext->is_used = MQTT_CONTEXT_USED;
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_57hh, P_INFO, 0, ".....connect mqtt client ok.......");
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.ret = 0;
                    mqtt_msg.conn_ret_code = 0;
                    mqttNewContext->is_connected = MQTT_CONN_CONNECTED;
                    //applSendCmsInd(BROADCAST_IND_HANDLER, APPL_MQTT, APPL_MQTT_CONN_IND, primSize, (void *)&mqtt_msg);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_58hhh, P_INFO, 0, ".....connect mqtt client fail.......");
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.ret = 2;
                    mqtt_msg.conn_ret_code = 2;
                    mqttNewContext->is_connected = MQTT_CONN_CONNECT_FAIL;
                    //mqttDeleteContext(mqttNewContext);
                    applSendCmsInd(mqttNewContext->reqHandle, APPL_MQTT, APPL_MQTT_CONN_IND, primSize, (void *)&mqtt_msg);
                }
                break;

            case MQTT_MSG_SUB:
                /* send sub packet */
                //MutexLock(&mqtt_mutex1);
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_62, P_INFO, 0, ".....start send mqtt sub packet.......");

                if(mqttNewContext->is_connected == MQTT_CONN_CONNECTED)
                {
                    memset(mqttNewContext->mqtt_send_buf, 0, mqttNewContext->mqtt_send_buf_len);
                    memset(mqttNewContext->mqtt_read_buf, 0, mqttNewContext->mqtt_read_buf_len);
                    if ((ret = mqttSubscribe(mqttNewContext, mqttMsg.msg_id, mqttMsg.sub_topic, (enum QoS)mqttMsg.qos, mqttNewContext->mqtt_msg_handler)) != MQTT_OK)
                    {
                        //mqttNewContext->mqtt_client->ping_outstanding = 1;
                    }
                    if(mqttMsg.sub_topic != NULL)
                    {
                        free(mqttMsg.sub_topic);
                        mqttMsg.sub_topic = NULL;
                    }
                }
                else
                {
                    if(mqttMsg.sub_topic != NULL)
                    {
                        free(mqttMsg.sub_topic);
                        mqttMsg.sub_topic = NULL;
                    }
                    ret = MQTT_ERR;
                }
                /* send result to at task */
                if(ret == MQTT_OK)
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_63, P_INFO, 0, ".....send mqtt sub packet ok.......");
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.msg_id = mqttMsg.msg_id;
                    mqtt_msg.ret = 0;
                    mqtt_msg.sub_ret_value = 0;
                    //applSendCmsInd(BROADCAST_IND_HANDLER, APPL_MQTT, APPL_MQTT_SUB_IND, primSize, (void *)&mqtt_msg);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_64, P_INFO, 0, ".....send mqtt sub packet fail.......");
                    socket_stat = sock_get_errno(mqttNewContext->mqtt_client->ipstack->my_socket);
                    socket_err = socket_error_is_fatal(socket_stat);
                    if(socket_err == 1)
                    {
                        ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_65, P_INFO, 0, ".....find need reconnect when send sub packet.......");
                        ret = MQTT_RECONNECT;
                    }
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.msg_id = mqttMsg.msg_id;
                    mqtt_msg.ret = 2;
                    mqtt_msg.sub_ret_value = 0;
                    applSendCmsInd(mqttNewContext->reqHandle, APPL_MQTT, APPL_MQTT_SUB_IND, primSize, (void *)&mqtt_msg);
                }
                //MutexUnlock(&mqtt_mutex1);
                break;

            case MQTT_MSG_UNSUB:
                /* send unsub packet */
                //MutexLock(&mqtt_mutex1);
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_66, P_INFO, 0, ".....start send mqtt unsub packet.......");

                if(mqttNewContext->is_connected == MQTT_CONN_CONNECTED)
                {
                    memset(mqttNewContext->mqtt_send_buf, 0, mqttNewContext->mqtt_send_buf_len);
                    memset(mqttNewContext->mqtt_read_buf, 0, mqttNewContext->mqtt_read_buf_len);

                    ret = mqttUnsubscribe(mqttNewContext, mqttMsg.msg_id, mqttMsg.unsub_topic);

                    if(mqttMsg.unsub_topic != NULL)
                    {
                        free(mqttMsg.unsub_topic);
                        mqttMsg.unsub_topic = NULL;
                    }
                }
                else
                {
                    if(mqttMsg.unsub_topic != NULL)
                    {
                        free(mqttMsg.unsub_topic);
                        mqttMsg.unsub_topic = NULL;
                    }
                    ret = MQTT_ERR;
                }
                
                /* send result to at task */
                if(ret == MQTT_OK)
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_67, P_INFO, 0, ".....send mqtt unsub packet ok.......");
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.msg_id = mqttMsg.msg_id;
                    mqtt_msg.ret = 0;
                    //applSendCmsInd(BROADCAST_IND_HANDLER, APPL_MQTT, APPL_MQTT_UNSUB_IND, primSize, (void *)&mqtt_msg);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_68, P_INFO, 0, ".....send mqtt unsub packet fail.......");
                    socket_stat = sock_get_errno(mqttNewContext->mqtt_client->ipstack->my_socket);
                    socket_err = socket_error_is_fatal(socket_stat);
                    if(socket_err == 1)
                    {
                        ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_69, P_INFO, 0, ".....find need reconnect when send unsub packet.......");
                        ret = MQTT_RECONNECT;
                    }
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.msg_id = mqttMsg.msg_id;
                    mqtt_msg.ret = 2;
                    applSendCmsInd(mqttNewContext->reqHandle, APPL_MQTT, APPL_MQTT_UNSUB_IND, primSize, (void *)&mqtt_msg);
                }
                //MutexUnlock(&mqtt_mutex1);
                break;

            case MQTT_MSG_PUBLISH:
                /* send packet */
                //MutexLock(&mqtt_mutex1);
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_70, P_INFO, 1, ".....start send mqtt publish packet...msg_id=%d....",mqttMsg.msg_id);

                if(mqttNewContext->is_connected == MQTT_CONN_CONNECTED)
                {
                    memset(mqttNewContext->mqtt_send_buf, 0, mqttNewContext->mqtt_send_buf_len);
                    ret = mqttPublish(mqttNewContext, mqttMsg.msg_id, mqttMsg.topic, &mqttMsg.message, mqttMsg.rai, false);

                    if(mqttMsg.topic != NULL)
                    {
                        free(mqttMsg.topic);
                        mqttMsg.topic = NULL;
                    }
                    if(mqttMsg.message.payload != NULL)
                    {
                        free(mqttMsg.message.payload);
                        mqttMsg.message.payload = NULL;
                    }   
                }
                else
                {
                    if(mqttMsg.topic != NULL)
                    {
                        free(mqttMsg.topic);
                        mqttMsg.topic = NULL;
                    }
                    if(mqttMsg.message.payload != NULL)
                    {
                        free(mqttMsg.message.payload);
                        mqttMsg.message.payload = NULL;
                    }   
                    ret = MQTT_ERR;
                }
                /* send result to at task */
                if(ret == MQTT_OK)
                {
                    if(mqttMsg.message.qos == QOS0)
                    {
                        ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_71, P_INFO, 1, ".....send mqtt publish packet ok..msg_id=%d......",mqttMsg.msg_id);
                        primSize = sizeof(mqtt_msg);
                        mqtt_msg.tcp_id = mqttMsg.tcp_id;
                        mqtt_msg.msg_id = mqttMsg.msg_id;
                        mqtt_msg.ret = 0;
                        applSendCmsInd(mqttNewContext->reqHandle, APPL_MQTT, APPL_MQTT_PUB_IND, primSize, (void *)&mqtt_msg);
                    }
                    else
                    {
                        ret = MQTT_OK;
                    }
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_72, P_INFO, 0, ".....send mqtt publish packet fail.......");
                    socket_stat = sock_get_errno(mqttNewContext->mqtt_client->ipstack->my_socket);
                    socket_err = socket_error_is_fatal(socket_stat);
                    if(socket_err == 1)
                    {
                        ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_73, P_INFO, 0, ".....find need reconnect when publish packet.......");
                        ret = MQTT_RECONNECT;
                    }
                    primSize = sizeof(mqtt_msg);
                    mqtt_msg.tcp_id = mqttMsg.tcp_id;
                    mqtt_msg.msg_id = mqttMsg.msg_id;
                    mqtt_msg.ret = 2;
                    applSendCmsInd(mqttNewContext->reqHandle, APPL_MQTT, APPL_MQTT_PUB_IND, primSize, (void *)&mqtt_msg);
                }
                //MutexUnlock(&mqtt_mutex1);
                break;

            case MQTT_MSG_PUBLISH_REC:
            case MQTT_MSG_PUBLISH_REL:
            case MQTT_MSG_PUBLISH_COMP:
            case MQTT_MSG_PUBLISH_ACK:
                /* send pubrel packet */
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_74, P_INFO, 1, ".....start send mqtt pub rec/rel/comp/ack packet....%d...",mqttMsg.server_ack_mode);

                if(mqttNewContext->is_connected == MQTT_CONN_CONNECTED)
                {
                    len = MQTTSerialize_ack(mqttNewContext->mqtt_client->buf, mqttNewContext->mqtt_client->buf_size, mqttMsg.server_ack_mode, 0, mqttMsg.msg_id);
                    TimerInit(&timer);
                    TimerCountdownMS(&timer, MQTT_SEND_TIMEOUT);
                    
                    ret = mqttSendPacket(mqttNewContext, len, &timer, 0, false);
                }
                else
                {
                    ret = FAILURE;
                }
                if(ret == MQTTSUCCESS)
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_75, P_INFO, 0, ".....send mqtt pub rec/rel/comp/ack packet ok.......");
                    ret = MQTT_OK;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_76, P_INFO, 0, ".....send mqtt pub rec/rel/comp/ack packet fail.......");
                    socket_stat = sock_get_errno(mqttNewContext->mqtt_client->ipstack->my_socket);
                    socket_err = socket_error_is_fatal(socket_stat);
                    if(socket_err == 1)
                    {
                        ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_77, P_INFO, 0, ".....find need reconnect when send pub rec/rel/comp/ack packet.......");
                        ret = MQTT_RECONNECT;
                    }
                }
                break;

            case MQTT_MSG_KEEPALIVE:
                /* send keepalive packet */
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_78, P_INFO, 0, ".....start send mqtt keeplive packet.......");

                ret = mqttKeepalive(mqttNewContext);

                if(ret == MQTTSUCCESS) // send the ping packet
                {
                    mqttNewContext->mqtt_client->ping_outstanding = 1;
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_79, P_INFO, 1, ".....mqtt keeplive send ok.......");
                }
                else
                {
                    socket_stat = sock_get_errno(mqttNewContext->mqtt_client->ipstack->my_socket);
                    socket_err = socket_error_is_fatal(socket_stat);
                    if(socket_err == 1)
                    {
                        ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_80, P_INFO, 0, ".....find need reconnect when send keeplive packet.......");
                        ret = MQTT_RECONNECT;
                    }
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_81, P_INFO, 1, ".....mqtt send keeplive Packet fail");
                }
                break;
                
            case MQTT_MSG_CONNECT_DOWN:
                /* send keepalive packet */
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_82, P_INFO, 0, ".....connect is shut down by server.......");
                ret = mqttDisconnectClient(mqttNewContext);
                if(ret == MQTT_OK)
                {
                    mqttNewContext->is_connected = MQTT_CONN_OPENED;
                }
                else
                {
                    mqttNewContext->is_connected = MQTT_CONN_OPENED;
                }
                break;
                
            case MQTT_MSG_RECONNECT:
                primSize = sizeof(mqtt_msg);
                mqtt_msg.tcp_id = mqttNewContext->tcp_id;
                mqtt_msg.ret = 1;
                applSendCmsInd(mqttNewContext->reqHandle, APPL_MQTT, APPL_MQTT_STAT_IND, primSize, (void *)&mqtt_msg);
                /* need reconnect */
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_83, P_INFO, 0, ".....find need reconnect when read packet.......");
                mqttNewContext = (mqtt_context *)mqttMsg.context_ptr;
                ret = MQTT_RECONNECT;
                break;

        }

        if(ret == MQTT_RECONNECT)
        {
            /*now the client maybe deleted already, so we need check it*/
            mqttNewContext = (mqtt_context *)mqttMsg.context_ptr;
            if(mqttNewContext->is_used == MQTT_CONTEXT_USED)
            {
                mqttNewContext->is_connected = MQTT_CONN_IS_CONNECTING;
                MutexLock(&mqtt_mutex1);
                ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_84, P_INFO, 0, ".....start reconnect mqtt server.......");
                len = mqttMsg.message.payloadlen;

                mqttNewContext->mqtt_client->isconnected = 0;
                ret = mqttReconnect((void *)mqttNewContext);

                /* send result to at task */
                if(ret == 0)
                {
                    mqttNewContext->is_connected = MQTT_CONN_CONNECTED;
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_85, P_INFO, 0, ".....reconnect mqtt server ok.......");
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_86, P_INFO, 0, ".....reconnect mqtt server fail.......");
                }
                mqttNewContext->reconnect_count++;
                MutexUnlock(&mqtt_mutex1);
            }
        }

        hasBusyClient = 0;
        for(i=0; i<MQTT_CONTEXT_NUMB_MAX; i++)
        {
            if(mqttContext[i].is_used == MQTT_CONTEXT_USED)
            {
                hasBusyClient = 1;
            }
            if((mqttContext[i].is_used == MQTT_CONTEXT_IS_CREATING)||(mqttContext[i].is_connected == MQTT_CONN_IS_CONNECTING))
            {
                hasBusyClient = 1;
                osDelay(400);
            }
        }

        if(hasBusyClient == 0)
        {
            break;
        }
    }

    vote_ret = slpManPlatVoteEnableSleep(mqttSlpHandler,SLP_SLP2_STATE);

    if(RET_TRUE != vote_ret)
    {
        ECOMM_TRACE(UNILOG_MQTT, mqttTaskSendProcess_2, P_INFO, 1, "...mqttTaskSendProcess apply vote fail, vote_ret=%d...",vote_ret);
    }

    vote_ret = slpManGivebackPlatVoteHandle(mqttSlpHandler);

    if(RET_TRUE != vote_ret)
    {
        ECOMM_TRACE(UNILOG_MQTT, mqttTaskSendProcess_3, P_INFO, 1, "...mqttTaskSendProcess apply vote fail, vote_ret=%d...",vote_ret);
    }
    mqtt_send_task_handle = NULL;

    mqtt_task_send_status_flag = MQTT_TASK_DELETE;
    osThreadExit();
}

int mqtt_recv_task_Init(void)
{
    osThreadAttr_t task_attr;

    memset(&task_attr, 0, sizeof(task_attr));
    task_attr.name = "mqttRecv";
    task_attr.stack_size = MQTT_RECV_TASK_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal5;

    if(mqtt_recv_task_handle == NULL)
    {
        mqtt_recv_task_handle = osThreadNew(mqttTaskRecvProcess, NULL,&task_attr);
        if(mqtt_recv_task_handle == NULL)
        {
            return MQTT_ERR;
        }
    }
    return MQTT_OK;
}

int mqtt_send_task_Init(void)
{
    osThreadAttr_t task_attr;

    if(mqtt_sempr_status_flag == MQTT_SEMPHR_NOT_CREATE)
    {
        mqtt_sempr_status_flag = MQTT_SEMPHR_HAVE_CREATE;
        if(mqtt_send_msg_handle == NULL)
        {
            mqtt_send_msg_handle = xQueueCreate(MQTT_SEMPHR_MAX_NUMB, sizeof(mqtt_send_msg));
        }
        if(mqtt_mutex1.sem == NULL)
        {
            MutexInit(&mqtt_mutex1);
        }
    }

    memset(&task_attr, 0, sizeof(task_attr));
    task_attr.name = "mqttSend";
    task_attr.stack_size = MQTT_SEND_TASK_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal6;

    if(mqtt_send_task_handle == NULL)
    {
        mqtt_send_task_handle = osThreadNew(mqttTaskSendProcess, NULL,&task_attr);
        if(mqtt_send_task_handle == NULL)
        {
            return MQTT_ERR;
        }
    }
    return MQTT_OK;
}
#define MQTT_CLIENT_AT_INTERFACE
int mqtt_client_config(int cnfType, int tcpId, mqtt_cfg_data *cfgData)
{
    mqtt_context *mqttNewContext = NULL;

    mqttNewContext = mqttFindContext(tcpId);

    if(mqttNewContext == NULL)
    {
        mqttNewContext = mqttCreateContext(tcpId, PNULL, MQTT_PORT_DEFAULT, MQTT_TX_BUF_DEFAULT, MQTT_RX_BUF_DEFAULT, MQTT_CONTEXT_CONFIGED);
    }

    if(mqttNewContext != NULL)
    {
        mqttConfigContext(cnfType,tcpId, cfgData);

        return MQTT_OK;
    }
    else
    {
        return MQTT_ERR;
    }

}

int mqtt_client_open(UINT32 reqHandle, int tcpId, char *mqttUri, int mqttPort)
{
    int ret = MQTT_OK;
    mqtt_context *mqttNewContext = NULL;
    NmAtiSyncRet netStatus;
    mqtt_send_msg mqttMsg;
    mqtt_cfg_data cfgData;

    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_11011, P_INFO, 0, ".....mqtt_client_open...NULL.0....");
    memset(&cfgData, 0, sizeof(cfgData));
    appGetNetInfoSync(0, &netStatus);
    ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_110111, P_INFO, 0, ".....mqtt_client_open...NULL.0....");
    if(netStatus.body.netInfoRet.netifInfo.netStatus != NM_NETIF_ACTIVATED)
    {
        return MQTT_NETWORK_ERR;
    }

    mqttNewContext = mqttFindContext(tcpId);

    if(mqttNewContext == NULL)
    {
        ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_110, P_INFO, 0, ".....mqtt_client_open...NULL.0....");
        mqttNewContext = mqttCreateContext(tcpId, mqttUri, mqttPort, MQTT_TX_BUF_DEFAULT, MQTT_RX_BUF_DEFAULT, MQTT_CONTEXT_OPENED);
        if(mqttNewContext == NULL)
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_111, P_INFO, 0, ".....mqtt_client_open...NULL.1....");
            return MQTT_CONTEXT_ERR;
        }
    }
    mqttNewContext->reqHandle = reqHandle;

    if(mqttNewContext != NULL)
    {
        ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_112, P_INFO, 1, ".....mqtt_client_open...%d....",mqttNewContext->is_connected);
        if((mqttNewContext->is_connected == MQTT_CONN_IS_OPENING)||(mqttNewContext->is_connected == MQTT_CONN_OPENED)
            ||(mqttNewContext->is_connected == MQTT_CONN_IS_CONNECTING)||(mqttNewContext->is_connected == MQTT_CONN_CONNECTED)
            ||(mqttNewContext->is_connected == MQTT_CONN_RECONNECTING))
        {
            return MQTT_OK;
        }
        cfgData.decParam1 = mqttPort;
        cfgData.strParam1 = mqttUri;
        mqttConfigContext(MQTT_CONFIG_OPEN, tcpId, &cfgData);
    }

    /* send open msg to mqtt send task */
    //mqttNewContext->is_connected = MQTT_CONN_IS_OPENING;
    if((mqtt_task_recv_status_flag == MQTT_TASK_DELETE)&&(mqtt_task_send_status_flag == MQTT_TASK_DELETE))
    {
        mqttCurrContext = mqttNewContext;
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_86, P_INFO, 0, "...mqtt_task start init..");
        if(mqtt_send_task_Init() != MQTT_OK)
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_task_87, P_INFO, 0, "...mqtt_task create fail...");
            if(mqtt_send_msg_handle != NULL)
            {
                xQueueReset(mqtt_send_msg_handle);
            }
            mqttDeleteContext(mqttNewContext);
            mqtt_recv_task_handle = NULL;
            mqtt_send_task_handle = NULL;
            mqtt_task_recv_status_flag = MQTT_TASK_DELETE;
            mqtt_task_send_status_flag = MQTT_TASK_DELETE;
            return MQTT_TASK_ERR;
        }
        else
        {
            if(mqtt_send_msg_handle != NULL)
            {
                xQueueReset(mqtt_send_msg_handle);
            }
            osDelay(1000);
            ECOMM_TRACE(UNILOG_MQTT, mqtt_task_88, P_INFO, 0, "...start send first create client msg...");
            mqttCurrContext = mqttNewContext;
            memset(&mqttMsg, 0, sizeof(mqttMsg));
            mqttMsg.cmd_type = MQTT_MSG_OPEN;
            mqttMsg.context_ptr = (void *)mqttNewContext;
            mqttMsg.reqhandle = reqHandle;
            mqttMsg.tcp_id = tcpId;
            xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
        }
    }
    else //if(mqtt_task_send_status_flag == MQTT_TASK_CREATE)
    {
        if(mqtt_send_msg_handle != NULL)
        {
            xQueueReset(mqtt_send_msg_handle);
        }
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_89, P_INFO, 0, "...start send create client msg...");
        mqttCurrContext = mqttNewContext;
        memset(&mqttMsg, 0, sizeof(mqttMsg));
        mqttMsg.cmd_type = MQTT_MSG_OPEN;
        mqttMsg.context_ptr = (void *)mqttNewContext;
        mqttMsg.reqhandle = reqHandle;
        mqttMsg.tcp_id = tcpId;
        
        xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
    }
    return ret;
}

int mqtt_client_close(UINT32 reqHandle, int tcpId)
{
    int ret = MQTT_OK;
    mqtt_context *mqttNewContext = NULL;
    mqtt_send_msg mqttMsg;

    mqttNewContext = mqttFindContext(tcpId);

    if(mqttNewContext == NULL)
    {
        return MQTT_CONTEXT_ERR;
    }
    else
    {
        if((mqttNewContext->is_connected != MQTT_CONN_CONNECTED)&&(mqttNewContext->is_connected != MQTT_CONN_OPENED))
        {
            return MQTT_CONTEXT_ERR;
        }
        //mqttNewContext->is_connected = MQTT_CONN_IS_CLOSING;
        memset(&mqttMsg, 0, sizeof(mqttMsg));
        mqttMsg.cmd_type = MQTT_MSG_CLOSE;
        mqttMsg.context_ptr = (void *)mqttNewContext;
        mqttMsg.reqhandle = reqHandle;
        mqttMsg.tcp_id = tcpId;

        xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
    }
    return ret;
}

int mqtt_client_connect(UINT32 reqHandle, int tcpId, char *clientId, char *userName, char *passWord)
{
    int ret = MQTT_OK;
    mqtt_context *mqttNewContext = NULL;
    mqtt_send_msg mqttMsg;
    int clientIDLen = 0;
    int userNameLen = 0;
    int passWordLen = 0;
    int hmacSourceLen = 0;
    char *hmac_source = NULL;
    char *ali_clientID = NULL;
    char *ali_username = NULL;
    char *ali_signature = NULL;

    mqttNewContext = mqttFindContext(tcpId);

    if(mqttNewContext == NULL)
    {
        ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_113, P_INFO, 0, ".....mqtt_client_connect...NULL....");
        return MQTT_CONTEXT_ERR;
    }
    else
    {
        mqttNewContext->reqHandle = reqHandle;
        if((mqttNewContext->is_connected == MQTT_CONN_IS_CONNECTING)||(mqttNewContext->is_connected == MQTT_CONN_CONNECTED)
            ||(mqttNewContext->is_connected == MQTT_CONN_RECONNECTING))
        {
            return MQTT_OK;
        }
        else if((mqttNewContext->is_connected == MQTT_CONN_DEFAULT)||(mqttNewContext->is_connected == MQTT_CONN_NOT_OPEN)
            ||(mqttNewContext->is_connected == MQTT_CONN_IS_CLOSING)||(mqttNewContext->is_connected == MQTT_CONN_IS_DISCONNECTING))
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_msg_114, P_INFO, 1, ".....mqtt_client_connect...%d....",mqttNewContext->is_connected);
            return MQTT_CONTEXT_ERR;
        }
        
        if(mqttNewContext->cloud_type == CLOUD_TYPE_ALI)
        {
            if(clientId == NULL)
            {
                return MQTT_CONTEXT_ERR;
            }
            if((mqttNewContext->aliAuth.device_name != NULL)&&(mqttNewContext->aliAuth.product_key != NULL))
            {
                if(ali_hmac_falg == ALI_HMAC_NOT_USED)
                {
                    return MQTT_ALI_ENCRYP_ERR;
                }
                mqttNewContext->cloud_type = CLOUD_TYPE_ALI;

                if(mqttNewContext->aliAuth.dynamic_register_used == ALI_DYNAMIC_REGISTER_IS_USED)
                {
                    if((mqttNewContext->aliAuth.device_name == NULL)||(mqttNewContext->aliAuth.product_key == NULL)||(mqttNewContext->aliAuth.auth_type == NULL)
                        ||(mqttNewContext->aliAuth.sign_method == NULL)||(mqttNewContext->aliAuth.auth_mode == NULL)||(mqttNewContext->aliAuth.secure_mode == NULL))
                    {
                        return MQTT_ALI_ENCRYP_ERR;
                    }

                    /*=======for client username password===================================*/
                    clientIDLen = strlen(mqttNewContext->aliAuth.product_name)+strlen(mqttNewContext->aliAuth.secure_mode)+strlen(mqttNewContext->aliAuth.auth_type)+strlen(ALI_RANDOM_TIMESTAMP)+strlen(mqttNewContext->aliAuth.sign_method)+64;
                    userNameLen = strlen(mqttNewContext->aliAuth.device_name)+strlen(mqttNewContext->aliAuth.product_key)+4;
                    hmacSourceLen = strlen(mqttNewContext->aliAuth.product_name)+strlen(mqttNewContext->aliAuth.device_name)+strlen(mqttNewContext->aliAuth.product_key)+strlen(ALI_RANDOM_TIMESTAMP)+64;                   
                    ali_clientID = malloc(clientIDLen);
                    ali_username = malloc(userNameLen);
                    ali_signature = malloc(96);
                    hmac_source = malloc(hmacSourceLen); 
                    memset(ali_clientID, 0, clientIDLen);
                    memset(ali_username, 0, userNameLen);
                    memset(ali_signature, 0, 96);
                    memset(hmac_source, 0, hmacSourceLen);
                    snprintf(hmac_source, hmacSourceLen, "deviceName%s" "productKey%s" "random%s", mqttNewContext->aliAuth.device_name, mqttNewContext->aliAuth.product_key, ALI_RANDOM_TIMESTAMP);
                    /*=======for client username password===================================*/
                    
                    #ifdef FEATURE_MBEDTLS_ENABLE
                    if(strcmp((CHAR*)mqttNewContext->aliAuth.sign_method, "hmacsha1") == 0)
                    {
                        atAliHmacSha1((unsigned char *)hmac_source, strlen(hmac_source), (unsigned char *)ali_signature,(unsigned char *)mqttNewContext->aliAuth.product_secret, strlen(mqttNewContext->aliAuth.product_secret));
                    }
                    else if(strcmp((CHAR*)mqttNewContext->aliAuth.sign_method, "hmacsha256") == 0)
                    {
                        atAliHmacSha256((unsigned char *)hmac_source, strlen(hmac_source), (unsigned char *)ali_signature,(unsigned char *)mqttNewContext->aliAuth.product_secret, strlen(mqttNewContext->aliAuth.product_secret));
                    }
                    else if(strcmp((CHAR*)mqttNewContext->aliAuth.sign_method, "hmacmd5") == 0)
                    {
                        atAliHmacMd5((unsigned char *)hmac_source, strlen(hmac_source), (unsigned char *)ali_signature,(unsigned char *)mqttNewContext->aliAuth.product_secret, strlen(mqttNewContext->aliAuth.product_secret));
                    }
                    else
                    {
                        free(ali_clientID);
                        free(ali_username);
                        free(ali_signature);
                        free(hmac_source);
                        return MQTT_ALI_ENCRYP_ERR;
                    }
                    #endif

                    ECOMM_STRING(UNILOG_MQTT, hh11, P_INFO, ".........hmac_source......:%s",(uint8_t*)hmac_source);
                    ECOMM_STRING(UNILOG_MQTT, hh12, P_INFO, ".........product_name......:%s",(uint8_t*)mqttNewContext->aliAuth.product_name);
                    ECOMM_STRING(UNILOG_MQTT, hh13, P_INFO, ".........product_key......:%s",(uint8_t*)mqttNewContext->aliAuth.product_key);
                    ECOMM_STRING(UNILOG_MQTT, hh14, P_INFO, ".........product_secret......:%s",(uint8_t*)mqttNewContext->aliAuth.product_secret);
                    ECOMM_STRING(UNILOG_MQTT, hh15, P_INFO, ".........device_name......:%s",(uint8_t*)mqttNewContext->aliAuth.device_name);
                    ECOMM_STRING(UNILOG_MQTT, hh16, P_INFO, ".........auth_type......:%s",(uint8_t*)mqttNewContext->aliAuth.auth_type);
                    ECOMM_STRING(UNILOG_MQTT, hh17, P_INFO, ".........sign_method......:%s",(uint8_t*)mqttNewContext->aliAuth.sign_method);
                    ECOMM_STRING(UNILOG_MQTT, hh18, P_INFO, ".........auth_mode......:%s",(uint8_t*)mqttNewContext->aliAuth.auth_mode);
                    ECOMM_STRING(UNILOG_MQTT, hh19, P_INFO, ".........secure_mode......:%s",(uint8_t*)mqttNewContext->aliAuth.secure_mode);
                    
                    free(hmac_source);
                    if(strcmp((CHAR*)mqttNewContext->aliAuth.auth_type, "register") == 0)
                    {
                        /*=======for client username password===================================*/
                        sprintf(ali_clientID,"%s|securemode=%s,authType=%s,random=%s,signmethod=%s|", mqttNewContext->aliAuth.product_name, mqttNewContext->aliAuth.secure_mode, mqttNewContext->aliAuth.auth_type, ALI_RANDOM_TIMESTAMP, mqttNewContext->aliAuth.sign_method);
                        sprintf(ali_username,"%s&%s",mqttNewContext->aliAuth.device_name,mqttNewContext->aliAuth.product_key);
                        /*=======for client username password===================================*/
                    }
                    else if(strcmp((CHAR*)mqttNewContext->aliAuth.auth_type, "regnwl") == 0)
                    {
                        if(mqttNewContext->aliAuth.instance_id != NULL)
                        {
                            /*=======for client username password===================================*/
                            //sprintf(ali_clientID,"%s|securemode=%s,authType=%s,random=%s,signmethod=%s,instanceId=%s|", mqttNewContext->aliAuth.product_name, mqttNewContext->aliAuth.secure_mode, mqttNewContext->aliAuth.auth_type, ALI_RANDOM_TIMESTAMP, mqttNewContext->aliAuth.sign_method, mqttNewContext->aliAuth.instance_id);
                            sprintf(ali_clientID,"%s|securemode=%s,authType=%s,random=%s,signmethod=%s|", mqttNewContext->aliAuth.product_name, mqttNewContext->aliAuth.secure_mode, mqttNewContext->aliAuth.auth_type, ALI_RANDOM_TIMESTAMP, mqttNewContext->aliAuth.sign_method);
                            sprintf(ali_username,"%s&%s",mqttNewContext->aliAuth.device_name,mqttNewContext->aliAuth.product_key);
                            /*=======for client username password===================================*/
                        }
                    }
                }
                else
                {
                    if(mqttNewContext->aliAuth.device_secret == NULL)
                    {
                        return MQTT_ALI_ENCRYP_ERR;
                    }
                    clientIDLen = strlen(clientId)+strlen(ALI_RANDOM_TIMESTAMP)+64;
                    userNameLen = strlen(mqttNewContext->aliAuth.device_name)+strlen(mqttNewContext->aliAuth.product_key)+4;
                    hmacSourceLen = strlen(clientId)+strlen(mqttNewContext->aliAuth.device_name)+strlen(mqttNewContext->aliAuth.product_key)+strlen(ALI_RANDOM_TIMESTAMP)+64;
                   
                    ali_clientID = malloc(clientIDLen);
                    ali_username = malloc(userNameLen);
                    ali_signature = malloc(96);
                    hmac_source = malloc(hmacSourceLen); 
                    
                    memset(ali_clientID, 0, clientIDLen);
                    memset(ali_username, 0, userNameLen);
                    memset(ali_signature, 0, 96);
                    memset(hmac_source, 0, hmacSourceLen);
                    snprintf(hmac_source, hmacSourceLen, "clientId%s" "deviceName%s" "productKey%s" "timestamp%s", clientId, mqttNewContext->aliAuth.device_name, mqttNewContext->aliAuth.product_key, ALI_RANDOM_TIMESTAMP);
                    
                    #ifdef FEATURE_MBEDTLS_ENABLE
                    atAliHmacSha1((unsigned char *)hmac_source, strlen(hmac_source), (unsigned char *)ali_signature,(unsigned char *)mqttNewContext->aliAuth.device_secret, strlen(mqttNewContext->aliAuth.device_secret));
                    #endif
                    free(hmac_source);
                    
                    sprintf(ali_clientID,"%s|securemode=3,signmethod=hmacsha1,timestamp=%s|", clientId, ALI_RANDOM_TIMESTAMP);
                    sprintf(ali_username,"%s&%s",mqttNewContext->aliAuth.device_name,mqttNewContext->aliAuth.product_key);
                }
                mqttNewContext->mqtt_connect_data.clientID.cstring = ali_clientID;
                mqttNewContext->mqtt_connect_data.username.cstring = ali_username;
                mqttNewContext->mqtt_connect_data.password.cstring = ali_signature;
            }
        }
        else
        {
            if((clientId != NULL)&&(userName != NULL)&&(passWord != NULL))
            {
                //mqttNewContext->cloud_type = cloudType;
                userNameLen = (strlen(userName)+1);
                clientIDLen = (strlen(clientId)+1);
                passWordLen = (strlen(passWord)+1);

                mqttNewContext->mqtt_connect_data.clientID.cstring = malloc(clientIDLen);
                mqttNewContext->mqtt_connect_data.username.cstring = malloc(userNameLen);
                mqttNewContext->mqtt_connect_data.password.cstring = malloc(passWordLen);
                memset(mqttNewContext->mqtt_connect_data.clientID.cstring, 0, clientIDLen);
                memset(mqttNewContext->mqtt_connect_data.username.cstring, 0, userNameLen);
                memset(mqttNewContext->mqtt_connect_data.password.cstring, 0, passWordLen);

                memcpy(mqttNewContext->mqtt_connect_data.username.cstring, userName, (userNameLen-1));
                memcpy(mqttNewContext->mqtt_connect_data.clientID.cstring, clientId, (clientIDLen-1));
                memcpy(mqttNewContext->mqtt_connect_data.password.cstring, passWord, (passWordLen-1));
            }
        }

        ECOMM_STRING(UNILOG_MQTT, hh1, P_INFO, ".........clientID......:%s",(uint8_t*)mqttNewContext->mqtt_connect_data.clientID.cstring);
        ECOMM_STRING(UNILOG_MQTT, hh2, P_INFO, ".........username......:%s",(uint8_t*)mqttNewContext->mqtt_connect_data.username.cstring);
        ECOMM_STRING(UNILOG_MQTT, hh3, P_INFO, ".........password......:%s",(uint8_t*)mqttNewContext->mqtt_connect_data.password.cstring);

        memset(&mqttMsg, 0, sizeof(mqttMsg));
        mqttMsg.cmd_type = MQTT_MSG_CONNECT;
        mqttMsg.context_ptr = (void *)mqttNewContext;
        mqttMsg.reqhandle = reqHandle;
        mqttMsg.tcp_id = tcpId;

        xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
    }
    return ret;
}

int mqtt_client_disconnect(UINT32 reqHandle, int tcpId)
{
    int ret = MQTT_OK;
    mqtt_context *mqttNewContext = NULL;
    mqtt_send_msg mqttMsg;

    mqttNewContext = mqttFindContext(tcpId);

    if(mqttNewContext == NULL)
    {
        return MQTT_CONTEXT_ERR;
    }
    else
    {
        if((mqttNewContext->is_connected != MQTT_CONN_CONNECTED)&&(mqttNewContext->is_connected != MQTT_CONN_OPENED)
            &&(mqttNewContext->is_connected != MQTT_CONN_CLOSED))
        {
            return MQTT_CONTEXT_ERR;
        }
        //mqttNewContext->is_connected = MQTT_CONN_IS_DISCONNECTING;
        memset(&mqttMsg, 0, sizeof(mqttMsg));
        mqttMsg.cmd_type = MQTT_MSG_DISCONNECT;
        mqttMsg.context_ptr = (void *)mqttNewContext;
        mqttMsg.reqhandle = reqHandle;
        mqttMsg.tcp_id = tcpId;

        xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
    }
    return ret;
}
int mqtt_client_sub(UINT32 reqHandle, int tcpId, int msgId, char *mqttSubTopic, int qos)
{
    int topicLen = 0;
    mqtt_send_msg mqttMsg;
    mqtt_context *mqttContext = mqttFindContext(tcpId);

    if((mqttContext == NULL)||(mqttContext->is_used == MQTT_CONTEXT_NOT_USED)||(mqttSubTopic == NULL))
    {
        return MQTT_CLIENT_ERR;
    }
    if((mqttContext->is_connected != MQTT_CONN_CONNECTED)||(mqttContext->mqtt_client->isconnected == 0))
    {
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_90, P_INFO, 2, "...mqtt is not connected...%d..%d.....\r\n",mqttContext->is_connected, mqttContext->mqtt_client->isconnected);
        return MQTT_MQTT_CONN_ERR;
    }
    mqttContext->reqHandle = reqHandle;

    memset(&mqttMsg, 0, sizeof(mqttMsg));

    topicLen = strlen(mqttSubTopic)+1;
    mqttMsg.sub_topic = malloc(topicLen);
    memset(mqttMsg.sub_topic, 0, topicLen);
    memcpy(mqttMsg.sub_topic, mqttSubTopic, (topicLen-1));
    mqttMsg.qos = qos;

    mqttMsg.cmd_type = MQTT_MSG_SUB;
    mqttMsg.tcp_id = tcpId;
    mqttMsg.msg_id = msgId;
    mqttMsg.context_ptr = (void *)mqttContext;
    mqttMsg.reqhandle = reqHandle;

    xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
    return MQTT_OK;
}

int mqtt_client_unsub(UINT32 reqHandle, int tcpId, int msgId, char *mqttSubTopic)
{
    int topicLen = 0;
    mqtt_send_msg mqttMsg;
    mqtt_context *mqttContext = mqttFindContext(tcpId);

    if((mqttContext == NULL)||(mqttContext->is_used == MQTT_CONTEXT_NOT_USED)||(mqttSubTopic == NULL))
    {
        return MQTT_CLIENT_ERR;
    }
    if((mqttContext->is_connected != MQTT_CONN_CONNECTED)||(mqttContext->mqtt_client->isconnected == 0))
    {
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_91, P_INFO, 2, "...mqtt is not connected...%d..%d.....\r\n",mqttContext->is_connected, mqttContext->mqtt_client->isconnected);
        return MQTT_MQTT_CONN_ERR;
    }
    mqttContext->reqHandle = reqHandle;

    memset(&mqttMsg, 0, sizeof(mqttMsg));

    topicLen = strlen(mqttSubTopic)+1;
    mqttMsg.unsub_topic = malloc(topicLen);
    memset(mqttMsg.unsub_topic, 0, topicLen);
    memcpy(mqttMsg.unsub_topic, mqttSubTopic, (topicLen-1));

    mqttMsg.cmd_type = MQTT_MSG_UNSUB;
    mqttMsg.tcp_id = tcpId;
    mqttMsg.msg_id = msgId;
    mqttMsg.context_ptr = (void *)mqttContext;
    mqttMsg.reqhandle = reqHandle;

    xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
    return MQTT_OK;
}

int mqtt_client_pub(UINT32 reqHandle, int tcpId, int msgId, int qos, int retained, int pubMode, char *mqttPubTopic, int msgLen, char *mqttMessage, int cloudType, int msgType, int rai)
{
    int pubTopicLen = 0;
    mqtt_send_msg mqttMsg;
    mqtt_context *mqttContext = mqttFindContext(tcpId);    
    #ifdef FEATURE_CJSON_ENABLE
    cJSON * payloadDataPtr = NULL;
    #endif
    int payloadLen = 0;
    int payloadAllLen = 0;
    char *mqttMessagePtr = NULL;
    unsigned char *tempPtr = NULL;
    unsigned char *ptr = NULL;

    if((mqttContext == NULL)||(mqttContext->is_used == MQTT_CONTEXT_NOT_USED)||(mqttPubTopic == NULL))
    {
        return MQTT_CLIENT_ERR;
    }
    if((mqttContext->is_connected != MQTT_CONN_CONNECTED)||(mqttContext->mqtt_client->isconnected == 0))
    {
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_92, P_INFO, 2, "...mqtt is not connected...%d..%d.....\r\n",mqttContext->is_connected, mqttContext->mqtt_client->isconnected);
        return MQTT_MQTT_CONN_ERR;
    }
    mqttContext->reqHandle = reqHandle;
    
    if(pubMode == MQTT_PUB_AT)
    {    
        memset(&mqttMsg, 0, sizeof(mqttMsg));
        mqttMsg.message.qos      = (enum QoS)qos;
        mqttMsg.message.retained = retained;
        
        if(mqttContext->cloud_type == CLOUD_TYPE_ONENET)
        {
            if(mqttMessage != NULL)
            {
                payloadLen = strlen(mqttMessage);
                if(mqttMessage[0] == '"')
                {
                    if(mqttMessage[payloadLen-1] == '"')
                    {
                        mqttMessage[payloadLen-1] = 0;
                    }
                    mqttMessagePtr = &mqttMessage[1];
                }
                else
                {
                    mqttMessagePtr = mqttMessage;
                }
                if((mqttContext->payloadType == ONENET_DATA_TYPE1)||(mqttContext->payloadType == ONENET_DATA_TYPE3)||(mqttContext->payloadType == ONENET_DATA_TYPE4))
                {
                    #ifdef FEATURE_CJSON_ENABLE
                    payloadDataPtr = cJSON_Parse(mqttMessagePtr);
                    if(payloadDataPtr == NULL)
                    {
                        return MQTT_PARAM_ERR;
                    }
                    else
                    {
                        cJSON_Delete(payloadDataPtr);
                    }
                    #endif
                }
                else if((mqttContext->payloadType < ONENET_DATA_TYPE1)||(mqttContext->payloadType > ONENET_DATA_TYPE9))
                {
                    return MQTT_PARAM_ERR;
                }                        
                payloadLen = strlen(mqttMessagePtr);
            }
            pubTopicLen = strlen(mqttPubTopic);
            
            /*set cmd*/
            mqttMsg.cmd_type = MQTT_MSG_PUBLISH;
            mqttMsg.tcp_id = tcpId;
            mqttMsg.msg_id = msgId;
            mqttMsg.pub_mode = pubMode;
            mqttMsg.rai = rai;
            
            /*set topic*/
            mqttMsg.topic       = malloc(pubTopicLen+1);
            memset(mqttMsg.topic, 0 , pubTopicLen+1);
            memcpy(mqttMsg.topic, mqttPubTopic, pubTopicLen);
            
            /*set payload*/
            if(payloadLen > 0)
            {
                if(mqttContext->payloadType == ONENET_DATA_TYPE2)
                {
                    /*02 00 payloadLen1 { "ds_id":"xx","at":"xxxx-xx-xx xx:xx:xx","desc":"xx"} 00 00 00 payloadLen2 {"xx":xx} */
                    //AT+ECMTPUB=0,0,0,0,"$dp","{"ds_id":"hh","at":"2018-08-08 09:09:09","desc":"tt"}0000{"ec":33}"                 
                    /*========================
                      0x02: data type2
                      payloadLen1: strlen({ "ds_id":"xx","at":"xxx-xx-xx xx:xx:xx","desc":"xx"})
                      payloadLen2: strlen({"xx":xx})
                    ==========================*/                    
                    if(strstr(mqttMessagePtr,"ds_id") != NULL)
                    {                        
                        mqttMsg.message.payload       = malloc(payloadLen+8);
                        memset(mqttMsg.message.payload, 0 , payloadLen+8);
                        sprintf(mqttMsg.message.payload, "%c%c%c%s",mqttContext->payloadType, mqttContext->payloadType, mqttContext->payloadType, mqttMessagePtr);
                        
                        tempPtr = (unsigned char *)strstr(mqttMsg.message.payload,"}");
                        payloadAllLen = strlen(mqttMsg.message.payload);
                        ptr = (unsigned char *)mqttMsg.message.payload;
                        writeChar(&ptr,mqttContext->payloadType);
                        //writeInt(&ptr,payloadLen);
                        tempPtr = tempPtr+1;
                        writeChar(&tempPtr,0);
                        writeChar(&tempPtr,0);
                        writeChar(&tempPtr,0);
                        payloadLen = strlen((char *)(tempPtr+1));
                        writeChar(&tempPtr,payloadLen);
                        payloadLen = strlen(mqttMsg.message.payload);
                        payloadLen = payloadLen-3; 
                        writeInt(&ptr,payloadLen);
                        mqttMsg.message.payloadlen = payloadAllLen;                         
                    }
                    else
                    {
                        return MQTT_PARAM_ERR;
                    }
                }   
                else if((mqttContext->payloadType == ONENET_DATA_TYPE1)||((mqttContext->payloadType > ONENET_DATA_TYPE2)&&(mqttContext->payloadType < ONENET_DATA_TYPE8)))
                {
                    mqttMsg.message.payload       = malloc(payloadLen+8);
                    memset(mqttMsg.message.payload, 0 , payloadLen+8);
                    sprintf(mqttMsg.message.payload, "%c%c%c%s",mqttContext->payloadType, mqttContext->payloadType, mqttContext->payloadType, mqttMessagePtr);
                    payloadAllLen = strlen(mqttMsg.message.payload);
                    ptr = (unsigned char *)mqttMsg.message.payload;
                    writeChar(&ptr,mqttContext->payloadType);
                    writeInt(&ptr,payloadLen);
                    mqttMsg.message.payloadlen = payloadAllLen;
                }
                else
                {
                    if(payloadLen > 0)
                    {
                        mqttMsg.message.payload       = malloc(payloadLen+8);
                        memset(mqttMsg.message.payload, 0 , payloadLen+8);
                        memcpy(mqttMsg.message.payload, mqttMessagePtr, payloadLen);
                        mqttMsg.message.payloadlen = payloadLen;
                    }
                }
            }

            mqttMsg.context_ptr = (void *)mqttContext;
            mqttMsg.reqhandle = reqHandle;
            ECOMM_TRACE(UNILOG_MQTT, mqtt_task_93, P_INFO, 2, "...mqtt send pub msg...0x%x..0x%x.....\r\n",mqttMsg.topic, msgLen);
        }
        else if(mqttContext->cloud_type == CLOUD_TYPE_ALI)
        {
            if(mqttMessage != NULL)
            {
                payloadLen = strlen(mqttMessage);
                if(mqttMessage[0] == '"')
                {
                    if(mqttMessage[payloadLen-1] == '"')
                    {
                        mqttMessage[payloadLen-1] = 0;
                    }
                    mqttMessagePtr = &mqttMessage[1];
                }
                else
                {
                    mqttMessagePtr = mqttMessage;
                }
                if(mqttContext->payloadType == MQTT_DATA_JSON)
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_task_93jj, P_INFO, 2, "...mqtt send pub msg...0x%x..0x%x.....\r\n",mqttMsg.topic, msgLen);
                    #ifdef FEATURE_CJSON_ENABLE
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_task_93kk, P_INFO, 2, "...mqtt send pub msg...0x%x..0x%x.....\r\n",mqttMsg.topic, msgLen);
                    payloadDataPtr = cJSON_Parse(mqttMessagePtr);
                    if(payloadDataPtr == NULL)
                    {
                        return MQTT_PARAM_ERR;
                    }
                    else
                    {
                        cJSON_Delete(payloadDataPtr);
                    }
                    #endif
                }
                payloadLen = strlen(mqttMessagePtr);
            }
            pubTopicLen = strlen(mqttPubTopic);
            
            /*set cmd*/
            mqttMsg.cmd_type = MQTT_MSG_PUBLISH;
            mqttMsg.tcp_id = tcpId;
            mqttMsg.msg_id = msgId;
            mqttMsg.pub_mode = pubMode;
            mqttMsg.rai = rai;
            
            /*set topic*/
            mqttMsg.topic       = malloc(pubTopicLen+1);
            memset(mqttMsg.topic, 0 , pubTopicLen+1);
            memcpy(mqttMsg.topic, mqttPubTopic, pubTopicLen);
            
            /*set payload*/
            if(payloadLen > 0)
            {
                mqttMsg.message.payload       = malloc(payloadLen+8);
                memset(mqttMsg.message.payload, 0 , payloadLen+8);
                memcpy(mqttMsg.message.payload, mqttMessagePtr, payloadLen);
                mqttMsg.message.payloadlen = payloadLen;
            }
            
            mqttMsg.context_ptr = (void *)mqttContext;
            mqttMsg.reqhandle = reqHandle;
            ECOMM_TRACE(UNILOG_MQTT, mqtt_task_94, P_INFO, 2, "...mqtt send pub msg...0x%x..0x%x.....\r\n",mqttMsg.topic, msgLen);
        }
        else
        {
            if(mqttMessage != NULL)
            {
                payloadLen = strlen(mqttMessage);
                if(mqttMessage[0] == '"')
                {
                    if(mqttMessage[payloadLen-1] == '"')
                    {
                        mqttMessage[payloadLen-1] = 0;
                    }
                    mqttMessagePtr = &mqttMessage[1];
                }
                else
                {
                    mqttMessagePtr = mqttMessage;
                }
                payloadLen = strlen(mqttMessagePtr);
            }
            pubTopicLen = strlen(mqttPubTopic);

            /*set cmd*/
            mqttMsg.cmd_type = MQTT_MSG_PUBLISH;
            mqttMsg.tcp_id = tcpId;
            mqttMsg.msg_id = msgId;
            mqttMsg.pub_mode = pubMode;
            mqttMsg.rai = rai;
            
            /*set topic*/
            mqttMsg.topic       = malloc(pubTopicLen+1);
            memset(mqttMsg.topic, 0 , pubTopicLen+1);
            memcpy(mqttMsg.topic, mqttPubTopic, pubTopicLen);

            /*set payload*/
            if(payloadLen > 0)
            {
                mqttMsg.message.payload       = malloc(payloadLen+8);
                memset(mqttMsg.message.payload, 0 , payloadLen+8);
                memcpy(mqttMsg.message.payload, mqttMessagePtr, payloadLen);
                mqttMsg.message.payloadlen = payloadLen;
            }
            mqttMsg.context_ptr = (void *)mqttContext;
            mqttMsg.reqhandle = reqHandle;
            ECOMM_TRACE(UNILOG_MQTT, mqtt_task_95, P_INFO, 2, "...mqtt send pub msg...0x%x..0x%x.....\r\n",mqttMsg.topic, msgLen);
        }
       
        xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
    }
    else if(pubMode == MQTT_PUB_CTRLZ)
    {
        memset(&mqttMsg, 0, sizeof(mqttMsg));
        mqttMsg.message.qos      = (enum QoS)qos;
        mqttMsg.message.retained = retained;
        
        payloadLen = msgLen;
        mqttMessagePtr = mqttMessage;
        pubTopicLen = strlen(mqttPubTopic);
        
        /*set cmd*/
        mqttMsg.cmd_type = MQTT_MSG_PUBLISH;
        mqttMsg.tcp_id = tcpId;
        mqttMsg.msg_id = msgId;
        mqttMsg.pub_mode = pubMode;
        mqttMsg.rai = rai;
        
        /*set topic*/
        mqttMsg.topic       = malloc(pubTopicLen+1);
        memset(mqttMsg.topic, 0 , pubTopicLen+1);
        memcpy(mqttMsg.topic, mqttPubTopic, pubTopicLen);

        /*set payload*/
        if(payloadLen > 0)
        {
            mqttMsg.message.payload       = malloc(payloadLen+1);
            memset(mqttMsg.message.payload, 0 , payloadLen+1);
            memcpy(mqttMsg.message.payload, mqttMessagePtr, payloadLen);
            mqttMsg.message.payloadlen = payloadLen;
        }
        
        mqttMsg.context_ptr = (void *)mqttContext;
        mqttMsg.reqhandle = reqHandle;
        ECOMM_TRACE(UNILOG_MQTT, mqtt_task_99, P_INFO, 2, "...mqtt send pub msg...0x%x..0x%x.....\r\n",mqttMsg.topic, msgLen);
        
        xQueueSend(mqtt_send_msg_handle, &mqttMsg, MQTT_MSG_TIMEOUT);
    }
    return MQTT_OK;
}









