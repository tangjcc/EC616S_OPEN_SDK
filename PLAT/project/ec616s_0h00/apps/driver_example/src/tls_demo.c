/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    wdt_demo.c
 * Description:  EC616 WDT driver example entry source file
 * History:      Rev1.0   2018-07-23
 *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "bsp.h"

#include "l2ctls_ec616s.h"


/*
   This project demos the usage of HW TLS engine, incuding AES/SHA
   AES: support ECB/CBC mode, support 128/192/256 bits key
   SHA: support SHA1/SHA224/SHA256
   The demo:
   sha part: use sha256 to generate the hash value of ShaInput
             the Digest message is stored in ShaDigest, print it to console and compare with PC tool SHA 256 result
             
   aes part: 
   step1: init AesInput buffer and key, use ECB mode with 192bits key
   step2: encypt AesInput buffer and the result is stored in AesEncyptOutput buffer
   step3: use AesEncyptOutput buffer as input to perform decypt, the result is stored in AesDecyptOutput buffer
   step4: compare the AesInput with AesDecyptOutput buffer, they should be same if success
 */

#define SHA_DATA_LEN 256
#define AES_DATA_LEN 256
#define AES_KEY_LEN  24//24bytes,192bits


uint8_t ShaInput[SHA_DATA_LEN]={0};
uint32_t ShaDigest[8]={0};//256bits


static __attribute__((aligned(4))) uint8_t AesInput[AES_DATA_LEN]={0};
static __attribute__((aligned(4))) uint8_t AesEncyptOutput[AES_DATA_LEN]={0};
static __attribute__((aligned(4))) uint8_t AesDecyptOutput[AES_DATA_LEN]={0};

static __attribute__((aligned(4)))  uint8_t AesKey   [AES_KEY_LEN]={0};



/*this API is used to convert the endianess of the key
also for IV when using cbc mode  the HW default endianess 
is little endian. In order to match with PC encypt tool,
we need to call ConvertEndian() in this driver example
In full project, HW will be initialised as big endian, so no need to call this API*/
static void ConvertEndian(uint32_t keybits,uint8_t*key)
{
    uint8_t tmp,i;
    uint8_t cnt=(keybits>>3);

     printf("ConvertEndian cnt %d\r\n",cnt);
    
    for(i=0;i<cnt/2;i++)
    {
        tmp=key[i];
        key[i]=key[cnt-1-i];
        key[cnt-1-i]=tmp;
    }
    
    printf("ConvertEndian done\r\n");


}



void TLS_ExampleEntry(void)
{
    uint32_t RetValue=L2CTLSDRV_OK;
    uint32_t i=0;


    printf("sha demo start\r\n");

    for(i=0;i<SHA_DATA_LEN;i++)
    {
       ShaInput[i]='a';//could be initialised to any other value
    }

    L2CTlsInit();
    L2CShaComInit(L2C_SHA_TYPE_256);//use sha256 for demo

    RetValue = L2CShaUpdate((uint32_t)ShaInput, (uint32_t)ShaDigest, SHA_DATA_LEN, 1);

    printf("sha digest part1 is 0x%x 0x%x 0x%x 0x%x \r\n",ShaDigest[0],ShaDigest[1],ShaDigest[2],ShaDigest[3]);
    printf("sha digest part2 is 0x%x 0x%x 0x%x 0x%x \r\n",ShaDigest[4],ShaDigest[5],ShaDigest[6],ShaDigest[7]);


    printf("sha demo end\r\n");



    printf("aes demo start\r\n");
    for(i=0;i<AES_DATA_LEN;i++)
    {
        AesInput[i]=i;//could be initialised to any other value
    }
    for(i=0;i<AES_KEY_LEN;i++)
    {
        AesKey[i]=i;//could be initialised to any other value
    }

    ConvertEndian(192,AesKey);

    
    L2CTlsAesStruct AesInfo;
    AesInfo.IVAddress =0;
    AesInfo.InputData = (uint32_t)(&AesInput[0]);
    AesInfo.OutputData = (uint32_t)(&AesEncyptOutput[0]);
    AesInfo.KeyAddress = (uint32_t)(&AesKey[0]);
    AesInfo.DataLen = AES_DATA_LEN;
    AesInfo.HeadLen = 0;    
    AesInfo.Ctrl.Encrypt = 0;//encrypt
    AesInfo.Ctrl.ChainMode = 0;//ecb
    AesInfo.Ctrl.PaddingMode = 0;//no padding
    AesInfo.Ctrl.KeySize = 1;//192bits

    RetValue = L2CTlsAesProcess(&AesInfo);

    printf("aes encypt is done!!\r\n");

    
    AesInfo.IVAddress =0;
    AesInfo.InputData = (uint32_t)(&AesEncyptOutput[0]);
    AesInfo.OutputData = (uint32_t)(&AesDecyptOutput[0]);
    AesInfo.KeyAddress = (uint32_t)(&AesKey[0]);
    AesInfo.DataLen = AES_DATA_LEN;
    AesInfo.HeadLen = 0;    
    AesInfo.Ctrl.Encrypt = 1;//decrypt
    AesInfo.Ctrl.ChainMode = 0;//ecb
    AesInfo.Ctrl.PaddingMode = 0;//no padding
    AesInfo.Ctrl.KeySize = 1;//192bits

    RetValue = L2CTlsAesProcess(&AesInfo);

    RetValue = memcmp(AesInput, AesDecyptOutput, AES_DATA_LEN);
    if(RetValue==0)
    {
        printf("aes encypt/decrpt ok!!\r\n");
        printf("AesDecyptOutput  is 0x%x 0x%x 0x%x 0x%x \r\n",AesDecyptOutput[0],AesDecyptOutput[1],AesDecyptOutput[2],AesDecyptOutput[3]);

    }
    else
    {
        printf("aes encypt/decrpt error!!\r\n");

        printf("AesInput is %d %d %d %d \r\n",AesInput[0],AesInput[1],AesInput[2],AesInput[3]);
        printf("AesDecyptOutput  is %d %d %d %d \r\n",AesDecyptOutput[0],AesDecyptOutput[1],AesDecyptOutput[2],AesDecyptOutput[3]);
        printf("AesEncyptOutput is %d %d %d %d \r\n",AesEncyptOutput[0],AesEncyptOutput[1],AesEncyptOutput[2],AesEncyptOutput[3]);

    }

    printf("aes demo end\r\n");

    L2CTlsDeInit();
   
}

