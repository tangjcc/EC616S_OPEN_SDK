
#include "mw_init.h"
#include "cmsis_compiler.h"

__WEAK void dmDeepSleepTimerCbRegister(void)
{

}


__WEAK void fotaDeepSleepTimerCbRegister(void)
{

}

void middleware_init(void)
{
	dmDeepSleepTimerCbRegister();
	fotaDeepSleepTimerCbRegister();
}

