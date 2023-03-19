/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

*******************************************************************************
*  Filename: atec_ctlwm2m1_5.h
*
*  Description: at entry for ctlwm2m header file
*
*  History:
*
*  Notes: China Telecom LWM2M version 1.5
*
******************************************************************************/

#ifndef _ATEC_CTLWM2M_1_5_H
#define _ATEC_CTLWM2M_1_5_H

#include "at_util.h"

#define CTM2M_VERSION         "1.1.0"
#define CTM2M_CTMV            "NB1.5"
#define MAX_CTM2M_VER_LEN     (100)
#define MAX_RSP_LEN           (100)
#define MAX_NAME_LEN          (32)
#define MAX_PORT_LEN          (8)
#define MAX_CONTENT_LEN       (1024)
#define MAX_OBJINSSTR_LEN	  (60)
#define MAX_AUTHSTR_LEN		  (128)
#define MAX_TOKEN_LEN		  (16)
#define MAX_URISTR_LEN  	  (18)// /65535/65535/65535

/* AT+CTM2MSETPM= */
#define CTM2MSETPM_1_PORT_MIN                 1
#define CTM2MSETPM_1_PORT_MAX                 65535
#define CTM2MSETPM_1_PORT_DEF                 5683
#define CTM2MSETPM_2_LIFETIME_MIN             300
#define CTM2MSETPM_2_LIFETIME_MAX             0x7FFFFFFF
#define CTM2MSETPM_2_LIFETIME_DEF             86400

/* AT+CTM2MSEND= */
#define CTM2MSEND_1_MODE_MIN                 0
#define CTM2MSEND_1_MODE_MAX                 3
#define CTM2MSEND_1_MODE_DEF                 1


enum CTM2M_MODID {
    IDAUTH_MODE = 0x01,
    NAT_TYPE,
    ON_UQMODE,
    AUTO_HEARTBEAT_MODE,
    WAKEUP_NOTIFY,
    PROTOCOL_MODE,
};

CmsRetId  ctm2mVERSION(const AtCmdInputContext *pAtCmdReq);
CmsRetId  ctm2mSETMOD(const AtCmdInputContext *pAtCmdReq);
CmsRetId  ctm2mSETPM(const AtCmdInputContext *pAtCmdReq);
CmsRetId  ctm2mSETPSK(const AtCmdInputContext *pAtCmdReq);
CmsRetId  ctm2mIDAUTHPM(const AtCmdInputContext *pAtCmdReq);
CmsRetId  ctm2mREG(const AtCmdInputContext *pAtCmdReq);
CmsRetId  ctm2mUPDATE(const AtCmdInputContext *pAtCmdReq);
CmsRetId  ctm2mDEREG(const AtCmdInputContext *pAtCmdReq);
CmsRetId  ctm2mSEND(const AtCmdInputContext *pAtCmdReq);
CmsRetId  ctm2mCMDRSP(const AtCmdInputContext *pAtCmdReq);
CmsRetId  ctm2mGETSTATUS(const AtCmdInputContext *pAtCmdReq);
CmsRetId  ctm2mRMODE(const AtCmdInputContext *pAtCmdReq);
CmsRetId  ctm2mREAD(const AtCmdInputContext *pAtCmdReq);

CmsRetId  ctm2mCLNV(const AtCmdInputContext *pAtCmdReq);
CmsRetId  ctm2mSETIMSI(const AtCmdInputContext *pAtCmdReq);
#ifdef WITH_FOTA_RESUME
CmsRetId  ctm2mCLFOTANV(const AtCmdInputContext *pAtCmdReq);
#endif
#endif

