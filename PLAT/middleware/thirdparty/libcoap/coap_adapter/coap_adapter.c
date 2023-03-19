#include "coap_adapter.h"



int coap_send_packet_count = 0;
int coap_send_packet_count_recv = 0;
int coap_send_packet_count_send = 0;


coap_mutex coap_mutex1;
coap_mutex coap_mutex2;
coap_mutex coap_mutex3;

void coap_mutex_init(coap_mutex* mutex)
{
    mutex->sem = xSemaphoreCreateMutex();
    //printf("sem=0x%x...!!!!!!!...1...\r\n",mutex->sem);
}

int coap_mutex_lock(coap_mutex* mutex)
{
    return xSemaphoreTake(mutex->sem, portMAX_DELAY);
}

int coap_mutex_unlock(coap_mutex* mutex)
{
    return xSemaphoreGive(mutex->sem);
}

int coap_mutex_delete(coap_mutex* mutex)
{
    vSemaphoreDelete(mutex->sem);

    return 0;
}

void coap_mutex_init_all(void)
{
    coap_mutex_init(&coap_mutex1);
    coap_mutex_init(&coap_mutex2);
}





