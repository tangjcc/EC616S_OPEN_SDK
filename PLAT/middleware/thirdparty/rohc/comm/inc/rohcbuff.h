/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcbuff.c
 Description:    - rohc buff related header file
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/
#ifndef ROHC_BUFF_H
#define ROHC_BUFF_H


#include "commontypedef.h"


typedef struct RohcIpPacket_Tag
{
    UINT16 length;
    UINT8 ipProtocol;//v4 or v6
    UINT8 nextProtocol;
    UINT8 *pNextProtocol;
    UINT8 *pData;
}RohcIpPacket;

typedef struct RohcInputBuff_Tag
{
    UINT16 dataLen;
    UINT16 buffLen;
    UINT8 *pData;
}RohcInputBuff;

typedef struct RohcOutputBuff_Tag
{
    UINT16 dataLen;
    UINT16 buffLen;
    UINT8 *pData;
}RohcOutputBuff;


typedef struct RohcFeedbackBuff_Tag
{
    UINT16 buffMaxLen;
    UINT16 feedbackLen;
    UINT8 *pFeedbackBuff;
}RohcFeedbackBuff;

typedef struct RohcCompressOutputHeaderBuff_Tag
{
    UINT16 buffMaxLen;
    UINT16 compressHdrLen;
    UINT16 payloadLen;
    UINT16 rsvd;
    UINT8 *pPayload;
    UINT8 *pCompressHdrBuff;
}RohcCompressOutputHeaderBuff;

typedef struct RohcDecompressOutputHeaderBuff_Tag
{
    UINT16 buffMaxLen;
    UINT16 decompressHdrLen;
    UINT16 payloadLen;
    UINT16 rsvd;
    UINT8 *pPayload;
    UINT8 *pDecompressHdrBuff;
}RohcDecompressOutputHeaderBuff;

#endif

