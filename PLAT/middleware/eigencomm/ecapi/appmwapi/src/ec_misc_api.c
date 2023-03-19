/******************************************************************************
 * (C) Copyright 2018 EIGENCOMM International Ltd.
 * All Rights Reserved
*******************************************************************************
 *  Filename: ec_misc_api.c
 *
 *  Description:
 *
 *  History:
 *
 *  Notes:
 *
******************************************************************************/

 #include "ec_misc_api.h"
 #include "npi_config.h"
 #include "debug_log.h"

#if defined CHIP_EC616
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif


#ifdef FEATURE_PRODMODE_ENABLE

static osEventFlagsId_t         g_prodModeEnterExitEventFlag = NULL;
static bool                     g_isProdModeOngoing = false;

static void npiInitProdModeEventFlag(void)
{
    if(g_prodModeEnterExitEventFlag == NULL)
    {
        g_prodModeEnterExitEventFlag = osEventFlagsNew(NULL);
        EC_ASSERT(g_prodModeEnterExitEventFlag, g_prodModeEnterExitEventFlag, 0, 0);
    }
}

static void npiDeInitProdModeEventFlag(void)
{
    if(g_prodModeEnterExitEventFlag != NULL)
    {
        osEventFlagsDelete(g_prodModeEnterExitEventFlag);
        g_prodModeEnterExitEventFlag = NULL;
    }
}

void npiSetProdModeEnterFlag(void)
{
    npiInitProdModeEventFlag();
    osEventFlagsSet(g_prodModeEnterExitEventFlag, PRODMODE_ENTER_FLAG);
}

void npiSetProdModeExitFlag(void)
{
    npiInitProdModeEventFlag();
    osEventFlagsSet(g_prodModeEnterExitEventFlag, PRODMODE_EXIT_FLAG);

}

int32_t npiCheckProdMode(uint32_t timeout_ms)
{
    int32_t ret = osOK;
    uint8_t prodModePmuHdr;
    slpManRet_t pmuRet;

    npiInitProdModeEventFlag();

    // 1. Check if prodMode is enabled first
    if(npiGetProdModeEnableStatus())
    {
        ECOMM_TRACE(UNILOG_EC_API, npiCheckProdMode_0, P_DEBUG, 1, "prodMode is enabled, wait %d ms for entering prodMode...", timeout_ms);

        // 2. prevent from sleeping
        pmuRet = slpManApplyPlatVoteHandle("prodMode", &prodModePmuHdr);
        OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

        pmuRet = slpManPlatVoteDisableSleep(prodModePmuHdr, SLP_SLP1_STATE);
        OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);


        // 3. Wait for prodMode enter flag is set in given time
        ret = osEventFlagsWait(g_prodModeEnterExitEventFlag, PRODMODE_ENTER_FLAG, osFlagsWaitAll, timeout_ms);

        if(ret == PRODMODE_ENTER_FLAG)
        {
            g_isProdModeOngoing = true;

            ECOMM_TRACE(UNILOG_EC_API, npiCheckProdMode_1, P_DEBUG, 0, "Enter prodMode, wait until prodMode exits!");

            // 3. Wait until prodMode exit flag is set
            ret = osEventFlagsWait(g_prodModeEnterExitEventFlag, PRODMODE_EXIT_FLAG, osFlagsWaitAll, osWaitForever);

            ECOMM_TRACE(UNILOG_EC_API, npiCheckProdMode_2, P_DEBUG, 1, "Exit prodMode, error code:0x%x!", ret);

            g_isProdModeOngoing = false;

            npiDeInitProdModeEventFlag();

        }
        else if(ret == osFlagsErrorTimeout)
        {
            ECOMM_TRACE(UNILOG_EC_API, npiCheckProdMode_3, P_DEBUG, 0, "Enter prodMode timeout!");
        }

        // 4. exit prodMode, permit to sleep
        pmuRet = slpManPlatVoteEnableSleep(prodModePmuHdr, SLP_SLP1_STATE);
        OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

        pmuRet = slpManGivebackPlatVoteHandle(prodModePmuHdr);
        OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);
    }

    return ret;
}

void npiClearProdMode(void)
{
    npiSetProdModeEnableStatus(false);
}

bool npiIsProdModeOngoing(void)
{
    return g_isProdModeOngoing;
}

#endif

