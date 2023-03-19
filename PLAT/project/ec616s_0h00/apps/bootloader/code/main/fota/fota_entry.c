/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename:
*
*  Description:
*
*  History: initiated by xuwang
*
*  Notes:
*
******************************************************************************/

#ifdef MIDDLEWARE_FOTA_ENABLE

extern void FOTA_mainProc(void);


void FotaProcedure(void)
{
    FOTA_mainProc();
}



#endif


