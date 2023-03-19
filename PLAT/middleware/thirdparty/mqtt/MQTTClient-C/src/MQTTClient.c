/*******************************************************************************
 * Copyright (c) 2014, 2017 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *   Allan Stockdill-Mander/Ian Craggs - initial API and implementation and/or initial documentation
 *   Ian Craggs - fix for #96 - check rem_len in readPacket
 *   Ian Craggs - add ability to set message handler separately #6
 *******************************************************************************/
#include "MQTTClient.h"
#ifdef FEATURE_MBEDTLS_ENABLE
#include "sha1.h"
#include "sha256.h"
#include "md5.h"
#endif

char *mqttSendbuf = NULL;
char *mqttReadbuf = NULL;

unsigned char mqttJsonbuff[128];
char mqtt_payload[128];
int ec_sensor_temp = 20;
char ec_data_type = 3;
int ec_data_len = 0;
QueueHandle_t mqttRecvMsgHandle = NULL;
QueueHandle_t mqttSendMsgHandle = NULL;
QueueHandle_t appMqttMsgHandle = NULL;

osThreadId_t mqttRecvTaskHandle = NULL;
osThreadId_t mqttSendTaskHandle = NULL;
osThreadId_t appMqttTaskHandle = NULL;

Mutex mqttMutex1;
Mutex mqttMutex2;
MQTTClient mqttClient;
Network mqttNetwork;
int mqtt_send_task_status_flag = 0;
int mqtt_keepalive_retry_count = 0;
#ifdef FEATURE_MBEDTLS_ENABLE
char mqttHb2Hex(unsigned char hb)
{
    hb = hb&0xF;
    return (char)(hb<10 ? '0'+hb : hb-10+'a');
}

/*
 * output = SHA-1( input buffer )
 */
void mqttAliHmacSha1(const unsigned char *input, int ilen, unsigned char *output,const unsigned char *key, int keylen)
{
    int i;
    mbedtls_sha1_context ctx;
    unsigned char k_ipad[ALI_SHA1_KEY_IOPAD_SIZE] = {0};
    unsigned char k_opad[ALI_SHA1_KEY_IOPAD_SIZE] = {0};
    unsigned char tempbuf[ALI_SHA1_DIGEST_SIZE];

    memset(k_ipad, 0x36, ALI_SHA1_KEY_IOPAD_SIZE);
    memset(k_opad, 0x5C, ALI_SHA1_KEY_IOPAD_SIZE);

    for(i=0; i<keylen; i++)
    {
        if(i>=ALI_SHA1_KEY_IOPAD_SIZE)
        {
            break;
        }
        k_ipad[i] ^=key[i];
        k_opad[i] ^=key[i];
    }
    mbedtls_sha1_init(&ctx);

    mbedtls_sha1_starts(&ctx);
    mbedtls_sha1_update(&ctx, k_ipad, ALI_SHA1_KEY_IOPAD_SIZE);
    mbedtls_sha1_update(&ctx, input, ilen);
    mbedtls_sha1_finish(&ctx, tempbuf);

    mbedtls_sha1_starts(&ctx);
    mbedtls_sha1_update(&ctx, k_opad, ALI_SHA1_KEY_IOPAD_SIZE);
    mbedtls_sha1_update(&ctx, tempbuf, ALI_SHA1_DIGEST_SIZE);
    mbedtls_sha1_finish(&ctx, tempbuf);

    for(i=0; i<ALI_SHA1_DIGEST_SIZE; ++i)
    {
        output[i*2] = mqttHb2Hex(tempbuf[i]>>4);
        output[i*2+1] = mqttHb2Hex(tempbuf[i]);
    }
    mbedtls_sha1_free(&ctx);
}
/*
 * output = SHA-256( input buffer )
 */
void mqttAliHmacSha256(const unsigned char *input, int ilen, unsigned char *output,const unsigned char *key, int keylen)
{
    int i;
    mbedtls_sha256_context ctx;
    unsigned char k_ipad[ALI_SHA256_KEY_IOPAD_SIZE] = {0};
    unsigned char k_opad[ALI_SHA256_KEY_IOPAD_SIZE] = {0};

    memset(k_ipad, 0x36, 64);
    memset(k_opad, 0x5C, 64);

    if ((NULL == input) || (NULL == key) || (NULL == output)) {
        return;
    }

    if (keylen > ALI_SHA256_KEY_IOPAD_SIZE) {
        return;
    }

    for(i=0; i<keylen; i++)
    {
        if(i>=ALI_SHA256_KEY_IOPAD_SIZE)
        {
            break;
        }
        k_ipad[i] ^=key[i];
        k_opad[i] ^=key[i];
    }
    mbedtls_sha256_init(&ctx);

    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, k_ipad, ALI_SHA256_KEY_IOPAD_SIZE);
    mbedtls_sha256_update(&ctx, input, ilen);
    mbedtls_sha256_finish(&ctx, output);

    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, k_opad, ALI_SHA256_KEY_IOPAD_SIZE);
    mbedtls_sha256_update(&ctx, output, ALI_SHA256_DIGEST_SIZE);
    mbedtls_sha256_finish(&ctx, output);

    mbedtls_sha256_free(&ctx);
}
/*
 * output = MD-5( input buffer )
 */
void mqttAliHmacMd5(const unsigned char *input, int ilen, unsigned char *output,const unsigned char *key, int keylen)
{
    int i;
    mbedtls_md5_context ctx;
    unsigned char k_ipad[ALI_MD5_KEY_IOPAD_SIZE] = {0};
    unsigned char k_opad[ALI_MD5_KEY_IOPAD_SIZE] = {0};
    unsigned char tempbuf[ALI_MD5_DIGEST_SIZE];

    memset(k_ipad, 0x36, ALI_MD5_KEY_IOPAD_SIZE);
    memset(k_opad, 0x5C, ALI_MD5_KEY_IOPAD_SIZE);

    for(i=0; i<keylen; i++)
    {
        if(i>=ALI_MD5_KEY_IOPAD_SIZE)
        {
            break;
        }
        k_ipad[i] ^=key[i];
        k_opad[i] ^=key[i];
    }
    mbedtls_md5_init(&ctx);

    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, k_ipad, ALI_MD5_KEY_IOPAD_SIZE);
    mbedtls_md5_update(&ctx, input, ilen);
    mbedtls_md5_finish(&ctx, tempbuf);

    mbedtls_md5_starts(&ctx);
    mbedtls_md5_update(&ctx, k_opad, ALI_MD5_KEY_IOPAD_SIZE);
    mbedtls_md5_update(&ctx, tempbuf, ALI_MD5_DIGEST_SIZE);
    mbedtls_md5_finish(&ctx, tempbuf);

    for(i=0; i<ALI_MD5_DIGEST_SIZE; ++i)
    {
        output[i*2] = mqttHb2Hex(tempbuf[i]>>4);
        output[i*2+1] = mqttHb2Hex(tempbuf[i]);
    }
    mbedtls_md5_free(&ctx);
}
#endif

void mqttDefMessageArrived(MessageData* data)
{
    char *bufTemp = NULL;
    bufTemp = malloc(data->message->payloadlen+1);
    memset(bufTemp, 0, data->message->payloadlen+1);
    memcpy(bufTemp, data->message->payload, data->message->payloadlen);
    ECOMM_STRING(UNILOG_MQTT, mqttRecvTask_2, P_SIG, ".........MQTT topic is:%s", (const uint8_t *)data->topicName->lenstring.data);
    ECOMM_STRING(UNILOG_MQTT, mqttRecvTask_1, P_SIG, ".........MQTT_messageArrived is:%s", (const uint8_t *)bufTemp);
    free(bufTemp);
}

static void NewMessageData(MessageData* md, MQTTString* aTopicName, MQTTMessage* aMessage) {
    md->topicName = aTopicName;
    md->message = aMessage;
}


static int getNextPacketId(MQTTClient *c) {
    return c->next_packetid = (c->next_packetid == MAX_PACKET_ID) ? 1 : c->next_packetid + 1;
}

static int sendPacket(MQTTClient* c, int length, Timer* timer)
{
    int rc = FAILURE,
        sent = 0;

    while (sent < length && !TimerIsExpired(timer))
    {
        #ifdef MQTT_RAI_OPTIMIZE
        rc = c->ipstack->mqttwrite(c->ipstack, &c->buf[sent], length, TimerLeftMS(timer), 0, false);
        #else
        rc = c->ipstack->mqttwrite(c->ipstack, &c->buf[sent], length, TimerLeftMS(timer));
        #endif
        if (rc < 0)  // there was an error writing the data
            break;
        sent += rc;
    }
    if (sent == length)
    {
        TimerCountdown(&c->last_sent, c->keepAliveInterval); // record the fact that we have successfully sent the packet
        rc = SUCCESS;
    }
    else
        rc = FAILURE;
    return rc;
}

void MQTTClientInit(MQTTClient* c, Network* network, unsigned int command_timeout_ms,
        unsigned char* sendbuf, size_t sendbuf_size, unsigned char* readbuf, size_t readbuf_size)
{
    int i;
    c->ipstack = network;

    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
        c->messageHandlers[i].topicFilter = 0;
    c->command_timeout_ms = command_timeout_ms;
    c->buf = sendbuf;
    c->buf_size = sendbuf_size;
    c->readbuf = readbuf;
    c->readbuf_size = readbuf_size;
    c->isconnected = 0;
    c->cleansession = 0;
    c->ping_outstanding = 0;
    c->defaultMessageHandler = mqttDefMessageArrived;
      c->next_packetid = 1;
    TimerInit(&c->last_sent);
    TimerInit(&c->last_received);
#if defined(MQTT_TASK)
      MutexInit(&c->mutex);
#endif
}

static int decodePacket(MQTTClient* c, int* value, int timeout)
{
    unsigned char i;
    int multiplier = 1;
    int len = 0;
    const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

    *value = 0;
    do
    {
        int rc = MQTTPACKET_READ_ERROR;

        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
        {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            goto exit;
        }
        rc = c->ipstack->mqttread(c->ipstack, &i, 1, timeout);
        if (rc != 1)
            goto exit;
        *value += (i & 127) * multiplier;
        multiplier *= 128;
    } while ((i & 128) != 0);
exit:
    return len;
}

static int readPacket(MQTTClient* c, Timer* timer)
{
    MQTTHeader header = {0};
    int len = 0;
    int rem_len = 0;

    /* 1. read the header byte.  This has the packet type in it */
    int rc = c->ipstack->mqttread(c->ipstack, c->readbuf, 1, TimerLeftMS(timer));
    if (rc != 1)
        goto exit;

    len = 1;
    /* 2. read the remaining length.  This is variable in itself */
    decodePacket(c, &rem_len, TimerLeftMS(timer));
    len += MQTTPacket_encode(c->readbuf + 1, rem_len); /* put the original remaining length back into the buffer */

    if (rem_len > (c->readbuf_size - len))
    {
        rc = BUFFER_OVERFLOW;
        goto exit;
    }

    /* 3. read the rest of the buffer using a callback to supply the rest of the data */
    if (rem_len > 0 && (c->ipstack->mqttread(c->ipstack, c->readbuf + len, rem_len, TimerLeftMS(timer)) != rem_len)) {
        rc = 0;
        goto exit;
    }

    header.byte = c->readbuf[0];
    rc = header.bits.type;
    if (c->keepAliveInterval > 0)
        TimerCountdown(&c->last_received, c->keepAliveInterval); // record the fact that we have successfully received a packet
exit:
    return rc;
}

// assume topic filter and name is in correct format
// # can only be at end
// + and # can only be next to separator
static char isTopicMatched(char* topicFilter, MQTTString* topicName)
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

int deliverMessage(MQTTClient* c, MQTTString* topicName, MQTTMessage* message)
{
    int i;
    int rc = FAILURE;
    ECOMM_TRACE(UNILOG_MQTT, deliverMessage1, P_SIG, 0, "....1....deliverMessage..");

    // we have to find the right message handler - indexed by topic
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        if (c->messageHandlers[i].topicFilter != 0 && (MQTTPacket_equals(topicName, (char*)c->messageHandlers[i].topicFilter) ||
                isTopicMatched((char*)c->messageHandlers[i].topicFilter, topicName)))
        {
            if (c->messageHandlers[i].fp != NULL)
            {
                MessageData md;
                NewMessageData(&md, topicName, message);
                c->messageHandlers[i].fp(&md);
                rc = SUCCESS;
                ECOMM_TRACE(UNILOG_MQTT, deliverMessage2, P_SIG, 0, "....2....deliverMessage..");
            }
        }
    }

    if (rc == FAILURE && c->defaultMessageHandler != NULL)
    {
        MessageData md;
        ECOMM_TRACE(UNILOG_MQTT, deliverMessage3, P_SIG, 0, "....3....deliverMessage..");
        NewMessageData(&md, topicName, message);
        c->defaultMessageHandler(&md);
        rc = SUCCESS;
        ECOMM_TRACE(UNILOG_MQTT, deliverMessage4, P_SIG, 0, "....4....deliverMessage..");
    }

    return rc;
}

int keepalive(MQTTClient* c)
{
    int rc = SUCCESS;

    if (c->keepAliveInterval == 0)
    {
        goto exit;
    }

    if (TimerIsExpired(&c->last_sent) || TimerIsExpired(&c->last_received))
    {
        if (c->ping_outstanding)
        {
            mqtt_keepalive_retry_count++;
            ECOMM_TRACE(UNILOG_MQTT, keepalive_0, P_SIG, 0, "....keepalive....ping_outstanding..=1..");
            rc = FAILURE; /* PINGRESP not received in keepalive interval */
        }
        else
        {
            Timer timer;
            TimerInit(&timer);
            TimerCountdownMS(&timer, 1000);
            
            memset(mqttSendbuf, 0, MQTT_SEND_BUFF_LEN);
            memset(mqttReadbuf, 0, MQTT_RECV_BUFF_LEN);
            int len = MQTTSerialize_pingreq(c->buf, c->buf_size);
            
            ECOMM_TRACE(UNILOG_MQTT, keepalive_1, P_SIG, 0, "....keepalive....send packet..");
            if (len > 0 && (rc = sendPacket(c, len, &timer)) == SUCCESS) // send the ping packet
                c->ping_outstanding = 1;
        }
    }

exit:
    return rc;
}

int keepaliveRetry(MQTTClient* c)
{
    int rc = SUCCESS;

    if (c->keepAliveInterval == 0)
    {
        goto exit;
    }

    if (TimerIsExpired(&c->last_sent) || TimerIsExpired(&c->last_received))
    {
        {
            Timer timer;
            TimerInit(&timer);
            TimerCountdownMS(&timer, 1000);
            
            memset(mqttSendbuf, 0, MQTT_SEND_BUFF_LEN);
            memset(mqttReadbuf, 0, MQTT_RECV_BUFF_LEN);
            int len = MQTTSerialize_pingreq(c->buf, c->buf_size);
            
            ECOMM_TRACE(UNILOG_MQTT, keepalive_1pp, P_SIG, 0, "....keepalive....send packet..");
            if (len > 0 && (rc = sendPacket(c, len, &timer)) == SUCCESS) // send the ping packet
                c->ping_outstanding = 1;
        }
    }

exit:
    return rc;
}

void MQTTCleanSession(MQTTClient* c)
{
    int i = 0;

    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
        c->messageHandlers[i].topicFilter = NULL;
}

void MQTTCloseSession(MQTTClient* c)
{
    c->ping_outstanding = 0;
    c->isconnected = 0;
    if (c->cleansession)
        MQTTCleanSession(c);
}

int cycle(MQTTClient* c, Timer* timer)
{
    int len = 0,
        rc = SUCCESS;

    int packet_type = readPacket(c, timer);     /* read the socket, see what work is due */
    ECOMM_TRACE(UNILOG_MQTT, mqttRecvTask_0001, P_SIG, 1, ".....mqttRecvTask..packet_type=%d....",packet_type);

    switch (packet_type)
    {
        default:
            /* no more data to read, unrecoverable. Or read packet fails due to unexpected network error */
            rc = packet_type;
            break;
        case 0: /* timed out reading packet */
            break;
        case CONNACK:
        case PUBACK:
        case SUBACK:
        case UNSUBACK:
            break;
        case PUBLISH:
        {
            MQTTString topicName;
            MQTTMessage msg;
            int intQoS;
            msg.payloadlen = 0; /* this is a size_t, but deserialize publish sets this as int */
            if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName,
               (unsigned char**)&msg.payload, (int*)&msg.payloadlen, c->readbuf, c->readbuf_size) != 1)
                goto exit;
            msg.qos = (enum QoS)intQoS;
            deliverMessage(c, &topicName, &msg);
            if (msg.qos != QOS0)
            {
                if (msg.qos == QOS1)
                    len = MQTTSerialize_ack(c->buf, c->buf_size, PUBACK, 0, msg.id);
                else if (msg.qos == QOS2)
                    len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREC, 0, msg.id);
                if (len <= 0)
                    rc = FAILURE;
                else
                    rc = sendPacket(c, len, timer);
                if (rc == FAILURE)
                    goto exit; // there was a problem
            }
            break;
        }
        case PUBREC:
        case PUBREL:
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
                rc = FAILURE;
            else if ((len = MQTTSerialize_ack(c->buf, c->buf_size,
                (packet_type == PUBREC) ? PUBREL : PUBCOMP, 0, mypacketid)) <= 0)
                rc = FAILURE;
            else if ((rc = sendPacket(c, len, timer)) != SUCCESS) // send the PUBREL packet
                rc = FAILURE; // there was a problem
            if (rc == FAILURE)
                goto exit; // there was a problem
            break;
        }

        case PUBCOMP:
            break;
        case PINGRESP:
            c->ping_outstanding = 0;
            break;
    }

    if (keepalive(c) != SUCCESS) {      
        int socket_stat = 0;        
        mqttSendMsg mqttMsg;
        //check only keepalive FAILURE status so that previous FAILURE status can be considered as FAULT
        rc = FAILURE;
        socket_stat = sock_get_errno(c->ipstack->my_socket);
        if((socket_stat == MQTT_ERR_ABRT)||(socket_stat == MQTT_ERR_RST)||(socket_stat == MQTT_ERR_CLSD)||(socket_stat == MQTT_ERR_BADE))
        {
            ECOMM_TRACE(UNILOG_MQTT, mqttRecvTask_0, P_INFO, 0, ".....now, need reconnect.......");
            /* send  reconnect msg to send task */
            memset(&mqttMsg, 0, sizeof(mqttMsg));
            mqttMsg.cmdType = MQTT_DEMO_MSG_RECONNECT;

            xQueueSend(mqttSendMsgHandle, &mqttMsg, MQTT_MSG_TIMEOUT);
        }
        else
        {
            if(mqtt_keepalive_retry_count>3)
            {
                mqtt_keepalive_retry_count = 0;
                ECOMM_TRACE(UNILOG_MQTT, mqttRecvTask_ee0, P_INFO, 0, ".....now, need reconnect.......");
                /* send  reconnect msg to send task */
                memset(&mqttMsg, 0, sizeof(mqttMsg));
                mqttMsg.cmdType = MQTT_DEMO_MSG_RECONNECT;
                
                xQueueSend(mqttSendMsgHandle, &mqttMsg, MQTT_MSG_TIMEOUT);
            }
            else
            {
                keepaliveRetry(c);
            }
        }
    }

exit:
    if (rc == SUCCESS)
        rc = packet_type;
    else if (c->isconnected)
        ;//MQTTCloseSession(c);
    return rc;
}

int MQTTYield(MQTTClient* c, int timeout_ms)
{
    int rc = SUCCESS;
    Timer timer;

    TimerInit(&timer);
    TimerCountdownMS(&timer, timeout_ms);

      do
    {
        if (cycle(c, &timer) < 0)
        {
            rc = FAILURE;
            break;
        }
    } while (!TimerIsExpired(&timer));

    return rc;
}

int MQTTIsConnected(MQTTClient* client)
{
  return client->isconnected;
}

void MQTTRun(void* parm)
{
    Timer timer;
    MQTTClient* c = (MQTTClient*)&mqttClient;
    if(mqttSendMsgHandle == NULL)
    {
        mqttSendMsgHandle = xQueueCreate(16, sizeof(mqttSendMsg));
    }
    
    if(appMqttMsgHandle == NULL)
    {
        appMqttMsgHandle = xQueueCreate(16, sizeof(mqttDataMsg));
    }
    
    if(mqttMutex1.sem == NULL)
    {
        MutexInit(&mqttMutex1);
    }

    TimerInit(&timer);

    while (1)
    {
#if defined(MQTT_TASK)
        MutexLock(&c->mutex);
#endif
        MutexLock(&mqttMutex1);

        TimerCountdownMS(&timer, 1500); /* Don't wait too long if no traffic is incoming */
        cycle(c, &timer);
        MutexUnlock(&mqttMutex1);
        
#if defined(MQTT_TASK)
        MutexUnlock(&c->mutex);
#endif
        osDelay(200);
    }
}

#if defined(MQTT_TASK)
int MQTTStartTask(MQTTClient* client)
{
    return ThreadStart(&client->thread, &MQTTRun, client);
}
#endif

int MQTTStartRECVTask(void)
{
    osThreadAttr_t task_attr;

    memset(&task_attr, 0, sizeof(task_attr));
    task_attr.name = "mqttRecv";
    task_attr.stack_size = MQTT_DEMO_TASK_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal7;

    mqttRecvTaskHandle = osThreadNew(MQTTRun, NULL,&task_attr);
    if(mqttRecvTaskHandle == NULL)
    {
        return FAILURE;
    }

    return SUCCESS;
}

int waitfor(MQTTClient* c, int packet_type, Timer* timer)
{
    int rc = FAILURE;

    do
    {
        if (TimerIsExpired(timer))
            break; // we timed out
        rc = cycle(c, timer);
    }
    while (rc != packet_type && rc >= 0);

    return rc;
}

int MQTTConnectWithResults(MQTTClient* c, MQTTPacket_connectData* options, MQTTConnackData* data)
{
    Timer connect_timer;
    int rc = FAILURE;
    MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
    int len = 0;

#if defined(MQTT_TASK)
      MutexLock(&c->mutex);
#endif
      if (c->isconnected) /* don't send connect packet again if we are already connected */
          goto exit;

    TimerInit(&connect_timer);
    TimerCountdownMS(&connect_timer, c->command_timeout_ms);

    if (options == 0)
        options = &default_options; /* set default options if none were supplied */

    c->keepAliveInterval = options->keepAliveInterval;
    c->cleansession = options->cleansession;
    TimerCountdown(&c->last_received, c->keepAliveInterval);
    if ((len = MQTTSerialize_connect(c->buf, c->buf_size, options)) <= 0)
        goto exit;
    if ((rc = sendPacket(c, len, &connect_timer)) != SUCCESS)  // send the connect packet
        goto exit; // there was a problem

    // this will be a blocking call, wait for the connack
    if (waitfor(c, CONNACK, &connect_timer) == CONNACK)
    {
        data->rc = 0;
        data->sessionPresent = 0;
        if (MQTTDeserialize_connack(&data->sessionPresent, &data->rc, c->readbuf, c->readbuf_size) == 1)
            rc = data->rc;
        else
            rc = FAILURE;
    }
    else
        rc = FAILURE;

exit:
    if (rc == SUCCESS)
    {
        c->isconnected = 1;
        c->ping_outstanding = 0;
    }

#if defined(MQTT_TASK)
      MutexUnlock(&c->mutex);
#endif

    return rc;
}

int MQTTReConnect(MQTTClient* client, MQTTPacket_connectData* connectData)
{
    int ret = FAILURE;
    
    ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_13hh0, P_INFO, 0, "...start tcp disconnect ..");
    client->ipstack->disconnect(client->ipstack);
    
    ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_14hh1, P_INFO, 0, "...start tcp connect ...");
    if ((NetworkSetConnTimeout(client->ipstack, MQTT_SEND_TIMEOUT, MQTT_RECV_TIMEOUT)) != 0)
    {
        ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_15hh2, P_INFO, 0, "...tcp socket set timeout fail...");
    }
    else
    {
        ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_16hh3, P_INFO, 0, "...tcp connect ok...");
        client->isconnected = 0;
        if((NetworkConnect(client->ipstack, MQTT_SERVER_URI, MQTT_SERVER_PORT)) < 0)
        {
            ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_17hh4, P_INFO, 0, "...tcp reconnect fail!!!...\r\n");
        }
        else
        {
            ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_18hh5, P_INFO, 0, "...start mqtt connect ..");
    
            if ((MQTTConnect(client, connectData)) != 0)
            {
                ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_19hh6, P_INFO, 0, "...mqtt reconnect fial!!!...");
            }
            else
            {
                ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_20hh7, P_INFO, 0, "...mqtt reconnect ok!!!...");
                ret = SUCCESS;
            }
        }
    }
    return ret;
}

int MQTTConnect(MQTTClient* c, MQTTPacket_connectData* options)
{
    MQTTConnackData data;
    return MQTTConnectWithResults(c, options, &data);
}

int MQTTSetMessageHandler(MQTTClient* c, const char* topicFilter, messageHandler messageHandler)
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

int MQTTSubscribeWithResults(MQTTClient* c, const char* topicFilter, enum QoS qos,
       messageHandler messageHandler, MQTTSubackData* data)
{
    int rc = FAILURE;
    Timer timer;
    int len = 0;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicFilter;

#if defined(MQTT_TASK)
      MutexLock(&c->mutex);
#endif
      if (!c->isconnected)
            goto exit;

    TimerInit(&timer);
    TimerCountdownMS(&timer, c->command_timeout_ms);

    len = MQTTSerialize_subscribe(c->buf, c->buf_size, 0, getNextPacketId(c), 1, &topic, (int*)&qos);
    if (len <= 0)
        goto exit;
    if ((rc = sendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
        goto exit;             // there was a problem

    if (waitfor(c, SUBACK, &timer) == SUBACK)      // wait for suback
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

exit:
    if (rc == FAILURE)
        ;//MQTTCloseSession(c);
#if defined(MQTT_TASK)
      MutexUnlock(&c->mutex);
#endif
    return rc;
}

int MQTTSubscribe(MQTTClient* c, const char* topicFilter, enum QoS qos,
       messageHandler messageHandler)
{
    MQTTSubackData data;
    return MQTTSubscribeWithResults(c, topicFilter, qos, messageHandler, &data);
}

int MQTTUnsubscribe(MQTTClient* c, const char* topicFilter)
{
    int rc = FAILURE;
    Timer timer;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicFilter;
    int len = 0;

#if defined(MQTT_TASK)
      MutexLock(&c->mutex);
#endif
      if (!c->isconnected)
          goto exit;

    TimerInit(&timer);
    TimerCountdownMS(&timer, c->command_timeout_ms);

    if ((len = MQTTSerialize_unsubscribe(c->buf, c->buf_size, 0, getNextPacketId(c), 1, &topic)) <= 0)
        goto exit;
    if ((rc = sendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
        goto exit; // there was a problem

    if (waitfor(c, UNSUBACK, &timer) == UNSUBACK)
    {
        unsigned short mypacketid;  // should be the same as the packetid above
        if (MQTTDeserialize_unsuback(&mypacketid, c->readbuf, c->readbuf_size) == 1)
        {
            /* remove the subscription message handler associated with this topic, if there is one */
            MQTTSetMessageHandler(c, topicFilter, NULL);
        }
    }
    else
        rc = FAILURE;

exit:
    if (rc == FAILURE)
        ;//MQTTCloseSession(c);
#if defined(MQTT_TASK)
      MutexUnlock(&c->mutex);
#endif
    return rc;
}

int MQTTPublish(MQTTClient* c, const char* topicName, MQTTMessage* message)
{
    int rc = FAILURE;
    Timer timer;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicName;
    int len = 0;

#if defined(MQTT_TASK)
      MutexLock(&c->mutex);
#endif
      if (!c->isconnected)
            goto exit;

    TimerInit(&timer);
    TimerCountdownMS(&timer, c->command_timeout_ms);

    if (message->qos == QOS1 || message->qos == QOS2)
        message->id = getNextPacketId(c);

    len = MQTTSerialize_publish(c->buf, c->buf_size, 0, message->qos, message->retained, message->id,
              topic, (unsigned char*)message->payload, message->payloadlen);
    if (len <= 0)
        goto exit;
    if ((rc = sendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
        goto exit; // there was a problem

    if (message->qos == QOS1)
    {
        if (waitfor(c, PUBACK, &timer) == PUBACK)
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
        if (waitfor(c, PUBCOMP, &timer) == PUBCOMP)
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
                rc = FAILURE;
        }
        else
            rc = FAILURE;
    }

exit:
    if (rc == FAILURE)
        ;//MQTTCloseSession(c);
#if defined(MQTT_TASK)
      MutexUnlock(&c->mutex);
#endif
    return rc;
}

int MQTTDisconnect(MQTTClient* c)
{
    int rc = FAILURE;
    Timer timer;     // we might wait for incomplete incoming publishes to complete
    int len = 0;

#if defined(MQTT_TASK)
    MutexLock(&c->mutex);
#endif
    TimerInit(&timer);
    TimerCountdownMS(&timer, c->command_timeout_ms);

      len = MQTTSerialize_disconnect(c->buf, c->buf_size);
    if (len > 0)
        rc = sendPacket(c, len, &timer);            // send the disconnect packet
    MQTTCloseSession(c);

#if defined(MQTT_TASK)
      MutexUnlock(&c->mutex);
#endif
    return rc;
}

int MQTTInit(MQTTClient* c, Network* n, unsigned char* sendBuf, unsigned char* readBuf)
{    
    NetworkInit(n);
    MQTTClientInit(c, n, 40000, (unsigned char *)sendBuf, 1000, (unsigned char *)readBuf, 1000);

    return 0;
}

int MQTTCreate(MQTTClient* c, Network* n, char* clientID, char* username, char* password, char *serverAddr, int port, MQTTPacket_connectData* connData)
{
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    int clientLen = 0;
    int usernameLen = 0;
    int passwordLen = 0;

    if(connData != NULL)
    {
        memcpy(&connectData, connData, sizeof(MQTTPacket_connectData));
    }
    else
    {
        connectData.MQTTVersion = 4;
        connectData.keepAliveInterval = 120;
    }

    if(clientID != NULL)
    {
        clientLen = strlen(clientID);
        connectData.clientID.cstring = malloc(clientLen+1);
        memset(connectData.clientID.cstring, 0, (clientLen+1));
        memcpy(connectData.clientID.cstring, clientID, clientLen);
    }

    if(username != NULL)
    {
        usernameLen = strlen(username);
        connectData.username.cstring = malloc(usernameLen+1);;
        memset(connectData.username.cstring, 0, (usernameLen+1));
        memcpy(connectData.username.cstring, username, usernameLen);
    }

    if(password != NULL)
    {
        passwordLen = strlen(password);
        connectData.password.cstring = malloc(passwordLen+1);
        memset(connectData.password.cstring, 0, (passwordLen+1));
        memcpy(connectData.password.cstring, password, passwordLen);
    } 
      
    if((NetworkSetConnTimeout(n, 5000, 5000)) != 0)
    {
        c->keepAliveInterval = connectData.keepAliveInterval;
        c->ping_outstanding = 1;
        return 1;
    }
    else
    {
        if ((NetworkConnect(n, serverAddr, port)) != 0)
        {
            c->keepAliveInterval = connectData.keepAliveInterval;
            c->ping_outstanding = 1;
            return 1;
        }
        else
        {
            if ((MQTTConnect(c, &connectData)) != 0)
            {
                c->ping_outstanding = 1;
                return 1;
            }
            else
            {
                #if defined(MQTT_TASK)
                    if ((MQTTStartTask(c)) != pdPASS)
                    {
                        return 1;
                    }
                    else
                    {
                        return 0;
                    }
                #endif
            }
        }
    }
    return 1;
}

#define MQTT_DEMO_EXAMPLE_1_ONENET
void mqtt_demo_onenet(void)
{
    int len = 0;
    MQTTMessage message;
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    
    connectData.MQTTVersion = 4;
    connectData.clientID.cstring = "34392813";
    connectData.username.cstring = "122343";
    connectData.password.cstring = "test001";
    connectData.keepAliveInterval = 120;
    ECOMM_TRACE(UNILOG_MQTT, mqtt_hh00, P_SIG, 0, "mqtt_demo........");
    mqttSendbuf = malloc(1000);
    mqttReadbuf = malloc(1000);
    memset(mqttSendbuf, 0, 1000);
    memset(mqttReadbuf, 0, 1000);
    
    NetworkInit(&mqttNetwork);
    MQTTClientInit(&mqttClient, &mqttNetwork, 40000, (unsigned char *)mqttSendbuf, 1000, (unsigned char *)mqttReadbuf, 1000);

    if((NetworkSetConnTimeout(&mqttNetwork, 5000, 5000)) != 0)
    {
        mqttClient.keepAliveInterval = connectData.keepAliveInterval;
        mqttClient.ping_outstanding = 1;
    }
    else
    {
        if ((NetworkConnect(&mqttNetwork, "183.230.40.39", 6002)) != 0)
        {
            mqttClient.keepAliveInterval = connectData.keepAliveInterval;
            mqttClient.ping_outstanding = 1;
        }
        else
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_hh01, P_SIG, 0, "mqtt_demo....1....");
            if ((MQTTConnect(&mqttClient, &connectData)) != 0)
            {
                mqttClient.ping_outstanding = 1;
            }
            else
            {
                ECOMM_TRACE(UNILOG_MQTT, mqtt_hh02, P_SIG, 0, "mqtt_demo....2....");
                mqttClient.ping_outstanding = 0;
            }
        }
        if(mqttClient.ping_outstanding == 0)
        {
            if ((MQTTStartRECVTask()) != SUCCESS)
                ;
        }

    }
    while(1)
    {
        sprintf(mqtt_payload,"{\"ec_smart_sensor_data\":%d}", ec_sensor_temp);
        len = strlen(mqtt_payload);
        ec_data_len = len;
        unsigned char *ptr = mqttJsonbuff;
        sprintf((char *)mqttJsonbuff,"%c%c%c%s", ec_data_type, ec_data_type,ec_data_type, mqtt_payload);
        message.payload = mqttJsonbuff;
        message.payloadlen = strlen((char *)mqttJsonbuff);
        writeChar(&ptr, ec_data_type);
        writeInt(&ptr, ec_data_len);
        ECOMM_TRACE(UNILOG_MQTT, mqtt_hh03, P_SIG, 0, "mqtt_demo....3....");

        MQTTPublish(&mqttClient, "$dp", &message);

        osDelay(10000);
    }
    /* do not return */
}

#define MQTT_DEMO_EXAMPLE_2_ALI
void mqtt_demo_ali(void)
{
    //char *pub_topic;
    int len = 0;
    MQTTMessage message;
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    char hmac_source[256] = {0};
    char *ali_clientID = NULL;
    char *ali_username = NULL;
    char *ali_signature = NULL;

    /* eigencomm  ec_smoke     a1xFDTv3InR  sWZtNMYkODMxvauyaSiGeJrVEp9jZ4Tg  8964 ---"eigencomm|securemode=3,signmethod=hmacsha1,timestamp=8964|" "deviceGrape&a1fsx061r0x" */
    /* eigencomm  deviceGrape  a1fsx061r0x  WLar6NunAcCJ0aZHbaNw4eQwdsYVKyC9  8964 ---"eigencomm|securemode=3,signmethod=hmacsha1,timestamp=8964|" "deviceGrape&a1fsx061r0x" */
    /* clientID   deviceName   productKey   deviceSecret*/
    ali_clientID = malloc(128);
    ali_username = malloc(64);
    ali_signature = malloc(96);
    
    memset(ali_clientID, 0, 128);
    memset(ali_username, 0, 64);
    memset(ali_signature, 0, 96);
    
    snprintf(hmac_source, sizeof(hmac_source), "clientId%s" "deviceName%s" "productKey%s" "timestamp%s", "eigencomm", "ec_smoke", "a1xFDTv3InR", "8964");
    
#ifdef FEATURE_MBEDTLS_ENABLE
    mqttAliHmacSha1((unsigned char *)hmac_source, strlen(hmac_source), (unsigned char *)ali_signature,(unsigned char *)"sWZtNMYkODMxvauyaSiGeJrVEp9jZ4Tg", strlen("sWZtNMYkODMxvauyaSiGeJrVEp9jZ4Tg"));
#endif
    
    sprintf(ali_clientID,"%s|securemode=3,signmethod=hmacsha1,timestamp=8964|", "eigencomm");
    sprintf(ali_username,"%s&%s","ec_smoke","a1xFDTv3InR");
    connectData.clientID.cstring = ali_clientID;
    connectData.username.cstring = ali_username;
    connectData.password.cstring = ali_signature;
    
    connectData.MQTTVersion = 4;
    connectData.keepAliveInterval = 120;
    ECOMM_TRACE(UNILOG_MQTT, mqtt_hh001, P_SIG, 0, "mqtt_demo........");
    mqttSendbuf = malloc(1000);
    mqttReadbuf = malloc(1000);
    memset(mqttSendbuf, 0, 1000);
    memset(mqttReadbuf, 0, 1000);
    
    NetworkInit(&mqttNetwork);
    MQTTClientInit(&mqttClient, &mqttNetwork, 40000, (unsigned char *)mqttSendbuf, 1000, (unsigned char *)mqttReadbuf, 1000);

    if((NetworkSetConnTimeout(&mqttNetwork, 5000, 5000)) != 0)
    {
        mqttClient.keepAliveInterval = connectData.keepAliveInterval;
        mqttClient.ping_outstanding = 1;
    }
    else
    {
        if ((NetworkConnect(&mqttNetwork, "a1xFDTv3InR.iot-as-mqtt.cn-shanghai.aliyuncs.com", 1883)) != 0)
        {
            mqttClient.keepAliveInterval = connectData.keepAliveInterval;
            mqttClient.ping_outstanding = 1;
        }
        else
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_hh012, P_SIG, 0, "mqtt_demo....1....");
            if ((MQTTConnect(&mqttClient, &connectData)) != 0)
            {
                mqttClient.ping_outstanding = 1;
            }
            else
            {
                ECOMM_TRACE(UNILOG_MQTT, mqtt_hh023, P_SIG, 0, "mqtt_demo....2....");
                mqttClient.ping_outstanding = 0;
            }
        }
        if(mqttClient.ping_outstanding == 0)
        {
            if ((MQTTStartRECVTask()) != SUCCESS)
                ;
        }

    }
    while(1)
    {
        memset(mqtt_payload, 0, sizeof(mqtt_payload));
        memcpy(mqtt_payload, "update", strlen("update"));
        len = strlen(mqtt_payload);
        message.payload = mqtt_payload;
        message.payloadlen = len;
        ECOMM_TRACE(UNILOG_MQTT, mqtt_hh034, P_SIG, 0, "mqtt_demo....3....");

        MQTTPublish(&mqttClient, "a1xFDTv3InR/ec_smoke/user/get", &message);

        osDelay(10000);
    }
    /* do not return */
}

#define MQTT_DEMO_EXAMPLE_3_APP
void mqtt_demo_send_task(void *argument)
{
    int ret = FAILURE;
    int msgType = 0xff;
    mqttSendMsg mqttMsg;
    mqttDataMsg mqttMessage;
    int socket_stat = -1;
    int socket_err = -1;    
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    
    connectData.MQTTVersion = 4;
    connectData.clientID.cstring = "34392813";
    connectData.username.cstring = "122343";
    connectData.password.cstring = "test001";
    connectData.keepAliveInterval = 120;
    
    ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_0, P_SIG, 0, "mqttSendTask........");
    mqttSendbuf = malloc(MQTT_SEND_BUFF_LEN);
    mqttReadbuf = malloc(MQTT_RECV_BUFF_LEN);
    memset(mqttSendbuf, 0, MQTT_SEND_BUFF_LEN);
    memset(mqttReadbuf, 0, MQTT_RECV_BUFF_LEN);

    NetworkInit(&mqttNetwork);
    MQTTClientInit(&mqttClient, &mqttNetwork, 40000, (unsigned char *)mqttSendbuf, MQTT_SEND_BUFF_LEN, (unsigned char *)mqttReadbuf, MQTT_RECV_BUFF_LEN);

    if((NetworkSetConnTimeout(&mqttNetwork, 10000, 10000)) != 0)
    {
        mqttClient.keepAliveInterval = connectData.keepAliveInterval;
        mqttClient.ping_outstanding = 1;
    }
    else
    {
        if ((NetworkConnect(&mqttNetwork, "183.230.40.39", 6002)) != 0)
        {
            mqttClient.keepAliveInterval = connectData.keepAliveInterval;
            mqttClient.ping_outstanding = 1;
            ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_1, P_SIG, 0, "mqttSendTask..tcp connect fail....");
        }
        else
        {
            ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_2, P_SIG, 0, "mqttSendTask..tcp connect ok....");
            if ((MQTTConnect(&mqttClient, &connectData)) != 0)
            {
                mqttClient.ping_outstanding = 1;
                ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_3, P_SIG, 0, "mqttSendTask..mqtt connect fail....");
            }
            else
            {
                mqttClient.keepAliveInterval = connectData.keepAliveInterval;
                ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_4, P_SIG, 0, "mqttSendTask..mqtt connect ok....");
                mqtt_send_task_status_flag = 1;             
                mqttClient.ping_outstanding = 0;
            }
        }
    }
    
    /* sub topic*/
    //MQTTSubscribe(&mqttClient, topic_data, 0, NULL);       
    
    /*start recv task*/
    if(mqttClient.ping_outstanding == 0)
    {
        mqtt_demo_recv_task_init();
    }
    ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_4000, P_SIG, 0, "mqttSendTask..start recv msg....");
    
    while(1)
    {
        ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_4001, P_SIG, 1, "mqttSendTask..recv msg hand=0x%x....",mqttSendMsgHandle);
        /* recv msg (block mode) */
        xQueueReceive(mqttSendMsgHandle, &mqttMsg, osWaitForever);
        msgType = mqttMsg.cmdType;
        ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_4002, P_SIG, 1, "mqttSendTask..recv msg=%d....",msgType);

        switch(msgType)
        {
            case MQTT_DEMO_MSG_PUBLISH:
                /* send packet */
                ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_500, P_INFO, 0, ".....start send mqtt publish packet.......");
                MutexLock(&mqttMutex1);
                ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_5, P_INFO, 0, ".....start send mqtt publish packet.......");
                memset(mqttSendbuf, 0, MQTT_SEND_BUFF_LEN);
                ret = MQTTPublish(&mqttClient, mqttMsg.topic, &mqttMsg.message);
                
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
                
                /* send result to at task */
                if(ret == SUCCESS)
                {
                    memset(&mqttMessage, 0, sizeof(mqttMessage));
                    mqttMessage.cmdType = MQTT_DEMO_MSG_PUBLISH_ACK;
                    mqttMessage.ret = SUCCESS;
                    ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_600, P_INFO, 0, ".....send mqtt publish packet ok.......");
                
                    xQueueSend(appMqttMsgHandle, &mqttMessage, MQTT_MSG_TIMEOUT);
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_6, P_INFO, 0, ".....send mqtt publish packet fail.......");
                    socket_stat = sock_get_errno(mqttClient.ipstack->my_socket);
                    socket_err = socket_error_is_fatal(socket_stat);
                    if(socket_err == 1)
                    {
                        ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_7, P_INFO, 0, ".....find need reconnect when publish packet.......");
                        MQTTReConnect(&mqttClient, &connectData);

                    }
                    else
                    {
                        memset(&mqttMessage, 0, sizeof(mqttMessage));
                        mqttMessage.cmdType = MQTT_DEMO_MSG_PUBLISH_ACK;
                        mqttMessage.ret = SUCCESS;
                        ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_600ii, P_INFO, 0, ".....send mqtt publish packet ok.......");
                    
                        xQueueSend(appMqttMsgHandle, &mqttMessage, MQTT_MSG_TIMEOUT);
                    }
                    //memset(&mqttMessage, 0, sizeof(mqttMessage));
                    //mqttMessage.cmdType = MQTT_MSG_RECONNECT;
                    //mqttMessage.ret = FAILURE;
                
                    //xQueueSend(appMqttMsgHandle, &mqttMessage, MQTT_MSG_TIMEOUT);
                }
                MutexUnlock(&mqttMutex1);
                break;
#if 0
            case MQTT_DEMO_MSG_KEEPALIVE:
                /* send keepalive packet */
                ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_8, P_INFO, 0, ".....start send mqtt keeplive packet.......");
                ret = keepalive(&mqttClient);

                if(ret == SUCCESS) // send the ping packet
                {
                    mqttClient->ping_outstanding = 1;
                    ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_9, P_INFO, 1, ".....mqtt keeplive send ok.......");
                }
                else
                {
                    socket_stat = sock_get_errno(mqttNewContext->mqtt_client->ipstack->my_socket);
                    socket_err = socket_error_is_fatal(socket_stat);
                    if(socket_err == 1)
                    {
                        ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_10, P_INFO, 0, ".....find need reconnect when send keeplive packet.......");
                        ret = MQTT_RECONNECT;
                    }
                    ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_11, P_INFO, 1, ".....mqtt send keeplive Packet fail");
                }
                break;
#endif
            case MQTT_DEMO_MSG_RECONNECT:
                MutexLock(&mqttMutex1);
                ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_12, P_INFO, 0, ".....find need reconnect when read packet.......");
                ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_13, P_INFO, 0, "...start tcp disconnect ..");
                mqttClient.ipstack->disconnect(mqttClient.ipstack);
                
                ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_14, P_INFO, 0, "...start tcp connect ...");
                if ((NetworkSetConnTimeout(mqttClient.ipstack, MQTT_SEND_TIMEOUT, MQTT_RECV_TIMEOUT)) != 0)
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_15, P_INFO, 0, "...tcp socket set timeout fail...");
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_16, P_INFO, 0, "...tcp connect ok...");
                    mqttClient.isconnected = 0;
                    if((NetworkConnect(mqttClient.ipstack, MQTT_SERVER_URI, MQTT_SERVER_PORT)) < 0)
                    {
                        ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_17, P_INFO, 0, "...tcp reconnect fail!!!...\r\n");
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_18, P_INFO, 0, "...start mqtt connect ..");

                        if ((MQTTConnect(&mqttClient, &connectData)) != 0)
                        {
                            ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_19, P_INFO, 0, "...mqtt reconnect fial!!!...");
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_MQTT, mqttSendTask_20, P_INFO, 0, "...mqtt reconnect ok!!!...");
                        }
                    }
                }
                MutexUnlock(&mqttMutex1);
                break;
            }
    }
}

void app_mqtt_demo_task(void *argument)
{
    int msgType = 1;
    int payloadLen = 0;
    mqttSendMsg mqttMsg;
    mqttDataMsg mqttMessage;
    int ret = FAILURE;
    
    /*init driver*/

    /*start mqtt send task*/
    ret = mqtt_demo_send_task_init();
    if(ret == FAILURE)
    {
        ;
    }
    while(1)
    {
        if(mqtt_send_task_status_flag == 1)
        {
            break;
        }
        osDelay(4000);
    }
    
    /*state machine*/
    while(1)
    {
        /*read data*/

        osDelay(4000);

        /*send msg to mqtt send task, and wait for excute result*/
        ECOMM_TRACE(UNILOG_MQTT, appMqttTask_0, P_SIG, 0, "appMqttTask...send start.....");
        memset(&mqttMsg, 0, sizeof(mqttSendMsg));
        mqttMsg.cmdType = MQTT_DEMO_MSG_PUBLISH;

        mqttMsg.topic = malloc(128);
        memset(mqttMsg.topic, 0, 128);
        memcpy(mqttMsg.topic, "$dp", strlen("$dp"));        
        mqttMsg.message.qos = QOS0;
        mqttMsg.message.retained = 0;
        mqttMsg.message.id = 0;
        mqttMsg.message.payload = malloc(32);
        memset(mqttMsg.message.payload, 0, 32);
        memcpy(mqttMsg.message.payload, "{\"data\":90}", 32);
        
        sprintf(mqttMsg.message.payload, "%c%c%c%s",msgType, msgType, msgType, "{\"data\":90}");
        payloadLen = strlen(mqttMsg.message.payload);
        unsigned char *ptr = (unsigned char *)mqttMsg.message.payload;
        writeChar(&ptr,3);
        writeInt(&ptr,strlen("{\"data\":90}"));
        mqttMsg.message.payloadlen = payloadLen;

        xQueueSend(mqttSendMsgHandle, &mqttMsg, (2*MQTT_MSG_TIMEOUT));
        
        xQueueReceive(appMqttMsgHandle, &mqttMessage, portMAX_DELAY);
        if(mqttMessage.cmdType == MQTT_DEMO_MSG_PUBLISH_ACK)
        {
            ECOMM_TRACE(UNILOG_MQTT, appMqttTask_1, P_SIG, 0, "appMqttTask...send ok.....");
        }
        else
        {
            ECOMM_TRACE(UNILOG_MQTT, appMqttTask_2, P_SIG, 0, "appMqttTask...send fail.....");
        }
    }
}

int mqtt_demo_recv_task_init(void)
{
    osThreadAttr_t task_attr;

    memset(&task_attr, 0, sizeof(task_attr));
    task_attr.name = "mqttRecv";
    task_attr.stack_size = MQTT_DEMO_TASK_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal7;

    mqttRecvTaskHandle = osThreadNew(MQTTRun, NULL,&task_attr);
    if(mqttRecvTaskHandle == NULL)
    {
        return FAILURE;
    }

    return SUCCESS;
}

int mqtt_demo_send_task_init(void)
{
    osThreadAttr_t task_attr;
    
    memset(&task_attr, 0, sizeof(task_attr));
    task_attr.name = "mqttSend";
    task_attr.stack_size = MQTT_DEMO_TASK_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal6;

    mqttSendTaskHandle = osThreadNew(mqtt_demo_send_task, NULL,&task_attr);
    if(mqttSendTaskHandle == NULL)
    {
        return FAILURE;
    }

    return SUCCESS;
}

int app_mqtt_demo_task_init(void)
{
    osThreadAttr_t task_attr;

    if(mqttSendMsgHandle == NULL)
    {
        mqttSendMsgHandle = xQueueCreate(16, sizeof(mqttSendMsg));
    }
    
    if(appMqttMsgHandle == NULL)
    {
        appMqttMsgHandle = xQueueCreate(16, sizeof(mqttDataMsg));
    }
    
    if(mqttMutex1.sem == NULL)
    {
        MutexInit(&mqttMutex1);
    }

    memset(&task_attr, 0, sizeof(task_attr));
    task_attr.name = "appTask";
    task_attr.stack_size = MQTT_DEMO_TASK_STACK_SIZE;
    task_attr.priority = osPriorityBelowNormal5;

    appMqttTaskHandle = osThreadNew(app_mqtt_demo_task, NULL,&task_attr);
    if(appMqttTaskHandle == NULL)
    {
        return FAILURE;
    }

    return SUCCESS;
}

#define MQTT_DEMO_EXAMPLE_4_SMOKE

#ifdef FEATURE_SMOKE_ENABLE


void mqtt_demo_smoke(uint32_t smokeValue)
{
    //MQTTClient client;
    //Network network;
    char *pub_topic;
    int len = 0;
    MQTTMessage message;
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    int rc = FAILURE;
    
    connectData.MQTTVersion = 4;
    connectData.clientID.cstring = "34392813";
    connectData.username.cstring = "122343";
    connectData.password.cstring = "test001";
    connectData.keepAliveInterval = 120;
    ECOMM_TRACE(UNILOG_MQTT, mqtt_hh00uuu, P_SIG, 1, "mqtt_demo....smokeValue=%d....",smokeValue);
    mqttSendbuf = malloc(1000);
    mqttReadbuf = malloc(1000);
    memset(mqttSendbuf, 0, 1000);
    memset(mqttReadbuf, 0, 1000);
    memset(&message, 0, sizeof(MQTTMessage));

    if(mqttMutex1.sem == NULL)
    {
        MutexInit(&mqttMutex1);
    }

    NetworkInit(&mqttNetwork);
    MQTTClientInit(&mqttClient, &mqttNetwork, 40000, (unsigned char *)mqttSendbuf, 1000, (unsigned char *)mqttReadbuf, 1000);

    if((NetworkSetConnTimeout(&mqttNetwork, 5000, 5000)) != 0)
    {
        mqttClient.keepAliveInterval = connectData.keepAliveInterval;
        mqttClient.ping_outstanding = 1;
    }
    else
    {
        if ((NetworkConnect(&mqttNetwork, "183.230.40.39", 6002)) != 0)
        {
            mqttClient.keepAliveInterval = connectData.keepAliveInterval;
            mqttClient.ping_outstanding = 1;
        }
        else
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_hh01uu, P_SIG, 0, "mqtt_demo....tcp connect ok....");
            for(int i=0; i<3; i++)
            {
                if ((MQTTConnect(&mqttClient, &connectData)) != 0)
                {
                    mqttClient.ping_outstanding = 1;
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_hh02uu1, P_SIG, 0, "mqtt_demo....mqtt connect fail....");
                }
                else
                {
                    ECOMM_TRACE(UNILOG_MQTT, mqtt_hh02uu, P_SIG, 0, "mqtt_demo....mqtt connect ok....");
                    mqttClient.ping_outstanding = 0;
                    break;
                }
            }
        }
        if(mqttClient.ping_outstanding == 0)
        {
            if ((MQTTStartRECVTask()) != SUCCESS)
                    ;
        }
    }
    while(1)
    {    
        memset(mqtt_payload, 0, 128);
        sprintf(mqtt_payload,"{\"ec_smart_sensor_data\":%d}", smokeValue);
        len = strlen(mqtt_payload);
        ec_data_len = len;
        unsigned char *ptr = mqttJsonbuff;
        sprintf((char *)mqttJsonbuff,"%c%c%c%s", ec_data_type, ec_data_type,ec_data_type, mqtt_payload);
        message.payload = mqttJsonbuff;
        message.payloadlen = strlen((char *)mqttJsonbuff);
        writeChar(&ptr, ec_data_type);
        writeInt(&ptr, ec_data_len);
        message.id = 0x123;
        
        ECOMM_TRACE(UNILOG_MQTT, mqtt_hh03uu, P_SIG, 0, "mqtt_demo....send packet....");
        MutexLock(&mqttMutex1);

        rc = MQTTPublish(&mqttClient, "$dp", &message);
        MutexUnlock(&mqttMutex1);
        if(rc == SUCCESS)
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_hh04uu, P_SIG, 0, "mqtt_demo....send ok....");
        }
        else
        {
            ECOMM_TRACE(UNILOG_MQTT, mqtt_hh05uu, P_SIG, 0, "mqtt_demo....send fail....");
        }
        
        //while(0)
        //{
        //    osDelay(60000);
        //    ECOMM_TRACE(UNILOG_MQTT, mqtt_hh06uu, P_SIG, 0, "mqtt_demo....wait....");
        //}

        ECOMM_TRACE(UNILOG_MQTT, mqtt_hh07uu, P_SIG, 0, "mqtt_demo....go to sleep....");
        pmuSetMcuMode(1);
        pmuMcuModeThdSet(30000, 2000);
        appSetCFUN(0);
        while(1)
        {
            osDelay(60000);
        }
    }
    /* do not return */
}
#endif
