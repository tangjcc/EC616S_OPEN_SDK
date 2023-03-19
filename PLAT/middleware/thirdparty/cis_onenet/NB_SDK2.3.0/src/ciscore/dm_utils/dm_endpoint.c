/*
 *  FIPS-197 compliant AES implementation
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS 
 *  The AES block cipher was designed by Vincent Rijmen and Joan Daemen.
 *
 *  http://csrc.nist.gov/encryption/aes/rijndael/Rijndael.pdf
 *  http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf
 */



#include <string.h>
#include <stddef.h>
#include "cis_internals.h"
#if CIS_ENABLE_DM
#include "cis_api.h"
#include "dm_endpoint.h"
#include "sha256.h"
#include "j_base64.h"
#include "aes.h"
#include "debug_log.h"
#if _MSC_VER
#define snprintf _snprintf
#endif

void dmSdkInit(void *DMconfig)
{
	//to init g_opt
	Options *g_opt=(Options *)DMconfig;	
	cissys_getIMEI(g_opt->szCMEI_IMEI,sizeof(g_opt->szCMEI_IMEI));
    cissys_getIMSI(g_opt->szIMSI,sizeof(g_opt->szIMSI), true);
    //ECOMM_STRING(UNILOG_ONENET, dmSdkInit_1, P_INFO, "imei:%s", (uint8_t*)g_opt->szCMEI_IMEI);
    //ECOMM_STRING(UNILOG_ONENET, dmSdkInit_2, P_INFO, "imsi:%s", (uint8_t*)g_opt->szIMSI);
	return ;
}
#if 1

#define AES_BLOCK_SIZE 16

int my_aes_encrypt(char* enckey,char* encbuf, char* decbuf,int inlen,int* outlen)

{

		char key[34]="";// = "12345678"

		unsigned char iv[16] = "";

		cis_memset(key,0,sizeof(key));	
		cis_memset(iv,0,sizeof(iv));

		cis_memcpy(key, enckey, sizeof(key));

	    int nLen = inlen;//input_string.length();
	    int nBei;
		if((!encbuf)||(!decbuf))

			return -1;
		nBei = nLen / AES_BLOCK_SIZE + 1;
    int nTotal = nBei * AES_BLOCK_SIZE;

	   // printf("+++nTotal:%d+++\n",nTotal);

	    unsigned char *enc_s = ( unsigned char*)cis_malloc(nTotal);

	    #ifdef DEBUG_INFO

        printf("de enc_s=%p,size=%d,len=%d\n",enc_s,nTotal,nLen);

        #endif

	    if(enc_s==NULL)

	    {

			printf("enc_s mem err\n");

			return -1;

		}

	    int nNumber;

	    if (nLen % AES_BLOCK_SIZE > 0)

	        nNumber = nTotal - nLen;

	    else

	        nNumber = AES_BLOCK_SIZE;

	    cis_memset(enc_s, nNumber, nTotal);

	    cis_memcpy(enc_s, encbuf, nLen);

	    mbedtls_aes_context ctx;
	    mbedtls_aes_init( &ctx );
	    mbedtls_aes_setkey_enc( &ctx, (const unsigned char *)key, 256);
        mbedtls_aes_crypt_cbc( &ctx, MBEDTLS_AES_ENCRYPT, nBei * AES_BLOCK_SIZE, iv, enc_s, (unsigned char *)decbuf );
	    mbedtls_aes_free( &ctx );

		*outlen = nBei * AES_BLOCK_SIZE;

		cis_free(enc_s);

		enc_s=NULL;

	return 0;

}
#endif

void my_sha256(const char *src,int srclen, char *resultT )
{ 
	mbedtls_sha256((const unsigned char *)src,srclen,(unsigned char *)resultT,0);
	return ;
}


int genDmRegEndpointName(char ** data,void *dmconfig)
{	
    #define EP_MEM_SIZE  (264)
    char key[64]={0};
    char *szEpname=NULL;
    char *ciphertext=NULL;   
    char *szStars="****";
    char * name = "";
    int  ciphertext_len,ret=0;
    char *base64tmp=NULL;
    char *epnametmp=NULL;
    int i=0;
    char *passwd ="00000000000000000000000000000000";
    char iv[16]={0};
    Options *g_opt=(Options *)dmconfig;
    cis_memset(iv,0,16);
    for (i=0; i<16; ++i)
    {  
        iv[i] = 0;  
    } 
    
    szEpname=( char *)cis_malloc(EP_MEM_SIZE);
    ciphertext=( char *)cis_malloc(EP_MEM_SIZE);
    cis_memset(szEpname,0,EP_MEM_SIZE);
    cis_memset(ciphertext,0,EP_MEM_SIZE);

    sprintf(szEpname,"%s-%s-%s-%s",
    strlen((const char *)(g_opt->szCMEI_IMEI))>0?g_opt->szCMEI_IMEI:szStars,
    strlen((const char *)(g_opt->szCMEI_IMEI2))>0?g_opt->szCMEI_IMEI2:szStars,
    strlen((const char *)(g_opt->szIMSI))>0?g_opt->szIMSI:szStars,	
    strlen((const char *)(g_opt->szDMv))>0?g_opt->szDMv:szStars);
    //printf("reg szEpname:%s,%d\n",szEpname,strlen(szEpname));
    //ECOMM_STRING(UNILOG_ONENET, genDmRegEndpointName_1, P_INFO, "szEpname:%s", (uint8_t*)szEpname);
    if (strlen((const char *)name)<=0)
        name=szEpname;
    if(strlen((const char *)(g_opt->szPwd))>0)
    {
       passwd = g_opt->szPwd;
    }
    else
    {
       printf("pwd is null,use default pwd is:%s~\n", passwd);
    }

    //ECOMM_STRING(UNILOG_ONENET, genDmRegEndpointName_2, P_INFO, "passwd:%s", (uint8_t*)passwd);
    
    my_sha256(passwd,strlen((const char *)passwd),key);
    //ECOMM_STRING(UNILOG_ONENET, genDmRegEndpointName_3, P_INFO, "passwd->sha256->key:%s", (uint8_t*)key);
    
    /* A 128 bit IV */  
    /* Buffer for the decrypted text */  
    char *plaintext =  "plaintext";
    plaintext = name;
    /* Encrypt the plaintext */  
    //ECOMM_STRING(UNILOG_ONENET, genDmRegEndpointName_4, P_INFO, "plaintext:%s", (uint8_t*)plaintext);
    my_aes_encrypt((char *)key,(char *)plaintext, ciphertext,strlen((const char *)plaintext),&ciphertext_len );  
    //ECOMM_STRING(UNILOG_ONENET, genDmRegEndpointName_5, P_INFO, "key+plaintext->ciphertext:%s", (uint8_t*)ciphertext);

    name = ciphertext; //???????
    //base64
    char *testbase64="123456789";//MTIzNDU2Nzg5
    testbase64=name;

    base64tmp=( char *)cis_malloc(EP_MEM_SIZE);//szEpname is free now,use again;
    cis_memset(base64tmp,0,EP_MEM_SIZE);

    unsigned char *encData=0;
    //unsigned char *decData=0;EC add avoid compile warning
    int encDataLen=0;
    //int decDataLen=0;EC add avoid compile warning
    ret= j_base64_encode((unsigned char *)testbase64, ciphertext_len, &encData, (unsigned int *)&encDataLen);
    //ECOMM_STRING(UNILOG_ONENET, genDmRegEndpointName_6, P_INFO, "ciphertext-base64->encData:%s", (uint8_t*)encData);

    cis_memcpy(base64tmp,encData,encDataLen);
    //ECOMM_STRING(UNILOG_ONENET, genDmRegEndpointName_7, P_INFO, "base64tmp:%s", (uint8_t*)base64tmp);
    j_base64_free(encData, encDataLen);

    epnametmp=( char *)cis_malloc(EP_MEM_SIZE*4);
    if(epnametmp==NULL)
    {
        printf("mem err u3\n");ret=-1;
        goto out;
    }
    cis_memset(epnametmp,0,EP_MEM_SIZE*4);

    //snprintf((char *)epnametmp,EP_MEM_SIZE*4,"I@#@%s@#@%s",base64tmp,g_opt->szAppKey);
    snprintf((char *)epnametmp,EP_MEM_SIZE*4,"I@#@%s@#@%s@#@%s@#@%s",base64tmp,g_opt->szAppKey,g_opt->szCMEI_IMEI,g_opt->szDMv);//complain to spec v2.0.3
    //ECOMM_STRING(UNILOG_ONENET, genDmRegEndpointName_8, P_INFO, "base64tmp+opt->szAppKey=epnametemp:%s", (uint8_t*)epnametmp);
    //printf("reg epname=%s,%d\n", epnametmp,strlen(epnametmp));
    name = epnametmp; //?????????
    ///////////////
    *data=(char *)cis_malloc(strlen((const char *)name)+1);
    cis_memset(*data,0,strlen((const char *)name)+1);
    cis_memcpy(*data,name,strlen((const char *)name));

    ret=0;
out:
    if(szEpname)
    {
        cis_free(szEpname);szEpname=NULL;
    }
    if(ciphertext)
    {
        cis_free(ciphertext);ciphertext=NULL;
    }
    if(epnametmp)
    {
        cis_free(epnametmp);epnametmp=NULL;
    }
    if(base64tmp)
    {
        cis_free(base64tmp);base64tmp=NULL;
    }
    return ret;
}

int genDmUpdateEndpointName(char **data,void *dmconfig)
{
    #define EP_MEM_SIZE  (264)
    char key[64]={0};
    char *szEpname=NULL;
    char *ciphertext=NULL;   
    char *szStars="****";
    char * name = "";
    int  ciphertext_len;
    char *base64tmp=NULL;
    char *epnametmp=NULL;
    int i=0,ret=-1;
    char *passwd = "00000000000000000000000000000000";
    char iv[16]={0};
    Options *g_opt=(Options *)dmconfig;
    cis_memset(iv,0,16);
    for (i=0; i<16; ++i) {  
        iv[i] = 0;  
    }
    szEpname=( char *)cis_malloc(EP_MEM_SIZE);
    ciphertext=( char *)cis_malloc(EP_MEM_SIZE);
    cis_memset(szEpname,0,EP_MEM_SIZE);
    cis_memset(ciphertext,0,EP_MEM_SIZE);
    
    snprintf((char *)szEpname,EP_MEM_SIZE,"%s-%s-%s",
    strlen((const char *)(g_opt->szCMEI_IMEI))>0?g_opt->szCMEI_IMEI:szStars,
    strlen((const char *)(g_opt->szCMEI_IMEI2))>0?g_opt->szCMEI_IMEI2:szStars,
    strlen((const char *)(g_opt->szIMSI))>0?g_opt->szIMSI:szStars);//complain to spec v2.0.3
    //strlen((const char *)(g_opt->szIMSI))>0?g_opt->szIMSI:szStars,szStars);
    //ECOMM_STRING(UNILOG_ONENET, genDmUpdateEndpointName_1, P_INFO, "szEpname:%s", (uint8_t*)szEpname);
    //printf("update szEpname:%s,%d\n",szEpname,strlen(szEpname));
    if (strlen((const char *)name)<=0)
        name=szEpname;
    
    if(strlen((const char *)(g_opt->szPwd))>0)
    {
        passwd = g_opt->szPwd;
    }
    else
    {
        printf("pwd is null,use default pwd is:%s~\n", passwd);
    }
    //sha256(passwd,strlen(passwd),key);
    
    my_sha256(passwd,strlen((const char *)passwd),key);
    //ECOMM_STRING(UNILOG_ONENET, genDmUpdateEndpointName_2, P_INFO, "passwd:%s", (uint8_t*)passwd);
    
    //ECOMM_STRING(UNILOG_ONENET, genDmUpdateEndpointName_3, P_INFO, "passwd->sha256->key:%s", (uint8_t*)key);
    /* A 128 bit IV */  
    /* Buffer for the decrypted text */  
    char *plaintext =  "plaintext";
    
    plaintext = name;
    /* Encrypt the plaintext */  
    //ECOMM_STRING(UNILOG_ONENET, genDmUpdateEndpointName_4, P_INFO, "plaintext:%s", (uint8_t*)plaintext);
    my_aes_encrypt((char *)key,plaintext, ciphertext,strlen((const char *)plaintext),&ciphertext_len );  
    //ECOMM_STRING(UNILOG_ONENET, genDmUpdateEndpointName_5, P_INFO, "key+plaintext->ciphertext:%s", (uint8_t*)ciphertext);
    name = ciphertext; //???????
    //base64
    char *testbase64="123456789";//MTIzNDU2Nzg5
    testbase64=name;
    base64tmp=( char *)cis_malloc(EP_MEM_SIZE);//szEpname is free now,use again;
    cis_memset(base64tmp,0,EP_MEM_SIZE);

    unsigned char *encData=0;
    //unsigned char *decData=0;EC add avoid compile warning
    int encDataLen=0;
    //int decDataLen=0;EC add avoid compile warning
    ret= j_base64_encode((unsigned char *)testbase64, ciphertext_len, &encData,( unsigned int *)&encDataLen);
    //ECOMM_STRING(UNILOG_ONENET, genDmUpdateEndpointName_6, P_INFO, "ciphertext-base64->encData:%s", (uint8_t*)encData);
    cis_memcpy(base64tmp,encData,encDataLen);	
    //ECOMM_STRING(UNILOG_ONENET, genDmUpdateEndpointName_7, P_INFO, "base64tmp:%s", (uint8_t*)base64tmp);
    j_base64_free(encData, encDataLen);
    epnametmp=(char *)cis_malloc(EP_MEM_SIZE*4);
    if(epnametmp==NULL)
    {
        printf("mem err u3\n");ret=-1;
        goto out;
    }
    cis_memset(epnametmp,0,EP_MEM_SIZE*4);
    //sprintf((char *)epnametmp,"I@#@%s@#@%s",base64tmp,g_opt->szAppKey);
    sprintf((char *)epnametmp,"I@#@%s@#@%s@#@%s@#@%s",base64tmp,g_opt->szAppKey,g_opt->szCMEI_IMEI,g_opt->szDMv);//complain to spec v2.0.3
    //ECOMM_STRING(UNILOG_ONENET, genDmUpdateEndpointName_8, P_INFO, "base64tmp+opt->szAppKey=epnametemp:%s", (uint8_t*)epnametmp);
    //printf("update epname=%s,%d\n", epnametmp,strlen(epnametmp));
    name = epnametmp;
    *data=(char *)cis_malloc(strlen((const char *)name)+1);
    cis_memset(*data,0,strlen((const char *)name)+1);
    cis_memcpy(*data,name,strlen((const char *)name));
    ret=0;
out:
    if(szEpname)
    {
        cis_free(szEpname);szEpname=NULL;
    }
    if(ciphertext)
    {
        cis_free(ciphertext);ciphertext=NULL;
    }
    if(epnametmp)
    {
        cis_free(epnametmp);epnametmp=NULL;
    }
    if(base64tmp)
    {
        cis_free(base64tmp);base64tmp=NULL;
    }
    return ret;
}

uint8* genEncodeValue(char *inValue, uint32_t * outLen, void *dmconfig)
{
    #define EP_MEM_SIZE  (264)
    char key[64]={0};
    char *ciphertext=NULL;   
    char * name = "";
    int  ciphertext_len;
    char *passwd = "00000000000000000000000000000000";
    Options *g_opt=(Options *)dmconfig;
    uint8* data;
    
    ciphertext=( char *)cis_malloc(EP_MEM_SIZE);
    cis_memset(ciphertext,0,EP_MEM_SIZE);
    
    //ECOMM_STRING(UNILOG_ONENET, genEncodeValue_1, P_INFO, "inValue:%s", (uint8_t*)inValue);
    if (strlen((const char *)name)<=0)
        name=inValue;
    
    if(strlen((const char *)(g_opt->szPwd))>0)
    {
        passwd = g_opt->szPwd;
    }
    else
    {
        printf("pwd is null,use default pwd is:%s~\n", passwd);
    }
    
    my_sha256(passwd,strlen((const char *)passwd),key);
    //ECOMM_STRING(UNILOG_ONENET, genEncodeValue_2, P_INFO, "passwd:%s", (uint8_t*)passwd);
    
    //ECOMM_STRING(UNILOG_ONENET, genEncodeValue_3, P_INFO, "passwd->sha256->key:%s", (uint8_t*)key);
    /* A 128 bit IV */  
    /* Buffer for the decrypted text */  
    char *plaintext =  "plaintext";
    
    plaintext = name;
    /* Encrypt the plaintext */  
    //ECOMM_STRING(UNILOG_ONENET, genEncodeValue_4, P_INFO, "plaintext:%s", (uint8_t*)plaintext);
    my_aes_encrypt((char *)key,plaintext, ciphertext,strlen((const char *)plaintext),&ciphertext_len );  
    //ECOMM_STRING(UNILOG_ONENET, genEncodeValue_5, P_INFO, "key+plaintext->ciphertext:%s", (uint8_t*)ciphertext);
    name = ciphertext; //???????
    //base64
    char *testbase64="123456789";//MTIzNDU2Nzg5
    testbase64=name;

    unsigned char *encData=0;
    //unsigned char *decData=0;EC add avoid compile warning
    int encDataLen=0;
    //int decDataLen=0;EC add avoid compile warning
    j_base64_encode((unsigned char *)testbase64, ciphertext_len, &encData,( unsigned int *)&encDataLen);
    *outLen = strlen((const char *)encData);
    //ECOMM_TRACE(UNILOG_ONENET, genEncodeValue_6, P_INFO, 2,"j_base64_encode return:%d,outLen:%d", ret,*outLen);
    //ECOMM_STRING(UNILOG_ONENET, genEncodeValue_7, P_INFO, "ciphertext-base64->encData:%s", (uint8_t*)encData);
    data=(uint8 *)cis_malloc(*outLen+1);
    cis_memset(data,0,*outLen+1);
    cis_memcpy(data,encData,*outLen);
    j_base64_free(encData, encDataLen);
    if(ciphertext)
    {
        cis_free(ciphertext);ciphertext=NULL;
    }
    return data;
}

int prv_getDmUpdateQueryLength(st_context_t * contextP,
                                          st_server_t * server)
{
    int index;
    //int res;EC add avoid compile warning
    //char buffer[21];EC add avoid compile warning

    index = strlen("epi=");
    index += strlen((const char *)(contextP->DMprivData));
    return index + 1;
}

int prv_getDmUpdateQuery(st_context_t * contextP,
                                    st_server_t * server,
                                    uint8_t * buffer,
                                    size_t length)
{
    int index;
    //int res,name_len;
    int res;//EC add avoid compile warning
    
    index = cis_utils_stringCopy((char *)buffer, length, "epi=");
    if (index < 0) return 0;
    res = cis_utils_stringCopy((char *)buffer + index, length - index, (const char *)(contextP->DMprivData));
    if (res < 0) return 0;
    index += res;

    if(index < (int)length)
    {
        buffer[index++] = '\0';
    }
    else
    {
        return 0;
    }

    return index;
}
#endif
