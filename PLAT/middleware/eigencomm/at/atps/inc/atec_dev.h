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
#ifndef _ATEC__DEV_H
#define _ATEC__DEV_H

#include "at_util.h"

#include "app.h" // 20210926 add to support dahua qfashion1

/* AT+CFUN */
#define ATC_CFUN_0_FUN_VAL_MIN                0
#define ATC_CFUN_0_FUN_VAL_MAX                4
#define ATC_CFUN_0_FUN_VAL_DEFAULT            1
#define ATC_CFUN_1_RST_VAL_MIN                0
#define ATC_CFUN_1_RST_VAL_MAX                1
#define ATC_CFUN_1_RST_VAL_DEFAULT            0

/* AT+CPAS */
#define ATC_CPAS_0_VAL_MIN                   0
#define ATC_CPAS_0_VAL_MAX                   2
#define ATC_CPAS_0_VAL_DEFAULT               0

/* AT+ECBAND */
#define ATC_ECBAND_0_NW_MODE_VAL_MIN              0
#define ATC_ECBAND_0_NW_MODE_VAL_MAX              4
#define ATC_ECBAND_0_NW_MODE_VAL_DEFAULT          2
#define ATC_ECBAND_1_BAND_VAL_MIN                 0
#define ATC_ECBAND_1_BAND_VAL_MAX                 85
#define ATC_ECBAND_1_BAND_VAL_DEFAULT             0

/* AT+ECFREQ */
#define ATC_ECFREQ_0_NW_MODE_VAL_MIN                0
#define ATC_ECFREQ_0_NW_MODE_VAL_MAX                3
#define ATC_ECFREQ_0_NW_MODE_VAL_DEFAULT            0
#define ATC_ECFREQ_1_EARFCN_VAL_MIN                 0
#define ATC_ECFREQ_1_EARFCN_VAL_MAX                 0x7FFFFFFF
#define ATC_ECFREQ_1_EARFCN_VAL_DEFAULT             0
#define ATC_ECFREQ_2_PHYCELL_VAL_MIN                0
#define ATC_ECFREQ_2_PHYCELL_VAL_MAX                503
#define ATC_ECFREQ_2_PHYCELL_VAL_DEFAULT            0

/* AT+ECCGSN */
#define ATC_CGSN_0_MAX_PARM_STR_LEN                16
#define ATC_CGSN_0_MAX_PARM_STR_DEFAULT            NULL
#define ATC_CGSN_1_MAX_PARM_STR_LEN                32
#define ATC_CGSN_1_MAX_PARM_STR_DEFAULT            NULL

/* AT+ECCGSNLOCK */
#define ATC_ECCGSNLOCK_0_IMEI_STR_LEN                16
#define ATC_ECCGSNLOCK_0_IMEI_STR_DEFAULT            "imeiLock"
#define ATC_ECCGSNLOCK_0_SN_STR_LEN                16
#define ATC_ECCGSNLOCK_0_SN_STR_DEFAULT            "snLock"


/* AT+CIOTPOWER */
#define ATC_CIOTPOWER_0_VAL_MIN                   0
#define ATC_CIOTPOWER_0_VAL_MAX                   2
#define ATC_CIOTPOWER_0_VAL_DEFAULT               0


/* AT+ECCFG */
#define ATC_ECCFG_0_MAX_PARM_STR_LEN                 32
#define ATC_ECCFG_0_MAX_PARM_STR_DEFAULT             NULL
#define ATC_ECCFG_1_GCFTEST_VAL_MIN                  0
#define ATC_ECCFG_1_GCFTEST_VAL_MAX                  1
#define ATC_ECCFG_1_GCFTEST_VAL_DEFAULT              (-1)
#define ATC_ECCFG_1_AUTOAPN_VAL_MIN                  0
#define ATC_ECCFG_1_AUTOAPN_VAL_MAX                  1
#define ATC_ECCFG_1_AUTOAPN_VAL_DEFAULT              (-1)
#define ATC_ECCFG_1_SUPPORTSMS_VAL_MIN               0
#define ATC_ECCFG_1_SUPPORTSMS_VAL_MAX               1
#define ATC_ECCFG_1_SUPPORTSMS_VAL_DEFAULT           (-1)
#define ATC_ECCFG_1_TAUFORSMS_VAL_MIN                0
#define ATC_ECCFG_1_TAUFORSMS_VAL_MAX                1
#define ATC_ECCFG_1_TAUFORSMS_VAL_DEFAULT            (-1)
#define ATC_ECCFG_1_T3324_VAL_MIN                    0
#define ATC_ECCFG_1_T3324_VAL_MAX                    0xFFFFFF
#define ATC_ECCFG_1_T3324_VAL_DEFAULT                0xFFFFFF
#define ATC_ECCFG_1_BAR_VAL_MIN                      1
#define ATC_ECCFG_1_BAR_VAL_MAX                      600
#define ATC_ECCFG_1_BAR_VAL_DEFAULT                  120
#define ATC_ECCFG_1_SIMTEST_VAL_MIN                  0
#define ATC_ECCFG_1_SIMTEST_VAL_MAX                  1
#define ATC_ECCFG_1_SIMTEST_VAL_DEFAULT              (-1)
#define ATC_ECCFG_1_USIMSIMULATOR_VAL_MIN            0
#define ATC_ECCFG_1_USIMSIMULATOR_VAL_MAX            1
#define ATC_ECCFG_1_USIMSIMULATOR_VAL_DEFAULT        (-1)
#define ATC_ECCFG_1_SUPPORTUPRAI_VAL_MIN             0
#define ATC_ECCFG_1_SUPPORTUPRAI_VAL_MAX             1
#define ATC_ECCFG_1_SUPPORTUPRAI_VAL_DEFAULT         (-1)
#define ATC_ECCFG_1_DIT_VAL_MIN                      0
#define ATC_ECCFG_1_DIT_VAL_MAX                      0xff
#define ATC_ECCFG_1_DIT_VAL_DEFAULT                  (-1)
#define ATC_ECCFG_1_RMD_VAL_MIN                      0
#define ATC_ECCFG_1_RMD_VAL_MAX                      17
#define ATC_ECCFG_1_RMD_VAL_DEFAULT                  (-1)
#define ATC_ECCFG_1_POWERLEVEL_VAL_MIN               0
#define ATC_ECCFG_1_POWERLEVEL_VAL_MAX               4
#define ATC_ECCFG_1_POWERLEVEL_VAL_DEFAULT           (-1)
#define ATC_ECCFG_1_REL_VERSION_VAL_MIN              13
#define ATC_ECCFG_1_REL_VERSION_VAL_MAX              14
#define ATC_ECCFG_1_REL_VERSION_VAL_DEFAULT          13
#define ATC_ECCFG_1_ROHC_VAL_MIN                     0
#define ATC_ECCFG_1_ROHC_VAL_MAX                     1
#define ATC_ECCFG_1_ROHC_VAL_DEFAULT                 (-1)
#define ATC_ECCFG_1_EPCO_VAL_MIN                     0
#define ATC_ECCFG_1_EPCO_VAL_MAX                     1
#define ATC_ECCFG_1_EPCO_VAL_DEFAULT                 (-1)
#define ATC_ECCFG_1_MULTICARRIER_VAL_MIN             0
#define ATC_ECCFG_1_MULTICARRIER_VAL_MAX             1
#define ATC_ECCFG_1_MULTICARRIER_VAL_DEFAULT         (-1)
#define ATC_ECCFG_1_MULTITONE_VAL_MIN                0
#define ATC_ECCFG_1_MULTITONE_VAL_MAX                1
#define ATC_ECCFG_1_MULTITONE_VAL_DEFAULT            (-1)
#define ATC_ECCFG_1_IPV6RSFORTESTSIM_VAL_MIN         0
#define ATC_ECCFG_1_IPV6RSFORTESTSIM_VAL_MAX         1
#define ATC_ECCFG_1_IPV6RSFORTESTSIM_VAL_DEFAULT     (-1)
#define ATC_ECCFG_1_POWERCFUN_VAL_MIN               0
#define ATC_ECCFG_1_POWERCFUN_VAL_MAX               4
#define ATC_ECCFG_1_POWERCFUN_VAL_DEFAULT           1   //CFUN1
#define ATC_ECCFG_1_PSPOWERONMAXDEALY_VAL_MIN        0
#define ATC_ECCFG_1_PSPOWERONMAXDEALY_VAL_MAX        0xFFFF
#define ATC_ECCFG_1_PSPOWERONMAXDEALY_VAL_DEFAULT    0
#define ATC_ECCFG_1_IPV6RSDELAY_VAL_MIN              0
#define ATC_ECCFG_1_IPV6RSDELAY_VAL_MAX              65535
#define ATC_ECCFG_1_IPV6RSDELAY_VAL_DEFAULT          (15)
#define ATC_ECCFG_1_DISABLENCELLMEAS_VAL_MIN         (0)
#define ATC_ECCFG_1_DISABLENCELLMEAS_VAL_MAX         (1)
#define ATC_ECCFG_1_DISABLENCELLMEAS_VAL_DEFAULT     (0)
#define ATC_ECCFG_1_NBCATEGORY_VAL_MIN               (1)
#define ATC_ECCFG_1_NBCATEGORY_VAL_MAX               (2)
#define ATC_ECCFG_1_NBCATEGORY_VAL_DEFAULT           (1)

#ifdef  FEATURE_REF_AT_ENABLE
#define ATC_ECCFG_1_CFUN1POWERONPS_VAL_MIN           (0)
#define ATC_ECCFG_1_CFUN1POWERONPS_VAL_MAX           (1)
#define ATC_ECCFG_1_CFUN1POWERONPS_VAL_DEFAULT       (0)
#endif

#define ATC_ECCFG_1_PSSOFTRESET_VAL_MIN           (0)
#define ATC_ECCFG_1_PSSOFTRESET_VAL_MAX           (1)
#define ATC_ECCFG_1_PSSOFTRESET_VAL_DEFAULT       (0)

#define ATC_ECCFG_1_DISABLESIB14CHECK_VAL_MIN           (0)
#define ATC_ECCFG_1_DISABLESIB14CHECK_VAL_MAX           (1)
#define ATC_ECCFG_1_DISABLESIB14CHECK_VAL_DEFAULT       (0)

#define ATC_ECCFG_1_SIMBIP_VAL_MIN           (0)
#define ATC_ECCFG_1_SIMBIP_VAL_MAX           (1)
#define ATC_ECCFG_1_SIMBIP_VAL_DEFAULT       (0)

/* AT+ECSTATUS */
#define ATC_ECSTATUS_0_MAX_PARM_STR_LEN              16
#define ATC_ECSTATUS_0_MAX_PARM_STR_DEFAULT          NULL


/* AT+ECRMFPLMN */
#define ATC_ECRMFPLMN_0_VAL_MIN                 0
#define ATC_ECRMFPLMN_0_VAL_MAX                 2
#define ATC_ECRMFPLMN_0_VAL_DEFAULT             0

/*AT+ECBCINFO*/
#define ATC_ECBCINFO_0_VAL_MIN                  0
#define ATC_ECBCINFO_0_VAL_MAX                  2
#define ATC_ECBCINFO_0_VAL_DEFAULT              0

#define ATC_ECBCINFO_1_VAL_MIN                  4
#define ATC_ECBCINFO_1_VAL_MAX                  300     /*timeout, MAX: 5 mins*/
#define ATC_ECBCINFO_1_VAL_DEFAULT              8       /*default: 8s*/

#define ATC_ECBCINFO_2_VAL_MIN                  0
#define ATC_ECBCINFO_2_VAL_MAX                  1       /*save_for_later*/
#define ATC_ECBCINFO_2_VAL_DEFAULT              0       /*not need to save*/

#define ATC_ECBCINFO_3_VAL_MIN                  1       /*max_cell_number*/
#define ATC_ECBCINFO_3_VAL_MAX                  5       /*max_cell_number, 1 serving cell + 4 neighber cell */
#define ATC_ECBCINFO_3_VAL_DEFAULT              5

#define ATC_ECBCINFO_4_VAL_MIN                  0       /*report mode, 0 - report in AT response */
#define ATC_ECBCINFO_4_VAL_MAX                  1       /*report mode, 1 - report in URC response */
#define ATC_ECBCINFO_4_VAL_DEFAULT              0



/* AT+CMAR */
#define ATC_CMAR_0_VAL_MIN                   0
#define ATC_CMAR_0_VAL_MAX                   2
#define ATC_CMAR_0_VAL_DEFAULT               0

/* AT+CMOLR */
#define ATC_CMOLR_0_ENABLE_VAL_MIN                   0
#define ATC_CMOLR_0_ENABLE_VAL_MAX                   3
#define ATC_CMOLR_0_ENABLE_VAL_DEFAULT               0
#define ATC_CMOLR_1_METHOD_VAL_MIN                     0
#define ATC_CMOLR_1_METHOD_VAL_MAX                     6
#define ATC_CMOLR_1_METHOD_VAL_DEFAULT                 0
#define ATC_CMOLR_2_HORACCSET_VAL_MIN                0
#define ATC_CMOLR_2_HORACCSET_VAL_MAX                1
#define ATC_CMOLR_2_HORACCSET_VAL_DEFAULT            0
#define ATC_CMOLR_3_HORACC_VAL_MIN                     0
#define ATC_CMOLR_3_HORACC_VAL_MAX                     127
#define ATC_CMOLR_3_HORACC_VAL_DEFAULT                 0
#define ATC_CMOLR_4_VERREQ_VAL_MIN                   0
#define ATC_CMOLR_4_VERREQ_VAL_MAX                   1
#define ATC_CMOLR_4_VERREQ_VAL_DEFAULT               0
#define ATC_CMOLR_5_VERACCSET_VAL_MIN                  0
#define ATC_CMOLR_5_VERACCSET_VAL_MAX                  1
#define ATC_CMOLR_5_VERACCSET_VAL_DEFAULT              0
#define ATC_CMOLR_6_VERACC_VAL_MIN                   0
#define ATC_CMOLR_6_VERACC_VAL_MAX                   127
#define ATC_CMOLR_6_VERACC_VAL_DEFAULT               0
#define ATC_CMOLR_7_VELREQ_VAL_MIN                     0
#define ATC_CMOLR_7_VELREQ_VAL_MAX                     4
#define ATC_CMOLR_7_VELREQ_VAL_DEFAULT                 0
#define ATC_CMOLR_8_REQMODE_VAL_MIN                  0
#define ATC_CMOLR_8_REQMODE_VAL_MAX                  1
#define ATC_CMOLR_8_REQMODE_VAL_DEFAULT              0
#define ATC_CMOLR_9_TIMEOUT_VAL_MIN                    0
#define ATC_CMOLR_9_TIMEOUT_VAL_MAX                    65535
#define ATC_CMOLR_9_TIMEOUT_VAL_DEFAULT                0
#define ATC_CMOLR_10_INTERVAL_VAL_MIN                0
#define ATC_CMOLR_10_INTERVAL_VAL_MAX                65535
#define ATC_CMOLR_10_INTERVAL_VAL_DEFAULT            0
#define ATC_CMOLR_11_SHAPEREQ_VAL_MIN                  1
#define ATC_CMOLR_11_SHAPEREQ_VAL_MAX                  64
#define ATC_CMOLR_11_SHAPEREQ_VAL_DEFAULT              0
#define ATC_CMOLR_12_PLANE_VAL_MIN                   0
#define ATC_CMOLR_12_PLANE_VAL_MAX                   1
#define ATC_CMOLR_12_PLANE_VAL_DEFAULT               0
#define ATC_CMOLR_13_NMEAREQ_STR_DEFAULT               NULL
#define ATC_CMOLR_13_NMEAREQ_STR_MAX_LEN               16         /* ATCI_PLMN_LONG_NAME_LENGTH */
#define ATC_CMOLR_14_THIRDPARTYADDR_STR_DEFAULT      NULL
#define ATC_CMOLR_14_THIRDPARTYADDR_STR_MAX_LEN      16         /* ATCI_PLMN_LONG_NAME_LENGTH */

/* AT+CMTLR */
#define ATC_CMTLR_0_VAL_MIN                   0
#define ATC_CMTLR_0_VAL_MAX                   3
#define ATC_CMTLR_0_VAL_DEFAULT               0

/* AT+CMTLRA */
#define ATC_CMTLRA_0_VAL_MIN                   0
#define ATC_CMTLRA_0_VAL_MAX                   1
#define ATC_CMTLRA_0_VAL_DEFAULT               0
#define ATC_CMTLRA_1_VAL_MIN                   0
#define ATC_CMTLRA_1_VAL_MAX                   255
#define ATC_CMTLRA_1_VAL_DEFAULT               0

/*AT+ECSTATIS*/
#define ATC_ESTATIS_0_VAL_MIN                   0
#define ATC_ESTATIS_0_VAL_MAX                   600
#define ATC_ESTATIS_0_VAL_DEFAULT               0

/*AT+ECCGSN*/
#define ATC_ECCGSN_MAX_PARM_STR_LEN             8
#define ATC_ECCGSN_MAX_PARM_STR_DEFAULT         NULL

/* AT+ECPSTEST */
#define ATC_ECPSTEST_VAL_MIN                    0
#define ATC_ECPSTEST_VAL_MAX                    1
#define ATC_ECPSTEST_VAL_DEFAULT                0

/* AT+ECPOWERCLASS */
#define ATC_ECPOWERCLASS_1_VAL_MIN              0
#define ATC_ECPOWERCLASS_1_VAL_MAX              85
#define ATC_ECPOWERCLASS_1_VAL_DEFAULT          0

#define ATC_ECPOWERCLASS_2_VAL_MIN              3
#define ATC_ECPOWERCLASS_2_VAL_MAX              6
#define ATC_ECPOWERCLASS_2_VAL_DEFAULT          3

/* AT+ECEVENTSTATIS */
#define ATC_ECEVENTSTATIS_1_VAL_MIN              0
#define ATC_ECEVENTSTATIS_1_VAL_MAX              2
#define ATC_ECEVENTSTATIS_1_VAL_DEFAULT          0

/* AT+ECPSSLPCFG */
#define ATC_ECPSSLPCFG_0_MAX_PARM_STR_LEN       32
#define ATC_ECPSSLPCFG_0_MAX_PARM_STR_DEFAULT   NULL

#define ATC_ECPSSLPCFG_1_VAL_MIN                0
#define ATC_ECPSSLPCFG_1_VAL_MAX                0xFFFF
#define ATC_ECPSSLPCFG_1_VAL_DEFAULT            0

#define ATC_ECPSSLPCFG_2_VAL_MIN                0
#define ATC_ECPSSLPCFG_2_VAL_MAX                0xFFFFFF
#define ATC_ECPSSLPCFG_2_VAL_DEFAULT            0

#define ATC_ECPSSLPCFG_3_VAL_MIN                0
#define ATC_ECPSSLPCFG_3_VAL_MAX                0xFFFF
#define ATC_ECPSSLPCFG_3_VAL_DEFAULT            0


CmsRetId  devCFUN(const AtCmdInputContext *pAtCmdReq);
//CmsRetId  devCPAS(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devECBAND(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devECFREQ(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devCGSN(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devECCGSN(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devECCGSNLOCK(const AtCmdInputContext *pAtCmdReq);
//CmsRetId  devCIOTPOWER(const UINT32 operaType, const AtParamValueCP pParamList, const size_t paramNumb, const CHAR *infoPtr, UINT32 *xidPtr, void *argPtr);
//CmsRetId  devNBAND(const AtCmdInputContext *pAtCmdReq);
//CmsRetId  devNCONFIG(const AtCmdInputContext *pAtCmdReq);
//CmsRetId  devNUESTATS(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devECCFG(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devECRMFPLMN(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devCMAR(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devCMOLR(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devCMTLR(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devCMTLRA(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devECSTATUS(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devECSTATIS(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devECBCINFO(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devECPSTEST(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devECPOWERCLASS(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devECEVENTSTATIS(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devECNBR14(const AtCmdInputContext *pAtCmdReq);
CmsRetId  devECPSSLPCFG(const AtCmdInputContext *pAtCmdReq);

#ifdef CUSTOMER_DAHUA
CmsRetId  pdevHXSTATUS(const AtCmdInputContext *pAtCmdReq);
#endif

#endif

