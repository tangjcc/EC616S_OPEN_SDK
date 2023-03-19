#ifndef _ESIM_TA_H_
#define _ESIM_TA_H_

#include "esim_types.h"

uint16 esim_process_entry(uint8 function, uint8 *input, uint16 in_len, uint8 *output, uint16 *out_len);

#endif
