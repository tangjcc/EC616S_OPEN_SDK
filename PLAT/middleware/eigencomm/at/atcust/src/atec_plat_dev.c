/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_plat_dev.c
*
*  Description: Process device service related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "hal_uart.h"
#include "bsp.h"
#include "atec_plat_dev.h"
#include "atc_decoder.h"
#include "at_util.h"
#include "ip_addr.h"
#include "nvram.h"
#include "osasys.h"
#include "queue.h"
#include "plat_config.h"
#include "mw_config.h"
#include "cmicomm.h"


#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "flash_ec616_rt.h"
#include "ecpm_ec616_external.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif

#elif defined CHIP_EC616S
#include "flash_ec616s_rt.h"
#include "ecpm_ec616s_external.h"
#include "slpman_ec616s.h"
#endif
#include "os_callback_hook.h"
#include "lfs_port.h"

#include "app.h"
#include "atec_dev.h"

extern INT32 curr_task_numb;
extern task_node_list ec_task_list[TASK_LIST_LEN];

extern INT32 RfAtTestCmd(UINT8* dataIn, UINT16 length, UINT8* dataOut, UINT16* lenOut);
extern INT32 RfAtNstCmd(UINT8* dataIn, UINT16 length, UINT8* dataOut, UINT16* lenOut);
extern BOOL RfFcTableValid(void);

extern void ec_show_task_history(void);
extern bool atCmdIsBaudRateValid(uint32_t baudRate);
extern void atCmdUartChangeConfig(UINT8 atChannel, UINT32 newBaudRate, UINT32 newFrameFormat, CHAR saveFlag);

#define _DEFINE_AT_REQ_FUNCTION_LIST_

/**
  \fn          CmsRetId pdevREST(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function for trigger system reset
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevRST(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_RET_SUCC;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 paramNumb = pAtCmdReq->paramRealNum;
    UINT8 paraStr[16] = {0};

    UINT16  paraLen = 0;
    INT32   paraValue = 0;
    UINT8   paraIdx = 0;
    UINT32 RstDelayValid = 0;

    extern void atCmdUartResetChip(UINT32 delay_ms);

    switch (operaType)
    {
        case AT_EXEC_REQ:           /* AT+ECRST */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);

            atCmdUartResetChip(0);

            break;
        }
       case AT_SET_REQ:

            if((paramNumb > 0) && (paramNumb & 0x01) == 0)
            {
                for(paraIdx = 0; paraIdx < paramNumb; paraIdx++)
                {
                    memset(paraStr, 0x00, sizeof(paraStr));
                    if(atGetStrValue(pParamList, paraIdx, paraStr,
                                     sizeof(paraStr), &paraLen, NULL) == AT_PARA_ERR)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, devRST_1, P_WARNING, 0, "AT+ECRST, invalid input string parameter");
                        break;
                    }
                    paraIdx++;
                    if(strncasecmp((const CHAR*)paraStr, "delay", strlen("delay")) == 0)
                    {
                        if(atGetNumValue(pParamList, paraIdx, &paraValue,
                                         0, ATC_ECRST_MAX_DELAY_MS, NULL) == AT_PARA_OK)
                        {
                            RstDelayValid = 1;
                            break;
                        }
                        else
                        {
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devRST_2, P_WARNING, 0, "AT+ECRST, invalid input 'delay' value");
                            break;
                        }
                    }

                }
                if (RstDelayValid != 1)
                {
                    ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
                    atCmdUartResetChip(paraValue);
                }
                break;
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
                break;

            }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }
    return ret;
}

/**
  \fn          CmsRetId pdevFACTORYTEST(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function for factory test 
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevFACTORYTEST(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_RET_SUCC;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 paramNumb = pAtCmdReq->paramRealNum;

	INT8   paraStr[2][ATC_ECCFG_0_MAX_PARM_STR_LEN] = {0};  //max STR parameter length is 32;
    UINT16  paraLen[12] = {0};
	INT32	paraNum;
	uint8_t atcmd[16];
	uint8_t calibration;
	static uint8_t mode = 0;

	user_printf("\r\n");
	user_printf("operaType = %d\r\n",operaType);
	user_printf("paramNumb = %d\r\n",paramNumb);

	if(mode == 0)
	{
		mode = 1;
		app_user_atcmd_init();
		osDelay(100);
	}

    switch (operaType)
    {
        case AT_EXEC_REQ:           /* AT+FACTORYTEST */
        {
			user_printf("\r\npdevFACTORYTEST AT_EXEC_REQ\r\n\r\n");
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

		case AT_SET_REQ:           /* AT+FACTORYTEST=*/
        {
			user_printf("\r\npdevFACTORYTEST AT_SET_REQ\r\n\r\n");

			if(paramNumb != 2)
			{
			    ret = atcReply(atHandle, AT_RC_ERROR , 0, "param number error");
				break;
			}

			if(atGetStrValue(pParamList, 0, (UINT8 *)paraStr[0],
								ATC_ECCFG_0_MAX_PARM_STR_LEN, &paraLen[0], 
								ATC_ECCFG_0_MAX_PARM_STR_DEFAULT) == AT_PARA_ERR)
			{
				ret = atcReply(atHandle, AT_RC_ERROR, 0, "param 0 error");
				break;
			}
			user_printf("paramlist[%d] = %s\r\n",0,paraStr[0]);
			user_printf("paraLen[%d] = %d\r\n",0,paraLen[0]);		

			if(atGetNumValue(pAtCmdReq->pParamList, 1, &paraNum, 
								0,31,0) == AT_PARA_ERR)
			{
				ret = atcReply(atHandle, AT_RC_ERROR, 0, "param 1 error");
				break;
			}

			user_printf("paraNum = %d\r\n",paraNum);
		
			if(strcmp((const char*)paraStr[0],"GPIO") == 0)
			{
				user_printf("GPIO get.\r\n");
				atcmd[0] = 0;
				atcmd[1] = paraNum;
				userAtCmdSemRelease(ATCMD_TEST_SET,atcmd,2);
			}
			else if(strcmp((const char*)paraStr[0],"PWM") == 0)
			{
				user_printf("PWM get.\r\n");
				atcmd[0] = 1;
				atcmd[1] = paraNum;
				userAtCmdSemRelease(ATCMD_TEST_SET,atcmd,2);
			}
			else if(strcmp((const char*)paraStr[0],"ADC") == 0)
			{
				user_printf("ADC get.\r\n");
				atcmd[0] = 2;
				atcmd[1] = paraNum;
				userAtCmdSemRelease(ATCMD_TEST_SET,atcmd,2);
			}
			else if(strcmp((const char*)paraStr[0],"STEP") == 0)
			{
				user_printf("STEP get.\r\n");
				atcmd[0] = 3;
				atcmd[1] = paraNum;
				userAtCmdSemRelease(ATCMD_TEST_SET,atcmd,2);
			}
			else if(strcmp((const char*)paraStr[0],"FILE") == 0)
			{
				user_printf("FILE get.\r\n");
				if(paraNum == 1)
				{	
					user_printf("FILE write\r\n");
					file_test_write();
				}
				else if(paraNum == 0)
				{
					user_printf("FILE read\r\n");
					file_test_read();
				}
			}
			else if(strcmp((const char*)paraStr[0],"DEBUG") == 0)
			{
				user_printf("DEBUG get.\r\n");
				if(paraNum == 1)
				{	
					user_printf("DEBUG enable\r\n");
					enable_debug_set(1);
				}
				else if(paraNum == 0)
				{
					user_printf("DEBUG disable\r\n");
					enable_debug_set(0);
				}
			}
			else if(strcmp((const char*)paraStr[0],"CALIBRATION") == 0)
			{
				user_printf("CALIBRATION get.\r\n");
				if(paraNum == 1)
				{	
					
				}
				else if(paraNum == 0)
				{
					user_printf("CALIBRATION query\r\n");
					calibration = file_test_read();
					if(calibration == 0)
					{
						ret = atcReply(atHandle, AT_RC_OK, 0, "+CALIBRATION YES");
					}
					else
					{
						ret = atcReply(atHandle, AT_RC_OK, 0, "+CALIBRATION NO");
					}	
				}
				break;
			}
			else
			{
		    	ret = atcReply(atHandle, AT_RC_ERROR, 0, "param 1 value error");
				break;
			}

		    ret = atcReplyNoOK(atHandle, AT_RC_OK, 0, NULL);			
            break;
        }

		case AT_TEST_REQ:           /* AT+FACTORYTEST=?*/
        {
			user_printf("\r\npdevFACTORYTEST AT_TEST_REQ\r\n\r\n");
			
			at_reply("\r\n");
			at_reply("+FACTORYTEST=(GPIO),(0-13)\r\n");
			at_reply("+FACTORYTEST=(PWM),(4-5)\r\n");
			at_reply("+FACTORYTEST=(ADC),(1-4)\r\n");
			at_reply("+FACTORYTEST=(STEP),(0-15)\r\n");
			at_reply("+FACTORYTEST=(FILE),(0-1)\r\n");
			at_reply("+FACTORYTEST=(DEBUG),(0-1)\r\n");
			at_reply("+FACTORYTEST=(CALIBRATION),(0-1)\r\n");
            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

		case AT_READ_REQ:           /* AT+FACTORYTEST?*/
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }
    return ret;
}

/**
  \fn          CmsRetId pdevECTASKINFO(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function to show task info
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECTASKINFO(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    CHAR printf_buf[EC_PRINT_BUF_LEN] = {0};
    CHAR * ec_cmd_buf = malloc(EC_CMD_BUF_LEN);

    if(ec_cmd_buf == NULL)
    {
        ret = atcReply(atHandle, AT_RC_ERROR, CME_MEMORY_FAILURE, NULL);
        return ret;
    }
    memset(ec_cmd_buf, 0, EC_CMD_BUF_LEN);
    snprintf(printf_buf, EC_PRINT_BUF_LEN,"task_name...........status...prio....stack...task_id\r\n");
    ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)printf_buf);

    vTaskList(ec_cmd_buf);
    ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)ec_cmd_buf);

    memset(printf_buf, 0, EC_PRINT_BUF_LEN);
    memset(ec_cmd_buf, 0, EC_CMD_BUF_LEN);
    snprintf(printf_buf, EC_PRINT_BUF_LEN,"task_name............count............used\r\n");
    ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)printf_buf);

    vTaskGetRunTimeStats_ec(ec_cmd_buf);
    ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)ec_cmd_buf);
    free(ec_cmd_buf);

    ret = atcReply(atHandle, AT_RC_OK, 0, NULL);

    return ret;
}

/**
  \fn          CmsRetId pdevECTASKHISTINFO(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function to show latest task schedule info
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECTASKHISTINFO(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 i = 0;
    UINT32 task_curr_index = 0;
    CHAR *send_buf = NULL;
    UINT32 send_buf_len = 0;

    send_buf_len = sizeof(task_node_list)+ATC_TASKINFO_LEN;
    send_buf = malloc(send_buf_len);

    if(send_buf == NULL)
    {
        ret = atcReply(atHandle, AT_RC_ERROR, CME_MEMORY_FAILURE, NULL);
        return ret;
    }
    
    if(curr_task_numb != 0)
    {
        task_curr_index = curr_task_numb - 1;
    }
    else
    {
        task_curr_index = TASK_LIST_LEN - 1;
    }

    for(i=task_curr_index; i>0; i--)
    {
        taskENTER_CRITICAL();
        memset(send_buf, 0, send_buf_len);

        snprintf(send_buf, send_buf_len,"task:%s...\r\n",ec_task_list[i].ec_task_name);

        taskEXIT_CRITICAL();
        ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)send_buf);
    }

    for(i=(TASK_LIST_LEN-1); i>task_curr_index; i--)
    {
        taskENTER_CRITICAL();
        memset(send_buf, 0, send_buf_len);

        snprintf(send_buf, send_buf_len,"task:%s...\r\n",ec_task_list[i].ec_task_name);

        taskEXIT_CRITICAL();
        ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)send_buf);
    }

    free(send_buf);

    ret = atcReply(atHandle, AT_RC_OK, 0, NULL);

    return ret;
}

/**
  \fn          CmsRetId pdevECSHOWMEM(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function to show heap memory usage
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECSHOWMEM(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32 curr_free_heap;
    UINT32 min_free_heap;
    CHAR printf_buf[EC_PRINT_BUF_LEN] = {0};

    curr_free_heap = xPortGetFreeHeapSize();
    min_free_heap = xPortGetMinimumEverFreeHeapSize();
    snprintf(printf_buf,EC_PRINT_BUF_LEN, "curr_free_heap:%d...min_free_heap:%d\r\n",curr_free_heap, min_free_heap);

    ret = atcReply(atHandle, AT_RC_OK, 0, printf_buf);

    return ret;
}

/**
  \fn          CmsRetId pdevSYSTEST(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function for system test
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevSYSTEST(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT8   testBuf[56] = {0};
    UINT16  testBufLen = 0;
    UINT32  valFlag = 0;
    INT32   dec=0;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT^SYSTEST=? */
        case AT_READ_REQ:            /* AT^SYSTEST?  */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"^SYSTEST\r\n");
            break;
        }

        case AT_SET_REQ:              /* AT+ECSYSTEST= */
        {
            /* Extract the arguments starting with the functionality. */
            if(atGetMixValue(pParamList, 0, &valFlag, testBuf, ATC_SYSTEST_0_STR_MAX_LEN, &testBufLen, NULL,
                             &dec, ATC_SYSTEST_0_VAL_MIN, ATC_SYSTEST_0_VAL_MAX, ATC_SYSTEST_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if(strcmp((CHAR*)testBuf, "handshake") == 0)
                {
                    ret = atcReply(atHandle, AT_RC_NO_RESULT, 0, "KO");
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                }
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:           /* AT^SYSTEST */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId pdevECSYSTEST(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function for system test
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECSYSTEST(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT8   testParam[ATC_SYSTEST_0_STR_MAX_LEN] = {0};
    UINT16  testParamLen = 0;
    //UINT32  valFlag = 0;
    //INT32   dec=0;
    UINT32  atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECSYSTEST=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECSYSTEST: (\"assert\",\"hardfault\",\"handshake\",\"testwdt\",\"fsassert\",\"atinfo\")");
            break;
        }

        case AT_SET_REQ:              /* AT+ECSYSTEST= */
        {
            if (atGetStrValue(pParamList, 0, testParam,
                              ATC_SYSTEST_0_STR_MAX_LEN, &testParamLen, PNULL) == AT_PARA_OK)
            {
                if (strcasecmp((CHAR*)testParam, "assert") == 0)
                {
                    EC_ASSERT(0, 0, 0, 0);
                    // fix Warning:  #111-D: statement is unreachable
                    //ret = atcReply( atHandle, AT_RC_OK, 0, NULL);

                }
                else if (strcasecmp((CHAR*)testParam, "hardfault") == 0)
                {

                    *((UINT32 *)0x0)=0;
                    ret = atcReply(atHandle, AT_RC_OK, 0, NULL);

                }
                else if (strcasecmp((CHAR*)testParam, "handshake") == 0)
                {
                    ret = atcReply(atHandle, AT_RC_NO_RESULT, 0, "KO");
                }
                else if (strcasecmp((CHAR*)testParam, "testwdt") == 0)
                {
                    // stop kick wdt
                    __disable_irq();
                    while(1);
                }
                else if (strcasecmp((CHAR*)testParam, "fsassert") == 0)
                {
                    EC_ASSERT(0,0x46535F41,0x73736572,0x745F7346);
                    //EC_ASSERT(0, 0, 0, 0);
                    // fix Warning:  #111-D: statement is unreachable
                    //ret = atcReply( atHandle, AT_RC_OK, 0, NULL);

                }
                else if (strcasecmp((CHAR*)testParam, "atinfo") == 0)
                {
                    #if defined CHIP_EC616S
                    {
                        #ifdef FEATURE_REF_AT_ENABLE
                        ret = atcReply(atHandle, AT_RC_OK, 0, "+ECSYSTEST: \"atinfo\":\"EC616S_REF_AT\"");
                        #else
                        ret = atcReply(atHandle, AT_RC_OK, 0, "+ECSYSTEST: \"atinfo\":\"EC616S_EC_AT\"");
                        #endif
                    }
                    #else
                    {
                        #ifdef FEATURE_REF_AT_ENABLE
                        ret = atcReply(atHandle, AT_RC_OK, 0, "+ECSYSTEST: \"atinfo\":\"EC616_REF_AT\"");
                        #else
                        ret = atcReply(atHandle, AT_RC_OK, 0, "+ECSYSTEST: \"atinfo\":\"EC616_EC_AT\"");
                        #endif
                    }
                    #endif
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                }
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:           /* AT+ECSYSTEST */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId pdevECLOGDBVER(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function to show log  database version
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECLOGDBVER(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[32] = {0};
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR * LogDbVserion = DB_VERSION_UNIQ_ID;

    switch (operaType)
    {
        case AT_READ_REQ:            /* AT+ECLOGDBVER?  */
        {
            snprintf((char*)RspBuf,32,"+ECLOGDBVER: %s", LogDbVserion);
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)RspBuf);
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId pdevECPMUCFG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function to config PMU related setting
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECPMUCFG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CHAR RspBuf[96] = {0};
    INT32 pmuEnable, pmuMode;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECPMUCFG=? */
        {
            snprintf((char*)RspBuf, 96,"+ECPMUCFG: (0-1),(0-4)\r\n");

            ret = atcReply( atHandle, AT_RC_OK, 0, (CHAR *)RspBuf);
            break;
        }

        case AT_READ_REQ:            /* AT+ECPMUCFG?  */
        {
            UINT32 pmuEnMagic = BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_ENABLE_PM);
            pmuMode = BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_SLEEP_MODE);

            if((pmuEnMagic == PMU_ENABLE_MAGIC) || (pmuEnMagic == PMU_DISABLE_MAGIC))
            {
                if(pmuEnMagic == PMU_ENABLE_MAGIC)
                {
                    pmuEnable = true;
                    snprintf((char*)RspBuf,96, "+ECPMUCFG: %d,%d\r\n",
                                                    pmuEnable,
                                                    pmuMode);
                }
                else
                {
                    pmuEnable = false;
                    snprintf((char*)RspBuf, 96, "+ECPMUCFG: %d\r\n",
                                                                pmuEnable);
                }
            }
            else
            {
                snprintf((char*)RspBuf, 96, "+ECPMUCFG: 1,1\r\n");
            }

            ret = atcReply( atHandle, AT_RC_OK, 0, (CHAR *)RspBuf);

            break;
        }

        case AT_SET_REQ:              /* AT+ECPMUCFG= */
        {
            if(atGetNumValue(pParamList, 0, &pmuEnable,
                             PLAT_DISABLE_CONFIG_ACTION, PLAT_ENABLE_CONFIG_ACTION, PLAT_DISABLE_CONFIG_ACTION) != AT_PARA_ERR)
            {
                if(atGetNumValue(pParamList, 1, &pmuMode,
                                 LP_STATE_ACTIVE, LP_STATE_HIBERNATE, LP_STATE_ACTIVE) != AT_PARA_ERR)
                {
                    switch(pmuEnable)
                    {
                        case PLAT_DISABLE_CONFIG_ACTION:
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_SLEEP_MODE, 0);
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_ENABLE_PM, PMU_DISABLE_MAGIC);

                            pmuSetDeepestSleepMode(LP_STATE_ACTIVE);

                            BSP_SavePlatConfigToFs();
                            ret = CMS_RET_SUCC;
                            break;

                        case PLAT_ENABLE_CONFIG_ACTION:
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_SLEEP_MODE, (UINT8)pmuMode);
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_ENABLE_PM, PMU_ENABLE_MAGIC);
                            pmuSetDeepestSleepMode((LpState)pmuMode);

                            BSP_SavePlatConfigToFs();
                            ret = CMS_RET_SUCC;
                            break;

                        default:
                            break;
                    }
                }
            }

            if(ret == CMS_RET_SUCC)
            {
                ret = atcReply( atHandle, AT_RC_OK, 0, NULL);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }

            break;
        }

        default:
        {
            ret = atcReply( atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;

}





/**
  \fn          CmsRetId pdevECPCFG(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function to config platform related setting
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECPCFG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 paramNumb = pAtCmdReq->paramRealNum;
    CHAR RspBuf[128] = {0};
    UINT8 paraStr[ATC_ECPCFG_MAX_PARM_STR_LEN] = {0};

    // bitmap of config valid flag, platConfig and logConfig each occupies two bits, the lower bit indicates whether it's required to set while
    // the higher indicates the parameter check result
    UINT8   configValidFlag = 0;

    UINT16  paraLen = 0;
    INT32   paraValue = 0;
    UINT8   paraIdx = 0;

    plat_config_fs_t* fsPlatConfigPtr = NULL;
    plat_config_raw_flash_t* rawFlashPlatConfigPtr = NULL;

    UINT32 waitBeforeSlp = 0x0;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECPCFG=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECPCFG:<option>,<setting>\r\n");
            break;
        }
        case AT_READ_REQ:            /* AT+ECPCFG?  */
        {
            fsPlatConfigPtr = BSP_GetFsPlatConfig();
            rawFlashPlatConfigPtr = BSP_GetRawFlashPlatConfig();

            if(fsPlatConfigPtr && rawFlashPlatConfigPtr)
            {

                snprintf((char*)RspBuf, 128, "+ECPCFG: \"faultAction\":%d,\"dumpToATPort\":%d,\"startWDT\":%d,\"logCtrl\":%d,\"logLevel\":%d,\"logBaudrate\":%d,\"slpWaitTime\":%d\r\n",
                                                    rawFlashPlatConfigPtr->faultAction,
                                                    rawFlashPlatConfigPtr->dumpToATPort,
                                                    rawFlashPlatConfigPtr->startWDT,
                                                    rawFlashPlatConfigPtr->logControl,
                                                    rawFlashPlatConfigPtr->logLevel,
                                                    rawFlashPlatConfigPtr->uartBaudRate,
                                                    pmuGetWaitSlpCfg(fsPlatConfigPtr->slpWaitTime));

                ret = atcReply( atHandle, AT_RC_OK, 0, (CHAR *)RspBuf);

            }

            break;
        }

        case AT_SET_REQ:              /* AT+ECPCFG= */
        {

            /*
             * AT+ECPCFG="faultAction",[0-4],"dumpToATPort",0/1,"startWDT",0/1,"logCtrl",0/1/2,"logLevel",[0-5],"logBaudrate",3M/6M,slpWaitTime",[0-65535]
             */
            if((paramNumb > 0) && (paramNumb & 0x01) == 0)
            {
                for(paraIdx = 0; paraIdx < paramNumb; paraIdx++)
                {
                    memset(paraStr, 0x00, ATC_ECPCFG_MAX_PARM_STR_LEN);
                    if(atGetStrValue(pParamList, paraIdx, paraStr,
                                     ATC_ECPCFG_MAX_PARM_STR_LEN, &paraLen, ATC_ECPCFG_MAX_PARM_STR_DEFAULT) == AT_PARA_ERR)
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECPCFG_1, P_WARNING, 0, "AT+ECPCFG, invalid input string parameter");
                        break;
                    }

                    // next parameters
                    paraIdx++;
                    if(strncasecmp((const CHAR*)paraStr, "faultAction", strlen("faultAction")) == 0)
                    {
                        configValidFlag |= 0x4;
                        if(atGetNumValue(pParamList, paraIdx, &paraValue,
                                         ATC_ECPCFG_FAULT_VAL_MIN, ATC_ECPCFG_FAULT_VAL_MAX, ATC_ECPCFG_FAULT_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            configValidFlag |= 0x8;
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_FAULT_ACTION, (UINT8)paraValue);
                        }
                        else
                        {
                            configValidFlag &= ~0x8;
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECPCFG_2, P_WARNING, 0, "AT+ECPCFG, invalid input 'faultAction' value");
                            break;
                        }
                    }

                    if(strncasecmp((const CHAR*)paraStr, "dumpToATPort", strlen("dumpToATPort")) == 0)
                    {
                        configValidFlag |= 0x4;
                        if(atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECPCFG_AT_PORT_DUMP_VAL_MIN, ATC_ECPCFG_AT_PORT_DUMP_VAL_MAX, ATC_ECPCFG_AT_PORT_DUMP_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            configValidFlag |= 0x8;
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_DUMP, (bool)paraValue);
                        }
                        else
                        {
                            configValidFlag &= ~0x8;
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECPCFG_8, P_WARNING, 0, "AT+ECPCFG, invalid input 'dumpToATPort' value");
                            break;
                        }
                    }

                    if(strncasecmp((const CHAR*)paraStr, "startWDT", strlen("startWDT")) == 0)
                    {
                        configValidFlag |= 0x4;
                        if(atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECPCFG_WDT_VAL_MIN, ATC_ECPCFG_WDT_VAL_MAX, ATC_ECPCFG_WDT_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            configValidFlag |= 0x8;
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_START_WDT, (bool)paraValue);
                        }
                        else
                        {
                            configValidFlag &= ~0x8;
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECPCFG_3, P_WARNING, 0, "AT+ECPCFG, invalid input 'startWDT' value");
                            break;
                        }
                    }

                    if(strncasecmp((const CHAR*)paraStr, "logCtrl", strlen("logCtrl")) == 0)
                    {
                        configValidFlag |= 0x4;
                        if(atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECPCFG_UNI_CTRL_VAL_MIN, ATC_ECPCFG_UNI_CTRL_VAL_MAX, ATC_ECPCFG_UNI_CTRL_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            configValidFlag |= 0x8;
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_CONTROL, paraValue);
                        }
                        else
                        {
                            configValidFlag &= ~0x8;
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECPCFG_4, P_WARNING, 0, "AT+ECPCFG, invalid input 'logCtrl' value");
                            break;
                        }
                    }

                    if(strncasecmp((const CHAR*)paraStr, "logLevel", strlen("logLevel")) == 0)
                    {
                        configValidFlag |= 0x4;
                        if(atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECPCFG_UNI_LEVEL_VAL_MIN, ATC_ECPCFG_UNI_LEVEL_VAL_MAX, ATC_ECPCFG_UNI_LEVEL_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            configValidFlag |= 0x8;
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_LEVEL, paraValue);
                            uniLogDebugLevelSet((debugTraceLevelType)paraValue);
                        }
                        else
                        {
                            configValidFlag &= ~0x8;
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECPCFG_5, P_WARNING, 0, "AT+ECPCFG, invalid input 'logLevel' value");
                            break;
                        }
                    }

                    if(strncasecmp((const CHAR*)paraStr, "logBaudrate", strlen("logBaudrate")) == 0)
                    {
                        configValidFlag |= 0x4;
                        if(atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECPCFG_LOG_BAUDRATE_VAL_MIN, ATC_ECPCFG_LOG_BAUDRATE_VAL_MAX, ATC_ECPCFG_LOG_BAUDRATE_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            configValidFlag |= 0x8;
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_BAUDRATE, paraValue);
                        }
                        else
                        {
                            configValidFlag &= ~0x8;
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECPCFG_6, P_WARNING, 0, "AT+ECPCFG, invalid input 'logBaudrate' value");
                            break;
                        }
                    }

                    if(strncasecmp((const CHAR*)paraStr, "slpWaitTime", strlen("slpWaitTime")) == 0)
                    {
                        configValidFlag |= 0x1;
                        if(atGetNumValue(pParamList, paraIdx, &paraValue,
                                          ATC_ECPCFG_SLEEP_VAL_MIN, ATC_ECPCFG_SLEEP_VAL_MAX, ATC_ECPCFG_SLEEP_VAL_DEFAULT) == AT_PARA_OK)
                        {
                            configValidFlag |= 0x2;
                            waitBeforeSlp = pmuBuildWaitSlpCfg(paraValue);
                            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_WAIT_SLEEP, waitBeforeSlp);
                        }
                        else
                        {
                            configValidFlag &= ~0x2;
                            ECOMM_TRACE(UNILOG_ATCMD_EXEC, devECPCFG_7, P_WARNING, 0, "AT+ECPCFG, invalid input 'slpWaitTime' value");
                            break;
                        }
                    }
                }
            }

            if((configValidFlag != 0x3) && (configValidFlag != 0xC) && (configValidFlag != 0xF))
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            else
            {
                if(configValidFlag & 0x3)
                {
                    BSP_SavePlatConfigToFs();
                }

                if(configValidFlag & 0xC)
                {
                    BSP_SavePlatConfigToRawFlash();
                }

                ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply( atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }


    return ret;
}


/**
  \fn          CmsRetId pdevECSLPFAIL(const AtCmdInputContext *AtCmdReqParaPtr)
  \brief       at cmd req function to show low power mode vote result
  \param[in]   pAtCmdReq
  \returns     CmsRetId
 */
CmsRetId pdevECVOTECHK(const AtCmdInputContext *pAtCmdReq)
{
    extern void slpManSlpFailedReasonCheck(slpManSlpState_t *DeepestSlpMode, slpManSlpState_t *psphyVoteState, slpManSlpState_t *appVoteState,  slpManSlpState_t *usrdefSlpMode,  uint32_t *drvVoteMap, uint32_t *drvVoteMask);
    extern void slpManGetSDKVoteDetail(uint32_t *sleepVoteFlag, uint32_t *sleep2VoteFlag, uint32_t *hibVoteFlag);
    CHAR printf_buf[EC_PRINT_BUF_LEN] = {0};

    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);

    slpManSlpState_t DeepestSlpMode;
    slpManSlpState_t psphyVoteState;
    slpManSlpState_t appVoteState;
    slpManSlpState_t usrdefSlpMode;
    uint32_t drvVoteMap;
    uint32_t drvVoteMask;
    slpManSlpState_t appVoteDetail;
    uint8_t vote_counter;
    uint32_t sdkSleep1Vote;
    uint32_t sdkSleep2Vote;
    uint32_t sdkHibVote;

    char slpStateText[5][5]={{"Actv"},{"Idle"},{"Slp1"},{"Slp2"},{"Hibn"}};

    slpManSlpFailedReasonCheck(&DeepestSlpMode, &psphyVoteState, &appVoteState, &usrdefSlpMode, &drvVoteMap, &drvVoteMask);

    memset(printf_buf, 0, EC_PRINT_BUF_LEN);
    snprintf(printf_buf, EC_PRINT_BUF_LEN, "\nSleep Vote Info:\n");


    ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)printf_buf);

    memset(printf_buf, 0, EC_PRINT_BUF_LEN);
    snprintf(printf_buf, EC_PRINT_BUF_LEN, "Deepest Sleep Mode: \t\t\t\t%s\n",slpStateText[DeepestSlpMode]);

    ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)printf_buf);


    memset(printf_buf, 0, EC_PRINT_BUF_LEN);
    snprintf(printf_buf, EC_PRINT_BUF_LEN, "EC SDK Vote for: \t\t\t\t%s\n",slpStateText[psphyVoteState]);

    ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)printf_buf);


    ///// detail for sdk vote ////////////
    slpManGetSDKVoteDetail(&sdkSleep1Vote, &sdkSleep2Vote, &sdkHibVote);
    memset(printf_buf, 0, EC_PRINT_BUF_LEN);
    snprintf(printf_buf, EC_PRINT_BUF_LEN, "\tDetail: 0x%x,0x%x,0x%x\n",sdkSleep1Vote,sdkSleep2Vote,sdkHibVote);

    ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)printf_buf);

    memset(printf_buf, 0, EC_PRINT_BUF_LEN);
    snprintf(printf_buf, EC_PRINT_BUF_LEN, "Application Vote for: \t\t\t\t%s\n",slpStateText[appVoteState]);

    ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)printf_buf);


    ///// detail for application vote ////////////
    for(uint8_t i=0;i<32;i++)
    {
        if(slpManCheckVoteState(i, &appVoteDetail, &vote_counter) == RET_TRUE)
        {

            memset(printf_buf, 0, EC_PRINT_BUF_LEN);
            if(vote_counter != 0)
                snprintf(printf_buf, EC_PRINT_BUF_LEN, "\tHandle: %d\tName: %s\tProhibit State: %s\tVote count: %d\n",i,slpManGetVoteInfo(i),slpStateText[appVoteDetail],vote_counter);
            else
                snprintf(printf_buf, EC_PRINT_BUF_LEN, "\tHandle: %d\tName: %s\tProhibit State: NULL\tVote count: %d\n",i,slpManGetVoteInfo(i),vote_counter);

              ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)printf_buf);
        }
    }

    memset(printf_buf, 0, EC_PRINT_BUF_LEN);
    snprintf(printf_buf, EC_PRINT_BUF_LEN, "User defined Sleep Callback Vote for: \t\t%s\n",slpStateText[usrdefSlpMode]);

    ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)printf_buf);

    memset(printf_buf, 0, EC_PRINT_BUF_LEN);
    snprintf(printf_buf, EC_PRINT_BUF_LEN, "Driver Vote bitMap: 0x%x, with vote mask: 0x%x\n",drvVoteMap,drvVoteMask);

    ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)printf_buf);

    ret = atcReply(atHandle, AT_RC_OK, 0, NULL);


    return ret;

}


/**
  \fn          CmsRetId pdevIPR(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd IPR
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevIPR(const AtCmdInputContext *pAtCmdReq)
{
    extern uint8_t* atCmdGetIPRString(void);

    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CHAR rspBuf[32] = {0};
    UINT8 paraStr[ATC_IPR_MAX_PARM_STR_LEN] = {0};

    UINT16  paraLen = 0;

    UINT32 baudRateToSet = 0;

    plat_config_fs_t* fsPlatConfigPtr = BSP_GetFsPlatConfig();

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+IPR=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)atCmdGetIPRString());
            break;
        }

        case AT_READ_REQ:            /* AT+IPR?  */
        {
            if(fsPlatConfigPtr)
            {
                snprintf((char*)rspBuf, 32, "+IPR: %d", (fsPlatConfigPtr->atPortBaudRate & 0x80000000UL) ? 0 : (fsPlatConfigPtr->atPortBaudRate & 0x7FFFFFFFUL));

                ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)rspBuf);

            }

            break;
        }

        case AT_SET_REQ:              /* AT+IPR= */
        {
            if(atGetStrValue(pParamList, 0, paraStr, ATC_IPR_MAX_PARM_STR_LEN, &paraLen, ATC_IPR_MAX_PARM_STR_DEFAULT) != AT_PARA_ERR)
            {
                char* end;
                baudRateToSet = strtol((char *)paraStr, &end, 10);

                if(((char *)paraStr == end) || atCmdIsBaudRateValid(baudRateToSet) == false)
                {
                    ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                    break;
                }

                ret = atcReply(atHandle, AT_RC_OK, 0, NULL);

                if(baudRateToSet == 0)
                {
                    atCmdUartChangeConfig(pAtCmdReq->chanId, 0, 0, 1);
                }
                else
                {
                    atCmdUartChangeConfig(pAtCmdReq->chanId, baudRateToSet, fsPlatConfigPtr->atPortFrameFormat.wholeValue, 1);
                }

            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }

            break;
        }

        case AT_EXEC_REQ:           /* AT+IPR */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId pdevICF(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd ICF
  \param[in]   pAtCmdReq
  \returns     CmsRetId
  \details
  V.250 ICF defined values
  <format>                          Valid numeric values
     0                                  auto detect(not support)
     1                                  8 data; 2 stop
     2                                  8 data; 1 parity; 1 stop
     3                                  8 data; 1 stop
     4                                  7 data; 2 stop
     5                                  7 data; 1 parity; 1 stop
     6                                  7 data; 1 stop
  <parity>                           Defined numeric values
     0                                     odd
     1                                     even
     3                                     mark(not support)
     4                                     space(not support)
*/
CmsRetId pdevICF(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CHAR rspBuf[32] = {0};

    INT32 formatValue = 0, parityValue = 0;
    UINT8 dataBits, stopBits, parity;
    atPortFrameFormat_t newFrameFormat = {.wholeValue = 0};

    plat_config_fs_t* fsPlatConfigPtr = BSP_GetFsPlatConfig();

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ICF=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, "+ICF: (1-6), (0-1)\r\n");
            break;
        }

        case AT_READ_REQ:            /* AT+ICF?  */
        {

            dataBits = fsPlatConfigPtr->atPortFrameFormat.config.dataBits;
            stopBits = fsPlatConfigPtr->atPortFrameFormat.config.stopBits;
            parity = fsPlatConfigPtr->atPortFrameFormat.config.parity;

            // 7 data
            if(dataBits == 1)
            {
                // 2 stop
                if(stopBits == 2)
                {
                    formatValue = 4;
                    parityValue = 0;
                }
                // 1 stop
                else
                {   // no parity
                    if(parity == 0)
                    {
                        formatValue = 6;
                        parityValue = 0;
                    }
                    // parity
                    else
                    {
                        formatValue = 5;
                        parityValue = parity - 1;
                    }
                }

            }
            // 8 data
            else
            {
                // 2 stop
                if(stopBits == 2)
                {
                    formatValue = 1;
                    parityValue = 0;
                }
                // 1 stop
                else
                {   // no parity
                    if(parity == 0)
                    {
                        formatValue = 3;
                        parityValue = 0;
                    }
                    // parity
                    else
                    {
                        formatValue = 2;
                        parityValue = parity - 1;
                    }
                }
            }
            snprintf((char*)rspBuf, 32, "+ICF: %d, %d\n", formatValue, parityValue);

            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)rspBuf);

            break;
        }

        case AT_SET_REQ:              /* AT+ICF= */
        {
            if(atGetNumValue(pParamList, 0, &formatValue, ATC_ICF_FORMAT_VAL_MIN, ATC_ICF_FORMAT_VAL_MAX, ATC_ICF_FORMAT_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &parityValue, ATC_ICF_PARITY_VAL_MIN, ATC_ICF_PARITY_VAL_MAX, ATC_ICF_PARITY_VAL_DEFAULT)) != AT_PARA_ERR)
                {
                    switch(formatValue)
                    {
                        case 1:
                            newFrameFormat.config.dataBits = 0;
                            newFrameFormat.config.stopBits = 2;
                            newFrameFormat.config.parity = 0;
                            break;
                        case 2:
                            newFrameFormat.config.dataBits = 0;
                            newFrameFormat.config.stopBits = 0;
                            newFrameFormat.config.parity = parityValue + 1;
                            break;
                        case 3:
                            newFrameFormat.config.dataBits = 0;
                            newFrameFormat.config.stopBits = 0;
                            newFrameFormat.config.parity = 0;
                            break;
                        case 4:
                            newFrameFormat.config.dataBits = 1;
                            newFrameFormat.config.stopBits = 2;
                            newFrameFormat.config.parity = 0;
                            break;
                        case 5:
                            newFrameFormat.config.dataBits = 1;
                            newFrameFormat.config.stopBits = 0;
                            newFrameFormat.config.parity = parityValue + 1;
                            break;
                        case 6:
                            newFrameFormat.config.dataBits = 1;
                            newFrameFormat.config.stopBits = 0;
                            newFrameFormat.config.parity = 0;
                            break;
                        default:
                            break;
                    }

                    ret = atcReply(atHandle, AT_RC_OK, 0, NULL);

                    atCmdUartChangeConfig(pAtCmdReq->chanId, fsPlatConfigPtr->atPortBaudRate, newFrameFormat.wholeValue, 1);

                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                }

            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }

            break;
        }

        case AT_EXEC_REQ:           /* AT+ICF */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


/**
  \fn          CmsRetId pdevNetLight(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd ECLEDMODE
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevNetLight(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CHAR RspBuf[16] = {0};

    INT32 ledModeEn = ATC_ECLED_MODE_VAL_DEFAULT;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECLEDMODE=? */
        {
            // Add supported baudrate
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECLEDMODE: (0,1)");
            break;
        }

        case AT_READ_REQ:            /* AT+ECLEDMODE?  */
        {
            // read from middleware nv

            ledModeEn = mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECLED_MODE_CFG);

            snprintf((char*)RspBuf, 16, "+ECLEDMODE: %d", ledModeEn);

            ret = atcReply( atHandle, AT_RC_OK, 0, (CHAR *)RspBuf);

            break;
        }

        case AT_SET_REQ:              /* AT+ECLEDMODE= */
        {
            if(atGetNumValue(pParamList, 0, &ledModeEn,
                             ATC_ECLED_MODE_VAL_MIN, ATC_ECLED_MODE_VAL_MAX, ATC_ECLED_MODE_VAL_DEFAULT) != AT_PARA_ERR)
            {
                extern void TIMER_Netlight_PWM(uint8_t mode);
                if(ledModeEn == ATC_ECLED_MODE_VAL_MIN)
                    TIMER_Netlight_PWM(0);

                // write to middleware nv
                mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECLED_MODE_CFG, ledModeEn);

                ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }

            break;
        }

        case AT_EXEC_REQ:           /* AT+ECLEDMODE */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

CmsRetId pdevPMUSTATUS(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    CHAR RspBuf[64] = {0};
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;

    slpManSlpState_t slpState = SLP_ACTIVE_STATE;
    uint32_t sleepTime = 0;
    uint32_t wakeupSlowCnt, currentSlowCnt;

    CHAR slpStateText[5][5]={{"Actv"},{"Idle"},{"Slp1"},{"Slp2"},{"Hibn"}};

    extern uint32_t PmuSlowCntRead(void);

    switch (operaType)
    {
        case AT_READ_REQ:            /* AT+ECPMUSTATUS?  */
        {
            slpState = slpManGetLastSlpState();

            switch(slpState)
            {
                case SLP_ACTIVE_STATE:
                case SLP_IDLE_STATE:
                    break;

                case SLP_SLP1_STATE:
                case SLP_SLP2_STATE:
                case SLP_HIB_STATE:

                    currentSlowCnt = PmuSlowCntRead();
                    wakeupSlowCnt = slpManGetWakeupSlowCnt();

                    if(((currentSlowCnt - wakeupSlowCnt) & 0x1FFFFFFFU) < 5*32768)
                    {
                        sleepTime = slpManGetSleepTime() * 125;
                    }
                    else
                    {
                        slpState = SLP_ACTIVE_STATE;
                    }

                    break;

                default:
                    break;
            }

            snprintf((char*)RspBuf, 64, "+ECPMUSTATUS: %s, %d", slpStateText[slpState], sleepTime);
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)RspBuf);
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;

}


#ifndef FEATURE_REF_AT_ENABLE
__WEAK void atCmdURCReportInPMUFlow(bool enable)
{


}

/**
  \fn          CmsRetId pdevECPURC(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId pdevECPURC(const AtCmdInputContext *pAtCmdReq)
{
    /*
     * internal using, to close/open PLAT URC (unsolicited result code) report
     * AT+ECPURC=<urcStr>,<value>
     * <urcStr> value:
     *  "hibnate" - configure +HIB URC
     * <value> value:
     *  0 - disable URC report
     *  1 - enable URC report
    */

    CmsRetId    ret = CMS_FAIL;
    UINT32      atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32      operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32       nValue = 0xff;
    UINT8       paraStr[ATC_ECPURC_0_MAX_PARM_STR_LEN] = {0};  //max STR parameter length is 16;
    UINT16      paraLen = 0;
    CHAR        *rspBuf = PNULL;
    UINT8       hibRptMode = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:           /* AT+ECPURC=? */
        {
            ret = atcReply(atHandle,
                           AT_RC_OK,
                           0,
                           (CHAR *)"\r\n+ECPURC: \"HIBNATE\":(0-1)\r\n");
            break;
        }

        case AT_SET_REQ:            /* AT+ECPURC= */
        {
            if (atGetStrValue(pParamList, 0, paraStr,
                              ATC_ECPURC_0_MAX_PARM_STR_LEN, &paraLen, ATC_ECPURC_0_MAX_PARM_STR_DEFAULT) == AT_PARA_OK)
            {
                if (atGetNumValue(pParamList, 1, &nValue,
                                  ATC_ECPURC_1_VAL_MIN, ATC_ECPURC_1_VAL_MAX, ATC_ECPURC_1_VAL_DEFAULT) == AT_PARA_OK)
                {
                    ret = CMS_RET_SUCC;

                    if (strcasecmp((const CHAR*)paraStr, "HIBNATE") == 0)
                    {
                        mwSetAndSaveAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECPURC_HIB_RPT_CFG, nValue);
                        if(nValue == 0)
                        {
                            atCmdURCReportInPMUFlow(false);
                        }
                        else
                        {
                            atCmdURCReportInPMUFlow(true);
                        }
                    }
                    else
                    {
                        ECOMM_STRING(UNILOG_ATCMD_EXEC, pdevECPURC_warning_1, P_WARNING,
                                     "AT CMD, ECPURC, invalid param URC: %s", paraStr);
                        ret = CMS_INVALID_PARAM;
                    }
                }
            }


            if (ret == CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_READ_REQ:    /* AT+ECPURC? */
        {

            rspBuf = (CHAR *)OsaAllocZeroMemory(ATEC_IND_RESP_64_STR_LEN);
            OsaDebugBegin(rspBuf != PNULL, ATEC_IND_RESP_64_STR_LEN, 0, 0);
            return ret;
            OsaDebugEnd();

            hibRptMode  = mwGetAtChanConfigItemValue(pAtCmdReq->chanId, MID_WARE_AT_CHAN_ECPURC_HIB_RPT_CFG);

            snprintf(rspBuf, ATEC_IND_RESP_64_STR_LEN,
                     "\r\n+ECPURC: \"HIBNATE\":%d\r\n", hibRptMode);

            ret = atcReply(atHandle, AT_RC_OK, 0, rspBuf);
            if (rspBuf != PNULL)
            {
                OsaFreeMemory(&rspBuf);
            }
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_UNKNOWN, NULL);
            break;
        }
    }
    return ret;
}
#endif

/**
  \fn          CmsRetId pdevECFSINFO(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function to show FS info
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECFSINFO(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    INT32   errCode;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);

    CHAR printf_buf[ECFSINFO_PRINT_BUF_LEN] = {0};
    CHAR *pcWriteBuffer = printf_buf;

    INT32 writtenSize = 0;
    INT32 spaceLeft = ECFSINFO_PRINT_BUF_LEN;

    UINT32 fsFileCount = 0;

    lfs_dir_t dir;

    errCode = LFS_DirOpen(&dir, "/");

    if(errCode)
    {
        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_SYS_FAIL, NULL);
    }

    struct lfs_info info;
    lfs_status_t status = {0};

    snprintf(printf_buf, ECFSINFO_PRINT_BUF_LEN,"size\t file name");
    ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)printf_buf);

    while(true)
    {
        errCode = LFS_DirRead(&dir, &info);

        if(errCode < 0)
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_SYS_FAIL, NULL);
        }

        if(errCode == 0)
        {
            break;
        }

        if((fsFileCount > 0) && ((fsFileCount % 2) == 0) )
        {
            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, (CHAR *)printf_buf);

            pcWriteBuffer = printf_buf;
            spaceLeft = ECFSINFO_PRINT_BUF_LEN;
        }

        switch(info.type)
        {
            case LFS_TYPE_REG:

                if((fsFileCount % 2) == 0)
                {
                    writtenSize = snprintf(pcWriteBuffer, ECFSINFO_PRINT_BUF_LEN, "%uB\t%s", info.size, info.name);
                }
                else
                {
                    writtenSize = snprintf(pcWriteBuffer, ECFSINFO_PRINT_BUF_LEN, "\r\n%uB\t%s", info.size, info.name);
                }

                if((writtenSize < 0) || (writtenSize >= spaceLeft))
                {
                    break;
                }

                pcWriteBuffer += writtenSize;
                spaceLeft -= writtenSize;
                fsFileCount++;

                break;
        }

    }

    if(pcWriteBuffer != printf_buf)
    {
        ret = atcReply(atHandle, AT_RC_CONTINUE, 0, printf_buf);
    }

    errCode = LFS_DirClose(&dir);

    if(errCode)
    {
        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_SYS_FAIL, NULL);
    }

    errCode = LFS_Statfs(&status);

    if(errCode)
    {
        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_SYS_FAIL, NULL);
    }

    snprintf(printf_buf, ECFSINFO_PRINT_BUF_LEN, "FS block: Totoal : %d  used: %d", status.total_block, status.block_used);

    ret = atcReply(atHandle, AT_RC_OK, 0, printf_buf);

    return ret;
}

/**
  \fn          CmsRetId pdevECFSFORMAT(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function to format FS region and PLAT config raw flash region
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECFSFORMAT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);

    if(BSP_QSPI_Erase_Safe(FLASH_FS_REGION_OFFSET, FLASH_FS_REGION_SIZE) != 0)
    {
        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_SYS_FAIL, NULL);
        return ret;
    }

    if(BSP_QSPI_Erase_Safe(FLASH_MEM_PLAT_INFO_NONXIP_ADDR, FLASH_MEM_PLAT_INFO_SIZE) != 0)
    {
        ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_SYS_FAIL, NULL);
        return ret;
    }

    ret = atcReply(atHandle, AT_RC_OK, 0, NULL);

    return ret;
}


/**
  \fn          CmsRetId pdevECFLASHMONITORINFO(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function to show FS monitor info
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECFLASHMONITORINFO(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;

    INT32 paraValue = 0, errCode = 0;

    CHAR * printfBuf = NULL, *pcWriteBuffer = NULL;
    INT32 writtenSize = 0;
    INT32 spaceLeft = PRINTF_BUF_LEN;

    uint32_t blockEraseCount = 0, blockIndex = 0;
    uint32_t PMUPlatPSRG0,PMUPlatPSRG1,PMUPlatPSRG2,PMUPlatPSRG3;
    uint32_t PMUPhyRG0,PMUPhyRG1,PMUPhyRG2,PMUPhyRG3;
    uint8_t curPlatPsRegion = 0, curPhyRegion = 0;

    file_operation_statistic_result_t result = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECFLASHMONITORINFO=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECFLASHMONITORINFO: (0,1,2,3)");
            break;
        }

        case AT_SET_REQ:              /* AT+ECFLASHMONITORINFO= */
        {
            if(atGetNumValue(pParamList, 0, &paraValue,
                             ATC_ECFLASHMONITORINFO_VAL_MIN, ATC_ECFLASHMONITORINFO_VAL_MAX, ATC_ECFLASHMONITORINFO_VAL_DEFAULT) != AT_PARA_ERR)
            {
                switch(paraValue)
                {
                    case 0:

                        LFS_ResetMonitorResult();
                        ret = atcReply(atHandle, AT_RC_OK, 0, NULL);;

                        break;

                    case 1:

                        printfBuf = malloc(PRINTF_BUF_LEN);

                        OsaDebugBegin(printfBuf, printfBuf, 0, 0);
                        return ret;
                        OsaDebugEnd();

                        memset(printfBuf, 0, PRINTF_BUF_LEN);

                        pcWriteBuffer = printfBuf;

                        writtenSize = snprintf(pcWriteBuffer, spaceLeft, "writeCount,writeBytesCount,file name\r\n");
                        pcWriteBuffer += writtenSize;
                        spaceLeft -= writtenSize;

                        while(true)
                        {
                            errCode = LFS_GetFileWriteMonitorResult(&result);

                            if(errCode == 0)
                            {
                                break;
                            }
                            else if(errCode == 1)
                            {
                                writtenSize = snprintf(pcWriteBuffer, spaceLeft, "%u,%uB,%s\r\n", result.writeCount, result.writeBytesCount , result.name);
                                if((writtenSize < 0) || (writtenSize >= spaceLeft))
                                {
                                    break;
                                }
                                pcWriteBuffer += writtenSize;
                                spaceLeft -= writtenSize;
                            }
                        }

                        if(pcWriteBuffer != printfBuf)
                        {
                            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, printfBuf);
                        }

                        free(printfBuf);
                        ret = atcReply(atHandle, AT_RC_OK, 0, NULL);

                        break;

                    case 2:

                        printfBuf = malloc(PRINTF_BUF_LEN);

                        OsaDebugBegin(printfBuf, printfBuf, 0, 0);
                        return ret;
                        OsaDebugEnd();

                        memset(printfBuf, 0, PRINTF_BUF_LEN);

                        pcWriteBuffer = printfBuf;

                        while(true)
                        {
                            errCode = LFS_GetBlockEraseCountResult(&blockEraseCount);

                            if(errCode == 0)
                            {
                                break;
                            }
                            else if(errCode == 1)
                            {
                                if((blockIndex > 0) && ((blockIndex % 10) == 0) )
                                {
                                    writtenSize = snprintf(pcWriteBuffer, spaceLeft, "\r\n");

                                    if((writtenSize < 0) || (writtenSize >= spaceLeft))
                                    {
                                        break;
                                    }

                                    pcWriteBuffer += writtenSize;
                                    spaceLeft -= writtenSize;
                                }

                                if((blockIndex % 10) == 0)
                                {
                                    writtenSize = snprintf(pcWriteBuffer, spaceLeft, "%u", blockEraseCount);
                                }
                                else
                                {
                                    writtenSize = snprintf(pcWriteBuffer, spaceLeft, ",%u", blockEraseCount);
                                }

                                if((writtenSize < 0) || (writtenSize >= spaceLeft))
                                {
                                    break;
                                }

                                pcWriteBuffer += writtenSize;
                                spaceLeft -= writtenSize;
                                blockIndex++;

                            }
                        }

                        if(pcWriteBuffer != printfBuf)
                        {
                            ret = atcReply(atHandle, AT_RC_CONTINUE, 0, printfBuf);
                        }

                        free(printfBuf);
                        ret = atcReply(atHandle, AT_RC_OK, 0, NULL);

                        break;

                   case 3:

                        printfBuf = malloc(PRINTF_BUF_LEN);

                        OsaDebugBegin(printfBuf, printfBuf, 0, 0);
                        return ret;
                        OsaDebugEnd();

                        memset(printfBuf, 0, PRINTF_BUF_LEN);

                        pcWriteBuffer = printfBuf;

                        PmuRawFlashEraseCntCheck(&PMUPlatPSRG0, &PMUPlatPSRG1, &PMUPlatPSRG2, &PMUPlatPSRG3,
                            &PMUPhyRG0, &PMUPhyRG1, &PMUPhyRG2, &PMUPhyRG3);

                        slpManGetRawFlashRegionFlag(&curPlatPsRegion, &curPhyRegion);

                        snprintf(printfBuf, PRINTF_BUF_LEN, "PMU Flash Erase Count: \r\nPlatPS Region0: %d Phy Region0: %d\r\nPlatPS Region1: %d Phy Region1: %d\r\nPlatPS Region2: %d Phy Region2: %d\r\nPlatPS Region3: %d Phy Region3: %d\r\nCurPlatPs Region=%d, CurPhy Region=%d",
                            PMUPlatPSRG0, PMUPhyRG0, PMUPlatPSRG1, PMUPhyRG1, PMUPlatPSRG2, PMUPhyRG2, PMUPlatPSRG3, PMUPhyRG3, curPlatPsRegion, curPhyRegion);

                        ret = atcReply(atHandle, AT_RC_CONTINUE, 0, printfBuf);

                        free(printfBuf);

                        ret = atcReply(atHandle, AT_RC_OK, 0, NULL);

                        break;
                }

            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }

            break;
        }

        case AT_EXEC_REQ:           /* AT+ECFLASHMONITORINFO */
        case AT_READ_REQ:           /* AT+ECFLASHMONITORINFO?  */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId pdevECABFOTACTL(const pdevABUPFOTAFLAG *pAtCmdReq)
  \brief       at cmd IPR
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId pdevECABFOTACTL(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    CHAR rspBuf[32] = {0};
    UINT8 paraStr[ATC_ABUPFOTAFLAG_MAX_PARM_STR_LEN] = {0};
    UINT16  paraLen = 0;
    UINT32  flag = 0xff;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECABFOTACTL=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECABFOTACTL: (enable,disable)");
            break;
        }

        case AT_READ_REQ:            /* AT+ECABFOTACTL?  */
        {
            flag = mwGetAbupFotaEnableFlag();
            if(flag == 1)
            {
                snprintf((char*)rspBuf, 32, "+ECABFOTACTL: enable");
                ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)rspBuf);
            }
            else
            {
                snprintf((char*)rspBuf, 32, "+ECABFOTACTL: disable");
                ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)rspBuf);
            }

            break;
        }

        case AT_SET_REQ:              /* AT+ECABFOTACTL= */
        {
            if(atGetStrValue(pParamList, 0, paraStr, ATC_ABUPFOTAFLAG_MAX_PARM_STR_LEN, &paraLen, ATC_ABUPFOTAFLAG_MAX_PARM_STR_DEFAULT) == AT_PARA_OK)
            {
                if(strncasecmp((const CHAR*)paraStr, "enable", strlen("enable")) == 0)
                {
                    mwSetAbupFotaEnableFlag(1);
                    ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
                }
                else if(strncasecmp((const CHAR*)paraStr, "disable", strlen("disable")) == 0)
                {
                    mwSetAbupFotaEnableFlag(0);
                    ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
                }
                else
                {
                    ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
                }
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }

            break;
        }

        case AT_EXEC_REQ:           /* AT+ECABFOTACTL */
        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

#ifdef CUSTOMER_DAHUA
CmsRetId pdevNYSM(const AtCmdInputContext *pAtCmdReq)
{
	CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
	int pmuMode = LP_STATE_IDLE;
	UINT32   operaType = (UINT32)pAtCmdReq->operaType;
	switch(operaType)
	{
		case AT_EXEC_REQ:		/*AT+NYSM*/
		{
			BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_SLEEP_MODE, (UINT8)pmuMode);
			BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_ENABLE_PM, PMU_ENABLE_MAGIC);
			pmuSetDeepestSleepMode(LP_STATE_IDLE);			
			//BSP_SavePlatConfigToFs();
			ret = atcReply(atHandle, AT_RC_OK, 0, NULL);

		}
		default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
	}
	return ret;
}

CmsRetId pdevNYGOSLEEP(const AtCmdInputContext *pAtCmdReq)
{
	CmsRetId ret = CMS_FAIL;
	int pmuMode = LP_STATE_HIBERNATE;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
	UINT32   operaType = (UINT32)pAtCmdReq->operaType;

   extern void atCmdUartSwitchSleepMode(UINT8 mode);
    
	switch(operaType)
	{
		case AT_EXEC_REQ:    /* AT+NYGOSLEEP*/
		{
			set_atcmd_name(1);
			
			BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_SLEEP_MODE, (UINT8)pmuMode);
    		BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_ENABLE_PM, PMU_ENABLE_MAGIC);
    		pmuSetDeepestSleepMode((LpState)pmuMode);			
			//BSP_SavePlatConfigToFs();
			
		//	atCmdUartSwitchSleepMode(3);
			ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
		}
		default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
	}
	return ret;
}

CmsRetId pdevNYVT(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    UINT32  reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    CHAR    rspBuf[ATEC_IND_RESP_256_STR_LEN] = {0};  
    INT32   snt;
    unsigned char wb[1]={0};
    INT32 len = 1;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+NYVT=? */
        {
            sprintf(rspBuf, "+NYVT: (0,1)");
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
            break;
        }

        case AT_SET_REQ:            /* AT+NYVT=<snt> */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &snt, ATC_NYVT_VAL_MIN, ATC_NYVT_VAL_MAX, ATC_NYVT_VAL_DEFAULT)) != AT_PARA_ERR)
            {      
//                if (snt == 1)
//                {                    
//                    wb[0]=0x55;                   
//                }
//                else if(snt == 0)
//               {
//                    wb[0]=0xaa;
//                }
//                else
//                {
//                    //should not arrive here
//                    OsaDebugBegin(FALSE, snt, 0, 0);
//                    OsaDebugEnd();                 
//                }

//                 if(!file_write(wb, len,1))
//                   {
//                         ret = CMS_RET_SUCC;
//                    }
				if(snt == 1)
				{
					wb[0] = 0x02;
					set_flash_data(wb,1,3);
					user_file_write();
				}
				else if(snt == 0)
				{
					wb[0] = 0x01;
					set_flash_data(wb,1,3);
					user_file_write();
				}
				else
				{
					ret = CMS_FAIL;
				}
            }

            if (ret == CMS_RET_SUCC)
            {
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_INCORRECT_PARAM), NULL);
            }
            break;
        }        

        case AT_READ_REQ:
		{
			unsigned char rd[12];
			int len = 12;
			if(0==user_file_read())
			{
				get_flash_data(rd,12);
				if(rd[3] == 0x02)  
				{
					sprintf(rspBuf, "+NYVT: 1");
					ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
				}
				else  if(rd[3] == 0x01)  
				{
					sprintf(rspBuf, "+NYVT: 0");
					ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
				}
				else
				{
					sprintf(rspBuf, "+NYVT: 1");
					ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
				}
			}
			else
			{     
				ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_INCORRECT_PARAM), NULL);
			}
		}
        break;
        

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}
#endif

#if 0//only used for internal test
void hexdump(unsigned char *buf, const int num)
{
    int i;
    for(i = 0; i < num; i++)
    {
        printf("%02X ", buf[i]);
        if ((i+1)%8 == 0)
            printf("\n");
    }
    printf("\n");
    return;
}


/****************************************************************************************
*  FUNCTION:
*
*  PARAMETERS:
*
*  DESCRIPTION:
*
*  RETURNS:
*
****************************************************************************************/
void test_nvram()
{
    int ret = 0xff;
    UINT8 *pdata=NULL;
    UINT8 *p_org=NULL;
    int i=0;

    p_org = (UINT8 *)calloc(1,4000);
    for(i=0;i<4000;i++)
        p_org[i] =0x5A;

    nvram_init();
    printf("nvram_write: !\n");
    ret = nvram_write(NV10,p_org,4000);

    if(ret== 0)
        printf("nvram_write data OK !\n");
    else
        printf("nvram_write data ERR !\n");

    pdata = (UINT8 *)calloc(1,4000);
    ret = nvram_read(NV10,pdata,4000,0);
    if(ret== 0)
        printf("nvram_read data ERR !\n");

    printf("nvread: !\n");

    ret = memcmp(pdata,p_org,4000);

    if(ret == 0)
        printf("nv read is OK !\n");
    else
        printf("nv write/read is ERR !\n");
    free(pdata);
    free(p_org);
}

void test_nvram_crack()
{
   UINT8 *p_org=NULL;
   char response_data[128] = {0};
   int i=0;

    p_org = (UINT8 *)calloc(1,4000);
    for(i=0;i<4000;i++)
        p_org[i] =0x00;

    sprintf(response_data, "test_nvram_crack:!\n");
    //atcReply( IND_REQ_HANDLER, AT_RC_OK, 0, (char *)response_data );
    atcURC(AT_CHAN_DEFAULT, response_data);

    BSP_QSPI_Write_Safe(p_org,9*NVRAM_SECTOR_SIZE+NVRAM_PHYSICAL_BASE,0x100);

}

void test_nvram_run()
{
    int ret = 0xff;
    UINT8 *p_org=NULL;
    char response_data[128] = {0};
    int i=0;

    p_org = (UINT8 *)calloc(1,4000);
    for(i=0;i<4000;i++)
        p_org[i] =0x5A;

    nvram_init();
    while(1)
    {
        sprintf(response_data, "nvram_write: !\n");
        //atcReply( IND_REQ_HANDLER, AT_RC_OK, 0, (char *)response_data );
        atcURC(AT_CHAN_DEFAULT, response_data);
        ret = nvram_write(NV10,p_org,4000);

        if(ret== 0)
        {
            sprintf(response_data, "nvram_write data OK !\n");
            //atcReply( IND_REQ_HANDLER, AT_RC_OK, 0, (char *)response_data );
            atcURC(AT_CHAN_DEFAULT, response_data);
        }
        else
        {
            sprintf(response_data, "nvram_write data ERR !\n");
            //atcReply( IND_REQ_HANDLER, AT_RC_OK, 0, (char *)response_data );
            atcURC(AT_CHAN_DEFAULT, response_data);
        }
    }

}

void test_nvram_chk()
{
    int ret = 0xff;
    UINT8 *pdata=NULL;
    UINT8 *p_org=NULL;
    char response_data[128] = {0};
    int i=0;

    p_org = (UINT8 *)calloc(1,4000);
    for(i=0;i<4000;i++)
        p_org[i] =0x5A;

    nvram_init();

    pdata = (UINT8 *)calloc(1,4000);
    for(i=0;i<4000;i++)
        pdata[i] =0x0;

    ret = nvram_read(NV10,pdata,4000,0);
    if(ret== 0)
    {
        sprintf(response_data, "nvram_read data WRG !\n");
        //atcReply( IND_REQ_HANDLER, AT_RC_OK, 0, (char *)response_data );
        atcURC(AT_CHAN_DEFAULT, response_data);
    }
    ret = memcmp(pdata,p_org,4000);

    if(ret == 0)
    {
        sprintf(response_data, "nvram_read data OK!\n");
        //atcReply( IND_REQ_HANDLER, AT_RC_OK, 0, (char *)response_data );
        atcURC(AT_CHAN_DEFAULT, response_data);
    }
    else
    {
        for(i=0;i<4000;i++)
            p_org[i] =0xFF;

        ret = memcmp(pdata,p_org,4000);
        if(ret == 0)
        {
            sprintf(response_data, "nvram_read data form FAC!\n");
            //atcReply( IND_REQ_HANDLER, AT_RC_OK, 0, (char *)response_data );
            atcURC(AT_CHAN_DEFAULT, response_data);
        }
        else
        {
            sprintf(response_data, "nvram_read data ERR!\n");
            //atcReply( IND_REQ_HANDLER, AT_RC_OK, 0, (char *)response_data );
            atcURC(AT_CHAN_DEFAULT, response_data);
        }

    }
    free(pdata);
    free(p_org);

}


/****************************************************************************************
*  FUNCTION:
*
*  PARAMETERS:
*
*  DESCRIPTION:
*
*  RETURNS:
*
****************************************************************************************/
void test_osa_file()
{
    int ret = 0xff;
    UINT8 *pdata=NULL;
    UINT8 *p_org=NULL;
    NVFILE_HANDLER nv_h=0;
    int i=0;

    p_org = (UINT8 *)calloc(1,4000);
    for(i=0;i<4000;i++)
        p_org[i] =0x5A;

    nvram_init();
    nv_h = NVFopen(NV10,0);

    printf("NVFwrite: !\n");
    NVFwrite(p_org,4000,1,nv_h);

    pdata = (UINT8 *)calloc(1,4000);
    NVFread(pdata,40,1,nv_h);
    printf("NVFread 0: !\n");

    memset(pdata,0,4000);
    NVFread(pdata,3000,1,nv_h);
    printf("NVFread 40: !\n");
    i=NVFtell(nv_h);
    printf("NVFtell : %i\n",i);
    ret = memcmp(pdata,&p_org[i],3000);

    if(ret == 0)
        printf("nv read is OK !\n");
    else
        printf("nv write/read is ERR !\n");

    NVFclose(nv_h);
    free(pdata);
    free(p_org);
}

/**
  \fn          CmsRetId pdevECUNITTEST(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId pdevECUNITTEST(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 flag=0;

    switch (operaType)
    {
        case AT_TEST_REQ:               /* AT+ECUNITTEST=? */
        case AT_READ_REQ:                /* AT+ECUNITTEST?  */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, (CHAR *)"+ECUNITTEST: (0-4)\r\n");
            break;
        }

        case AT_SET_REQ:                /* AT+ECUNITTEST= */
        {
            /* Extract the arguments starting with the functionality. */
            if(atGetNumValue(pParamList, 0, &flag, ATC_ECUNITTEST_0_VAL_MIN, ATC_ECUNITTEST_0_VAL_MAX, ATC_ECUNITTEST_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                                //flag = 0;
                switch(flag)
                {
                    case 1:
                        test_nvram();
                        break;
                    case 2:
                        test_osa_file();
                        break;
                    case 3:
                        //test_runnv();
                        test_nvram_run();
                        break;
                    case 4:
                        test_nvram_chk();
                        //test_runnv_gc();
                        break;
                    case 5:
                        test_nvram_crack();
                        break;

                    default:
                        break;
                }
                ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            }
            else
            {
                ret = atcReply(atHandle, AT_RC_ERROR, 0, NULL);
            }
            break;
        }

        case AT_EXEC_REQ:           /* AT+ECUNITTEST */
        default:
        {
            ret = atcReply(atHandle, AT_RC_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}
#endif
