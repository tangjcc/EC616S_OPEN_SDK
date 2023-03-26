
#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "gps_proc.h"

unsigned char Flag_Calc_GPGGA_OK = 0;//计算完成标志位
unsigned char Flag_Calc_GPRMC_OK = 0;//计算完成标志位

unsigned char IsLeapYear (unsigned int uiYear);
unsigned char GetMaxDay (unsigned char Month_Value, unsigned int Year_Value);


//****************************************************
//UTC日期与当地日期转换
//****************************************************
void UTCDate2LocalDate (GPS_Date *date)
{
	date->Day = date->Day + date->Flag_OV;		//日  加一
	date->Month = date->Month;
	date->Year = 2000 + date->Year;

	unsigned char MaxDay = GetMaxDay (date->Month, date->Year);				//获取当月 天数 最大值
	if (date->Day > MaxDay)		//溢出
	{
		date->Day = 1;
		date->Month += 1;
		if (date->Month > 12)
		{
			date->Year += 1;
		}
	}
}

//****************************************************
//获取当月日期最大值
//****************************************************
unsigned char GetMaxDay (unsigned char Month_Value, unsigned int Year_Value)
{
	unsigned char iDays;
	switch (Month_Value)
	{
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
	{
		iDays = 31;
	}
	break;
	case 2:
	{
		//2月份比较特殊，需要根据是不是闰年来判断当月是28天还29天
		iDays = IsLeapYear (Year_Value) ? 29 : 28;
	}
	break;
	case 4:
	case 6:
	case 9:
	case 11:
	{
		iDays = 30;
	}
	break;
	default :
		break;
	}
	return (iDays);
}

//****************************************************
//闰年检测
//****************************************************
unsigned char IsLeapYear (unsigned int uiYear)
{
	return ( ( (uiYear % 4) == 0) && ( (uiYear % 100) != 0)) || ( (uiYear % 400) == 0);
}

void GPS_GPGGA_CAL(unsigned char *rxBuffer, GPS_Date *date, GPS_Location *loc)  //"GPGGA"这一帧数据检测处理
{
	if (rxBuffer[3] == 'G' && rxBuffer[4] == 'G' && rxBuffer[5] == 'A' && rxBuffer[13] == '.')			//确定是否收到"GPGGA"这一帧数据
	{
		date->Hour = (rxBuffer[7] - 0x30) * 10 + (rxBuffer[8] - 0x30) + 8;	//UTC时间转换到北京时间UTC+8
		//0X30为ASCII码转换为数字
		if (date->Hour >= 24) {
			date->Hour %= 24;				//获取当前Hour
			date->Flag_OV = 1;			//日期进位
		}
		else {
			date->Flag_OV = 0;
		}
		//Min_High = rxBuffer[9] - 0X30; //-0X30由ASCII转为十进制
		//Min_Low = rxBuffer[10] - 0X30;
		date->Min = (rxBuffer[9] - 0X30) * 10 + (rxBuffer[10] - 0X30);
		//Sec_High = rxBuffer[11] - 0X30;
		//Sec_Low = rxBuffer[12] - 0X30;
		date->Sec = (rxBuffer[11] - 0X30) * 10 + (rxBuffer[12] - 0X30);

		if (rxBuffer[28] == 78) {
			loc->weiduFlag = 'N';//北纬
		} else {	
			loc->weiduFlag = 'S';	//南纬
		}
		loc->weidu = (rxBuffer[17] - 0X30) * 10 + (rxBuffer[18] - 0X30);
		loc->weiduMin = (rxBuffer[19] - 0X30) * 10 + (rxBuffer[20] - 0X30);
		loc->weiduSec = (rxBuffer[22] - 0X30) * 100 + (rxBuffer[23] - 0X30) * 10 + (rxBuffer[24] - 0X30);

		if (rxBuffer[42] == 69) {
			loc->jingduFlag = 'E';//北纬
		} else {	
			loc->jingduFlag = 'W';	//南纬
		}
		loc->jingdu = (rxBuffer[30] - 0X30) * 100 + (rxBuffer[31] - 0X30) * 10 + (rxBuffer[32] - 0X30);
		loc->jingduMin = (rxBuffer[33] - 0X30) * 10 + (rxBuffer[34] - 0X30);
		loc->jingduSec = (rxBuffer[36] - 0X30) * 100 + (rxBuffer[37] - 0X30) * 10 + (rxBuffer[38] - 0X30);

		loc->altitude= (rxBuffer[54] - 0X30) * 100 + (rxBuffer[55] - 0X30) * 10 + (rxBuffer[56] - 0X30);
		loc->satNum= (rxBuffer[47] - 0X30);
		Flag_Calc_GPGGA_OK = 1;
	}
}


void GPS_GPRMC_CAL(unsigned char *rxBuffer, GPS_Date *date) //"GPRMC"这一帧数据检测处理
{
	if (rxBuffer[4] == 'M' && rxBuffer[52] == ',' && rxBuffer[59] == ',')			//确定是否收到"GPRMC"这一帧数据
	{
		//Year_High = rxBuffer[57];
		//Year_Low = rxBuffer[58];
		date->Year = (rxBuffer[57] - 0x30) * 10 + rxBuffer[58];

		//Month_High = rxBuffer[55];
		//Month_Low = rxBuffer[56];
		date->Month= (rxBuffer[55] - 0x30) * 10 + rxBuffer[56];

		//Day_High = rxBuffer[53];
		//Day_Low = rxBuffer[54];
		date->Day = (rxBuffer[53] - 0x30) * 10 + rxBuffer[54];


		UTCDate2LocalDate(date);			//UTC日期转换为北京时间

		Flag_Calc_GPRMC_OK = 1;
	}
}

void GPS_DateShow(GPS_Date *date)
{
	// YYYY/MM/DD-HH:MM:SS
	printf("DATE:%04d/%02d/%02d-%02d:%02d:%02d\r\n", 
		date->Year, date->Month, date->Day, date->Hour, date->Min, date->Sec);
}

void GPS_LocationShow(GPS_Location *loc)
{
	//jindu weidu altitude
	printf("LOC:%d:%03d^%02d\'%02d\'\'/%c:%03d^%02d\'%02d\'\'#%dm\r\n",
		loc->jingduFlag, loc->jingdu, loc->jingduMin, loc->jingduSec,
		loc->weiduFlag, loc->weidu, loc->weiduMin, loc->weiduSec,
		loc->altitude);
}

unsigned char GPS_IS_GPGGA_DATA(unsigned char *rawData)
{
	if (strlen((const char *)rawData) < GPS_DATA_HEAD_LEN) {
		return 0;
	}
	if (strncmp((const char *)rawData, GPGGA_STR, GPS_DATA_HEAD_LEN) == 0) {
		return 1;
	}
	return 0;
}

unsigned char GPS_IS_GPRMC_DATA(unsigned char *rawData)
{
	if (strlen((const char *)rawData) < GPS_DATA_HEAD_LEN) {
		return 0;
	}
	if (strncmp((const char *)rawData, GPRMC_STR, GPS_DATA_HEAD_LEN) == 0) {
		return 1;
	}
	return 0;
}

