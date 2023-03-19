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
#ifndef _ATEC_CUST_DEV_H_
#define _ATEC_CUST_DEV_H_

#include <stdint.h>
#include "at_def.h"
#include "debug_log.h"
#include "debug_trace.h"

#include "app.h" // 20210926 add to support dahua qfashion1

#define  CC_CGSN_VALUE_MIN   0
#define  CC_CGSN_VALUE_MAX   3
#define  CC_CGSN_VALUE_DEF   1

/* AT&F[<value>] */
#define  CC_DEV_ATF_0_VAL_MIN       0
#define  CC_DEV_ATF_0_VAL_MAX       0
#define  CC_DEV_ATF_0_VAL_DEFAULT   1

#ifdef CUSTOMER_DAHUA
/*AT+ECVER*/
#define ATC_ECVER_VAL_MIN                0
#define ATC_ECVER_VAL_MAX                1
#define ATC_ECVER_VAL_DEFAULT            0
#endif

CmsRetId ccCGMI(const AtCmdInputContext *pAtCmdReq);
CmsRetId ccCGMM(const AtCmdInputContext *pAtCmdReq);
CmsRetId ccCGMR(const AtCmdInputContext *pAtCmdReq);
CmsRetId ccCGSN(const AtCmdInputContext *pAtCmdReq);
CmsRetId ccATI(const AtCmdInputContext *pAtCmdReq);
CmsRetId cdevATANDF(const AtCmdInputContext *pAtCmdReq);    //AT&F

#ifdef CUSTOMER_DAHUA
CmsRetId ccSGSW(const AtCmdInputContext *pAtCmdReq);    
CmsRetId ccECVER(const AtCmdInputContext *pAtCmdReq);
#endif

#endif

/* END OF FILE */
