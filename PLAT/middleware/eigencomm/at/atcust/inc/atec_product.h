/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_product.h
*
*  Description: production related command
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef __ATEC_PRODUCT_H__
#define __ATEC_PRODUCT_H__

#include "at_util.h"



/* AT+ECRFTEST */
#define ATC_ECRFTEST_0_STR_DEFAULT          NULL
#define ATC_ECRFTEST_0_STR_MAX_LEN          5000         /* */

/* AT+ECRFNST */
#define ATC_ECRFNST_0_STR_DEFAULT          NULL
#define ATC_ECRFNST_0_STR_MAX_LEN          5000         /* */

/* AT+ECSLEEP */
#define ECSLEEP_HANDLE_CREATED      0x1
#define ECSLEEP_HANDLE_NOT_CREATED  0x0

#define ECSLEEP_HIB2  0x0
#define ECSLEEP_HIB1  0x1
#define ECSLEEP_SLP2  0x2
#define ECSLEEP_SLP1  0x3

#define  CC_ECSLEEP_VALUE_MIN   0
#define  CC_ECSLEEP_VALUE_MAX   3
#define  CC_ECSLEEP_VALUE_DEF   0

/* AT+ECSAVEFAC */
#define SAVEFAC_0_STR_LEN             16
#define SAVEFAC_0_STR_BUF_LEN         (SAVEFAC_0_STR_LEN +1)
#define SAVEFAC_0_STR_DEF             NULL

/* AT+ECIPR */
#define ATC_ECIPR_MAX_PARM_STR_LEN             32
#define ATC_ECIPR_MAX_PARM_STR_DEFAULT         NULL

/* AT+ECNPICFG */
#define ATC_ECNPICFG_MAX_PARM_STR_LEN             32
#define ATC_ECNPICFG_MAX_PARM_STR_DEFAULT         NULL

#define ATC_ECNPICFG_RFCALI_VAL_MIN                0
#define ATC_ECNPICFG_RFCALI_VAL_MAX                1
#define ATC_ECNPICFG_RFCALI_VAL_DEFAULT            0  /* full functionality */

#define ATC_ECNPICFG_RFNST_VAL_MIN                0
#define ATC_ECNPICFG_RFNST_VAL_MAX                1
#define ATC_ECNPICFG_RFNST_VAL_DEFAULT            0  /* full functionality */

/* AT+ECPRODMODE */
#define ATC_ECPRODMODE_MAX_PARM_STR_LEN             32

CmsRetId pdevECRFTEST(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECRFNST(const AtCmdInputContext *pAtCmdReq);
CmsRetId ccECSLEEP(const AtCmdInputContext *pAtCmdReq);
CmsRetId ccECSAVEFAC(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECIPR(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevNPICFG(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECRFSTAT(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECPRODMODE(const AtCmdInputContext *pAtCmdReq);
#endif

/* END OF FILE */
