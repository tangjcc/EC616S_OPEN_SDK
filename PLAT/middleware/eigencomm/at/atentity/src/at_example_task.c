#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "ctype.h"
#include "cmicomm.h"
#include "task.h"
#include "queue.h"
#include "commontypedef.h"
#include "at_example_task.h"

StaticTask_t  custAtDemoTask;
UINT8         custAtDemoTaskStack[EG_AT_DEMO_TASK_STACK_SIZE];
osThreadId_t  custAtDemoTaskHandle = NULL;
QueueHandle_t custAtDemoMsgHandle = NULL;

void ecSendMsg(UINT32 atHandle, QueueHandle_t msgHandle, INT32 cmdType, UINT32 data, CHAR *dataPtr)
{
    EC_SEND_Q_MSG ecMsg;
    INT32 dataLen = 0;
    CHAR *dataPtrTemp = NULL;

    memset(&ecMsg, 0, sizeof(ecMsg));
    ecMsg.cmd_type = cmdType;
    ecMsg.reqhandle = atHandle;
    ecMsg.send_data.data_value = data;

    if(dataPtr != NULL)
    {
        dataLen = strlen(dataPtr);
        if(dataLen > 0)
        {
            dataPtrTemp = malloc(dataLen+1);
            memset(dataPtrTemp, 0, (dataLen+1));
            memcpy(dataPtrTemp, dataPtr, dataLen);
        }
    }
    ecMsg.send_data.data_ptr = dataPtrTemp;

    xQueueSend(msgHandle, &ecMsg, EG_MSG_TIMEOUT);
}

void ecFreeMsg(const EC_SEND_Q_MSG *ecMsg)
{
    if(ecMsg->send_data.data_ptr != NULL)
    {
        free(ecMsg->send_data.data_ptr);
    }
}

#define _DEFINE_AT_REQ_FUNCTION_LIST_
int ecFuncTestA(int input)
{
    int ret = 0;

    if(input == 0)
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}

int ecFuncTestB(int input)
{
    int ret = 0;

    if(input == 0)
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}

int ecFuncTestC(int input)
{
    int ret = 0;

    if(input == 0)
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}


/****************************************************************************************
*  FUNCTION:  ecTestaApi
*
*  PARAMETERS:
*
*  DESCRIPTION: , implementation of AT+TESTA=
*
*  RETURNS: RetCodeT
*
****************************************************************************************/
int ecTestaApi(UINT32 atHandle, QueueHandle_t msgHandle, INT32 cmdType, UINT32 data, CHAR *dataPtr)
{

    return 0;
}

/****************************************************************************************
*  FUNCTION:  ecTestbApi
*
*  PARAMETERS:
*
*  DESCRIPTION: , implementation of AT+TESTB=
*
*  RETURNS: RetCodeT
*
****************************************************************************************/
int ecTestbApi(UINT32 atHandle, QueueHandle_t msgHandle, INT32 cmdType, UINT32 data, CHAR *dataPtr)
{
    ecSendMsg(atHandle, msgHandle, cmdType, data, dataPtr);
    return 0;
}

/****************************************************************************************
*  FUNCTION:  ecTestcApi
*
*  PARAMETERS:
*
*  DESCRIPTION: , implementation of AT+TESTC=
*
*  RETURNS: RetCodeT
*
****************************************************************************************/
int ecTestcApi(UINT32 atHandle, QueueHandle_t msgHandle, INT32 cmdType, UINT32 data, CHAR *dataPtr)
{
    ecSendMsg(atHandle, msgHandle, cmdType, data, dataPtr);
    return 0;
}

int ecCmdStateMachine(const EC_SEND_Q_MSG *ecMsg)
{
    int rc = 0;
    int primSize = 0;
    EC_CNF_MSG cnfMsg;
    EC_IND_MSG indMsg;

    switch(ecMsg->cmd_type)
    {
        case AT_EC_REQ_TESTA:
			ecFuncTestA(0x1);
            break;

        case AT_EC_REQ_TESTB:
            /*send confirm msg to at task*/
            primSize = sizeof(cnfMsg);
            cnfMsg.data = 0x1;

			ecFuncTestB(0x1);
            #ifdef FEATURE_AT_ENABLE
			applSendCmsCnf(ecMsg->reqhandle, APPL_RET_SUCC, APPL_EXAMPLE, APPL_EXAMPLE_ECTESTB_CNF, primSize, &cnfMsg);
            #endif

            break;

        case AT_EC_REQ_TESTC:
            /*send confirm msg to at task*/
            primSize = sizeof(cnfMsg);
            cnfMsg.data = 0x1;

			ecFuncTestC(0x1);
            #ifdef FEATURE_AT_ENABLE
			applSendCmsCnf(ecMsg->reqhandle, APPL_RET_SUCC, APPL_EXAMPLE, APPL_EXAMPLE_ECTESTC_CNF, primSize, &cnfMsg);
            #endif

            /*send indcation msg to at task*/
            primSize = sizeof(indMsg);
            indMsg.data = 0x2;
            #ifdef FEATURE_AT_ENABLE
			applSendCmsInd(BROADCAST_IND_HANDLER, APPL_EXAMPLE, APPL_EXAMPLE_ECTESTC_IND, primSize, (void *)&indMsg);
            #endif
            break;

        default:
            break;
    }

    return rc;
}


void custAtDemoTaskProcess(void *argument)
{
    EC_SEND_Q_MSG ecMsg;

    while(1)
    {
        /* recv msg (block mode) */
        xQueueReceive(custAtDemoMsgHandle, &ecMsg, osWaitForever);
        ecCmdStateMachine(&ecMsg);
        ecFreeMsg(&ecMsg);
    }
}

int custAtDemoTaskInit(void)
{
	int ret = 0;

	if(custAtDemoTaskHandle == NULL)
	{
		custAtDemoMsgHandle = xQueueCreate(EG_MSG_MAX_NUMB, sizeof(EC_SEND_Q_MSG));

		osThreadAttr_t task_attr;
		memset(&task_attr, 0, sizeof(task_attr));
		task_attr.name = "custAtDemo";
		task_attr.stack_mem = custAtDemoTaskStack;
		task_attr.stack_size = EG_AT_DEMO_TASK_STACK_SIZE;
		task_attr.priority = osPriorityNormal;

		task_attr.cb_mem = &custAtDemoTask;//task control block
		task_attr.cb_size = sizeof(StaticTask_t);//size of task control block
		memset(custAtDemoTaskStack, 0xA5,EG_AT_DEMO_TASK_STACK_SIZE);

		custAtDemoTaskHandle = osThreadNew(custAtDemoTaskProcess, NULL,&task_attr);
		if(custAtDemoTaskHandle == NULL)
		{
			ret = 1;
		}
	}
	return ret;
}
