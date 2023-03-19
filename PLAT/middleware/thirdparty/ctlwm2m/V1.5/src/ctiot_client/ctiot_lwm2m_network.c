#include <string.h>
#include "ct_liblwm2m.h"
#include "ctiot_lwm2m_sdk.h"
#include "objects/lwm2mclient.h"
#ifdef WITH_MBEDTLS
#include "../port/dtlsconnection.h"
#else
#include "../port/connection.h"
#endif

#ifdef WITH_MBEDTLS
void * ct_lwm2m_connect_server(uint16_t secObjInstID,
                            void * userData)
{
  //printf("WITH_TINYDTLS:ct_lwm2m_connect_server\n");
  ctiot_funcv1_client_data_t * dataP;
  lwm2m_list_t * instance;
  mbedtls_connection_t * newConnP = NULL;
  dataP = (ctiot_funcv1_client_data_t *)userData;

  lwm2m_object_t  * securityObj = dataP->securityObjP;

  instance = LWM2M_LIST_FIND(dataP->securityObjP->instanceList, secObjInstID);
  if (instance == NULL) return NULL;

  newConnP = ct_connection_create(dataP->connList, dataP->sock, securityObj, instance->id, dataP->lwm2mH, dataP->addressFamily, userData);
  if (newConnP == NULL)
  {
      ECOMM_TRACE(UNILOG_CTLWM2M, ct_lwm2m_connect_server_0, P_INFO, 0, "Connection creation failed.");
      return NULL;
  }
  dataP->connList = newConnP;
  ECOMM_TRACE(UNILOG_CTLWM2M, ct_lwm2m_connect_server_1, P_INFO, 1, "dataP->connList = 0x%x", dataP->connList);
  return (void *)newConnP;
}
#else
void *ct_lwm2m_connect_server(uint16_t secObjInstID,
                           void *userData)
{
    //printf("NO WITH_TINYDTLS:ct_lwm2m_connect_server\n");
    ctiot_funcv1_client_data_t *dataP;
    char *uri;
    char *host;
    char *port;
    connection_t *newConnP = NULL;

    dataP = (ctiot_funcv1_client_data_t *)userData;

    uri = ct_get_server_uri(dataP->securityObjP, secObjInstID);
    lwm2m_printf("uri:%s\r\n",uri);
    if (uri == NULL)
        return NULL;

    // parse uri in the form "coaps://[host]:[port]"
    if (0 == strncmp(uri, "coaps://", strlen("coaps://")))
    {
        host = uri + strlen("coaps://");
    }
    else if (0 == strncmp(uri, "coap://", strlen("coap://")))
    {
        host = uri + strlen("coap://");
    }
    else
    {
        goto exit;
    }
    port = strrchr(host, ':');
    if (port == NULL)
        goto exit;
    // remove brackets
    if (host[0] == '[')
    {
        host++;
        if (*(port - 1) == ']')
        {
            *(port - 1) = 0;
        }
        else
            goto exit;
    }
    // split strings
    *port = 0;
    port++;

    lwm2m_printf("Opening connection to server at %s:%s\r\n", host, port);
    newConnP = ct_connection_create(dataP->connList, dataP->sock, host, port, dataP->addressFamily);
    if (newConnP == NULL)
    {
        lwm2m_printf("Connection creation failed.\r\n");
    }
    else
    {
        dataP->connList = newConnP;
    }

exit:
    ct_lwm2m_free(uri);
    return (void *)newConnP;
}
#endif
void ct_lwm2m_close_connection(void *sessionH,
                            void *userData)
{
    ctiot_funcv1_client_data_t *app_data;
#ifdef WITH_MBEDTLS
    mbedtls_connection_t * targetP;
#else
    connection_t * targetP;
#endif

    app_data = (ctiot_funcv1_client_data_t *)userData;
#ifdef WITH_MBEDTLS
    targetP = (mbedtls_connection_t *)sessionH;
#else
    targetP = (connection_t *)sessionH;
#endif

    if (targetP == app_data->connList)
    {
        app_data->connList = targetP->next;
        ct_connection_free(targetP);
    }
    else
    {
#ifdef WITH_MBEDTLS
        mbedtls_connection_t * parentP;
#else
        connection_t * parentP;
#endif

        parentP = app_data->connList;
        while (parentP != NULL && parentP->next != targetP)
        {
            parentP = parentP->next;
        }
        if (parentP != NULL)
        {
            parentP->next = targetP->next;
            ct_connection_free(targetP);
        }
    }
}
