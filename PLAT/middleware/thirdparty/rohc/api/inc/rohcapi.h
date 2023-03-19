/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcapi.h
 Description:    - rohc related api header file
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/
#ifndef ROHC_API_H
#define ROHC_API_H

#include "commontypedef.h"


/******************************************************************************
 *****************************************************************************
 * macro
 *****************************************************************************
******************************************************************************/


typedef struct RohcBuff_Tag
{
    UINT16 dataLen;
    UINT16 buffLen;
    UINT8 *pData;
}RohcBuff;



/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/

UINT32 RohcGetCapability(void);
void* RohcCreateCompChannel(UINT32 rohcProfiles, UINT16 maxCid, UINT32 mrru);
void* RohcCreateDecompChannel(UINT32 rohcProfiles, UINT16 maxCid, UINT32 mrru, UINT8 decompressMode);
void RohcDestroyCompChannel(void *compChannel);
void RohcDestroyDecompChannel(void *decompChannel);
void RohcResetCompChannel(void *compChannel);
void RohcResetDecompChannel(void *decompChannel);

BOOL RohcCompressPkt(void *compCahnnel, RohcBuff *ipInput, RohcBuff *compHdrOutput, RohcBuff *payload);

BOOL RohcDecompressPkt(void *decompCahnnel, RohcBuff *rohcInput, RohcBuff *decompHdrOutput, RohcBuff *payload, RohcBuff *feedback);

BOOL RohcRcvFeedback(void *compCahnnel, RohcBuff *feedback);

#endif

