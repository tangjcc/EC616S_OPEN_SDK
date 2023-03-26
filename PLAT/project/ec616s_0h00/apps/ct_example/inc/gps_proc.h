
#include <stdio.h>
#include <string.h>

typedef struct {
	unsigned int Year;
	unsigned int Month;
	unsigned int Day;
	unsigned char Flag_OV;
	unsigned int Hour;
	unsigned int Min;
	unsigned int Sec;
}GPS_Date;

typedef struct {
	unsigned char jingduFlag;
	unsigned int jingdu;
	unsigned int jingduMin;
	unsigned int jingduSec;
	unsigned char weiduFlag;
	unsigned int weidu;
	unsigned int weiduMin;
	unsigned int weiduSec;
	unsigned int altitude;
	unsigned int satNum;
}GPS_Location;

#define GPGGA_STR "GPGGA"
#define GPRMC_STR "GPRMC"

#define GPS_DATA_HEAD_LEN 5

unsigned char GPS_IS_GPGGA_DATA(unsigned char *rawData);
unsigned char GPS_IS_GPRMC_DATA(unsigned char *rawData);

void GPS_GPGGA_CAL(unsigned char *rxBuffer, GPS_Date *date, GPS_Location *loc);  //"GPGGA"这一帧数据检测处理
void GPS_GPRMC_CAL(unsigned char *rxBuffer, GPS_Date *date); //"GPRMC"这一帧数据检测处理

void GPS_DateShow(GPS_Date *date);
void GPS_LocationShow(GPS_Location *loc);

