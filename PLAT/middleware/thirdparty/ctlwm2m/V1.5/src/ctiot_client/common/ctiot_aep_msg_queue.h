#ifndef _CTIOT_AEP_MSG_QUEUE_H
#define _CTIOT_AEP_MSG_QUEUE_H

//#include "ctiot_common_type.h"
#include "../../core/ct_liblwm2m.h"


typedef struct _ctiot_funcv1_list_t
{
    struct _ctiot_funcv1_list_t * next;
    uint16_t   msgid;
} ctiot_funcv1_list_t;

typedef struct _ctiot_funcv1_msg_list_head{
    ctiot_funcv1_list_t* head;
    ctiot_funcv1_list_t* tail;
    uint16_t msg_count;
    uint16_t max_msg_num;
    thread_mutex_t mut;
    uint8_t init;
    uint16_t recv_msg_num;
    uint16_t del_msg_count;
}ctiot_funcv1_msg_list_head;

ctiot_funcv1_msg_list_head* ctiot_funcv1_coap_queue_init(uint32_t maxMsgCount);
uint16_t ctiot_funcv1_coap_queue_add(ctiot_funcv1_msg_list_head* list, void* ptr, void* delptr);
ctiot_funcv1_list_t* ctiot_funcv1_coap_queue_find(ctiot_funcv1_msg_list_head* list, uint16_t msgid);
ctiot_funcv1_list_t* ctiot_funcv1_coap_queue_get(ctiot_funcv1_msg_list_head* list);
uint16_t ctiot_funcv1_coap_queue_remove(ctiot_funcv1_msg_list_head* list, uint16_t msgid, void *ptr);
uint16_t ctiot_funcv1_coap_queue_free(ctiot_funcv1_msg_list_head** list);

#endif
