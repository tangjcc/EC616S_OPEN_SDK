#ifndef PSIF_HDR_API_H
#define PSIF_HDR_API_H

/******************************************************************************
 ******************************************************************************
 Copyright:      - 2018- Copyrights of EigenComm Ltd.
 File name:      - psifadpt.h
 Description:    - ps netif UL/DL function header
 History:        - 01/02/2018, Originated by xwang
 ******************************************************************************
******************************************************************************/
#include "pspdu.h"

typedef enum PsIfRetEnum_Tag
{
    PSIF_SUCC = 0,
    PSIF_ERROR
}PsIfRetEnum;

typedef UINT8   PsIfRet;

/******************************************************************************
 *                           DL, NW -> UE, PS -> LWIP                        *
******************************************************************************/

/*
 * PSIF API:
 * DL IP PKG memory allocation API
 * 1> Here, LWIP could reserve the "pbuf" memory before this return memory, and LWIP don't need to copy the PKG data again 
 *    in function "ps_receive_data()";
 * 2> This API dhouls safe for several tasks access 
 * 3> ip pkg len must not bigger than: (PBUF_POOL_BUFSIZE - sizeof(struct pbuf) - sizeof(DlPduBlock))
 * The memory allocated:
 * +-----------------+--------------------------+--------------------------+
 * | pbuf (16 bytes) |   DlPduBlock (12 bytes)  |      ip pkg Len          |
 * +-----------------+--------------------------+--------------------------+
 *                   ^
 *                   | return address
 * 
 * Input: UINT16 ipPkgLen = ip pkg len
 *
 * Note: DlPduBlock not need to init in this API
*/
DlPduBlock *PsifAllocDlIpPkgBlockMem(UINT16 ipPkgLen);

/*
 * LWIP API
 * DL IP PKG memory free API, free all list memory
 * if PS found the IP PKG is not right, could free the memory
*/
void PsifFreeDlIpPkgBlockList(DlPduBlock *pPkgBlock);

/*
 * LWIP API
 * process the DL IP PKG
 * 1> This API maybe called in ISR, don't take too much operation;
 * 2> PS -> LWIP, could send several IP pkg at one time;
 * 3> LWIP response for the memory free;
 * 4> 0 success,others fail
*/
PsIfRet PsifRecvDlIpPkg(UINT8 cid, DlPduBlock *pPduHdr);

/*
 * LWIP API
*/
void PsifFreeUlIpPkgMem(UINT8 *pPkgData);


/*
 * LWIP API
 * Whether LWIP DL high water, if return not zero, don't call "PsifAllocDlIpPkgBlockMem()" to allocate the DL memory;
*/
UINT8 PsifIsDlHighWater(void);


/*
 * LWIP API
 * Called by PS, notify LWIP task whether PS is suspended or resumed;
 * Note:
 * 1> if need to some operation with NETIF, suggest handle it in LWIP task;
 * 2> "bSuspend" = TRUE, just means PS suspended, can't send any UL pkg;
 * 3> "bSuspend" = FALSE, just means PS resumed, then LWIP could send UL pkg (maybe pended UL pkg);
 * 4> In some case, maybe UL "exception" data could still allow to send if suspended, here, we don't support it now; - TBD
*/
void PsifPsResumeSuspendInd(BOOL bSuspend);

/*
 * LWIP API
 * Check whether any UL pending(suspended) pkg in LWIP task;
*/
BOOL PsifAnySuspendULPendingPkg(void);


#endif

