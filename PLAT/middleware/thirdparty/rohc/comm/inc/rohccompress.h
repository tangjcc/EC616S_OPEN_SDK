/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohccompress.h
 Description:    - rohc channel compress pkt header file
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/

#ifndef ROHC_COMPRESS_H
#define ROHC_COMPRESS_H

#include "rohcbuff.h"
#include "rohcchannel.h"

/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/

BOOL RohcCompressIpPacket(RohcCompressChannel *channel, RohcInputBuff *input, RohcCompressOutputHeaderBuff *output);

#endif

