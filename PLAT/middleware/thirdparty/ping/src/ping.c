/**
 * @file
 * Ping sender module
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 */

/** 
 * This is an example of a "ping" sender (with raw API and socket API).
 * It can be used as a start point to maintain opened a network connection, or
 * like a network "watchdog" for your device.
 *
 */

#include "lwip/opt.h"

#if LWIP_RAW /* don't build if not configured for use in lwipopts.h */

#include "ping.h"

#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/icmp6.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/inet_chksum.h"
#include "lwip/prot/ip4.h"
#include "lwip/prot/ip6.h"


#if PING_USE_SOCKETS
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "posix/netdb.h"
#include <string.h>
#endif /* PING_USE_SOCKETS */
#include "osasys.h"

#include "psdial.h"
#include "netmgr.h"
#include "debug_log.h"
#if defined CHIP_EC616 || defined CHIP_EC616_Z0
#include "slpman_ec616.h"
#elif defined CHIP_EC616S
#include "slpman_ec616s.h"
#endif

#define PING_THREAD_STACKSIZE 2048
#define PING_THREAD_PRIO 16

/**
 * PING_DEBUG: Enable debugging for PING.
 */
#ifndef PING_DEBUG
#define PING_DEBUG     LWIP_DBG_ON
#endif

/** ping receive timeout - in milliseconds */
#ifndef PING_RCV_TIMEO
#define PING_RCV_TIMEO 20000
#endif

/** ping delay - in milliseconds */
#ifndef PING_DELAY
#define PING_DELAY     1000
#endif

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

/** ping result action - no default action */
#ifndef PING_RESULT
#define PING_RESULT(ping_ok)
#endif

/** ping max request times */
#ifndef PING_MAX_SEND_COUNT
#define PING_MAX_SEND_COUNT 0xFFFE
#endif

/*
 * PING task input parameters
*/
typedef enum NmPingDestTypeEnum_Tag
{
    // 0:ip address 1:url address
    NM_PING_IP_ADDR,
    NM_PING_URL_ADDR
}NmPingDestTypeEnum;

typedef struct ping_param_tag
{
    /*
     * Ping request count value;
     * if not set (0), use default value 15, if set to 0xFF, just means ping continuously
    */
    uint8_t     count;
    uint8_t     ping_target_type;   //NmPingDestTypeEnum
    uint16_t    payload_len;
    uint32_t    ping_timeout;       //ping time out value in ms
    uint16_t    req_handle;
    uint8_t     rai_Flag;
    uint8_t     rsvd;
    char        ping_url_target[256];
    ip_addr_t   ping_target;
}ping_param_t;

/* ping variables */
static ping_param_t ping_arg;
static u16_t    ping_seq_num = 0;
static BOOL     ping_is_ongoing = FALSE;
//static u32_t    ping_time = 0;
static sys_thread_t ping_task = NULL;
static u8_t     ping_terminal_flag = 0;
//static uint16_t ping_sent_count = 0;
//static uint16_t ping_receive_success_count = 0;
//static uint32_t ping_min_rtt = PING_RCV_TIMEO;
//static uint32_t ping_max_rtt = 0;
static uint32_t ping_total_time = 0;    //ping resp wait total time

//#if !PING_USE_SOCKETS
#if 0
static struct raw_pcb *ping_pcb;
#endif /* PING_USE_SOCKETS */

/** Prepare a echo ICMP request */
static void
ping_prepare_echo( void *iecho, u16_t len, BOOL bIpv6)
{
    size_t i;

    //ipv4
    if (bIpv6 == FALSE) 
    {
        struct icmp_echo_hdr *iecho4 = (struct icmp_echo_hdr *)iecho;
        size_t data_len = len - sizeof(struct icmp_echo_hdr);

        OsaCheck(len > sizeof(struct icmp_echo_hdr), len, sizeof(struct icmp_echo_hdr), 0);

        ICMPH_TYPE_SET(iecho4, ICMP_ECHO);
        ICMPH_CODE_SET(iecho4, 0);
        iecho4->chksum = 0;
        iecho4->id     = PING_ID;
        iecho4->seqno  = lwip_htons(++ping_seq_num);

        /* fill the additional data buffer with some data */
        for (i = 0; i < data_len; i++) 
        {
            ((char*)iecho4)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
        }

        iecho4->chksum = inet_chksum(iecho4, len);
    } 
    else    //ipv6 
    {
        struct icmp6_echo_hdr *iecho6 = (struct icmp6_echo_hdr *)iecho;
        size_t data_len = len - sizeof(struct icmp6_echo_hdr);

        OsaCheck(len > sizeof(struct icmp6_echo_hdr), len, sizeof(struct icmp6_echo_hdr), 0);

        iecho6->type = ICMP6_TYPE_EREQ;
        iecho6->code = 0;
        iecho6->chksum = 0;
        iecho6->id = PING_ID;
        iecho6->seqno = lwip_htons(++ping_seq_num);

        /* fill the additional data buffer with some data */
        for(i = 0; i < data_len; i++) 
        {
            ((char*)iecho6)[sizeof(struct icmp6_echo_hdr) + i] = (char)i;
        }

        //checksum will be done by lwip send/sendto API

    }
}

//#if PING_USE_SOCKETS
#if 1
/* Ping using the socket ip */
static err_t
ping_send(int s, const ip_addr_t *addr, uint16_t payload_len, uint8_t rai_flag)
{
    int err;
    void *iecho;
    struct sockaddr_storage to;
    size_t ping_size = 0;

    if (IP_IS_V4(addr)) 
    {
        if (payload_len > 0) 
        {
            ping_size = sizeof(struct icmp_echo_hdr) + payload_len;
        } 
        else 
        {
            ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
        }
    }
    else 
    {
        if (payload_len > 0) 
        {
            ping_size = sizeof(struct icmp6_echo_hdr) + payload_len;
        } 
        else 
        {
            ping_size = sizeof(struct icmp6_echo_hdr) + PING_DATA_SIZE;
        }    
    }
    
    //  LWIP_ASSERT("ping_size is too big", ping_size <= 0xffff);
    if (ping_size > 2048) 
    {
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_send_1, P_ERROR, 1, "ping_size is too big: %d > 2048", ping_size);
        return ERR_VAL;
    }

    iecho = (void *)mem_malloc((mem_size_t)ping_size);
    if (!iecho) 
    {
        return ERR_MEM;
    }

    if (IP_IS_V4(addr)) 
    {
        ping_prepare_echo(iecho, (u16_t)ping_size, FALSE);
    }
    else 
    {
        ping_prepare_echo(iecho, (u16_t)ping_size, TRUE);
    }

//#if LWIP_IPV4
    if (IP_IS_V4(addr)) 
    {
        struct sockaddr_in *to4 = (struct sockaddr_in*)&to;
        to4->sin_len    = sizeof(struct sockaddr_in);
        to4->sin_family = AF_INET;
        inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(addr));
    }
//#endif /* LWIP_IPV4 */

//#if LWIP_IPV6
    if(IP_IS_V6(addr)) 
    {
        struct sockaddr_in6 *to6 = (struct sockaddr_in6*)&to;
        to6->sin6_len    = sizeof(struct sockaddr_in6);
        to6->sin6_family = AF_INET6;
        inet6_addr_from_ip6addr(&to6->sin6_addr, ip_2_ip6(addr));
    }
//#endif /* LWIP_IPV6 */

    //if enable RAI flag, UE will expect only one ping response packet after ping request
    if(rai_flag == 0)
    {
        err = ps_sendto(s, iecho, ping_size, 0, (struct sockaddr*)&to, sizeof(to), 0, 0);
    }
    else
    {
        err = ps_sendto(s, iecho, ping_size, 0, (struct sockaddr*)&to, sizeof(to), 2, 0);
    }

    mem_free(iecho);

    return (err < 0 ? ERR_VAL : ERR_OK);
}

#if 0
static void
ping_recv(int s, uint16_t payload_len, bool bIpv6)
{
    char *buf = NULL;
    int len = 0, buf_len = 0, recv_len = 0;
    int basic_len = 0;  //basic ping header len
    struct sockaddr_storage from;
    int fromlen = sizeof(from);

    memset(nm_ping_result, 0x00, sizeof(struct NmAtiPingResultIndTag));

    if (bIpv6 == FALSE) 
    {
        if(payload_len > 0) 
        {
            buf_len = sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr)+payload_len;
        }
        else 
        {
            buf_len = sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr)+PING_DATA_SIZE;
        }    
        basic_len = sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr);
    }
    else 
    {
        if(payload_len > 0) 
        {
            buf_len = sizeof(struct ip6_hdr)+sizeof(struct icmp6_echo_hdr)+payload_len;
        }
        else 
        {
            buf_len = sizeof(struct ip6_hdr)+sizeof(struct icmp6_echo_hdr)+PING_DATA_SIZE;
        }
        basic_len = sizeof(struct ip6_hdr)+sizeof(struct icmp6_echo_hdr);    
    }

    buf = mem_malloc(buf_len);

    if (!buf) 
    {
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_1_1, P_ERROR, 0, "malloc reve buff fail");
        OsaDebugBegin(FALSE, buf_len, 0, 0);
        OsaDebugEnd();
        return;
    }

    while ((len = lwip_recvfrom(s, buf+recv_len, buf_len-recv_len, 0, (struct sockaddr*)&from, (socklen_t*)&fromlen)) > 0) 
    {
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_2_1, P_SIG, 1, "Ping recv len: %u", len);

        recv_len += len;
        OsaCheck(recv_len <= buf_len, recv_len, len, buf_len);

        if (recv_len >= basic_len) 
        {
            ip_addr_t fromaddr;
            memset(&fromaddr, 0, sizeof(fromaddr));

            /* set SRC (from) ADDR info, ipv4 or ipv6 */
            //#if LWIP_IPV4
            if(from.ss_family == AF_INET) 
            {
                struct sockaddr_in *from4 = (struct sockaddr_in*)&from;
                inet_addr_to_ip4addr(ip_2_ip4(&fromaddr), &from4->sin_addr);
                IP_SET_TYPE(&fromaddr, IPADDR_TYPE_V4);
            }
            //#endif /* LWIP_IPV4 */

            //#if LWIP_IPV6
            if(from.ss_family == AF_INET6) 
            {
                struct sockaddr_in6 *from6 = (struct sockaddr_in6*)&from;
                inet6_addr_to_ip6addr(ip_2_ip6(&fromaddr), &from6->sin6_addr);
                IP_SET_TYPE(&fromaddr, IPADDR_TYPE_V6);
            }
            //#endif /* LWIP_IPV6 */

            ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_3_1, P_SIG, 1, "Ping recv ms %u", sys_now() - ping_time);

            /* todo: support ICMP6 echo */
            //#if LWIP_IPV4
            if (IP_IS_V4_VAL(fromaddr)) 
            {
                struct ip_hdr *iphdr        = (struct ip_hdr *)buf;
                struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
                uint16_t ping_rtt = 0;
                
                if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(ping_seq_num))) 
                {
                    /* do some ping result processing */

                    ping_rtt = sys_now() - ping_time;
                    ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_4_1, P_SIG, 1, "Ping OK, %u ms", ping_rtt);

                    ping_receive_success_count++;
                    ping_total_time += ping_rtt;

                    if (ping_rtt < ping_min_rtt) //set the MIN RTT value
                    {
                        ping_min_rtt = ping_rtt;
                    }

                    if (ping_rtt > ping_max_rtt) //set MAX RTT value
                    {
                        ping_max_rtt = ping_rtt;
                    }

                    PING_RESULT((ICMPH_TYPE(iecho) == ICMP_ER));
                    mem_free(buf);
                    return;
                } 
                else 
                {
                    ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_5_1, P_WARNING, 1, "ping: drop, sequence number %u", iecho->seqno);
                }
            }
            else //IPV6
            {
                struct icmp6_echo_hdr *iecho6 = (struct icmp6_echo_hdr *)(buf + IP6_HLEN);
                uint16_t ping_rtt;

                //iecho6 = (struct icmp6_echo_hdr *)(buf + IP6_HLEN);
                if ((iecho6->id == PING_ID) && (iecho6->seqno == lwip_htons(ping_seq_num))) 
                {
                    /* do some ping result processing */
                    ping_rtt = sys_now() - ping_time;

                    ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_6_1, P_SIG, 1, "Ping OK, %u ms", ping_rtt);

                    ping_receive_success_count++;
                    ping_total_time += ping_rtt;

                    if(ping_rtt < ping_min_rtt) 
                    {
                        ping_min_rtt = ping_rtt;
                    }

                    if(ping_rtt > ping_max_rtt) 
                    {
                        ping_max_rtt = ping_rtt;
                    }

                    PING_RESULT((iecho6->type == ICMP6_TYPE_EREP));
                    mem_free(buf);
                    return;
                } 
                else 
                {
                    ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_7_1, P_WARNING, 1, "ping: drop, sequence number %u", iecho6->seqno);
                }
            }   //end of IPv6
        } //end of "(len >= basic_len)"
        
        //fromlen = sizeof(from);
    }

    mem_free(buf);

    if (len <= 0) 
    {
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_8_1, P_WARNING, 2, "Ping recv failed: %u, in %u ms, timeout", len, sys_now()- ping_time);
    }

    /* do some ping result processing */
    PING_RESULT(0);
}
#endif

/******************************************************************************
 * ping_recv
 * Description: Wait and recv ping reply
 * input: int s         //socket
 *        uint16_t payload_len  //ping reply payload size
 *        bool bIpv6    //whether ipv6 socket
 *        u32_t ping_sent_time  //ping sent time (in ms)
 * output: int
 * Comment: 
 * return
 * 1> > 0, ping reply succ, and return the RTT delay for this ping reply
 * 2> -1, time out;
 * 3> -2, other error
******************************************************************************/
#define NM_PING_RECV_RET_TIMEOUT    -1
#define NM_PING_RECV_RET_ERROR      -2
static int ping_recv(int s, uint16_t payload_len, bool bIpv6, u32_t ping_sent_time, u8_t *ttl, u32_t ping_timeout)
{
    char *buf = NULL;
    int len = 0, buf_len = 0, recv_len = 0;
    int basic_len = 0;  //basic ping header len
    struct sockaddr_storage from;
    int fromlen = sizeof(from);
    uint32_t    ping_rtt = 0;
    uint32_t    cur_time = 0;
    *ttl = 0;

    if (bIpv6 == FALSE) 
    {
        if(payload_len > 0) 
        {
            buf_len = sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr)+payload_len;
        }
        else 
        {
            buf_len = sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr)+PING_DATA_SIZE;
        }    
        basic_len = sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr);
    }
    else 
    {
        if(payload_len > 0) 
        {
            buf_len = sizeof(struct ip6_hdr)+sizeof(struct icmp6_echo_hdr)+payload_len;
        }
        else 
        {
            buf_len = sizeof(struct ip6_hdr)+sizeof(struct icmp6_echo_hdr)+PING_DATA_SIZE;
        }
        basic_len = sizeof(struct ip6_hdr)+sizeof(struct icmp6_echo_hdr);    
    }

    buf = mem_malloc(buf_len);

    if (!buf) 
    {
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_1, P_ERROR, 0, "malloc reve buff fail");
        OsaDebugBegin(FALSE, buf_len, 0, 0);
        OsaDebugEnd();

        return NM_PING_RECV_RET_ERROR;
    }

    while ((len = lwip_recvfrom(s, buf+recv_len, buf_len-recv_len, 0, (struct sockaddr*)&from, (socklen_t*)&fromlen)) > 0) 
    {
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_2, P_SIG, 1, "Ping recv len: %u", len);

        recv_len += len;
        OsaCheck(recv_len <= buf_len, recv_len, len, buf_len);

        /* do some ping result processing */
        cur_time = sys_now();
        if (cur_time >= ping_sent_time)
        {
            ping_rtt = cur_time - ping_sent_time;
        }
        else
        {
            ping_rtt = 0xFFFFFFFF - ping_sent_time + cur_time;
            if (ping_rtt > 0xFFFF)  //should not extend 65535 ms
            {
                OsaDebugBegin(FALSE, ping_rtt, cur_time, ping_sent_time);
                OsaDebugEnd();
                ping_rtt = 0xFFFF;
            }
        }        

        if (recv_len >= basic_len)
        {
            
            ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_3, P_SIG, 1, "Ping recv ms %u", ping_rtt);

            /* todo: support ICMP6 echo */
            //#if LWIP_IPV4
            if (bIpv6 == FALSE) 
            {
                struct ip_hdr *iphdr        = (struct ip_hdr *)buf;
                struct icmp_echo_hdr *iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));

                if(iecho->type == ICMP_ER)
                {
                
                    if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(ping_seq_num))) 
                    {
                        // ping succ
                        *ttl = IPH_TTL(iphdr);

                        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_4, P_SIG, 2, "Ping OK, %u ms, ttl %u", ping_rtt, *ttl);

                        mem_free(buf);
                        return ping_rtt;
                    } 
                    else 
                    {
                        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_5, P_WARNING, 1, "ping: drop, sequence number %u", iecho->seqno);
                        /*
                        * Need to still wait for next ping reply,
                        * Recv from header
                        */
                        recv_len = 0;
                    }
                }
                else
                {
                    recv_len = 0;
                    ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_9, P_WARNING, 0, "ping: drop, packet is not ipv4 ping reply");
                }
            }
            else //IPV6
            {
                struct ip6_hdr *ip6hdr = (struct ip6_hdr *)buf;
                struct icmp6_echo_hdr *iecho6 = (struct icmp6_echo_hdr *)(buf + IP6_HLEN);
                //uint16_t ping_rtt;

                //iecho6 = (struct icmp6_echo_hdr *)(buf + IP6_HLEN);

                if(iecho6 ->type == ICMP6_TYPE_EREP)
                {
                    if ((iecho6->id == PING_ID) && (iecho6->seqno == lwip_htons(ping_seq_num))) 
                    {
                        /* ping succ */
                        *ttl = IP6H_HOPLIM(ip6hdr);
                        
                        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_6, P_SIG, 1, "Ping OK, %u ms, ttl &u", ping_rtt, *ttl);

                        mem_free(buf);
                        return ping_rtt;
                    } 
                    else 
                    {
                        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_7, P_WARNING, 1, "ping: drop, sequence number %u", iecho6->seqno);
                        /*
                        * Need to still wait for next ping reply,
                        * Recv from header
                        */
                        recv_len = 0;   
                    }
                }
                else
                {
                    recv_len = 0;
                    ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_10, P_WARNING, 0, "ping: drop, packet is not ipv6 ping reply");
                }
            }   //end of IPv6   
        } //end of "(len >= basic_len)"
        else
        {
               recv_len = 0;
               ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_11, P_WARNING, 0, "ping: drop, packet is not valid");            
        }
    
        //retry again, calculate timeout again

        if(ping_rtt < ping_timeout)
        {
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
            int timeout = ping_timeout - ping_rtt;
#else
            struct timeval timeout;
            timeout.tv_sec = (ping_timeout - ping_rtt)/1000;
            timeout.tv_usec = ((ping_timeout - ping_rtt)%1000)*1000;
#endif

            if(lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0)
            {
                ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_12, P_WARNING, 0, "ping:socket set timeout fail");
                mem_free(buf);
                return NM_PING_RECV_RET_ERROR;
            }

        }
        else
        {
            ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_13, P_WARNING, 0, "ping:has timeout");
            mem_free(buf);
            return NM_PING_RECV_RET_TIMEOUT; //time out
        }

        //else, still wait for whole reply pkg
    }

    mem_free(buf);

    cur_time = sys_now();
    if (cur_time >= ping_sent_time)
    {
        ping_rtt = cur_time - ping_sent_time;
    }
    else
    {
        ping_rtt = 0xFFFFFFFF - ping_sent_time + cur_time;
        if (ping_rtt > 0xFFFF)  //should not extend 65535 ms
        {
            OsaDebugBegin(FALSE, ping_rtt, cur_time, ping_sent_time);
            OsaDebugEnd();
            ping_rtt = 0xFFFF;
        }
    }
    ECOMM_TRACE(UNILOG_TCPIP_PING, ping_recv_8, P_WARNING, 2, "Ping recv failed: %u, in %u ms, timeout", len, ping_rtt);

    return NM_PING_RECV_RET_TIMEOUT; //time out
}



static void 
ping_thread(void *arg)
{
    int ret;
    int ping_count = 0;
    int ping_retry_count = 0;
    int s;
    slpManRet_t pmuRet = RET_UNKNOW_FALSE;
    UINT8 pingPmuHdr = 0;
    int one_ping_rtt = 0;
    err_t send_result = ERR_OK;
    u8_t ttl = 0;
    int sockErr = 0;

    NmAtiPingResultInd nm_ping_result;    //whole ping result
    NmAtiPingResultInd nm_one_ping_result;//each ping result

    struct addrinfo hints, *target_address = PNULL;

    struct ping_param_tag *ping_param = PNULL;

    ping_param = (struct ping_param_tag*)arg;

    memset(&nm_ping_result, 0, sizeof(NmAtiPingResultInd));
    memset(&nm_one_ping_result, 0x00, sizeof(NmAtiPingResultInd));
//    nm_ping_result.minRtt = PING_RCV_TIMEO;

    if (!ping_param) 
    {
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_thread_1, P_ERROR, 0, "Ping thread start error, as ping dest is NULL");
        ping_is_ongoing = FALSE;
        ping_task = NULL;
        ping_terminal_flag = 0;

        nm_ping_result.result = NM_PING_PARAM_ERROR;
        NetMgrSendPingResultInd(&nm_ping_result, BROADCAST_IND_HANDLER);

        osThreadExit();    
    }

    ping_retry_count = ping_param->count > 0 ? ping_param->count : PING_TRY_COUNT;

    //init ping payload len for the result
    nm_ping_result.payloadLen = ping_param->payload_len;
    nm_one_ping_result.payloadLen = ping_param->payload_len;

    /*
     * start ping, ping task don't allow to enter HIB/Sleep2 state before ping terminated
    */
    pmuRet = slpManApplyPlatVoteHandle("ping", &pingPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    pmuRet = slpManPlatVoteDisableSleep(pingPmuHdr, SLP_SLP2_STATE);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    //get ping target address if target is url type
    if (ping_param->ping_target_type == NM_PING_URL_ADDR) 
    {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        //    hints.ai_socktype = SOCK_DGRAM;
        //    hints.ai_protocol = IPPROTO_UDP;
        if (getaddrinfo(ping_param->ping_url_target, NULL, &hints, &target_address) != 0) 
        {
            //      OsaPrintf(P_ERROR, "Ping thread start error, can not get ip from url");
            ECOMM_TRACE(UNILOG_TCPIP_PING, ping_thread_2, P_ERROR, 0, "Ping thread start error, can not get ip from url");
            ping_is_ongoing = FALSE;
            ping_task = NULL;
            ping_terminal_flag = 0;

            nm_ping_result.result =  NM_PING_DNS_ERROR;
            NetMgrSendPingResultInd(&nm_ping_result, ping_param->req_handle);

            /* before ping task exit, should allow to enter HIB/Sleep2, and release hander */
            pmuRet = slpManPlatVoteEnableSleep(pingPmuHdr, SLP_SLP2_STATE);
            OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

            pmuRet = slpManGivebackPlatVoteHandle(pingPmuHdr);
            OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

            osThreadExit();             
        }
        else 
        {
            if (target_address->ai_family == AF_INET) 
            {
                ECOMM_TRACE(UNILOG_TCPIP_PING, ping_thread_11, P_SIG, 0, "Ping thread resolve url success, it is ipv4 type");
                struct sockaddr_in *to = (struct sockaddr_in *)target_address->ai_addr;
                IP_SET_TYPE(&ping_param->ping_target, IPADDR_TYPE_V4);
                inet_addr_to_ip4addr(ip_2_ip4(&ping_param->ping_target), &to->sin_addr);
            }
            else if (target_address->ai_family == AF_INET6) 
            {
                ECOMM_TRACE(UNILOG_TCPIP_PING, ping_thread_12, P_SIG, 0, "Ping thread resolve url success, it is ipv6 type");
                struct sockaddr_in6 *to = (struct sockaddr_in6 *)target_address->ai_addr;
                IP_SET_TYPE(&ping_param->ping_target, IPADDR_TYPE_V6);
                inet6_addr_to_ip6addr(ip_2_ip6(&ping_param->ping_target),&to->sin6_addr);
            } 
            else 
            {
                ECOMM_TRACE(UNILOG_TCPIP_PING, ping_thread_10, P_ERROR, 0, "Ping thread start error, get ip from url is invalid");
                ping_is_ongoing = FALSE;
                ping_task = NULL;
                ping_terminal_flag = 0;

                nm_ping_result.result = NM_PING_DNS_ERROR;
                NetMgrSendPingResultInd(&nm_ping_result, ping_param->req_handle);

                freeaddrinfo(target_address);

                /* before ping task exit, should allow to enter HIB/Sleep2, and release hander */
                pmuRet = slpManPlatVoteEnableSleep(pingPmuHdr, SLP_SLP2_STATE);
                OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

                pmuRet = slpManGivebackPlatVoteHandle(pingPmuHdr);
                OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);
    
                osThreadExit();      

            }
            freeaddrinfo(target_address);
        }
    }

#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
    int timeout = ping_param->ping_timeout;
#else
    struct timeval timeout;
    timeout.tv_sec = ping_param->ping_timeout/1000;
    timeout.tv_usec = (ping_param->ping_timeout%1000)*1000;
#endif

#if LWIP_IPV6
    if (IP_IS_V4(&ping_param->ping_target)) 
    {
        s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
    } 
    else 
    {
        s = lwip_socket(AF_INET6, SOCK_RAW, IP6_NEXTH_ICMP6);
    }
#else
    s = lwip_socket(AF_INET,  SOCK_RAW, IP_PROTO_ICMP);
#endif

    if (s < 0) 
    {
        ping_is_ongoing = FALSE;
        ping_task = NULL;
        ping_terminal_flag = 0;

        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_thread_4, P_ERROR, 0, "Ping socket create error");

        nm_ping_result.result =  NM_PING_SOCKET_ERROR;
        NetMgrSendPingResultInd(&nm_ping_result, ping_param->req_handle);

        /* before ping task exit, should allow to enter HIB/Sleep2, and release hander */
        pmuRet = slpManPlatVoteEnableSleep(pingPmuHdr, SLP_SLP2_STATE);
        OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

        pmuRet = slpManGivebackPlatVoteHandle(pingPmuHdr);
        OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

        osThreadExit();
    }

    ret = lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    if (ret != 0) 
    {
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_thread_5, P_ERROR, 0, "setting receive timeout failed");
        nm_ping_result.result =  0x03;
        NetMgrSendPingResultInd(&nm_ping_result, ping_param->req_handle);

        lwip_close(s);

        /* before ping task exit, should allow to enter HIB/Sleep2, and release hander */
        pmuRet = slpManPlatVoteEnableSleep(pingPmuHdr, SLP_SLP2_STATE);
        OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

        pmuRet = slpManGivebackPlatVoteHandle(pingPmuHdr);
        OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

        osThreadExit();    
    }

    LWIP_UNUSED_ARG(ret);

    /*
     * set some ping result parameters
    */
    if (IP_IS_V4(&ping_param->ping_target)) 
    {
        snprintf(nm_ping_result.pingDst, 
                 NM_ATI_PING_DEST_IP_STR_LEN, 
                 "%d.%d.%d.%d",
                 ip4_addr1_16(&(ping_param->ping_target.u_addr.ip4)),
                 ip4_addr2_16(&(ping_param->ping_target.u_addr.ip4)),
                 ip4_addr3_16(&(ping_param->ping_target.u_addr.ip4)),
                 ip4_addr4_16(&(ping_param->ping_target.u_addr.ip4)));
        
        snprintf(nm_one_ping_result.pingDst, 
                 NM_ATI_PING_DEST_IP_STR_LEN, 
                 "%d.%d.%d.%d",
                 ip4_addr1_16(&(ping_param->ping_target.u_addr.ip4)),
                 ip4_addr2_16(&(ping_param->ping_target.u_addr.ip4)),
                 ip4_addr3_16(&(ping_param->ping_target.u_addr.ip4)),
                 ip4_addr4_16(&(ping_param->ping_target.u_addr.ip4)));
    }
    else 
    {
        snprintf(nm_ping_result.pingDst, 
                 NM_ATI_PING_DEST_IP_STR_LEN,
                 "%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F,
                 IP6_ADDR_BLOCK1(ip_2_ip6(&ping_param->ping_target)),
                 IP6_ADDR_BLOCK2(ip_2_ip6(&ping_param->ping_target)),
                 IP6_ADDR_BLOCK3(ip_2_ip6(&ping_param->ping_target)),
                 IP6_ADDR_BLOCK4(ip_2_ip6(&ping_param->ping_target)),
                 IP6_ADDR_BLOCK5(ip_2_ip6(&ping_param->ping_target)),
                 IP6_ADDR_BLOCK6(ip_2_ip6(&ping_param->ping_target)),
                 IP6_ADDR_BLOCK7(ip_2_ip6(&ping_param->ping_target)),
                 IP6_ADDR_BLOCK8(ip_2_ip6(&ping_param->ping_target)));

        snprintf(nm_one_ping_result.pingDst, 
                 NM_ATI_PING_DEST_IP_STR_LEN,
                 "%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F,
                 IP6_ADDR_BLOCK1(ip_2_ip6(&ping_param->ping_target)),
                 IP6_ADDR_BLOCK2(ip_2_ip6(&ping_param->ping_target)),
                 IP6_ADDR_BLOCK3(ip_2_ip6(&ping_param->ping_target)),
                 IP6_ADDR_BLOCK4(ip_2_ip6(&ping_param->ping_target)),
                 IP6_ADDR_BLOCK5(ip_2_ip6(&ping_param->ping_target)),
                 IP6_ADDR_BLOCK6(ip_2_ip6(&ping_param->ping_target)),
                 IP6_ADDR_BLOCK7(ip_2_ip6(&ping_param->ping_target)),
                 IP6_ADDR_BLOCK8(ip_2_ip6(&ping_param->ping_target)));
    }

    while ((ping_retry_count == PING_ALWAYS || ping_count < ping_retry_count) && (ping_terminal_flag == 0)) 
    {

        //check pdn status first
        if (!psDailBeCfun1State()) 
        {
            ECOMM_TRACE(UNILOG_TCPIP_PING, ping_thread_6, P_ERROR, 0, "Not cfun1 state , ping will terminal");
            break;
        }

        //if the ping request is the last one, maybe need send the request with rai flag
        if((ping_retry_count != PING_ALWAYS) && (ping_count == (ping_retry_count - 1)))
        {
            send_result = ping_send(s, &ping_param->ping_target, ping_param->payload_len, ping_param->rai_Flag);
        }
        else
        {
            send_result = ping_send(s, &ping_param->ping_target, ping_param->payload_len, 0);
        }

        if (send_result == ERR_OK) 
        {
            //ping_sent_count++; 
            nm_ping_result.requestNum++;
            
            ECOMM_TRACE(UNILOG_TCPIP_PING, ping_thread_7, P_SIG, 0, "ping: send");
            
            if (IP_IS_V4(&ping_param->ping_target)) 
            {
                one_ping_rtt = ping_recv(s, ping_param->payload_len, FALSE, sys_now(), &ttl, ping_param->ping_timeout);
            }
            else 
            {
                one_ping_rtt = ping_recv(s, ping_param->payload_len, TRUE, sys_now(), &ttl, ping_param->ping_timeout);
            }

            if (one_ping_rtt >= 0)   // ping succ
            {
                nm_one_ping_result.result           = NM_PING_RET_ONE_SUCC;
                nm_one_ping_result.packetLossRate   = 0;
                nm_one_ping_result.requestNum       = 1;
                nm_one_ping_result.responseNum      = 1;
                nm_one_ping_result.minRtt           = one_ping_rtt;
                nm_one_ping_result.maxRtt           = one_ping_rtt;
                nm_one_ping_result.avgRtt           = one_ping_rtt;
                nm_one_ping_result.ttl              = ttl;

                NetMgrSendPingResultInd(&nm_one_ping_result, ping_param->req_handle);

                /*
                 * update whole ping result
                */
                ping_total_time += one_ping_rtt;
                nm_ping_result.responseNum++;

                if(nm_ping_result.minRtt == 0)
                {
                    nm_ping_result.minRtt = one_ping_rtt;
                }
                else
                {
                    if (one_ping_rtt < nm_ping_result.minRtt)
                    {
                        nm_ping_result.minRtt = one_ping_rtt;
                    }                    
                }

                if (one_ping_rtt > nm_ping_result.maxRtt)
                {
                    nm_ping_result.maxRtt = one_ping_rtt;
                }
            }
            else // fail this time
            {
                nm_one_ping_result.result           = NM_PING_RET_ONE_FAIL;
                nm_one_ping_result.packetLossRate   = 100;
                nm_one_ping_result.requestNum       = 1;
                nm_one_ping_result.responseNum      = 0;
                nm_one_ping_result.ttl              = 0;

                if (one_ping_rtt == NM_PING_RECV_RET_TIMEOUT)
                {
                    nm_one_ping_result.minRtt       = ping_param->ping_timeout;
                    nm_one_ping_result.maxRtt       = ping_param->ping_timeout;
                    nm_one_ping_result.avgRtt       = ping_param->ping_timeout;
                }
                else    //other error
                {
                    nm_one_ping_result.minRtt       = 1;
                    nm_one_ping_result.maxRtt       = 1;
                    nm_one_ping_result.avgRtt       = 1;
                }

                NetMgrSendPingResultInd(&nm_one_ping_result, ping_param->req_handle);

                sockErr = sock_get_errno(s);
                if(socket_error_is_fatal(s))
                {
                    ECOMM_TRACE(UNILOG_TCPIP_PING, ping_thread_sock_fatal_error_1, P_WARNING, 0, "ping: socket fatal error %u", sockErr);
                    break;
                }
            }

            if (nm_ping_result.requestNum > PING_MAX_SEND_COUNT) 
            {
                break;
            }

        } 
        else
        {
            nm_one_ping_result.result           = NM_PING_RET_ONE_SEND_FAIL;
            nm_one_ping_result.packetLossRate   = 100;
            nm_one_ping_result.requestNum       = 1;
            nm_one_ping_result.responseNum      = 0;
            nm_one_ping_result.minRtt       = 1;
            nm_one_ping_result.maxRtt       = 1;
            nm_one_ping_result.avgRtt       = 1;
            nm_one_ping_result.ttl          = 0;
            NetMgrSendPingResultInd(&nm_one_ping_result, ping_param->req_handle);

            sockErr = sock_get_errno(s);
            if(socket_error_is_fatal(s))
            {
                ECOMM_TRACE(UNILOG_TCPIP_PING, ping_thread_sock_fatal_error_2, P_WARNING, 0, "ping: socket fatal error %u", sockErr);
                break;
            }

            sys_msleep(4000);

            ECOMM_TRACE(UNILOG_TCPIP_PING, ping_thread_8, P_WARNING, 0, "ping: send -error");
        }
        
        ping_count++;
//        sys_msleep(PING_DELAY);
    }

    ECOMM_TRACE(UNILOG_TCPIP_PING, ping_thread_9, P_SIG, 0, "ping terminal");

    /*
     * Ping done
     */
    if (nm_ping_result.responseNum > 0) 
    {
        nm_ping_result.result = NM_PING_RET_DONE;

        OsaCheck(nm_ping_result.requestNum >= nm_ping_result.responseNum, 
                 nm_ping_result.requestNum, nm_ping_result.responseNum, 0);

        nm_ping_result.packetLossRate = (((nm_ping_result.requestNum - nm_ping_result.responseNum)*100) / nm_ping_result.requestNum);
        nm_ping_result.avgRtt = (ping_total_time / nm_ping_result.responseNum);
    } 
    else 
    { 
        //there is not any success response
        nm_ping_result.result = NM_PING_ALL_FAIL;

        if (nm_ping_result.requestNum > 0)
        {
            nm_ping_result.packetLossRate = 100;
        }
        else
        {
            nm_ping_result.packetLossRate = 0;
        }

        nm_ping_result.maxRtt = 0;
        nm_ping_result.minRtt = 0;
        nm_ping_result.avgRtt = 0;
    }

    NetMgrSendPingResultInd(&nm_ping_result, ping_param->req_handle);

    ping_is_ongoing = FALSE;
    ping_terminal_flag = 0;
    ping_task = NULL;
    ping_total_time = 0;

    lwip_close(s);

    /* before ping task exit, should allow to enter HIB/Sleep2, and release hander */
    pmuRet = slpManPlatVoteEnableSleep(pingPmuHdr, SLP_SLP2_STATE);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    pmuRet = slpManGivebackPlatVoteHandle(pingPmuHdr);
    OsaCheck(pmuRet == RET_TRUE, pmuRet, 0, 0);

    osThreadExit();  
}

#else /* PING_USE_SOCKETS */

/* Ping using the raw ip */
static u8_t
ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
  struct icmp_echo_hdr *iecho;
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(addr);
  LWIP_ASSERT("p != NULL", p != NULL);

  if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))) &&
      pbuf_header(p, -PBUF_IP_HLEN) == 0) {
    iecho = (struct icmp_echo_hdr *)p->payload;

    if ((iecho->id == PING_ID) && (iecho->seqno == lwip_htons(ping_seq_num))) {
      LWIP_DEBUGF( PING_DEBUG, ("ping: recv "));
      ip_addr_debug_print(PING_DEBUG, addr);
      LWIP_DEBUGF( PING_DEBUG, (" %"U32_F" ms\n", (sys_now()-ping_time)));

      /* do some ping result processing */
      PING_RESULT(1);
      pbuf_free(p);
      return 1; /* eat the packet */
    }
    /* not eaten, restore original packet */
    pbuf_header(p, PBUF_IP_HLEN);
  }

  return 0; /* don't eat the packet */
}

static void
ping_send(struct raw_pcb *raw, ip_addr_t *addr)
{
  struct pbuf *p;
  struct icmp_echo_hdr *iecho;
  size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;

  LWIP_DEBUGF( PING_DEBUG, ("ping: send "));
  ip_addr_debug_print(PING_DEBUG, addr);
  LWIP_DEBUGF( PING_DEBUG, ("\n"));
  LWIP_ASSERT("ping_size <= 0xffff", ping_size <= 0xffff);

  p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
  if (!p) {
    return;
  }
  if ((p->len == p->tot_len) && (p->next == NULL)) {
    iecho = (struct icmp_echo_hdr *)p->payload;

    ping_prepare_echo(iecho, (u16_t)ping_size);

    raw_sendto(raw, p, addr);
    ping_time = sys_now();
  }
  pbuf_free(p);
}

static void
ping_timeout(void *arg)
{
  struct raw_pcb *pcb = (struct raw_pcb*)arg;
  ip_addr_t ping_target;

  LWIP_ASSERT("ping_timeout: no pcb given!", pcb != NULL);

  ip_addr_copy_from_ip4(ping_target, PING_TARGET);
  ping_send(pcb, &ping_target);

  sys_timeout(PING_DELAY, ping_timeout, pcb);
}

static void
ping_raw_init(void)
{
  ping_pcb = raw_new(IP_PROTO_ICMP);
  LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL);

  raw_recv(ping_pcb, ping_recv, NULL);
  raw_bind(ping_pcb, IP_ADDR_ANY);
  sys_timeout(PING_DELAY, ping_timeout, ping_pcb);
}

void
ping_send_now(void)
{
  ip_addr_t ping_target;
  LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL);
  ip_addr_copy_from_ip4(ping_target, PING_TARGET);
  ping_send(ping_pcb, &ping_target);
}

#endif /* PING_USE_SOCKETS */


/******************************************************************************
 * ping_init
 * Description: init/create ping task
 * input: const ip_addr_t *ping_addr; // target address, only support IP address
 *        uint8_t count;   //ping count number, if set to 255, just ping continuesly
 *        uint16_t payload_len  // ping payload len, if not set (0), just set to 32
 *        uint32_t timeout //ping reply timeout (ms)
 * output: BOOL 
 * Comment: 
******************************************************************************/
BOOL ping_init(const ip_addr_t *ping_addr, uint8_t count, uint16_t payloadLen, uint32_t timeout, BOOL raiFlag, uint16_t req_handle)
{
    if (ping_is_ongoing) 
    {
        //OsaPrintf(P_ERROR, "ERROR, ping is ongoing, please try it later");
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_init_1, P_ERROR, 0, "ERROR, ping is ongoing, please try it later");
        return FALSE;
    }

    /*
     * check the input
    */
    if (ping_addr == NULL)
    {
        //OsaPrintf(P_ERROR, "ERROR, ping dest addr is NULL");
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_init_2, P_ERROR, 0, "ERROR, ping dest addr is NULL");
        return FALSE;
    }

    /* if set it in "ping" task, maybe modify ping_arg for mutiple ping request */
    ping_is_ongoing = TRUE;
    
    ping_arg.count              = count;
    ping_arg.ping_target_type   = NM_PING_IP_ADDR;

    //ping payload setting
    if (payloadLen == 0)
    {
        ping_arg.payload_len = PING_DATA_SIZE;
    }
    else if (payloadLen > PING_PAYLOAD_MAX_LEN)
    {
        // print a warning here
        //OsaPrintf(P_WARNING, "Ping length is too long: %d, cut to : %d", payload_len, PING_PAYLOAD_MAX_LEN);
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_init_3, P_WARNING, 2, "Ping length is too long: %d, cut to : %d",
                   payloadLen, PING_PAYLOAD_MAX_LEN);
        
        ping_arg.payload_len = PING_PAYLOAD_MAX_LEN;
    }
    else
    {
        ping_arg.payload_len = payloadLen;
    }

    //ping timeout setting
    if (timeout > 0)
    {
        ping_arg.ping_timeout = timeout;
    }
    else
    {
        ping_arg.ping_timeout = PING_RCV_TIMEO;
    }

    //ping rai flag
    if(raiFlag > 0)
    {
        ping_arg.rai_Flag = 1;
    }
    else
    {
        ping_arg.rai_Flag = 0;
    }

    //init reqhandle
    ping_arg.req_handle = req_handle;
        
    //ping target setting
    ip_addr_copy(ping_arg.ping_target, *ping_addr);

    //LWIP_DEBUGF( PING_DEBUG, ("ping is starting arg address:0x%x \n",(void*)&ping_arg));
#if PING_USE_SOCKETS
    ping_task = sys_thread_new("ping_thread", ping_thread, (void*)&ping_arg, PING_THREAD_STACKSIZE, PING_THREAD_PRIO);

    if (ping_task == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_init_4, P_WARNING, 0, "Ping task create failed");
        ping_is_ongoing = FALSE;
        return FALSE;
    }
#else /* PING_USE_SOCKETS */
    ping_raw_init();
#endif /* PING_USE_SOCKETS */

    return TRUE;
}



/******************************************************************************
 * ping_url_init
 * Description: init/create ping task
 * input: const char* ping_url_addr; // target url address, the url length is not longer than 127 bytes
 *        uint8_t count;    //ping count number, if set to 255, just ping continuesly
 *        uint16_t payloadLen  // ping payload len, if not set (0), just set to 32
 *        uint32_t timeout
 * output: BOOL
 * Comment: 
******************************************************************************/
BOOL ping_url_init(const char* ping_url_addr, uint8_t count, uint16_t payloadLen, uint32_t timeout, BOOL raiFlag, uint16_t req_handle)
{
    /* check the input */
    if(!ping_url_addr || strlen(ping_url_addr) > 255 || strlen(ping_url_addr) == 0) 
    {
        //OsaPrintf(P_ERROR, "ERROR, ping url address is invalid");
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_url_init_1, P_ERROR, 0, 
                   "ERROR, ping url address is invalid");
        return FALSE;
    }

    if (ping_is_ongoing) 
    {
        //OsaPrintf(P_ERROR, "ERROR, ping is ongoing, please try it later");
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_url_init_2, P_ERROR, 0, 
                   "ERROR, ping is ongoing, please try it later");
        
        return FALSE;
    }

    /* if set it in "ping" task, maybe modify ping_arg for mutiple ping request */
    ping_is_ongoing = TRUE;

    ping_arg.ping_target_type   = NM_PING_URL_ADDR;
    ping_arg.count              = count;

    //ping payload setting
    if (payloadLen == 0)
    {
        ping_arg.payload_len = PING_DATA_SIZE;
    }
    else if (payloadLen > PING_PAYLOAD_MAX_LEN)
    {
        // print a warning here
        ECOMM_TRACE(UNILOG_TCPIP_PING, ping_url_addr_3, P_WARNING, 2, "Ping length is too long: %d, cut to : %d",
                   payloadLen, PING_PAYLOAD_MAX_LEN);

        ping_arg.payload_len = PING_PAYLOAD_MAX_LEN;
    }
    else
    {
        ping_arg.payload_len = payloadLen;
    }

    //ping timeout setting
    if (timeout > 0)
    {
        ping_arg.ping_timeout = timeout;
    }
    else
    {
        ping_arg.ping_timeout = PING_RCV_TIMEO;
    }

    //ping rai flag
    if(raiFlag > 0)
    {
        ping_arg.rai_Flag = 1;
    }
    else
    {
        ping_arg.rai_Flag = 0;
    }

    //ping target setting
    strcpy((char *)ping_arg.ping_url_target, ping_url_addr);

    //init reqhandle
    ping_arg.req_handle = req_handle;


    //LWIP_DEBUGF( PING_DEBUG, ("ping is starting arg address:0x%x \n",(void*)&ping_arg));
#if PING_USE_SOCKETS
    ping_task = sys_thread_new("ping_thread", ping_thread, (void*)&ping_arg, PING_THREAD_STACKSIZE, PING_THREAD_PRIO);

    if (ping_task == NULL)
    {
        ping_is_ongoing = FALSE;
        return FALSE;
    }

#else /* PING_USE_SOCKETS */
    ping_raw_init();
#endif /* PING_USE_SOCKETS */

    return TRUE;
}


void ping_terminat(void) {
    if(ping_task) {
        ping_terminal_flag = 1;
    }
    return;
}


int ping_get_addr_info_async(char *name, uint16_t src_hdr, uint8_t disable_cache_flag) {
  struct addrinfo hints;

  if(!name) {
//    LWIP_DEBUGF( PING_DEBUG|LWIP_DBG_LEVEL_SERIOUS, ("ping_get_addr_info get dns fail,invalid arg \n"));
    ECOMM_TRACE(UNILOG_TCPIP_PING, ping_get_addr_info_async_1, P_ERROR, 0, 
                   "ping_get_addr_info get dns fail,invalid arg");    
    return -1;
  }

  //check pdn status first
  if(!psDailBeCfun1State()) {
//      LWIP_DEBUGF( PING_DEBUG|LWIP_DBG_LEVEL_SERIOUS, ("ping_get_addr_info get dns fail: %s, because ps not cfun1 state \n",name));
      ECOMM_TRACE(UNILOG_TCPIP_PING, ping_get_addr_info_async_2, P_ERROR, 0, 
                   "ping_get_addr_info get dns fail,because ps not cfun1 state");      
      return -1;
  }

    
  //ipv4 domain only
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
//    hints.ai_socktype = SOCK_DGRAM;
//    hints.ai_protocol = IPPROTO_UDP;
    if (getaddrinfo_async(name, &hints, src_hdr, disable_cache_flag) != 0) {
//      LWIP_DEBUGF( PING_DEBUG|LWIP_DBG_LEVEL_SERIOUS, ("ping_get_addr_info get dns fail in ipv4 domain %s \n",name));
      ECOMM_TRACE(UNILOG_TCPIP_PING, ping_get_addr_info_async_3, P_ERROR, 0, 
                     "ping_get_addr_info get dns fail");
      return -1;
  }
  
  return 0;
}



#endif /* LWIP_RAW */

