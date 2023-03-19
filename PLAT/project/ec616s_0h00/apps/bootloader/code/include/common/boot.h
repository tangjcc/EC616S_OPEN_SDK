#ifndef __BOOTROM_H__
#define __BOOTROM_H__

#define RESET_STAT_NORMAL 0 
#define RESET_STAT_UNDEF 1
//#include "ARMCM3.h"
#include "core_cm3.h"

typedef struct {
    uint32_t Reserved; //to be fixed
}FuseInfo;

#define ECDSA_PUBLIC_KEY_LEN 64 //BYTES
typedef struct {
    //uint32_t BRDate;
    //uint32_t BRVersion;
    uint32_t PlatformType;
    uint32_t PlatformSubType;
    uint32_t ResetStat;
    FuseInfo Fuse;    
    uint32_t Error;
    uint32_t TransferAddr;
    uint8_t SecurityInitialized;
    uint8_t JTAGDisable;
    uint8_t SecureBootEnabled;    
    uint8_t DbgDisable;
    uint8_t DownloadDisable;
    uint8_t Reserved[3];    
    uint8_t ECDSAPublickKey[ECDSA_PUBLIC_KEY_LEN];
}BRInfo, *PBRInfo;

extern PBRInfo GetBRInfo(void);
extern BRInfo GBRInfo;

#define MONTH (__DATE__[2] == 'n' ? 1\
    : __DATE__[2] == 'b' ? 2 \
    : __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 3 : 4) \
    : __DATE__ [2] == 'y' ? 5 \
    : __DATE__ [2] == 'n' ? 6 \
    : __DATE__ [2] == 'l' ? 7 \
    : __DATE__ [2] == 'g' ? 8 \
    : __DATE__ [2] == 'p' ? 9 \
    : __DATE__ [2] == 't' ? 0x10 \
    : __DATE__ [2] == 'v' ? 0x11 : 0x12)

#define DAY ((__DATE__ [4] == ' ' ? 0 : __DATE__ [4] - '0') <<4 | (__DATE__ [5] - '0'))

#define YEAR  ((__DATE__[7]-'0')<<12 |(__DATE__[8]-'0')<<8|(__DATE__[9]-'0')<<4|(__DATE__[10]-'0'))

#define BR_DATE  (YEAR<<16 | DAY<<8 | MONTH)

#define BRVerMajorMinor 0x01000001
#define BRBuildNum 0x00000001  //relative to svn update version

#define BOOT_MODE_GPIO 6 //GPIO 6

#endif
