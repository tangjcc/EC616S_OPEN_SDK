#ifndef __COMMON_H__
#define __COMMON_H__


//#include "ARMCM3.h"
#include "ec616s.h"
#include "core_cm3.h"

#define ISRAM_PHY_ADDR 0x04000000

#define TICK_LOAD_VALUE 0x800000  //256 seconds for tick period

extern void trace(char*log,int len);
extern void show_err(uint32_t err);
void uDelay(void);

//#define QSPI_DRIVER_ORG
#endif
