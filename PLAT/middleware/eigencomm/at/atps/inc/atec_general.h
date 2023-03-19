/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_general.h
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef __ATEC_PLAT_H__
#define __ATEC_PLAT_H__


#include <stdint.h>
#include "at_util.h"

#include "debug_log.h"
#include "debug_trace.h"

/* ATE */
#define ATC_E_0_VAL_MIN                   0
#define ATC_E_0_VAL_MAX                   1
#define ATC_E_0_VAL_DEFAULT               0

/* ATQ */
#define ATC_Q_0_VAL_MIN                   0
#define ATC_Q_0_VAL_MAX                   1
#define ATC_Q_0_VAL_DEFAULT               0


/* AT+CGMI */
#define ATC_CGMI_0_VAL_MIN                   0
#define ATC_CGMI_0_VAL_MAX                   2
#define ATC_CGMI_0_VAL_DEFAULT               0

/* AT+CGMM */
#define ATC_CGMM_0_VAL_MIN                   0
#define ATC_CGMM_0_VAL_MAX                   2
#define ATC_CGMM_0_VAL_DEFAULT               0

/* AT+CSCS */
#define ATC_CSCS_0_VAL_MIN                   0
#define ATC_CSCS_0_VAL_MAX                   2
#define ATC_CSCS_0_VAL_DEFAULT               0

/* AT+CMUX */
#define ATC_CMUX_0_VAL_MIN                   0
#define ATC_CMUX_0_VAL_MAX                   2
#define ATC_CMUX_0_VAL_DEFAULT               0

/* AT+CMEE */
#define ATC_CMEE_0_VAL_MIN                   0
#define ATC_CMEE_0_VAL_MAX                   2
#define ATC_CMEE_0_VAL_DEFAULT               0



/* AT+ECURC */
#define ATC_ECURC_0_MAX_PARM_STR_LEN                 16
#define ATC_ECURC_0_MAX_PARM_STR_DEFAULT             NULL
#define ATC_ECURC_1_VAL_MIN                 0
#define ATC_ECURC_1_VAL_MAX                 1
#define ATC_ECURC_1_VAL_DEFAULT             0
#define ATC_ECURC_GET_RSP_STR_LEN                 128




CmsRetId gcAT(const AtCmdInputContext *pAtCmdReq);
CmsRetId gcATE(const AtCmdInputContext *pAtCmdReq);
CmsRetId gcATQ(const AtCmdInputContext *pAtCmdReq);
CmsRetId gcATW(const AtCmdInputContext *pAtCmdReq);
CmsRetId gcCGMI(const AtCmdInputContext *pAtCmdReq);
CmsRetId gcCGMM(const AtCmdInputContext *pAtCmdReq);
CmsRetId gcCSCS(const AtCmdInputContext *pAtCmdReq);
CmsRetId gcCMUX(const AtCmdInputContext *pAtCmdReq);
CmsRetId gcCMEE(const AtCmdInputContext *pAtCmdReq);
CmsRetId gcECURC(const AtCmdInputContext *pAtCmdReq);

#endif

/* END OF FILE */
