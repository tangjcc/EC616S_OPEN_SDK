/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_ref_plat.h
*
*  Description: Process plat/device related AT CMD
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef _ATEC_REF_PLAT_H_
#define _ATEC_REF_PLAT_H_

#include "at_util.h"

/* AT+NLOGLEVEL */
#define ATC_NLOGLEVEL_CORE_MAX_STR_LEN         16
#define ATC_NLOGLEVEL_CORE_STR_DEFAULT         NULL
#define ATC_NLOGLEVEL_LEVEL_MAX_STR_LEN         16
#define ATC_NLOGLEVEL_LEVEL_STR_DEFAULT         NULL

#define ATC_NLOGLEVEL_MAX_LEN         128

/* AT+NITZ */
#define ATC_NITZ_0_VAL_MIN                   0
#define ATC_NITZ_0_VAL_MAX                   1
#define ATC_NITZ_0_VAL_DEFAULT               0

/* AT+QLEDMODE */
#define ATC_QLED_MODE_VAL_MIN               0
#define ATC_QLED_MODE_VAL_MAX               1
#define ATC_QLED_MODE_VAL_DEFAULT           0

/* AT+NATSPEED */
#define ATC_NATSPEED_BAUDRATE_MIN         1
#define ATC_NATSPEED_BAUDRATE_MAX         460800
#define ATC_NATSPEED_BAUDRATE_DEFAULT     9600

#define ATC_NATSPEED_TIMEOUT_MIN          0
#define ATC_NATSPEED_TIMEOUT_MAX          30
#define ATC_NATSPEED_TIMEOUT_DEFAULT      3

#define ATC_NATSPEED_STORE_MIN            0
#define ATC_NATSPEED_STORE_MAX            1
#define ATC_NATSPEED_STORE_DEFAULT        1

#define ATC_NATSPEED_SYNCMODE_MIN         0
#define ATC_NATSPEED_SYNCMODE_MAX         3
#define ATC_NATSPEED_SYNCMODE_DEFAULT     0

#define ATC_NATSPEED_STOPBITS_MIN         1
#define ATC_NATSPEED_STOPBITS_MAX         2
#define ATC_NATSPEED_STOPBITS_DEFAULT     1

#define ATC_NATSPEED_PARITY_MIN           0
#define ATC_NATSPEED_PARITY_MAX           2
#define ATC_NATSPEED_PARITY_DEFAULT       0

#define ATC_NATSPEED_XONXOFF_MIN          0
#define ATC_NATSPEED_XONXOFF_MAX          1
#define ATC_NATSPEED_XONXOFF_DEFAULT      0

/* AT+ECDMPCONFIG */
#define ECDMPCONFIG_0_TYPE_MIN              0
#define ECDMPCONFIG_0_TYPE_MAX              6
#define ECDMPCONFIG_0_TYPE_DEF              0

#define ECDMPCONFIG_1_MODE_STR_LEN              8
#define ECDMPCONFIG_1_MODE_STR_DEF              NULL
#define ECDMPCONFIG_1_MODE_MIN                  0
#define ECDMPCONFIG_1_MODE_MAX                  1
#define ECDMPCONFIG_1_MODE_DEF                  0

#define ECDMPCONFIG_1_IP_STR_LEN              128
#define ECDMPCONFIG_1_IP_STR_DEF              NULL
#define ECDMPCONFIG_2_PORT_STR_LEN              12
#define ECDMPCONFIG_2_PORT_STR_DEF              NULL
#define ECDMPCONFIG_2_PORT_MIN              0
#define ECDMPCONFIG_2_PORT_MAX              65535
#define ECDMPCONFIG_2_PORT_DEF              5683

#define ECDMPCONFIG_3_PERIOD_STR_LEN              16
#define ECDMPCONFIG_3_PERIOD_STR_DEF              NULL

#define ECDMPCONFIG_3_PERIOD_MIN              300
#define ECDMPCONFIG_3_PERIOD_MAX              4294966
#define ECDMPCONFIG_3_PERIOD_DEF              86400
#define ECDMPCONFIG_1_KEY_STR_LEN               128
#define ECDMPCONFIG_1_KEY_STR_DEF               NULL
#define ECDMPCONFIG_2_PASSWD_STR_LEN               128
#define ECDMPCONFIG_2_PASSWD_STR_DEF               NULL

#define ECDMPCONFIG_3_IF_STR_LEN               8
#define ECDMPCONFIG_3_IF_STR_DEF               NULL
#define ECDMPCONFIG_3_IF_MIN                0
#define ECDMPCONFIG_3_IF_MAX                2
#define ECDMPCONFIG_3_IF_DEF                0

#define ECDMPCONFIG_4_IMEI_STR_LEN               17
#define ECDMPCONFIG_4_IMEI_STR_DEF               NULL
#define ECDMPCONFIG_1_PLMM1_STR_LEN               128
#define ECDMPCONFIG_1_PLMM1_STR_DEF               NULL
#define ECDMPCONFIG_2_PLMM2_STR_LEN           128
#define ECDMPCONFIG_2_PLMM2_STR_DEF           NULL
#define ECDMPCONFIG_3_PLMM3_STR_LEN               128
#define ECDMPCONFIG_3_PLMM3_STR_DEF               NULL

#define ECDMPCONFIG_1_QUERY_STR_LEN               8
#define ECDMPCONFIG_1_QUERY_STR_DEF               NULL
#define ECDMPCONFIG_1_QUERY_MIN                0
#define ECDMPCONFIG_1_QUERY_MAX                3
#define ECDMPCONFIG_1_QUERY_DEF                0

#define ECDMPCONFIG_1_ERASE_STR_LEN               8
#define ECDMPCONFIG_1_ERASE_STR_DEF               NULL
#define ECDMPCONFIG_1_ERASE_MIN                   0
#define ECDMPCONFIG_1_ERASE_MAX                   4
#define ECDMPCONFIG_1_ERASE_DEF                   0

#define ECDMPCONFIG_1_STATE_STR_LEN               8
#define ECDMPCONFIG_1_STATE_STR_DEF               NULL
#define ECDMPCONFIG_1_STATE_MIN                0
#define ECDMPCONFIG_1_STATE_MAX                13
#define ECDMPCONFIG_1_STATE_DEF                0

/* AT+ECDMPCFG */
#define ECDMPCFG_0_TYPE_MIN              0
#define ECDMPCFG_0_TYPE_MAX              6
#define ECDMPCFG_0_TYPE_DEF              0

#define ECDMPCFG_1_MODE_STR_LEN              8
#define ECDMPCFG_1_MODE_STR_DEF              NULL
#define ECDMPCFG_1_MODE_MIN                  0
#define ECDMPCFG_1_MODE_MAX                  1
#define ECDMPCFG_1_MODE_DEF                  0

#define ECDMPCFG_1_IP_STR_LEN              128
#define ECDMPCFG_1_IP_STR_DEF              NULL
#define ECDMPCFG_2_PORT_STR_LEN              12
#define ECDMPCFG_2_PORT_STR_DEF              NULL
#define ECDMPCFG_2_PORT_MIN              0
#define ECDMPCFG_2_PORT_MAX              65535
#define ECDMPCFG_2_PORT_DEF              5683

#define ECDMPCFG_3_PERIOD_STR_LEN              16
#define ECDMPCFG_3_PERIOD_STR_DEF              NULL

#define ECDMPCFG_3_PERIOD_MIN              300
#define ECDMPCFG_3_PERIOD_MAX              4294966
#define ECDMPCFG_3_PERIOD_DEF              86400
#define ECDMPCFG_1_KEY_STR_LEN               128
#define ECDMPCFG_1_KEY_STR_DEF               NULL
#define ECDMPCFG_2_PASSWD_STR_LEN               128
#define ECDMPCFG_2_PASSWD_STR_DEF               NULL

#define ECDMPCFG_3_IF_STR_LEN               8
#define ECDMPCFG_3_IF_STR_DEF               NULL
#define ECDMPCFG_3_IF_MIN                0
#define ECDMPCFG_3_IF_MAX                2
#define ECDMPCFG_3_IF_DEF                0

#define ECDMPCFG_4_IMEI_STR_LEN               17
#define ECDMPCFG_4_IMEI_STR_DEF               NULL
#define ECDMPCFG_1_PLMM1_STR_LEN               128
#define ECDMPCFG_1_PLMM1_STR_DEF               NULL
#define ECDMPCFG_2_PLMM2_STR_LEN           128
#define ECDMPCFG_2_PLMM2_STR_DEF           NULL
#define ECDMPCFG_3_PLMM3_STR_LEN               128
#define ECDMPCFG_3_PLMM3_STR_DEF               NULL

#define ECDMPCFG_1_QUERY_STR_LEN               8
#define ECDMPCFG_1_QUERY_STR_DEF               NULL
#define ECDMPCFG_1_QUERY_MIN                0
#define ECDMPCFG_1_QUERY_MAX                3
#define ECDMPCFG_1_QUERY_DEF                0

#define ECDMPCFG_1_ERASE_STR_LEN               8
#define ECDMPCFG_1_ERASE_STR_DEF               NULL
#define ECDMPCFG_1_ERASE_MIN                   0
#define ECDMPCFG_1_ERASE_MAX                   4
#define ECDMPCFG_1_ERASE_DEF                   0

#define ECDMPCFG_1_STATE_STR_LEN               8
#define ECDMPCFG_1_STATE_STR_DEF               NULL
#define ECDMPCFG_1_STATE_MIN                0
#define ECDMPCFG_1_STATE_MAX                13
#define ECDMPCFG_1_STATE_DEF                0


/* AT+ECDMPCFGEX */
#define ECDMPCFGEX_0_TYPE_MIN              0
#define ECDMPCFGEX_0_TYPE_MAX              5
#define ECDMPCFGEX_0_TYPE_DEF              0
#define ECDMPCFGEX_1_ACT_MIN                 0
#define ECDMPCFGEX_1_ACT_MAX                 2
#define ECDMPCFGEX_1_ACT_DEF                 0
#define ECDMPCFGEX_2_APP_STR_LEN              255
#define ECDMPCFGEX_2_APP_STR_DEF              NULL
#define ECDMPCFGEX_2_MAC_STR_LEN              19
#define ECDMPCFGEX_2_MAC_STR_DEF              NULL
#define ECDMPCFGEX_2_ROM_STR_LEN              19
#define ECDMPCFGEX_2_ROM_STR_DEF              NULL
#define ECDMPCFGEX_3_RAM_STR_LEN           19
#define ECDMPCFGEX_3_RAM_STR_DEF           NULL
#define ECDMPCFGEX_4_CPU_STR_LEN              19
#define ECDMPCFGEX_4_CPU_STR_DEF              NULL
#define ECDMPCFGEX_5_OSSYSVER_STR_LEN           47
#define ECDMPCFGEX_5_OSSYSVER_STR_DEF           NULL
#define ECDMPCFGEX_2_SWVER_STR_LEN              47
#define ECDMPCFGEX_2_SWVER_STR_DEF              NULL
#define ECDMPCFGEX_3_SWNAME_STR_LEN           47
#define ECDMPCFGEX_3_SWNAME_STR_DEF           NULL
#define ECDMPCFGEX_2_VOLTE_STR_LEN              7
#define ECDMPCFGEX_2_VOLTE_STR_DEF              NULL
#define ECDMPCFGEX_3_NET_STR_LEN             19
#define ECDMPCFGEX_3_NET_STR_DEF             NULL
#define ECDMPCFGEX_4_ACCOUNT_STR_LEN              47
#define ECDMPCFGEX_4_ACCOUNT_STR_DEF              NULL
#define ECDMPCFGEX_5_PHONENUM_STR_LEN           19
#define ECDMPCFGEX_5_PHONENUM_STR_DEF           NULL
#define ECDMPCFGEX_2_LOCATION_STR_LEN              255
#define ECDMPCFGEX_2_LOCATION_STR_DEF              NULL

#define ECDMPCONFIG_RSPBUFF_LEN                256
#define ECDMPCFG_RSPBUFF_LEN                   256
#define ECDMPCFGEX_RSPBUFF_LEN                 256

struct logLevel_t {
  UINT16 type;
  CHAR *name;
};

CmsRetId refNRB(const AtCmdInputContext *pAtCmdReq);
CmsRetId refNLOGLEVEL(const AtCmdInputContext *pAtCmdReq);
CmsRetId refNITZ(const AtCmdInputContext *pAtCmdReq);
CmsRetId refQLEDMODE(const AtCmdInputContext *pAtCmdReq);
CmsRetId refNATSPEED(const AtCmdInputContext *pAtCmdReq);
CmsRetId refECDMPCONFIG(const AtCmdInputContext *pAtCmdReq);
CmsRetId refECDMPCFG(const AtCmdInputContext *pAtCmdReq);
CmsRetId refECDMPCFGEX(const AtCmdInputContext *pAtCmdReq);

#endif

