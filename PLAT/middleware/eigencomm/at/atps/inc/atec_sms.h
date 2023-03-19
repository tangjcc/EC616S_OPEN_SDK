/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_sms.h
*
*  Description: SMS AT commands implementation
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef _ATEC_SMS_H
#define _ATEC_SMS_H

#include "commontypedef.h"
#include "ps_sms_if.h"

/******************************************************************************
 *****************************************************************************
 * AT SMS COMMON MARCO
 *****************************************************************************
******************************************************************************/
#define ATC_MAX_SMS_TDPU_SIZE           (232)

typedef enum atecMtSmsType_enum
{
    ATEC_MT_DELIVER_SMS = 0,
    ATEC_MT_SUBMIT_SMS = 1,
    ATEC_MT_COMMAND_SMS = 2,
    ATEC_MT_STATUS_REPORT_SMS = ATEC_MT_COMMAND_SMS
}atecMtSmsType;

#define ATEC_MT_SMS_TYPE_MASK             0x03       /* SMS Message-Type Indicator mask */





/* AT+CSMS */
#define ATC_CSMS_0_SERVICE_VAL_MIN                   0
#define ATC_CSMS_0_SERVICE_VAL_MAX                   127
#define ATC_CSMS_0_SERVICE_VAL_DEFAULT               0

/* AT+CPMS */
#define ATC_CPMS_0_MEM1_STR_MAX_LEN              3
#define ATC_CPMS_0_MEM1_STR_DEFAULT              NULL
#define ATC_CPMS_1_MEM2_STR_MAX_LEN                3
#define ATC_CPMS_1_MEM2_STR_DEFAULT                NULL
#define ATC_CPMS_2_MEM3_STR_MAX_LEN              3
#define ATC_CPMS_2_MEM3_STR_DEFAULT              NULL

/* AT+CSCA */
#define ATC_CSCA_0_ADDR_STR_MAX_LEN             21
#define ATC_CSCA_0_ADDR_STR_DEFAULT             NULL
#define ATC_CSCA_1_TYPE_VAL_MIN                 0
#define ATC_CSCA_1_TYPE_VAL_MAX                 255
#define ATC_CSCA_1_TYPE_VAL_DEFAULT             PSIL_SMS_TOA_OCT_NUMBER_UNKNOWN

/* AT+CNMI */
#define ATC_CNMI_0_MODE_VAL_MIN                   0
#define ATC_CNMI_0_MODE_VAL_MAX                   3
#define ATC_CNMI_0_MODE_VAL_DEFAULT               0
#define ATC_CNMI_1_MT_VAL_MIN                        0
#define ATC_CNMI_1_MT_VAL_MAX                        3
#define ATC_CNMI_1_MT_VAL_DEFAULT                    0
#define ATC_CNMI_2_BM_VAL_MIN                     0
#define ATC_CNMI_2_BM_VAL_MAX                     3
#define ATC_CNMI_2_BM_VAL_DEFAULT                 0
#define ATC_CNMI_3_DS_VAL_MIN                        0
#define ATC_CNMI_3_DS_VAL_MAX                        2
#define ATC_CNMI_3_DS_VAL_DEFAULT                    0
#define ATC_CNMI_4_BFR_VAL_MIN                    0
#define ATC_CNMI_4_BFR_VAL_MAX                    1
#define ATC_CNMI_4_BFR_VAL_DEFAULT                0

/* AT+CMGF */
#define ATC_CMGF_0_MODE_VAL_MIN                   0
#define ATC_CMGF_0_MODE_VAL_MAX                   1
#define ATC_CMGF_0_MODE_VAL_DEFAULT               0

/* AT+CMGL */
#define ATC_CMGL_0_MODE_STR_MAX_LEN              75
#define ATC_CMGL_0_MODE_STR_DEFAULT              NULL
#define ATC_CMGL_1_STATE_VAL_MIN                  0
#define ATC_CMGL_1_STATE_VAL_MAX                  255
#define ATC_CMGL_1_STATE_VAL_DEFAULT              255

/* AT+CMGR */
#define ATC_CMGR_0_MSG_VAL_MIN                   0
#define ATC_CMGR_0_MSG_VAL_MAX                   255
#define ATC_CMGR_0_MSG_VAL_DEFAULT               0

/* AT+CMGS */
#define ATC_CMGS_0_PDU_LENGTH_MIN               1
#define ATC_CMGS_0_PDU_LENGTH_MAX               PSIL_SMS_MAX_PDU_SIZE
#define ATC_CMGS_0_PDU_LENGTH_DEFAULT           1


#define ATC_ADDR_STR_MAX_LEN                    (CMI_SMS_MAX_ADDRESS_LENGTH+1)
#define ATC_ADDR_STR_DEFAULT                    PNULL
#define ATC_ADDR_TYPE_OCT_MIN                   0
#define ATC_ADDR_TYPE_OCT_MAX                   255
#define ATC_ADDR_TYPE_OCT_DEFAULT               PSIL_SMS_TOA_OCT_NUMBER_UNKNOWN





/* AT+CMGW */
#define ATC_CMGW_0_VAL_MIN                   0
#define ATC_CMGW_0_VAL_MAX                   2
#define ATC_CMGW_0_VAL_DEFAULT               0
#define ATC_CMGW_1_STAT_VAL_MIN                  0
#define ATC_CMGW_1_STAT_VAL_MAX                  3
#define ATC_CMGW_1_STAT_VAL_DEFAULT              2
#define ATC_CMGW_2_MODE_STR_MAX_LEN          75
#define ATC_CMGW_2_MODE_STR_DEFAULT          "STO UNSENT"

/* AT+CMGD */
#define ATC_CMGD_0_MSG_VAL_MIN                   0
#define ATC_CMGD_0_MSG_VAL_MAX                   255
#define ATC_CMGD_0_MSG_VAL_DEFAULT               0
#define ATC_CMGD_1_DEL_VAL_MIN                     0
#define ATC_CMGD_1_DEL_VAL_MAX                     4
#define ATC_CMGD_1_DEL_VAL_DEFAULT                 0

/* AT+CMMS */
#define ATC_CMMS_0_MODE_VAL_MIN                   0
#define ATC_CMMS_0_MODE_VAL_MAX                   2
#define ATC_CMMS_0_MODE_VAL_DEFAULT               0

/* AT+CNMA */
#define ATC_CNMA_0_REPLY_VAL_MIN                   0
#define ATC_CNMA_0_REPLY_VAL_MAX                   2
#define ATC_CNMA_0_REPLY_VAL_DEFAULT               0

/* AT+CSMP */
#define ATC_CSMP_0_FO_VAL_MIN                    0
#define ATC_CSMP_0_FO_VAL_MAX                    255
//#define ATC_CSMP_0_FO_VAL_DEFAULT                1
#define ATC_CSMP_1_VP_VAL_MIN                      0
#define ATC_CSMP_1_VP_VAL_MAX                      255
//#define ATC_CSMP_1_VP_VAL_DEFAULT                  167
#define ATC_CSMP_2_PID_VAL_MIN                   0
#define ATC_CSMP_2_PID_VAL_MAX                   1
#define ATC_CSMP_2_PID_VAL_DEFAULT               0
#define ATC_CSMP_3_DCS_VAL_MIN                     0
#define ATC_CSMP_3_DCS_VAL_MAX                     255
#define ATC_CSMP_3_DCS_VAL_DEFAULT                 0

/* AT+CSDH */
#define ATC_CSDH_0_SHOW_VAL_MIN                   0
#define ATC_CSDH_0_SHOW_VAL_MAX                   1
#define ATC_CSDH_0_SHOW_VAL_DEFAULT               0

/* AT+CSAS */
#define ATC_CSAS_0_PROFILE_VAL_MIN                   0
#define ATC_CSAS_0_PROFILE_VAL_MAX                   255
#define ATC_CSAS_0_PROFILE_VAL_DEFAULT               0

/* AT+CRES */
#define ATC_CRES_0_PROFILE_VAL_MIN                   0
#define ATC_CRES_0_PROFILE_VAL_MAX                   255
#define ATC_CRES_0_PROFILE_VAL_DEFAULT               0

/* AT+CMGCT */
#define ATC_CMGCT_0_FO_VAL_MIN                   0
#define ATC_CMGCT_0_FO_VAL_MAX                   255
#define ATC_CMGCT_0_FO_VAL_DEFAULT               1
#define ATC_CMGCT_1_CT_VAL_MIN                     0
#define ATC_CMGCT_1_CT_VAL_MAX                     255
#define ATC_CMGCT_1_CT_VAL_DEFAULT                 0
#define ATC_CMGCT_2_PID_VAL_MIN                  0
#define ATC_CMGCT_2_PID_VAL_MAX                  255
#define ATC_CMGCT_2_PID_VAL_DEFAULT              0
#define ATC_CMGCT_3_MN_VAL_MIN                     0
#define ATC_CMGCT_3_MN_VAL_MAX                     255
#define ATC_CMGCT_3_MN_VAL_DEFAULT                 0
#define ATC_CMGCT_4_DA_VAL_MIN                   0
#define ATC_CMGCT_4_DA_VAL_MAX                   255
#define ATC_CMGCT_4_DA_VAL_DEFAULT               145
#define ATC_CMGCT_5_TODA_VAL_MIN                   0
#define ATC_CMGCT_5_TODA_VAL_MAX                   255
#define ATC_CMGCT_5_TODA_VAL_DEFAULT               145

/* AT+CMGCP */
#define ATC_CMGCP_0_LEN_VAL_MIN                   1
#define ATC_CMGCP_0_LEN_VAL_MAX                   255
#define ATC_CMGCP_0_LEN_VAL_DEFAULT               0
#define ATC_CMGCP_1_PDU_STR_MAX_LEN                 255
#define ATC_CMGCP_1_PDU_STR_DEFAULT                 NULL

/* AT+CMSS */
#define ATC_CMSS_0_INDEX_VAL_MIN                   0
#define ATC_CMSS_0_INDEX_VAL_MAX                   255
#define ATC_CMSS_0_INDEX_VAL_DEFAULT               0
#define ATC_CMSS_1_INDEX_VAL_MIN                     0
#define ATC_CMSS_1_INDEX_VAL_MAX                     255
#define ATC_CMSS_1_INDEX_VAL_DEFAULT                 0
#define ATC_CMSS_2_TODA_VAL_MIN                   0
#define ATC_CMSS_2_TODA_VAL_MAX                   255
#define ATC_CMSS_2_TODA_VAL_DEFAULT               0

/* AT+ECSMSSEND */
#define ATC_ECSMSSEND_0_MODE_VAL_MIN            0
#define ATC_ECSMSSEND_0_MODE_VAL_MAX            1
#define ATC_ECSMSSEND_0_MODE_VAL_DEFAULT        0xff



/******************************************************************************
 *****************************************************************************
 * STRUCT
 *****************************************************************************
******************************************************************************/



/******************************************************************************
 *****************************************************************************
 * API
 *****************************************************************************
******************************************************************************/
CmsRetId  smsCSCA(const AtCmdInputContext *pAtCmdReq);
CmsRetId  smsCMGS(const AtCmdInputContext *pAtCmdReq);
CmsRetId  smsCMGC(const AtCmdInputContext *pAtCmdReq);
CmsRetId  smsCMGF(const AtCmdInputContext *pAtCmdReq);
CmsRetId  smsECSMSSEND(const AtCmdInputContext *pAtCmdReq);
CmsRetId  smsCSMP(const AtCmdInputContext *pAtCmdReq);
CmsRetId  smsCSMS(const AtCmdInputContext *pAtCmdReq);
CmsRetId  smsCNMA(const AtCmdInputContext *pAtCmdReq);
CmsRetId smsCMGSCMGCInputData(UINT8 chanId, UINT8 *pInput, UINT16 length);
void smsCMGSCMGCCancel(void);
CmsRetId  smsCMMS(const AtCmdInputContext *pAtCmdReq);



#endif

