/******************************************************************************
 ******************************************************************************
 Copyright:      - 2020- Copyrights of EigenComm Ltd.
 File name:      - rohcutil.h
 Description:    - rohc util header file
 History:        - 04/10/2020, Originated by xwang
 ******************************************************************************
******************************************************************************/


#ifndef ROHC_UTIL_H
#define ROHC_UTIL_H

#include "commontypedef.h"
#include "pssys.h"
#include "debug_log.h"



/******************************************************************************
 *****************************************************************************
 * MARCO
 *****************************************************************************
******************************************************************************/

#ifndef ROHC_UNUSED_ARG
#define ROHC_UNUSED_ARG(x) (void)x
#endif

//RFC 3095 5.9.1
#define ROHC_CRC8_POLYNOMIAL 0xE0  //C(x) = 1 + x + x2 + x8

#if 0
//RFC 3095 5.9.2
#define ROHC_CRC3_POLYMOMIAL        //C(x) = 1 + x + x3
#define ROHC_CRC7_POLYNOMIAL        //C(x) = 1 + x + x2 + x3 + x6 + x7
#endif



/******************************************************************************
 *****************************************************************************
 * STRUCT/ENUM
 *****************************************************************************
******************************************************************************/
typedef struct RohcList_Tag{
    struct RohcList_Tag *pre;
    struct RohcList_Tag *next;
    UINT8 private[];
}RohcList;


/******************************************************************************
 *****************************************************************************
 * FUNCTION
 *****************************************************************************
******************************************************************************/
RohcList * RohcNewList(UINT32 privateLen);
void RohcFreeList(RohcList *list);
void RohcInsertList(RohcList *listHeader, RohcList *newList);
void RohcRemoveList(RohcList *header, RohcList *list);
void *RohcGetListBody(RohcList *list);
RohcList *RohcGetListByBody(void *body);
UINT32 RohcGetCurrentSysTime(void);
INT32 RohcGetRandomNum(void);
BOOL RohcCheckListOwner(RohcList *header, RohcList *list);
UINT8 RohcGernCrc8(UINT8 *data, UINT16 dataLen);


#endif

