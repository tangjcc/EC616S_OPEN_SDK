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
#include "at_example_task.h"
#include "cms_comm.h"
#include "task.h"
#include "queue.h"
#include "atec_example.h"
#include "atc_reply.h"
#include "ec_example_api.h"

extern QueueHandle_t custAtDemoMsgHandle;

#define _DEFINE_AT_REQ_FUNCTION_LIST_
/**
  \fn          CmsRetId ecTESTDEMO(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  ecTESTDEMO(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 outValue = 0xff;

    switch (operaType)
    {
        case AT_TEST_REQ:          /* AT+TESTDEMO=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+TESTDEMO: 1-at demo task create");
            break;
        }

        case AT_READ_REQ:             /* AT+TESTDEMO?  */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+TESTDEMO: 1-at demo task create");
            break;
        }

        case AT_SET_REQ:             /* AT+TESTDEMO=  */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &outValue, EC_TESTDEMO_VALUE_MIN, EC_TESTDEMO_VALUE_MAX, EC_TESTDEMO_VALUE_DEF)) != AT_PARA_ERR)
            {
                if(outValue == 1)
                {
                    ret = custAtDemoTaskInit();
                }
                else
                {

                }
            }

            if(ret == AT_PARA_OK)
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_EXAMPLE_ERROR, EXAMPLE_PARAM_ERROR, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_EXAMPLE_ERROR, ( EXAMPLE_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ecTESTA(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  ecTESTA(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 outValue = 0;
    CHAR outValueStr[EC_TESTA_STR_BUF_SIZE] = {0};
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:          /* AT+TESTA=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+TESTA: TEST");
            break;
        }

        case AT_READ_REQ:             /* AT+TESTA?  */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+TESTA: GET");
            break;
        }

        case AT_SET_REQ:             /* AT+TESTA=  */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &outValue, EC_TESTA_VALUE_MIN, EC_TESTA_VALUE_MAX, EC_TESTA_VALUE_DEF)) != AT_PARA_ERR)
            {
                if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, (UINT8 *)outValueStr, EC_TESTA_STR_BUF_SIZE, &length , (CHAR *)EC_TESTA_STR_DEFAULT)) != AT_PARA_ERR)
                {
                    if(strncasecmp((const CHAR*)outValueStr, "test", strlen("test")) == 0)
                    {
                        ret = ecTestaApi(reqHandle, custAtDemoMsgHandle, AT_EC_REQ_TESTA, outValue, outValueStr); 
                    }
                    else
                    {
                        ret = AT_PARA_ERR;
                    }
                }
            }

            if(ret == AT_PARA_OK)
            {
                rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+TESTA: REQ");
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_EXAMPLE_ERROR, ( EXAMPLE_PARAM_ERROR), NULL);
            }
            break;
        }

        case AT_EXEC_REQ:             /* AT+CCTESTA  */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+TESTA: ACTION");
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_EXAMPLE_ERROR, ( EXAMPLE_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return rc;
}

/**
  \fn          CmsRetId ecTESTB(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  ecTESTB(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 outValue = 0;
    CHAR outValueStr[EC_TESTB_STR_BUF_SIZE] = {0};
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:          /* AT+TESTB=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+TESTB: TEST");
            break;
        }

        case AT_READ_REQ:             /* AT+TESTB? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+TESTB: GET");
            break;
        }

        case AT_SET_REQ:             /* AT+TESTB= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &outValue, EC_TESTB_VALUE_MIN, EC_TESTB_VALUE_MAX, EC_TESTB_VALUE_DEF)) != AT_PARA_ERR)
            {
                if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, (UINT8 *)outValueStr, EC_TESTB_STR_BUF_SIZE, &length , (CHAR *)EC_TESTB_STR_DEFAULT)) != AT_PARA_ERR)
                {
                    if(strncasecmp((const CHAR*)outValueStr, "test", strlen("test")) == 0)
                    {
                        if(custAtDemoMsgHandle != NULL)
                        {
                            ret = ecTestbApi(reqHandle, custAtDemoMsgHandle, AT_EC_REQ_TESTB, outValue, outValueStr); 
                        }
                        else
                        {
                            ret = AT_PARA_ERR;
                        }
                    }
                    else
                    {
                        ret = AT_PARA_ERR;
                    }
                }
            }

            if(ret == AT_PARA_OK)
            {
                rc = CMS_RET_SUCC;
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_EXAMPLE_ERROR, ( EXAMPLE_PARAM_ERROR), NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_EXAMPLE_ERROR, ( EXAMPLE_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return rc;
}


/**
  \fn          CmsRetId ecTESTC(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId  ecTESTC(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId rc = CMS_FAIL;
    INT32 ret = AT_PARA_ERR;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32 outValue = 0;
    CHAR outValueStr[EC_TESTC_STR_BUF_SIZE] = {0};
    UINT16 length = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:          /* AT+TESTC=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+TESTC: TEST");
            break;
        }

        case AT_READ_REQ:             /* AT+TESTC?  */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+TESTC: GET");
            break;
        }

        case AT_SET_REQ:             /* AT+TESTC=  */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &outValue, EC_TESTC_VALUE_MIN, EC_TESTC_VALUE_MAX, EC_TESTC_VALUE_DEF)) != AT_PARA_ERR)
            {
                if((ret = atGetStrValue(pAtCmdReq->pParamList, 1, (UINT8 *)outValueStr, EC_TESTC_STR_BUF_SIZE, &length , (CHAR *)EC_TESTC_STR_DEFAULT)) != AT_PARA_ERR)
                {
                    if(strncasecmp((const CHAR*)outValueStr, "test", strlen("test")) == 0)
                    {
                        if(custAtDemoMsgHandle != NULL)
                        {
                            ret = ecTestcApi(reqHandle, custAtDemoMsgHandle, AT_EC_REQ_TESTC, outValue, outValueStr); 
                        }
                        else
                        {
                            ret = AT_PARA_ERR;
                        }
                    }
                    else
                    {
                        ret = AT_PARA_ERR;
                    }
                }
            }

            if(ret == AT_PARA_OK)
            {
                rc = CMS_RET_SUCC;
            }
            else
            {
                rc = atcReply(reqHandle, AT_RC_EXAMPLE_ERROR, ( EXAMPLE_PARAM_ERROR), NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_EXAMPLE_ERROR, ( EXAMPLE_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return rc;
}

