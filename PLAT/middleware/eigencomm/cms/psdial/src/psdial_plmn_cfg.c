/******************************************************************************
 ******************************************************************************
 Copyright:      - 2017- Copyrights of EigenComm Ltd.
 File name:      - psdial_plmn_cfg.c
 Description:    - PLMN static configuration
 History:        - 06/26/2018, Originated by jcweng
 ******************************************************************************
******************************************************************************/
#include "psdial_plmn_cfg.h"


/******************************************************************************
 ******************************************************************************
 GOLBAL VARIABLES
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_VARIABLES__ //just for easy to find this position in SS


const DialPlmnCfgInfo    dialPlmnCfgList[] =
{
    /*1 PLMN            2 eitf  3 ipType            4 APN       5 pShortName,   6 pLongName             7 pBand,*/
    /*8 pDefIp4Dns1,    9 pDefIp4Dns2,              10 pDefIp6Dns1,             11 pDefIp6Dns2*/
    {{0x001, 0x001},    FALSE,  DIAL_CFG_IPV4V6,    PNULL,      PNULL,          PNULL,                  {0,0,0,0,0,0,0,0},
     PNULL,             PNULL,                      PNULL,                      PNULL},                 //TEST SIM CARD

    /*CMCC*/
    {{0x460, 0x000},    FALSE,  DIAL_CFG_IPV4V6,    "cmnbiot",  "CMCC",         "CHINA MOBILE",         {8,0,0,0,0,0,0,0},
    "211.136.192.6",  "221.130.13.133",             PNULL,                      PNULL},
    {{0x460, 0x002},    FALSE,  DIAL_CFG_IPV4V6,    "cmnbiot",  "CMCC",         "CHINA MOBILE",         {8,0,0,0,0,0,0,0},
    "211.136.192.6",  "221.130.13.133",             PNULL,                      PNULL},
    {{0x460, 0x004},    FALSE,  DIAL_CFG_IPV4V6,    "cmnbiot",  "CMCC",         "CHINA MOBILE",         {8,0,0,0,0,0,0,0},
    "211.136.192.6",  "221.130.13.133",             PNULL,                      PNULL},
    {{0x460, 0x007},    FALSE,  DIAL_CFG_IPV4V6,    "cmnbiot",  "CMCC",         "CHINA MOBILE",         {8,0,0,0,0,0,0,0},
    "211.136.192.6",  "221.130.13.133",             PNULL,                      PNULL},
    {{0x460, 0x008},    FALSE,  DIAL_CFG_IPV4V6,    "cmnbiot",  "CMCC",         "CHINA MOBILE",         {8,0,0,0,0,0,0,0},
    "211.136.192.6",  "221.130.13.133",             PNULL,                      PNULL},

    /*CTCC*/
    {{0x460, 0x003},    TRUE,   DIAL_CFG_IPV4V6,    "ctnb",     "CTCC",         "CHINA TELECOM",        {5,3,0,0,0,0,0,0},
    "218.4.4.4",        "218.2.2.2",                PNULL,                      PNULL},
    {{0x460, 0x005},    TRUE,   DIAL_CFG_IPV4V6,    "ctnb",     "CTCC",         "CHINA TELECOM",        {5,3,0,0,0,0,0,0},
    "218.4.4.4",        "218.2.2.2",                PNULL,                      PNULL},
    {{0x460, 0x011},    TRUE,   DIAL_CFG_IPV4V6,    "ctnb",     "CTCC",         "CHINA TELECOM",        {5,3,0,0,0,0,0,0},
    "218.4.4.4",        "218.2.2.2",                PNULL,                      PNULL},

    /*CUCC*/
    {{0x460, 0x001},    FALSE,  DIAL_CFG_IPV4,      "",         "CUCC",         "CHINA UNICOMM",        {8,3,0,0,0,0,0,0},
    "210.22.70.3",      "210.22.84.3",              PNULL,                      PNULL},
    {{0x460, 0x006},    FALSE,  DIAL_CFG_IPV4,      "",         "CUCC",         "CHINA UNICOMM",        {8,3,0,0,0,0,0,0},
    "210.22.70.3",      "210.22.84.3",              PNULL,                      PNULL},


    /*END*/
    {{0x000, 0x000},    FALSE,  DIAL_CFG_IPV4,      "",         "",             "",                     {0,0,0,0,0,0,0,0},
    PNULL,              PNULL,                      PNULL,                      PNULL},    //END
};


/******************************************************************************
 ******************************************************************************
 static functions
 ******************************************************************************
******************************************************************************/
#define __DEFINE_STATIC_FUNCTION__ //just for easy to find this position in SS




/******************************************************************************
 ******************************************************************************
 External functions
 ******************************************************************************
******************************************************************************/
#define __DEFINE_GLOBAL_FUNCTION__ //just for easy to find this position in SS

/**
  \fn          DialPlmnCfgInfo* psDialGetPlmnCfgByPlmn(UINT16 mcc, UINT16 mnc)
  \brief       Get PLMN config info by PLMN: mcc&mnc
  \param[in]   mcc
  \param[in]   mnc
  \returns     const DialPlmnCfgInfo*
*/
const DialPlmnCfgInfo* psDialGetPlmnCfgByPlmn(UINT16 mcc, UINT16 mnc)
{
    const DialPlmnCfgInfo *pPlmnCfg = PNULL;

    if (mcc == 0 && mnc == 0)
    {
        return PNULL;
    }

    /*
     * find the "DialPlmnCfgInfo" by mcc & mnc
    */
    pPlmnCfg = &(dialPlmnCfgList[0]);
    while (pPlmnCfg->plmn.mcc != 0 ||
           pPlmnCfg->plmn.mnc != 0)
    {
        /* if 2-digit MNC type, the 4 MSB bits should set to 'F' */
        if (pPlmnCfg->plmn.mcc == mcc &&
            (pPlmnCfg->plmn.mnc & 0xFFF) == (mnc & 0xFFF))
        {
            return pPlmnCfg;
        }

        pPlmnCfg++;
    }

    return PNULL;
}

