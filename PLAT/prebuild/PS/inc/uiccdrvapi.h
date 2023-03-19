#ifndef __UICC_DRV_API_H__
#define __UICC_DRV_API_H__
/******************************************************************************
Copyright:      - 2017, All rights reserved by Eigencomm
File name:      - uiccdrvapi.h
Description:    - provide Uicc driver api.
Function List:  -
History:        - 05/20/2020, Originated by xlhu
******************************************************************************/

/*********************************************************************************
* Includes
*********************************************************************************/
#include "pssys.h"

/*********************************************************************************
* Macros
*********************************************************************************/

/*********************************************************************************
* Type Definition
*********************************************************************************/
/*
 * PSAM return code
*/
typedef enum PsamRetCode_enum
{
    PSAM_RET_SUCC           = 0,
    PSAM_INVALID_PARAM      = 1,
    PSAM_OPER_NOT_SUPPROT   = 2, //operation not supported
    PSAM_ACT_FAILURE        = 3, //PSAM activate failure
    PSAM_APDU_FAILURE       = 4, //APDU command executed faied
    PSAM_BUSY               = 5,  //PSAM busy, cannot process any instruction
    PSAM_SEMP_ERROR         = 6, //Semaphore error
    PSAM_RET_UNKNOWN        = 0xFF,
}_PsamRetCode;

typedef UINT8 PsamRetCode;




/*
* GPIO parameters
* e.g. GPIO17
* .gpioInstance = 1 (17/16 )
* .gpioPin = 1 (17 % 16)
* .padIndex = 27
* .padAltFunc = PAD_MuxAlt0
*/
typedef struct GpioParam_Tag
{
    UINT8           gpioInstance;
    UINT8           gpioPin;
    UINT8           padIndex;//pad address
    UINT8           padAltFunc;//PAD pin mux
}
GpioParam;




/******************************************************************************
 *****************************************************************************
 * API
 *****************************************************************************
******************************************************************************/
#define __DECLARE_PSAM_FUNCTION_ //just for easy to find this position in SS

/**
  \fn          void UiccPsamConfigure
  \brief       Configure the switch IO and latch IO. The switch IO used GPIO to control
               switch bus for USIM and PSAM. The latch IO used AONIO to keep PSAM clock
               on H or L when enter clock stop mode and sleep 1 mode.
  \param[in]   GpioParam *pSwitchIO, the pointer to the GPIO paramter for switch
  \param[in]   GpioParam *pPsamLatchIO, the pointer to the AONIO paramter for PSAM clock latch.
  \returns     null
  \NTOE:
*/
void UiccPsamConfigure(GpioParam *pSwitchIO, GpioParam *pPsamLatchIO);

/**
  \fn          PsamRetCode ActivatePsamCard
  \brief       Activate the PSAM by cold reset and process ATR.
  \param[in]   null
  \param[out]  null
  \returns     PsamRetCode
  \NTOE:
*/
PsamRetCode ActivatePsamCard(void);


/**
  \fn          PsamRetCode PsamApduAccess
  \brief       PSAM APDU access.
  \param[in]   UINT16 cmdLength, the length of the command APDU
  \param[in]   UINT8 *cmdApdu, the pointer to the command APDU
  \param[out]  UINT16 *rspLength, the pointer to the length of the response APDU
  \param[out]  UINT8 *rspApdu, the pointer to the response APDU
  \returns     PsamRetCode
  \NTOE:
*/
PsamRetCode PsamApduAccess(UINT16 cmdLength, UINT8 *cmdApdu, UINT16 *rspLength, UINT8 *rspApdu);


/**
  \fn          void TerminatePsamCard
  \brief       Terminate the PSAM card.
  \param[in]   null
  \param[out]  null
  \returns     null
  \NTOE:
*/
void TerminatePsamCard(void);


/**
 \ A example for PSAM access
 \ run in app task
 void PsamAccessTest(void)
{
    PsamRetCode rc = PSAM_RET_SUCC;
    UINT8 selectAdf[] = {0x00, 0xa4, 0x04, 0x00, 0x0c, 0xa0, 0x00, 0x00, 0x00,0x03, 0x4b, 0x4c, 0x47, 0x41, 0x53, 0x30, 0x31};
    UINT16 rspLen = 0;
    UINT8 rspApdu[261] = {0};
    UINT8 getRand1[] = {0x00, 0x84, 0x00, 0x00, 0x08};
    UINT8 getRand2[] = {0x00, 0x84, 0x00, 0x00, 0x10};

    rc = ActivatePsamCard();
    if (rc == PSAM_RET_SUCC)
    {
        ECOMM_TRACE(UNILOG_PLA_APP, PsamAccessTest_1, P_INFO, 0, "PSAM activate ok");
    }
    else
    {
        ECOMM_TRACE(UNILOG_PLA_APP, PsamAccessTest_2, P_INFO, 0, "PSAM activate failure");
    }

    rc = PsamApduAccess(sizeof(selectAdf), selectAdf, &rspLen, rspApdu);
    if (rc == PSAM_RET_SUCC)
    {
        ECOMM_DUMP(UNILOG_PLA_DUMP, PsamAccessTest_3, P_INFO, "select ADF return: ", rspLen, rspApdu);
    }
    else
    {
        ECOMM_TRACE(UNILOG_PLA_APP, PsamAccessTest_4, P_INFO, 0, "PSAM APDU access failure");
    }

    rspLen = 0;
    memset(rspApdu, 0, sizeof(rspApdu));
    rc = PsamApduAccess(sizeof(getRand1), getRand1, &rspLen, rspApdu);
    if (rc == PSAM_RET_SUCC)
    {
        ECOMM_DUMP(UNILOG_PLA_DUMP, PsamAccessTest_5, P_INFO, "get rand return: ", rspLen, rspApdu);
    }
    else
    {
        ECOMM_TRACE(UNILOG_PLA_APP, PsamAccessTest_6, P_INFO, 0, "PSAM APDU access failure");
    }

    rspLen = 0;
    memset(rspApdu, 0, sizeof(rspApdu));
    rc = PsamApduAccess(sizeof(getRand2), getRand2, &rspLen, rspApdu);
    if (rc == PSAM_RET_SUCC)
    {
        ECOMM_DUMP(UNILOG_PLA_DUMP, PsamAccessTest_7, P_INFO, "get rand return: ", rspLen, rspApdu);
    }
    else
    {
        ECOMM_TRACE(UNILOG_PLA_APP, PsamAccessTest_8, P_INFO, 0, "PSAM APDU access failure");
    }

    TerminatePsamCard();
}

*/

#define __DECLARE_SOFTSIM_FUNCTION_ //just for easy to find this position in SS
/**
  \brief       definition of soft sim reset callback function.
  \param[out]   unsigned short *atrLen, the pointer to the length of ATR
  \param[out]   unsigned char *atrData, the pointer to the ATR data.
  \returns     null
  \NTOE:
*/
typedef void(* SoftSimResetCb_t)(UINT16 *atrLen, UINT8 *atrData);

/**
  \brief       definition of soft sim APDU request callback function.
  \param[in]   unsigned short txDataLen, the length of tx data
  \param[in]   unsigned char *txData, the pointer to the tx data.
  \param[out]   unsigned short *rxDataLen, the pointer to the length of rx data
  \param[out]   unsigned char *rxData, the pointer to the rx data.
  \returns     null
  \NTOE:
*/
typedef void(* SoftSimApduReqCb_t)(UINT16 txDataLen, UINT8 *txData, UINT16 *rxDataLen, UINT8 *rxData);

/**
  \brief definition of register callback
  \param[in]   SoftSimResetCb_t resetCb, the pointer to softsim reset func
  \param[in]   SoftSimApduReqCb_t apduReqCb, the pointer to softsim apdu request func.
 */
void EcSoftSimRegisterCallbackFunc(SoftSimResetCb_t resetCb, SoftSimApduReqCb_t apduReqCb);




#endif

