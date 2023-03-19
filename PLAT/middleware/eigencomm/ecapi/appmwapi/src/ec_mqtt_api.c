/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    ec_mqtt_api.c
 * Description:  API interface implementation source file for ctlwm2m request
 * History:      Rev1.0   2018-10-30
 *
 ****************************************************************************/
#include "ec_mqtt_api.h"
/*
MQTTClient ecMqttClient;
Network ecMqttNetwork;
unsigned char* ecMqttSendBuf;
unsigned char* ecMqttReadBuf;
int sendBufLen = 1024;
int readBufLen = 1024;

mqtt_init(&ecMqttClient, &ecMqttNetwork, &ecMqttSendBuf, sendBufLen, &ecMqttReadBuf, readBufLen);

mqtt_create(&ecMqttClient, &ecMqttNetwork, "clientID", "username", "password", "serverAddr", port, NULL);

*/
int mqtt_init(MQTTClient* client, Network* network, unsigned char** sendBuf, int sendBufLen, unsigned char** readBuf, int readBufLen)
{
    int ret = 1;

    if(sendBufLen > 0)
    {
        *sendBuf = malloc(sendBufLen);
        memset(*sendBuf, 0, sendBufLen);
    }
    if(readBufLen > 0)
    {
        *readBuf = malloc(readBufLen);
        memset(*readBuf, 0, readBufLen);
    }     
    
    ret = MQTTInit(client, network, *sendBuf, *readBuf);
    
    return ret;
}

int mqtt_create(MQTTClient* client, Network* network, char* clientID, char* username, char* password, char *serverAddr, int port, MQTTPacket_connectData* connData)
{
    int ret = 1;
    
    ret = MQTTCreate(client, network, clientID, username, password, serverAddr, port, connData);
    
    return ret;
}

int mqtt_close(MQTTClient* client, Network* network)
{
    int ret = 1;

    ret = MQTTDisconnect(client);
    if(ret == 0)
    {
        ret = network->disconnect(network);
    }

    return ret;
}

int mqtt_sub(MQTTClient* client, const char* topic, enum QoS qos, messageHandler messageHandler)
{
    int ret = 1;

    ret = MQTTSubscribe(client, topic, qos, messageHandler);

    return ret;
}

int mqtt_unsub(MQTTClient* client, const char* topic)
{
    int ret = 1;

    ret = MQTTUnsubscribe(client, topic);

    return ret;
}

int mqtt_pub(MQTTClient* client, const char* topic, MQTTMessage* message)
{
    int ret = 1;

    ret = MQTTPublish(client, topic, message);

    return ret;
}











