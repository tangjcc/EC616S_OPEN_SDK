/**
 * @file
 * User Datagram Protocol module\n
 * The code for the User Datagram Protocol UDP & UDPLite (RFC 3828).\n
 * See also @ref udp_raw
 * 
 * @defgroup udp_raw UDP
 * @ingroup callbackstyle_api
 * User Datagram Protocol module\n
 * @see @ref raw_api and @ref netconn
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/* @todo Check the use of '(struct udp_pcb).chksum_len_rx'!
 */

#include "lwip/opt.h"

#if LWIP_UDP /* don't build if not configured for use in lwipopts.h */

#include "lwip/udp.h"
#include "lwip/def.h"
#include "lwip/memp.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip_addr.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/netif.h"
#include "lwip/icmp.h"
#include "lwip/icmp6.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/dhcp.h"

#include <string.h>



#ifndef UDP_LOCAL_PORT_RANGE_START
/* From http://www.iana.org/assignments/port-numbers:
   "The Dynamic and/or Private Ports are those from 49152 through 65535" */
#define UDP_LOCAL_PORT_RANGE_START  0xc000
#define UDP_LOCAL_PORT_RANGE_END    0xffff
#define UDP_ENSURE_LOCAL_PORT_RANGE(port) ((u16_t)(((port) & ~UDP_LOCAL_PORT_RANGE_START) + UDP_LOCAL_PORT_RANGE_START))
#endif

/* last local UDP port */
static u16_t udp_port = UDP_LOCAL_PORT_RANGE_START;

/* The list of UDP PCBs */
/* exported in udp.h (was static) */
struct udp_pcb *udp_pcbs;

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE

// the global hib/sleep2 mode udp pcb point
struct udp_pcb *udp_hib_sleep2_pcbs = NULL;

//the current hib/sleep2 mode udp pcb number
u8_t udp_hib_sleep2_pcb_num = 0;
#endif

// socket sequence state handler pcb
#if ENABLE_PSIF
static struct udp_pcb *gsequence_handler_pcb = NULL;
#endif

#if ENABLE_PSIF

struct udp_pcb *get_global_sequence_handler_pcb(void)
{
    return gsequence_handler_pcb;
}

void set_global_sequence_handler_pcb(struct udp_pcb *handler_pcb)
{
    gsequence_handler_pcb = handler_pcb;
}

u8_t IsAnyActiveUdpConn(void)
{
    if(udp_pcbs != NULL) {
        return 1;
    }else {
        return 0;
    }
}

//get the udp_pcb list
struct udp_pcb *get_global_udp_pcb_list(void)
{
    return udp_pcbs;
}

#if LWIP_SUSPEND_UP_DATA_ENABLE

u8_t is_any_udp_pcb_pending_ul_data(void)
{
    struct udp_pcb * udp_pcb_tmp;

    for(udp_pcb_tmp = udp_pcbs; udp_pcb_tmp; udp_pcb_tmp = udp_pcb_tmp->next) {
        if(udp_pcb_tmp->pending_msg) {
            return 1;
        }
    }

    return 0;
}

#endif

#endif

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
// udp pcd will send the DL packet to conn->recvmbox directly
u8_t is_any_udp_pcb_pending_data(void)
{
    return 0;
}

#endif

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE

u8_t udp_get_curr_hib_sleep2_pcb_num(void)
{
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_get_curr_hib_sleep2_pcb_num_1, P_INFO, 1, "udp_get_curr_hib_sleep2_pcb_num %u", udp_hib_sleep2_pcb_num);
    return udp_hib_sleep2_pcb_num;
}

void udp_add_curr_hib_sleep2_pcb_num(void)
{  
    udp_hib_sleep2_pcb_num++;
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_add_curr_hib_sleep2_pcb_num_1, P_INFO, 1, "udp_add_hib_sleep2_context_pcb %u", udp_hib_sleep2_pcb_num);
}

void udp_del_curr_hib_sleep2_pcb_num(void)
{
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_del_curr_hib_sleep2_pcb_num_1, P_INFO, 1, "udp_del_curr_hib_sleep2_pcb_num %u", udp_hib_sleep2_pcb_num);
    udp_hib_sleep2_pcb_num--;
}

void udp_enable_hib_sleep2_mode(struct udp_pcb *pcb, u8_t state)
{
        pcb->pcb_hib_sleep2_mode_flag = state;
        udp_hib_sleep2_pcb_num ++;
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_enable_hib_sleep2_mode_1, P_INFO, 1, "udp_enable_hib_sleep2_mode %u", udp_hib_sleep2_pcb_num);
}

void udp_disable_hib_sleep2_mode(struct udp_pcb *pcb, u8_t state)
{
        pcb->pcb_hib_sleep2_mode_flag = state;
        udp_hib_sleep2_pcb_num --;
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_disable_hib_sleep2_mode_1, P_INFO, 1, "udp_disable_hib_sleep2_mode", udp_hib_sleep2_pcb_num);
}

void udp_set_hib_sleep2_state(struct udp_pcb *pcb, u8_t state)
{
        pcb->pcb_hib_sleep2_mode_flag = state;
}


struct udp_pcb *udp_get_hib_sleep2_pcb_list(void)
{
    return udp_hib_sleep2_pcbs;
}

void udp_set_hib_sleep2_pcb_list(struct udp_pcb * new_pcb)
{
    udp_hib_sleep2_pcbs = new_pcb;
}


void udp_add_hib_sleep2_context_pcb(PsifHibUdpContext *udpContext)
{
    struct udp_pcb *new_pcb;

    new_pcb = udp_new();

    if(new_pcb)
    {
        memcpy(&new_pcb->remote_ip, &udpContext->remoteAddr, sizeof(new_pcb->remote_ip));
        new_pcb->remote_port = udpContext->remotePort;
        memcpy(&new_pcb->local_ip, &udpContext->localAddr, sizeof(new_pcb->local_ip));
        new_pcb->local_port = udpContext->localPort;        
        new_pcb->sockid = (int)udpContext->sockid;

        if(ip_addr_isany_val(udpContext->localAddr))
        {
            udp_enable_hib_sleep2_mode(new_pcb, PCB_HIB_ENABLE_ACTIVE);
        }
        else
        {
            udp_enable_hib_sleep2_mode(new_pcb, PCB_HIB_ENABLE_DEACTIVE);
        }
        udp_hib_sleep2_pcbs = new_pcb;
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_add_hib_sleep2_context_pcb_1, P_INFO, 1, "udp_add_hib_sleep2_context_pcb alloc new udp_pcb success, sockid %d", new_pcb->sockid);        
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_add_hib_sleep2_context_pcb_2, P_ERROR, 0, "udp_add_hib_sleep2_context_pcb alloc new udp_pcb fail");
    }

}

void udp_check_hib_sleep2_pcb_active(ip_addr_t *ue_addr)
{
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_check_hib_sleep2_pcb_active_1, P_INFO, 0, "udp_check_hib_sleep2_pcb_active");
    
    if(udp_hib_sleep2_pcbs && udp_hib_sleep2_pcbs->pcb_hib_sleep2_mode_flag == PCB_HIB_ENABLE_DEACTIVE) {
        if(ip_addr_cmp(ue_addr, &udp_hib_sleep2_pcbs->local_ip) || ip_addr_isany_val(udp_hib_sleep2_pcbs->local_ip)) {
            udp_set_hib_sleep2_state(udp_hib_sleep2_pcbs, PCB_HIB_ENABLE_ACTIVE);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_check_hib_sleep2_pcb_active_2, P_INFO, 1, "udp_check_hib_sleep2_pcb_active change pcb 0x%x state to enable_active state", udp_hib_sleep2_pcbs);
        }
    }
}

void udp_check_hib_sleep2_pcb_deactive(ip_addr_t *ue_addr)
{
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_check_hib_sleep2_pcb_deactive_1, P_INFO, 0, "udp_check_hib_sleep2_pcb_deactive");
    
    if(udp_hib_sleep2_pcbs && udp_hib_sleep2_pcbs->pcb_hib_sleep2_mode_flag == PCB_HIB_ENABLE_ACTIVE) {
        if(ip_addr_cmp(ue_addr, &udp_hib_sleep2_pcbs->local_ip) || ip_addr_isany_val(udp_hib_sleep2_pcbs->local_ip)) {
            udp_set_hib_sleep2_state(udp_hib_sleep2_pcbs, PCB_HIB_ENABLE_DEACTIVE);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_check_hib_sleep2_pcb_deactive_2, P_INFO, 1, "udp_check_hib_sleep2_pcb_deactive change pcb 0x%x state to enable_deactive state", udp_hib_sleep2_pcbs);
        }
    }
}

int udp_get_sock_info(int fd, ip_addr_t *local_ip, ip_addr_t *remote_ip, u16_t *local_port, u16_t *remote_port)
{
    if(fd < 0) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_get_sock_info_1, P_ERROR, 0, "udp_get_sock_info invalid fd");
        return -1;
    }

    if(udp_hib_sleep2_pcbs && udp_hib_sleep2_pcbs->pcb_hib_sleep2_mode_flag == PCB_HIB_ENABLE_ACTIVE) {
        if(udp_hib_sleep2_pcbs->sockid == fd) {
            memcpy(local_ip, &udp_hib_sleep2_pcbs->local_ip, sizeof(ip_addr_t));
            memcpy(remote_ip, &udp_hib_sleep2_pcbs->remote_ip, sizeof(ip_addr_t));
            *local_port = udp_hib_sleep2_pcbs->local_port;
            *remote_port = udp_hib_sleep2_pcbs->remote_port;
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_get_sock_info_2, P_INFO, 0, "udp_get_sock_info find adpat udp hib pcb");
            return 0;
        }
    }

    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_get_sock_info_3, P_INFO, 0, "udp_get_sock_info can not find adpat udp hib pcb");
    return -1;
}

int udp_get_hib_sock_id(void)
{
    if(udp_hib_sleep2_pcbs && udp_hib_sleep2_pcbs->pcb_hib_sleep2_mode_flag != PCB_HIB_DISABLE_DEACTIVE) {
        if(udp_hib_sleep2_pcbs->sockid >= 0) {
            return udp_hib_sleep2_pcbs->sockid;
        }
    }

    return -1;
}

BOOL udp_check_is_include_list(struct udp_pcb* pcb)
{
    struct udp_pcb* pcb_tmp;

    for(pcb_tmp = udp_pcbs; pcb_tmp; pcb_tmp = pcb_tmp->next) {
        if(pcb_tmp == pcb) {
            return TRUE;
        }
    }

    return FALSE;
}

u16_t get_hib_udp_pcb_active_local_port(void)
{
    u16_t port = 0;

    if(udp_hib_sleep2_pcbs && udp_hib_sleep2_pcbs->pcb_hib_sleep2_mode_flag == PCB_HIB_ENABLE_ACTIVE) {
        port = udp_hib_sleep2_pcbs->local_port;
    }

    return port;
    
}


#endif


/**
 * Initialize this module.
 */
void
udp_init(void)
{
#if LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS && defined(LWIP_RAND)
  udp_port = UDP_ENSURE_LOCAL_PORT_RANGE(LWIP_RAND());
#endif /* LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS && defined(LWIP_RAND) */
}

/**
 * Allocate a new local UDP port.
 *
 * @return a new (free) local UDP port number
 */
static u16_t
udp_new_port(void)
{
  u16_t n = 0;
  struct udp_pcb *pcb;

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE  
  u16_t hib_udp_active_port = get_hib_udp_pcb_active_local_port();
#endif

again:
  if (udp_port++ == UDP_LOCAL_PORT_RANGE_END) {
    udp_port = UDP_LOCAL_PORT_RANGE_START;
  }
  /* Check all PCBs. */
  for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
    if (pcb->local_port == udp_port) {
      if (++n > (UDP_LOCAL_PORT_RANGE_END - UDP_LOCAL_PORT_RANGE_START)) {
        return 0;
      }
      goto again;
    }
#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
      if(hib_udp_active_port == udp_port) {
        goto again;
      }
#endif

  }
  return udp_port;
}

/** Common code to see if the current input packet matches the pcb
 * (current input packet is accessed via ip(4/6)_current_* macros)
 *
 * @param pcb pcb to check
 * @param inp network interface on which the datagram was received (only used for IPv4)
 * @param broadcast 1 if his is an IPv4 broadcast (global or subnet-only), 0 otherwise (only used for IPv4)
 * @return 1 on match, 0 otherwise
 */
static u8_t
udp_input_local_match(struct udp_pcb *pcb, struct netif *inp, u8_t broadcast)
{
  LWIP_UNUSED_ARG(inp);       /* in IPv6 only case */
  LWIP_UNUSED_ARG(broadcast); /* in IPv6 only case */

  /* Dual-stack: PCBs listening to any IP type also listen to any IP address */
  if (IP_IS_ANY_TYPE_VAL(pcb->local_ip)) {
#if LWIP_IPV4 && IP_SOF_BROADCAST_RECV
    if ((broadcast != 0) && !ip_get_option(pcb, SOF_BROADCAST)) {
      return 0;
    }
#endif /* LWIP_IPV4 && IP_SOF_BROADCAST_RECV */
    return 1;
  }

  /* Only need to check PCB if incoming IP version matches PCB IP version */
  if (IP_ADDR_PCB_VERSION_MATCH_EXACT(pcb, ip_current_dest_addr())) {
#if LWIP_IPV4
    /* Special case: IPv4 broadcast: all or broadcasts in my subnet
     * Note: broadcast variable can only be 1 if it is an IPv4 broadcast */
    if (broadcast != 0) {
#if IP_SOF_BROADCAST_RECV
      if (ip_get_option(pcb, SOF_BROADCAST))
#endif /* IP_SOF_BROADCAST_RECV */
      {
        if (ip4_addr_isany(ip_2_ip4(&pcb->local_ip)) ||
          ((ip4_current_dest_addr()->addr == IPADDR_BROADCAST)) ||
           ip4_addr_netcmp(ip_2_ip4(&pcb->local_ip), ip4_current_dest_addr(), netif_ip4_netmask(inp))) {
          return 1;
        }
      }
    } else
#endif /* LWIP_IPV4 */
    /* Handle IPv4 and IPv6: all or exact match */
    if (ip_addr_isany(&pcb->local_ip) || ip_addr_cmp(&pcb->local_ip, ip_current_dest_addr())) {
      return 1;
    }
  }

  return 0;
}

/**
 * Process an incoming UDP datagram.
 *
 * Given an incoming UDP datagram (as a chain of pbufs) this function
 * finds a corresponding UDP PCB and hands over the pbuf to the pcbs
 * recv function. If no pcb is found or the datagram is incorrect, the
 * pbuf is freed.
 *
 * @param p pbuf to be demultiplexed to a UDP PCB (p->payload pointing to the UDP header)
 * @param inp network interface on which the datagram was received.
 *
 */
void
udp_input(struct pbuf *p, struct netif *inp)
{
  struct udp_hdr *udphdr;
  struct udp_pcb *pcb, *prev;
  struct udp_pcb *uncon_pcb;
  u16_t src, dest;
  u8_t broadcast;
  u8_t for_us = 0;

  LWIP_UNUSED_ARG(inp);

  PERF_START;

  UDP_STATS_INC(udp.recv);

  /* Check minimum length (UDP header) */
  if (p->len < UDP_HLEN) {
    /* drop short packets */
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_input_1, P_WARNING, 1, "udp_input: short UDP datagram (%u bytes) discarded", p->tot_len);
#else    
    LWIP_DEBUGF(UDP_DEBUG,
                ("udp_input: short UDP datagram (%"U16_F" bytes) discarded\n", p->tot_len));
#endif
    UDP_STATS_INC(udp.lenerr);
    UDP_STATS_INC(udp.drop);
    MIB2_STATS_INC(mib2.udpinerrors);
    pbuf_free(p);
    goto end;
  }

  udphdr = (struct udp_hdr *)p->payload;

  /* is broadcast packet ? */
  broadcast = ip_addr_isbroadcast(ip_current_dest_addr(), ip_current_netif());

#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_input_2, P_INFO, 1, "udp_input: received datagram of length %u", p->tot_len);
#else 
  LWIP_DEBUGF(UDP_DEBUG, ("udp_input: received datagram of length %"U16_F"\n", p->tot_len));
#endif

  /* convert src and dest ports to host byte order */
  src = lwip_ntohs(udphdr->src);
  dest = lwip_ntohs(udphdr->dest);

  udp_debug_print(udphdr);

  /* print the UDP source and destination */
#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_input_3, P_INFO, 2, "udp (%u) <--(%u)", lwip_ntohs(udphdr->dest), lwip_ntohs(udphdr->src));
#else  
  LWIP_DEBUGF(UDP_DEBUG, ("udp ("));
#endif
  ip_addr_debug_print(UDP_DEBUG, ip_current_dest_addr());
#ifndef LWIP_ENABLE_UNILOG
  LWIP_DEBUGF(UDP_DEBUG, (", %"U16_F") <-- (", lwip_ntohs(udphdr->dest)));
#endif
  ip_addr_debug_print(UDP_DEBUG, ip_current_src_addr());
#ifndef LWIP_ENABLE_UNILOG
  LWIP_DEBUGF(UDP_DEBUG, (", %"U16_F")\n", lwip_ntohs(udphdr->src)));
#endif

  pcb = NULL;
  prev = NULL;
  uncon_pcb = NULL;
  /* Iterate through the UDP pcb list for a matching pcb.
   * 'Perfect match' pcbs (connected to the remote port & ip address) are
   * preferred. If no perfect match is found, the first unconnected pcb that
   * matches the local port and ip address gets the datagram. */
  for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
    /* print the PCB local and remote address */
#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_input_4, P_INFO, 2, "pcb (%u) <--(%u)", pcb->local_port, pcb->remote_port);
#else     
    LWIP_DEBUGF(UDP_DEBUG, ("pcb ("));
#endif
    ip_addr_debug_print(UDP_DEBUG, &pcb->local_ip);
#ifndef LWIP_ENABLE_UNILOG
    LWIP_DEBUGF(UDP_DEBUG, (", %"U16_F") <-- (", pcb->local_port));
#endif
    ip_addr_debug_print(UDP_DEBUG, &pcb->remote_ip);
#ifndef LWIP_ENABLE_UNILOG
    LWIP_DEBUGF(UDP_DEBUG, (", %"U16_F")\n", pcb->remote_port));
#endif

    /* compare PCB local addr+port to UDP destination addr+port */
    if ((pcb->local_port == dest) &&
        (udp_input_local_match(pcb, inp, broadcast) != 0)) {
      if (((pcb->flags & UDP_FLAGS_CONNECTED) == 0) &&
          ((uncon_pcb == NULL)
#if SO_REUSE
          /* prefer specific IPs over cath-all */
          || !ip_addr_isany(&pcb->local_ip)
#endif /* SO_REUSE */
          )) {
        /* the first unconnected matching PCB */
        uncon_pcb = pcb;
      }

      /* compare PCB remote addr+port to UDP source addr+port */
      if ((pcb->remote_port == src) &&
          (ip_addr_isany_val(pcb->remote_ip) ||
          ip_addr_cmp(&pcb->remote_ip, ip_current_src_addr()))) {
        /* the first fully matching PCB */
        if (prev != NULL) {
          /* move the pcb to the front of udp_pcbs so that is
             found faster next time */
          prev->next = pcb->next;
          pcb->next = udp_pcbs;
          udp_pcbs = pcb;
        } else {
          UDP_STATS_INC(udp.cachehit);
        }
        break;
      }
    }

    prev = pcb;
  }
  /* no fully matching pcb found? then look for an unconnected pcb */
  if (pcb == NULL) {
    pcb = uncon_pcb;
  }

  /* Check checksum if this is a match or if it was directed at us. */
  if (pcb != NULL) {
    for_us = 1;
  } else {
#if LWIP_IPV6
    if (ip_current_is_v6()) {
      for_us = netif_get_ip6_addr_match(inp, ip6_current_dest_addr()) >= 0;
    }
#endif /* LWIP_IPV6 */
#if LWIP_IPV4
    if (!ip_current_is_v6()) {
      for_us = ip4_addr_cmp(netif_ip4_addr(inp), ip4_current_dest_addr());
    }
#endif /* LWIP_IPV4 */
  }

  if (for_us) {
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_input_5, P_INFO, 0, "udp_input: calculating checksum");
#else     
    LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE, ("udp_input: calculating checksum\n"));
#endif
#if CHECKSUM_CHECK_UDP
    IF__NETIF_CHECKSUM_ENABLED(inp, CHECKSUM_CHECK_UDP) {
#if LWIP_UDPLITE
      if (ip_current_header_proto() == IP_PROTO_UDPLITE) {
        /* Do the UDP Lite checksum */
        u16_t chklen = lwip_ntohs(udphdr->len);
        if (chklen < sizeof(struct udp_hdr)) {
          if (chklen == 0) {
            /* For UDP-Lite, checksum length of 0 means checksum
               over the complete packet (See RFC 3828 chap. 3.1) */
            chklen = p->tot_len;
          } else {
            /* At least the UDP-Lite header must be covered by the
               checksum! (Again, see RFC 3828 chap. 3.1) */
            goto chkerr;
          }
        }
        if (ip_chksum_pseudo_partial(p, IP_PROTO_UDPLITE,
                     p->tot_len, chklen,
                     ip_current_src_addr(), ip_current_dest_addr()) != 0) {
          goto chkerr;
        }
      } else
#endif /* LWIP_UDPLITE */
      {
        if (udphdr->chksum != 0) {
          if (ip_chksum_pseudo(p, IP_PROTO_UDP, p->tot_len,
                               ip_current_src_addr(),
                               ip_current_dest_addr()) != 0) {
            goto chkerr;
          }
        }
      }
    }
#endif /* CHECKSUM_CHECK_UDP */
    if (pbuf_header(p, -UDP_HLEN)) {
      /* Can we cope with this failing? Just assert for now */
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_input_6, P_ERROR, 0, "pbuf_header failed");
#else       
      LWIP_ASSERT("pbuf_header failed\n", 0);
#endif
      UDP_STATS_INC(udp.drop);
      MIB2_STATS_INC(mib2.udpinerrors);
      pbuf_free(p);
      goto end;
    }

    if (pcb != NULL) {
      MIB2_STATS_INC(mib2.udpindatagrams);
#if SO_REUSE && SO_REUSE_RXTOALL
      if (ip_get_option(pcb, SOF_REUSEADDR) &&
          (broadcast || ip_addr_ismulticast(ip_current_dest_addr()))) {
        /* pass broadcast- or multicast packets to all multicast pcbs
           if SOF_REUSEADDR is set on the first match */
        struct udp_pcb *mpcb;
        u8_t p_header_changed = 0;
        s16_t hdrs_len = (s16_t)(ip_current_header_tot_len() + UDP_HLEN);
        for (mpcb = udp_pcbs; mpcb != NULL; mpcb = mpcb->next) {
          if (mpcb != pcb) {
            /* compare PCB local addr+port to UDP destination addr+port */
            if ((mpcb->local_port == dest) &&
                (udp_input_local_match(mpcb, inp, broadcast) != 0)) {
              /* pass a copy of the packet to all local matches */
              if (mpcb->recv != NULL) {
                struct pbuf *q;
                /* for that, move payload to IP header again */
                if (p_header_changed == 0) {
                  pbuf_header_force(p, hdrs_len);
                  p_header_changed = 1;
                }
                q = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
                if (q != NULL) {
                  err_t err = pbuf_copy(q, p);
                  if (err == ERR_OK) {
                    /* move payload to UDP data */
                    pbuf_header(q, -hdrs_len);
                    mpcb->recv(mpcb->recv_arg, mpcb, q, ip_current_src_addr(), src);
                  }
                }
              }
            }
          }
        }
        if (p_header_changed) {
          /* and move payload to UDP data again */
          pbuf_header(p, -hdrs_len);
        }
      }
#endif /* SO_REUSE && SO_REUSE_RXTOALL */
      /* callback */
      if (pcb->recv != NULL) {
        /* now the recv function is responsible for freeing p */
        pcb->recv(pcb->recv_arg, pcb, p, ip_current_src_addr(), src);
      } else {
        /* no recv function registered? then we have to free the pbuf! */
        pbuf_free(p);
        goto end;
      }
    } else {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_input_7, P_INFO, 0, "udp_input: not for us");
#else    
      LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE, ("udp_input: not for us.\n"));
#endif

#if LWIP_ICMP || LWIP_ICMP6
      /* No match was found, send ICMP destination port unreachable unless
         destination address was broadcast/multicast. */
      if (!broadcast && !ip_addr_ismulticast(ip_current_dest_addr())) {
        /* move payload pointer back to ip header */
        pbuf_header_force(p, (s16_t)(ip_current_header_tot_len() + UDP_HLEN));

#if UDP_UNREACHABLE_PACKET_NEED_REPLY_ICMP_ERROR_MESSAGE        
        icmp_port_unreach(ip_current_is_v6(), p);
#else
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_input_8, P_WARNING, 0, "udp_input:not for us,and not reply icmp unreachable message");
#endif

      }
#endif /* LWIP_ICMP || LWIP_ICMP6 */
      UDP_STATS_INC(udp.proterr);
      UDP_STATS_INC(udp.drop);
      MIB2_STATS_INC(mib2.udpnoports);
      pbuf_free(p);
    }
  } else {
    pbuf_free(p);
  }
end:
  PERF_STOP("udp_input");
  return;
#if CHECKSUM_CHECK_UDP
chkerr:
#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_input_9, P_WARNING, 0, "udp_input: UDP (or UDP Lite) datagram discarded due to failing checksum");
#else 
  LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
              ("udp_input: UDP (or UDP Lite) datagram discarded due to failing checksum\n"));
#endif
  UDP_STATS_INC(udp.chkerr);
  UDP_STATS_INC(udp.drop);
  MIB2_STATS_INC(mib2.udpinerrors);
  pbuf_free(p);
  PERF_STOP("udp_input");
#endif /* CHECKSUM_CHECK_UDP */
}

/**
 * @ingroup udp_raw
 * Send data using UDP.
 *
 * @param pcb UDP PCB used to send the data.
 * @param p chain of pbuf's to be sent.
 *
 * The datagram will be sent to the current remote_ip & remote_port
 * stored in pcb. If the pcb is not bound to a port, it will
 * automatically be bound to a random port.
 *
 * @return lwIP error code.
 * - ERR_OK. Successful. No error occurred.
 * - ERR_MEM. Out of memory.
 * - ERR_RTE. Could not find route to destination address.
 * - ERR_VAL. No PCB or PCB is dual-stack
 * - More errors could be returned by lower protocol layers.
 *
 * @see udp_disconnect() udp_sendto()
 */
err_t
udp_send(struct udp_pcb *pcb, struct pbuf *p)
{
  if ((pcb == NULL) || IP_IS_ANY_TYPE_VAL(pcb->remote_ip)) {
    return ERR_VAL;
  }

  /* send to the packet using remote ip and port stored in the pcb */
  return udp_sendto(pcb, p, &pcb->remote_ip, pcb->remote_port);
}

#if LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP
/** @ingroup udp_raw
 * Same as udp_send() but with checksum
 */
err_t
udp_send_chksum(struct udp_pcb *pcb, struct pbuf *p,
                u8_t have_chksum, u16_t chksum)
{
  if ((pcb == NULL) || IP_IS_ANY_TYPE_VAL(pcb->remote_ip)) {
    return ERR_VAL;
  }

  /* send to the packet using remote ip and port stored in the pcb */
  return udp_sendto_chksum(pcb, p, &pcb->remote_ip, pcb->remote_port,
    have_chksum, chksum);
}
#endif /* LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP */

/**
 * @ingroup udp_raw
 * Send data to a specified address using UDP.
 *
 * @param pcb UDP PCB used to send the data.
 * @param p chain of pbuf's to be sent.
 * @param dst_ip Destination IP address.
 * @param dst_port Destination UDP port.
 *
 * dst_ip & dst_port are expected to be in the same byte order as in the pcb.
 *
 * If the PCB already has a remote address association, it will
 * be restored after the data is sent.
 *
 * @return lwIP error code (@see udp_send for possible error codes)
 *
 * @see udp_disconnect() udp_send()
 */
err_t
udp_sendto(struct udp_pcb *pcb, struct pbuf *p,
  const ip_addr_t *dst_ip, u16_t dst_port)
{
#if LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP
  return udp_sendto_chksum(pcb, p, dst_ip, dst_port, 0, 0);
}

/** @ingroup udp_raw
 * Same as udp_sendto(), but with checksum */
err_t
udp_sendto_chksum(struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *dst_ip,
                  u16_t dst_port, u8_t have_chksum, u16_t chksum)
{
#endif /* LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP */
  struct netif *netif;
  const ip_addr_t *dst_ip_route = dst_ip;

  if ((pcb == NULL) || (dst_ip == NULL) || !IP_ADDR_PCB_VERSION_MATCH(pcb, dst_ip)) {
    return ERR_VAL;
  }

#if 0
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_chksum_1, P_INFO, 0, "udp_sendto_chksum");
  LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE, ("udp_send\n"));
#endif

#if LWIP_IPV6 || (LWIP_IPV4 && LWIP_MULTICAST_TX_OPTIONS)
  if (ip_addr_ismulticast(dst_ip_route)) {
#if LWIP_IPV6
    if (IP_IS_V6(dst_ip)) {
      /* For multicast, find a netif based on source address. */
      dst_ip_route = &pcb->local_ip;
    } else
#endif /* LWIP_IPV6 */
    {
#if LWIP_IPV4 && LWIP_MULTICAST_TX_OPTIONS
      /* IPv4 does not use source-based routing by default, so we use an
         administratively selected interface for multicast by default.
         However, this can be overridden by setting an interface address
         in pcb->multicast_ip that is used for routing. */
      if (!ip_addr_isany_val(pcb->multicast_ip) &&
          !ip4_addr_cmp(ip_2_ip4(&pcb->multicast_ip), IP4_ADDR_BROADCAST)) {
        dst_ip_route = &pcb->multicast_ip;
      }
#endif /* LWIP_IPV4 && LWIP_MULTICAST_TX_OPTIONS */
    }
  }
#endif /* LWIP_IPV6 || (LWIP_IPV4 && LWIP_MULTICAST_TX_OPTIONS) */

  /* find the outgoing network interface for this packet */
  if(IP_IS_ANY_TYPE_VAL(pcb->local_ip)) {
    /* Don't call ip_route() with IP_ANY_TYPE */
    netif = ip_route(IP46_ADDR_ANY(IP_GET_TYPE(dst_ip_route)), dst_ip_route);
  } else {
    netif = ip_route(&pcb->local_ip, dst_ip_route);
  }

  /* no outgoing network interface could be found? */
  if (netif == NULL) {
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_chksum_2, P_INFO, 0, "udp_sendto_chksum No route to ");
#else    
    LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("udp_send: No route to "));
#endif
    ip_addr_debug_print(UDP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, dst_ip);
#ifndef LWIP_ENABLE_UNILOG
    LWIP_DEBUGF(UDP_DEBUG, ("\n"));
#endif
    UDP_STATS_INC(udp.rterr);
    return ERR_RTE;
  }

#if LWIP_SUSPEND_UP_DATA_ENABLE
  if(netif->is_suspend) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_chksum_3, P_INFO, 2, "udp_send: netif %c%u is suspend", netif->name[0], netif->name[1]);
    return ERR_IF_SUSPEND;
  }
#endif

#if LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP
  return udp_sendto_if_chksum(pcb, p, dst_ip, dst_port, netif, have_chksum, chksum);
#else /* LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP */
  return udp_sendto_if(pcb, p, dst_ip, dst_port, netif);
#endif /* LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP */
}

/**
 * @ingroup udp_raw
 * Send data to a specified address using UDP.
 * The netif used for sending can be specified.
 *
 * This function exists mainly for DHCP, to be able to send UDP packets
 * on a netif that is still down.
 *
 * @param pcb UDP PCB used to send the data.
 * @param p chain of pbuf's to be sent.
 * @param dst_ip Destination IP address.
 * @param dst_port Destination UDP port.
 * @param netif the netif used for sending.
 *
 * dst_ip & dst_port are expected to be in the same byte order as in the pcb.
 *
 * @return lwIP error code (@see udp_send for possible error codes)
 *
 * @see udp_disconnect() udp_send()
 */
err_t
udp_sendto_if(struct udp_pcb *pcb, struct pbuf *p,
  const ip_addr_t *dst_ip, u16_t dst_port, struct netif *netif)
{
#if LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP
  return udp_sendto_if_chksum(pcb, p, dst_ip, dst_port, netif, 0, 0);
}

/** Same as udp_sendto_if(), but with checksum */
err_t
udp_sendto_if_chksum(struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *dst_ip,
                     u16_t dst_port, struct netif *netif, u8_t have_chksum,
                     u16_t chksum)
{
#endif /* LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP */
  const ip_addr_t *src_ip;

  if ((pcb == NULL) || (dst_ip == NULL) || !IP_ADDR_PCB_VERSION_MATCH(pcb, dst_ip)) {
    return ERR_VAL;
  }

  /* PCB local address is IP_ANY_ADDR? */
#if LWIP_IPV6
  if (IP_IS_V6(dst_ip)) {
    if (ip6_addr_isany(ip_2_ip6(&pcb->local_ip))) {
      src_ip = ip6_select_source_address(netif, ip_2_ip6(dst_ip));
      if (src_ip == NULL) {
        /* No suitable source address was found. */
        return ERR_RTE;
      }
#if ENABLE_PSIF
    //set the pcb local ip
    ip_addr_set_ipaddr(&pcb->local_ip, src_ip);
#endif      
    } else {
      /* use UDP PCB local IPv6 address as source address, if still valid. */
      if (netif_get_ip6_addr_match(netif, ip_2_ip6(&pcb->local_ip)) < 0) {
        /* Address isn't valid anymore. */
        return ERR_RTE;
      }
      src_ip = &pcb->local_ip;
    }
  }
#endif /* LWIP_IPV6 */
#if LWIP_IPV4 && LWIP_IPV6
  else
#endif /* LWIP_IPV4 && LWIP_IPV6 */
#if LWIP_IPV4
  if (ip4_addr_isany(ip_2_ip4(&pcb->local_ip)) ||
      ip4_addr_ismulticast(ip_2_ip4(&pcb->local_ip))) {
    /* if the local_ip is any or multicast
     * use the outgoing network interface IP address as source address */
    src_ip = netif_ip_addr4(netif);

#if ENABLE_PSIF
    //set the pcb local ip
    ip_addr_set_ipaddr(&pcb->local_ip, src_ip);
#endif    
  } else {
    /* check if UDP PCB local IP address is correct
     * this could be an old address if netif->ip_addr has changed */
    if (!ip4_addr_cmp(ip_2_ip4(&(pcb->local_ip)), netif_ip4_addr(netif))) {
      /* local_ip doesn't match, drop the packet */
      return ERR_RTE;
    }
    /* use UDP PCB local IP address as source address */
    src_ip = &pcb->local_ip;
  }
#endif /* LWIP_IPV4 */



#if LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP
  return udp_sendto_if_src_chksum(pcb, p, dst_ip, dst_port, netif, have_chksum, chksum, src_ip);
#else /* LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP */
  return udp_sendto_if_src(pcb, p, dst_ip, dst_port, netif, src_ip);
#endif /* LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP */
}

/** @ingroup udp_raw
 * Same as @ref udp_sendto_if, but with source address */
err_t
udp_sendto_if_src(struct udp_pcb *pcb, struct pbuf *p,
  const ip_addr_t *dst_ip, u16_t dst_port, struct netif *netif, const ip_addr_t *src_ip)
{
#if LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP
  return udp_sendto_if_src_chksum(pcb, p, dst_ip, dst_port, netif, 0, 0, src_ip);
}

/** Same as udp_sendto_if_src(), but with checksum */
err_t
udp_sendto_if_src_chksum(struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *dst_ip,
                     u16_t dst_port, struct netif *netif, u8_t have_chksum,
                     u16_t chksum, const ip_addr_t *src_ip)
{
#endif /* LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_UDP */
  struct udp_hdr *udphdr;
  err_t err;
  struct pbuf *q; /* q will be sent down the stack */
  u8_t ip_proto;
  u8_t ttl;

  if ((pcb == NULL) || (dst_ip == NULL) || !IP_ADDR_PCB_VERSION_MATCH(pcb, src_ip) ||
      !IP_ADDR_PCB_VERSION_MATCH(pcb, dst_ip)) {
    return ERR_VAL;
  }

#if LWIP_IPV4 && IP_SOF_BROADCAST
  /* broadcast filter? */
  if (!ip_get_option(pcb, SOF_BROADCAST) &&
#if LWIP_IPV6
      IP_IS_V4(dst_ip) &&
#endif /* LWIP_IPV6 */
      ip_addr_isbroadcast(dst_ip, netif)) {
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_if_src_chksum_1, P_WARNING, 0, "udp_sendto_if: SOF_BROADCAST not enabled on pcb 0x%x", (void *)pcb);
#else      
    LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
      ("udp_sendto_if: SOF_BROADCAST not enabled on pcb %p\n", (void *)pcb));
#endif
    return ERR_VAL;
  }
#endif /* LWIP_IPV4 && IP_SOF_BROADCAST */

  /* if the PCB is not yet bound to a port, bind it here */
  if (pcb->local_port == 0) {
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_if_src_chksum_2, P_INFO, 0, "udp_send: not yet bound to a port, binding now");
#else    
    LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE, ("udp_send: not yet bound to a port, binding now\n"));
#endif
    err = udp_bind(pcb, &pcb->local_ip, pcb->local_port);
    if (err != ERR_OK) {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_if_src_chksum_3, P_WARNING, 0, "udp_send: forced port bind failed");
#else         
      LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("udp_send: forced port bind failed\n"));
#endif
      return err;
    }
  }

  /* not enough space to add an UDP header to first pbuf in given p chain? */
  if (pbuf_header(p, UDP_HLEN)) {
    /* allocate header in a separate new pbuf */
    q = pbuf_alloc(PBUF_IP, UDP_HLEN, PBUF_RAM);
    /* new header pbuf could not be allocated? */
    if (q == NULL) {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_if_src_chksum_4, P_WARNING, 0, "udp_send: could not allocate header");
#else        
      LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("udp_send: could not allocate header\n"));
#endif
      return ERR_MEM;
    }
    if (p->tot_len != 0) {
      /* chain header q in front of given pbuf p (only if p contains data) */
      pbuf_chain(q, p);
    }

#if ENABLE_PSIF
    //init new pbuf dataRai&bExceptdata value
    q->dataRai = p->dataRai;
    q->bExceptData = p->bExceptData;
#endif

    /* first pbuf q points to header pbuf */
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_if_src_chksum_5, P_INFO, 2, "udp_send: added header pbuf 0x%x before given pbuf 0x%x", (void *)q, (void *)p);
#else    
    LWIP_DEBUGF(UDP_DEBUG,
                ("udp_send: added header pbuf %p before given pbuf %p\n", (void *)q, (void *)p));
#endif
  } else {
    /* adding space for header within p succeeded */
    /* first pbuf q equals given pbuf */
    q = p;
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_if_src_chksum_6, P_INFO, 1, "udp_send: added header in given pbuf 0x%x", (void *)p);
#else    
    LWIP_DEBUGF(UDP_DEBUG, ("udp_send: added header in given pbuf %p\n", (void *)p));
#endif
  }

#if LWIP_ENABLE_UNILOG
  if(q->len < sizeof(struct udp_hdr)) {
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_if_src_chksum_7, P_ERROR, 0, "check that first pbuf can hold struct udp_hdr");
  }
#else
  LWIP_ASSERT("check that first pbuf can hold struct udp_hdr",
              (q->len >= sizeof(struct udp_hdr)));
#endif
  /* q now represents the packet to be sent */
  udphdr = (struct udp_hdr *)q->payload;
  udphdr->src = lwip_htons(pcb->local_port);
  udphdr->dest = lwip_htons(dst_port);
  /* in UDP, 0 checksum means 'no checksum' */
  udphdr->chksum = 0x0000;

  /* Multicast Loop? */
#if (LWIP_IPV4 && LWIP_MULTICAST_TX_OPTIONS) || (LWIP_IPV6 && LWIP_IPV6_MLD)
  if (((pcb->flags & UDP_FLAGS_MULTICAST_LOOP) != 0) && ip_addr_ismulticast(dst_ip)) {
    q->flags |= PBUF_FLAG_MCASTLOOP;
  }
#endif /* (LWIP_IPV4 && LWIP_MULTICAST_TX_OPTIONS) || (LWIP_IPV6 && LWIP_IPV6_MLD) */

#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_if_src_chksum_8, P_INFO, 1, "udp_send: sending datagram of length %u", q->tot_len);
#else
  LWIP_DEBUGF(UDP_DEBUG, ("udp_send: sending datagram of length %"U16_F"\n", q->tot_len));
#endif

#if LWIP_UDPLITE
  /* UDP Lite protocol? */
  if (pcb->flags & UDP_FLAGS_UDPLITE) {
    u16_t chklen, chklen_hdr;
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_if_src_chksum_9, P_INFO, 1, "udp_send: UDP LITE packet length %u", q->tot_len);
#else    
    LWIP_DEBUGF(UDP_DEBUG, ("udp_send: UDP LITE packet length %"U16_F"\n", q->tot_len));
#endif
    /* set UDP message length in UDP header */
    chklen_hdr = chklen = pcb->chksum_len_tx;
    if ((chklen < sizeof(struct udp_hdr)) || (chklen > q->tot_len)) {
      if (chklen != 0) {
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_if_src_chksum_10, P_INFO, 1, "udp_send: UDP LITE pcb->chksum_len is illegal: %u", chklen);
#else        
        LWIP_DEBUGF(UDP_DEBUG, ("udp_send: UDP LITE pcb->chksum_len is illegal: %"U16_F"\n", chklen));
#endif
      }
      /* For UDP-Lite, checksum length of 0 means checksum
         over the complete packet. (See RFC 3828 chap. 3.1)
         At least the UDP-Lite header must be covered by the
         checksum, therefore, if chksum_len has an illegal
         value, we generate the checksum over the complete
         packet to be safe. */
      chklen_hdr = 0;
      chklen = q->tot_len;
    }
    udphdr->len = lwip_htons(chklen_hdr);
    /* calculate checksum */
#if CHECKSUM_GEN_UDP
    IF__NETIF_CHECKSUM_ENABLED(netif, NETIF_CHECKSUM_GEN_UDP) {
#if LWIP_CHECKSUM_ON_COPY
      if (have_chksum) {
        chklen = UDP_HLEN;
      }
#endif /* LWIP_CHECKSUM_ON_COPY */
      udphdr->chksum = ip_chksum_pseudo_partial(q, IP_PROTO_UDPLITE,
        q->tot_len, chklen, src_ip, dst_ip);
#if LWIP_CHECKSUM_ON_COPY
      if (have_chksum) {
        u32_t acc;
        acc = udphdr->chksum + (u16_t)~(chksum);
        udphdr->chksum = FOLD_U32T(acc);
      }
#endif /* LWIP_CHECKSUM_ON_COPY */

      /* chksum zero must become 0xffff, as zero means 'no checksum' */
      if (udphdr->chksum == 0x0000) {
        udphdr->chksum = 0xffff;
      }
    }
#endif /* CHECKSUM_GEN_UDP */

    ip_proto = IP_PROTO_UDPLITE;
  } else
#endif /* LWIP_UDPLITE */
  {      /* UDP */
#if 0
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_if_src_chksum_11, P_INFO, 1, "udp_send: UDP packet length %u", q->tot_len);   
    LWIP_DEBUGF(UDP_DEBUG, ("udp_send: UDP packet length %"U16_F"\n", q->tot_len));
#endif
    udphdr->len = lwip_htons(q->tot_len);
    /* calculate checksum */
#if CHECKSUM_GEN_UDP
    IF__NETIF_CHECKSUM_ENABLED(netif, NETIF_CHECKSUM_GEN_UDP) {
      /* Checksum is mandatory over IPv6. */
      if (IP_IS_V6(dst_ip) || (pcb->flags & UDP_FLAGS_NOCHKSUM) == 0) {
        u16_t udpchksum;
#if LWIP_CHECKSUM_ON_COPY
        if (have_chksum) {
          u32_t acc;
          udpchksum = ip_chksum_pseudo_partial(q, IP_PROTO_UDP,
            q->tot_len, UDP_HLEN, src_ip, dst_ip);
          acc = udpchksum + (u16_t)~(chksum);
          udpchksum = FOLD_U32T(acc);
        } else
#endif /* LWIP_CHECKSUM_ON_COPY */
        {
          udpchksum = ip_chksum_pseudo(q, IP_PROTO_UDP, q->tot_len,
            src_ip, dst_ip);
        }

        /* chksum zero must become 0xffff, as zero means 'no checksum' */
        if (udpchksum == 0x0000) {
          udpchksum = 0xffff;
        }
        udphdr->chksum = udpchksum;
      }
    }
#endif /* CHECKSUM_GEN_UDP */
    ip_proto = IP_PROTO_UDP;
  }

  /* Determine TTL to use */
#if LWIP_MULTICAST_TX_OPTIONS
  ttl = (ip_addr_ismulticast(dst_ip) ? udp_get_multicast_ttl(pcb) : pcb->ttl);
#else /* LWIP_MULTICAST_TX_OPTIONS */
  ttl = pcb->ttl;
#endif /* LWIP_MULTICAST_TX_OPTIONS */

#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_sendto_if_src_chksum_12, P_INFO, 2, "udp_send: UDP packet checksum  %u ip_output_if  %u", udphdr->chksum, (u16_t)ip_proto);
#else 
  LWIP_DEBUGF(UDP_DEBUG, ("udp_send: UDP checksum 0x%04"X16_F"\n", udphdr->chksum));
  LWIP_DEBUGF(UDP_DEBUG, ("udp_send: ip_output_if (,,,,0x%02"X16_F",)\n", (u16_t)ip_proto));
#endif

  /* output to IP */
  NETIF_SET_HWADDRHINT(netif, &(pcb->addr_hint));
  err = ip_output_if_src(q, src_ip, dst_ip, ttl, pcb->tos, ip_proto, netif);
  NETIF_SET_HWADDRHINT(netif, NULL);

  /* @todo: must this be increased even if error occurred? */
  MIB2_STATS_INC(mib2.udpoutdatagrams);

  /* did we chain a separate header pbuf earlier? */
  if (q != p) {
    /* free the header pbuf */
    pbuf_free(q);
    q = NULL;
    /* p is still referenced by the caller, and will live on */
  }

  UDP_STATS_INC(udp.xmit);
  return err;
}

/**
 * @ingroup udp_raw
 * Bind an UDP PCB.
 *
 * @param pcb UDP PCB to be bound with a local address ipaddr and port.
 * @param ipaddr local IP address to bind with. Use IP4_ADDR_ANY to
 * bind to all local interfaces.
 * @param port local UDP port to bind with. Use 0 to automatically bind
 * to a random port between UDP_LOCAL_PORT_RANGE_START and
 * UDP_LOCAL_PORT_RANGE_END.
 *
 * ipaddr & port are expected to be in the same byte order as in the pcb.
 *
 * @return lwIP error code.
 * - ERR_OK. Successful. No error occurred.
 * - ERR_USE. The specified ipaddr and port are already bound to by
 * another UDP PCB.
 *
 * @see udp_disconnect()
 */
err_t
udp_bind(struct udp_pcb *pcb, const ip_addr_t *ipaddr, u16_t port)
{
  struct udp_pcb *ipcb;
  u8_t rebind;

#if LWIP_IPV4
  /* Don't propagate NULL pointer (IPv4 ANY) to subsequent functions */
  if (ipaddr == NULL) {
    ipaddr = IP4_ADDR_ANY;
  }
#endif /* LWIP_IPV4 */

  /* still need to check for ipaddr == NULL in IPv6 only case */
  if ((pcb == NULL) || (ipaddr == NULL)) {
    return ERR_VAL;
  }

#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_bind_1, P_INFO, 1, "udp_bind port = %u", port);
#else
  LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE, ("udp_bind(ipaddr = "));
#endif
  ip_addr_debug_print(UDP_DEBUG | LWIP_DBG_TRACE, ipaddr);
#ifndef LWIP_ENABLE_UNILOG
  LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE, (", port = %"U16_F")\n", port));
#endif

  rebind = 0;
  /* Check for double bind and rebind of the same pcb */
  for (ipcb = udp_pcbs; ipcb != NULL; ipcb = ipcb->next) {
    /* is this UDP PCB already on active list? */
    if (pcb == ipcb) {
      rebind = 1;
      break;
    }
  }

  /* no port specified? */
  if (port == 0) {
    port = udp_new_port();
    if (port == 0) {
      /* no more ports available in local range */
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_bind_2, P_WARNING, 0, "udp_bind: out of free UDP ports");
#else      
      LWIP_DEBUGF(UDP_DEBUG, ("udp_bind: out of free UDP ports\n"));
#endif
      return ERR_USE;
    }
  } else {
    for (ipcb = udp_pcbs; ipcb != NULL; ipcb = ipcb->next) {
      if (pcb != ipcb) {
      /* By default, we don't allow to bind to a port that any other udp
         PCB is already bound to, unless *all* PCBs with that port have tha
         REUSEADDR flag set. */
#if SO_REUSE
        if (!ip_get_option(pcb, SOF_REUSEADDR) ||
            !ip_get_option(ipcb, SOF_REUSEADDR))
#endif /* SO_REUSE */
        {
          /* port matches that of PCB in list and REUSEADDR not set -> reject */
          if ((ipcb->local_port == port) &&
              /* IP address matches? */
              ip_addr_cmp(&ipcb->local_ip, ipaddr)) {
            /* other PCB already binds to this local IP and port */
#if LWIP_ENABLE_UNILOG
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_bind_3, P_INFO, 1, "udp_bind: local port %u already bound by another pcb", port);
#else             
            LWIP_DEBUGF(UDP_DEBUG,
                        ("udp_bind: local port %"U16_F" already bound by another pcb\n", port));
#endif
            return ERR_USE;
          }
        }
      }
    }
  }

  ip_addr_set_ipaddr(&pcb->local_ip, ipaddr);

  pcb->local_port = port;
  mib2_udp_bind(pcb);
  /* pcb not active yet? */
  if (rebind == 0) {
    /* place the PCB on the active list if not already there */
    pcb->next = udp_pcbs;
    udp_pcbs = pcb;
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_bind_5, P_INFO, 0, "udp_bind: add active udp list)");
  }
#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_bind_4, P_INFO, 1, "udp_bind: bound to port(%u)", pcb->local_port);
#else  
  LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("udp_bind: bound to "));
#endif
  ip_addr_debug_print(UDP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, &pcb->local_ip);
#ifndef LWIP_ENABLE_UNILOG
  LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, (", port %"U16_F")\n", pcb->local_port));
#endif
  return ERR_OK;
}

/**
 * @ingroup udp_raw
 * Connect an UDP PCB.
 *
 * This will associate the UDP PCB with the remote address.
 *
 * @param pcb UDP PCB to be connected with remote address ipaddr and port.
 * @param ipaddr remote IP address to connect with.
 * @param port remote UDP port to connect with.
 *
 * @return lwIP error code
 *
 * ipaddr & port are expected to be in the same byte order as in the pcb.
 *
 * The udp pcb is bound to a random local port if not already bound.
 *
 * @see udp_disconnect()
 */
err_t
udp_connect(struct udp_pcb *pcb, const ip_addr_t *ipaddr, u16_t port)
{
  struct udp_pcb *ipcb;

  if ((pcb == NULL) || (ipaddr == NULL)) {
    return ERR_VAL;
  }

  if (pcb->local_port == 0) {
    err_t err = udp_bind(pcb, &pcb->local_ip, pcb->local_port);
    if (err != ERR_OK) {
      return err;
    }
  }

  ip_addr_set_ipaddr(&pcb->remote_ip, ipaddr);
  pcb->remote_port = port;
  pcb->flags |= UDP_FLAGS_CONNECTED;

#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_connect_1, P_INFO, 1, "udp_connect: connected to port(%u)", pcb->remote_port);
#else
  LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("udp_connect: connected to "));
#endif
  ip_addr_debug_print(UDP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
                      &pcb->remote_ip);
#ifndef LWIP_ENABLE_UNILOG
  LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, (", port %"U16_F")\n", pcb->remote_port));
#endif

  /* Insert UDP PCB into the list of active UDP PCBs. */
  for (ipcb = udp_pcbs; ipcb != NULL; ipcb = ipcb->next) {
    if (pcb == ipcb) {
      /* already on the list, just return */
      return ERR_OK;
    }
  }
  /* PCB not yet on the list, add PCB now */
  pcb->next = udp_pcbs;
  udp_pcbs = pcb;
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_connect_2, P_INFO, 0, "udp_bind: add active udp list)");

  return ERR_OK;
}

/**
 * @ingroup udp_raw
 * Disconnect a UDP PCB
 *
 * @param pcb the udp pcb to disconnect.
 */
void
udp_disconnect(struct udp_pcb *pcb)
{
  /* reset remote address association */
#if LWIP_IPV4 && LWIP_IPV6
  if (IP_IS_ANY_TYPE_VAL(pcb->local_ip)) {
    ip_addr_copy(pcb->remote_ip, *IP_ANY_TYPE);
  } else {
#endif
    ip_addr_set_any(IP_IS_V6_VAL(pcb->remote_ip), &pcb->remote_ip);
#if LWIP_IPV4 && LWIP_IPV6
  }
#endif
  pcb->remote_port = 0;
  /* mark PCB as unconnected */
  pcb->flags &= ~UDP_FLAGS_CONNECTED;
  
}

/**
 * @ingroup udp_raw
 * Set a receive callback for a UDP PCB
 *
 * This callback will be called when receiving a datagram for the pcb.
 *
 * @param pcb the pcb for which to set the recv callback
 * @param recv function pointer of the callback function
 * @param recv_arg additional argument to pass to the callback function
 */
void
udp_recv(struct udp_pcb *pcb, udp_recv_fn recv, void *recv_arg)
{
  /* remember recv() callback and user data */
  pcb->recv = recv;
  pcb->recv_arg = recv_arg;
}

#if ENABLE_PSIF
void udp_poosf(struct udp_pcb *pcb, udp_process_oos_fn poosf)
{
    if(pcb != NULL) {
        pcb->poosf = poosf;
    }
}


int udp_send_ul_state_ind(u32_t bitmap[8], int socket_id, int status)
{
    struct Ul_Sequence_Status_t *ul_state_bitmap;
    struct pbuf *p;
    struct Ul_Sequence_Status_Request_t ul_state_req;
    ip_addr_t source_ip;
    int i;
    u8_t b_need_ind = 0;

    if(bitmap == NULL || socket_id < 0) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_send_ul_state_ind_1, P_ERROR, 0, "send_ul_state_ind: invalid parameter");
        return 1;
    }

    if(gsequence_handler_pcb == NULL) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_send_ul_state_ind_2, P_INFO, 0, "send_ul_state_ind: the ul sequence handler pcb is null");
        return 1;
    }

    //check whether need send indication
    for(i = 0; i < 8; i++) {
        if(bitmap[i] != 0) {
            b_need_ind = 1;
            break;
        }
    }

    if(b_need_ind == 0) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_send_ul_state_ind_3, P_INFO, 0, "send_ul_state_ind: no sequence state need indicate");
        return 1;
    }
    
    ul_state_bitmap = &ul_state_req.request_body;


    p = pbuf_alloc(PBUF_RAW, sizeof(struct Ul_Sequence_Status_Request_t), PBUF_RAM);

    if(p == NULL) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_send_ul_state_ind_5, P_ERROR, 0, "send_ul_state_ind: malloc pbuf fail");
        return 1;
    }

    ul_state_req.magic = UL_STATE_INDICATION_REQ_MAGIC;
    ul_state_req.request_id = UL_STATE_INDICATION_REQ_ID;
    ul_state_req.request_source = UL_STATE_INDICATION_REQ_SOURCE_ID;
    ul_state_bitmap = &ul_state_req.request_body ;
    ul_state_bitmap->socket_id = socket_id;
    ul_state_bitmap->ul_status = status;    
    memcpy(ul_state_bitmap->sequence_state_bitmap, bitmap, sizeof(ul_state_bitmap->sequence_state_bitmap));

    ipaddr_aton("127.0.0.1", &source_ip);

    memcpy(p->payload, &ul_state_req, sizeof(struct Ul_Sequence_Status_Request_t));

    if(gsequence_handler_pcb != NULL) {
        if(gsequence_handler_pcb->recv(gsequence_handler_pcb->recv_arg, gsequence_handler_pcb, p, &source_ip, 60002) < 0) {
            pbuf_free(p);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_send_ul_state_ind_6, P_INFO, 1, "send_ul_state_ind: reqeust_body 0x%x fail", (void *)ul_state_bitmap);
            return 1;
        }
        else {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_send_ul_state_ind_7, P_INFO, 1, "end_ul_state_ind: reqeust_body 0x%x success", (void *)ul_state_bitmap);
        }
    }else {
        pbuf_free(p);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_send_ul_state_ind_8, P_INFO, 0, "send_ul_state_ind: the ul sequence handler pcb is null");
        return 1;
    }

    return 0;
    
}
#endif

/**
 * @ingroup udp_raw
 * Remove an UDP PCB.
 *
 * @param pcb UDP PCB to be removed. The PCB is removed from the list of
 * UDP PCB's and the data structure is freed from memory.
 *
 * @see udp_new()
 */
void
udp_remove(struct udp_pcb *pcb)
{
  struct udp_pcb *pcb2;

  mib2_udp_unbind(pcb);
  /* pcb to be removed is first in list? */
  if (udp_pcbs == pcb) {
    /* make list start at 2nd pcb */
    udp_pcbs = udp_pcbs->next;
    /* pcb not 1st in list */
  } else {
    for (pcb2 = udp_pcbs; pcb2 != NULL; pcb2 = pcb2->next) {
      /* find pcb in udp_pcbs list */
      if (pcb2->next != NULL && pcb2->next == pcb) {
        /* remove pcb from list */
        pcb2->next = pcb->next;
        break;
      }
    }
  }
  memp_free(MEMP_UDP_PCB, pcb);
}

/**
 * @ingroup udp_raw
 * Create a UDP PCB.
 *
 * @return The UDP PCB which was created. NULL if the PCB data structure
 * could not be allocated.
 *
 * @see udp_remove()
 */
struct udp_pcb *
udp_new(void)
{
  struct udp_pcb *pcb;
  pcb = (struct udp_pcb *)memp_malloc(MEMP_UDP_PCB);
  /* could allocate UDP PCB? */
  if (pcb != NULL) {
    /* UDP Lite: by initializing to all zeroes, chksum_len is set to 0
     * which means checksum is generated over the whole datagram per default
     * (recommended as default by RFC 3828). */
    /* initialize PCB to all zeroes */
    memset(pcb, 0, sizeof(struct udp_pcb));
    pcb->ttl = UDP_TTL;
#if LWIP_MULTICAST_TX_OPTIONS
    udp_set_multicast_ttl(pcb, UDP_TTL);
#endif /* LWIP_MULTICAST_TX_OPTIONS */

#if ENABLE_PSIF
    pcb->sockid = -1;
#endif
  }
  return pcb;
}

/**
 * @ingroup udp_raw
 * Create a UDP PCB for specific IP type.
 *
 * @param type IP address type, see @ref lwip_ip_addr_type definitions.
 * If you want to listen to IPv4 and IPv6 (dual-stack) packets,
 * supply @ref IPADDR_TYPE_ANY as argument and bind to @ref IP_ANY_TYPE.
 * @return The UDP PCB which was created. NULL if the PCB data structure
 * could not be allocated.
 *
 * @see udp_remove()
 */
struct udp_pcb *
udp_new_ip_type(u8_t type)
{
  struct udp_pcb *pcb;
  pcb = udp_new();
#if LWIP_IPV4 && LWIP_IPV6
  if (pcb != NULL) {
    IP_SET_TYPE_VAL(pcb->local_ip,  type);
    IP_SET_TYPE_VAL(pcb->remote_ip, type);
  }
#else
  LWIP_UNUSED_ARG(type);
#endif /* LWIP_IPV4 && LWIP_IPV6 */
  return pcb;
}

/** This function is called from netif.c when address is changed
 *
 * @param old_addr IP address of the netif before change
 * @param new_addr IP address of the netif after change
 */
void udp_netif_ip_addr_changed(const ip_addr_t* old_addr, const ip_addr_t* new_addr)
{
  struct udp_pcb* upcb;

#if ENABLE_PSIF
  if (!ip_addr_isany(old_addr)) {
#else
  if (!ip_addr_isany(old_addr) && !ip_addr_isany(new_addr)) {
#endif    
    for (upcb = udp_pcbs; upcb != NULL; upcb = upcb->next) {
      /* PCB bound to current local interface address? */
      if (ip_addr_cmp(&upcb->local_ip, old_addr)) {
        /* The PCB is bound to the old ipaddr and
         * is set to bound to the new one instead */
#if ENABLE_PSIF
        if(new_addr == NULL) {
            if(IP_IS_V6(&upcb->local_ip))
            {
                ip_addr_set_any(1, &upcb->local_ip);
            }
            else
            {
                ip_addr_set_any(0, &upcb->local_ip);
            }
        }else {
            ip_addr_copy(upcb->local_ip, *new_addr);
        }
#else
        ip_addr_copy(upcb->local_ip, *new_addr);
#endif
      }
    }
  }
}

#if ENABLE_PSIF
void udp_netif_enter_oos_state(const ip_addr_t *address)
{
  struct udp_pcb* upcb;

  if (!ip_addr_isany(address)) {
    for (upcb = udp_pcbs; upcb != NULL; upcb = upcb->next) {
      if (ip_addr_cmp(&upcb->local_ip, address)) {
        upcb->related_netif_oos = 1;
        if(upcb->poosf != NULL) {
            upcb->poosf(upcb->recv_arg, 1);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_netif_enter_oos_state_1, P_INFO, 1, "udp_netif_enter_oos_state pcb 0x%x", (void *)upcb);
        }
      }
    }
  }    
}

void udp_netif_exit_oos_state(const ip_addr_t *address)
{
  struct udp_pcb* upcb;

  if (!ip_addr_isany(address)) {
    for (upcb = udp_pcbs; upcb != NULL; upcb = upcb->next) {
      if (ip_addr_cmp(&upcb->local_ip, address)) {
        upcb->related_netif_oos = 0;
        if(upcb->poosf != NULL) {
            upcb->poosf(upcb->recv_arg, 0);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_netif_exit_oos_state_1, P_INFO, 1, "udp_netif_exit_oos_state pcb 0x%x", (void *)upcb);
        }
      }
    }
  }    
}


#endif
#if UDP_DEBUG
/**
 * Print UDP header information for debug purposes.
 *
 * @param udphdr pointer to the udp header in memory.
 */
void
udp_debug_print(struct udp_hdr *udphdr)
{
#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_debug_print_1, P_INFO, 2, "UDP header: | %u | %u | (src port, dest port)", lwip_ntohs(udphdr->src), lwip_ntohs(udphdr->dest));
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, udp_debug_print_2, P_INFO, 2, "UDP header: | %u | %u | (len, chksum)", lwip_ntohs(udphdr->len), lwip_ntohs(udphdr->chksum));
#else
  LWIP_DEBUGF(UDP_DEBUG, ("UDP header:\n"));
  LWIP_DEBUGF(UDP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(UDP_DEBUG, ("|     %5"U16_F"     |     %5"U16_F"     | (src port, dest port)\n",
                          lwip_ntohs(udphdr->src), lwip_ntohs(udphdr->dest)));
  LWIP_DEBUGF(UDP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(UDP_DEBUG, ("|     %5"U16_F"     |     0x%04"X16_F"    | (len, chksum)\n",
                          lwip_ntohs(udphdr->len), lwip_ntohs(udphdr->chksum)));
  LWIP_DEBUGF(UDP_DEBUG, ("+-------------------------------+\n"));
#endif
}
#endif /* UDP_DEBUG */

#endif /* LWIP_UDP */
