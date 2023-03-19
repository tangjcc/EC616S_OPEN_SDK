
#ifndef COAP_ADAPTER_H
#define COAP_ADAPTER_H
 
#include "cmsis_os2.h"
#include "FreeRTOS.h" 
#include "semphr.h"




typedef struct coap_mutex
{
    SemaphoreHandle_t sem;
} coap_mutex;

void coap_mutex_init(coap_mutex* mutex);

int coap_mutex_lock(coap_mutex* mutex);

int coap_mutex_unlock(coap_mutex* mutex);

#endif

