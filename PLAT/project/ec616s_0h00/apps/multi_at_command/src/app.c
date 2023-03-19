/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.c
 * Description:  EC616 at command demo entry source file
 * History:      Rev1.0   2018-07-12
 *
 ****************************************************************************/
#include "string.h"
#include "bsp.h"
#include "bsp_custom.h"
#include "ostask.h"
#include "debug_log.h"
#include "plat_config.h"
#include "app.h"
#include "FreeRTOS.h"
#include "at_def.h"
#include "at_api.h"
#include "mw_config.h"
#include "slpman_ec616s.h"
#include "hal_uart_multi_atcmd.h"

#if defined(FEATURE_CTCC_DM_ENABLE) || defined(FEATURE_CUCC_DM_ENABLE) || defined(FEATURE_CMCC_DM_ENABLE)
#include "autoreg_common.h"
#endif

extern void ecAutoRegisterInit(void);

/**
  \fn          static void appInit(void *arg)
  \brief
  \return
*/
static void appInit(void *arg)
{
    uint8_t ec_autoreg_enable_flag = 0xff;

    atCmdRegisterMultiAtChannel();

    ec_autoreg_enable_flag = mwGetEcAutoRegEnableFlag();
    if(ec_autoreg_enable_flag == 2)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, ec_autoReg100, P_SIG, 0, "autoReg is enable");
        #if defined(FEATURE_CTCC_DM_ENABLE) || defined(FEATURE_CUCC_DM_ENABLE) || defined(FEATURE_CMCC_DM_ENABLE)
        ecAutoRegisterInit();
        #else
        ECOMM_TRACE(UNILOG_PLA_APP, ec_autoReg101, P_SIG, 0, "autoReg all the reg code is not compiled");
        #endif
    }
    else
    {
        ECOMM_TRACE(UNILOG_PLA_APP, ec_autoReg102, P_SIG, 0, "autoReg is disable");
    }
    //Below are app low power entrys
    // 1. CTIOT
#ifdef FEATURE_CTWINGV1_5_ENABLE
    slpManRestoreUsrNVMem();
    extern void  ctiotEngineInit(uint8_t autoLoginFlag);
    ctiotEngineInit(0);
#endif
    // 2. ONENET
#ifdef FEATURE_CISONENET_ENABLE
    extern void onenetEngineInit(uint8_t autoLoginFlag);
    onenetEngineInit(0);
#endif
#ifdef FEATURE_LIBCOAP_ENABLE
    extern void coapEngineInit(uint32_t autoreg_flag);
    coapEngineInit(0);              
#endif

#ifdef FEATURE_WAKAAMA_ENABLE
    extern void lwm2mEngineInit(uint8_t autoLoginFlag);
    lwm2mEngineInit(0);
#endif


}

/**
  \fn          int main_entry(void)
  \brief       main entry function.
  \return
*/
void main_entry(void) {

    BSP_CommonInit();
    osKernelInitialize();
    registerAppEntry(appInit, NULL);
    if (osKernelGetState() == osKernelReady)
    {
        osKernelStart();
    }
    while(1);

}
