/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.c
 * Description:  EC616S fs example entry source file
 * History:      Rev1.0   2018-07-12
 *
 ****************************************************************************/

#include "bsp_custom.h"
#include "ostask.h"
#include "app.h"
#include "FreeRTOS.h"

/**
  \fn          int main_entry(void)
  \brief       main entry function.
  \return
*/
void main_entry(void)
{
    BSP_CommonInit();
    osKernelInitialize();
    registerAppEntry(fs_example, NULL);
    if (osKernelGetState() == osKernelReady)
    {
        osKernelStart();
    }
    while(1);
}
