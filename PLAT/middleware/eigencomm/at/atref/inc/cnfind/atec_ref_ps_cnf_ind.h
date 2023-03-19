/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_ref_ps_cnf_ind.h
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef __ATEC_REF_PS_CNF_IND_H__
#define __ATEC_REF_PS_CNF_IND_H__

#include "cmicomm.h"




void atRefDevProcCmiCnf(CacCmiCnf *pCmiCnf);
void atRefMmProcCmiCnf(CacCmiCnf *pCmiCnf);
void atRefPsProcCmiCnf(CacCmiCnf *pCmiCnf);
void atRefSimProcCmiCnf(CacCmiCnf *pCmiCnf);
void atRefSmsProcCmiCnf(CacCmiCnf * pCmiCnf);

void atRefDevProcCmiInd(CacCmiInd *pCmiInd);
void atRefMmProcCmiInd(CacCmiInd *pCmiInd);
void atRefPsProcCmiInd(CacCmiInd *pCmiInd);
void atRefSimProcCmiInd(CacCmiInd *pCmiInd);
void atRefSmsProcCmiInd(CacCmiInd *pCmiInd);


#endif

