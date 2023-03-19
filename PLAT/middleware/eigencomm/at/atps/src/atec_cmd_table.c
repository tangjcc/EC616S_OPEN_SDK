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

/******************************************************************************
*   include files
******************************************************************************/
#include <ctype.h>
#include <math.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "atec_mm.h"
#include "atec_ps.h"
#include "atec_dev.h"
#include "atec_sms.h"
#include "atec_sim.h"
#include "atc_decoder.h"
#include "atec_phy.h"
#include "atec_tcpip.h"
#include "atec_general.h"

#ifdef SOFTSIM_FEATURE_ENABLE
#include "atec_softsim.h"
#endif

#include "app.h" // 20210926 add to support dahua qfashion1

/******************************************************************************
 ******************************************************************************
 * AT COMMAND PARAMETER ATTRIBUTE DEFINATION
 ******************************************************************************
******************************************************************************/
#define __AT_CMD_PARAM_ATTR_DEFINE__    //just for easy to find this position in SS


const static AtValueAttr  attrQ[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };


const static AtValueAttr  attrCMEE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

/*AT+ECURC=<typeStr>,<n>*/
const static AtValueAttr  attrECURC[] = { AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_MUST_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};

//const static AtValueAttr  attrCGREG[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrCGATT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrCGACT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                         AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrCGDATA[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrCGDCONT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                           AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                           AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                           AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
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
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };
/* yrhao - AT+CGTFT support */
const static AtValueAttr  attrCGTFT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                         AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrCFUN[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };


/* yrhao - Add a generic comm. feature configuration option in  CI_DEV_PRIM_SET_FUNC_REQ*/
const static AtValueAttr  attrCOPS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrCPSMS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                         AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrCEDRXS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrCCIOTOPT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

//const static AtValueAttr  attrCCIOTOPTI[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
//                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
//                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
//                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrCPWD[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };

//const static AtValueAttr  attrCPOL[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
//                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
//                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL) };

//const static AtValueAttr  attrCPLS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrCCLK[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrCTZR[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

//static AtValueAttr  attrCLTS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrCTZU[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

//static AtValueAttr  attrCENG[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrCREG[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

/*
 *static AtValueAttr  attrCIND[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };
*/

const static AtValueAttr  attrECCESQS[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

#ifndef FEATURE_REF_AT_ENABLE
//AT+ECPSMR
const static AtValueAttr attrECPSMR[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL)};
#endif

//AT+ECPTWEDRXS
const static AtValueAttr  attrECPTWEDRXS[] = {  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                                AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                                AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };

//AT+ECEMMTIME
const static AtValueAttr  attrECEMMTIME[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};


//const static AtValueAttr  attrCGMD[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
//                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };
/*
 * if text mode (+CMGF=1): AT+CMGS=<da>[,<toda>]<CR>
 *                                 text is entered<ctrl-Z/ESC>
 * if PDU mode (+CMGF=0):  AT+CMGS=<length><CR>
 *                                 PDU is given<ctrl-Z/ESC>
*/
const static AtValueAttr  attrCMGS[] = {AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};

const static AtValueAttr  attrCMGC[] = {AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};


//const static AtValueAttr  attrCMGR[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

//const static AtValueAttr  attrCMGW[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
//                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
//                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };

//const static AtValueAttr  attrCMGL[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrCPIN[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };
//Added by yrhao
const static AtValueAttr  attrCSIM[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL) };
//change by  add path parameter
const static AtValueAttr  attrCRSM[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };
const static AtValueAttr  attrCLCK[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrCPINR[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };

//const static AtValueAttr  attrCSUS[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrCCHO[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrCCHC[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrCGLA[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrECSIMSLEEP[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

const static AtValueAttr  attrECSWC[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

const static AtValueAttr  attrECSIMPD[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

const static AtValueAttr  attrECUSATP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL) };

//const static AtValueAttr  attrCNMI[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
//                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
//                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
//                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
//                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrECSIMEDRX[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrCSMS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

//const static AtValueAttr  attrCPMS[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL), //for txt mode, no para required
//                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
//                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrCNMA[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrCSCA [] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                         AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };
// support AT+CMMS
const static AtValueAttr  attrCMMS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

/* yrhao - AT+CGCMOD support*/
const static AtValueAttr  attrCGCMOD[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };
/* yrhao - AT+CEQOS support*/
//const static AtValueAttr  attrCEQOS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };
/* yrhao - AT+CGEQOS support*/
const static AtValueAttr  attrCGEQOS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };
/* yrhao - AT+CGEQOSRDP support*/
const static AtValueAttr  attrCGEQOSRDP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };
/* yrhao - AT+CGCONTRDP support*/
const static AtValueAttr  attrCGCONTRDP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };
//yjzhong - AT+CEREG
const static AtValueAttr  attrCEREG[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrECCGSN[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                          AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL)};

const static AtValueAttr  attrECCGSNLOCK[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                              AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};

//yjzhong - AT+ECBAND
const static AtValueAttr attrECBAND[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                         AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
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
                                         AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                         AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

//yjzhong - AT+CMOLR
const static AtValueAttr  attrCMOLR[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
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
                                   AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                   AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };
//yjzhong - AT+CMTLR
const static AtValueAttr  attrCMTLR[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

//yjzhong - AT+CMTLRA
const static AtValueAttr  attrCMTLRA[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                    AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

const static AtValueAttr  attrECSTATIS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrECSTATUS[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };

//yjzhong - AT+CSCON
const static AtValueAttr  attrCSCON[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

//yjzhong - AT+ECCFG
const static AtValueAttr  attrECCFG[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};

const static AtValueAttr attrECRMFPLMN[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

//AT+ECBCINFO[=<mode>[,<timeoutS>[,<save_for_later>[,<max_cell_number>[,<report_mode>]]]]]
const static AtValueAttr attrECBCINFO[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};


const static AtValueAttr attrECPSTEST[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

const static AtValueAttr attrECPOWERCLASS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

const static AtValueAttr attrECEVENTSTATIS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };


const static AtValueAttr attrECPSSLPCFG[] = {AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};

//yjzhong - AT+CSODCP
const static AtValueAttr  attrCSODCP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                    AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                    AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                    AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                    AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };
//yjzhong - AT+ECATTBEARER
const static AtValueAttr  attrECATTBEARER[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                     };


//AT+CRTDCP
const static AtValueAttr attrCRTDCP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

//AT+ECPHYCFG
const static AtValueAttr attrECPHYCFG[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL,  AT_MUST_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL)};


//AT+CMGF
const static AtValueAttr attrCMGF[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL)};
//AT+CGAUTH
const static AtValueAttr attrCGAUTH[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL),
                                          AT_PARAM_ATTR_DEF(AT_STR_VAL,  AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_STR_VAL,  AT_OPT_VAL)};

//AT+CIPCA
const static AtValueAttr attrCIPCA[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL),
                                   AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL)};

//AT+CGAPNRC
const static AtValueAttr attrCGAPNRC[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL)};


//AT+CGEREP=[<mode>[,<bfr>]]
const static AtValueAttr attrCGEREP[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL)};


//AT+CSMP=[<fo>[,<vp>[,<pid>[,<dcs>]]]]
const static AtValueAttr attrCSMP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL)};

//AT+ECFREQ
const static AtValueAttr attrECFREQ[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL)};

//AT+CMAR
//const static AtValueAttr attrCMAR[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL,  AT_OPT_VAL) };

const static AtValueAttr  attrECSENDDATA[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                      AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                      AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                      AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                      AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

//AT+ECCIOTPLANE
const static AtValueAttr attrECCIOTPLANE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL)};

//AT+ECNBIOTRAI
const static AtValueAttr attrECNBIOTRAI[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL)};

//AT+CGPADDR
const static AtValueAttr attrCGPADDR[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL)};

//AT+CSDH
//const static AtValueAttr attrCSDH[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL)};

//AT+CSAS
//const static AtValueAttr attrCSAS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL)};

//AT+CRES
//const static AtValueAttr attrCRES[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL)};

//AT+CMGCT
//const static AtValueAttr  attrCMGCT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
//                                    AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
//                                    AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
//                                    AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
//                                    AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
//                                    AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

//AT+CMGCP
//const static AtValueAttr  attrCMGCP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
//                                    AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL)};

//AT+CMSS
//const static AtValueAttr  attrCMSS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
//                                    AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
//                                    AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};

//AT+ECSMSSEND=<mode>,<pdu/da>[,<toda>,<text_sms>]
const static AtValueAttr  attrECSMSSEND[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};


#ifdef SOFTSIM_FEATURE_ENABLE
/*AT+ESIMSWITCH=*/
const static AtValueAttr  attrESIMSWITCH[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};
#endif

/******************************************************************************
 ******************************************************************************
 * AT COMMAND TABLE
 ******************************************************************************
******************************************************************************/
#define __AT_PROTOCOL_CMD_TABLE_DEFINE__    //just for easy to find this position in SS

const AtCmdPreDefInfo psAtCmdTable[] =
{
    /*
     *AT_CMD_PRE_DEFINE(name,       atProcFunc,         paramAttrList,  cmdType,                timeOutS)
    */
    // General command---gc
    AT_CMD_PRE_DEFINE(" ",          gcAT,               PNULL,          AT_BASIC_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("Q",          gcATQ,              attrQ,          AT_BASIC_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("E",          gcATE,              attrE,          AT_BASIC_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("&W",       gcATW,              attrW,          AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CGMI",    gcCGMI,             NULL,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CGMM",    gcCGMM,             NULL,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CSCS",    gcCSCS,             attrCSCS,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CMUX",    gcCMUX,             attrCMUX,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CMEE",      gcCMEE,             attrCMEE,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECURC",     gcECURC,            attrECURC,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),

    // Dev Control and Status---dev
    //AT_CMD_PRE_DEFINE("+CPAS",        devCPAS,            NULL,               AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CFUN",          devCFUN,            attrCFUN,           AT_EXT_PARAM_CMD,   (5*AT_DEFAULT_TIMEOUT_SEC)),    //25s
    AT_CMD_PRE_DEFINE("+ECCGSN",        devECCGSN,          attrECCGSN,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECCGSNLOCK",    devECCGSNLOCK,      attrECCGSNLOCK,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CMAR",        devCMAR,            attrCMAR,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+NBAND",       devNBAND,           attrNBAND,          AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+NCONFIG",     devNCONFIG,         attrNCONFIG,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+NUESTATS",    devNUESTATS,        NULL,               AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CMOLR",         devCMOLR,           attrCMOLR,          AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CMTLR",         devCMTLR,           attrCMTLR,          AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CMTLRA",        devCMTLRA,          attrCMTLRA,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSTATUS",      devECSTATUS,        attrECSTATUS,       AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSTATIS",      devECSTATIS,        attrECSTATIS,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECBAND",        devECBAND,          attrECBAND,         AT_EXT_PARAM_CMD,   (5*AT_DEFAULT_TIMEOUT_SEC)),    //25s
    AT_CMD_PRE_DEFINE("+ECFREQ",        devECFREQ,          attrECFREQ,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECCFG",         devECCFG,           attrECCFG,          AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECRMFPLMN",     devECRMFPLMN,       attrECRMFPLMN,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECBCINFO",      devECBCINFO,        attrECBCINFO,       AT_EXT_ACT_CMD,     305),     //305s, the guard time set via AT, here set the MAX value
    AT_CMD_PRE_DEFINE("+ECPSTEST",      devECPSTEST,        attrECPSTEST,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECPOWERCLASS",  devECPOWERCLASS,    attrECPOWERCLASS,   AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECEVENTSTATIS", devECEVENTSTATIS,   attrECEVENTSTATIS,  AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECNBR14",       devECNBR14,         PNULL,              AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECPSSLPCFG",    devECPSSLPCFG,      attrECPSSLPCFG,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),

    // Network Service Related---mm
    //AT_CMD_PRE_DEFINE("+CIND",       mmCIND,             attrCIND,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CSQ",          mmCSQ,              NULL,           AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CESQ",         mmCESQ,             NULL,           AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CREG",         mmCREG,             attrCREG,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+COPS",         mmCOPS,             attrCOPS,       AT_EXT_PARAM_CMD,   (61*AT_DEFAULT_TIMEOUT_SEC)),   //305s
    AT_CMD_PRE_DEFINE("+CPSMS",        mmCPSMS,            attrCPSMS,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CEDRXS",       mmCEDRXS,           attrCEDRXS,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CEDRXRDP",     mmCEDRXRDP,         NULL,           AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CCIOTOPT",     mmCCIOTOPT,         attrCCIOTOPT,   AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CCIOTOPTI",  PNULL,              attrCCIOTOPTI,  AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),            // - TBD
    AT_CMD_PRE_DEFINE("+CRCES",        mmCRCES,            NULL,           AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CPOL",       mmCPOL,             attrCPOL,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CPLS",       mmCPLS,             attrCPLS,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CCLK",         mmCCLK,             attrCCLK,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CTZR",         mmCTZR,             attrCTZR,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CTZU",         mmCTZU,             attrCTZU,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CLTS",       mmCLTS,             attrCLTS,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CENG",       mmCENG,             attrCENG,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+ECNITZ",     PNULL,              NULL,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),            // -TBD
    AT_CMD_PRE_DEFINE("+ECPLMNS",      mmECPLMNS,          NULL,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECCESQS",      mmECCESQS,          attrECCESQS,    AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#ifndef FEATURE_REF_AT_ENABLE
    AT_CMD_PRE_DEFINE("+ECPSMR",       mmECPSMR,           attrECPSMR,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#endif
    AT_CMD_PRE_DEFINE("+ECPTWEDRXS",   mmECPTWEDRXS,       attrECPTWEDRXS, AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECEMMTIME",    mmECEMMTIME,        attrECEMMTIME,  AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),

    // Packet Domain---ps
    //AT_CMD_PRE_DEFINE("+CGREG",       psCGREG,            attrCGREG,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CGATT",         psCGATT,            attrCGATT,       AT_EXT_PARAM_CMD,   (14*AT_DEFAULT_TIMEOUT_SEC)),   //70s
    AT_CMD_PRE_DEFINE("+CGDATA",        psCGDATA,           attrCGDATA,      AT_EXT_ACT_CMD,     (14*AT_DEFAULT_TIMEOUT_SEC)),   //70s
    AT_CMD_PRE_DEFINE("+CGTFT",         psCGTFT,            attrCGTFT,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CGCMOD",        psCGCMOD,           attrCGCMOD,      AT_EXT_ACT_CMD,     (14*AT_DEFAULT_TIMEOUT_SEC)),   //70s
    AT_CMD_PRE_DEFINE("+CGDCONT",       psCGDCONT,          attrCGDCONT,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CGACT",         psCGACT,            attrCGACT,       AT_EXT_PARAM_CMD,   (14*AT_DEFAULT_TIMEOUT_SEC)),   //70s
    //AT_CMD_PRE_DEFINE("+CEQOS",       psCEQOS,            attrCEQOS,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CGEQOS",        psCGEQOS,           attrCGEQOS,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CGEQOSRDP",     psCGEQOSRDP,        attrCGEQOSRDP,   AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CGCONTRDP",     psCGCONTRDP,        attrCGCONTRDP,   AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CEREG",         psCEREG,            attrCEREG,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CSCON",         psCSCON,            attrCSCON,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CSODCP",        psCSODCP,           attrCSODCP,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CRTDCP",        psCRTDCP,           attrCRTDCP,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CGAUTH",        psCGAUTH,           attrCGAUTH,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CIPCA",         psCIPCA,            attrCIPCA,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CGAPNRC",       psCGAPNRC,          attrCGAPNRC,     AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CGEREP",        psCGEREP,           attrCGEREP,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CGEV",        PNULL,              NULL,            AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CGDSCONT",    psCGDSCONT,         NULL,            AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CGPADDR",       psCGPADDR,          attrCGPADDR,     AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CNMPSD",        psCNMPSD,           NULL,            AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CEER",          psCEER,             NULL,            AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),

    AT_CMD_PRE_DEFINE("+ECATTBEARER",   psECATTBEARER,      attrECATTBEARER, AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+ECATTAUTH",   PNULL,              NULL,            AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECCIOTPLANE",   psECCIOTPLANE,      attrECCIOTPLANE, AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CGVCID",      PNULL,              NULL,            AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSENDDATA",    psECSENDDATA,       attrECSENDDATA,  AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+RECVNONIP",   PNULL,              NULL,            AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECNBIOTRAI",    psECNBIOTRAI,       attrECNBIOTRAI,  AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),

    // SIM---sim
    AT_CMD_PRE_DEFINE("+CIMI",      simCIMI,            NULL,           AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CPIN",      simCPIN,            attrCPIN,       AT_EXT_PARAM_CMD,   5*AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CPWD",      simCPWD,            attrCPWD,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CSIM",      simCSIM,            attrCSIM,       AT_EXT_PARAM_CMD,   (4*AT_DEFAULT_TIMEOUT_SEC)),
    AT_CMD_PRE_DEFINE("+CRSM",      simCRSM,            attrCRSM,       AT_EXT_PARAM_CMD,   (4*AT_DEFAULT_TIMEOUT_SEC)),
    //AT_CMD_PRE_DEFINE("+CUSATR",  simCUSATR,          attrCUSATR,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CUSATW",  simCUSATW,          attrCUSATW,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+USATE",   simUSATE,           attrUSATE,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+USATT",   simUSATT,           attrUSATT,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECICCID",   simECICCID,         NULL,           AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CLCK",      simCLCK,            attrCLCK,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CPINR",     simCPINR,           attrCPINR,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CSUS",    simCSUS,            attrCSUS,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CCHO",      simCCHO,            attrCCHO,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CCHC",      simCCHC,            attrCCHC,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CGLA",      simCGLA,            attrCGLA,       AT_EXT_PARAM_CMD,   (4*AT_DEFAULT_TIMEOUT_SEC)),
    AT_CMD_PRE_DEFINE("+ECSIMSLEEP",simECSIMSLEEP,      attrECSIMSLEEP, AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSWC",     simECSWC,           attrECSWC,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSIMPD",   simECSIMPD,         attrECSIMPD,    AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CNUM",      simCNUM,            NULL,           AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECUSATP",   simECUSATP,            attrECUSATP,    AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
 #ifdef SOFTSIM_FEATURE_ENABLE
    AT_CMD_PRE_DEFINE("+ESIMSWITCH",esimSWITCH,         attrESIMSWITCH, AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
 #endif
    // SMS---sms
    AT_CMD_PRE_DEFINE("+CMGS",      smsCMGS,            attrCMGS,       AT_EXT_ACT_CMD,     (12*AT_DEFAULT_TIMEOUT_SEC)),   //60s
    AT_CMD_PRE_DEFINE("+CMGC",      smsCMGC,            attrCMGC,       AT_EXT_ACT_CMD,     (12*AT_DEFAULT_TIMEOUT_SEC)),   //60s
    //AT_CMD_PRE_DEFINE("+CMGR",    smsCMGR,            attrCMGR,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CMGW",    smsCMGW,            attrCMGW,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CSCA",      smsCSCA,            attrCSCA,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CNMI",    smsCNMI,            attrCNMI,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CMMS",      smsCMMS,            attrCMMS,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CMGD",    smsCMGD,            attrCGMD,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CMGL",    smsCMGL,            attrCMGL,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CSMS",      smsCSMS,            attrCSMS,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CPMS",    smsCPMS,            attrCPMS,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CNMA",      smsCNMA,            attrCNMA,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CMGF",      smsCMGF,            attrCMGF,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CSMP",      smsCSMP,            attrCSMP,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CMTI",    PNULL,              NULL,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CMT",     PNULL,              NULL,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CSDH",    smsCSDH,            attrCSDH,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CSAS",    smsCSAS,            attrCSAS,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CRES",    smsCRES,            attrCRES,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CMGCT",   smsCMGCT,           attrCMGCT,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CMGCP",   smsCMGCP,           attrCMGCP,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+CMSS",    smsCMSS,            attrCMSS,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSMSSEND", smsECSMSSEND,       attrECSMSSEND,  AT_EXT_ACT_CMD,     (12*AT_DEFAULT_TIMEOUT_SEC)),   //60s

    // PHY---phy
    AT_CMD_PRE_DEFINE("+ECPHYCFG",    phyECPHYCFG,        attrECPHYCFG,   AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECEDRXSIMU",  phyECEDRXSIMU,      attrECSIMEDRX,  AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),

#ifdef CUSTOMER_DAHUA
    AT_CMD_PRE_DEFINE("+HXSTATUS",	  pdevHXSTATUS,		  NULL, 		  AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#endif

};

AtCmdPreDefInfoC* atcGetATCommandsSeqPointer(void)
{
    return  (AtCmdPreDefInfoC *)psAtCmdTable;
}
UINT32 atcGetATCommandsSeqNumb(void)
{
    return AT_NUM_OF_ARRAY( psAtCmdTable);
}

