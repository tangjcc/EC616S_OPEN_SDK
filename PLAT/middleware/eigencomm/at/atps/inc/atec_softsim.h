
/******************************************************************************

*(C) Copyright 2020 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: 
*
*  Description: esim AT commands interface function
*
*  History:     09/22/2020  yjwang create
*
*  Notes:    NULL
*
******************************************************************************/


#ifndef _ATEC_CUST_SOFTSIM_H_
#define _ATEC_CUST_SOFTSIM_H_


/* AT+ESIMSWITCH */
#define ATC_ESIMSWITCH_0_FUN_VAL_MIN                0
#define ATC_ESIMSWITCH_0_FUN_VAL_MAX                1
#define ATC_ESIMSWITCH_0_FUN_VAL_DEFAULT            0


CmsRetId esimSWITCH(const AtCmdInputContext *pAtCmdReq);

#endif


