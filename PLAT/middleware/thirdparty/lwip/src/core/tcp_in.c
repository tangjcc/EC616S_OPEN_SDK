/**
 * @file
 * Transmission Control Protocol, incoming traffic
 *
 * The input processing functions of the TCP layer.
 *
 * These functions are generally called in the order (ip_input() ->)
 * tcp_input() -> * tcp_process() -> tcp_receive() (-> application).
 *
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

#include "lwip/priv/tcp_priv.h"
#include "lwip/def.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/inet_chksum.h"
#include "lwip/stats.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#if ENABLE_PSIF
#include "lwip/udp.h"
#include "psifmem.h"
#endif
#if LWIP_ND6_TCP_REACHABILITY_HINTS
#include "lwip/nd6.h"
#endif /* LWIP_ND6_TCP_REACHABILITY_HINTS */

#if LWIP_TIMER_ON_DEMOND
#include "lwip/timeouts.h"
#endif




/** Initial CWND calculation as defined RFC 2581 */
#define LWIP_TCP_CALC_INITIAL_CWND(mss) LWIP_MIN((4U * (mss)), LWIP_MAX((2U * (mss)), 4380U));

/* These variables are global to all functions involved in the input
   processing of TCP segments. They are set by the tcp_input()
   function. */
static struct tcp_seg inseg;
static struct tcp_hdr *tcphdr;
static u16_t tcphdr_optlen;
static u16_t tcphdr_opt1len;
static u8_t* tcphdr_opt2;
static u16_t tcp_optidx;
static u32_t seqno, ackno;
static tcpwnd_size_t recv_acked;
static u16_t tcplen;
static u8_t flags;

static u8_t recv_flags;
static struct pbuf *recv_data;

struct tcp_pcb *tcp_input_pcb;

/* Forward declarations. */
static err_t tcp_process(struct tcp_pcb *pcb);
static void tcp_receive(struct tcp_pcb *pcb);
static void tcp_parseopt(struct tcp_pcb *pcb);

static void tcp_listen_input(struct tcp_pcb_listen *pcb);
static void tcp_timewait_input(struct tcp_pcb *pcb);

static int tcp_input_delayed_close(struct tcp_pcb *pcb);

/**
 * The initial input processing of TCP. It verifies the TCP header, demultiplexes
 * the segment between the PCBs and passes it on to tcp_process(), which implements
 * the TCP finite state machine. This function is called by the IP layer (in
 * ip_input()).
 *
 * @param p received TCP segment to process (p->payload pointing to the TCP header)
 * @param inp network interface on which this segment was received
 */
void
tcp_input(struct pbuf *p, struct netif *inp)
{
  struct tcp_pcb *pcb, *prev;
  struct tcp_pcb_listen *lpcb;
#if SO_REUSE
  struct tcp_pcb *lpcb_prev = NULL;
  struct tcp_pcb_listen *lpcb_any = NULL;
#endif /* SO_REUSE */
  u8_t hdrlen_bytes;
  err_t err;

  LWIP_UNUSED_ARG(inp);

  PERF_START;

  TCP_STATS_INC(tcp.recv);
  MIB2_STATS_INC(mib2.tcpinsegs);

  tcphdr = (struct tcp_hdr *)p->payload;

#if TCP_INPUT_DEBUG
  tcp_debug_print(tcphdr);
#endif

  /* Check that TCP header fits in payload */
  if (p->len < TCP_HLEN) {
    /* drop short packets */
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_1, P_WARNING, 1, "tcp_input: short packet (%u bytes) discarded", p->tot_len);
#else     
    LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_input: short packet (%"U16_F" bytes) discarded\n", p->tot_len));
#endif
    TCP_STATS_INC(tcp.lenerr);
    goto dropped;
  }

  /* Don't even process incoming broadcasts/multicasts. */
  if (ip_addr_isbroadcast(ip_current_dest_addr(), ip_current_netif()) ||
      ip_addr_ismulticast(ip_current_dest_addr())) {
    TCP_STATS_INC(tcp.proterr);
    goto dropped;
  }

#if CHECKSUM_CHECK_TCP
  IF__NETIF_CHECKSUM_ENABLED(inp, NETIF_CHECKSUM_CHECK_TCP) {
    /* Verify TCP checksum. */
    u16_t chksum = ip_chksum_pseudo(p, IP_PROTO_TCP, p->tot_len,
                               ip_current_src_addr(), ip_current_dest_addr());
    if (chksum != 0) {
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_2, P_WARNING, 1, "tcp_input: packet discarded due to failing checksum 0x%x", chksum);
#else         
        LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_input: packet discarded due to failing checksum 0x%04"X16_F"\n",
          chksum));
#endif
      tcp_debug_print(tcphdr);
      TCP_STATS_INC(tcp.chkerr);
      goto dropped;
    }
  }
#endif /* CHECKSUM_CHECK_TCP */

  /* sanity-check header length */
  hdrlen_bytes = TCPH_HDRLEN(tcphdr) * 4;
  if ((hdrlen_bytes < TCP_HLEN) || (hdrlen_bytes > p->tot_len)) {
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_3, P_WARNING, 1, "tcp_input: invalid header length (%u)", (u16_t)hdrlen_bytes);
#else    
    LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_input: invalid header length (%"U16_F")\n", (u16_t)hdrlen_bytes));
#endif
    TCP_STATS_INC(tcp.lenerr);
    goto dropped;
  }

  /* Move the payload pointer in the pbuf so that it points to the
     TCP data instead of the TCP header. */
  tcphdr_optlen = hdrlen_bytes - TCP_HLEN;
  tcphdr_opt2 = NULL;
  if (p->len >= hdrlen_bytes) {
    /* all options are in the first pbuf */
    tcphdr_opt1len = tcphdr_optlen;
    pbuf_header(p, -(s16_t)hdrlen_bytes); /* cannot fail */
  } else {
    u16_t opt2len;
    /* TCP header fits into first pbuf, options don't - data is in the next pbuf */
    /* there must be a next pbuf, due to hdrlen_bytes sanity check above */
#if LWIP_ENABLE_UNILOG
    if(p->next == NULL) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_4, P_ERROR, 0, "p->next == NULL");
    }
#else    
    LWIP_ASSERT("p->next != NULL", p->next != NULL);
#endif

    /* advance over the TCP header (cannot fail) */
    pbuf_header(p, -TCP_HLEN);

    /* determine how long the first and second parts of the options are */
    tcphdr_opt1len = p->len;
    opt2len = tcphdr_optlen - tcphdr_opt1len;

    /* options continue in the next pbuf: set p to zero length and hide the
        options in the next pbuf (adjusting p->tot_len) */
    pbuf_header(p, -(s16_t)tcphdr_opt1len);

    /* check that the options fit in the second pbuf */
    if (opt2len > p->next->len) {
      /* drop short packets */
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_5, P_WARNING, 1, "tcp_input: options overflow second pbuf (%u bytes)", p->next->len);
#else        
      LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_input: options overflow second pbuf (%"U16_F" bytes)\n", p->next->len));
#endif
      TCP_STATS_INC(tcp.lenerr);
      goto dropped;
    }

    /* remember the pointer to the second part of the options */
    tcphdr_opt2 = (u8_t*)p->next->payload;

    /* advance p->next to point after the options, and manually
        adjust p->tot_len to keep it consistent with the changed p->next */
    pbuf_header(p->next, -(s16_t)opt2len);
    p->tot_len -= opt2len;

#if LWIP_ENABLE_UNILOG
    if(p->len != 0 || p->tot_len != p->next->tot_len) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_6, P_ERROR, 0, "p->len != 0 or p->tot_len != p->next->tot_len");
    }
#else
    LWIP_ASSERT("p->len == 0", p->len == 0);
    LWIP_ASSERT("p->tot_len == p->next->tot_len", p->tot_len == p->next->tot_len);
#endif
  }

  /* Convert fields in TCP header to host byte order. */
  tcphdr->src = lwip_ntohs(tcphdr->src);
  tcphdr->dest = lwip_ntohs(tcphdr->dest);
  seqno = tcphdr->seqno = lwip_ntohl(tcphdr->seqno);
  ackno = tcphdr->ackno = lwip_ntohl(tcphdr->ackno);
  tcphdr->wnd = lwip_ntohs(tcphdr->wnd);

  flags = TCPH_FLAGS(tcphdr);
  tcplen = p->tot_len + ((flags & (TCP_FIN | TCP_SYN)) ? 1 : 0);

  /* Demultiplex an incoming segment. First, we check if it is destined
     for an active connection. */
  prev = NULL;

  for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {

#if LWIP_ENABLE_UNILOG
    if(pcb->state == CLOSED || pcb->state == TIME_WAIT || pcb->state == LISTEN) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_7, P_ERROR, 0, "tcp_input: active pcb invalid state");
    }
#else    
    LWIP_ASSERT("tcp_input: active pcb->state != CLOSED", pcb->state != CLOSED);
    LWIP_ASSERT("tcp_input: active pcb->state != TIME-WAIT", pcb->state != TIME_WAIT);
    LWIP_ASSERT("tcp_input: active pcb->state != LISTEN", pcb->state != LISTEN);
#endif    
    if (pcb->remote_port == tcphdr->src &&
        pcb->local_port == tcphdr->dest &&
        ip_addr_cmp(&pcb->remote_ip, ip_current_src_addr()) &&
        ip_addr_cmp(&pcb->local_ip, ip_current_dest_addr())) {
      /* Move this PCB to the front of the list so that subsequent
         lookups will be faster (we exploit locality in TCP segment
         arrivals). */
#if LWIP_ENABLE_UNILOG
      if(pcb->next == pcb) {
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_8, P_ERROR, 0, "tcp_input: pcb->next != pcb (before cache)");
      }
#else          
      LWIP_ASSERT("tcp_input: pcb->next != pcb (before cache)", pcb->next != pcb);
#endif
      if (prev != NULL) {
        prev->next = pcb->next;
        pcb->next = tcp_active_pcbs;
        tcp_active_pcbs = pcb;
      } else {
        TCP_STATS_INC(tcp.cachehit);
      }
#if LWIP_ENABLE_UNILOG
      if(pcb->next == pcb) {
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_9, P_ERROR, 0, "tcp_input: pcb->next != pcb (after cache)");
      }
#else      
      LWIP_ASSERT("tcp_input: pcb->next != pcb (after cache)", pcb->next != pcb);
#endif
      break;
    }
    prev = pcb;
  }

  if (pcb == NULL) {
    /* If it did not go to an active connection, we check the connections
       in the TIME-WAIT state. */
    for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
#if LWIP_ENABLE_UNILOG
      if(pcb->state != TIME_WAIT) {
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_10, P_ERROR, 0, "tcp_input: TIME-WAIT pcb->state == TIME-WAIT");
      }
#else         
      LWIP_ASSERT("tcp_input: TIME-WAIT pcb->state == TIME-WAIT", pcb->state == TIME_WAIT);
#endif
      if (pcb->remote_port == tcphdr->src &&
          pcb->local_port == tcphdr->dest &&
          ip_addr_cmp(&pcb->remote_ip, ip_current_src_addr()) &&
          ip_addr_cmp(&pcb->local_ip, ip_current_dest_addr())) {
        /* We don't really care enough to move this PCB to the front
           of the list since we are not very likely to receive that
           many segments for connections in TIME-WAIT. */
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_11, P_INFO, 0, "tcp_input: packed for TIME_WAITing connection");
#else           
        LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_input: packed for TIME_WAITing connection.\n"));
#endif
        tcp_timewait_input(pcb);
        pbuf_free(p);
        return;
      }
    }

    /* Finally, if we still did not get a match, we check all PCBs that
       are LISTENing for incoming connections. */
    prev = NULL;
    for (lpcb = tcp_listen_pcbs.listen_pcbs; lpcb != NULL; lpcb = lpcb->next) {
      if (lpcb->local_port == tcphdr->dest) {
        if (IP_IS_ANY_TYPE_VAL(lpcb->local_ip)) {
          /* found an ANY TYPE (IPv4/IPv6) match */
#if SO_REUSE
          lpcb_any = lpcb;
          lpcb_prev = prev;
#else /* SO_REUSE */
          break;
#endif /* SO_REUSE */
        } else if (IP_ADDR_PCB_VERSION_MATCH_EXACT(lpcb, ip_current_dest_addr())) {
          if (ip_addr_cmp(&lpcb->local_ip, ip_current_dest_addr())) {
            /* found an exact match */
            break;
          } else if (ip_addr_isany(&lpcb->local_ip)) {
            /* found an ANY-match */
#if SO_REUSE
            lpcb_any = lpcb;
            lpcb_prev = prev;
#else /* SO_REUSE */
            break;
 #endif /* SO_REUSE */
          }
        }
      }
      prev = (struct tcp_pcb *)lpcb;
    }
#if SO_REUSE
    /* first try specific local IP */
    if (lpcb == NULL) {
      /* only pass to ANY if no specific local IP has been found */
      lpcb = lpcb_any;
      prev = lpcb_prev;
    }
#endif /* SO_REUSE */
    if (lpcb != NULL) {
      /* Move this PCB to the front of the list so that subsequent
         lookups will be faster (we exploit locality in TCP segment
         arrivals). */
      if (prev != NULL) {
        ((struct tcp_pcb_listen *)prev)->next = lpcb->next;
              /* our successor is the remainder of the listening list */
        lpcb->next = tcp_listen_pcbs.listen_pcbs;
              /* put this listening pcb at the head of the listening list */
        tcp_listen_pcbs.listen_pcbs = lpcb;
      } else {
        TCP_STATS_INC(tcp.cachehit);
      }

#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_12, P_INFO, 0, "tcp_input: packed for LISTENing connection");
#else 
      LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_input: packed for LISTENing connection.\n"));
#endif
      tcp_listen_input(lpcb);
      pbuf_free(p);
      return;
    }
  }

#if TCP_INPUT_DEBUG
#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_13, P_INFO, 0, "tcp_input: flags ");
#else 
  LWIP_DEBUGF(TCP_INPUT_DEBUG, ("+-+-+-+-+-+-+-+-+-+-+-+-+-+- tcp_input: flags "));
#endif
  tcp_debug_print_flags(TCPH_FLAGS(tcphdr));
#ifndef LWIP_ENABLE_UNILOG
  LWIP_DEBUGF(TCP_INPUT_DEBUG, ("-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n"));
#endif
#endif /* TCP_INPUT_DEBUG */


  if (pcb != NULL) {
    /* The incoming segment belongs to a connection. */
#if TCP_INPUT_DEBUG
    tcp_debug_print_state(pcb->state);
#endif /* TCP_INPUT_DEBUG */

    /* Set up a tcp_seg structure. */
    inseg.next = NULL;
    inseg.len = p->tot_len;
    inseg.p = p;
    inseg.tcphdr = tcphdr;

    recv_data = NULL;
    recv_flags = 0;
    recv_acked = 0;

    if (flags & TCP_PSH) {
      p->flags |= PBUF_FLAG_PUSH;
    }

    /* If there is data which was previously "refused" by upper layer */
    if (pcb->refused_data != NULL) {
      if ((tcp_process_refused_data(pcb) == ERR_ABRT) ||
        ((pcb->refused_data != NULL) && (tcplen > 0))) {
        /* pcb has been aborted or refused data is still refused and the new
           segment contains data */
        if (pcb->rcv_ann_wnd == 0) {
          /* this is a zero-window probe, we respond to it with current RCV.NXT
          and drop the data segment */
          tcp_send_empty_ack(pcb);
        }
        TCP_STATS_INC(tcp.drop);
        MIB2_STATS_INC(mib2.tcpinerrs);
        goto aborted;
      }
    }
    tcp_input_pcb = pcb;
    err = tcp_process(pcb);
    /* A return value of ERR_ABRT means that tcp_abort() was called
       and that the pcb has been freed. If so, we don't do anything. */
    if (err != ERR_ABRT) {
      if (recv_flags & TF_RESET) {
        /* TF_RESET means that the connection was reset by the other
           end. We then call the error callback to inform the
           application that the connection is dead before we
           deallocate the PCB. */
        TCP_EVENT_ERR(pcb->state, pcb->errf, pcb->callback_arg, ERR_RST);
#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
    if(pcb->pcb_hib_sleep2_mode_flag != PCB_HIB_DISABLE_DEACTIVE) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_20, P_SIG, 0, "tcp receive rst, THE hib/sleep2 tcp context pcb will shutdown");
        tcp_disable_hib_sleep2_mode(pcb, PCB_HIB_DISABLE_DEACTIVE);
        tcp_set_hib_sleep2_pcb_list(NULL);
    }
#endif        
        tcp_pcb_remove(&tcp_active_pcbs, pcb);
#if LWIP_TIMER_ON_DEMOND      
        tcp_remove_all_timer(pcb);
#endif
        memp_free(MEMP_TCP_PCB, pcb);
      } else {
        err = ERR_OK;
        /* If the application has registered a "sent" function to be
           called when new send buffer space is available, we call it
           now. */
        if (recv_acked > 0) {
          u16_t acked16;
#if LWIP_WND_SCALE
          /* recv_acked is u32_t but the sent callback only takes a u16_t,
             so we might have to call it multiple times. */
          u32_t acked = recv_acked;
          while (acked > 0) {
            acked16 = (u16_t)LWIP_MIN(acked, 0xffffu);
            acked -= acked16;
#else
          {
            acked16 = recv_acked;
#endif
            TCP_EVENT_SENT(pcb, (u16_t)acked16, err);
            if (err == ERR_ABRT) {
              goto aborted;
            }
          }
          recv_acked = 0;
        }
        if (tcp_input_delayed_close(pcb)) {
          goto aborted;
        }
#if TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
        while (recv_data != NULL) {
          struct pbuf *rest = NULL;
          pbuf_split_64k(recv_data, &rest);
#else /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
        if (recv_data != NULL) {
#endif /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */

#if LWIP_ENABLE_UNILOG
          if(pcb->refused_data != NULL) {
             ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_14, P_ERROR, 0, "pcb->refused_data == NULL");
          }
#else
          LWIP_ASSERT("pcb->refused_data == NULL", pcb->refused_data == NULL);
#endif
          if (pcb->flags & TF_RXCLOSED) {
            /* received data although already closed -> abort (send RST) to
               notify the remote host that not all data has been processed */
            pbuf_free(recv_data);
#if TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
            if (rest != NULL) {
              pbuf_free(rest);
            }
#endif /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
            tcp_abort(pcb);
            goto aborted;
          }

          /* Notify application that data has been received. */
          TCP_EVENT_RECV(pcb, recv_data, ERR_OK, err);
          if (err == ERR_ABRT) {
#if TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
            if (rest != NULL) {
              pbuf_free(rest);
            }
#endif /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
            goto aborted;
          }

          /* If the upper layer can't receive this data, store it */
          if (err != ERR_OK) {
#if TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
            if (rest != NULL) {
              pbuf_cat(recv_data, rest);
            }
#endif /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
#if LWIP_TIMER_ON_DEMOND
            //if refuse data timer deactive,active refuse data because exist refuse data
            if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA)) {
               ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_15, P_INFO, 1, "tcp refuse data timer has enable");  
            }else{
               sys_timeout(TCP_TMR_INTERVAL, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA], (void *)pcb);
               tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA);
               ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_16, P_INFO, 1, "enable tcp refuse data timer %u", TCP_TMR_INTERVAL);  
            }
#endif

            pcb->refused_data = recv_data;
#if LWIP_ENABLE_UNILOG
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_17, P_WARNING, 0, "tcp_input: keep incoming packet, because pcb is full");
#else
            LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_input: keep incoming packet, because pcb is \"full\"\n"));
#endif
#if TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
            break;
          } else {
            /* Upper layer received the data, go on with the rest if > 64K */
            recv_data = rest;
#endif /* TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */
          }
        }

        /* If a FIN segment was received, we call the callback
           function with a NULL buffer to indicate EOF. */
        if (recv_flags & TF_GOT_FIN) {
          if (pcb->refused_data != NULL) {
            /* Delay this if we have refused data. */
            pcb->refused_data->flags |= PBUF_FLAG_TCP_FIN;
          } else {
            /* correct rcv_wnd as the application won't call tcp_recved()
               for the FIN's seqno */
            if (pcb->rcv_wnd != TCP_WND_MAX(pcb)) {
              pcb->rcv_wnd++;
            }
            TCP_EVENT_CLOSED(pcb, err);
            if (err == ERR_ABRT) {
              goto aborted;
            }
          }
        }

        tcp_input_pcb = NULL;
        if (tcp_input_delayed_close(pcb)) {
          goto aborted;
        }
        /* Try to send something out. */
        tcp_output(pcb);
#if TCP_INPUT_DEBUG
#if TCP_DEBUG
        tcp_debug_print_state(pcb->state);
#endif /* TCP_DEBUG */
#endif /* TCP_INPUT_DEBUG */
      }
    }
    /* Jump target if pcb has been aborted in a callback (by calling tcp_abort()).
       Below this line, 'pcb' may not be dereferenced! */
aborted:
    tcp_input_pcb = NULL;
    recv_data = NULL;

    /* give up our reference to inseg.p */
    if (inseg.p != NULL)
    {
      pbuf_free(inseg.p);
      inseg.p = NULL;
    }
  } else {

    /* If no matching PCB was found, send a TCP RST (reset) to the
       sender. */
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_18, P_WARNING, 0, "tcp_input: no PCB match found, resetting");
#else       
    LWIP_DEBUGF(TCP_RST_DEBUG, ("tcp_input: no PCB match found, resetting.\n"));
#endif
    if (!(TCPH_FLAGS(tcphdr) & TCP_RST)) {
      TCP_STATS_INC(tcp.proterr);
      TCP_STATS_INC(tcp.drop);
      tcp_rst(ackno, seqno + tcplen, ip_current_dest_addr(),
        ip_current_src_addr(), tcphdr->dest, tcphdr->src);
    }
    pbuf_free(p);
  }

#if LWIP_ENABLE_UNILOG
  if(!tcp_pcbs_sane()) {
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_input_19, P_WARNING, 0, "tcp_input: tcp_pcbs_sane()");
  }
#else  
  LWIP_ASSERT("tcp_input: tcp_pcbs_sane()", tcp_pcbs_sane());
#endif
  PERF_STOP("tcp_input");
  return;
dropped:
  TCP_STATS_INC(tcp.drop);
  MIB2_STATS_INC(mib2.tcpinerrs);
  pbuf_free(p);
}

/** Called from tcp_input to check for TF_CLOSED flag. This results in closing
 * and deallocating a pcb at the correct place to ensure noone references it
 * any more.
 * @returns 1 if the pcb has been closed and deallocated, 0 otherwise
 */
static int
tcp_input_delayed_close(struct tcp_pcb *pcb)
{
  if (recv_flags & TF_CLOSED) {
    /* The connection has been closed and we will deallocate the
        PCB. */
    if (!(pcb->flags & TF_RXCLOSED)) {
      /* Connection closed although the application has only shut down the
          tx side: call the PCB's err callback and indicate the closure to
          ensure the application doesn't continue using the PCB. */
      TCP_EVENT_ERR(pcb->state, pcb->errf, pcb->callback_arg, ERR_CLSD);
    }
    tcp_pcb_remove(&tcp_active_pcbs, pcb);
    memp_free(MEMP_TCP_PCB, pcb);
    return 1;
  }
  return 0;
}

/**
 * Called by tcp_input() when a segment arrives for a listening
 * connection (from tcp_input()).
 *
 * @param pcb the tcp_pcb_listen for which a segment arrived
 *
 * @note the segment which arrived is saved in global variables, therefore only the pcb
 *       involved is passed as a parameter to this function
 */
static void
tcp_listen_input(struct tcp_pcb_listen *pcb)
{
  struct tcp_pcb *npcb;
  u32_t iss;
  err_t rc;

  if (flags & TCP_RST) {
    /* An incoming RST should be ignored. Return. */
    return;
  }

  /* In the LISTEN state, we check for incoming SYN segments,
     creates a new PCB, and responds with a SYN|ACK. */
  if (flags & TCP_ACK) {
    /* For incoming segments with the ACK flag set, respond with a
       RST. */
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_listen_input_1, P_INFO, 0, "tcp_listen_input: ACK in LISTEN, sending reset");
#else        
    LWIP_DEBUGF(TCP_RST_DEBUG, ("tcp_listen_input: ACK in LISTEN, sending reset\n"));
#endif
    tcp_rst(ackno, seqno + tcplen, ip_current_dest_addr(),
      ip_current_src_addr(), tcphdr->dest, tcphdr->src);
  } else if (flags & TCP_SYN) {
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_listen_input_2, P_INFO, 2, "TCP connection request %u -> %u", tcphdr->src, tcphdr->dest);
#else   
    LWIP_DEBUGF(TCP_DEBUG, ("TCP connection request %"U16_F" -> %"U16_F".\n", tcphdr->src, tcphdr->dest));
#endif
#if TCP_LISTEN_BACKLOG
    if (pcb->accepts_pending >= pcb->backlog) {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_listen_input_3, P_INFO, 1, "tcp_listen_input: listen backlog exceeded for port %u", tcphdr->dest);
#else          
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_listen_input: listen backlog exceeded for port %"U16_F"\n", tcphdr->dest));
#endif
      return;
    }
#endif /* TCP_LISTEN_BACKLOG */
    npcb = tcp_alloc(pcb->prio);
    /* If a new PCB could not be created (probably due to lack of memory),
       we don't do anything, but rely on the sender will retransmit the
       SYN at a time when we have more memory available. */
    if (npcb == NULL) {
      err_t err;
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_listen_input_4, P_WARNING, 0, "tcp_listen_input: could not allocate PCB");
#else         
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_listen_input: could not allocate PCB\n"));
#endif
      TCP_STATS_INC(tcp.memerr);
      TCP_EVENT_ACCEPT(pcb, NULL, pcb->callback_arg, ERR_MEM, err);
      LWIP_UNUSED_ARG(err); /* err not useful here */
      return;
    }
#if TCP_LISTEN_BACKLOG
    pcb->accepts_pending++;
    npcb->flags |= TF_BACKLOGPEND;
#endif /* TCP_LISTEN_BACKLOG */
    /* Set up the new PCB. */
    ip_addr_copy(npcb->local_ip, *ip_current_dest_addr());
    ip_addr_copy(npcb->remote_ip, *ip_current_src_addr());
    npcb->local_port = pcb->local_port;
    npcb->remote_port = tcphdr->src;
    npcb->state = SYN_RCVD;
    npcb->rcv_nxt = seqno + 1;
    npcb->rcv_ann_right_edge = npcb->rcv_nxt;
    iss = tcp_next_iss(npcb);
    npcb->snd_wl2 = iss;
    npcb->snd_nxt = iss;
    npcb->lastack = iss;
    npcb->snd_lbb = iss;
    npcb->snd_wl1 = seqno - 1;/* initialise to seqno-1 to force window update */
    npcb->callback_arg = pcb->callback_arg;
#if LWIP_CALLBACK_API || TCP_LISTEN_BACKLOG
    npcb->listener = pcb;
#endif /* LWIP_CALLBACK_API || TCP_LISTEN_BACKLOG */
    /* inherit socket options */
    npcb->so_options = pcb->so_options & SOF_INHERITED;
    /* Register the new PCB so that we can begin receiving segments
       for it. */
    TCP_REG_ACTIVE(npcb);
    /* Parse any options in the SYN. */
    tcp_parseopt(npcb);
    npcb->snd_wnd = tcphdr->wnd;
    npcb->snd_wnd_max = npcb->snd_wnd;

#if TCP_CALCULATE_EFF_SEND_MSS
    npcb->mss = tcp_eff_send_mss(npcb->mss, &npcb->local_ip, &npcb->remote_ip);
#endif /* TCP_CALCULATE_EFF_SEND_MSS */

    MIB2_STATS_INC(mib2.tcppassiveopens);

    /* Send a SYN|ACK together with the MSS option. */
    rc = tcp_enqueue_flags(npcb, TCP_SYN | TCP_ACK);
    if (rc != ERR_OK) {
      tcp_abandon(npcb, 0);
      return;
    }
    tcp_output(npcb);
#if LWIP_TIMER_ON_DEMOND
    //tcp server accept new connection,active syncrcv timer after sending SYNC&ACK message
    sys_timeout(TCP_SYN_RCVD_TIMEOUT, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT], (void *)npcb);
    tcp_enable_timer_active_mask(npcb, LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT);
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_listen_input_5, P_INFO, 1, "enable tcp syncrcy timer %u", TCP_SYN_RCVD_TIMEOUT);

#endif    
  }
  return;
}

/**
 * Called by tcp_input() when a segment arrives for a connection in
 * TIME_WAIT.
 *
 * @param pcb the tcp_pcb for which a segment arrived
 *
 * @note the segment which arrived is saved in global variables, therefore only the pcb
 *       involved is passed as a parameter to this function
 */
static void
tcp_timewait_input(struct tcp_pcb *pcb)
{
  /* RFC 1337: in TIME_WAIT, ignore RST and ACK FINs + any 'acceptable' segments */
  /* RFC 793 3.9 Event Processing - Segment Arrives:
   * - first check sequence number - we skip that one in TIME_WAIT (always
   *   acceptable since we only send ACKs)
   * - second check the RST bit (... return) */
  if (flags & TCP_RST) {
    return;
  }
  /* - fourth, check the SYN bit, */
  if (flags & TCP_SYN) {
    /* If an incoming segment is not acceptable, an acknowledgment
       should be sent in reply */
    if (TCP_SEQ_BETWEEN(seqno, pcb->rcv_nxt, pcb->rcv_nxt + pcb->rcv_wnd)) {
      /* If the SYN is in the window it is an error, send a reset */
      tcp_rst(ackno, seqno + tcplen, ip_current_dest_addr(),
        ip_current_src_addr(), tcphdr->dest, tcphdr->src);
      return;
    }
  } else if (flags & TCP_FIN) {
    /* - eighth, check the FIN bit: Remain in the TIME-WAIT state.
         Restart the 2 MSL time-wait timeout.*/
#if LWIP_TIMER_ON_DEMOND
    //reset TIMEWAIT timer
    pcb->tmr = sys_now();
    if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT], (void *)pcb);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_timewait_input_1, P_INFO, 0, "remove tcp time wait timer");
    }
    sys_timeout(2 * TCP_MSL, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT], (void *)pcb);
    tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT);
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_timewait_input_2, P_INFO, 1, "enable tcp time wait timer %u", 2 * TCP_MSL);     
#else
    pcb->tmr = tcp_ticks;
#endif
  }

  if ((tcplen > 0)) {
    /* Acknowledge data, FIN or out-of-window SYN */
    pcb->flags |= TF_ACK_NOW;
    tcp_output(pcb);
  }
  return;
}

#if ENABLE_PSIF
 void tcp_rebuild_seg_ul_sequence_bitmap(struct tcp_seg *seg, u32_t bitmap[8])
{
    u8_t sequence;
    struct tcp_seg *seg_tmp;
    u8_t seq_need_disable = 0;

    if(seg == NULL || bitmap == NULL) {
        return;
    }

    for(sequence = 1; sequence <= 255; sequence ++) {

        for(seg_tmp = seg; seg_tmp != NULL; seg_tmp = seg_tmp->next) {
            if(getSequenceBitmap(seg_tmp->sequence_state, sequence)) {
                seq_need_disable = 1;
                break;
            }
        }

        if(seq_need_disable) {
            updateSequenceBitmap(bitmap, sequence, 0);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_rebuild_seg_ul_sequence_bitmap_1, P_INFO, 1, "tcp_rebuild_seg_ul_sequence_bitmap disable sequence %d", sequence); 
            seq_need_disable = 0;
        }

        if(sequence == 255) {
            break;
        }
    }
}
#endif

/**
 * Implements the TCP state machine. Called by tcp_input. In some
 * states tcp_receive() is called to receive data. The tcp_seg
 * argument will be freed by the caller (tcp_input()) unless the
 * recv_data pointer in the pcb is set.
 *
 * @param pcb the tcp_pcb for which a segment arrived
 *
 * @note the segment which arrived is saved in global variables, therefore only the pcb
 *       involved is passed as a parameter to this function
 */
static err_t
tcp_process(struct tcp_pcb *pcb)
{
  struct tcp_seg *rseg;
  u8_t acceptable = 0;
  err_t err;

  err = ERR_OK;
  

  /* Process incoming RST segments. */
  if (flags & TCP_RST) {
    /* First, determine if the reset is acceptable. */
    if (pcb->state == SYN_SENT) {
      /* "In the SYN-SENT state (a RST received in response to an initial SYN),
          the RST is acceptable if the ACK field acknowledges the SYN." */
      if (ackno == pcb->snd_nxt) {
        acceptable = 1;
      }
    } else {
      /* "In all states except SYN-SENT, all reset (RST) segments are validated
          by checking their SEQ-fields." */
      if (seqno == pcb->rcv_nxt) {
        acceptable = 1;
      } else  if (TCP_SEQ_BETWEEN(seqno, pcb->rcv_nxt,
                                  pcb->rcv_nxt + pcb->rcv_wnd)) {
        /* If the sequence number is inside the window, we only send an ACK
           and wait for a re-send with matching sequence number.
           This violates RFC 793, but is required to protection against
           CVE-2004-0230 (RST spoofing attack). */
        tcp_ack_now(pcb);
      }
    }

    if (acceptable) {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_1, P_INFO, 0, "tcp_process: Connection RESET");
      if(pcb->state == CLOSED) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_2, P_ERROR, 0, "tcp_process: pcb->state == CLOSED");      
      }
#else          
      LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_process: Connection RESET\n"));
      LWIP_ASSERT("tcp_input: pcb->state != CLOSED", pcb->state != CLOSED);
#endif
      recv_flags |= TF_RESET;
#if LWIP_TIMER_ON_DEMOND
      //disable delay ack timer
      if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK)) {
          sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK], (void *)pcb);
          tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK);
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_3, P_INFO, 0, "remove tcp delay ack timer");
      }
#endif 
      pcb->flags &= ~TF_ACK_DELAY;     
      return ERR_RST;
    } else {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_4, P_INFO, 2, "tcp_process: unacceptable reset seqno %u rcv_nxt %u", seqno, pcb->rcv_nxt);
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_5, P_INFO, 2, "tcp_process: Connection RESET seqno %u rcv_nxt %u", seqno, pcb->rcv_nxt);
#else     
      LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_process: unacceptable reset seqno %"U32_F" rcv_nxt %"U32_F"\n",
       seqno, pcb->rcv_nxt));
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_process: unacceptable reset seqno %"U32_F" rcv_nxt %"U32_F"\n",
       seqno, pcb->rcv_nxt));
#endif
      return ERR_OK;
    }
  }

  if ((flags & TCP_SYN) && (pcb->state != SYN_SENT && pcb->state != SYN_RCVD)) {
    /* Cope with new connection attempt after remote end crashed */
    tcp_ack_now(pcb);
    return ERR_OK;
  }

  if ((pcb->flags & TF_RXCLOSED) == 0) {
    /* Update the PCB (in)activity timer unless rx is closed (see tcp_shutdown) */
#if LWIP_TIMER_ON_DEMOND
    pcb->tmr = sys_now();
#else
    pcb->tmr = tcp_ticks;
#endif
  }
  pcb->keep_cnt_sent = 0;

  tcp_parseopt(pcb);

  /* Do different things depending on the TCP state. */
  switch (pcb->state) {
  case SYN_SENT:
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_6, P_INFO, 3, "SYN-SENT: ackno %u pcb->snd_nxt %u unacked %u", ackno, pcb->snd_nxt, lwip_ntohl(pcb->unacked->tcphdr->seqno));
#else 
    LWIP_DEBUGF(TCP_INPUT_DEBUG, ("SYN-SENT: ackno %"U32_F" pcb->snd_nxt %"U32_F" unacked %"U32_F"\n", ackno,
     pcb->snd_nxt, lwip_ntohl(pcb->unacked->tcphdr->seqno)));
#endif
    /* received SYN ACK with expected sequence number? */
    if ((flags & TCP_ACK) && (flags & TCP_SYN)
        && (ackno == pcb->lastack + 1)) {
      pcb->rcv_nxt = seqno + 1;
      pcb->rcv_ann_right_edge = pcb->rcv_nxt;
      pcb->lastack = ackno;
      pcb->snd_wnd = tcphdr->wnd;
      pcb->snd_wnd_max = pcb->snd_wnd;
      pcb->snd_wl1 = seqno - 1; /* initialise to seqno - 1 to force window update */
      pcb->state = ESTABLISHED;

#if LWIP_TIMER_ON_DEMOND
#if LWIP_TCP_KEEPALIVE
      //active keepalive timer when connection established
      if (ip_get_option(pcb, SOF_KEEPALIVE)) {
        if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT)) {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_7, P_INFO, 0, "tcp keepalive timer has active");  
        }else {
            sys_timeout(pcb->keep_idle, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT], (void *)pcb);
            tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_8, P_INFO, 1, "enable tcp keepalive timer %u", pcb->keep_intvl);     
        }
      }
#endif
#endif 

      

#if TCP_CALCULATE_EFF_SEND_MSS
      pcb->mss = tcp_eff_send_mss(pcb->mss, &pcb->local_ip, &pcb->remote_ip);
#endif /* TCP_CALCULATE_EFF_SEND_MSS */

      pcb->cwnd = LWIP_TCP_CALC_INITIAL_CWND(pcb->mss);
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_9, P_INFO, 2, "tcp_process (SENT): cwnd  %u ssthresh %u", pcb->cwnd, pcb->ssthresh);
      if(pcb->snd_queuelen <= 0) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_10, P_ERROR, 0, "pcb->snd_queuelen > 0");      
      }
#else 
      LWIP_DEBUGF(TCP_CWND_DEBUG, ("tcp_process (SENT): cwnd %"TCPWNDSIZE_F
                                   " ssthresh %"TCPWNDSIZE_F"\n",
                                   pcb->cwnd, pcb->ssthresh));
      LWIP_ASSERT("pcb->snd_queuelen > 0", (pcb->snd_queuelen > 0));
#endif
      --pcb->snd_queuelen;
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_11, P_INFO, 1, "tcp_process: SYN-SENT --queuelen %u", (tcpwnd_size_t)pcb->snd_queuelen);
#else 
      LWIP_DEBUGF(TCP_QLEN_DEBUG, ("tcp_process: SYN-SENT --queuelen %"TCPWNDSIZE_F"\n", (tcpwnd_size_t)pcb->snd_queuelen));
#endif
      rseg = pcb->unacked;
      if (rseg == NULL) {
        /* might happen if tcp_output fails in tcp_rexmit_rto()
           in which case the segment is on the unsent list */
        rseg = pcb->unsent;

#if LWIP_ENABLE_UNILOG
        if(rseg == NULL) {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_12, P_ERROR, 0, "no segment to free");
        }
        else
        {
            pcb->unsent = rseg->next;
        }
#else        
        LWIP_ASSERT("no segment to free", rseg != NULL);
#endif
        
      } else {
        pcb->unacked = rseg->next;
      }
      tcp_seg_free(rseg);

      /* If there's nothing left to acknowledge, stop the retransmit
         timer, otherwise reset it to start again */
      if (pcb->unacked == NULL) {
#if LWIP_TIMER_ON_DEMOND
        //deactive retry timer when no need retry
        if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY)) {
          sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_RETRY], (void *)pcb);
          tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY);
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_13, P_INFO, 1, "remove tcp retry timer, pcb 0x%x", (void *)pcb);
        }
#endif
        pcb->rtime = -1;
      } else {
        pcb->rtime = 0;
        pcb->nrtx = 0;
#if LWIP_TIMER_ON_DEMOND
        //restart retry timer
        if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY)) {
          sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_RETRY], (void *)pcb);
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_14, P_INFO, 1, "remove tcp retry timer, pcb 0x%x", (void *)pcb);
        }
        sys_timeout(pcb->rto * TCP_SLOW_INTERVAL, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_RETRY], (void *)pcb);
        tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY);        
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_15, P_INFO, 2, "enable tcp retry timer %u, pcb 0x%x", pcb->rto * TCP_SLOW_INTERVAL, (void *)pcb);  
#endif

      }

      /* Call the user specified function to call when successfully
       * connected. */
#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
  if(pcb->pcb_hib_sleep2_mode_flag == PCB_HIB_ENABLE_DEACTIVE) {
     ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_43, P_SIG, 0, "tcp_process THE tcp pcb(enable  hib/sleep2) has established");
     tcp_set_hib_sleep2_state(pcb, PCB_HIB_ENABLE_ACTIVE);
     tcp_set_hib_sleep2_pcb_list(pcb);
  }
#endif       
      TCP_EVENT_CONNECTED(pcb, ERR_OK, err);
      if (err == ERR_ABRT) {
        return ERR_ABRT;
      }
      tcp_ack_now(pcb);
    }
    /* received ACK? possibly a half-open connection */
    else if (flags & TCP_ACK) {
      /* send a RST to bring the other side in a non-synchronized state. */
      tcp_rst(ackno, seqno + tcplen, ip_current_dest_addr(),
        ip_current_src_addr(), tcphdr->dest, tcphdr->src);
      /* Resend SYN immediately (don't wait for rto timeout) to establish
        connection faster, but do not send more SYNs than we otherwise would
        have, or we might get caught in a loop on loopback interfaces. */
      if (pcb->nrtx < TCP_SYNMAXRTX) {   
        pcb->rtime = 0;         
        tcp_rexmit_rto(pcb);
#if LWIP_TIMER_ON_DEMOND
        //reset retry timer
        if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY)) {
          sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_RETRY], (void *)pcb);
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_16, P_INFO, 1, "remove tcp retry timer, pcb 0x%x", (void *)pcb);
        }

        sys_timeout(pcb->rto * TCP_SLOW_INTERVAL, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_RETRY], (void *)pcb);
        tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_17, P_INFO, 2, "enable tcp retry timer %u, pcb 0x%x", pcb->rto * TCP_SLOW_INTERVAL, (void *)pcb);  
#endif        
      }
    }
    break;
  case SYN_RCVD:
    if (flags & TCP_ACK) {
      /* expected ACK number? */
      if (TCP_SEQ_BETWEEN(ackno, pcb->lastack+1, pcb->snd_nxt)) {
        pcb->state = ESTABLISHED;

#if LWIP_TIMER_ON_DEMOND
      //deactive SYNCRCV timer since connection has establish
      if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT)) {
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT); 
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_18, P_INFO, 0, "remove tcp syncrcv timer");
      }
#endif

#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_19, P_INFO, 2, "TCP connection established %u -> %u", inseg.tcphdr->src, inseg.tcphdr->dest);
#else        
        LWIP_DEBUGF(TCP_DEBUG, ("TCP connection established %"U16_F" -> %"U16_F".\n", inseg.tcphdr->src, inseg.tcphdr->dest));
#endif
#if LWIP_CALLBACK_API || TCP_LISTEN_BACKLOG
#if LWIP_CALLBACK_API
#if LWIP_ENABLE_UNILOG
        if((pcb->listener != NULL) && (pcb->listener->accept == NULL)) {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_20, P_ERROR, 0, "pcb->listener->accept != NULL");
        }
#else
        LWIP_ASSERT("pcb->listener->accept != NULL",
          (pcb->listener == NULL) || (pcb->listener->accept != NULL));
#endif
#endif
        if (pcb->listener == NULL) {
          /* listen pcb might be closed by now */
          err = ERR_VAL;
        } else
#endif /* LWIP_CALLBACK_API || TCP_LISTEN_BACKLOG */
        {
          tcp_backlog_accepted(pcb);
          /* Call the accept function. */
          TCP_EVENT_ACCEPT(pcb->listener, pcb, pcb->callback_arg, ERR_OK, err);
        }
        if (err != ERR_OK) {
          /* If the accept function returns with an error, we abort
           * the connection. */
          /* Already aborted? */
          if (err != ERR_ABRT) {
            tcp_abort(pcb);
          }
          return ERR_ABRT;
        }
        /* If there was any data contained within this ACK,
         * we'd better pass it on to the application as well. */
        tcp_receive(pcb);

        /* Prevent ACK for SYN to generate a sent event */
        if (recv_acked != 0) {
          recv_acked--;
        }

        pcb->cwnd = LWIP_TCP_CALC_INITIAL_CWND(pcb->mss);
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_21, P_INFO, 2, "tcp_process (SYN_RCVD): cwnd %u ssthresh %u", pcb->cwnd, pcb->ssthresh);
#else        
        LWIP_DEBUGF(TCP_CWND_DEBUG, ("tcp_process (SYN_RCVD): cwnd %"TCPWNDSIZE_F
                                     " ssthresh %"TCPWNDSIZE_F"\n",
                                     pcb->cwnd, pcb->ssthresh));
#endif

        if (recv_flags & TF_GOT_FIN) {
          tcp_ack_now(pcb);
          pcb->state = CLOSE_WAIT;
        }
#if LWIP_TIMER_ON_DEMOND
        else {
#if LWIP_TCP_KEEPALIVE
      //active keepalive timer when connection established
        if (ip_get_option(pcb, SOF_KEEPALIVE)) {
            if(!tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT)) {
                sys_timeout(pcb->keep_idle, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT], (void *)pcb);
                tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT);
                ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_22, P_INFO, 1, "enable tcp keepalive timer %u", pcb->keep_intvl);
            }
        }
#endif            
        }
#endif
      } else {
        /* incorrect ACK number, send RST */
        tcp_rst(ackno, seqno + tcplen, ip_current_dest_addr(),
          ip_current_src_addr(), tcphdr->dest, tcphdr->src);
      }
    } else if ((flags & TCP_SYN) && (seqno == pcb->rcv_nxt - 1)) {
      /* Looks like another copy of the SYN - retransmit our SYN-ACK */
      tcp_rexmit(pcb);
    }
    break;
  case CLOSE_WAIT:
    /* FALLTHROUGH */
  case ESTABLISHED:    
    tcp_receive(pcb);
    if (recv_flags & TF_GOT_FIN) { /* passive close */
      tcp_ack_now(pcb);
      pcb->state = CLOSE_WAIT;
#if LWIP_TIMER_ON_DEMOND
#if LWIP_TCP_KEEPALIVE
      //deactive keepalive timer since receive FIN message
      if (ip_get_option(pcb, SOF_KEEPALIVE)) {
        if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT)){
            sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT], (void *)pcb);
            tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_23, P_INFO, 0, "remove tcp keepalive timer");
        }
      }
#endif
#endif       
    }
#if LWIP_TIMER_ON_DEMOND
    else
    {
        //active ooseq time if oos segment is not NULL
        if(pcb->ooseq != NULL) {
            if(!tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT)){
                sys_timeout(pcb->rto * TCP_OOSEQ_TIMEOUT * TCP_SLOW_INTERVAL, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT], (void *)pcb);
                tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT);
                ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_24, P_INFO, 1, "enable tcp ooseq timer %u", pcb->rto * TCP_OOSEQ_TIMEOUT * TCP_SLOW_INTERVAL);
            }
            else {
                ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_25, P_INFO, 0, "tcp ooseq timer has active");
            }
        }else {
            if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT)){
                sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT], (void *)pcb);
                tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT);
                ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_26, P_INFO, 0, "disable tcp ooseq timer");
            }        
        }
#if LWIP_TCP_KEEPALIVE
        // if keepalive timer is deactived, active it     
        if (ip_get_option(pcb, SOF_KEEPALIVE)) {
             if(!tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT)){
                sys_timeout(pcb->keep_idle, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT], (void *)pcb);
                tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT);
                ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_27, P_INFO, 1, "enable tcp keepalive timer %u", pcb->keep_idle);
            }
        }
#endif    
    }
#endif
    break;
  case FIN_WAIT_1:
    tcp_receive(pcb);
    if (recv_flags & TF_GOT_FIN) {
      if ((flags & TCP_ACK) && (ackno == pcb->snd_nxt) &&
          pcb->unsent == NULL) {
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_28, P_INFO, 2, "TCP connection closed: FIN_WAIT_1 %u -> %u", inseg.tcphdr->src, inseg.tcphdr->dest);
#else          
        LWIP_DEBUGF(TCP_DEBUG,
          ("TCP connection closed: FIN_WAIT_1 %"U16_F" -> %"U16_F".\n", inseg.tcphdr->src, inseg.tcphdr->dest));
#endif
        tcp_ack_now(pcb);
        tcp_pcb_purge(pcb);
        TCP_RMV_ACTIVE(pcb);
        pcb->state = TIME_WAIT;
        TCP_REG(&tcp_tw_pcbs, pcb);
#if LWIP_TIMER_ON_DEMOND
        //active TIMEWAIT timer
        if(!tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT)){
            sys_timeout(2 * TCP_MSL, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT], (void *)pcb);
            tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_29, P_INFO, 1, "enable tcp timewait timeout %u", 2 * TCP_MSL);
        }else {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_30, P_WARNING, 0, "tcp keepalive timer has active");
        }
#endif
      } else {
        tcp_ack_now(pcb);
        pcb->state = CLOSING;
      }
    } else if ((flags & TCP_ACK) && (ackno == pcb->snd_nxt) &&
               pcb->unsent == NULL) {
      pcb->state = FIN_WAIT_2;
#if LWIP_TIMER_ON_DEMOND
      //active FIN_WAIT2 timer
      if(!tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT)){
          sys_timeout(TCP_FIN_WAIT_TIMEOUT, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT], (void *)pcb);
          tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT);
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_31, P_INFO, 1, "enable tcp fin wait2 %u", TCP_FIN_WAIT_TIMEOUT);
      }else {
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_32, P_WARNING, 0, "tcp FIN WAIT2 timer has active");
      }
#endif      
    }
    break;
  case FIN_WAIT_2:
    tcp_receive(pcb);
    if (recv_flags & TF_GOT_FIN) {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_33, P_INFO, 2, "TCP connection closed: FIN_WAIT_2 %u -> %u", inseg.tcphdr->src, inseg.tcphdr->dest);
#else         
      LWIP_DEBUGF(TCP_DEBUG, ("TCP connection closed: FIN_WAIT_2 %"U16_F" -> %"U16_F".\n", inseg.tcphdr->src, inseg.tcphdr->dest));
#endif
      tcp_ack_now(pcb);
      tcp_pcb_purge(pcb);
      TCP_RMV_ACTIVE(pcb);
      pcb->state = TIME_WAIT;
#if LWIP_TIMER_ON_DEMOND
      //deactive FIN_WAIT2 timer
      if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT)){
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_34, P_INFO, 0, "remove fin wait2 timer");
      }else {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_35, P_WARNING, 0, "tcp FIN WAIT2 timer has deactive");
      }

      //active TIME_WAIT timer
      if(!tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT)){
        sys_timeout(2 * TCP_MSL, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT], (void *)pcb);
        tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_36, P_INFO, 1, "enable tcp time wait timer %u", 2 * TCP_MSL);
      }else {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_37, P_WARNING, 0, "tcp TIME WAIT timer has active");
      }
#endif      
      TCP_REG(&tcp_tw_pcbs, pcb);
    }
    break;
  case CLOSING:
    tcp_receive(pcb);
    if ((flags & TCP_ACK) && ackno == pcb->snd_nxt && pcb->unsent == NULL) {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_38, P_INFO, 2, "TCP connection closed: CLOSING %u -> %u", inseg.tcphdr->src, inseg.tcphdr->dest);
#else         
      LWIP_DEBUGF(TCP_DEBUG, ("TCP connection closed: CLOSING %"U16_F" -> %"U16_F".\n", inseg.tcphdr->src, inseg.tcphdr->dest));
#endif
      tcp_pcb_purge(pcb);
      TCP_RMV_ACTIVE(pcb);
      pcb->state = TIME_WAIT;
      TCP_REG(&tcp_tw_pcbs, pcb);
#if LWIP_TIMER_ON_DEMOND
      //active TIME_WAIT timer
      if(!tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT)){
        sys_timeout(2 * TCP_MSL, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT], (void *)pcb);
        tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_39, P_INFO, 1, "enable tcp time wait timer %u", 2 * TCP_MSL);
      }else {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_40, P_WARNING, 0, "tcp TIME WAIT timer has active");
      }

#endif      
    }
    break;
  case LAST_ACK:
    tcp_receive(pcb);
    if ((flags & TCP_ACK) && ackno == pcb->snd_nxt && pcb->unsent == NULL) {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_41, P_INFO, 2, "TCP connection closed: LAST_ACK %u -> %u", inseg.tcphdr->src, inseg.tcphdr->dest);
#else        
      LWIP_DEBUGF(TCP_DEBUG, ("TCP connection closed: LAST_ACK %"U16_F" -> %"U16_F".\n", inseg.tcphdr->src, inseg.tcphdr->dest));
#endif
      /* bugfix #21699: don't set pcb->state to CLOSED here or we risk leaking segments */
      recv_flags |= TF_CLOSED;
#if LWIP_TIMER_ON_DEMOND
      //deactive TIME_WAIT timer
      if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_LASTACK_TIMEOUT)){
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_LASTACK_TIMEOUT], (void *)pcb);
        tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_LASTACK_TIMEOUT);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_process_42, P_INFO, 0, "remove tcp last ack timer");
      }
#endif

    }
    break;
  default:
    break;
  }
  return ERR_OK;
}

#if TCP_QUEUE_OOSEQ
/**
 * Insert segment into the list (segments covered with new one will be deleted)
 *
 * Called from tcp_receive()
 */
static void
tcp_oos_insert_segment(struct tcp_seg *cseg, struct tcp_seg *next)
{
  struct tcp_seg *old_seg;

  if (TCPH_FLAGS(cseg->tcphdr) & TCP_FIN) {
    /* received segment overlaps all following segments */
    tcp_segs_free(next);
    next = NULL;
  } else {
    /* delete some following segments
       oos queue may have segments with FIN flag */
    while (next &&
           TCP_SEQ_GEQ((seqno + cseg->len),
                      (next->tcphdr->seqno + next->len))) {
      /* cseg with FIN already processed */
      if (TCPH_FLAGS(next->tcphdr) & TCP_FIN) {
        TCPH_SET_FLAG(cseg->tcphdr, TCP_FIN);
      }
      old_seg = next;
      next = next->next;
      tcp_seg_free(old_seg);
    }
    if (next &&
        TCP_SEQ_GT(seqno + cseg->len, next->tcphdr->seqno)) {
      /* We need to trim the incoming segment. */
      cseg->len = (u16_t)(next->tcphdr->seqno - seqno);
      pbuf_realloc(cseg->p, cseg->len);
    }
  }
  cseg->next = next;
}
#endif /* TCP_QUEUE_OOSEQ */

#if ENABLE_PSIF

u32_t tcp_get_oos_pkg_total_size(struct tcp_pcb *pcb)
{
    u32_t oos_total_size = 0;
    struct tcp_seg *next;

    if(pcb == PNULL)
    {
        return oos_total_size;
    }

    if(pcb->state == ESTABLISHED)
    {
        for (next = pcb->ooseq; next != NULL; next = next->next)
        {
            struct pbuf *p = next->p;
            if(p != PNULL)
            {
                oos_total_size += p->tot_len;
            }
        }
    }

    return oos_total_size;
}

struct tcp_pcb *tcp_get_oos_pkg_total_size_max_pcb(void)
{
    struct tcp_pcb *pcb = PNULL;
    struct tcp_pcb *pcb_max = PNULL;
    u32_t oos_total_size_tmp = 0;
    u32_t oos_total_size = 0;

    for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next)
    {
        oos_total_size_tmp = tcp_get_oos_pkg_total_size(pcb);
        if( oos_total_size_tmp > oos_total_size)
        {
            pcb_max = pcb;
            oos_total_size = oos_total_size_tmp;
        }
    }

    return pcb_max;
}

void tcp_free_last_oos_pkg(struct tcp_pcb *pcb)
{
    struct tcp_seg *prev = PNULL;
    struct tcp_seg *next = PNULL;

    if(pcb == PNULL)
    {
        return;
    }

    for (next = pcb->ooseq; next != NULL; prev = next, next = next->next) {
        if(next->next == PNULL)
        {
            tcp_segs_free(next);
            if(prev == NULL)
            {
                pcb->ooseq = NULL;
            }
            else
            {
                prev->next = NULL;
            }
            break;
        }
    }

    //check whether need disable oos timer;
    if(pcb->ooseq == PNULL)
    {
        if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT)) {
            sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT], (void *)pcb);
            tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_free_last_oos_pkg_1, P_INFO, 0, "remove tcp ooseq timer");
        }        
    }

    
}

void tcp_check_oos_pkg_size_high_water(void)
{
    struct tcp_pcb *pcb = PNULL;

    u16_t max_mtu;

    //get the max mtu size
    max_mtu = netif_get_max_mtu_size();

    while(PsMmGetRemainFreeSize() < max_mtu)
    {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_check_oos_pkg_size_high_water_1, P_INFO, 1, "tcp_check_oos_pkg_size_high_water mtu %u", max_mtu);
        
        //get the tcp pcb which oos pkg size is the largest one
        pcb = tcp_get_oos_pkg_total_size_max_pcb();

        if(pcb != PNULL)
        {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_check_oos_pkg_size_high_water_2, P_INFO, 0, "tcp_check_oos_pkg_size_high_water free pcb oos last pkg");

            //free the last oos pkg
            tcp_free_last_oos_pkg(pcb);
        }
        else
        {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_check_oos_pkg_size_high_water_3, P_WARNING, 0, "tcp_check_oos_pkg_size_high_water no any oos pkg within any tcp pcb");
            break;
        }
    }

}

#endif

/**
 * Called by tcp_process. Checks if the given segment is an ACK for outstanding
 * data, and if so frees the memory of the buffered data. Next, it places the
 * segment on any of the receive queues (pcb->recved or pcb->ooseq). If the segment
 * is buffered, the pbuf is referenced by pbuf_ref so that it will not be freed until
 * it has been removed from the buffer.
 *
 * If the incoming segment constitutes an ACK for a segment that was used for RTT
 * estimation, the RTT is estimated here as well.
 *
 * Called from tcp_process().
 */
static void
tcp_receive(struct tcp_pcb *pcb)
{
  struct tcp_seg *next;
#if TCP_QUEUE_OOSEQ
  struct tcp_seg *prev, *cseg;
#endif /* TCP_QUEUE_OOSEQ */
  s32_t off;
  s16_t m;
  u32_t right_wnd_edge;
  u16_t new_tot_len;
  int found_dupack = 0;
#if TCP_OOSEQ_MAX_BYTES || TCP_OOSEQ_MAX_PBUFS
  u32_t ooseq_blen;
  u16_t ooseq_qlen;
#endif /* TCP_OOSEQ_MAX_BYTES || TCP_OOSEQ_MAX_PBUFS */

#if LWIP_ENABLE_UNILOG
  if(pcb->state < ESTABLISHED) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_1, P_ERROR, 0, "tcp_receive: wrong state");
  }
#else
  LWIP_ASSERT("tcp_receive: wrong state", pcb->state >= ESTABLISHED);
#endif

  if (flags & TCP_ACK) {
    right_wnd_edge = pcb->snd_wnd + pcb->snd_wl2;

    /* Update window. */
    if (TCP_SEQ_LT(pcb->snd_wl1, seqno) ||
       (pcb->snd_wl1 == seqno && TCP_SEQ_LT(pcb->snd_wl2, ackno)) ||
       (pcb->snd_wl2 == ackno && (u32_t)SND_WND_SCALE(pcb, tcphdr->wnd) > pcb->snd_wnd)) {
      pcb->snd_wnd = SND_WND_SCALE(pcb, tcphdr->wnd);
      /* keep track of the biggest window announced by the remote host to calculate
         the maximum segment size */
      if (pcb->snd_wnd_max < pcb->snd_wnd) {
        pcb->snd_wnd_max = pcb->snd_wnd;
      }
      pcb->snd_wl1 = seqno;
      pcb->snd_wl2 = ackno;
      if (pcb->snd_wnd == 0) {
        if (pcb->persist_backoff == 0) {
          /* start persist timer */
          pcb->persist_cnt = 0;
          pcb->persist_backoff = 1;
        }
      } else if (pcb->persist_backoff > 0) {
        /* stop persist timer */
          pcb->persist_backoff = 0;
      }
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_2, P_INFO, 1, "tcp_receive: window update %u", pcb->snd_wnd);
#else      
      LWIP_DEBUGF(TCP_WND_DEBUG, ("tcp_receive: window update %"TCPWNDSIZE_F"\n", pcb->snd_wnd));
#endif
#if TCP_WND_DEBUG
    } else {
      if (pcb->snd_wnd != (tcpwnd_size_t)SND_WND_SCALE(pcb, tcphdr->wnd)) {
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_3, P_INFO, 5, "tcp_receive: no window update lastack %u ackno %u wl1 %u seqno %u wl2 %u", pcb->lastack, ackno, pcb->snd_wl1, seqno, pcb->snd_wl2);
#else        
        LWIP_DEBUGF(TCP_WND_DEBUG,
                    ("tcp_receive: no window update lastack %"U32_F" ackno %"
                     U32_F" wl1 %"U32_F" seqno %"U32_F" wl2 %"U32_F"\n",
                     pcb->lastack, ackno, pcb->snd_wl1, seqno, pcb->snd_wl2));
#endif
      }
#endif /* TCP_WND_DEBUG */
    }

    /* (From Stevens TCP/IP Illustrated Vol II, p970.) Its only a
     * duplicate ack if:
     * 1) It doesn't ACK new data
     * 2) length of received packet is zero (i.e. no payload)
     * 3) the advertised window hasn't changed
     * 4) There is outstanding unacknowledged data (retransmission timer running)
     * 5) The ACK is == biggest ACK sequence number so far seen (snd_una)
     *
     * If it passes all five, should process as a dupack:
     * a) dupacks < 3: do nothing
     * b) dupacks == 3: fast retransmit
     * c) dupacks > 3: increase cwnd
     *
     * If it only passes 1-3, should reset dupack counter (and add to
     * stats, which we don't do in lwIP)
     *
     * If it only passes 1, should reset dupack counter
     *
     */

    /* Clause 1 */
    if (TCP_SEQ_LEQ(ackno, pcb->lastack)) {
      /* Clause 2 */
      if (tcplen == 0) {
        /* Clause 3 */
        if (pcb->snd_wl2 + pcb->snd_wnd == right_wnd_edge) {
          /* Clause 4 */
          if (pcb->rtime >= 0) {
            /* Clause 5 */
            if (pcb->lastack == ackno) {
              found_dupack = 1;
              if ((u8_t)(pcb->dupacks + 1) > pcb->dupacks) {
                ++pcb->dupacks;
              }
              if (pcb->dupacks > 3) {
                /* Inflate the congestion window, but not if it means that
                   the value overflows. */
                if ((tcpwnd_size_t)(pcb->cwnd + pcb->mss) > pcb->cwnd) {
                  pcb->cwnd += pcb->mss;
                }
              } else if (pcb->dupacks == 3) {
                /* Do fast retransmit */
                tcp_rexmit_fast(pcb);
              }
            }
          }
        }
      }
      /* If Clause (1) or more is true, but not a duplicate ack, reset
       * count of consecutive duplicate acks */
      if (!found_dupack) {
        pcb->dupacks = 0;
      }
    } else if (TCP_SEQ_BETWEEN(ackno, pcb->lastack+1, pcb->snd_nxt)) {
      /* We come here when the ACK acknowledges new data. */

      /* Reset the "IN Fast Retransmit" flag, since we are no longer
         in fast retransmit. Also reset the congestion window to the
         slow start threshold. */
      if (pcb->flags & TF_INFR) {
        pcb->flags &= ~TF_INFR;
        pcb->cwnd = pcb->ssthresh;
      }

      /* Reset the number of retransmissions. */
      pcb->nrtx = 0;
    

      /* Reset the retransmission time-out. */
      pcb->rto = (pcb->sa >> 3) + pcb->sv;


      /* Reset the fast retransmit variables. */
      pcb->dupacks = 0;
      pcb->lastack = ackno;

      /* Update the congestion control variables (cwnd and
         ssthresh). */
      if (pcb->state >= ESTABLISHED) {
        if (pcb->cwnd < pcb->ssthresh) {
          if ((tcpwnd_size_t)(pcb->cwnd + pcb->mss) > pcb->cwnd) {
            pcb->cwnd += pcb->mss;
          }
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_4, P_INFO, 1, "tcp_receive: slow start cwnd %u", pcb->cwnd);
#else            
          LWIP_DEBUGF(TCP_CWND_DEBUG, ("tcp_receive: slow start cwnd %"TCPWNDSIZE_F"\n", pcb->cwnd));
#endif
        } else {
          tcpwnd_size_t new_cwnd = (pcb->cwnd + pcb->mss * pcb->mss / pcb->cwnd);
          if (new_cwnd > pcb->cwnd) {
            pcb->cwnd = new_cwnd;
          }
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_5, P_INFO, 1, "tcp_receive: congestion avoidance cwnd %u", pcb->cwnd);
#else           
          LWIP_DEBUGF(TCP_CWND_DEBUG, ("tcp_receive: congestion avoidance cwnd %"TCPWNDSIZE_F"\n", pcb->cwnd));
#endif
        }
      }
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_6, P_INFO, 3, "tcp_receive: ACK for %u, unacked->seqno %u:%u",
                                    ackno,
                                    pcb->unacked != NULL?
                                    lwip_ntohl(pcb->unacked->tcphdr->seqno): 0,
                                    pcb->unacked != NULL?
                                    lwip_ntohl(pcb->unacked->tcphdr->seqno) + TCP_TCPLEN(pcb->unacked): 0);
#else       
      LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_receive: ACK for %"U32_F", unacked->seqno %"U32_F":%"U32_F"\n",
                                    ackno,
                                    pcb->unacked != NULL?
                                    lwip_ntohl(pcb->unacked->tcphdr->seqno): 0,
                                    pcb->unacked != NULL?
                                    lwip_ntohl(pcb->unacked->tcphdr->seqno) + TCP_TCPLEN(pcb->unacked): 0));
#endif

      /* Remove segment from the unacknowledged list if the incoming
         ACK acknowledges them. */
      while (pcb->unacked != NULL &&
             TCP_SEQ_LEQ(lwip_ntohl(pcb->unacked->tcphdr->seqno) +
                         TCP_TCPLEN(pcb->unacked), ackno)) {
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_7, P_INFO, 2, "tcp_receive: removing %u:%u from pcb->unacked", lwip_ntohl(pcb->unacked->tcphdr->seqno), 
                    lwip_ntohl(pcb->unacked->tcphdr->seqno) + TCP_TCPLEN(pcb->unacked));
#else                         
        LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_receive: removing %"U32_F":%"U32_F" from pcb->unacked\n",
                                      lwip_ntohl(pcb->unacked->tcphdr->seqno),
                                      lwip_ntohl(pcb->unacked->tcphdr->seqno) +
                                      TCP_TCPLEN(pcb->unacked)));
#endif

        next = pcb->unacked;
        pcb->unacked = pcb->unacked->next;

#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_8, P_INFO, 1, "tcp_receive: queuelen %u", (tcpwnd_size_t)pcb->snd_queuelen);
        if(pcb->snd_queuelen < pbuf_clen(next->p)) {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_9, P_ERROR, 0, "pcb->snd_queuelen >= pbuf_clen(next->p)");
        }
#else
        LWIP_DEBUGF(TCP_QLEN_DEBUG, ("tcp_receive: queuelen %"TCPWNDSIZE_F" ... ", (tcpwnd_size_t)pcb->snd_queuelen));
        LWIP_ASSERT("pcb->snd_queuelen >= pbuf_clen(next->p)", (pcb->snd_queuelen >= pbuf_clen(next->p)));
#endif

//send the UL sequence state indicate
#if ENABLE_PSIF
        if(pcb->sockid < 0) {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_38, P_ERROR, 0, "tcp_receive_37: UL sequence state indicate fail,the pcb socketid is inavlid");
        }else {
            tcp_rebuild_seg_ul_sequence_bitmap(pcb->unacked, next->sequence_state);
            udp_send_ul_state_ind(next->sequence_state, pcb->sockid, UL_SEQUENCE_STATE_SENT);
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_40, P_INFO, 1, "tcp_receive_37: UL sequence state indicate success, the pcb socket id %u", pcb->sockid);
        }
#endif

        pcb->snd_queuelen -= pbuf_clen(next->p);
        recv_acked += next->len;
        tcp_seg_free(next);

#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_10, P_INFO, 1, "%u (after freeing unacked)", (tcpwnd_size_t)pcb->snd_queuelen);
#else
        LWIP_DEBUGF(TCP_QLEN_DEBUG, ("%"TCPWNDSIZE_F" (after freeing unacked)\n", (tcpwnd_size_t)pcb->snd_queuelen));
#endif
        if (pcb->snd_queuelen != 0) {
#if LWIP_ENABLE_UNILOG
        if( pcb->unacked == NULL && pcb->unsent == NULL) {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_11, P_ERROR, 0, "tcp_receive: valid queue length");
        }
#else            
          LWIP_ASSERT("tcp_receive: valid queue length", pcb->unacked != NULL ||
                      pcb->unsent != NULL);
#endif
        }
      }

      /* If there's nothing left to acknowledge, stop the retransmit
         timer, otherwise reset it to start again */
      if (pcb->unacked == NULL) {
#if LWIP_TIMER_ON_DEMOND
        //stop tcp retry time
        if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY)){
          sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_RETRY], (void *)pcb);
          tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY);
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_12, P_INFO, 1, "remove tcp retry time, pcb 0x%x", (void *)pcb);
        }

        //stop tcp total retry time
        if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT)){
          sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT], (void *)pcb);
          tcp_disable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT);
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_tcp_total_retry_time1, P_INFO, 0, "remove tcp total retry timer");
        }
#endif        
        pcb->rtime = -1;
      } else {
        pcb->rtime = 0;
#if LWIP_TIMER_ON_DEMOND
        //restart tcp retry time
        if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY)){
          sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_RETRY], (void *)pcb);
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_13, P_INFO, 1, "remove tcp retry timer, pcb 0x%x", (void *)pcb);        
        }
        sys_timeout(pcb->rto * TCP_SLOW_INTERVAL, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_RETRY], (void *)pcb);
        tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_RETRY);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_14, P_INFO, 2, "enable tcp retry timer %u, pcb 0x%x", pcb->rto * TCP_SLOW_INTERVAL, (void *)pcb); 

      //restart tcp total retry time
      if(tcp_get_timer_active_state(pcb, LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT)){
        sys_untimeout(lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT], (void *)pcb);
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_tcp_total_retry_time_2, P_INFO, 0, "remove tcp total retry timer");        
      }
      sys_timeout(pcb->tcp_max_total_retry_time * 1000, lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT], (void *)pcb);
      tcp_enable_timer_active_mask(pcb, LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT);
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_tcp_total_retry_time_4, P_INFO, 1, "enable tcp total retry timer %u", pcb->tcp_max_total_retry_time * 1000); 

#endif        
      }

      pcb->polltmr = 0;

#if LWIP_IPV6 && LWIP_ND6_TCP_REACHABILITY_HINTS
      if (ip_current_is_v6()) {
        /* Inform neighbor reachability of forward progress. */
        nd6_reachability_hint(ip6_current_src_addr());
      }
#endif /* LWIP_IPV6 && LWIP_ND6_TCP_REACHABILITY_HINTS*/
    } else {
      /* Out of sequence ACK, didn't really ack anything */
      tcp_send_empty_ack(pcb);
    }

    /* We go through the ->unsent list to see if any of the segments
       on the list are acknowledged by the ACK. This may seem
       strange since an "unsent" segment shouldn't be acked. The
       rationale is that lwIP puts all outstanding segments on the
       ->unsent list after a retransmission, so these segments may
       in fact have been sent once. */
    while (pcb->unsent != NULL &&
           TCP_SEQ_BETWEEN(ackno, lwip_ntohl(pcb->unsent->tcphdr->seqno) +
                           TCP_TCPLEN(pcb->unsent), pcb->snd_nxt)) {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_15, P_ERROR, 2, "tcp_receive: removing %u:%u from pcb->unsent", lwip_ntohl(pcb->unsent->tcphdr->seqno),
          lwip_ntohl(pcb->unsent->tcphdr->seqno) + TCP_TCPLEN(pcb->unsent));
#else                            
      LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_receive: removing %"U32_F":%"U32_F" from pcb->unsent\n",
                                    lwip_ntohl(pcb->unsent->tcphdr->seqno), lwip_ntohl(pcb->unsent->tcphdr->seqno) +
                                    TCP_TCPLEN(pcb->unsent)));
#endif

      next = pcb->unsent;
      pcb->unsent = pcb->unsent->next;
#if TCP_OVERSIZE
      if (pcb->unsent == NULL) {
        pcb->unsent_oversize = 0;
      }
#endif /* TCP_OVERSIZE */

#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_16, P_INFO, 1, "tcp_receive: queuelen %u", (tcpwnd_size_t)pcb->snd_queuelen);
      if(pcb->snd_queuelen < pbuf_clen(next->p)) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_17, P_ERROR, 0, "pcb->snd_queuelen >= pbuf_clen(next->p)");
      }
#else
      LWIP_DEBUGF(TCP_QLEN_DEBUG, ("tcp_receive: queuelen %"TCPWNDSIZE_F" ... ", (tcpwnd_size_t)pcb->snd_queuelen));
      LWIP_ASSERT("pcb->snd_queuelen >= pbuf_clen(next->p)", (pcb->snd_queuelen >= pbuf_clen(next->p)));
#endif
      /* Prevent ACK for FIN to generate a sent event */
      pcb->snd_queuelen -= pbuf_clen(next->p);
      recv_acked += next->len;
      tcp_seg_free(next);
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_18, P_INFO, 1, "%u (after freeing unsent)", (tcpwnd_size_t)pcb->snd_queuelen);
#else      
      LWIP_DEBUGF(TCP_QLEN_DEBUG, ("%"TCPWNDSIZE_F" (after freeing unsent)\n", (tcpwnd_size_t)pcb->snd_queuelen));
#endif
      if (pcb->snd_queuelen != 0) {
#if LWIP_ENABLE_UNILOG
      if(pcb->unacked == NULL && pcb->unsent == NULL) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_19, P_ERROR, 0, "tcp_receive: valid queue length");
      }
#else        
        LWIP_ASSERT("tcp_receive: valid queue length",
          pcb->unacked != NULL || pcb->unsent != NULL);
#endif
      }
    }
    pcb->snd_buf += recv_acked;
    /* End of ACK for new data processing. */

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_20, P_INFO, 3, "tcp_receive: pcb->rttest %u rtseq %u ackno %u", pcb->rttest, pcb->rtseq, ackno);
#else
    LWIP_DEBUGF(TCP_RTO_DEBUG, ("tcp_receive: pcb->rttest %"U32_F" rtseq %"U32_F" ackno %"U32_F"\n",
                                pcb->rttest, pcb->rtseq, ackno));
#endif

    /* RTT estimation calculations. This is done by checking if the
       incoming segment acknowledges the segment we use to take a
       round-trip time measurement. */
    if (pcb->rttest && TCP_SEQ_LT(pcb->rtseq, ackno)) {
      /* diff between this shouldn't exceed 32K since this are tcp timer ticks
         and a round-trip shouldn't be that long... */
#if LWIP_TIMER_ON_DEMOND
      u32_t sys_time_now = sys_now();
      if(sys_time_now - pcb->rttest > TCP_SLOW_INTERVAL)
      {
        m = (s16_t)((sys_time_now - pcb->rttest)/TCP_SLOW_INTERVAL);
      }
      else
      {
        m = 1;
      }
#else
      m = (s16_t)(tcp_ticks - pcb->rttest);
#endif

#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_21, P_INFO, 2, "tcp_receive: experienced rtt %u ticks (%u msec)", m, m * TCP_SLOW_INTERVAL);
#else
      LWIP_DEBUGF(TCP_RTO_DEBUG, ("tcp_receive: experienced rtt %"U16_F" ticks (%"U16_F" msec).\n",
                                  m, (u16_t)(m * TCP_SLOW_INTERVAL)));
#endif

      /* This is taken directly from VJs original code in his paper */
      m = m - (pcb->sa >> 3);
      pcb->sa += m;
      if (m < 0) {
        m = -m;
      }
      m = m - (pcb->sv >> 2);
      pcb->sv += m;
      pcb->rto = (pcb->sa >> 3) + pcb->sv;

#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_22, P_INFO, 4, "tcp_receive: RTO %u (%u msec), sv %d, sa %d", pcb->rto, pcb->rto * TCP_SLOW_INTERVAL, pcb->sv, pcb->sa);
#else
      LWIP_DEBUGF(TCP_RTO_DEBUG, ("tcp_receive: RTO %"U16_F" (%"U16_F" milliseconds)\n",
                                  pcb->rto, (u16_t)(pcb->rto * TCP_SLOW_INTERVAL)));
#endif

      pcb->rttest = 0;
    }
  }

  /* If the incoming segment contains data, we must process it
     further unless the pcb already received a FIN.
     (RFC 793, chapter 3.9, "SEGMENT ARRIVES" in states CLOSE-WAIT, CLOSING,
     LAST-ACK and TIME-WAIT: "Ignore the segment text.") */
  if ((tcplen > 0) && (pcb->state < CLOSE_WAIT)) {
    /* This code basically does three things:

    +) If the incoming segment contains data that is the next
    in-sequence data, this data is passed to the application. This
    might involve trimming the first edge of the data. The rcv_nxt
    variable and the advertised window are adjusted.

    +) If the incoming segment has data that is above the next
    sequence number expected (->rcv_nxt), the segment is placed on
    the ->ooseq queue. This is done by finding the appropriate
    place in the ->ooseq queue (which is ordered by sequence
    number) and trim the segment in both ends if needed. An
    immediate ACK is sent to indicate that we received an
    out-of-sequence segment.

    +) Finally, we check if the first segment on the ->ooseq queue
    now is in sequence (i.e., if rcv_nxt >= ooseq->seqno). If
    rcv_nxt > ooseq->seqno, we must trim the first edge of the
    segment on ->ooseq before we adjust rcv_nxt. The data in the
    segments that are now on sequence are chained onto the
    incoming segment so that we only need to call the application
    once.
    */

    /* First, we check if we must trim the first edge. We have to do
       this if the sequence number of the incoming segment is less
       than rcv_nxt, and the sequence number plus the length of the
       segment is larger than rcv_nxt. */
    /*    if (TCP_SEQ_LT(seqno, pcb->rcv_nxt)) {
          if (TCP_SEQ_LT(pcb->rcv_nxt, seqno + tcplen)) {*/
    if (TCP_SEQ_BETWEEN(pcb->rcv_nxt, seqno + 1, seqno + tcplen - 1)) {
      /* Trimming the first edge is done by pushing the payload
         pointer in the pbuf downwards. This is somewhat tricky since
         we do not want to discard the full contents of the pbuf up to
         the new starting point of the data since we have to keep the
         TCP header which is present in the first pbuf in the chain.

         What is done is really quite a nasty hack: the first pbuf in
         the pbuf chain is pointed to by inseg.p. Since we need to be
         able to deallocate the whole pbuf, we cannot change this
         inseg.p pointer to point to any of the later pbufs in the
         chain. Instead, we point the ->payload pointer in the first
         pbuf to data in one of the later pbufs. We also set the
         inseg.data pointer to point to the right place. This way, the
         ->p pointer will still point to the first pbuf, but the
         ->p->payload pointer will point to data in another pbuf.

         After we are done with adjusting the pbuf pointers we must
         adjust the ->data pointer in the seg and the segment
         length.*/

      struct pbuf *p = inseg.p;
      off = pcb->rcv_nxt - seqno;
#if LWIP_ENABLE_UNILOG
      if(inseg.p == NULL || off >= 0x7fff) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_23, P_ERROR, 0, "inseg.p != NULL or insane offset!");
      }
#else      
      LWIP_ASSERT("inseg.p != NULL", inseg.p);
      LWIP_ASSERT("insane offset!", (off < 0x7fff));
#endif
      if (inseg.p->len < off) {
#if LWIP_ENABLE_UNILOG
      if(((s32_t)inseg.p->tot_len) < off) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_24, P_ERROR, 0, "pbuf too short!");
      }
#else         
        LWIP_ASSERT("pbuf too short!", (((s32_t)inseg.p->tot_len) >= off));
#endif
        new_tot_len = (u16_t)(inseg.p->tot_len - off);
        while (p->len < off) {
          off -= p->len;
          /* KJM following line changed (with addition of new_tot_len var)
             to fix bug #9076
             inseg.p->tot_len -= p->len; */
          p->tot_len = new_tot_len;
          p->len = 0;
          p = p->next;
        }
        if (pbuf_header(p, (s16_t)-off)) {
          /* Do we need to cope with this failing?  Assert for now */
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_25, P_ERROR, 0, "pbuf_header failed1");
#else           
          LWIP_ASSERT("pbuf_header failed", 0);
#endif
        }
      } else {
        if (pbuf_header(inseg.p, (s16_t)-off)) {
          /* Do we need to cope with this failing?  Assert for now */
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_26, P_ERROR, 0, "pbuf_header failed2");
#else           
          LWIP_ASSERT("pbuf_header failed", 0);
#endif
        }
      }
      inseg.len -= (u16_t)(pcb->rcv_nxt - seqno);
      inseg.tcphdr->seqno = seqno = pcb->rcv_nxt;
    }
    else {
      if (TCP_SEQ_LT(seqno, pcb->rcv_nxt)) {
        /* the whole segment is < rcv_nxt */
        /* must be a duplicate of a packet that has already been correctly handled */

#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_27, P_INFO, 1, "tcp_receive: duplicate seqno %u", seqno);
#else
        LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_receive: duplicate seqno %"U32_F"\n", seqno));
#endif
        tcp_ack_now(pcb);
      }
    }

    /* The sequence number must be within the window (above rcv_nxt
       and below rcv_nxt + rcv_wnd) in order to be further
       processed. */
    if (TCP_SEQ_BETWEEN(seqno, pcb->rcv_nxt,
                        pcb->rcv_nxt + pcb->rcv_wnd - 1)) {
      if (pcb->rcv_nxt == seqno) {
        /* The incoming segment is the next in sequence. We check if
           we have to trim the end of the segment and update rcv_nxt
           and pass the data to the application. */
        tcplen = TCP_TCPLEN(&inseg);

        if (tcplen > pcb->rcv_wnd) {

#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_28, P_INFO, 3, "tcp_receive: other end overran receive window seqno %u len %u right edge %u", seqno, tcplen, pcb->rcv_nxt + pcb->rcv_wnd);
#else            
          LWIP_DEBUGF(TCP_INPUT_DEBUG,
                      ("tcp_receive: other end overran receive window"
                       "seqno %"U32_F" len %"U16_F" right edge %"U32_F"\n",
                       seqno, tcplen, pcb->rcv_nxt + pcb->rcv_wnd));
#endif
          if (TCPH_FLAGS(inseg.tcphdr) & TCP_FIN) {
            /* Must remove the FIN from the header as we're trimming
             * that byte of sequence-space from the packet */
            TCPH_FLAGS_SET(inseg.tcphdr, TCPH_FLAGS(inseg.tcphdr) & ~(unsigned int)TCP_FIN);
          }
          /* Adjust length of segment to fit in the window. */
          TCPWND_CHECK16(pcb->rcv_wnd);
          inseg.len = (u16_t)pcb->rcv_wnd;
          if (TCPH_FLAGS(inseg.tcphdr) & TCP_SYN) {
            inseg.len -= 1;
          }
          pbuf_realloc(inseg.p, inseg.len);
          tcplen = TCP_TCPLEN(&inseg);
#if LWIP_ENABLE_UNILOG
          if((seqno + tcplen) != (pcb->rcv_nxt + pcb->rcv_wnd)) {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_29, P_ERROR, 0, "tcp_receive: segment not trimmed correctly to rcv_wnd");
          }
#else            
          LWIP_ASSERT("tcp_receive: segment not trimmed correctly to rcv_wnd\n",
                      (seqno + tcplen) == (pcb->rcv_nxt + pcb->rcv_wnd));
#endif
        }
#if TCP_QUEUE_OOSEQ
        /* Received in-sequence data, adjust ooseq data if:
           - FIN has been received or
           - inseq overlaps with ooseq */
        if (pcb->ooseq != NULL) {
          if (TCPH_FLAGS(inseg.tcphdr) & TCP_FIN) {
#if LWIP_ENABLE_UNILOG
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_30, P_INFO, 0, "tcp_receive: received in-order FIN, binning ooseq queue");
#else             
            LWIP_DEBUGF(TCP_INPUT_DEBUG,
                        ("tcp_receive: received in-order FIN, binning ooseq queue\n"));
#endif
            /* Received in-order FIN means anything that was received
             * out of order must now have been received in-order, so
             * bin the ooseq queue */
            while (pcb->ooseq != NULL) {
              struct tcp_seg *old_ooseq = pcb->ooseq;
              pcb->ooseq = pcb->ooseq->next;
              tcp_seg_free(old_ooseq);
            }
          } else {
            next = pcb->ooseq;
            /* Remove all segments on ooseq that are covered by inseg already.
             * FIN is copied from ooseq to inseg if present. */
            while (next &&
                   TCP_SEQ_GEQ(seqno + tcplen,
                               next->tcphdr->seqno + next->len)) {
              /* inseg cannot have FIN here (already processed above) */
              if ((TCPH_FLAGS(next->tcphdr) & TCP_FIN) != 0 &&
                  (TCPH_FLAGS(inseg.tcphdr) & TCP_SYN) == 0) {
                TCPH_SET_FLAG(inseg.tcphdr, TCP_FIN);
                tcplen = TCP_TCPLEN(&inseg);
              }
              prev = next;
              next = next->next;
              tcp_seg_free(prev);
            }
            /* Now trim right side of inseg if it overlaps with the first
             * segment on ooseq */
            if (next &&
                TCP_SEQ_GT(seqno + tcplen,
                           next->tcphdr->seqno)) {
              /* inseg cannot have FIN here (already processed above) */
              inseg.len = (u16_t)(next->tcphdr->seqno - seqno);
              if (TCPH_FLAGS(inseg.tcphdr) & TCP_SYN) {
                inseg.len -= 1;
              }
              pbuf_realloc(inseg.p, inseg.len);
              tcplen = TCP_TCPLEN(&inseg);
#if LWIP_ENABLE_UNILOG
              if((seqno + tcplen) != next->tcphdr->seqno) {
                ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_31, P_ERROR, 0, "tcp_receive: segment not trimmed correctly to ooseq queue");
              }
#else               
              LWIP_ASSERT("tcp_receive: segment not trimmed correctly to ooseq queue\n",
                          (seqno + tcplen) == next->tcphdr->seqno);
#endif
            }
            pcb->ooseq = next;
          }
        }
#endif /* TCP_QUEUE_OOSEQ */

        pcb->rcv_nxt = seqno + tcplen;

        /* Update the receiver's (our) window. */
#if LWIP_ENABLE_UNILOG
        if(pcb->rcv_wnd < tcplen) {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_32, P_ERROR, 0, "tcp_receive: tcplen > rcv_wnd");
        }
#else          
        LWIP_ASSERT("tcp_receive: tcplen > rcv_wnd\n", pcb->rcv_wnd >= tcplen);
#endif
        pcb->rcv_wnd -= tcplen;

        tcp_update_rcv_ann_wnd(pcb);

        /* If there is data in the segment, we make preparations to
           pass this up to the application. The ->recv_data variable
           is used for holding the pbuf that goes to the
           application. The code for reassembling out-of-sequence data
           chains its data on this pbuf as well.

           If the segment was a FIN, we set the TF_GOT_FIN flag that will
           be used to indicate to the application that the remote side has
           closed its end of the connection. */
        if (inseg.p->tot_len > 0) {
          recv_data = inseg.p;
          /* Since this pbuf now is the responsibility of the
             application, we delete our reference to it so that we won't
             (mistakingly) deallocate it. */
          inseg.p = NULL;
        }
        if (TCPH_FLAGS(inseg.tcphdr) & TCP_FIN) {
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_33, P_INFO, 0, "tcp_receive: received FIN.");
#else            
          LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_receive: received FIN.\n"));
#endif
          recv_flags |= TF_GOT_FIN;
        }

#if TCP_QUEUE_OOSEQ
        /* We now check if we have segments on the ->ooseq queue that
           are now in sequence. */
        while (pcb->ooseq != NULL &&
               pcb->ooseq->tcphdr->seqno == pcb->rcv_nxt) {

          cseg = pcb->ooseq;
          seqno = pcb->ooseq->tcphdr->seqno;

          pcb->rcv_nxt += TCP_TCPLEN(cseg);
#if LWIP_ENABLE_UNILOG
          if(pcb->rcv_wnd < TCP_TCPLEN(cseg)) {
              ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_34, P_ERROR, 0, "tcp_receive: ooseq tcplen > rcv_wnd");
          }
#else           
          LWIP_ASSERT("tcp_receive: ooseq tcplen > rcv_wnd\n",
                      pcb->rcv_wnd >= TCP_TCPLEN(cseg));
#endif
          pcb->rcv_wnd -= TCP_TCPLEN(cseg);

          tcp_update_rcv_ann_wnd(pcb);

          if (cseg->p->tot_len > 0) {
            /* Chain this pbuf onto the pbuf that we will pass to
               the application. */
            /* With window scaling, this can overflow recv_data->tot_len, but
               that's not a problem since we explicitly fix that before passing
               recv_data to the application. */
            if (recv_data) {
              pbuf_cat(recv_data, cseg->p);
            } else {
              recv_data = cseg->p;
            }
            cseg->p = NULL;
          }
          if (TCPH_FLAGS(cseg->tcphdr) & TCP_FIN) {
#if LWIP_ENABLE_UNILOG
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_35, P_INFO, 0, "tcp_receive: dequeued FIN");
#else             
            LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_receive: dequeued FIN.\n"));
#endif
            recv_flags |= TF_GOT_FIN;
            if (pcb->state == ESTABLISHED) { /* force passive close or we can move to active close */
              pcb->state = CLOSE_WAIT;
            }
          }

          pcb->ooseq = cseg->next;
          tcp_seg_free(cseg);
        }
#endif /* TCP_QUEUE_OOSEQ */


        /* Acknowledge the segment(s). */
        tcp_ack(pcb);

#if LWIP_IPV6 && LWIP_ND6_TCP_REACHABILITY_HINTS
        if (ip_current_is_v6()) {
          /* Inform neighbor reachability of forward progress. */
          nd6_reachability_hint(ip6_current_src_addr());
        }
#endif /* LWIP_IPV6 && LWIP_ND6_TCP_REACHABILITY_HINTS*/

      } else {
        /* We get here if the incoming segment is out-of-sequence. */
        tcp_send_empty_ack(pcb);
#if TCP_QUEUE_OOSEQ
        /* We queue the segment on the ->ooseq queue. */
        if (pcb->ooseq == NULL) {
          pcb->ooseq = tcp_seg_copy(&inseg);
        } else {
          /* If the queue is not empty, we walk through the queue and
             try to find a place where the sequence number of the
             incoming segment is between the sequence numbers of the
             previous and the next segment on the ->ooseq queue. That is
             the place where we put the incoming segment. If needed, we
             trim the second edges of the previous and the incoming
             segment so that it will fit into the sequence.

             If the incoming segment has the same sequence number as a
             segment on the ->ooseq queue, we discard the segment that
             contains less data. */

          prev = NULL;
          for (next = pcb->ooseq; next != NULL; next = next->next) {
            if (seqno == next->tcphdr->seqno) {
              /* The sequence number of the incoming segment is the
                 same as the sequence number of the segment on
                 ->ooseq. We check the lengths to see which one to
                 discard. */
              if (inseg.len > next->len) {
                /* The incoming segment is larger than the old
                   segment. We replace some segments with the new
                   one. */
                cseg = tcp_seg_copy(&inseg);
                if (cseg != NULL) {
                  if (prev != NULL) {
                    prev->next = cseg;
                  } else {
                    pcb->ooseq = cseg;
                  }
                  tcp_oos_insert_segment(cseg, next);
                }
                break;
              } else {
                /* Either the lengths are the same or the incoming
                   segment was smaller than the old one; in either
                   case, we ditch the incoming segment. */
                break;
              }
            } else {
              if (prev == NULL) {
                if (TCP_SEQ_LT(seqno, next->tcphdr->seqno)) {
                  /* The sequence number of the incoming segment is lower
                     than the sequence number of the first segment on the
                     queue. We put the incoming segment first on the
                     queue. */
                  cseg = tcp_seg_copy(&inseg);
                  if (cseg != NULL) {
                    pcb->ooseq = cseg;
                    tcp_oos_insert_segment(cseg, next);
                  }
                  break;
                }
              } else {
                /*if (TCP_SEQ_LT(prev->tcphdr->seqno, seqno) &&
                  TCP_SEQ_LT(seqno, next->tcphdr->seqno)) {*/
                if (TCP_SEQ_BETWEEN(seqno, prev->tcphdr->seqno+1, next->tcphdr->seqno-1)) {
                  /* The sequence number of the incoming segment is in
                     between the sequence numbers of the previous and
                     the next segment on ->ooseq. We trim trim the previous
                     segment, delete next segments that included in received segment
                     and trim received, if needed. */
                  cseg = tcp_seg_copy(&inseg);
                  if (cseg != NULL) {
                    if (TCP_SEQ_GT(prev->tcphdr->seqno + prev->len, seqno)) {
                      /* We need to trim the prev segment. */
                      prev->len = (u16_t)(seqno - prev->tcphdr->seqno);
                      pbuf_realloc(prev->p, prev->len);
                    }
                    prev->next = cseg;
                    tcp_oos_insert_segment(cseg, next);
                  }
                  break;
                }
              }
              /* If the "next" segment is the last segment on the
                 ooseq queue, we add the incoming segment to the end
                 of the list. */
              if (next->next == NULL &&
                  TCP_SEQ_GT(seqno, next->tcphdr->seqno)) {
                if (TCPH_FLAGS(next->tcphdr) & TCP_FIN) {
                  /* segment "next" already contains all data */
                  break;
                }
                next->next = tcp_seg_copy(&inseg);
                if (next->next != NULL) {
                  if (TCP_SEQ_GT(next->tcphdr->seqno + next->len, seqno)) {
                    /* We need to trim the last segment. */
                    next->len = (u16_t)(seqno - next->tcphdr->seqno);
                    pbuf_realloc(next->p, next->len);
                  }
                  /* check if the remote side overruns our receive window */
                  if (TCP_SEQ_GT((u32_t)tcplen + seqno, pcb->rcv_nxt + (u32_t)pcb->rcv_wnd)) {
#if LWIP_ENABLE_UNILOG
                    ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_36, P_INFO, 3, "tcp_receive: other end overran receive window seqno %u len %u right edge %u", seqno, tcplen, pcb->rcv_nxt + pcb->rcv_wnd);
#else                      
                    LWIP_DEBUGF(TCP_INPUT_DEBUG,
                                ("tcp_receive: other end overran receive window"
                                 "seqno %"U32_F" len %"U16_F" right edge %"U32_F"\n",
                                 seqno, tcplen, pcb->rcv_nxt + pcb->rcv_wnd));
#endif
                    if (TCPH_FLAGS(next->next->tcphdr) & TCP_FIN) {
                      /* Must remove the FIN from the header as we're trimming
                       * that byte of sequence-space from the packet */
                      TCPH_FLAGS_SET(next->next->tcphdr, TCPH_FLAGS(next->next->tcphdr) & ~TCP_FIN);
                    }
                    /* Adjust length of segment to fit in the window. */
                    next->next->len = (u16_t)(pcb->rcv_nxt + pcb->rcv_wnd - seqno);
                    pbuf_realloc(next->next->p, next->next->len);
                    tcplen = TCP_TCPLEN(next->next);
#if LWIP_ENABLE_UNILOG
                    if((seqno + tcplen) != (pcb->rcv_nxt + pcb->rcv_wnd)) {
                        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_receive_37, P_ERROR, 0, "tcp_receive: segment not trimmed correctly to rcv_wnd");
                    }
#else                     
                    LWIP_ASSERT("tcp_receive: segment not trimmed correctly to rcv_wnd\n",
                                (seqno + tcplen) == (pcb->rcv_nxt + pcb->rcv_wnd));
#endif
                  }
                }
                break;
              }
            }
            prev = next;
          }
        }

#if ENABLE_PSIF
    tcp_check_oos_pkg_size_high_water();
#else
#if TCP_OOSEQ_MAX_BYTES || TCP_OOSEQ_MAX_PBUFS
        /* Check that the data on ooseq doesn't exceed one of the limits
           and throw away everything above that limit. */
        ooseq_blen = 0;
        ooseq_qlen = 0;
        prev = NULL;
        for (next = pcb->ooseq; next != NULL; prev = next, next = next->next) {
          struct pbuf *p = next->p;
          ooseq_blen += p->tot_len;
          ooseq_qlen += pbuf_clen(p);
          if ((ooseq_blen > TCP_OOSEQ_MAX_BYTES) ||
              (ooseq_qlen > TCP_OOSEQ_MAX_PBUFS)) {
             /* too much ooseq data, dump this and everything after it */
             tcp_segs_free(next);
             if (prev == NULL) {
               /* first ooseq segment is too much, dump the whole queue */
               pcb->ooseq = NULL;
             } else {
               /* just dump 'next' and everything after it */
               prev->next = NULL;
             }
             break;
          }
        }
#endif /* TCP_OOSEQ_MAX_BYTES || TCP_OOSEQ_MAX_PBUFS */
#endif
#endif /* TCP_QUEUE_OOSEQ */
      }
    } else {
      /* The incoming segment is not within the window. */
      tcp_send_empty_ack(pcb);
    }
  } else {
    /* Segments with length 0 is taken care of here. Segments that
       fall out of the window are ACKed. */
    if (!TCP_SEQ_BETWEEN(seqno, pcb->rcv_nxt, pcb->rcv_nxt + pcb->rcv_wnd - 1)) {
      tcp_ack_now(pcb);
    }
  }
}

static u8_t
tcp_getoptbyte(void)
{
  if ((tcphdr_opt2 == NULL) || (tcp_optidx < tcphdr_opt1len)) {
    u8_t* opts = (u8_t *)tcphdr + TCP_HLEN;
    return opts[tcp_optidx++];
  } else {
    u8_t idx = (u8_t)(tcp_optidx++ - tcphdr_opt1len);
    return tcphdr_opt2[idx];
  }
}

/**
 * Parses the options contained in the incoming segment.
 *
 * Called from tcp_listen_input() and tcp_process().
 * Currently, only the MSS option is supported!
 *
 * @param pcb the tcp_pcb for which a segment arrived
 */
static void
tcp_parseopt(struct tcp_pcb *pcb)
{
  u8_t data;
  u16_t mss;
#if LWIP_TCP_TIMESTAMPS
  u32_t tsval;
#endif

  /* Parse the TCP MSS option, if present. */
  if (tcphdr_optlen != 0) {
    for (tcp_optidx = 0; tcp_optidx < tcphdr_optlen; ) {
      u8_t opt = tcp_getoptbyte();
      switch (opt) {
      case LWIP_TCP_OPT_EOL:
        /* End of options. */
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_parseopt_1, P_INFO, 0, "tcp_parseopt: EOL");
#else        
        LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: EOL\n"));
#endif
        return;
      case LWIP_TCP_OPT_NOP:
        /* NOP option. */
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_parseopt_2, P_INFO, 0, "tcp_parseopt: NOP");
#else         
        LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: NOP\n"));
#endif
        break;
      case LWIP_TCP_OPT_MSS:
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_parseopt_3, P_INFO, 0, "tcp_parseopt: MSS");
#else        
        LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: MSS\n"));
#endif
        if (tcp_getoptbyte() != LWIP_TCP_OPT_LEN_MSS || (tcp_optidx - 2 + LWIP_TCP_OPT_LEN_MSS) > tcphdr_optlen) {
          /* Bad length */
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_parseopt_4, P_WARNING, 0, "tcp_parseopt: bad length");
#else           
          LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: bad length\n"));
#endif
          return;
        }
        /* An MSS option with the right option length. */
        mss = (tcp_getoptbyte() << 8);
        mss |= tcp_getoptbyte();
        /* Limit the mss to the configured TCP_MSS and prevent division by zero */
        pcb->mss = ((mss > TCP_MSS) || (mss == 0)) ? TCP_MSS : mss;
        break;
#if LWIP_WND_SCALE
      case LWIP_TCP_OPT_WS:
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_parseopt_5, P_INFO, 0, "tcp_parseopt: WND_SCALE");
#else         
        LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: WND_SCALE\n"));
#endif
        if (tcp_getoptbyte() != LWIP_TCP_OPT_LEN_WS || (tcp_optidx - 2 + LWIP_TCP_OPT_LEN_WS) > tcphdr_optlen) {
          /* Bad length */
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_parseopt_6, P_WARNING, 0, "tcp_parseopt: bad length2");
#else            
          LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: bad length\n"));
#endif
          return;
        }
        /* An WND_SCALE option with the right option length. */
        data = tcp_getoptbyte();
        /* If syn was received with wnd scale option,
           activate wnd scale opt, but only if this is not a retransmission */
        if ((flags & TCP_SYN) && !(pcb->flags & TF_WND_SCALE)) {
          pcb->snd_scale = data;
          if (pcb->snd_scale > 14U) {
            pcb->snd_scale = 14U;
          }
          pcb->rcv_scale = TCP_RCV_SCALE;
          pcb->flags |= TF_WND_SCALE;
          /* window scaling is enabled, we can use the full receive window */
#if LWIP_ENABLE_UNILOG
          if(pcb->rcv_wnd != TCPWND_MIN16(TCP_WND) || pcb->rcv_ann_wnd != TCPWND_MIN16(TCP_WND)) { 
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_parseopt_7, P_ERROR, 0, "tcp_parseopt: window not at default value");
          }
#else           
          LWIP_ASSERT("window not at default value", pcb->rcv_wnd == TCPWND_MIN16(TCP_WND));
          LWIP_ASSERT("window not at default value", pcb->rcv_ann_wnd == TCPWND_MIN16(TCP_WND));
#endif
          pcb->rcv_wnd = pcb->rcv_ann_wnd = TCP_WND;
        }
        break;
#endif
#if LWIP_TCP_TIMESTAMPS
      case LWIP_TCP_OPT_TS:
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_parseopt_8, P_INFO, 0, "tcp_parseopt: TS");
#else           
        LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: TS\n"));
#endif
        if (tcp_getoptbyte() != LWIP_TCP_OPT_LEN_TS || (tcp_optidx - 2 + LWIP_TCP_OPT_LEN_TS) > tcphdr_optlen) {
          /* Bad length */
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_parseopt_9, P_WARNING, 0, "tcp_parseopt: bad length3");
#else           
          LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: bad length\n"));
#endif
          return;
        }
        /* TCP timestamp option with valid length */
        tsval = tcp_getoptbyte();
        tsval |= (tcp_getoptbyte() << 8);
        tsval |= (tcp_getoptbyte() << 16);
        tsval |= (tcp_getoptbyte() << 24);
        if (flags & TCP_SYN) {
          pcb->ts_recent = lwip_ntohl(tsval);
          /* Enable sending timestamps in every segment now that we know
             the remote host supports it. */
          pcb->flags |= TF_TIMESTAMP;
        } else if (TCP_SEQ_BETWEEN(pcb->ts_lastacksent, seqno, seqno+tcplen)) {
          pcb->ts_recent = lwip_ntohl(tsval);
        }
        /* Advance to next option (6 bytes already read) */
        tcp_optidx += LWIP_TCP_OPT_LEN_TS - 6;
        break;
#endif
      default:
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_parseopt_10, P_INFO, 0, "tcp_parseopt: other");
#else       
        LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: other\n"));
#endif
        data = tcp_getoptbyte();
        if (data < 2) {
#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, tcp_parseopt_11, P_WARNING, 0, "tcp_parseopt: bad length3");
#else            
          LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: bad length\n"));
#endif
          /* If the length field is zero, the options are malformed
             and we don't process them further. */
          return;
        }
        /* All other options have a length field, so that we easily
           can skip past them. */
        tcp_optidx += data - 2;
      }
    }
  }
}

void
tcp_trigger_input_pcb_close(void)
{
  recv_flags |= TF_CLOSED;
}

#endif /* LWIP_TCP */
