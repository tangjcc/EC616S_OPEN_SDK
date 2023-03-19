
/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - ecSecApi.h
 Description:    - support security api to vendor for softSIM functions
 History:        - 09/09/2020  created by yjwang
 ******************************************************************************
******************************************************************************/

#ifdef  SOFTSIM_CT_ENABLE

#include <stdio.h>
#include <string.h>

#include "ecSeccallback.h"
#include "ecSecApi.h"
#include "ecSecFlashApi.h"

#include "debug_trace.h"
#include "debug_log.h"

#if defined CHIP_EC616
#include "unilog_ec616.h"
#elif defined CHIP_EC616S
#include "unilog_ec616s.h"
#endif

static UINT8  gRegister = 0;

/******************************************************************************
 * function:    TA_get_random
 * Description: create a random with current time value
 * input:  length: the defined length of required random
 * output: random
 * return: succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 TA_get_random(UINT16 length, UINT8 *random)
{
    UINT8 ret  = TA_RESULTE_FAIL;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    if(!random || length == 0)
    {
        return (ret);
    }

    if(EC_ecc_context->method.EC_softsim_random != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_random(length,random))  == 0 )
        {
            ret  = TA_RESULTE_SUCC;
        }
    }
    ECOMM_DUMP(UNILOG_SOFTSIM,TA_get_random_1, P_INFO, "RANDOM->",length,random);
    return (ret);
}


/******************************************************************************
 * function  :    TA_AES_ctrl
 * Description:   AES 128bits encryption or decryption with
 *key_in[input]:     the 128bits AES key
 *data_in[input]：    the original data
 *length[input]：     the original data length
 *iv_in[input]：      the CBC encryption IV value
 *data_out[output]： the output data
 *mode[input]：       AES mode 1. CBC mode;0:ECB mode
 *encryption[input]：1: encryption ;0: decryption
 * return:   succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 TA_aes_ctrl(UINT8 *key_in, UINT8 *data_in, UINT32 length, UINT8 *iv_in, UINT8 *data_out, UINT8 mode, UINT8 encryption)
{
    UINT8 ret     = TA_RESULTE_FAIL;
    UINT8 keybits = 128;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    ECOMM_TRACE(UNILOG_SOFTSIM,TA_aes_ctrl_0, P_INFO,3, "TA_aes_ctrl Entry,Len:%d,mode:%d,encryption:%d",length,mode,encryption);
    ECOMM_DUMP(UNILOG_SOFTSIM, TA_aes_ctrl_1, P_INFO, "key_in->",16,key_in);
    ECOMM_DUMP(UNILOG_SOFTSIM, TA_aes_ctrl_2, P_INFO, "data_in->",length,data_in);
    ECOMM_DUMP(UNILOG_SOFTSIM, TA_aes_ctrl_3, P_INFO, "lv_in->",16,iv_in);

    if(mode != ECC_SEC_AES_EBC_MODE &&  mode != ECC_SEC_AES_CBC_MODE)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM,TA_aes_ctrl_4, P_INFO,1, "mode %d invalid!!!",mode);
        return (ret);
    }

    if(encryption != ECC_SEC_ENCRYPT_OPERATION && encryption != ECC_SEC_DECRYPT_OPERATION)
    {
         ECOMM_TRACE(UNILOG_SOFTSIM,TA_aes_ctrl_5, P_INFO,1, "encryption %d invalid!!!",encryption);
         return (ret);
    }

    if( length == 0 || (length % 16) != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM,TA_aes_ctrl_6, P_INFO,1, "length %d invalid!!!",length);
        return (ret);
    }

    if(!key_in || !data_in)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM,TA_aes_ctrl_7, P_INFO,2, "key_in 0x%x or data_in 0x%x invalid!!!",key_in,data_in);
        return (ret);
    }

    if(ECC_SEC_AES_CBC_MODE == mode)
    {
        if(!iv_in)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM,TA_aes_ctrl_8, P_INFO,1, "key_in 0x%x  invalid!!!",iv_in);
            return (ret);
        }
    }

    if(EC_ecc_context->method.EC_softsim_aes != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_aes(key_in,data_in,length,iv_in,data_out,mode,encryption,keybits))  == 0 )
        {
            ret  = TA_RESULTE_SUCC;
        }
    }
    return (ret);
}


/******************************************************************************
 * function :TA_sha256
 * Description:  create a 32 BYTE md with sha256 hash
 * input:
 *             content : the original data
 *             content_len  the original data  len
 * output:   32 BYTES md value
 * return:   succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 TA_sha256(const UINT8 *content, UINT16 content_len, UINT8 md[32])
{
    UINT8 ret  = TA_RESULTE_FAIL;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();
    ECOMM_DUMP(UNILOG_SOFTSIM,TA_sha256_1, P_INFO, "input->",content_len,content);

    if(!content || 0 == content_len)
    {
        return (ret);
    }

    if(EC_ecc_context->method.EC_softsim_sha256 != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_sha256(content,content_len,md,32))  == 0 )
        {
            ret  = TA_RESULTE_SUCC;
        }
    }

    ECOMM_DUMP(UNILOG_SOFTSIM,TA_sha256_2, P_INFO, "SHA256->:",32,md);
    return  (ret);

}

/******************************************************************************
 * function :TA_ec256_register_curve
 * Description:  register a ECDH curve with curve name
 * input:
 *            ec_name : the input curve name
 * output:    NULL
 * return:   succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8  TA_ec256_register_curve(const           CHAR  *ec_name)
{
    UINT8                      ret  = TA_RESULTE_FAIL;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();
    if( EC_ecc_context == NULL  ||
        ec_name   == NULL  ||
        *ec_name  == '\0'  ||
        strlen(ec_name) > sizeof(EC_ecc_context->conf.curve_name)
    )
    {
        return (ret);
    }

    if(EC_ecc_context->method.EC_softsim_ecpregister != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_ecpregister(ec_name))  == 0 )
        {
            ret  = TA_RESULTE_SUCC;
        }
    }
    ECOMM_TRACE(UNILOG_SOFTSIM,TA_ec256_register_curve_1, P_INFO,1, "TA_ec256_register_curve ret:%d",ret);
    return (ret);

}

/******************************************************************************
 * function :TA_ec256_create_key_pair
 * Description:  used the Private key and public key to create a pair of ECDH key group
 * input:
 *            sk : the 32 BYTE private key
 *            pk : the 32 BYTE public  key
 * output:    NULL
 * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 TA_ec256_create_key_pair(UINT8 sk[32], UINT8 pk[64])
{
    UINT8  ret  = TA_RESULTE_FAIL;
    UINT8  uncompressPK[65] = {0};
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    if(EC_ecc_context->method.EC_softsim_create_ecc_keypair != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_create_ecc_keypair(sk,32,uncompressPK,sizeof(uncompressPK)))  == 0 )
        {
            ret  = TA_RESULTE_SUCC;
        }
    }

    if(uncompressPK[0] == 0x04)
    {
        memcpy(pk,(const void*)(uncompressPK + 1),64);
    }
    else
    {
        memcpy(pk,(const void*)(uncompressPK),64);
    }

    ECOMM_DUMP(UNILOG_SOFTSIM, TA_ec256_create_key_pair_1, P_INFO,   "output sk->",32,sk);
    ECOMM_DUMP(UNILOG_SOFTSIM, TA_ec256_create_key_pair_2, P_INFO,   "output pk->",64,pk);
    ECOMM_TRACE(UNILOG_SOFTSIM,TA_ec256_create_key_pair_3, P_INFO,1, "TA_ec256_create_key_pair ret:%d",ret);
    return (ret);

}

/******************************************************************************
 * function :    TA_sfs_write
 * Description:  write the softsim configurations from flash with/or not with a security method
 * input:
 *            pk :         the public key of ECDH curve
 * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 TA_ec256_is_on_curve(UINT8 pk[64])
{
    UINT8 ret  = TA_RESULTE_FAIL;
    UINT8 uncompresspK[65] = {0};
    ECOMM_DUMP(UNILOG_SOFTSIM, TA_ec256_is_on_curve_1, P_INFO, "input pk->",64,pk);
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    if(pk[0] != 0x04)
    {
        uncompresspK[0] = 0x04;
        memcpy(&uncompresspK[1],pk,64);
    }
    else
    {
        memcpy(uncompresspK,pk,64);
    }

    if(EC_ecc_context->method.EC_softsim_check_pk_ison_curve != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_check_pk_ison_curve(uncompresspK,sizeof(uncompresspK)))  == 0 )
        {
            ret  = TA_RESULTE_SUCC;
        }
    }
    ECOMM_TRACE(UNILOG_SOFTSIM,TA_ec256_is_on_curve_2, P_INFO,1, "TA_ec256_is_on_curve ret:%d",ret);
    return (ret);

}
/******************************************************************************
 * function :    TA_ec256_ecdh
 * Description:  Create a share key with oneself private key and peer PUB key
 * input:
 *        sk:  private key
 *        pk:  public key
 *
 * output:
 *            sab : the  share key
 * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 TA_ec256_ecdh(UINT8 sk[32], UINT8 pk[64], UINT8 sab[32])
{
    UINT8 ret  = TA_RESULTE_FAIL;
    UINT8 uncompresspK[65] = {0};
    ECOMM_DUMP(UNILOG_SOFTSIM, TA_ec256_ecdh_1, P_INFO, "input sk->",32,sk);
    ECOMM_DUMP(UNILOG_SOFTSIM, TA_ec256_ecdh_2, P_INFO, "input pk->",64,pk);
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    if(pk[0] != 0x04)
    {
        uncompresspK[0] = 0x04;
        memcpy(&uncompresspK[1],pk,64);
    }
    else
    {
        memcpy(uncompresspK,pk,64);
    }

    if(EC_ecc_context->method.EC_softsim_ecc_ecdh != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_ecc_ecdh(sk,32,uncompresspK,sizeof(uncompresspK),sab,32)) == 0 )
        {
            ret  = TA_RESULTE_SUCC;
        }
    }
    ECOMM_DUMP(UNILOG_SOFTSIM, TA_ec256_ecdh_3, P_INFO, "OUTPUT SAB->",32,sab);
    return (ret);
}

/******************************************************************************
 * function :    TA_ec256_ecdsa_sign
 * Description:  Create a signature data with SK input
 * input:
 *          dgst :    the original data need to be signatured
 *          dgstLen : the original data length
 *          sk:     : the private key
 *
 * output:
 *            sign : the sign data
 * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 TA_ec256_ecdsa_sign(UINT8 *dgst, UINT16 dgstLen, UINT8 sk[32], UINT8 sign[64])
{
    UINT8 ret  = TA_RESULTE_FAIL;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();
    ECOMM_DUMP(UNILOG_SOFTSIM,TA_ec256_ecdsa_sign_1, P_INFO, "input dgst->",dgstLen,dgst);
    ECOMM_DUMP(UNILOG_SOFTSIM,TA_ec256_ecdsa_sign_2, P_INFO, "input sk->",32,sk);
    if(EC_ecc_context->method.EC_softsim_ecdsa_sign != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_ecdsa_sign(dgst,dgstLen,sk,32,sign,64))  == 0)
        {
            ret  = TA_RESULTE_SUCC;
        }
    }

    ECOMM_DUMP(UNILOG_SOFTSIM,TA_ec256_ecdsa_sign_3, P_INFO,    "OUTPUT sign->",64,sign);
    return (ret);

}

/******************************************************************************
 * function :    TA_ec256_ecdsa_verify
 * Description:  verify a signature with PK
 * input:
 *        dgst  : the original data
 *        dgst_len  : the original data length
 *        sign     : the signature data
 *        pk:     the peer public key
 * output:
 *
 * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 TA_ec256_ecdsa_verify(UINT8 *dgst, UINT16 dgst_len, UINT8 sign[64],UINT8 pk[64])
{
    UINT8 ret  = TA_RESULTE_FAIL;
    UINT8 uncompresspK[65] = {0};

    ECOMM_DUMP(UNILOG_SOFTSIM,TA_ec256_ecdsa_verify_2, P_INFO, "input sign->",64,sign);
    ECOMM_DUMP(UNILOG_SOFTSIM,TA_ec256_ecdsa_verify_3, P_INFO, "input pk->",64,pk);

    if(pk[0] != 0x04)
    {
        uncompresspK[0] = 0x04;
        memcpy(&uncompresspK[1],pk,64);
    }
    else
    {
        memcpy(uncompresspK,pk,64);
    }

    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    if(EC_ecc_context->method.EC_softsim_ecdsa_verify != NULL)
    {
         if((ret = EC_ecc_context->method.EC_softsim_ecdsa_verify(dgst,dgst_len,sign,64,uncompresspK,sizeof(uncompresspK)))  == 0)
         {
             ret  = TA_RESULTE_SUCC;
         }
    }

    ECOMM_TRACE(UNILOG_SOFTSIM,TA_ec256_ecdsa_verify_4, P_INFO,1, "TA_ec256_ecdsa_verify ret %d",ret);
    return (ret);

}

/******************************************************************************
 * function :    TA_ec256_create_csr
 * Description:  create X509 csr data
 * input:
 *            sk :     the private key
 *            pk   :   the public  key
 *            subject : the CSR subject
 * output:    outbuf:  the CSR  data
 *            iolen  :  the CSR len
 * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 TA_ec256_create_csr(UINT8 sk[32], UINT8 pk[64], UINT8 *subject,UINT8 *outbuf, UINT16 *iolen)
{
    UINT8 ret  = TA_RESULTE_FAIL;
    UINT8 uncompresspK[65]    = {0};

    ECOMM_STRING(UNILOG_SOFTSIM,TA_ec256_create_csr_0, P_INFO, "subject:%s",subject);
    ECOMM_DUMP(UNILOG_SOFTSIM,TA_ec256_create_csr_1, P_INFO, "input sk->",32,sk);
    ECOMM_DUMP(UNILOG_SOFTSIM,TA_ec256_create_csr_2, P_INFO, "input pk->",64,pk);

    if(pk[0] != 0x04)
    {
        uncompresspK[0] = 0x04;
        memcpy(&uncompresspK[1],pk,64);
    }
    else
    {
        memcpy(uncompresspK,pk,64);
    }

    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    if(EC_ecc_context->method.EC_softsim_create_csr != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_create_csr(sk, 32, uncompresspK, sizeof(uncompresspK), subject,outbuf, iolen))  == 0)
        {
            ret  = TA_RESULTE_SUCC;
        }
    }

    ECOMM_TRACE(UNILOG_SOFTSIM,TA_ec256_create_csr_6, P_INFO,2, "TA_ec256_create_csr LEN:%d,ret:%d",*iolen,ret);
    return (ret);
}

/******************************************************************************
 * function :    GenerateCMac
 * Description:  generate a CMAC
 * input:
 *            key :      the  input key
 *            Message   : the data
 *            MessLen :    the input data len
 * output:    Cmac:  the CMAC data
 * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/

UINT8 GenerateCMAC(UINT8 *Key,UINT8 *Message, UINT32 MessLen, UINT8* Cmac)
{
    UINT8  ret         = TA_RESULTE_FAIL;
    INT32  keybits     = 128;
    UINT8  cipher_type = MBEDTLS_CIPHER_AES_128_ECB;

    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    if(EC_ecc_context->method.EC_softsim_generateCMac != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_generateCMac(Key,keybits, Message, MessLen, Cmac,cipher_type))  == 0)
        {
            ret  = TA_RESULTE_SUCC;
        }
    }
    ECOMM_DUMP(UNILOG_SOFTSIM,GenerateCMac_1,  P_INFO,   "CMAC->",16,Cmac);
    return (ret);
}

/******************************************************************************
 * function :    GenerateCMac and verify it by internal testing
 * Description:  generate a CMAC
 * input:
 *            key :        the  input key
 *            Message   :  the data
 *            MessLen :    the input data len
 * output:    Cmac:  the CMAC data
 * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 VerifyCMac( const CHAR*                testname,
                      const UINT8*           key,
                      INT32                  keybits,
                      const UINT8*           messages,
                      const UINT32           message_lengths[4],
                      const UINT8*           expected_result,
                      UINT8                  cipher_type,
                      INT32                  block_size,
                      INT32                  num_tests )
{
    UINT8 ret  = TA_RESULTE_FAIL;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    if(EC_ecc_context->method.EC_softsim_VerifyCMac != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_VerifyCMac(testname, key, keybits, messages,message_lengths,expected_result,cipher_type,block_size,num_tests))  == 0)
        {
            ret  = TA_RESULTE_SUCC;
        }
    }
    return (ret);

}

/******************************************************************************
 * function :    TA_sfs_write
 * Description:  write the softsim configurations from flash with/or not with a security method
 * input:
 *   name_id :	   the ID which need to write
 *	 sec_store	 :	need to  a security method write or not
 *	 kvalue_length : the input data len
 *	 kvalue 	   : the input data

 * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/

UINT8 TA_sfs_write(UINT16 name_id, const UINT8 *kvalue, UINT16 kvalue_length, UINT8 sec_store)
{
    UINT8 ret  = TA_RESULTE_FAIL;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context;
    EC_SOFTSIM_DATA_ID   softsim_id = (EC_SOFTSIM_DATA_ID)name_id;

    ECOMM_TRACE(UNILOG_SOFTSIM, TA_sfs_write_0, P_INFO,3,  "TA_sfs_write ENTRY:index:%d,len %d,isSec:%d",name_id,kvalue_length,sec_store);
    ECOMM_DUMP(UNILOG_SOFTSIM,  TA_sfs_write_1, P_INFO,   "INPUT->",kvalue_length,kvalue);
    if(softsim_id > SFS_FILE_ID_EID)
    {
        return (ret);
    }

    if(!kvalue || kvalue_length == 0)
    {
        return (ret);
    }

    if(sec_store != 0 && sec_store != 1)
    {
        return (ret);
    }

    EC_ecc_context = EC_sec_get_instance();

    if(EC_ecc_context->method.EC_softsim_flash_write != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_flash_write(softsim_id, kvalue, kvalue_length, sec_store))  == 0)
        {
            ret  = TA_RESULTE_SUCC;
        }
    }

    ECOMM_TRACE(UNILOG_SOFTSIM, TA_sfs_write_2, P_INFO,3, "TA_sfs_write EXIT:index:%d,len %d,result:%d",name_id,kvalue_length,ret);
    return (ret);
}

/******************************************************************************
 * function :    TA_sfs_read
 * Description:  read the softsim configurations from flash with/or not with a security method
 * input:
 *            name_id :   the ID which need to read
 *            kvalue_max_length : the buffer max len
 *            sec_store   :  need to  read flsh with /or not with a security method
 * output:
 *            kvalue_length     : the buffer actual used len
 *            buffer            : the output buffer
 * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 TA_sfs_read(UINT16 name_id, UINT16 kvalue_max_length,UINT16 *kvalue_length,UINT8 *buffer,UINT8 sec_store)
{
    UINT8                      ret            = TA_RESULTE_FAIL;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = NULL;
    EC_SOFTSIM_DATA_ID         softsim_id     = (EC_SOFTSIM_DATA_ID)name_id;
    UINT32                     readlen        = 0;

    ECOMM_TRACE(UNILOG_SOFTSIM, TA_sfs_read_0, P_INFO,2, "TA_sfs_read id:%d,sec_store:%d",name_id,sec_store);

    if(softsim_id > SFS_FILE_ID_EID)
    {
         return (ret);
    }

    if(!buffer || kvalue_max_length == 0)
    {
        return (ret);
    }

    if(sec_store != 0 && sec_store != 1)
    {
         return (ret);
    }

    EC_ecc_context = EC_sec_get_instance();
    if(EC_ecc_context->method.EC_softsim_flash_read != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_flash_read(softsim_id, kvalue_max_length, &readlen, buffer, sec_store ))  == 0)
        {
            *kvalue_length = (UINT16)readlen;
             ret  = TA_RESULTE_SUCC;
        }
    }

    ECOMM_TRACE(UNILOG_SOFTSIM, TA_sfs_read_1, P_INFO,3, "TA_sfs_read id:%d,ret:%d,readlen:%d",name_id,ret,*kvalue_length);
    ECOMM_DUMP(UNILOG_SOFTSIM,  TA_sfs_read_2, P_INFO,   "ouput->",*kvalue_length,buffer);

    return (ret);
}

#if 0

UINT8 TA_ec256_test(void)
{
 #if 0
    UINT8  ret;
    UINT8  index = 0;
    UINT8  sab[64] = {0};
    UINT8  md[32]  = {0};



    UINT8  sk_in[]    =    {0x8a,0x9f,0x3d,0x3f,0x58,0xe6,0x23,0x6a,0x5e,0x90,0xd4,0x87,0x78,0x46,0x85,0x42,
                           0xd7,0x77,0xb0,0xef,0x3f,0x00,0x5c,0x82,0x69,0x09,0x7c,0xf9,0xb8,0xad,0x4b,0x1e};
    UINT8  pk_in[]    =
                          {0x4d,0x4f,0xad,0xe8,0xf3,0xbd,0x0d,0xb0,0xaf,0xf6,0xb7,0x57,0xe4,0x50,0xff,0x65,
                           0x05,0x62,0xc9,0x5d,0x7f,0x51,0x31,0x30,0x7d,0xc3,0x64,0xea,0x75,0x99,0x74,0x6b,
                           0x99,0x42,0xbf,0xf8,0x7a,0x2a,0xa0,0xdc,0x81,0x29,0xee,0xba,0xa3,0x49,0x4c,0x59,
                           0xb5,0x02,0xee,0x23,0x14,0x0a,0x4d,0xc2,0x68,0x39,0xd8,0x1d,0x5f,0x19,0x63,0x26};
    UINT8 sign_sk[] =
                    {0X3f,0X23,0X9e,0Xb5,0Xb0,0Xbc,0Xdc,0X19,0Xa9,0Xc7,0Xfd,0X9e,0X44,0X9d,0Xae,0X6a,
                     0Xcb,0X49,0Xaa,0X35,0X8d,0Xdc,0Xf1,0X34,0Xc5,0X7d,0X16,0Xc8,0X63,0X9d,0Xf5,0Xc5};
    UINT8 sign_msg[] =
                    {0x30,0x4D,0x80,0x10,0x64,0xBB,0xEB,0x09,0x70,0xB4,0xA0,0x6E,0x3C,0xD4,0x91,0x58,
                     0xE6,0xBD,0xF9,0xF6,0x81,0x10,0x85,0xA7,0x70,0x64,0x39,0xAD,0x9D,0xB9,0x3F,0x4A,
                     0x22,0x38,0x26,0x90,0x98,0xEA,0x83,0x15,0x74,0x65,0x73,0x74,0x2E,0x64,0x70,0x2E,
                     0x72,0x6F,0x61,0x6D,0x32,0x66,0x72,0x65,0x65,0x2E,0x63,0x6F,0x6D,0x84,0x10,0x26,
                     0xC0,0xCD,0xC0,0x43,0xA2,0x5A,0xBD,0x54,0x80,0xD5,0x6B,0x32,0xAE,0x08,0xB4};
    UINT8  verify_pk[] =
                        {0x4F,0x9A,0xEE,0x02,0xEB,0xE5,0x8D,0x69,0xAD,0xE9,0xA5,0x5C,0xF3,0x43,0x8E,0xAC,
                         0x46,0x5F,0xFF,0xC4,0x76,0x66,0x44,0x99,0xAF,0x68,0xE0,0x65,0x7A,0xA7,0x04,0xC1,
                         0x3F,0x35,0xE9,0x9F,0xB7,0x7C,0xAD,0x95,0x01,0xDA,0xEA,0x71,0x01,0xF3,0xB5,0x99,
                         0x2E,0xA4,0x9B,0x4E,0xA9,0xFF,0xDC,0x25,0x9A,0xB6,0x6B,0x91,0x8D,0x86,0x43,0x6C};


    TA_ec256_register_curve("secp256r1");
    for(index =0 ;index < 10;index++)
    {

        UINT8  data_out[32] = {0};
        UINT8  key_in[]   =   {0x68,0x76,0x77,0x56,0x49,0x4C,0x63,0x44,0x76,0x4E,0x7A,0x59,0x58,0x49,0x76,0x53};
        UINT8  data_in[]  =   {0x79,0x79,0x36,0x32,0x32,0x30,0x39,0x66,0x66,0x30,0x38,0x30,0x30,0x33,0x33,0x37,
                               0x62,0x39,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
        UINT8  iv_in[]    =   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
        UINT8  tempdecData[32] = {0};

        ECOMM_DUMP(UNILOG_SOFTSIM,  TA_ec256_test_11, P_INFO,  "AES128,IN->",32,data_in);
        TA_aes_ctrl(key_in,data_in,sizeof(data_in),iv_in,data_out,1,1);
        ECOMM_DUMP(UNILOG_SOFTSIM,  TA_ec256_test_22, P_INFO,  "AES128,ENC->",32,data_out);

        memset(tempdecData,0,sizeof(tempdecData));

        UINT8  iv_in_sec[]    =   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
        TA_aes_ctrl(key_in,data_out,sizeof(data_out),iv_in_sec,tempdecData,1,0);
        ECOMM_DUMP(UNILOG_SOFTSIM,  TA_ec256_test_33, P_INFO,  "AES128,DEC->",32,tempdecData);

        if(memcmp(data_in,tempdecData,32) == 0)
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, TA_ec256_test_44, P_INFO,1, "[%d]AES SUCC!!!",index);
        }
        else
        {
            ECOMM_TRACE(UNILOG_SOFTSIM, TA_ec256_test_55, P_INFO,1, "[%d]AES FAIL!!!",index);
        }
    }
    TA_ec256_ecdh(sk_in, pk_in,sab);

    UINT8 sign_sig[64] = {0};
    memset(md,0,32);
    memset(sign_sig,0,64);

    TA_sha256(sign_msg, sizeof(sign_msg), md);
    ECOMM_DUMP(UNILOG_SOFTSIM,  TA_ec256_test_2, P_INFO,  "SIGN MD->",32,md);

    TA_ec256_ecdsa_sign(md, 32, sign_sk,sign_sig);
    ECOMM_DUMP(UNILOG_SOFTSIM,  TA_ec256_test_3, P_INFO,  "SIGN ->",64,sign_sig);

    ret = TA_ec256_ecdsa_verify(md,32,sign_sig,verify_pk);
    ECOMM_TRACE(UNILOG_SOFTSIM, TA_ec256_test_4, P_INFO,2, "TA_ec256_ecdsa_verify ret:%d ",ret);


    UINT8 verify_sign[] =
                        {0x77,0xE8,0x1F,0x68,0x79,0x67,0xBA,0xC1,0xCE,0x27,0x75,0xAA,0x46,0x74,0x15,0x18,
                         0xD5,0x6D,0x06,0x3A,0xFE,0x20,0xB4,0x05,0x13,0xEE,0xFF,0x8C,0x9E,0x80,0xB1,0x51,
                         0x35,0x49,0xB3,0x1A,0x9E,0x36,0xE5,0x1C,0x73,0xD5,0x39,0xD2,0x56,0x29,0xED,0x6A,
                         0xDE,0x88,0x36,0x7D,0x64,0xD4,0xCE,0xBE,0xA0,0xE7,0xC6,0x80,0x4F,0x22,0x78,0x0C};

    UINT8  content[] =
                         {0XA0,0X46,0X80,0X10,0X74,0XB5,0X1F,0X42,0X1F,0X3A,0X47,0XD7,0X90,0X8D,0X78,0X06,
                          0XCA,0X86,0X6A,0X5F,0X81,0X10,0X85,0XA7,0X70,0X64,0X39,0XAD,0X9D,0XB9,0X3F,0X4A,
                          0X22,0X38,0X26,0X90,0X98,0XEA,0X83,0X0E,0X73,0X6D,0X64,0X70,0X2E,0X6E,0X62,0X2E,
                          0X63,0X74,0X63,0X2E,0X63,0X6E,0X84,0X10,0X1E,0XEE,0XBF,0XF3,0X37,0X19,0X4A,0X2B,
                          0XBD,0X27,0XEE,0XB1,0X65,0X5D,0XB9,0X15};
     memset(md,0,32);
     TA_sha256(content, sizeof(content), md);
     ECOMM_DUMP(UNILOG_SOFTSIM,  TA_ec256_test_5, P_INFO,  "SHA256->",32,md);

     ret = TA_ec256_ecdsa_verify(md,32,verify_sign,verify_pk);
     ECOMM_TRACE(UNILOG_SOFTSIM, TA_ec256_test_6, P_INFO,2, "TA_ec256_ecdsa_verify ret:%d ",ret);

     UINT8 ecdh_sk[] =
                     {0x8a,0x9f,0x3d,0x3f,0x58,0xe6,0x23,0x6a,0x5e,0x90,0xd4,0x87,0x78,0x46,0x85,0x42,
                      0xd7,0x77,0xb0,0xef,0x3f,0x00,0x5c,0x82,0x69,0x09,0x7c,0xf9,0xb8,0xad,0x4b,0x1e};
     UINT8 ecdh_pk[] =
                     {0x4d,0x4f,0xad,0xe8,0xf3,0xbd,0x0d,0xb0,0xaf,0xf6,0xb7,0x57,0xe4,0x50,0xff,0x65,
                      0x05,0x62,0xc9,0x5d,0x7f,0x51,0x31,0x30,0x7d,0xc3,0x64,0xea,0x75,0x99,0x74,0x6b,
                      0x99,0x42,0xbf,0xf8,0x7a,0x2a,0xa0,0xdc,0x81,0x29,0xee,0xba,0xa3,0x49,0x4c,0x59,
                      0xb5,0x02,0xee,0x23,0x14,0x0a,0x4d,0xc2,0x68,0x39,0xd8,0x1d,0x5f,0x19,0x63,0x26};
     memset(sab,0, sizeof(sab));
     TA_ec256_ecdh(ecdh_sk,ecdh_pk,sab);
     ECOMM_DUMP(UNILOG_SOFTSIM,  TA_ec256_test_7, P_INFO,  "SAB->",64,sab);

     EC_verify_AES_hw();


     UINT8 cmac_key[]     = {0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x40,0x41,0x42,0x43,0x44,0x45,0x46};
     UINT8 cmac_message[] = {0X01,0X02,0X03,0X04,0X05,0X06,0X07,0X08,0X09,0X00,0X0A,0X0B,0X0C,0X0D,0X0E,0X0F,
                             0X86,0X18,0X12,0X34,0X56,0X78,0X90,0XAB,0XCD,0XEF,0X12,0X34,0X56,0X78,0X90,0XAB,
                             0XCD,0XEF};
     UINT8 cmac_cmac[16]  = {0};

     GenerateCMAC(cmac_key,cmac_message, sizeof(cmac_message),cmac_cmac);
     return  (0);
   #endif

}
#endif

/******************************************************************************
 * function :    TA_ec256_method_register
 * Description:  when used the security api, first used this function to register method
 * return:    succ, TA_RESULTE_SUCC ; fail  TA_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 TA_ec256_method_register(void)
{
    if(gRegister == 0)
    {
        EC_sec_context_register();
        gRegister = 1;
    }

    return (TA_RESULTE_SUCC);
}

void esim_log_printf(const CHAR *format)
{
    UINT16   len  = 0;
    UINT16   type = 1;

    if(type == 1)
    {
        ECOMM_STRING(UNILOG_SOFTSIM, esim_log_printf_1, P_INFO, "esim_log_printf: %s",(const UINT8 *)format);
    }else if(type == 2)
    {
        ECOMM_DUMP(UNILOG_SOFTSIM,  esim_log_printf_2, P_INFO,  "DUMP->",len,(const UINT8 *)format);
    }else
    {
        ECOMM_TRACE(UNILOG_SOFTSIM, esim_log_printf_3, P_INFO,1, "error type :%d",type);
    }
    return;
}


#endif

