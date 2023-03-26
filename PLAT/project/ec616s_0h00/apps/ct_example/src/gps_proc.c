
#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "gps_proc.h"

unsigned char Flag_Calc_GPGGA_OK = 0;//������ɱ�־λ
unsigned char Flag_Calc_GPRMC_OK = 0;//������ɱ�־λ

unsigned char IsLeapYear (unsigned int uiYear);
unsigned char GetMaxDay (unsigned char Month_Value, unsigned int Year_Value);


//****************************************************
//UTC�����뵱������ת��
//****************************************************
void UTCDate2LocalDate (GPS_Date *date)
{
	date->Day = date->Day + date->Flag_OV;		//��  ��һ
	date->Month = date->Month;
	date->Year = 2000 + date->Year;

	unsigned char MaxDay = GetMaxDay (date->Month, date->Year);				//��ȡ���� ���� ���ֵ
	if (date->Day > MaxDay)		//���
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
//��ȡ�����������ֵ
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
		//2�·ݱȽ����⣬��Ҫ�����ǲ����������жϵ�����28�컹29��
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
//������
//****************************************************
unsigned char IsLeapYear (unsigned int uiYear)
{
	return ( ( (uiYear % 4) == 0) && ( (uiYear % 100) != 0)) || ( (uiYear % 400) == 0);
}

void GPS_GPGGA_CAL(unsigned char *rxBuffer, GPS_Date *date, GPS_Location *loc)  //"GPGGA"��һ֡���ݼ�⴦��
{
	if (rxBuffer[3] == 'G' && rxBuffer[4] == 'G' && rxBuffer[5] == 'A' && rxBuffer[13] == '.')			//ȷ���Ƿ��յ�"GPGGA"��һ֡����
	{
		date->Hour = (rxBuffer[7] - 0x30) * 10 + (rxBuffer[8] - 0x30) + 8;	//UTCʱ��ת��������ʱ��UTC+8
		//0X30ΪASCII��ת��Ϊ����
		if (date->Hour >= 24) {
			date->Hour %= 24;				//��ȡ��ǰHour
			date->Flag_OV = 1;			//���ڽ�λ
		}
		else {
			date->Flag_OV = 0;
		}
		//Min_High = rxBuffer[9] - 0X30; //-0X30��ASCIIתΪʮ����
		//Min_Low = rxBuffer[10] - 0X30;
		date->Min = (rxBuffer[9] - 0X30) * 10 + (rxBuffer[10] - 0X30);
		//Sec_High = rxBuffer[11] - 0X30;
		//Sec_Low = rxBuffer[12] - 0X30;
		date->Sec = (rxBuffer[11] - 0X30) * 10 + (rxBuffer[12] - 0X30);

		if (rxBuffer[28] == 78) {
			loc->weiduFlag = 'N';//��γ
		} else {	
			loc->weiduFlag = 'S';	//��γ
		}
		loc->weidu = (rxBuffer[17] - 0X30) * 10 + (rxBuffer[18] - 0X30);
		loc->weiduMin = (rxBuffer[19] - 0X30) * 10 + (rxBuffer[20] - 0X30);
		loc->weiduSec = (rxBuffer[22] - 0X30) * 100 + (rxBuffer[23] - 0X30) * 10 + (rxBuffer[24] - 0X30);

		if (rxBuffer[42] == 69) {
			loc->jingduFlag = 'E';//��γ
		} else {	
			loc->jingduFlag = 'W';	//��γ
		}
		loc->jingdu = (rxBuffer[30] - 0X30) * 100 + (rxBuffer[31] - 0X30) * 10 + (rxBuffer[32] - 0X30);
		loc->jingduMin = (rxBuffer[33] - 0X30) * 10 + (rxBuffer[34] - 0X30);
		loc->jingduSec = (rxBuffer[36] - 0X30) * 100 + (rxBuffer[37] - 0X30) * 10 + (rxBuffer[38] - 0X30);

		loc->altitude= (rxBuffer[54] - 0X30) * 100 + (rxBuffer[55] - 0X30) * 10 + (rxBuffer[56] - 0X30);
		loc->satNum= (rxBuffer[47] - 0X30);
		Flag_Calc_GPGGA_OK = 1;
	}
}


void GPS_GPRMC_CAL(unsigned char *rxBuffer, GPS_Date *date) //"GPRMC"��һ֡���ݼ�⴦��
{
	if (rxBuffer[4] == 'M' && rxBuffer[52] == ',' && rxBuffer[59] == ',')			//ȷ���Ƿ��յ�"GPRMC"��һ֡����
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


		UTCDate2LocalDate(date);			//UTC����ת��Ϊ����ʱ��

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

