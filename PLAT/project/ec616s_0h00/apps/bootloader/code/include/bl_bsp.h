#ifndef  BL_BSP_H
#define  BL_BSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"
#include "slpman_ec616s.h"

void BSP_Init(void);
void BSP_DeInit(void);
void CleanBootRomResouce(void);
void bl_clk_check(void);
void SystemNormalBootInit(void);
uint8_t SystemWakeUpBootInit(void);
void BSP_QSPI_Set_Clk_68M(void);
void BSP_WdtInit(void);

void SelNormalOrURCPrint(uint32_t Sel);

#define LOG_ON      1


/*
 *  URC set enable and baudrate value config
 *  Parameter:   enable uart1 init for uarc, 0-disable, 1-enable
 *  Parameter:   baudrate for UART1(URC)
 */
void BSP_URCSetCfg(uint8_t enable,uint32_t baud);

/*
 *  WDT wdt timecnt value config
 *  Parameter:   Value - timecnt, timeout period equal to Value *20/32768 second
 */

void BSP_WdtTimeCntCfg(uint32_t Value);
/**
  \fn          void BlNormalIOVoltSet(IOVoltageSel_t sel);
  \brief       select normal io voltage.
  \return      none
*/
void BlNormalIOVoltSet(IOVoltageSel_t sel);
/**
  \fn          void BlAONIOVoltSet(IOVoltageSel_t sel);
  \brief       select aon io voltage.
  \return      none
*/
void BlAONIOVoltSet(IOVoltageSel_t sel);



#ifdef __cplusplus
}
#endif

#endif /* BL_BSP_H */
