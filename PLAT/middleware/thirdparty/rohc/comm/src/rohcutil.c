/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcutil.c
 Description:    - rohc util api
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/


/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/
#include "rohcutil.h"

RohcList * RohcNewList(UINT32 privateLen)
{
    RohcList *list = PNULL;

    //malloc new list
    list = (RohcList *)malloc(sizeof(RohcList) + privateLen);

    if(list == PNULL)
    {
        ECOMM_TRACE(UNILOG_ROHC, RohcNewList_1, P_WARNING, 1, "RohcNewList malloc %u fail", privateLen);    
    }
    else
    {
        //init list
        memset(list , 0, sizeof(RohcList) + privateLen);
    }

    return list;
}

void RohcFreeList(RohcList *list)
{
    //input parameter check
    GosDebugBegin(list != PNULL, list, 0, 0);  
    return; 
    GosDebugEnd();

    if(list)
    {
        free(list);
    }

    return;
}

void RohcInsertList(RohcList *listHeader, RohcList *newList)
{
    
    //input parameter check
    GosDebugBegin(listHeader != PNULL && newList != PNULL, listHeader, newList, 0);  
    return; 
    GosDebugEnd();

    if(listHeader->next)
    {
        listHeader->next->pre = newList;
    }
    newList->next = listHeader->next;    
    listHeader->next = newList;
    newList->pre = listHeader;

    return;
}

void RohcRemoveList(RohcList *header, RohcList *list)
{
    //input parameter check
    GosDebugBegin(list != PNULL && header!= PNULL, list, header, 0);  
    return; 
    GosDebugEnd();

    GosDebugBegin(list != header, list, header, 0);  
    return; 
    GosDebugEnd();

    if(list->next)
    {
        list->next->pre = list->pre;
    }
    if(list->pre)
    {
        list->pre->next = list->next;
    }

    return;
}

void *RohcGetListBody(RohcList *list)
{
    //input parameter check
    GosDebugBegin(list != PNULL, list, 0, 0);  
    return PNULL; 
    GosDebugEnd();

    return (void *)list->private;
}

RohcList *RohcGetListByBody(void *body)
{
    UINT32 bodyOffset = 0;

    //input parameter check
    GosDebugBegin(body != PNULL, body, 0, 0);  
    return PNULL; 
    GosDebugEnd();

    bodyOffset = (UINT32)(((RohcList *)0)->private);

    return (RohcList *)((UINT32)body - bodyOffset);
}

BOOL RohcCheckListOwner(RohcList *header, RohcList *list)
{
    RohcList *listTmp;

    //input parameter check
    GosDebugBegin(header != PNULL && list != PNULL, header, list, 0);  
    return FALSE; 
    GosDebugEnd();

    for(listTmp = header->next; listTmp; listTmp = listTmp->next)
    {
        if(listTmp == list)
        {
            return TRUE;
        }
    }

    return FALSE;
}


UINT32 RohcGetCurrentSysTime(void)
{
    UINT32 currentMs = 0;
    
    currentMs = osKernelGetTickCount()/portTICK_PERIOD_MS;

    return currentMs;
}

INT32 RohcGetRandomNum(void)
{
    return rand();
}

//RFC3095 5.9.1 c(x) = 1 + X + X2 + X8
UINT8 RohcGernCrc8(UINT8 *data, UINT16 dataLen)
{
    UINT8 i;
    UINT8 result = 0xFF;

    //input parameter check
    GosDebugBegin(data != PNULL && dataLen > 0, data, dataLen, 0); 
    return result; 
    GosDebugEnd();

    while((dataLen --) > 0)
    {
        result ^= *data++;
        for(i = 0; i < 8; i++)
        {
            if(result & 0x01)
            {
                result = (result >> 1) ^ ROHC_CRC8_POLYNOMIAL;
            }
            else
            {
                result = (result >> 1);
            }
        }
    }

    return result;
    
}

