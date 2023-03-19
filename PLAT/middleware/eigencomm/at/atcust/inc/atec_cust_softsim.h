
/******************************************************************************

*(C) Copyright 2020 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename:
*
*  Description: esim AT commands interface function
*
*  History:     09/22/2020  yjwang create
*
*  Notes:    NULL
*
******************************************************************************/
#ifdef SOFTSIM_CT_ENABLE

#ifndef _ATEC_CUST_SOFTSIM_H_
#define _ATEC_CUST_SOFTSIM_H_


/*AT+ESIM*/
#define ATC_ESIM_0_FUN_VAL_MIN                     0
#define ATC_ESIM_0_FUN_VAL_MAX                     14
#define ATC_ESIM_0_FUN_VAL_DEFAULT                 0

#define ESIM_DATA_CONFIG_MAX_LEN                   1024

typedef enum
{
    ESIM_OPERATE_ID_CSR_SERVICE = 0,
    ESIM_OPERATE_ID_CERT_SERVICE,
    ESIM_OPERATE_ID_VENDER_ID,
    ESIM_OPERATE_ID_UICC_SERVICE = 10,
    ESIM_OPERATE_ID_CHALLENGE_SERVICE,
    ESIM_OPERATE_ID_AUTHSERREQ_SERVICE,
    ESIM_OPERATE_ID_PREDOWNLOAD_SERVICE,
    ESIM_OPERATE_ID_LOADBOUNDPROFILE_SERVICE,
    ESIM_OPERATE_ID_MAX_SERVICE =  255
}ESIM_OPERATE_ITEM_ID;

typedef enum
{
    ESIM_ATCONFIG_STATE_IDLE = 0,
    ESIM_ATCONFIG_STATE_WORKING,
    ESIM_ATCONFIG_STATE_INVALID,
}ESIM_ATCONFIG_STATE_T;

CmsRetId esimDATACONFIG(const AtCmdInputContext *pAtCmdReq);

#endif
#endif


