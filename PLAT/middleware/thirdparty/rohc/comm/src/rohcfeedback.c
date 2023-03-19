/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcfeedback.c
 Description:    - rohc channel feedback pkt process api
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/


#include "rohccompress.h"
#include "rohcippacket.h"
#include "rohcpacket.h"
#include "rohccontext.h"


BOOL RohcProcessFeedback(RohcCompressChannel *channel, RohcInputBuff *input)
{
    RohcCompressContext *compressContext = PNULL;
    RohcPacketBuff pRohcPkt;
    UINT16 cidPkt;

    //input parameter check
    GosDebugBegin(channel != PNULL && input!= PNULL, channel, input, 0);
    return FALSE;
    GosDebugEnd();

    //parse input feedback packet
    if(RohcParseRohcPkt(input, &pRohcPkt, channel->maxCid) == FALSE)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcProcessFeedback_1, P_ERROR, 0, "RohcProcessFeedback parse input rohc pkt fail");
        return FALSE;
    }

    //check support rohc pkt and cid
    switch(pRohcPkt.type)
    {
        case ROHC_PACKET_TYPE_FEEDBACK:
        {
            cidPkt = pRohcPkt.private.infoFeedback.cid;
            break;
        }
        case ROHC_PACKET_TYPE_IR:
        {
            cidPkt = pRohcPkt.private.infoIR.cid;
            break;
        }
        case ROHC_PACKET_TYPE_IR_DYN:
        {
            cidPkt = pRohcPkt.private.infoIRDYN.cid;
            break;
        }
        case ROHC_PACKET_TYPE_NORMAL:
        {
            cidPkt = pRohcPkt.private.infoNormal.cid;
            break;
        }
        default:
        {
            ECOMM_TRACE(UNILOG_ROHC, RohcProcessFeedback_2, P_ERROR, 1, "RohcProcessFeedback not support type %u", pRohcPkt.type);
            return FALSE;         
        }
    }

    ECOMM_TRACE(UNILOG_ROHC, RohcProcessFeedback_3, P_INFO, 2, "RohcProcessFeedback rohc pkt type %u, cid %u", pRohcPkt.type, cidPkt);

    compressContext = RohcGetCompressContextByCid(channel, cidPkt);
    if(compressContext == PNULL)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcProcessFeedback_4, P_ERROR, 0, "RohcProcessFeedback find adpt compress context fail");
        return FALSE;    
    }

    return RohcCompressContextProcessFeedback(compressContext, &pRohcPkt);

}

