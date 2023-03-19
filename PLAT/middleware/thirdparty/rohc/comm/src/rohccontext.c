/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohccontext.c
 Description:    - rohc context related API
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/


#include "rohccontext.h"
#include "rohcpacket.h"
#include "rohcippacket.h"
#include "rohcprofile.h"

/******************************************************************************
 *****************************************************************************
 * global/static
 *****************************************************************************
******************************************************************************/

/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/

void RohcDecompressContextStateChange(RohcDecompressContext *context, UINT8 newState, UINT8 cause)
{
    //input parameter check
    GosDebugBegin(context != PNULL, context, 0, 0);
    return;
    GosDebugEnd();

    ECOMM_TRACE(UNILOG_ROHC, RohcDecompressContextStateChange_1, P_INFO, 4, "RohcDecompressContextStateChange cid %u change state %u to new state %u,cause %u"
        , context->cid, context->state, newState, cause);

    context->state = newState;

}

void RohcCompressContextStateChange(RohcCompressContext *context, UINT8 newState, UINT8 cause)
{
    //input parameter check
    GosDebugBegin(context != PNULL, context, 0, 0);
    return;
    GosDebugEnd();

    switch(context->mode)
    {
        case ROHC_MODE_U:
        {
            if(context->state == ROHC_COMPRESS_STATE_IR)
            {            
                if(newState == ROHC_COMPRESS_STATE_FO)
                {
                    context->pktsIrState = 0;
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_OPTIMISTIC_APPROACH)
                    {
                        context->statePrivate.umodePrivate.upward2FoPktsNum = 0;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_1, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }
                }
            }
            else if(context->state == ROHC_COMPRESS_STATE_FO)
            {
                if(newState == ROHC_COMPRESS_STATE_IR)
                {
                    context->pktsFoState = 0;                
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_PERIODIC_TIMEOUT)
                    {
                        context->statePrivate.umodePrivate.downwardTrans2IrPktsNum = 0;
                        context->statePrivate.umodePrivate.upward2SoPktsNum = 0;
                        context->statePrivate.umodePrivate.lastIrPktTime = 0;                        
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_2, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }
                }
                else if(newState == ROHC_COMPRESS_STATE_SO)
                {
                    context->pktsFoState = 0;
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_OPTIMISTIC_APPROACH)
                    {
                        context->statePrivate.umodePrivate.upward2SoPktsNum = 0;                        
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_3, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                    
                }
            }
            else if(context->state == ROHC_COMPRESS_STATE_SO)
            {
                if(newState == ROHC_COMPRESS_STATE_IR)
                {
                    context->pktsSoState = 0;
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_PERIODIC_TIMEOUT)
                    {
                        context->statePrivate.umodePrivate.downwardTrans2IrPktsNum = 0;
                        context->statePrivate.umodePrivate.downwardTrans2FoPktsNum = 0;
                        context->statePrivate.umodePrivate.lastIrPktTime = 0;
                        context->statePrivate.umodePrivate.lastFoPktTime = 0;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_4, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                    
                }
                else if(context->state == ROHC_COMPRESS_STATE_FO)
                {
                    context->pktsSoState = 0;
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_PERIODIC_TIMEOUT)
                    {
                        context->statePrivate.umodePrivate.downwardTrans2FoPktsNum = 0;
                        context->statePrivate.umodePrivate.lastFoPktTime = 0;
                    }
                    else if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_UPDATE)
                    {
                        context->statePrivate.umodePrivate.downwardTrans2FoPktsNum = 0;
                        context->statePrivate.umodePrivate.lastFoPktTime = 0;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_5, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                    
                }
            }
            break;
        }
        case ROHC_MODE_O:
        {
            if(context->state == ROHC_COMPRESS_STATE_IR)
            {            
                if(newState == ROHC_COMPRESS_STATE_FO)
                {
                    context->pktsIrState = 0;
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_OPTIMISTIC_APPROACH)
                    {
                        context->statePrivate.omodePrivate.upward2FoPktsNum = 0;
                    }
                    else if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_ACK)
                    {
                        context->statePrivate.omodePrivate.upward2FoPktsNum = 0;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_6, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                    
                }
            }
            else if(context->state == ROHC_COMPRESS_STATE_FO)
            {
                if(newState == ROHC_COMPRESS_STATE_IR)
                {
                    context->pktsFoState = 0;                
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_STATIC_NACK)
                    {
                        context->statePrivate.omodePrivate.upward2SoPktsNum = 0;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_7, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                    
                }
                else if(newState == ROHC_COMPRESS_STATE_SO)
                {
                    context->pktsFoState = 0;
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_OPTIMISTIC_APPROACH)
                    {
                        context->statePrivate.omodePrivate.upward2SoPktsNum = 0;                        
                    }
                    else if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_ACK)
                    {
                        context->statePrivate.omodePrivate.upward2SoPktsNum = 0;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_8, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                    
                }
            }
            else if(context->state == ROHC_COMPRESS_STATE_SO)
            {
                if(newState == ROHC_COMPRESS_STATE_IR)
                {
                    context->pktsSoState = 0;
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_STATIC_NACK)
                    {
                        context->statePrivate.omodePrivate.upward2FoPktsNum = 0;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_9, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                     
                }
                else if(context->state == ROHC_COMPRESS_STATE_FO)
                {
                    context->pktsSoState = 0;
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_NACK)
                    {
                        context->statePrivate.omodePrivate.upward2SoPktsNum = 0;
                    }
                    else if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_UPDATE)
                    {
                        context->statePrivate.omodePrivate.upward2SoPktsNum = 0;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_10, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                    
                }
                else if(context->state == ROHC_COMPRESS_STATE_SO)
                {
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_ACK)
                    {
                        //do nothing
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_11, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                 
                }
            }
            break;
        }
        case ROHC_MODE_R:
        {
            if(context->state == ROHC_COMPRESS_STATE_IR)
            {            
                if(newState == ROHC_COMPRESS_STATE_FO)
                {
                    context->pktsIrState = 0;
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_ACK)
                    {
                        //do nothing
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_12, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                    
                }
            }
            else if(context->state == ROHC_COMPRESS_STATE_FO)
            {
                if(newState == ROHC_COMPRESS_STATE_IR)
                {
                    context->pktsFoState = 0;                
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_STATIC_NACK)
                    {
                        //do nothing
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_13, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                    
                }
                else if(newState == ROHC_COMPRESS_STATE_SO)
                {
                    context->pktsFoState = 0;
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_ACK)
                    {
                        //do nothing
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_14, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                    
                }
            }
            else if(context->state == ROHC_COMPRESS_STATE_SO)
            {
                if(newState == ROHC_COMPRESS_STATE_IR)
                {
                    context->pktsSoState = 0;
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_STATIC_NACK)
                    {
                        //do nothing
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_15, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                     
                }
                else if(context->state == ROHC_COMPRESS_STATE_FO)
                {
                    context->pktsSoState = 0;
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_NACK)
                    {
                        //do nothing
                    }
                    else if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_UPDATE)
                    {
                        //do nothing
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_15, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                    
                }
                else if(context->state == ROHC_COMPRESS_STATE_SO)
                {
                    if(cause == ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_ACK)
                    {
                        //do nothing
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_16, P_WARNING, 1, "RohcCompressContextStateChange invalid cause %u", cause);
                    }                 
                }
            }
            break;
        }
            
    }

    ECOMM_TRACE(UNILOG_ROHC, RohcCompressContextStateChange_17, P_INFO, 3, "RohcCompressContextStateChange context change from %u to %u, cause %u", context->state, newState, cause);

    context->state = newState;
        
}


//RFC 3095 5.3.1 U mode state transition downward > upward
void RohcCompressUmodeStateTransition(RohcCompressContext *context, UINT32 CurrTime, UINT8 targetState)
{

    //input parameter check
    GosDebugBegin(context != PNULL, context, 0, 0);
    return;
    GosDebugEnd();    

    if(context->mode != ROHC_MODE_U)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCompressUmodeStateTransition_1, P_WARNING, 1, "RohcCompressUmodeStateTransition the context mode %u is not U-MODE", context->mode);
        return;
    }

    //first check SO state
    if(context->state == ROHC_COMPRESS_STATE_SO)
    {
        //check thansfer to IR state
        if((context->statePrivate.umodePrivate.downwardTrans2IrPktsNum >= context->statePrivate.umodePrivate.refreshIrPktsNum) || (CurrTime - context->statePrivate.umodePrivate.lastIrPktTime >= context->statePrivate.umodePrivate.refreshIrTimeout))
        {
            RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_IR, ROHC_COMP_STATE_CHANGE_CAUSE_PERIODIC_TIMEOUT);
        } //check transe fer to FO state
        else if((context->statePrivate.umodePrivate.downwardTrans2FoPktsNum  >= context->statePrivate.umodePrivate.refreshFoPktsNum) || (CurrTime - context->statePrivate.umodePrivate.lastFoPktTime >= context->statePrivate.umodePrivate.refreshFoTimeout))
        {
            RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_FO, ROHC_COMP_STATE_CHANGE_CAUSE_PERIODIC_TIMEOUT);
        }
        
    }
    else if(context->state == ROHC_COMPRESS_STATE_FO)
    {
        //check thansfer to IR state(high priority)
        if((context->statePrivate.umodePrivate.downwardTrans2IrPktsNum >= context->statePrivate.umodePrivate.refreshIrPktsNum) || (CurrTime - context->statePrivate.umodePrivate.lastIrPktTime >= context->statePrivate.umodePrivate.refreshIrTimeout))
        {
            RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_IR, ROHC_COMP_STATE_CHANGE_CAUSE_PERIODIC_TIMEOUT);
        }
        else if(context->pktsFoState >= context->statePrivate.umodePrivate.upward2SoPktsNum)
        {
            if(targetState == ROHC_COMPRESS_STATE_SO)
            {
                RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_SO, ROHC_COMP_STATE_CHANGE_CAUSE_OPTIMISTIC_APPROACH);
            }
        }      
    }
    else if(context->state == ROHC_COMPRESS_STATE_IR)
    {
        if(context->pktsIrState >= context->statePrivate.umodePrivate.upward2FoPktsNum)
        {
            if(targetState == ROHC_COMPRESS_STATE_FO || targetState == ROHC_COMPRESS_STATE_SO)
            {
                RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_FO, ROHC_COMP_STATE_CHANGE_CAUSE_OPTIMISTIC_APPROACH);
            }
        } 

    }
}


//RFC 3095 5.4.1 downward > upward
void RohcCompressOmodeStateTransition(RohcCompressContext *context, UINT8 feedbackType, BOOL update, UINT8 targetState)
{
    
    //input parameter check
    GosDebugBegin(context != PNULL, context, 0, 0);
    return;
    GosDebugEnd();    

    if(context->mode != ROHC_MODE_O)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCompressOmodeStateTransition_1, P_WARNING, 1, "RohcCompressOmodeStateTransition the context mode %u is not O-MODE", context->mode);
        return;
    }


    if(context->state == ROHC_COMPRESS_STATE_SO)
    {
        if(feedbackType == ROHC_FEEDBACK_TYPE_ACK)
        {
            RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_SO, ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_ACK);           
        }
        else if(feedbackType == ROHC_FEEDBACK_TYPE_NACK)
        {
            RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_FO, ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_NACK); 
        }
        else if(feedbackType == ROHC_FEEDBACK_TYPE_STATIC_NACK)
        {
            RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_IR, ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_STATIC_NACK); 
        }
        else if(update == TRUE)
        {
            RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_FO, ROHC_COMP_STATE_CHANGE_CAUSE_UPDATE); 
        }
    }
    else if(context->state == ROHC_COMPRESS_STATE_FO)
    {
        if(feedbackType == ROHC_FEEDBACK_TYPE_STATIC_NACK)
        {
            RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_IR, ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_STATIC_NACK);            
        }
        else
        {
            if(targetState == ROHC_COMPRESS_STATE_SO)
            {
                if(feedbackType == ROHC_FEEDBACK_TYPE_ACK)
                {
                    RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_SO, ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_ACK);
                }
                else if(context->pktsFoState >= context->statePrivate.omodePrivate.upward2SoPktsNum)
                {
                    RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_SO, ROHC_COMP_STATE_CHANGE_CAUSE_OPTIMISTIC_APPROACH);
                }                   
            }
        }    
    }
    else if(context->state == ROHC_COMPRESS_STATE_IR)
    {
        if(targetState == ROHC_COMPRESS_STATE_SO || targetState == ROHC_COMPRESS_STATE_FO)
        {
            if(context->pktsIrState >= context->statePrivate.omodePrivate.upward2FoPktsNum)
            {
                RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_FO, ROHC_COMP_STATE_CHANGE_CAUSE_OPTIMISTIC_APPROACH);
            }
        }
    }
}


//RFC 3095 5.5.1 downward > upward
void RohcCompressRmodeStateTransition(RohcCompressContext *context, UINT8 feedbackType, BOOL update, UINT8 targetState)
{
    
    //input parameter check
    GosDebugBegin(context != PNULL, context, 0, 0);
    return;
    GosDebugEnd();    

    if(context->mode != ROHC_MODE_R)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCompressRmodeStateTransition_1, P_WARNING, 1, "RohcCompressRmodeStateTransition the context mode %u is not O-MODE", context->mode);
        return;
    }


    if(context->state == ROHC_COMPRESS_STATE_SO)
    {
        if(feedbackType == ROHC_FEEDBACK_TYPE_ACK)
        {
            RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_SO, ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_ACK);           
        }
        else if(feedbackType == ROHC_FEEDBACK_TYPE_NACK)
        {
            RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_FO, ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_NACK); 
        }
        else if(feedbackType == ROHC_FEEDBACK_TYPE_STATIC_NACK)
        {
            RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_IR, ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_STATIC_NACK); 
        }
        else if(update == TRUE)
        {
            RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_FO, ROHC_COMP_STATE_CHANGE_CAUSE_UPDATE); 
        }
    }
    else if(context->state == ROHC_COMPRESS_STATE_FO)
    {
        if(feedbackType == ROHC_FEEDBACK_TYPE_STATIC_NACK)
        {
            RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_IR, ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_STATIC_NACK);            
        }
        else
        {
            if(targetState == ROHC_COMPRESS_STATE_SO)
            {
                if(feedbackType == ROHC_FEEDBACK_TYPE_ACK)
                {
                    RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_SO, ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_ACK);
                }                  
            }
        }    
    }
    else if(context->state == ROHC_COMPRESS_STATE_IR)
    {
        if(targetState == ROHC_COMPRESS_STATE_SO || targetState == ROHC_COMPRESS_STATE_FO)
        {
            if(feedbackType == ROHC_FEEDBACK_TYPE_ACK)
            {
                RohcCompressContextStateChange(context, ROHC_COMPRESS_STATE_FO, ROHC_COMP_STATE_CHANGE_CAUSE_FEEDBACK_ACK);
            } 

        }
    }
}




void RohcDestroyCompressContext(RohcCompressContext *compressContext)
{
    RohcCompressProfileUtility *utility;

    if(compressContext && compressContext->pProfileUtility)
    {
        utility = (RohcCompressProfileUtility *)compressContext->pProfileUtility;
        
        utility->Destroy(compressContext);
    }
}

BOOL RohcCreateCompressContext(RohcCompressContext *compressContext, RohcIpPacket *pIpPkt)
{
    RohcCompressProfileUtility *utility;

    if(compressContext && compressContext->pProfileUtility)
    {
        utility = (RohcCompressProfileUtility *)compressContext->pProfileUtility;
        
        return utility->Create(compressContext, pIpPkt);
    }
    else
    {
        return FALSE;
    }
}

BOOL RohcCompressContextCheckProfile(RohcCompressContext *compressContext, RohcIpPacket *pIpPkt)
{
    RohcCompressProfileUtility *utility;

    if(compressContext && compressContext->pProfileUtility)
    {
        utility = (RohcCompressProfileUtility *)compressContext->pProfileUtility;
        
        return utility->CheckProile(pIpPkt);
    }
    else
    {
        return FALSE;
    }
}

BOOL RohcCompressContextCheckContext(RohcCompressContext *compressContext, RohcIpPacket *pIpPkt)
{
    RohcCompressProfileUtility *utility;

    if(compressContext && compressContext->pProfileUtility)
    {
        utility = (RohcCompressProfileUtility *)compressContext->pProfileUtility;
        
        return utility->CheckContext(compressContext, pIpPkt);
    }
    else
    {
        return FALSE;
    }
}

BOOL RohcCompressContextCompress(RohcCompressContext *compressContext, RohcIpPacket *pIpPkt, RohcCompressOutputHeaderBuff *pOutput)
{
    RohcCompressProfileUtility *utility;

    if(compressContext && compressContext->pProfileUtility)
    {
        utility = (RohcCompressProfileUtility *)compressContext->pProfileUtility;
        
        return utility->PktCompress(compressContext, pIpPkt, pOutput);
    }
    else
    {
        return FALSE;
    }
}

BOOL RohcCompressContextProcessFeedback(RohcCompressContext *compressContext, RohcPacketBuff *pRohcPkt)
{
    RohcCompressProfileUtility *utility;

    if(compressContext && compressContext->pProfileUtility)
    {
        utility = (RohcCompressProfileUtility *)compressContext->pProfileUtility;
        
        return utility->FeedbackProcess(compressContext, pRohcPkt);
    }
    else
    {
        return FALSE;
    }
}

void RohcDestroyDecompressContext(RohcDecompressContext *decompressContext)
{
    RohcDecompressProfileUtility *utility;

    if(decompressContext && decompressContext->pProfileUtility)
    {
        utility = (RohcDecompressProfileUtility *)decompressContext->pProfileUtility;
        
        utility->Destroy(decompressContext);
    }
}

BOOL RohcCreateDecompressContext(RohcDecompressContext *decompressContext, RohcPacketBuff *pRohcPkt)
{
    RohcDecompressProfileUtility *utility;

    if(decompressContext && decompressContext->pProfileUtility)
    {
        utility = (RohcDecompressProfileUtility *)decompressContext->pProfileUtility;
        
        return utility->Create(decompressContext, pRohcPkt);
    }
    else
    {
        return FALSE;
    }
}

BOOL RohcDecompressContextCheckProfile(RohcDecompressContext *decompressContext, RohcPacketBuff *pRohcPkt)
{
    RohcDecompressProfileUtility *utility;

    if(decompressContext && decompressContext->pProfileUtility)
    {
        utility = (RohcDecompressProfileUtility *)decompressContext->pProfileUtility;
        
        return utility->CheckProile(pRohcPkt);
    }
    else
    {
        return FALSE;
    }
}

BOOL RohcDecompressContextCheckContext(RohcDecompressContext *decompressContext, RohcPacketBuff *pRohcPkt)
{
    RohcDecompressProfileUtility *utility;

    if(decompressContext && decompressContext->pProfileUtility)
    {
        utility = (RohcDecompressProfileUtility *)decompressContext->pProfileUtility;
        
        return utility->CheckContext(decompressContext, pRohcPkt);
    }
    else
    {
        return FALSE;
    }
}

BOOL RohcDecompressContextDecompress(RohcDecompressContext *decompressContext, RohcPacketBuff *pRohcPkt, RohcDecompressOutputHeaderBuff *pOutput, RohcFeedbackBuff *feedback)
{
    RohcDecompressProfileUtility *utility;

    if(decompressContext && decompressContext->pProfileUtility)
    {
        utility = (RohcDecompressProfileUtility *)decompressContext->pProfileUtility;
        
        return utility->PktDecompress(decompressContext, pRohcPkt, pOutput, feedback);
    }
    else
    {
        return FALSE;
    }
}

