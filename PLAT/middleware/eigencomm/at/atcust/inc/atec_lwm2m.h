/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_lwm2m.h
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef _ATEC_LWM2M_H
#define _ATEC_LWM2M_H

#include "at_util.h"

/* AT+LWM2MCREATE */
#define LWM2MCREATE_0_SERVER_STR_MAX_LEN              40
#define LWM2MCREATE_0_SERVER_STR_DEF                  NULL
#define LWM2MCREATE_1_PORT_MIN                        1024
#define LWM2MCREATE_1_PORT_MAX                        65535
#define LWM2MCREATE_1_PORT_DEF                        5683
#define LWM2MCREATE_2_LOCALPORT_MIN                   1024
#define LWM2MCREATE_2_LOCALPORT_MAX                   65535
#define LWM2MCREATE_2_LOCALPORT_DEF                   56830
#define LWM2MCREATE_3_ENDPOINT_STR_MAX_LEN          32
#define LWM2MCREATE_3_ENDPOINT_STR_DEF              NULL
#define LWM2MCREATE_4_LIFETIME_MIN                    0xF
#define LWM2MCREATE_4_LIFETIME_MAX                    0xFFFFFFF
#define LWM2MCREATE_4_LIFETIME_DEF                    0
#define LWM2MCREATE_5_PSKID_STR_MAX_LEN             32
#define LWM2MCREATE_5_PSKID_STR_DEF                 NULL
#define LWM2MCREATE_6_PSK_STR_MAX_LEN                 32
#define LWM2MCREATE_6_PSK_STR_DEF                     NULL

/* AT+LWM2MDELETE */
#define LWM2MDELETE_0_CLIENTID_MIN                   0
#define LWM2MDELETE_0_CLIENTID_MAX                   0
#define LWM2MDELETE_0_CLIENTID_DEF                   0

/* AT+LWM2MADDOBJ */
#define LWM2MADDOBJ_0_ID_MIN                   0
#define LWM2MADDOBJ_0_ID_MAX                   0
#define LWM2MADDOBJ_0_ID_DEF                   0
#define LWM2MADDOBJ_1_OBJID_MIN                   0
#define LWM2MADDOBJ_1_OBJID_MAX                   0xFFFF
#define LWM2MADDOBJ_1_OBJID_DEF                   0
#define LWM2MADDOBJ_2_INSTID_MIN               0
#define LWM2MADDOBJ_2_INSTID_MAX               0xFFFF
#define LWM2MADDOBJ_2_INSTID_DEF               0
#define LWM2MADDOBJ_3_RESOURCECOUNT_MIN           1
#define LWM2MADDOBJ_3_RESOURCECOUNT_MAX           0xFF
#define LWM2MADDOBJ_3_RESOURCECOUNT_DEF           0
#define LWM2MADDOBJ_4_RESID_STR_MAX_LEN        32
#define LWM2MADDOBJ_4_RESID_STR_DEF            NULL

/* AT+LWM2MDELOBJ */
#define LWM2MDELOBJ_0_ID_MIN                   0
#define LWM2MDELOBJ_0_ID_MAX                   0
#define LWM2MDELOBJ_0_ID_DEF                   0
#define LWM2MDELOBJ_1_OBJID_MIN                   0
#define LWM2MDELOBJ_1_OBJID_MAX                   0xFFFF
#define LWM2MDELOBJ_1_OBJID_DEF                   0

/* AT+LWM2MREADCONF */
#define LWM2MREADCONF_0_ID_MIN                   0
#define LWM2MREADCONF_0_ID_MAX                   0
#define LWM2MREADCONF_0_ID_DEF                   0
#define LWM2MREADCONF_1_OBJID_MIN                   0
#define LWM2MREADCONF_1_OBJID_MAX                   0xFFFF
#define LWM2MREADCONF_1_OBJID_DEF                   0
#define LWM2MREADCONF_2_INSTID_MIN               0
#define LWM2MREADCONF_2_INSTID_MAX               0xFFFF
#define LWM2MREADCONF_2_INSTID_DEF               0
#define LWM2MREADCONF_3_RESOURCEID_MIN               0
#define LWM2MREADCONF_3_RESOURCEID_MAX               0xFFFF
#define LWM2MREADCONF_3_RESOURCEID_DEF               0
#define LWM2MREADCONF_4_TYPE_MIN                 0
#define LWM2MREADCONF_4_TYPE_MAX                 (VALUE_TYPE_UNKNOWN-1)
#define LWM2MREADCONF_4_TYPE_DEF                 0
#define LWM2MREADCONF_5_LEN_MIN                     1
#define LWM2MREADCONF_5_LEN_MAX                     40
#define LWM2MREADCONF_5_LEN_DEF                     0
#define LWM2MREADCONF_6_VALUE_MAX_LEN            LWM2MREADCONF_5_LEN_MAX
#define LWM2MREADCONF_6_VALUE_DEF                NULL

/* AT+LWM2MWRITECONF */
#define LWM2MWRITECONF_0_ID_MIN                   0
#define LWM2MWRITECONF_0_ID_MAX                   0
#define LWM2MWRITECONF_0_ID_DEF                   0
#define LWM2MWRITECONF_1_RET_MIN                0
#define LWM2MWRITECONF_1_RET_MAX                0xFF
#define LWM2MWRITECONF_1_RET_DEF                0

/* AT+LWM2MEXECUTECONF */
#define LWM2MEXECUTECONF_0_ID_MIN                   0
#define LWM2MEXECUTECONF_0_ID_MAX                   0
#define LWM2MEXECUTECONF_0_ID_DEF                   0
#define LWM2MEXECUTECONF_1_RET_MIN               0
#define LWM2MEXECUTECONF_1_RET_MAX               0xFF
#define LWM2MEXECUTECONF_1_RET_DEF               0

/* AT+LWM2MNOTIFY */
#define LWM2MNOTIFY_0_ID_MIN                   0
#define LWM2MNOTIFY_0_ID_MAX                   0
#define LWM2MNOTIFY_0_ID_DEF                   0
#define LWM2MNOTIFY_1_OBJID_MIN                   0
#define LWM2MNOTIFY_1_OBJID_MAX                   0xFFFF
#define LWM2MNOTIFY_1_OBJID_DEF                   0
#define LWM2MNOTIFY_2_INSTID_MIN               0
#define LWM2MNOTIFY_2_INSTID_MAX               0xFFFF
#define LWM2MNOTIFY_2_INSTID_DEF               0
#define LWM2MNOTIFY_3_RESOURCECOUNT_MIN            0
#define LWM2MNOTIFY_3_RESOURCECOUNT_MAX            0xFFFF
#define LWM2MNOTIFY_3_RESOURCECOUNT_DEF            0
#define LWM2MNOTIFY_4_TYPE_MIN                 0
#define LWM2MNOTIFY_4_TYPE_MAX                 (VALUE_TYPE_UNKNOWN-1)
#define LWM2MNOTIFY_4_TYPE_DEF                 0
#define LWM2MNOTIFY_5_LEN_MIN                     1
#define LWM2MNOTIFY_5_LEN_MAX                     40
#define LWM2MNOTIFY_5_LEN_DEF                     0
#define LWM2MNOTIFY_6_VALUE_MAX_LEN            LWM2MNOTIFY_5_LEN_MAX
#define LWM2MNOTIFY_6_VALUE_DEF                NULL

/* AT+LWM2MUPDATE */
#define LWM2MUPDATE_0_ID_MIN                   0
#define LWM2MUPDATE_0_ID_MAX                   0
#define LWM2MUPDATE_0_ID_DEF                   0
#define LWM2MUPDATE_1_WITHOBJ_MIN                    0
#define LWM2MUPDATE_1_WITHOBJ_MAX                    1
#define LWM2MUPDATE_1_WITHOBJ_DEF                    0

CmsRetId  lwm2mCREATE(const AtCmdInputContext *pAtCmdReq);
CmsRetId  lwm2mDELETE(const AtCmdInputContext *pAtCmdReq);
CmsRetId  lwm2mADDOBJ(const AtCmdInputContext *pAtCmdReq);
CmsRetId  lwm2mDELOBJ(const AtCmdInputContext *pAtCmdReq);
CmsRetId  lwm2mREADCONF(const AtCmdInputContext *pAtCmdReq);
CmsRetId  lwm2mWRITECONF(const AtCmdInputContext *pAtCmdReq);
CmsRetId  lwm2mEXECUTECONF(const AtCmdInputContext *pAtCmdReq);
CmsRetId  lwm2mNOTIFY(const AtCmdInputContext *pAtCmdReq);
CmsRetId  lwm2mUPDATE(const AtCmdInputContext *pAtCmdReq);

#endif

