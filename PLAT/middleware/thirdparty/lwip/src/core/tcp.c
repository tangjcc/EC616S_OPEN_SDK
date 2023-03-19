/**
 * @file
 * Transmission Control Protocol for IP
 * See also @ref tcp_raw
 *
 * @defgroup tcp_raw TCP
 * @ingroup callbackstyle_api
 * Transmission Control Protocol for IP\n
 * @see @ref raw_api and @ref netconn
 *
 * Common functions for the TCP implementation, such as functinos
 * for manipulating the data structures and the TCP timer functions. TCP functions
 * related to input and output is found in tcp_in.c and tcp_out.c respectively.\n
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

#include "lwip/opt.h"

#if LWIP_TCP /* don't build if not configured for use in lwipopts.h */

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/nd6.h"

#include <string.h>


#if ENABLE_PSIF
#include "lwip/udp.h"
#include "psifadpt.h"
#endif

#if LWIP_TIMER_ON_DEMOND
#include "lwip/timeouts.h"
#endif



#ifdef LWIP_HOOK_FILENAME
#include LWIP_HOOK_FILENAME
#endif

#ifndef TCP_LOCAL_PORT_RANGE_START
/* From http://www.iana.org/assignments/port-numbers:
   "The Dynamic and/or Private Ports are those from 49152 through 65535" */
#define TCP_LOCAL_PORT_RANGE_START        0xc000
#define TCP_LOCAL_PORT_RANGE_END          0xffff
#define TCP_ENSURE_LOCAL_PORT_RANGE(port) ((u16_t)(((port) & ~TCP_LOCAL_PORT_RANGE_START) + TCP_LOCAL_PORT_RANGE_START))
#endif

#if LWIP_TCP_KEEPALIVE
#define TCP_KEEP_DUR(pcb)   ((pcb)->keep_cnt * (pcb)->keep_intvl)
#define TCP_KEEP_INTVL(pcb) ((pcb)->keep_intvl)
#else /* LWIP_TCP_KEEPALIVE */
#define TCP_KEEP_DUR(pcb)   TCP_MAXIDLE
#define TCP_KEEP_INTVL(pcb) TCP_KEEPINTVL_DEFAULT
#endif /* LWIP_TCP_KEEPALIVE */

/* As initial send MSS, we use TCP_MSS but limit it to 536. */
#if TCP_MSS > 536
#define INITIAL_MSS 536
#else
#define INITIAL_MSS TCP_MSS
#endif

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE

// the global hib/sleep2 mode tcp pcb point
struct tcp_pcb *tcp_hib_sleep2_pcbs = NULL;

//the current hib/sleep2 mode tcp pcb number
u8_t tcp_hib_sleep2_pcb_num = 0;
#endif


static const char * const tcp_state_str[] = {
  "CLOSED",
  "LISTEN",
  "SYN_SENT",
  "SYN_RCVD",
  "ESTABLISHED",
  "FIN_WAIT_1",
  "FIN_WAIT_2",
  "CLOSE_WAIT",
  "CLOSING",
  "LAST_ACK",
  "TIME_WAIT"
};

/* last local TCP port */
static u16_t tcp_port = TCP_LOCAL_PORT_RANGE_START;

/* Incremented every coarse grained timer shot (typically every 500 ms). */
u32_t tcp_ticks;
#if ENABLE_PSIF
static const u8_t tcp_backoff[13] =
    { 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7};
#else
static const u8_t tcp_backoff[13] =
    { 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 7, 7, 7};
#endif
 /* Times per slowtmr hits */
static const u8_t tcp_persist_backoff[7] = { 3, 6, 12, 24, 48, 96, 120 };

/* The TCP PCB lists. */

/** List of all TCP PCBs bound but not yet (connected || listening) */
struct tcp_pcb *tcp_bound_pcbs;
/** List of all TCP PCBs in LISTEN state */
union tcp_listen_pcbs_t tcp_listen_pcbs;
/** List of all TCP PCBs that are in a state in which
 * they accept or send data. */
struct tcp_pcb *tcp_active_pcbs;
/** List of all TCP PCBs in TIME-WAIT state */
struct tcp_pcb *tcp_tw_pcbs;

/** An array with all (non-temporary) PCB lists, mainly used for smaller code size */
struct tcp_pcb ** const tcp_pcb_lists[] = {&tcp_listen_pcbs.pcbs, &tcp_bound_pcbs,
  &tcp_active_pcbs, &tcp_tw_pcbs};

u8_t tcp_active_pcbs_changed;

/** Timer counter to handle calling slow-timer from tcp_tmr() */
static u8_t tcp_timer;
static u8_t tcp_timer_ctr;
static u16_t tcp_new_port(void);

static err_t tcp_close_shutdown_fin(struct tcp_pcb *pcb);

#if TCP_DEBUG_PCB_LISTS
void TCP_REG(struct tcp_pcb **pcbs, struct tcp_pcb *npcb) {
       struct tcp_pcb *tcp_tmp_pcb;
       ECOMM_TRACE(UNILOG_TCPIP_LWIP, TCP_REG_1, P_INFO, 3, "TCP_REG 0x%x local port %d to 0x%x", (npcb), (npcb)->local_port, *(pcbs));

       //first check whether the pcb has insert the pcb list
       for (tcp_tmp_pcb = *(pcbs); tcp_tmp_pcb != NULL; tcp_tmp_pcb = tcp_tmp_pcb->next) {
         if(tcp_tmp_pcb == (npcb)) {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, TCP_REG_2, P_WARNING, 0, "TCP_REG:tcp_tmp_pcb == (npcb)");
            return;
         }
       }
       if(((pcbs) != &tcp_bound_pcbs) && ((npcb)->state == CLOSED)) {
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, TCP_REG_3, P_ERROR, 0, "TCP_REG: pcb->state == CLOSED");
       }
       (npcb)->next = *(pcbs);
       if((npcb)->next == (npcb)) {
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, TCP_REG_4, P_ERROR, 0, "TCP_REG: npcb->next == npcb");
       }
       *(pcbs) = (npcb);
       if(!tcp_pcbs_sane()) {
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, TCP_REG_5, P_ERROR, 0, "TCP_REG: tcp_pcbs sane");
       }

}
void TCP_RMV(struct tcp_pcb **pcbs, struct tcp_pcb *npcb) {
       struct tcp_pcb *tcp_tmp_pcb;
       if(*(pcbs) == NULL) {
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, TCP_RMV_1, P_ERROR, 0, "TCP_RMV: pcbs == NULL");
          return;
       }
       ECOMM_TRACE(UNILOG_TCPIP_LWIP, TCP_RMV_2, P_INFO, 2, "TCP_RMV removing 0x%x from 0x%x", (npcb), *(pcbs));
       if(*(pcbs) == (npcb)) {
          *(pcbs) = (*pcbs)->next;
       } else {
          for (tcp_tmp_pcb = *(pcbs); tcp_tmp_pcb != NULL; tcp_tmp_pcb = tcp_tmp_pcb->next) {
              if(tcp_tmp_pcb->next == (npcb)) {
                 tcp_tmp_pcb->next = (npcb)->next;
                 break;
              }
          }
       }
       (npcb)->next = NULL;
       if(!tcp_pcbs_sane()) {
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, TCP_RMV_3, P_ERROR, 0, "TCP_RMV: tcp_pcbs sane");
       }

}

#endif


#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE

u8_t tcp_get_curr_hib_sleep2_pcb_num(void)
{
    return tcp_hib_sleep2_pcb_num;
}

void tcp_add_curr_hib_sleep2_pcb_num(void)
{
    tcp_hib_sleep2_pcb_num++;
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_add_curr_hib_sleep2_pcb_num_1, P_INFO, 1, "tcp_add_curr_hib_sleep2_pcb_num %u", tcp_hib_sleep2_pcb_num);
}

void tcp_del_curr_hib_sleep2_pcb_num(void)
{
    tcp_hib_sleep2_pcb_num--;
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_del_curr_hib_sleep2_pcb_num_1, P_INFO, 1, "tcp_del_curr_hib_sleep2_pcb_num %u", tcp_hib_sleep2_pcb_num);
}

void tcp_enable_hib_sleep2_mode(struct tcp_pcb *pcb, u8_t state)
{
        pcb->pcb_hib_sleep2_mode_flag = state;
        tcp_hib_sleep2_pcb_num ++;
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_enable_hib_sleep2_mode_1, P_INFO, 1, "tcp_enable_hib_sleep2_mode %u", tcp_hib_sleep2_pcb_num);
}

void tcp_disable_hib_sleep2_mode(struct tcp_pcb *pcb, u8_t state)
{
        pcb->pcb_hib_sleep2_mode_flag = state;
        tcp_hib_sleep2_pcb_num --;
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_disable_hib_sleep2_mode_1, P_INFO, 1, "tcp_disable_hib_sleep2_mode %u", tcp_hib_sleep2_pcb_num);
}

void tcp_set_hib_sleep2_state(struct tcp_pcb *pcb, u8_t state)
{
        pcb->pcb_hib_sleep2_mode_flag = state;
}


struct tcp_pcb *tcp_get_hib_sleep2_pcb_list(void)
{
    return tcp_hib_sleep2_pcbs;
}

void tcp_set_hib_sleep2_pcb_list(struct tcp_pcb * new_pcb)
{
    tcp_hib_sleep2_pcbs = new_pcb;
}


void tcp_add_hib_sleep2_context_pcb(PsifHibTcpContext *tcpContext)
{
    struct tcp_pcb *new_pcb;

    new_pcb = tcp_new();

    if(new_pcb)
    {
        new_pcb->cwnd = tcpContext->cWnd;
        new_pcb->flags = tcpContext->flags;
        new_pcb->lastack = tcpContext->lastAck;
        new_pcb->mss = tcpContext->mss;
        new_pcb->so_options = tcpContext->option;
        new_pcb->rcv_ann_right_edge = tcpContext->rcvAnnRitEdge;
        new_pcb->rcv_ann_wnd = tcpContext->rcvAnnWnd;
        new_pcb->rcv_nxt = tcpContext->rcvNext;
        new_pcb->rcv_scale = tcpContext->RcvScale;
        new_pcb->rcv_wnd = tcpContext->rcvWnd;
        memcpy(&new_pcb->remote_ip, &tcpContext->remoteAddr, sizeof(new_pcb->remote_ip));
        new_pcb->remote_port = tcpContext->remotePort;
        memcpy(&new_pcb->local_ip, &tcpContext->localAddr, sizeof(new_pcb->local_ip));
        new_pcb->local_port = tcpContext->localPort;
        new_pcb->rto = tcpContext->rto;
        new_pcb->sa = tcpContext->sa;
        new_pcb->snd_lbb = tcpContext->SndLbb;
        new_pcb->snd_nxt = tcpContext->sndNxt;
        new_pcb->snd_scale = tcpContext->SndScale;
        new_pcb->snd_wl1 = tcpContext->sndWl1;
        new_pcb->snd_wl2 = tcpContext->sndWl2;
        new_pcb->snd_wnd = tcpContext->SndWnd;
        new_pcb->snd_wnd_max = tcpContext->SndWndMax;
        new_pcb->ssthresh = tcpContext->ssthresh;
        new_pcb->sv = tcpContext->sv;
        new_pcb->tos = tcpContext->tos;
        new_pcb->ttl = tcpContext->ttl;
        new_pcb->remote_ip.type = tcpContext->type;
        new_pcb->sockid = (int)tcpContext->sockid;

        tcp_enable_hib_sleep2_mode(new_pcb, PCB_HIB_ENABLE_DEACTIVE);
        tcp_hib_sleep2_pcbs = new_pcb;
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_add_hib_sleep2_context_pcb_1, P_INFO, 1, "tcp_add_hib_sleep2_context_pcb alloc new tcp_pcb success, sockid %d", new_pcb->sockid);
    }
    else
    {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_add_hib_sleep2_context_pcb_2, P_ERROR, 0, "tcp_add_hib_sleep2_context_pcb alloc new tcp_pcb fail");
    }

}

void tcp_check_hib_sleep2_pcb_active(ip_addr_t *ue_addr)
{
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_check_hib_sleep2_pcb_active_1, P_INFO, 0, "tcp_check_hib_sleep2_pcb_active");

    if(tcp_hib_sleep2_pcbs && tcp_hib_sleep2_pcbs->pcb_hib_sleep2_mode_flag == PCB_HIB_ENABLE_DEACTIVE) {
        if(ip_addr_cmp(ue_addr, &tcp_hib_sleep2_pcbs->local_ip)) {
            tcp_set_hib_sleep2_state(tcp_hib_sleep2_pcbs, PCB_HIB_ENABLE_ACTIVE);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_check_hib_sleep2_pcb_active_2, P_INFO, 1, "tcp_check_hib_sleep2_pcb_active change pcb 0x%x state to enable_active state", tcp_hib_sleep2_pcbs);
        }
    }
}

void tcp_check_hib_sleep2_pcb_deactive(ip_addr_t *ue_addr)
{
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_check_hib_sleep2_pcb_deactive_1, P_INFO, 0, "tcp_check_hib_sleep2_pcb_deactive");

    if(tcp_hib_sleep2_pcbs && tcp_hib_sleep2_pcbs->pcb_hib_sleep2_mode_flag == PCB_HIB_ENABLE_ACTIVE) {
        if(ip_addr_cmp(ue_addr, &tcp_hib_sleep2_pcbs->local_ip)) {
            tcp_set_hib_sleep2_state(tcp_hib_sleep2_pcbs, PCB_HIB_ENABLE_DEACTIVE);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_check_hib_sleep2_pcb_deactive_2, P_INFO, 1, "tcp_check_hib_sleep2_pcb_deactive change pcb 0x%x state to enable_deactive state", tcp_hib_sleep2_pcbs);
        }
    }
}


void tcp_remove_pcb_from_bounds_list(struct tcp_pcb* pcb)
{
    TCP_RMV(&tcp_bound_pcbs, pcb);
}


int tcp_get_sock_info(int fd, ip_addr_t *local_ip, ip_addr_t *remote_ip, u16_t *local_port, u16_t *remote_port)
{
    if(fd < 0) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_get_sock_info_1, P_ERROR, 0, "tcp_get_sock_info invalid fd");
        return -1;
    }

    if(tcp_hib_sleep2_pcbs && tcp_hib_sleep2_pcbs->pcb_hib_sleep2_mode_flag == PCB_HIB_ENABLE_ACTIVE) {
        if(tcp_hib_sleep2_pcbs->sockid == fd) {
            memcpy(local_ip, &tcp_hib_sleep2_pcbs->local_ip, sizeof(ip_addr_t));
            memcpy(remote_ip, &tcp_hib_sleep2_pcbs->remote_ip, sizeof(ip_addr_t));
            *local_port = tcp_hib_sleep2_pcbs->local_port;
            *remote_port = tcp_hib_sleep2_pcbs->remote_port;
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_get_sock_info_2, P_INFO, 0, "tcp_get_sock_info find adpat tcp hib pcb");
            return 0;
        }
    }

    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_get_sock_info_3, P_INFO, 0, "tcp_get_sock_info can not find adpat tcp hib pcb");
    return -1;
}

int tcp_get_hib_sock_id(void)
{
    if(tcp_hib_sleep2_pcbs && tcp_hib_sleep2_pcbs->pcb_hib_sleep2_mode_flag != PCB_HIB_DISABLE_DEACTIVE) {
        if(tcp_hib_sleep2_pcbs->sockid >= 0) {
            return tcp_hib_sleep2_pcbs->sockid;
        }
    }

    return -1;
}

BOOL tcp_check_is_include_list(struct tcp_pcb* pcb)
{
    struct tcp_pcb* pcb_tmp;

    for(pcb_tmp = tcp_active_pcbs; pcb_tmp; pcb_tmp = pcb_tmp->next) {
        if(pcb_tmp == pcb) {
            return TRUE;
        }
    }

    for(pcb_tmp = tcp_bound_pcbs; pcb_tmp; pcb_tmp = pcb_tmp->next) {
        if(pcb_tmp == pcb) {
            return TRUE;
        }
    }

    return FALSE;
}

u16_t get_hib_tcp_pcb_active_local_port(void)
{
    u16_t port = 0;

    if(tcp_hib_sleep2_pcbs && tcp_hib_sleep2_pcbs->pcb_hib_sleep2_mode_flag == PCB_HIB_ENABLE_ACTIVE) {
        port = tcp_hib_sleep2_pcbs->local_port;
    }

    return port;

}


#endif



#if ENABLE_PSIF

u8_t IsAnyActiveTcpConn(void)
{
    if(tcp_listen_pcbs.pcbs != NULL || tcp_bound_pcbs != NULL || tcp_active_pcbs != NULL || tcp_tw_pcbs != NULL) {
        return 1;
    }else {
        return 0;
    }
}


u8_t is_any_tcp_pcb_pending_ul_data(void)
{
    struct tcp_pcb * tcp_pcb_tmp;

    for(tcp_pcb_tmp = tcp_active_pcbs; tcp_pcb_tmp; tcp_pcb_tmp = tcp_pcb_tmp->next) {
        if(tcp_pcb_tmp->unsent || tcp_pcb_tmp->unacked) {
            return 1;
        }
    }

    return 0;
}


#endif

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
u8_t is_any_tcp_pcb_pending_data(void)
{
    struct tcp_pcb * tcp_pcb_tmp;

    for(tcp_pcb_tmp = tcp_active_pcbs; tcp_pcb_tmp; tcp_pcb_tmp = tcp_pcb_tmp->next) {
        if(tcp_pcb_tmp->unsent || tcp_pcb_tmp->unacked || tcp_pcb_tmp->refused_data || tcp_pcb_tmp->ooseq) {
            return 1;
        }
    }

    return 0;
}
#endif

/**
 * Initialize this module.
 */
void
tcp_init(void)
{
#if LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS && defined(LWIP_RAND)
  tcp_port = TCP_ENSURE_LOCAL_PORT_RANGE(LWIP_RAND());
#endif /* LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS && defined(LWIP_RAND) */
}

/**
 * Called periodically to dispatch TCP timers.
 */
void
tcp_tmr(void)
{
  /* Call tcp_fasttmr() every 250 ms */
  tcp_fasttmr();

  if (++tcp_timer & 1) {
    /* Call tcp_slowtmr() every 500 ms, i.e., every other timer
       tcp_tmr() is called. */
    tcp_slowtmr();
  }
}

#if LWIP_CALLBACK_API || TCP_LISTEN_BACKLOG
/** Called when a listen pcb is closed. Iterates one pcb list and removes the
 * closed listener pcb from pcb->listener if matching.
 */
static void
tcp_remove_listener(struct tcp_pcb *list, struct tcp_pcb_listen *lpcb)
{
   struct tcp_pcb *pcb;
   for (pcb = list; pcb != NULL; pcb = pcb->next) {
      if (pcb->listener == lpcb) {
         pcb->listener = NULL;
      }
   }
}
#endif

/** Called when a listen pcb is closed. Iterates all pcb lists and removes the
 * closed listener pcb from pcb->listener if matching.
 */
static void
tcp_listen_closed(struct tcp_pcb *pcb)
{
#if LWIP_CALLBACK_API || TCP_LISTEN_BACKLOG
  size_t i;
#if LWIP_ENABLE_UNILOG
  if(pcb == NULL || pcb->state != LISTEN) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_listen_closed_1, P_ERROR, 0, "pcb == NULL or pcb->state != LISTEN");
  }
#else
  LWIP_ASSERT("pcb != NULL", pcb != NULL);
  LWIP_ASSERT("pcb->state == LISTEN", pcb->state == LISTEN);
#endif
  for (i = 1; i < LWIP_ARRAYSIZE(tcp_pcb_lists); i++) {
    tcp_remove_listener(*tcp_pcb_lists[i], (struct tcp_pcb_listen*)pcb);
  }
#endif
  LWIP_UNUSED_ARG(pcb);
}

#if TCP_LISTEN_BACKLOG
/** @ingroup tcp_raw
 * Delay accepting a connection in respect to the listen backlog:
 * the number of outstanding connections is increased until
 * tcp_backlog_accepted() is called.
 *
 * ATTENTION: the caller is responsible for calling tcp_backlog_accepted()
 * or else the backlog feature will get out of sync!
 *
 * @param pcb the connection pcb which is not fully accepted yet
 */
void
tcp_backlog_delayed(struct tcp_pcb* pcb)
{
#if LWIP_ENABLE_UNILOG
  if(pcb == NULL) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_backlog_delayed_1, P_ERROR, 0, "pcb == NULL");
    return;
  }
#else
  LWIP_ASSERT("pcb != NULL", pcb != NULL);
#endif
  if ((pcb->flags & TF_BACKLOGPEND) == 0) {
    if (pcb->listener != NULL) {
      pcb->listener->accepts_pending++;
#if LWIP_ENABLE_UNILOG
     if(pcb->listener->accepts_pending == 0) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_backlog_delayed_2, P_ERROR, 0, "accepts_pending != 0");
     }
#else
      LWIP_ASSERT("accepts_pending != 0", pcb->listener->accepts_pending != 0);
#endif
      pcb->flags |= TF_BACKLOGPEND;
    }
  }
}

/** @ingroup tcp_raw
 * A delayed-accept a connection is accepted (or closed/aborted): decreases
 * the number of outstanding connections after calling tcp_backlog_delayed().
 *
 * ATTENTION: the caller is responsible for calling tcp_backlog_accepted()
 * or else the backlog feature will get out of sync!
 *
 * @param pcb the connection pcb which is now fully accepted (or closed/aborted)
 */
void
tcp_backlog_accepted(struct tcp_pcb* pcb)
{
#if LWIP_ENABLE_UNILOG
  if(pcb == NULL) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_backlog_accepted_1, P_ERROR, 0, "pcb == NULL");
    return;
  }
#else
  LWIP_ASSERT("pcb != NULL", pcb != NULL);
#endif
  if ((pcb->flags & TF_BACKLOGPEND) != 0) {
    if (pcb->listener != NULL) {
#if LWIP_ENABLE_UNILOG
      if(pcb->listener->accepts_pending == 0) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_backlog_accepted_2, P_ERROR, 0, "accepts_pending != 0");
      }
#else
      LWIP_ASSERT("accepts_pending != 0", pcb->listener->accepts_pending != 0);
#endif
      pcb->listener->accepts_pending--;
      pcb->flags &= ~TF_BACKLOGPEND;
    }
  }
}
#endif /* TCP_LISTEN_BACKLOG */

/**
 * Closes the TX side of a connection held by the PCB.
 * For tcp_close(), a RST is sent if the application didn't receive all data
 * (tcp_recved() not called for all data passed to recv callback).
 *
 * Listening pcbs are freed and may not be referenced any more.
 * Connection pcbs are freed if not yet connected and may not be referenced
 * any more. If a connection is established (at least SYN received or in
 * a closing state), the connection is closed, and put in a closing state.
 * The pcb is then automatically freed in tcp_slowtmr(). It is therefore
 * unsafe to reference it.
 *
 * @param pcb the tcp_pcb to close
 * @return ERR_OK if connection has been closed
 *         another err_t if closing failed and pcb is not freed
 */
static err_t
tcp_close_shutdown(struct tcp_pcb *pcb, u8_t rst_on_unacked_data)
{

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
    if(pcb->pcb_hib_sleep2_mode_flag != PCB_HIB_DISABLE_DEACTIVE) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_close_shutdown_5, P_SIG, 0, "tcp_close_shutdown THE hib/sleep2 tcp context pcb will shutdown");
        tcp_disable_hib_sleep2_mode(pcb, PCB_HIB_DISABLE_DEACTIVE);
        tcp_set_hib_sleep2_pcb_list(NULL);
    }
#endif

  if (rst_on_unacked_data && ((pcb->state == ESTABLISHED) || (pcb->state == CLOSE_WAIT))) {
    if ((pcb->refused_data != NULL) || (pcb->rcv_wnd != TCP_WND_MAX(pcb))) {
      /* Not all data received by application, send RST to tell the remote
         side about this. */
#if LWIP_ENABLE_UNILOG
      if(!(pcb->flags & TF_RXCLOSED)) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_close_shutdown_1, P_ERROR, 0, "pcb->flags & TF_RXCLOSED");
      }
#else
      LWIP_ASSERT("pcb->flags & TF_RXCLOSED", pcb->flags & TF_RXCLOSED);
#endif

      /* don't call tcp_abort here: we must not deallocate the pcb since
         that might not be expected when calling tcp_close */
      tcp_rst(pcb->snd_nxt, pcb->rcv_nxt, &pcb->local_ip, &pcb->remote_ip,
               pcb->local_port, pcb->remote_port);

      tcp_pcb_purge(pcb);
      TCP_RMV_ACTIVE(pcb);
       if (pcb->state == ESTABLISHED) {
         /* move to TIME_WAIT since we close actively */
         pcb->state = TIME_WAIT;
         TCP_REG(&tcp_tw_pcbs, pcb);

 #if LWIP_TIMER_ON_DEMOND
 #if LWIP_TCP_KEEPALIVE
         //deactive keepalive timer
         if (ip_get_option(pcb, SOF_KEEPALIVE)) {
             if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT)) {
                 sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT], (void *)pcb);
                 tcp_disable_timer_active_mask(pcb,LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT);
                 ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_close_shutdown_2, P_INFO, 0, "remove tcp keepalive timer");
             }
         }

         //active TIME_WAIT timer
         if(!tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT)) {
             sys_timeout(2 * TCP_MSL, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT], (void *)pcb);
             tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT);
             ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_close_shutdown_3, P_INFO, 1, "enable tcp time_wait timer %u", 2 * TCP_MSL);
         }else {
             ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_close_shutdown_4, P_INFO, 0, "tcp time_wait timer has active");
         }
 #endif
 #endif

       } else {
        /* CLOSE_WAIT: deallocate the pcb since we already sent a RST for it */
        if (tcp_input_pcb == pcb) {
          /* prevent using a deallocated pcb: free it from tcp_input later */
          tcp_trigger_input_pcb_close();
        } else {
          memp_free(MEMP_TCP_PCB, pcb);
        }
      }
      return ERR_OK;
    }
  }

  /* - states which free the pcb are handled here,
     - states which send FIN and change state are handled in tcp_close_shutdown_fin() */
  switch (pcb->state) {
  case CLOSED:
    /* Closing a pcb in the CLOSED state might seem erroneous,
     * however, it is in this state once allocated and as yet unused
     * and the user needs some way to free it should the need arise.
     * Calling tcp_close() with a pcb that has already been closed, (i.e. twice)
     * or for a pcb that has been used and then entered the CLOSED state
     * is erroneous, but this should never happen as the pcb has in those cases
     * been freed, and so any remaining handles are bogus. */
    if (pcb->local_port != 0) {
      TCP_RMV(&tcp_bound_pcbs, pcb);
    }
    memp_free(MEMP_TCP_PCB, pcb);
    break;
  case LISTEN:
    tcp_listen_closed(pcb);
    tcp_pcb_remove(&tcp_listen_pcbs.pcbs, pcb);
    memp_free(MEMP_TCP_PCB_LISTEN, pcb);
    break;
  case SYN_SENT:
    TCP_PCB_REMOVE_ACTIVE(pcb);
    memp_free(MEMP_TCP_PCB, pcb);
    MIB2_STATS_INC(mib2.tcpattemptfails);
    break;
  default:
    return tcp_close_shutdown_fin(pcb);
  }
  return ERR_OK;
}

static err_t
tcp_close_shutdown_fin(struct tcp_pcb *pcb)
{
  err_t err;

#if LWIP_ENABLE_UNILOG
  if(pcb == NULL) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_close_shutdown_fin_1, P_ERROR, 0, "pcb != NULL");
    return ERR_ARG;
  }
#else
  LWIP_ASSERT("pcb != NULL", pcb != NULL);
#endif

  switch (pcb->state) {
  case SYN_RCVD:
    err = tcp_send_fin(pcb);
    if (err == ERR_OK) {
      tcp_backlog_accepted(pcb);
      MIB2_STATS_INC(mib2.tcpattemptfails);
      pcb->state = FIN_WAIT_1;
#if LWIP_TIMER_ON_DEMOND
      //deactive SYNCRCV timer
      if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_close_shutdown_fin_2, P_INFO, 0, "remove tcp sync rcv timer");
      }
#endif
    }
    break;
  case ESTABLISHED:
    err = tcp_send_fin(pcb);
    if (err == ERR_OK) {
      MIB2_STATS_INC(mib2.tcpestabresets);
      pcb->state = FIN_WAIT_1;
    }
#if LWIP_TIMER_ON_DEMOND
#if LWIP_TCP_KEEPALIVE
    //deactive keepalive timer
    if (ip_get_option(pcb, SOF_KEEPALIVE)) {
        if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT)) {
            sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT], (void *)pcb);
            tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_close_shutdown_fin_3, P_INFO, 0, "remove tcp keepalive timer");
        }
    }
#endif

    //deactive delay ack timer
    if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_close_shutdown_fin_4, P_INFO, 0, "remove tcp delay ack timer");
    }

#endif
    break;
  case CLOSE_WAIT:
    err = tcp_send_fin(pcb);
    if (err == ERR_OK) {
      MIB2_STATS_INC(mib2.tcpestabresets);
      pcb->state = LAST_ACK;
#if LWIP_TIMER_ON_DEMOND
      //active LAST_ACK timer
      if(!tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_LASTACK_TIMEOUT)) {
        sys_timeout(2 * TCP_MSL, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_LASTACK_TIMEOUT], (void *)pcb);
        tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_LASTACK_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_close_shutdown_fin_5, P_INFO, 1, "enable tcp last ack timer %u", 2 * TCP_MSL);
      }
#endif
    }
    break;
  default:
    /* Has already been closed, do nothing. */
    return ERR_OK;
  }

  if (err == ERR_OK) {
    /* To ensure all data has been sent when tcp_close returns, we have
       to make sure tcp_output doesn't fail.
       Since we don't really have to ensure all data has been sent when tcp_close
       returns (unsent data is sent from tcp timer functions, also), we don't care
       for the return value of tcp_output for now. */
    tcp_output(pcb);
  } else if (err == ERR_MEM) {
    /* Mark this pcb for closing. Closing is retried from tcp_tmr. */
    pcb->flags |= TF_CLOSEPEND;
#if LWIP_TIMER_ON_DEMOND
     //active pending fin timer
     if(!tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_PENDING_FIN)) {
        sys_timeout(TCP_TMR_INTERVAL*2, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_PENDING_FIN], (void *)pcb);
        tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_PENDING_FIN);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_close_shutdown_fin_6, P_INFO, 1, "enable tcp pending fin timer %u", TCP_TMR_INTERVAL*2);
     }
#endif
    /* We have to return ERR_OK from here to indicate to the callers that this
       pcb should not be used any more as it will be freed soon via tcp_tmr.
       This is OK here since sending FIN does not guarantee a time frime for
       actually freeing the pcb, either (it is left in closure states for
       remote ACK or timeout) */
    return ERR_OK;
  }
  return err;
}

/**
 * @ingroup tcp_raw
 * Closes the connection held by the PCB.
 *
 * Listening pcbs are freed and may not be referenced any more.
 * Connection pcbs are freed if not yet connected and may not be referenced
 * any more. If a connection is established (at least SYN received or in
 * a closing state), the connection is closed, and put in a closing state.
 * The pcb is then automatically freed in tcp_slowtmr(). It is therefore
 * unsafe to reference it (unless an error is returned).
 *
 * @param pcb the tcp_pcb to close
 * @return ERR_OK if connection has been closed
 *         another err_t if closing failed and pcb is not freed
 */
err_t
tcp_close(struct tcp_pcb *pcb)
{
#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_close_1, P_INFO, 0, "tcp_close: closing in");
#else
  LWIP_DEBUGF(TCP_DEBUG, ("tcp_close: closing in "));
#endif
  tcp_debug_print_state(pcb->state);

  if (pcb->state != LISTEN) {
    /* Set a flag not to receive any more data... */
    pcb->flags |= TF_RXCLOSED;
  }
  /* ... and close */
  return tcp_close_shutdown(pcb, 1);
}

/**
 * @ingroup tcp_raw
 * Causes all or part of a full-duplex connection of this PCB to be shut down.
 * This doesn't deallocate the PCB unless shutting down both sides!
 * Shutting down both sides is the same as calling tcp_close, so if it succeds
 * (i.e. returns ER_OK), the PCB must not be referenced any more!
 *
 * @param pcb PCB to shutdown
 * @param shut_rx shut down receive side if this is != 0
 * @param shut_tx shut down send side if this is != 0
 * @return ERR_OK if shutdown succeeded (or the PCB has already been shut down)
 *         another err_t on error.
 */
err_t
tcp_shutdown(struct tcp_pcb *pcb, int shut_rx, int shut_tx)
{
  if (pcb->state == LISTEN) {
    return ERR_CONN;
  }
  if (shut_rx) {
    /* shut down the receive side: set a flag not to receive any more data... */
    pcb->flags |= TF_RXCLOSED;
    if (shut_tx) {
      /* shutting down the tx AND rx side is the same as closing for the raw API */
      return tcp_close_shutdown(pcb, 1);
    }
    /* ... and free buffered data */
    if (pcb->refused_data != NULL) {
#if LWIP_TIMER_ON_DEMOND
      //deactive refuse data timer
      if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_shutdown_1, P_INFO, 0, "remove tcp refuse data timer");
      }
#endif
      pbuf_free(pcb->refused_data);
      pcb->refused_data = NULL;
    }
  }
  if (shut_tx) {
    /* This can't happen twice since if it succeeds, the pcb's state is changed.
       Only close in these states as the others directly deallocate the PCB */
    switch (pcb->state) {
    case SYN_RCVD:
    case ESTABLISHED:
    case CLOSE_WAIT:
      return tcp_close_shutdown(pcb, (u8_t)shut_rx);
    default:
      /* Not (yet?) connected, cannot shutdown the TX side as that would bring us
        into CLOSED state, where the PCB is deallocated. */
      return ERR_CONN;
    }
  }
  return ERR_OK;
}

/**
 * Abandons a connection and optionally sends a RST to the remote
 * host.  Deletes the local protocol control block. This is done when
 * a connection is killed because of shortage of memory.
 *
 * @param pcb the tcp_pcb to abort
 * @param reset boolean to indicate whether a reset should be sent
 */
void
tcp_abandon(struct tcp_pcb *pcb, int reset)
{
  u32_t seqno, ackno;
#if LWIP_CALLBACK_API
  tcp_err_fn errf;
#endif /* LWIP_CALLBACK_API */
  void *errf_arg;

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
  if(pcb->pcb_hib_sleep2_mode_flag != PCB_HIB_DISABLE_DEACTIVE) {
     ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_abandon_12, P_SIG, 0, "tcp_abandon THE hib/sleep2 tcp context pcb will abandon");
     tcp_disable_hib_sleep2_mode(pcb, PCB_HIB_DISABLE_DEACTIVE);
     tcp_set_hib_sleep2_pcb_list(NULL);
  }
#endif


  /* pcb->state LISTEN not allowed here */
#if LWIP_ENABLE_UNILOG
  if(pcb->state == LISTEN) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_abandon_1, P_ERROR, 0, "don't call tcp_abort/tcp_abandon for listen-pcbs");
  }
#else
  LWIP_ASSERT("don't call tcp_abort/tcp_abandon for listen-pcbs",
    pcb->state != LISTEN);
#endif
  /* Figure out on which TCP PCB list we are, and remove us. If we
     are in an active state, call the receive function associated with
     the PCB with a NULL argument, and send an RST to the remote end. */
  if (pcb->state == TIME_WAIT) {
#if LWIP_TIMER_ON_DEMOND
    //deactive TIME_WAIT timer
    if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_abandon_2, P_INFO, 0, "remove time wait timer");
    }
#endif
    tcp_pcb_remove(&tcp_tw_pcbs, pcb);
    memp_free(MEMP_TCP_PCB, pcb);
  } else {
    int send_rst = 0;
    u16_t local_port = 0;
    enum tcp_state last_state;
    seqno = pcb->snd_nxt;
    ackno = pcb->rcv_nxt;
#if LWIP_CALLBACK_API
    errf = pcb->errf;
#endif /* LWIP_CALLBACK_API */
    errf_arg = pcb->callback_arg;
    if (pcb->state == CLOSED) {
      if (pcb->local_port != 0) {
        /* bound, not yet opened */
        TCP_RMV(&tcp_bound_pcbs, pcb);
      }
    } else {
      send_rst = reset;
      local_port = pcb->local_port;
      TCP_PCB_REMOVE_ACTIVE(pcb);
    }

#if LWIP_TIMER_ON_DEMOND
    if (pcb->state == ESTABLISHED) {
#if LWIP_TCP_KEEPALIVE
      //deactive keepalive timer
      if (ip_get_option(pcb, SOF_KEEPALIVE)) {
        if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT)) {
            sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT], (void *)pcb);
            tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_abandon_3, P_INFO, 0, "remove tcp keepalive timer");
        }
      }
#endif
    }else if (pcb->state == SYN_RCVD) {
        //deactive SYNCRCV timer
        if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT)) {
            sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT], (void *)pcb);
            tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_abandon_4, P_INFO, 0, "remove tcp syncrcv timer");
        }
    }else if (pcb->state == LAST_ACK) {
        //deactive LAST_ACK timer
        if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_LASTACK_TIMEOUT)) {
            sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_LASTACK_TIMEOUT], (void *)pcb);
            tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_LASTACK_TIMEOUT);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_abandon_5, P_INFO, 0, "remove tcp last ack timer");
        }
    }else if (pcb->state == FIN_WAIT_2) {
        //deactive fin_wait2 timer
        if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT)) {
            sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT], (void *)pcb);
            tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_abandon_6, P_INFO, 0, "remove tcp FIN_WAIT2 timer");
        }
    }

    //deactive tcp total retry timer
    if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_abandon_tcp_total_retry_time_1, P_INFO, 0, "remove tcp total retry timer");
    }

    //deactive tcp retry timer
    if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_RETRY], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_abandon_tcp_retry_time_1, P_INFO, 1, "remove tcp retry timer, pcb 0x%x", (void *)pcb);
    }


#endif

    if (pcb->unacked != NULL) {
          //indicate TCP UL sequence state
#if ENABLE_PSIF
          tcp_send_unack_ul_state(pcb);
#endif

      tcp_segs_free(pcb->unacked);
    }

    if (pcb->unsent != NULL) {
      tcp_segs_free(pcb->unsent);
    }

#if TCP_QUEUE_OOSEQ
    if (pcb->ooseq != NULL) {
      tcp_segs_free(pcb->ooseq);
    }
#endif /* TCP_QUEUE_OOSEQ */
    tcp_backlog_accepted(pcb);
    if (send_rst) {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_abandon_11, P_INFO, 0, "tcp_abandon: sending RST");
#else
      LWIP_DEBUGF(TCP_RST_DEBUG, ("tcp_abandon: sending RST\n"));
#endif
      tcp_rst(seqno, ackno, &pcb->local_ip, &pcb->remote_ip, local_port, pcb->remote_port);
    }
    last_state = pcb->state;
    memp_free(MEMP_TCP_PCB, pcb);
    TCP_EVENT_ERR(last_state, errf, errf_arg, ERR_ABRT);
  }
}

/**
 * @ingroup tcp_raw
 * Aborts the connection by sending a RST (reset) segment to the remote
 * host. The pcb is deallocated. This function never fails.
 *
 * ATTENTION: When calling this from one of the TCP callbacks, make
 * sure you always return ERR_ABRT (and never return ERR_ABRT otherwise
 * or you will risk accessing deallocated memory or memory leaks!
 *
 * @param pcb the tcp pcb to abort
 */
void
tcp_abort(struct tcp_pcb *pcb)
{
  tcp_abandon(pcb, 1);
}

/**
 * @ingroup tcp_raw
 * Binds the connection to a local port number and IP address. If the
 * IP address is not given (i.e., ipaddr == NULL), the IP address of
 * the outgoing network interface is used instead.
 *
 * @param pcb the tcp_pcb to bind (no check is done whether this pcb is
 *        already bound!)
 * @param ipaddr the local ip address to bind to (use IP4_ADDR_ANY to bind
 *        to any local address
 * @param port the local port to bind to
 * @return ERR_USE if the port is already in use
 *         ERR_VAL if bind failed because the PCB is not in a valid state
 *         ERR_OK if bound
 */
err_t
tcp_bind(struct tcp_pcb *pcb, const ip_addr_t *ipaddr, u16_t port)
{
  int i;
  int max_pcb_list = NUM_TCP_PCB_LISTS;
  struct tcp_pcb *cpcb;

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
  if(pcb->state != CLOSED) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_bind_1, P_ERROR, 0, "tcp_bind: can only bind in state CLOSED");
    return ERR_VAL;
  }
#else
  LWIP_ERROR("tcp_bind: can only bind in state CLOSED", pcb->state == CLOSED, return ERR_VAL);
#endif

#if SO_REUSE
  /* Unless the REUSEADDR flag is set,
     we have to check the pcbs in TIME-WAIT state, also.
     We do not dump TIME_WAIT pcb's; they can still be matched by incoming
     packets using both local and remote IP addresses and ports to distinguish.
   */
  if (ip_get_option(pcb, SOF_REUSEADDR)) {
    max_pcb_list = NUM_TCP_PCB_LISTS_NO_TIME_WAIT;
  }
#endif /* SO_REUSE */

  if (port == 0) {
    port = tcp_new_port();
    if (port == 0) {
      return ERR_BUF;
    }
  } else {
    /* Check if the address already is in use (on all lists) */
    for (i = 0; i < max_pcb_list; i++) {
      for (cpcb = *tcp_pcb_lists[i]; cpcb != NULL; cpcb = cpcb->next) {
        if (cpcb->local_port == port) {
#if SO_REUSE
          /* Omit checking for the same port if both pcbs have REUSEADDR set.
             For SO_REUSEADDR, the duplicate-check for a 5-tuple is done in
             tcp_connect. */
          if (!ip_get_option(pcb, SOF_REUSEADDR) ||
              !ip_get_option(cpcb, SOF_REUSEADDR))
#endif /* SO_REUSE */
          {
            /* @todo: check accept_any_ip_version */
            if ((IP_IS_V6(ipaddr) == IP_IS_V6_VAL(cpcb->local_ip)) &&
                (ip_addr_isany(&cpcb->local_ip) ||
                ip_addr_isany(ipaddr) ||
                ip_addr_cmp(&cpcb->local_ip, ipaddr))) {
              return ERR_USE;
            }
          }
        }
      }
    }
  }

  if (!ip_addr_isany(ipaddr)) {
    ip_addr_set(&pcb->local_ip, ipaddr);
  }
  pcb->local_port = port;
  TCP_REG(&tcp_bound_pcbs, pcb);
#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_bind_2, P_INFO, 1, "tcp_bind: bind to port %u", port);
#else
  LWIP_DEBUGF(TCP_DEBUG, ("tcp_bind: bind to port %"U16_F"\n", port));
#endif
  return ERR_OK;
}
#if LWIP_CALLBACK_API
/**
 * Default accept callback if no accept callback is specified by the user.
 */
static err_t
tcp_accept_null(void *arg, struct tcp_pcb *pcb, err_t err)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);

  tcp_abort(pcb);

  return ERR_ABRT;
}
#endif /* LWIP_CALLBACK_API */

/**
 * @ingroup tcp_raw
 * Set the state of the connection to be LISTEN, which means that it
 * is able to accept incoming connections. The protocol control block
 * is reallocated in order to consume less memory. Setting the
 * connection to LISTEN is an irreversible process.
 *
 * @param pcb the original tcp_pcb
 * @param backlog the incoming connections queue limit
 * @return tcp_pcb used for listening, consumes less memory.
 *
 * @note The original tcp_pcb is freed. This function therefore has to be
 *       called like this:
 *             tpcb = tcp_listen_with_backlog(tpcb, backlog);
 */
struct tcp_pcb *
tcp_listen_with_backlog(struct tcp_pcb *pcb, u8_t backlog)
{
  return tcp_listen_with_backlog_and_err(pcb, backlog, NULL);
}

/**
 * @ingroup tcp_raw
 * Set the state of the connection to be LISTEN, which means that it
 * is able to accept incoming connections. The protocol control block
 * is reallocated in order to consume less memory. Setting the
 * connection to LISTEN is an irreversible process.
 *
 * @param pcb the original tcp_pcb
 * @param backlog the incoming connections queue limit
 * @param err when NULL is returned, this contains the error reason
 * @return tcp_pcb used for listening, consumes less memory.
 *
 * @note The original tcp_pcb is freed. This function therefore has to be
 *       called like this:
 *             tpcb = tcp_listen_with_backlog_and_err(tpcb, backlog, &err);
 */
struct tcp_pcb *
tcp_listen_with_backlog_and_err(struct tcp_pcb *pcb, u8_t backlog, err_t *err)
{
  struct tcp_pcb_listen *lpcb = NULL;
  err_t res;

  LWIP_UNUSED_ARG(backlog);
#if LWIP_ENABLE_UNILOG
  if(pcb->state != CLOSED) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_listen_with_backlog_and_err_1, P_ERROR, 0, "tcp_listen: pcb already connected");
    res = ERR_CLSD;
    goto done;
  }
#else
  LWIP_ERROR("tcp_listen: pcb already connected", pcb->state == CLOSED, res = ERR_CLSD; goto done);
#endif

  /* already listening? */
  if (pcb->state == LISTEN) {
    lpcb = (struct tcp_pcb_listen*)pcb;
    res = ERR_ALREADY;
    goto done;
  }
#if SO_REUSE
  if (ip_get_option(pcb, SOF_REUSEADDR)) {
    /* Since SOF_REUSEADDR allows reusing a local address before the pcb's usage
       is declared (listen-/connection-pcb), we have to make sure now that
       this port is only used once for every local IP. */
    for (lpcb = tcp_listen_pcbs.listen_pcbs; lpcb != NULL; lpcb = lpcb->next) {
      if ((lpcb->local_port == pcb->local_port) &&
          ip_addr_cmp(&lpcb->local_ip, &pcb->local_ip)) {
        /* this address/port is already used */
        lpcb = NULL;
        res = ERR_USE;
        goto done;
      }
    }
  }
#endif /* SO_REUSE */
  lpcb = (struct tcp_pcb_listen *)memp_malloc(MEMP_TCP_PCB_LISTEN);
  if (lpcb == NULL) {
    res = ERR_MEM;
    goto done;
  }
  lpcb->callback_arg = pcb->callback_arg;
  lpcb->local_port = pcb->local_port;
  lpcb->state = LISTEN;
  lpcb->prio = pcb->prio;
  lpcb->so_options = pcb->so_options;
  lpcb->ttl = pcb->ttl;
  lpcb->tos = pcb->tos;
#if LWIP_IPV4 && LWIP_IPV6
  IP_SET_TYPE_VAL(lpcb->remote_ip, pcb->local_ip.type);
#endif /* LWIP_IPV4 && LWIP_IPV6 */
  ip_addr_copy(lpcb->local_ip, pcb->local_ip);
  if (pcb->local_port != 0) {
    TCP_RMV(&tcp_bound_pcbs, pcb);
  }
  memp_free(MEMP_TCP_PCB, pcb);
#if LWIP_CALLBACK_API
  lpcb->accept = tcp_accept_null;
#endif /* LWIP_CALLBACK_API */
#if TCP_LISTEN_BACKLOG
  lpcb->accepts_pending = 0;
  tcp_backlog_set(lpcb, backlog);
#endif /* TCP_LISTEN_BACKLOG */
  TCP_REG(&tcp_listen_pcbs.pcbs, (struct tcp_pcb *)lpcb);
  res = ERR_OK;
done:
  if (err != NULL) {
    *err = res;
  }
  return (struct tcp_pcb *)lpcb;
}

/**
 * Update the state that tracks the available window space to advertise.
 *
 * Returns how much extra window would be advertised if we sent an
 * update now.
 */
u32_t
tcp_update_rcv_ann_wnd(struct tcp_pcb *pcb)
{
  u32_t new_right_edge = pcb->rcv_nxt + pcb->rcv_wnd;

  if (TCP_SEQ_GEQ(new_right_edge, pcb->rcv_ann_right_edge + LWIP_MIN((TCP_WND / 2), pcb->mss))) {
    /* we can advertise more window */
    pcb->rcv_ann_wnd = pcb->rcv_wnd;
    return new_right_edge - pcb->rcv_ann_right_edge;
  } else {
    if (TCP_SEQ_GT(pcb->rcv_nxt, pcb->rcv_ann_right_edge)) {
      /* Can happen due to other end sending out of advertised window,
       * but within actual available (but not yet advertised) window */
      pcb->rcv_ann_wnd = 0;
    } else {
      /* keep the right edge of window constant */
      u32_t new_rcv_ann_wnd = pcb->rcv_ann_right_edge - pcb->rcv_nxt;
#if !LWIP_WND_SCALE
#if LWIP_ENABLE_UNILOG
      if(new_rcv_ann_wnd > 0xffff) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_update_rcv_ann_wnd_1, P_ERROR, 0, "new_rcv_ann_wnd <= 0xffff");
      }
#else
      LWIP_ASSERT("new_rcv_ann_wnd <= 0xffff", new_rcv_ann_wnd <= 0xffff);
#endif
#endif
      pcb->rcv_ann_wnd = (tcpwnd_size_t)new_rcv_ann_wnd;
    }
    return 0;
  }
}

/**
 * @ingroup tcp_raw
 * This function should be called by the application when it has
 * processed the data. The purpose is to advertise a larger window
 * when the data has been processed.
 *
 * @param pcb the tcp_pcb for which data is read
 * @param len the amount of bytes that have been read by the application
 */
void
tcp_recved(struct tcp_pcb *pcb, u16_t len)
{
  int wnd_inflation;

  /* pcb->state LISTEN not allowed here */
#if LWIP_ENABLE_UNILOG
  if(pcb->state == LISTEN) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_recved_1, P_ERROR, 0, "don't call tcp_recved for listen-pcbs");
  }
#else
  LWIP_ASSERT("don't call tcp_recved for listen-pcbs",
    pcb->state != LISTEN);
#endif

  pcb->rcv_wnd += len;
  if (pcb->rcv_wnd > TCP_WND_MAX(pcb)) {
    pcb->rcv_wnd = TCP_WND_MAX(pcb);
  } else if (pcb->rcv_wnd == 0) {
    /* rcv_wnd overflowed */
    if ((pcb->state == CLOSE_WAIT) || (pcb->state == LAST_ACK)) {
      /* In passive close, we allow this, since the FIN bit is added to rcv_wnd
         by the stack itself, since it is not mandatory for an application
         to call tcp_recved() for the FIN bit, but e.g. the netconn API does so. */
      pcb->rcv_wnd = TCP_WND_MAX(pcb);
    } else {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_recved_2, P_ERROR, 0, "tcp_recved: len wrapped rcv_wnd");
#else
      LWIP_ASSERT("tcp_recved: len wrapped rcv_wnd\n", 0);
#endif
    }
  }

  wnd_inflation = tcp_update_rcv_ann_wnd(pcb);

  /* If the change in the right edge of window is significant (default
   * watermark is TCP_WND/4), then send an explicit update now.
   * Otherwise wait for a packet to be sent in the normal course of
   * events (or more window to be available later) */
  if (wnd_inflation >= TCP_WND_UPDATE_THRESHOLD) {
    tcp_ack_now(pcb);
    tcp_output(pcb);
  }

#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_recved_3, P_INFO, 3, "tcp_recved: received(%u) bytes,wnd (%u)%u ", len, pcb->rcv_wnd, (u16_t)(TCP_WND_MAX(pcb) - pcb->rcv_wnd));
#else
  LWIP_DEBUGF(TCP_DEBUG, ("tcp_recved: received %"U16_F" bytes, wnd %"TCPWNDSIZE_F" (%"TCPWNDSIZE_F").\n",
         len, pcb->rcv_wnd, (u16_t)(TCP_WND_MAX(pcb) - pcb->rcv_wnd)));
#endif
}

/**
 * Allocate a new local TCP port.
 *
 * @return a new (free) local TCP port number
 */
static u16_t
tcp_new_port(void)
{
  u8_t i;
  u16_t n = 0;
  struct tcp_pcb *pcb;

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
   u16_t hib_tcp_pcb_active_port = get_hib_tcp_pcb_active_local_port();
#endif


again:
  if (tcp_port++ == TCP_LOCAL_PORT_RANGE_END) {
    tcp_port = TCP_LOCAL_PORT_RANGE_START;
  }
  /* Check all PCB lists. */
  for (i = 0; i < NUM_TCP_PCB_LISTS; i++) {
    for (pcb = *tcp_pcb_lists[i]; pcb != NULL; pcb = pcb->next) {
      if (pcb->local_port == tcp_port) {
        if (++n > (TCP_LOCAL_PORT_RANGE_END - TCP_LOCAL_PORT_RANGE_START)) {
          return 0;
        }
        goto again;
      }
#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
    if(hib_tcp_pcb_active_port == tcp_port) {
        goto again;
    }
#endif
    }
  }
  return tcp_port;
}

/**
 * @ingroup tcp_raw
 * Connects to another host. The function given as the "connected"
 * argument will be called when the connection has been established.
 *
 * @param pcb the tcp_pcb used to establish the connection
 * @param ipaddr the remote ip address to connect to
 * @param port the remote tcp port to connect to
 * @param connected callback function to call when connected (on error,
                    the err calback will be called)
 * @return ERR_VAL if invalid arguments are given
 *         ERR_OK if connect request has been sent
 *         other err_t values if connect request couldn't be sent
 */
err_t
tcp_connect(struct tcp_pcb *pcb, const ip_addr_t *ipaddr, u16_t port,
      tcp_connected_fn connected)
{
  err_t ret;
  u32_t iss;
  u16_t old_local_port;

  if ((pcb == NULL) || (ipaddr == NULL)) {
    return ERR_VAL;
  }

#if LWIP_ENABLE_UNILOG
  if(pcb->state != CLOSED) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_connect_1, P_ERROR, 0, "tcp_connect: can only connect from state CLOSED");
    return ERR_ISCONN;
  }
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_connect_2, P_INFO, 1, "tcp_connect to port %u", port);
#else
  LWIP_ERROR("tcp_connect: can only connect from state CLOSED", pcb->state == CLOSED, return ERR_ISCONN);

  LWIP_DEBUGF(TCP_DEBUG, ("tcp_connect to port %"U16_F"\n", port));
#endif
  ip_addr_set(&pcb->remote_ip, ipaddr);
  pcb->remote_port = port;

  /* check if we have a route to the remote host */
    /* no local IP address set, yet. */
    struct netif *netif;
    const ip_addr_t *local_ip;
    ip_route_get_local_ip(&pcb->local_ip, &pcb->remote_ip, netif, local_ip);
    if ((netif == NULL) || (local_ip == NULL)) {
      /* Don't even try to send a SYN packet if we have no route
         since that will fail. */
      return ERR_RTE;
    }
    /* Use the address as local address of the pcb. */
  if (ip_addr_isany(&pcb->local_ip)) {
    ip_addr_copy(pcb->local_ip, *local_ip);
  }

  old_local_port = pcb->local_port;
  if (pcb->local_port == 0) {
    pcb->local_port = tcp_new_port();
    if (pcb->local_port == 0) {
      return ERR_BUF;
    }
  } else {
#if SO_REUSE
    if (ip_get_option(pcb, SOF_REUSEADDR)) {
      /* Since SOF_REUSEADDR allows reusing a local address, we have to make sure
         now that the 5-tuple is unique. */
      struct tcp_pcb *cpcb;
      int i;
      /* Don't check listen- and bound-PCBs, check active- and TIME-WAIT PCBs. */
      for (i = 2; i < NUM_TCP_PCB_LISTS; i++) {
        for (cpcb = *tcp_pcb_lists[i]; cpcb != NULL; cpcb = cpcb->next) {
          if ((cpcb->local_port == pcb->local_port) &&
              (cpcb->remote_port == port) &&
              ip_addr_cmp(&cpcb->local_ip, &pcb->local_ip) &&
              ip_addr_cmp(&cpcb->remote_ip, ipaddr)) {
            /* linux returns EISCONN here, but ERR_USE should be OK for us */
            return ERR_USE;
          }
        }
      }
    }
#endif /* SO_REUSE */
  }

#if LWIP_CALLBACK_API
    pcb->connected = connected;
#else /* LWIP_CALLBACK_API */
    LWIP_UNUSED_ARG(connected);
#endif /* LWIP_CALLBACK_API */

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
    // if the tcp pcb state has established, triger the connected callback
  if(pcb->pcb_hib_sleep2_mode_flag == PCB_HIB_ENABLE_ACTIVE) {
    pcb->state = ESTABLISHED;
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_connect_3, P_SIG, 0, "tcp_connect: pcb has connected");
    TCP_REG_ACTIVE(pcb);
    MIB2_STATS_INC(mib2.tcpactiveopens);
    return ERR_ISCONN;
  }

#endif


  iss = tcp_next_iss(pcb);
  pcb->rcv_nxt = 0;
  pcb->snd_nxt = iss;
  pcb->lastack = iss - 1;
  pcb->snd_wl2 = iss - 1;
  pcb->snd_lbb = iss - 1;
  /* Start with a window that does not need scaling. When window scaling is
     enabled and used, the window is enlarged when both sides agree on scaling. */
  pcb->rcv_wnd = pcb->rcv_ann_wnd = TCPWND_MIN16(TCP_WND);
  pcb->rcv_ann_right_edge = pcb->rcv_nxt;
  pcb->snd_wnd = TCP_WND;
  /* As initial send MSS, we use TCP_MSS but limit it to 536.
     The send MSS is updated when an MSS option is received. */
  pcb->mss = INITIAL_MSS;
#if TCP_CALCULATE_EFF_SEND_MSS
  pcb->mss = tcp_eff_send_mss(pcb->mss, &pcb->local_ip, &pcb->remote_ip);
#endif /* TCP_CALCULATE_EFF_SEND_MSS */
  pcb->cwnd = 1;


  /* Send a SYN together with the MSS option. */
  ret = tcp_enqueue_flags(pcb, TCP_SYN);
  if (ret == ERR_OK) {
    /* SYN segment was enqueued, changed the pcbs state now */
    pcb->state = SYN_SENT;
    if (old_local_port != 0) {
      TCP_RMV(&tcp_bound_pcbs, pcb);
    }
    TCP_REG_ACTIVE(pcb);
    MIB2_STATS_INC(mib2.tcpactiveopens);

    tcp_output(pcb);
  }
  return ret;
}

/**
 * Called every 500 ms and implements the retransmission timer and the timer that
 * removes PCBs that have been in TIME-WAIT for enough time. It also increments
 * various timers such as the inactivity timer in each PCB.
 *
 * Automatically called from tcp_tmr().
 */
void
tcp_slowtmr(void)
{
  struct tcp_pcb *pcb, *prev;
  tcpwnd_size_t eff_wnd;
  u8_t pcb_remove;      /* flag if a PCB should be removed */
  u8_t pcb_reset;       /* flag if a RST should be sent when removing */
  err_t err;

  err = ERR_OK;

  ++tcp_ticks;
  ++tcp_timer_ctr;

tcp_slowtmr_start:
  /* Steps through all of the active PCBs. */
  prev = NULL;
  pcb = tcp_active_pcbs;
  if (pcb == NULL) {
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_1, P_INFO, 0, "tcp_slowtmr: no active pcbs");
#else
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_slowtmr: no active pcbs\n"));
#endif
  }
  while (pcb != NULL) {
#if LWIP_ENABLE_UNILOG
    if(pcb->state == CLOSED || pcb->state == LISTEN || pcb->state == TIME_WAIT) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_2, P_ERROR, 0, "tcp_slowtmr: active pcb->state inlavid");
    }
#else
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_slowtmr: processing active pcb\n"));
    LWIP_ASSERT("tcp_slowtmr: active pcb->state != CLOSED\n", pcb->state != CLOSED);
    LWIP_ASSERT("tcp_slowtmr: active pcb->state != LISTEN\n", pcb->state != LISTEN);
    LWIP_ASSERT("tcp_slowtmr: active pcb->state != TIME-WAIT\n", pcb->state != TIME_WAIT);
#endif
    if (pcb->last_timer == tcp_timer_ctr) {
      /* skip this pcb, we have already processed it */
      pcb = pcb->next;
      continue;
    }
    pcb->last_timer = tcp_timer_ctr;

    pcb_remove = 0;
    pcb_reset = 0;

    if (pcb->state == SYN_SENT && pcb->nrtx >= TCP_SYNMAXRTX) {
      ++pcb_remove;
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_3, P_INFO, 0, "tcp_slowtmr: max SYN retries reached");
#else
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_slowtmr: max SYN retries reached\n"));
#endif
    }
    else if (pcb->nrtx >= TCP_MAXRTX) {
      ++pcb_remove;
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_4, P_INFO, 0, "tcp_slowtmr: max DATA retries reached");
#else
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_slowtmr: max DATA retries reached\n"));
#endif
    } else {
      if (pcb->persist_backoff > 0) {
        /* If snd_wnd is zero, use persist timer to send 1 byte probes
         * instead of using the standard retransmission mechanism. */
        u8_t backoff_cnt = tcp_persist_backoff[pcb->persist_backoff-1];
        if (pcb->persist_cnt < backoff_cnt) {
          pcb->persist_cnt++;
        }
        if (pcb->persist_cnt >= backoff_cnt) {
          if (tcp_zero_window_probe(pcb, backoff_cnt * TCP_SLOW_INTERVAL) == ERR_OK) {
            pcb->persist_cnt = 0;
            if (pcb->persist_backoff < sizeof(tcp_persist_backoff)) {
              pcb->persist_backoff++;
            }
          }
        }
      } else {
        /* Increase the retransmission timer if it is running */
        if (pcb->rtime >= 0) {
          ++pcb->rtime;
        }

        if (pcb->unacked != NULL && pcb->rtime >= pcb->rto) {
          /* Time for a retransmission. */
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_5, P_INFO, 2, "tcp_slowtmr: rtime %u pcb->rto %u", pcb->rtime, pcb->rto);
#else
          LWIP_DEBUGF(TCP_RTO_DEBUG, ("tcp_slowtmr: rtime %"S16_F
                                      " pcb->rto %"S16_F"\n",
                                      pcb->rtime, pcb->rto));
#endif

          /* Double retransmission time-out unless we are trying to
           * connect to somebody (i.e., we are in SYN_SENT). */
          if (pcb->state != SYN_SENT) {
            u8_t backoff_idx = LWIP_MIN(pcb->nrtx, sizeof(tcp_backoff)-1);
            pcb->rto = ((pcb->sa >> 3) + pcb->sv) << tcp_backoff[backoff_idx];
          }

          /* Reset the retransmission timer. */
          pcb->rtime = 0;

          /* Reduce congestion window and ssthresh. */
          eff_wnd = LWIP_MIN(pcb->cwnd, pcb->snd_wnd);
          pcb->ssthresh = eff_wnd >> 1;
          if (pcb->ssthresh < (tcpwnd_size_t)(pcb->mss << 1)) {
            pcb->ssthresh = (pcb->mss << 1);
          }
          pcb->cwnd = pcb->mss;
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_6, P_INFO, 2, "tcp_slowtmr: cwnd %u ssthresh %u", pcb->cwnd, pcb->ssthresh);
#else
          LWIP_DEBUGF(TCP_CWND_DEBUG, ("tcp_slowtmr: cwnd %"TCPWNDSIZE_F
                                       " ssthresh %"TCPWNDSIZE_F"\n",
                                       pcb->cwnd, pcb->ssthresh));
#endif

          /* The following needs to be called AFTER cwnd is set to one
             mss - STJ */
          tcp_rexmit_rto(pcb);
        }
      }
    }
    /* Check if this PCB has stayed too long in FIN-WAIT-2 */
    if (pcb->state == FIN_WAIT_2) {
      /* If this PCB is in FIN_WAIT_2 because of SHUT_WR don't let it time out. */
      if (pcb->flags & TF_RXCLOSED) {
        /* PCB was fully closed (either through close() or SHUT_RDWR):
           normal FIN-WAIT timeout handling. */
        if ((u32_t)(tcp_ticks - pcb->tmr) >
            TCP_FIN_WAIT_TIMEOUT / TCP_SLOW_INTERVAL) {
          ++pcb_remove;
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_7, P_INFO, 0, "tcp_slowtmr: removing pcb stuck in FIN-WAIT-2");
#else
          LWIP_DEBUGF(TCP_DEBUG, ("tcp_slowtmr: removing pcb stuck in FIN-WAIT-2\n"));
#endif
        }
      }
    }

    /* Check if KEEPALIVE should be sent */
    if (ip_get_option(pcb, SOF_KEEPALIVE) &&
       ((pcb->state == ESTABLISHED) ||
        (pcb->state == CLOSE_WAIT))) {
      if ((u32_t)(tcp_ticks - pcb->tmr) >
         (pcb->keep_idle + TCP_KEEP_DUR(pcb)) / TCP_SLOW_INTERVAL)
      {
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_8, P_INFO, 0, "tcp_slowtmr: KEEPALIVE timeout. Aborting connection to ");
#else
        LWIP_DEBUGF(TCP_DEBUG, ("tcp_slowtmr: KEEPALIVE timeout. Aborting connection to "));
#endif
        ip_addr_debug_print(TCP_DEBUG, &pcb->remote_ip);

#ifndef LWIP_ENABLE_UNILOG
        LWIP_DEBUGF(TCP_DEBUG, ("\n"));
#endif

        ++pcb_remove;
        ++pcb_reset;
      } else if ((u32_t)(tcp_ticks - pcb->tmr) >
                (pcb->keep_idle + pcb->keep_cnt_sent * TCP_KEEP_INTVL(pcb))
                / TCP_SLOW_INTERVAL)
      {
        err = tcp_keepalive(pcb);
        if (err == ERR_OK) {
          pcb->keep_cnt_sent++;
        }
      }
    }

    /* If this PCB has queued out of sequence data, but has been
       inactive for too long, will drop the data (it will eventually
       be retransmitted). */
#if TCP_QUEUE_OOSEQ
    if (pcb->ooseq != NULL &&
        (u32_t)tcp_ticks - pcb->tmr >= pcb->rto * TCP_OOSEQ_TIMEOUT) {
      tcp_segs_free(pcb->ooseq);
      pcb->ooseq = NULL;
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_9, P_INFO, 0, "tcp_slowtmr: dropping OOSEQ queued data");
#else
      LWIP_DEBUGF(TCP_CWND_DEBUG, ("tcp_slowtmr: dropping OOSEQ queued data\n"));
#endif
    }
#endif /* TCP_QUEUE_OOSEQ */

    /* Check if this PCB has stayed too long in SYN-RCVD */
    if (pcb->state == SYN_RCVD) {
      if ((u32_t)(tcp_ticks - pcb->tmr) >
          TCP_SYN_RCVD_TIMEOUT / TCP_SLOW_INTERVAL) {
        ++pcb_remove;
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_10, P_INFO, 0, "tcp_slowtmr: removing pcb stuck in SYN-RCVD");
#else
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_slowtmr: removing pcb stuck in SYN-RCVD\n"));
#endif
      }
    }

    /* Check if this PCB has stayed too long in LAST-ACK */
    if (pcb->state == LAST_ACK) {
      if ((u32_t)(tcp_ticks - pcb->tmr) > 2 * TCP_MSL / TCP_SLOW_INTERVAL) {
        ++pcb_remove;
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_11, P_INFO, 0, "tcp_slowtmr: removing pcb stuck in LAST-ACK");
#else
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_slowtmr: removing pcb stuck in LAST-ACK\n"));
#endif
      }
    }

    /* If the PCB should be removed, do it. */
    if (pcb_remove) {
      struct tcp_pcb *pcb2;
#if LWIP_CALLBACK_API
      tcp_err_fn err_fn = pcb->errf;
#endif /* LWIP_CALLBACK_API */
      void *err_arg;
      enum tcp_state last_state;
      tcp_pcb_purge(pcb);
      /* Remove PCB from tcp_active_pcbs list. */
      if (prev != NULL) {
#if LWIP_ENABLE_UNILOG
        if(pcb == tcp_active_pcbs) {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_12, P_ERROR, 0, "tcp_slowtmr: middle tcp != tcp_active_pcbs");
        }
#else
        LWIP_ASSERT("tcp_slowtmr: middle tcp != tcp_active_pcbs", pcb != tcp_active_pcbs);
#endif
        prev->next = pcb->next;
      } else {
        /* This PCB was the first. */
#if LWIP_ENABLE_UNILOG
        if(tcp_active_pcbs != pcb) {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_13, P_ERROR, 0, "tcp_slowtmr: first pcb == tcp_active_pcbs");
        }
#else
        LWIP_ASSERT("tcp_slowtmr: first pcb == tcp_active_pcbs", tcp_active_pcbs == pcb);
#endif
        tcp_active_pcbs = pcb->next;
      }

      if (pcb_reset) {
        tcp_rst(pcb->snd_nxt, pcb->rcv_nxt, &pcb->local_ip, &pcb->remote_ip,
                 pcb->local_port, pcb->remote_port);
      }

      err_arg = pcb->callback_arg;
      last_state = pcb->state;
      pcb2 = pcb;
      pcb = pcb->next;
      memp_free(MEMP_TCP_PCB, pcb2);

      tcp_active_pcbs_changed = 0;
      TCP_EVENT_ERR(last_state, err_fn, err_arg, ERR_ABRT);
      if (tcp_active_pcbs_changed) {
        goto tcp_slowtmr_start;
      }
    } else {
      /* get the 'next' element now and work with 'prev' below (in case of abort) */
      prev = pcb;
      pcb = pcb->next;

      /* We check if we should poll the connection. */
      ++prev->polltmr;
      if (prev->polltmr >= prev->pollinterval) {
        prev->polltmr = 0;
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_14, P_INFO, 0, "tcp_slowtmr: polling application");
#else
        LWIP_DEBUGF(TCP_DEBUG, ("tcp_slowtmr: polling application\n"));
#endif
        tcp_active_pcbs_changed = 0;
        TCP_EVENT_POLL(prev, err);
        if (tcp_active_pcbs_changed) {
          goto tcp_slowtmr_start;
        }
        /* if err == ERR_ABRT, 'prev' is already deallocated */
        if (err == ERR_OK) {
          tcp_output(prev);
        }
      }
    }
  }


  /* Steps through all of the TIME-WAIT PCBs. */
  prev = NULL;
  pcb = tcp_tw_pcbs;
  while (pcb != NULL) {
 #if LWIP_ENABLE_UNILOG
    if(pcb->state != TIME_WAIT) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_15, P_ERROR, 0, "tcp_slowtmr: TIME-WAIT pcb->state == TIME-WAIT");
    }
#else
    LWIP_ASSERT("tcp_slowtmr: TIME-WAIT pcb->state == TIME-WAIT", pcb->state == TIME_WAIT);
#endif
    pcb_remove = 0;

    /* Check if this PCB has stayed long enough in TIME-WAIT */
    if ((u32_t)(tcp_ticks - pcb->tmr) > 2 * TCP_MSL / TCP_SLOW_INTERVAL) {
      ++pcb_remove;
    }

    /* If the PCB should be removed, do it. */
    if (pcb_remove) {
      struct tcp_pcb *pcb2;
      tcp_pcb_purge(pcb);
      /* Remove PCB from tcp_tw_pcbs list. */
      if (prev != NULL) {
#if LWIP_ENABLE_UNILOG
        if(pcb == tcp_tw_pcbs) {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_16, P_ERROR, 0, "tcp_slowtmr: middle tcp != tcp_tw_pcbs");
        }
#else
        LWIP_ASSERT("tcp_slowtmr: middle tcp != tcp_tw_pcbs", pcb != tcp_tw_pcbs);
#endif
        prev->next = pcb->next;
      } else {
        /* This PCB was the first. */
#if LWIP_ENABLE_UNILOG
        if(tcp_tw_pcbs != pcb) {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_slowtmr_17, P_ERROR, 0, "tcp_slowtmr: first pcb == tcp_tw_pcbs");
        }
#else
        LWIP_ASSERT("tcp_slowtmr: first pcb == tcp_tw_pcbs", tcp_tw_pcbs == pcb);
#endif
        tcp_tw_pcbs = pcb->next;
      }
      pcb2 = pcb;
      pcb = pcb->next;
      memp_free(MEMP_TCP_PCB, pcb2);
    } else {
      prev = pcb;
      pcb = pcb->next;
    }
  }
}

/**
 * Is called every TCP_FAST_INTERVAL (250 ms) and process data previously
 * "refused" by upper layer (application) and sends delayed ACKs.
 *
 * Automatically called from tcp_tmr().
 */
void
tcp_fasttmr(void)
{
  struct tcp_pcb *pcb;

  ++tcp_timer_ctr;

tcp_fasttmr_start:
  pcb = tcp_active_pcbs;

  while (pcb != NULL) {
    if (pcb->last_timer != tcp_timer_ctr) {
      struct tcp_pcb *next;
      pcb->last_timer = tcp_timer_ctr;
      /* send delayed ACKs */
      if (pcb->flags & TF_ACK_DELAY) {
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_fasttmr_1, P_INFO, 0, "tcp_fasttmr: delayed ACK");
#else
        LWIP_DEBUGF(TCP_DEBUG, ("tcp_fasttmr: delayed ACK\n"));
#endif
        tcp_ack_now(pcb);
        tcp_output(pcb);
        pcb->flags &= ~(TF_ACK_DELAY | TF_ACK_NOW);
      }
      /* send pending FIN */
      if (pcb->flags & TF_CLOSEPEND) {
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_fasttmr_2, P_INFO, 0, "tcp_fasttmr: pending FIN");
#else
        LWIP_DEBUGF(TCP_DEBUG, ("tcp_fasttmr: pending FIN\n"));
#endif
        pcb->flags &= ~(TF_CLOSEPEND);
        tcp_close_shutdown_fin(pcb);
      }

      next = pcb->next;

      /* If there is data which was previously "refused" by upper layer */
      if (pcb->refused_data != NULL) {
        tcp_active_pcbs_changed = 0;
        tcp_process_refused_data(pcb);
        if (tcp_active_pcbs_changed) {
          /* application callback has changed the pcb list: restart the loop */
          goto tcp_fasttmr_start;
        }
      }
      pcb = next;
    } else {
      pcb = pcb->next;
    }
  }
}


#if LWIP_TIMER_ON_DEMOND

void tcp_enable_timer_active_mask(struct tcp_pcb *pcb, u8_t type)
{
    if(pcb == NULL) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_enable_timer_active_mask_1, P_ERROR, 0, "invalid argument");
        return;
    }

    pcb->pcb_timer_active_mask |= 1<<type;
}

void tcp_disable_timer_active_mask(struct tcp_pcb *pcb, u8_t type)
{
    if(pcb == NULL) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_disable_timer_active_mask_1, P_ERROR, 0, "invalid argument");
        return;
    }

    pcb->pcb_timer_active_mask &= ~(1<<type);
}

u8_t tcp_get_timer_active_state(struct tcp_pcb *pcb, u8_t type)
{
    if(pcb != NULL) {
        if(pcb->pcb_timer_active_mask & 1 << type) {
            return 1;
        }
    }
    return 0;
}


void tcp_remove_pcb(struct tcp_pcb *pcb, struct tcp_pcb **pcb_list, u8_t reset_flag)
{

      if(pcb == NULL || pcb_list == NULL) {
        return;
      }

#if LWIP_CALLBACK_API
      tcp_err_fn err_fn = pcb->errf;
#endif /* LWIP_CALLBACK_API */
      void *err_arg;
      enum tcp_state last_state;

      tcp_pcb_purge(pcb);

      TCP_RMV(pcb_list, pcb);

      if (reset_flag) {
        tcp_rst(pcb->snd_nxt, pcb->rcv_nxt, &pcb->local_ip, &pcb->remote_ip,
                 pcb->local_port, pcb->remote_port);
      }

      err_arg = pcb->callback_arg;
      last_state = pcb->state;
      memp_free(MEMP_TCP_PCB, pcb);

      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_remove_pcb_1, P_INFO, 2, "tcp_remove_pcb remove pcb 0x%x from list 0x%x", pcb, *pcb_list);

      TCP_EVENT_ERR(last_state, err_fn, err_arg, ERR_ABRT);

}

void tcp_delay_ack_handler(void *arg)
{
    struct tcp_pcb *pcb;

    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_delay_ack_handler_1, P_INFO, 0, "tcp_delay_ack_handler timeout");
      //first check argument
    if(arg == NULL ) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_delay_ack_handler_2, P_ERROR, 0, "invalid argument");
        return;
    }

    pcb = (struct tcp_pcb *)arg;

     //remove timer active state first
    tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK);

    if (pcb->flags & TF_ACK_DELAY) {
        tcp_ack_now(pcb);
        tcp_output(pcb);
        pcb->flags &= ~(TF_ACK_DELAY | TF_ACK_NOW);
    }

}

void tcp_pending_fin_handler(void *arg)
{
    struct tcp_pcb *pcb;

    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pending_fin_handler_1, P_INFO, 0, "tcp_pending_fin_handler timeout");
      //first check argument
    if(arg == NULL ) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pending_fin_handler_2, P_ERROR, 0, "invalid argument");
        return;
    }

    pcb = (struct tcp_pcb *)arg;

     //remove timer active state first
    tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_PENDING_FIN);

      /* send pending FIN */
    if (pcb->flags & TF_CLOSEPEND) {
        pcb->flags &= ~(TF_CLOSEPEND);
        tcp_close_shutdown_fin(pcb);
    }

}

void tcp_refused_data_handler(void *arg)
{
    struct tcp_pcb *pcb;

    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_refused_data_handler_1, P_INFO, 0, "tcp_refused_data_handler timeout");
      //first check argument
    if(arg == NULL ) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_refused_data_handler_2, P_ERROR, 0, "invalid argument");
        return;
    }

    pcb = (struct tcp_pcb *)arg;

     //remove timer active state first
    tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA);

      /* If there is data which was previously "refused" by upper layer */
    if (pcb->refused_data != NULL) {
        tcp_process_refused_data(pcb);
    }

}

void tcp_retry_timeout_handler(void *arg)
{
    struct tcp_pcb *pcb;
    u8_t pcb_remove = 0;      /* flag if a PCB should be removed */
    tcpwnd_size_t eff_wnd;

    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_retry_timeout_handler_1, P_INFO, 0, "tcp_retry_timeout_handler timeout");
      //first check argument
    if(arg == NULL ) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_retry_timeout_handler_2, P_ERROR, 0, "invalid argument");
        return;
    }

    pcb = (struct tcp_pcb *)arg;

    //remove timer active state first
    tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY);

    //check active pcb state
    if(pcb->state == CLOSED || pcb->state == LISTEN || pcb->state == TIME_WAIT) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_retry_timeout_handler_3, P_ERROR, 0, "tcp_retry_timeout_handler: active pcb->state inlavid");
        return;
    }

    if (pcb->state == SYN_SENT && pcb->nrtx >= TCP_SYNMAXRTX) {
      ++pcb_remove;
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_retry_timeout_handler_4, P_ERROR, 0, "tcp_retry_timeout_handler: max SYN retries reached");
#else
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_retry_timeout_handler: max SYN retries reached\n"));
#endif
    }
    else if (pcb->nrtx >= pcb->tcp_max_retry_times) {
      ++pcb_remove;
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_retry_timeout_handler_5, P_INFO, 0, "tcp_retry_timeout_handler: max DATA retries reached");
#else
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_retry_timeout_handler: max DATA retries reached\n"));
#endif
    } else {
      if (pcb->persist_backoff > 0) {
        /* If snd_wnd is zero, use persist timer to send 1 byte probes
         * instead of using the standard retransmission mechanism. */
        if (tcp_zero_window_probe(pcb, tcp_persist_backoff[pcb->persist_backoff-1]) == ERR_OK) {
          pcb->persist_cnt = 0;
          if (pcb->persist_backoff < sizeof(tcp_persist_backoff)) {
            pcb->persist_backoff++;
            sys_timeout(tcp_persist_backoff[pcb->persist_backoff-1] * TCP_SLOW_INTERVAL, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_RETRY], (void *)pcb);
            tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_retry_timeout_handler_6, P_INFO, 2, "active tcp retry timer %u, pcb 0x%x", tcp_persist_backoff[pcb->persist_backoff-1] * TCP_SLOW_INTERVAL, (void *)pcb);
          }
        }
      } else {
        if (pcb->unacked != NULL) {
          /* Time for a retransmission. */
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_retry_timeout_handler_7, P_INFO, 4, "tcp_retry_timeout_handler:  pcb->rto %u nrtx %u sa %d sv %d", pcb->rto, pcb->nrtx, pcb->sa, pcb->sv);
#else
          LWIP_DEBUGF(TCP_RTO_DEBUG, ("tcp_retry_timeout_handler: rtime %"S16_F
                                      " pcb->rto %"S16_F"\n",
                                      pcb->rtime, pcb->rto));
#endif

          /* Double retransmission time-out unless we are trying to
           * connect to somebody (i.e., we are in SYN_SENT). */
          if (pcb->state != SYN_SENT) {
            u8_t backoff_idx = LWIP_MIN(pcb->nrtx, sizeof(tcp_backoff)-1);
            pcb->rto = ((pcb->sa >> 3) + pcb->sv) << tcp_backoff[backoff_idx];
          }

          /* Reduce congestion window and ssthresh. */
          eff_wnd = LWIP_MIN(pcb->cwnd, pcb->snd_wnd);
          pcb->ssthresh = eff_wnd >> 1;
          if (pcb->ssthresh < (tcpwnd_size_t)(pcb->mss << 1)) {
            pcb->ssthresh = (pcb->mss << 1);
          }
          pcb->cwnd = pcb->mss;
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_retry_timeout_handler_8, P_INFO, 2, "tcp_retry_timeout_handler: cwnd %u ssthresh %u", pcb->cwnd, pcb->ssthresh);
#else
          LWIP_DEBUGF(TCP_CWND_DEBUG, ("tcp_retry_timeout_handler: cwnd %"TCPWNDSIZE_F
                                       " ssthresh %"TCPWNDSIZE_F"\n",
                                       pcb->cwnd, pcb->ssthresh));
#endif

          /* The following needs to be called AFTER cwnd is set to one
             mss - STJ */
          tcp_rexmit_rto(pcb);
        }
      }
    }

    if(pcb_remove) {
        tcp_remove_pcb(pcb, &tcp_active_pcbs, 0);
    }

}


void tcp_total_retry_timeout_handler(void *arg)
{
    struct tcp_pcb *pcb;

    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_total_retry_timeout_handler_1, P_INFO, 0, "tcp_total_retry_timeout_handler timeout");
      //first check argument
    if(arg == NULL ) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_total_retry_timeout_handler_2, P_ERROR, 0, "invalid argument");
        return;
    }

    pcb = (struct tcp_pcb *)arg;

    //remove timer active state first
    tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT);

    //check active pcb state
    if(pcb->state == SYN_RCVD || pcb->state == FIN_WAIT_2 || pcb->state == LAST_ACK || pcb->state == TIME_WAIT) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_total_retry_timeout_handler_3, P_ERROR, 0, "tcp_total_retry_timeout_handler: active pcb->state inlavid");
        return;
    }

    tcp_remove_pcb(pcb, &tcp_active_pcbs, 0);

}


void tcp_fin_wait2_timeout_handler(void *arg)
{
    struct tcp_pcb *pcb;

    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_fin_wait2_timeout_handler_1, P_INFO, 0, "tcp_fin_wait2_timeout_handler timeout");
      //first check argument
    if(arg == NULL ) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_fin_wait2_timeout_handler_2, P_ERROR, 0, "invalid argument");
        return;
    }

    pcb = (struct tcp_pcb *)arg;

    //remove timer active state first
    tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT);

    /* Check if this PCB has stayed too long in FIN-WAIT-2 */
    if (pcb->state == FIN_WAIT_2) {
       /* If this PCB is in FIN_WAIT_2 because of SHUT_WR don't let it time out. */
        if (pcb->flags & TF_RXCLOSED) {
        /* PCB was fully closed (either through close() or SHUT_RDWR):
            normal FIN-WAIT timeout handling. */
#if LWIP_ENABLE_UNILOG
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_fin_wait2_timeout_handler_3, P_INFO, 0, "tcp_fin_wait2_timeout_handler: removing pcb stuck in FIN-WAIT-2");
#else
            LWIP_DEBUGF(TCP_DEBUG, ("tcp_fin_wait2_timeout_handler: removing pcb stuck in FIN-WAIT-2\n"));
#endif
            tcp_remove_pcb(pcb, &tcp_active_pcbs, 0);
        }
    }

}


void tcp_keepalive_timeout_handler(void *arg)
{
    struct tcp_pcb *pcb;
    err_t err;

    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_keepalive_timeout_handler_1, P_INFO, 0, "tcp_keepalive_timeout_handler timeout");
      //first check argument
    if(arg == NULL ) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_keepalive_timeout_handler_2, P_ERROR, 0, "invalid argument");
        return;
    }

    pcb = (struct tcp_pcb *)arg;

    //remove timer active state first
    tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT);

    /* Check if KEEPALIVE should be sent */
    if (ip_get_option(pcb, SOF_KEEPALIVE) &&
       ((pcb->state == ESTABLISHED) ||
          (pcb->state == CLOSE_WAIT))) {
#if LWIP_TCP_KEEPALIVE
      if (pcb->keep_cnt_sent >= pcb->keep_cnt)
#else
      if (pcb->keep_cnt_sent >= TCP_KEEPCNT_DEFAULT)
#endif
      {
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_keepalive_timeout_handler_3, P_INFO, 0, "tcp_keepalive_timeout_handler: KEEPALIVE timeout. Aborting connection to ");
#else
        LWIP_DEBUGF(TCP_DEBUG, ("tcp_keepalive_timeout_handler: KEEPALIVE timeout. Aborting connection to "));
#endif
        ip_addr_debug_print(TCP_DEBUG, &pcb->remote_ip);

#ifndef LWIP_ENABLE_UNILOG
        LWIP_DEBUGF(TCP_DEBUG, ("\n"));
#endif
        tcp_remove_pcb(pcb, &tcp_active_pcbs, 1);

      } else {
        err = tcp_keepalive(pcb);
        if (err == ERR_OK) {
          pcb->keep_cnt_sent++;
        }
#if LWIP_TCP_KEEPALIVE
        sys_timeout(pcb->keep_intvl, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT], (void *)pcb);
        tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_keepalive_timeout_handler_4, P_INFO, 2, "active tcp keepalive timer %u, send cnt %u", pcb->keep_intvl, pcb->keep_cnt_sent);
#else
        sys_timeout(TCP_KEEPINTVL_DEFAULT, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT], (void *)pcb);
        tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_keepalive_timeout_handler_5, P_INFO, 2, "active tcp keepalive timer %u, send cnt %u", TCP_KEEPINTVL_DEFAULT, pcb->keep_cnt_sent);
#endif

      }
    }

}


void tcp_ooseq_timeout_handler(void *arg)
{
    struct tcp_pcb *pcb;

    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_ooseq_timeout_handler_1, P_INFO, 0, "tcp_ooseq_timeout_handler timeout");
      //first check argument
    if(arg == NULL ) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_ooseq_timeout_handler_2, P_ERROR, 0, "invalid argument");
        return;
    }

    pcb = (struct tcp_pcb *)arg;

    //remove timer active state first
    tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT);

#if TCP_QUEUE_OOSEQ
    if (pcb->ooseq != NULL ) {
      tcp_segs_free(pcb->ooseq);
      pcb->ooseq = NULL;
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_ooseq_timeout_handler_3, P_INFO, 0, "tcp_ooseq_timeout_handler: dropping OOSEQ queued data");
#else
      LWIP_DEBUGF(TCP_CWND_DEBUG, ("tcp_ooseq_timeout_handler: dropping OOSEQ queued data\n"));
#endif
    }
#endif /* TCP_QUEUE_OOSEQ */

}

void tcp_syncrcv_timeout_handler(void *arg)
{
    struct tcp_pcb *pcb;

    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_syncrcv_timeout_handler_1, P_INFO, 0, "tcp_syncrcv_timeout_handler timeout");
      //first check argument
    if(arg == NULL ) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_syncrcv_timeout_handler_2, P_ERROR, 0, "invalid argument");
        return;
    }

    pcb = (struct tcp_pcb *)arg;

    //remove timer active state first
    tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT);

    /* Check if this PCB has stayed too long in SYN-RCVD */
    if (pcb->state == SYN_RCVD) {

#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_syncrcv_timeout_handler_3, P_INFO, 0, "tcp_syncrcv_timeout_handler: removing pcb stuck in SYN-RCVD");
#else
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_syncrcv_timeout_handler: removing pcb stuck in SYN-RCVD\n"));
#endif

      tcp_remove_pcb(pcb, &tcp_active_pcbs, 0);

    }


}


void tcp_lastack_timeout_handler(void *arg)
{
    struct tcp_pcb *pcb;

    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_lastack_timeout_handler_1, P_INFO, 0, "tcp_lastack_timeout_handler timeout");
      //first check argument
    if(arg == NULL ) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_lastack_timeout_handler_2, P_ERROR, 0, "invalid argument");
        return;
    }

    pcb = (struct tcp_pcb *)arg;

    //remove timer active state first
    tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_LASTACK_TIMEOUT);

        /* Check if this PCB has stayed too long in LAST-ACK */
     if (pcb->state == LAST_ACK) {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_lastack_timeout_handler_3, P_INFO, 0, "tcp_lastack_timeout_handler: removing pcb stuck in LAST-ACK");
#else
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_lastack_timeout_handler: removing pcb stuck in LAST-ACK\n"));
#endif

      tcp_remove_pcb(pcb, &tcp_active_pcbs, 0);
    }



}


void tcp_timewait_timeout_handler(void *arg)
{
    struct tcp_pcb *pcb;

    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_timewait_timeout_handler_1, P_INFO, 0, "tcp_timewait_timeout_handler timeout");
      //first check argument
    if(arg == NULL ) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_timewait_timeout_handler_2, P_ERROR, 0, "invalid argument");
        return;
    }

    pcb = (struct tcp_pcb *)arg;

    //remove timer active state first
    tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT);

        /* Check if this PCB has stayed too long in LAST-ACK */
     if (pcb->state == TIME_WAIT) {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_timewait_timeout_handler_3, P_INFO, 0, "tcp_timewait_timeout_handler: removing pcb stuck in LAST-ACK");
#else
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_timewait_timeout_handler: removing pcb stuck in LAST-ACK\n"));
#endif

      tcp_remove_pcb(pcb, &tcp_tw_pcbs, 0);
    }



}

void tcp_remove_all_timer(struct tcp_pcb *pcb)
{

    if(pcb == NULL) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_remove_all_timer_1, P_INFO, 0, "tcp_remove_all_timer invalid pcb point");
        return;
    }

     if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT)){
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_remove_all_timer_12, P_INFO, 0, "remove tcp total retry timer");
     }

     if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY)){
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_RETRY], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_remove_all_timer_2, P_INFO, 1, "remove tcp retry timer, pcb 0x%x", (void *)pcb);
     }

        //deactive FIN_WAIT2 timer
     if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK)){
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_remove_all_timer_3, P_INFO, 0, "remove delay ack timer");
     }


     if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_PENDING_FIN)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_PENDING_FIN], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_PENDING_FIN);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_remove_all_timer_4, P_INFO, 0, "remove tcp pending fin timer");
     }

     if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_remove_all_timer_5, P_INFO, 0, "remove tcp refuse data timer");
     }

     if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_remove_all_timer_6, P_INFO, 0, "remove tcp fin wait2 timer");
     }

#if LWIP_TCP_KEEPALIVE
     if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_remove_all_timer_7, P_INFO, 0, "remove tcp keepalive timer");
     }
#endif

     if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_remove_all_timer_8, P_INFO, 0, "remove tcp ooseq timer");
     }

     if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_remove_all_timer_9, P_INFO, 0, "remove tcp syncrcv timer");
     }

     if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_LASTACK_TIMEOUT)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_LASTACK_TIMEOUT], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_LASTACK_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_remove_all_timer_10, P_INFO, 0, "remove tcp lastack timer");
     }

     if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_remove_all_timer_11, P_INFO, 0, "remove tcp timewait timer");
     }
}

#endif

#if ENABLE_PSIF
void tcp_rebuild_unack_seg_bitmap(struct tcp_seg *seg)
{
    u8_t sequence;
    struct tcp_seg *seg_tmp;

    if(seg == NULL) {
        return;
    }

    for(sequence = 1; sequence <= 255; sequence ++) {

        if(getSequenceBitmap(seg->sequence_state, sequence)) {
            for(seg_tmp = seg->next; seg_tmp != NULL; seg_tmp = seg_tmp->next) {
                updateSequenceBitmap(seg_tmp->sequence_state, sequence, 0);
            }
        }

        if(sequence == 255) {
            break;
        }
    }
}


void tcp_send_unack_ul_state(struct tcp_pcb *pcb)
{
  struct tcp_seg *seg;

  if(pcb == NULL) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_send_unack_ul_state_1, P_ERROR, 0, "tcp_send_unack_ul_state:pcb invalid");
    return;
  }

  if(pcb->sockid < 0) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_send_unack_ul_state_2, P_ERROR, 0, "tcp_send_unack_ul_state: the pcb socketid is invalid");
    return;
  }

  seg = pcb->unacked;


  while (seg != NULL) {
    struct tcp_seg *next = seg->next;

  //rebuild. because the next segments maybe include the same sequence
    tcp_rebuild_unack_seg_bitmap(seg);

    udp_send_ul_state_ind(seg->sequence_state, pcb->sockid, UL_SEQUENCE_STATE_DISCARD);
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_send_unack_ul_state_3, P_INFO, 1, "tcp_send_unack_ul_state: UL sequence state indicate success, the pcb socket id %u", pcb->sockid);
    seg = next;
  }
}

err_t tcp_set_max_retry_times(struct tcp_pcb *pcb, u8_t times)
{
  if(pcb == NULL) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_set_max_retry_times_1, P_ERROR, 0, "tcp_set_max_retry_times:pcb invalid");
    return ERR_VAL;
  }

  if(times >= TCP_MINRTX && times <= TCP_MAXRTX) {
    pcb->tcp_max_retry_times = times;
  }else {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_set_max_retry_times_2, P_ERROR, 1, "tcp_set_max_retry_times:invalid tcp max retry times %u", times);
    return ERR_VAL;
  }
  return ERR_OK;
}

err_t tcp_set_max_total_retry_time(struct tcp_pcb *pcb, u16_t time)
{
  if(pcb == NULL) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_set_max_total_retry_time_1, P_ERROR, 0, "tcp_set_max_total_retry_time:pcb invalid");
    return ERR_VAL;
  }

  if(time >= TCP_MINRTX_TOTAL_TIME && time <= TCP_MAXRTX_TOTAL_TIME) {
    pcb->tcp_max_total_retry_time = time;
  }else {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_set_max_total_retry_time_2, P_ERROR, 0, "tcp_set_max_total_retry_time:invalid tcp max total retry time %u", time);
    return ERR_VAL;
  }
  return ERR_OK;
}


#endif

#if ENABLE_PSIF
void tcp_ack(struct tcp_pcb *pcb)
{
    if(pcb == NULL)
    {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_ack_1, P_WARNING, 0, "tcp_ack:pcb invalid");
        return;
    }

    if(pcb->flags & TF_ACK_DELAY) {
      pcb->flags &= ~TF_ACK_DELAY;
      pcb->flags |= TF_ACK_NOW;
    }
    else {
#if LWIP_TIMER_ON_DEMOND
    //active delay ack timer
  if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK)) {
     ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_ack_2, P_INFO, 1, "tcp delay ack timer has enable");
  }else{
     sys_timeout(TCP_DELAY_ACK_TIMER, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK], (void *)pcb);
     tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK);
     ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_ack_3, P_INFO, 1, "enable tcp delay ack timer %u", TCP_DELAY_ACK_TIMER);
  }
#endif
      pcb->flags |= TF_ACK_DELAY;

    }

    return;
}

#endif


/** Call tcp_output for all active pcbs that have TF_NAGLEMEMERR set */
void
tcp_txnow(void)
{
  struct tcp_pcb *pcb;

  for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
    if (pcb->flags & TF_NAGLEMEMERR) {
      tcp_output(pcb);
    }
  }
}

/** Pass pcb->refused_data to the recv callback */
err_t
tcp_process_refused_data(struct tcp_pcb *pcb)
{
#if TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
  struct pbuf *rest;
  while (pcb->refused_data != NULL)
#endif /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
  {
    err_t err;
    u8_t refused_flags = pcb->refused_data->flags;
    /* set pcb->refused_data to NULL in case the callback frees it and then
       closes the pcb */
    struct pbuf *refused_data = pcb->refused_data;
#if TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
    pbuf_split_64k(refused_data, &rest);
    pcb->refused_data = rest;
#else /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
    pcb->refused_data = NULL;
#endif /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
    /* Notify again application with data previously received. */
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_refused_data_1, P_INFO, 0, "tcp_input: notify kept packet");
#else
    LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_input: notify kept packet\n"));
#endif
    TCP_EVENT_RECV(pcb, refused_data, ERR_OK, err);
    if (err == ERR_OK) {
      /* did refused_data include a FIN? */
      if (refused_flags & PBUF_FLAG_TCP_FIN
#if TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
          && (rest == NULL)
#endif /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
         ) {
        /* correct rcv_wnd as the application won't call tcp_recved()
           for the FIN's seqno */
        if (pcb->rcv_wnd != TCP_WND_MAX(pcb)) {
          pcb->rcv_wnd++;
        }
        TCP_EVENT_CLOSED(pcb, err);
        if (err == ERR_ABRT) {
          return ERR_ABRT;
        }
      }
    } else if (err == ERR_ABRT) {
      /* if err == ERR_ABRT, 'pcb' is already deallocated */
      /* Drop incoming packets because pcb is "full" (only if the incoming
         segment contains data). */
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_refused_data_2, P_INFO, 0, "tcp_input: drop incoming packets, because pcb is full");
#else
      LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_input: drop incoming packets, because pcb is \"full\"\n"));
#endif
      return ERR_ABRT;
    } else {
      /* data is still refused, pbuf is still valid (go on for ACK-only packets) */
#if TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
      if (rest != NULL) {
        pbuf_cat(refused_data, rest);
      }
#endif /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
      pcb->refused_data = refused_data;
#if LWIP_TIMER_ON_DEMOND
      //active refuse data timer
      if(!tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA)) {
        sys_timeout(TCP_TMR_INTERVAL*4, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA], (void *)pcb);
        tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_refused_data_3, P_INFO, 1, "enable tcp refse data timer %u", TCP_TMR_INTERVAL*4);
      }
#endif
      return ERR_INPROGRESS;
    }
  }
  return ERR_OK;
}

/**
 * Deallocates a list of TCP segments (tcp_seg structures).
 *
 * @param seg tcp_seg list of TCP segments to free
 */
void
tcp_segs_free(struct tcp_seg *seg)
{
  while (seg != NULL) {
    struct tcp_seg *next = seg->next;
    tcp_seg_free(seg);
    seg = next;
  }
}

/**
 * Frees a TCP segment (tcp_seg structure).
 *
 * @param seg single tcp_seg to free
 */
void
tcp_seg_free(struct tcp_seg *seg)
{
  if (seg != NULL) {
    if (seg->p != NULL) {
      pbuf_free(seg->p);
#if TCP_DEBUG
      seg->p = NULL;
#endif /* TCP_DEBUG */
    }
    memp_free(MEMP_TCP_SEG, seg);
  }
}

/**
 * Sets the priority of a connection.
 *
 * @param pcb the tcp_pcb to manipulate
 * @param prio new priority
 */
void
tcp_setprio(struct tcp_pcb *pcb, u8_t prio)
{
  pcb->prio = prio;
}

#if TCP_QUEUE_OOSEQ
/**
 * Returns a copy of the given TCP segment.
 * The pbuf and data are not copied, only the pointers
 *
 * @param seg the old tcp_seg
 * @return a copy of seg
 */
struct tcp_seg *
tcp_seg_copy(struct tcp_seg *seg)
{
  struct tcp_seg *cseg;

  cseg = (struct tcp_seg *)memp_malloc(MEMP_TCP_SEG);
  if (cseg == NULL) {
    return NULL;
  }
  SMEMCPY((u8_t *)cseg, (const u8_t *)seg, sizeof(struct tcp_seg));
  pbuf_ref(cseg->p);
  return cseg;
}
#endif /* TCP_QUEUE_OOSEQ */

#if LWIP_CALLBACK_API
/**
 * Default receive callback that is called if the user didn't register
 * a recv callback for the pcb.
 */
err_t
tcp_recv_null(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
  LWIP_UNUSED_ARG(arg);
  if (p != NULL) {
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);
  } else if (err == ERR_OK) {
    return tcp_close(pcb);
  }
  return ERR_OK;
}
#endif /* LWIP_CALLBACK_API */

/**
 * Kills the oldest active connection that has the same or lower priority than
 * 'prio'.
 *
 * @param prio minimum priority
 */
static void
tcp_kill_prio(u8_t prio)
{
  struct tcp_pcb *pcb, *inactive;
  u32_t inactivity;
  u8_t mprio;

#if LWIP_TIMER_ON_DEMOND
  u32_t now = sys_now();
#endif

  mprio = LWIP_MIN(TCP_PRIO_MAX, prio);

  /* We kill the oldest active connection that has lower priority than prio. */
  inactivity = 0;
  inactive = NULL;
  for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
    if (pcb->prio <= mprio &&
#if LWIP_TIMER_ON_DEMOND
        (u32_t)(now - pcb->tmr) >= inactivity) {
        inactivity = now - pcb->tmr;
#else
       (u32_t)(tcp_ticks - pcb->tmr) >= inactivity) {
      inactivity = tcp_ticks - pcb->tmr;
#endif
      inactive = pcb;
      mprio = pcb->prio;
    }
  }
  if (inactive != NULL) {

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_kill_prio_1, P_INFO, 2, "tcp_kill_prio: killing oldest PCB 0x%x (%u)", (void *)inactive, inactivity);
#else
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_kill_prio: killing oldest PCB %p (%"S32_F")\n",
           (void *)inactive, inactivity));
#endif
    tcp_abort(inactive);
  }
}

/**
 * Kills the oldest connection that is in specific state.
 * Called from tcp_alloc() for LAST_ACK and CLOSING if no more connections are available.
 */
static void
tcp_kill_state(enum tcp_state state)
{
  struct tcp_pcb *pcb, *inactive;
  u32_t inactivity;

#if LWIP_TIMER_ON_DEMOND
  u32_t now = sys_now();
#endif

#if LWIP_ENABLE_UNILOG

  if((state != CLOSING) && (state != LAST_ACK)) {
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_kill_state_1, P_INFO, 0, "tcp_kill_state invalid state");
  }
#else
  LWIP_ASSERT("invalid state", (state == CLOSING) || (state == LAST_ACK));
#endif

  inactivity = 0;
  inactive = NULL;
  /* Go through the list of active pcbs and get the oldest pcb that is in state
     CLOSING/LAST_ACK. */
  for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
    if (pcb->state == state) {
#if LWIP_TIMER_ON_DEMOND
        if((u32_t)(now - pcb->tmr) >= inactivity) {
        inactivity = now - pcb->tmr;
#else
      if ((u32_t)(tcp_ticks - pcb->tmr) >= inactivity) {
        inactivity = tcp_ticks - pcb->tmr;
#endif
        inactive = pcb;
      }
    }
  }
  if (inactive != NULL) {

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_kill_state_2, P_INFO, 3, "tcp_kill_closing: killing oldest %d PCB 0x%x (%u)", state, (void *)inactive, inactivity);
#else
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_kill_closing: killing oldest %s PCB %p (%"S32_F")\n",
           tcp_state_str[state], (void *)inactive, inactivity));
#endif
    /* Don't send a RST, since no data is lost. */
    tcp_abandon(inactive, 0);
  }
}

/**
 * Kills the oldest connection that is in TIME_WAIT state.
 * Called from tcp_alloc() if no more connections are available.
 */
static void
tcp_kill_timewait(void)
{
  struct tcp_pcb *pcb, *inactive;
  u32_t inactivity;

#if LWIP_TIMER_ON_DEMOND
  u32_t now = sys_now();
#endif

  inactivity = 0;
  inactive = NULL;
  /* Go through the list of TIME_WAIT pcbs and get the oldest pcb. */
  for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
#if LWIP_TIMER_ON_DEMOND
        if((u32_t)(now - pcb->tmr) >= inactivity) {
        inactivity = now - pcb->tmr;
#else
    if ((u32_t)(tcp_ticks - pcb->tmr) >= inactivity) {
      inactivity = tcp_ticks - pcb->tmr;
#endif
      inactive = pcb;
    }
  }
  if (inactive != NULL) {

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_kill_timewait_1, P_INFO, 2, "tcp_kill_timewait: killing oldest TIME-WAIT PCB 0x%x (%u)", (void *)inactive, inactivity);
#else
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_kill_timewait: killing oldest TIME-WAIT PCB %p (%"S32_F")\n",
           (void *)inactive, inactivity));
#endif
    tcp_abort(inactive);
  }
}

/**
 * Allocate a new tcp_pcb structure.
 *
 * @param prio priority for the new pcb
 * @return a new tcp_pcb that initially is in state CLOSED
 */
struct tcp_pcb *
tcp_alloc(u8_t prio)
{
  struct tcp_pcb *pcb;

  pcb = (struct tcp_pcb *)memp_malloc(MEMP_TCP_PCB);
  if (pcb == NULL) {
    /* Try killing oldest connection in TIME-WAIT. */

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_alloc_1, P_INFO, 0, "tcp_alloc: killing off oldest TIME-WAIT connection");
#else
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_alloc: killing off oldest TIME-WAIT connection\n"));
#endif
    tcp_kill_timewait();
    /* Try to allocate a tcp_pcb again. */
    pcb = (struct tcp_pcb *)memp_malloc(MEMP_TCP_PCB);
    if (pcb == NULL) {
      /* Try killing oldest connection in LAST-ACK (these wouldn't go to TIME-WAIT). */

#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_alloc_2, P_INFO, 0, "tcp_alloc: killing off oldest LAST-ACK connection");
#else
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_alloc: killing off oldest LAST-ACK connection\n"));
#endif
      tcp_kill_state(LAST_ACK);
      /* Try to allocate a tcp_pcb again. */
      pcb = (struct tcp_pcb *)memp_malloc(MEMP_TCP_PCB);
      if (pcb == NULL) {
        /* Try killing oldest connection in CLOSING. */

#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_alloc_3, P_INFO, 0, "tcp_alloc: killing off oldest CLOSING connection");
#else
        LWIP_DEBUGF(TCP_DEBUG, ("tcp_alloc: killing off oldest CLOSING connection\n"));
#endif
        tcp_kill_state(CLOSING);
        /* Try to allocate a tcp_pcb again. */
        pcb = (struct tcp_pcb *)memp_malloc(MEMP_TCP_PCB);
        if (pcb == NULL) {
          /* Try killing active connections with lower priority than the new one. */

#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_alloc_4, P_INFO, 1, "tcp_alloc: killing connection with prio lower than %d", prio);
#else
          LWIP_DEBUGF(TCP_DEBUG, ("tcp_alloc: killing connection with prio lower than %d\n", prio));
#endif

          tcp_kill_prio(prio);
          /* Try to allocate a tcp_pcb again. */
          pcb = (struct tcp_pcb *)memp_malloc(MEMP_TCP_PCB);
          if (pcb != NULL) {
            /* adjust err stats: memp_malloc failed multiple times before */
            MEMP_STATS_DEC(err, MEMP_TCP_PCB);
          }
        }
        if (pcb != NULL) {
          /* adjust err stats: memp_malloc failed multiple times before */
          MEMP_STATS_DEC(err, MEMP_TCP_PCB);
        }
      }
      if (pcb != NULL) {
        /* adjust err stats: memp_malloc failed multiple times before */
        MEMP_STATS_DEC(err, MEMP_TCP_PCB);
      }
    }
    if (pcb != NULL) {
      /* adjust err stats: memp_malloc failed above */
      MEMP_STATS_DEC(err, MEMP_TCP_PCB);
    }
  }
  if (pcb != NULL) {
    /* zero out the whole pcb, so there is no need to initialize members to zero */
    memset(pcb, 0, sizeof(struct tcp_pcb));
    pcb->prio = prio;
    pcb->snd_buf = TCP_SND_BUF;
    /* Start with a window that does not need scaling. When window scaling is
       enabled and used, the window is enlarged when both sides agree on scaling. */
    pcb->rcv_wnd = pcb->rcv_ann_wnd = TCPWND_MIN16(TCP_WND);
    pcb->ttl = TCP_TTL;
    /* As initial send MSS, we use TCP_MSS but limit it to 536.
       The send MSS is updated when an MSS option is received. */
    pcb->mss = INITIAL_MSS;

    //add by xwang
#if ENABLE_PSIF
    uint16_t current_delay;

    current_delay = PsifGetCurrentPacketDelay();

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_alloc_5, P_INFO, 1, "tcp_alloc: current UE packet delay %u", current_delay);
#else
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_alloc: current UE packet delay %u \n", current_delay));
#endif

    if(current_delay) {
        if(((current_delay * 2) / TCP_SLOW_INTERVAL) > 0) {
            pcb->rto = (current_delay * 2) / TCP_SLOW_INTERVAL;
        }else {
            pcb->rto = 1;
        }
    }else {
        pcb->rto = 3000 / TCP_SLOW_INTERVAL;
    }
#else
    pcb->rto = 3000 / TCP_SLOW_INTERVAL;
#endif
    pcb->sv = 3000 / TCP_SLOW_INTERVAL;
    pcb->rtime = -1;
    pcb->cwnd = 1;
#if LWIP_TIMER_ON_DEMOND
    pcb->tmr = sys_now();
    pcb->last_timer = sys_now();
#else
    pcb->tmr = tcp_ticks;
    pcb->last_timer = tcp_timer_ctr;
#endif

    /* RFC 5681 recommends setting ssthresh abritrarily high and gives an example
    of using the largest advertised receive window.  We've seen complications with
    receiving TCPs that use window scaling and/or window auto-tuning where the
    initial advertised window is very small and then grows rapidly once the
    connection is established. To avoid these complications, we set ssthresh to the
    largest effective cwnd (amount of in-flight data) that the sender can have. */
    pcb->ssthresh = TCP_SND_BUF;

#if LWIP_CALLBACK_API
    pcb->recv = tcp_recv_null;
#endif /* LWIP_CALLBACK_API */

    /* Init KEEPALIVE timer */
    pcb->keep_idle  = TCP_KEEPIDLE_DEFAULT;

#if LWIP_TCP_KEEPALIVE
    pcb->keep_intvl = TCP_KEEPINTVL_DEFAULT;
    pcb->keep_cnt   = TCP_KEEPCNT_DEFAULT;
#endif /* LWIP_TCP_KEEPALIVE */

#if LWIP_TIMER_ON_DEMOND
    pcb->pcb_timer_active_mask = 0;
#endif

#if ENABLE_PSIF
    pcb->sockid = -1;
    pcb->tcp_max_retry_times = TCP_DEFRTX;
    pcb->tcp_max_total_retry_time = TCP_DEFRTX_TOTAL_TIME;
#endif

  }
  return pcb;
}

/**
 * @ingroup tcp_raw
 * Creates a new TCP protocol control block but doesn't place it on
 * any of the TCP PCB lists.
 * The pcb is not put on any list until binding using tcp_bind().
 *
 * @internal: Maybe there should be a idle TCP PCB list where these
 * PCBs are put on. Port reservation using tcp_bind() is implemented but
 * allocated pcbs that are not bound can't be killed automatically if wanting
 * to allocate a pcb with higher prio (@see tcp_kill_prio())
 *
 * @return a new tcp_pcb that initially is in state CLOSED
 */
struct tcp_pcb *
tcp_new(void)
{
  return tcp_alloc(TCP_PRIO_NORMAL);
}

/**
 * @ingroup tcp_raw
 * Creates a new TCP protocol control block but doesn't
 * place it on any of the TCP PCB lists.
 * The pcb is not put on any list until binding using tcp_bind().
 *
 * @param type IP address type, see @ref lwip_ip_addr_type definitions.
 * If you want to listen to IPv4 and IPv6 (dual-stack) connections,
 * supply @ref IPADDR_TYPE_ANY as argument and bind to @ref IP_ANY_TYPE.
 * @return a new tcp_pcb that initially is in state CLOSED
 */
struct tcp_pcb *
tcp_new_ip_type(u8_t type)
{
  struct tcp_pcb * pcb;
  pcb = tcp_alloc(TCP_PRIO_NORMAL);
#if LWIP_IPV4 && LWIP_IPV6
  if (pcb != NULL) {
    IP_SET_TYPE_VAL(pcb->local_ip, type);
    IP_SET_TYPE_VAL(pcb->remote_ip, type);
  }
#else
  LWIP_UNUSED_ARG(type);
#endif /* LWIP_IPV4 && LWIP_IPV6 */
  return pcb;
}

/**
 * @ingroup tcp_raw
 * Used to specify the argument that should be passed callback
 * functions.
 *
 * @param pcb tcp_pcb to set the callback argument
 * @param arg void pointer argument to pass to callback functions
 */
void
tcp_arg(struct tcp_pcb *pcb, void *arg)
{
  /* This function is allowed to be called for both listen pcbs and
     connection pcbs. */
  if (pcb != NULL) {
    pcb->callback_arg = arg;
  }
}
#if LWIP_CALLBACK_API

/**
 * @ingroup tcp_raw
 * Used to specify the function that should be called when a TCP
 * connection receives data.
 *
 * @param pcb tcp_pcb to set the recv callback
 * @param recv callback function to call for this pcb when data is received
 */
void
tcp_recv(struct tcp_pcb *pcb, tcp_recv_fn recv)
{
  if (pcb != NULL) {

#if LWIP_ENABLE_UNILOG
    if(pcb->state == LISTEN) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_recv_1, P_ERROR, 0, "invalid socket state for recv callback");
    }
#else
    LWIP_ASSERT("invalid socket state for recv callback", pcb->state != LISTEN);
#endif

    pcb->recv = recv;
  }
}

/**
 * @ingroup tcp_raw
 * Used to specify the function that should be called when TCP data
 * has been successfully delivered to the remote host.
 *
 * @param pcb tcp_pcb to set the sent callback
 * @param sent callback function to call for this pcb when data is successfully sent
 */
void
tcp_sent(struct tcp_pcb *pcb, tcp_sent_fn sent)
{
  if (pcb != NULL) {

#if LWIP_ENABLE_UNILOG
    if(pcb->state == LISTEN) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_sent_1, P_ERROR, 0, "invalid socket state for sent callback");
    }
#else
    LWIP_ASSERT("invalid socket state for sent callback", pcb->state != LISTEN);
#endif

    pcb->sent = sent;
  }
}

/**
 * @ingroup tcp_raw
 * Used to specify the function that should be called when a fatal error
 * has occurred on the connection.
 *
 * @note The corresponding pcb is already freed when this callback is called!
 *
 * @param pcb tcp_pcb to set the err callback
 * @param err callback function to call for this pcb when a fatal error
 *        has occurred on the connection
 */
void
tcp_err(struct tcp_pcb *pcb, tcp_err_fn err)
{
  if (pcb != NULL) {

#if LWIP_ENABLE_UNILOG
    if(pcb->state == LISTEN) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_err_1, P_ERROR, 0, "invalid socket state for err callback");
    }
#else
    LWIP_ASSERT("invalid socket state for err callback", pcb->state != LISTEN);
#endif

    pcb->errf = err;
  }
}

/**
 * @ingroup tcp_raw
 * Used for specifying the function that should be called when a
 * LISTENing connection has been connected to another host.
 *
 * @param pcb tcp_pcb to set the accept callback
 * @param accept callback function to call for this pcb when LISTENing
 *        connection has been connected to another host
 */
void
tcp_accept(struct tcp_pcb *pcb, tcp_accept_fn accept)
{
  if ((pcb != NULL) && (pcb->state == LISTEN)) {
    struct tcp_pcb_listen *lpcb = (struct tcp_pcb_listen*)pcb;
    lpcb->accept = accept;
  }
}

#if ENABLE_PSIF
void
tcp_poosf(struct tcp_pcb *pcb, tcp_process_oos_fn poosf)
{
  if (pcb != NULL) {
    pcb->poosf = poosf;
  }
}
#endif
#endif /* LWIP_CALLBACK_API */


/**
 * @ingroup tcp_raw
 * Used to specify the function that should be called periodically
 * from TCP. The interval is specified in terms of the TCP coarse
 * timer interval, which is called twice a second.
 *
 */
void
tcp_poll(struct tcp_pcb *pcb, tcp_poll_fn poll, u8_t interval)
{

#if LWIP_ENABLE_UNILOG
  if(pcb->state == LISTEN) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_poll_1, P_ERROR, 0, "invalid socket state for poll");
  }
#else
  LWIP_ASSERT("invalid socket state for poll", pcb->state != LISTEN);
#endif

#if LWIP_CALLBACK_API
  pcb->poll = poll;
#else /* LWIP_CALLBACK_API */
  LWIP_UNUSED_ARG(poll);
#endif /* LWIP_CALLBACK_API */
  pcb->pollinterval = interval;
}

/**
 * Purges a TCP PCB. Removes any buffered data and frees the buffer memory
 * (pcb->ooseq, pcb->unsent and pcb->unacked are freed).
 *
 * @param pcb tcp_pcb to purge. The pcb itself is not deallocated!
 */
void
tcp_pcb_purge(struct tcp_pcb *pcb)
{
  if (pcb->state != CLOSED &&
     pcb->state != TIME_WAIT &&
     pcb->state != LISTEN) {

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_purge_1, P_INFO, 0, "tcp_pcb_purge");
#else
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_pcb_purge\n"));
#endif

    tcp_backlog_accepted(pcb);

    if (pcb->refused_data != NULL) {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_purge_2, P_INFO, 0, "tcp_pcb_purge: data left on ->refused_data");
#else
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_pcb_purge: data left on ->refused_data\n"));
#endif
      pbuf_free(pcb->refused_data);
      pcb->refused_data = NULL;
    }

    if (pcb->unsent != NULL) {

#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_purge_3, P_INFO, 0, "tcp_pcb_purge: not all data sent");
#else
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_pcb_purge: not all data sent\n"));
#endif

    }

    if (pcb->unacked != NULL) {

#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_purge_4, P_INFO, 0, "tcp_pcb_purge: data left on ->unacked");
#else
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_pcb_purge: data left on ->unacked\n"));
#endif

    }

#if TCP_QUEUE_OOSEQ
    if (pcb->ooseq != NULL) {

#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_purge_5, P_INFO, 0, "tcp_pcb_purge: data left on ->ooseq");
#else
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_pcb_purge: data left on ->ooseq\n"));
#endif

    }
    tcp_segs_free(pcb->ooseq);
    pcb->ooseq = NULL;
#endif /* TCP_QUEUE_OOSEQ */

#if LWIP_TIMER_ON_DEMOND
    if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_purge_6, P_INFO, 0, "remove tcp refuse data timer");
    }

    if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT)) {
      sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT], (void *)pcb);
      tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT);
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_purge_10, P_INFO, 0, "remove tcp total retry timer");
    }

    if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY)) {
      sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_RETRY], (void *)pcb);
      tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY);
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_purge_7, P_INFO, 1, "remove tcp retry timer, pcb 0x%x", (void *)pcb);
    }


    if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT)) {
       sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT], (void *)pcb);
       tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT);
       ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_purge_8, P_INFO, 0, "remove tcp ooseq timer");
    }

       //deactive FIN_WAIT2 timer
    if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT)){
       sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT], (void *)pcb);
       tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT);
       ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_purge_9, P_INFO, 0, "remove fin wait2 timer");
    }

#endif

    //remove hib/sleep2 tcp context
#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
    if(pcb->pcb_hib_sleep2_mode_flag != PCB_HIB_DISABLE_DEACTIVE) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_purge_11, P_SIG, 0, "tcp_pcb_purge THE hib/sleep2 tcp context pcb will shutdown");
        tcp_disable_hib_sleep2_mode(pcb, PCB_HIB_DISABLE_DEACTIVE);
        tcp_set_hib_sleep2_pcb_list(NULL);
    }
#endif

    /* Stop the retransmission timer as it will expect data on unacked
       queue if it fires */
    pcb->rtime = -1;

    //indicate TCP UL sequence state
#if ENABLE_PSIF
    tcp_send_unack_ul_state(pcb);
#endif
    tcp_segs_free(pcb->unsent);
    tcp_segs_free(pcb->unacked);
    pcb->unacked = pcb->unsent = NULL;
#if TCP_OVERSIZE
    pcb->unsent_oversize = 0;
#endif /* TCP_OVERSIZE */
  }
}

/**
 * Purges the PCB and removes it from a PCB list. Any delayed ACKs are sent first.
 *
 * @param pcblist PCB list to purge.
 * @param pcb tcp_pcb to purge. The pcb itself is NOT deallocated!
 */
void
tcp_pcb_remove(struct tcp_pcb **pcblist, struct tcp_pcb *pcb)
{
  TCP_RMV(pcblist, pcb);

  tcp_pcb_purge(pcb);

  /* if there is an outstanding delayed ACKs, send it */
  if (pcb->state != TIME_WAIT &&
     pcb->state != LISTEN &&
     pcb->flags & TF_ACK_DELAY) {
#if LWIP_TIMER_ON_DEMOND
     if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_remove_4, P_INFO, 0, "remove tcp delay ack timer");
     }
#endif
    pcb->flags |= TF_ACK_NOW;
    tcp_output(pcb);
  }

  if (pcb->state != LISTEN) {

#if LWIP_ENABLE_UNILOG
    if(pcb->unsent != NULL || pcb->unacked != NULL) {
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_remove_1, P_ERROR, 0, "unsent or unacked segments leaking");
    }
#else
    LWIP_ASSERT("unsent segments leaking", pcb->unsent == NULL);
    LWIP_ASSERT("unacked segments leaking", pcb->unacked == NULL);
#endif

#if TCP_QUEUE_OOSEQ

#if LWIP_ENABLE_UNILOG
    if(pcb->ooseq != NULL) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_remove_2, P_ERROR, 0, "ooseq segments leaking");
    }
#else
    LWIP_ASSERT("ooseq segments leaking", pcb->ooseq == NULL);
#endif

#endif /* TCP_QUEUE_OOSEQ */
  }

  pcb->state = CLOSED;
  /* reset the local port to prevent the pcb from being 'bound' */
  pcb->local_port = 0;

#if LWIP_ENABLE_UNILOG
  if(!tcp_pcbs_sane()) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcb_remove_3, P_ERROR, 0, "tcp_pcb_remove: tcp_pcbs_sane");
  }
#else
  LWIP_ASSERT("tcp_pcb_remove: tcp_pcbs_sane()", tcp_pcbs_sane());
#endif
}

/**
 * Calculates a new initial sequence number for new connections.
 *
 * @return u32_t pseudo random sequence number
 */
u32_t
tcp_next_iss(struct tcp_pcb *pcb)
{
#ifdef LWIP_HOOK_TCP_ISN
  return LWIP_HOOK_TCP_ISN(&pcb->local_ip, pcb->local_port, &pcb->remote_ip, pcb->remote_port);
#else /* LWIP_HOOK_TCP_ISN */
  static u32_t iss = 6510;

  LWIP_UNUSED_ARG(pcb);

#if LWIP_TIMER_ON_DEMOND
  iss += rand();
#else
  iss += tcp_ticks;       /* XXX */
#endif
  return iss;
#endif /* LWIP_HOOK_TCP_ISN */
}

#if TCP_CALCULATE_EFF_SEND_MSS
/**
 * Calculates the effective send mss that can be used for a specific IP address
 * by using ip_route to determine the netif used to send to the address and
 * calculating the minimum of TCP_MSS and that netif's mtu (if set).
 */
u16_t
tcp_eff_send_mss_impl(u16_t sendmss, const ip_addr_t *dest
#if LWIP_IPV6 || LWIP_IPV4_SRC_ROUTING
                     , const ip_addr_t *src
#endif /* LWIP_IPV6 || LWIP_IPV4_SRC_ROUTING */
                     )
{
  u16_t mss_s;
  struct netif *outif;
  s16_t mtu;

  outif = ip_route(src, dest);
#if LWIP_IPV6
#if LWIP_IPV4
  if (IP_IS_V6(dest))
#endif /* LWIP_IPV4 */
  {
    /* First look in destination cache, to see if there is a Path MTU. */
    mtu = nd6_get_destination_mtu(ip_2_ip6(dest), outif);
  }
#if LWIP_IPV4
  else
#endif /* LWIP_IPV4 */
#endif /* LWIP_IPV6 */
#if LWIP_IPV4
  {
    if (outif == NULL) {
      return sendmss;
    }
    mtu = outif->mtu;
  }
#endif /* LWIP_IPV4 */

  if (mtu != 0) {
#if LWIP_IPV6
#if LWIP_IPV4
    if (IP_IS_V6(dest))
#endif /* LWIP_IPV4 */
    {
      mss_s = mtu - IP6_HLEN - TCP_HLEN;
    }
#if LWIP_IPV4
    else
#endif /* LWIP_IPV4 */
#endif /* LWIP_IPV6 */
#if LWIP_IPV4
    {
      mss_s = mtu - IP_HLEN - TCP_HLEN;
    }
#endif /* LWIP_IPV4 */
    /* RFC 1122, chap 4.2.2.6:
     * Eff.snd.MSS = min(SendMSS+20, MMS_S) - TCPhdrsize - IPoptionsize
     * We correct for TCP options in tcp_write(), and don't support IP options.
     */
    sendmss = LWIP_MIN(sendmss, mss_s);
  }
  return sendmss;
}
#endif /* TCP_CALCULATE_EFF_SEND_MSS */

/** Helper function for tcp_netif_ip_addr_changed() that iterates a pcb list */
static void
tcp_netif_ip_addr_changed_pcblist(const ip_addr_t* old_addr, struct tcp_pcb* pcb_list)
{
  struct tcp_pcb *pcb;
  pcb = pcb_list;
  while (pcb != NULL) {
    /* PCB bound to current local interface address? */
    if (ip_addr_cmp(&pcb->local_ip, old_addr)
#if LWIP_AUTOIP
      /* connections to link-local addresses must persist (RFC3927 ch. 1.9) */
      && (!IP_IS_V4_VAL(pcb->local_ip) || !ip4_addr_islinklocal(ip_2_ip4(&pcb->local_ip)))
#endif /* LWIP_AUTOIP */
      ) {
      /* this connection must be aborted */
      struct tcp_pcb *next = pcb->next;

#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_eff_send_mss_impl_1, P_INFO, 1, "netif_set_ipaddr: aborting TCP pcb 0x%x", (void *)pcb);
#else
      LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_STATE, ("netif_set_ipaddr: aborting TCP pcb %p\n", (void *)pcb));
#endif

      tcp_abort(pcb);
      pcb = next;
    } else {
      pcb = pcb->next;
    }
  }
}

/** This function is called from netif.c when address is changed or netif is removed
 *
 * @param old_addr IP address of the netif before change
 * @param new_addr IP address of the netif after change or NULL if netif has been removed
 */
void
tcp_netif_ip_addr_changed(const ip_addr_t* old_addr, const ip_addr_t* new_addr)
{
  struct tcp_pcb_listen *lpcb, *next;

  if (!ip_addr_isany(old_addr)) {
#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
    if(tcp_hib_sleep2_pcbs && tcp_hib_sleep2_pcbs->pcb_hib_sleep2_mode_flag == PCB_HIB_ENABLE_ACTIVE) {
        if(tcp_check_is_include_list(tcp_hib_sleep2_pcbs) == FALSE) {
            if (ip_addr_cmp(&tcp_hib_sleep2_pcbs->local_ip, old_addr)) {
                if (!ip_addr_cmp(&tcp_hib_sleep2_pcbs->local_ip, new_addr)) {
                    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_netif_ip_addr_changed_1, P_INFO, 1, "tcp_netif_ip_addr_changed: aborting TCP hib pcb 0x%x", (void *)tcp_hib_sleep2_pcbs);
                    tcp_set_hib_sleep2_state(tcp_hib_sleep2_pcbs, PCB_HIB_ENABLE_DEACTIVE);
                    tcp_abort(tcp_hib_sleep2_pcbs);
                }
            }
        }
    }
#endif
    tcp_netif_ip_addr_changed_pcblist(old_addr, tcp_active_pcbs);
    tcp_netif_ip_addr_changed_pcblist(old_addr, tcp_bound_pcbs);


    if (!ip_addr_isany(new_addr)) {
      /* PCB bound to current local interface address? */
      for (lpcb = tcp_listen_pcbs.listen_pcbs; lpcb != NULL; lpcb = next) {
        next = lpcb->next;
        /* PCB bound to current local interface address? */
        if (ip_addr_cmp(&lpcb->local_ip, old_addr)) {
          /* The PCB is listening to the old ipaddr and
            * is set to listen to the new one instead */
          ip_addr_copy(lpcb->local_ip, *new_addr);
        }
      }
    }
  }
}

#if ENABLE_PSIF
void tcp_netif_enter_oos_state(const ip_addr_t * address)
{
    struct tcp_pcb *lpcb;

    if (!ip_addr_isany(address)) {
        for(lpcb = tcp_active_pcbs; lpcb != NULL; lpcb = lpcb->next) {
           if(ip_addr_cmp(&lpcb->local_ip, address))
           {
               if(lpcb->state == ESTABLISHED) {
                    lpcb->related_netif_oos = 1;
                    if(lpcb->poosf != NULL) {
                        lpcb->poosf(lpcb->callback_arg, 1);
                        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_netif_enter_oos_state_1, P_INFO, 1, "tcp pcb 0x%x enter oos state event", lpcb);
                    }
               }
           }

        }
    }
}

void tcp_netif_exit_oos_state(const ip_addr_t * address)
{
    struct tcp_pcb *lpcb;

    if (!ip_addr_isany(address)) {
        for(lpcb = tcp_active_pcbs; lpcb != NULL; lpcb = lpcb->next) {
           if(ip_addr_cmp(&lpcb->local_ip, address))
           {
               if(lpcb->state == ESTABLISHED) {
                    lpcb->related_netif_oos = 0;
                    if(lpcb->poosf != NULL) {
                        lpcb->poosf(lpcb->callback_arg, 0);
                        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_netif_exit_oos_state_1, P_INFO, 1, "tcp pcb 0x%x exit oos state event", lpcb);
                    }
               }
           }

        }
    }
}

#endif

const char*
tcp_debug_state_str(enum tcp_state s)
{
  return tcp_state_str[s];
}

#if TCP_DEBUG || TCP_INPUT_DEBUG || TCP_OUTPUT_DEBUG
/**
 * Print a tcp header for debugging purposes.
 *
 * @param tcphdr pointer to a struct tcp_hdr
 */
void
tcp_debug_print(struct tcp_hdr *tcphdr)
{
#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_1, P_INFO, 0, "TCP header:");
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_2, P_INFO, 2, "| %u | %u | (src port, dest port)", lwip_ntohs(tcphdr->src), lwip_ntohs(tcphdr->dest));
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_3, P_INFO, 2, "| %u | %u | (seq no) (ack no)", lwip_ntohl(tcphdr->seqno), lwip_ntohl(tcphdr->ackno));
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_4, P_INFO, 8, "|%u|%u%u%u%u%u%u|%u|(seq no) (hdrlen flags wnd)", TCPH_HDRLEN(tcphdr), (u16_t)(TCPH_FLAGS(tcphdr) >> 5 & 1),
    (u16_t)(TCPH_FLAGS(tcphdr) >> 4 & 1),
    (u16_t)(TCPH_FLAGS(tcphdr) >> 3 & 1),
    (u16_t)(TCPH_FLAGS(tcphdr) >> 2 & 1),
    (u16_t)(TCPH_FLAGS(tcphdr) >> 1 & 1),
    (u16_t)(TCPH_FLAGS(tcphdr)      & 1),
    lwip_ntohs(tcphdr->wnd));
#else
  LWIP_DEBUGF(TCP_DEBUG, ("TCP header:\n"));
  LWIP_DEBUGF(TCP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(TCP_DEBUG, ("|    %5"U16_F"      |    %5"U16_F"      | (src port, dest port)\n",
         lwip_ntohs(tcphdr->src), lwip_ntohs(tcphdr->dest)));
  LWIP_DEBUGF(TCP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(TCP_DEBUG, ("|           %010"U32_F"          | (seq no)\n",
          lwip_ntohl(tcphdr->seqno)));
  LWIP_DEBUGF(TCP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(TCP_DEBUG, ("|           %010"U32_F"          | (ack no)\n",
         lwip_ntohl(tcphdr->ackno)));
  LWIP_DEBUGF(TCP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(TCP_DEBUG, ("| %2"U16_F" |   |%"U16_F"%"U16_F"%"U16_F"%"U16_F"%"U16_F"%"U16_F"|     %5"U16_F"     | (hdrlen, flags (",
       TCPH_HDRLEN(tcphdr),
         (u16_t)(TCPH_FLAGS(tcphdr) >> 5 & 1),
         (u16_t)(TCPH_FLAGS(tcphdr) >> 4 & 1),
         (u16_t)(TCPH_FLAGS(tcphdr) >> 3 & 1),
         (u16_t)(TCPH_FLAGS(tcphdr) >> 2 & 1),
         (u16_t)(TCPH_FLAGS(tcphdr) >> 1 & 1),
         (u16_t)(TCPH_FLAGS(tcphdr)      & 1),
         lwip_ntohs(tcphdr->wnd)));
#endif
  tcp_debug_print_flags(TCPH_FLAGS(tcphdr));

#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_5, P_INFO, 2, "| %u | %u | (chksum, urgp)", lwip_ntohs(tcphdr->chksum), lwip_ntohs(tcphdr->urgp));
#else
  LWIP_DEBUGF(TCP_DEBUG, ("), win)\n"));
  LWIP_DEBUGF(TCP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(TCP_DEBUG, ("|    0x%04"X16_F"     |     %5"U16_F"     | (chksum, urgp)\n",
         lwip_ntohs(tcphdr->chksum), lwip_ntohs(tcphdr->urgp)));
  LWIP_DEBUGF(TCP_DEBUG, ("+-------------------------------+\n"));
#endif
}

/**
 * Print a tcp state for debugging purposes.
 *
 * @param s enum tcp_state to print
 */
void
tcp_debug_print_state(enum tcp_state s)
{
#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_state_1, P_INFO, 1, "State: %d", s);
#else
  LWIP_DEBUGF(TCP_DEBUG, ("State: %s\n", tcp_state_str[s]));
#endif
}

/**
 * Print tcp flags for debugging purposes.
 *
 * @param flags tcp flags, all active flags are printed
 */
void
tcp_debug_print_flags(u8_t flags)
{
  if (flags & TCP_FIN) {

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_flags_1, P_INFO, 0, "FIN");
#else
    LWIP_DEBUGF(TCP_DEBUG, ("FIN "));
#endif

  }
  if (flags & TCP_SYN) {

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_flags_2, P_INFO, 0, "SYN");
#else
    LWIP_DEBUGF(TCP_DEBUG, ("SYN "));
#endif

  }
  if (flags & TCP_RST) {

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_flags_3, P_INFO, 0, "RST");
#else
    LWIP_DEBUGF(TCP_DEBUG, ("RST "));
#endif

  }
  if (flags & TCP_PSH) {

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_flags_4, P_INFO, 0, "PSH");
#else
    LWIP_DEBUGF(TCP_DEBUG, ("PSH "));
#endif

  }
  if (flags & TCP_ACK) {

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_flags_5, P_INFO, 0, "PSH");
#else
    LWIP_DEBUGF(TCP_DEBUG, ("ACK "));
#endif

  }
  if (flags & TCP_URG) {

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_flags_6, P_INFO, 0, "URG");
#else
    LWIP_DEBUGF(TCP_DEBUG, ("URG "));
#endif

  }
  if (flags & TCP_ECE) {

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_flags_7, P_INFO, 0, "ECE");
#else
    LWIP_DEBUGF(TCP_DEBUG, ("ECE "));
#endif

  }
  if (flags & TCP_CWR) {

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_flags_8, P_INFO, 0, "CWR");
#else
    LWIP_DEBUGF(TCP_DEBUG, ("CWR "));
#endif

  }

#ifndef LWIP_ENABLE_UNILOG
  LWIP_DEBUGF(TCP_DEBUG, ("\n"));
#endif

}

/**
 * Print all tcp_pcbs in every list for debugging purposes.
 */
void
tcp_debug_print_pcbs(void)
{
  struct tcp_pcb *pcb;
  struct tcp_pcb_listen *pcbl;

#ifndef LWIP_ENABLE_UNILOG
  LWIP_DEBUGF(TCP_DEBUG, ("Active PCB states:\n"));
#endif

  for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_pcbs_1, P_INFO, 4, "Active PCB Local port(%u) foreign port(%u) snd_nxt(%u) rcv_nxt(%u)", pcb->local_port, pcb->remote_port, pcb->snd_nxt, pcb->rcv_nxt);
#else
    LWIP_DEBUGF(TCP_DEBUG, ("Local port %"U16_F", foreign port %"U16_F" snd_nxt %"U32_F" rcv_nxt %"U32_F" ",
                       pcb->local_port, pcb->remote_port,
                       pcb->snd_nxt, pcb->rcv_nxt));
#endif
    tcp_debug_print_state(pcb->state);
  }

#ifndef LWIP_ENABLE_UNILOG
  LWIP_DEBUGF(TCP_DEBUG, ("Listen PCB states:\n"));
#endif

  for (pcbl = tcp_listen_pcbs.listen_pcbs; pcbl != NULL; pcbl = pcbl->next) {
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_pcbs_2, P_INFO, 1, "Listen PCB Local port %u", pcbl->local_port);
#else
    LWIP_DEBUGF(TCP_DEBUG, ("Local port %"U16_F" ", pcbl->local_port));
#endif
    tcp_debug_print_state(pcbl->state);
  }

#ifndef LWIP_ENABLE_UNILOG
  LWIP_DEBUGF(TCP_DEBUG, ("TIME-WAIT PCB states:\n"));
#endif

  for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_debug_print_pcbs_3, P_INFO, 4, "TIME-WAIT PCB Local port(%u) foreign port(%u) snd_nxt(%u) rcv_nxt(%u)", pcb->local_port, pcb->remote_port, pcb->snd_nxt, pcb->rcv_nxt);
#else
    LWIP_DEBUGF(TCP_DEBUG, ("Local port %"U16_F", foreign port %"U16_F" snd_nxt %"U32_F" rcv_nxt %"U32_F" ",
                       pcb->local_port, pcb->remote_port,
                       pcb->snd_nxt, pcb->rcv_nxt));
#endif
    tcp_debug_print_state(pcb->state);
  }
}

/**
 * Check state consistency of the tcp_pcb lists.
 */
s16_t
tcp_pcbs_sane(void)
{
  struct tcp_pcb *pcb;
  for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
#if LWIP_ENABLE_UNILOG
    if(pcb->state == CLOSED || pcb->state == LISTEN || pcb->state == TIME_WAIT) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcbs_sane_1, P_ERROR, 0, "tcp_pcbs_sane");
    }
#else
    LWIP_ASSERT("tcp_pcbs_sane: active pcb->state != CLOSED", pcb->state != CLOSED);
    LWIP_ASSERT("tcp_pcbs_sane: active pcb->state != LISTEN", pcb->state != LISTEN);
    LWIP_ASSERT("tcp_pcbs_sane: active pcb->state != TIME-WAIT", pcb->state != TIME_WAIT);
#endif
  }
  for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
#if LWIP_ENABLE_UNILOG
    if(pcb->state != TIME_WAIT) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_pcbs_sane_2, P_ERROR, 0, "tcp_pcbs_sane: tw pcb->state == TIME-WAIT");
    }
#else
    LWIP_ASSERT("tcp_pcbs_sane: tw pcb->state == TIME-WAIT", pcb->state == TIME_WAIT);
#endif
  }
  return 1;
}
#endif /* TCP_DEBUG */

#endif /* LWIP_TCP */
