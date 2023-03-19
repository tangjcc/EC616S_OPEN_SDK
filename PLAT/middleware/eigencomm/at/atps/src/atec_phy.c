/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename:
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "atc_decoder.h"
#include "ctype.h"
#include "cmicomm.h"
#include "PhyAtCmdDebug.h"
#include "atec_phy.h"
#include "atec_phy_cnf_ind.h"
#include "ps_phy_if.h"


#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position----Below for internal use
/**
  \fn           void phyGetDebugMode(void)
  \brief        Enable all URC
  \returns      void
*/
static BOOL phyGetDebugModule(CHAR *strName, UINT8 *phyModule)
{
    BOOL isMatched = TRUE;

    OsaDebugBegin(strName != PNULL && phyModule != PNULL, strName, phyModule, 0);
    return FALSE;
    OsaDebugEnd();

    if(strcasecmp(strName, "POWER") == 0)
    {
        *phyModule = PHY_DEBUG_SET_POWER;
    }
    else if(strcasecmp(strName, "ESCAPE") == 0)
    {
        *phyModule = PHY_DEBUG_SET_ESCAPE;
    }
    else if(strcasecmp(strName, "OOS") == 0)
    {
        *phyModule = PHY_DEBUG_SET_OOS;
    }
    else if(strcasecmp(strName, "DUMP") == 0)
    {
        *phyModule = PHY_DEBUG_SET_DUMP;
    }
    else if(strcasecmp(strName, "TXDPD") == 0)
    {
        *phyModule = PHY_DEBUG_SET_TXDPD;
    }
    else if(strcasecmp(strName, "GRAPH") == 0)
    {
        *phyModule = PHY_DEBUG_SET_GRAPH;
    }
    else if(strcasecmp(strName, "ALGOPARAM") == 0)
    {
        *phyModule = PHY_DEBUG_SET_ALGOPARAM;
    }
    else if(strcasecmp(strName, "DCXOINIT") == 0)
    {
        *phyModule = PHY_DEBUG_SET_DCXO_CALI_INIT;
    }
    else if(strcasecmp(strName, "LOGLEVEL") == 0)
    {
        *phyModule = PHY_DEBUG_SET_LOG_LEVEL;
    }
    else if(strcasecmp(strName, "ASSERT") == 0)
    {
        *phyModule = PHY_DEBUG_SET_ASSERT;
    }
    else if(strcasecmp(strName, "CLEAN") == 0)
    {
        *phyModule = PHY_DEBUG_CLEAR_ALL;
    }
    else
    {
        isMatched = FALSE;
    }

    return isMatched;
}



#define _DEFINE_AT_REQ_FUNCTION_LIST_

/**
  \fn          CmsRetId phyPHYDBGSET(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   operaType
  \param[in]   pParamList
  \param[in]   paramMaxNum
  \param[in]   paramRealNum
  \param[in]   *handIdPtr
  \returns     CmsRetId
*/
CmsRetId phyECPHYCFG(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList; 
    INT32   paramNumb = pAtCmdReq->paramRealNum;    
    UINT16  paraStrLen = 0;
    UINT8   paraIdx = 0;
    UINT8   paraStr[ATC_PHYDBG_0_STR_MAX_LEN] = {0};
    UINT8   phyModule = 0xff;
    INT32   phyCfgParam[8] = {0xff};
    BOOL    cmdValid = TRUE;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECPHYCFG=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, PNULL);
            break;
        }

        case AT_READ_REQ:         /* AT+ECPHYCFG? */
        {
            ret = phyGetPhyInfo(atHandle);
            break;
        }

        case AT_SET_REQ:          /* AT+ECPHYCFG= */
        {
            if(atGetStrValue(pParamList, 0, paraStr, ATC_PHYDBG_0_STR_MAX_LEN, &paraStrLen, ATC_PHYDBG_0_STR_DEFAULT) != AT_PARA_ERR)
            {
                if (phyGetDebugModule((CHAR *)paraStr, &phyModule) == FALSE)
                {
                    ECOMM_TRACE(UNILOG_ATCMD_EXEC, phyECPHYCFG_0, P_WARNING, 0, "AT+ECPHYCFG, invalid input string parameter");
                    cmdValid = FALSE;
                }
                else//string parameter check ok 
                {   
                    //get number parameters
                    if (paramNumb > 1)
                    {
                        for (paraIdx = 1; paraIdx < paramNumb; paraIdx++)
                        {
                            if (atGetNumValue(pParamList, paraIdx, &phyCfgParam[paraIdx - 1], 
                                ATC_PHYDBG_VAL_MIN, ATC_PHYDBG_VAL_MAX, ATC_PHYDBG_VAL_DEFAULT) == AT_PARA_ERR)
                            {
                                ECOMM_TRACE(UNILOG_ATCMD_EXEC, phyECPHYCFG_1, P_WARNING, 0, "AT+ECPHYCFG, invalid input dec parameter");
                                cmdValid = FALSE;
                                break;
                            }
                        }
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ATCMD_EXEC, phyECPHYCFG_2, P_WARNING, 1, "AT+ECPHYCFG, the paramNumb %d is wrong", paramNumb);
                        cmdValid = FALSE;
                    }
                }
                
                if (cmdValid == TRUE)
                {
                    ret = phySetPhyInfo(atHandle, phyModule, phyCfgParam, (paramNumb - 1));
                }
            }


            if(ret != CMS_RET_SUCC)
            {
                ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        default:
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_UNKNOWN, NULL);
            break;
        }
    }

    return ret;
}

CmsRetId phyECEDRXSIMU(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 atHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32 mode = 0;
    INT32 derxPeriod = 0;
    INT32 ptw = 0;
    AtCmdPhyEdrxPara atCmdPhyEdrxPara;
    CHAR  rspBuf[64] = {0};
    INT32 atRet = 0;

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+ECEDRXSIMU=? */
        {
            ret = atcReply(atHandle, AT_RC_OK, 0, "+ECEDRXSIMU:(0,1),(0-2),(0-1)");
            break;
        }

        case AT_READ_REQ:         /* AT+ECEDRXSIMU? */
        {
            ret = mwGetSimEdrxValue(&atCmdPhyEdrxPara);
            sprintf((char*)rspBuf, "+ECEDRXSIMU: %d,%d,%d", atCmdPhyEdrxPara.enableFlag, atCmdPhyEdrxPara.eDRXPrdIdx, atCmdPhyEdrxPara.eDRXPtwIdx);

            ret = atcReply(atHandle, AT_RC_OK, 0, (char*)rspBuf);
            break;
        }

        case AT_SET_REQ:         /* AT+ECEDRXSIMU=<mode>[,<edrx_period>[,<ptw>]] */
        {
            memset(&atCmdPhyEdrxPara, 0, sizeof(AtCmdPhyEdrxPara));
            ret = mwGetSimEdrxValue(&atCmdPhyEdrxPara);
            if(atGetNumValue(pParamList, 0, &mode, ATC_ECEDRXSIMU_0_VAL_MIN, ATC_ECEDRXSIMU_0_VAL_MAX, ATC_ECEDRXSIMU_0_VAL_DEFAULT) != AT_PARA_ERR)
            {
                if((atRet = atGetNumValue(pParamList, 1, &derxPeriod, ATC_ECEDRXSIMU_1_VAL_MIN, ATC_ECEDRXSIMU_1_VAL_MAX, ATC_ECEDRXSIMU_1_VAL_DEFAULT)) != AT_PARA_ERR)
                {
                    if(atRet == AT_PARA_OK)
                    {
                        atCmdPhyEdrxPara.eDRXPrdIdx = derxPeriod;
                    }
                    if((atRet = atGetNumValue(pParamList, 2, &ptw, ATC_ECEDRXSIMU_2_VAL_MIN, ATC_ECEDRXSIMU_2_VAL_MAX, ATC_ECEDRXSIMU_2_VAL_DEFAULT)) != AT_PARA_ERR)
                    {
                        if(atRet == AT_PARA_OK)
                        {
                            atCmdPhyEdrxPara.eDRXPtwIdx = ptw;
                        }
                        atCmdPhyEdrxPara.enableFlag = mode;
                    
                        ret = mwSetSimEdrxValue(&atCmdPhyEdrxPara);

                        UINT32 paraVal[3] = {mode, derxPeriod, ptw};
                        phySetPhyInfo(atHandle, PHY_DEBUG_SET_EDRX_SIMU, (INT32 *)paraVal, 3);
                    }
                }
            }

            ret = atcReply(atHandle, AT_RC_OK, 0, NULL);
            break;
        }

        default:         
        {
            ret = atcReply(atHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}

