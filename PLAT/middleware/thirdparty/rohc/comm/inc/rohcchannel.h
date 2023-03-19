/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcchannel.h
 Description:    - rohc compress/decompress channel related header file
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/

#ifndef ROHC_CHANNEL_H
#define ROHC_CHANNEL_H

#include "rohcutil.h"
#include "rohccontext.h"

/******************************************************************************
 *****************************************************************************
 * macro
 *****************************************************************************
******************************************************************************/


#define ROHC_COMPRESS_CHANNEL_MAGIC 0xABCDDCBA
#define ROHC_DECOMPRESS_CHANNEL_MAGIC 0xDCBAABCD

/******************************************************************************
 *****************************************************************************
 * structure/ENUM
 *****************************************************************************
******************************************************************************/


typedef struct RohcCompressChannel_Tag
{
    UINT32 magic;
    UINT16 maxCid;
    UINT16 activeCidNum;
    //channel capability
    UINT32 capablilty;
    //context list
    RohcList pContextList;
    
}RohcCompressChannel;

typedef struct RohcDecompressChannel_Tag
{
    UINT32 magic;
    UINT16 maxCid;
    UINT16 activeCidNum;
    UINT8 targetMode;
    UINT8 rsvd1;
    UINT8 rsvd2;
    //channel capability
    UINT32 capablilty;
    //context list
    RohcList pContextList;
    
}RohcDecompressChannel;


/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/
RohcCompressChannel *RohcCreateCompressChannel(UINT32 profiles, UINT16 maxCid);
void RohcDestroyCompressChannel(RohcCompressChannel *compressChannel);
RohcDecompressChannel *RohcCreateDecompressChannel(UINT32 profiles, UINT16 maxCid, UINT8 targetMode);
void RohcDestroyDecompressChannel(RohcDecompressChannel *decompressChannel);
void RohcResetCompressChannel(RohcCompressChannel *compressChannel);
void RohcResetDecompressChannel(RohcDecompressChannel *decompressChannel);

RohcList *RohcCreateNewCompressContextList(RohcCompressChannel *channel, UINT32 profileId);


RohcCompressContext *RohcCompressGetAdptContext(RohcCompressChannel *channel, RohcIpPacket *inpPkt);


RohcCompressContext *RohcGetCompressContextByCid(RohcCompressChannel *channel, UINT16 cid);

RohcDecompressContext *RohcGetDecompressContextByCid(RohcDecompressChannel *channel, UINT16 cid);


RohcList *RohcCreateNewDecompressContextList(RohcDecompressChannel *channel, UINT32 profileId, UINT8 targetMode);




#endif

