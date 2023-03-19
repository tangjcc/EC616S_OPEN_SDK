#ifndef _CRC_H
#define _CRC_H
 

#ifdef __cplusplus
extern "C" {
 
#endif

unsigned int calc_crc(ML_U8* pbuf, int len);
ML_BOOL generate_file_crc(ML_U32 *p_crc, ML_U32 *p_nsize, char *file, int type);

#ifdef __cplusplus
 
}
#endif
 
#endif //_CRC_H
