/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_coap_cnf_ind.h
*
*  Description: COAP related AT CMD RESP/URC code header
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef __ATEC_COAP_CNF_IND_H__
#define __ATEC_COAP_CNF_IND_H__

#include "at_util.h"

#define COAP_RSP_BUFF_LEN_OFFSET    128
#define COAP_RSP_BUFF_LEN           256
#define COAP_RSP_TEMP_BUFF_LEN      128


CmsRetId coapCreateCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId coapDeleteCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId coapAddresCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId coapHeadCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId coapOptionCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId coapSendCnf(UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId coapConfigCnf(UINT16 reqHandle, UINT16 rc, void *paras);

CmsRetId coapRecvInd(UINT16 indHandle, void *paras);

void atApplCoapProcCmsCnf(CmsApplCnf *pCmsCnf);
void atApplCoapProcCmsInd(CmsApplInd *pCmsInd);


#endif

