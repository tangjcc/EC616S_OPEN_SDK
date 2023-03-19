
/************************************************************************

            (c) Copyright 2018 by 中国电信上海研究院. All rights reserved.

**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ctiot_lwm2m_sdk.h"


extern object_instance_array_t objInsArray[MAX_MCU_OBJECT];
/*
 * Multiple instance objects can use userdata to store data that will be shared between the different instances.
 * The lwm2m_object_t object structure - which represent every object of the liblwm2m as seen in the single instance
 * object - contain a chained list called instanceList with the object specific structure prv_instance_t:
 */
uint8_t find_use_targe(char data, char *targe)
{
	uint8_t cnt=0;

	while(targe[cnt] != '\0')
	{
		if( targe[cnt] == data )
		{
			return cnt;
		}
		cnt++;
	}
	return 0xff;
}
char t_strtok(char *in, char *targe, char **args, uint8_t *argc)
{
	char *p=in;

	if( in == NULL ) return 0;
	if( args ==NULL ) return 0;

	/**********parse in params************/
	*argc = 1;
	*args++ = p;
	while(*p != '\0')
	{
		if( find_use_targe(*p, targe) != 0xff )  //,x  ,,
		{
			if( find_use_targe(*(p+1), targe) == 0xff )
			{
				*p = '\0';
				*args++ = ++p;
				(*argc)++;
			}
			else
			{
				*p++ = '\0';
				*args++ = '\0';
				(*argc)++;
			}
		}
		else
		{
			p++;
		}
	}
	/*************end param ***************/

	return 1;
}
static uint8_t prv_read(uint16_t instanceId,
                        int * numDataP,
                        lwm2m_data_t ** dataArrayP,
                        lwm2m_object_t * objectP)
{
     return COAP_205_CONTENT;
}

static uint8_t prv_discover(uint16_t instanceId,
                            int * numDataP,
                            lwm2m_data_t ** dataArrayP,
                            lwm2m_object_t * objectP)
{
    return COAP_205_CONTENT;
}

static uint8_t prv_write(uint16_t instanceId,
                         int numData,
                         lwm2m_data_t * dataArray,
                         lwm2m_object_t * objectP)
{
    return COAP_204_CHANGED;
}


static uint8_t prv_delete(uint16_t id,
                          lwm2m_object_t * objectP)
{
	return COAP_204_CHANGED;
}

static uint8_t prv_create(uint16_t instanceId,
                          int numData,
                          lwm2m_data_t * dataArray,
                          lwm2m_object_t * objectP)
{
	return COAP_204_CHANGED;
}

static uint8_t prv_exec(uint16_t instanceId,
                        uint16_t resourceId,
                        uint8_t * buffer,
                        int length,
                        lwm2m_object_t * objectP)
{
	return COAP_204_CHANGED;
}


lwm2m_object_t * get_mcu_object(uint32_t serialNum,object_instance_array_t objInsArray[])
{
    lwm2m_object_t * dataReportObject;

    dataReportObject = (lwm2m_object_t *)ct_lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != dataReportObject)
    {
        int i;
        lwm2m_list_t * targetP;

        memset(dataReportObject, 0, sizeof(lwm2m_object_t));

        dataReportObject->objID = objInsArray[serialNum].objId;
        for(i = 0; i < objInsArray[serialNum].intsanceCount; i++)
        {
        	targetP = (lwm2m_list_t *)ct_lwm2m_malloc(sizeof(lwm2m_list_t));
			if (NULL == targetP) goto exit;
			memset(targetP, 0, sizeof(lwm2m_list_t));
			targetP->id=objInsArray[serialNum].intanceId[i];
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

