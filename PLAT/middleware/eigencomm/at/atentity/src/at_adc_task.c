#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "ctype.h"
#include "cmicomm.h"
#include "task.h"
#include "queue.h"
#include "commontypedef.h"
#include "at_adc_task.h"
#include "hal_adc.h"

#ifdef CHIP_EC616S
#include "adc_ec616s.h"
#elif defined CHIP_EC616
#include "adc_ec616.h"
#endif

static StaticTask_t  atAdcTask;
static UINT8         atAdcTaskStack[AT_ADC_TASK_STACK_SIZE];
static osThreadId_t  atAdcTaskHandle = NULL;
static osEventFlagsId_t atAdcEvtHandle = NULL;

QueueHandle_t atAdcMsgHandle = NULL;

volatile static uint32_t vbatChannelResult = 0;
volatile static uint32_t thermalChannelResult = 0;

#define _DEFINE_AT_REQ_FUNCTION_LIST_

int32_t adcSendMsg(uint32_t atHandle, QueueHandle_t msgHandle, uint32_t req)
{
    int32_t ret;
    adcReqMsg ecMsg;

    ecMsg.reqhandle = atHandle;
    ecMsg.request = req;

    ret = osMessageQueuePut(msgHandle, &ecMsg, 0, AT_ADC_MSG_TIMEOUT);

    if(ret != osOK)
    {
        return AT_PARA_ERR;
    }
    else
    {
        return AT_PARA_OK;
    }
}

static void ADC_VbatChannelCallback(uint32_t result)
{
    vbatChannelResult = result;
    osEventFlagsSet(atAdcEvtHandle, AT_ADC_REQ_BITMAP_VBAT);
}

static void ADC_ThermalChannelCallback(uint32_t result)
{
    thermalChannelResult = result;
    osEventFlagsSet(atAdcEvtHandle, AT_ADC_REQ_BITMAP_TEMP);
}

static int32_t adcConvertProcess(uint32_t request)
{
    int32_t ret;

    adc_config_t adcConfig;

    ADC_GetDefaultConfig(&adcConfig);

    osEventFlagsClear(atAdcEvtHandle, request);

    if(request & AT_ADC_REQ_BITMAP_VBAT)
    {
        adcConfig.channelConfig.vbatResDiv = ADC_VbatResDivRatio3Over16;
        ADC_ChannelInit(ADC_ChannelVbat, ADC_UserPLAT, &adcConfig, ADC_VbatChannelCallback);
        ADC_StartConversion(ADC_ChannelVbat, ADC_UserPLAT);
    }

    if(request & AT_ADC_REQ_BITMAP_TEMP)
    {
        adcConfig.channelConfig.thermalInput = ADC_ThermalInputVbat;
        ADC_ChannelInit(ADC_ChannelThermal, ADC_UserPLAT, &adcConfig, ADC_ThermalChannelCallback);
        ADC_StartConversion(ADC_ChannelThermal, ADC_UserPLAT);
    }

    ret = osEventFlagsWait(atAdcEvtHandle, request, osFlagsWaitAll, AT_ADC_GET_RESULT_TIMOUT);

    if(request & AT_ADC_REQ_BITMAP_VBAT)
    {
        ADC_ChannelDeInit(ADC_ChannelVbat, ADC_UserPLAT);

    }

    if(request & AT_ADC_REQ_BITMAP_TEMP)
    {
        ADC_ChannelDeInit(ADC_ChannelThermal, ADC_UserPLAT);
    }

    if(ret == request)
    {
        return 0;
    }
    else
    {   // timeout
        return -1;
    }
}

static void atAdcTaskProcess(void *argument)
{
    adcReqMsg regMsg;
    int32_t ret;
    int32_t primSize = 0;
    adcCnfMsg cnfMsg = {0};
    osStatus_t status;

    while(1)
    {
        /* recv msg (block mode) */
        status = osMessageQueueGet(atAdcMsgHandle, &regMsg, 0, osWaitForever);
        if(status == osOK)
        {
            ret = adcConvertProcess(regMsg.request);
            /*send confirm msg to at task*/
            primSize = sizeof(cnfMsg);

            if(ret == -1)
            {
                // timeout
                #ifdef FEATURE_AT_ENABLE
                applSendCmsCnf(regMsg.reqhandle, APPL_RET_FAIL, APPL_ADC, APPL_ADC_CNF, primSize, &cnfMsg);
                #endif
            }
            else
            {
                cnfMsg.ack = regMsg.request;
                cnfMsg.data[0] = HAL_ADC_ConvertThermalRawCodeToTemperature(thermalChannelResult);
                cnfMsg.data[1] = HAL_ADC_CalibrateRawCode(vbatChannelResult) * 16 / 3;
                #ifdef FEATURE_AT_ENABLE
                applSendCmsCnf(regMsg.reqhandle, APPL_RET_SUCC, APPL_ADC, APPL_ADC_CNF, primSize, &cnfMsg);
                #endif
            }
        }
    }
}

int32_t atAdcTaskInit(void)
{
    if(atAdcMsgHandle == NULL)
    {
        atAdcMsgHandle = osMessageQueueNew(AT_ADC_MSG_MAX_NUM, sizeof(adcReqMsg), NULL);

        if(atAdcMsgHandle == NULL)
        {
            return 1;
        }
    }

    if(atAdcEvtHandle == NULL)
    {
        atAdcEvtHandle = osEventFlagsNew(NULL);

        if(atAdcEvtHandle == NULL)
        {
            return 1;
        }
    }

    if(atAdcTaskHandle == NULL)
    {
        osThreadAttr_t task_attr;
        memset(&task_attr, 0, sizeof(task_attr));
        task_attr.name = "atAdc";
        task_attr.stack_mem = atAdcTaskStack;
        task_attr.stack_size = AT_ADC_TASK_STACK_SIZE;
        task_attr.priority = osPriorityNormal;

        task_attr.cb_mem = &atAdcTask;//task control block
        task_attr.cb_size = sizeof(StaticTask_t);//size of task control block
        memset(atAdcTaskStack, 0xA5,AT_ADC_TASK_STACK_SIZE);

        atAdcTaskHandle = osThreadNew(atAdcTaskProcess, NULL, &task_attr);
        if(atAdcTaskHandle == NULL)
        {
            return 1;
        }
    }

    return 0;
}
