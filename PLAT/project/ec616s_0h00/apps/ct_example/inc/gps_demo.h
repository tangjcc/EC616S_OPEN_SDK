/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.h
 * Description:  EC616S China Telecomm NB-IOT demo app header file
 * History:      Rev1.0   2020-03-24
 *
 ****************************************************************************/
#include "gps_proc.h"

#define GPS_RECV_BUFFER_LEN     (128)
extern uint8_t gpsDataBuffer[];
extern uint32_t gpsDataCount;

void gpsApiInit(void);
 
void GpsDataRecvProcessing(uint8_t *rawData, uint32_t rawDataLen);
void GpsGetData(GPS_Date **date, GPS_Location **loc);
void GpsGetDateTimeString(char *out, int maxLen, int *actLen);
void GpsGetLocationString(char *out, int maxLen, int *actLen);


