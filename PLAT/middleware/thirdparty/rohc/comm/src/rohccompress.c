/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohccompress.c
 Description:    - rohc channel compress pkt
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/


#include "rohccompress.h"
#include "rohcippacket.h"
#include "rohcpacket.h"
#include "rohccontext.h"
#include "rohcprofile.h"


/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/

BOOL RohcCompressIpPacket(RohcCompressChannel *channel, RohcInputBuff *input, RohcCompressOutputHeaderBuff *output)
{

    RohcCompressContext *compressContext = PNULL;
    RohcList *newContextList = PNULL;
    RohcIpPacket ipPkt;
    INT32 profileId = 0;

    //input parameter check
    GosDebugBegin(channel != PNULL && input != PNULL && output != PNULL, channel, input, output);
    return FALSE;
    GosDebugEnd();

    //check channel capability
    if(channel->capablilty == 0)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCompressIpPacket_1, P_ERROR, 0, "RohcCompressIpPacket channel capability is invalid");
        return FALSE;
    }
    
    //pase the input packet
    if(RohcParseIpPacket(input, &ipPkt) == FALSE)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCompressIpPacket_2, P_ERROR, 0, "RohcCompressIpPacket parse Ip pkt fail");
        return FALSE;        
    }

    //try find adpt context
    compressContext = RohcCompressGetAdptContext(channel, &ipPkt);
    if(compressContext != PNULL)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCompressIpPacket_3, P_INFO, 2, "RohcCompressIpPacket find adpt compress context,profile %u cid %u", compressContext->profileId, compressContext->cid);

        return RohcCompressContextCompress(compressContext, &ipPkt, output);
    }

    //try find adpt profile
    profileId = RohcGetAdptCompressProfileByIpPacket(channel->capablilty, &ipPkt);
    if(profileId >= 0)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCompressIpPacket_4, P_INFO, 1, "RohcCompressIpPacket find adpt profile %u ", profileId);

        //create new compress context
        newContextList = RohcCreateNewCompressContextList(channel, (UINT32)profileId);
        if(newContextList)
        {
            compressContext = (RohcCompressContext *)RohcGetListBody(newContextList);

            ECOMM_TRACE(UNILOG_ROHC, RohcCompressIpPacket_5, P_INFO, 1, "RohcCompressIpPacket create new compress context list 0x%x success", newContextList);

            OsaCheck(compressContext != PNULL, compressContext, 0, 0);
            
            if(RohcCreateCompressContext(compressContext, &ipPkt) == TRUE)
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
                ECOMM_TRACE(UNILOG_ROHC, RohcCompressIpPacket_6, P_ERROR, 0, "RohcCompressIpPacket init compress context fail");
                RohcFreeList(newContextList);                
                return FALSE;            
            }

        }
        
    }
    else
    {
            ECOMM_TRACE(UNILOG_ROHC, RohcCompressIpPacket_7, P_ERROR, 0, "RohcCompressIpPacket can not find adpt profile");
            return FALSE;        
    }

    if(compressContext)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCompressIpPacket_8, P_INFO, 1, "RohcCompressIpPacket compress pkt ,cid %u", compressContext->cid);    
        return RohcCompressContextCompress(compressContext, &ipPkt, output);
    }    

    return FALSE;
}

