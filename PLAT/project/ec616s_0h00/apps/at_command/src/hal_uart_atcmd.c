/****************************************************************************
 *
 * Copy right:   2020-, Copyrigths of EigenComm Ltd.
 * File name:    hal_uart_atcmd.c
 * Description:  EC616S uart at command source file
 * History:      Rev1.0   2020-02-24
 *
 ****************************************************************************/
#include "string.h"
#include "bsp.h"
#include "bsp_custom.h"
#include "ostask.h"
#include "debug_log.h"
#include "plat_config.h"
#include "FreeRTOS.h"
#include "at_def.h"
#include "at_api.h"
#include "slpman_ec616s.h"
#include "mw_config.h"
#include "hal_uart.h"
#include "os_exception.h"
#include "at_http_task.h"

#include "app.h"

typedef struct _uart_change_config_msg
{
    UINT32 baudRate;
    UINT32 frameFormat;
    CHAR  saveFlag;
}uart_change_config_msg_t;

#define AT_CMD_RECV_BUFFER_MARGIN               (64)
#define AT_CMD_RECV_BUFFER_SIZE                 (AT_CMD_BUF_MAX_LEN + AT_CMD_RECV_BUFFER_MARGIN)
static uint8_t at_uart_recv_buf[AT_CMD_RECV_BUFFER_SIZE];

#define AT_CMD_HAL_UART_IDX                     (1)

#ifndef FEATURE_REF_AT_ENABLE
#define HIB_ENTER_URC_STR                        "\r\n+HIB Enter\r\n"
#define HIB_EXIT_URC_STR                         "\r\n+HIB Exit\r\n"
#define HIB_FAIL_URC_STR                         "\r\n+HIB Fail\r\n"
#else
//#define HIB_ENTER_URC_STR                        "\r\n+NPSMR:1\r\n"
//#define HIB_EXIT_URC_STR                         "\r\n+NPSMR:0\r\n"
//#define HIB_REPORT_THD_MS                        (0)

#define HIB_ENTER_URC_STR                        "\r\nECSLEEP\r\n"
#define HIB_EXIT_URC_STR                         "\r\nECWAKEUP\r\n"
#define HIB_FAIL_URC_STR                         "\r\n+HIB Fail\r\n"
#define HIB_REPORT_THD_MS                        (0)
#endif

//const static uint32_t autoBaudRateList[] = {600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400};
const static uint32_t fixedBaudRateList[] = {0, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800};

static INT32 uart_at_chann = -1;

void atCmdRecvCallback(uint32_t event, void* dataPtr, uint32_t dataLen)
{
    uint32_t baudRate, baudRateDetected;
    slpManStartWaitATTimer();

    if((event == ARM_USART_EVENT_RX_TIMEOUT) || (event == ARM_USART_EVENT_RECEIVE_COMPLETE))
    {
        atSendAtcmdStrSig(uart_at_chann, dataPtr, dataLen);
    }
    else if(event == ARM_USART_EVENT_AUTO_BAUDRATE_DONE)
    {
        baudRate = BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_BAUDRATE);

        baudRateDetected = *(uint32_t*)dataPtr;

        ECOMM_TRACE(UNILOG_PLA_APP, atCmdRecvCallback_1, P_SIG, 2, "at port auto baud detected: %d, previous saved: %d", baudRateDetected, baudRate & 0x7FFFFFFFUL);

        if(baudRate != baudRateDetected)
        {
            BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_BAUDRATE, baudRateDetected | 0x80000000UL);
            BSP_SavePlatConfigToFs();
        }
    }
}

void atCmdPostSendCallback(hal_uart_send_msg_type_t msgType, void* dataPtr, uint32_t dataLen)
{
    if(msgType == HAL_UART_ACT_CHANGE_CONFIG && dataLen == sizeof(uart_change_config_msg_t))
    {
        UINT32 baudRate = 0, baudRateToSet;
        atPortFrameFormat_t frameFormatToSet, frameFormat;
        CHAR saveFlag;

        uart_change_config_msg_t *msgPtr = (uart_change_config_msg_t *)dataPtr;

        baudRateToSet = msgPtr->baudRate;
        frameFormatToSet.wholeValue = msgPtr->frameFormat;
        saveFlag = msgPtr->saveFlag;

        if(saveFlag)
        {
            baudRate = BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_BAUDRATE);
            frameFormat.wholeValue = BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_FRAME_FORMAT);

            if((baudRate != baudRateToSet) || (frameFormatToSet.wholeValue != frameFormat.wholeValue))
            {
                BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_BAUDRATE, ((baudRateToSet == 0) ? (baudRateToSet | 0x80000000UL) : baudRateToSet));
                BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_FRAME_FORMAT, frameFormatToSet.wholeValue);

                BSP_SavePlatConfigToFs();
            }
            else
            {
                return;
            }
        }

        hal_uart_hardware_config_t newHwConfig = {
                                                ARM_POWER_FULL,
                                                ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_FLOW_CONTROL_NONE,
                                                baudRateToSet
                                               };

        switch(frameFormatToSet.config.dataBits)
        {
            case 1:
                newHwConfig.controlSetting |= ARM_USART_DATA_BITS_7;
                break;
            default:
                newHwConfig.controlSetting |= ARM_USART_DATA_BITS_8;
                break;
        }

        switch(frameFormatToSet.config.parity)
        {
            case 1:
                newHwConfig.controlSetting |= ARM_USART_PARITY_ODD;
                break;
            case 2:
                newHwConfig.controlSetting |= ARM_USART_PARITY_EVEN;
                break;
            default:
                newHwConfig.controlSetting |= ARM_USART_PARITY_NONE;
                break;
        }

        switch(frameFormatToSet.config.stopBits)
        {
            case 2:
                newHwConfig.controlSetting |= ARM_USART_STOP_BITS_2;
                break;
            default:
                newHwConfig.controlSetting |= ARM_USART_STOP_BITS_1;
                break;
        }

        switch(baudRateToSet)
        {
            case 9600:
            case 4800:
            case 2400:
            case 1200:
            case 600:
                newHwConfig.powerMode = ARM_POWER_LOW;
                break;
            default:
                break;
        }

        HAL_UART_ResetUartSetting(AT_CMD_HAL_UART_IDX, &newHwConfig, true);

    }
    else if(msgType == HAL_UART_ACT_RESET_CHIP && dataLen == sizeof(UINT32))
    {
        UINT32 delay_ms = *(UINT32*)dataPtr;

        if(delay_ms)
        {
            delay_us(delay_ms*1000);
        }

#ifdef FS_FILE_OPERATION_STATISTIC
        LFS_SaveMonitorResult();
#endif
        EC_SystemReset();
    }
    else if(msgType == HAL_UART_ACT_SLEEP_MODE_SWITCH && dataLen == sizeof(UINT8))
    {
        UINT8 mode = *(UINT8*)dataPtr;
        slpManProductionTest(mode);
    }
    else if(msgType == HAL_UART_HTTP_RECVIND_FLOW_CONTROL)
    {
        httpOutflowContrl();
    }
}

void atCmdUartRespFunc(const CHAR* pStr, UINT32 strLen, void *pArg)
{
    HAL_UART_SendMsg(HAL_UART_SEND_MSG_TYPE_CNF, AT_CMD_HAL_UART_IDX, (uint8_t*)pStr, strLen);
}

void atCmdUartUrcFunc(const CHAR* pStr, UINT32 strLen)
{
    HAL_UART_SendMsg(HAL_UART_SEND_MSG_TYPE_URC, AT_CMD_HAL_UART_IDX, (uint8_t*)pStr, strLen);
}

void atCmdUartChangeConfig(UINT8 atChannel, UINT32 newBaudRate, UINT32 newFrameFormat, CHAR saveFlag)
{
    (void)atChannel;

    uart_change_config_msg_t msgBody =
    {
        .baudRate = newBaudRate,
        .frameFormat = newFrameFormat,
        .saveFlag = saveFlag
    };

    HAL_UART_SendMsg(HAL_UART_ACT_CHANGE_CONFIG, AT_CMD_HAL_UART_IDX, (uint8_t*)&msgBody, sizeof(msgBody));
}

void atCmdUartResetChip(UINT32 delay_ms)
{
    HAL_UART_SendMsg(HAL_UART_ACT_RESET_CHIP, AT_CMD_HAL_UART_IDX, (uint8_t*)&delay_ms, sizeof(UINT32));
}

void atCmdUartSwitchSleepMode(UINT8 mode)
{
    HAL_UART_SendMsg(HAL_UART_ACT_SLEEP_MODE_SWITCH, AT_CMD_HAL_UART_IDX, (uint8_t*)&mode, sizeof(UINT8));
}

void atCmdUartHttpFlowControl(UINT8 mode)
{
    HAL_UART_SendMsg(HAL_UART_HTTP_RECVIND_FLOW_CONTROL, AT_CMD_HAL_UART_IDX, (uint8_t*)&mode, sizeof(UINT8));
}

void atCmdDirectSendStr(uint8_t* strPtr, uint16_t strLen)
{
    HAL_UART_DirectSendStr(AT_CMD_HAL_UART_IDX, strPtr, strLen);
}

uint8_t* atCmdGetIPRString(void)
{
    return (uint8_t*)"+IPR:(600,1200,2400,4800,9600,19200,38400,57600,115200,230400)," \
        "(0,600,1200,2400,4800,9600,19200,38400,57600,115200,230400,460800)";
}

uint8_t* atCmdGetBaudRateString(void)
{
    return (uint8_t*)"600,1200,2400,4800,9600,19200,38400,57600,115200,230400,460800";
}

bool atCmdIsBaudRateValid(uint32_t baudRate)
{
    for(uint32_t i = 0; i < sizeof(fixedBaudRateList)/sizeof(fixedBaudRateList[0]); i++)
    {
        if(baudRate == fixedBaudRateList[i])
            return true;
    }

    return false;
}

void atCmdInitUartHandler(ARM_DRIVER_USART * uartDriverHandler, uint32_t baudRate, uint32_t frameFormat)
{
    hal_uart_config_t halUartConfig = {0};

    atPortFrameFormat_t uartFrameFormat;

    uartFrameFormat.wholeValue = frameFormat;

    hal_uart_hardware_config_t hwConfig = {
                                            ARM_POWER_FULL,
                                            ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_FLOW_CONTROL_NONE,
                                            baudRate
                                           };

    switch(uartFrameFormat.config.dataBits)
    {
        case 1:
            hwConfig.controlSetting |= ARM_USART_DATA_BITS_7;
            break;
        default:
            hwConfig.controlSetting |= ARM_USART_DATA_BITS_8;
            break;
    }

    switch(uartFrameFormat.config.parity)
    {
        case 1:
            hwConfig.controlSetting |= ARM_USART_PARITY_ODD;
            break;
        case 2:
            hwConfig.controlSetting |= ARM_USART_PARITY_EVEN;
            break;
        default:
            hwConfig.controlSetting |= ARM_USART_PARITY_NONE;
            break;
    }

    switch(uartFrameFormat.config.stopBits)
    {
        case 2:
            hwConfig.controlSetting |= ARM_USART_STOP_BITS_2;
            break;
        default:
            hwConfig.controlSetting |= ARM_USART_STOP_BITS_1;
            break;
    }

    switch(baudRate)
    {
        case 9600:
        case 4800:
        case 2400:
        case 1200:
        case 600:
            hwConfig.powerMode = ARM_POWER_LOW;
            break;
        default:
            break;
    }

    halUartConfig.uartDriverHandler = uartDriverHandler;
    halUartConfig.recv_cb = atCmdRecvCallback;
    halUartConfig.recvBuffPtr = at_uart_recv_buf;
    halUartConfig.recvBuffSize = AT_CMD_RECV_BUFFER_SIZE;
    halUartConfig.post_send_cb = atCmdPostSendCallback;
    HAL_UART_InitHandler(AT_CMD_HAL_UART_IDX, &halUartConfig, &hwConfig, HAL_UART_TASK_CREATE_FLAG_SEND_RECV);
}

/**
  \fn     void atCmdLowPowerEnterCallBack(void)
  \brief  URC report on entering Low power state
  \return
*/
static void atCmdLowPowerEnterCallBack(void *pdata, slpManLpState state)
{
    switch(state)
    {
        case SLPMAN_HIBERNATE_STATE:
			//USER ADD
    	    atCmdDirectSendStr(HIB_ENTER_URC_STR, strlen(HIB_ENTER_URC_STR));		

			if(get_atcmd_name() == 1)
			{
				user_disable_lpusart1();
			}
			
			
            if(mwGetAtChanConfigItemValue(uart_at_chann, MID_WARE_AT_CHAN_ECPURC_HIB_RPT_CFG) == 1)
            {
#ifdef FEATURE_REF_AT_ENABLE
                extern BOOL CerrcBeDeactState(void);
                if((slpManAPPGetAONFlag1() == false) && (CerrcBeDeactState() == true) && (slpManGetEstimateSlpTime() > HIB_REPORT_THD_MS))
#endif
                {
					ECOMM_STRING(UNILOG_ATCMD_EXEC, atCmdLowPowerEnterCallBack_1, P_SIG, "AT CMD, URC: %s", (const UINT8 *)(HIB_ENTER_URC_STR+2));
                    atCmdDirectSendStr(HIB_ENTER_URC_STR, strlen(HIB_ENTER_URC_STR));
                    slpManAPPSetAONFlag1(true);
                }
            }
            break;
        default:
            break;
    }
    return;
}

/**
  \fn     void atCmdLowPowerExitCallBack(void)
  \brief  URC report on exit from Low power state
  \return
*/
static void atCmdLowPowerExitCallBack(void *pdata, slpManLpState state)
{
    switch(state)
    {
#ifndef FEATURE_REF_AT_ENABLE
        case SLPMAN_HIBERNATE_STATE:
            if(mwGetAtChanConfigItemValue(uart_at_chann, MID_WARE_AT_CHAN_ECPURC_HIB_RPT_CFG) == 1)
            {
                if(slpManAPPGetAONFlag1() == true)
                {
					ECOMM_STRING(UNILOG_ATCMD_EXEC, atCmdLowPowerEnterCallBack_1, P_SIG, "AT CMD, URC: %s", (const UINT8 *)(HIB_FAIL_URC_STR+2));
                    atCmdDirectSendStr(HIB_FAIL_URC_STR, strlen(HIB_FAIL_URC_STR));
                }
            }
            break;
#endif
        default:
            break;
    }

    return;
}

/**
  \fn     void atCmdAdjustPmuTimingCfg(uint32_t preHibURCStrLen, uint32_t postHibURCStrLen, uint32_t baudRate)
  \brief  Adjust pmu timing introduced by HIB URC print
  \param[in] preHibURCStrLen     length of URC string on entering Hib
  \param[in] postHibURCStrLen    length of URC string on failing to enter Hib
  \return
*/
static void atCmdAdjustPmuTimingCfg(uint32_t preHibURCStrLen, uint32_t postHibURCStrLen)
{
    pmuTimeCfg_t cfg;

    uint32_t baudRate = BSP_GetPlatConfigItemValue(PLAT_CONFIG_ITEM_AT_PORT_BAUDRATE);

    baudRate &= 0x7FFFFFFFUL;

    if(baudRate == 0)
    {
        baudRate = 115200;
    }

	slpManRestoreDefaultPmuTimingCfg();

    slpManGetCurrentPmuTimingCfg(&cfg);

    // Use 8-N-1 UART port setting for estimation
    cfg.prehib_coderun_time = cfg.prehib_coderun_time + (preHibURCStrLen*10*1000/baudRate) + (postHibURCStrLen*10*1000/baudRate);
    cfg.worth_hib_time = cfg.worth_hib_time + (preHibURCStrLen*10*1000/baudRate) + (postHibURCStrLen*10*1000/baudRate);

    slpManSetPmuTimingCfg(&cfg);
}


void atCmdURCReportInPMUFlow(bool enable)
{
	if(enable)
	{
#ifndef FEATURE_REF_AT_ENABLE
		atCmdAdjustPmuTimingCfg(strlen(HIB_ENTER_URC_STR), strlen(HIB_FAIL_URC_STR));
#else
		atCmdAdjustPmuTimingCfg(strlen(HIB_ENTER_URC_STR), 0);
#endif
	}
	else
	{
		atCmdAdjustPmuTimingCfg(0, 0);   // use default pmu timing cfg
	}
}


/**
  \fn     void atCmdRegisterAtChannel(void)
  \brief  Apply an AT channel
  \return
*/
void atCmdRegisterAtChannel(void)
{
    AtChanRegInfo pAtRegInfo;

    memset((CHAR *)&pAtRegInfo, 0, sizeof(pAtRegInfo));
    pAtRegInfo.atRespFunc = atCmdUartRespFunc;
    pAtRegInfo.atUrcFunc = atCmdUartUrcFunc;
    memcpy(pAtRegInfo.chanName, AT_DEFAULT_CHAN_NAME, AT_DEFAULT_CHAN_NAME_LEN);

    uart_at_chann = atRegisterAtChannel(&pAtRegInfo);
    HAL_UART_RecvFlowControl(false);

    slpManRegisterPredefinedBackupCb(SLP_CALLBACK_ATCMD_MODULE, atCmdLowPowerEnterCallBack, NULL, SLPMAN_HIBERNATE_STATE);
    slpManRegisterPredefinedRestoreCb(SLP_CALLBACK_ATCMD_MODULE, atCmdLowPowerExitCallBack, NULL, SLPMAN_HIBERNATE_STATE);

	if(mwGetAtChanConfigItemValue(uart_at_chann, MID_WARE_AT_CHAN_ECPURC_HIB_RPT_CFG) == 1)
    {
		atCmdURCReportInPMUFlow(true);
	}

	//USER ADD
	if(slpManGetLastSlpState() == SLP_HIB_STATE)
	{
		HAL_UART_SendMsg(HAL_UART_SEND_MSG_TYPE_STR, AT_CMD_HAL_UART_IDX, HIB_EXIT_URC_STR, strlen(HIB_EXIT_URC_STR));
	}

#ifndef FEATURE_REF_AT_ENABLE
    if((slpManGetLastSlpState() == SLP_HIB_STATE) && (mwGetAtChanConfigItemValue(uart_at_chann, MID_WARE_AT_CHAN_ECPURC_HIB_RPT_CFG) == 1))
    {
        if(slpManAPPGetAONFlag1() == true)
        {
            HAL_UART_SendMsg(HAL_UART_SEND_MSG_TYPE_STR, AT_CMD_HAL_UART_IDX, HIB_EXIT_URC_STR, strlen(HIB_EXIT_URC_STR));
        }
    }
#endif
}


