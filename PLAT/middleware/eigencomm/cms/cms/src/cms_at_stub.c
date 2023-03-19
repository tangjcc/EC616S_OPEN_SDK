/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: cms_at_stub.c
*
*  Description: For some project, maybe not need AT module, but some AT APIs
*               called in CMS task, here, just define AT stub functions
*
*  History:
*
*  Notes:
*
******************************************************************************/
#include "osasys.h"

void atInit(void)
{
    return;
}

void atProcSignal(const SignalBuf *pSig)
{

    return;
}



#if 0
extern __WEAK void atInit()
{
    OsaDebugBegin(FALSE, 0, 0, 0);
    OsaDebugEnd();
    return;
}

extern __WEAK void atProcSignal(const SignalBuf *pSig)
{
    OsaDebugBegin(FALSE, pSig->sigId, pSig->sigBodyLen, 0);
    OsaDebugEnd();

    switch(pSig->sigId)
    {
        case SIG_AT_CMD_STR_REQ:
        {
            /* free the buffer */
            AtCmdStrReq *pAtCmdReq = (AtCmdStrReq *)(pSig->sigBody);

            if (pAtCmdReq->pAtStr != PNULL)
            {
                OsaFreeMemory(&(pAtCmdReq->pAtStr));
            }
            //atcProcAtCmdStrReqSig((AtCmdStrReq *)(pSig->sigBody));
            break;
        }

        default:
            break;
    }

    /* signal freed in cms_main.c*/

    return;
}
#endif

