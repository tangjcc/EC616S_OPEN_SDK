/*******************************************************************************
 *
 * Copyright (c) 2013, 2014, 2015 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *******************************************************************************/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
//#include <errno.h>

#include "ct_liblwm2m.h"

#ifdef PLATFORM_LINUX
//使用 ifconf结构体和ioctl函数时需要用到该头文�?
#include <net/if.h>
#include <sys/ioctl.h>

//使用ifaddrs结构体时需要用到该头文�?
#include <ifaddrs.h>
#endif

#ifdef PLATFORM_MCU_ECOM
#include "osasys.h"

#endif
/********************************************************
*      log
*
*********************************************************/
#if defined(PLATFORM_LINUX)
uint8_t buffer[512];
void lwm2m_printf(const char * format, ...)
{
#if defined(WITH_CTIOT_LOGS)
    va_list ap;

    va_start(ap, format);
    vprintf(format, ap);
   // vsprintf(buffer, format, ap);
    //puts(buffer);
    va_end(ap);
#endif
}
#endif

#ifndef CTIOT_HEAP_TRACE
/********************************************************
*      heap api
*
*********************************************************/
void *ct_lwm2m_malloc(size_t s)
{
    void* ptr=NULL;

    ptr= malloc(s);
    return ptr;
}

void ct_lwm2m_free(void * p)
{
    if( p == NULL )
    {
        lwm2m_printf("free pointer is NULL\r\n");
        return;
    }

    free(p);
    p = NULL;
}

/********************************************************
*      str lib api
*
*********************************************************/
char *ct_lwm2m_strdup(const char * str)
{
    char *dstr = NULL;
    size_t len;

    if(str == NULL)
        return NULL;

    len = strlen(str);
    dstr = (char *)ct_lwm2m_malloc(len+1);

    if( dstr == NULL )
        return NULL;

    memcpy(dstr,str, len);
    dstr[len] = '\0';

    return dstr;

}
#endif

int ct_lwm2m_strncmp(const char * s1,const char * s2, size_t n)
{
    return strncmp(s1, s2, n);
}


/********************************************************
*      time api
*
*********************************************************/

time_t ct_lwm2m_gettime(void)
{
#if defined(PLATFORM_LINUX)

    struct timeval tv;

    if (0 != gettimeofday(&tv, NULL))
    {
        return -1;
    }

    return tv.tv_sec;

#elif defined(PLATFORM_MCU_LITEOS)

    time_t seconds;

    seconds = atiny_gettime_ms()/1000;
    return seconds;

#elif defined(PLATFORM_MCU_ECOM)

    return xTaskGetTickCount()/osKernelGetTickFreq();
#endif
}

#ifdef PLATFORM_MCU_LITEOS
//add by tony 05/15
struct timeval {
  long    tv_sec;         /* seconds */
  long    tv_usec;        /* and microseconds */
};
#endif

int lwm2m_gettimeofday(void *tv, void *tz)
{
#if defined(PLATFORM_LINUX)

    return gettimeofday((struct timeval *)tv, tz);

#elif defined(PLATFORM_MCU_LITEOS)
    uint64_t seconds;
    struct timeval *p;

    p = (struct timeval *)tv;
    seconds = atiny_gettime_ms();
    p->tv_sec = seconds/1000;
    p->tv_usec =    (seconds%1000)*1000;

    return 0;
#endif
}


#if defined(PLATFORM_MCU_LITEOS)
void usleep(uint32_t usec)
{
    uint32_t i=1000;

    if( i<1000 )
    {
        while(i--);
    }
    else
    {
        atiny_delayms(usec/1000);
    }
}
#elif defined(PLATFORM_MCU_ECOM)
void usleep(uint32_t usec)
{
    uint32_t i=1000;
    uint32_t ms,tick;

    if( i<1000 )
    {
        while(i--);
    }
    else
    {
        ms = usec/1000;
        tick = ms*osKernelGetTickFreq()/1000;
        vTaskDelay(tick);
    }
}

#endif


/********************************************************
*     rtos api
*
*********************************************************/


int thread_create(thread_handle_t *thread, const thread_attr_t *attr, void(*start_routine)(void *), void* arg, lwm2m_task_type type)
{

#if defined(PLATFORM_LINUX)

    return pthread_create(thread, attr,start_routine, arg);
#elif defined(PLATFORM_MCU_LITEOS)
    //return task_create(start_routine, arg, 10, NULL, 256, "ucosii_thread");

      UINT32 uwRet = LOS_OK;
    TSK_INIT_PARAM_S task_init_param;

    task_init_param.usTaskPrio = 5;
    task_init_param.pcName = "lwm2m_coap_task";
    task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)start_routine;
    task_init_param.uwArg = (uint32_t)arg;
    task_init_param.uwStackSize = 0x1000;

    if( attr == NULL )
    {
        uwRet = LOS_TaskCreate(thread, &task_init_param);

    }
    else
    {
        uwRet = LOS_TaskCreate(thread, attr);

    }

    if(LOS_OK != uwRet)
    {
        return uwRet;
    }
    return uwRet;
#elif defined(PLATFORM_MCU_ECOM)
    thread_attr_t task_init_param;

    memset(&task_init_param, 0, sizeof(thread_attr_t));
    task_init_param.priority = osPriorityBelowNormal7;
    switch (type)
    {
        case LWM2M_SEND_TASK:
            task_init_param.name = "CTSEND";
#ifdef WITH_MBEDTLS
            task_init_param.stack_size = 5120;
#else
            task_init_param.stack_size = 3072;
#endif
            break;
        case LWM2M_RECV_TASK:
            task_init_param.name = "CTRECV";
#ifdef WITH_MBEDTLS
            task_init_param.stack_size = 4608;
#else
            task_init_param.stack_size = 2048;
#endif
            break;
        case LWM2M_FOTA_DOWNLOAD_TASK:
            task_init_param.name = "CTOTAD";
#ifdef WITH_FOTA_RESUME
            task_init_param.stack_size = 3072;
#else
            task_init_param.stack_size = 2048;
#endif
            break;
        case LWM2M_TEMP_INIT_TASK:
            task_init_param.name = "CTINIT";
#ifdef WITH_MBEDTLS
            task_init_param.stack_size = 4608;
#else
            task_init_param.stack_size = 2048;
#endif
            break;
        default:
            break;
    }

    if( attr == NULL )
    {
        *thread = osThreadNew((osThreadFunc_t)start_routine, arg, &task_init_param);
    }
    else
    {
        *thread = osThreadNew((osThreadFunc_t)start_routine, arg, attr);
    }

    if( *thread != NULL )
        return 0;
    else
        return -1;
#endif

}

int thread_exit(void *retval)
{
#if defined(PLATFORM_LINUX)

    pthread_exit(NULL);
    return 0;
#elif defined(PLATFORM_MCU_LITEOS)

//  return = OSTaskDel(thread_id);

    return LOS_TaskDelete(NULL);
#elif defined(PLATFORM_MCU_ECOM)

    vTaskDelete(NULL);
    return 0;
#endif

}

int thread_cancel(thread_handle_t thread_id)
{
#if defined(PLATFORM_LINUX)

    pthread_cancel(thread_id);
    return 0;
#elif defined(PLATFORM_MCU_LITEOS)

//  return = OSTaskDel(thread_id);

    return LOS_TaskDelete(thread_id);
#elif defined(PLATFORM_MCU_ECOM)

    vTaskDelete(thread_id);
    return 0;
#endif

}


int thread_join(thread_handle_t thread_id ,void **retval)
{
#if defined(PLATFORM_LINUX)
    pthread_join(thread_id, retval);

    return 0;
#elif defined(PLATFORM_MCU_LITEOS) || defined(PLATFORM_MCU_ECOM)
    return 0;
#endif
}


 int thread_mutex_init(thread_mutex_t *mutex, const char *name)
{
#if defined(PLATFORM_LINUX)

    return pthread_mutex_init(mutex,NULL);

#elif defined(PLATFORM_MCU_LITEOS)

    return atiny_task_mutex_create(mutex);

#elif defined(PLATFORM_MCU_ECOM)
    *mutex = osMutexNew(NULL);
    return 0;
#endif
}

int thread_mutex_lock(thread_mutex_t *mutex)
{
#if defined(PLATFORM_LINUX)

    return pthread_mutex_lock(mutex);

#elif defined(PLATFORM_MCU_LITEOS)

    return atiny_task_mutex_lock(mutex);

#elif defined(PLATFORM_MCU_ECOM)

    return osMutexAcquire(*mutex, osWaitForever);
#endif
}

int thread_mutex_unlock(thread_mutex_t *mutex)
{
#if defined(PLATFORM_LINUX)

    return pthread_mutex_unlock(mutex);

#elif defined(PLATFORM_MCU_LITEOS)

    return atiny_task_mutex_unlock(mutex);

#elif defined(PLATFORM_MCU_ECOM)

    return osMutexRelease(*mutex);
#endif
}

int thread_mutex_destory(thread_mutex_t *mutex)
{
#if defined(PLATFORM_LINUX)

    return pthread_mutex_destroy(mutex);

#elif defined(PLATFORM_MCU_LITEOS)

    return atiny_task_mutex_delete(mutex);

#elif defined(PLATFORM_MCU_ECOM)

    return osMutexDelete(*mutex);
#endif
}

uint32_t ct_conn_radiosignalstrength()
{
    uint32_t ret = 99;
    INT8 snr = 0;
    INT8 rsrp = 0;
    UINT8 csq = 0;
    
    if(appGetSignalInfoSync(&csq, &snr, &rsrp) == CMS_RET_SUCC)
    {
        ret = csq;
    }

    ECOMM_TRACE(UNILOG_CTLWM2M, ct_conn_radiosignalstrength_1, P_INFO, 1, " Signal Strength = %d", ret);

    return ret;

}

uint32_t ct_conn_cellid()
{
    uint32_t cellID = 6888;
    
    CmiDevGetExtStatusCnf   ueExtStatusInfo;
    memset(&ueExtStatusInfo, 0, sizeof(CmiDevGetExtStatusCnf));

    if(appGetUeExtStatusInfoSync(UE_EXT_STATUS_ERRC, &ueExtStatusInfo) == CMS_RET_SUCC)
    {
        cellID = ueExtStatusInfo.rrcStatus.cellId;
    }
    
    ECOMM_TRACE(UNILOG_CTLWM2M, ct_conn_cellid_1, P_INFO, 1, "Cell ID = %d", cellID);
    return cellID;
}

void ct_conn_localIP(char* localIP)
{
    if(localIP==NULL)
    {
        return;
    }

    NmAtiSyncRet netInfo;
    NBStatus_t nbstatus=appGetNetInfoSync(0,&netInfo);
    if ( NM_NET_TYPE_IPV4 == netInfo.body.netInfoRet.netifInfo.ipType)
    {
        uint8_t* ips=(uint8_t *)&netInfo.body.netInfoRet.netifInfo.ipv4Info.ipv4Addr.addr;
        sprintf(localIP,"%u.%u.%u.%u",ips[0],ips[1],ips[2],ips[3]);
        ECOMM_STRING(UNILOG_CTLWM2M, ct_conn_localIP_1, P_INFO, "local IP:%s", (const uint8_t *)localIP);
    }
}


