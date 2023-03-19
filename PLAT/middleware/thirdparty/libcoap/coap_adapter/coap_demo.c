/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* coap-client -- simple CoAP client
 *
 * Copyright (C) 2010--2019 Olaf Bergmann <bergmann@tzi.org> and others
 *
 * This file is part of the CoAP library libcoap. Please see README for terms of
 * use.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <coap2/coap.h>
//#include "coap_adapter.h"

#define MAX_USER 128 /* Maximum length of a user name (i.e., PSK
                      * identity) in bytes. */
#define MAX_KEY   64 /* Maximum length of a key (i.e., PSK) in bytes. */

//extern coap_mutex coap_mutex1;
//extern coap_mutex coap_mutex2;
//extern coap_mutex coap_mutex3;

int flags = 0;

static unsigned char _token_data[8];
coap_binary_t the_token = { 0, _token_data };

#define FLAGS_BLOCK 0x01

static coap_optlist_t *optlist = NULL;
/* Request URI.
 * TODO: associate the resources with transaction id and make it expireable */
static coap_uri_t uri;
static unsigned int ping_seconds = 0;

/* reading is done when this flag is set */
static int ready = 0;

static coap_string_t payload = { 0, NULL };       /* optional payload to send */

unsigned char msgtype = COAP_MESSAGE_CON; /* usually, requests are sent confirmable */

static char *cert_file = NULL; /* Combined certificate and private key in PEM */
static char *ca_file = NULL;   /* CA for cert_file - for cert checking in PEM */
static char *root_ca_file = NULL; /* List of trusted Root CAs in PEM */

typedef unsigned char method_t;
method_t method = 1;                    /* the method we are using in our requests */

coap_block_t block = { .num = 0, .m = 0, .szx = 6 };
uint16_t last_block1_tid = 0;


unsigned int wait_seconds = 90;                /* default timeout in seconds */
unsigned int wait_ms = 0;
int wait_ms_reset = 0;
int obs_started = 0;
unsigned int obs_seconds = 30;          /* default observe time */
unsigned int obs_ms = 0;                /* timeout for current subscription */
int obs_ms_reset = 0;

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__ ((unused))
#else /* not a GCC */
#define UNUSED_PARAM
#endif /* GCC */

static int quit = 0;
extern void coap_mutex_init_all(void);

static coap_pdu_t *
coap_new_request(coap_context_t *ctx,
                 coap_session_t *session,
                 method_t m,
                 coap_optlist_t **options,
                 unsigned char *data,
                 size_t length) {
  coap_pdu_t *pdu;
  (void)ctx;

  if (NULL == (pdu = coap_new_pdu(session)))
    return NULL;

  pdu->type = msgtype;
  pdu->tid = coap_new_message_id(session);
  pdu->code = m;

  if ( !coap_add_token(pdu, the_token.length, the_token.s)) {
    coap_log(LOG_DEBUG, "cannot add token to request\n");
  }

  if (options)
    coap_add_optlist_pdu(pdu, options);

  if (length) {
    if ((flags & FLAGS_BLOCK) == 0)
      coap_add_data(pdu, length, data);
    else
      coap_add_block(pdu, length, data, block.num, block.szx);
  }

  return pdu;
}

static coap_tid_t
clear_obs(coap_context_t *ctx, coap_session_t *session) {
  coap_pdu_t *pdu;
  coap_optlist_t *option;
  coap_tid_t tid = COAP_INVALID_TID;
  unsigned char buf[2];
  (void)ctx;

  /* create bare PDU w/o any option  */
  pdu = coap_pdu_init(msgtype,
                      COAP_REQUEST_GET,
                      coap_new_message_id(session),
                      coap_session_max_pdu_size(session));

  if (!pdu) {
    return tid;
  }

  if (!coap_add_token(pdu, the_token.length, the_token.s)) {
    coap_log(LOG_CRIT, "cannot add token\n");
    goto error;
  }

  for (option = optlist; option; option = option->next ) {
    if (option->number == COAP_OPTION_URI_HOST) {
      if (!coap_add_option(pdu, option->number, option->length,
                           option->data)) {
        goto error;
      }
      break;
    }
  }

  if (!coap_add_option(pdu,
      COAP_OPTION_OBSERVE,
      coap_encode_var_safe(buf, sizeof(buf), COAP_OBSERVE_CANCEL),
      buf)) {
    coap_log(LOG_CRIT, "cannot add option Observe: %u\n", COAP_OBSERVE_CANCEL);
    goto error;
  }

  for (option = optlist; option; option = option->next ) {
    switch (option->number) {
    case COAP_OPTION_URI_PORT :
    case COAP_OPTION_URI_PATH :
    case COAP_OPTION_URI_QUERY :
      if (!coap_add_option(pdu, option->number, option->length,
                           option->data)) {
        goto error;
      }
      break;
      default:
      ;
    }
  }

  if (flags & FLAGS_BLOCK) {
    block.num = 0;
    block.m = 0;
    coap_add_option(pdu,
      COAP_OPTION_BLOCK2,
      coap_encode_var_safe(buf, sizeof(buf), (block.num << 4 | block.m << 3 | block.szx)),
      buf);
  }

  if (coap_get_log_level() < LOG_DEBUG)
    coap_show_pdu(LOG_INFO, pdu);


  tid = coap_send(session, pdu);

  if (tid == COAP_INVALID_TID)
    coap_log(LOG_DEBUG, "clear_obs: error sending new request\n");

  return tid;
 error:

  coap_delete_pdu(pdu);
  return tid;
}

static int
resolve_address(const coap_str_const_t *server, struct sockaddr *dst) {

  struct addrinfo *res, *ainfo;
  struct addrinfo hints;
  static char addrstr[256];
  int error, len=-1;

  memset(addrstr, 0, sizeof(addrstr));
  if (server->length)
    memcpy(addrstr, server->s, server->length);
  else
    memcpy(addrstr, "localhost", 9);

  memset ((char *)&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_family = AF_UNSPEC;

  error = getaddrinfo(addrstr, NULL, &hints, &res);

  if (error != 0) {
    return error;
  }

  for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) {
    switch (ainfo->ai_family) {
    case AF_INET6:
    case AF_INET:
      len = ainfo->ai_addrlen;
      memcpy(dst, ainfo->ai_addr, len);
      goto finish;
    default:
      ;
    }
  }

 finish:
  freeaddrinfo(res);
  return len;
}

#define HANDLE_BLOCK1(Pdu)                                        \
  ((method == COAP_REQUEST_PUT || method == COAP_REQUEST_POST) && \
   ((flags & FLAGS_BLOCK) == 0) &&                                \
   ((Pdu)->hdr->code == COAP_RESPONSE_CODE(201) ||                \
    (Pdu)->hdr->code == COAP_RESPONSE_CODE(204)))

static inline int
check_token(coap_pdu_t *received) {
  return received->token_length == the_token.length &&
    memcmp(received->token, the_token.s, the_token.length) == 0;
}

static int
event_handler(coap_context_t *ctx UNUSED_PARAM,
              coap_event_t event,
              struct coap_session_t *session UNUSED_PARAM) {

  switch(event) {
  case COAP_EVENT_DTLS_CLOSED:
  case COAP_EVENT_TCP_CLOSED:
  case COAP_EVENT_SESSION_CLOSED:
    quit = 1;
    break;
  default:
    break;
  }
  return 0;
}

static void
message_handler(struct coap_context_t *ctx,
                coap_session_t *session,
                coap_pdu_t *sent,
                coap_pdu_t *received,
                const coap_tid_t id UNUSED_PARAM) {

  coap_pdu_t *pdu = NULL;
  coap_opt_t *block_opt;
  coap_opt_iterator_t opt_iter;
  unsigned char buf[4];
  coap_optlist_t *option;
  size_t len;
  unsigned char *databuf;
  coap_tid_t tid;

  ECOMM_TRACE(UNILOG_COAP, coap_demo_0, P_INFO, 0, "...4....coap_demo..message_handler.");

  /* check if this is a response to our original request */
  if (!check_token(received)) {
    /* drop if this was just some message, or send RST in case of notification */
    if (!sent && (received->type == COAP_MESSAGE_CON ||
                  received->type == COAP_MESSAGE_NON))
      coap_send_rst(session, received);
    return;
  }

  if (received->type == COAP_MESSAGE_RST) {
    coap_log(LOG_INFO, "got RST\n");
    return;
  }

  /* output the received data, if any */
  if (COAP_RESPONSE_CLASS(received->code) == 2) {

    /* set obs timer if we have successfully subscribed a resource */
    if (!obs_started && coap_check_option(received, COAP_OPTION_OBSERVE, &opt_iter)) {
      coap_log(LOG_DEBUG,
               "observation relationship established, set timeout to %d\n",
               obs_seconds);
      obs_started = 1;
      obs_ms = obs_seconds * 1000;
      obs_ms_reset = 1;
    }

    /* Got some data, check if block option is set. Behavior is undefined if
     * both, Block1 and Block2 are present. */
    block_opt = coap_check_option(received, COAP_OPTION_BLOCK2, &opt_iter);
    if (block_opt) { /* handle Block2 */
      uint16_t blktype = opt_iter.type;

      /* TODO: check if we are looking at the correct block number */
      if (coap_get_data(received, &len, &databuf))
        ;//append_to_output(databuf, len);

      if (coap_opt_block_num(block_opt) == 0) {
        /* See if observe is set in first response */
        ready = coap_check_option(received,
                                  COAP_OPTION_OBSERVE, &opt_iter) == NULL;
      }
      if(COAP_OPT_BLOCK_MORE(block_opt)) {
        /* more bit is set */
        coap_log(LOG_DEBUG, "found the M bit, block size is %u, block nr. %u\n",
              COAP_OPT_BLOCK_SZX(block_opt),
              coap_opt_block_num(block_opt));

        /* create pdu with request for next block */
        pdu = coap_new_request(ctx, session, method, NULL, NULL, 0); /* first, create bare PDU w/o any option  */
        if ( pdu ) {
          /* add URI components from optlist */
          for (option = optlist; option; option = option->next ) {
            switch (option->number) {
              case COAP_OPTION_URI_HOST :
              case COAP_OPTION_URI_PORT :
              case COAP_OPTION_URI_PATH :
              case COAP_OPTION_URI_QUERY :
                coap_add_option(pdu, option->number, option->length,
                                option->data);
                break;
              default:
                ;     /* skip other options */
            }
          }

          /* finally add updated block option from response, clear M bit */
          /* blocknr = (blocknr & 0xfffffff7) + 0x10; */
          coap_log(LOG_DEBUG, "query block %d\n",
                   (coap_opt_block_num(block_opt) + 1));
          coap_add_option(pdu,
                          blktype,
                          coap_encode_var_safe(buf, sizeof(buf),
                                 ((coap_opt_block_num(block_opt) + 1) << 4) |
                                  COAP_OPT_BLOCK_SZX(block_opt)), buf);

          tid = coap_send(session, pdu);

          if (tid == COAP_INVALID_TID) {
            coap_log(LOG_DEBUG, "message_handler: error sending new request\n");
          } else {
            wait_ms = wait_seconds * 1000;
            wait_ms_reset = 1;
          }

          return;
        }
      }
      return;
    } else { /* no Block2 option */
      block_opt = coap_check_option(received, COAP_OPTION_BLOCK1, &opt_iter);

      if (block_opt) { /* handle Block1 */
        unsigned int szx = COAP_OPT_BLOCK_SZX(block_opt);
        unsigned int num = coap_opt_block_num(block_opt);
        coap_log(LOG_DEBUG,
                 "found Block1 option, block size is %u, block nr. %u\n",
                 szx, num);
        if (szx != block.szx) {
          unsigned int bytes_sent = ((block.num + 1) << (block.szx + 4));
          if (bytes_sent % (1 << (szx + 4)) == 0) {
            /* Recompute the block number of the previous packet given the new block size */
            num = block.num = (bytes_sent >> (szx + 4)) - 1;
            block.szx = szx;
            coap_log(LOG_DEBUG,
                     "new Block1 size is %u, block number %u completed\n",
                     (1 << (block.szx + 4)), block.num);
          } else {
            coap_log(LOG_DEBUG, "ignoring request to increase Block1 size, "
            "next block is not aligned on requested block size boundary. "
            "(%u x %u mod %u = %u != 0)\n",
                  block.num + 1, (1 << (block.szx + 4)), (1 << (szx + 4)),
                  bytes_sent % (1 << (szx + 4)));
          }
        }

        if (payload.length <= (block.num+1) * (1 << (block.szx + 4))) {
          coap_log(LOG_DEBUG, "upload ready\n");
          ready = 1;
          return;
        }
        if (last_block1_tid == received->tid) {
          /*
           * Duplicate BLOCK1 ACK
           *
           * RFCs not clear here, but on a lossy connection, there could
           * be multiple BLOCK1 ACKs, causing the client to retransmit the
           * same block multiple times.
           *
           * Once a block has been ACKd, there is no need to retransmit it.
           */
          return;
        }
        last_block1_tid = received->tid;

        /* create pdu with request for next block */
        pdu = coap_new_request(ctx, session, method, NULL, NULL, 0); /* first, create bare PDU w/o any option  */
        if (pdu) {

          /* add URI components from optlist */
          for (option = optlist; option; option = option->next ) {
            switch (option->number) {
              case COAP_OPTION_URI_HOST :
              case COAP_OPTION_URI_PORT :
              case COAP_OPTION_URI_PATH :
              case COAP_OPTION_CONTENT_FORMAT :
              case COAP_OPTION_URI_QUERY :
                coap_add_option(pdu, option->number, option->length,
                                option->data);
                break;
              default:
              ;     /* skip other options */
            }
          }

          /* finally add updated block option from response, clear M bit */
          /* blocknr = (blocknr & 0xfffffff7) + 0x10; */
          block.num = num + 1;
          block.m = ((block.num+1) * (1 << (block.szx + 4)) < payload.length);

          coap_log(LOG_DEBUG, "send block %d\n", block.num);
          coap_add_option(pdu,
                          COAP_OPTION_BLOCK1,
                          coap_encode_var_safe(buf, sizeof(buf),
                          (block.num << 4) | (block.m << 3) | block.szx), buf);

          coap_add_block(pdu,
                         payload.length,
                         payload.s,
                         block.num,
                         block.szx);
          if (coap_get_log_level() < LOG_DEBUG)
            coap_show_pdu(LOG_INFO, pdu);

          tid = coap_send(session, pdu);

          if (tid == COAP_INVALID_TID) {
            coap_log(LOG_DEBUG, "message_handler: error sending new request\n");
          } else {
            wait_ms = wait_seconds * 1000;
            wait_ms_reset = 1;
          }

          return;
        }
      } else {
        /* There is no block option set, just read the data and we are done. */
        if (coap_get_data(received, &len, &databuf))
          ;//append_to_output(databuf, len);
      }
    }
  } else {      /* no 2.05 */

    /* check if an error was signaled and output payload if so */
    if (COAP_RESPONSE_CLASS(received->code) >= 4) {
      if (coap_get_data(received, &len, &databuf)) {
      }
    }

  }

  /* any pdu that has been created in this function must be sent by now */
  assert(pdu == NULL);

  /* our job is done, we can exit at any time */
  ready = coap_check_option(received, COAP_OPTION_OBSERVE, &opt_iter) == NULL;
}

typedef struct {
  unsigned char code;
  const char *media_type;
} content_type_t;

/* Called after processing the options from the commandline to set
 * Block1 or Block2 depending on method. */
static void
set_blocksize(void) {
  static unsigned char buf[4];        /* hack: temporarily take encoded bytes */
  uint16_t opt;
  unsigned int opt_length;

  if (method != COAP_REQUEST_DELETE) {
    opt = method == COAP_REQUEST_GET ? COAP_OPTION_BLOCK2 : COAP_OPTION_BLOCK1;

    block.m = (opt == COAP_OPTION_BLOCK1) &&
      ((1u << (block.szx + 4)) < payload.length);

    opt_length = coap_encode_var_safe(buf, sizeof(buf),
          (block.num << 4 | block.m << 3 | block.szx));

    coap_insert_optlist(&optlist, coap_new_optlist(opt, opt_length, buf));
  }
}



/**
 * Calculates decimal value from hexadecimal ASCII character given in
 * @p c. The caller must ensure that @p c actually represents a valid
 * heaxdecimal character, e.g. with isxdigit(3).
 *
 * @hideinitializer
 */
#define hexchar_to_dec(c) ((c) & 0x40 ? ((c) & 0x0F) + 9 : ((c) & 0x0F))
static int
verify_cn_callback(const char *cn,
                   const uint8_t *asn1_public_cert UNUSED_PARAM,
                   size_t asn1_length UNUSED_PARAM,
                   coap_session_t *session UNUSED_PARAM,
                   unsigned depth,
                   int validated UNUSED_PARAM,
                   void *arg UNUSED_PARAM
) {
  coap_log(LOG_INFO, "CN '%s' presented by server (%s)\n",
           cn, depth ? "CA" : "Certificate");
  return 1;
}

static coap_dtls_pki_t *
setup_pki(coap_context_t *ctx) {
  static coap_dtls_pki_t dtls_pki;
  static char client_sni[256];

  memset (&dtls_pki, 0, sizeof(dtls_pki));
  dtls_pki.version = COAP_DTLS_PKI_SETUP_VERSION;
  if (ca_file || root_ca_file) {
    /*
     * Add in additional certificate checking.
     * This list of enabled can be tuned for the specific
     * requirements - see 'man coap_encryption'.
     *
     * Note: root_ca_file is setup separately using
     * coap_context_set_pki_root_cas(), but this is used to define what
     * checking actually takes place.
     */
    dtls_pki.verify_peer_cert        = 1;
    dtls_pki.require_peer_cert       = 1;
    dtls_pki.allow_self_signed       = 1;
    dtls_pki.allow_expired_certs     = 1;
    dtls_pki.cert_chain_validation   = 1;
    dtls_pki.cert_chain_verify_depth = 2;
    dtls_pki.check_cert_revocation   = 1;
    dtls_pki.allow_no_crl            = 1;
    dtls_pki.allow_expired_crl       = 1;
    dtls_pki.validate_cn_call_back   = verify_cn_callback;
    dtls_pki.cn_call_back_arg        = NULL;
    dtls_pki.validate_sni_call_back  = NULL;
    dtls_pki.sni_call_back_arg       = NULL;
    memset(client_sni, 0, sizeof(client_sni));
    if (uri.host.length)
      memcpy(client_sni, uri.host.s, min(uri.host.length, sizeof(client_sni)));
    else
      memcpy(client_sni, "localhost", 9);
    dtls_pki.client_sni = client_sni;
  }
  dtls_pki.pki_key.key_type = COAP_PKI_KEY_PEM;
  dtls_pki.pki_key.key.pem.public_cert = cert_file;
  dtls_pki.pki_key.key.pem.private_key = cert_file;
  dtls_pki.pki_key.key.pem.ca_file = ca_file;
  return &dtls_pki;
}


static coap_session_t *
get_session(
  coap_context_t *ctx,
  const char *local_addr,
  const char *local_port,
  coap_proto_t proto,
  coap_address_t *dst,
  const char *identity,
  const uint8_t *key,
  unsigned key_len
) {
  coap_session_t *session = NULL;

  if ( local_addr ) {
    int s;
    struct addrinfo hints;
    struct addrinfo *result = NULL, *rp;

    memset( &hints, 0, sizeof( struct addrinfo ) );
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = COAP_PROTO_RELIABLE(proto) ? SOCK_STREAM : SOCK_DGRAM; /* Coap uses UDP */
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV | AI_ALL;

    s = getaddrinfo( local_addr, local_port, &hints, &result );
    if ( s != 0 ) {
      return NULL;
    }

    /* iterate through results until success */
    for ( rp = result; rp != NULL; rp = rp->ai_next ) {
      coap_address_t bind_addr;
      if ( rp->ai_addrlen <= sizeof( bind_addr.addr ) ) {
        coap_address_init( &bind_addr );
        bind_addr.size = rp->ai_addrlen;
        memcpy( &bind_addr.addr, rp->ai_addr, rp->ai_addrlen );
        if ((root_ca_file || ca_file || cert_file) &&
            (proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS)) {
          coap_dtls_pki_t *dtls_pki = setup_pki(ctx);
          session = coap_new_client_session_pki(ctx, &bind_addr, dst, proto, dtls_pki);
        }
        else if ((identity || key) &&
                 (proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS) ) {
          session = coap_new_client_session_psk( ctx, &bind_addr, dst, proto,
                           identity, key, key_len );
        }
        else {
          session = coap_new_client_session( ctx, &bind_addr, dst, proto );
        }
        if ( session )
          break;
      }
    }
    freeaddrinfo( result );
  } else {
    if ((root_ca_file || ca_file || cert_file) &&
        (proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS)) {
      coap_dtls_pki_t *dtls_pki = setup_pki(ctx);
      session = coap_new_client_session_pki(ctx, NULL, dst, proto, dtls_pki);
    }
    else if ((identity || key) &&
             (proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS) )
      session = coap_new_client_session_psk( ctx, NULL, dst, proto,
                      identity, key, key_len );
    else
      session = coap_new_client_session( ctx, NULL, dst, proto );
  }
  return session;
}

int coap_demo(void) 
{
  coap_context_t  *ctx = NULL;
  coap_session_t *session = NULL;
  coap_address_t dst;
  int result = -1;
  coap_pdu_t  *pdu;
  static coap_str_const_t server;
  uint16_t port = COAP_DEFAULT_PORT;
  int res;
  coap_log_t log_level = LOG_WARNING;
  unsigned char user[MAX_USER + 1], key[MAX_KEY];
  ssize_t user_length = 0, key_length = 0;

  coap_mutex_init_all();

  coap_startup();
  coap_dtls_set_log_level(log_level);
  coap_set_log_level(log_level);

  {
    server.s = "coap.me";
    server.length = strlen("coap.me");
    port = 5683;
  }

  /* resolve destination address where server should be sent */
  res = resolve_address(&server, &dst.addr.sa);
  ECOMM_TRACE(UNILOG_COAP, coap_demo_1, P_INFO, 1, ".......resolve_address.%d", res);
  if (res < 0) {
    return result;
  }

  ctx = coap_new_context( NULL );
  if ( !ctx ) {
    coap_log( LOG_EMERG, "cannot create context\n" );
    goto finish;
  }
  ECOMM_TRACE(UNILOG_COAP, coap_demo_2, P_INFO, 0, "...1....coap_demo.");

  coap_context_set_keepalive(ctx, ping_seconds);

  dst.size = res;
  dst.addr.sin.sin_port = htons( port );

  session = get_session(
    ctx,
    NULL, NULL,
    COAP_PROTO_UDP,
    &dst,
    user_length > 0 ? (const char *)user : NULL,
    key_length > 0  ? key : NULL, (unsigned)key_length
  );
  ECOMM_TRACE(UNILOG_COAP, coap_demo_3, P_INFO, 0, "...2....coap_demo.");

  if ( !session ) {
    coap_log( LOG_EMERG, "cannot create client session\n" );
    goto finish;
  }

  /* add Uri-Host if server address differs from uri.host */
  ECOMM_TRACE(UNILOG_COAP, coap_demo_4, P_INFO, 0, "...3....coap_demo.");

  switch (dst.addr.sa.sa_family) {
  case AF_INET:
    //addrptr = &dst.addr.sin.sin_addr;
    /* create context for IPv4 */
    break;
  case AF_INET6:
    //addrptr = &dst.addr.sin6.sin6_addr;
    break;
  default:
    ;
  }

  coap_register_option(ctx, COAP_OPTION_BLOCK2);
  coap_register_response_handler(ctx, message_handler);
  coap_register_event_handler(ctx, event_handler);

  /* set block option if requested at commandline */
  if (flags & FLAGS_BLOCK)
    set_blocksize();
  ECOMM_TRACE(UNILOG_COAP, coap_demo_5, P_INFO, 0, "...4....coap_demo.");

  if (NULL == (pdu = coap_new_request(ctx, session, method, &optlist, payload.s, payload.length))) {
    goto finish;
  }
  ECOMM_TRACE(UNILOG_COAP, coap_demo_6, P_INFO, 0, "...5....coap_demo.");

  coap_send(session, pdu);
  ECOMM_TRACE(UNILOG_COAP, coap_demo_7, P_INFO, 0, "...6....coap_demo.");

  wait_ms = wait_seconds * 1000;
  coap_log(LOG_DEBUG, "timeout is set to %u seconds\n", wait_seconds);

  while (!quit && !(ready && coap_can_exit(ctx)) ) {

    result = coap_run_once( ctx, 2000, ctx);

    if ( result >= 0 ) {
      if ( wait_ms > 0 && !wait_ms_reset ) {
        if ( (unsigned)result >= wait_ms ) {
          coap_log(LOG_INFO, "timeout\n");
          break;
        } else {
          wait_ms -= result;
        }
      }
      if ( obs_ms > 0 && !obs_ms_reset ) {
        if ( (unsigned)result >= obs_ms ) {
          coap_log(LOG_DEBUG, "clear observation relationship\n" );
          clear_obs( ctx, session ); /* FIXME: handle error case COAP_TID_INVALID */

          /* make sure that the obs timer does not fire again */
          obs_ms = 0;
          obs_seconds = 0;
        } else {
          obs_ms -= result;
        }
      }
      wait_ms_reset = 0;
      obs_ms_reset = 0;
    }
  }

  result = 0;

 finish:

  coap_delete_optlist(optlist);
  coap_session_release( session );
  coap_free_context( ctx );
  coap_cleanup();

  return result;
}
