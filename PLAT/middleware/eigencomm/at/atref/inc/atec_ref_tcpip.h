/******************************************************************************
 * (C) Copyright 2018 EIGENCOMM International Ltd.
 * All Rights Reserved
*******************************************************************************
 *  Filename: atec_tcpip.h
 *
 *  Description:
 *
 *  History:
 *
 *  Notes:
 *
******************************************************************************/
#ifndef _ATEC_TCPIP_H
#define _ATEC_TCPIP_H

#include "at_util.h"

#define REF_MAX_URL_IPADDR_LEN                    255

/* AT+NPING */
#define NPING_0_STR_MAX_LEN              REF_MAX_URL_IPADDR_LEN
#define NPING_0_STR_DEF                  NULL
#define NPING_1_PAYLOAD_MIN              12
#define NPING_1_PAYLOAD_MAX              1500
#define NPING_1_PAYLOAD_DEF              12
#define NPING_2_TIMEOUT_MIN              10
#define NPING_2_TIMEOUT_MAX              10*60*1000
#define NPING_2_TIMEOUT_DEF              10000

/* AT+QDNS */
#define QDNS_0_MODE_MIN                  0
#define QDNS_0_MODE_MAX                  2
#define QDNS_0_MODE_DEF                  0
#define QDNS_1_STR_MAX_LEN              REF_MAX_URL_IPADDR_LEN
#define QDNS_1_STR_DEF                  NULL

/* AT+QIDNSCFG */
#define QIDNSCFG_DNS_NUM                      4

#define QIDNSCFG_DNS_STR_MAX_LEN              64
#define QIDNSCFG_DNS_STR_DEF                  PNULL
#define ATEC_QIDNSCFG_GET_CNF_STR_LEN        256
#define ATEC_QIDNSCFG_GET_CNF_TMP_STR_LEN    50

/* AT+NIPINFO */
#define NIPINFO_0_MODE_MIN                  0
#define NIPINFO_0_MODE_MAX                  1
#define NIPINFO_0_MODE_DEF                  0


CmsRetId  nmNPING(const AtCmdInputContext *pAtCmdReq);
CmsRetId  nmQDNS(const AtCmdInputContext *pAtCmdReq);
CmsRetId  nmQIDNSCFG(const AtCmdInputContext *pAtCmdReq);

CmsRetId  nmNIPINFO(const AtCmdInputContext *pAtCmdReq);


#endif

