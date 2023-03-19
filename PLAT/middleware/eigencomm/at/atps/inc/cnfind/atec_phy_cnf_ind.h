/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_phy_cnf_ind.h
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef __ATEC_PHY_CNF_IND_H__
#define __ATEC_PHY_CNF_IND_H__

#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "at_util.h"
#include "atec_phy.h"
#include "ctype.h"
#include "cmicomm.h"

CmsRetId phySetPhyInfoCnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId phyGetPhyInfoCnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId phyGetPhyAllInfoCnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);
CmsRetId phyECPHYCFGcnf(UINT16 primId, UINT16 reqHandle, UINT16 rc, void *paras);

#endif

