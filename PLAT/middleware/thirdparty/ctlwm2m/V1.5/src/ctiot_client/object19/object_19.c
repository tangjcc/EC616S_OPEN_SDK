
/************************************************************************

            (c) Copyright 2018 by 中国电信上海研究院. All rights reserved.

**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ctiot_lwm2m_sdk.h"




/*
 * Multiple instance objects can use userdata to store data that will be shared between the different instances.
 * The lwm2m_object_t object structure - which represent every object of the liblwm2m as seen in the single instance
 * object - contain a chained list called instanceList with the object specific structure prv_instance_t:
 */
typedef struct _prv_instance_
{
    /*
     * The first two are mandatories and represent the pointer to the next instance and the ID of this one. The rest
     * is the instance scope user data (uint8_t test in this case)
     */
    struct _prv_instance_ * next;   // matches lwm2m_list_t::next
    uint16_t shortID;               // matches lwm2m_list_t::id
    uint8_t  test;
    double   dec;
} prv_instance_t;

#if 0
static void prv_output_buffer(uint8_t * buffer,
                              int length)
{
    int i;
    uint8_t array[16];

    i = 0;
    while (i < length)
    {
        int j;
        fprintf(stderr, "  ");

        memcpy(array, buffer+i, 16);

        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            fprintf(stderr, "%02X ", array[j]);
        }
        while (j < 16)
        {
            fprintf(stderr, "   ");
            j++;
        }
        fprintf(stderr, "  ");
        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            if (isprint(array[j]))
                fprintf(stderr, "%c ", array[j]);
            else
                fprintf(stderr, ". ");
        }
        fprintf(stderr, "\n");

        i += 16;
    }
}
#endif
static uint8_t prv_read(uint16_t instanceId,
                        int * numDataP,
                        lwm2m_data_t ** dataArrayP,
                        lwm2m_object_t * objectP)
{
	 lwm2m_printf("prv_read COAP_401_UNAUTHORIZED\r\n");
     return COAP_401_UNAUTHORIZED;
}

static uint8_t prv_discover(uint16_t instanceId,
                            int * numDataP,
                            lwm2m_data_t ** dataArrayP,
                            lwm2m_object_t * objectP)
{
	lwm2m_printf("prv_discover COAP_401_UNAUTHORIZED\r\n");
    return COAP_401_UNAUTHORIZED;
}

static uint8_t prv_write(uint16_t instanceId,
                         int numData,
                         lwm2m_data_t * dataArray,
                         lwm2m_object_t * objectP)
{
    lwm2m_list_t * targetP;
    int i;
    if(instanceId != 1) {
        lwm2m_printf("prv_write COAP_401_UNAUTHORIZED\r\n");
        return COAP_401_UNAUTHORIZED;
    }

    targetP = lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;
    for (i = 0 ; i < numData ; i++)
    {
        switch (dataArray[i].id)
        {
            case 0:
            {
                ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_RECEIVE,dataArray->value.asBuffer.buffer, dataArray->value.asBuffer.length);
            }
            break;
            default:
            {
                ECOMM_TRACE(UNILOG_CTLWM2M, prv_write_0, P_INFO, 1, "obj19 write id=%d", dataArray[i].id);
                return COAP_404_NOT_FOUND;
            }
        }
    }

    return COAP_204_CHANGED;
}


static uint8_t prv_delete(uint16_t id,
                          lwm2m_object_t * objectP)
{
	lwm2m_printf("prv_delete COAP_401_UNAUTHORIZED\r\n");
	return COAP_202_DELETED;
    #if 0
    prv_instance_t * targetP;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&targetP);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    ct_lwm2m_free(targetP);

    return COAP_202_DELETED;
	#endif
}

static uint8_t prv_create(uint16_t instanceId,
                          int numData,
                          lwm2m_data_t * dataArray,
                          lwm2m_object_t * objectP)
{
	lwm2m_printf("prv_create COAP_401_UNAUTHORIZED\r\n");
	return COAP_401_UNAUTHORIZED;
    #if 0

    prv_instance_t * targetP;
    uint8_t result;


    targetP = (prv_instance_t *)ct_lwm2m_malloc(sizeof(prv_instance_t));
    if (NULL == targetP) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(prv_instance_t));

    targetP->shortID = instanceId;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, targetP);

    result = prv_write(instanceId, numData, dataArray, objectP);

    if (result != COAP_204_CHANGED)
    {
        (void)prv_delete(instanceId, objectP);
    }
    else
    {
        result = COAP_201_CREATED;
    }

    return result;
	#endif
}

static uint8_t prv_exec(uint16_t instanceId,
                        uint16_t resourceId,
                        uint8_t * buffer,
                        int length,
                        lwm2m_object_t * objectP)
{

    if (NULL == lwm2m_list_find(objectP->instanceList, instanceId)) return COAP_404_NOT_FOUND;

    switch (resourceId)
    {
        case 0:
        case 2:
        {
            ECOMM_TRACE(UNILOG_CTLWM2M, prv_exec_0, P_INFO, 0, "obj19 prv_exec");
            ctiot_funcv1_notify_nb_info(CTIOT_NB_SUCCESS, AT_TO_MCU_RECEIVE, buffer, length);
            return COAP_204_CHANGED;
        }
        case 1:
            return COAP_405_METHOD_NOT_ALLOWED;
        case 3:
            return COAP_405_METHOD_NOT_ALLOWED;
        default:
            return COAP_404_NOT_FOUND;
    }
}



lwm2m_object_t * get_data_report_object(void)
{
    lwm2m_object_t * dataReportObject;

    dataReportObject = (lwm2m_object_t *)ct_lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != dataReportObject)
    {
        int i;
        lwm2m_list_t * targetP;

        memset(dataReportObject, 0, sizeof(lwm2m_object_t));

        dataReportObject->objID = DATA_REPORT_OBJECT;
        for(i = 0; i < 2; i++)
        {
        	targetP = (lwm2m_list_t *)ct_lwm2m_malloc(sizeof(lwm2m_list_t));
			if (NULL == targetP) goto exit;
			memset(targetP, 0, sizeof(lwm2m_list_t));
			targetP->id=i;
			dataReportObject->instanceList = LWM2M_LIST_ADD(dataReportObject->instanceList, targetP);
        }
        
        /*
         * From a single instance object, two more functions are available.
         * - The first one (createFunc) create a new instance and filled it with the provided informations. If an ID is
         *   provided a check is done for verifying his disponibility, or a new one is generated.
         * - The other one (deleteFunc) delete an instance by removing it from the instance list (and freeing the memory
         *   allocated to it)
         */
        dataReportObject->readFunc = prv_read;
        dataReportObject->discoverFunc = prv_discover;
        dataReportObject->writeFunc = prv_write;
        dataReportObject->executeFunc = prv_exec;
        dataReportObject->createFunc = prv_create;
        dataReportObject->deleteFunc = prv_delete;
    }

    return dataReportObject;
exit:
	LWM2M_LIST_FREE(dataReportObject->instanceList);
	ct_lwm2m_free(dataReportObject);
return NULL;
}

void free_data_report_object(lwm2m_object_t * object)
{
    LWM2M_LIST_FREE(object->instanceList);
    if (object->userData != NULL)
    {
        ct_lwm2m_free(object->userData);
        object->userData = NULL;
    }
    ct_lwm2m_free(object);
}

