/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of EigenComm Ltd.
 * File name:    hal_misc.c
 * Description:  EC616 hal for misc API
 * History:      Rev1.0   2019-12-12
 *
 ****************************************************************************/

#include "cmsis_os2.h"
#include "hal_misc.h"
#include "debug_log.h"


extern uint32_t GPR_GetChipFullID(void);
extern uint32_t GPR_GetChipRevID(void);
extern uint32_t GPR_GetChipID(void);


/**
  \breif Get chip id
  \param[in] input     HAL_Get_ChipID enum
  \return              chip id value
  \details
   31-----8 is chip id
   7------0 is revision id
   sel=CHIP_ID_ONLYID, return chip id
   sel=CHIP_ID_REVID,  return revision id
   sel=CHIP_ID_FULL, return both revision and chip id
 */
uint32_t HAL_Get_ChipID(chip_id_sel sel)
{
   uint32_t chipID=0;


   switch(sel)
   {
	   case CHIP_ID_ONLYID:
   
		   chipID=GPR_GetChipID();
		   break;
		   
	   case CHIP_ID_REVID:
	       chipID=GPR_GetChipRevID();
		   break;
	
	   case CHIP_ID_FULLID:
   
		   chipID=GPR_GetChipFullID();
		   break;
	   
	   default:
	   	   break;
   	}

   //ECOMM_TRACE(UNILOG_PLA_DRIVER, HAL_Get_ChipID, P_INFO, 1, "chip id is 0x%x",chipID);
	
   return chipID;

}

