/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.h
 * Description:  EC616S at command demo entry header file
 * History:      Rev1.0   2018-07-12
 *
 ****************************************************************************/

#ifndef APP_H
#define APP_H

//#define ATCMD_NCONFIG_READ 	1
//#define ATCMD_NBAND_TEST	2
//#define ATCMD_NBAND_READ	3
//#define ATCMD_NCCID_READ	4
//#define ATCMD_NUESTATS_SET	5

//void userAtCmdSemRelease(uint32_t cmd);

//#define FIRMWARE_VERSION "SLM_100_1.0.1_EQ00A_001_20210902"

#include "app_user_atcmd.h"
#include "app_uart2_handle.h"

#define SDKVERSION	"1.37.1.8"
#define MVERSION	"M001"

#if (USE_MODULAR == MODULAR_SLM100)
#define FIRMWARE_VERSION "SLM100\r\nSLM100.1.37.1.8T64S0918_M001_EC616S" 

#define MODULE 		"SLM100"
#define TVERSION 	"T65"
#define SDATE		"S0928"

//#elif (USE_MODULAR == MODULAR_SLM130)
//#define FIRMWARE_VERSION "SLM130\r\nSLM_130_1.0.3_EQ00A_001_20210906"

#elif (USE_MODULAR == MODULAR_SLM160)
#define FIRMWARE_VERSION "SLM160\r\nSLM160.1.37.1.8T64S0918_M001_EC616S"

#define MODULE 		"SLM160"
#define TVERSION 	"T65"
#define SDATE		"S0928"

#elif (USE_MODULAR == MODULAR_SLM190)
#define FIRMWARE_VERSION "SLM190\r\nSLM190.1.37.1.8T64S0918_M001_EC616S"

#define MODULE 		"SLM190"
#define TVERSION 	"T66"
#define SDATE		"S1013"

#endif

#endif

