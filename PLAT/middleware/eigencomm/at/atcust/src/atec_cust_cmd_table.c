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
#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "atc_decoder.h"
#include "cmicomm.h"
#include "atec_cust_cmd_table.h"
#include "atec_cust_dev.h"
#include "atec_plat_dev.h"
#include "atec_product.h"


#ifdef FEATURE_CTWINGV1_5_ENABLE
#include "atec_ctlwm2m1_5.h"
#endif

#ifdef FEATURE_WAKAAMA_ENABLE
#include "atec_lwm2m.h"
#endif
#ifdef FEATURE_LIBCOAP_ENABLE
#include "atec_coap.h"
#endif
#if defined(FEATURE_MQTT_ENABLE) || defined(FEATURE_MQTT_TLS_ENABLE)
#include "atec_mqtt.h"
#endif
#ifdef  FEATURE_HTTPC_ENABLE
#include "atec_http.h"
#endif
#ifdef FEATURE_CISONENET_ENABLE
#include "atec_onenet.h"
#endif
#include "atec_tcpip.h"

#include "atec_dm.h"

#ifdef FEATURE_ATADC_ENABLE
#include "atec_adc.h"
#endif

#include "atec_fwupd.h"

#ifdef FEATURE_EXAMPLE_ENABLE
#include "atec_example.h"
#endif

#ifdef SOFTSIM_CT_ENABLE
#include "atec_cust_softsim.h"
#include "esimmonitortask.h"
#endif

#include "app.h"	// 20210926 add to support dahua qfashion1

/*******************************************************************************
 ******************************************************************************
 * AT COMMAND PARAMETER ATTRIBUTE DEFINATION
 ******************************************************************************
******************************************************************************/
#define __AT_CMD_PARAM_ATTR_DEFINE__    //just for easy to find this position in SS


/* AT+ECSLEEP */
static AtValueAttr attrECSLEEP[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

/* AT+ECSAVEFAC */
static AtValueAttr attrECSAVEFAC[] = {AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL)};

/* AT+CGSN */
static AtValueAttr attrCGSN[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};


/* ATI<number> */
static AtValueAttr attrATI[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};

/* AT&F<number> */
static AtValueAttr attrATANDF[] =   {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};



#ifdef FEATURE_EXAMPLE_ENABLE
/* AT+TESTDEMO */
static AtValueAttr attrTESTDEMO[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                     AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };
/* AT+TESTA */
static AtValueAttr attrTESTA[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                    AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };
/* AT+TESTB */
static AtValueAttr attrTESTB[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                    AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };
/* AT+TESTC */
static AtValueAttr attrTESTC[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                    AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };
#endif


#ifdef FEATURE_CTWINGV1_5_ENABLE
/* AT+CTM2MSETMOD */
static AtValueAttr attrCTM2MSETMOD[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//MOD_ID
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//MOD_DATA
                                    //  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//Auto_TAUTimer_Update
                                     // AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//ON_UQMode
                                     // AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//ON_CELevel2Policy
                                     // AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//Auto_Heartbeat
                                     // AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//Wakeup_Notify
                                    //  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL)};//Protocol_mode

/* AT+CTM2MSETPM */
static AtValueAttr attrCTM2MSETPM[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//Sever_IP
                                       AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//Port
                                       AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//lifetime
                                       AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};//Object_Instance_List


static AtValueAttr attrCTM2MSETPSK[]= { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//Security_Mode
                                       AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//PSKID
                                       AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};//PSK}


/* AT+CTM2MSETPM */
static AtValueAttr attrCTM2MIDAUTHPM[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL) };//IDAuth_STR


/* AT+CTM2MSEND */
static AtValueAttr attrCTM2MSEND[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL), //Data
                                      AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };//mode

/* AT+CTM2MCMDRSP */
static AtValueAttr attrCTM2MCMDRSP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//msgid
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//token
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//rspcode
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//uri_str
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//observe
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//dataformat
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};//data

/* AT+CTM2MGETSTATUS */
static AtValueAttr attrCTM2MGETSTATUS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL), //query_type
                                    AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };//msgid
/* AT+CTM2MRMODE */
static AtValueAttr attrRMODE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//Type

static AtValueAttr attrCTM2MCTM2MCLNV[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)}; //clean nv

static AtValueAttr attrCTM2MSETIMSI[] = {AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL)}; //set imsi

#endif


#ifdef FEATURE_WAKAAMA_ENABLE
//AT+LWM2MCREATE
const static AtValueAttr attrLWM2MCREATE[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//server
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//port
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//localport
                                              AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//enderpoint
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//lifetime
                                              AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//pskId
                                              AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};//psk

//AT+LWM2MDELETE
const static AtValueAttr attrLWM2MDELETE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//clientId

//AT+LWM2MADDOBJ
const static AtValueAttr attrLWM2MADDOBJ[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//clientId
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//objectId
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//instanceid
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//resourceCount
                                              AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL)};//resourceList
//AT+LWM2MDELOBJ
const static AtValueAttr attrLWM2MDELOBJ[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//clientid
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//objectid

//AT+LWM2MREADCONF
const static AtValueAttr attrLWM2MREADCONF[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//clientid
                                                AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//objectid
                                                AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//instanceid
                                                AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//resourceid
                                                AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//valuetype
                                                AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//valuelen
                                                AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL)};//value
//AT+LWM2MWRITECONF
const static AtValueAttr attrLWM2MWRITECONF[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//clientid
                                                 AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//result

//AT+LWM2MEXECUTECONF
const static AtValueAttr attrLWM2MEXECUTECONF[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//clientid
                                                   AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//result
//AT+LWM2MNOTIFY
const static AtValueAttr attrLWM2MNOTIFY[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//clientid
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//objectid
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//instanceid
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//resourceid
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//valuetype
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//valuelen
                                              AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};//value
//AT+LWM2MUPDATE
const static AtValueAttr attrLWM2MUPDATE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//clientid
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};//withObj
#endif

#ifdef FEATURE_LIBCOAP_ENABLE
//AT+COAPCREATE
const static AtValueAttr attrCOAPCREATE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };//

//AT+COAPADDRES
const static AtValueAttr attrCOAPADDRES[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL) };//

//AT+COAPHEAD
const static AtValueAttr attrCOAPHEAD[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL), //
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//
                                             AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),//
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) }; //

//AT+COAPOPTION
const static AtValueAttr attrCOAPOPTION[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL), //
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL), //
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL), //
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL), //
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL), //
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL), //
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL), //
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL), //
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL), //
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL), //
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL), //
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL), //
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) }; //

//AT+COAPSEND
const static AtValueAttr attrCOAPSEND[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL), //
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL), //
                                           AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//
                                           AT_PARAM_ATTR_DEF(AT_LAST_MIX_STR_VAL, AT_OPT_VAL) }; //

//AT+COAPCFG
const static AtValueAttr attrCOAPCFG[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) }; //

//AT+COAPALISIGN
const static AtValueAttr attrCOAPALISIGN[] = {AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL), //
                                              AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//
                                              AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//
                                              AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL), //
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) }; //
#endif

#if defined(FEATURE_MQTT_ENABLE) || defined(FEATURE_MQTT_TLS_ENABLE)
const static AtValueAttr  attrMQTTCFG[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),   
                                            AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),   
                                            AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),                                        
                                            AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL) };

const static AtValueAttr  attrMQTTOPEN[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrMQTTCLOSE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };


const static AtValueAttr  attrMQTTCONN[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };


const static AtValueAttr  attrMQTTDISC[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrMQTTSUB[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrMQTTUNS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL) };

const static AtValueAttr  attrMQTTPUB[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_LAST_MIX_STR_VAL, AT_OPT_VAL) };
#endif

#ifdef  FEATURE_HTTPC_ENABLE
//AT+HTTPCREATE
const static AtValueAttr attrHTTPCREATE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//flag
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//host
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//authuser
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//authpasswd
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//totalCaCertlen
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//currentCaCertlen
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//caCert
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//ClientCertlen
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//clientCert
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//ClientPklen
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};//clientPk

//AT+HTTPCONNECT
const static AtValueAttr attrHTTPCON[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//id

//AT+HTTPSEND
const static AtValueAttr attrHTTPSEND[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//id          0
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//method       1
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//pathLen       2
                                           AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//path          3
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//custheadlen   4
                                           AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//custhead      5
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//contenttypelen 6
                                           AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//contenttype   7
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//contentstrLen 8
                                           AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//contentstr    9
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};//rangeflag    10
//AT+HTTPDESTROY
const static AtValueAttr attrHTTPDESTROY[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//id
#endif

#ifdef FEATURE_CISONENET_ENABLE
//AT+MIPLCONFIG
const static AtValueAttr attrMIPLCONFIG[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//mode
                                             AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_MUST_VAL),//parameter1
                                             AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_OPT_VAL)};//parameter2
//AT+MIPLCREATE
const static AtValueAttr attrMIPLCREATE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};
//AT+MIPLDELETE
const static AtValueAttr attrMIPLDELETE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

//AT+MIPLOPEN
const static AtValueAttr attrMIPLOPEN[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                           AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};
//AT+MIPLCLOSE
const static AtValueAttr attrMIPLCLOSE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

//AT+MIPLADDOBJ
const static AtValueAttr attrMIPLADDOBJ[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

//AT+MIPLDELOBJ
const static AtValueAttr attrMIPLDELOBJ[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

//AT+MIPLNOTIFY
const static AtValueAttr attrMIPLNOTIFY[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//value
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//index
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//flag
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//ackid
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};//raiflag
//AT+MIPLREADRSP
const static AtValueAttr attrMIPLREADRSP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//objectid
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//instanceid
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//resourceid
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//valuetype
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//valuelen
                                              AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//value
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//index
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//flag
                                              AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};//raiflag


//AT+MIPLWRITERSP
const static AtValueAttr attrMIPLWRITERSP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                               AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                               AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                               AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};
//AT+MIPLEXECUTERSP
const static AtValueAttr attrMIPLEXECUTERSP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                 AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                 AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                 AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};
//AT+MIPLOBSERVERSP
const static AtValueAttr attrMIPLOBSERVERSP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                 AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                 AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                 AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};
//AT+MIPLDISCOVERRSP
const static AtValueAttr attrMIPLDISCOVERRSP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };

//AT+MIPLPARAMETERRSP
const static AtValueAttr attrMIPLPARAMETERRSP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                   AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                   AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                                   AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };
//AT+MIPLUPDATE
const static AtValueAttr attrMIPLUPDATE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };

//AT+OTASTART
const static AtValueAttr attrOTASTART[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };

//AT+OTAOTASTATE
const static AtValueAttr attrOTASTATE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };


//AT+MIPLRD
const static AtValueAttr attrMIPLRD[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};

#endif

//AT+ECPING
const static AtValueAttr  attrPING[] = { AT_PARAM_ATTR_DEF(AT_MIX_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };
#ifndef FEATURE_REF_AT_ENABLE
//AT+ECDNSCFG
const static AtValueAttr  attrECDNSCFG[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};
#endif

//AT+ECIPERF=<action>,[protcol],[port],["ipaddr"],[tpt],[payload_size],[pkg_num],[duration],[report_interval]
const static AtValueAttr attrECIPERF[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };

//AT+CMDNS
const static AtValueAttr  attrECDNS[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL) };

//AT+SKTCREATE
const static AtValueAttr attrSKTCREATE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//domain (V4/V6)
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//type (TCP.UDP)
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };//protocol (tcp udp)
//AT+SKTSEND
const static AtValueAttr attrSKTSEND[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//socketId
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//data len
                                          AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//data
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//dataRai info
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};//dataExcept info
//AT+SKTSENDT
const static AtValueAttr attrSKTSENDT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//socketId
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//data len
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//dataRai info
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};//dataExcept info

//AT+SKTCONNECT
const static AtValueAttr attrSKTCONNECT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//seq
                                             AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//addr
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };//port
//AT+SKTBIND
const static AtValueAttr attrSKTBIND[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//seq
                                          AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//addr
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };//port
//AT+SKTSTATUS
const static AtValueAttr attrSKTSTATUS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };//seq
//AT+SKTDELELTE
const static AtValueAttr attrSKTDELETE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };//seq

//AT+ECSOCR
const static AtValueAttr attrECSOCR[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//type
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//protocol
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//listenport
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//receive control
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//af_type
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };//ip address

//AT+ECSOST
const static AtValueAttr attrECSOST[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//socket
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//remote addr
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//remote port
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//length
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//data
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//sequence
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//segment id
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };//segement num

//AT+ECSOSTT
const static AtValueAttr attrECSOSTT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//socket
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//remote addr
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//remote port
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//length
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};//sequence

//AT+ECSOSTF
const static AtValueAttr attrECSOSTF[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//socket
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//remote addr
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//remote port
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//flag
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//length
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//data
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//sequence
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//segment id
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };//segement num

//AT+ECSOSTFT
const static AtValueAttr attrECSOSTFT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//socket
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//remote addr
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//remote port
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//flag
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//length
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};//sequence


//AT+ECQSOS
const static AtValueAttr attrECQSOS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//socket
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//socket
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//socket
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//socket
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };//socket

//AT+ECSORF
const static AtValueAttr attrECSORF[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//socket
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };//req length

//AT+ECSOCO
const static AtValueAttr attrECSOCO[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//socket
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//remote addr
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };//remote port

//AT+ECSOSD
const static AtValueAttr attrECSOSD[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//socket
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//length
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//data
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//flag
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };//sequence
//AT+ECSOSDT
const static AtValueAttr attrECSOSDT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//socket
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//length
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//flag
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };//sequence

//AT+ECSOCL
const static AtValueAttr attrECSOCL[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };//socket

//AT+ECSONMI
const static AtValueAttr attrECSONMI[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//mode
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//max DL buffer size
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };//max DL pkg num

//AT+ECSONMIE
const static AtValueAttr attrECSONMIE[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//socket
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//mode
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//max DL buffer size
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };//max DL pkg num

//AT+ECSOSTATUS
const static AtValueAttr attrECSOSTATUS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL) };//at socket id

//AT+ECSRVSOCRTCP
const static AtValueAttr attrECSRVSOCRTCP[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//listen port
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),//domain
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL) };//local address
//AT+ECSRVSOCCLTCPLISTEN
const static AtValueAttr attrECSRVSOCCLTCPLISTEN[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//server socket id

//AT+ECSRVSOCCLTCPCL
const static AtValueAttr attrECSRVSOCCLTCPCLT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//server socket id
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL) };//client socket id

//AT+ECSRVSOCTCPSENDCLT
const static AtValueAttr attrECSRVSOCTCPSENDCLT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//client socket id
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//data length
                                            AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),//data
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//dataRai info
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};//dataExcept info

//AT+ECSRVSOCTCPLISTENSTATUS
const static AtValueAttr attrECSRVSOCTCPLISTENSTATUS[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};//server socket id

//AT+ECSRVSOCTCPSENDCLTT
const static AtValueAttr attrECSRVSOCTCPSENDCLTT[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),//client socket id
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//data length
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),//dataRai info
                                            AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};//dataExcept info

#if defined(FEATURE_CTCC_DM_ENABLE) || defined(FEATURE_CUCC_DM_ENABLE)

//AT+DMCONFIG
const static AtValueAttr attrDMCFG[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                        AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};


//AT+AUTOREGCFG
const static AtValueAttr attrAUTOREGCFG[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                              AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                              AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};
#endif


#ifdef  FEATURE_ATDEBUG_ENABLE
const static AtValueAttr attrECRST[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};

//AT+ECRFTEST
const static AtValueAttr attrECRFTEST[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL,  AT_MUST_VAL)};
//AT+ECRFNST
const static AtValueAttr attrECRFNST[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL,  AT_MUST_VAL)};
//AT+ECUNITTEST
//const static AtValueAttr attrECUNITTEST[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL)};

//AT+TESTCMD
//static AtValueAttr attrTESTCMD[] = { AT_PARAM_ATTR_DEF(AT_MIX_VAL,  AT_MUST_VAL) };

//AT^SYSTEST
const static AtValueAttr attrSYSTEST[] = { AT_PARAM_ATTR_DEF(AT_MIX_VAL,  AT_MUST_VAL) };

//AT+ECSYSTEST
const static AtValueAttr attrECSYSTEST[] = {AT_PARAM_ATTR_DEF(AT_STR_VAL,  AT_MUST_VAL)};

//AT+ECPCFG
const static AtValueAttr attrECPCFG[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};

//AT+ECPMUCFG
const static AtValueAttr attrECPMUCFG[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL),
                                   AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL)};


//AT+ECFLASHMONITORINFO
const static AtValueAttr attrECFLASHMONITORINFO[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL) };


#endif

//AT+SNTPSTART
const static AtValueAttr attrSNTPSTART[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL,  AT_MUST_VAL),//URI or IP
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL), //0~65535
                                             AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_OPT_VAL)}; // 0 ~ 1


#ifdef FEATURE_ATADC_ENABLE
//AT+ECADC
const static AtValueAttr attrADC[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL)};
#endif

// AT+NFWUPD=<cmd>[,<sn>,<len>,<data>,<crc8>]
static AtValueAttr attrNFWUPD[] = { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL), //cmd
                                    AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),  //sn
                                    AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),  //len
                                    AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),  //data
                                    AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)}; //crc

//AT+IPR
const static AtValueAttr attrIPR[] = {AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL)};

//AT+ECIPR
const static AtValueAttr attrECIPR[] = {AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL)};

//AT+ECLEDMODE
const static AtValueAttr attrLEDMODE[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL,  AT_MUST_VAL)};

//AT+ECNPICFG
const static AtValueAttr attrNPICFG[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL),
                                  AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};

//AT+ECPRODMODE
#ifdef FEATURE_PRODMODE_ENABLE
const static AtValueAttr attrPRODMODE[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL)};
#endif

#ifndef FEATURE_REF_AT_ENABLE
/*AT+ECPURC=<urcStr>,<n>*/
const static AtValueAttr  attrECPURC[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL),
                                          AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};
#endif
//AT+ICF
const static AtValueAttr attrICF[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),
                                      AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};

/*AT+ECABFOTACTL=<Str>*/
const static AtValueAttr  attrECABFOTACTL[] = { AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_MUST_VAL)};





#ifdef SOFTSIM_CT_ENABLE
/*AT+ESIM=*/
const static AtValueAttr  attrESIM[] =       { AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL),  //ID
                                               AT_PARAM_ATTR_DEF(AT_STR_VAL, AT_OPT_VAL)};  //sim data
#endif

#ifdef CUSTOMER_DAHUA
//AT+ECVER=<n>
const static AtValueAttr attrECVER[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_OPT_VAL)};
//AT+NYVT=<n>
const static AtValueAttr attrNYVT[] = {AT_PARAM_ATTR_DEF(AT_DEC_VAL, AT_MUST_VAL)};
#endif

/******************************************************************************
 ******************************************************************************
 * AT COMMAND TABLE
 ******************************************************************************
******************************************************************************/
#define __AT_CUST_CMD_TABLE_DEFINE__    //just for easy to find this position in SS

const AtCmdPreDefInfo  custAtCmdTable[] =
{
    /*
     *AT_CMD_PRE_DEFINE(name,               atProcFunc,         paramAttrList,  cmdType,                timeOutS)
    */

    // example command---ec
#ifdef FEATURE_EXAMPLE_ENABLE
    AT_CMD_PRE_DEFINE("+TESTDEMO",          ecTESTDEMO,         attrTESTDEMO,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+TESTA",             ecTESTA,            attrTESTA,          AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+TESTB",             ecTESTB,            attrTESTB,          AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+TESTC",             ecTESTC,            attrTESTC,          AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#endif
    // customer command---cc
    AT_CMD_PRE_DEFINE("+CGMI",              ccCGMI,             PNULL,              AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CGMM",              ccCGMM,             PNULL,              AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CGMR",              ccCGMR,             PNULL,              AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("I",                  ccATI,              attrATI,            AT_BASIC_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("&F",                 cdevATANDF,         attrATANDF,         AT_BASIC_CMD,       AT_DEFAULT_TIMEOUT_SEC),

    AT_CMD_PRE_DEFINE("+ECSLEEP",           ccECSLEEP,          attrECSLEEP,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSAVEFAC",         ccECSAVEFAC,        attrECSAVEFAC,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CGSN",              ccCGSN,             attrCGSN,           AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),

#ifdef FEATURE_CTWINGV1_5_ENABLE
    // lwm2m---China Telecom---ctl
    AT_CMD_PRE_DEFINE("+CTM2MVER",          ctm2mVERSION,       NULL,               AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CTM2MSETMOD",       ctm2mSETMOD,        attrCTM2MSETMOD,    AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CTM2MSETPM",        ctm2mSETPM,         attrCTM2MSETPM,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CTM2MSETPSK",       ctm2mSETPSK,        attrCTM2MSETPSK,    AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CTM2MIDAUTHPM",     ctm2mIDAUTHPM,      attrCTM2MIDAUTHPM,  AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CTM2MREG",          ctm2mREG,           NULL,               AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CTM2MUPDATE",       ctm2mUPDATE,        NULL           ,    AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC*8),//40s
    AT_CMD_PRE_DEFINE("+CTM2MDEREG",        ctm2mDEREG,         NULL,               AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC*8),//40s
    AT_CMD_PRE_DEFINE("+CTM2MSEND",         ctm2mSEND,          attrCTM2MSEND,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC*8),//40s
    AT_CMD_PRE_DEFINE("+CTM2MCMDRSP",       ctm2mCMDRSP,        attrCTM2MCMDRSP,    AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CTM2MGETSTATUS",    ctm2mGETSTATUS,     attrCTM2MGETSTATUS, AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CTM2MRMODE",        ctm2mRMODE,         attrRMODE,          AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CTM2MREAD",         ctm2mREAD,          NULL,               AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CTM2MCLNV",         ctm2mCLNV,          attrCTM2MCTM2MCLNV, AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+CTM2MSETIMSI",      ctm2mSETIMSI,       attrCTM2MSETIMSI  , AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#ifdef WITH_FOTA_RESUME
    AT_CMD_PRE_DEFINE("+CTM2MCLFOTANV",     ctm2mCLFOTANV,      NULL,               AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#endif
#endif

#ifdef FEATURE_WAKAAMA_ENABLE
    // lwm2m---wakaama
    AT_CMD_PRE_DEFINE("+LWM2MCREATE",       lwm2mCREATE,        attrLWM2MCREATE,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC*13),
    AT_CMD_PRE_DEFINE("+LWM2MDELETE",       lwm2mDELETE,        attrLWM2MDELETE,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC*13),
    AT_CMD_PRE_DEFINE("+LWM2MADDOBJ",       lwm2mADDOBJ,        attrLWM2MADDOBJ,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC*13),
    AT_CMD_PRE_DEFINE("+LWM2MDELOBJ",       lwm2mDELOBJ,        attrLWM2MDELOBJ,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC*13),
    AT_CMD_PRE_DEFINE("+LWM2MNOTIFY",       lwm2mNOTIFY,        attrLWM2MNOTIFY,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC*2),
    AT_CMD_PRE_DEFINE("+LWM2MUPDATE",       lwm2mUPDATE,        attrLWM2MUPDATE,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC*13),
    AT_CMD_PRE_DEFINE("+LWM2MREADCONF",     lwm2mREADCONF,      attrLWM2MREADCONF,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+LWM2MWRITECONF",    lwm2mWRITECONF,     attrLWM2MWRITECONF,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+LWM2MEXECUTECONF",  lwm2mEXECUTECONF,   attrLWM2MEXECUTECONF,   AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#endif

#ifdef FEATURE_LIBCOAP_ENABLE
    // Coap---coap
    AT_CMD_PRE_DEFINE("+COAPCREATE",        coapCREATE,         attrCOAPCREATE,        AT_EXT_PARAM_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+COAPDEL",           coapDELETE,         NULL,                  AT_EXT_PARAM_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+COAPADDRES",        coapADDRES,         attrCOAPADDRES,        AT_EXT_PARAM_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+COAPHEAD",          coapHEAD,           attrCOAPHEAD,          AT_EXT_PARAM_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+COAPOPTION",        coapOPTION,         attrCOAPOPTION,        AT_EXT_PARAM_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+COAPSEND",          coapSEND,           attrCOAPSEND,          AT_EXT_PARAM_CMD,       (4*AT_DEFAULT_TIMEOUT_SEC)),
    AT_CMD_PRE_DEFINE("+COAPDATASTATUS",    coapDATASTATUS,     NULL,                  AT_EXT_PARAM_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+COAPCFG",           coapCFG,            attrCOAPCFG,           AT_EXT_PARAM_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+COAPALISIGN",       coapALISIGN,        attrCOAPALISIGN,       AT_EXT_PARAM_CMD,       AT_DEFAULT_TIMEOUT_SEC),
#endif

#if defined(FEATURE_MQTT_ENABLE) || defined(FEATURE_MQTT_TLS_ENABLE)
    // Mqtt---mqtt
    AT_CMD_PRE_DEFINE("+ECMTCFG",           mqttCFG,            attrMQTTCFG,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECMTOPEN",          mqttOPEN,           attrMQTTOPEN,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECMTCLOSE",         mqttCLOSE,          attrMQTTCLOSE,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECMTCONN",          mqttCONN,           attrMQTTCONN,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECMTDISC",          mqttDISC,           attrMQTTDISC,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECMTSUB",           mqttSUB,            attrMQTTSUB,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECMTUNS",           mqttUNS,            attrMQTTUNS,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECMTPUB",           mqttPUB,            attrMQTTPUB,        AT_EXT_PARAM_CMD,   (4*AT_DEFAULT_TIMEOUT_SEC)),
#endif

#ifdef  FEATURE_HTTPC_ENABLE
    // http(s)---http
    AT_CMD_PRE_DEFINE("+HTTPCREATE",        httpCREATE,         attrHTTPCREATE,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+HTTPCON",           httpCONNECT,        attrHTTPCON,        AT_EXT_PARAM_CMD,   (12*AT_DEFAULT_TIMEOUT_SEC)),   //60s
    AT_CMD_PRE_DEFINE("+HTTPSEND",          httpSEND,           attrHTTPSEND,       AT_EXT_PARAM_CMD,   (12*AT_DEFAULT_TIMEOUT_SEC)),
    AT_CMD_PRE_DEFINE("+HTTPDESTROY",       httpDESTROY,        attrHTTPDESTROY,    AT_EXT_PARAM_CMD,   (6*AT_DEFAULT_TIMEOUT_SEC)),
#endif

#ifdef FEATURE_CISONENET_ENABLE
    // cis onenet---China Mobile
    AT_CMD_PRE_DEFINE("+MIPLCONFIG",        onenetCONFIG,       attrMIPLCONFIG,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLCREATE",        onenetCREATE,       attrMIPLCREATE,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLDELETE",        onenetDELETE,       attrMIPLDELETE,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLOPEN",          onenetOPEN,         attrMIPLOPEN,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC*2),
    AT_CMD_PRE_DEFINE("+MIPLCLOSE",         onenetCLOSE,        attrMIPLCLOSE,          AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLADDOBJ",        onenetADDOBJ,       attrMIPLADDOBJ,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLDELOBJ",        onenetDELOBJ,       attrMIPLDELOBJ,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLNOTIFY",        onenetNOTIFY,       attrMIPLNOTIFY,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLREADRSP",       onenetREADRSP,      attrMIPLREADRSP,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLWRITERSP",      onenetWRITERSP,     attrMIPLWRITERSP,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLEXECUTERSP",    onenetEXECUTERSP,   attrMIPLEXECUTERSP,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLOBSERVERSP",    onenetOBSERVERSP,   attrMIPLOBSERVERSP,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLDISCOVERRSP",   onenetDISCOVERRSP,  attrMIPLDISCOVERRSP,    AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLPARAMETERRSP",  onenetPARAMETERRSP, attrMIPLPARAMETERRSP,   AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLUPDATE",        onenetUPDATE,       attrMIPLUPDATE,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLVER",           onenetVERSION,      NULL,                   AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+OTASTART",          onenetOTASTART,     attrOTASTART,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+OTASTATE",          onenetOTASTATE,     attrOTASTATE,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+MIPLRD",            onenetMIPLRD,       attrMIPLRD,             AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#endif

    // NetManager---nm
    AT_CMD_PRE_DEFINE("+ECPING",                 nmPING,                   attrPING,                     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECDNS",                  nmECDNS,                  attrECDNS,                    AT_EXT_PARAM_CMD,   (6*AT_DEFAULT_TIMEOUT_SEC)),    //30s
    AT_CMD_PRE_DEFINE("+ECIPERF",                nmECIPERF,                attrECIPERF,                  AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+SKTCREATE",              nmSKTCREATE,              attrSKTCREATE,                AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+SKTSEND",                nmSKTSEND,                attrSKTSEND,                  AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+SKTSENDT",               nmSKTSENDT,               attrSKTSENDT,                 AT_EXT_PARAM_CMD,   12*AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+SKTCONNECT",             nmSKTCONNECT,             attrSKTCONNECT,               AT_EXT_PARAM_CMD,   (7*AT_DEFAULT_TIMEOUT_SEC)),    //35s
    AT_CMD_PRE_DEFINE("+SKTBIND",                nmSKTBIND,                attrSKTBIND,                  AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+SKTSTATUS",              nmSKTSTATUS,              attrSKTSTATUS,                AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+SKTDELETE",              nmSKTDELETE,              attrSKTDELETE,                AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+SKTRECV",              NULL,                     PNULL,                        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+SKTERROR",             NULL,                     PNULL,                        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+ECGPADDR",             NULL,                     PNULL,                        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#ifndef FEATURE_REF_AT_ENABLE
    AT_CMD_PRE_DEFINE("+ECDNSCFG",               nmECDNSCFG,               attrECDNSCFG,                 AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#endif
    AT_CMD_PRE_DEFINE(ECSOCR_NAME,            nmECSOCR,           attrECSOCR,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE(ECSOST_NAME,            nmECSOST,           attrECSOST,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE(ECSOSTT_NAME,           nmECSOSTT,          attrECSOSTT,        AT_EXT_PARAM_CMD,   12*AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE(ECSOSTF_NAME,           nmECSOSTF,          attrECSOSTF,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE(ECSOSTFT_NAME,          nmECSOSTFT,         attrECSOSTFT,       AT_EXT_PARAM_CMD,   12*AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE(ECQSOS_NAME,            nmECQSOS,           attrECQSOS,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE(ECSORF_NAME,            nmECSORF,           attrECSORF,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE(ECSOCO_NAME,            nmECSOCO,           attrECSOCO,         AT_EXT_PARAM_CMD,   (7*AT_DEFAULT_TIMEOUT_SEC)),    //35s
    AT_CMD_PRE_DEFINE(ECSOSD_NAME,            nmECSOSD,           attrECSOSD,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE(ECSOSDT_NAME,           nmECSOSDT,          attrECSOSDT,        AT_EXT_PARAM_CMD,   12*AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE(ECSOCL_NAME,            nmECSOCL,           attrECSOCL,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE(ECSONMI_NAME,           nmECSONMI,          attrECSONMI,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE(ECSONMIE_NAME,          nmECSONMIE,         attrECSONMIE,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE(ECSOSTATUS_NAME,        nmECSOSTATUS,       attrECSOSTATUS,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+ECSOCLI",              NULL,                     PNULL,                        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+ECSOSTR",              NULL,                     PNULL,                        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+ECQSOSR",              NULL,                     PNULL,                        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSRVSOCRTCP",           nmECSRVSOCRTCP,           attrECSRVSOCRTCP,             AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSRVSOCLTCPLISTEN",     nmECSRVSOCLLISTEN,        attrECSRVSOCCLTCPLISTEN,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSRVSOCLTCPCLIENT",     nmECSRVSOCLCLIENT,        attrECSRVSOCCLTCPCLT,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSRVSOTCPSENDCLT",      nmECSRVSOTCPSENDCLT,      attrECSRVSOCTCPSENDCLT,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSRVSOTCPLISTENSTATUS", nmECSRVSOTCPLISTENSTATUS, attrECSRVSOCTCPLISTENSTATUS,  AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSRVSOTCPSENDCLTT",     nmECSRVSOTCPSENDCLTT,     attrECSRVSOCTCPSENDCLTT,      AT_EXT_PARAM_CMD,   12*AT_DEFAULT_TIMEOUT_SEC),
    // SNTP---sntp
    AT_CMD_PRE_DEFINE("+ECSNTP",                 nmSNTP,                   attrSNTPSTART,                AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+SNTPSTOP",             sntpStop,                 NULL,                         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),

#if defined(FEATURE_CTCC_DM_ENABLE) || defined(FEATURE_CUCC_DM_ENABLE)
    // device manage for auto reg---dm
    AT_CMD_PRE_DEFINE("+AUTOREGCFG",        dmAUTOREGCFG,       attrAUTOREGCFG,     AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#endif

#if defined(FEATURE_CMCC_DM_ENABLE) && !defined(FEATURE_REF_AT_ENABLE)
    AT_CMD_PRE_DEFINE("+DMCONFIG",          dmDMCONFIG,         attrDMCFG,          AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+DMREGSTAT",         dmREGSTAT,          PNULL,              AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#endif

#ifdef  FEATURE_ATDEBUG_ENABLE
    // Debug---dbg
    //AT_CMD_PRE_DEFINE("+HELP",              dbgHELP,                  NULL,                      AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+POWERON",           dbgPOWERON,               NULL,                      AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+POWEROFF",          dbgPOWEROFF,              NULL,                      AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECTASKINFO",          pdevECTASKINFO,           NULL,                      AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECTASKHISTINFO",      pdevECTASKHISTINFO,       NULL,                      AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSHOWMEM",           pdevECSHOWMEM,            NULL,                      AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECRFTEST",            pdevECRFTEST,             attrECRFTEST,              AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECRFNST",             pdevECRFNST,              attrECRFNST,               AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECRFSTAT",            pdevECRFSTAT,             NULL,                      AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+ECUNITTEST",        pdevECUNITTEST,           attrECUNITTEST,            AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+DUMPDATA",          dbgDUMPDATA,              NULL,                      AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    //AT_CMD_PRE_DEFINE("+TESTCMD",           dbgTESTCMD,               attrTESTCMD,               AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("^SYSTEST",             pdevSYSTEST,              attrSYSTEST,               AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECSYSTEST",           pdevECSYSTEST,            attrECSYSTEST,             AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECLOGDBVER",          pdevECLOGDBVER,           NULL,                      AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECFSINFO",            pdevECFSINFO,             NULL,                      AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECFSFORMAT",          pdevECFSFORMAT,           NULL,                      AT_EXT_ACT_CMD,       AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECFLASHMONITORINFO",  pdevECFLASHMONITORINFO,   attrECFLASHMONITORINFO,    AT_EXT_PARAM_CMD,     AT_DEFAULT_TIMEOUT_SEC),
#endif

#ifdef FEATURE_ATADC_ENABLE
    AT_CMD_PRE_DEFINE("+ECADC",               ecADC,                attrADC,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#endif
    AT_CMD_PRE_DEFINE("+NFWUPD",              ecNFWUPD,             attrNFWUPD,        AT_EXT_PARAM_CMD,   (2 * AT_DEFAULT_TIMEOUT_SEC)),

    AT_CMD_PRE_DEFINE("+ECRST",               pdevRST,              attrECRST,         AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),

    AT_CMD_PRE_DEFINE("+FACTORYTEST",         pdevFACTORYTEST,      attrECRST,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),

    AT_CMD_PRE_DEFINE("+ECPMUCFG",            pdevECPMUCFG,         attrECPMUCFG,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECVOTECHK",           pdevECVOTECHK,        NULL,              AT_EXT_ACT_CMD,     AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECPCFG",              pdevECPCFG,           attrECPCFG,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+IPR",                 pdevIPR,              attrIPR,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECIPR",               pdevECIPR,            attrECIPR,         AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECLEDMODE",           pdevNetLight,         attrLEDMODE,       AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECNPICFG",            pdevNPICFG,           attrNPICFG,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),

#ifdef FEATURE_PRODMODE_ENABLE
    AT_CMD_PRE_DEFINE("+ECPRODMODE",          pdevECPRODMODE,       attrPRODMODE,      AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#endif

    AT_CMD_PRE_DEFINE("+ECPMUSTATUS",         pdevPMUSTATUS,        NULL,              AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#ifndef FEATURE_REF_AT_ENABLE
    AT_CMD_PRE_DEFINE("+ECPURC",              pdevECPURC,           attrECPURC,        AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#endif
    AT_CMD_PRE_DEFINE("+ICF",                 pdevICF,              attrICF,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+ECABFOTACTL",         pdevECABFOTACTL,      attrECABFOTACTL,   AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),

#ifdef SOFTSIM_CT_ENABLE
    AT_CMD_PRE_DEFINE("+ESIM",                esimDATACONFIG,      attrESIM,           AT_EXT_PARAM_CMD,   AT_DEFAULT_TIMEOUT_SEC),
#endif

#ifdef CUSTOMER_DAHUA
	AT_CMD_PRE_DEFINE("+NYSM", 		  		  pdevNYSM,	   		   NULL,			   AT_EXT_PARAM_CMD,	AT_DEFAULT_TIMEOUT_SEC),
	AT_CMD_PRE_DEFINE("+NYGOSLEEP",  		  pdevNYGOSLEEP,	   NULL,  			   AT_EXT_PARAM_CMD,	AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+SGSW", 		  		  ccSGSW,	           NULL,			   AT_EXT_PARAM_CMD,	AT_DEFAULT_TIMEOUT_SEC),   
    AT_CMD_PRE_DEFINE("+ECVER",       		  ccECVER,             attrECVER,          AT_EXT_PARAM_CMD,    AT_DEFAULT_TIMEOUT_SEC),
    AT_CMD_PRE_DEFINE("+NYVT",       		  pdevNYVT,            attrNYVT,           AT_EXT_PARAM_CMD,    AT_DEFAULT_TIMEOUT_SEC),

#endif

};



AtCmdPreDefInfoC* atcGetATCustCommandsSeqPointer(void)
{
    return  custAtCmdTable;
}

UINT32 atcGetATCustCommandsSeqNumb(void)
{
    return AT_NUM_OF_ARRAY(custAtCmdTable);
}





