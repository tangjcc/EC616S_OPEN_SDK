#ifndef __IMAGE_H__
#define __IMAGE_H__

//#include "ARMCM3.h"
#include "ec616s.h"
#include "core_cm3.h"
//#pragma pack(1)

#define TIMIDENTIFIER		0x54494D48		// "TIMH"
#define OBMIDENTIFIER		0x4F424D49		// "OBMI"
#define DKBIDENTIFIER		0x444B4249		// "DKBI"
#define WTPRESERVEDAREAID	0x4F505448      // "OPTH"
#define DOWNLOAD_EXEC 0
#define DOWNLOAD_BURN 1
#define SHA256_NONE  0xee

typedef struct {
    uint32_t Version;
    uint32_t Identifier;
    uint32_t Date;
    uint32_t OEMReserve;
}VersionInfo;

typedef struct {
    uint32_t ImageId;
    uint32_t BurnAddr; //Flash addr
    uint32_t LoadAddr;
    uint32_t ImageSze;
    uint32_t Reserved[4];
    uint8_t  Hash[32]; //hash 256
    uint8_t ECDSASign[64];
    uint8_t PublicKey[64];//reserved for test
}ImageBodyInfo;

typedef struct {
    uint32_t ReservedAreaId;
    uint32_t ReservedSize;
    uint32_t Reserved[2];    
}ReservedArea;

typedef struct {
    VersionInfo Version;
    uint32_t ImageNum;
    uint32_t Type; //bit7-0, hashih type: 0xee-none, other-sha256
    uint32_t reserved[2];
    uint8_t  HashIH[32];
    ImageBodyInfo ImgBodyInfo;
    ReservedArea RsvArea;  //point to reaserved area at tail of ImageHead
}ImageHead;

typedef struct DownloadCtrl
{
    uint32_t type;
}DownloadCtrl;

extern uint32_t LoadVerifyImageHead(void);
extern uint32_t LoadIVerifymageBody(void);

extern ImageHead GImageHead;
//0x00800000----0x00820000 reserved 128KB for OBM
//BLIMG_HEAD_FLASH_ENTRY_ADDR     0x00800000 - 0x00802000   8k
//SYSIMG head MAP  4k + 4k 
////SYSIMG1_HEAD_FLASH_ENTRY_ADDR     0x00802000 - 0x00803000   4k
////SYSIMG2_HEAD_FLASH_ENTRY_ADDR     0x00803000 - 0x00804000   4k

//BLIMG_BODY_FLASH_ENTRY_ADDR     0x00804000 - 0x00820000  112k

//0x00820000----0x00C00000 reserved 4M-128K for system
//SYSIMG_BODY_FLASH_ENTRY_ADDR  0x00820000 - 0x00C00000   4M-128K

//LOAD BUFFER FOR VERIFY
//IMG_BODY_LOAD_ADDR                0x00020000 - 0x00038000 96k
//IMG_HEAD_LOAD_ADDR                0x00038000 - 0x0003a000 8k

//#define REMAP_MEMORY

#ifndef REMAP_MEMORY
#define IRAM_IMAGE_LOAD_BASE 0x00020000
#else
#define IRAM_IMAGE_LOAD_BASE 0x00420000
#endif
#define IMAGE_HEAD_UNIT_SIZE 0x1000 //4k unit size for image head
#define BLIMG_HEAD_AREA_SIZE (IMAGE_HEAD_UNIT_SIZE+IMAGE_HEAD_UNIT_SIZE)  //8k unit size for bl image head area
#define SYSIMG_HEAD_AREA_SIZE (IMAGE_HEAD_UNIT_SIZE+IMAGE_HEAD_UNIT_SIZE)  //8k unit size for sys image head area

//LOAD BUFFER AREA
#define IMG_HEAD_LOAD_ADDR (IRAM_IMAGE_LOAD_BASE)
#define IMG_HEAD_MAX_SIZE  (BLIMG_HEAD_AREA_SIZE) //8k

#define IMG_BODY_LOAD_ADDR (IMG_HEAD_LOAD_ADDR+IMG_HEAD_MAX_SIZE)
#define IMG_BODY_LOAD_BUF_MAX_SIZE 0x018000 //96K

#define IRAM_IMAGE_LOAD_LIMIT (IMG_HEAD_MAX_SIZE+IMG_BODY_LOAD_BUF_MAX_SIZE)

//AGENT IMAGE
#define AGIMG_BODY_MAX_SIZE 0x18000  //96K 

//IMAGE AREA
#define FLASH_BASE_ADDR 0x00800000

//IMAGE OFFSET
//spi rw addr, for security boot verify system img
#define BLIMG_HEAD_FLASH_OFFSET_ADDR  0
#define SYSIMG_HEAD_FLASH_OFFSET_ADDR  (BLIMG_HEAD_FLASH_OFFSET_ADDR+BLIMG_HEAD_AREA_SIZE)
#define BLIMG_BODY_FLASH_OFFSET_ADDR  (BLIMG_HEAD_AREA_SIZE + SYSIMG_HEAD_AREA_SIZE)  //8k + 8k 
#define SYSIMG_BODY_FLASH_OFFSET_ADDR  (0x20000)

//BOOTLOADER IMAGE
#define BLIMG_BODY_MAX_SIZE (0x400000- 0x4000)  // 4M - 16K
#define BLIMG_HEAD_FLASH_ENTRY_ADDR (FLASH_BASE_ADDR)
#define BLIMG_BODY_FLASH_ENTRY_ADDR (FLASH_BASE_ADDR+BLIMG_BODY_FLASH_OFFSET_ADDR) // flash base + 8k bl head area + ((4k sys1 head + 4k sys1 head) or 8k sys head)

//SYSTEM IMAGE
#define SYSIMG_HEAD_FLASH_ENTRY_ADDR (FLASH_BASE_ADDR+SYSIMG_HEAD_FLASH_OFFSET_ADDR) 
//#define SYSIMG_HEAD_MAX_SIZE 0x2000  

#define SYSIMG_BODY_MAX_SIZE 0x03e0000    ////4 MB -128K
#define SYSIMG_BODY_FLASH_ENTRY_ADDR (FLASH_BASE_ADDR+SYSIMG_BODY_FLASH_OFFSET_ADDR)

uint32_t LoadVerifyImageHead(void);
uint32_t LoadVerifyImageBody(void);


#endif
