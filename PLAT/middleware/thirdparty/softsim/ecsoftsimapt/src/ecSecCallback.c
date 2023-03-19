
/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - secwrapper.h
 Description:    - support security api to vendor for softSIM functions
 History:        - 09/09/2020  created by yjwang
 ******************************************************************************/
#ifdef SOFTSIM_FEATURE_ENABLE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#include "cmsis_os2.h"
#include "ecSecCallback.h"
#include "platform.h"
#include "ecSecFlashApi.h"

#include "EC_pk.h"

#if !defined(_WIN32)
#include "debug_trace.h"
#include "debug_log.h"
#else
#define ECOMM_TRACE(x, ...)
#define ECOMM_DUMP(x, ...)

#endif


static  EC_ESIM_SEC_CONTEXT_INFO  gECSecContext = {0};

static mbedtls_ctr_drbg_context*  EC_sec_get_seed_context(void);
static UINT8 EC_sec_init(EC_ESIM_SEC_CONTEXT_INFO *pSecontext);
static UINT8 EC_sec_context_init(EC_ESIM_SEC_CONTEXT_INFO *pSecontext);
static UINT8 EC_ecc256_create_ecdsa_key_pair(
                    mbedtls_ecdh_context       *pEcdhContext,
                    UINT8                       *priKey,
                    UINT16                       priKLen,
                    UINT8                       *pubKey,
                    UINT16                       pubLen);


static UINT8 EC_ecc256_ecdsa_sign_internal(
                                          mbedtls_ecdsa_context   *ecdsa_context,
                                          UINT8                   *priKey,
                                          UINT16                   priKLen,
                                          UINT8                   *dgst,
                                          UINT16                   dgstLen,
                                          UINT8                   *sig,
                                          UINT16                   sigLen,
                                          UINT8                    needHash);

static INT16 EC_ecc256_verify_internal(
                            mbedtls_ecdsa_context  *ecdsainfo,
                            UINT8                  *hash,
                            UINT16                  hashlen,
                            UINT8                  *signData,
                            UINT16                  signLen,
                            UINT8                  *pubKey,
                            UINT16                  pubLen);

UINT8 EC_softsim_flash_Read(UINT16 nameID, UINT16 kvmaxLen, UINT32 *kOutLen, UINT8 *storebuff,UINT8 sec_store);


extern INT32 RngGenRandom(uint8_t Rand[24]);

 /**
  * function :    EC_ecc256_rand
  * Description:  create a internal rand for internal used
  * input:
  *            rng_state : is NULL
  *            len  : needed output data length
  *output
               output :  output data
  * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL

*/

static int EC_ecc256_rand( void *rng_state, unsigned char *output, size_t len )
{
    size_t use_len;
    INT32  rnd;

    if( rng_state != NULL )
        rng_state  = NULL;

    while( len > 0 )
    {
        use_len = len;
        if( use_len > sizeof(int) )
            use_len = sizeof(int);
        rnd = rand();
        memcpy( output, &rnd, use_len );
        output += use_len;
        len -= use_len;
    }

    return( 0 );
}


 /**
  * function :    EC_ecc256_create_ecdsa_key_pair
  * Description:  used the Private key and public key to create a pair of ECDH key group
  * input:
  *            pEcdhContext : ecdh context pointer
  *            priKey : the 32 BYTEs private  key
  *            priKLen :private  key length
  *            pubKey : the 65 BYTEs public  key
  *            priKLen :pubLen  key length
  * output:    NULL
  * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL

*/

UINT8 EC_ecc256_create_ecdsa_key_pair(
                    mbedtls_ecdh_context        *pEcdhContext,
                    UINT8                       *priKey,
                    UINT16                       priKLen,
                    UINT8                       *pubKey,
                    UINT16                       pubLen)
{
    INT32                     ret      = -1;
    size_t                    olen     = 0;
    mbedtls_mpi               priK     = {0};   //output private key
    mbedtls_ecp_point         pubK     = {0};   //output public key
    mbedtls_ecp_group         group;

    mbedtls_ctr_drbg_context   *ctr_drbg = EC_sec_get_seed_context();
    if(!pEcdhContext )
    {
        return  (ECC_RESULTE_FAIL);
    }

    if(!priKey || !pubKey)
    {
        return  (ECC_RESULTE_FAIL);
    }

    mbedtls_mpi_init(&priK);
    mbedtls_ecp_point_init(&pubK);
    mbedtls_ecp_group_init(&group);

    if(mbedtls_ecp_group_load(&group,pEcdhContext->grp.id) != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_create_ecdsa_key_pair_0, P_INFO, 1, "mbedtls_ecp_gen_keypair,groupid:%d INVALID!!!",pEcdhContext->grp.id );
        mbedtls_mpi_free(&priK);
        mbedtls_ecp_point_free(&pubK);
        mbedtls_ecp_group_free(&group);
        return (ECC_RESULTE_FAIL);
    }

    /*step 2. generate the pair of  private and public key */
    if((ret = mbedtls_ecdh_gen_public( &group, &priK, &pubK, EC_ecc256_rand,ctr_drbg ) ) != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_create_ecdsa_key_pair_1, P_INFO, 1, "mbedtls_ecp_gen_keypair,ret :%d",ret );
        mbedtls_mpi_free(&priK);
        mbedtls_ecp_point_free(&pubK);
        mbedtls_ecp_group_free(&group);
        return (ECC_RESULTE_FAIL);
    }

    if( mbedtls_ecp_is_zero( &pubK))
    {
        ret = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        mbedtls_mpi_free(&priK);
        mbedtls_ecp_point_free(&pubK);
        mbedtls_ecp_group_free(&group);
        return (ret);
    }

    /*the output publickey */
    ret = mbedtls_ecp_point_write_binary(&group, &pubK,
                                  MBEDTLS_ECP_PF_UNCOMPRESSED, &olen, pubKey, pubLen);
    if(ret != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_create_ecdsa_key_pair_12, P_INFO, 1, "ret :%d",ret );
        mbedtls_mpi_free(&priK);
        mbedtls_ecp_point_free(&pubK);
        mbedtls_ecp_group_free(&group);
        return (ret);
    }

    ret = mbedtls_mpi_write_binary( &priK, priKey, priKLen );
    if( ret != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_create_ecdsa_key_pair_2, P_INFO, 1, "mbedtls_mpi_write_binary,ret :%d",ret );
        mbedtls_mpi_free(&priK);
        mbedtls_ecp_point_free(&pubK);
        mbedtls_ecp_group_free(&group);
        return (ret);
    }

    mbedtls_mpi_free(&priK);
    mbedtls_ecp_point_free(&pubK);
    mbedtls_ecp_group_free(&group);

    return (ECC_RESULTE_SUCC);
}


/**
  * function :    EC_ecc256_find_curve
  * Description:  find a register ECDH curve infor by curve name
  * return:      succ, ECDH curve infor ; fail  NULL
*/

mbedtls_ecp_curve_info *EC_ecc256_find_curve(const char *ec_name)
{
    const mbedtls_ecp_curve_info *curve_info = NULL;

    for( curve_info = mbedtls_ecp_curve_list();
         curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
         curve_info++ )
    {
        if( strncasecmp( curve_info->name, ec_name, strlen(curve_info->name)) == 0 )
        {
            return ((mbedtls_ecp_curve_info *)curve_info );
        }
    }

    ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_find_curve_1, P_INFO,0, "NO FOUND!!!" );
    return (NULL);

}


/**
  * function :    EC_ecc256_ecdsa_sign_internal
  * Description:  Create a signature data with SK input
  * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
*/

static UINT8 EC_ecc256_ecdsa_sign_internal(
                                          mbedtls_ecdsa_context   *pecdsa_context,
                                          UINT8                   *priKey,
                                          UINT16                   priKLen,
                                          UINT8                   *dgst,
                                          UINT16                   dgstLen,
                                          UINT8                   *sig,
                                          UINT16                   sigLen,
                                          UINT8                    needHash)
{
    mbedtls_mpi   priK;
    INT32         ret      =  -1;
    mbedtls_mpi   r, s;
    mbedtls_ctr_drbg_context   *ctr_drbg = EC_sec_get_seed_context();
    mbedtls_ecp_group           group;
    if(pecdsa_context == NULL)
    {
        return (ECC_RESULTE_FAIL);
    }

    mbedtls_ecp_group_init(&group);
    if(mbedtls_ecp_group_load(&group,pecdsa_context->grp.id) != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_ecdsa_sign_internal_0, P_INFO, 0, "group_load FAIL!!!" );
        mbedtls_ecp_group_free(&group);
        return (ECC_RESULTE_FAIL);
    }

    mbedtls_mpi_init(&priK);
    mbedtls_mpi_read_binary(&priK,priKey,priKLen);

    /*first check this private whether belongs to a exist ECDH curve*/
    if( ( ret = mbedtls_ecp_check_privkey(&group, &priK) ) != 0 )
    {
        mbedtls_mpi_free( &priK);
        mbedtls_ecp_group_free(&group);
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_ecdsa_sign_internal_1, P_INFO, 0, "check_privkey FAIL!!!" );
        return( ECC_RESULTE_FAIL );
    }

    mbedtls_mpi_init( &r );
    mbedtls_mpi_init( &s );
    ret = EC_mbedtls_ecdsa_sign( &group, &r, &s, &priK,
                             dgst, dgstLen, EC_ecc256_rand, (void*)ctr_drbg);
    if(ret != 0)
    {
        mbedtls_mpi_free( &r);
        mbedtls_mpi_free( &s);
        mbedtls_ecp_group_free(&group);
        mbedtls_mpi_free( &priK);
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_ecdsa_sign_internal_2, P_INFO,1, "ret :%d  FAIL!!!",ret );
        return (ECC_RESULTE_FAIL);
    }

    ret = mbedtls_mpi_write_binary( &r, sig, 32 );
    if( ret != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_ecdsa_sign_internal_33, P_INFO, 1, "mbedtls_mpi_write_binary,ret :%d",ret );
        mbedtls_mpi_free( &r);
        mbedtls_mpi_free( &s);
        mbedtls_ecp_group_free(&group);
        mbedtls_mpi_free( &priK);

        return (ret);
    }

    ret = mbedtls_mpi_write_binary( &s, sig + 32, sigLen - 32 );
    if( ret != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_ecdsa_sign_internal_44, P_INFO, 1, "mbedtls_mpi_write_binary,ret :%d",ret );
        mbedtls_mpi_free( &r);
        mbedtls_mpi_free( &s);
        mbedtls_ecp_group_free(&group);
        mbedtls_mpi_free( &priK);
        return (ret);
    }

    mbedtls_mpi_free( &r);
    mbedtls_mpi_free( &s);
    mbedtls_ecp_group_free(&group);
    mbedtls_mpi_free( &priK);

    ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_ecdsa_sign_internal_4, P_INFO,0, "SUCC,return" );

    return  ECC_RESULTE_SUCC;


}


/**
  * function :    EC_ecc256_verify_internal
  * Description:  verify a signature with PK
  * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
*/

static INT16 EC_ecc256_verify_internal(
                            mbedtls_ecdsa_context  *ecdsainfo,
                            UINT8                  *hash,
                            UINT16                  hashlen,
                            UINT8                  *signData,
                            UINT16                  signLen,
                            UINT8                  *pubKey,
                            UINT16                  pubLen)
{
    INT16               ret = -1;
    mbedtls_ecp_point   pubPoint;

    mbedtls_mpi r, s;

    mbedtls_mpi_init( &r );
    mbedtls_mpi_init( &s );
    mbedtls_ecp_point_init(&pubPoint);

    mbedtls_ecp_point_read_binary(&ecdsainfo->grp,&pubPoint,pubKey,(size_t)pubLen);

    /*check this pubpoint whether is a valid point of curve */
    ret = mbedtls_ecp_check_pubkey( &ecdsainfo->grp, &pubPoint);
    if(ret != 0)
    {
        mbedtls_mpi_free( &r );
        mbedtls_mpi_free( &s );
        mbedtls_ecp_point_free(&pubPoint);
        ECOMM_TRACE(UNILOG_SOFTSIM, mbedtls_ecdsa_verify_internal_11, P_INFO,1, "mbedtls_ecp_check_pubkey,ret :%d",ret );
        return (ret);
    }

    if(mbedtls_mpi_read_binary( &r, signData, 32 ) != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, mbedtls_ecdsa_verify_internal_22, P_INFO,0, "read R FAIL!" );
        return (ret);
    }

    if(mbedtls_mpi_read_binary( &s, signData + 32, 32 ) != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, mbedtls_ecdsa_verify_internal_33, P_INFO,0, "read S FAIL!" );
        return (ret);
    }

    if(( ret = mbedtls_ecdsa_verify( &ecdsainfo->grp, hash, hashlen,
                              &pubPoint, &r, &s)) != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, mbedtls_ecdsa_verify_internal_44, P_INFO,1, "ret:%d FAIL!",ret );
        return (ret);
    }

    return  (ret);

}

  /**
  * function :    EC_softsim_gen_random
  * Description: create a random with current time value
  * input:  length: the defined length of required random
  * output: random
  * return: succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
*/
UINT8 EC_softsim_gen_random(UINT16 length, UINT8 *random)
{
    UINT8       *p        = random;
    UINT16      offlen    = length;
    UINT8       offset    = 0;
    UINT32      t         = 0;
    UINT32      a=0;
    UINT32      b=0;
    UINT8       i=0;
    UINT8       Rand[32];

    if(!p)
    {
        return (ECC_RESULTE_FAIL);
    }

    t    =  (UINT32)osKernelGetTickCount();
    *p++ =  (UINT8)( t >> 24 );
    *p++ =  (UINT8)( t >> 16 );
    *p++ =  (UINT8)( t >>  8 );
    *p++ =  (UINT8)(t);

    offlen -= 4;

    while (offlen > 0) {
        RngGenRandom(Rand);
        for(i=0; i< 32; i++){
            a += Rand[i];
            b ^= Rand[i];
        }
        *(p + offset) = ( a << 8) + b  ;
        offset++;
        offlen--;
    }
    return (ECC_RESULTE_SUCC);

}

 /**
  * function :    EC_softsim_aes_ctrl
  * Description:   AES 128bits encryption or decryption with
  *key_in[input]:     the 128bits AES key
  *data_in[input]：    the original data
  *length[input]：     the original data length
  *iv_in[input]：      the CBC encryption IV value
  *data_out[output]： the output data
  *mode[input]：       AES mode 1. CBC mode;0:ECB mode
  *encryption[input]：1: encryption ;0: decryption
  * return:   succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAILs
*/

UINT8 EC_softsim_aes_ctrl
                  (   UINT8          *key_in,
                      UINT8          *data_in,
                      UINT32          length,
                      UINT8          *iv_in,
                      UINT8          *data_out,
                      UINT8           mode,
                      UINT8           encryption,
                      UINT8           keybits)
{
    mbedtls_aes_context aes_context;

    EC_mbedtls_aes_init(&aes_context );

    /*if used the HW encrtption,maybe this is unused*/
    if(encryption == MBEDTLS_AES_DECRYPT)
    {
        EC_mbedtls_aes_setkey_dec(&aes_context, key_in,keybits);
    }
    else
    {
        EC_mbedtls_aes_setkey_enc(&aes_context, key_in,keybits);
    }
    /*end*/

    if(ECC_SEC_AES_CBC_MODE == mode)
    {
        if((EC_mbedtls_aes_crypt_cbc(&aes_context,encryption,length,iv_in,data_in,data_out)) == 0)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, EC_softsim_aes_ctrl_0, P_INFO,1, "EC_AES_ctrl,aes cbc[encryp:%d] succ" ,encryption);
            EC_mbedtls_aes_free(&aes_context);
            return (ECC_RESULTE_SUCC);
        }

    }else if(ECC_SEC_AES_EBC_MODE  ==  mode)
    {
        if((EC_mbedtls_aes_crypt_ecb( &aes_context, encryption, data_in, data_out)) != 0)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, EC_softsim_aes_ctrl_1, P_INFO,1, "EC_AES_ctrl,aes ebc[encryp:%d] succ" ,encryption);
            EC_mbedtls_aes_free( &aes_context );
            return (ECC_RESULTE_SUCC);
        }
    }
    EC_mbedtls_aes_free( &aes_context );
    return (ECC_RESULTE_FAIL);
}


  /**
   * function :    EC_softsim_sha256
   * Description:  create a 32 BYTE md with sha256 hash
   * input:
   *             content : the original data
   *             content_len  the original data  len
   * output:   32 BYTES md value
   * return:   succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL

 */

 UINT8 EC_softsim_sha256(const UINT8 *content, UINT16 content_len, UINT8 *output, UINT16 mdlen)
{
    mbedtls_md_context_t ctx      = {0};
    const mbedtls_md_info_t *info = NULL ;

    EC_mbedtls_md_init( &ctx );
    info = EC_mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    EC_mbedtls_md_setup(&ctx,info,0);

    EC_mbedtls_md_starts( &ctx );
    EC_mbedtls_md_update( &ctx, content, content_len);
    EC_mbedtls_md_finish( &ctx, output);
    EC_mbedtls_md_free( &ctx );

    return  (ECC_RESULTE_SUCC);

}


  /**
   * function :    EC_softsim_reg_curve
   * Description:  register a ECDH curve with curve name
   * input:
   *            ec_name : the input curve name
   * output:    NULL
   * return:   succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 */

 UINT8  EC_softsim_reg_curve(const char *ec_name)
{
    mbedtls_ecp_group_id       group_id    = MBEDTLS_ECP_DP_NONE;
    mbedtls_ecp_curve_info     *curve_info = NULL;
    EC_ESIM_SEC_CONTEXT_INFO   *EC_ecc_context = EC_sec_get_instance();

    curve_info = EC_ecc256_find_curve(ec_name);
    if(!curve_info || curve_info->grp_id ==  MBEDTLS_ECP_DP_NONE)
    {
        ECOMM_STRING(UNILOG_SOFTSIM, EC_softsim_register_curve_0, P_INFO, "EC_ecc256_register_curve,Name:%s,FAIL!" ,(const UINT8*)ec_name);
        return (ECC_RESULTE_FAIL);
    }

    group_id = curve_info->grp_id;
    mbedtls_ecp_group_load( &EC_ecc_context->ecdsa_info.ecdsa_context.grp, group_id);
    /*ecdh cipher exchange current we not used it */
    mbedtls_ecp_group_load( &EC_ecc_context->ecdh_info.ecdh_context.grp, group_id);
    strncpy(EC_ecc_context->conf.curve_name ,(char*)ec_name,sizeof(EC_ecc_context->conf.curve_name));

    return (ECC_RESULTE_SUCC);

}

  /**
   * function :    EC_softsim_create_key_pair
   * Description:  used the Private key and public key to create a pair of ECDH key group
   * input:
   *            sk : the 32 BYTE private key
   *            pk : the 32 BYTE public  key
   * output:    NULL
   * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL

 */

 UINT8 EC_softsim_create_key_pair(UINT8 *sk, UINT16 skLen, UINT8 *pk, UINT16 pkLen)
{
    EC_ESIM_SEC_CONTEXT_INFO   *EC_ecc_context = EC_sec_get_instance();

    if(EC_ecc_context->conf.curve_name[0] == 0)
    {
        return (ECC_RESULTE_FAIL);
    }
    if(EC_ecc256_find_curve(EC_ecc_context->conf.curve_name) == NULL)
    {
        return (ECC_RESULTE_FAIL);
    }

    return   EC_ecc256_create_ecdsa_key_pair(&EC_ecc_context->ecdh_info.ecdh_context,sk,skLen,pk,pkLen);
}


 /**
   * function :    EC_softsim_is_on_curve
   * Description:  write the softsim configurations from flash with/or not with a security method
   * input:
   *            pk :         the public key of ECDH curve
   * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 */

 UINT8 EC_softsim_is_on_curve(UINT8 *pk, UINT16 pkLen)
{
    INT8 ret = -1;
    mbedtls_ecp_point           verifyQpoint;
    EC_ESIM_SEC_CONTEXT_INFO   *EC_ecc_context = EC_sec_get_instance();

    if(pk   == NULL ||
      pkLen == 0)
    {
        return ret;
    }

    mbedtls_ecp_point_init( &verifyQpoint);
    if((ret = mbedtls_ecp_point_read_binary(&EC_ecc_context->ecdh_info.ecdh_context.grp, &verifyQpoint,
                    (const UINT8 *)pk, pkLen)) == 0)
    {
        ret = mbedtls_ecp_check_pubkey( &EC_ecc_context->ecdh_info.ecdh_context.grp, &verifyQpoint);
        if(ret != 0)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_is_on_curve_00, P_INFO,1, "check_pubkey,ret:%d,ERROR!" ,ret);
            mbedtls_ecp_point_free(&verifyQpoint);
            return (1);
        }
    }
    mbedtls_ecp_point_free(&verifyQpoint);
    return( ret );
}

 /**
   * function :    EC_softsim_ecdh
   * Description:  Create a share key with oneself private key and peer PUB key
   * input:
   *        sk:  private key
   *        pk:  public key
   *
   * output:
   *            sab : the  share key
   * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL

 */

 UINT8 EC_softsim_ecdh(UINT8 *sk, UINT16 skLen, UINT8 *pk, UINT16 pkLen, UINT8 *sab,UINT16 sabLen)
{
    int ret = -1;
    mbedtls_mpi                prikeyP;
    mbedtls_ecp_point          pubKeyP;
    mbedtls_ecp_group          group;
    mbedtls_mpi                shareK;
    mbedtls_ecdh_context      *pEcdh_context  = NULL;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    pEcdh_context  =  &EC_ecc_context->ecdh_info.ecdh_context;

    mbedtls_mpi_init(&prikeyP);
    mbedtls_ecp_group_init( &group);
    mbedtls_ecp_point_init( &pubKeyP);

    if(mbedtls_mpi_read_binary(&prikeyP,sk,skLen) < 0)
    {
        mbedtls_ecp_point_free( &pubKeyP);
        mbedtls_mpi_free(&prikeyP);
        mbedtls_ecp_group_free(&group);
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_softsim_ecdh_0, P_INFO, 0, "read_binary INVALID!!!");
        return (ECC_RESULTE_FAIL);
    }

    if(mbedtls_ecp_group_load(&group, pEcdh_context->grp.id) != 0)
    {
        mbedtls_ecp_point_free( &pubKeyP);
        mbedtls_mpi_free(&prikeyP);
        mbedtls_ecp_group_free(&group);
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_softsim_ecdh_1, P_INFO, 1, "EC_ecc256_ecdh,groupid:%d INVALID!!!",pEcdh_context->grp.id );
        return (ECC_RESULTE_FAIL);
    }

    if((ret = mbedtls_ecp_point_read_binary( &group, &pubKeyP,
                    (const UINT8 *)pk, pkLen)) != 0)
    {
        mbedtls_ecp_point_free(&pubKeyP);
        mbedtls_mpi_free(&prikeyP);
        mbedtls_ecp_group_free(&group);
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_softsim_ecdh_2, P_INFO, 1, "read prikeyP ret:%d ",ret );
        return (ECC_RESULTE_FAIL);
    }

    mbedtls_mpi_init(&shareK);
    if((ret = EC_mbedtls_ecdh_compute_shared(&group,
                                       &shareK,     /*the share key*/
                                       &pubKeyP,
                                       &prikeyP,
                                       EC_ecc256_rand, &EC_ecc_context->ctrDrbgContext)) < 0)
    {
         mbedtls_ecp_point_free( &pubKeyP);
         mbedtls_mpi_free(&prikeyP);
         mbedtls_ecp_group_free(&group);
         mbedtls_mpi_free(&shareK);
         ECOMM_TRACE(UNILOG_SOFTSIM, EC_softsim_ecdh_3, P_INFO, 1, "compute_shared ret:%d ",ret );
         return (ECC_RESULTE_FAIL);
    }

    mbedtls_mpi_write_binary( &shareK, sab, sabLen );
    mbedtls_ecp_point_free( &pubKeyP);
    mbedtls_mpi_free(&prikeyP);
    mbedtls_ecp_group_free(&group);
    mbedtls_mpi_free(&shareK);

    return (ECC_RESULTE_SUCC);

}

 /**
   * function :    EC_softsim_ecdsa_sign
   * Description:  Create a signature data with SK input
   * input:
   *          dgst :    the original data need to be signatured
   *          dgstLen : the original data length
   *          sk:     : the private key
   *
   * output:
   *            sign : the sign data
   * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 */

 UINT8 EC_softsim_ecdsa_sign(UINT8 *dgst, UINT16 dgstLen, UINT8 *sk, UINT16 skLen, UINT8 *sign, UINT16 signLen)
{
    EC_ESIM_SEC_CONTEXT_INFO   *EC_ecc_context = EC_sec_get_instance();
    return EC_ecc256_ecdsa_sign_internal(&EC_ecc_context->ecdsa_info.ecdsa_context,sk,skLen,dgst,dgstLen,sign,signLen,ECC_SIGN_CSR_NO_NEED_HASH);
}


 /**
   * function :    EC_softsim_ecdsa_verify
   * Description:  verify a signature with PK
   * input:
   *        dgst  : the original data
   *        dgst_len  : the original data length
   *        sign     : the signature data
   *        pk:     the peer public key
   * output:
   *
   * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL

 */

 UINT8 EC_softsim_ecdsa_verify(UINT8 *dgst, UINT16 dgst_len, UINT8 *sign,UINT16 signLen,UINT8 *pk,UINT16 pkLen )
{
    INT32 ret = -1;
    EC_ESIM_SEC_CONTEXT_INFO   *EC_ecc_context = EC_sec_get_instance();

    ret = EC_ecc256_verify_internal(&EC_ecc_context->ecdsa_info.ecdsa_context,dgst,dgst_len,sign,signLen,pk,pkLen);
    return ((ret == 0) ? ECC_RESULTE_SUCC : ECC_RESULTE_FAIL);
}

#define PEM_BEGIN_CSR           "-----BEGIN CERTIFICATE REQUEST-----\n"
#define PEM_END_CSR             "-----END CERTIFICATE REQUEST-----\n"
#define  REQ_BUF_SIZE   4096


/**
  * function :    EC_mbedtls_x509write_csr_der
  * Description:  create X509 csr request data
  * input:
  *            crt :   input X509 csr request
  *            f_rng   : random function
  *            p_rng   : seeds  function
  * output:    pbuf:      the CSR  data
  *            iolen  :  the CSR len
  * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
*/

INT16 EC_mbedtls_x509write_csr_der(mbedtls_x509write_csr *ctx, UINT8 *buf, UINT8 *tmp_buf, UINT8 *sig, size_t size,
                       int (*f_rng)(void *, unsigned char *, size_t),void *p_rng)
{
    INT16       ret;
    const CHAR *sig_oid;
    size_t      sig_oid_len = 0;
    UINT8       *c, *c2;
    UINT8       hash[64];
    size_t      pub_len = 0, sig_and_oid_len = 0, sig_len;
    size_t      len = 0;
    mbedtls_pk_type_t pk_alg;
#define TEMP_BUF_SIZE  4096
#define SIG_BUF_SIZE   2048
    /*
     * Prepare data to be signed in tmp_buf
     */

    c = tmp_buf + TEMP_BUF_SIZE;

    MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_x509_write_extensions( &c, tmp_buf, ctx->extensions));
    if( len )
    {
        MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_asn1_write_len( &c, tmp_buf, len ) );
        MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_asn1_write_tag( &c, tmp_buf, MBEDTLS_ASN1_CONSTRUCTED |
                                                        MBEDTLS_ASN1_SEQUENCE ) );

        MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_asn1_write_len( &c, tmp_buf, len ) );
        MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_asn1_write_tag( &c, tmp_buf, MBEDTLS_ASN1_CONSTRUCTED |
                                                        MBEDTLS_ASN1_SET ) );

        MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_asn1_write_oid( &c, tmp_buf, MBEDTLS_OID_PKCS9_CSR_EXT_REQ,
                                          MBEDTLS_OID_SIZE( MBEDTLS_OID_PKCS9_CSR_EXT_REQ ) ) );
        MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_asn1_write_len( &c, tmp_buf, len ) );
        MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_asn1_write_tag( &c, tmp_buf, MBEDTLS_ASN1_CONSTRUCTED |
                                                        MBEDTLS_ASN1_SEQUENCE ) );
    }


    MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_asn1_write_len( &c, tmp_buf, len ) );
    MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_asn1_write_tag( &c, tmp_buf, MBEDTLS_ASN1_CONSTRUCTED |
                                                    MBEDTLS_ASN1_CONTEXT_SPECIFIC ) );

    MBEDTLS_ASN1_CHK_ADD( pub_len, EC_mbedtls_pk_write_pubkey_der(ctx->key,
                                                tmp_buf, c - tmp_buf) );
    c -= pub_len;
    len += pub_len;

    /*
     *  Subject  ::=  Name
     */
    MBEDTLS_ASN1_CHK_ADD( len, mbedtls_x509_write_names( &c, tmp_buf, ctx->subject ) );

    /*
     *  Version  ::=  INTEGER  {  v1(0), v2(1), v3(2)  }
     */
    MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_asn1_write_int( &c, tmp_buf, 0 ) );

    MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_asn1_write_len( &c, tmp_buf, len ) );
    MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_asn1_write_tag( &c, tmp_buf, MBEDTLS_ASN1_CONSTRUCTED |
                                                    MBEDTLS_ASN1_SEQUENCE ) );

    /*
     * Prepare signature
     */

    EC_mbedtls_md( EC_mbedtls_md_info_from_type( ctx->md_alg ), c, len, hash );

    pk_alg = EC_mbedtls_pk_get_type(ctx->key);
    if( pk_alg == MBEDTLS_PK_ECKEY )
        pk_alg = MBEDTLS_PK_ECDSA;


    if( ( ret = EC_mbedtls_pk_sign(ctx->key, ctx->md_alg, hash, 0, sig, &sig_len,
                         f_rng, p_rng ) ) != 0)
    {

        ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509write_csr_der_16, P_INFO,1,
                "fail ret:%d",ret);
        return  (ret);
    }

    if( (ret = EC_mbedtls_oid_get_oid_by_sig_alg( pk_alg, ctx->md_alg,
                                        &sig_oid, &sig_oid_len ) ) != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509write_csr_der_18, P_INFO,1,
                    "fail ret:%d",ret);
        return( ret );
    }

    /*
     * Write data to output buffer
     */
    c2 = buf + 4096;
    MBEDTLS_ASN1_CHK_ADD( sig_and_oid_len, EC_mbedtls_x509_write_sig( &c2, buf,
                                        sig_oid, sig_oid_len, sig, sig_len ) );


    if( len > (size_t)( c2 - buf ) )
    {
        return( MBEDTLS_ERR_ASN1_BUF_TOO_SMALL );
    }

    c2 -= len;
    memcpy( c2, c, len );

    len += sig_and_oid_len;
    MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_asn1_write_len( &c2, buf, len));
    MBEDTLS_ASN1_CHK_ADD( len, EC_mbedtls_asn1_write_tag( &c2, buf, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

    return( (int) len );
}



/**
  * function :    EC_mbedtls_x509write_csr_pem
  * Description:  create X509 csr request data
  * input:
  *            crt :   input X509 csr request
  *            f_rng   : random function
  *            p_rng   : seeds  function
  * output:    pbuf:      the CSR  data
  *            iolen  :  the CSR len
  * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
*/

int EC_mbedtls_x509write_csr_pem(mbedtls_x509write_csr *ctx, UINT8 *OutBuf, UINT16 *outBufLen,
                       int (*f_rng)(void *, UINT8 *, size_t),
                       void *p_rng)
{
    INT16            ret;
    size_t           outlen  = 0;
    UINT8           *tmp_buf = NULL;
    UINT8           *pReqBuf = NULL;
    UINT8           *sig     = NULL;

    pReqBuf = malloc(REQ_BUF_SIZE);
    if(pReqBuf == NULL)
    {
         ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509write_csr_pem_0, P_INFO,0, "malloc fail!!!");
         return (-1);
    }
    memset(pReqBuf,0x0,REQ_BUF_SIZE);

    tmp_buf = malloc(TEMP_BUF_SIZE);
    if(tmp_buf == NULL)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509write_csr_der_0, P_INFO,0,
                    "tmp_buf malloc 2048 size failed!");
        free(pReqBuf);
        return  (-1);
    }
    memset(tmp_buf,0x0,TEMP_BUF_SIZE);

    sig = malloc(SIG_BUF_SIZE);
    if(sig == NULL)
    {
        free(pReqBuf);
        free(tmp_buf);
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509write_csr_der_1, P_INFO,0,
                    "sig malloc 1024 size failed!");
        return  (-1);
    }
    memset(sig,0x0,SIG_BUF_SIZE);

    if( ( ret = EC_mbedtls_x509write_csr_der(ctx, pReqBuf,tmp_buf,sig, REQ_BUF_SIZE,f_rng, p_rng) ) < 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509write_csr_pem_1, P_INFO,1,
                    "return ret:%d, fail!!!",ret);
        free(pReqBuf);
        free(tmp_buf);
        free(sig);
        return  (-1);
    }

    free(tmp_buf);
    free(sig);

    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509write_csr_pem_2, P_INFO,1,
                "CSR LEN：%d",ret);

    if( ( ret = EC_mbedtls_pem_write_buffer( PEM_BEGIN_CSR, PEM_END_CSR,
                                 (const UINT8 *)(pReqBuf + REQ_BUF_SIZE - ret),
                                  ret, OutBuf, REQ_BUF_SIZE, (size_t *)&outlen ) ) != 0 )
    {

        ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509write_csr_pem_3, P_INFO,1, "return ret:%d, fail!!!",ret);
        free(pReqBuf);
        return (-1);
    }

    free(pReqBuf);
    if(outBufLen) *outBufLen = outlen;

    ECOMM_TRACE(UNILOG_SOFTSIM, EC_mbedtls_x509write_csr_pem_4, P_INFO,1,"finally succ,len:%d",outlen);

    return( 0 );
}


/**
  * function :    EC_ecc256_write_certificate
  * Description:  create X509 csr request data
  * input:
  *            crt :   input X509 csr request
  *            f_rng   : random function
  *            p_rng   : seeds  function
  * output:    pbuf:      the CSR  data
  *            iolen  :  the CSR len
  * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
*/

INT8 EC_ecc256_write_certificate( mbedtls_x509write_csr *crt, UINT8 *pOutBuf, UINT16 *OutLen,
                       int (*f_rng)(void *, UINT8*, size_t),void *p_rng )
{
    if(EC_mbedtls_x509write_csr_pem(crt, pOutBuf, OutLen , f_rng, p_rng) < 0 )
    {
        return( ECC_RESULTE_FAIL );
    }

    return( ECC_RESULTE_SUCC);
}

 /**
   * function :    EC_ecc256_write_certificate_request
   * Description:  create X509 csr request data
   * input:
   *            x509_csr :   input X509 csr request
   * output:    pbuf:      the CSR  data
   *            iolen  :  the CSR len
   * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 */

 INT8 EC_ecc256_write_certificate_request(mbedtls_x509write_csr *x509_csr,UINT8 *pbuf, UINT16 *outlen)
{
    INT32 ret = -1;
    mbedtls_ctr_drbg_context   *ctr_drbg       = EC_sec_get_seed_context();
    ret =  EC_ecc256_write_certificate(x509_csr,pbuf,outlen,EC_ecc256_rand,(void*)ctr_drbg);
    return ((ret == 0) ? ECC_RESULTE_SUCC : ECC_RESULTE_FAIL);


}


 /**
   * function :    EC_softsim_create_csr
   * Description:  create X509 csr data
   * input:
   *            sk :     the private key
   *            pk   :   the public  key
   *            subject : the CSR subject
   * output:    outbuf:  the CSR  data
   *            iolen  :  the CSR len
   * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 */

 UINT8 EC_softsim_create_csr(UINT8 *sk,UINT16 skLen, UINT8 *pk, UINT16 pkLen,UINT8 *subject,UINT8 *outbuf, UINT16 *iolen)
{

    INT16                       ret      = -1;
    UINT8                      *pOutBuf  = NULL;
    mbedtls_pk_context          pemKey   = {0};
    mbedtls_ecp_keypair        *pkeyPair = NULL;
    const mbedtls_pk_info_t    *pk_info  = NULL;

    mbedtls_x509write_csr       x509write_Req;
    mbedtls_ctr_drbg_context   *ctr_drbg       = EC_sec_get_seed_context();
    EC_ESIM_SEC_CONTEXT_INFO   *EC_ecc_context = EC_sec_get_instance();

    if(EC_ecc_context == NULL)
    {
        return  1;
    }

    mbedtls_ecdsa_context     *pEcdsaContext = &EC_ecc_context->ecdsa_info.ecdsa_context;
    EC_mbedtls_x509write_csr_init( &x509write_Req );
    EC_mbedtls_x509write_csr_set_md_alg( &x509write_Req, MBEDTLS_MD_SHA256 );

    if( ( ret = EC_mbedtls_x509write_csr_set_subject_name( &x509write_Req, (const CHAR*)subject)) != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_create_csr_1, P_INFO,1, "mbedtls_x509write_csr_set_subject_name ret:%d" ,ret);
        EC_mbedtls_x509write_csr_free( &x509write_Req );
        return (1);
    }

    memset(&pemKey,0x0,sizeof(mbedtls_pk_context));

    pk_info = EC_mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY);
    if(pk_info == NULL)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_create_csr_12, P_INFO,0, "pk_info NULL");
        EC_mbedtls_x509write_csr_free( &x509write_Req );
        return (1);
    }

    EC_mbedtls_pk_init(&pemKey);
    if( ( ret = EC_mbedtls_pk_setup(&pemKey, pk_info)) != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_create_csr_2, P_INFO,1, "EC_mbedtls_pk_setup ret:%d,FAIL!!!" ,ret);
        EC_mbedtls_x509write_csr_free( &x509write_Req );
        return (1);
    }

    pkeyPair =  (mbedtls_ecp_keypair *)EC_mbedtls_pk_ec(pemKey);
    if( ( ret = mbedtls_ecp_group_load(&pkeyPair->grp, pEcdsaContext->grp.id ) ) != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_create_csr_31, P_INFO,1, "mbedtls_ecp_group_load ret:%d,FAIL!!!" ,ret);
        EC_mbedtls_x509write_csr_free( &x509write_Req );
        return (1);
    }

    mbedtls_ecp_point_init(&pkeyPair->Q);
    /* check this public whether belongs to a exist ECDH curve*/
    if((ret = mbedtls_ecp_point_read_binary(&pkeyPair->grp, &pkeyPair->Q,
                    (const UINT8 *)pk, pkLen)) == 0)
    {
        ret = mbedtls_ecp_check_pubkey(&pkeyPair->grp, &pkeyPair->Q);
        if(ret != 0)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_create_csr_4, P_INFO,1, "mbedtls_ecp_check_pubkey ret:%d,FAIL!!!" ,ret);
            EC_mbedtls_x509write_csr_free( &x509write_Req );
            return (1);
        }
    }

    mbedtls_mpi_init(&pkeyPair->d);
    mbedtls_mpi_read_binary(&pkeyPair->d,sk,skLen);

    /* check this private whether belongs to a exist ECDH curve*/
    if((ret = mbedtls_ecp_check_privkey(&pkeyPair->grp, &pkeyPair->d)) != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_create_csr_5, P_INFO,1, "mbedtls_ecp_check_privkey ret:%d,FAIL!!!" ,ret);
        EC_mbedtls_x509write_csr_free( &x509write_Req );
        return (1);
    }

    x509write_Req.key = &pemKey;

    pOutBuf =  malloc(4096);
    if(pOutBuf == NULL)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_create_csr_18, P_INFO,0, "malloc failed!!!");
        return (1);
    }

    memset(pOutBuf,0,4096);

    ret = EC_ecc256_write_certificate_request(&x509write_Req, pOutBuf,iolen);
    if(ret != 0)
    {
        EC_mbedtls_x509write_csr_free( &x509write_Req );
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_create_csr_3, P_INFO,1, "ret :%d,ERROR!" ,ret);
        free(pOutBuf);
        return (1);
    }

    memcpy(outbuf,pOutBuf,*iolen);
    free(pOutBuf);

    return (0);

}


 /**
   \fn          EC_softsim_gen_CMac
   \brief       generate the CMAC
   \param[in]   Key : the input key
   \param[in]   keybits : the input key  bit sizes
   \param[in]   Message : the original message
   \param[in]   MessLen : the original message  length
   \param[in]   cipher_type : cipher tye 128 BIT
   \param[out]  Cmac :   CMAC
   \returns     ECC_RESULTE_SUCC or  ECC_RESULTE_FAIL
 */

 UINT8 EC_softsim_gen_CMac(const UINT8 *Key, INT32 keybits,const UINT8 *Message, UINT32 MessLen,UINT8* Cmac,UINT8 cipher_type)
{
    UINT8    out_key[16] = {0};
    mbedtls_cipher_info_t  *pcipher_info = NULL;

    pcipher_info = (mbedtls_cipher_info_t *)mbedtls_cipher_info_from_type((mbedtls_cipher_type_t)cipher_type);
    if( pcipher_info == NULL )
    {
        return (ECC_RESULTE_FAIL);
    }

    memset(Cmac,0x0,16);

    if( EC_mbedtls_cipher_cmac(pcipher_info, Key, keybits, Message,
                                   MessLen, out_key ) == 0)
    {
        memcpy(Cmac,out_key,sizeof(out_key));
        return ( ECC_RESULTE_SUCC);
    }

    return (ECC_RESULTE_FAIL);


}

 /**
  * function :    generate  cmac and verify it by internal testing
  * Description:  generate a CMAC
  * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL

*/
UINT8 EC_softsim_verify_CMac(
                      const CHAR*            testname,
                      const UINT8*           keyMsg,
                      INT32                  keybits,
                      const UINT8*           messages,
                      const UINT32           message_lengths[4],
                      const UINT8*           expected_result,
                      UINT8                  cipher_type,
                      INT32                  block_size,
                      INT32                  num_tests )
{
    int     i = 0;
    UINT8 ret = 0;
    UINT8 output[16];
    for(i = 0; i< num_tests; i++)
    {
        memset(output,0,sizeof(output));
        if((ret = EC_softsim_gen_CMac(keyMsg,keybits,messages,message_lengths[i],output,cipher_type)) != 0)
        {
            return (1);
        }
        if( ( ret = memcmp( output, (void *)&expected_result[i * block_size], block_size ) ) != 0 )
        {
            return (1);
        }
    }

    return ret;
}



 /**
  \fn          EC_softsim_flash_Write
  \brief       write the data to corresponding flash position
  \param[in]   index : NameID
  \param[in]   dataIn : the writting data
  \param[in]   dataIn : the writting data  Length
  \param[in]   sec_store : need encryption write or not
  \returns     0: Write OK ;-1 :Write FAILED
*/
 UINT8 EC_softsim_flash_write(UINT16 nameID,const UINT8 *kvalue, UINT16 kvLen, UINT8 sec_store)
 {
    int ret;
    if(
       nameID      > 255    ||
       kvalue     == PNULL ||
       kvLen      == 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_ecc256_flash_Write_0, P_INFO, 0, "INVALID parameters");
        return (ret);
    }

    if(0 == sec_store)
    {
        ret = ecSoftSimFlashDataWrite(nameID,kvalue,kvLen,0,0);
    }
    else
    {
        ret =  ecSoftSimFlashEncryDataWrite(nameID,kvalue,kvLen);
    }
    return  (ret == 0 ? ECC_RESULTE_SUCC : ECC_RESULTE_FAIL);

 }

/**
  \fn          EC_softsim_flash_Read
  \brief       read the softsim flash data according nameID
  \param[in]   index : nameID
  \param[in]   outbuffMax : output buf max size
  \param[in]   sec_store : encryption data or not
  \param[out]  dataOut : the reading date of flash
  \param[out]  outlen :  the reading date actual Length
  \returns     offset
*/
UINT8 EC_softsim_flash_Read(UINT16 nameID, UINT16 kvmaxLen, UINT32 *kOutLen, UINT8 *storebuff,UINT8 sec_store)
{
    if(
       nameID      > 255    ||
       storebuff   == PNULL ||
       kvmaxLen    == 0)
    {
        return (ECC_RESULTE_FAIL);
    }

    if(sec_store == 0)
    {
        if(ecSoftSimFlashDataRead(nameID, kvmaxLen,storebuff,kOutLen,sec_store) != 0)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, EC_softsim_flash_Read_0, P_INFO,1,
                        "read nameID :%d fail!!!" ,nameID);
            return (ECC_RESULTE_FAIL);
        }

        if(*kOutLen == 0)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, EC_softsim_flash_Read_1, P_INFO,1,
                        "read nameID :%d len 0 INVLALID!!!" ,nameID);
            return (ECC_RESULTE_FAIL);
        }
        return  (ECC_RESULTE_SUCC);
    }

    UINT8 *pDecData = NULL;
    UINT32 readLen = 0;
    UINT16 oriLen  = 0;
    UINT16 secLen  = 0;
    UINT16 decLen  = 0;
    UINT8  EnRead[128];
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_softsim_flash_Read_2, P_INFO,1,
                "read nameID %d SEC" ,nameID);

    memset(EnRead,0x0,sizeof(EnRead));
    if(ecSoftSimFlashDataRead(nameID, sizeof(EnRead), EnRead, &readLen,sec_store) != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_softsim_flash_Read_3, P_INFO,1,
                    "read nameID :%d fail!!!" ,nameID);
        *kOutLen = 0;
        return (ECC_RESULTE_FAIL);
    }

    if(readLen == 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_softsim_flash_Read_4, P_INFO,1,
                    "read nameID :%d len is 0, INVLALID!!!" ,nameID);
        *kOutLen = 0;
        return (ECC_RESULTE_FAIL);
    }

    oriLen   = (UINT16)(readLen >> 16);
    secLen   = (UINT16)(readLen);


    ECOMM_TRACE(UNILOG_SOFTSIM, EC_softsim_flash_Read_5, P_INFO,4,
                    "EC_ecc256_flash_Read,EN[index:%d,isSec:%d,oriLen:%d,secLen:%d]" ,nameID,sec_store,oriLen,secLen);
    if(ecSoftSimFlashConstructAesData(1,EnRead,secLen,&pDecData,&decLen) != ECC_RESULTE_SUCC)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_softsim_flash_Read_6, P_INFO, 0, "construct AES data FAIL!!!");
        free(pDecData);
        return (ECC_RESULTE_FAIL);
    }

    if(oriLen > kvmaxLen )
    {
        return (ECC_RESULTE_FAIL);
    }

    memcpy(storebuff ,pDecData,oriLen);

    *kOutLen = oriLen;

    ECOMM_DUMP(UNILOG_SOFTSIM,  EC_softsim_flash_Read_7, P_INFO,  "PIAIN READ->",oriLen,storebuff);
    free(pDecData);
    return (ECC_RESULTE_SUCC);

}

UINT8 EC_verify_AES_hw(void)
{
    UINT16 modoffset    = 0;
    UINT16 remainoffset = 0;
    UINT16 encNum       = 0;
    UINT16 kvLen        = 0;
    int ret;
    UINT8  *Encdata   =  NULL;
    UINT8  iv_in[]    =  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    UINT8  SourData[] =  {0x08,0x49,0x06,0x11,0x11,0x34,0x56,0x23,0x60};

    mbedtls_aes_context aes_context = {0};
    EC_mbedtls_aes_init(&aes_context );

    ECOMM_TRACE(UNILOG_SOFTSIM, EC_verify_AES_hw_00, P_INFO,0,
                "EC_verify_AES_hw BEGIN!!!");

    kvLen = sizeof(SourData);
    modoffset = kvLen / 16 ;
    remainoffset = kvLen % 16;
    if(remainoffset)
    {
        modoffset += 1;
    }

    if(modoffset == 0)
    {
        return (ECC_RESULTE_FAIL);
    }

    ECOMM_TRACE(UNILOG_SOFTSIM, EC_verify_AES_hw_11, P_INFO,2,
                "modoffset:%d,remainoffset:%d",modoffset,remainoffset);
    encNum = modoffset * 16;
    if((Encdata = (UINT8 *)malloc(encNum)) == NULL)
    {
        return (ECC_RESULTE_FAIL);
    }

    memset(Encdata, 0x0, encNum);
    if((ret = (mbedtls_aes_crypt_cbc_hw(&aes_context, 0 ,encNum ,iv_in,SourData,Encdata))) != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_verify_AES_hw_1, P_INFO,1,
            "EC_verify_AES_hw ENC,ret:%d ,FAIL",ret);
        EC_mbedtls_aes_free(&aes_context);
        free(Encdata);
        return (ECC_RESULTE_FAIL);
    }

    ECOMM_DUMP(UNILOG_SOFTSIM,  EC_verify_AES_hw_2, P_INFO,  "ENC DATA->",encNum,Encdata);
    UINT8  iv_in_sec[]   =  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    UINT8  Decdata[32] = {0};

    memset(Decdata,0x0,32);
    if((ret = mbedtls_aes_crypt_cbc_hw(&aes_context,1,encNum,iv_in_sec,Encdata,Decdata)) != 0)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_verify_AES_hw_3, P_INFO,1,
                    "EC_verify_AES_hw DEC, fail" ,ret);
        EC_mbedtls_aes_free(&aes_context);
        free(Encdata);
        return (ECC_RESULTE_FAIL);
    }

    ECOMM_DUMP(UNILOG_SOFTSIM,  EC_verify_AES_hw_4, P_INFO,  "DEC DATA->",kvLen,Decdata);
    ECOMM_TRACE(UNILOG_SOFTSIM, EC_verify_AES_hw_44, P_INFO,0,
                "EC_verify_AES_hw END!!!");

    free(Encdata);
    return (ECC_RESULTE_SUCC);

}
/**
  \fn          EC_sec_ecdh_context_init
  \brief       init the ECDH alg  context
  \returns     void
*/
static void  EC_sec_ecdh_context_init(EC_ALG_ECDH_INFO *pEcAlgEcdhInfo)
{
    mbedtls_ecdh_init(&pEcAlgEcdhInfo->ecdh_context);
}

/**
  \fn          EC_sec_ecdsa_context_init
  \brief       init the ECDSA data signature ALG  context
  \returns     void
*/
static void EC_sec_ecdsa_context_init( EC_DSIG_ECDSA_INFO   *pEcdsainfo)
{
    mbedtls_ecdsa_init(&pEcdsainfo->ecdsa_context);
}


/**
  \fn          EC_sec_x509_context_init
  \brief       init the X509 CSR  context
  \returns     ECC_RESULTE_SUCC
*/
static UINT8 EC_sec_x509_context_init(EC_DIG_X509_CSR_REQ_INFO *pEcDigx509CsrInfo)
{
    mbedtls_x509_crt_init(&pEcDigx509CsrInfo->csr);
    return (0);
}

/**
  \fn          EC_sec_seed_context_init
  \brief       init the seeds  context
  \returns     ECC_RESULTE_SUCC or ECC_RESULTE_FAIL
*/
static UINT8 EC_sec_seed_context_init(mbedtls_entropy_context *pEntryContext,mbedtls_ctr_drbg_context *pCtrSeedContext)
{
    INT32  ret  = -1;
    const CHAR *seedStr = "EC_ecdh";
    mbedtls_ctr_drbg_init(pCtrSeedContext);
    mbedtls_entropy_init(pEntryContext);

    if((ret = mbedtls_ctr_drbg_seed(pCtrSeedContext, mbedtls_entropy_func,pEntryContext,
                                    (const UINT8 *)seedStr,strlen(seedStr) )) != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, EC_sec_seed_context_init_0, P_INFO,1, "create seed ret:%d,FAIL!!!" ,
                                                                             ret);
        mbedtls_ctr_drbg_free(pCtrSeedContext);
        mbedtls_entropy_free(pEntryContext);
        return  (ECC_RESULTE_FAIL);
    }
    return (ECC_RESULTE_SUCC);
}

/**
  \fn          EC_sec_get_seed_context
  \brief       get the seeds  context
  \returns     the seeds  context
*/
mbedtls_ctr_drbg_context*  EC_sec_get_seed_context(void)
{
   EC_ESIM_SEC_CONTEXT_INFO *pEsimSecContext =   EC_sec_get_instance();
   if(!pEsimSecContext)
   {
       return  (NULL);
   }
   return  (&pEsimSecContext->ctrDrbgContext);
}

 /**
   \fn          EC_sec_init
   \brief       init the security  context
   \returns
 */
 UINT8  EC_sec_init(EC_ESIM_SEC_CONTEXT_INFO *pSecontext)
{
    EC_ESIM_SEC_CONTEXT_INFO  *pTemp = pSecontext;
    if(!pTemp)
    {
        return (1);
    }
    memset(pTemp,0x0,sizeof(EC_ESIM_SEC_CONTEXT_INFO));
    pTemp->isInit =   0;

    return (0);

}

/**  \fn          EC_sec_context_init
  \brief       init the security  context
  \returns
*/
static  UINT8 EC_sec_context_init(EC_ESIM_SEC_CONTEXT_INFO *pSecontext)
{
    if(!pSecontext)
    {
        return  (0);
    }

    EC_sec_init(pSecontext);
    EC_sec_ecdh_context_init(&pSecontext->ecdh_info);
    EC_sec_ecdsa_context_init(&pSecontext->ecdsa_info);
    EC_sec_x509_context_init(&pSecontext->x509_csr_info);
    EC_sec_seed_context_init(&pSecontext->entropyContext,&pSecontext->ctrDrbgContext);

    return (1);
}

/**  \fn          EC_sec_method_register
  \brief       register the security method
  \returns
*/
static void EC_sec_method_register(EC_SEC_METHOD *method)
{
    if(!method)
    {
        return;
    }

    method->EC_softsim_random               =   EC_softsim_gen_random;
    method->EC_softsim_aes                  =   EC_softsim_aes_ctrl;
    method->EC_softsim_sha256               =   EC_softsim_sha256;
    method->EC_softsim_ecpregister          =   EC_softsim_reg_curve;
    method->EC_softsim_create_ecc_keypair   =   EC_softsim_create_key_pair;
    method->EC_softsim_check_pk_ison_curve  =   EC_softsim_is_on_curve;
    method->EC_softsim_ecc_ecdh             =   EC_softsim_ecdh;
    method->EC_softsim_ecdsa_sign           =   EC_softsim_ecdsa_sign;
    method->EC_softsim_generateCMac         =   EC_softsim_gen_CMac;
    method->EC_softsim_VerifyCMac           =   EC_softsim_verify_CMac;
    method->EC_softsim_create_csr      = EC_softsim_create_csr;
    method->EC_softsim_ecdsa_verify    = EC_softsim_ecdsa_verify;
    method->EC_softsim_flash_write     = EC_softsim_flash_write;
    method->EC_softsim_flash_read      = EC_softsim_flash_Read;

    return ;

}

/**  \fn          EC_sec_context_register
  \brief       register the security method and init flash
  \returns
*/
INT8 EC_sec_context_register(void)
{
    EC_sec_context_init(&gECSecContext);
    EC_sec_method_register(&gECSecContext.method);
    ecSoftSimFlashInit();

    /*for debug*/
#if 0
    TA_ec256_test();
#endif
     /*for debug end*/
    return (0);

}

/**  \fn      EC_sec_get_instance
  \brief       get the gECSecContext global instance
  \returns     the gECSecContext global instance
*/
EC_ESIM_SEC_CONTEXT_INFO  *EC_sec_get_instance(void)
{
    if(gECSecContext.isInit == 0)
    {
        EC_sec_context_register();
        gECSecContext.isInit = 1;
    }
    return (&gECSecContext);
}

#endif
