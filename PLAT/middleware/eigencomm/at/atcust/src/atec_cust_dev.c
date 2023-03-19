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
#include "atc_decoder.h"
#include "at_util.h"
#include "ctype.h"
#include "cmicomm.h"
#include "cms_comm.h"
#include "task.h"
#include "queue.h"
#include "bsp.h"
#include "atec_cust_dev.h"
#include "nvram.h"
#include "ps_lib_api.h"

#include "app.h"

extern UINT8* getBuildInfo(void);

#define _DEFINE_AT_REQ_FUNCTION_LIST_

/**
  \fn          CmsRetId ccCGMI(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId ccCGMI(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR RspBuf[64] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CGMI=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+CGMI:<manufacturer_ID>");
            break;
        }

        case AT_EXEC_REQ:          /* AT+CGMI */
        {
            sprintf(RspBuf, "+CGMI:\"eigencomm\"");
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, RspBuf);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

#ifdef CUSTOMER_DAHUA

/**
  \fn          CmsRetId ccCGMM(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId ccCGMM(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR RspBuf[64] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CGMM=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+CGMM:<list of supported technologies>,<model>");
            break;
        }

        case AT_EXEC_REQ:          /* AT+CGMM */
        {
			sprintf((char*)RspBuf,"%s",MODULE);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, RspBuf);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId ccCGMR(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId  ccCGMR(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32  reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    CHAR RspBuf[300] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+CGMR=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        case AT_EXEC_REQ:         /* AT+CGMR */
        {
	        sprintf((char*)RspBuf, "+CGMR:%s_%s%s",MODULE,TVERSION,SDATE);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, RspBuf);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId ccCGSN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId  ccCGSN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    UINT32  reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    CHAR    rspBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
    CHAR    rspBufTemp[8] = {0};
    CHAR    imeiBuf[18] = {0};
    CHAR    snBuf[32] = {0};
    INT32   snt;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+CGSN=? */
        {
            sprintf(rspBuf, "+CGSN: (0,1,2,3)");
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
            break;
        }

        case AT_SET_REQ:            /* AT+CGSN=<snt> */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &snt, CC_CGSN_VALUE_MIN, CC_CGSN_VALUE_MAX, CC_CGSN_VALUE_DEF)) != AT_PARA_ERR)
            {
                ret = CMS_RET_SUCC;

                if (snt == 1)
                {
                    /*
                     * returns the IMEI (International Mobile station Equipment Identity)
                     */
                    memset(imeiBuf, 0, sizeof(imeiBuf));
                    appGetImeiNumSync(imeiBuf);
                    snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "+CGSN: %s", imeiBuf);
                }
                else if (snt == 0)
                {
                    /*
                     * returns <sn>
                     */
                    memset(snBuf, 0, sizeof(snBuf));

                    if (!appGetSNNumSync(snBuf))
                    {
                        /*
                         * <sn>: one or more lines of information text determined by the MT manufacturer. Typically,
                         *  the text will consist of a single line containing the IMEI number of the MT, but manufacturers
                         *  may choose to provide more information if desired.
                         * If SN not set, return IMEI
                         */
                        memset(snBuf, 0, sizeof(snBuf));
                        appGetImeiNumSync(snBuf);
                    }

                    snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "+CGSN: %s", snBuf);
                }
                else if (snt == 2)
                {
                    /*
                     * returns the IMEISV (International Mobile station Equipment Identity and Software Version number)
                     */
                    memset(imeiBuf, 0, sizeof(imeiBuf));
                    appGetImeiNumSync(imeiBuf);

                    snprintf(rspBufTemp, 8, "%s", SDK_MAJOR_VERSION);

                    /* get 2 bytes SN */
                    rspBufTemp[0] = rspBufTemp[1];
                    rspBufTemp[1] = rspBufTemp[2];
                    rspBufTemp[2] = 0;

                    imeiBuf[14] = 0;      //delete CD

                    snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "+CGSN: %s%s", imeiBuf, rspBufTemp);
                }
                else if (snt == 3)
                {
                    /*
                     * returns the SVN (Software Version Number)
                    */
                    snprintf(rspBufTemp, 8, "%s", SDK_MAJOR_VERSION);

                    /* get 2 bytes SN */
                    rspBufTemp[0] = rspBufTemp[1];
                    rspBufTemp[1] = rspBufTemp[2];
                    rspBufTemp[2] = 0;
                    snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "+CGSN: %s", rspBufTemp);
                }
                else
                {
                    //should not arrive here
                    OsaDebugBegin(FALSE, snt, 0, 0);
                    OsaDebugEnd();

                    ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_INCORRECT_PARAM), NULL);
                }
            }

            if (ret == CMS_RET_SUCC)
            {
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_INCORRECT_PARAM), NULL);
            }
            break;
        }

        case AT_EXEC_REQ:            /* AT+CGSN */
        {
            /*
             * To get <sn> which returns IMEI of the MT
             * AT+CGSN
             * 490154203237518
             * OK
            */
           #if 0 
           ret = CMS_RET_SUCC;
           memset(snBuf, 0, sizeof(snBuf));
           if (!appGetSNNumSync(snBuf))
           {
               /*
                * <sn>: one or more lines of information text determined by the MT manufacturer. Typically,
                *  the text will consist of a single line containing the IMEI number of the MT, but manufacturers
                *  may choose to provide more information if desired.
                * If SN not set, return IMEI
                */
               memset(snBuf, 0, sizeof(snBuf));
               appGetImeiNumSync(snBuf);
           }
           snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "%s", snBuf);
           #else
             ret = CMS_RET_SUCC;
            memset(imeiBuf, 0, sizeof(imeiBuf));
            appGetImeiNumSync(imeiBuf);
            snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "%s", imeiBuf);
           #endif
           ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
           break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId ccATI(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId ccATI(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32  reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    CHAR    rspBuf[ATEC_IND_RESP_256_STR_LEN] = {0};

    switch (operaType)
    {
        case AT_EXEC_REQ:         /* ATI */
        {
			snprintf((char*)rspBuf,ATEC_IND_RESP_256_STR_LEN,"MeiG\r\n%s\r\n%s_%s%s",MODULE,MODULE,TVERSION,SDATE);
			ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, (CME_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

CmsRetId ccSGSW(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32  reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    CHAR    rspBuf[ATEC_IND_RESP_256_STR_LEN] = {0};

    switch (operaType)
    {
        case AT_EXEC_REQ:         /* AT+SGSW*/
        {   
            sprintf((char*)rspBuf, "%s.%s%s%s_%s_EC616S",MODULE,SDKVERSION,TVERSION,SDATE,MVERSION);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, (CME_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

CmsRetId ccECVER(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    UINT32  reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    CHAR    rspBuf[ATEC_IND_RESP_256_STR_LEN] = {0};  
    INT32   snt;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+ECVER=? */
        {
            sprintf(rspBuf, "+ECVER: (1)");
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
            break;
        }

        case AT_SET_REQ:            /* AT+ECVER=<snt> */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &snt, ATC_ECVER_VAL_MIN, ATC_ECVER_VAL_MAX, ATC_ECVER_VAL_DEFAULT)) != AT_PARA_ERR)
            {
                ret = CMS_RET_SUCC;

                if (snt == 1)
                {                    
                    snprintf((char*)rspBuf, ATEC_IND_RESP_256_STR_LEN, "+ECVER: \"\r\n%s\"\r\n", getBuildInfo());                    
                }                
                else
                {
                    //should not arrive here
                    OsaDebugBegin(FALSE, snt, 0, 0);
                    OsaDebugEnd();

                    ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_INCORRECT_PARAM), NULL);
                }
            }

            if (ret == CMS_RET_SUCC)
            {
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_INCORRECT_PARAM), NULL);
            }
            break;
        }        

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

#else


/**
  \fn          CmsRetId ccCGMM(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId ccCGMM(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32 reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    CHAR RspBuf[64] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:         /* AT+CGMM=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+CGMM:<list of supported technologies>,<model>");
            break;
        }

        case AT_EXEC_REQ:          /* AT+CGMM */
        {

            #if defined CHIP_EC616 || defined CHIP_EC616_Z0
            sprintf(RspBuf, "+CGMM:\"eigencomm\",\"EC616\"");
            #elif defined CHIP_EC616S
            sprintf(RspBuf, "+CGMM:\"eigencomm\",\"EC616S\"");
            #endif
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, RspBuf);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId ccCGMR(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId  ccCGMR(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32  reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    CHAR RspBuf[300] = {0};

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+CGMR=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, NULL);
            break;
        }

        case AT_EXEC_REQ:         /* AT+CGMR */
        {
//            sprintf((char*)RspBuf, "+CGMR: %s", getBuildInfo());
	        sprintf((char*)RspBuf, "MeiG \r\n%s",FIRMWARE_VERSION);
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, RspBuf);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

/**
  \fn          CmsRetId ccCGSN(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId  ccCGSN(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    UINT32  reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    CHAR    rspBuf[ATEC_IND_RESP_128_STR_LEN] = {0};
    CHAR    rspBufTemp[8] = {0};
    CHAR    imeiBuf[18] = {0};
    CHAR    snBuf[32] = {0};
    INT32   snt;

    switch (operaType)
    {
        case AT_TEST_REQ:            /* AT+CGSN=? */
        {
            sprintf(rspBuf, "+CGSN: (0,1,2,3)");
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
            break;
        }

        case AT_SET_REQ:            /* AT+CGSN=<snt> */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &snt, CC_CGSN_VALUE_MIN, CC_CGSN_VALUE_MAX, CC_CGSN_VALUE_DEF)) != AT_PARA_ERR)
            {
                ret = CMS_RET_SUCC;

                if (snt == 1)
                {
                    /*
                     * returns the IMEI (International Mobile station Equipment Identity)
                     */
                    memset(imeiBuf, 0, sizeof(imeiBuf));
                    appGetImeiNumSync(imeiBuf);
                    snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "+CGSN: \"%s\"", imeiBuf);
                }
                else if (snt == 0)
                {
                    /*
                     * returns <sn>
                     */
                    memset(snBuf, 0, sizeof(snBuf));

                    if (!appGetSNNumSync(snBuf))
                    {
                        /*
                         * <sn>: one or more lines of information text determined by the MT manufacturer. Typically,
                         *  the text will consist of a single line containing the IMEI number of the MT, but manufacturers
                         *  may choose to provide more information if desired.
                         * If SN not set, return IMEI
                         */
                        memset(snBuf, 0, sizeof(snBuf));
                        appGetImeiNumSync(snBuf);
                    }

                    snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "+CGSN: \"%s\"", snBuf);
                }
                else if (snt == 2)
                {
                    /*
                     * returns the IMEISV (International Mobile station Equipment Identity and Software Version number)
                     */
                    memset(imeiBuf, 0, sizeof(imeiBuf));
                    appGetImeiNumSync(imeiBuf);

                    snprintf(rspBufTemp, 8, "%s", SDK_MAJOR_VERSION);

                    /* get 2 bytes SN */
                    rspBufTemp[0] = rspBufTemp[1];
                    rspBufTemp[1] = rspBufTemp[2];
                    rspBufTemp[2] = 0;

                    imeiBuf[14] = 0;      //delete CD

                    snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "+CGSN: \"%s%s\"", imeiBuf, rspBufTemp);
                }
                else if (snt == 3)
                {
                    /*
                     * returns the SVN (Software Version Number)
                    */
                    snprintf(rspBufTemp, 8, "%s", SDK_MAJOR_VERSION);

                    /* get 2 bytes SN */
                    rspBufTemp[0] = rspBufTemp[1];
                    rspBufTemp[1] = rspBufTemp[2];
                    rspBufTemp[2] = 0;
                    snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "+CGSN: \"%s\"", rspBufTemp);
                }
                else
                {
                    //should not arrive here
                    OsaDebugBegin(FALSE, snt, 0, 0);
                    OsaDebugEnd();

                    ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_INCORRECT_PARAM), NULL);
                }
            }

            if (ret == CMS_RET_SUCC)
            {
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_INCORRECT_PARAM), NULL);
            }
            break;
        }

        case AT_EXEC_REQ:            /* AT+CGSN */
        {
            /*
             * To get <sn> which returns IMEI of the MT
             * AT+CGSN
             * 490154203237518
             * OK
            */
           ret = CMS_RET_SUCC;
           memset(snBuf, 0, sizeof(snBuf));
           if (!appGetSNNumSync(snBuf))
           {
               /*
                * <sn>: one or more lines of information text determined by the MT manufacturer. Typically,
                *  the text will consist of a single line containing the IMEI number of the MT, but manufacturers
                *  may choose to provide more information if desired.
                * If SN not set, return IMEI
                */
               memset(snBuf, 0, sizeof(snBuf));
               appGetImeiNumSync(snBuf);
           }
           snprintf(rspBuf, ATEC_IND_RESP_128_STR_LEN, "%s", snBuf);

           ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
           break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, ( CME_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}



/**
  \fn          CmsRetId ccATI(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId ccATI(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId ret = CMS_FAIL;
    UINT32  reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32  operaType = (UINT32)pAtCmdReq->operaType;
    CHAR    rspBuf[ATEC_IND_RESP_256_STR_LEN] = {0};

    switch (operaType)
    {
        case AT_EXEC_REQ:         /* ATI */
        {
            snprintf((char*)rspBuf, ATEC_IND_RESP_256_STR_LEN, "Eigencomm %s", getBuildInfo());
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, rspBuf);
            break;
        }

        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, (CME_OPERATION_NOT_SUPPORT), NULL);
            break;
        }
    }

    return ret;
}

#endif

/**
  \fn          CmsRetId cdevATANDF(const AtCmdInputContext *pAtCmdReq)
  \brief       at cmd req function
  \param[in]   pAtCmdReq
  \returns     CmsRetId
*/
CmsRetId cdevATANDF(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId    ret = CMS_FAIL;
    UINT32      reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32      operaType = (UINT32)pAtCmdReq->operaType;
    AtParamValueCP  pParamList = pAtCmdReq->pParamList;
    INT32       param0 = 0;

    switch (operaType)
    {
        case AT_EXEC_REQ:       /* AT&F */
        {
            //snprintf((char*)rspBuf, ATEC_IND_RESP_256_STR_LEN, "Eigencomm %s", getBuildInfo());
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, "&F: EXEC not support now");

            break;
        }

        case AT_SET_REQ:            /* AT&F0 */
        case AT_BASIC_EXT_SET_REQ:  /* AT&F=0 */
        {
            if (atGetNumValue(pParamList, 0, &param0,
                              CC_DEV_ATF_0_VAL_MIN, CC_DEV_ATF_0_VAL_MAX, CC_DEV_ATF_0_VAL_DEFAULT) == AT_PARA_OK)
            {
                ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, "&F0: not support now");
            }
            else
            {
                ret = atcReply(reqHandle, AT_RC_CME_ERROR, CME_INCORRECT_PARAM, NULL);
            }
            break;
        }

        case AT_TEST_REQ:       /* AT&F=? */
        {
            ret = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, "&F: (0)");
            break;
        }

        case AT_READ_REQ:       /* AT&F?, not support */
        default:
        {
            ret = atcReply(reqHandle, AT_RC_CME_ERROR, CME_OPERATION_NOT_SUPPORT, NULL);
            break;
        }
    }

    return ret;
}


