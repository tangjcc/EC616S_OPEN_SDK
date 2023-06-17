
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bsp.h"
#include "gps_proc.h"

unsigned char Flag_Calc_GPGGA_OK = 0;//������ɱ�־λ
unsigned char Flag_Calc_GPRMC_OK = 0;//������ɱ�־λ

static unsigned char IsLeapYear (unsigned int uiYear);
static unsigned char GetMaxDay (unsigned char Month_Value, unsigned int Year_Value);

static int GetComma(int num, const char *str)
{
	int i,j=0;
	int len=strlen(str);
	for(i=0;i<len;i++)
	{
		if(str[i]==',')
			j++;
		if(j==num)
			return i+1; /*���ص�ǰ�ҵ��Ķ���λ�õ���һ��λ��*/
	}
	return 0;
}
static double get_double_number(const char *s)
{
	char buf[128];
	int i;
	double rev;
	i=GetComma(1,s);/*�õ����ݳ��� */
	strncpy(buf,s,i);
	buf[i]=0; /*���ַ���������־*/
	rev=atof(buf);/*�ַ���תfloat */
	return rev;
}

static int get_int_number(const char *s)
{
	char buf[128];
	int i;
	double rev;
	i=GetComma(1,s);/*�õ����ݳ���*/
	strncpy(buf,s,i);
	buf[i]=0; /*���ַ���������־ */
	rev=atoi(buf);/*�ַ���תfloat */
	return rev;
}

//****************************************************
//UTC�����뵱������ת��
//****************************************************
static void UTCDate2LocalDate (GPS_Date *date)
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
static unsigned char GetMaxDay (unsigned char Month_Value, unsigned int Year_Value)
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
static unsigned char IsLeapYear (unsigned int uiYear)
{
	return ( ( (uiYear % 4) == 0) && ( (uiYear % 100) != 0)) || ( (uiYear % 400) == 0);
}

void GPS_GPGGA_CAL(unsigned char *rxBuffer, GPS_Date *date, GPS_Location *loc)  //"GPGGA"��һ֡���ݼ�⴦��
{
	int pos = 0;
	int posEnd = 0;
	if (rxBuffer[3] == 'G' && rxBuffer[4] == 'G' && rxBuffer[5] == 'A')			//ȷ���Ƿ��յ�"GPGGA"��һ֡����
	{
		pos = GetComma(1, (const char *)rxBuffer); //7
		posEnd = GetComma(2, (const char *)rxBuffer);
		if (posEnd - pos < 10) {
			return;
		}
		date->Hour = (rxBuffer[pos] - 0x30) * 10 + (rxBuffer[pos + 1] - 0x30) + 8;	//UTCʱ��ת��������ʱ��UTC+8
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
		date->Min = (rxBuffer[pos + 2] - 0X30) * 10 + (rxBuffer[pos + 3] - 0X30);
		//Sec_High = rxBuffer[11] - 0X30;
		//Sec_Low = rxBuffer[12] - 0X30;
		date->Sec = (rxBuffer[pos + 4] - 0X30) * 10 + (rxBuffer[pos + 5] - 0X30);

		pos = posEnd;
		posEnd = GetComma(3, (const char *)rxBuffer);
		if (posEnd - pos < 9) {
			return;
		}
		loc->weidu = (rxBuffer[pos] - 0X30) * 10 + (rxBuffer[pos + 1] - 0X30);
		//loc->weiduMin = (rxBuffer[pos + 2] - 0X30) * 10 + (rxBuffer[pos + 3] - 0X30);
		//loc->weiduSec = (rxBuffer[pos + 5] - 0X30) * 1000 + (rxBuffer[pos + 6] - 0X30) * 100 + (rxBuffer[pos + 7] - 0X30) * 10 + (rxBuffer[pos + 8] - 0X30);
		loc->weiduMin = get_double_number((const char *)&rxBuffer[pos + 2]);

		pos = posEnd;
		posEnd = GetComma(4, (const char *)rxBuffer);
		if (posEnd - pos < 1) {
			return;
		}
		loc->weiduFlag = rxBuffer[pos];

		pos = posEnd;
		posEnd = GetComma(5, (const char *)rxBuffer);
		if (posEnd - pos < 10) {
			return;
		}
		loc->jingdu = (rxBuffer[pos] - 0X30) * 100 + (rxBuffer[pos + 1] - 0X30) * 10 + (rxBuffer[pos + 2] - 0X30);
		//loc->jingduMin = (rxBuffer[pos + 3] - 0X30) * 10 + (rxBuffer[pos + 4] - 0X30);
		//loc->jingduSec = (rxBuffer[pos + 6] - 0X30) * 1000 + (rxBuffer[pos + 7] - 0X30) * 100 + (rxBuffer[pos + 8] - 0X30) * 10 + (rxBuffer[pos + 9] - 0X30);
		loc->jingduMin = get_double_number((const char *)&rxBuffer[pos + 3]);

		pos = posEnd;
		posEnd = GetComma(6, (const char *)rxBuffer);
		if (posEnd - pos < 1) {
			return;
		}
		loc->jingduFlag = rxBuffer[pos];

		pos = GetComma(7, (const char *)rxBuffer);
		loc->satNum= get_int_number((const char *)&rxBuffer[pos]);
		pos = GetComma(9, (const char *)rxBuffer);
		loc->altitude = get_double_number((const char *)&rxBuffer[pos]);

		Flag_Calc_GPGGA_OK = 1;
	}
}


void GPS_GPRMC_CAL(unsigned char *rxBuffer, GPS_Date *date) //"GPRMC"��һ֡���ݼ�⴦��
{
	int pos = 0;
	int posEnd = 0;
	if (rxBuffer[3] == 'R' && rxBuffer[4] == 'M' && rxBuffer[5] == 'C')			//ȷ���Ƿ��յ�"GPRMC"��һ֡����
	{
		pos = GetComma(7, (const char *)rxBuffer);
		date->spd = get_double_number((const char *)&rxBuffer[pos]);
		pos = GetComma(8, (const char *)rxBuffer);
		date->cog= get_double_number((const char *)&rxBuffer[pos]);
		pos = GetComma(9, (const char *)rxBuffer);
		posEnd = GetComma(10, (const char *)rxBuffer);
		if (posEnd - pos < 6) {
			return;
		}
		date->Day = (rxBuffer[pos] - 0x30) * 10 + (rxBuffer[pos + 1] - 0x30);
		date->Month= (rxBuffer[pos + 2] - 0x30) * 10 + (rxBuffer[pos + 3] - 0x30);
		date->Year = (rxBuffer[pos + 4] - 0x30) * 10 + (rxBuffer[pos + 5] - 0x30);

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
	printf("LOC:%c:%03d.%05.2f|%c:%02d.%05.2f#%lfm\r\n",
		loc->jingduFlag, loc->jingdu, loc->jingduMin,
		loc->weiduFlag, loc->weidu, loc->weiduMin,
		loc->altitude);
}

unsigned char GPS_IS_GPGGA_DATA(unsigned char *rawData)
{
	if (strlen((const char *)rawData) < GPS_DATA_HEAD_LEN) {
		return 0;
	}
	return (rawData[3] == 'G' && rawData[4] == 'G' && rawData[5] == 'A');
}

unsigned char GPS_IS_GPRMC_DATA(unsigned char *rawData)
{
	if (strlen((const char *)rawData) < GPS_DATA_HEAD_LEN) {
		return 0;
	}
	return (rawData[3] == 'R' && rawData[4] == 'M' && rawData[5] == 'C');
}

