
/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - secwrapper.h
 Description:    - support security api to vendor for softSIM functions
 History:        - 09/09/2020  created by yjwang
 ******************************************************************************
******************************************************************************/

#ifndef _ECSECAPI_H
#define _ECSECAPI_H

#if defined(_WIN32)
#include "win32typedef.h"
#else
#include "commontypedef.h"
#endif

#include "debug_trace.h"
#include "debug_log.h"

#define   TA_RESULTE_FAIL   (1)
#define   TA_RESULTE_SUCC   (0)

/*-------------------------- define macro interface-------------------*/

extern UINT8 TA_get_random(UINT16 length, UINT8* random);
extern UINT8 TA_aes_ctrl(UINT8 *key_in, UINT8 *data_in, UINT32 length, UINT8 *iv_in, UINT8 *data_out, UINT8 mode, UINT8 encryption);
extern UINT8 TA_sha256(const UINT8 *content, UINT16 content_len, UINT8 md[32]);
extern UINT8 TA_ec256_register_curve(const char *ec_name);
extern UINT8 TA_ec256_create_key_pair(UINT8 sk[32], UINT8 pk[64]);
extern UINT8 TA_ec256_is_on_curve(UINT8 pk[64]);
extern UINT8 TA_ec256_ecdh(UINT8 sk[32], UINT8 pk[64], UINT8 sab[32]);
extern UINT8 TA_ec256_ecdsa_sign(UINT8 *dgst, UINT16 dgstLen, UINT8 sk[32], UINT8 sign[64]);
extern UINT8 GenerateCMAC(UINT8 *Key, UINT8 *Message, UINT32 MessLen,UINT8 *Cmac);
extern UINT8 TA_ec256_create_csr(UINT8 sk[32], UINT8 pk[64], UINT8 *subject,UINT8 *outbuf, UINT16 *iolen);
extern UINT8 TA_ec256_ecdsa_verify(UINT8 *dgst, UINT16 dgst_len, UINT8 sign[64],UINT8 pk[64]);
extern UINT8 TA_sfs_write(UINT16 name_id, const UINT8 *kvalue, UINT16 kvalue_length, UINT8 sec_store);
extern UINT8 TA_sfs_read(UINT16 name_id, UINT16 kvalue_max_length,UINT16 *kvalue_length,UINT8 *buffer,UINT8 sec_store);
extern UINT8  VerifyCMac( 
                     const CHAR*             testname,
                      const UINT8*           key,
                      INT32                  keybits,
                      const UINT8*           messages,	
                      const UINT32           message_lengths[4],
                      const UINT8*           expected_result,
                      UINT8                  cipher_type,
                      INT32                  block_size,
                      INT32                  num_tests );

extern UINT8 TA_ec256_method_register(void);
extern void  esim_log_printf(const char *format);

#endif

