/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcchannel.c
 Description:    - rohc compress/decompress channel related API
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/

#include "rohcconfig.h"
#include "rohcchannel.h"
#include "rohcpacket.h"
#include "rohcippacket.h"
#include "rohcprofile.h"

/******************************************************************************
 *****************************************************************************
 * global/static
 *****************************************************************************
******************************************************************************/
static RohcList gRohcCompressChannelListHeader = {PNULL, PNULL};
static RohcList gRohcDecompressChannelListHeader = {PNULL, PNULL};

/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/


RohcCompressContext *RohcCompressGetAdptContext(RohcCompressChannel *channel, RohcIpPacket *inpPkt)
{

    RohcCompressContext *compressContext = PNULL;
    RohcList *list;    

    //input parameter check
    GosDebugBegin(channel != PNULL && inpPkt != PNULL, channel, inpPkt, 0);
    return PNULL;
    GosDebugEnd();

    for(list = channel->pContextList.next; list; list = list->next)
    {
        compressContext = (RohcCompressContext *)RohcGetListBody(list);
        if(RohcCompressContextCheckContext(compressContext, inpPkt) == TRUE)
        {
            return compressContext;
        }
    }

    return PNULL;
   
}

RohcDecompressContext *RohcDecompressGetAdptContext(RohcDecompressChannel *channel, RohcPacketBuff *rohcPacket)
{

    RohcDecompressContext *decompressContext = PNULL;
    RohcList *list;    

    //input parameter check
    GosDebugBegin(channel != PNULL && rohcPacket != PNULL, channel, rohcPacket, 0);
    return PNULL;
    GosDebugEnd();

    for(list = channel->pContextList.next; list; list = list->next)
    {
        decompressContext = (RohcDecompressContext *)RohcGetListBody(list);
        if(RohcDecompressContextCheckContext(decompressContext, rohcPacket) == TRUE)
        {
            return decompressContext;
        }        
    }

    return PNULL;
   
}

void RohcDestroyTimeoutCompressContext(RohcCompressChannel *channel)
{
    UINT32 currentMs;
    RohcCompressContext *compressContext = PNULL;
    RohcList *list;

    //input parameter check
    GosDebugBegin(channel != PNULL , channel, 0, 0);
    return;
    GosDebugEnd();    

    currentMs = RohcGetCurrentSysTime();
    for(list = channel->pContextList.next; list; list = list->next)
    {        
        compressContext = (RohcCompressContext *)RohcGetListBody(list);
        if(compressContext)
        {
            if(currentMs >= compressContext->lastUsedTime + ROHC_COMPRESS_CONTEXT_TIMEOUT)
            {
                ECOMM_TRACE(UNILOG_ROHC, RohcDestroyTimeoutCompressContext_1, P_WARNING, 1, "RohcDestroyTimeoutCompressContext cid context has timeout", compressContext->cid);
                RohcDestroyCompressContext(compressContext);
                RohcRemoveList(&channel->pContextList, list);
                RohcFreeList(list);
                channel->activeCidNum --;
            }
        }
        
    }
        
}

void RohcDestroyTimeoutDecompressContext(RohcDecompressChannel *channel)
{
    UINT32 currentMs;
    RohcDecompressContext *decompressContext = PNULL;
    RohcList *list;

    //input parameter check
    GosDebugBegin(channel != PNULL , channel, 0, 0);
    return;
    GosDebugEnd();    

    currentMs = RohcGetCurrentSysTime();
    for(list = channel->pContextList.next; list; list = list->next)
    {        
        decompressContext = (RohcDecompressContext *)RohcGetListBody(list);
        if(decompressContext)
        {
            if(currentMs >= decompressContext->lastUsedTime + ROHC_COMPRESS_CONTEXT_TIMEOUT)
            {
                ECOMM_TRACE(UNILOG_ROHC, RohcDestroyTimeoutDecompressContext_1, P_WARNING, 1, "RohcDestroyTimeoutDecompressContext cid context has timeout", decompressContext->cid);
                RohcDestroyDecompressContext(decompressContext);
                RohcRemoveList(&channel->pContextList, list);
                RohcFreeList(list);
                channel->activeCidNum --;
            }
        }
        
    }
        
}

void RohcDestryOldestCompressContext(RohcCompressChannel *channel)
{
    RohcCompressContext *compressContext = PNULL;
    RohcCompressContext *compressContextOldest = PNULL;    
    RohcList *list;
    RohcList *listOldest = PNULL;

    //input parameter check
    GosDebugBegin(channel != PNULL , channel, 0, 0);
    return;
    GosDebugEnd();    

    for(list = channel->pContextList.next; list; list = list->next)
    {        
        compressContext = (RohcCompressContext *)RohcGetListBody(list);
        if(compressContext)
        {
            if(compressContextOldest)
            {
                if(compressContext->lastUsedTime < compressContextOldest->lastUsedTime)
                {
                    compressContextOldest = compressContext;
                    listOldest = list;
                }
            }
            else
            {
                compressContextOldest = compressContext;
                listOldest = list;
            }
        }
    }

    if(compressContextOldest)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcDestryOldestCompressContext_1, P_WARNING, 1, "RohcDestryOldestCompressContexts destroy oldest context", compressContextOldest->cid);
        
        RohcDestroyCompressContext(compressContextOldest);
        RohcRemoveList(&channel->pContextList, listOldest);
        RohcFreeList(listOldest);
        channel->activeCidNum --;   
    }
        
}

void RohcDestryOldestDecompressContext(RohcDecompressChannel *channel)
{
    RohcDecompressContext *decompressContext = PNULL;
    RohcDecompressContext *decompressContextOldest = PNULL;    
    RohcList *list;
    RohcList *listOldest = PNULL;

    //input parameter check
    GosDebugBegin(channel != PNULL , channel, 0, 0);
    return;
    GosDebugEnd();    

    for(list = channel->pContextList.next; list; list = list->next)
    {        
        decompressContext = (RohcDecompressContext *)RohcGetListBody(list);
        if(decompressContext)
        {
            if(decompressContextOldest)
            {
                if(decompressContext->lastUsedTime < decompressContextOldest->lastUsedTime)
                {
                    decompressContextOldest = decompressContext;
                    listOldest = list;
                }
            }
            else
            {
                decompressContextOldest = decompressContext;
                listOldest = list;
            }
        }
    }

    if(decompressContextOldest)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcDestryOldestDecompressContext_1, P_WARNING, 1, "RohcDestryOldestDecompressContexts destroy oldest context", decompressContextOldest->cid);

        RohcDestroyDecompressContext(decompressContextOldest);
        RohcRemoveList(&channel->pContextList, listOldest);
        RohcFreeList(listOldest);
        channel->activeCidNum --;   
    }
        
}

UINT16 RohcGetFreeCompressCid(RohcCompressChannel *channel)
{
    UINT16 cid = ROHC_INVAID_CID;
    RohcCompressContext *compressContext = PNULL;
    RohcList *list;
    BOOL used = FALSE;

    //input parameter check
    GosDebugBegin(channel != PNULL , channel, 0, 0);
    return cid;
    GosDebugEnd();    

    for(cid = ROHC_DEFAULT_CID; cid <= channel->maxCid + 1; cid ++)
    {
        //check whether the cid has used
        for(list = channel->pContextList.next; list; list = list->next)
        {        
            compressContext = (RohcCompressContext *)RohcGetListBody(list);
            if(compressContext->cid == cid)
            {
                used = TRUE;
                break;
            }
        }

        if(used == TRUE)
        {
            used = FALSE;
            continue;
        }
        else
        {
            break;
        }
        
    }

    if(cid == channel->maxCid + 1)
    {
        cid = ROHC_INVAID_CID;
    }

    return cid;

    
}

UINT16 RohcGetFreeDecompressCid(RohcDecompressChannel *channel)
{
    UINT16 cid = ROHC_INVAID_CID;
    RohcDecompressContext *decompressContext = PNULL;
    RohcList *list;
    BOOL used = FALSE;

    //input parameter check
    GosDebugBegin(channel != PNULL , channel, 0, 0);
    return cid;
    GosDebugEnd();    

    for(cid = ROHC_DEFAULT_CID; cid <= channel->maxCid + 1; cid ++)
    {
        //check whether the cid has used
        for(list = channel->pContextList.next; list; list = list->next)
        {        
            decompressContext = (RohcDecompressContext *)RohcGetListBody(list);
            if(decompressContext->cid == cid)
            {
                used = TRUE;
                break;
            }
        }

        if(used == TRUE)
        {
            used = FALSE;
            continue;
        }
        else
        {
            break;
        }
        
    }

    if(cid == channel->maxCid + 1)
    {
        cid = ROHC_INVAID_CID;
    }

    return cid;

    
}

RohcList *RohcCreateNewCompressContextList(RohcCompressChannel *channel, UINT32 profileId)
{

    RohcList * newContextList = PNULL;
    RohcCompressContext *newCompressContext = PNULL;
    RohcProfileCapability *pProfileUtility = PNULL;
    UINT16 newCid = ROHC_INVAID_CID;
    
    //input parameter check
    GosDebugBegin(channel != PNULL , channel, 0, 0);
    return PNULL;
    GosDebugEnd();

    //check current cid used number
    if(channel->activeCidNum >= channel->maxCid)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewCompressContextList_1, P_WARNING, 1, "RohcCreateNewCompressContextList context number %u reach max", channel->activeCidNum);

        //try destroy timeout context
        RohcDestroyTimeoutCompressContext(channel);

        if(channel->activeCidNum >= channel->maxCid)
        {
            ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewCompressContextList_2, P_WARNING, 0, "RohcCreateNewCompressContextList need destroy the oldest context");
            RohcDestryOldestCompressContext(channel);
        }
    }

    if(channel->activeCidNum >= channel->maxCid)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewCompressContextList_3, P_ERROR, 0, "RohcCreateNewCompressContextList context reach max");
        return newContextList;
    }
    else
    {
        newCid = RohcGetFreeCompressCid(channel);
        if(newCid == ROHC_INVAID_CID)
        {
            ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewCompressContextList_4, P_ERROR, 0, "RohcCreateNewCompressContextList can not find adpt cid");
            return newContextList;            
        }
        else
        {
            newContextList = RohcNewList(sizeof(RohcCompressContext));
            if(newContextList == PNULL)
            {
                ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewCompressContextList_5, P_ERROR, 0, "RohcCreateNewCompressContextList malloc context fail");
                return newContextList;            
            }
            else
            {
                newCompressContext = (RohcCompressContext *)RohcGetListBody(newContextList);

                OsaCheck(newCompressContext != PNULL, newCompressContext, 0, 0);

                //init new compress context
                if(newCompressContext)
                {
                    newCompressContext->cid = newCid;
                    newCompressContext->profileId = profileId;
                    newCompressContext->mode = ROHC_MODE_U;
                    newCompressContext->state = ROHC_COMPRESS_STATE_IR;
                    pProfileUtility = RohcGetProfileUtilityByProfileId(profileId);
                    if(pProfileUtility && pProfileUtility->pProfiltCompressUtility != PNULL)
                    {
                        newCompressContext->pProfileUtility = pProfileUtility->pProfiltCompressUtility;
                    }
                    else
                    {
                        ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewCompressContextList_6, P_ERROR, 0, "RohcCreateNewCompressContextList profile compress utility is invalid");
                        RohcFreeList(newContextList);
                        return PNULL;
                    }                  
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewCompressContextList_7, P_ERROR, 0, "RohcCreateNewCompressContextList context is invalid");
                    RohcFreeList(newContextList);
                    return PNULL;                
                }
                
            }
        }
    }

    ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewCompressContextList_8, P_INFO, 2, "RohcCreateNewCompressContextList new context success 0x%x, profile id %u", newContextList, profileId);

    return newContextList;
}

RohcList *RohcCreateNewDecompressContextList(RohcDecompressChannel *channel, UINT32 profileId, UINT8 targetMode)
{

    RohcList * newContextList = PNULL;
    RohcDecompressContext *newDecompressContext = PNULL;
    RohcProfileCapability *pProfileUtility = PNULL;
    
    //input parameter check
    GosDebugBegin(channel != PNULL , channel, 0, 0);
    return PNULL;
    GosDebugEnd();

    //check current cid used number
    if(channel->activeCidNum >= channel->maxCid)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewDecompressContextList_1, P_WARNING, 1, "RohcCreateNewDecompressContextList context number %u reach max", channel->activeCidNum);

        //try destroy timeout context
        RohcDestroyTimeoutDecompressContext(channel);

        if(channel->activeCidNum >= channel->maxCid)
        {
            ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewDecompressContextList_2, P_WARNING, 0, "RohcCreateNewDecompressContextList need destroy the oldest context");
            RohcDestryOldestDecompressContext(channel);
        }
    }

    if(channel->activeCidNum >= channel->maxCid)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewDecompressContextList_3, P_ERROR, 0, "RohcCreateNewDecompressContextList context reach max");
        return newContextList;
    }
    else
    {
        newContextList = RohcNewList(sizeof(RohcDecompressContext));
        if(newContextList == PNULL)
        {
            ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewDecompressContextList_5, P_ERROR, 0, "RohcCreateNewDecompressContextList malloc context fail");
            return newContextList;            
        }
        else
        {
            newDecompressContext = (RohcDecompressContext *)RohcGetListBody(newContextList);

            OsaCheck(newDecompressContext != PNULL, newDecompressContext, 0, 0);

            //init new compress context
            if(newDecompressContext)
            {
                newDecompressContext->cid = ROHC_INVAID_CID;
                newDecompressContext->profileId = profileId;
                newDecompressContext->state = ROHC_DECOMPRESS_STATE_NO_CONTEXT;
                newDecompressContext->targetMode = targetMode;
                pProfileUtility = RohcGetProfileUtilityByProfileId(profileId);
                if(pProfileUtility && pProfileUtility->pProfiltDecompressUtility != PNULL)
                {
                    newDecompressContext->pProfileUtility = pProfileUtility->pProfiltDecompressUtility;
                }
                else
                {
                    ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewDecompressContextList_6, P_ERROR, 0, "RohcCreateNewDecompressContextList profile compress utility is invalid");
                    RohcFreeList(newContextList);
                    return PNULL;
                }              
            }
            else
            {
                ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewDecompressContextList_7, P_ERROR, 0, "RohcCreateNewDecompressContextList context is invalid");
                RohcFreeList(newContextList);
                return PNULL;                
            }        
        }
    }

    ECOMM_TRACE(UNILOG_ROHC, RohcCreateNewDecompressContextList_8, P_INFO, 3, "RohcCreateNewDecompressContextList new context list 0x%x, profileid %u, target mode %u", newContextList, profileId, targetMode);

    return newContextList;
}

RohcCompressContext *RohcGetCompressContextByCid(RohcCompressChannel *channel, UINT16 cid)
{
    RohcCompressContext *compressContext = PNULL;
    RohcList *list;

    //input parameter check
    GosDebugBegin(channel != PNULL , channel, 0, 0);
    return PNULL;
    GosDebugEnd();

    for(list = channel->pContextList.next; list; list = list->next)
    {
        compressContext = (RohcCompressContext *)RohcGetListBody(list);
        if(compressContext && compressContext->cid == cid)
        {
            break;
        }
    }

    return compressContext;
    
}

RohcDecompressContext *RohcGetDecompressContextByCid(RohcDecompressChannel *channel, UINT16 cid)
{
    RohcDecompressContext *decompressContext = PNULL;
    RohcList *list;

    //input parameter check
    GosDebugBegin(channel != PNULL , channel, 0, 0);
    return PNULL;
    GosDebugEnd();

    for(list = channel->pContextList.next; list; list = list->next)
    {
        decompressContext = (RohcDecompressContext *)RohcGetListBody(list);
        if(decompressContext && decompressContext->cid == cid)
        {
            break;
        }
    }

    return decompressContext;
    
}

RohcCompressChannel *RohcCreateCompressChannel(UINT32 profiles, UINT16 maxCid)
{
    RohcList *list;
    RohcCompressChannel *compressChannel = PNULL;

    //new rohc compress channel list
    list = RohcNewList(sizeof(RohcCompressChannel));

    if(list  == PNULL)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCreateCompressChannel_1, P_ERROR, 0, "RohcCreateCompressChannel malloc compress channel list fail");
        return (void *)compressChannel;
    }

    compressChannel = (RohcCompressChannel *)list->private;
    
    GosDebugBegin(compressChannel != PNULL, compressChannel, 0, 0);    
    RohcFreeList(list);
    return PNULL;
    GosDebugEnd();

    
    //init compress channel
    compressChannel->magic = ROHC_COMPRESS_CHANNEL_MAGIC;
    compressChannel->activeCidNum = 0;
    compressChannel->capablilty = profiles;
    compressChannel->maxCid = maxCid;

    //add global compress channel list
    RohcInsertList(&gRohcCompressChannelListHeader, list);

    ECOMM_TRACE(UNILOG_ROHC, RohcCreateCompressChannel_2, P_INFO, 2, "RohcCreateCompressChannel create compress channel success, profiles %u, maxcid %u", profiles, maxCid);

    return compressChannel;
    
}

void RohcDestroyCompressChannel(RohcCompressChannel *compressChannel)
{
    RohcCompressContext *compressContext = PNULL;
    RohcList *listContext;
    RohcList *listChannel;

    //input parameter check
    GosDebugBegin(compressChannel != PNULL , compressChannel, 0, 0);
    return;
    GosDebugEnd();


    //check magic
    OsaCheck(compressChannel->magic == ROHC_COMPRESS_CHANNEL_MAGIC, compressChannel->magic, 0, 0);

    //check compress channel list
    listChannel = RohcGetListByBody((void *)compressChannel);
    OsaCheck(listChannel != PNULL, listChannel, 0, 0);
    if(RohcCheckListOwner(&gRohcCompressChannelListHeader, listChannel) == FALSE)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcDestroyCompressChannel_1, P_ERROR, 0, "RohcDestroyCompressChannel the channel is not create");
        return;
    }

    //destroy context
    for(listContext = compressChannel->pContextList.next; listContext; listContext = listContext->next)
    {
        compressContext = (RohcCompressContext *)RohcGetListBody(listContext);
        RohcDestroyCompressContext(compressContext);
        RohcRemoveList(&compressChannel->pContextList, listContext);
        RohcFreeList(listContext);
        
    }

    RohcRemoveList(&gRohcCompressChannelListHeader, listChannel);

    RohcFreeList(listChannel);
    
}

RohcDecompressChannel *RohcCreateDecompressChannel(UINT32 profiles, UINT16 maxCid, UINT8 targetMode)
{
    RohcList *list;
    RohcDecompressChannel *decompressChannel = PNULL;

    //new rohc compress channel list
    list = RohcNewList(sizeof(RohcDecompressChannel));

    if(list  == PNULL)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcCreateDecompressChannel_1, P_ERROR, 0, "RohcCreateDecompressChannel malloc decompress channel fail");
        return (void *)decompressChannel;
    }

    decompressChannel = (RohcDecompressChannel *)list->private;

    GosDebugBegin(decompressChannel != PNULL, decompressChannel, 0, 0);
    RohcFreeList(list);
    return PNULL;
    GosDebugEnd();

    //init compress channel
    decompressChannel->magic = ROHC_DECOMPRESS_CHANNEL_MAGIC;
    decompressChannel->activeCidNum = 0;
    decompressChannel->capablilty = profiles;
    decompressChannel->maxCid = maxCid;
    decompressChannel->targetMode = targetMode;

    //add global compress channel list
    RohcInsertList(&gRohcDecompressChannelListHeader, list);    

    ECOMM_TRACE(UNILOG_ROHC, RohcCreateDecompressChannel_2, P_INFO, 2, "RohcCreateDecompressChannel create decompress channel success, profiles %u, maxcid %u", profiles, maxCid);

    return decompressChannel;
    
}

void RohcDestroyDecompressChannel(RohcDecompressChannel *decompressChannel)
{
    RohcDecompressContext *decompressContext = PNULL;
    RohcList *listContext;
    RohcList *listChannel;

     //input parameter check
    GosDebugBegin(decompressChannel != PNULL , decompressChannel, 0, 0);
    return;
    GosDebugEnd();

    //check magic
    OsaCheck(decompressChannel->magic == ROHC_DECOMPRESS_CHANNEL_MAGIC, decompressChannel->magic, 0, 0);

    //check compress channel list
    listChannel = RohcGetListByBody((void *)decompressChannel);
    OsaCheck(listChannel != PNULL, listChannel, 0, 0);
    if(RohcCheckListOwner(&gRohcDecompressChannelListHeader, listChannel) == FALSE)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcDestroyDecompressChannel_1, P_ERROR, 0, "RohcDestroyDecompressChannel the channel is not create");
        return;
    }    

    //destroy context
    for(listContext = decompressChannel->pContextList.next; listContext; listContext = listContext->next)
    {
        decompressContext = (RohcDecompressContext *)RohcGetListBody(listContext);
        RohcDestroyDecompressContext(decompressContext);
        RohcRemoveList(&decompressChannel->pContextList, listContext);
        RohcFreeList(listContext);
        
    }

    RohcRemoveList(&gRohcDecompressChannelListHeader, listChannel);

    RohcFreeList(listChannel);
    
}

void RohcResetCompressChannel(RohcCompressChannel *compressChannel)
{
    RohcCompressContext *compressContext = PNULL;
    RohcList *listContext;
    RohcList *listChannel;


    //input parameter check
    GosDebugBegin(compressChannel != PNULL , compressChannel, 0, 0);
    return;
    GosDebugEnd();

    //check magic
    OsaCheck(compressChannel->magic == ROHC_COMPRESS_CHANNEL_MAGIC, compressChannel->magic, 0, 0);

    //check compress channel list
    listChannel = RohcGetListByBody((void *)compressChannel);
    OsaCheck(listChannel != PNULL, listChannel, 0, 0);
    if(RohcCheckListOwner(&gRohcDecompressChannelListHeader, listChannel) == FALSE)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcResetCompressChannel_1, P_ERROR, 0, "RohcResetCompressChannel the channel is not create");
        return;
    }

    //destroy context
    for(listContext = compressChannel->pContextList.next; listContext; listContext = listContext->next)
    {
        compressContext = (RohcCompressContext *)RohcGetListBody(listContext);
        RohcDestroyCompressContext(compressContext);
        RohcRemoveList(&compressChannel->pContextList, listContext);
        RohcFreeList(listContext);
        
    }

    compressChannel->activeCidNum = 0;
    compressChannel->pContextList.next = PNULL;
    
}

void RohcResetDecompressChannel(RohcDecompressChannel *decompressChannel)
{
    RohcDecompressContext *decompressContext = PNULL;
    RohcList *listContext;
    RohcList *listChannel;


    //input parameter check
    GosDebugBegin(decompressChannel != PNULL , decompressChannel, 0, 0);
    return;
    GosDebugEnd();

    //check magic
    OsaCheck(decompressChannel->magic == ROHC_DECOMPRESS_CHANNEL_MAGIC, decompressChannel->magic, 0, 0);

    //check compress channel list
    listChannel = RohcGetListByBody((void *)decompressChannel);
    OsaCheck(listChannel != PNULL, listChannel, 0, 0);
    if(RohcCheckListOwner(&gRohcDecompressChannelListHeader, listChannel) == FALSE)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcResetDecompressChannel_1, P_ERROR, 0, "RohcResetDecompressChannel the channel is not create");
        return;
    }

    //destroy context
    for(listContext = decompressChannel->pContextList.next; listContext; listContext = listContext->next)
    {
        decompressContext = (RohcDecompressContext *)RohcGetListBody(listContext);
        RohcDestroyDecompressContext(decompressContext);
        RohcRemoveList(&decompressChannel->pContextList, listContext);
        RohcFreeList(listContext);
        
    }

    decompressChannel->activeCidNum = 0;
    decompressChannel->pContextList.next = PNULL;

}


