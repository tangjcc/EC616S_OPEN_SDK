/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_ps.h
*
*  Description: Macro definition for packet switched service related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef _ATEC_PS_H
#define _ATEC_PS_H

/* AT+CGCMOD */
#define ATC_CGCMOD_0_CID_VAL_MIN                   0
#define ATC_CGCMOD_0_CID_VAL_MAX                   10
#define ATC_CGCMOD_0_CID_VAL_DEFAULT               0

/* AT+CGREG */
#define ATC_CGREG_0_VAL_MIN                   0
#define ATC_CGREG_0_VAL_MAX                   2
#define ATC_CGREG_0_VAL_DEFAULT               0

/* AT+CGATT */
#define ATC_CGATT_0_STATE_VAL_MIN                0
#define ATC_CGATT_0_STATE_VAL_MAX                1
#define ATC_CGATT_0_STATE_VAL_DEFAULT            1  /* attached */

/* AT+CGACT */
#define ATC_CGACT_0_STATE_VAL_MIN                0
#define ATC_CGACT_0_STATE_VAL_MAX                1
#define ATC_CGACT_0_STATE_VAL_DEFAULT            1  /* activated */
#define ATC_CGACT_1_CID_VAL_MIN                0
#define ATC_CGACT_1_CID_VAL_MAX                10
#define ATC_CGACT_1_CID_VAL_DEFAULT            0

/* AT+CGDATA */
#define ATC_CGDATA_0_L2P_STR_DEFAULT            "M-PT"
#define ATC_CGDATA_0_L2P_STR_MAX_LEN            16
#define ATC_CGDATA_1_CID_VAL_MIN                0
#define ATC_CGDATA_1_CID_VAL_MAX                10
#define ATC_CGDATA_1_CID_VAL_DEFAULT            0

/* AT+CGDCONT */
#define ATC_CGDCONT_0_CID_VAL_MIN                 0
#define ATC_CGDCONT_0_CID_VAL_MAX                 10
#define ATC_CGDCONT_0_CID_VAL_DEFAULT             0
#define ATC_CGDCONT_1_PDPTYPE_STR_DEFAULT            "IP"
#define ATC_CGDCONT_1_PDPTYPE_STR_MAX_LEN            ATC_CMDSTR_MAX_LEN + CMS_NULL_CHAR_LEN
#define ATC_CGDCONT_2_APN_STR_DEFAULT             NULL
#define ATC_CGDCONT_2_APN_STR_MAX_LEN             100
#define ATC_CGDCONT_3_PDPADDRESS_STR_DEFAULT         NULL
#define ATC_CGDCONT_3_PDPADDRESS_STR_MAX_LEN         16 /* CI_PS_PDP_IP_V6_SIZE */
#define ATC_CGDCONT_4_DCOMP_VAL_MIN                0
#define ATC_CGDCONT_4_DCOMP_VAL_MAX                3
#define ATC_CGDCONT_4_DCOMP_VAL_DEFAULT            0  /*  */
#define ATC_CGDCONT_5_HCOMP_VAL_MIN                  0
#define ATC_CGDCONT_5_HCOMP_VAL_MAX                  4
#define ATC_CGDCONT_5_HCOMP_VAL_DEFAULT              0  /*  */
#define ATC_CGDCONT_6_IPV4ADDRALLOC_VAL_MIN        0
#define ATC_CGDCONT_6_IPV4ADDRALLOC_VAL_MAX        0
#define ATC_CGDCONT_6_IPV4ADDRALLOC_VAL_DEFAULT    0
#define ATC_CGDCONT_7_PARA_VAL_MIN        0
#define ATC_CGDCONT_7_PARA_VAL_MAX        2
#define ATC_CGDCONT_7_PARA_VAL_DEFAULT    0
#define ATC_CGDCONT_8_PARA_VAL_MIN          0
#define ATC_CGDCONT_8_PARA_VAL_MAX          0
#define ATC_CGDCONT_8_PARA_VAL_DEFAULT      0
#define ATC_CGDCONT_9_PARA_VAL_MIN        0
#define ATC_CGDCONT_9_PARA_VAL_MAX        0
#define ATC_CGDCONT_9_PARA_VAL_DEFAULT    0
#define ATC_CGDCONT_10_PARA_VAL_MIN        0
#define ATC_CGDCONT_10_PARA_VAL_MAX        1
#define ATC_CGDCONT_10_PARA_VAL_DEFAULT    0
#define ATC_CGDCONT_11_PARA_VAL_MIN          0
#define ATC_CGDCONT_11_PARA_VAL_MAX          1
#define ATC_CGDCONT_11_PARA_VAL_DEFAULT      0
#define ATC_CGDCONT_12_PARA_VAL_MIN        0
#define ATC_CGDCONT_12_PARA_VAL_MAX        1
#define ATC_CGDCONT_12_PARA_VAL_DEFAULT    0
#define ATC_CGDCONT_13_PARA_VAL_MIN          0
#define ATC_CGDCONT_13_PARA_VAL_MAX          1
#define ATC_CGDCONT_13_PARA_VAL_DEFAULT      0
#define ATC_CGDCONT_14_PARA_VAL_MIN        0
#define ATC_CGDCONT_14_PARA_VAL_MAX        1
#define ATC_CGDCONT_14_PARA_VAL_DEFAULT    0

/* AT+CGDSCONT */
#define ATC_CGDSCONT_0_VAL_MIN                   0
#define ATC_CGDSCONT_0_VAL_MAX                   2
#define ATC_CGDSCONT_0_VAL_DEFAULT               0

/* AT+CEQOS */
#define ATC_CEQOS_0_VAL_MIN                   0
#define ATC_CEQOS_0_VAL_MAX                   2
#define ATC_CEQOS_0_VAL_DEFAULT               0

/* AT+CGEQOS */
#define ATC_CGEQOS_0_CID_VAL_MIN                0
#define ATC_CGEQOS_0_CID_VAL_MAX                10
#define ATC_CGEQOS_0_CID_VAL_DEFAULT            0  /*  */
#define ATC_CGEQOS_1_QCI_VAL_MIN                  0
#define ATC_CGEQOS_1_QCI_VAL_MAX                  79
#define ATC_CGEQOS_1_QCI_VAL_DEFAULT              0  /*  */
#define ATC_CGEQOS_2_DLGBR_VAL_MIN              0
#define ATC_CGEQOS_2_DLGBR_VAL_MAX              256000
#define ATC_CGEQOS_2_DLGBR_VAL_DEFAULT          0  /*  */
#define ATC_CGEQOS_3_ULGBR_VAL_MIN                0
#define ATC_CGEQOS_3_ULGBR_VAL_MAX                256000
#define ATC_CGEQOS_3_ULGBR_VAL_DEFAULT            0  /*  */
#define ATC_CGEQOS_4_DLMBR_VAL_MIN              0
#define ATC_CGEQOS_4_DLMBR_VAL_MAX              256000
#define ATC_CGEQOS_4_DLMBR_VAL_DEFAULT          0  /*  */
#define ATC_CGEQOS_5_ULMBR_VAL_MIN                0
#define ATC_CGEQOS_5_ULMBR_VAL_MAX                256000
#define ATC_CGEQOS_5_ULMBR_VAL_DEFAULT            0  /*  */

/* AT+CGEQOSRDP */
#define ATC_CGEQOSRDP_0_CID_VAL_MIN                   0
#define ATC_CGEQOSRDP_0_CID_VAL_MAX                   10
#define ATC_CGEQOSRDP_0_CID_VAL_DEFAULT               0

/* AT+CGCONTRDP */
#define ATC_CGCONTRDP_0_CID_VAL_MIN                   0
#define ATC_CGCONTRDP_0_CID_VAL_MAX                   10
#define ATC_CGCONTRDP_0_CID_VAL_DEFAULT               0

/* AT+CEREG */
#define ATC_CEREG_0_VAL_MIN                   0
#define ATC_CEREG_0_VAL_MAX                   5
#define ATC_CEREG_0_VAL_DEFAULT               0

/* AT+CSCON */
#define ATC_CSCON_0_VAL_MIN                   0
#define ATC_CSCON_0_VAL_MAX                   2
#define ATC_CSCON_0_VAL_DEFAULT               0

/* AT+CGTFT */
#define ATC_CGTFT_0_CID_VAL_MIN                0
#define ATC_CGTFT_0_CID_VAL_MAX                10
#define ATC_CGTFT_0_CID_VAL_DEFAULT            0
#define ATC_CGTFT_1_PFID_VAL_MIN                  1
#define ATC_CGTFT_1_PFID_VAL_MAX                  16
#define ATC_CGTFT_1_PFID_VAL_DEFAULT              0
#define ATC_CGTFT_2_EPINDEX_VAL_MIN             0
#define ATC_CGTFT_2_EPINDEX_VAL_MAX             255
#define ATC_CGTFT_2_EPINDEX_VAL_DEFAULT         0
#define ATC_CGTFT_3_ADDRESS_STR_MAX_LEN            128
#define ATC_CGTFT_3_ADDRESS_STR_DEFAULT            NULL
#define ATC_CGTFT_4_PROTOCOLNUM_VAL_MIN         0
#define ATC_CGTFT_4_PROTOCOLNUM_VAL_MAX         255
#define ATC_CGTFT_4_PROTOCOLNUM_VAL_DEFAULT     0
#define ATC_CGTFT_5_PORT_STR_MAX_LEN                12
#define ATC_CGTFT_5_PORT_STR_DEFAULT                NULL
#define ATC_CGTFT_6_PORT_STR_MAX_LEN                12
#define ATC_CGTFT_6_PORT_STR_DEFAULT                NULL
#define ATC_CGTFT_7_IPSEC_STR_MAX_LEN           9
#define ATC_CGTFT_7_IPSEC_STR_DEFAULT           NULL
#define ATC_CGTFT_8_TOS_STR_MAX_LEN                 8
#define ATC_CGTFT_8_TOS_STR_DEFAULT                 NULL
#define ATC_CGTFT_9_FLOWLABLE_STR_MAX_LEN       6
#define ATC_CGTFT_9_FLOWLABLE_STR_DEFAULT       NULL
#define ATC_CGTFT_10_DIRECTION_VAL_MIN               0
#define ATC_CGTFT_10_DIRECTION_VAL_MAX               3
#define ATC_CGTFT_10_DIRECTION_VAL_DEFAULT           0

/* AT+CSODCP */
/* as some equipment send CID = 0, which act as the initial PDP */
#define ATC_CSODCP_0_CID_VAL_MIN                    0
#define ATC_CSODCP_0_CID_VAL_MAX                    10
#define ATC_CSODCP_0_CID_VAL_DEFAULT                0
#define ATC_CSODCP_1_CPD_VAL_MIN                    1
#define ATC_CSODCP_1_CPD_VAL_MAX                    950
#define ATC_CSODCP_1_CPD_VAL_DEFAULT                24
#define ATC_CSODCP_2_CPDATA_STR_MAX_LEN             (ATC_CSODCP_1_CPD_VAL_MAX*2+1)
#define ATC_CSODCP_2_CPDATA_STR_DEFAULT             NULL
#define ATC_CSODCP_3_RAI_VAL_MIN                    0
#define ATC_CSODCP_3_RAI_VAL_MAX                    2
#define ATC_CSODCP_3_RAI_VAL_DEFAULT                0   /*RAI: No information available*/
#define ATC_CSODCP_4_TYPE_VAL_MIN                   0
#define ATC_CSODCP_4_TYPE_VAL_MAX                   1   /*Regular data*/
#define ATC_CSODCP_4_TYPE_VAL_DEFAULT               0

/* AT+ECATTBEARER */
#define ATC_ECATTBEARER_0_PDN_VAL_MIN                   1
#define ATC_ECATTBEARER_0_PDN_VAL_MAX                   6
#define ATC_ECATTBEARER_0_PDN_VAL_DEFAULT               1
#define ATC_ECATTBEARER_1_EITF_VAL_MIN                  0
#define ATC_ECATTBEARER_1_EITF_VAL_MAX                  1
#define ATC_ECATTBEARER_1_EITF_VAL_DEFAULT              0
#define ATC_ECATTBEARER_2_APN_STR_MAX_LEN               100
#define ATC_ECATTBEARER_2_APN_STR_DEFAULT               NULL
#define ATC_ECATTBEARER_3_IPV4ALLOC_VAL_MIN             0
#define ATC_ECATTBEARER_3_IPV4ALLOC_VAL_MAX             0
#define ATC_ECATTBEARER_3_IPV4ALLOC_VAL_DEFAULT         0
#define ATC_ECATTBEARER_4_NSLPI_VAL_MIN                 0
#define ATC_ECATTBEARER_4_NSLPI_VAL_MAX                 1
#define ATC_ECATTBEARER_4_NSLPI_VAL_DEFAULT             0
#define ATC_ECATTBEARER_5_IPV4MTU_VAL_MIN               0
#define ATC_ECATTBEARER_5_IPV4MTU_VAL_MAX               1
#define ATC_ECATTBEARER_5_IPV4MTU_VAL_DEFAULT           0
#define ATC_ECATTBEARER_6_NOIPMTU_VAL_MIN               0
#define ATC_ECATTBEARER_6_NOIPMTU_VAL_MAX               1
#define ATC_ECATTBEARER_6_NOIPMTU_VAL_DEFAULT           0
#define ATC_ECATTBEARER_7_AUTH_PROTOCOL_MIN             0
#define ATC_ECATTBEARER_7_AUTH_PROTOCOL_MAX             1
#define ATC_ECATTBEARER_7_AUTH_PROTOCOL_DEFAULT         0
#define ATC_ECATTBEARER_8_AUTH_USERNAME_MAX_LEN         20
#define ATC_ECATTBEARER_8_AUTH_USERNAME_DEFAULT         NULL
#define ATC_ECATTBEARER_9_AUTH_PASSWORD_MAX_LEN         20
#define ATC_ECATTBEARER_9_AUTH_PASSWORD_DEFAULT         NULL



#define ATC_ECATTBEARER_10_CSCF_VAL_MIN                    0
#define ATC_ECATTBEARER_10_CSCF_VAL_MAX                    2
#define ATC_ECATTBEARER_10_CSCF_VAL_DEFAULT                0
#define ATC_ECATTBEARER_11_IMCN_VAL_MIN                  0
#define ATC_ECATTBEARER_11_IMCN_VAL_MAX                  1
#define ATC_ECATTBEARER_11_IMCN_VAL_DEFAULT              0

/* AT+CRTDCP */
#define ATC_CRTDCP_0_VAL_MIN                   0
#define ATC_CRTDCP_0_VAL_MAX                   2
#define ATC_CRTDCP_0_VAL_DEFAULT               0

/* AT+CGAUTH */
#define ATC_CGAUTH_0_CID_VAL_MIN                   0
#define ATC_CGAUTH_0_CID_VAL_MAX                   10
#define ATC_CGAUTH_0_CID_VAL_DEFAULT               0
#define ATC_CGAUTH_1_AUTHPROT_VAL_MIN                0
#define ATC_CGAUTH_1_AUTHPROT_VAL_MAX                1
#define ATC_CGAUTH_1_AUTHPROT_VAL_DEFAULT            0
#define ATC_CGAUTH_2_USER_STR_MAX_LEN              20
#define ATC_CGAUTH_2_USER_STR_DEFAULT              NULL
#define ATC_CGAUTH_3_PASSWD_STR_MAX_LEN              20
#define ATC_CGAUTH_3_PASSWD_STR_DEFAULT              NULL

/* AT+CIPCA */
#define ATC_CIPCA_0_VAL_MIN                   3
#define ATC_CIPCA_0_VAL_MAX                   3
#define ATC_CIPCA_0_VAL_DEFAULT               3
#define ATC_CIPCA_1_ATT_VAL_MIN                 0
#define ATC_CIPCA_1_ATT_VAL_MAX                 1
#define ATC_CIPCA_1_ATT_VAL_DEFAULT             0

/* AT+CGAPNRC */
#define ATC_CGAPNRC_0_CID_VAL_MIN               0
#define ATC_CGAPNRC_0_CID_VAL_MAX               10
#define ATC_CGAPNRC_0_CID_VAL_DEFAULT           0

/* AT+CGEREP */
#define ATC_CGEREP_0_VAL_MIN                    0
#define ATC_CGEREP_0_VAL_MAX                    1
#define ATC_CGEREP_0_VAL_DEFAULT                0
#define ATC_CGEREP_1_VAL_MIN                    0
#define ATC_CGEREP_1_VAL_MAX                    0
#define ATC_CGEREP_1_VAL_DEFAULT                0


/*
 * +CGEREP=[<mode>[,<bfr>]]
 * <mode>: integer type
 *   0 buffer unsolicited result codes in the MT; if MT result code buffer is full, the oldest ones can be discarded.
 *     No codes are forwarded to the TE.
 *   1 discard unsolicited result codes when MT-TE link is reserved (e.g. in on-line data mode); otherwise forward
 *     them directly to the TE
 *   2 buffer unsolicited result codes in the MT when MT-TE link is reserved (e.g. in on-line data mode) and flush
 *     them to the TE when MT-TE link becomes available; otherwise forward them directly to the TE; - NOT SUPPORT
*/
typedef enum AtcCGEREPMode_Enum
{
    ATC_CGEREP_DISCARD_OLD_CGEV = 0,
    ATC_CGEREP_FWD_CGEV
}AtcCGEREPMode;



/* AT+ECSENDDATA */
#define ATC_ECSENDDATA_0_CID_VAL_MIN                    0
#define ATC_ECSENDDATA_0_CID_VAL_MAX                    10
#define ATC_ECSENDDATA_0_CID_VAL_DEFAULT                0
#define ATC_ECSENDDATA_1_DATALEN_VAL_MIN                1
#define ATC_ECSENDDATA_1_DATALEN_VAL_MAX                950
#define ATC_ECSENDDATA_1_DATALEN_VAL_DEFAULT            24
#define ATC_ECSENDDATA_2_DATA_STR_MAX_LEN               (ATC_ECSENDDATA_1_DATALEN_VAL_MAX*2+1)
#define ATC_ECSENDDATA_2_DATA_STR_DEFAULT               NULL
#define ATC_ECSENDDATA_3_RAI_VAL_MIN                    0
#define ATC_ECSENDDATA_3_RAI_VAL_MAX                    2
#define ATC_ECSENDDATA_3_RAI_VAL_DEFAULT                0   /*RAI: No information available*/
#define ATC_ECSENDDATA_4_TYPE_VAL_MIN                   0
#define ATC_ECSENDDATA_4_TYPE_VAL_MAX                   1
#define ATC_ECSENDDATA_4_TYPE_VAL_DEFAULT               0   /*Regular data*/

/* AT+ECCIOTPLANE */
#define ATC_ECCIOTPLANE_0_PLANE_VAL_MIN                   0
#define ATC_ECCIOTPLANE_0_PLANE_VAL_MAX                   2
#define ATC_ECCIOTPLANE_0_PLANE_VAL_DEFAULT               0

/* AT+ECNBIOTRAI */
#define ATC_ECNBIOTRAI_0_RAI_VAL_MIN            0
#define ATC_ECNBIOTRAI_0_RAI_VAL_MAX            1
#define ATC_ECNBIOTRAI_0_RAI_VAL_DEFAULT        0

/* AT+CGPADDR */
#define ATC_CGPADDR_0_CID_VAL_MIN                0
#define ATC_CGPADDR_0_CID_VAL_MAX                10
#define ATC_CGPADDR_0_CID_VAL_DEFAULT            0

/* AT+CSCON */
#define ATC_CSCON_0_STATE_VAL_MIN                0
#define ATC_CSCON_0_STATE_VAL_MAX                1
#define ATC_CSCON_0_STATE_VAL_DEFAULT            0  /* disable indication */
/*
 * <n>: integer type
 * 0 disable unsolicited result code
 * 1 enable unsolicited result code +CSCON: <mode>
 * 2 enable unsolicited result code +CSCON: <mode>[,<state>]
 * 3 enable unsolicited result code +CSCON: <mode>[,<state>[,<access>]]
*/
typedef enum AtcCSCONRptLevel_Enum
{
    ATC_DISABLE_CSCON_RPT = 0,
    ATC_CSCON_RPT_MODE,
    ATC_CSCON_RPT_MODE_STATE,
    ATC_CSCON_RPT_MODE_STATE_ACCESS
}AtcCSCONRptLevel;


#define MAX_CPDATA_LEN   1600
#define MAX_PORT_SUPPORT 65535
#define ATCI_PDP_ACTIVE    1
#define ATCI_PDP_INACTIVE  0

CmsRetId  psCGREG(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGDCONT(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGATT(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGACT(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGDATA(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGQMIN(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGQREQ(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGDSCONT(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psGETIP(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGCMOD(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCEQOS(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGEQOS(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGEQOSRDP(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGCONTRDP(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCEREG(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCSCON(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGTFT(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCSODCP(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psECATTBEARER(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCRTDCP(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGAUTH(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCIPCA(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGEREP(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGAPNRC(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psECSENDDATA(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psECCIOTPLANE(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psECNBIOTRAI(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCGPADDR(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCNMPSD(const AtCmdInputContext *pAtCmdReq);
CmsRetId  psCEER(const AtCmdInputContext *pAtCmdReq);

#if 0
CmsRetId psCGDCONTcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
//CmsRetId psCGVCIDcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psECATTAUTHcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCGDSCONTcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCGPADDRcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCGATTcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCGACTcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCGDATAcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCGCMODcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCEREGcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCGCONTRDPcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCGTFTcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCGEQOScnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCGEQOSRDPcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCSODCPcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psECATTBEARERcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCRTDCPcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCGAUTHcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCIPCAcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCGAPNRCcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psECSENDDATAcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psECCIOTPLANEcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCGEVind(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCEREGind(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCRTDCPind(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psRECVNONIPind(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCSCONcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId psCSCONind(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);

UINT8 psCheckCidCnf(void *paras);
#endif

#endif

