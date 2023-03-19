/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: at_ref_entity.h
*
*  Description: Process protocol related AT CMD
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef __AT_REF_ENTITY_H__
#define __AT_REF_ENTITY_H__

#include "at_util.h"
#include "cmicomm.h"



/******************************************************************************
 *****************************************************************************
 * EXTERNAL API
 *****************************************************************************
******************************************************************************/


/*
 * process: SIG_CAC_CMI_CNF
*/
CmsRetId atRefProcCmiCnf(CacCmiCnf *pCmiCnf);

/*
 * process: SIG_CAC_CMI_IND
*/
CmsRetId atRefProcCmiInd(CacCmiInd *pCmiInd);

/*
 * process: SIG_CMS_APPL_CNF
*/
CmsRetId atRefProcApplCnf(CmsApplCnf *pApplCnf);

/*
 * process: SIG_CMS_APPL_IND
*/
CmsRetId atRefProcApplInd(CmsApplInd *pApplInd);

#endif

