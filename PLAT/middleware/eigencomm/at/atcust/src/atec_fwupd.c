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

#include "ec_fwupd_api.h"
#include "at_fwupd_task.h"
#include "atec_fwupd.h"

/******************************************************************************
 * @brief : ecNFWUPD
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
CmsRetId ecNFWUPD(const AtCmdInputContext *pAtCmdReq)
{
    CmsRetId        rc = CMS_RET_SUCC;
    AtParaRet      ret = AT_PARA_ERR;
    UINT32   reqHandle = AT_SET_SRC_HANDLER(pAtCmdReq->tid, CMS_DEFAULT_SUB_AT_ID, pAtCmdReq->chanId);
    UINT32   operaType = (UINT32)pAtCmdReq->operaType;
    INT32      cmdCode = FWUPD_CMD_CODE_MAXNUM;
    INT32        pkgSn = 0;
    INT32       nbytes = 0;
    UINT16      crcLen = 0;
    UINT8       crcStr[3] = {'\0'};
    FwupdReqMsg_t  msg;

    FWUPD_REQMSG_INIT(msg);

    switch (operaType)
    {
        case AT_TEST_REQ:          /* AT+NFWUPD=? */
        {
            rc = atcReply(reqHandle, AT_RC_OK, ATC_SUCC_CODE, (CHAR *)"+NFWUPD: (0-6)\r\n");
            break;
        }

        case AT_SET_REQ:           /* AT+NFWUPD= */
        {
            if((ret = atGetNumValue(pAtCmdReq->pParamList, 0, &cmdCode, FWUPD_CMD_CODE_BEGIN, FWUPD_CMD_CODE_END, FWUPD_CMD_CODE_MAXNUM)) != AT_PARA_ERR)
            {
                if((ret = atGetNumValue(pAtCmdReq->pParamList, 1, &pkgSn, FWUPD_FW_PSN_MINNUM, FWUPD_FW_PSN_MAXNUM, FWUPD_FW_PSN_MINNUM)) != AT_PARA_ERR)
                {
                    if((ret = atGetNumValue(pAtCmdReq->pParamList, 2, &nbytes, 4, FWUPD_DATA_BYTES_MAXNUM, 0)) != AT_PARA_ERR)
                    {
                        if((ret = atGetStrValue(pAtCmdReq->pParamList, 3, msg.hexStr, (FWUPD_DATA_HEXSTR_MAXLEN + 1), &msg.strLen, (CHAR *)NULL)) != AT_PARA_ERR)
                        {
                            if(FWUPD_HEXSTR_LEN(nbytes) == msg.strLen)
                            {
                                ret = atGetStrValue(pAtCmdReq->pParamList, 4, crcStr, 3, &crcLen , (CHAR *)NULL);
                                if(0 > cmsHexStrToHex(&msg.crc8, 1, (const CHAR*)crcStr, 3))
                                {
                                    ret = AT_PARA_ERR;
                                }
                            }
                            else
                            {
                                ret = AT_PARA_ERR;
                            }
                        }
                    }
                }
            }

            if(AT_PARA_ERR != ret)
            {
                FWUPD_initTask();

                msg.atHandle = reqHandle;
                msg.cmdCode  = cmdCode;
                msg.pkgSn    = pkgSn;
                ret = FWUPD_sendMsg(&msg);
            }

            if(AT_PARA_ERR == ret)
            {
                rc = atcReply(reqHandle, AT_RC_FWUPD_ERROR, FWUPD_EC_PARAM_INVALID, NULL);
            }
            break;
        }

        default:
        {
            rc = atcReply(reqHandle, AT_RC_FWUPD_ERROR, FWUPD_EC_OPER_UNSUPP, NULL);
            break;
        }
    }

    return rc;
}


