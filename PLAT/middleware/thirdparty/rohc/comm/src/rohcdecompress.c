/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohccompress.c
 Description:    - rohc channel decompress pkt
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/


#include "rohccompress.h"
#include "rohcippacket.h"
#include "rohcpacket.h"
#include "rohccontext.h"
#include "rohcprofile.h"
#include "rohcdecompress.h"

BOOL RohcDecompressRohcPacket(RohcDecompressChannel *channel, RohcInputBuff *input, RohcDecompressOutputHeaderBuff *output, RohcFeedbackBuff *feedback)
{

    RohcDecompressContext *decompressContext = PNULL;
    RohcList *newContextList = PNULL;
    RohcPacketBuff pRohcPkt;
    INT32 profileId = 0;
    UINT16 cidPkt;

    //input parameter check
    GosDebugBegin(channel != PNULL && input != PNULL && output != PNULL && feedback != PNULL, channel, input, output);
    return FALSE;
    GosDebugEnd();

    //check channel capability
    if(channel->capablilty == 0)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcDecompressRohcPacket_1, P_ERROR, 0, "RohcDecompressRohcPacket channel capability is invalid");
        return FALSE;
    }
    
    //pase the input packet
    if(RohcParseRohcPkt(input, &pRohcPkt, channel->maxCid) == FALSE)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcDecompressRohcPacket_2, P_ERROR, 0, "RohcDecompressRohcPacket parse Ip pkt fail");
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
            ECOMM_TRACE(UNILOG_ROHC, RohcDecompressRohcPacket_3, P_ERROR, 1, "RohcDecompressRohcPacket not support type %u", pRohcPkt.type);
            return FALSE;         
        }
    }

    ECOMM_TRACE(UNILOG_ROHC, RohcDecompressRohcPacket_4, P_INFO, 2, "RohcDecompressRohcPacket rohc type %u, cid %u", pRohcPkt.type, cidPkt);
    
    decompressContext = RohcGetDecompressContextByCid(channel, cidPkt);
    if(decompressContext != PNULL)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcDecompressRohcPacket_5, P_INFO, 2, "RohcDecompressRohcPacket find adpt compress context,profile %u cid %u", decompressContext->profileId, decompressContext->cid);

        return RohcDecompressContextDecompress(decompressContext, &pRohcPkt, output, feedback);
    }

    //try find adpt profile
    profileId = RohcGetAdptDecompressProfileByIpPacket(channel->capablilty, &pRohcPkt);
    if(profileId >= 0)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcDecompressRohcPacket_6, P_INFO, 1, "RohcDecompressRohcPacket find adpt profile %u ", profileId);

        //create new compress context
        newContextList = RohcCreateNewDecompressContextList(channel, profileId, channel->targetMode);
        if(newContextList)
        {
            decompressContext = (RohcDecompressContext *)RohcGetListBody(newContextList);

            ECOMM_TRACE(UNILOG_ROHC, RohcDecompressRohcPacket_7, P_INFO, 1, "RohcDecompressRohcPacket create new decompress context list 0x%x success ", newContextList);

            OsaCheck(decompressContext != PNULL, decompressContext, 0, 0);
            
            if(RohcCreateDecompressContext(decompressContext, &pRohcPkt) == TRUE)
            {
                if(channel->pContextList.next == PNULL)
                {
                    channel->pContextList.next = newContextList;
                }
                else
                {
                    RohcInsertList(&channel->pContextList, newContextList);
                }
                channel->activeCidNum ++;
            }
            else
            {
                ECOMM_TRACE(UNILOG_ROHC, RohcDecompressRohcPacket_8, P_ERROR, 0, "RohcDecompressRohcPacket init compress context fail");
                RohcFreeList(newContextList);
                return FALSE;            
            }

        }
        
    }
    else
    {
            ECOMM_TRACE(UNILOG_ROHC, RohcDecompressRohcPacket_9, P_ERROR, 0, "RohcDecompressRohcPacket can not find adpt profile");
            return FALSE;        
    }

    if(decompressContext)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcDecompressRohcPacket_10, P_INFO, 1, "RohcDecompressRohcPacket compress pkt ,cid %u", decompressContext->cid);    
        return RohcDecompressContextDecompress(decompressContext, &pRohcPkt, output, feedback);
    }

    return FALSE;
    
}

