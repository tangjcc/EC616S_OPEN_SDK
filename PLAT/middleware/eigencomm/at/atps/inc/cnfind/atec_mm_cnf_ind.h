/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_mm_cnf_ind.h
*
*  Description: Process Network related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef _ATEC_MM_CNF_IND_API_H
#define _ATEC_MM_CNF_IND_API_H

#include "at_util.h"

#define CPSMS_BUF_LEN 64

void atMmProcCmiCnf(CacCmiCnf *pCmiCnf);
void atMmProcCmiInd(CacCmiInd *pCmiInd);


#endif
