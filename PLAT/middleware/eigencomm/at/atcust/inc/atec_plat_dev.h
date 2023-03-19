/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_plat_dev.h
*
*  Description: Device debug related AT CMD
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef __ATEC_DEBUG_H__
#define __ATEC_DEBUG_H__

#include "at_util.h"

#include "app.h"	// 20210926 add to support dahua qfashion1

#define EC_CMD_BUF_LEN          640
#define EC_PRINT_BUF_LEN        72
#define EC_DUMP_DATA_LEN        32
#define EC_DUMP_DATA_BLOCK      8192
#define ATC_ECRFTEST_STR_MAX_LEN           5000


/* AT+ECUNITTEST */
#define ATC_ECUNITTEST_0_VAL_MIN                0
#define ATC_ECUNITTEST_0_VAL_MAX                16
#define ATC_ECUNITTEST_0_VAL_DEFAULT            0  /* full functionality */



#if 0
/* AT+TESTCMD */
#define ATC_TESTCMD_0_VAL_MIN                0
#define ATC_TESTCMD_0_VAL_MAX                512
#define ATC_TESTCMD_0_VAL_DEFAULT            0  /* full functionality */
#define ATC_TESTCMD_0_STR_DEFAULT          NULL
#define ATC_TESTCMD_0_STR_MAX_LEN          888         /* */
#endif
/*AT+ECRST*/
#define ATC_ECRST_MAX_DELAY_MS              2000

/* AT+ECSYSTEST */
#define ATC_SYSTEST_0_VAL_MIN               0
#define ATC_SYSTEST_0_VAL_MAX               512
#define ATC_SYSTEST_0_VAL_DEFAULT           0  /* full functionality */
#define ATC_SYSTEST_0_STR_DEFAULT           NULL
#define ATC_SYSTEST_0_STR_MAX_LEN           64

/* AT+ECPMUCFG */
#define ATC_ECPMUCFG_0_VAL_MIN                0
#define ATC_ECPMUCFG_0_VAL_MAX                1
#define ATC_ECPMUCFG_0_VAL_DEFAULT            0  /* full functionality */
#define ATC_ECPMUCFG_1_VAL_MIN                0
#define ATC_ECPMUCFG_1_VAL_MAX                5
#define ATC_ECPMUCFG_1_VAL_DEFAULT            0  /* full functionality */

/* AT+ECPCFG */
#define ATC_ECPCFG_MAX_PARM_STR_LEN             32
#define ATC_ECPCFG_MAX_PARM_STR_DEFAULT         NULL

#define ATC_ECPCFG_VAL_MIN                0
#define ATC_ECPCFG_VAL_MAX                0xffff
#define ATC_ECPCFG_VAL_DEFAULT            0  /* full functionality */

#define ATC_ECPCFG_WDT_VAL_MIN                0
#define ATC_ECPCFG_WDT_VAL_MAX                1
#define ATC_ECPCFG_WDT_VAL_DEFAULT            0  /* full functionality */

#define ATC_ECPCFG_AT_PORT_DUMP_VAL_MIN                0
#define ATC_ECPCFG_AT_PORT_DUMP_VAL_MAX                1
#define ATC_ECPCFG_AT_PORT_DUMP_VAL_DEFAULT            0  /* full functionality */

#define ATC_ECPCFG_UNI_CTRL_VAL_MIN                0
#define ATC_ECPCFG_UNI_CTRL_VAL_MAX                2
#define ATC_ECPCFG_UNI_CTRL_VAL_DEFAULT            2  /* full functionality */

#define ATC_ECPCFG_UNI_LEVEL_VAL_MIN                0
#define ATC_ECPCFG_UNI_LEVEL_VAL_MAX                5
#define ATC_ECPCFG_UNI_LEVEL_VAL_DEFAULT            0  /* full functionality */

#define ATC_ECPCFG_LOG_BAUDRATE_VAL_MIN                921600
#define ATC_ECPCFG_LOG_BAUDRATE_VAL_MAX                6000001
#define ATC_ECPCFG_LOG_BAUDRATE_VAL_DEFAULT            6000000  /* full functionality */

#define ATC_ECPCFG_SLEEP_VAL_MIN                0
#define ATC_ECPCFG_SLEEP_VAL_MAX                0xffff
#define ATC_ECPCFG_SLEEP_VAL_DEFAULT            0  /* full functionality */

#define ATC_ECPCFG_FAULT_VAL_MIN                EXCEP_OPTION_DUMP_FLASH_EPAT_LOOP
#define ATC_ECPCFG_FAULT_VAL_MAX                (EXCEP_OPTION_MAX)
#define ATC_ECPCFG_FAULT_VAL_DEFAULT            EXCEP_OPTION_DUMP_FLASH_EPAT_LOOP  /* full functionality */

#define ATC_TASKINFO_LEN                64

/* AT+IPR */
#define ATC_IPR_MAX_PARM_STR_LEN             32
#define ATC_IPR_MAX_PARM_STR_DEFAULT         NULL

/* AT+ECLEDMODE */
#define ATC_ECLED_MODE_VAL_MIN               0
#define ATC_ECLED_MODE_VAL_MAX               1
#define ATC_ECLED_MODE_VAL_DEFAULT           0

/* AT+ECFLASHMONITORINFO */
#define ATC_ECFLASHMONITORINFO_VAL_MIN            0
#define ATC_ECFLASHMONITORINFO_VAL_MAX            3
#define ATC_ECFLASHMONITORINFO_VAL_DEFAULT        1

/* AT+ECPURC */
#define ATC_ECPURC_0_MAX_PARM_STR_LEN                 16
#define ATC_ECPURC_0_MAX_PARM_STR_DEFAULT             NULL
#define ATC_ECPURC_1_VAL_MIN                 0
#define ATC_ECPURC_1_VAL_MAX                 1
#define ATC_ECPURC_1_VAL_DEFAULT             0

/* AT+ICF */
#define ATC_ICF_FORMAT_VAL_MIN               1
#define ATC_ICF_FORMAT_VAL_MAX               6
#define ATC_ICF_FORMAT_VAL_DEFAULT           3

#define ATC_ICF_PARITY_VAL_MIN               0
#define ATC_ICF_PARITY_VAL_MAX               1
#define ATC_ICF_PARITY_VAL_DEFAULT           0

/* AT+ABUPFOTAFLAG */
#define ATC_ABUPFOTAFLAG_MAX_PARM_STR_LEN             16
#define ATC_ABUPFOTAFLAG_MAX_PARM_STR_DEFAULT         NULL

#define PRINTF_BUF_LEN 512
#define ECFSINFO_PRINT_BUF_LEN     (256)

#ifdef CUSTOMER_DAHUA
/*AT+NYVT*/
#define ATC_NYVT_VAL_MIN                0
#define ATC_NYVT_VAL_MAX                1
#define ATC_NYVT_VAL_DEFAULT            0
#endif

//CmsRetId pdevHELP(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevRST(const AtCmdInputContext *pAtCmdReq);

CmsRetId pdevFACTORYTEST(const AtCmdInputContext *pAtCmdReq);

//CmsRetId pdevPOWERON(const AtCmdInputContext *pAtCmdReq);
//CmsRetId pdevPOWEROFF(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECTASKINFO(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECTASKHISTINFO(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECSHOWMEM(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECFSINFO(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECFSFORMAT(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECFLASHMONITORINFO(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECUNITTEST(const AtCmdInputContext *pAtCmdReq);
//CmsRetId pdevDUMPDATA(const AtCmdInputContext *pAtCmdReq);
//CmsRetId pdevTESTCMD(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevSYSTEST(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECSYSTEST(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECLOGDBVER(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECPCFG(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECPMUCFG(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECVOTECHK(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevIPR(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevNetLight(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevPMUSTATUS(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECPURC(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevICF(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevECABFOTACTL(const AtCmdInputContext *pAtCmdReq);

#ifdef CUSTOMER_DAHUA
CmsRetId pdevNYSM(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevNYGOSLEEP(const AtCmdInputContext *pAtCmdReq);
CmsRetId pdevNYVT(const AtCmdInputContext *pAtCmdReq);
#endif

#endif

/* END OF FILE */
