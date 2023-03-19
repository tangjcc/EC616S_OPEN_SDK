#ifndef PSIF_HDR_ADAPT_H
#define PSIF_HDR_ADAPT_H

/******************************************************************************
 ******************************************************************************
 Copyright:      - 2018- Copyrights of EigenComm Ltd.
 File name:      - psifadpt.h
 Description:    - ps netif adapt later
 History:        - 01/02/2018, Originated by xwang
 ******************************************************************************
******************************************************************************/

#include "commontypedef.h"
#include "pspdu.h"
#include "lwip/netif.h"
#include "psifapi.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************** 
 *****************************************************************************
 * MARCO
 *****************************************************************************
******************************************************************************/
    
#define PS_NETIF_PREFIX_NAME 'p'
#define INVALID_CID 255
#define PS_MAX_DNS_NUM 2
#define PS_IF_UL_MAGIC_WORD 0x3456
#define LWIP_UL_PDU_MEM_TYPE  1


/****************************************************************************** 
 *****************************************************************************
 * STRUCT
 *****************************************************************************
******************************************************************************/

/*
 * PS IF control header allocated before UL IP PKG send to PS
 *
 * +---------------------------+----------------------------------------------------+
 * | PsifUlDataHeader(4 bytes) |                  UL IP PKG                         |
 * +---------------------------+----------------------------------------------------+
*/
typedef struct PsifUlDataHeader_Tag
{
    UINT16 magic; //PS_UL_MAGIC
    UINT16 ulLen; // UL packet length
}PsifUlDataHeader;


/******************************************************************************
 *****************************************************************************
 * Functions
 *****************************************************************************
******************************************************************************/



/******************************************************************************
 *                   UL, UE -> LWIP, LWIP -> PS                               *
******************************************************************************/

/*
 * PS/CEDR API
 *  UL ip/non-ip PDU list
 * 1> "DataPduInfo" use the dynamic memory (HEAP), and free later in the target TASK
*/
extern void CedrUlPkgListDataReqFromLwip(UINT8 cid, UINT16 totalLen, UlPduInfo *pHdr, UlPduInfo *pTailer);


/*
 * PS/CEDR API
 * Whether PS UL high water, if set to TRUE, can't send the UL data;
*/
extern UINT8 CedrUlIsPduHighWater(void);

/*
 * PS/CEDR API
 * Whether PS under test loop back mode
*/
extern BOOL CedrBeTestLoop(void);


#if 0
/*
* add ps total ul data size
*/
void PsAddTotalUlDataSize(UINT16 size);
#endif

/*
 * DL PKG input
*/
PsIfRet PsifDlInput(UINT8 cid, DlPduBlock *pPduHdr);

void PsNetifSyncRohcChannel(struct netif *netif, UINT8 cid);

err_t PsNetifInit(struct netif *netif);

void PsifDumpUlPacket(UlPduInfo *psData);

void PsifDumpDlPacket(struct pbuf *pbufData);

UINT16 PsifGetCurrentPacketDelay(void);

/*
 * PS IF UL PKG memory functions
*/
UINT8 *PsifAllocUlPkgMem(UINT16 len);
void PsifFreeUlPkgMem(UINT8 *pUlPkg);


#ifdef __cplusplus
}
#endif

#endif

