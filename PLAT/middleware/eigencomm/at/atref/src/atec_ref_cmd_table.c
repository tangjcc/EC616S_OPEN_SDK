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
#ifdef  FEATURE_REF_AT_ENABLE
#include "at_util.h"
#include "atec_ref_ps.h"
#include "atec_ref_plat.h"
#include "atec_ref_tcpip.h"
#ifdef FEATURE_CTWINGV1_5_ENABLE
#include "atec_ref_ctlwm2m.h"
#endif

/******************************************************************************
 ******************************************************************************
 * AT COMMAND PARAMETER ATTRIBUTE DEFINATION
 ******************************************************************************
******************************************************************************/
#define __AT_CMD_PARAM_ATTR_DEFINE__    //just for easy to find this position in SS

/* AT+NEARFCN=<search_mode>,<earfcn>[,pci] */
const static AtValueAttr attrNEARFCN[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                          AT_PARAM_ATTR_DEF(AT_HEX_VAL, AT_OPT_VAL)};

const static AtValueAttr attrNLOGLEVEL[] = {AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL)};

const static AtValueAttr attrNITZ[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

// AT+NATSPEED=<baud_rate>,<timeout>,<store>,<sync_mode>[,<stopbits>[,<parity>[,<xonxoff>]]]
const static AtValueAttr attrNATSPEED[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};

//AT+NCONFIG=<function>,<value>
const static AtValueAttr  attrNCONFIG[] = {AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                           AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_MUST_VAL)};

//AT+NUESTATS
const static AtValueAttr  attrNUESTATS[] = {AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};

//AT+NBAND=<n>[,<n>[,<n>[...]]]
const static AtValueAttr attrNBAND[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};


//AT+NCPCDPR=<parameter>,<state>
const static AtValueAttr attrNCPCDPR[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL)};


//AT+NPSMR=<n>
const static AtValueAttr attrNPSMR[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL)};


//AT+NPTWEDRXS=<mode>,<Act-Type>[,<Requested_Paging_time_window>[,<Requested_eDRX_value>]]
const static AtValueAttr  attrNPTWEDRXS[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};

//AT+NPOWERCLASS=<band>,<powerclass>
const static AtValueAttr attrNPOWERCLASS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                               AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

//AT+NPIN=<command>,<parameter1>[,<parameter2>]
const static AtValueAttr attrNPIN[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};

//AT+NUICC=<mode>
const static AtValueAttr attrNUICC[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

#ifdef FEATURE_CTWINGV1_5_ENABLE
/* AT+NCDP */
static AtValueAttr attrNCDP[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//Sever_IP
                                 AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};//Port

static AtValueAttr attrQLWSREGIND[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//Type

static AtValueAttr attrQLWULDATA[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//length
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//data
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};//seq_num

static AtValueAttr attrQLWULDATAEX[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//length
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//data
                                        AT_PARAM_ATTR_DEF(AT_HEX_VAL, AT_MUST_VAL),//mode
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};//seq_num

static AtValueAttr attrNMGS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//length
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//data
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};//seq_num
static AtValueAttr attrQLWFOTAIND[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//Type

static AtValueAttr attrQREGSWT[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//Type

static AtValueAttr attrQSETPSK[] = {AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//pskid
	                                AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL)};//psk

static AtValueAttr attrNSMI[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//Type

static AtValueAttr attrNNMI[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//Type

static AtValueAttr attrQLWSERVERIP[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//LWM2M/BS/DEL
	                                     AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//Sever_IP
                                         AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};//Port
static AtValueAttr attrQCFG[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//Type
                                  AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL)};//endpoint/lifetime/bindmode
static AtValueAttr attrQSECSWT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//Type
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};//NAT Type
static AtValueAttr attrQCRITICALDATA[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//state

#endif

//AT+NPING
const static AtValueAttr  attrNPING[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

//AT+QDNS
const static AtValueAttr  attrQDNS[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                         AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };

//AT+QIDNSCFG
const static AtValueAttr  attrQIDNSCFG[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};

//AT+NIPINFO
const static AtValueAttr attrNIPINFO[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL)};

//AT+QLEDMODE
const static AtValueAttr attrQLEDMODE[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL)};

//AT+DMPCONFIG
const static AtValueAttr attrECDMPCONFIG[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                               AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                               AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                               AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                               AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};

//AT+DMPCFG
const static AtValueAttr attrECDMPCFG[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};

//AT+DMPCFGEX
const static AtValueAttr attrECDMPCFGEX[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                              AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),
                                              AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),
                                              AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),
                                              AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL)};

/******************************************************************************
 ******************************************************************************
 * AT COMMAND TABLE
 ******************************************************************************
******************************************************************************/
#define __AT_CUST_CMD_TABLE_DEFINE__    //just for easy to find this position in SS

const AtCmdPreDefInfo  refAtCmdTable[] =
{

    AT_CMD_PRE_DEFINE("+NEARFCN",       refPsNEARFCN,        attrNEARFCN,      AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NCSEARFCN",     refPsNCSEARFCN,      NULL,             AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NCPCDPR",       refPsNCPCDPR,        attrNCPCDPR,      AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NBAND",         refPsNBAND,          attrNBAND,        AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NCONFIG",       refPsNCONFIG,        attrNCONFIG,      AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NUESTATS",      refPsNUESTATS,       attrNUESTATS,     AT_EXT_ACT_CMD,       (2 * AT_DEFAULT_TIMEOUT_SEC)),
    AT_CMD_PRE_DEFINE("+NPSMR",         refPsNPSMR,          attrNPSMR,        AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NPTWEDRXS",     refPsNPTWEDRXS,      attrNPTWEDRXS,    AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NPOWERCLASS",   refPsNPOWERCLASS,    attrNPOWERCLASS,  AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),

    AT_CMD_PRE_DEFINE("+NCCID",         refPsNCCID,          NULL,             AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NPIN",          refPsNPIN,           attrNPIN,         AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NUICC",         refPsNUICC,          attrNUICC,        AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),

    AT_CMD_PRE_DEFINE("+NRB",           refNRB,              NULL,             AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NLOGLEVEL",     refNLOGLEVEL,        attrNLOGLEVEL,    AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NITZ",          refNITZ,             attrNITZ,         AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NATSPEED",      refNATSPEED,         attrNATSPEED,     AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),

#ifdef FEATURE_CTWINGV1_5_ENABLE
    // BC28 ctlwm2m CMD
    AT_CMD_PRE_DEFINE("+NCDP",              ctm2mNCDP,          attrNCDP,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+QLWSREGIND",        ctm2mQLWSREGIND,    attrQLWSREGIND,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+QLWULDATA",         ctm2mQLWULDATA  ,   attrQLWULDATA,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+QLWULDATAEX",       ctm2mQLWULDATAEX,   attrQLWULDATAEX,    AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+QLWULDATASTATUS",   ctm2mQLWULDATASTATUS,NULL,              AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+QLWFOTAIND",        ctm2mQLWFOTAIND,    attrQLWFOTAIND,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+QREGSWT",           ctm2mQREGSWT,       attrQREGSWT,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+QSETPSK",           ctm2mQSETPSK,       attrQSETPSK,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NMGS",              ctm2mNMGS,          attrNMGS,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NSMI",              ctm2mNSMI,          attrNSMI,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NNMI",              ctm2mNNMI,          attrNNMI,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NMGR",              ctm2mNMGR,           NULL,              AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NMSTATUS",          ctm2mNMSTATUS,       NULL,              AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+QRESETDTLS",        ctm2mQRESETDTLS,     NULL,              AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+QDTLSSTAT",         ctm2mQDTLSSTAT,      NULL,              AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+QLWSERVERIP",       ctm2mQLWSERVERIP,   attrQLWSERVERIP,    AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+QCFG",              ctm2mQCFG,          attrQCFG,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NQMGR",             ctm2mNQMGR,          NULL   ,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NQMGS",             ctm2mNQMGS,          NULL   ,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+QSECSWT",           ctm2mQSECSWT,       attrQSECSWT,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+QCRITICALDATA",     ctm2mQCRITICALDATA, attrQCRITICALDATA,  AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#endif

    //**28 ping
    AT_CMD_PRE_DEFINE("+NPING",             nmNPING,               attrNPING,          AT_EXT_PARAM_CMD,    AT_DEFAULT_TIMEOUT_SEC),
    //**28 dns
    AT_CMD_PRE_DEFINE("+QDNS",              nmQDNS,                attrQDNS,           AT_EXT_PARAM_CMD,   (6*AT_DEFAULT_TIMEOUT_SEC)),    //30s

    //**28 QIDNSCFG
    AT_CMD_PRE_DEFINE("+QIDNSCFG",          nmQIDNSCFG,         attrQIDNSCFG,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),

    //**28 QIDNSCFG
    AT_CMD_PRE_DEFINE("+NIPINFO",          nmNIPINFO,         attrNIPINFO,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),

    //**28 netlight
    AT_CMD_PRE_DEFINE("+QLEDMODE",          refQLEDMODE,           attrQLEDMODE,       AT_EXT_PARAM_CMD,    AT_DEFAULT_TIMEOUT_SEC),

#if defined(FEATURE_CMCC_DM_ENABLE)
    /* dm cmd*/
    AT_CMD_PRE_DEFINE("+DMPCONFIG",         refECDMPCONFIG,        attrECDMPCONFIG,    AT_EXT_PARAM_CMD,    AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+DMPCFG",            refECDMPCFG,           attrECDMPCFG,       AT_EXT_PARAM_CMD,    AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+DMPCFGEX",          refECDMPCFGEX,         attrECDMPCFGEX,     AT_EXT_PARAM_CMD,    AT_DEFAULT_TIMEOUT_SEC),
#endif

};


AtCmdPreDefInfoC* atcGetRefAtCmdSeqPointer(void)
{
    return  (AtCmdPreDefInfoC *)refAtCmdTable;
}

UINT32 atcGetRefAtCmdSeqNumb(void)
{
    return AT_NUM_OF_ARRAY(refAtCmdTable);
}


#endif

