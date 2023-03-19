/* coap_session.h -- Session management for libcoap
*
* Copyright (C) 2017 Jean-Claue Michelou <jcm@spinetix.com>
*
* This file is part of the CoAP library libcoap. Please see
* README for terms of use.
*/

#ifndef COAP_SESSION_H_
#define COAP_SESSION_H_


#include <coap2/coap_io.h>
#include <coap2/coap_time.h>
#include <coap2/pdu.h>
#include <coap2/coap_mbedtls.h>

struct coap_endpoint_t;
struct coap_context_t;
struct coap_queue_t;

struct coap_str_const_t;
struct coap_bin_const_t;
struct coap_session_t;
struct coap_dtls_pki_t;
struct coap_dtls_cpsk_t;


/**
 * The structure that holds the PKI PEM definitions.
 */
typedef struct coap_pki_key_pem_t {
  const char *ca_file;       /**< File location of Common CA in PEM format */
  const char *public_cert;   /**< File location of Public Cert in PEM format */
  const char *private_key;   /**< File location of Private Key in PEM format */
} coap_pki_key_pem_t;

/**
 * The structure that holds the PKI PEM buffer definitions.
 * The certificates and private key data must be in PEM format.
 *
 * Note:  The Certs and Key should be NULL terminated strings for
 * performance reasons (to save a potential buffer copy) and the length include
 * this NULL terminator. It is not a requirement to have the NULL terminator
 * though and the length must then reflect the actual data size.
 */
typedef struct coap_pki_key_pem_buf_t {
  const uint8_t *ca_cert;     /**< PEM buffer Common CA Cert */
  const uint8_t *public_cert; /**< PEM buffer Public Cert */
  const uint8_t *private_key; /**< PEM buffer Private Key */
  size_t ca_cert_len;         /**< PEM buffer CA Cert length */
  size_t public_cert_len;     /**< PEM buffer Public Cert length */
  size_t private_key_len;     /**< PEM buffer Private Key length */
} coap_pki_key_pem_buf_t;


/**
 * The enum used for determining the provided PKI ASN.1 (DER) Private Key
 * formats.
 */
typedef enum coap_asn1_privatekey_type_t {
  COAP_ASN1_PKEY_NONE,     /**< NONE */
  COAP_ASN1_PKEY_RSA,      /**< RSA type */
  COAP_ASN1_PKEY_RSA2,     /**< RSA2 type */
  COAP_ASN1_PKEY_DSA,      /**< DSA type */
  COAP_ASN1_PKEY_DSA1,     /**< DSA1 type */
  COAP_ASN1_PKEY_DSA2,     /**< DSA2 type */
  COAP_ASN1_PKEY_DSA3,     /**< DSA3 type */
  COAP_ASN1_PKEY_DSA4,     /**< DSA4 type */
  COAP_ASN1_PKEY_DH,       /**< DH type */
  COAP_ASN1_PKEY_DHX,      /**< DHX type */
  COAP_ASN1_PKEY_EC,       /**< EC type */
  COAP_ASN1_PKEY_HMAC,     /**< HMAC type */
  COAP_ASN1_PKEY_CMAC,     /**< CMAC type */
  COAP_ASN1_PKEY_TLS1_PRF, /**< TLS1_PRF type */
  COAP_ASN1_PKEY_HKDF      /**< HKDF type */
} coap_asn1_privatekey_type_t;

/**
 * The structure that holds the PKI ASN.1 (DER) definitions.
 */
typedef struct coap_pki_key_asn1_t {
  const uint8_t *ca_cert;     /**< ASN1 (DER) Common CA Cert */
  const uint8_t *public_cert; /**< ASN1 (DER) Public Cert */
  const uint8_t *private_key; /**< ASN1 (DER) Private Key */
  size_t ca_cert_len;         /**< ASN1 CA Cert length */
  size_t public_cert_len;     /**< ASN1 Public Cert length */
  size_t private_key_len;     /**< ASN1 Private Key length */
  coap_asn1_privatekey_type_t private_key_type; /**< Private Key Type */
} coap_pki_key_asn1_t;


/**
 * The enum used for determining the PKI key formats.
 */
typedef enum coap_pki_key_t {
  COAP_PKI_KEY_PEM = 0,        /**< The PKI key type is PEM file */
  COAP_PKI_KEY_ASN1,           /**< The PKI key type is ASN.1 (DER) */
  COAP_PKI_KEY_PEM_BUF,        /**< The PKI key type is PEM buffer */
} coap_pki_key_t;

/**
 * The structure that holds the PKI key information.
 */
typedef struct coap_dtls_key_t {
  coap_pki_key_t key_type;          /**< key format type */
  union {
    coap_pki_key_pem_t pem;          /**< for PEM file keys */
    coap_pki_key_pem_buf_t pem_buf;  /**< for PEM memory keys */
    coap_pki_key_asn1_t asn1;        /**< for ASN.1 (DER) file keys */
  } key;
} coap_dtls_key_t;

/**
 * Server Name Indication (SNI) Validation call-back that can be set up by
 * coap_context_set_pki().
 * Invoked if the SNI is not previously seen and prior to sending a certificate
 * set back to the client so that the appropriate certificate set can be used
 * based on the requesting SNI.
 *
 * @param sni  The requested SNI
 * @param arg  The same as was passed into coap_context_set_pki()
 *             in setup_data->sni_call_back_arg
 *
 * @return New set of certificates to use, or @c NULL if SNI is to be rejected.
 */
typedef coap_dtls_key_t *(*coap_dtls_sni_callback_t)(const char *sni,
             void* arg);

/**
 * CN Validation call-back that can be set up by coap_context_set_pki().
 * Invoked when libcoap has done the validation checks at the TLS level,
 * but the application needs to check that the CN is allowed.
 * CN is the SubjectAltName in the cert, if not present, then the leftmost
 * Common Name (CN) component of the subject name.
 *
 * @param cn  The determined CN from the certificate
 * @param asn1_public_cert  The ASN.1 DER encoded X.509 certificate
 * @param asn1_length  The ASN.1 length
 * @param coap_session  The CoAP session associated with the certificate update
 * @param depth  Depth in cert chain.  If 0, then client cert, else a CA
 * @param validated  TLS layer can find no issues if 1
 * @param arg  The same as was passed into coap_context_set_pki()
 *             in setup_data->cn_call_back_arg
 *
 * @return @c 1 if accepted, else @c 0 if to be rejected.
 */
typedef int (*coap_dtls_cn_callback_t)(const char *cn,
             const uint8_t *asn1_public_cert,
             size_t asn1_length,
             struct coap_session_t *coap_session,
             unsigned int depth,
             int validated,
             void *arg);

#define COAP_DTLS_PKI_SETUP_VERSION 1 /**< Latest PKI setup version */


/**
 * Additional Security setup handler that can be set up by
 * coap_context_set_pki().
 * Invoked when libcoap has done the validation checks at the TLS level,
 * but the application needs to do some additional checks/changes/updates.
 *
 * @param tls_session The security session definition - e.g. SSL * for OpenSSL.
 *                    NULL if server call-back.
 *                    This will be dependent on the underlying TLS library -
 *                    see coap_get_tls_library_version()
 * @param setup_data A structure containing setup data originally passed into
 *                   coap_context_set_pki() or coap_new_client_session_pki().
 *
 * @return @c 1 if successful, else @c 0.
 */
typedef int (*coap_dtls_security_setup_t)(void* tls_session,
                                        struct coap_dtls_pki_t *setup_data);

/**
 * The structure used for defining the PKI setup data to be used.
 */
typedef struct coap_dtls_pki_t {
  uint8_t version; /** Set to 1 to support this version of the struct */

  /* Options to enable different TLS functionality in libcoap */
  uint8_t verify_peer_cert;        /**< 1 if peer cert is to be verified */
  uint8_t require_peer_cert;       /**< 1 if peer cert is required */
  uint8_t allow_self_signed;       /**< 1 if self signed certs are allowed */
  uint8_t allow_expired_certs;     /**< 1 if expired certs are allowed */
  uint8_t cert_chain_validation;   /**< 1 if to check cert_chain_verify_depth */
  uint8_t cert_chain_verify_depth; /**< recommended depth is 3 */
  uint8_t check_cert_revocation;   /**< 1 if revocation checks wanted */
  uint8_t allow_no_crl;            /**< 1 ignore if CRL not there */
  uint8_t allow_expired_crl;       /**< 1 if expired crl is allowed */
  uint8_t allow_bad_md_hash;       /**< 1 if unsupported MD hashes are allowed */
  uint8_t allow_short_rsa_length;  /**< 1 if small RSA keysizes are allowed */
  uint8_t reserved[4];             /**< Reserved - must be set to 0 for
                                        future compatibility */
                                   /* Size of 4 chosen to align to next
                                    * parameter, so if newly defined option
                                    * it can use one of the reserverd slot so
                                    * no need to change
                                    * COAP_DTLS_PKI_SETUP_VERSION and just
                                    * decrement the reserved[] count.
                                    */

  /** CN check callback function.
   * If not NULL, is called when the TLS connection has passed the configured
   * TLS options above for the application to verify if the CN is valid.
   */
  coap_dtls_cn_callback_t validate_cn_call_back;
  void *cn_call_back_arg;  /**< Passed in to the CN callback function */

  /** SNI check callback function.
   * If not @p NULL, called if the SNI is not previously seen and prior to
   * sending a certificate set back to the client so that the appropriate
   * certificate set can be used based on the requesting SNI.
   */
  coap_dtls_sni_callback_t validate_sni_call_back;
  void *sni_call_back_arg;  /**< Passed in to the sni call-back function */

  /** Additional Security call-back handler that is invoked when libcoap has
   * done the standerd, defined validation checks at the TLS level,
   * If not @p NULL, called from within the TLS Client Hello connection
   * setup.
   */
  coap_dtls_security_setup_t additional_tls_setup_call_back;

  char* client_sni;    /**<  If not NULL, SNI to use in client TLS setup.
                             Owned by the client app and must remain valid
                             during the call to coap_new_client_session_pki() */

  coap_dtls_key_t pki_key;  /**< PKI key definition */
} coap_dtls_pki_t;

/**
 * The structure that holds the Client PSK information.
 */
typedef struct coap_dtls_cpsk_info_t {
  coap_bin_const_t identity;
  coap_bin_const_t key;
} coap_dtls_cpsk_info_t;

/**
 * Identity Hint Validation callback that can be set up by
 * coap_new_client_session_psk2().
 * Invoked when libcoap has done the validation checks at the TLS level,
 * but the application needs to check that the Identity Hint is allowed,
 * and thus needs to use the appropriate PSK information for the Identity
 * Hint for the (D)TLS session.
 * Note: Identity Hint is not supported in (D)TLS1.3.
 *
 * @param hint  The server provided Identity Hint
 * @param coap_session  The CoAP session associated with the Identity Hint
 * @param arg  The same as was passed into coap_new_client_session_psk2()
 *             in setup_data->ih_call_back_arg
 *
 * @return New coap_dtls_cpsk_info_t object or @c NULL on error.
 */
typedef const coap_dtls_cpsk_info_t *(*coap_dtls_ih_callback_t)(
                                struct coap_str_const_t *hint,
                                struct coap_session_t *coap_session,
                                void *arg);

#define COAP_DTLS_CPSK_SETUP_VERSION 1 /**< Latest CPSK setup version */

/**
 * The structure used for defining the Client PSK setup data to be used.
 */
typedef struct coap_dtls_cpsk_t {
  uint8_t version; /** Set to COAP_DTLS_CPSK_SETUP_VERSION
                       to support this version of the struct */

  /* Options to enable different TLS functionality in libcoap */
  uint8_t reserved[7];             /**< Reserved - must be set to 0 for
                                        future compatibility */
                                   /* Size of 7 chosen to align to next
                                    * parameter, so if newly defined option
                                    * it can use one of the reserverd slot so
                                    * no need to change
                                    * COAP_DTLS_CPSK_SETUP_VERSION and just
                                    * decrement the reserved[] count.
                                    */

  /** Identity Hint check callback function.
   * If not NULL, is called when the Identity Hint (TLS1.2 or earlier) is
   * provided by the server.
   * The appropriate Identity and Pre-shared Key to use can then be returned.
   */
  coap_dtls_ih_callback_t validate_ih_call_back;
  void *ih_call_back_arg;  /**< Passed in to the Identity Hint callback
                                function */

  char* client_sni;    /**< If not NULL, SNI to use in client TLS setup.
                            Owned by the client app and must remain valid
                            during the call to coap_new_client_session_psk2()
                            Note: Not supported by TinyDTLS. */

  coap_dtls_cpsk_info_t psk_info;  /**< Client PSK definition */
} coap_dtls_cpsk_t;

/**
 * The structure that holds the Server Pre-Shared Key and Identity
 * Hint information.
 */
typedef struct coap_dtls_spsk_info_t {
  coap_bin_const_t hint;
  coap_bin_const_t key;
} coap_dtls_spsk_info_t;


/**
 * Identity Validation callback that can be set up by
 * coap_context_set_psk2().
 * Invoked when libcoap has done the validation checks at the TLS level,
 * but the application needs to check that the Identity is allowed,
 * and needs to use the appropriate Pre-Shared Key for the (D)TLS session.
 *
 * @param identity  The client provided Identity
 * @param coap_session  The CoAP session associated with the Identity Hint
 * @param arg  The value as passed into coap_context_set_psk2()
 *             in setup_data->id_call_back_arg
 *
 * @return New coap_bin_const_t object containing the Pre-Shared Key or
           @c NULL on error.
 *         Note: This information will be duplicated into an internal
 *               structure.
 */
typedef const coap_bin_const_t *(*coap_dtls_id_callback_t)(
                                 struct coap_bin_const_t *identity,
                                 struct coap_session_t *coap_session,
                                 void *arg);
/**
 * PSK SNI callback that can be set up by coap_context_set_psk2().
 * Invoked when libcoap has done the validation checks at the TLS level
 * and the application needs to:-
 * a) check that the SNI is allowed
 * b) provide the appropriate PSK information for the (D)TLS session.
 *
 * @param sni  The client provided SNI
 * @param coap_session  The CoAP session associated with the SNI
 * @param arg  The same as was passed into coap_context_set_psk2()
 *             in setup_data->sni_call_back_arg
 *
 * @return New coap_dtls_spsk_info_t object or @c NULL on error.
 */
typedef const coap_dtls_spsk_info_t *(*coap_dtls_psk_sni_callback_t)(
                                 const char *sni,
                                 struct coap_session_t *coap_session,
                                 void *arg);

#define COAP_DTLS_SPSK_SETUP_VERSION 1 /**< Latest CPSK setup version */

/**
 * The structure used for defining the Server PSK setup data to be used.
 */
typedef struct coap_dtls_spsk_t {
  uint8_t version; /** Set to COAP_DTLS_SPSK_SETUP_VERSION
                       to support this version of the struct */

  /* Options to enable different TLS functionality in libcoap */
  uint8_t reserved[7];             /**< Reserved - must be set to 0 for
                                        future compatibility */
                                   /* Size of 7 chosen to align to next
                                    * parameter, so if newly defined option
                                    * it can use one of the reserverd slot so
                                    * no need to change
                                    * COAP_DTLS_SPSK_SETUP_VERSION and just
                                    * decrement the reserved[] count.
                                    */

  /** Identity check callback function.
   * If not @p NULL, is called when the Identity is provided by the client.
   * The appropriate Pre-Shared Key to use can then be returned.
   */
  coap_dtls_id_callback_t validate_id_call_back;
  void *id_call_back_arg;  /**< Passed in to the Identity callback function */

  /** SNI check callback function.
   * If not @p NULL, called if the SNI is not previously seen and prior to
   * sending PSK information back to the client so that the appropriate
   * PSK information can be used based on the requesting SNI.
   */
  coap_dtls_psk_sni_callback_t validate_sni_call_back;
  void *sni_call_back_arg;  /**< Passed in to the SNI callback function */

  coap_dtls_spsk_info_t psk_info;  /**< Server PSK definition */
} coap_dtls_spsk_t;



/**
* Abstraction of a fixed point number that can be used where necessary instead
* of a float.  1,000 fractional bits equals one integer
*/
typedef struct coap_fixed_point_t {
  uint16_t integer_part;    /**< Integer part of fixed point variable */
  uint16_t fractional_part; /**< Fractional part of fixed point variable
                                1/1000 (3 points) precision */
} coap_fixed_point_t;

#define COAP_DEFAULT_SESSION_TIMEOUT 300
#define COAP_PARTIAL_SESSION_TIMEOUT_TICKS (30 * COAP_TICKS_PER_SECOND)
#define COAP_DEFAULT_MAX_HANDSHAKE_SESSIONS 100

#define COAP_PROTO_NOT_RELIABLE(p) ((p)==COAP_PROTO_UDP || (p)==COAP_PROTO_DTLS)
#define COAP_PROTO_RELIABLE(p) ((p)==COAP_PROTO_TCP || (p)==COAP_PROTO_TLS)

typedef uint8_t coap_session_type_t;
/**
 * coap_session_type_t values
 */
#define COAP_SESSION_TYPE_CLIENT 1  /**< client-side */
#define COAP_SESSION_TYPE_SERVER 2  /**< server-side */
#define COAP_SESSION_TYPE_HELLO  3  /**< server-side ephemeral session for responding to a client hello */

typedef uint8_t coap_session_state_t;
/**
 * coap_session_state_t values
 */
#define COAP_SESSION_STATE_NONE                0
#define COAP_SESSION_STATE_CONNECTING        1
#define COAP_SESSION_STATE_HANDSHAKE        2
#define COAP_SESSION_STATE_CSM                3
#define COAP_SESSION_STATE_ESTABLISHED        4

typedef struct coap_session_t {
  struct coap_session_t *next;
  coap_proto_t proto;               /**< protocol used */
  coap_session_type_t type;         /**< client or server side socket */
  coap_session_state_t state;       /**< current state of relationaship with peer */
  unsigned ref;                     /**< reference count from queues */
  unsigned tls_overhead;            /**< overhead of TLS layer */
  unsigned mtu;                     /**< path or CSM mtu */
  coap_address_t local_if;          /**< optional local interface address */
  coap_address_t remote_addr;       /**< remote address and port */
  coap_address_t local_addr;        /**< local address and port */
  int ifindex;                      /**< interface index */
  coap_socket_t sock;               /**< socket object for the session, if any */
  struct coap_endpoint_t *endpoint; /**< session's endpoint */
  struct coap_context_t *context;   /**< session's context */
  void *tls;                        /**< security parameters */
  uint16_t tx_mid;                  /**< the last message id that was used in this session */
  uint8_t con_active;               /**< Active CON request sent */
  struct coap_queue_t *delayqueue;  /**< list of delayed messages waiting to be sent */
  size_t partial_write;             /**< if > 0 indicates number of bytes already written from the pdu at the head of sendqueue */
  uint8_t read_header[8];           /**< storage space for header of incoming message header */
  size_t partial_read;              /**< if > 0 indicates number of bytes already read for an incoming message */
  coap_pdu_t *partial_pdu;          /**< incomplete incoming pdu */
  coap_tick_t last_rx_tx;
  coap_tick_t last_tx_rst;
  coap_tick_t last_ping;
  coap_tick_t last_pong;
  coap_tick_t csm_tx;
  uint8_t *psk_identity;
  size_t psk_identity_len;
  uint8_t *psk_key;
  size_t psk_key_len;
  
  coap_dtls_cpsk_t mbedtls_cpsk_setup_data; /**< client provided PSK initial setup data */
  
  coap_bin_const_t *mbedtls_psk_identity;   /**< If client, this field contains the
                                      current identity for server; When this
                                      field is NULL, the current identity is
                                      contained in cpsk_setup_data

                                      If server, this field contains the client
                                      provided identity.

                                      Value maintained internally */
  coap_bin_const_t *mbedtls_psk_key;        /**< If client, this field contains the
                                      current pre-shared key for server;
                                      When this field is NULL, the current
                                      key is contained in cpsk_setup_data

                                      If server, this field contains the
                                      client's current key.

                                      Value maintained internally */
  coap_bin_const_t *mbedtls_psk_hint;       /**< If client, this field contains the
                                      server provided identity hint.

                                      If server, this field contains the
                                      current hint for the client; When this
                                      field is NULL, the current hint is
                                      contained in context->spsk_setup_data

                                      Value maintained internally */
  void *app;                        /**< application-specific data */
  unsigned int max_retransmit;      /**< maximum re-transmit count (default 4) */
  coap_fixed_point_t ack_timeout;   /**< timeout waiting for ack (default 2 secs) */
  coap_fixed_point_t ack_random_factor; /**< ack random factor backoff (default 1.5) */
  unsigned int dtls_timeout_count;      /**< dtls setup retry counter */
  int dtls_event;                       /**< Tracking any (D)TLS events on this sesison */
  uint8_t raiFlag;                      /**< for NB Network to release immediately, add by wqzhao */
} coap_session_t;

/**
* Increment reference counter on a session.
*
* @param session The CoAP session.
* @return same as session
*/
coap_session_t *coap_session_reference(coap_session_t *session);

/**
* Decrement reference counter on a session.
* Note that the session may be deleted as a result and should not be used
* after this call.
*
* @param session The CoAP session.
*/
void coap_session_release(coap_session_t *session);

/**
* Stores @p data with the given session. This function overwrites any value
* that has previously been stored with @p session.
*/
void coap_session_set_app_data(coap_session_t *session, void *data);

/**
* Returns any application-specific data that has been stored with @p
* session using the function coap_session_set_app_data(). This function will
* return @c NULL if no data has been stored.
*/
void *coap_session_get_app_data(const coap_session_t *session);

/**
* Notify session that it has failed.
*
* @param session The CoAP session.
* @param reason The reason why the session was disconnected.
*/
void coap_session_disconnected(coap_session_t *session, coap_nack_reason_t reason);

/**
* Notify session transport has just connected and CSM exchange can now start.
*
* @param session The CoAP session.
*/
void coap_session_send_csm(coap_session_t *session);

/**
* Notify session that it has just connected or reconnected.
*
* @param session The CoAP session.
*/
void coap_session_connected(coap_session_t *session);

/**
* Set the session MTU. This is the maximum message size that can be sent,
* excluding IP and UDP overhead.
*
* @param session The CoAP session.
* @param mtu maximum message size
*/
void coap_session_set_mtu(coap_session_t *session, unsigned mtu);

/**
 * Get maximum acceptable PDU size
 *
 * @param session The CoAP session.
 * @return maximum PDU size, not including header (but including token).
 */
size_t coap_session_max_pdu_size(const coap_session_t *session);

/**
* Creates a new client session to the designated server.
* @param ctx The CoAP context.
* @param local_if Address of local interface. It is recommended to use NULL to let the operating system choose a suitable local interface. If an address is specified, the port number should be zero, which means that a free port is automatically selected.
* @param server The server's address. If the port number is zero, the default port for the protocol will be used.
* @param proto Protocol.
*
* @return A new CoAP session or NULL if failed. Call coap_session_release to free.
*/
coap_session_t *coap_new_client_session(
  struct coap_context_t *ctx,
  const coap_address_t *local_if,
  const coap_address_t *server,
  coap_proto_t proto
);

/**
* Creates a new client session to the designated server with PSK credentials
* @param ctx The CoAP context.
* @param local_if Address of local interface. It is recommended to use NULL to let the operating system choose a suitable local interface. If an address is specified, the port number should be zero, which means that a free port is automatically selected.
* @param server The server's address. If the port number is zero, the default port for the protocol will be used.
* @param proto Protocol.
* @param identity PSK client identity
* @param key PSK shared key
* @param key_len PSK shared key length
*
* @return A new CoAP session or NULL if failed. Call coap_session_release to free.
*/
coap_session_t *coap_new_client_session_psk(
  struct coap_context_t *ctx,
  const coap_address_t *local_if,
  const coap_address_t *server,
  coap_proto_t proto,
  const char *identity,
  const uint8_t *key,
  unsigned key_len
);

struct coap_dtls_pki_t;

/**
* Creates a new client session to the designated server with PKI credentials
* @param ctx The CoAP context.
* @param local_if Address of local interface. It is recommended to use NULL to
*                 let the operating system choose a suitable local interface.
*                 If an address is specified, the port number should be zero,
*                 which means that a free port is automatically selected.
* @param server The server's address. If the port number is zero, the default
*               port for the protocol will be used.
* @param proto CoAP Protocol.
* @param setup_data PKI parameters.
*
* @return A new CoAP session or NULL if failed. Call coap_session_release()
*         to free.
*/
coap_session_t *coap_new_client_session_pki(
  struct coap_context_t *ctx,
  const coap_address_t *local_if,
  const coap_address_t *server,
  coap_proto_t proto,
  struct coap_dtls_pki_t *setup_data
);

/**
* Creates a new server session for the specified endpoint.
* @param ctx The CoAP context.
* @param ep An endpoint where an incoming connection request is pending.
*
* @return A new CoAP session or NULL if failed. Call coap_session_release to free.
*/
coap_session_t *coap_new_server_session(
  struct coap_context_t *ctx,
  struct coap_endpoint_t *ep
);

/**
* Function interface for datagram data transmission. This function returns
* the number of bytes that have been transmitted, or a value less than zero
* on error.
*
* @param session          Session to send data on.
* @param data             The data to send.
* @param datalen          The actual length of @p data.
*
* @return                 The number of bytes written on success, or a value
*                         less than zero on error.
*/
ssize_t coap_session_send(coap_session_t *session,
  const uint8_t *data, size_t datalen);

/**
* Function interface for stream data transmission. This function returns
* the number of bytes that have been transmitted, or a value less than zero
* on error. The number of bytes written may be less than datalen because of
* congestion control.
*
* @param session          Session to send data on.
* @param data             The data to send.
* @param datalen          The actual length of @p data.
*
* @return                 The number of bytes written on success, or a value
*                         less than zero on error.
*/
ssize_t coap_session_write(coap_session_t *session,
  const uint8_t *data, size_t datalen);

/**
* Send a pdu according to the session's protocol. This function returns
* the number of bytes that have been transmitted, or a value less than zero
* on error.
*
* @param session          Session to send pdu on.
* @param pdu              The pdu to send.
*
* @return                 The number of bytes written on success, or a value
*                         less than zero on error.
*/
ssize_t coap_session_send_pdu(coap_session_t *session, coap_pdu_t *pdu);


/**
 * @ingroup logging
 * Get session description.
 *
 * @param session  The CoAP session.
 * @return description string.
 */
const char *coap_session_str(const coap_session_t *session);

ssize_t
coap_session_delay_pdu(coap_session_t *session, coap_pdu_t *pdu,
                       struct coap_queue_t *node);
/**
* Abstraction of virtual endpoint that can be attached to coap_context_t. The
* tuple (handle, addr) must uniquely identify this endpoint.
*/
typedef struct coap_endpoint_t {
  struct coap_endpoint_t *next;
  struct coap_context_t *context; /**< endpoint's context */
  coap_proto_t proto;             /**< protocol used on this interface */
  uint16_t default_mtu;           /**< default mtu for this interface */
  coap_socket_t sock;             /**< socket object for the interface, if any */
  coap_address_t bind_addr;       /**< local interface address */
  coap_session_t *sessions;       /**< list of active sessions */
  coap_session_t hello;           /**< special session of DTLS hello messages */
} coap_endpoint_t;

/**
* Create a new endpoint for communicating with peers.
*
* @param context        The coap context that will own the new endpoint
* @param listen_addr    Address the endpoint will listen for incoming requests on or originate outgoing requests from. Use NULL to specify that no incoming request will be accepted and use a random endpoint.
* @param proto          Protocol used on this endpoint
*/

coap_endpoint_t *coap_new_endpoint(struct coap_context_t *context, const coap_address_t *listen_addr, coap_proto_t proto);

/**
* Set the endpoint's default MTU. This is the maximum message size that can be
* sent, excluding IP and UDP overhead.
*
* @param endpoint The CoAP endpoint.
* @param mtu maximum message size
*/
void coap_endpoint_set_default_mtu(coap_endpoint_t *endpoint, unsigned mtu);

void coap_free_endpoint(coap_endpoint_t *ep);


/**
 * @ingroup logging
* Get endpoint description.
*
* @param endpoint  The CoAP endpoint.
* @return description string.
*/
const char *coap_endpoint_str(const coap_endpoint_t *endpoint);

/**
* Lookup the server session for the packet received on an endpoint, or create
* a new one.
*
* @param endpoint Active endpoint the packet was received on.
* @param packet Received packet.
* @param now The current time in ticks.
* @return The CoAP session.
*/
coap_session_t *coap_endpoint_get_session(coap_endpoint_t *endpoint,
  const struct coap_packet_t *packet, coap_tick_t now);

/**
 * Create a new DTLS session for the @p endpoint.
 *
 * @ingroup dtls_internal
 *
 * @param endpoint  Endpoint to add DTLS session to
 * @param packet    Received packet information to base session on.
 * @param now       The current time in ticks.
 *
 * @return Created CoAP session or @c NULL if error.
 */
coap_session_t *coap_endpoint_new_dtls_session(coap_endpoint_t *endpoint,
  const struct coap_packet_t *packet, coap_tick_t now);

coap_session_t *coap_session_get_by_peer(struct coap_context_t *ctx,
  const struct coap_address_t *remote_addr, int ifindex);

void coap_session_free(coap_session_t *session);
void coap_session_mfree(coap_session_t *session);

 /**
  * @defgroup cc Rate Control
  * The transmission parameters for CoAP rate control ("Congestion
  * Control" in stream-oriented protocols) are defined in
  * https://tools.ietf.org/html/rfc7252#section-4.8
  * @{
  */

  /**
   * Number of seconds when to expect an ACK or a response to an
   * outstanding CON message.
   * RFC 7252, Section 4.8 Default value of ACK_TIMEOUT is 2
   */
#define COAP_DEFAULT_ACK_TIMEOUT ((coap_fixed_point_t){2,0})

   /**
    * A factor that is used to randomize the wait time before a message
    * is retransmitted to prevent synchronization effects.
    * RFC 7252, Section 4.8 Default value of ACK_RANDOM_FACTOR is 1.5
    */
#define COAP_DEFAULT_ACK_RANDOM_FACTOR ((coap_fixed_point_t){1,500})

    /**
     * Number of message retransmissions before message sending is stopped
     * RFC 7252, Section 4.8 Default value of MAX_RETRANSMIT is 4
     */
#define COAP_DEFAULT_MAX_RETRANSMIT  4

     /**
      * The number of simultaneous outstanding interactions that a client
      * maintains to a given server.
      * RFC 7252, Section 4.8 Default value of NSTART is 1
      */
#define COAP_DEFAULT_NSTART 1

      /** @} */

/**
* Set the CoAP maximum retransmit count before failure
*
* Number of message retransmissions before message sending is stopped
*
* @param session The CoAP session.
* @param value The value to set to. The default is 4 and should not normally
*              get changed.
*/
void coap_session_set_max_retransmit(coap_session_t *session,
                                     unsigned int value);

/**
* Set the CoAP initial ack response timeout before the next re-transmit
*
* Number of seconds when to expect an ACK or a response to an
* outstanding CON message.
*
* @param session The CoAP session.
* @param value The value to set to. The default is 2 and should not normally
*              get changed.
*/
void coap_session_set_ack_timeout(coap_session_t *session,
                                  coap_fixed_point_t value);

/**
* Set the CoAP ack randomize factor
*
* A factor that is used to randomize the wait time before a message
* is retransmitted to prevent synchronization effects.
*
* @param session The CoAP session.
* @param value The value to set to. The default is 1.5 and should not normally
*              get changed.
*/
void coap_session_set_ack_random_factor(coap_session_t *session,
                                        coap_fixed_point_t value);

/**
* Get the CoAP maximum retransmit before failure
*
* Number of message retransmissions before message sending is stopped
*
* @param session The CoAP session.
*
* @return Current maximum retransmit value
*/
unsigned int coap_session_get_max_transmit(coap_session_t *session);

/**
* Get the CoAP initial ack response timeout before the next re-transmit
*
* Number of seconds when to expect an ACK or a response to an
* outstanding CON message.
*
* @param session The CoAP session.
*
* @return Current ack response timeout value
*/
coap_fixed_point_t coap_session_get_ack_timeout(coap_session_t *session);

/**
* Get the CoAP ack randomize factor
*
* A factor that is used to randomize the wait time before a message
* is retransmitted to prevent synchronization effects.
*
* @param session The CoAP session.
*
* @return Current ack randomize value
*/
coap_fixed_point_t coap_session_get_ack_random_factor(coap_session_t *session);

/**
 * Send a ping message for the session.
 * @param session The CoAP session.
 *
 * @return COAP_INVALID_TID if there is an error
 */
coap_tid_t coap_session_send_ping(coap_session_t *session);

#endif  /* COAP_SESSION_H */
