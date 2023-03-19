#include "ctiot_aep_msg_queue.h"
#include "ctiot_lwm2m_sdk.h"

ctiot_funcv1_msg_list_head* ctiot_funcv1_coap_queue_init(uint32_t maxMsgCount)
{
    ctiot_funcv1_msg_list_head* list = (ctiot_funcv1_msg_list_head *)ct_lwm2m_malloc(sizeof(ctiot_funcv1_msg_list_head));
    if(list == NULL)
    {
        return NULL;
    }
    if(thread_mutex_init(&list->mut, NULL))
    {
        ct_lwm2m_free(list);
        return NULL;
    }

    list->max_msg_num = maxMsgCount;
    list->msg_count = 0;
    list->head = NULL;
    list->tail = NULL;
    list->del_msg_count=0;
    list->recv_msg_num=0;

    return list;
}

uint16_t ctiot_funcv1_coap_queue_add(ctiot_funcv1_msg_list_head* list, void* ptr, void *ptrToDelete)
{
    ctiot_funcv1_list_t* node = (ctiot_funcv1_list_t*) ptr;
    if(list == NULL || node == NULL)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_coap_queue_add_1, P_INFO, 2, "list=%d, node=%d", list, node);
        return CTIOT_EB_OTHER;
    }

    int32_t ret = thread_mutex_lock(&list->mut);
    if(ret != 0)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_coap_queue_add_3, P_INFO, 0, "acquire mutex failed");
        return CTIOT_EB_OTHER;
    }
    
    if(list->msg_count >= list->max_msg_num)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_coap_queue_add_2, P_INFO, 1, "list count=%d, drop the first one and add new one", list->msg_count);
        ctiot_funcv1_list_t* pTmp = list->head;
    
        if(pTmp != NULL)
        {
            ctiot_funcv1_list_t ** delnode = (ctiot_funcv1_list_t **) ptrToDelete;
            (*delnode) = pTmp;
            list->head = pTmp->next;
            if(list->head == NULL)
            {
                list->tail = NULL;
            }
            list->msg_count--;
            list->del_msg_count++;
            //ct_lwm2m_free(pTmp);
        }
    }
    node->next = NULL;
    if(list->head == NULL)
    {
        list->head = node;
        list->tail = node;
    }
    else
    {
        list->tail->next = node;
        list->tail = node;
    }
    list->msg_count++;
    ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_coap_queue_add_4, P_INFO, 2, "list count=%d, drop num=%d", list->msg_count, list->del_msg_count);
    ret = thread_mutex_unlock(&list->mut);
    if(ret != 0)
    {
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_coap_queue_add_5, P_INFO, 0, "release mutex failed");
        return CTIOT_EB_OTHER;
    }

    return CTIOT_NB_SUCCESS;
}

ctiot_funcv1_list_t* ctiot_funcv1_coap_queue_find(ctiot_funcv1_msg_list_head* list, uint16_t msgid)
{
    if(list == NULL)
    {
        return NULL;
    }
    ctiot_funcv1_list_t* pTmp = list->head;

    while(pTmp != NULL)
    {
        if(pTmp->msgid == msgid)
        {
            return pTmp;
        }
        pTmp = pTmp->next;
    }

    return NULL;
}

ctiot_funcv1_list_t* ctiot_funcv1_coap_queue_get(ctiot_funcv1_msg_list_head* list)
{
    if(list == NULL)
    {
        return NULL;
    }
    ctiot_funcv1_list_t* pTmp = list->head;

    int32_t ret = thread_mutex_lock(&list->mut);
    if(ret != 0)
    {
        return NULL;
    }
    if(pTmp != NULL)
    {
        list->head = pTmp->next;
        if(list->head == NULL)
        {
            list->tail = NULL;
        }
        list->msg_count--;
        list->del_msg_count++;
        ECOMM_TRACE(UNILOG_CTLWM2M, ctiot_funcv1_coap_queue_get, P_INFO, 2, "list count=%d, del from list count=%d", list->msg_count, list->del_msg_count);
    }
    ret = thread_mutex_unlock(&list->mut);
    if(ret!= 0)
    {
        return NULL;
    }

    return pTmp;
}

uint16_t ctiot_funcv1_coap_queue_remove(ctiot_funcv1_msg_list_head* list, uint16_t msgid, void *ptr)
{
    ctiot_funcv1_list_t ** node = (ctiot_funcv1_list_t **) ptr;
    if(list == NULL || list->head==NULL)
    {
        return CTIOT_EB_OTHER;
    }
    ctiot_funcv1_list_t* prev;
    ctiot_funcv1_list_t* next;
    ctiot_funcv1_list_t* curr;
    int32_t ret = thread_mutex_lock(&list->mut);
    if(ret)
    {
        return CTIOT_EB_OTHER;
    }
    
    if(list->head->msgid == msgid)
    {
        
        curr = list->head;
        list->head = list->head->next;
        (*node) = curr;
        //ct_lwm2m_free(curr);
        if(list->head == NULL)
        {
            list->tail = NULL;
        }
        list->msg_count--;
        list->del_msg_count++;
    }
    else
    {
        curr = list->head->next;
        prev = list->head;
        next = curr->next;
        while(curr != NULL)
        {
            if(curr->msgid == msgid)
            {
                prev->next = next;
                if(curr == list->tail)
                {
                    list->tail = prev;
                }
                (*node) = curr;
                list->msg_count--;
                list->del_msg_count++;
                break;
            }
            prev = curr;
            curr = next;
            if(next != NULL)
            {
                next = next->next;
            }
        }
    }
    ret = thread_mutex_unlock(&list->mut);
    if(ret)
    {
        return CTIOT_EB_OTHER;
    }

    return CTIOT_NB_SUCCESS;
}

uint16_t ctiot_funcv1_coap_queue_free(ctiot_funcv1_msg_list_head** list)
{
    if(list == NULL)
    {
        return CTIOT_NB_SUCCESS;
    }
    ctiot_funcv1_list_t* pTmp = (*list)->head;

    int32_t ret = thread_mutex_lock(&(*list)->mut);
    if(ret)
    {
        return CTIOT_EB_OTHER;
    }
    while(pTmp != NULL)
    {
        (*list)->head = (*list)->head->next;
        ct_lwm2m_free(pTmp);
        pTmp = (*list)->head;
    }
    ret = thread_mutex_unlock(&(*list)->mut);
    if(ret)
    {
        return CTIOT_EB_OTHER;
    }
    ret = thread_mutex_destory(&(*list)->mut);
    if(ret)
    {
        return CTIOT_EB_OTHER;
    }

    ct_lwm2m_free((*list));
    (*list) = NULL;

    return CTIOT_NB_SUCCESS;
}


