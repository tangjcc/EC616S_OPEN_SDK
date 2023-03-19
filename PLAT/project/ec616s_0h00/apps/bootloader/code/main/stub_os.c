#include <string.h>
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2
#include "FreeRTOS.h"                   // ARM.FreeRTOS::RTOS:Core
#include "task.h"                       // ARM.FreeRTOS::RTOS:Core
#include "semphr.h"                     // ARM.FreeRTOS::RTOS:Core
#include "slpman_ec616s.h"
#include "plat_config.h"

#ifndef IS_IRQ_MODE
#define IS_IRQ_MODE()             (__get_IPSR() != 0U)
#endif


osStatus_t osSemaphoreAcquire (osSemaphoreId_t semaphore_id, uint32_t timeout) {
    return (osStatus_t)0;    
}

osStatus_t osSemaphoreDelete (osSemaphoreId_t semaphore_id) {
    return (osStatus_t)0;
}

osSemaphoreId_t osSemaphoreNew (uint32_t max_count, uint32_t initial_count, const osSemaphoreAttr_t *attr) {
    return 0;
}

osStatus_t osSemaphoreRelease (osSemaphoreId_t semaphore_id) {
    return (osStatus_t)0;
}


slpManRet_t slpManRegisterPredefinedBackupCb(slpCbModule_t module, slpManBackupCb_t backup_cb, void *pdata, slpManLpState state)
{
    return (slpManRet_t)0;
}

slpManRet_t slpManRegisterPredefinedRestoreCb(slpCbModule_t module, slpManRestoreCb_t restore_cb, void *pdata, slpManLpState state)
{
    return (slpManRet_t)0;
}

void PhyDebugPrint(uint8_t moduleID, uint8_t subID, uint32_t *pSwLogData, uint32_t swLogLen, uint8_t uniLogLevel, uint8_t ramLogSwitch)

{
    return;
}

slpManRet_t slpManDrvVoteSleep(slpDrvVoteModule_t module, slpManSlpState_t status)
{
    return (slpManRet_t)0;
}

uint32_t BSP_GetPlatConfigItemValue(plat_config_id_t id)
{
    return 0;
}

char *pcTaskGetName( TaskHandle_t xTaskToQuery )
{
    return NULL;
}

TaskHandle_t xTaskGetCurrentTaskHandle( void )
{
    return (TaskHandle_t)NULL;
}

osStatus_t osDelay (uint32_t ticks)
{
    return (osStatus_t)0;
}

osKernelState_t osKernelGetState (void)
{
    return (osKernelState_t)0;
}

uint8_t osThreadIsSuspendAll (void)
{
    return 0;
}



