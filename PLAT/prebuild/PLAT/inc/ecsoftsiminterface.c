
/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - ecsoftsiminterface.c
 Description:    - support softsim api to vendor for softSIM functions
 History:        - 25/12/2020  created by yjwang
 ******************************************************************************
******************************************************************************/
#ifdef SOFTSIM_FEATURE_ENABLE
#include <stdio.h>
#include <string.h>

#include "ecSeccallback.h"
#include "ecSecFlashApi.h"

#include "debug_trace.h"
#include "debug_log.h"

#if defined CHIP_EC616
#include "unilog_ec616.h"
#elif defined CHIP_EC616S
#include "unilog_ec616s.h"
#endif

#include "ecsoftsiminterface.h"

static UINT8  gRegister = 0;

/******************************************************************************
 * function:    ecSoftSimRandom
 * Description: create a random with current time value
 * input:  length: the defined length of required random
 * output: random
 * return: succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 ecSoftSimRandom(UINT16 length, UINT8 *random)
{
    UINT8 ret  = EC_SOFTSIM_RESULTE_FAIL;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    if(!random || length == 0)
    {
        return (ret);
    }

    if(EC_ecc_context->method.EC_softsim_random != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_random(length,random))  == 0 )
        {
            ret  = EC_SOFTSIM_RESULTE_SUCC;
        }
    }
    return (ret);
}


/******************************************************************************
 * function  :       ecSoftSimAes
 * Description:      AES 128bits encryption or decryption with
 *key_in[input]:     the 128bits AES key
 *data_in[input]：    the original data
 *length[input]：     the original data length
 *iv_in[input]：      the CBC encryption IV value
 *data_out[output]： the output data
 *mode[input]：       AES mode 1. CBC mode;0:ECB mode
 *encryption[input]：1: encryption ;0: decryption
 * return:   succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 ecSoftSimAes(UINT8 *key_in, UINT8 *data_in, UINT32 length, UINT8 *iv_in, UINT8 *data_out, UINT8 mode, UINT8 encryption)
{
    UINT8 ret     = EC_SOFTSIM_RESULTE_FAIL;
    UINT8 keybits = 128;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    if(mode != ECC_SEC_AES_EBC_MODE &&  mode != ECC_SEC_AES_CBC_MODE)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM,ecSoftSimAes_0, P_INFO,1, "mode %d invalid!!!",mode);
        return (ret);
    }

    if(encryption != ECC_SEC_ENCRYPT_OPERATION && encryption != ECC_SEC_DECRYPT_OPERATION)
    {
         ECOMM_TRACE(UNILOG_SOFTSIM,ecSoftSimAes_1, P_INFO,1, "encryption %d invalid!!!",encryption);
         return (ret);
    }

    if( length == 0 || (length % 16) != 0 )
    {
        ECOMM_TRACE(UNILOG_SOFTSIM,ecSoftSimAes_2, P_INFO,1, "length %d invalid!!!",length);
        return (ret);
    }

    if(!key_in || !data_in)
    {
        ECOMM_TRACE(UNILOG_SOFTSIM,ecSoftSimAes_3, P_INFO,2, "key_in 0x%x or data_in 0x%x invalid!!!",key_in,data_in);
        return (ret);
    }

    if(ECC_SEC_AES_CBC_MODE == mode)
    {
        if(!iv_in)
        {
            return (ret);
        }
    }

    if(EC_ecc_context->method.EC_softsim_aes != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_aes(key_in,data_in,length,iv_in,data_out,mode,encryption,keybits))  == 0 )
        {
            ret  = EC_SOFTSIM_RESULTE_SUCC;
        }
    }
    return (ret);
}


/******************************************************************************
 * function :ecSoftSimSha256
 * Description:  create a 32 BYTE md with sha256 hash
 * input:
 *             content : the original data
 *             content_len  the original data  len
 * output:   32 BYTES md value
 * return:   succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 ecSoftSimSha256(const UINT8 *content, UINT16 content_len, UINT8 md[32])
{
    UINT8 ret  = EC_SOFTSIM_RESULTE_FAIL;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    if(!content || 0 == content_len)
    {
        return (ret);
    }

    if(EC_ecc_context->method.EC_softsim_sha256 != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_sha256(content,content_len,md,32))  == 0 )
        {
            ret  = EC_SOFTSIM_RESULTE_SUCC;
        }
    }

    return  (ret);

}

/******************************************************************************
 * function :ecSoftSimRegisterCurve
 * Description:  register a ECDH curve with curve name
 * input:
 *            ec_name : the input curve name
 * output:    NULL
 * return:   succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8  ecSoftSimRegisterCurve(const           CHAR  *ec_name)
{
    UINT8                      ret  = EC_SOFTSIM_RESULTE_FAIL;
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
            ret  = EC_SOFTSIM_RESULTE_SUCC;
        }
    }
    return (ret);

}

/******************************************************************************
 * function :ecSoftSimCreateEcpKeyPair
 * Description:  used the Private key and public key to create a pair of ECDH key group
 * input:
 *            sk : the 32 BYTE private key
 *            pk : the 32 BYTE public  key
 * output:    NULL
 * return:   succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 ecSoftSimCreateEcpKeyPair(UINT8 sk[32], UINT8 pk[64])
{
    UINT8  ret  = EC_SOFTSIM_RESULTE_FAIL;
    UINT8  uncompressPK[65] = {0};
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    if(EC_ecc_context->method.EC_softsim_create_ecc_keypair != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_create_ecc_keypair(sk,32,uncompressPK,sizeof(uncompressPK)))  == 0 )
        {
            ret  = EC_SOFTSIM_RESULTE_SUCC;
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
    return (ret);

}

/******************************************************************************
 * function :     ecSoftSimCheckPkWhetherOnCurve
 * Description:   check the 64 byte key whether belong to a registered ecp curve
 * input:
 *            pk :the public key of ECDH curve
 * return:   succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 ecSoftSimCheckPkWhetherOnCurve(UINT8 pk[64])
{
    UINT8 ret  = EC_SOFTSIM_RESULTE_FAIL;
    UINT8 uncompresspK[65] = {0};

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
            ret  = EC_SOFTSIM_RESULTE_SUCC;
        }
    }
    return (ret);

}
/******************************************************************************
 * function :    ecSoftSimCreateEcdhKey
 * Description:  Create a share key with oneself private key and peer PUB key
 * input:
 *        sk:  private key
 *        pk:  public key
 *
 * output:
 *            sab : the  share key
 * return:    succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 ecSoftSimCreateEcdhKey(UINT8 sk[32], UINT8 pk[64], UINT8 sab[32])
{
    UINT8 ret  = EC_SOFTSIM_RESULTE_FAIL;
    UINT8 uncompresspK[65] = {0};

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
            ret  = EC_SOFTSIM_RESULTE_SUCC;
        }
    }
    return (ret);
}

/******************************************************************************
 * function :    ecSoftSimEcdsaSign
 * Description:  Create a signature data with SK input
 * input:
 *          dgst :    the original data need to be signatured
 *          dgstLen : the original data length
 *          sk:     : the private key
 *
 * output:
 *            sign : the sign data
 * return:   succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 ecSoftSimEcdsaSign(UINT8 *dgst, UINT16 dgstLen, UINT8 sk[32], UINT8 sign[64])
{
    UINT8 ret  = EC_SOFTSIM_RESULTE_FAIL;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    if(EC_ecc_context->method.EC_softsim_ecdsa_sign != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_ecdsa_sign(dgst,dgstLen,sk,32,sign,64))  == 0)
        {
            ret  = EC_SOFTSIM_RESULTE_SUCC;
        }
    }
    return (ret);
}

/******************************************************************************
 * function :    ecSoftSimEcdsaSignVerify
 * Description:  verify a signature with PK
 * input:
 *        dgst  : the original data
 *        dgst_len  : the original data length
 *        sign     : the signature data
 *        pk:     the peer public key
 * output:
 *
 * return:   succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 ecSoftSimEcdsaSignVerify(UINT8 *dgst, UINT16 dgst_len, UINT8 sign[64],UINT8 pk[64])
{
    UINT8 ret  = EC_SOFTSIM_RESULTE_FAIL;
    UINT8 uncompresspK[65] = {0};
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
             ret  = EC_SOFTSIM_RESULTE_SUCC;
         }
    }

    return (ret);

}

/******************************************************************************
 * function :    ecSoftSimCreateCsr
 * Description:  create X509 csr data
 * input:
 *            sk   :   the private key
 *            pk   :   the public  key
 *            subject : the CSR subject
 * output:    outbuf:  the CSR  data
 *            iolen  :  the CSR len
 * return:    succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 ecSoftSimCreateCsr(UINT8 sk[32], UINT8 pk[64], UINT8 *subject,UINT8 *outbuf, UINT16 *iolen)
{
    UINT8 ret  = EC_SOFTSIM_RESULTE_FAIL;
    UINT8 uncompresspK[65]    = {0};

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
            ret  = EC_SOFTSIM_RESULTE_SUCC;
        }
    }

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
 * return:   succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 ecSoftSimCreateCmac(UINT8 *Key,UINT8 *Message, UINT32 MessLen, UINT8* Cmac)
{
    UINT8  ret         = EC_SOFTSIM_RESULTE_FAIL;
    INT32  keybits     = 128;
    UINT8  cipher_type = MBEDTLS_CIPHER_AES_128_ECB;

    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = EC_sec_get_instance();

    if(EC_ecc_context->method.EC_softsim_generateCMac != NULL)
    {
        if((ret = EC_ecc_context->method.EC_softsim_generateCMac(Key,keybits, Message, MessLen, Cmac,cipher_type))  == 0)
        {
            ret  = EC_SOFTSIM_RESULTE_SUCC;
        }
    }
    return (ret);
}


/******************************************************************************
 * function :    ecSoftSimFlashWrite
 * Description:  write the softsim configurations from flash with/or not with a security method
 * input:
 *   name_id :       the ID which need to write
 *   sec_store :     need to  a security method write or not
 *   kvalue_length : the input data len
 *   kvalue       :  the input data

 * return:   succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/

UINT8 ecSoftSimFlashWrite(UINT16 name_id, const UINT8 *kvalue, UINT16 kvalue_length, UINT8 sec_store)
{
    UINT8 ret  = EC_SOFTSIM_RESULTE_FAIL;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context;
    EC_SOFTSIM_DATA_ID   softsim_id = (EC_SOFTSIM_DATA_ID)name_id;

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
            ret  = EC_SOFTSIM_RESULTE_SUCC;
        }
    }

    return (ret);
}

/******************************************************************************
 * function :    ecSoftSimFlashRead
 * Description:  read the softsim configurations from flash with/or not with a security method
 * input:
 *            name_id :   the ID which need to read
 *            kvalue_max_length : the buffer max len
 *            sec_store   :  need to  read flsh with /or not with a security method
 * output:
 *            kvalue_length     : the buffer actual used len
 *            buffer            : the output buffer
 * return:    succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 ecSoftSimFlashRead(UINT16 name_id, UINT16 kvalue_max_length,UINT16 *kvalue_length,UINT8 *buffer,UINT8 sec_store)
{
    UINT8                      ret            = EC_SOFTSIM_RESULTE_FAIL;
    EC_ESIM_SEC_CONTEXT_INFO  *EC_ecc_context = NULL;
    EC_SOFTSIM_DATA_ID         softsim_id     = (EC_SOFTSIM_DATA_ID)name_id;
    UINT32                     readlen        = 0;


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
             ret  = EC_SOFTSIM_RESULTE_SUCC;
        }
    }
    return (ret);
}



/******************************************************************************
 * function :    ecSoftSimMethodRegister
 * Description:  when used the security api, first used this function to register method
 * return:       succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
UINT8 ecSoftSimMethodRegister(void)
{
    if(gRegister == 0)
    {
        EC_sec_context_register();
        gRegister = 1;
    }

    return (EC_SOFTSIM_RESULTE_SUCC);
}

#endif
