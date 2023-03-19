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
#ifndef __ATC_REPLY_H__
#define __ATC_REPLY_H__

#include "CommonTypedef.h"
#include "atc_decoder.h"

/******************************************************************************
 *   +-----+                +-----+
 *   | DTE | --> AT CMD --> | DCE |
 *   +-----+                +-----+
 *
 * V.250: Table 3/V.250 ¨C Effect of V parameter on response formats
 * 1> Two types of responses: "information text" and "result codes"
 *  A) "information text" three parts: a header, text, and a trailer
 *     a> ATV0: <text><S3><S4>            (normal: <text>\r\n)
 *     b> ATV1: <S3><S4><text><S3><S4>    (normal: \r\n<text>\r\n)
 *     c> information text returned in response to manufacturer-specific commands
 *        may contain multiple lines, may therefore include: CR,LF (\r\n)
 *
 *  B) "result codes" three parts: a header, the result text, and a trailer
 *     a> ATV0: <numeric code><S3>        (normal: <numeric code>\r)
 *         example: 0\r (OK)
 *
 *     b> ATV1: <S3><S4><verbose code><S3><S4>    (normal: \r\n<verbose code>\r\n)
 *         example: \r\nOK\r\n
 *
 *     c> Three types of result codes: "final", "intermediate", and "unsolicited".
 *        i> "final": indicates the completion of a full DCE action and a willingness
 *                    to accept new commands from the DTE
 *        ii> "intermediate": suc as: the DCE moves from "command state" to
 *                            "online data state", and issues a: "CONNECT"
 *        iii> "Unsolicited": URC
 *
 *  C) "Extended syntax result codes"
 *     a> "extended syntax result codes" format is the same as "result codes" regard to
 *        headers and trailers; (Note: also controlled by ATV)
 *     b> "extended syntax result codes" have no numeric equivalent,
 *        and are always issued in alphabetic form, so only: ===  ATV1 ===?
 *     c> "Extended syntax result codes" may be:
 *         "final", "intermediate", "unsolicited"
 *     d> "Extended syntax result codes" shall be prefixed by the "+" character
 *        example: \r\n+<name>\r\n  => \r\n+CME ERROR: <err>\r\n
 *
 *  D) "Information text formats for test commands"
 *    example:
 *     a> (0)
 *     b> (0,1,4)
 *     c> (1-5)
 *     d> (0,4-6,9,11-12)
******************************************************************************/

/******************************************************************************
 * 1. Additional commands may follow an extended syntax command on the same command line if a
 *    semicolon (";", IA5 3/11) is inserted after the preceding extended command as a separator.
 * 2. Extended syntax commands may appear on the same command line after a basic syntax command
 *    without a separator
 * 3. So whether a basic syntax command could be following after an command? - maybe can't --TBD
 *
 * Example:
 * AT command:
 *  ATCMD1 CMD2=12; +CMD1; +CMD2=,,15; +CMD2?; +CMD2=?<CR>
 *  ^basic|       |                                    ^
 *        ^       ^                                    |
 * ================================================================
 * Information responses and result codes:
 * A) If ATV1 (default)
 *    <CR><LF>+CMD2: 3,0,15,"GSM"<CR><LF>   --> information text
 *    <CR><LF>+CMD2: (0-3),(0,1),(0-12,15),("GSM","IRA")<CR><LF>    --> information text
 *    <CR><LF>OK<CR><LF>                    --> final result code
 * B) If ATV0
 *    +CMD2: 3,0,15,"GSM"<CR><LF>           --> information text
 *    +CMD2: (0-3),(0,1),(0-12,15),("GSM","IRA")<CR><LF>    --> information text
 *    0<CR>                                 --> final result code
 *
 * =============================================================
 * Error command, or process error, response:
 * A) If command is not accepted/right, such cases:
 *     a> command itself is invalid, as: ATBCDFG
 *     b> command cannot be performed for some reasons, such as:
 *        i>  one or more mandatory values are omitted, or
 *        ii> one or more values are of the wrong type or
 *        iii> outside the permitted range.
 *   if AT1:
 *     <CR><LF>ERROR<CR><LF>                --> final result code
 *   if AT0
 *     4<CR>                                --> final result code
 * B) If command was not processed due to an error related to MT operation,
 *    response (no matter the ATV value):
 *    <CR><LF>+CME ERROR: <err><CR><LF>
 *
******************************************************************************/


/******************************************************************************
 *****************************************************************************
 * MARCO
 *****************************************************************************
******************************************************************************/
//basicResultStr
#define ATC_BASIC_RESULT_STR_LEN        64
#define ATC_EXTENDED_RESULT_STR_LEN     128
#define ATC_IND_RESP_MAX_STR_LEN        4096


#define ATC_SUCC_CODE               0

/******************************************************************************
 *****************************************************************************
 * Enum & structure
 *****************************************************************************
******************************************************************************/
/*
 * <n>: integer type.
 *  0 disable +CME ERROR: <err> result code and use ERROR instead
 *  1 enable +CME ERROR: <err> result code and use numeric <err> values (refer subclause 9.2)
 *  2 enable +CME ERROR: <err> result code and use verbose <err> values (refer subclause 9.2)
*/
typedef enum AtcCMEERetType_Enum
{
    ATC_CMEE_DISABLE_ERROR_CODE = 0,
    ATC_CMEE_NUM_ERROR_CODE,
    ATC_CMEE_VERBOSE_ERROR_CODE
}AtcCMEERetType;


/*
 * REFER: V.250, 5.7.1, Table 1/V.250 esult codes
*/
typedef enum AtResultCode_Tag
{
    AT_RC_OK = 0,           /* AT command done/acknowledged */
    AT_RC_CONNECT = 1,      /* A connection has been established;
                               the DCE is moving from command state to online data state */
    AT_RC_RING = 2,         /* An incoming call signal from the network, -- not used now */
    AT_RC_NO_CARRIER = 3,   /* The connection has been terminated or the attempt to establish
                               a connection failed*/
    AT_RC_ERROR = 4,        /* Command not recognized, command line maximum length exceeded,
                               parameter value invalid, or other problem with processing the command line*/
    AT_RC_NO_DIALTONE = 6,  /* No dial tone detected, -- not used now*/
    AT_RC_BUSY = 7,         /* Engaged (busy) signal detected -- not used now*/

    AT_RC_NO_ANSWER = 8,    /* (Wait for Quiet Answer) dial modifier was used, but
                               remote ringing followed by five seconds of silence was not
                               detected before expiration of the connection timer -- not used now */

    /*
     * Extended syntax result codes:
    */
    AT_RC_CONTINUE = 10,    /* AT procedure not finished/continued, guard timer should not stop */
    AT_RC_NO_RESULT = 11,   /* AT command done, guard timer should stop, but no "result code" should be replied/printed */
    AT_RC_RAW_INFO_CONTINUE,/* AT procedure not finished/continued, guard timer should not stop, and response the RAW infornation text*/

    AT_RC_CME_ERROR,        /* +CME ERROR */
    AT_RC_CMS_ERROR,        /* +CMS ERROR */



    AT_RC_CUSTOM_ERROR = 20,
    AT_RC_CIS_ERROR,            //Timer will Stop, +CIS ERROR
    AT_RC_HTTP_ERROR,           //Timer will Stop, +HTTP ERROR
    AT_RC_LWM2M_ERROR,          //Timer will Stop, +LWM2M ERROR
    AT_RC_SOCKET_ERROR,         //Timer will Stop, +SOCKET ERROR
    AT_RC_MQTT_ERROR,           //Timer will Stop, +MQTT ERROR
    AT_RC_COAP_ERROR,           //Timer will Stop, +COAP ERROR
    AT_RC_CTM2M_ERROR,          //Timer will Stop, +CTM2M ERROR
    AT_RC_DM_ERROR,             //DM ERROR
    AT_RC_ADC_ERROR,            //ADC ERROR
    AT_RC_EXAMPLE_ERROR,        //EC ERROR
    AT_RC_ECSOC_ERROR,          //Timer will Stop, +CME ERROR
    AT_RC_ECSRVSOC_ERROR,       //EC server soc error
    AT_RC_FWUPD_ERROR,          //FWUPD ERROR

    #if 0
    AT_RC_CIS_ERROR = 0x020000,             //Timer will Stop, CIS ERROR will be sent
    AT_RC_HTTP_ERROR = 0x030000,            //Timer will Stop, HTTP ERROR will be sent
    AT_RC_LWM2M_ERROR = 0x040000,           //Timer will Stop, LWM2M ERROR will be sent
    AT_RC_SOCKET_ERROR = 0x050000,          //Timer will Stop, SOCKET ERROR will be sent
    AT_RC_MQTT_ERROR = 0x060000,        //Timer will Stop, SOCKET ERROR will be sent
    AT_RC_COAP_ERROR = 0x070000,        //Timer will Stop, SOCKET ERROR will be sent
    AT_RC_DM_ERROR = 0x080000,        //Timer will Stop, SOCKET ERROR will be sent
    AT_RC_ECSOC_ERROR = 0x090000,          //Timer will Stop, SOCKET ERROR will be sent
    AT_RC_EXAMPLE_ERROR = 0x0A0000,
    #endif

}AtResultCode;


#ifdef  FEATURE_REF_AT_ENABLE
/*
 * Some REF AT don't need to clear blank char after colon
*/
typedef struct AtcRefIgnoreClearAt_Tag
{
    const CHAR  *pAtName;
    UINT32      atNameStrLen;
}AtcRefIgnoreClearAt;

#endif


/******************************************************************************
 *****************************************************************************
 * external API
 *****************************************************************************
******************************************************************************/

/*
 * AT REPLY
*/
CmsRetId atcReply(UINT16 srcHandle, AtResultCode resCode, UINT32 errCode, const CHAR *pRespInfo);
CmsRetId atcReplyNoOK(UINT16 srcHandler, AtResultCode resCode, UINT32 errCode, const CHAR *pRespInfo);

/*
 * AT URC
*/
CmsRetId atcURC(UINT32 chanId, const CHAR *pUrcStr);

/*
 * send the result code string
*/
CmsRetId atcSendResultCode(AtChanEntity *pAtChanEty, AtResultCode resCode, const CHAR *pResultStr);

/*
 * send echo string
*/
CmsRetId atcEchoString(AtChanEntityP pAtChanEty, const CHAR *pStr, UINT32 strLen);

/*
 * Send response string: "pStr" via AT channel, without any modification, or suppression
*/
CmsRetId atcSendRawRespStr(AtChanEntity *pAtChanEty, const CHAR *pStr, UINT32 strLen);

/*
 * whether URC API configured
*/
BOOL atcBeURCConfiged(UINT32 chanId);

#endif

