/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename:
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "atc_decoder.h"
#include "ctype.h"
#include "cmicomm.h"
#include "at_adc_task.h"
#include "cms_comm.h"
#include "task.h"
#include "queue.h"
#include "atec_adc.h"
#include "atc_reply.h"
#include "ec_adc_api.h"

extern QueueHandle_t atAdcMsgHandle;

#define _DEFINE_AT_REQ_FUNCTION_LIST_

/**
  \fn          CmsRetId ecADC(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId ecADC(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR outValueStr[EC_ADC_STR_BUF_SIZE] = {0};
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:          /* AT+ECADC=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+ECADC: <option>\r\n");
            break;
        }

        case AT_SET_REQ:             /* AT+ECADC= */
        {
            if((ret = atGetStrValue(pAtCmdReq->pParamList, 0, (UINT8 *)outValueStr, EC_ADC_STR_BUF_SIZE, &length , (CHAR *)EC_ADC_STR_DEFAULT)) != AT_PARA_ERR)
            {
                ret = atAdcTaskInit();

                if(ret != 0)
                {
                    rc = atcReply(reqHandle, AT_RC_ADC_ERROR, ADC_TASK_NOT_CREATE, NULL);

                    return rc;
                }

                if(strncasecmp((const CHAR*)outValueStr, "all", strlen("all")) == 0)
                {
                    ret = adcSendMsg(reqHandle, atAdcMsgHandle, (AT_ADC_REQ_BITMAP_TEMP | AT_ADC_REQ_BITMAP_VBAT));
                }
                else if(strncasecmp((const CHAR*)outValueStr, "temp", strlen("temp")) == 0)
                {
                    ret = adcSendMsg(reqHandle, atAdcMsgHandle, AT_ADC_REQ_BITMAP_TEMP);
                }
                else if(strncasecmp((const CHAR*)outValueStr, "vbat", strlen("vbat")) == 0)
                {
                    ret = adcSendMsg(reqHandle, atAdcMsgHandle, AT_ADC_REQ_BITMAP_VBAT);
                }
                else
                {
                    ret = AT_PARA_ERR;
                }
            }

            if(ret == AT_PARA_OK)
            {
                rc = CMS_RET_SUCC;
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_ADC_ERROR, ADC_PARAM_ERROR, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_ADC_ERROR, ADC_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return rc;
}

