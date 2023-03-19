/******************************************************************************
 ******************************************************************************
 Copyright:      - 2017- Copyrights of EigenComm Ltd.
 File name:      - cmsnetlight.c
 Description:    - control the net light based on PS network indication:
 History:        - 03/30/2020, Originated by xlhu
 ******************************************************************************
******************************************************************************/
#include "cmidev.h"
#include "cmips.h"
#include "cmsnetlight.h"
#include "mw_config.h"
#include "psdial.h"

/******************************************************************************
 ******************************************************************************
 GOLBAL VARIABLES
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_VARIABLES__ //just for easy to find this position in SS

/******************************************************************************
 ******************************************************************************
  External API/Variables specification
 ******************************************************************************
******************************************************************************/
extern void TIMER_Netlight_PWM(uint8_t mode);

/******************************************************************************
 ******************************************************************************
 static functions
 ******************************************************************************
******************************************************************************/
#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position in SS


/******************************************************************************
 * cmsNetlightProcCmiPsCEREGInd
 * Description: netlight PROC "CmiPsCeregInd"
 * input: CmiPsCeregInd *pPsCeregInd
 * output: void
 * Comment:
******************************************************************************/
static void cmsNetlightProcCmiPsCEREGInd(CmiPsCeregInd *pPsCeregInd)
{
    if (pPsCeregInd->state == CMI_PS_NOT_REG_SEARCHING)
    {
        /*
        * if start plmn search, start the light flicker fast
        * plmn search may occure in plmn searching timer expired after OOS
        */
        TIMER_Netlight_PWM((UINT8)NETLIGHT_FLICKER_FAST);
    }
    else if ((pPsCeregInd->state == CMI_PS_REG_HOME) ||
             (pPsCeregInd->state == CMI_PS_REG_ROAMING))
    {
        /*
        * if registered, start the light slow flicker
        */
        TIMER_Netlight_PWM(NETLIGHT_FLICKER_SLOW);
    }
    else if (pPsCeregInd->state == CMI_PS_NOT_REG)
    {
        /*
        * if OOS, stop the light flicker
        */
        TIMER_Netlight_PWM(NETLIGHT_FLICKER_NONE);
    }

    return;
}

/******************************************************************************
 * cmsNetlightProcCmiPsCSCONInd
 * Description: netlight PROC "CmiPsCeregInd", start SLOW FLICKER if registered with connected
 * input: CmiPsCeregInd *pPsCeregInd
 * output: void
 * Comment:
******************************************************************************/
static void cmsNetlightProcCmiPsCSCONInd(CmiPsConnStatusInd *pPsConnStatusInd)
{
    if (pPsConnStatusInd->connMode == CMI_Ps_CONN_IDLE_MODE)
    {
        /*
        * if enter idle state, stop the light flicker
        */
        TIMER_Netlight_PWM(NETLIGHT_FLICKER_NONE);
    }
    else if ((pPsConnStatusInd->connMode == CMI_Ps_CONNECTED_MODE) &&
             (psDialLwipNetIfIsAct() == TRUE))
    {
        /*
        * if enter connected state and the net is ok, start the light flicker slow
        */
        TIMER_Netlight_PWM(NETLIGHT_FLICKER_SLOW);
    }

    return;
}


/******************************************************************************
 * cmsNetlightProcCmiDevSetCfunCnf
 * Description: Netlight PROC "CmiDevSetCfunCnf", start fast flicker after cfun1
 * input: uint16 rc;
 *     CmiDevSetCfunCnf *pSetCfunCnf;
 * output: void
 * Comment:
******************************************************************************/
static void cmsNetlightProcCmiDevSetCfunCnf(UINT16 rc, CmiDevSetCfunCnf *pSetCfunCnf)
{
    if((rc == CME_SUCC) && (pSetCfunCnf->func == CMI_DEV_FULL_FUNC))
    {
        TIMER_Netlight_PWM(NETLIGHT_FLICKER_FAST);
    }

    return;
}


/******************************************************************************
 * cmsGetNetlightMode
 * Description: get the netlight mode from the NVM file, which configured by AT+ECLEDMODE.
 * input:void
 * output: CmsNetlightMode netlightMode
 * Comment:
******************************************************************************/
static CmsNetlightMode cmsGetNetlightMode(void)
{
    CmsNetlightMode netlightMode = NET_LIGHT_DISABLE;
    UINT8 chanId = 0;

    for (chanId = CMS_CHAN_1; chanId < MID_WARE_USED_AT_CHAN_NUM; chanId++)
    {
        if (mwGetAtChanConfigItemValue(chanId, MID_WARE_AT_CHAN_ECLED_MODE_CFG) == 1)
        {
            netlightMode = NET_LIGHT_ENABLE;
            break;
        }
    }

    return netlightMode;
}

/******************************************************************************
 ******************************************************************************
 External functions
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS




/******************************************************************************
 * cmsNetlightMonitorCmiInd
 * Description:  Monitor the CMI INDICATION and control the net light if need
 * input: SignalBuf *indSignalPtr;
 * output: CmsRetId
 * Comment:
******************************************************************************/
CmsRetId cmsNetlightMonitorCmiInd(const SignalBuf *indSignalPtr)
{
    CmsRetId ret = CMS_RET_SUCC;
    CacCmiInd   *pCmiInd = PNULL;
    UINT16    primId = 0, sgId = 0;

    if (cmsGetNetlightMode() == NET_LIGHT_DISABLE)
    {
        return ret;
    }

    pCmiInd = (CacCmiInd *)(indSignalPtr->sigBody);
    sgId    = CMS_GET_CMI_SG_ID(pCmiInd->header.indId);
    primId  = CMS_GET_CMI_PRIM_ID(pCmiInd->header.indId);

    if (sgId == CAC_PS)
    {
        switch (primId)
        {
            case CMI_PS_CEREG_IND:
                cmsNetlightProcCmiPsCEREGInd((CmiPsCeregInd *)pCmiInd->body);
                break;

            case CMI_PS_CONN_STATUS_IND:
                cmsNetlightProcCmiPsCSCONInd((CmiPsConnStatusInd *)pCmiInd->body);
                break;

            default:
                break;
        }
    }

    return ret;
}


/******************************************************************************
 * cmsNetlightMonitorCmiCnf
 * Description: Monitor Cmi Cnf and control the net light if need
 * input: const SignalBuf *cnfSignalPtr;    //signal
 * output: void
 * Comment:
******************************************************************************/
CmsRetId cmsNetlightMonitorCmiCnf(const SignalBuf *cnfSignalPtr)
{
    CmsRetId ret = CMS_RET_SUCC;
    UINT16    sgId = 0;
    UINT16    primId = 0;
    CacCmiCnf   *pCmiCnf = PNULL;

    if (cmsGetNetlightMode() == NET_LIGHT_DISABLE)
    {
        return ret;
    }

    pCmiCnf = (CacCmiCnf *)(cnfSignalPtr->sigBody);
    sgId    = CMS_GET_CMI_SG_ID(pCmiCnf->header.cnfId);
    primId  = CMS_GET_CMI_PRIM_ID(pCmiCnf->header.cnfId);

    if (sgId == CAC_DEV)
    {
        switch (primId)
        {
            case CMI_DEV_SET_CFUN_CNF:
                cmsNetlightProcCmiDevSetCfunCnf(pCmiCnf->header.rc, (CmiDevSetCfunCnf *)pCmiCnf->body);
                break;

            default:
                break;
        }
    }

    return ret;
}



