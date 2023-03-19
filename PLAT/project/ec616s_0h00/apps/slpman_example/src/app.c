/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.c
 * Description:  EC616 at command demo entry source file
 * History:      Rev1.0   2018-07-12
 *
 ****************************************************************************/
#include "bsp.h"
#include "bsp_custom.h"
#include "osasys.h"
#include "ostask.h"
#include "slpman_ec616s.h"
//#include "psproxytask.h"
#include "debug_log.h"
#include "plat_config.h"
#include "app.h"
#include "mw_config.h"
#include "netmgr.h"
#include "ps_event_callback.h"
#include "cmips.h"
#include "ps_lib_api.h"

// at cmd task static stack and control block
#define USRAPP_TASK_STACK_SIZE        (1024)
typedef enum
{
    EXAMPLE_STEP_1 = 0,         // keep in sleep1 state
    EXAMPLE_STEP_2,
    EXAMPLE_STEP_3,
    EXAMPLE_STEP_4
}exampleSM;


static StaticTask_t             usrapp_task;
static uint8_t                  usrapp_task_stack[USRAPP_TASK_STACK_SIZE];
static exampleSM state;

#define EXAMPLE_STATE_SWITCH(a)  (state = a)


uint8_t slpmanSlpHandler           = 0xff;
uint8_t deepslpTimerID          = 0;


static void appBeforeHib(void *pdata, slpManLpState state)
{
    uint32_t *p_param = (uint32_t *)pdata;

    ECOMM_TRACE(UNILOG_PLA_APP, appBeforeHib_1, P_SIG, 1, "Before Hibernate = %x",*p_param);

}

static void appAfterHib(void *pdata, slpManLpState state)
{
    ECOMM_TRACE(UNILOG_PLA_APP, appAfterHib_1, P_SIG, 0, "Try Hibernate Failed, Interrupt Pending. Only sleep failed this function will excute");
}


static void appBeforeSlp1(void *pdata, slpManLpState state)
{
    ECOMM_TRACE(UNILOG_PLA_APP, appBeforeSlp1_1, P_SIG, 0, "Before Sleep1");
}

static void appAfterSlp1(void *pdata, slpManLpState state)
{
    ECOMM_TRACE(UNILOG_PLA_APP, appAfterSlp1_1, P_SIG, 0, "After Sleep1, no matter sleep success or not this function will excute");
}


static void appBeforeSlp2(void *pdata, slpManLpState state)
{
    ECOMM_TRACE(UNILOG_PLA_APP, appBeforeSlp2_1, P_SIG, 0, "Before Sleep2");
}

static void appAfterSlp2(void *pdata, slpManLpState state)
{
    ECOMM_TRACE(UNILOG_PLA_APP, appAfterSlp2_1, P_SIG, 0, "Sleep2 Failed, Interrupt Pending. Only sleep failed this function will excute");
}



static void UserAppTask(void *arg)
{
    uint32_t inParam = 0xAABBCCDD;

    uint32_t cnt;

    slpManSetPmuSleepMode(true,SLP_HIB_STATE,false);

    slpManApplyPlatVoteHandle("SLPMAN",&slpmanSlpHandler);            // apply an app layer vote handle

    slpManRegisterUsrdefinedBackupCb(appBeforeHib,&inParam,SLPMAN_HIBERNATE_STATE);
    slpManRegisterUsrdefinedRestoreCb(appAfterHib,NULL,SLPMAN_HIBERNATE_STATE);

    slpManRegisterUsrdefinedBackupCb(appBeforeSlp1,NULL,SLPMAN_SLEEP1_STATE);
    slpManRegisterUsrdefinedRestoreCb(appAfterSlp1,NULL,SLPMAN_SLEEP1_STATE);

    slpManRegisterUsrdefinedBackupCb(appBeforeSlp2,NULL,SLPMAN_SLEEP2_STATE);
    slpManRegisterUsrdefinedRestoreCb(appAfterSlp2,NULL,SLPMAN_SLEEP2_STATE);

    slpManSlpState_t slpstate = slpManGetLastSlpState();

    if((slpstate == SLP_SLP2_STATE) || (slpstate == SLP_HIB_STATE))     // wakeup from sleep
    {
        ECOMM_TRACE(UNILOG_PLA_APP, UserAppTask_0, P_SIG, 1, "Wakeup From state = %u", slpstate);
        while(1)                // now app can enter hib, but ps and phy maybe not, so wait here
        {
            osDelay(3000);
        }
    }
    else
    {

        EXAMPLE_STATE_SWITCH(EXAMPLE_STEP_1);

        while(1)
        {
            switch(state)
            {
                case EXAMPLE_STEP_1:
                {
                    slpManPlatVoteDisableSleep(slpmanSlpHandler, SLP_SLP2_STATE);  // sleep deeper than sleep2(include) is prohibited
                    
                    ECOMM_TRACE(UNILOG_PLA_APP, UserAppTask_1, P_SIG, 0, "Example Step1: Keep in Sleep1 state");

                    cnt = 0;

                    while(cnt<20)
                    {
                        cnt++;

                        osDelay(3000);
                    }

                    EXAMPLE_STATE_SWITCH(EXAMPLE_STEP_2);
                    break;
                }
                case EXAMPLE_STEP_2:
                {

                    slpManSlpState_t State;
                    uint8_t cnt;

                    if(slpManCheckVoteState(slpmanSlpHandler, &State, &cnt)==RET_TRUE)
                    {
                        ECOMM_TRACE(UNILOG_PLA_APP, UserAppTask_2, P_SIG, 2, "We Can Check Vote State, state=%u, cnt=%u",State,cnt);
                    }

                    slpManPlatVoteEnableSleep(slpmanSlpHandler, SLP_SLP2_STATE);  // cancel the prohibition of sleep2

                    if(slpManCheckVoteState(slpmanSlpHandler, &State, &cnt)==RET_TRUE)
                    {
                        ECOMM_TRACE(UNILOG_PLA_APP, UserAppTask_3, P_SIG, 2, "We Can Check Vote State Again, state=%u, cnt=%u",State,cnt);
                    }

                    ECOMM_TRACE(UNILOG_PLA_APP, UserAppTask_4, P_SIG, 0, "Now App cancel the prohibition of sleep2.");

                    slpManDeepSlpTimerStart(deepslpTimerID, 30000);     // create a 30s timer, DeepSleep Timer is always oneshoot

                    while(1)                // now app can enter hib, but ps and phy maybe not, so wait here
                    {
                        osDelay(3000);
                    }
                }




            }
        }

    }



}


static void appInit(void *arg)
{
    osThreadAttr_t task_attr;

    memset(&task_attr,0,sizeof(task_attr));
    memset(usrapp_task_stack, 0xA5,USRAPP_TASK_STACK_SIZE);
    task_attr.name = "usrapp";
    task_attr.stack_mem = usrapp_task_stack;
    task_attr.stack_size = USRAPP_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &usrapp_task;//task control block
    task_attr.cb_size = sizeof(StaticTask_t);//size of task control block

    osThreadNew(UserAppTask, NULL, &task_attr);

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
