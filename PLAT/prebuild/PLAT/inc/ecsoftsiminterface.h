#ifndef EC_SOFTSIM_INTERFACE_H
#define EC_SOFTSIM_INTERFACE_H

/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - ecsoftsiminterface.h
 Description:    - support security api to vendor for softSIM functions
 History:        - 09/09/2020  created by yjwang
 ******************************************************************************
******************************************************************************/


#if defined(_WIN32)
#include "win32typedef.h"
#else
#include "commontypedef.h"
#endif

#include "debug_trace.h"
#include "debug_log.h"

#define   EC_SOFTSIM_RESULTE_FAIL   (1)
#define   EC_SOFTSIM_RESULTE_SUCC   (0)


/*-------------------------- define macro interface-------------------*/
/******************************************************************************
 * function:    ecSoftSimRandom
 * Description: create a random with current time value
 * input:  length: the defined length of required random
 * output: random
 * return: succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
extern UINT8 ecSoftSimRandom(UINT16 length, UINT8 *random);


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
 
extern UINT8 ecSoftSimAes(UINT8 *key_in, UINT8 *data_in, UINT32 length, UINT8 *iv_in, UINT8 *data_out, UINT8 mode, UINT8 encryption);

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
extern UINT8 ecSoftSimSha256(const UINT8 *content, UINT16 content_len, UINT8 md[32]);

/******************************************************************************
 * function :ecSoftSimRegisterCurve
 * Description:  register a ECDH curve with curve name
 * input: 
 *            ec_name : the input curve name
 * output:    NULL
 * return:   succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
extern UINT8 ecSoftSimRegisterCurve(const char *ec_name);

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
extern UINT8 ecSoftSimCreateEcpKeyPair(UINT8 sk[32], UINT8 pk[64]);


/******************************************************************************
 * function :     ecSoftSimCheckPkWhetherOnCurve
 * Description:   check the 64 byte key whether belong to a registered ecp curve
 * input: 
 *            pk :the public key of ECDH curve
 * return:   succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
extern UINT8 ecSoftSimCheckPkWhetherOnCurve(UINT8 pk[64]);

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
extern UINT8 ecSoftSimCreateEcdhKey(UINT8 sk[32], UINT8 pk[64], UINT8 sab[32]);

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

extern UINT8 ecSoftSimEcdsaSign(UINT8 *dgst, UINT16 dgstLen, UINT8 sk[32], UINT8 sign[64]);

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
extern UINT8 ecSoftSimCreateCmac(UINT8 *Key, UINT8 *Message, UINT32 MessLen,UINT8 *Cmac);

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
extern UINT8 ecSoftSimCreateCsr(UINT8 sk[32], UINT8 pk[64], UINT8 *subject,UINT8 *outbuf, UINT16 *iolen);

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
extern UINT8 ecSoftSimEcdsaSignVerify(UINT8 *dgst, UINT16 dgst_len, UINT8 sign[64],UINT8 pk[64]);

/******************************************************************************
 * SoftsimReadRawFlash
 * Description: Erase to raw flash
 * param[in]   UINT32 addr, read flash address
 * param[out]   UINT8 *pBuffer, the pointer to the data buffer
 * param[in]   UINT32 length, the length of the data read
 * Comment:
******************************************************************************/
extern INT8 SoftsimReadRawFlash(UINT32 addr, UINT8 *pBuffer, UINT32 bufferSize);

/******************************************************************************
 * SoftsimEraseRawFlash
 * Description: Erase to raw flash
 * param[in]   UINT32 sectorAddr, the flash sector address
 * param[in]   UINT32 size, the size of erase flash
 * Comment:
******************************************************************************/
extern INT8 SoftsimEraseRawFlash(UINT32 sectorAddr, UINT32 size);

/******************************************************************************
 * SoftsimWriteToRawFlash
 * Description: write data to raw flash
 * param[in]   UINT32 addr, the flash address to be wrote
 * param[in]   UINT8 *pBuffer, the pointer to the buffer for write data
 * param[in]   UINT8 length, the data length to be wrote
 * Comment:
******************************************************************************/
extern INT8 SoftsimWriteToRawFlash(UINT32 addr, UINT8 *pBuffer, UINT32 bufferSize);

/******************************************************************************
 * function :    ecSoftSimMethodRegister
 * Description:  when used the security api, first used this function to register  method
 * return:       succ, EC_SOFTSIM_RESULTE_SUCC ; fail  EC_SOFTSIM_RESULTE_FAIL
 * Comment:
******************************************************************************/
extern UINT8 ecSoftSimMethodRegister(void);


#endif
