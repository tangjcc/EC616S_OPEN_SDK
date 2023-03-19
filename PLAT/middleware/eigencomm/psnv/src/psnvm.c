/******************************************************************************
 ******************************************************************************
 Copyright:      - 2017- Copyrights of EigenComm Ltd.
 File name:      - psnvm.c
 Description:    - Protocol Stack NVM files
 History:        - 13/09/2017, Originated by jcweng
 ******************************************************************************
******************************************************************************/
#include "psnvm.h"
#include "psnvmutil.h"

/******************************************************************************
 ******************************************************************************
 * INERNAL/STATIC FUNCTION SPECIFICATION
 ******************************************************************************
******************************************************************************/
static void PsNvmSetCcmConfigDefaultFunc(void *ctxBuf, UINT16 bufSize);
static BOOL PsNvmAdjustCcmConfigVerFunc(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize);
static void PsNvmSetEmmInformationDefaultFunc(void *ctxBuf, UINT16 bufSize);
static BOOL PsNvmAdjustEmmInformationVerFunc(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize);
static void PsNvmSetUeInformationDefaultFunc(void *ctxBuf, UINT16 bufSize);
static BOOL PsNvmAdjustUeInformationVerFunc(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize);
static void PsNvmSetCemmPlmnConfigDefaultFunc(void *ctxBuf, UINT16 bufSize);
static BOOL PsNvmAdjustCemmPlmnConfigVerFunc(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize);
static void PsNvmSetCemmEsmConfigDefaultFunc(void *ctxBuf, UINT16 bufSize);
static BOOL PsNvmAdjustCemmEsmConfigVerFunc(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize);
static void PsNvmSetUiccCtrlConfigDefaultFunc(void *ctxBuf, UINT16 bufSize);
static BOOL PsNvmAdjustUiccCtrlConfigVerFunc(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize);
static void PsNvmSetCerrcConfigDefaultFunc(void *ctxBuf, UINT16 bufSize);
static BOOL PsNvmAdjustCerrcConfigVerFunc(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize);


/******************************************************************************
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
******************************************************************************/
/******************************************************************************
 * psNvmFileOperList
 * PS NVM file operation specification list
 * READ only, should STORE in flash
******************************************************************************/
const PsNvmFileOper psNvmFileOperList[] =
{
    {
        PS_CCM_CONFIG_NVM,                  /* fileId */
        CUR_CCM_CFG_VER,                    /* curVersion */
        sizeof(PsNvmCcmConfig),             /* fileSize */
        "ccmconfig.nvm",                    /* fileName */
        PsNvmSetCcmConfigDefaultFunc,       /* setDefaultFunc */
        PsNvmAdjustCcmConfigVerFunc         /* adjustVerFunc */
    },
    {
        PS_CEMM_CONFIG_EMM_INFORMATION_NVM,     /* fileId */
        CUR_CEMM_EMM_INFORMATION_CFG_VER,       /* curVersion */
        sizeof(PsNvmCemmConfigEmmInformation),  /* fileSize */
        "cemmconfigemminformation.nvm",         /* fileName */
        PsNvmSetEmmInformationDefaultFunc,      /* setDefaultFunc */
        PsNvmAdjustEmmInformationVerFunc        /* adjustVerFunc */
    },
    {
        PS_CEMM_CONFIG_UE_INFORMATION_NVM,      /* fileId */
        CUR_CEMM_UE_INFORMATION_CFG_VER,        /* curVersion */
        sizeof(PsNvmCemmConfigUeInformation),   /* fileSize */
        "cemmconfigueinformation.nvm",          /* fileName */
        PsNvmSetUeInformationDefaultFunc,       /* setDefaultFunc */
        PsNvmAdjustUeInformationVerFunc         /* adjustVerFunc */
    },
    {
        PS_CEMM_PLMN_CONFIG_NVM,            /* fileId */
        CUR_CEMM_PLMN_NVM_CFG_VER,          /* curVersion */
        sizeof(CePlmnNvmConfig),            /* fileSize */
        "cemmplmnconfig.nvm",               /* fileName */
        PsNvmSetCemmPlmnConfigDefaultFunc,  /* setDefaultFunc */
        PsNvmAdjustCemmPlmnConfigVerFunc    /* adjustVerFunc */
    },
    {
        PS_CESM_CONFIG_NVM,                 /* fileId */
        CUR_CEMM_ESM_NVM_CFG_VER,           /* curVersion */
        sizeof(CemmEsmNvmConfig),           /* fileSize */
        "cemmesmconfig.nvm",                /* fileName */
        PsNvmSetCemmEsmConfigDefaultFunc,   /* setDefaultFunc */
        PsNvmAdjustCemmEsmConfigVerFunc     /* adjustVerFunc */
    },
    {
        PS_UICCCTRL_CONFIG_NVM,             /* fileId */
        CUR_UICCCTRL_NVM_CFG_VER,           /* curVersion */
        sizeof(UiccCtrlNvmConfig),          /* fileSize */
        "uiccctrlconfig.nvm",               /* fileName */
        PsNvmSetUiccCtrlConfigDefaultFunc,  /* setDefaultFunc */
        PsNvmAdjustUiccCtrlConfigVerFunc    /* adjustVerFunc */
    },
    {
        PS_CERRC_CONFIG_NVM,                /* fileId */
        CUR_CERRC_NVM_CFG_VER,              /* curVersion */
        sizeof(CerrcNvmConfig),             /* fileSize */
        "cerrcconfig.nvm",                  /* fileName */
        PsNvmSetCerrcConfigDefaultFunc,     /* setDefaultFunc */
        PsNvmAdjustCerrcConfigVerFunc       /* adjustVerFunc */
    },
    // ...
};


/******************************************************************************
 ******************************************************************************
 * INERNAL/STATIC FUNCTION
 ******************************************************************************
******************************************************************************/


/******************************************************************************
 * PsNvmSetCcmConfigDefaultFunc
 * Description: CCM CONFIG NVM file, default value
 * input: void *ctxBuf; //output buffer
 *        UINT16 bufSize; // buffer size
 * output: void
 * Comment:
******************************************************************************/
static void PsNvmSetCcmConfigDefaultFunc(void *ctxBuf, UINT16 bufSize)
{
    PsNvmCcmConfig *pCcmCfg = (PsNvmCcmConfig *)ctxBuf;
    GosCheck(ctxBuf != PNULL && bufSize == sizeof(PsNvmCcmConfig), ctxBuf, bufSize, sizeof(PsNvmCcmConfig));

    pCcmCfg->srvType           = EPS_ONLY;
    pCcmCfg->nwMode            = NB_IOT;
    pCcmCfg->bAutoApn          = FALSE;
    //pCcmCfg->bSIMTest          = FALSE;
    pCcmCfg->bUsimSimulator    = FALSE;
    pCcmCfg->bRohc             = TRUE;
    pCcmCfg->bIpv6RsForTestSim = FALSE;
    pCcmCfg->bAutoBand         = FALSE;
    pCcmCfg->powerOnCfun       = 1;    //CFUN1 when power on
    pCcmCfg->powerOnMaxDelay   = 0;
    pCcmCfg->ipv6GetPrefixTime = 15;
    pCcmCfg->bEnableBip        = TRUE;
    pCcmCfg->bEnableSimPsm     = TRUE;
    pCcmCfg->smsService        = 0;
    //pCcmCfg->bSimSleep = TRUE;
#ifdef PS_NB_SUPPORT_HS_REF_FEATURE
    pCcmCfg->t3324AndT3412ExtCeregUrc = FALSE;
#else
    pCcmCfg->t3324AndT3412ExtCeregUrc = TRUE;
#endif
    pCcmCfg->eventStatisMode = FALSE;
    pCcmCfg->smsMoreMessageMode = 0;

#ifdef PS_NB_SUPPORT_HS_REF_FEATURE
    pCcmCfg->cfun1PowerOnPs = FALSE;
#else
    pCcmCfg->cfun1PowerOnPs = TRUE;
#endif
    pCcmCfg->uint8Default0Rsvd = 0;

    pCcmCfg->uint8Default0Rsvd1 = 0;
    pCcmCfg->bEnablePsSoftReset = 1;

    pCcmCfg->uint32Default0Rsvd = 0;
    pCcmCfg->uint32Default1Rsvd = 1;

    return;
}

/******************************************************************************
 * PsNvmAdjustCcmConfigVerFunc
 * Description: CCM CONFIG NVM file, OLD version file -> NEW version file
 * input: UINT8 oldVer; // old version
 *        void *oldCtx; // old NVM context, input value
 *        UINT16 oldCtxSize; // size of "oldCtx", input value
 *        void *curCtx; // cur version NVM context, out put value
 *        UINT16 curCtxSize; // size of "curCtx"
 * output: BOOL
 * Comment:
******************************************************************************/
static BOOL PsNvmAdjustCcmConfigVerFunc(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize)
{
    PsNvmCcmConfig_v00    *pCcmNvmCfg_v00 = PNULL;
    PsNvmCcmConfig        *pCurCcmNvmCfg = (PsNvmCcmConfig *)curCtx;

    OsaCheck(oldCtx != PNULL && curCtx != PNULL, oldVer, oldCtx, curCtx);

    if (oldVer == 0)
    {
        pCcmNvmCfg_v00 = (PsNvmCcmConfig_v00 *)oldCtx;

        GosDebugBegin(sizeof(PsNvmCcmConfig_v00) == oldCtxSize, sizeof(PsNvmCcmConfig_v00), oldCtxSize, 0);
        return FALSE;
        GosDebugEnd();

        GosDebugBegin(sizeof(PsNvmCcmConfig) == curCtxSize, sizeof(PsNvmCcmConfig), curCtxSize, 0);
        return FALSE;
        GosDebugEnd();

        memset(pCurCcmNvmCfg, 0x00, sizeof(PsNvmCcmConfig));

        memcpy(pCurCcmNvmCfg,
               pCcmNvmCfg_v00,
               sizeof(PsNvmCcmConfig_v00));

        /* V1 add new flag: t3324AndT3412ExtCeregUrc,uint8Default0Rsvd,uint8Default1Rsvd,
           uint16Default0Rsvd,uint16Default1Rsvd,uint32Default0Rsvd,uint32Default1Rsvd
           V1 del old flag: rsvd1,rsvd2
        */
        pCurCcmNvmCfg->t3324AndT3412ExtCeregUrc = TRUE;

        pCurCcmNvmCfg->eventStatisMode = FALSE;
        pCurCcmNvmCfg->smsMoreMessageMode = 0;
        pCurCcmNvmCfg->cfun1PowerOnPs = FALSE;
        pCurCcmNvmCfg->uint8Default0Rsvd = 0;
        pCurCcmNvmCfg->uint8Default0Rsvd1 = 0;
        pCurCcmNvmCfg->bEnablePsSoftReset = 1;
        pCurCcmNvmCfg->uint32Default0Rsvd = 0;
        pCurCcmNvmCfg->uint32Default1Rsvd = 1;

        return TRUE;
    }

    GosDebugBegin(FALSE, oldVer, oldCtxSize, 0);
    GosDebugEnd();

    return FALSE;
}

/******************************************************************************
 * PsNvmSetPsmDefaultValue
 * Description: set default PSM value
 * input: GprsTimer2 *pT3324Value, GprsTimer3 *pT3412ExtendedValue
 * output: void
 * Comment:
******************************************************************************/
void PsNvmSetPsmDefaultValue(GprsTimer2 *pT3324Value, GprsTimer3 *pT3412ExtendedValue)
{
    GosCheck((pT3324Value != PNULL && pT3412ExtendedValue != PNULL), pT3324Value, pT3412ExtendedValue, 0);

#ifdef PS_NB_SUPPORT_HS_REF_FEATURE
    pT3324Value->unit = TIMER1_MULTIPLES_OF_TWO_SECONDS;
    pT3324Value->timerValue = 5;
    pT3412ExtendedValue->unit = TIMER3_MULTIPLES_OF_TEN_HOURS;
    pT3412ExtendedValue->timerValue = 1;
#else
    pT3324Value->unit = TIMER1_MULTIPLES_OF_ONE_MINUTE;
    pT3324Value->timerValue = 5;
    pT3412ExtendedValue->unit = TIMER3_MULTIPLES_OF_ONE_HOUR;
    pT3412ExtendedValue->timerValue = 20;
#endif

    return;
}

/******************************************************************************
 * PsNvmSetEdrxDefaultValue
 * Description: set default PSM value
 * input: ExtendedDrxParameters   *pEdrxValue
 * output: void
 * Comment:
******************************************************************************/
void PsNvmSetEdrxDefaultValue(ExtendedDrxParameters   *pEdrxValue)
{
    GosCheck((pEdrxValue != PNULL), pEdrxValue, 0, 0);

#ifdef PS_NB_SUPPORT_HS_REF_FEATURE
    pEdrxValue->edrxValue = EDRX_CYCLE_LEN_20_SEC_480_MS;
#else
    pEdrxValue->edrxValue = EDRX_CYCLE_LEN_40_SEC_960_MS;
#endif
    pEdrxValue->ptw = PAGING_TIME_WINDOW_NB_10_SEC_240_MS;

    return;
}

/******************************************************************************
 * PsNvmSetCciotoptDefaultValue
 * Description: set default CCIOTOPT value
 * input: CemmCiotOptTypeEnum     *pSuppCiotOpt
 *        CemmCiotOptTypeEnum     *pPrefCiotOptimization
 * output: void
 * Comment:
******************************************************************************/
void PsNvmSetCciotoptDefaultValue(CemmCiotOptTypeEnum   *pSuppCiotOpt,
                                  CemmCiotOptTypeEnum   *pPrefCiotOptimization,
                                  UINT8                 *pNetworkCapability)
{
    GosCheck((pSuppCiotOpt != PNULL) && (pPrefCiotOptimization != PNULL), 0, 0, 0);

    *pSuppCiotOpt = CEMM_CP_CIOT_OPT;           /* Default support Control Plane  CIoT EPS optimization */
    *pPrefCiotOptimization = CEMM_CP_CIOT_OPT;

    switch (*pSuppCiotOpt)
    {
        case CEMM_NO_CIOT_OPT:
            pNetworkCapability[5] &= 0xF3;
            break;
        case CEMM_CP_CIOT_OPT:
            pNetworkCapability[5] &= 0xE3;
            pNetworkCapability[5] |= 0x04;      /* bit 3 indicates 'Control plane CIoT EPS optimization' */
            pNetworkCapability[6] &= 0xFE;
            break;
        case CEMM_UP_CIOT_OPT:
            pNetworkCapability[5] &= 0xF3;
            pNetworkCapability[5] |= 0x18;      /* bit 4 indicates 'User plane CIoT EPS optimization' */
            pNetworkCapability[6] |= 0x01;
            break;
        case CEMM_CP_AND_UP_OPT:
            pNetworkCapability[5] |= 0x1C;      /* Should also set bit 'S1-U data' to TRUE */
            pNetworkCapability[6] |= 0x01;      /* Should also set bit 'multipleDRB' to TRUE */
            break;
    }

    return;
}

/******************************************************************************
 * PsNvmSetUeInformationDefaultFunc
 * Description: CEMM UE CONFIG NVM file, UE information default value
 * input: void *ctxBuf; //output buffer
 *        UINT16 bufSize; // buffer size
 * output: void
 * Comment:
******************************************************************************/
static void PsNvmSetUeInformationDefaultFunc(void *ctxBuf, UINT16 bufSize)
{
    PsNvmCemmConfigUeInformation *pCemmCfgUeInfo = (PsNvmCemmConfigUeInformation *)ctxBuf;

    GosCheck(ctxBuf != PNULL && bufSize == sizeof(PsNvmCemmConfigUeInformation),
             ctxBuf, bufSize, sizeof(PsNvmCemmConfigUeInformation));

    /* Ue Nw Capability Default Values Setting */
    memset(pCemmCfgUeInfo->cemmUeNwCapability.networkCapability, 0, UE_NETWORK_CAPABILITY_MAX_LEN);
    pCemmCfgUeInfo->cemmUeNwCapability.networkCapability[0] = 0xF0;
    pCemmCfgUeInfo->cemmUeNwCapability.networkCapability[1] = 0xF0;
    pCemmCfgUeInfo->cemmUeNwCapability.networkCapability[2] = 0x00;
    pCemmCfgUeInfo->cemmUeNwCapability.networkCapability[3] = 0x00;
    pCemmCfgUeInfo->cemmUeNwCapability.networkCapability[4] = 0x0C;
    pCemmCfgUeInfo->cemmUeNwCapability.networkCapability[5] = 0xE4;
    pCemmCfgUeInfo->cemmUeNwCapability.networkCapability[6] = 0x08;
    pCemmCfgUeInfo->cemmUeNwCapability.networkCapabilityLength = 7; /* Other 6 octets are spare */

    /* Ue Psm/Extended Periodic TAU/Edrx Default Values Setting */
    pCemmCfgUeInfo->cemmPsmEdrx.t3324Mode = CEMM_ENABLE_PSM;
    pCemmCfgUeInfo->cemmPsmEdrx.t3412ExtMode = CEMM_ENABLE_PSM;
    pCemmCfgUeInfo->cemmPsmEdrx.edrxMode = CEMM_ENABLE_EDRX;
    PsNvmSetPsmDefaultValue(&pCemmCfgUeInfo->cemmPsmEdrx.t3324Value, &pCemmCfgUeInfo->cemmPsmEdrx.t3412ExtendedValue);
    PsNvmSetEdrxDefaultValue(&pCemmCfgUeInfo->cemmPsmEdrx.extendedDrxParameters);

    /* Ue Comm Info Default Values Setting */
    pCemmCfgUeInfo->cemmCommInfo.smsOnly = TRUE;
    pCemmCfgUeInfo->cemmCommInfo.tauForSmsControl = FALSE;
#if defined (WIN32)
    pCemmCfgUeInfo->cemmCommInfo.tauForSmsControl = TRUE;
#endif
    pCemmCfgUeInfo->cemmCommInfo.userSetT3324Value = 0xFFFFFF;
    pCemmCfgUeInfo->cemmCommInfo.attachWithoutPdn = FALSE;
    pCemmCfgUeInfo->cemmCommInfo.userSetBarValue = 120;
    pCemmCfgUeInfo->cemmCommInfo.minT3324Value = 0;
    pCemmCfgUeInfo->cemmCommInfo.minT3412Value = 0;
    pCemmCfgUeInfo->cemmCommInfo.minEdrxValue = 0;

    PsNvmSetCciotoptDefaultValue(&pCemmCfgUeInfo->cemmCommInfo.ciotOptimization,
                                 &pCemmCfgUeInfo->cemmCommInfo.preferedCiotOptimization,
                                 pCemmCfgUeInfo->cemmUeNwCapability.networkCapability);

    pCemmCfgUeInfo->cemmCommInfo.enableNonIpNoSms = FALSE;

    pCemmCfgUeInfo->cemmCommInfo.emmEventInfo.numAttachFail = 0;
    pCemmCfgUeInfo->cemmCommInfo.emmEventInfo.numAttachSucc = 0;
    pCemmCfgUeInfo->cemmCommInfo.emmEventInfo.numAuthFail = 0;
    pCemmCfgUeInfo->cemmCommInfo.emmEventInfo.numDetach = 0;
    pCemmCfgUeInfo->cemmCommInfo.emmEventInfo.numSrFail = 0;
    pCemmCfgUeInfo->cemmCommInfo.emmEventInfo.numSrSucc = 0;
    pCemmCfgUeInfo->cemmCommInfo.emmEventInfo.numTauFail = 0;
    pCemmCfgUeInfo->cemmCommInfo.emmEventInfo.numTauSucc = 0;

    pCemmCfgUeInfo->cemmCommInfo.uint8Default0Rsvd = 0;
    pCemmCfgUeInfo->cemmCommInfo.uint8Default1Rsvd = 1;
    pCemmCfgUeInfo->cemmCommInfo.uint8Default2Rsvd = 0;
    pCemmCfgUeInfo->cemmCommInfo.uint8Default3Rsvd = 1;
    pCemmCfgUeInfo->cemmCommInfo.uint8Default4Rsvd = 0;
    pCemmCfgUeInfo->cemmCommInfo.uint8Default5Rsvd = 1;
    pCemmCfgUeInfo->cemmCommInfo.uint8Default6Rsvd = 0;
    pCemmCfgUeInfo->cemmCommInfo.uint8Default7Rsvd = 1;
    pCemmCfgUeInfo->cemmCommInfo.uint16Default0Rsvd = 0;
    pCemmCfgUeInfo->cemmCommInfo.uint16Default1Rsvd = 0;
    pCemmCfgUeInfo->cemmCommInfo.uint16Default2Rsvd = 0;
    pCemmCfgUeInfo->cemmCommInfo.uint32Default0Rsvd = 0;
    pCemmCfgUeInfo->cemmCommInfo.uint32Default1Rsvd = 0;
    pCemmCfgUeInfo->cemmCommInfo.uint32Default2Rsvd = 0;

    return;
}

/******************************************************************************
 * PsNvmSetEmmInformationDefaultFunc
 * Description: CEMM EMM CONFIG NVM file, EMM information default value
 * input: void *ctxBuf; //output buffer
 *        UINT16 bufSize; // buffer size
 * output: void
 * Comment:
******************************************************************************/
static void PsNvmSetEmmInformationDefaultFunc(void *ctxBuf, UINT16 bufSize)
{
    PsNvmCemmConfigEmmInformation *pCemmEmmCfg = (PsNvmCemmConfigEmmInformation *)ctxBuf;
    GosCheck(ctxBuf != PNULL && bufSize == sizeof(PsNvmCemmConfigEmmInformation),
        ctxBuf, bufSize, sizeof(PsNvmCemmConfigEmmInformation));

    memset(pCemmEmmCfg->gutiContents, 0xFF, 10);
    pCemmEmmCfg->epsUpdateStatus = USIM_EUS_NOT_UPDATED;
    pCemmEmmCfg->lastRegisteredTai.tac = 0xFFFE;
    pCemmEmmCfg->asmeKsi = NO_KEY_IS_AVAILABLE;
    pCemmEmmCfg->algorithmId = 0;
    memset(pCemmEmmCfg->ulNasCount.data, 0x00, NAS_COUNT_SIZE);
    memset(pCemmEmmCfg->dlNasCount.data, 0x00, NAS_COUNT_SIZE);
    memset(pCemmEmmCfg->dcnId, 0x00, sizeof(DedicatedCoreNetworkId)*32);
    pCemmEmmCfg->imsi.length = 0;

    return;
}

/******************************************************************************
 * PsNvmAdjustEmmInformationVerFunc
 * Description: CEMM EMM CONFIG NVM file, OLD version file -> NEW version file
 * input: UINT8 oldVer; // old version
 *        void *oldCtx; // old NVM context, input value
 *        UINT16 oldCtxSize; // size of "oldCtx", input value
 *        void *curCtx; // cur version NVM context, out put value
 *        UINT16 curCtxSize; // size of "curCtx"
 * output: void
 * Comment:
******************************************************************************/
static BOOL PsNvmAdjustEmmInformationVerFunc(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize)
{
    //-TBD
    return FALSE;
}

/******************************************************************************
 * PsNvmAdjustUeInformationVerFunc
 * Description: CEMM EMM CONFIG NVM file, OLD version file -> NEW version file
 * input: UINT8 oldVer; // old version
 *        void *oldCtx; // old NVM context, input value
 *        UINT16 oldCtxSize; // size of "oldCtx", input value
 *        void *curCtx; // cur version NVM context, out put value
 *        UINT16 curCtxSize; // size of "curCtx"
 * output: void
 * Comment:
******************************************************************************/
static BOOL PsNvmAdjustUeInformationVerFunc(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize)
{
    PsNvmCemmConfigUeInformation_v00    *pCemmUeInfoNvmCfg_v00 = PNULL;
    PsNvmCemmConfigUeInformation_v01    *pCemmUeInfoNvmCfg_v01 = (PsNvmCemmConfigUeInformation_v01 *)curCtx;
    PsNvmCemmConfigUeInformation_v02    *pCemmUeInfoNvmCfg_v02 = (PsNvmCemmConfigUeInformation_v02 *)curCtx;
    PsNvmCemmConfigUeInformation        *pCurCemmUeInfoNvmCfg  = (PsNvmCemmConfigUeInformation *)curCtx;

    OsaCheck(oldCtx != PNULL && curCtx != PNULL, oldVer, oldCtx, curCtx);

    if (oldVer == 0)
    {
        if (CUR_CEMM_UE_INFORMATION_CFG_VER == 0x03)
        {
            pCemmUeInfoNvmCfg_v00 = (PsNvmCemmConfigUeInformation_v00 *)oldCtx;

            GosDebugBegin(sizeof(PsNvmCemmConfigUeInformation_v00) == oldCtxSize, sizeof(PsNvmCemmConfigUeInformation_v00), oldCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            GosDebugBegin(sizeof(PsNvmCemmConfigUeInformation) == curCtxSize, sizeof(PsNvmCemmConfigUeInformation), curCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            memset(pCurCemmUeInfoNvmCfg, 0x00, sizeof(PsNvmCemmConfigUeInformation));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmUeNwCapability,
                   &pCemmUeInfoNvmCfg_v00->cemmUeNwCapability,
                   sizeof(PsNvmCemmConfigUeNwCapability));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmPsmEdrx,
                   &pCemmUeInfoNvmCfg_v00->cemmPsmEdrx,
                   sizeof(PsNvmCemmPsmEdrx));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmCommInfo,
                   &pCemmUeInfoNvmCfg_v00->cemmCommInfo,
                   sizeof(PsNvmCemmCommInfo_v00));

            /* V1 add new flag in cemmCommInfo: enableNonIpNoSms,uint8Default0Rsvd,uint8Default1Rsvd,
               uint16Default0Rsvd,uint16Default1Rsvd,uint32Default0Rsvd,uint32Default1Rsvd
            */
            pCurCemmUeInfoNvmCfg->cemmCommInfo.enableNonIpNoSms = FALSE;

            /* V2 add emmEventInfo,uint8Default2Rsvd in cemmCommInfo */
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numAttachFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numAttachSucc = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numAuthFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numDetach = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numSrFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numSrSucc = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numTauFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numTauSucc = 0;

            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default1Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default2Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint16Default1Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default1Rsvd = 0;

            /* V3 add userSetBarValue,minT3324Value,minT3412Value,minEdrxValue,uint8Default3Rsvd,uint8Default4Rsvd,
            uint8Default5Rsvd,uint8Default6Rsvd,uint8Default7Rsvd,uint16Default0Rsvd,uint16Default2Rsvd,uint32Default2Rsvd */
            pCurCemmUeInfoNvmCfg->cemmCommInfo.userSetBarValue = 120;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.minT3324Value = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.minT3412Value = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.minEdrxValue = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default3Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default4Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default5Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default6Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default7Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint16Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint16Default2Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default2Rsvd = 0;

            return TRUE;
        }
        else if (CUR_CEMM_UE_INFORMATION_CFG_VER == 0x02)
        {
            pCemmUeInfoNvmCfg_v00 = (PsNvmCemmConfigUeInformation_v00 *)oldCtx;

            GosDebugBegin(sizeof(PsNvmCemmConfigUeInformation_v00) == oldCtxSize, sizeof(PsNvmCemmConfigUeInformation_v00), oldCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            GosDebugBegin(sizeof(PsNvmCemmConfigUeInformation) == curCtxSize, sizeof(PsNvmCemmConfigUeInformation), curCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            memset(pCurCemmUeInfoNvmCfg, 0x00, sizeof(PsNvmCemmConfigUeInformation));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmUeNwCapability,
                   &pCemmUeInfoNvmCfg_v00->cemmUeNwCapability,
                   sizeof(PsNvmCemmConfigUeNwCapability));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmPsmEdrx,
                   &pCemmUeInfoNvmCfg_v00->cemmPsmEdrx,
                   sizeof(PsNvmCemmPsmEdrx));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmCommInfo,
                   &pCemmUeInfoNvmCfg_v00->cemmCommInfo,
                   sizeof(PsNvmCemmCommInfo_v00));

            /* V1 add new flag in cemmCommInfo: enableNonIpNoSms,uint8Default0Rsvd,uint8Default1Rsvd,
               uint16Default0Rsvd,uint16Default1Rsvd,uint32Default0Rsvd,uint32Default1Rsvd
            */
            pCurCemmUeInfoNvmCfg->cemmCommInfo.enableNonIpNoSms = FALSE;

            /* V2 add emmEventInfo,uint8Default2Rsvd in cemmCommInfo */
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numAttachFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numAttachSucc = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numAuthFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numDetach = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numSrFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numSrSucc = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numTauFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numTauSucc = 0;

            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default1Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default2Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint16Default1Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default1Rsvd = 0;

            return TRUE;
        }
        if (CUR_CEMM_UE_INFORMATION_CFG_VER == 0x01)
        {
            pCemmUeInfoNvmCfg_v00 = (PsNvmCemmConfigUeInformation_v00 *)oldCtx;

            GosDebugBegin(sizeof(PsNvmCemmConfigUeInformation_v00) == oldCtxSize, sizeof(PsNvmCemmConfigUeInformation_v00), oldCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            GosDebugBegin(sizeof(PsNvmCemmConfigUeInformation) == curCtxSize, sizeof(PsNvmCemmConfigUeInformation), curCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            memset(pCurCemmUeInfoNvmCfg, 0x00, sizeof(PsNvmCemmConfigUeInformation));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmUeNwCapability,
                   &pCemmUeInfoNvmCfg_v00->cemmUeNwCapability,
                   sizeof(PsNvmCemmConfigUeNwCapability));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmPsmEdrx,
                   &pCemmUeInfoNvmCfg_v00->cemmPsmEdrx,
                   sizeof(PsNvmCemmPsmEdrx));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmCommInfo,
                   &pCemmUeInfoNvmCfg_v00->cemmCommInfo,
                   sizeof(PsNvmCemmCommInfo_v00));

            /* V1 add new flag in cemmCommInfo: enableNonIpNoSms,uint8Default0Rsvd,uint8Default1Rsvd,
               uint16Default0Rsvd,uint16Default1Rsvd,uint32Default0Rsvd,uint32Default1Rsvd
            */
            pCurCemmUeInfoNvmCfg->cemmCommInfo.enableNonIpNoSms = FALSE;

            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default1Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint16Default1Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default1Rsvd = 0;

            return TRUE;
        }
        else
        {
            /* Nothing */
        }
    }
    else if (oldVer == 1)
    {
        if (CUR_CEMM_UE_INFORMATION_CFG_VER == 0x03)
        {
            pCemmUeInfoNvmCfg_v01 = (PsNvmCemmConfigUeInformation_v01 *)oldCtx;

            GosDebugBegin(sizeof(PsNvmCemmConfigUeInformation_v01) == oldCtxSize, sizeof(PsNvmCemmConfigUeInformation_v01), oldCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            GosDebugBegin(sizeof(PsNvmCemmConfigUeInformation) == curCtxSize, sizeof(PsNvmCemmConfigUeInformation), curCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            memset(pCurCemmUeInfoNvmCfg, 0x00, sizeof(PsNvmCemmConfigUeInformation));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmUeNwCapability,
                   &pCemmUeInfoNvmCfg_v01->cemmUeNwCapability,
                   sizeof(PsNvmCemmConfigUeNwCapability));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmPsmEdrx,
                   &pCemmUeInfoNvmCfg_v01->cemmPsmEdrx,
                   sizeof(PsNvmCemmPsmEdrx));

            pCurCemmUeInfoNvmCfg->cemmCommInfo.smsOnly = pCemmUeInfoNvmCfg_v01->cemmCommInfo.smsOnly;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.tauForSmsControl = pCemmUeInfoNvmCfg_v01->cemmCommInfo.tauForSmsControl;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.attachWithoutPdn = pCemmUeInfoNvmCfg_v01->cemmCommInfo.attachWithoutPdn;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.userSetT3324Value = pCemmUeInfoNvmCfg_v01->cemmCommInfo.userSetT3324Value;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.ciotOptimization = pCemmUeInfoNvmCfg_v01->cemmCommInfo.ciotOptimization;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.preferedCiotOptimization = pCemmUeInfoNvmCfg_v01->cemmCommInfo.preferedCiotOptimization;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.enableNonIpNoSms = pCemmUeInfoNvmCfg_v01->cemmCommInfo.enableNonIpNoSms;

            /* V2 add emmEventInfo,uint8Default2Rsvd in cemmCommInfo */
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numAttachFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numAttachSucc = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numAuthFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numDetach = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numSrFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numSrSucc = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numTauFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numTauSucc = 0;

            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default1Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default2Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint16Default1Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default1Rsvd = 0;

            /* V3 add userSetBarValue,minT3324Value,minT3412Value,minEdrxValue,uint8Default3Rsvd,uint8Default4Rsvd,
            uint8Default5Rsvd,uint8Default6Rsvd,uint8Default7Rsvd,uint16Default0Rsvd,uint16Default2Rsvd,uint32Default2Rsvd */
            pCurCemmUeInfoNvmCfg->cemmCommInfo.userSetBarValue = 120;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.minT3324Value = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.minT3412Value = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.minEdrxValue = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default3Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default4Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default5Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default6Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default7Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint16Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint16Default2Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default2Rsvd = 0;

            return TRUE;
        }
        else if (CUR_CEMM_UE_INFORMATION_CFG_VER == 0x02)
        {
            pCemmUeInfoNvmCfg_v01 = (PsNvmCemmConfigUeInformation_v01 *)oldCtx;

            GosDebugBegin(sizeof(PsNvmCemmConfigUeInformation_v01) == oldCtxSize, sizeof(PsNvmCemmConfigUeInformation_v01), oldCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            GosDebugBegin(sizeof(PsNvmCemmConfigUeInformation) == curCtxSize, sizeof(PsNvmCemmConfigUeInformation), curCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            memset(pCurCemmUeInfoNvmCfg, 0x00, sizeof(PsNvmCemmConfigUeInformation));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmUeNwCapability,
                   &pCemmUeInfoNvmCfg_v01->cemmUeNwCapability,
                   sizeof(PsNvmCemmConfigUeNwCapability));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmPsmEdrx,
                   &pCemmUeInfoNvmCfg_v01->cemmPsmEdrx,
                   sizeof(PsNvmCemmPsmEdrx));

            pCurCemmUeInfoNvmCfg->cemmCommInfo.smsOnly = pCemmUeInfoNvmCfg_v01->cemmCommInfo.smsOnly;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.tauForSmsControl = pCemmUeInfoNvmCfg_v01->cemmCommInfo.tauForSmsControl;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.attachWithoutPdn = pCemmUeInfoNvmCfg_v01->cemmCommInfo.attachWithoutPdn;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.userSetT3324Value = pCemmUeInfoNvmCfg_v01->cemmCommInfo.userSetT3324Value;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.ciotOptimization = pCemmUeInfoNvmCfg_v01->cemmCommInfo.ciotOptimization;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.preferedCiotOptimization = pCemmUeInfoNvmCfg_v01->cemmCommInfo.preferedCiotOptimization;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.enableNonIpNoSms = pCemmUeInfoNvmCfg_v01->cemmCommInfo.enableNonIpNoSms;

            /* V2 add emmEventInfo,uint8Default2Rsvd in cemmCommInfo */
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numAttachFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numAttachSucc = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numAuthFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numDetach = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numSrFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numSrSucc = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numTauFail = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo.numTauSucc = 0;

            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default1Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default2Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint16Default1Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default1Rsvd = 0;

            return TRUE;
        }
        else
        {
            /* Nothing */
        }
    }
    else if (oldVer == 2)
    {
        if (CUR_CEMM_UE_INFORMATION_CFG_VER == 0x03)
        {
            ECOMM_TRACE(UNILOG_PS, PsNvmAdjustUeInformationVerFunc_00, P_WARNING, 0, "adjustVerFun");
            pCemmUeInfoNvmCfg_v02 = (PsNvmCemmConfigUeInformation_v02 *)oldCtx;

            GosDebugBegin(sizeof(PsNvmCemmConfigUeInformation_v02) == oldCtxSize, sizeof(PsNvmCemmConfigUeInformation_v02), oldCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            GosDebugBegin(sizeof(PsNvmCemmConfigUeInformation) == curCtxSize, sizeof(PsNvmCemmConfigUeInformation), curCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            memset(pCurCemmUeInfoNvmCfg, 0x00, sizeof(PsNvmCemmConfigUeInformation));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmUeNwCapability,
                   &pCemmUeInfoNvmCfg_v02->cemmUeNwCapability,
                   sizeof(PsNvmCemmConfigUeNwCapability));

            memcpy(&pCurCemmUeInfoNvmCfg->cemmPsmEdrx,
                   &pCemmUeInfoNvmCfg_v02->cemmPsmEdrx,
                   sizeof(PsNvmCemmPsmEdrx));

            pCurCemmUeInfoNvmCfg->cemmCommInfo.smsOnly = pCemmUeInfoNvmCfg_v02->cemmCommInfo.smsOnly;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.tauForSmsControl = pCemmUeInfoNvmCfg_v02->cemmCommInfo.tauForSmsControl;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.attachWithoutPdn = pCemmUeInfoNvmCfg_v02->cemmCommInfo.attachWithoutPdn;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.userSetT3324Value = pCemmUeInfoNvmCfg_v02->cemmCommInfo.userSetT3324Value;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.ciotOptimization = pCemmUeInfoNvmCfg_v02->cemmCommInfo.ciotOptimization;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.preferedCiotOptimization = pCemmUeInfoNvmCfg_v02->cemmCommInfo.preferedCiotOptimization;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.emmEventInfo = pCemmUeInfoNvmCfg_v02->cemmCommInfo.emmEventInfo;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.enableNonIpNoSms = pCemmUeInfoNvmCfg_v02->cemmCommInfo.enableNonIpNoSms;

            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default1Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default2Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint16Default1Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default1Rsvd = 0;

            /* V3 add userSetBarValue,minT3324Value,minT3412Value,minEdrxValue,uint8Default3Rsvd,uint8Default4Rsvd,
            uint8Default5Rsvd,uint8Default6Rsvd,uint8Default7Rsvd,uint16Default0Rsvd,uint16Default2Rsvd,uint32Default2Rsvd */
            pCurCemmUeInfoNvmCfg->cemmCommInfo.userSetBarValue = 120;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.minT3324Value = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.minT3412Value = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.minEdrxValue = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default3Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default4Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default5Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default6Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint8Default7Rsvd = 1;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint16Default0Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint16Default2Rsvd = 0;
            pCurCemmUeInfoNvmCfg->cemmCommInfo.uint32Default2Rsvd = 0;

            return TRUE;
        }
        else
        {
            /* Nothing */
        }
    }

    GosDebugBegin(FALSE, oldVer, oldCtxSize, 0);
    GosDebugEnd();

    return FALSE;
}


/******************************************************************************
 * PsNvmSetCemmPlmnConfigDefaultFunc
 * Description: CEMM PLMN CONFIG NVM file, default value
 * input: void *ctxBuf; //output buffer
 *        UINT16 bufSize; // buffer size
 * output: void
 * Comment:
******************************************************************************/
static void PsNvmSetCemmPlmnConfigDefaultFunc(void *ctxBuf, UINT16 bufSize)
{
    CePlmnNvmConfig *pCemmPlmnCfg = (CePlmnNvmConfig *)ctxBuf;

    GosCheck(ctxBuf != PNULL && bufSize == sizeof(CePlmnNvmConfig), ctxBuf, bufSize, sizeof(CePlmnNvmConfig));

    memset(pCemmPlmnCfg, 0x00, sizeof(CePlmnNvmConfig));
    pCemmPlmnCfg->plmnSelectType = CEMM_AUTO_PLMN_REG;
    pCemmPlmnCfg->plmnSearchPowerLevel = PLMN_SEARCH_POWER_LEVEL_STRICT_CARE;
    pCemmPlmnCfg->bEnableHPPlmnSearch  = TRUE;

    pCemmPlmnCfg->phyCellId = PS_INVALID_PHY_CELL_ID;

    memset(pCemmPlmnCfg->eplmnList, 0x00, sizeof(Plmn)*CEMM_PLMN_LIST_MAX_NUM);
    memset(pCemmPlmnCfg->fPlmnList, 0x00, sizeof(Plmn)*CEMM_PLMN_LIST_MAX_NUM);

    /*
     * set band 5 & 8 as default
    */
    pCemmPlmnCfg->band[0] = 5;
    pCemmPlmnCfg->band[1] = 8;
    pCemmPlmnCfg->band[2] = 3;
#if defined (WIN32)
    pCemmPlmnCfg->band[4] = 66;
    pCemmPlmnCfg->band[5] = 71;
    pCemmPlmnCfg->band[6] = 4;
#endif
    pCemmPlmnCfg->numPlmnOos = 0;

    pCemmPlmnCfg->uint16Default0Rsvd = 0;
    pCemmPlmnCfg->uint16Default1Rsvd = 1;
    pCemmPlmnCfg->uint32Default0Rsvd = 0;
    pCemmPlmnCfg->uint32Default1Rsvd = 1;

    return;
}

/******************************************************************************
 * PsNvmAdjustCemmPlmnConfigVerFunc
 * Description: CEMM PLMN CONFIG NVM file, OLD version file -> NEW version file
 * input: UINT8 oldVer; // old version
 *        void *oldCtx; // old NVM context, input value
 *        UINT16 oldCtxSize; // size of "oldCtx", input value
 *        void *curCtx; // cur version NVM context, out put value
 *        UINT16 curCtxSize; // size of "curCtx"
 * output: void
 * Comment:
******************************************************************************/
static BOOL PsNvmAdjustCemmPlmnConfigVerFunc(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize)
{
    CePlmnNvmConfig_v00    *pCePlmnNvmCfg_v00 = PNULL;
    CePlmnNvmConfig_v01    *pCePlmnNvmCfg_v01 = PNULL;
    CePlmnNvmConfig        *pCurCePlmnNvmCfg = (CePlmnNvmConfig *)curCtx;

    OsaCheck(oldCtx != PNULL && curCtx != PNULL, oldVer, oldCtx, curCtx);

    if (oldVer == 0)
    {
        if (CUR_CEMM_PLMN_NVM_CFG_VER == 0x02)
        {
            pCePlmnNvmCfg_v01 = (CePlmnNvmConfig_v01 *)oldCtx;

            GosDebugBegin(sizeof(CePlmnNvmConfig_v01) == oldCtxSize, sizeof(CePlmnNvmConfig_v01), oldCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            GosDebugBegin(sizeof(CePlmnNvmConfig) == curCtxSize, sizeof(CePlmnNvmConfig), curCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            memset(pCurCePlmnNvmCfg, 0x00, sizeof(CePlmnNvmConfig));

            pCurCePlmnNvmCfg->plmnSelectType = pCePlmnNvmCfg_v01->plmnSelectType;
            pCurCePlmnNvmCfg->plmnSearchPowerLevel = pCePlmnNvmCfg_v01->plmnSearchPowerLevel;
            pCurCePlmnNvmCfg->bCellLock = pCePlmnNvmCfg_v01->bCellLock;
            pCurCePlmnNvmCfg->phyCellId = pCePlmnNvmCfg_v01->phyCellId;
            pCurCePlmnNvmCfg->manualPlmn.mcc = pCePlmnNvmCfg_v01->manualPlmn.mcc;
            pCurCePlmnNvmCfg->manualPlmn.mncWithAddInfo = pCePlmnNvmCfg_v01->manualPlmn.mncWithAddInfo;
            pCurCePlmnNvmCfg->rplmn.mcc = pCePlmnNvmCfg_v01->rplmn.mcc;
            pCurCePlmnNvmCfg->rplmn.mncWithAddInfo = pCePlmnNvmCfg_v01->rplmn.mncWithAddInfo;

            memcpy(&pCurCePlmnNvmCfg->preImsi, &pCePlmnNvmCfg_v01->preImsi, sizeof(Imsi));

            memcpy(&pCurCePlmnNvmCfg->eplmnList[0],
                   &pCePlmnNvmCfg_v01->eplmnList[0],
                   sizeof(Plmn) * CEMM_PLMN_LIST_MAX_NUM);

            memcpy(&pCurCePlmnNvmCfg->fPlmnList[0],
                   &pCePlmnNvmCfg_v01->fPlmnList[0],
                   sizeof(Plmn) * CEMM_PLMN_LIST_MAX_NUM);

            memcpy(&pCurCePlmnNvmCfg->band[0],
                   &pCePlmnNvmCfg_v01->band[0],
                   sizeof(UINT8) * 16); /* SUPPORT_MAX_BAND_NUM is defined as 16 in v00 */

            pCurCePlmnNvmCfg->lockedFreq = pCePlmnNvmCfg_v01->lockedFreq;

            memcpy(&pCurCePlmnNvmCfg->preFreq[0],
                   &pCePlmnNvmCfg_v01->preFreq[0],
                   sizeof(UINT32) * SUPPORT_MAX_FREQ_NUM);

            /* V1 add new flag: bEnableHPPlmnSearch,uint8Default0Rsvd,uint8Default1Rsvd,
               uint16Default0Rsvd,uint16Default1Rsvd,uint32Default0Rsvd,uint32Default1Rsvd
               V1 del old flag: rsvd1,rsvd2
            */
            pCurCePlmnNvmCfg->bEnableHPPlmnSearch = pCePlmnNvmCfg_v01->bEnableHPPlmnSearch;

            pCurCePlmnNvmCfg->numPlmnOos = 0;

            pCurCePlmnNvmCfg->uint16Default0Rsvd = 0;
            pCurCePlmnNvmCfg->uint16Default1Rsvd = 1;
            pCurCePlmnNvmCfg->uint32Default0Rsvd = 0;
            pCurCePlmnNvmCfg->uint32Default1Rsvd = 1;

            //check some basic value
            if (pCurCePlmnNvmCfg->band[0] == 0)
            {
                GosDebugBegin(FALSE, pCurCePlmnNvmCfg->band[0], pCurCePlmnNvmCfg->band[1], pCurCePlmnNvmCfg->band[2]);
                GosDebugEnd();

                return FALSE;
            }
        }
        else if (CUR_CEMM_PLMN_NVM_CFG_VER == 0x01)
        {
            pCePlmnNvmCfg_v00 = (CePlmnNvmConfig_v00 *)oldCtx;

            GosDebugBegin(sizeof(CePlmnNvmConfig_v00) == oldCtxSize, sizeof(CePlmnNvmConfig_v00), oldCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            GosDebugBegin(sizeof(CePlmnNvmConfig) == curCtxSize, sizeof(CePlmnNvmConfig), curCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            memset(pCurCePlmnNvmCfg, 0x00, sizeof(CePlmnNvmConfig));

            pCurCePlmnNvmCfg->plmnSelectType = pCePlmnNvmCfg_v00->plmnSelectType;
            pCurCePlmnNvmCfg->plmnSearchPowerLevel = pCePlmnNvmCfg_v00->plmnSearchPowerLevel;
            pCurCePlmnNvmCfg->bCellLock = pCePlmnNvmCfg_v00->bCellLock;
            pCurCePlmnNvmCfg->phyCellId = pCePlmnNvmCfg_v00->phyCellId;
            pCurCePlmnNvmCfg->manualPlmn.mcc = pCePlmnNvmCfg_v00->manualPlmn.mcc;
            pCurCePlmnNvmCfg->manualPlmn.mncWithAddInfo = pCePlmnNvmCfg_v00->manualPlmn.mncWithAddInfo;
            pCurCePlmnNvmCfg->rplmn.mcc = pCePlmnNvmCfg_v00->rplmn.mcc;
            pCurCePlmnNvmCfg->rplmn.mncWithAddInfo = pCePlmnNvmCfg_v00->rplmn.mncWithAddInfo;

            memcpy(&pCurCePlmnNvmCfg->preImsi, &pCePlmnNvmCfg_v00->preImsi, sizeof(Imsi));

            memcpy(&pCurCePlmnNvmCfg->eplmnList[0],
                   &pCePlmnNvmCfg_v00->eplmnList[0],
                   sizeof(Plmn) * CEMM_PLMN_LIST_MAX_NUM);

            memcpy(&pCurCePlmnNvmCfg->fPlmnList[0],
                   &pCePlmnNvmCfg_v00->fPlmnList[0],
                   sizeof(Plmn) * CEMM_PLMN_LIST_MAX_NUM);

            memcpy(&pCurCePlmnNvmCfg->band[0],
                   &pCePlmnNvmCfg_v00->band[0],
                   sizeof(UINT8) * 16); /* SUPPORT_MAX_BAND_NUM is defined as 16 in v00 */

            pCurCePlmnNvmCfg->lockedFreq = pCePlmnNvmCfg_v00->lockedFreq;

            memcpy(&pCurCePlmnNvmCfg->preFreq[0],
                   &pCePlmnNvmCfg_v00->preFreq[0],
                   sizeof(UINT32) * SUPPORT_MAX_FREQ_NUM);

            /* V1 add new flag: bEnableHPPlmnSearch,uint8Default0Rsvd,uint8Default1Rsvd,
               uint16Default0Rsvd,uint16Default1Rsvd,uint32Default0Rsvd,uint32Default1Rsvd
               V1 del old flag: rsvd1,rsvd2
            */
            pCurCePlmnNvmCfg->bEnableHPPlmnSearch = TRUE;

            pCurCePlmnNvmCfg->numPlmnOos = 0;

            pCurCePlmnNvmCfg->uint16Default0Rsvd = 0;
            pCurCePlmnNvmCfg->uint16Default1Rsvd = 1;
            pCurCePlmnNvmCfg->uint32Default0Rsvd = 0;
            pCurCePlmnNvmCfg->uint32Default1Rsvd = 1;

            //check some basic value
            if (pCurCePlmnNvmCfg->band[0] == 0)
            {
                GosDebugBegin(FALSE, pCurCePlmnNvmCfg->band[0], pCurCePlmnNvmCfg->band[1], pCurCePlmnNvmCfg->band[2]);
                GosDebugEnd();

                return FALSE;
            }
        }

        return TRUE;
    }
    else if (oldVer == 1)
    {
        if (CUR_CEMM_PLMN_NVM_CFG_VER == 0x02)
        {
            pCePlmnNvmCfg_v01 = (CePlmnNvmConfig_v01 *)oldCtx;

            GosDebugBegin(sizeof(CePlmnNvmConfig_v01) == oldCtxSize, sizeof(CePlmnNvmConfig_v01), oldCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            GosDebugBegin(sizeof(CePlmnNvmConfig) == curCtxSize, sizeof(CePlmnNvmConfig), curCtxSize, 0);
            return FALSE;
            GosDebugEnd();

            memset(pCurCePlmnNvmCfg, 0x00, sizeof(CePlmnNvmConfig));

            pCurCePlmnNvmCfg->plmnSelectType = pCePlmnNvmCfg_v01->plmnSelectType;
            pCurCePlmnNvmCfg->plmnSearchPowerLevel = pCePlmnNvmCfg_v01->plmnSearchPowerLevel;
            pCurCePlmnNvmCfg->bCellLock = pCePlmnNvmCfg_v01->bCellLock;
            pCurCePlmnNvmCfg->phyCellId = pCePlmnNvmCfg_v01->phyCellId;
            pCurCePlmnNvmCfg->manualPlmn.mcc = pCePlmnNvmCfg_v01->manualPlmn.mcc;
            pCurCePlmnNvmCfg->manualPlmn.mncWithAddInfo = pCePlmnNvmCfg_v01->manualPlmn.mncWithAddInfo;
            pCurCePlmnNvmCfg->rplmn.mcc = pCePlmnNvmCfg_v01->rplmn.mcc;
            pCurCePlmnNvmCfg->rplmn.mncWithAddInfo = pCePlmnNvmCfg_v01->rplmn.mncWithAddInfo;

            memcpy(&pCurCePlmnNvmCfg->preImsi, &pCePlmnNvmCfg_v01->preImsi, sizeof(Imsi));

            memcpy(&pCurCePlmnNvmCfg->eplmnList[0],
                   &pCePlmnNvmCfg_v01->eplmnList[0],
                   sizeof(Plmn) * CEMM_PLMN_LIST_MAX_NUM);

            memcpy(&pCurCePlmnNvmCfg->fPlmnList[0],
                   &pCePlmnNvmCfg_v01->fPlmnList[0],
                   sizeof(Plmn) * CEMM_PLMN_LIST_MAX_NUM);

            memcpy(&pCurCePlmnNvmCfg->band[0],
                   &pCePlmnNvmCfg_v01->band[0],
                   sizeof(UINT8) * 16); /* SUPPORT_MAX_BAND_NUM is defined as 16 in v00 */

            pCurCePlmnNvmCfg->lockedFreq = pCePlmnNvmCfg_v01->lockedFreq;

            memcpy(&pCurCePlmnNvmCfg->preFreq[0],
                   &pCePlmnNvmCfg_v01->preFreq[0],
                   sizeof(UINT32) * SUPPORT_MAX_FREQ_NUM);

            /* V1 add new flag: bEnableHPPlmnSearch,uint8Default0Rsvd,uint8Default1Rsvd,
               uint16Default0Rsvd,uint16Default1Rsvd,uint32Default0Rsvd,uint32Default1Rsvd
               V1 del old flag: rsvd1,rsvd2
            */
            pCurCePlmnNvmCfg->bEnableHPPlmnSearch = pCePlmnNvmCfg_v01->bEnableHPPlmnSearch;

            pCurCePlmnNvmCfg->numPlmnOos = 0;

            pCurCePlmnNvmCfg->uint16Default0Rsvd = 0;
            pCurCePlmnNvmCfg->uint16Default1Rsvd = 1;
            pCurCePlmnNvmCfg->uint32Default0Rsvd = 0;
            pCurCePlmnNvmCfg->uint32Default1Rsvd = 1;

            //check some basic value
            if (pCurCePlmnNvmCfg->band[0] == 0)
            {
                GosDebugBegin(FALSE, pCurCePlmnNvmCfg->band[0], pCurCePlmnNvmCfg->band[1], pCurCePlmnNvmCfg->band[2]);
                GosDebugEnd();

                return FALSE;
            }
        }

        return TRUE;
    }

    GosDebugBegin(FALSE, oldVer, oldCtxSize, 0);
    GosDebugEnd();

    return FALSE;
}

/******************************************************************************
 * PsNvmSetCemmEsmConfigDefaultFunc
 * Description: CEMM ESM CONFIG NVM file, default value
 * input: void *ctxBuf; //output buffer
 *        UINT16 bufSize; // buffer size
 * output: void
 * Comment:
******************************************************************************/
static void PsNvmSetCemmEsmConfigDefaultFunc(void *ctxBuf, UINT16 bufSize)
{
    CemmEsmNvmConfig *pCemmEsmCfg = (CemmEsmNvmConfig *)ctxBuf;
    GosCheck(ctxBuf != PNULL && bufSize == sizeof(CemmEsmNvmConfig), ctxBuf, bufSize, sizeof(CemmEsmNvmConfig));

    memset(pCemmEsmCfg, 0x00, sizeof(CemmEsmNvmConfig));
    pCemmEsmCfg->eitfPresent    = TRUE;
    pCemmEsmCfg->eitf           = 1;
    pCemmEsmCfg->epcoFlag       = TRUE;
    pCemmEsmCfg->ipv4DnsToBeRead = TRUE;
    pCemmEsmCfg->ipv6DnsToBeRead = TRUE;

    pCemmEsmCfg->pdnType = PDN_IP_V4V6;
    pCemmEsmCfg->pdnPcoInfo.dnsIpv6Req      = 1;
    pCemmEsmCfg->pdnPcoInfo.dnsIpv4AddrReq  = 1;
    pCemmEsmCfg->pdnPcoInfo.ipAddrViaNas    = 1;
    pCemmEsmCfg->pdnPcoInfo.ipv4MtuReq      = 1;

    return;
}

/******************************************************************************
 * PsNvmAdjustCemmEsmConfigVerFunc
 * Description: CEMM ESM CONFIG NVM file, OLD version file -> NEW version file
 * input: UINT8 oldVer; // old version
 *        void *oldCtx; // old NVM context, input value
 *        UINT16 oldCtxSize; // size of "oldCtx", input value
 *        void *curCtx; // cur version NVM context, out put value
 *        UINT16 curCtxSize; // size of "curCtx"
 * output: void
 * Comment:
******************************************************************************/
static BOOL PsNvmAdjustCemmEsmConfigVerFunc(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize)
{
    // - TBD
    return FALSE;
}

/******************************************************************************
 * PsNvmSetUiccCtrlConfigDefaultFunc
 * Description: UICC CTRL CONFIG NVM file, default value
 * input: void *ctxBuf; //output buffer
 *        UINT16 bufSize; // buffer size
 * output: void
 * Comment:
******************************************************************************/
static void PsNvmSetUiccCtrlConfigDefaultFunc(void *ctxBuf, UINT16 bufSize)
{
    UiccCtrlNvmConfig *pUiccCtrlCfg = (UiccCtrlNvmConfig *)ctxBuf;
     UINT8 defUsatProfile[MAX_TP_LEN_NVM] = {0xB3, 0x1F, 0xE8, 0xC2, 0x11, 0x8C, 0x00, 0x87, 0x84, 0x00, 0x00,
                                             0x1F, 0xE2, 0x60, 0x00, 0x00, 0x43, 0xC0, 0x00, 0x00, 0x00, 0x00,
                                             0x40, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x80};

    GosCheck(ctxBuf != PNULL && bufSize == sizeof(UiccCtrlNvmConfig), ctxBuf, bufSize, sizeof(UiccCtrlNvmConfig));

    memset(pUiccCtrlCfg, 0x00, sizeof(UiccCtrlNvmConfig));
    pUiccCtrlCfg->tpLen = 32;
    memcpy(pUiccCtrlCfg->tpData, defUsatProfile, pUiccCtrlCfg->tpLen);
    pUiccCtrlCfg->swcEnable = FALSE;
    pUiccCtrlCfg->bSPDEnbale = FALSE;//default value: disabled
    //pUiccCtrlCfg->bSPDEnbale = TRUE;//default value: enabled
    pUiccCtrlCfg->uint8Default1Rsvd1 = 1;
    pUiccCtrlCfg->uint8Default1Rsvd2 = 1;

    return;
}

/******************************************************************************
 * PsNvmAdjustUiccCtrlConfigVerFunc
 * Description: UICC CTRL CONFIG NVM file, OLD version file -> NEW version file
 * input: UINT8 oldVer; // old version
 *        void *oldCtx; // old NVM context, input value
 *        UINT16 oldCtxSize; // size of "oldCtx", input value
 *        void *curCtx; // cur version NVM context, out put value
 *        UINT16 curCtxSize; // size of "curCtx"
 * output: void
 * Comment:
******************************************************************************/
static BOOL PsNvmAdjustUiccCtrlConfigVerFunc(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize)
{
    UiccCtrlNvmConfig_v00    *pUiccCtrlNvmCfg_v00 = PNULL;
    UiccCtrlNvmConfig        *pCurUiccCtrlNvmCfg = (UiccCtrlNvmConfig *)curCtx;

    OsaCheck(oldCtx != PNULL && curCtx != PNULL, oldVer, oldCtx, curCtx);

    if (oldVer == 0)
    {
        pUiccCtrlNvmCfg_v00 = (UiccCtrlNvmConfig_v00 *)oldCtx;

        GosDebugBegin(sizeof(UiccCtrlNvmConfig_v00) == oldCtxSize, sizeof(UiccCtrlNvmConfig_v00), oldCtxSize, 0);
        return FALSE;
        GosDebugEnd();

        GosDebugBegin(sizeof(UiccCtrlNvmConfig) == curCtxSize, sizeof(UiccCtrlNvmConfig), curCtxSize, 0);
        return FALSE;
        GosDebugEnd();

        memset(pCurUiccCtrlNvmCfg, 0x00, sizeof(UiccCtrlNvmConfig));

        memcpy(pCurUiccCtrlNvmCfg,
               pUiccCtrlNvmCfg_v00,
               sizeof(UiccCtrlNvmConfig_v00));

        //pCurUiccCtrlNvmCfg->bSPDEnbale = TRUE;
        pCurUiccCtrlNvmCfg->uint8Default1Rsvd1 = 1;
        pCurUiccCtrlNvmCfg->uint8Default1Rsvd2 = 1;

        return TRUE;
    }

    GosDebugBegin(FALSE, oldVer, oldCtxSize, 0);
    GosDebugEnd();

    return FALSE;
}


/******************************************************************************
 * PsNvmSetCerrcConfigDefaultFunc
 * Description: Cerrc CONFIG NVM file, default value
 * input: void *ctxBuf; //output buffer
 *        UINT16 bufSize; // buffer size
 * output: void
 * Comment:
******************************************************************************/
static void PsNvmSetCerrcConfigDefaultFunc(void *ctxBuf, UINT16 bufSize)
{
    CerrcNvmConfig *pCerrcCfg = (CerrcNvmConfig *)ctxBuf;
    GosCheck(ctxBuf != PNULL && bufSize == sizeof(CerrcNvmConfig), ctxBuf, bufSize, sizeof(CerrcNvmConfig));

    memset(pCerrcCfg, 0x00, sizeof(CerrcNvmConfig));

#ifdef PS_NB_SUPPORT_HS_REF_FEATURE
    pCerrcCfg->asRelease = AsRelease_13;
    pCerrcCfg->ueCategory = UeCategory_NB1;
    pCerrcCfg->bRaiSupport = FALSE;
#else
    pCerrcCfg->asRelease = AsRelease_13;
    pCerrcCfg->ueCategory = UeCategory_NB1;
    pCerrcCfg->bRaiSupport = FALSE;
#endif
    pCerrcCfg->bMultiCarrierSupport = TRUE;
    pCerrcCfg->bMultiToneSupport = TRUE;
    pCerrcCfg->bEnableCellResel = TRUE;
    pCerrcCfg->bEnableConnReEst = TRUE;
    pCerrcCfg->bDisableNCellMeas = FALSE;

    pCerrcCfg->sSearchDeltaP = 6;
    pCerrcCfg->dataInactivityTimer = 60; //seconds
    pCerrcCfg->uint8Def0Rsvd0 = 0;
    pCerrcCfg->uint8Def0Rsvd1 = 0;
    //pCerrcCfg->tSearchDeltaP = 60; //seconds
    memset(&pCerrcCfg->uePowerClass, 0, sizeof(ConfigedUePowerClassList)); //set uePowerClass->numOfBand to 0

    //4 The followed IEs is added by Version 0x01
    pCerrcCfg->bSupportTwoHarq = TRUE;
    pCerrcCfg->boolDefTrueRsvd0     = TRUE;
    pCerrcCfg->boolDefTrueRsvd1     = TRUE;
    pCerrcCfg->boolDefTrueRsvd2     = TRUE;

    pCerrcCfg->disableSib14Check = FALSE;

    pCerrcCfg->boolDefFalRsvd0 = FALSE;
    pCerrcCfg->boolDefFalRsvd1 = FALSE;
    pCerrcCfg->boolDefFalRsvd2 = FALSE;

    pCerrcCfg->rrcConnEstSuccNum = 0;
    pCerrcCfg->rrcConnEstFailNum = 0;
    pCerrcCfg->uint32Def0Rsvd0 = 0;
    pCerrcCfg->uint32Def0Rsvd1 = 0;

    return;
}

/******************************************************************************
 * PsNvmAdjustCerrcConfigVerFunc
 * Description: CERRC CONFIG NVM file, OLD version file -> NEW version file
 * input: UINT8 oldVer; // old version
 *        void *oldCtx; // old NVM context, input value
 *        UINT16 oldCtxSize; // size of "oldCtx", input value
 *        void *curCtx; // cur version NVM context, out put value
 *        UINT16 curCtxSize; // size of "curCtx"
 * output: void
 * Comment:
******************************************************************************/
static BOOL PsNvmAdjustCerrcConfigVerFunc(UINT8 oldVer, void *oldCtx, UINT16 oldCtxSize, void *curCtx, UINT16 curCtxSize)
{
    CerrcNvmConfig *pCurCerrcNvmCfg = (CerrcNvmConfig *)curCtx;

    OsaCheck(oldCtx != PNULL && curCtx != PNULL, oldVer, oldCtx, curCtx);

    if (0x00 == oldVer)
    {
        CerrcNvmConfig_v00 *pCerrcNvmCfg_v00 = (CerrcNvmConfig_v00 *)oldCtx;
        UINT8 i;

        GosDebugBegin(sizeof(CerrcNvmConfig_v00) == oldCtxSize, sizeof(CerrcNvmConfig_v00), oldCtxSize, 0);
        return FALSE;
        GosDebugEnd();

        GosDebugBegin(sizeof(CerrcNvmConfig) == curCtxSize, sizeof(CerrcNvmConfig), curCtxSize, 0);
        return FALSE;
        GosDebugEnd();

        //4 initilized current CerrcNvmConfig
        memset(pCurCerrcNvmCfg, 0x00, sizeof(CerrcNvmConfig));

        //4 copy old version parameters
        pCurCerrcNvmCfg->asRelease = pCerrcNvmCfg_v00->asRelease;
        pCurCerrcNvmCfg->ueCategory = pCerrcNvmCfg_v00->ueCategory;

        pCurCerrcNvmCfg->bRaiSupport = pCerrcNvmCfg_v00->bRaiSupport;
        pCurCerrcNvmCfg->bMultiCarrierSupport = pCerrcNvmCfg_v00->bMultiCarrierSupport;
        pCurCerrcNvmCfg->bMultiToneSupport = pCerrcNvmCfg_v00->bMultiToneSupport;
        pCurCerrcNvmCfg->bEnableCellResel = pCerrcNvmCfg_v00->bEnableCellResel;
        pCurCerrcNvmCfg->bEnableConnReEst = pCerrcNvmCfg_v00->bEnableConnReEst;
        pCurCerrcNvmCfg->bDisableNCellMeas = pCerrcNvmCfg_v00->bDisableNCellMeas;

        pCurCerrcNvmCfg->sSearchDeltaP = pCerrcNvmCfg_v00->sSearchDeltaP;
        pCurCerrcNvmCfg->dataInactivityTimer = pCerrcNvmCfg_v00->dataInactivityTimer; //seconds
        pCurCerrcNvmCfg->uint8Def0Rsvd0 = 0;
        pCurCerrcNvmCfg->uint8Def0Rsvd1 = 0;

        //extend RRC_PHY_SUPPORT_BAND_NUM from 8 to 32
        pCurCerrcNvmCfg->uePowerClass.numOfBand = MIN(pCerrcNvmCfg_v00->uePowerClass.numOfBand, 8);
        for (i = 0; i < pCurCerrcNvmCfg->uePowerClass.numOfBand; i++)
        {
            pCurCerrcNvmCfg->uePowerClass.bandAndPowerClass[i].freqBandIndicator = pCerrcNvmCfg_v00->uePowerClass.bandAndPowerClass[i].freqBandIndicator;
            pCurCerrcNvmCfg->uePowerClass.bandAndPowerClass[i].uePowerClass = pCerrcNvmCfg_v00->uePowerClass.bandAndPowerClass[i].uePowerClass;
        }

        //4 The followed IEs is added by Version 0x01
        pCurCerrcNvmCfg->bSupportTwoHarq = TRUE;
        pCurCerrcNvmCfg->boolDefTrueRsvd0     = TRUE;
        pCurCerrcNvmCfg->boolDefTrueRsvd1     = TRUE;
        pCurCerrcNvmCfg->boolDefTrueRsvd2     = TRUE;

        pCurCerrcNvmCfg->disableSib14Check = FALSE;

        pCurCerrcNvmCfg->boolDefFalRsvd0 = FALSE;
        pCurCerrcNvmCfg->boolDefFalRsvd1 = FALSE;
        pCurCerrcNvmCfg->boolDefFalRsvd2 = FALSE;

        pCurCerrcNvmCfg->rrcConnEstSuccNum = 0;
        pCurCerrcNvmCfg->rrcConnEstFailNum = 0;
        pCurCerrcNvmCfg->uint32Def0Rsvd0 = 0;
        pCurCerrcNvmCfg->uint32Def0Rsvd1 = 0;

        return TRUE;
    }

    GosDebugBegin(FALSE, oldVer, oldCtxSize, 0);
    GosDebugEnd();

    return FALSE;
}


/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL FUNCTION
 ******************************************************************************
******************************************************************************/







