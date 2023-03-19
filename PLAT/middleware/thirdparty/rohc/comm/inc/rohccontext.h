/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohccontext.h
 Description:    - rohc context related header file
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/

#ifndef ROHC_CONTEXT_H
#define ROHC_CONTEXT_H

#include "rohcbuff.h"
#include "rohcutil.h"
#include "rohcpacket.h"
#include "rohcippacket.h"

/******************************************************************************
 *****************************************************************************
 * macro
 *****************************************************************************
******************************************************************************/



/******************************************************************************
 *****************************************************************************
 * structure/ENUM
 *****************************************************************************
******************************************************************************/

typedef enum
{
    ROHC_MODE_U     = 0,
    ROHC_MODE_O     = 1,
    ROHC_MODE_R     = 2,
}RohcMode;


typedef enum
{
    ROHC_COMPRESS_STATE_RESERVED = 0,
    ROHC_COMPRESS_STATE_IR       = 1,
    ROHC_COMPRESS_STATE_FO       = 2,
    ROHC_COMPRESS_STATE_SO       = 3,
}RohcCompressState;

typedef enum
{
    ROHC_DECOMPRESS_STATE_NO_CONTEXT           = 0,
    ROHC_DECOMPRESS_STATE_STATIC_CONTEXT       = 1,
    ROHC_DECOMPRESS_STATE_FULL_CONTEXT         = 2,
}RohcDecompressState;

typedef enum
{
    ROHC_COMP_STATE_CHANGE_CAUSE_OPTIMISTIC_APPROACH   = 0,
    ROHC_COMP_STATE_CHANGE_CAUSE_PERIODIC_TIMEOUT      = 1,
    ROHC_COMP_STATE_CHANGE_CAUSE_UPDATE                = 2,
    ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_ACK          = 3,
    ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_NACK         = 4,
    ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_STATIC_NACK   = 5,
}RohcCompressContextStateChangeCause;

typedef enum
{
    ROHC_DECOMP_STATE_CHANGE_CAUSE_DECOMP_SUCCESS   = 0,
    ROHC_DECOMP_STATE_CHANGE_CAUSE_DECOMP_FAIL      = 1,
    ROHC_DECOMP_STATE_CHANGE_CAUSE_NO_DYNAMIC       = 2,
}RohcDecompressContextStateChangeCause;


typedef struct RohcCompressUmodeStatePrivate_Tag
{
    UINT16 downwardTrans2IrPktsNum;
    UINT16 downwardTrans2FoPktsNum;
    UINT32 lastIrPktTime;
    UINT32 lastFoPktTime;
    UINT16 refreshIrPktsNum;
    UINT16 refreshFoPktsNum;
    UINT32 refreshIrTimeout;
    UINT32 refreshFoTimeout;
    UINT16 upward2FoPktsNum;
    UINT16 upward2SoPktsNum;    
}RohcCompressUmodeStatePrivate;

typedef struct RohcCompressOmodeStatePrivate_Tag
{
    UINT16 upward2FoPktsNum;
    UINT16 upward2SoPktsNum;    
}RohcCompressOmodeStatePrivate;



typedef struct RohcCompressContext_Tag
{
    UINT16 cid;
    UINT16 profileId;
    UINT8 mode;
    UINT8 state;
    UINT16 pktsIrState;
    UINT16 pktsFoState;
    UINT16 pktsSoState;
    union
    {
        RohcCompressUmodeStatePrivate umodePrivate;
        RohcCompressOmodeStatePrivate omodePrivate;
    }statePrivate;
    UINT32 lastUsedTime;
    void *pProfileUtility;
    void *pPrivate;
}RohcCompressContext;

typedef struct RohcDecompressContext_Tag
{
    UINT16 cid;
    UINT16 profileId;
    UINT8 targetMode;
    UINT8 state;
    UINT32 lastUsedTime;
    void *pProfileUtility;
    void *pPrivate;
}RohcDecompressContext;


/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/
void RohcCompressUmodeStateTransition(RohcCompressContext *context, UINT32 CurrTime, UINT8 targetState);
void RohcCompressOmodeStateTransition(RohcCompressContext *context, UINT8 feedbackType, BOOL update, UINT8 targetState);
void RohcCompressRmodeStateTransition(RohcCompressContext *context, UINT8 feedbackType, BOOL update, UINT8 targetState);
void RohcDecompressContextStateChange(RohcDecompressContext *context, UINT8 newState, UINT8 cause);
void RohcCompressContextStateChange(RohcCompressContext *context, UINT8 newState, UINT8 cause);


void RohcDestroyCompressContext(RohcCompressContext *compressContext);
BOOL RohcCreateCompressContext(RohcCompressContext *compressContext, RohcIpPacket *pIpPkt);
BOOL RohcCompressContextCheckProfile(RohcCompressContext *compressContext, RohcIpPacket *pIpPkt);
BOOL RohcCompressContextCheckContext(RohcCompressContext *compressContext, RohcIpPacket *pIpPkt);
BOOL RohcCompressContextCompress(RohcCompressContext *compressContext, RohcIpPacket *pIpPkt, RohcCompressOutputHeaderBuff *pOutput);
BOOL RohcCompressContextProcessFeedback(RohcCompressContext *compressContext, RohcPacketBuff *pRohcPkt);
void RohcDestroyDecompressContext(RohcDecompressContext *decompressContext);
BOOL RohcCreateDecompressContext(RohcDecompressContext *decompressContext, RohcPacketBuff *pRohcPkt);
BOOL RohcDecompressContextCheckProfile(RohcDecompressContext *decompressContext, RohcPacketBuff *pRohcPkt);
BOOL RohcDecompressContextCheckContext(RohcDecompressContext *decompressContext, RohcPacketBuff *pRohcPkt);
BOOL RohcDecompressContextDecompress(RohcDecompressContext *decompressContext, RohcPacketBuff *pRohcPkt, RohcDecompressOutputHeaderBuff *pOutput, RohcFeedbackBuff *feedback);


#endif

