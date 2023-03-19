/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_ref_ps.h
*
*  Description: Process protocol related AT CMD
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef _ATEC_REF_PS_H_
#define _ATEC_REF_PS_H_

#include "at_util.h"



/* AT+NBAND */
#define ATC_NBAND_0_BAND_VAL_MIN                 0
#define ATC_NBAND_0_BAND_VAL_MAX                 511
#define ATC_NBAND_0_BAND_VAL_DEFAULT             0


/* AT+NEARFCN */
#define ATC_NEARFCN_0_NW_MODE_VAL_MIN                0
#define ATC_NEARFCN_0_NW_MODE_VAL_MAX                0
#define ATC_NEARFCN_0_NW_MODE_VAL_DEFAULT            0
#define ATC_NEARFCN_1_EARFCN_VAL_MIN                 0
#define ATC_NEARFCN_1_EARFCN_VAL_MAX                 0x7FFFFFFF
#define ATC_NEARFCN_1_EARFCN_VAL_DEFAULT             0
#define ATC_NEARFCN_2_PHYCELL_VAL_MIN                0
#define ATC_NEARFCN_2_PHYCELL_VAL_MAX                503
#define ATC_NEARFCN_2_PHYCELL_VAL_DEFAULT            0


/* AT+NCONFIG */
#define ATC_NCFG_0_MAX_PARM_STR_LEN                 32
#define ATC_NCFG_0_MAX_PARM_STR_DEFAULT             NULL
#define ATC_NCFG_1_AUTO_CONNECT_STR_LEN_MAX         10
#define ATC_NCFG_1_AUTO_CONNECT_STR_DEFAULT         NULL
#define ATC_NCFG_1_CELL_RESEL_STR_LEN_MAX           10
#define ATC_NCFG_1_CELL_RESEL_STR_DEFAULT           NULL
#define ATC_NCFG_1_ENABLE_BIP_STR_LEN_MAX           10
#define ATC_NCFG_1_ENABLE_BIP_STR_DEFAULT           NULL
#define ATC_NCFG_1_MULTITONE_STR_LEN_MAX            10
#define ATC_NCFG_1_MULTITONE_STR_DEFAULT            NULL
#define ATC_NCFG_1_SIM_PSM_ENABLE_STR_LEN_MAX       10
#define ATC_NCFG_1_SIM_PSM_ENABLE_STR_DEFAULT       NULL
#define ATC_NCFG_1_REL_VERSION_VAL_MIN              13
#define ATC_NCFG_1_REL_VERSION_VAL_MAX              14
#define ATC_NCFG_1_REL_VERSION_VAL_DEFAULT          13
#define ATC_NCFG_1_IPV6_GET_PREFIX_TIME_VAL_MIN     0
#define ATC_NCFG_1_IPV6_GET_PREFIX_TIME_VAL_MAX     65535
#define ATC_NCFG_1_IPV6_GET_PREFIX_TIME_VAL_DEFAULT 15
#define ATC_NCFG_1_NB_CATEGORY_VAL_MIN              1
#define ATC_NCFG_1_NB_CATEGORY_VAL_MAX              2
#define ATC_NCFG_1_NB_CATEGORY_VAL_DEFAULT          1
#define ATC_NCFG_1_RAI_STR_LEN_MAX                  10
#define ATC_NCFG_1_RAI_STR_DEFAULT                  NULL
#define ATC_NCFG_1_HEAD_COMPRESS_STR_LEN_MAX        10
#define ATC_NCFG_1_HEAD_COMPRESS_STR_DEFAULT        NULL
#define ATC_NCFG_1_CONNECTION_REEST_STR_LEN_MAX     10
#define ATC_NCFG_1_CONNECTION_REEST_STR_DEFAULT     NULL
#define ATC_NCFG_1_PCO_IE_TYPE_STR_LEN_MAX          10
#define ATC_NCFG_1_PCO_IE_TYPE_STR_DEFAULT          NULL
#define ATC_NCFG_1_TWO_HARQ_STR_LEN_MAX             10
#define ATC_NCFG_1_TWO_HARQ_STR_DEFAULT             NULL
#define ATC_NCFG_1_HPPLMN_SEARCH_ENABLE_STR_LEN_MAX 10
#define ATC_NCFG_1_HPPLMN_SEARCH_ENABLE_STR_DEFAULT NULL
#define ATC_NCFG_1_T3324_T3412_EXT_CHANGE_RPT_STR_LEN_MAX  10
#define ATC_NCFG_1_T3324_T3412_EXT_CHANGE_RPT_STR_DEFAULT  NULL
#define ATC_NCFG_1_NON_IP_NO_SMS_ENABLE_STR_LEN_MAX 10
#define ATC_NCFG_1_NON_IP_NO_SMS_ENABLE_STR_DEFAULT NULL
#define ATC_NCFG_1_SUPPORT_SMS_STR_LEN_MAX          10
#define ATC_NCFG_1_SUPPORT_SMS_STR_DEFAULT          NULL


/* AT+NCPCDPR */
#define ATC_NCPCDPR_0_IP_TYPE_VAL_MIN                0
#define ATC_NCPCDPR_0_IP_TYPE_VAL_MAX                1
#define ATC_NCPCDPR_0_IP_TYPE_VAL_DEFAULT            0
#define ATC_NCPCDPR_1_READ_STATE_VAL_MIN             0
#define ATC_NCPCDPR_1_READ_STATE_VAL_MAX             1
#define ATC_NCPCDPR_1_READ_STATE_VAL_DEFAULT         0


/* AT+NUESTATS */
#define ATC_NUESTATS_0_MAX_PARM_STR_LEN              16
#define ATC_NUESTATS_0_MAX_PARM_STR_DEFAULT          NULL


/*AT+NPSMR*/
#define ATC_NPSMR_0_VAL_MIN                0
#define ATC_NPSMR_0_VAL_MAX                1
#define ATC_NPSMR_0_VAL_DEFAULT            0


/* AT+NPTWEDRXS */
#define ATC_NPTWEDRXS_0_MODE_VAL_MIN                0
#define ATC_NPTWEDRXS_0_MODE_VAL_MAX                3
#define ATC_NPTWEDRXS_0_MODE_VAL_DEFAULT            0
#define ATC_NPTWEDRXS_1_VAL_MIN                     0
#define ATC_NPTWEDRXS_1_VAL_MAX                     5
#define ATC_NPTWEDRXS_1_VAL_DEFAULT                 5
#define ATC_NPTWEDRXS_2_STR_DEFAULT                 NULL
#define ATC_NPTWEDRXS_2_STR_MAX_LEN                 4
#define ATC_NPTWEDRXS_3_STR_DEFAULT                 NULL
#define ATC_NPTWEDRXS_3_STR_MAX_LEN                 4

/* AT+ECPOWERCLASS */
#define ATC_NPOWERCLASS_1_VAL_MIN                   0
#define ATC_NPOWERCLASS_1_VAL_MAX                   85
#define ATC_NPOWERCLASS_1_VAL_DEFAULT               0

#define ATC_NPOWERCLASS_2_VAL_MIN                   3
#define ATC_NPOWERCLASS_2_VAL_MAX                   6
#define ATC_NPOWERCLASS_2_VAL_DEFAULT               3

/* AT+NPIN */
#define ATC_NPIN_0_VAL_MIN                   0
#define ATC_NPIN_0_VAL_MAX                   4
#define ATC_NPIN_0_VAL_DEFAULT               0
#define ATC_NPIN_1_STR_MIN_LEN              4
#define ATC_NPIN_1_STR_MAX_LEN              8
#define ATC_NPIN_1_STR_DEFAULT              NULL
#define ATC_NPIN_2_STR_MIN_LEN              4
#define ATC_NPIN_2_STR_MAX_LEN              8
#define ATC_NPIN_2_STR_DEFAULT              NULL

/* AT+NUICC */
#define ATC_NUICC_0_VAL_MIN                   0
#define ATC_NUICC_0_VAL_MAX                   1
#define ATC_NUICC_0_VAL_DEFAULT               0


/*
 * AT+NPSMR=<n>
 * <n>, bitmap of report level
 * bit[0]: 1 enable unsolicited result code +NPSMR
 *         0 disable unsolicited result code
*/
typedef enum AtcNPSMRValue_Enum
{
    ATC_NPSMR_DISABLE = 0,
    ATC_NPSMR_ENABLE
}AtcNPSMRValue;


/*
 * AT+NPTWEDRXS=2,XX,XX
 * <n>, bitmap of report level
 * bit[0]: 1 enable unsolicited result code +NPTWEDRXP
 *         0 disable unsolicited result code
*/
typedef enum AtcNPTWEDRXSValue_Enum
{
    ATC_NPTWEDRXS_DISABLE = 0,
    ATC_NPTWEDRXS_ENABLE
}AtcNPTWEDRXSValue;

/*
* AT+NPIN=<command>,xx,xx
* <command> refer to PIN operation mode
*/
typedef enum AtcNPINOperationMode_Enum
{
    ATC_NPIN_VERIFY_PIN = 0,
    ATC_NPIN_CHANGE_PIN,
    ATC_NPIN_ENABLE_PIN,
    ATC_NPIN_DISABLE_PIN,
    ATC_NPIN_UNBLOCK_PIN
}
AtcNPINOperationMode;


/******************************************************************************
 *****************************************************************************
 * EXTERNAL API
 *****************************************************************************
******************************************************************************/

CmsRetId refPsNEARFCN(const AtCmdInputContext *pAtCmdReq);
CmsRetId refPsNCCID(const AtCmdInputContext *pAtCmdReq);
CmsRetId refPsNCSEARFCN(const AtCmdInputContext *pAtCmdReq);
CmsRetId refPsNBAND(const AtCmdInputContext *pAtCmdReq);
CmsRetId refPsNCONFIG(const AtCmdInputContext *pAtCmdReq);
CmsRetId refPsNUESTATS(const AtCmdInputContext *pAtCmdReq);
CmsRetId refPsNCPCDPR(const AtCmdInputContext *pAtCmdReq);
CmsRetId refPsNPSMR(const AtCmdInputContext *pAtCmdReq);
CmsRetId refPsNPTWEDRXS(const AtCmdInputContext *pAtCmdReq);
CmsRetId refPsNPOWERCLASS(const AtCmdInputContext *pAtCmdReq);
CmsRetId refPsNPIN(const AtCmdInputContext *pAtCmdReq);
CmsRetId refPsNUICC(const AtCmdInputContext *pAtCmdReq);


#endif

