/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.h
 * Description:  EC616 at command demo entry header file
 * History:      Rev1.0   2018-07-12
 *
 ****************************************************************************/
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif

#define CTCC_AUTO_REG_TASK_STACK_SIZE   2300 
#define CTCC_COAP_LOCAL_PORT   5188

#define CTCC_AUTO_REG_RETRY_TIMEOUT_ID  DEEPSLP_TIMER_ID8
#define CTCC_AUTO_REG_RETRY  0

void ctccAutoRegisterInit(void);
void ctccDeepSleepTimerCbRegister(void);

