/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcv1uncompress.c
 Description:    - rohcv1 uncompress profile utility API
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/

/******************************************************************************
 *****************************************************************************
 * external
 *****************************************************************************
******************************************************************************/
#include "rohcconfig.h"
#include "rohccontext.h"
#include "rohcpacket.h"
#include "rohcippacket.h"
#include "rohcprofile.h"
#include "rohcbuff.h"

/******************************************************************************
 *****************************************************************************
 * structure/ENUM
 *****************************************************************************
******************************************************************************/


/******************************************************************************
 *****************************************************************************
 * function
 *****************************************************************************
******************************************************************************/




BOOL Rohcv1UncompressCompressCreateContext(RohcCompressContext *context, RohcIpPacket *pIpPkt)
{

    //input parameter check
    GosDebugBegin(context != PNULL && pIpPkt != PNULL, context, pIpPkt, 0); 
    return FALSE; 
    GosDebugEnd();

    if(pIpPkt->ipProtocol == ROHC_IPV4_PROTOCOL || pIpPkt->ipProtocol == ROHC_IPV6_PROTOCOL)
    {
        //init context
        context->statePrivate.umodePrivate.refreshIrPktsNum = ROHC_STATE_DOWNWARD_IR_PKT_NUM;
        context->statePrivate.umodePrivate.refreshIrTimeout = ROHC_STATE_DOWNWARD_IR_TIMEOUT;
        context->statePrivate.umodePrivate.upward2FoPktsNum = ROHC_STATE_UPWARD_FO_PKT_NUM;
        context->statePrivate.umodePrivate.upward2SoPktsNum = ROHC_STATE_UPWARD_SO_PKT_NUM;
        return TRUE;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressCompressCreateContext_1, P_ERROR, 0, "Rohcv1UncompressCompressCreateContext invalid ip packet");
        return FALSE;
    }

}

void Rohcv1UncompressCompressDestroyContext(RohcCompressContext *context)
{
    //input parameter check
    GosDebugBegin(context != PNULL, context, 0, 0); 
    return; 
    GosDebugEnd();

}

BOOL Rohcv1UncompressCompressCheckProfile(RohcIpPacket *pIpPkt)
{

    //input parameter check
    GosDebugBegin(pIpPkt != PNULL, pIpPkt, 0, 0); 
    return FALSE; 
    GosDebugEnd();

    if(pIpPkt->ipProtocol == ROHC_IPV4_PROTOCOL || pIpPkt->ipProtocol == ROHC_IPV6_PROTOCOL)
    {
        return TRUE;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressCompressCheckProfile_1, P_ERROR, 0, "Rohcv1UncompressCompressCheckProfile invalid ip packet");
        return FALSE;
    }

}

BOOL Rohcv1UncompressCompressCheckContext(RohcCompressContext *context, RohcIpPacket *pIpPkt)
{

    //input parameter check
    GosDebugBegin(context != PNULL && pIpPkt != PNULL, context, pIpPkt, 0); 
    return FALSE; 
    GosDebugEnd();

    if(pIpPkt->ipProtocol == ROHC_IPV4_PROTOCOL || pIpPkt->ipProtocol == ROHC_IPV6_PROTOCOL)
    {
        return TRUE;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressCompressCheckContext_1, P_ERROR, 1, "Rohcv1UncompressCompressCheckContext invalid ip packet %u", pIpPkt->ipProtocol);
        return FALSE;
    }

}


BOOL Rohcv1UncompressCompressPktCompress(RohcCompressContext *context, RohcIpPacket *pIpPkt, RohcCompressOutputHeaderBuff *pOutput)
{
    UINT32 currTime;
    RohcPacketType targetPktType = ROHC_PACKET_TYPE_INVALID;
    
     //input parameter check
    GosDebugBegin(context != PNULL && pIpPkt != PNULL && pOutput != PNULL, context, pIpPkt, pOutput); 
    return FALSE; 
    GosDebugEnd();

    //check input pkt
    if(pIpPkt->ipProtocol != ROHC_IPV4_PROTOCOL && pIpPkt->ipProtocol != ROHC_IPV6_PROTOCOL)
    {
        ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressCompressPktCompress_1, P_ERROR, 1, "Rohcv1UncompressCompressPktCompress invalid ip packet %u", pIpPkt->ipProtocol);
        return FALSE;
    }

    currTime = RohcGetCurrentSysTime();

    //check state transfer
    if(context->mode == ROHC_MODE_U)
    {
        if(context->state == ROHC_COMPRESS_STATE_IR)
        {
        //check whether state tansfer
            if(context->pktsIrState != 0)
            {
                RohcCompressUmodeStateTransition(context, currTime, ROHC_COMPRESS_STATE_FO);
            }
        }
        else if(context->state == ROHC_COMPRESS_STATE_FO)
        {
        //check whether state tansfer
            RohcCompressUmodeStateTransition(context, currTime, ROHC_COMPRESS_STATE_FO);        
        }
        else
        {
            ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressCompressPktCompress_2, P_ERROR, 1, "Rohcv1UncompressCompressPktCompress invalid state %u", context->state);
            return FALSE;            
        }

        if(context->state == ROHC_COMPRESS_STATE_IR)
        {
            targetPktType = ROHC_PACKET_TYPE_IR;
        }
        else if(context->state == ROHC_COMPRESS_STATE_FO)
        {
            targetPktType = ROHC_PACKET_TYPE_NORMAL;
        }
    }
    else if(context->mode == ROHC_MODE_O)
    {
        targetPktType = ROHC_PACKET_TYPE_NORMAL;
    }

    //gern rohc pkt
    if(targetPktType == ROHC_COMPRESS_STATE_IR)
    {
        RohcOutputBuff hdrIR;
        UINT8 *pCrc;

        hdrIR.buffLen = pOutput->buffMaxLen;
        hdrIR.pData = pOutput->pCompressHdrBuff;
        if(RohcCreateIrHeaderWithoutCrc(ROHC_PROFILE_UNCOMPRESSED, context->cid, &hdrIR, &pCrc) == TRUE)
        {
            GosDebugBegin(pCrc != PNULL, 0, 0, 0); 
            return FALSE; 
            GosDebugEnd();
  
            //add CRC-8
            *pCrc = RohcGernCrc8(hdrIR.pData, hdrIR.dataLen -1);
            pOutput->compressHdrLen = hdrIR.dataLen;
            pOutput->pPayload = pIpPkt->pData;
            pOutput->payloadLen = pIpPkt->length;
        }
        else
        {
            ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressCompressPktCompress_3, P_ERROR, 0, "Rohcv1UncompressCompressPktCompress create rohc IR pkt fail");
            return FALSE;             
        }

        context->pktsIrState ++;
        if(context->mode == ROHC_MODE_U)
        {
            context->statePrivate.umodePrivate.lastIrPktTime = RohcGetCurrentSysTime();
        }

    }
    else if(targetPktType == ROHC_PACKET_TYPE_NORMAL)
    {
        RohcOutputBuff hdrNormal;
        UINT8 ipFst = *((UINT8 *)pIpPkt->pData);

        hdrNormal.buffLen = pOutput->buffMaxLen;
        hdrNormal.pData = pOutput->pCompressHdrBuff;
        if(RohcCreateNormalHeader(ipFst, context->cid, &hdrNormal) == TRUE)
        {
            pOutput->compressHdrLen = hdrNormal.dataLen;
            pOutput->pPayload = pIpPkt->pData + 1;
            pOutput->payloadLen = pIpPkt->length -1;
        }
        else
        {
            ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressCompressPktCompress_4, P_ERROR, 0, "Rohcv1UncompressCompressPktCompress create rohc IR pkt fail");
            return FALSE;             
        }

        context->pktsFoState ++;
        if(context->mode == ROHC_MODE_U)
        {
            context->statePrivate.umodePrivate.downwardTrans2IrPktsNum ++;            
        }        
    }

    context->lastUsedTime = RohcGetCurrentSysTime();

    ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressCompressPktCompress_5, P_INFO, 1, "Rohcv1UncompressCompressPktCompress compress success, tye %u", targetPktType);
    
    return TRUE;
    
    
}

BOOL Rohcv1UncompressCompressFeedbackProcess(RohcCompressContext *context, RohcPacketBuff *pRohcPkt)
{
    RohcFeedbackPacket *feedback;
    UINT8 *feedback1Info;

    //input parameter check
    GosDebugBegin(context != PNULL && pRohcPkt != PNULL, context, pRohcPkt, 0); 
    return FALSE; 
    GosDebugEnd();

    //check 
    if(pRohcPkt->type != ROHC_PACKET_TYPE_FEEDBACK)
    {
        ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressCompressFeedbackProcess_1, P_ERROR, 1, "Rohcv1UncompressCompressFeedbackProcess is not feedback pkt ,type %u", pRohcPkt->type);
        return FALSE;         
    }

    feedback = &(pRohcPkt->private.infoFeedback);

    GosDebugBegin(feedback != PNULL, feedback, 0, 0);
    return FALSE; 
    GosDebugEnd();

    //check cid
    if(feedback->cid != context->cid)
    {
        ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressCompressFeedbackProcess_2, P_ERROR, 1, "Rohcv1UncompressCompressFeedbackProcess cid in valid ,cid %u",feedback->cid);
        return FALSE;         
    }

    //check feedback type
    if(feedback->lenFeedbackData != 1)
    {
        ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressCompressFeedbackProcess_3, P_ERROR, 1, "Rohcv1UncompressCompressFeedbackProcess is not feedback-1 pkt ,len %u",feedback->lenFeedbackData);
        return FALSE;        
    }

    //check feedback-1 profile-specific information
    feedback1Info = feedback->pFeedbackData;
    
    GosDebugBegin(feedback1Info != PNULL, feedback1Info, 0, 0);
    return FALSE; 
    GosDebugEnd();

    if(*feedback1Info != ROHC_PROFILE_UNCOMPRESSED)
    {
        ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressCompressFeedbackProcess_4, P_ERROR, 1, "Rohcv1UncompressCompressFeedbackProcess inavlid feedback-1 profile-specific info %u",*feedback1Info);
        return FALSE;        
    }

    //transfer mode
    if(context->mode == ROHC_MODE_U)
    {
        context->mode = ROHC_MODE_O;
        RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_FO, ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_ACK);
    }

    context->lastUsedTime = RohcGetCurrentSysTime();

    return TRUE;

}

BOOL Rohcv1UncompressDecompressCreateContext(RohcDecompressContext *context, RohcPacketBuff *pRohcPkt)
{

    //input parameter check
    GosDebugBegin(context != PNULL, context, 0, 0);
    return FALSE; 
    GosDebugEnd();

    if(pRohcPkt->type == ROHC_PACKET_TYPE_IR)
    {
        if(context->targetMode != ROHC_MODE_O)
        {
            context->targetMode = ROHC_MODE_O;
        }
        return TRUE;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressCreateContext_1, P_ERROR, 1, "Rohcv1UncompressDecompressCreateContext invalid type %u", pRohcPkt->type);
        return FALSE;    
    }

}

void Rohcv1UncompressDecompressDestroyContext(RohcDecompressContext *context)
{
    //input parameter check
    GosDebugBegin(context != PNULL, context, 0, 0);
    return; 
    GosDebugEnd();

}

BOOL Rohcv1UncompressDecompressCheckProfile(RohcPacketBuff *pRohcPkt)
{

    //input parameter check
    GosDebugBegin(pRohcPkt != PNULL, pRohcPkt, 0, 0);
    return FALSE; 
    GosDebugEnd();

    if(pRohcPkt->type == ROHC_PACKET_TYPE_IR)
    {
        if(pRohcPkt->private.infoIR.profileId == ROHC_PROFILE_UNCOMPRESSED)
        {
            return TRUE;
        }
        else
        {
            ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressCheckProfile_1, P_ERROR, 1, "Rohcv1UncompressDecompressCheckProfile invalid profile id %u in IR", pRohcPkt->private.infoIR.profileId);
            return FALSE;            
        }
    }
    else if(pRohcPkt->type == ROHC_PACKET_TYPE_NORMAL)
    {
        return TRUE;
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressCheckProfile_2, P_ERROR, 1, "Rohcv1UncompressDecompressCheckProfile invalid type %u", pRohcPkt->type);
        return FALSE;     
    }

    
}


BOOL Rohcv1UncompressDecompressCheckContext(RohcDecompressContext *context, RohcPacketBuff *pRohcPkt)
{

    //input parameter check
    GosDebugBegin(context != PNULL && pRohcPkt != PNULL, context, pRohcPkt, 0);
    return FALSE; 
    GosDebugEnd();

    if(pRohcPkt->type == ROHC_PACKET_TYPE_NORMAL)
    {
        if(pRohcPkt->private.infoNormal.cid == context->cid)
        {
            return TRUE;
        }
        else
        {
            ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressCheckContext_1, P_ERROR, 1, "Rohcv1UncompressDecompressCheckContext cid id %u in NORMAL", pRohcPkt->private.infoNormal.cid);
            return FALSE;         
        }
    }
    else if(pRohcPkt->type == ROHC_PACKET_TYPE_IR)
    {
        if(pRohcPkt->private.infoIR.cid == context->cid)
        {
            return TRUE;
        }
        else
        {
            ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressCheckContext_2, P_ERROR, 1, "Rohcv1UncompressDecompressCheckContext cid id %u in IR", pRohcPkt->private.infoIR.cid);
            return FALSE;         
        }        
    }
    else
    {
        ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressCheckContext_3, P_ERROR, 1, "Rohcv1UncompressDecompressCheckContext invalid type %u", pRohcPkt->type);
        return FALSE;    
    }

}


//RFC 5795 5.4.4
BOOL Rohcv1UncompressDecompressPktDecompress(RohcDecompressContext *context, RohcPacketBuff *pRohcPkt, RohcDecompressOutputHeaderBuff *pOutput, RohcFeedbackBuff *feedback)
{

    //input parameter check
    GosDebugBegin(context != PNULL && pRohcPkt != PNULL && pOutput != PNULL && feedback != PNULL, pOutput, pRohcPkt, feedback);
    return FALSE; 
    GosDebugEnd();

    //just rcv IR pkt
    if(pRohcPkt->type == ROHC_PACKET_TYPE_IR)
    {
        //do CRC check
        UINT8 *crcData;
        UINT16 crcLen;
        UINT8 crcResult;

        crcData = pRohcPkt->private.infoIR.pRawWithoutPadding;
        crcLen =  pRohcPkt->private.infoIR.infoProfilespeccific - pRohcPkt->private.infoIR.pRawWithoutPadding - 1;

        GosDebugBegin(crcData != PNULL && crcLen > 0, crcData, crcLen, 0);
        return FALSE; 
        GosDebugEnd();

        crcResult = RohcGernCrc8(crcData, crcLen);
        if(crcResult == pRohcPkt->private.infoIR.CRC)
        {
            ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressPktDecompress_1, P_INFO, 0, "Rohcv1UncompressDecompressPktDecompress CRC check pass");

            //check profile id
            if(pRohcPkt->private.infoIR.profileId != ROHC_PROFILE_UNCOMPRESSED)
            {
                ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressPktDecompress_2, P_WARNING, 1, "Rohcv1UncompressDecompressPktDecompress IR pkt profile id is not uncompress profile",
                    pRohcPkt->private.infoIR.profileId);
                return FALSE;                
            }
            else
            {
                //init output info
                context->cid = pRohcPkt->private.infoIR.cid;
                pOutput->decompressHdrLen = 0;
                pOutput->pDecompressHdrBuff = PNULL;
                pOutput->pPayload = pRohcPkt->private.infoIR.infoProfilespeccific;
                pOutput->payloadLen = pRohcPkt->private.infoIR.lenRawWithoutPadding - ((UINT16)(pRohcPkt->private.infoIR.infoProfilespeccific - pRohcPkt->private.infoIR.pRawWithoutPadding));

                //gen feedback
                UINT8 fData = ROHC_PROFILE_UNCOMPRESSED;
                RohcFeebackData feedbackData;
                RohcOutputBuff fdOutput;
                
                feedbackData.dataLen = 1;
                feedbackData.pData = &fData;
                fdOutput.buffLen = feedback->buffMaxLen;
                fdOutput.pData = feedback->pFeedbackBuff;
                if(RohcCreateFeedback(context->cid, &feedbackData, &fdOutput) == FALSE)
                {
                    ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressPktDecompress_3, P_WARNING, 1, "Rohcv1UncompressDecompressPktDecompress create feedback fail, cid %u",
                        context->cid);
                    return FALSE;                     
                }
                else
                {
                    feedback->feedbackLen = fdOutput.dataLen;
                }

                //transfer state
                RohcDecompressContextStateChange(context, ROHC_DECOMPRESS_STATE_FULL_CONTEXT, ROHC_DECOMP_STATE_CHANGE_CAUSE_DECOMP_SUCCESS);
                
            }
        }
        else
        {
            ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressPktDecompress_4, P_WARNING, 2, "Rohcv1UncompressDecompressPktDecompress CRC check fail,pkt CRC %u, cacl CRC %u",
                pRohcPkt->private.infoIR.CRC, crcResult);
            return FALSE;
        }
    }
    else if(pRohcPkt->type == ROHC_PACKET_TYPE_NORMAL)
    {
        //check cid
        if(context->state == ROHC_DECOMPRESS_STATE_FULL_CONTEXT)
        {
            if(context->cid == pRohcPkt->private.infoNormal.cid)
            {
                //init output info
                pOutput->decompressHdrLen = 1;
                pOutput->pDecompressHdrBuff[0] = pRohcPkt->private.infoNormal.firstOctet;
                pOutput->payloadLen = pRohcPkt->lenRawPkt -(pRohcPkt->pRawPkt - pRohcPkt->private.infoNormal.restOctect);
                pOutput->pPayload = pRohcPkt->private.infoNormal.restOctect;

                //init feedback info
                feedback->feedbackLen = 0;
                
            }
            else
            {
                ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressPktDecompress_5, P_WARNING, 2, "Rohcv1UncompressDecompressPktDecompress Normal pkt cid %u is not adpt context cid %u",
                        pRohcPkt->private.infoNormal.cid, context->cid);
                return FALSE;            
            }
        }
        else if(context->state == ROHC_DECOMPRESS_STATE_NO_CONTEXT)
        {
                ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressPktDecompress_6, P_WARNING, 1, "Rohcv1UncompressDecompressPktDecompress rcv Normal PKT,but context cid %u state is NO_CONTEXT", context->cid);
                return FALSE;        
        }
        else
        {
                ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressPktDecompress_7, P_WARNING, 2, "Rohcv1UncompressDecompressPktDecompress rcv Normal PKT,but context cid %u invalid state %u", context->cid, context->state);
                return FALSE;         
        }
    }
    else
    {
            ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressPktDecompress_8, P_WARNING, 2, "Rohcv1UncompressDecompressPktDecompress rcv Normal PKT,but context cid %u not support rohc pkt type %u", context->cid, pRohcPkt->type);
            return FALSE;     
    }

       context->lastUsedTime = RohcGetCurrentSysTime();

       ECOMM_TRACE(UNILOG_ROHC, Rohcv1UncompressDecompressPktDecompress_9, P_WARNING, 1, "Rohcv1UncompressDecompressPktDecompress rcv Normal PKT,but context cid %u decompress rohc pkt type %u SUCCESS", context->cid, pRohcPkt->type);

       return TRUE;
    
}


/******************************************************************************
 *****************************************************************************
 * global/static
 *****************************************************************************
******************************************************************************/


const RohcCompressProfileUtility rohcv1UncompressCompressUtility =
{
    .Create = Rohcv1UncompressCompressCreateContext,
    .Destroy = Rohcv1UncompressCompressDestroyContext,
    .CheckProile = Rohcv1UncompressCompressCheckProfile,
    .CheckContext = Rohcv1UncompressCompressCheckContext,
    .PktCompress = Rohcv1UncompressCompressPktCompress,
    .FeedbackProcess = Rohcv1UncompressCompressFeedbackProcess,
};

const RohcDecompressProfileUtility rohcv1UncompressDecompressUtility =
{
    .Create = Rohcv1UncompressDecompressCreateContext,
    .Destroy = Rohcv1UncompressDecompressDestroyContext,
    .CheckProile = Rohcv1UncompressDecompressCheckProfile,
    .CheckContext = Rohcv1UncompressDecompressCheckContext,
    .PktDecompress = Rohcv1UncompressDecompressPktDecompress,
};



