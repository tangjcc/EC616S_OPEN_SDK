/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_coap.h
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef _ATEC_COAP_H
#define _ATEC_COAP_H

#include "at_util.h"


#define COAP_ID_MIN                 0
#define COAP_ID_MAX                 3
#define COAP_ID_DEF                 0xff
#define COAP_TAG_MAX_LEN          8

/* AT+COAPCREATE= */
#define COAPCREATE_0_PORT_MIN      1
#define COAPCREATE_0_PORT_MAX      65535
#define COAPCREATE_0_PORT_DEF      56830

/* AT+COAPADDRES= */
#define COAPADDRES_0_LEN_MIN                 1
#define COAPADDRES_0_LEN_MAX                 51
#define COAPADDRES_0_LEN_DEF                 1
#define COAPADDRES_1_RESOURCE_MAX_LEN      51
#define COAPADDRES_1_RESOURCE_STR_DEF      NULL

/* AT+COAPHEAD= */
#define COAPHEAD_0_MODE_MIN                 1
#define COAPHEAD_0_MODE_MAX                 5
#define COAPHEAD_0_MODE_DEF                 1
#define COAPHEAD_1_MSGID_MIN                 0
#define COAPHEAD_1_MSGID_MAX                 65535
#define COAPHEAD_1_MSGID_DEF                 1
#define COAPHEAD_1_TKL_MIN                 1
#define COAPHEAD_1_TKL_MAX                 8
#define COAPHEAD_1_TKL_DEF                 1
#define COAPHEAD_2_TKL_MIN                 COAPHEAD_1_TKL_MIN
#define COAPHEAD_2_TKL_MAX                 COAPHEAD_1_TKL_MAX
#define COAPHEAD_2_TKL_DEF                 COAPHEAD_1_TKL_DEF
#define COAPHEAD_3_TOKEN_MAX_LEN      21
#define COAPHEAD_3_TOKEN_STR_DEF      NULL

/* AT+COAPOPTION= */
#define COAPOPTION_0_CNT_MIN                 1
#define COAPOPTION_0_CNT_MAX                 12
#define COAPOPTION_0_CNT_DEF                 1
#define COAPOPTION_1_NAME_MIN                 1
#define COAPOPTION_1_NAME_MAX                 60
#define COAPOPTION_1_NAME_DEF                 11
#define COAPOPTION_2_VALUE_MAX_LEN      181
#define COAPOPTION_2_VALUE_STR_DEF      NULL

/* AT+COAPSEND= */
#define COAPSEND_0_MSGTYPE_MIN             0
#define COAPSEND_0_MSGTYPE_MAX             44
#define COAPSEND_0_MSGTYPE_DEF             0 
#define COAPSEND_1_METHOD_MIN           0
#define COAPSEND_1_METHOD_MAX           505
#define COAPSEND_1_METHOD_DEF           1

#define COAPSEND_2_IP_MAX_LEN     129
#define COAPSEND_2_IP_STR_DEF         NULL
#define COAPSEND_3_PORT_MIN      1
#define COAPSEND_3_PORT_MAX      65535
#define COAPSEND_3_PORT_DEF      56830
#define COAPSEND_4_LEN_MIN      0
#define COAPSEND_4_LEN_MAX      512
#define COAPSEND_4_LEN_DEF      0
#define COAPSEND_5_DATA_MAX_LEN         515
#define COAPSEND_5_DATA_STR_DEF         NULL

/* AT+COAPCFG= */
#define COAPCFG_0_MODE_MAX_LEN         13
#define COAPCFG_0_MODE_STR_DEF         NULL
#define COAPCFG_1_VALUE_MIN                 0
#define COAPCFG_1_VALUE_MAX                 1
#define COAPCFG_1_VALUE_DEF                 0xff
#define COAPCFG_2_VALUE_MIN                 0
#define COAPCFG_2_VALUE_MAX                 24
#define COAPCFG_2_VALUE_DEF                 0xff
#define COAPCFG_3_VALUE_MIN                 0
#define COAPCFG_3_VALUE_MAX                 24
#define COAPCFG_3_VALUE_DEF                 0xff

/* AT+COAPALISIGN= */
#define COAPALISIGN_0_DEVID_MAX_LEN         33
#define COAPALISIGN_0_DEVID_STR_DEF         NULL
#define COAPALISIGN_1_DEVNAME_MAX_LEN         33
#define COAPALISIGN_1_DEVNAME_STR_DEF         NULL
#define COAPALISIGN_2_DEVSECRET_MAX_LEN         65
#define COAPALISIGN_2_DEVSECRET_STR_DEF         NULL
#define COAPALISIGN_3_PRODUCTKEY_MAX_LEN         33
#define COAPALISIGN_3_PRODUCTKEY_STR_DEF         NULL
#define COAPALISIGN_4_SEQ__MIN      1
#define COAPALISIGN_4_SEQ__MAX      0x7FFFFFFF
#define COAPALISIGN_4_SEQ__DEF      2715
#define COAPALISIGN_MAX_LEN         64


typedef enum
{
    COAPHEAD_MODE_MIN,
    COAPHEAD_MODE_1 = 1,
    COAPHEAD_MODE_2,
    COAPHEAD_MODE_3,
    COAPHEAD_MODE_4,
    COAPHEAD_MODE_5,
        
    COAPHEAD_MODE_MAX,
}CoapHeadMode;
    
typedef struct
{
	UINT16 chanId;
    UINT32 reqHandle;
    int msg_type;
    int msg_method;
    int port;
    char *ip_addr;
    
}coap_send_data;

CmsRetId coapCREATE(const AtCmdInputContext *AtCmdReqParaPtr);
CmsRetId coapDELETE(const AtCmdInputContext *AtCmdReqParaPtr);
CmsRetId coapADDRES(const AtCmdInputContext *AtCmdReqParaPtr);
CmsRetId coapHEAD(const AtCmdInputContext *AtCmdReqParaPtr);
CmsRetId coapOPTION(const AtCmdInputContext *AtCmdReqParaPtr);
CmsRetId coapSEND(const AtCmdInputContext *AtCmdReqParaPtr);
CmsRetId coapDATASTATUS(const AtCmdInputContext *AtCmdReqParaPtr);
CmsRetId coapCFG(const AtCmdInputContext *AtCmdReqParaPtr);
CmsRetId coapALISIGN(const AtCmdInputContext *AtCmdReqParaPtr);
CmsRetId  coapSENDInputData(UINT8 chanId, UINT8 *pData, INT16 dataLength);
CmsRetId  coapSENDCancel(void);

CmsRetId coapSENDind(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId coapRECVind(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);

#endif

/* END OF FILE */
