/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atc_reply_err_map.c
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "atc_reply_err_map.h"
#include "cmicomm.h"
#include "at_sock_entity.h"

/******************************************************************************
 ******************************************************************************
 * Global variable
 ******************************************************************************
******************************************************************************/

/*
 * cmeIdStrOrderTbl
 *  CME ERROR ID -> ERROR STRING table.
 * !!!! NOTE: ERROR ID should in ascending order !!!!
*/
const AtcErrStrMapping  cmeIdStrOrderTbl[] =
{
    /* !!! should in ascending order !!! */
    {CME_MT_NO_CONNECTION,                              "no connection to phone"},
    {CME_MT_LINK_RESERVED,                              "phone-adaptor link reserved"},
    {CME_OPERATION_NOT_ALLOW,                           "operation not allowed"},
    {CME_OPERATION_NOT_SUPPORT,                         "operation not supported"},
    {CME_PH_SIM_PIN_REQ,                                "PH-SIM PIN required"},
    {CME_PH_FSIM_PIN_REQ,                               "PH-FSIM PIN required"},
    {CME_PH_FSIM_PUK_REQ,                               "PH-FSIM PUK required"},
    {CME_SIM_NOT_INSERT,                                "SIM not inserted"},
    {CME_SIM_PIN_REQ,                                   "SIM PIN required"},
    {CME_SIM_PUK_REQ,                                   "SIM PUK required"},
    {CME_SIM_FAILURE,                                   "SIM failure"},
    {CME_SIM_BUSY,                                      "SIM busy"},
    {CME_SIM_WRONG,                                     "SIM wrong"},
    {CME_INCORRECT_PASSWORD,                            "incorrect password"},
    {CME_SIM_PIN2_REQ,                                  "SIM PIN2 required"},
    {CME_SIM_PUK2_REQ,                                  "SIM PUK2 required"},
    {CME_MEMORY_FULL,                                   "memory full"},
    {CME_INVALID_INDEX,                                 "invalid index"},
    {CME_NOT_FOUND,                                     "not found"},
    {CME_MEMORY_FAILURE,                                "memory failure"},
    {CME_TEXT_STR_TOO_LONG,                             "text str too long"},
    {CME_INVALID_CHAR_IN_TXT_STR,                       "invalid char"},
    {CME_DIAL_STR_TOO_LONG,                             "dial str too long"},
    {CME_INVALID_CHAR_IN_DIAL_STR,                      "invalid char in dial str"},
    {CME_NO_NW_SERVICE,                                 "no nw service"},
    {CME_NW_TIMEOUT,                                    "nw timeout"},
    {CME_NW_NOT_ALLOWED_EC_ONLY,                        "network not allowed - emergency calls only"},
    {CME_NW_PERSONAL_PIN_REQ,                           "network personalization PIN required"},
    {CME_NW_PERSONAL_PUK_REQ,                           "network personalization PUK required"},
    {CME_NW_SUBSET_PERSONAL_PIN_REQ,                    "network subset personalization PIN required"},
    {CME_NW_SUBSET_PERSONAL_PUK_REQ,                    "network subset personalization PUK required"},
    {CME_SRV_PROVIDER_PERSONAL_PIN_REQ,                 "service provider personalization PIN required"},
    {CME_SRV_PROVIDER_PERSONAL_PUK_REQ,                 "service provider personalization PUK required"},
    {CME_CORPORATE_PERSONAL_PIN_REQ,                    "corporate personalization PIN required"},
    {CME_CORPORATE_PERSONAL_PUK_REQ,                    "corporate personalization PUK required"},
    {CME_HIDDEN_KEY_REQ,                                "hidden key required"},
    {CME_EAP_METHOD_NOT_SUPPORT,                        "EAP method not supported"},
    {CME_INCORRECT_PARAM,                               "Incorrect parameters"},
    {CME_CMD_IMPLEMENTED_BUT_CUR_DISABLED,              "command implemented but currently disabled"},
    {CME_CMD_ABORT_BY_USER,                             "command aborted by user"},
    {CME_NOT_ATTACHED_DUE_TO_RESTRICT,                  "not attached to network due to MT functionality restrictions"},
    {CME_MODEM_NOT_ALLOWED_EC_ONLY,                     "modem not allowed - MT restricted to emergency calls only"},
    {CME_OPER_NOT_ALLOWED_DUE_TO_RESTRICT,              "operation not allowed because of MT functionality restrictions"},
    {CME_FIXED_DIAL_NUM_ONLY,                           "fixed dial number only allowed"},
    {CME_TEMP_OOS_DUE_TO_OTHER_USAGE,                   "temporarily out of service due to other MT usage"},
    {CME_LANG_NOT_SUPPORT,                              "language/alphabet not supported"},
    {CME_UNEXPECTED_DATA_VALUE,                         "unexpected data value"},
    {CME_SYS_FAIL,                                      "system failure"},
    {CME_DATA_MISSING,                                  "data missing"},
    {CME_CALL_BARRED,                                   "call barred"},
    {CME_MSG_WAIT_IND_SUB_FAIL,                         "message waiting indication subscription failure"},
    {CME_UNKNOWN,                                       "unknown error"},
    // 101 - 150 for use by GPRS and EPS
    {CME_ILLEGAL_MS,                                    "Illegal MS (#3)"},
    {CME_ILLEGAL_ME,                                    "Illegal ME (#6)"},
    {CME_GPRS_SERVICES_NOT_ALLOWED,                     "GPRS services not allowed (#7)"},
    {CME_GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED,   "GPRS services and Non GPRS services not allow"},
    {CME_PLMN_NOT_ALLOWED,                              "PLMN not allowed (#11)"},
    {CME_LOCATION_AREA_NOT_ALLOWED,                     "Location area not allowed (#12)"},
    {CME_ROAMING_NOT_ALLOWED_IN_THIS_LOCATION_AREA,     "Roaming not allowed in this location area (#13)"},
    {CME_GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN,        "GPRS services not allowed in this plmn"},
    {CME_NO_SUITABLE_CELLS_IN_LOCATION_AREA,            "no suitable cells in location area"},
    {CME_CONGESTION,                                    "congestion"},
    {CME_INSUFFICIENT_RESOURCES,                        "insufficient resources"},
    {CME_MISSING_OR_UNKNOWN_APN,                        "missing or unknown apn"},
    {CME_UNKNOWN_PDP_ADDRESS_OR_PDP_TYPE,               "unknown pdp address or pdp type"},
    {CME_USER_AUTHENTICATION_FAILED,                    "user authentication failed"},
    {CME_ACTIVATE_REJECT_BY_GGSN_SERVING_GW_OR_PDN_GW,  "activate rej by GGSN serving GW or PDN GW"},
    {CME_ACTIVATE_REJECT_UNSPECIFIED,                   "activate rej unspecified"},
    {CME_SERVICE_OPTION_NOT_SUPPORTED,                  "service option not supported (#32)"},
    {CME_REQUESTED_SERVICE_OPTION_NOT_SUBSCRIBED,       "requested service option not subscribed (#33)"},
    {CME_SERVICE_OPTION_TEMPORATILY_OUT_OF_ORDER,       "service option temporarily out of order (#34)"},
    {CME_FEATURE_NOT_SUPPORTED,                         "feature not supported"},
    {CME_SEMANTIC_ERRORS_IN_THE_TFT_OPERATION,          "semantic errors in the TFT operation"},
    {CME_SYNTACTICAL_ERRORS_IN_THE_TFT_OPERATION,       "syntactical errors in the TFT operation"},
    {CME_UNKNOWN_PDP_CONTEXT,                           "unknown pdp context"},
    {CME_SEMANTIC_ERRORS_IN_PACKET_FILTERS,             "semantic errors in packet filters"},
    {CME_SYNTACTICAL_ERRORS_IN_PACKET_FILTERS,          "syntactical errors in packet filters"},
    {CME_PDP_CONTEXT_WITHOUT_TFT_ALREADY_ACTIVATED,     "pdp context without TFT already activated"},
    {CME_UNSPECIFIED_GPRS_ERROR,                        "unspecified GPRS error"},
    {CME_PDP_AUTHENTICATION_FAILURE,                    "PDP authentication failure"},
    {CME_INVALID_MOBILE_CLASS,                          "invalid mobile class"},
    //171 - 256 are reserved for use by GPRS or EPS
    {CME_LAST_PDN_DISCONNECTION_NOT_ALLOWED,                                "Last PDN disconnection not allowed"},
    {CME_SEMANTICALLY_INCORECT_MESSAGE,                                     "semantically incorect message"},
    {CME_MANDATORY_INFORMATION_ELEMENT_ERROR,                               "mandatory information element error"},
    {CME_INFORMATION_ELEMENT_NON_EXISTENT_OR_NOT_IMPLEMENTED,               "information element non exist or not implemented"},
    {CME_CONDITIONAL_IE_ERROR,                                              "conditional ie error"},
    {CME_PROTOCOL_ERROR_UNSPECIFIED,                                        "protocol error unspecified"},
    {CME_OPERATOR_DETERMINED_BARRING,                                       "Operator Determined Barring"},
    {CME_MAX_NUMBER_OF_PDP_CONTEXTS_REACHED,                                "max num of pdp contexts reached"},
    {CME_REQUESTED_APN_NOT_SUPPORTED_IN_CURRENT_RAT_AND_PLMN_COMBINATION,   "requested apn not supported in current RAT and PLMN combination"},
    {CME_REQUEST_REJECTED_BEARER_CONTROL_MODE_VIOLATION,                    "request rejected bearer control mode violation"},
    {CME_UNSUPPORTED_QCI_VALUE,                                             "unsupported qci value"},
    {CME_USER_DATA_TRANSMISSION_VIA_CONTROL_PLANE_IS_CONGESTED,             "user data transmission via control plane is congested"},
    //301 -399 for internal
    {CME_UE_BUSY,                                       "ue busy"},
    {CME_NOT_POWER_ON,                                  "ue not power on"},
    {CME_PDN_NOT_ACTIVED,                               "pdn not actived"},
    {CME_PDN_NOT_VALID,                                 "pdn not valid"},
    {CME_PDN_INVALID_TYPE,                              "pdn type invalid"},
    {CME_PDN_NO_PARAM,                                  "pdn leak param"},
    {CME_UE_FAIL,                                       "ue fail"},
    {CME_PDP_APN_AND_PDN_TYPE_DUPLICATE_USED,           "pdn type and APN duplicate used"},
    {CME_PDP_PAP_AND_EITF_NOT_MATCHED,                  "PAP and EITF not matched"},
    {CME_SIM_PIN_DISABLED,                              "SIM PIN disabled"},
    {CME_SIM_PIN_ALREADY_ENABLED,                       "SIM PIN already enabled"},
    {CME_SIM_PIN_WRONG_FORMAT,                          "SIM PIN wrong format"},
    {CME_PS_INTERNAL_ERROR_MAX1,                        "PS internal error max1"},  //useless

    /*
     * 401 - 500 are reserved for socket error.
     *  Defined in: cms_sock_mgr.h
     * should also add here, - TBD
    */

    /*
     * 500 - 600 is reserved for REF error
    */
    {CME_REF_START_ERROR,                               "REF CME ERROR START"},  //useless
    {CME_REQ_PARAM_NOT_CFG,                             "Required parameter not configured"},
    {CME_TUP_NOT_REGISTERED,                            "TUP not registered"},
    {CME_AT_INTERNAL_ERROR,                             "AT internal error"},
    {CME_CID_IS_ACT,                                    "CID is active"},
    {CME_INCORRECT_STATE_FOR_CMD,                       "Incorrect state for command"},
    {CME_CID_INVALID,                                   "CID is invalid"},
    {CME_CID_NOT_ACT,                                   "CID is not active"},
    {CME_DEACT_LAST_ACT_CID,                            "Deactivate the last active CID"},
    {CME_CID_NOT_DEFINED,                               "CID is not defined"},
    {CME_UART_PARITY_ERROR,                             "UART parity error"},
    {CME_UART_FRAME_ERROR,                              "UART frame error"},
    {CME_IN_CFUN0_STATE,                                "UE is in minimal function mode"},
    {CME_CMD_ABORT_ONGOING,                             "AT command aborted: in processing"},
    {CME_CMD_ABORT_ERROR,                               "AT command aborted: error"},
    {CME_CMD_INTERRUPT,                                 "Command interrupted"},
    {CME_CFG_CONFLICT,                                  "Configuration conflicts"},
    {CME_DURING_FOTA_UPDATING,                          "During FOTA updating"},
    {CME_NOT_AT_ALLOC_SOCKET,                           "Not the AT allocated socket"},
    {CME_USIM_PIN_BLOCKED,                              "USIM PIN is blocked"},
    //{CME_SIM_PIN_BLOCKED = CME_USIM_PIN_BLOCKED,
    {CME_USIM_PUK_BLOCKED,                              "USIM PUK is blocked"},
    {CME_NOT_MIPI_MODE,                                 "Not mipi module"},
    {CME_FILE_NOT_FOUND,                                "File not found"},
    {CME_CONDITION_NOT_SATISFIED,                       "conditions of use not satisfied"},
    {CME_AT_UART_BUF_ERROR,                             "AT UART buffer error"},
    {CME_BACK_OFF_TIME_RUNNING,                         "Back off timer is running"},
    //
    {CME_REF_END_ERROR,                                 "REF CME ERROR END"},   //useless


    /*
     * ADD here, in order !!!!
    */



    {CME_MAX_ERROR,                     "last internal unknown error"}
};



/******************************************************************************
 ******************************************************************************
 * STATIC FUNCTION
 ******************************************************************************
******************************************************************************/

#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position----Below for internal use


/******************************************************************************
 ******************************************************************************
 * EXTERNAL/GLOBAL FUNCTION
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS


/**
  \fn           const CHAR* atcGetCMEEStr(UINT16 errId)
  \brief        Get CMEE error string, accord to the error id
  \param[in]    errId       CMEE error ID
  \returns      const CHAR
*/
const CHAR* atcGetCMEEStr(const UINT32 errId)
{
    UINT32  startIdx = 0, endIdx = sizeof(cmeIdStrOrderTbl)/sizeof(AtcErrStrMapping) - 1;
    UINT32  curIdx = 0;

    OsaDebugBegin(errId > 0 && errId < CME_MAX_ERROR, errId, CME_MAX_ERROR, 0);
    return "Invalid error code";
    OsaDebugEnd();

    while (startIdx + 1 < endIdx)
    {
        curIdx = (startIdx + endIdx) >> 1;
        if (errId == cmeIdStrOrderTbl[curIdx].errId)
        {
            return cmeIdStrOrderTbl[curIdx].pStr;
        }
        else if (errId > cmeIdStrOrderTbl[curIdx].errId)
        {
            startIdx = curIdx;
        }
        else    //errId < cmeIdStrOrderTbl[curIdx].errId
        {
            endIdx = curIdx;
        }
    }

    if (errId == cmeIdStrOrderTbl[startIdx].errId)
    {
        return cmeIdStrOrderTbl[startIdx].pStr;
    }
    else if (errId == cmeIdStrOrderTbl[endIdx].errId)
    {
        return cmeIdStrOrderTbl[endIdx].pStr;
    }

    GosDebugBegin(FALSE, errId, 0, 0);
    GosDebugEnd();

    return "unknown";
}

