#ifndef _ESIM_COS_H_
#define _ESIM_COS_H_

#include "esim_types.h"

uint16 esim_reset_entry(uint8 *output, uint16* output_len);
uint16 apdu_process_entry(uint8 *input, uint16 input_len, uint8 *output, uint16 *output_len);

#endif
