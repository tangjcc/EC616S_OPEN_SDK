/******************************************************************************

*(C) Copyright 2018 EIGENCOMM International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_tcpip_cnf_ind.h
*
*  Description: Process TCP/IP service related AT commands
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef _ATEC_TCPIP_CNF_IND_API_H
#define _ATEC_TCPIP_CNF_IND_API_H

#include "at_util.h"

void atTcpIpProcRefNmApplCnf(CmsApplCnf *pApplCnf);
void atTcpIpProcRefNmApplInd(CmsApplInd *pApplInd);


#endif

