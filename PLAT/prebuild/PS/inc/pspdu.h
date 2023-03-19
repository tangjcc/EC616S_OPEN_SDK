#ifndef __PS_PDU_H__
#define __PS_PDU_H__

/******************************************************************************
 ******************************************************************************
 Copyright:      - 2017, All rights reserved by EigenComm Ltd.
 File name:      - pspdu.h
 Description:    - PS DATA PDU info
 History:        - 10/19/2018, Originated by jcweng
 ******************************************************************************
******************************************************************************/
#ifdef WIN32
#include "winsys.h"
#else
#include "commontypedef.h"
#endif

/*
 * ip/non-ip list
*/
typedef enum DataPduMemType_Tag
{
    PDU_HEAP_MEM = 0,       // dynamic memory, GosAllocMemory/GosAllocZeroMemory/GosFreeMemory
    PDU_LWIP_UL_PKG_MEM,    // LwipFreeUlIpPkgMem() free it
    PDU_STACK_MEM           // stack memory, don't need to free
}DataPduMemType;


/*
 * ticks type, which carried in "UlPduInfo"
*/
typedef enum UlPduTickType_Tag
{
    UL_PDU_NO_TICK_INFO = 0,// no tick info
    UL_PDU_DISCARD_TICK,    // this UL PDU could be discarded if exceed this tick: "current tick" > "UL_PDU_DISCARD_TICK"
    UL_PDU_START_TICK       // systick when LWIP create/send this UL PDU
}UlPduTickType;


/*
 * UL data PDU info, this just the UL user data need to send to NW
 * 
 * Note:
 * 1> structure memory should be used "OsaAllocMemory" which allocated in heap, and could be freed in other task
 * 2> UL "memType" for LWIP should be: PDU_LWIP_UL_PKG_MEM (1)
 * 3> here, we extended the structure size to 16 bytes, for latter optimiztion;
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * A: if UL data is "IP PKG" from LWIP, memtype = "PDU_LWIP_UL_PKG_MEM"
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Maybe need ROHC: 
 *  Before ROHC:
 *  +------------------+-----------------------------------+
 *  | ip/udp/other hdr |       payload                     |
 *  +------------------+-----------------------------------+
 *  ^
 *  |ptr
 *  |<------------------- length ------------------------->|
 *
 *  After ROHC:
 *  1> if (ip/udp/other hdr length) >= (comp len): (most case)
 *  +---------+--------+-----------------------------------+
 *  |         |comp hdr|        payload                    |
 *  +---------+--------+-----------------------------------+
 *  ^
 *  |ptr
 *  |<------->|validOffset
 *  |         |<-------------- length -------------------->|
 *
 *  2> if (ip/udp/other hdr length) < (comp len): 
 *      i> need to create a new "data_pdu_info" to store the compressed header
 *      ii> and this "data_pdu_info" only contain the payload info;
 *  +--------------------+
 *  |      comp hdr      |
 *  +--------------------+
 *  ^
 *  |ptr
 *  |<---- length ------>|
 *  bContinue = TRUE, and pNext ->
 *  +------------------+-----------------------------------+
 *  | ip/udp/other hdr |        payload                    |
 *  +------------------+-----------------------------------+
 *  ^
 *  |ptr
 *  |<---------------->|validOffset
 *  |                  |<----------- length -------------->|
 * 
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * B: if UL data is not from LWIP, maybe from ATCMD, memtype = "PDU_HEAP_MEM"
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  +------------------------------------------------------+
 *  |                   UL USER DATA                       |
 *  +------------------------------------------------------+
 *  ^
 *  |ptr
 *  |<------------------- length ------------------------->|
 * 
 */
typedef struct UlPduInfo_Tag
{
    /*
     * indicate the "ptr" memory type, which would affect how to free the "*ptr" memory
    */
    UINT8       memType;    //DataPduMemType
    UINT8       validOffset;
    
    /*
     * Note: DATA info
    */
    UINT16      epsId : 4;      /* epsId, should set in ESM task */
    UINT16      esmRai : 2;     /* DataRelAssistIndEnum, release assistance indication, set in APP */
    UINT16      bExceptData: 1; /* need to pass to CEMM/CERRC, if "Except Data", CERRC need to establish connection even if it's barred, set in APP */

    /*
     * Whether following seg belong to the same IP PKG:
     *  TRUE - next seg is the same IP PKG PDU
     *  FALSE - Not the same IP PKG PDU
     * Fill by LWIP
    */
    UINT16      bContinue: 1;

    UINT16      bRohcFeedback : 1;  /* Whether this pkg is ROHC feedback pkg only */
    UINT16      bRohcComped: 1;     /* Whether ROHC compressed in this pkg */
    UINT16      bTestLoopback : 1;  /* Whether this UL data is test loop back data*/
    UINT16      tickType : 2;       /* UlPduTickType, indicate the "sysTick" type */
    UINT16      reserved0 : 3;

    UINT16      length;
    UINT16      reserved1;

    /*
     * Note:
     * 1> if "systick" is "UL_PDU_DISCARD_TICK", then PS (DR/ESM/EMM) MAYBE (not must) discard this pkg, if "current sys tick" > "sysTick";
     *    LWIP could set this if has a clear valid period for this UL PKG, such as: TCP pkg, or DNS/IPv6 RS pkg (as has retry timer);
     * 2> if "systick" is "UL_PDU_START_TICK", then PS (DR/ESM/EMM) MAYBE (not must) discard this pkg in case:
     *     a> the time passed more than 20s, and b> has anyother UL pkg followed;
     * 3> if "systick" is "UL_PDU_NO_TICK_INFO", then PS (DR/ESM/EMM) could try best to send it, don't discard it easily;
     *
     * Another note:
     * 1> DR/ESM will pending not more then (3*1500) bytes pkgs, or not more then 10 pkgs;
     *    if exceeded, will discard the oldest pkg;
    */
    UINT32      sysTick;            /*system tick, type is indicated by: tickType */

    UINT8       *ptr;
    struct UlPduInfo_Tag *pNext;
}UlPduInfo; // 20 bytes


/*
 * Reserved for ROHC decompression;
 * 80 = ipv6 + udp + rtp + 20 others
*/
#define ROHC_DL_HDR_RESV_LEN    80

/*
 * One byte magic byte
*/
#define PS_PDU_MAGIC_BYTE       0xA7

/*
 * set and check one magic byte
*/
#define PS_DL_PDU_BLOCK_SET_MAGIC_BYTE(pDlPduBlock)     ((pDlPduBlock)->magicByte = PS_PDU_MAGIC_BYTE)
#define PS_DL_PDU_BLOCK_CHECK_MAGIC_BYTE(pDlPduBlock)   ((pDlPduBlock)->magicByte == PS_PDU_MAGIC_BYTE)


/*
 * DL PDU BLOCK info, just the DL user data
 * 
 * !!! "DlPduBlock" and "DL user data" just use the continuous memory !!!
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * A. if DL user PDU is IP PKG data, and need to pass to LWIP
 *    This memory should be allocated in PSIF
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * DL IP PDU buffer:
 * If no ROHC:
 * +-----------------+--------------------------+-----------------------------------+
 * | pbuf (16 bytes) |   DlPduBlock(12 bytes)   |      ip pkg (len <= MTU)          |
 * +-----------------+--------------------------+-----------------------------------+
 *    *payload----------------------------------^
 *                           pPdu---------------^
 *                     length       =           |<--------------------------------->|
 *
 * If ROHC Feedback:
 * +-----------------+--------------------------+---------------------------+
 * | pbuf (16 bytes) |   DlPduBlock (12 bytes)  |     ROHC Feedback         |
 * +-----------------+--------------------------+---------------------------+
 *    *payload----------------------------------^
 *                           pPdu---------------^
 *                     length       =           |<------------------------->|
 *
 * If ROHC, before decomp:
 * +-----------------+--------------------------+---------------------------+---------+-----------------+
 * | pbuf (16 bytes) |   DlPduBlock (12 bytes)  | ROHC_DL_HDR_RESV_LEN (80) |comp hdr |   payload       |
 * +-----------------+--------------------------+---------------------------+---------+-----------------+
 *    *payload----------------------------------^
 *                           pPdu-------------------------------------------^
 *                     length       =                                       |<------------------------->|
 *
 * Note: ROHC_DL_HDR_RESV_LEN = 80, reserved for decompressed IP header, 80 = ipv6 + udp + rtp + 20 others
 *
 * After decomp:
 ** 1> If reserved 80 bytes is enough (most case):
 * +-----------------+--------------------------+-------+-----------------------------+-----------------+
 * | pbuf (16 bytes) |   DlPduBlock (12 bytes)  |       |decomped ip/udp/other hdr    |   payload       |
 * +-----------------+--------------------------+-------+-----------------------------+-----------------+
 *    *payload------------------------------------------^
 *                           pPdu-----------------------^
 *                     length       =                   |<--------------------------------------------->|
 *                                              |useless|
 *
 ** 2> If reserved 80 bytes is not enough (rarely)
 * ROHC decompression side, need to allocate a new LWIP block, and soft copy to this new block;
 *   i> copy to ->
 * +-----------------+--------------------------+------------------------------+-----------------+
 * | pbuf (16 bytes) |   DlPduBlock (12 bytes)  | decomped ip/udp/other hdr    |   payload       |
 * +-----------------+--------------------------+------------------------------+-----------------+
 *    *payload----------------------------------^
 *                           pPdu---------------^
 *                     length       =           |<--------------------------------------------->|
 *
 *  ii> Free the old LWIP block
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * A. if DL user PDU is not need to pass to LWIP, (if need to LOOPBACK, or non-ip data)
 *    This memory should be allcated in HEAP
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * +--------------------------+-----------------------------------+
 * |   DlPduBlock(12 bytes)   |      ip pkg (len <= MTU)          |
 * +--------------------------+-----------------------------------+
 *         pPdu---------------^
 *   length       =           |<--------------------------------->|
 *
*/
typedef struct DlPduBlock_Tag
{
    UINT16      length;
    
    /*
     * Whether only need to do ROHC DECOMP, but don't need to pass to LWIP;
     * Note:
     * 1> During PDCP DL re-establishment procedure, here, as PDCP COUNT maybe re-order, DL PDCP SDU need to 
     *    deciphered & decompressed, and stored in PDCP side;
     * 2> Here, set it (in PDCP task), notify LWIP(ROHC) task only do decompression, but don't pass to LWIP task;
     * 3> Set & Clear it in PDCP task;
    */
    UINT16      bOnlyDecomp : 1;
    /*
     * If configed ROHC, whether need to ignore decompression for this pkg;
     * Note:
     * 1> PDCP set it, if pass re-order PDCP SDU, which decomped before
    */
    UINT16      bIgnoreDecomp : 1;
    UINT16      bRohcFeedback : 1;
    UINT16      bNotLwipMem : 1;   //whether this block memory is not allocated in LWIP, in fact suggest to involve a memType;
    UINT16      reserved : 4;
    UINT16      magicByte : 8;

    UINT8       *pPdu;
    struct DlPduBlock_Tag *pNext;
}DlPduBlock;   // 12 bytes


#endif

