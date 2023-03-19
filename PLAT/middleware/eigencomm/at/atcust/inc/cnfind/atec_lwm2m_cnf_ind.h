/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_lwm2m_cnf_ind_api.h
*
*  Description: Process LWM2M client related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef _ATEC_LWM2M_CNF_IND_API_H
#define _ATEC_LWM2M_CNF_IND_API_H

#include "at_util.h"

void atApplLwm2mProcCmsCnf(CmsApplCnf *pCmsCnf);
void atApplLwm2mProcCmsInd(CmsApplInd *pCmsInd);

#endif

