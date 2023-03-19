#ifndef __EC_SOFTSIM_APT_H__
#define __EC_SOFTSIM_APT_H__
/******************************************************************************
Copyright:      - 2017, All rights reserved by Eigencomm Ltd.
File name:      - ecsoftsimapt.h
Description:    - the header file for softsim adapter API.
Function List:  -
History:        - 09/15/2020, Originated by xlhu
******************************************************************************/

/*********************************************************************************
* Includes
*********************************************************************************/

/*********************************************************************************
* Macros
*********************************************************************************/
#define RESULT_OK            0
#define RESULT_ERROR         -1

/*********************************************************************************
* Type Definition
*********************************************************************************/


/******************************************************************************
 *****************************************************************************
 * Functions
 *****************************************************************************
******************************************************************************/
void SoftSimInit(void);
void SoftSimReset(UINT16 *atrLen, UINT8 *atrData);
void SoftSimApduReq(UINT16 txDataLen, UINT8 *txData, UINT16 *rxDataLen, UINT8 *rxData);
INT8 SoftsimReadRawFlash(UINT32 addr, UINT8 *pBuffer, UINT32 bufferSize);
INT8 SoftsimEraseRawFlash(UINT32 sectorAddr, UINT32 size);
INT8 SoftsimWriteToRawFlash(UINT32 addr, UINT8 *pBuffer, UINT32 bufferSize);
extern UINT32 SOFTSIM_RAWFLASH_SECTOR_BASE_ADDR(UINT8 index);


#endif


