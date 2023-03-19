/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_http_cnf_ind.h
*
*  Description: Process http(s) client related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef _ATEC_HTTP_CNF_IND_H_
#define _ATEC_HTTP_CNF_IND_H_

#include "at_util.h"

CmsRetId httpConnCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId httpSendCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId httpDestoryCnf(UINT16 reqHandle, UINT16 rc, void *paras);

void atApplHttpProcCmsCnf(CmsApplCnf *pCmsCnf);
void atApplHttpProcCmsInd(CmsApplInd *pCmsInd);

#endif

