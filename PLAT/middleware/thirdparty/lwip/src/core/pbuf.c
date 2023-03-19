/**
 * @file
 * Packet buffer management
 */

/**
 * @defgroup pbuf Packet buffers (PBUF)
 * @ingroup infrastructure
 *
 * Packets are built from the pbuf data structure. It supports dynamic
 * memory allocation for packet contents or can reference externally
 * managed packet contents both in RAM and ROM. Quick allocation for
 * incoming packets is provided through pools with fixed sized pbufs.
 *
 * A packet may span over multiple pbufs, chained as a singly linked
 * list. This is called a "pbuf chain".
 *
 * Multiple packets may be queued, also using this singly linked list.
 * This is called a "packet queue".
 *
 * So, a packet queue consists of one or more pbuf chains, each of
 * which consist of one or more pbufs. CURRENTLY, PACKET QUEUES ARE
 * NOT SUPPORTED!!! Use helper structs to queue multiple packets.
 *
 * The differences between a pbuf chain and a packet queue are very
 * precise but subtle.
 *
 * The last pbuf of a packet has a ->tot_len field that equals the
 * ->len field. It can be found by traversing the list. If the last
 * pbuf of a packet has a ->next field other than NULL, more packets
 * are on the queue.
 *
 * Therefore, looping through a pbuf of a single packet, has an
 * loop end condition (tot_len == p->len), NOT (next == NULL).
 *
 * Example of custom pbuf usage for zero-copy RX:
  @code{.c}
typedef struct my_custom_pbuf
{
   struct pbuf_custom p;
   void* dma_descriptor;
} my_custom_pbuf_t;

LWIP_MEMPOOL_DECLARE(RX_POOL, 10, sizeof(my_custom_pbuf_t), "Zero-copy RX PBUF pool");

void my_pbuf_free_custom(void* p)
{
  my_custom_pbuf_t* my_puf = (my_custom_pbuf_t*)p;

  LOCK_INTERRUPTS();
  free_rx_dma_descriptor(my_pbuf->dma_descriptor);
  LWIP_MEMPOOL_FREE(RX_POOL, my_pbuf);
  UNLOCK_INTERRUPTS();
}

void eth_rx_irq()
{
  dma_descriptor*   dma_desc = get_RX_DMA_descriptor_from_ethernet();
  my_custom_pbuf_t* my_pbuf  = (my_custom_pbuf_t*)LWIP_MEMPOOL_ALLOC(RX_POOL);

  my_pbuf->p.custom_free_function = my_pbuf_free_custom;
  my_pbuf->dma_descriptor         = dma_desc;

  invalidate_cpu_cache(dma_desc->rx_data, dma_desc->rx_length);
  
  struct pbuf* p = pbuf_alloced_custom(PBUF_RAW,
     dma_desc->rx_length,
     PBUF_REF,
     &my_pbuf->p,
     dma_desc->rx_data,
     dma_desc->max_buffer_size);

  if(netif->input(p, netif) != ERR_OK) {
    pbuf_free(p);
  }
}
  @endcode
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

#include "lwip/stats.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#if LWIP_TCP && TCP_QUEUE_OOSEQ
#include "lwip/priv/tcp_priv.h"
#endif
#if LWIP_CHECKSUM_ON_COPY
#include "lwip/inet_chksum.h"
#endif

#include <string.h>

#if PBUF_POOL_MM_USE_CUSTOM
#include "psifmem.h"
#endif


#if ENABLE_PSIF
#define PBUF_PDU_HLEN  LWIP_MEM_ALIGN_SIZE(sizeof(DlPduBlock))
#endif

/* Since the pool is created in memp, PBUF_POOL_BUFSIZE will be automatically
   aligned there. Therefore, PBUF_POOL_BUFSIZE_ALIGNED can be used here. */
#define PBUF_POOL_BUFSIZE_ALIGNED LWIP_MEM_ALIGN_SIZE(PBUF_POOL_BUFSIZE)

#if !LWIP_TCP || !TCP_QUEUE_OOSEQ || !PBUF_POOL_FREE_OOSEQ
#define PBUF_POOL_IS_EMPTY()
#else /* !LWIP_TCP || !TCP_QUEUE_OOSEQ || !PBUF_POOL_FREE_OOSEQ */

#if !NO_SYS
#ifndef PBUF_POOL_FREE_OOSEQ_QUEUE_CALL
#include "lwip/tcpip.h"
#define PBUF_POOL_FREE_OOSEQ_QUEUE_CALL()  do { \
  if (tcpip_callback_with_block(pbuf_free_ooseq_callback, NULL, 0) != ERR_OK) { \
      SYS_ARCH_PROTECT(old_level); \
      pbuf_free_ooseq_pending = 0; \
      SYS_ARCH_UNPROTECT(old_level); \
  } } while(0)
#endif /* PBUF_POOL_FREE_OOSEQ_QUEUE_CALL */
#endif /* !NO_SYS */

volatile u8_t pbuf_free_ooseq_pending;
#if !PBUF_POOL_MM_USE_CUSTOM
#define PBUF_POOL_IS_EMPTY() pbuf_pool_is_empty()
#endif

/**
 * Attempt to reclaim some memory from queued out-of-sequence TCP segments
 * if we run out of pool pbufs. It's better to give priority to new packets
 * if we're running out.
 *
 * This must be done in the correct thread context therefore this function
 * can only be used with NO_SYS=0 and through tcpip_callback.
 */
#if !PBUF_POOL_MM_USE_CUSTOM
#if !NO_SYS
static
#endif /* !NO_SYS */
void
pbuf_free_ooseq(void)
{
  struct tcp_pcb* pcb;
  SYS_ARCH_SET(pbuf_free_ooseq_pending, 0);

  for (pcb = tcp_active_pcbs; NULL != pcb; pcb = pcb->next) {
    if (NULL != pcb->ooseq) {
      /** Free the ooseq pbufs of one PCB only */
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_free_ooseq_1, P_INFO, 0, "pbuf_free_ooseq: freeing out-of-sequence pbufs");   
#else      
      LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_free_ooseq: freeing out-of-sequence pbufs\n"));
#endif
      tcp_segs_free(pcb->ooseq);
      pcb->ooseq = NULL;
      return;
    }
  }
}
#endif

#if !NO_SYS
#if !PBUF_POOL_MM_USE_CUSTOM

/**
 * Just a callback function for tcpip_callback() that calls pbuf_free_ooseq().
 */
static void
pbuf_free_ooseq_callback(void *arg)
{
  LWIP_UNUSED_ARG(arg);
  pbuf_free_ooseq();
}
#endif
#endif /* !NO_SYS */

#if !PBUF_POOL_MM_USE_CUSTOM

/** Queue a call to pbuf_free_ooseq if not already queued. */
static void
pbuf_pool_is_empty(void)
{
#ifndef PBUF_POOL_FREE_OOSEQ_QUEUE_CALL
  SYS_ARCH_SET(pbuf_free_ooseq_pending, 1);
#else /* PBUF_POOL_FREE_OOSEQ_QUEUE_CALL */
  u8_t queued;
  SYS_ARCH_DECL_PROTECT(old_level);
  SYS_ARCH_PROTECT(old_level);
  queued = pbuf_free_ooseq_pending;
  pbuf_free_ooseq_pending = 1;
  SYS_ARCH_UNPROTECT(old_level);

  if (!queued) {
    /* queue a call to pbuf_free_ooseq if not already queued */
    PBUF_POOL_FREE_OOSEQ_QUEUE_CALL();
  }
#endif /* PBUF_POOL_FREE_OOSEQ_QUEUE_CALL */
}
#endif

#endif /* !LWIP_TCP || !TCP_QUEUE_OOSEQ || !PBUF_POOL_FREE_OOSEQ */


/**
 * @ingroup pbuf
 * Allocates a pbuf of the given type (possibly a chain for PBUF_POOL type).
 *
 * The actual memory allocated for the pbuf is determined by the
 * layer at which the pbuf is allocated and the requested size
 * (from the size parameter).
 *
 * @param layer flag to define header size
 * @param length size of the pbuf's payload
 * @param type this parameter decides how and where the pbuf
 * should be allocated as follows:
 *
 * - PBUF_RAM: buffer memory for pbuf is allocated as one large
 *             chunk. This includes protocol headers as well.
 * - PBUF_ROM: no buffer memory is allocated for the pbuf, even for
 *             protocol headers. Additional headers must be prepended
 *             by allocating another pbuf and chain in to the front of
 *             the ROM pbuf. It is assumed that the memory used is really
 *             similar to ROM in that it is immutable and will not be
 *             changed. Memory which is dynamic should generally not
 *             be attached to PBUF_ROM pbufs. Use PBUF_REF instead.
 * - PBUF_REF: no buffer memory is allocated for the pbuf, even for
 *             protocol headers. It is assumed that the pbuf is only
 *             being used in a single thread. If the pbuf gets queued,
 *             then pbuf_take should be called to copy the buffer.
 * - PBUF_POOL: the pbuf is allocated as a pbuf chain, with pbufs from
 *              the pbuf pool that is allocated during pbuf_init().
 *
 * @return the allocated pbuf. If multiple pbufs where allocated, this
 * is the first pbuf of a pbuf chain.
 */
struct pbuf *
pbuf_alloc(pbuf_layer layer, u16_t length, pbuf_type type)
{
  struct pbuf *p;
  u16_t offset;

#if 0
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_alloc_1, P_INFO, 2, "pbuf_alloc length %u type %d", length, type);     
  LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_alloc(length=%"U16_F")\n", length));
#endif

  /* determine header offset */
  switch (layer) {
  case PBUF_TRANSPORT:
    /* add room for transport (often TCP) layer header */
    offset = PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN + PBUF_IP_HLEN + PBUF_TRANSPORT_HLEN;
    break;
  case PBUF_IP:
    /* add room for IP layer header */
    offset = PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN + PBUF_IP_HLEN;
    break;
  case PBUF_LINK:
    /* add room for link layer header */
    offset = PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN;
    break;
  case PBUF_RAW_TX:
    /* add room for encapsulating link layer headers (e.g. 802.11) */
    offset = PBUF_LINK_ENCAPSULATION_HLEN;
    break;
  case PBUF_RAW:
    /* no offset (e.g. RX buffers or chain successors) */
    offset = 0;
    break;
#if ENABLE_PSIF
  case PBUF_PDU:
  	/*add pdu header room for receive pdu packet*/
  	offset = PBUF_PDU_HLEN;
	break;
#endif
  default:
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_alloc_2, P_ERROR, 0, "pbuf_alloc: bad pbuf layer");   
#else   
    LWIP_ASSERT("pbuf_alloc: bad pbuf layer", 0);
#endif
    return NULL;
  }

  switch (type) {
  case PBUF_POOL:
#if PBUF_POOL_MM_USE_CUSTOM
	p = (struct pbuf *)PsMmMalloc(SIZEOF_STRUCT_PBUF + offset + length);
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_alloc_3, P_INFO, 1, "PsMmMalloc: allocated pbuf pool 0x%x", (void *)p);   
#else  
    LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("PsMmMalloc: allocated pbuf pool %p\n", (void *)p));
#endif
	if(p == NULL ) {
#if LWIP_ENABLE_UNILOG
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_alloc_4, P_WARNING, 0, "PsMmMalloc: allocated pbuf pool fail");   
#else        
		LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("PsMmMalloc: allocated pbuf pool fail\n"));
#endif
		return NULL;
	}
    p->type = type;
    p->next = NULL;

    /* make the payload pointer point 'offset' bytes into pbuf data memory */
    p->payload = LWIP_MEM_ALIGN((void *)((u8_t *)p + (SIZEOF_STRUCT_PBUF + offset)));	
	p->tot_len = length;
	p->len = length;
	p->ref = 1;
	
#else 	
    /* allocate head of pbuf chain into p */
	struct pbuf *q, *r;
	s32_t rem_len; /* remaining length */

    p = (struct pbuf *)memp_malloc(MEMP_PBUF_POOL);
    LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_alloc: allocated pbuf %p\n", (void *)p));
    if (p == NULL) {
      PBUF_POOL_IS_EMPTY();
      return NULL;
    }
    p->type = type;
    p->next = NULL;

    /* make the payload pointer point 'offset' bytes into pbuf data memory */
    p->payload = LWIP_MEM_ALIGN((void *)((u8_t *)p + (SIZEOF_STRUCT_PBUF + offset)));
    LWIP_ASSERT("pbuf_alloc: pbuf p->payload properly aligned",
            ((mem_ptr_t)p->payload % MEM_ALIGNMENT) == 0);
    /* the total length of the pbuf chain is the requested size */
    p->tot_len = length;
    /* set the length of the first pbuf in the chain */
    p->len = LWIP_MIN(length, PBUF_POOL_BUFSIZE_ALIGNED - LWIP_MEM_ALIGN_SIZE(offset));
    LWIP_ASSERT("check p->payload + p->len does not overflow pbuf",
                ((u8_t*)p->payload + p->len <=
                 (u8_t*)p + SIZEOF_STRUCT_PBUF + PBUF_POOL_BUFSIZE_ALIGNED));
    LWIP_ASSERT("PBUF_POOL_BUFSIZE must be bigger than MEM_ALIGNMENT",
      (PBUF_POOL_BUFSIZE_ALIGNED - LWIP_MEM_ALIGN_SIZE(offset)) > 0 );
    /* set reference count (needed here in case we fail) */
    p->ref = 1;

    /* now allocate the tail of the pbuf chain */

    /* remember first pbuf for linkage in next iteration */
    r = p;
    /* remaining length to be allocated */
    rem_len = length - p->len;
    /* any remaining pbufs to be allocated? */
    while (rem_len > 0) {
      q = (struct pbuf *)memp_malloc(MEMP_PBUF_POOL);
      if (q == NULL) {
        PBUF_POOL_IS_EMPTY();
        /* free chain so far allocated */
        pbuf_free(p);
        /* bail out unsuccessfully */
        return NULL;
      }
      q->type = type;
      q->flags = 0;
      q->next = NULL;
      /* make previous pbuf point to this pbuf */
      r->next = q;
      /* set total length of this pbuf and next in chain */
      LWIP_ASSERT("rem_len < max_u16_t", rem_len < 0xffff);
      q->tot_len = (u16_t)rem_len;
      /* this pbuf length is pool size, unless smaller sized tail */
      q->len = LWIP_MIN((u16_t)rem_len, PBUF_POOL_BUFSIZE_ALIGNED);
      q->payload = (void *)((u8_t *)q + SIZEOF_STRUCT_PBUF);
      LWIP_ASSERT("pbuf_alloc: pbuf q->payload properly aligned",
              ((mem_ptr_t)q->payload % MEM_ALIGNMENT) == 0);
      LWIP_ASSERT("check p->payload + p->len does not overflow pbuf",
                  ((u8_t*)p->payload + p->len <=
                   (u8_t*)p + SIZEOF_STRUCT_PBUF + PBUF_POOL_BUFSIZE_ALIGNED));
      q->ref = 1;
      /* calculate remaining length to be allocated */
      rem_len -= q->len;
      /* remember this pbuf for linkage in next iteration */
      r = q;
    }
    /* end of chain */
    /*r->next = NULL;*/
#endif
    break;
  case PBUF_RAM:
    {
      mem_size_t alloc_len = LWIP_MEM_ALIGN_SIZE(SIZEOF_STRUCT_PBUF + offset) + LWIP_MEM_ALIGN_SIZE(length);
      
      /* bug #50040: Check for integer overflow when calculating alloc_len */
      if (alloc_len < LWIP_MEM_ALIGN_SIZE(length)) {
        return NULL;
      }
    
      /* If pbuf is to be allocated in RAM, allocate memory for it. */
      p = (struct pbuf*)mem_malloc(alloc_len);
    }

    if (p == NULL) {
      return NULL;
    }
    /* Set up internal structure of the pbuf. */
    p->payload = LWIP_MEM_ALIGN((void *)((u8_t *)p + SIZEOF_STRUCT_PBUF + offset));
    p->len = p->tot_len = length;
    p->next = NULL;
    p->type = type;

#if LWIP_ENABLE_UNILOG
    if(((mem_ptr_t)p->payload % MEM_ALIGNMENT) != 0) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_alloc_5, P_ERROR, 0, "pbuf_alloc: pbuf->payload properly aligned");   
    }
#else  
    LWIP_ASSERT("pbuf_alloc: pbuf->payload properly aligned",
           ((mem_ptr_t)p->payload % MEM_ALIGNMENT) == 0);
#endif
    break;
  /* pbuf references existing (non-volatile static constant) ROM payload? */
  case PBUF_ROM:
  /* pbuf references existing (externally allocated) RAM payload? */
  case PBUF_REF:
    /* only allocate memory for the pbuf structure */
    p = (struct pbuf *)memp_malloc(MEMP_PBUF);
    if (p == NULL) {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_alloc_6, P_WARNING, 1, "pbuf_alloc: Could not allocate MEMP_PBUF for PBUF type %d", type);   
#else         
      LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
                  ("pbuf_alloc: Could not allocate MEMP_PBUF for PBUF_%s.\n",
                  (type == PBUF_ROM) ? "ROM" : "REF"));
#endif
      return NULL;
    }
    /* caller must set this field properly, afterwards */
    p->payload = NULL;
    p->len = p->tot_len = length;
    p->next = NULL;
    p->type = type;
    break;
  default:

#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_alloc_7, P_ERROR, 0, "pbuf_alloc: erroneous type");   
#else    
    LWIP_ASSERT("pbuf_alloc: erroneous type", 0);
#endif
    return NULL;
  }
  /* set reference count */
  p->ref = 1;
  /* set flags */
  p->flags = 0;

#if ENABLE_PSIF
  p->bExceptData = 0;
  p->dataRai = 0;
  p->tickType = UL_PDU_START_TICK;
//  p->dataLifeTime = sys_now(); it will set when call netif->output() function
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_alloc_8, P_INFO, 3, "pbuf_alloc 0x%x, the init ticktype %u, the dataLifeTime %u", (void *)p, p->tickType, p->sysTick); 
#endif

#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_alloc_9, P_INFO, 3, "pbuf_alloc(length=%u) 0x%x  type %d", length, (void *)p, type);   
#else   
  LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_alloc(length=%"U16_F") == %p\n", length, (void *)p));
#endif

  return p;
}

#if LWIP_SUPPORT_CUSTOM_PBUF
/**
 * @ingroup pbuf
 * Initialize a custom pbuf (already allocated).
 *
 * @param l flag to define header size
 * @param length size of the pbuf's payload
 * @param type type of the pbuf (only used to treat the pbuf accordingly, as
 *        this function allocates no memory)
 * @param p pointer to the custom pbuf to initialize (already allocated)
 * @param payload_mem pointer to the buffer that is used for payload and headers,
 *        must be at least big enough to hold 'length' plus the header size,
 *        may be NULL if set later.
 *        ATTENTION: The caller is responsible for correct alignment of this buffer!!
 * @param payload_mem_len the size of the 'payload_mem' buffer, must be at least
 *        big enough to hold 'length' plus the header size
 */
struct pbuf*
pbuf_alloced_custom(pbuf_layer l, u16_t length, pbuf_type type, struct pbuf_custom *p,
                    void *payload_mem, u16_t payload_mem_len)
{
  u16_t offset;

#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_alloced_custom_1, P_INFO, 1, "pbuf_alloced_custom length %u", length);   
#else    
  LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_alloced_custom(length=%"U16_F")\n", length));
#endif

  /* determine header offset */
  switch (l) {
  case PBUF_TRANSPORT:
    /* add room for transport (often TCP) layer header */
    offset = PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN + PBUF_IP_HLEN + PBUF_TRANSPORT_HLEN;
    break;
  case PBUF_IP:
    /* add room for IP layer header */
    offset = PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN + PBUF_IP_HLEN;
    break;
  case PBUF_LINK:
    /* add room for link layer header */
    offset = PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN;
    break;
  case PBUF_RAW_TX:
    /* add room for encapsulating link layer headers (e.g. 802.11) */
    offset = PBUF_LINK_ENCAPSULATION_HLEN;
    break;
  case PBUF_RAW:
    offset = 0;
    break;
  default:
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_alloced_custom_2, P_ERROR, 0, "pbuf_alloced_custom: bad pbuf layer");   
#else     
    LWIP_ASSERT("pbuf_alloced_custom: bad pbuf layer", 0);
#endif
    return NULL;
  }

  if (LWIP_MEM_ALIGN_SIZE(offset) + length > payload_mem_len) {
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_alloced_custom_3, P_WARNING, 1, "pbuf_alloced_custom(length=%u) buffer too short", length);   
#else     
    LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_LEVEL_WARNING, ("pbuf_alloced_custom(length=%"U16_F") buffer too short\n", length));
#endif
    return NULL;
  }

  p->pbuf.next = NULL;
  if (payload_mem != NULL) {
    p->pbuf.payload = (u8_t *)payload_mem + LWIP_MEM_ALIGN_SIZE(offset);
  } else {
    p->pbuf.payload = NULL;
  }
  p->pbuf.flags = PBUF_FLAG_IS_CUSTOM;
  p->pbuf.len = p->pbuf.tot_len = length;
  p->pbuf.type = type;
  p->pbuf.ref = 1;
  return &p->pbuf;
}
#endif /* LWIP_SUPPORT_CUSTOM_PBUF */

/**
 * @ingroup pbuf
 * Shrink a pbuf chain to a desired length.
 *
 * @param p pbuf to shrink.
 * @param new_len desired new length of pbuf chain
 *
 * Depending on the desired length, the first few pbufs in a chain might
 * be skipped and left unchanged. The new last pbuf in the chain will be
 * resized, and any remaining pbufs will be freed.
 *
 * @note If the pbuf is ROM/REF, only the ->tot_len and ->len fields are adjusted.
 * @note May not be called on a packet queue.
 *
 * @note Despite its name, pbuf_realloc cannot grow the size of a pbuf (chain).
 */
void
pbuf_realloc(struct pbuf *p, u16_t new_len)
{
  struct pbuf *q;
  u16_t rem_len; /* remaining length */
  s32_t grow;

#if LWIP_ENABLE_UNILOG
  if(p == NULL || !(p->type == PBUF_POOL || p->type == PBUF_ROM || p->type == PBUF_RAM || p->type == PBUF_REF)) {
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_realloc_1, P_ERROR, 0, "pbuf_realloc INVALID argument");
      return;
  }
#else
  LWIP_ASSERT("pbuf_realloc: p != NULL", p != NULL);
  LWIP_ASSERT("pbuf_realloc: sane p->type", p->type == PBUF_POOL ||
              p->type == PBUF_ROM ||
              p->type == PBUF_RAM ||
              p->type == PBUF_REF);
#endif

  /* desired length larger than current length? */
  if (new_len >= p->tot_len) {
    /* enlarging not yet supported */
    return;
  }

  /* the pbuf chain grows by (new_len - p->tot_len) bytes
   * (which may be negative in case of shrinking) */
  grow = new_len - p->tot_len;

  /* first, step over any pbufs that should remain in the chain */
  rem_len = new_len;
  q = p;
  /* should this pbuf be kept? */
  while (rem_len > q->len) {
    /* decrease remaining length by pbuf length */
    rem_len -= q->len;
    /* decrease total length indicator */
#if LWIP_ENABLE_UNILOG
  if(grow >= 0xffff) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_realloc_2, P_ERROR, 0, "grow < max_u16_t");   
  }
#else    
    LWIP_ASSERT("grow < max_u16_t", grow < 0xffff);
#endif
    q->tot_len += (u16_t)grow;
    /* proceed to next pbuf in chain */
    q = q->next;
#if LWIP_ENABLE_UNILOG
  if(q == NULL) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_realloc_3, P_ERROR, 0, "pbuf_realloc: q != NULL");   
  }
#else     
    LWIP_ASSERT("pbuf_realloc: q != NULL", q != NULL);
#endif
  }
  /* we have now reached the new last pbuf (in q) */
  /* rem_len == desired length for pbuf q */

  /* shrink allocated memory for PBUF_RAM */
  /* (other types merely adjust their length fields */
  if ((q->type == PBUF_RAM) && (rem_len != q->len)
#if LWIP_SUPPORT_CUSTOM_PBUF
      && ((q->flags & PBUF_FLAG_IS_CUSTOM) == 0)
#endif /* LWIP_SUPPORT_CUSTOM_PBUF */
     ) {
    /* reallocate and adjust the length of the pbuf that will be split */
    q = (struct pbuf *)mem_trim(q, (u16_t)((u8_t *)q->payload - (u8_t *)q) + rem_len);
#if LWIP_ENABLE_UNILOG
  if(q == NULL) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_realloc_4, P_ERROR, 0, "mem_trim returned q == NULL");   
  }
#else     
    LWIP_ASSERT("mem_trim returned q == NULL", q != NULL);
#endif
  }
  /* adjust length fields for new last pbuf */
  q->len = rem_len;
  q->tot_len = q->len;

  /* any remaining pbufs in chain? */
  if (q->next != NULL) {
    /* free remaining pbufs in chain */
    pbuf_free(q->next);
  }
  /* q is last packet in chain */
  q->next = NULL;

}

/**
 * Adjusts the payload pointer to hide or reveal headers in the payload.
 * @see pbuf_header.
 *
 * @param p pbuf to change the header size.
 * @param header_size_increment Number of bytes to increment header size.
 * @param force Allow 'header_size_increment > 0' for PBUF_REF/PBUF_ROM types
 *
 * @return non-zero on failure, zero on success.
 *
 */
static u8_t
pbuf_header_impl(struct pbuf *p, s16_t header_size_increment, u8_t force)
{
  u16_t type;
  void *payload;
  u16_t increment_magnitude;

#if LWIP_ENABLE_UNILOG
  if(p == NULL) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_header_impl_1, P_ERROR, 0, "p != NULL");   
  }
#else 
  LWIP_ASSERT("p != NULL", p != NULL);
#endif

  if ((header_size_increment == 0) || (p == NULL)) {
    return 0;
  }

  if (header_size_increment < 0) {
    increment_magnitude = (u16_t)-header_size_increment;
    /* Check that we aren't going to move off the end of the pbuf */
#if LWIP_ENABLE_UNILOG
    if(increment_magnitude > p->len) {
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_header_impl_2, P_ERROR, 0, "increment_magnitude <= p->len");
      return 1;
    }
#else     
    LWIP_ERROR("increment_magnitude <= p->len", (increment_magnitude <= p->len), return 1;);
#endif
  } else {
    increment_magnitude = (u16_t)header_size_increment;
#if 0
    /* Can't assert these as some callers speculatively call
         pbuf_header() to see if it's OK.  Will return 1 below instead. */
    /* Check that we've got the correct type of pbuf to work with */
    LWIP_ASSERT("p->type == PBUF_RAM || p->type == PBUF_POOL",
                p->type == PBUF_RAM || p->type == PBUF_POOL);
    /* Check that we aren't going to move off the beginning of the pbuf */
    LWIP_ASSERT("p->payload - increment_magnitude >= p + SIZEOF_STRUCT_PBUF",
                (u8_t *)p->payload - increment_magnitude >= (u8_t *)p + SIZEOF_STRUCT_PBUF);
#endif
  }

  type = p->type;
  /* remember current payload pointer */
  payload = p->payload;

  /* pbuf types containing payloads? */
  if (type == PBUF_RAM || type == PBUF_POOL) {
    /* set new payload pointer */
    p->payload = (u8_t *)p->payload - header_size_increment;
    /* boundary check fails? */
    if ((u8_t *)p->payload < (u8_t *)p + SIZEOF_STRUCT_PBUF) {

#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_header_impl_3, P_WARNING, 2, "pbuf_header: failed as 0x%x < 0x%x (not enough space for new header size)", (void *)p->payload, (void *)((u8_t *)p + SIZEOF_STRUCT_PBUF));   
#else        
      LWIP_DEBUGF( PBUF_DEBUG | LWIP_DBG_TRACE,
        ("pbuf_header: failed as %p < %p (not enough space for new header size)\n",
        (void *)p->payload, (void *)((u8_t *)p + SIZEOF_STRUCT_PBUF)));
#endif
      /* restore old payload pointer */
      p->payload = payload;
      /* bail out unsuccessfully */
      return 1;
    }
  /* pbuf types referring to external payloads? */
  } else if (type == PBUF_REF || type == PBUF_ROM) {
    /* hide a header in the payload? */
    if ((header_size_increment < 0) && (increment_magnitude <= p->len)) {
      /* increase payload pointer */
      p->payload = (u8_t *)p->payload - header_size_increment;
    } else if ((header_size_increment > 0) && force) {
      p->payload = (u8_t *)p->payload - header_size_increment;
    } else {
      /* cannot expand payload to front (yet!)
       * bail out unsuccessfully */
      return 1;
    }
  } else {
    /* Unknown type */
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_header_impl_4, P_ERROR, 0, "bad pbuf type");   
#else    
    LWIP_ASSERT("bad pbuf type", 0);
#endif
    return 1;
  }
  /* modify pbuf length fields */
  p->len += header_size_increment;
  p->tot_len += header_size_increment;

#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_header_impl_5, P_INFO, 3, "pbuf_header: old 0x%x new 0x%x %u", (void *)payload, (void *)p->payload, header_size_increment);   
#else 
  LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_header: old %p new %p (%"S16_F")\n",
    (void *)payload, (void *)p->payload, header_size_increment));
#endif

  return 0;
}

/**
 * Adjusts the payload pointer to hide or reveal headers in the payload.
 *
 * Adjusts the ->payload pointer so that space for a header
 * (dis)appears in the pbuf payload.
 *
 * The ->payload, ->tot_len and ->len fields are adjusted.
 *
 * @param p pbuf to change the header size.
 * @param header_size_increment Number of bytes to increment header size which
 * increases the size of the pbuf. New space is on the front.
 * (Using a negative value decreases the header size.)
 * If hdr_size_inc is 0, this function does nothing and returns successful.
 *
 * PBUF_ROM and PBUF_REF type buffers cannot have their sizes increased, so
 * the call will fail. A check is made that the increase in header size does
 * not move the payload pointer in front of the start of the buffer.
 * @return non-zero on failure, zero on success.
 *
 */
u8_t
pbuf_header(struct pbuf *p, s16_t header_size_increment)
{
   return pbuf_header_impl(p, header_size_increment, 0);
}

/**
 * Same as pbuf_header but does not check if 'header_size > 0' is allowed.
 * This is used internally only, to allow PBUF_REF for RX.
 */
u8_t
pbuf_header_force(struct pbuf *p, s16_t header_size_increment)
{
   return pbuf_header_impl(p, header_size_increment, 1);
}

/**
 * @ingroup pbuf
 * Dereference a pbuf chain or queue and deallocate any no-longer-used
 * pbufs at the head of this chain or queue.
 *
 * Decrements the pbuf reference count. If it reaches zero, the pbuf is
 * deallocated.
 *
 * For a pbuf chain, this is repeated for each pbuf in the chain,
 * up to the first pbuf which has a non-zero reference count after
 * decrementing. So, when all reference counts are one, the whole
 * chain is free'd.
 *
 * @param p The pbuf (chain) to be dereferenced.
 *
 * @return the number of pbufs that were de-allocated
 * from the head of the chain.
 *
 * @note MUST NOT be called on a packet queue (Not verified to work yet).
 * @note the reference counter of a pbuf equals the number of pointers
 * that refer to the pbuf (or into the pbuf).
 *
 * @internal examples:
 *
 * Assuming existing chains a->b->c with the following reference
 * counts, calling pbuf_free(a) results in:
 *
 * 1->2->3 becomes ...1->3
 * 3->3->3 becomes 2->3->3
 * 1->1->2 becomes ......1
 * 2->1->1 becomes 1->1->1
 * 1->1->1 becomes .......
 *
 */
u8_t
pbuf_free(struct pbuf *p)
{
  u16_t type;
  struct pbuf *q;
  u8_t count;

  if (p == NULL) {

#if LWIP_ENABLE_UNILOG
    if(p == NULL) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_free_1, P_ERROR, 0, "p != NULL");   
    }
#else     
    LWIP_ASSERT("p != NULL", p != NULL);
    /* if assertions are disabled, proceed with debug output */
    LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
      ("pbuf_free(p == NULL) was called.\n"));
#endif    
    return 0;
  }

#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_free_2, P_INFO, 1, "pbuf_free 0x%x", (void *)p);   
#else   
  LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_free(%p)\n", (void *)p));
#endif

  PERF_START;

#if LWIP_ENABLE_UNILOG
    if(p->type != PBUF_RAM && p->type != PBUF_ROM && p->type != PBUF_REF && p->type != PBUF_POOL) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_free_3, P_ERROR, 0, "pbuf_free: sane type");   
    }
#else 
  LWIP_ASSERT("pbuf_free: sane type",
    p->type == PBUF_RAM || p->type == PBUF_ROM ||
    p->type == PBUF_REF || p->type == PBUF_POOL);
#endif

  count = 0;
  /* de-allocate all consecutive pbufs from the head of the chain that
   * obtain a zero reference count after decrementing*/
  while (p != NULL) {
    u16_t ref;
    SYS_ARCH_DECL_PROTECT(old_level);
    /* Since decrementing ref cannot be guaranteed to be a single machine operation
     * we must protect it. We put the new ref into a local variable to prevent
     * further protection. */
    SYS_ARCH_PROTECT(old_level);
    /* all pbufs in a chain are referenced at least once */
#if LWIP_ENABLE_UNILOG
    if(p->ref <= 0) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_free_4, P_ERROR, 0, "pbuf_free: p->ref > 0");   
    }
#else    
    LWIP_ASSERT("pbuf_free: p->ref > 0", p->ref > 0);
#endif
    /* decrease reference count (number of pointers to pbuf) */
    ref = --(p->ref);
    SYS_ARCH_UNPROTECT(old_level);
    /* this pbuf is no longer referenced to? */
    if (ref == 0) {
      /* remember next pbuf in chain for next iteration */
      q = p->next;
#ifndef LWIP_ENABLE_UNILOG
      LWIP_DEBUGF( PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_free: deallocating %p\n", (void *)p));
#endif
      type = p->type;
#if LWIP_SUPPORT_CUSTOM_PBUF
      /* is this a custom pbuf? */
      if ((p->flags & PBUF_FLAG_IS_CUSTOM) != 0) {
        struct pbuf_custom *pc = (struct pbuf_custom*)p;
#ifndef LWIP_ENABLE_UNILOG
        LWIP_ASSERT("pc->custom_free_function != NULL", pc->custom_free_function != NULL);
#endif
        pc->custom_free_function(p);
      } else
#endif /* LWIP_SUPPORT_CUSTOM_PBUF */
      {
        /* is this a pbuf from the pool? */
        if (type == PBUF_POOL) {
#if PBUF_POOL_MM_USE_CUSTOM

#if LWIP_ENABLE_UNILOG
          ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_free_5, P_INFO, 2, "PsMmFree: 0x%x has ref %u, ending here.", (void *)p, ref);   
#else  
		  LWIP_DEBUGF( PBUF_DEBUG | LWIP_DBG_TRACE, ("PsMmFree: %p has ref %"U16_F", ending here.\n", (void *)p, ref));
#endif
		  PsMmFree(p);
#else
          memp_free(MEMP_PBUF_POOL, p);
#endif
        /* is this a ROM or RAM referencing pbuf? */
        } else if (type == PBUF_ROM || type == PBUF_REF) {
          memp_free(MEMP_PBUF, p);
        /* type == PBUF_RAM */
        } else {
          mem_free(p);
        }
      }
      count++;
      /* proceed to next pbuf */
      p = q;
    /* p->ref > 0, this pbuf is still referenced to */
    /* (and so the remaining pbufs in chain as well) */
    } else {
#ifndef LWIP_ENABLE_UNILOG
      LWIP_DEBUGF( PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_free: %p has ref %"U16_F", ending here.\n", (void *)p, ref));
#endif
      /* stop walking through the chain */
      p = NULL;
    }
  }
  PERF_STOP("pbuf_free");
  /* return number of de-allocated pbufs */
  return count;
}

/**
 * Count number of pbufs in a chain
 *
 * @param p first pbuf of chain
 * @return the number of pbufs in a chain
 */
u16_t
pbuf_clen(const struct pbuf *p)
{
  u16_t len;

  len = 0;
  while (p != NULL) {
    ++len;
    p = p->next;
  }
  return len;
}

/**
 * @ingroup pbuf
 * Increment the reference count of the pbuf.
 *
 * @param p pbuf to increase reference counter of
 *
 */
void
pbuf_ref(struct pbuf *p)
{
  /* pbuf given? */
  if (p != NULL) {
    SYS_ARCH_INC(p->ref, 1);
#if LWIP_ENABLE_UNILOG
    if(p->ref <= 0) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_ref_1, P_ERROR, 0, "pbuf ref overflow");   
    }
#else     
    LWIP_ASSERT("pbuf ref overflow", p->ref > 0);
#endif
  }
}

/**
 * @ingroup pbuf
 * Concatenate two pbufs (each may be a pbuf chain) and take over
 * the caller's reference of the tail pbuf.
 *
 * @note The caller MAY NOT reference the tail pbuf afterwards.
 * Use pbuf_chain() for that purpose.
 *
 * @see pbuf_chain()
 */
void
pbuf_cat(struct pbuf *h, struct pbuf *t)
{
  struct pbuf *p;

#if LWIP_ENABLE_UNILOG
  if((h == NULL) || (t == NULL)) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_cat_1, P_ERROR, 0, "pbuf_cat invalid argument");
    return;
  }
#else 
  LWIP_ERROR("(h != NULL) && (t != NULL) (programmer violates API)",
             ((h != NULL) && (t != NULL)), return;);
#endif

  /* proceed to last pbuf of chain */
  for (p = h; p->next != NULL; p = p->next) {
    /* add total length of second chain to all totals of first chain */
    p->tot_len += t->tot_len;
  }
  /* { p is last pbuf of first h chain, p->next == NULL } */
#if LWIP_ENABLE_UNILOG
  if(p->tot_len != p->len || p->next != NULL) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_cat_2, P_ERROR, 0, "p->tot_len != p->len or p->next != NULL");
  }
#else  
  LWIP_ASSERT("p->tot_len == p->len (of last pbuf in chain)", p->tot_len == p->len);
  LWIP_ASSERT("p->next == NULL", p->next == NULL);
#endif
  /* add total length of second chain to last pbuf total of first chain */
  p->tot_len += t->tot_len;
  /* chain last pbuf of head (p) with first of tail (t) */
  p->next = t;
  /* p->next now references t, but the caller will drop its reference to t,
   * so netto there is no change to the reference count of t.
   */
}

/**
 * @ingroup pbuf
 * Chain two pbufs (or pbuf chains) together.
 *
 * The caller MUST call pbuf_free(t) once it has stopped
 * using it. Use pbuf_cat() instead if you no longer use t.
 *
 * @param h head pbuf (chain)
 * @param t tail pbuf (chain)
 * @note The pbufs MUST belong to the same packet.
 * @note MAY NOT be called on a packet queue.
 *
 * The ->tot_len fields of all pbufs of the head chain are adjusted.
 * The ->next field of the last pbuf of the head chain is adjusted.
 * The ->ref field of the first pbuf of the tail chain is adjusted.
 *
 */
void
pbuf_chain(struct pbuf *h, struct pbuf *t)
{
  pbuf_cat(h, t);
  /* t is now referenced by h */
  pbuf_ref(t);
#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_chain_1, P_INFO, 2, "pbuf_chain: 0x%x references 0x%x", (void *)h, (void *)t);
#else   
  LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_chain: %p references %p\n", (void *)h, (void *)t));
#endif
}

/**
 * Dechains the first pbuf from its succeeding pbufs in the chain.
 *
 * Makes p->tot_len field equal to p->len.
 * @param p pbuf to dechain
 * @return remainder of the pbuf chain, or NULL if it was de-allocated.
 * @note May not be called on a packet queue.
 */
struct pbuf *
pbuf_dechain(struct pbuf *p)
{
  struct pbuf *q;
  u8_t tail_gone = 1;
  /* tail */
  q = p->next;
  /* pbuf has successor in chain? */
  if (q != NULL) {
    /* assert tot_len invariant: (p->tot_len == p->len + (p->next? p->next->tot_len: 0) */

#if LWIP_ENABLE_UNILOG
    if(q->tot_len != p->tot_len - p->len) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_dechain_1, P_ERROR, 0, "p->tot_len == p->len + q->tot_len");
    }
#else    
    LWIP_ASSERT("p->tot_len == p->len + q->tot_len", q->tot_len == p->tot_len - p->len);
#endif
    /* enforce invariant if assertion is disabled */
    q->tot_len = p->tot_len - p->len;
    /* decouple pbuf from remainder */
    p->next = NULL;
    /* total length of pbuf p is its own length only */
    p->tot_len = p->len;
    /* q is no longer referenced by p, free it */
#if LWIP_ENABLE_UNILOG
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_dechain_2, P_INFO, 1, "pbuf_dechain: unreferencing 0x%x", (void *)q);
#else      
    LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_dechain: unreferencing %p\n", (void *)q));
#endif
    tail_gone = pbuf_free(q);
    if (tail_gone > 0) {
#if LWIP_ENABLE_UNILOG
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_dechain_3, P_INFO, 1, "pbuf_dechain: deallocated 0x%x (as it is no longer referenced)", (void *)q);
#else          
      LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE,
                  ("pbuf_dechain: deallocated %p (as it is no longer referenced)\n", (void *)q));
#endif
    }
    /* return remaining tail or NULL if deallocated */
  }
  /* assert tot_len invariant: (p->tot_len == p->len + (p->next? p->next->tot_len: 0) */

#if LWIP_ENABLE_UNILOG
  if(p->tot_len != p->len) {
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_dechain_4, P_ERROR, 0, "p->tot_len == p->len");
  }
#else   
  LWIP_ASSERT("p->tot_len == p->len", p->tot_len == p->len);
#endif
  return ((tail_gone > 0) ? NULL : q);
}

/**
 * @ingroup pbuf
 * Create PBUF_RAM copies of pbufs.
 *
 * Used to queue packets on behalf of the lwIP stack, such as
 * ARP based queueing.
 *
 * @note You MUST explicitly use p = pbuf_take(p);
 *
 * @note Only one packet is copied, no packet queue!
 *
 * @param p_to pbuf destination of the copy
 * @param p_from pbuf source of the copy
 *
 * @return ERR_OK if pbuf was copied
 *         ERR_ARG if one of the pbufs is NULL or p_to is not big
 *                 enough to hold p_from
 */
err_t
pbuf_copy(struct pbuf *p_to, const struct pbuf *p_from)
{
  u16_t offset_to=0, offset_from=0, len;

#if LWIP_ENABLE_UNILOG
  ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_copy_1, P_INFO, 2, "pbuf_copy to 0x%x from 0x%x", (const void*)p_to, (const void*)p_from);
#else 
  LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_copy(%p, %p)\n",
    (const void*)p_to, (const void*)p_from));
#endif

  /* is the target big enough to hold the source? */
#if LWIP_ENABLE_UNILOG
  if((p_to == NULL) || (p_from == NULL) || (p_to->tot_len < p_from->tot_len)) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_copy_2, P_ERROR, 0, "pbuf_copy: target not big enough to hold source");
    return ERR_ARG;
  }
#else 
  LWIP_ERROR("pbuf_copy: target not big enough to hold source", ((p_to != NULL) &&
             (p_from != NULL) && (p_to->tot_len >= p_from->tot_len)), return ERR_ARG;);
#endif

  /* iterate through pbuf chain */
  do
  {
    /* copy one part of the original chain */
    if ((p_to->len - offset_to) >= (p_from->len - offset_from)) {
      /* complete current p_from fits into current p_to */
      len = p_from->len - offset_from;
    } else {
      /* current p_from does not fit into current p_to */
      len = p_to->len - offset_to;
    }
    MEMCPY((u8_t*)p_to->payload + offset_to, (u8_t*)p_from->payload + offset_from, len);
    offset_to += len;
    offset_from += len;
#if LWIP_ENABLE_UNILOG
  if(offset_to > p_to->len || offset_from > p_from->len) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_copy_3, P_ERROR, 0, "offset_to > p_to->len or offset_from > p_from->len");
    return ERR_ARG;
  }
#else    
    LWIP_ASSERT("offset_to <= p_to->len", offset_to <= p_to->len);
    LWIP_ASSERT("offset_from <= p_from->len", offset_from <= p_from->len);
#endif
    if (offset_from >= p_from->len) {
      /* on to next p_from (if any) */
      offset_from = 0;
      p_from = p_from->next;
    }
    if (offset_to == p_to->len) {
      /* on to next p_to (if any) */
      offset_to = 0;
      p_to = p_to->next;
#if LWIP_ENABLE_UNILOG
  if((p_to == NULL) && (p_from != NULL)) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_copy_4, P_ERROR, 0, "p_to != NULL");
    return ERR_ARG;
  }
#else        
      LWIP_ERROR("p_to != NULL", (p_to != NULL) || (p_from == NULL) , return ERR_ARG;);
#endif
    }

    if ((p_from != NULL) && (p_from->len == p_from->tot_len)) {
      /* don't copy more than one packet! */
#if LWIP_ENABLE_UNILOG
    if(p_from->next != NULL) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_copy_5, P_ERROR, 0, "pbuf_copy() does not allow packet queues!");
        return ERR_VAL;
    }
#else       
      LWIP_ERROR("pbuf_copy() does not allow packet queues!",
                 (p_from->next == NULL), return ERR_VAL;);
#endif
    }
    if ((p_to != NULL) && (p_to->len == p_to->tot_len)) {
      /* don't copy more than one packet! */
#if LWIP_ENABLE_UNILOG
    if(p_to->next != NULL) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_copy_6, P_ERROR, 0, "pbuf_copy() does not allow packet queues!");
        return ERR_VAL;
    }
#else      
      LWIP_ERROR("pbuf_copy() does not allow packet queues!",
                  (p_to->next == NULL), return ERR_VAL;);
#endif
    }
  } while (p_from);
#ifndef LWIP_ENABLE_UNILOG
  LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_copy: end of chain reached.\n"));
#endif
  return ERR_OK;
}

/**
 * @ingroup pbuf
 * Copy (part of) the contents of a packet buffer
 * to an application supplied buffer.
 *
 * @param buf the pbuf from which to copy data
 * @param dataptr the application supplied buffer
 * @param len length of data to copy (dataptr must be big enough). No more
 * than buf->tot_len will be copied, irrespective of len
 * @param offset offset into the packet buffer from where to begin copying len bytes
 * @return the number of bytes copied, or 0 on failure
 */
u16_t
pbuf_copy_partial(const struct pbuf *buf, void *dataptr, u16_t len, u16_t offset)
{
  const struct pbuf *p;
  u16_t left;
  u16_t buf_copy_len;
  u16_t copied_total = 0;

#if LWIP_ENABLE_UNILOG
  if(buf == NULL || dataptr == NULL) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_copy_partial_1, P_ERROR, 0, "pbuf_copy_partial: invalid buf or dataptr");
    return 0;
  }
#else 
  LWIP_ERROR("pbuf_copy_partial: invalid buf", (buf != NULL), return 0;);
  LWIP_ERROR("pbuf_copy_partial: invalid dataptr", (dataptr != NULL), return 0;);
#endif

  left = 0;

  if ((buf == NULL) || (dataptr == NULL)) {
    return 0;
  }

  /* Note some systems use byte copy if dataptr or one of the pbuf payload pointers are unaligned. */
  for (p = buf; len != 0 && p != NULL; p = p->next) {
    if ((offset != 0) && (offset >= p->len)) {
      /* don't copy from this buffer -> on to the next */
      offset -= p->len;
    } else {
      /* copy from this buffer. maybe only partially. */
      buf_copy_len = p->len - offset;
      if (buf_copy_len > len) {
        buf_copy_len = len;
      }
      /* copy the necessary parts of the buffer */
      MEMCPY(&((char*)dataptr)[left], &((char*)p->payload)[offset], buf_copy_len);
      copied_total += buf_copy_len;
      left += buf_copy_len;
      len -= buf_copy_len;
      offset = 0;
    }
  }
  return copied_total;
}

#if ENABLE_PSIF
void updateSequenceBitmap(u32_t bitmap[8], UINT8 sequence, u8_t bActive)
{
    UINT8 arrayNum = 0;
    UINT8 i = 0;

    //check parameter
    if(bitmap == NULL || sequence == 0)
    {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, updateSequenceBitmap_1, P_ERROR, 0, "updateSequenceBitmap parameter invalid");
        return ;        
    }

    if(sequence%32 == 0)
    {
        i = 31;
    }
    else
    {
        i = ((sequence%32) -1);
    }

    if(sequence > 1)
    {
        arrayNum = ((sequence - 1)/32);
    }
    else
    {
        arrayNum = 0;
    }


    if(bActive != 0)
    {
        bitmap[arrayNum] |= (1 << i);
    }
    else
    {
        bitmap[arrayNum] &= (~(1 << i));
    }
}

u8_t getSequenceBitmap(u32_t bitmap[8], UINT8 sequence)
{
    UINT8 arrayNum = 0;
    UINT8 i = 0;

    //check parameter
    if(bitmap == NULL || sequence == 0)
    {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, getSequenceBitmap_1, P_ERROR, 0, "getSequenceBitmap parameter invalid");
        return FALSE;        
    }

    if(sequence%32 == 0)
    {
        i = 31;
    }
    else
    {
        i = ((sequence%32) -1);
    }

    if(sequence > 1)
    {
        arrayNum = ((sequence - 1)/32);
    }
    else
    {
        arrayNum = 0;
    }

    if((bitmap[arrayNum] & (1 << i)) != 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

#endif

#if LWIP_TCP && TCP_QUEUE_OOSEQ && LWIP_WND_SCALE
/**
 * This method modifies a 'pbuf chain', so that its total length is
 * smaller than 64K. The remainder of the original pbuf chain is stored
 * in *rest.
 * This function never creates new pbufs, but splits an existing chain
 * in two parts. The tot_len of the modified packet queue will likely be
 * smaller than 64K.
 * 'packet queues' are not supported by this function.
 *
 * @param p the pbuf queue to be split
 * @param rest pointer to store the remainder (after the first 64K)
 */
void pbuf_split_64k(struct pbuf *p, struct pbuf **rest)
{
  *rest = NULL;
  if ((p != NULL) && (p->next != NULL)) {
    u16_t tot_len_front = p->len;
    struct pbuf *i = p;
    struct pbuf *r = p->next;

    /* continue until the total length (summed up as u16_t) overflows */
    while ((r != NULL) && ((u16_t)(tot_len_front + r->len) > tot_len_front)) {
      tot_len_front += r->len;
      i = r;
      r = r->next;
    }
    /* i now points to last packet of the first segment. Set next
       pointer to NULL */
    i->next = NULL;

    if (r != NULL) {
      /* Update the tot_len field in the first part */
      for (i = p; i != NULL; i = i->next) {
        i->tot_len -= r->tot_len;
#if LWIP_ENABLE_UNILOG
        if((i->next == NULL) && (i->tot_len != i->len)) {
            ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_split_64k_1, P_ERROR, 0, "tot_len/len mismatch in last pbuf");
        }
#else         
        LWIP_ASSERT("tot_len/len mismatch in last pbuf",
                    (i->next != NULL) || (i->tot_len == i->len));
#endif
      }
      if (p->flags & PBUF_FLAG_TCP_FIN) {
        r->flags |= PBUF_FLAG_TCP_FIN;
      }

      /* tot_len field in rest does not need modifications */
      /* reference counters do not need modifications */
      *rest = r;
    }
  }
}
#endif /* LWIP_TCP && TCP_QUEUE_OOSEQ && LWIP_WND_SCALE */

/* Actual implementation of pbuf_skip() but returning const pointer... */
static const struct pbuf*
pbuf_skip_const(const struct pbuf* in, u16_t in_offset, u16_t* out_offset)
{
  u16_t offset_left = in_offset;
  const struct pbuf* q = in;

  /* get the correct pbuf */
  while ((q != NULL) && (q->len <= offset_left)) {
    offset_left -= q->len;
    q = q->next;
  }
  if (out_offset != NULL) {
    *out_offset = offset_left;
  }
  return q;
}

/**
 * @ingroup pbuf
 * Skip a number of bytes at the start of a pbuf
 *
 * @param in input pbuf
 * @param in_offset offset to skip
 * @param out_offset resulting offset in the returned pbuf
 * @return the pbuf in the queue where the offset is
 */
struct pbuf*
pbuf_skip(struct pbuf* in, u16_t in_offset, u16_t* out_offset)
{
  const struct pbuf* out = pbuf_skip_const(in, in_offset, out_offset);
  return LWIP_CONST_CAST(struct pbuf*, out);
}

/**
 * @ingroup pbuf
 * Copy application supplied data into a pbuf.
 * This function can only be used to copy the equivalent of buf->tot_len data.
 *
 * @param buf pbuf to fill with data
 * @param dataptr application supplied data buffer
 * @param len length of the application supplied data buffer
 *
 * @return ERR_OK if successful, ERR_MEM if the pbuf is not big enough
 */
err_t
pbuf_take(struct pbuf *buf, const void *dataptr, u16_t len)
{
  struct pbuf *p;
  u16_t buf_copy_len;
  u16_t total_copy_len = len;
  u16_t copied_total = 0;

#if LWIP_ENABLE_UNILOG
  if((buf == NULL) || (dataptr == NULL)) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_take_1, P_ERROR, 0, "pbuf_take: invalid buf or dataptr");
    return ERR_ARG;
  }

  if(buf->tot_len < len) {
    ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_take_2, P_ERROR, 0, "pbuf_take: buf not large enough");
    return ERR_MEM;      
  }
#else  
  LWIP_ERROR("pbuf_take: invalid buf", (buf != NULL), return ERR_ARG;);
  LWIP_ERROR("pbuf_take: invalid dataptr", (dataptr != NULL), return ERR_ARG;);
  LWIP_ERROR("pbuf_take: buf not large enough", (buf->tot_len >= len), return ERR_MEM;);
#endif  

  if ((buf == NULL) || (dataptr == NULL) || (buf->tot_len < len)) {
    return ERR_ARG;
  }

  /* Note some systems use byte copy if dataptr or one of the pbuf payload pointers are unaligned. */
  for (p = buf; total_copy_len != 0; p = p->next) {
#if LWIP_ENABLE_UNILOG
    if(p == NULL) {
        ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_take_3, P_ERROR, 0, "pbuf_take: invalid pbuf");
    }
#else  
    LWIP_ASSERT("pbuf_take: invalid pbuf", p != NULL);
#endif
    buf_copy_len = total_copy_len;
    if (buf_copy_len > p->len) {
      /* this pbuf cannot hold all remaining data */
      buf_copy_len = p->len;
    }
    /* copy the necessary parts of the buffer */
    MEMCPY(p->payload, &((const char*)dataptr)[copied_total], buf_copy_len);
    total_copy_len -= buf_copy_len;
    copied_total += buf_copy_len;
  }
#if LWIP_ENABLE_UNILOG
  if(total_copy_len != 0 || copied_total != len) {
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_take_4, P_ERROR, 0, "did not copy all data");
  }
#else   
  LWIP_ASSERT("did not copy all data", total_copy_len == 0 && copied_total == len);
#endif
  return ERR_OK;
}

/**
 * @ingroup pbuf
 * Same as pbuf_take() but puts data at an offset
 *
 * @param buf pbuf to fill with data
 * @param dataptr application supplied data buffer
 * @param len length of the application supplied data buffer
 * @param offset offset in pbuf where to copy dataptr to
 *
 * @return ERR_OK if successful, ERR_MEM if the pbuf is not big enough
 */
err_t
pbuf_take_at(struct pbuf *buf, const void *dataptr, u16_t len, u16_t offset)
{
  u16_t target_offset;
  struct pbuf* q = pbuf_skip(buf, offset, &target_offset);

  /* return requested data if pbuf is OK */
  if ((q != NULL) && (q->tot_len >= target_offset + len)) {
    u16_t remaining_len = len;
    const u8_t* src_ptr = (const u8_t*)dataptr;
    /* copy the part that goes into the first pbuf */
    u16_t first_copy_len = LWIP_MIN(q->len - target_offset, len);
    MEMCPY(((u8_t*)q->payload) + target_offset, dataptr, first_copy_len);
    remaining_len -= first_copy_len;
    src_ptr += first_copy_len;
    if (remaining_len > 0) {
      return pbuf_take(q->next, src_ptr, remaining_len);
    }
    return ERR_OK;
  }
  return ERR_MEM;
}

/**
 * @ingroup pbuf
 * Creates a single pbuf out of a queue of pbufs.
 *
 * @remark: Either the source pbuf 'p' is freed by this function or the original
 *          pbuf 'p' is returned, therefore the caller has to check the result!
 *
 * @param p the source pbuf
 * @param layer pbuf_layer of the new pbuf
 *
 * @return a new, single pbuf (p->next is NULL)
 *         or the old pbuf if allocation fails
 */
struct pbuf*
pbuf_coalesce(struct pbuf *p, pbuf_layer layer)
{
  struct pbuf *q;
  err_t err;
  if (p->next == NULL) {
    return p;
  }
  q = pbuf_alloc(layer, p->tot_len, PBUF_RAM);
  if (q == NULL) {
    /* @todo: what do we do now? */
    return p;
  }
  err = pbuf_copy(q, p);
  LWIP_UNUSED_ARG(err); /* in case of LWIP_NOASSERT */
#if LWIP_ENABLE_UNILOG
  if(err != ERR_OK) {
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_coalesce_1, P_ERROR, 0, "pbuf_copy failed");
  }
#else  
  LWIP_ASSERT("pbuf_copy failed", err == ERR_OK);
#endif
  pbuf_free(p);
  return q;
}

#if LWIP_CHECKSUM_ON_COPY
/**
 * Copies data into a single pbuf (*not* into a pbuf queue!) and updates
 * the checksum while copying
 *
 * @param p the pbuf to copy data into
 * @param start_offset offset of p->payload where to copy the data to
 * @param dataptr data to copy into the pbuf
 * @param len length of data to copy into the pbuf
 * @param chksum pointer to the checksum which is updated
 * @return ERR_OK if successful, another error if the data does not fit
 *         within the (first) pbuf (no pbuf queues!)
 */
err_t
pbuf_fill_chksum(struct pbuf *p, u16_t start_offset, const void *dataptr,
                 u16_t len, u16_t *chksum)
{
  u32_t acc;
  u16_t copy_chksum;
  char *dst_ptr;

#if LWIP_ENABLE_UNILOG
  if(p == NULL || dataptr == NULL || chksum == NULL || len == 0) {
      ECOMM_TRACE(UNILOG_TCPIP_LWIP, pbuf_fill_chksum_1, P_ERROR, 0, "pbuf_fill_chksum invalid argument");
  }
#else   
  LWIP_ASSERT("p != NULL", p != NULL);
  LWIP_ASSERT("dataptr != NULL", dataptr != NULL);
  LWIP_ASSERT("chksum != NULL", chksum != NULL);
  LWIP_ASSERT("len != 0", len != 0);
#endif

  if ((start_offset >= p->len) || (start_offset + len > p->len)) {
    return ERR_ARG;
  }

  dst_ptr = ((char*)p->payload) + start_offset;
  copy_chksum = LWIP_CHKSUM_COPY(dst_ptr, dataptr, len);
  if ((start_offset & 1) != 0) {
    copy_chksum = SWAP_BYTES_IN_WORD(copy_chksum);
  }
  acc = *chksum;
  acc += copy_chksum;
  *chksum = FOLD_U32T(acc);
  return ERR_OK;
}
#endif /* LWIP_CHECKSUM_ON_COPY */

/**
 * @ingroup pbuf
 * Get one byte from the specified position in a pbuf
 * WARNING: returns zero for offset >= p->tot_len
 *
 * @param p pbuf to parse
 * @param offset offset into p of the byte to return
 * @return byte at an offset into p OR ZERO IF 'offset' >= p->tot_len
 */
u8_t
pbuf_get_at(const struct pbuf* p, u16_t offset)
{
  int ret = pbuf_try_get_at(p, offset);
  if (ret >= 0) {
    return (u8_t)ret;
  }
  return 0;
}

/**
 * @ingroup pbuf
 * Get one byte from the specified position in a pbuf
 *
 * @param p pbuf to parse
 * @param offset offset into p of the byte to return
 * @return byte at an offset into p [0..0xFF] OR negative if 'offset' >= p->tot_len
 */
int
pbuf_try_get_at(const struct pbuf* p, u16_t offset)
{
  u16_t q_idx;
  const struct pbuf* q = pbuf_skip_const(p, offset, &q_idx);

  /* return requested data if pbuf is OK */
  if ((q != NULL) && (q->len > q_idx)) {
    return ((u8_t*)q->payload)[q_idx];
  }
  return -1;
}

/**
 * @ingroup pbuf
 * Put one byte to the specified position in a pbuf
 * WARNING: silently ignores offset >= p->tot_len
 *
 * @param p pbuf to fill
 * @param offset offset into p of the byte to write
 * @param data byte to write at an offset into p
 */
void
pbuf_put_at(struct pbuf* p, u16_t offset, u8_t data)
{
  u16_t q_idx;
  struct pbuf* q = pbuf_skip(p, offset, &q_idx);

  /* write requested data if pbuf is OK */
  if ((q != NULL) && (q->len > q_idx)) {
    ((u8_t*)q->payload)[q_idx] = data;
  }
}

/**
 * @ingroup pbuf
 * Compare pbuf contents at specified offset with memory s2, both of length n
 *
 * @param p pbuf to compare
 * @param offset offset into p at which to start comparing
 * @param s2 buffer to compare
 * @param n length of buffer to compare
 * @return zero if equal, nonzero otherwise
 *         (0xffff if p is too short, diffoffset+1 otherwise)
 */
u16_t
pbuf_memcmp(const struct pbuf* p, u16_t offset, const void* s2, u16_t n)
{
  u16_t start = offset;
  const struct pbuf* q = p;
  u16_t i;
 
  /* pbuf long enough to perform check? */
  if(p->tot_len < (offset + n)) {
    return 0xffff;
  }
 
  /* get the correct pbuf from chain. We know it succeeds because of p->tot_len check above. */
  while ((q != NULL) && (q->len <= start)) {
    start -= q->len;
    q = q->next;
  }
 
  /* return requested data if pbuf is OK */
  for (i = 0; i < n; i++) {
    /* We know pbuf_get_at() succeeds because of p->tot_len check above. */
    u8_t a = pbuf_get_at(q, start + i);
    u8_t b = ((const u8_t*)s2)[i];
    if (a != b) {
      return i+1;
    }
  }
  return 0;
}

/**
 * @ingroup pbuf
 * Find occurrence of mem (with length mem_len) in pbuf p, starting at offset
 * start_offset.
 *
 * @param p pbuf to search, maximum length is 0xFFFE since 0xFFFF is used as
 *        return value 'not found'
 * @param mem search for the contents of this buffer
 * @param mem_len length of 'mem'
 * @param start_offset offset into p at which to start searching
 * @return 0xFFFF if substr was not found in p or the index where it was found
 */
u16_t
pbuf_memfind(const struct pbuf* p, const void* mem, u16_t mem_len, u16_t start_offset)
{
  u16_t i;
  u16_t max = p->tot_len - mem_len;
  if (p->tot_len >= mem_len + start_offset) {
    for (i = start_offset; i <= max; i++) {
      u16_t plus = pbuf_memcmp(p, i, mem, mem_len);
      if (plus == 0) {
        return i;
      }
    }
  }
  return 0xFFFF;
}

/**
 * Find occurrence of substr with length substr_len in pbuf p, start at offset
 * start_offset
 * WARNING: in contrast to strstr(), this one does not stop at the first \0 in
 * the pbuf/source string!
 *
 * @param p pbuf to search, maximum length is 0xFFFE since 0xFFFF is used as
 *        return value 'not found'
 * @param substr string to search for in p, maximum length is 0xFFFE
 * @return 0xFFFF if substr was not found in p or the index where it was found
 */
u16_t
pbuf_strstr(const struct pbuf* p, const char* substr)
{
  size_t substr_len;
  if ((substr == NULL) || (substr[0] == 0) || (p->tot_len == 0xFFFF)) {
    return 0xFFFF;
  }
  substr_len = strlen(substr);
  if (substr_len >= 0xFFFF) {
    return 0xFFFF;
  }
  return pbuf_memfind(p, substr, (u16_t)substr_len, 0);
}
