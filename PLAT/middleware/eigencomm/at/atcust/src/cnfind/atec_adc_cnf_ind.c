/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_adc_cnf_ind.c
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "atec_adc_cnf_ind.h"
#include "atec_cust_cmd_table.h"
#include "at_adc_task.h"
#include "ec_adc_api.h"

/******************************************************************************
 * adcCmsCnfHdrList
 * const table, should store in FLASH memory
******************************************************************************/
static const ApplCnfFuncMapList adcCmsCnfHdrList[] =
{
    {APPL_ADC_CNF, ecAdcCnf},

    {APPL_ADC_PRIM_ID_END, PNULL}   /* this should be the last */
};


/****************************************************************************************
*  FUNCTION:  ecAdcCnf
*
*  PARAMETERS:
*
*  DESCRIPTION:
*
*  RETURNS: CmsRetId
*
****************************************************************************************/
#define _DEFINE_AT_CNF_FUNCTION_LIST_
CmsRetId ecAdcCnf(UINT16 reqHandle, UINT16 rc, void *paras)
{
    adcCnfMsg* cnfMsgPtr = (adcCnfMsg*)(paras);

    CHAR buff[64] = {0};

    if(rc == APPL_RET_FAIL)
    {
        atcReply(reqHandle, AT_RC_ADC_ERROR, ADC_CONVERSION_TIMEOUT, NULL);
        return CMS_FAIL;
    }
    else
    {
        if(cnfMsgPtr->ack == AT_ADC_REQ_BITMAP_TEMP)
        {
            snprintf(buff, 64, "+ECADC: TEMP, %d\r\n", cnfMsgPtr->data[0]);
        }
        else if(cnfMsgPtr->ack == AT_ADC_REQ_BITMAP_VBAT)
        {
            snprintf(buff, 64, "+ECADC: VBAT, %d\r\n", cnfMsgPtr->data[1]);
        }
        else if(cnfMsgPtr->ack == (AT_ADC_REQ_BITMAP_VBAT |AT_ADC_REQ_BITMAP_TEMP))
        {
            snprintf(buff, 64, "+ECADC: TEMP, %d, VBAT, %d\r\n", cnfMsgPtr->data[0], cnfMsgPtr->data[1]);
        }

        atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, buff);

        return CMS_RET_SUCC;
    }
}

void atApplAdcProcCmsCnf(CmsApplCnf *pCmsCnf)
{
    UINT8 hdrIndex = 0;
    ApplCnfHandler applCnfHdr = PNULL;
    UINT16 primId = 0;

    OsaDebugBegin(pCmsCnf != NULL, 0, 0, 0);
    return;
    OsaDebugEnd();

    primId  = AT_GET_CMI_PRIM_ID(pCmsCnf->header.primId);

    while (adcCmsCnfHdrList[hdrIndex].primId != 0)
    {
        if (adcCmsCnfHdrList[hdrIndex].primId == primId)
        {
            applCnfHdr = adcCmsCnfHdrList[hdrIndex].applCnfHdr;
            break;
        }
        hdrIndex++;
    }

    if (applCnfHdr != PNULL)
    {
        (*applCnfHdr)(pCmsCnf->header.srcHandler, pCmsCnf->header.rc, pCmsCnf->body);
    }
    else
    {
        OsaDebugBegin(applCnfHdr != NULL, 0, 0, 0);
        OsaDebugEnd();
    }

    return;
}

