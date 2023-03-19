/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohccompress.h
 Description:    - rohc channel decompress pkt header file
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/

#ifndef ROHC_DECOMPRESS_H
#define ROHC_DECOMPRESS_H

#include "rohcbuff.h"
#include "rohcchannel.h"

/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/

BOOL RohcDecompressRohcPacket(RohcDecompressChannel *channel, RohcInputBuff *input, RohcDecompressOutputHeaderBuff *output, RohcFeedbackBuff *feedback);

#endif

