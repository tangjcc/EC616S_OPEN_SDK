/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcpacket.h
 Description:    - rohc packet related header
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/

#ifndef ROHC_PACKET_H
#define ROHC_PACKET_H

#include "rohcbuff.h"

/******************************************************************************
 *****************************************************************************
 * macro
 *****************************************************************************
******************************************************************************/
//ROHC Sdv
#define ROHC_SDV_VALUE_BITS_1 7
#define ROHC_SDV_VALUE_BITS_2 14
#define ROHC_SDV_VALUE_BITS_3 21
#define ROHC_SDV_VALUE_BITS_4 28

//get rohc type code
#define ROHC_CHECK_PADDING_TYPE(x)  ((x) == (0xE0))
#define ROHC_CHECK_SEGEMNT_TYPE(x)  ((x)&(0xFE) == (0xFE))
#define ROHC_CHECK_ADD_CID_TYPE(x)  ((((x)&(0xF0)) == (0xE0)) && (((x)&(0x0F)) != (0x00)))
#define ROHC_CHECK_FEEDBACK_TYPE(x) (((x)&(0xF8)) == (0xF0))
#define ROHC_CHECK_IR_DYN_TYPE(x)   ((x) == (0xF8))
#define ROHC_CHECK_IR_TYPE(x)       (((x)&(0xFE)) == (0xFC))

//feebback related
#define ROHC_GET_SIZE_FROM_TYE(x)   ((x)&(0x07))

//add-cid related
#define ROHC_GET_CID_FROM_ADD_CID(x) ((x)&(0x0F))

#define ROHC_LARGE_CID_MAX_BYTES 2

//cid related
#define ROHC_INVAID_CID 0xFFFF
#define ROHC_DEFAULT_CID 0

#define ROHC_CHECK_SMALL_CID(x) ((x) <= 15)
#define ROHC_CHECK_LARGE_CID(x) (((x) > 15) && ((x) <= ((1 << 14) - 1)))

/******************************************************************************
 *****************************************************************************
 * STRUCT/ENUM
 *****************************************************************************
******************************************************************************/

typedef enum
{
    ROHC_PACKET_TYPE_INVALID  = 0,
    ROHC_PACKET_TYPE_IR       = 1,
    ROHC_PACKET_TYPE_IR_DYN   = 2,
    ROHC_PACKET_TYPE_FEEDBACK = 3,
    ROHC_PACKET_TYPE_SEGMENT  = 4,
    ROHC_PACKET_TYPE_PADDING  = 5,
    ROHC_PACKET_TYPE_ADDCID   = 6,
    ROHC_PACKET_TYPE_NORMAL   = 7,
}RohcPacketType;

//RFC5795 5.2.4.1 
typedef enum
{
    ROHC_FEEDBACK_TYPE_ACK            = 0,
    ROHC_FEEDBACK_TYPE_NACK           = 1,
    ROHC_FEEDBACK_TYPE_STATIC_NACK    = 2,
    ROHC_FEEDBACK_TYPE_RESERVED       = 3,
    ROHC_FEEDBACK_TYPE_NOT_FEEDBACK   = 4,
}RohcFeedbackType;
    

typedef struct RohcFeedbackPacket_Tag
{
    UINT16 cid;
    UINT16 lenFeedbackData;
    UINT8 *pFeedbackData;
}RohcFeedbackPacket;

typedef struct RohcFeebackData_Tag
{
    UINT16 dataLen;
    UINT8 *pData;
}RohcFeebackData;

typedef struct RohcIRPacket_Tag
{
    UINT16 cid;
    UINT8 profileId;
    UINT8 CRC;   // 8bits CRC
    UINT8 xInfo; //0 or 1
    UINT8 rsvd1;
    UINT16 lenRawWithoutPadding;
    UINT8 *pRawWithoutPadding;
    UINT8 *infoProfilespeccific;
}RohcIRPacket;

typedef struct RohcIRDYNPacket_Tag
{
    UINT16 cid;
    UINT8 profileId;
    UINT8 CRC;  //8 bits CRC
    UINT8 *pRawWithoutPadding;
    UINT8 *infoProfilespeccific;
}RohcIRDYNPacket;

typedef struct RohcNormalPacket_Tag
{
    UINT16 cid;
    UINT8 firstOctet;
    UINT8 rsvd;
    UINT8 *restOctect;
}RohcNormalPacket;


typedef struct RohcPacketBuff_Tag
{
    UINT16 type;
    UINT16 lenRawPkt;
    union
    {
        RohcFeedbackPacket infoFeedback;
        RohcIRPacket       infoIR;
        RohcIRDYNPacket    infoIRDYN;
        RohcNormalPacket   infoNormal;
    }private;
    UINT8 *pRawPkt;
    UINT8 *uncompPayload;
}RohcPacketBuff;

/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/



UINT8 RohcDecodeSdvValue(UINT8 *data, UINT16 len, UINT32 *value, UINT8 *bits);


UINT8 RohcEncodeSdvValue(UINT32 value, UINT8 maxBytes, UINT8 *buff);


BOOL RohcParseFeedbackPkt(RohcInputBuff *input, RohcPacketBuff *output);


BOOL RohcParseRohcPkt(RohcInputBuff *input, RohcPacketBuff *output, UINT16 maxCid);


BOOL RohcParseGeneralHeaderPkt(RohcInputBuff *input, RohcPacketBuff *output, UINT16 maxCid);

BOOL RohcCreateIrHeaderWithoutCrc(UINT8 profileId, UINT16 cid, RohcOutputBuff *output, UINT8 **pCrc);

BOOL RohcCreateNormalHeader(UINT8 ipFirst, UINT16 cid, RohcOutputBuff *output);

BOOL RohcCreateFeedback(UINT16 cid, RohcFeebackData *feedbackData, RohcOutputBuff *output);

#endif
