
 /******************************************************************************
  ******************************************************************************
  Copyright:	  - 2020- Copyrights of EigenComm Ltd.
  File name:	  - secwrapper.h
  Description:	  - support security api to vendor for softSIM functions
  History:		  - 09/19/2020	created by yjwang
  ******************************************************************************
 ******************************************************************************/


#ifndef _EC_SEC_CALLBACK_H
#define _EC_SEC_CALLBACK_H


#ifdef __cplusplus
 extern "C" {
#endif

#include "EC_interface.h"

#if defined(_WIN32)
#include "win32typedef.h"
#else
#include "commontypedef.h"
#endif

#define ECC_SIGN_CSR_NEED_HASH        1
#define ECC_SIGN_CSR_NO_NEED_HASH     0
#define ECC_SEC_AES_CBC_MODE     1
#define ECC_SEC_AES_EBC_MODE     0
#define ECC_SEC_ENCRYPT_OPERATION   1
#define ECC_SEC_DECRYPT_OPERATION   0

#define ECC_RESULTE_SUCC     0
#define ECC_RESULTE_FAIL     1

/*the ecdh alg struct*/
typedef struct ec_alg_ecdh_info{
    mbedtls_ecdh_context  ecdh_context;
}EC_ALG_ECDH_INFO,*PEC_ALG_ECDH_INFO;


/*the digit signature based on ECC alg struct*/
typedef struct ec_dsig_ecdsa_info{
    mbedtls_ecdsa_context  ecdsa_context;
}EC_DSIG_ECDSA_INFO,*PEC_DSIG_ECDSA_INFO;

/*the digit X509 CSR request struct*/
typedef struct ec_dig_x509_csr_req_info{
    UINT8       csr_usage;           /*csr usage:*/
    UINT8       csr_type;            /*client csr or server csr*/
    UINT8       csr_name[256];       /*csr subject*/
    UINT8       csr_default_type;    /*csr default type, PEM type*/
    mbedtls_x509_crt      csr;

}EC_DIG_X509_CSR_REQ_INFO, *PEC_DIG_X509_CSR_REQ_INFO;

typedef struct ec_sec_method
{
    UINT8    (*EC_softsim_random)(UINT16 , UINT8 *);
    UINT8    (*EC_softsim_aes)( UINT8 *key_in, UINT8 *data_in, UINT32 length, UINT8 *iv_in, UINT8 *data_out, UINT8 mode, UINT8 encryption,UINT8 keybits);
    UINT8    (*EC_softsim_sha256)(const UINT8 *content, UINT16 content_len, UINT8 *hash_md,UINT16 mdlen);
    UINT8    (*EC_softsim_ecpregister)(const CHAR *ec_name);
    UINT8    (*EC_softsim_create_ecc_keypair)(UINT8 *sk, UINT16 skLen,UINT8 *pk, UINT16 pkLen);
    UINT8    (*EC_softsim_check_pk_ison_curve)(UINT8 *pk, UINT16 pkLen);
    UINT8    (*EC_softsim_ecc_ecdh)(UINT8 *sk,UINT16 skLen, UINT8 *pk,UINT16 pkLen, UINT8 *sab,UINT16 sabLen);
    UINT8    (*EC_softsim_ecdsa_sign)(UINT8 *dgst, UINT16 dgstLen, UINT8 *sk,UINT16 skLen, UINT8 *sign,UINT16 signLen);
    UINT8    (*EC_softsim_generateCMac)(const UINT8 *Key, INT32 keybits,const UINT8 *Message, UINT32 MessLen,UINT8* Cmac,UINT8 cipher_type);
    UINT8    (*EC_softsim_VerifyCMac)(
                      const CHAR*            testname,
                      const UINT8*           keyMsg,
                      INT32                  keybits,
                      const UINT8*           messages,
                      const UINT32           message_lengths[4],
                      const UINT8*           expected_result,
                      UINT8                  cipher_type,
                      INT32                  block_size,
                      INT32                  num_tests );
    UINT8    (*EC_softsim_ecdsa_verify)(UINT8 *dgst, UINT16 dgst_len, UINT8 *sign,UINT16 signLen,UINT8 *pk,UINT16 pkLen);
    UINT8    (*EC_softsim_create_csr)(UINT8 *sk,UINT16 skLen, UINT8 *pk, UINT16 pkLen,UINT8 *subject,UINT8 *outbuf, UINT16 *iolen);
    UINT8    (*EC_softsim_flash_write)(UINT16 nameID,const UINT8 *kvalue, UINT16 kvLen, UINT8 sec_store);
    UINT8    (*EC_softsim_flash_read)(UINT16 nameID, UINT16 kvmaxLen, UINT32 *kOutLen, UINT8 *storebuff,UINT8 sec_store);
}EC_SEC_METHOD;

typedef struct  ECC_EC_SEC_CONF{
    char                      curve_name[64];
    UINT8                     alg_type;
    UINT8                     hash_type;
    UINT8                     encry_bit_type;
}ECC_EC_SEC_CONF,*PECC_EC_SEC_CONF;


/*the ESIM security context struct*/
typedef struct _EC_ESIM_SEC_CONTEXT_INFO_T{
    UINT8                     isInit;
    ECC_EC_SEC_CONF           conf;
    EC_ALG_ECDH_INFO          ecdh_info;   /*current we just no used it*/
    EC_DSIG_ECDSA_INFO        ecdsa_info;
    EC_DIG_X509_CSR_REQ_INFO  x509_csr_info;
    mbedtls_entropy_context   entropyContext;
    mbedtls_ctr_drbg_context  ctrDrbgContext;
    EC_SEC_METHOD             method;
}EC_ESIM_SEC_CONTEXT_INFO;


extern INT8 EC_sec_context_register(void);
extern EC_ESIM_SEC_CONTEXT_INFO  *EC_sec_get_instance(void);



#ifdef __cplusplus
}
#endif

#endif
