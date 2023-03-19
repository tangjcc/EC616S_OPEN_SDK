/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_http.h
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef _ATEC_HTTP_H
#define _ATEC_HTTP_H

#include "at_util.h"

/* AT+HTTPCREATE */
#define HTTPCREATE_0_FLAG_MIN                   0
#define HTTPCREATE_0_FLAG_MAX                   1
#define HTTPCREATE_0_FLAG_DEF                   0
#define HTTPCREATE_1_URL_STR_MAX_LEN               128
#define HTTPCREATE_1_URL_STR_DEF                   NULL
#define HTTPCREATE_2_AUTHUSER_STR_MAX_LEN       MAX_BASIC_AUTHUSER_SIZE
#define HTTPCREATE_2_AUTHUSER_STR_DEF           NULL
#define HTTPCREATE_3_AUTHPASSWD_STR_MAX_LEN         MAX_BASIC_AUTHPASSWD_SIZE
#define HTTPCREATE_3_AUTHPASSWD_STR_DEF             NULL
#define HTTPCREATE_4_TOTALSERCERLEN_MIN          0
#define HTTPCREATE_4_TOTALSERCERLEN_MAX          0x1000
#define HTTPCREATE_4_TOTALSERCERLEN_DEF          0
#define HTTPCREATE_5_CUEESERCERLEN_MIN              0
#define HTTPCREATE_5_CUEESERCERLEN_MAX              4096
#define HTTPCREATE_5_CUEESERCERLEN_DEF              0
#define HTTPCREATE_6_SERCERT_STR_MAX_LEN         0x200
#define HTTPCREATE_6_SERCERT_STR_DEF             NULL
#define HTTPCREATE_7_CLIENTCERTLEN_MIN              0
#define HTTPCREATE_7_CLIENTCERTLEN_MAX              0x1000
#define HTTPCREATE_7_CLIENTCERTLEN_DEF              0
#define HTTPCREATE_8_CLIENTCERT_STR_MAX_LEN      0x1000
#define HTTPCREATE_8_CLIENTCERT_STR_DEF          NULL
#define HTTPCREATE_9_CLIENTPKLEN_MIN                0
#define HTTPCREATE_9_CLIENTPKLEN_MAX                0x1000
#define HTTPCREATE_9_CLIENTPKLEN_DEF                0
#define HTTPCREATE_10_CLIENTPK_STR_MAX_LEN       0x1000
#define HTTPCREATE_10_CLIENTPK_STR_DEF           NULL

/* AT+HTTPCONNECT */
#define HTTPCONNECT_0_CLIENTID_MIN                   0
#define HTTPCONNECT_0_CLIENTID_MAX                   0
#define HTTPCONNECT_0_CLIENTID_DEF                   0

/* AT+HTTPSEND */
#define HTTPSEND_0_CLIENTID_MIN                   0
#define HTTPSEND_0_CLIENTID_MAX                   0
#define HTTPSEND_0_CLIENTID_DEF                   0
#define HTTPSEND_1_METHOD_MIN                  0
#define HTTPSEND_1_METHOD_MAX                  4
#define HTTPSEND_1_METHOD_DEF                  0
#define HTTPSEND_2_PATHLEN_MIN                    0
#define HTTPSEND_2_PATHLEN_MAX                    260
#define HTTPSEND_2_PATHLEN_DEF                    0
#define HTTPSEND_3_PATH_STR_MAX_LEN            260
#define HTTPSEND_3_PATH_STR_DEF                NULL
#define HTTPSEND_4_HEADERLEN_MIN                   0
#define HTTPSEND_4_HEADERLEN_MAX                   255
#define HTTPSEND_4_HEADERLEN_DEF                   0
#define HTTPSEND_5_HEADER_STR_MAX_LEN          256
#define HTTPSEND_5_HEADER_STR_DEF              NULL
#define HTTPSEND_6_CONTENTTYPELEN_MIN              0
#define HTTPSEND_6_CONTENTTYPELEN_MAX              MAX_CONTENT_TYPE_SIZE
#define HTTPSEND_6_CONTENTTYPELEN_DEF              0
#define HTTPSEND_7_CONTENTTYPE_STR_MAX_LEN     MAX_CONTENT_TYPE_SIZE
#define HTTPSEND_7_CONTENTTYPE_STR_DEF         NULL
#define HTTPSEND_8_CONTENTLEN_MIN                   0
#define HTTPSEND_8_CONTENTLEN_MAX                   0x400
#define HTTPSEND_8_CONTENTLEN_DEF                   0
#define HTTPSEND_9_CONTENT_STR_MAX_LEN         0x200
#define HTTPSEND_9_CONTENT_STR_DEF             NULL

/* AT+HTTPDESTROY */
#define HTTPDESTROY_0_CLIENTID_MIN                   0
#define HTTPDESTROY_0_CLIENTID_MAX                   0
#define HTTPDESTROY_0_CLIENTID_DEF                   0

#define HTTP_TLS_CA_SUB_SEQ_LEN                         64

CmsRetId  httpCREATE(const AtCmdInputContext *pAtCmdReq);
CmsRetId  httpCONNECT(const AtCmdInputContext *pAtCmdReq);
CmsRetId  httpSEND(const AtCmdInputContext *pAtCmdReq);
CmsRetId  httpDESTROY(const AtCmdInputContext *pAtCmdReq);
CmsRetId httpCONNCnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId httpSENDCnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId httpDESTORYCnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);



#endif

