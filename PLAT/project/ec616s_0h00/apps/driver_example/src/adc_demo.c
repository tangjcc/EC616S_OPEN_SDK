/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    adc_demo.c
 * Description:  EC616S ADC driver example entry source file
 * History:      Rev1.0   2019-06-27
 *
 ****************************************************************************/
#include <stdio.h>
#include "adc_ec616s.h"
#include "hal_adc.h"
#include "ic_ec616s.h"
#include "bsp.h"

/*
   This project demotrates the usage of ADC, in this example, we keep sampling THERMAL, VBAT, AIO2 channels.
 */

/** \brief ADC conversion done flag and channel conversion result */
static volatile uint32_t conversionDone = 0;
static volatile uint32_t aio2ChannelResult = 0;
static volatile uint32_t vbatChannelResult = 0;
static volatile uint32_t thermalChannelResult = 0;

/**
  \fn          void ADC_Aio2ChannelCallback()
  \brief       Aio2 channel callback
  \return
*/
void ADC_Aio2ChannelCallback(uint32_t result)
{
    conversionDone |= ADC_ChannelAio2;
    aio2ChannelResult = result;
}

/**
  \fn          void ADC_VbatChannelCallback()
  \brief       Vbat channel callback
  \return
*/
void ADC_VbatChannelCallback(uint32_t result)
{
    conversionDone |= ADC_ChannelVbat;
    vbatChannelResult = result;
}

/**
  \fn          void ADC_ThermalChannelCallback()
  \brief       Thermal channel callback
  \return
*/
void ADC_ThermalChannelCallback(uint32_t result)
{
    conversionDone |= ADC_ChannelThermal;
    thermalChannelResult = result;
}

void ADC_ExampleEntry(void)
{
    adc_config_t adcConfig;

    printf("ADC Example Start\r\n");

    ADC_GetDefaultConfig(&adcConfig);

    adcConfig.channelConfig.aioResDiv = ADC_AioResDivRatio1;
    ADC_ChannelInit(ADC_ChannelAio2, ADC_UserAPP, &adcConfig, ADC_Aio2ChannelCallback);

    adcConfig.channelConfig.vbatResDiv = ADC_VbatResDivRatio3Over16;
    ADC_ChannelInit(ADC_ChannelVbat, ADC_UserAPP, &adcConfig, ADC_VbatChannelCallback);

    adcConfig.channelConfig.thermalInput = ADC_ThermalInputVbat;
    ADC_ChannelInit(ADC_ChannelThermal, ADC_UserAPP, &adcConfig, ADC_ThermalChannelCallback);

    while(1)
    {
        conversionDone = 0;

        ADC_StartConversion(ADC_ChannelAio2, ADC_UserAPP);
        ADC_StartConversion(ADC_ChannelVbat, ADC_UserAPP);

        ADC_StartConversion(ADC_ChannelThermal, ADC_UserAPP);

        // Wait for all channels conversion completes
        while(conversionDone != (ADC_ChannelAio2 | ADC_ChannelVbat | ADC_ChannelThermal));

        printf("Conversion result:\r\n");

        printf("Thermal : raw register :%d, Temperature:%d\r\n", (int)thermalChannelResult, (int)HAL_ADC_ConvertThermalRawCodeToTemperature(thermalChannelResult));
        printf("Vbat    : raw register :%d, Voltage:%dmv\r\n", (int)vbatChannelResult, (int)HAL_ADC_CalibrateRawCode(vbatChannelResult) * 16 / 3);
        printf("Aio2    : raw register :%d, Voltage:%dmv\r\n", (int)aio2ChannelResult, (int)HAL_ADC_CalibrateRawCode(aio2ChannelResult));

        delay_us(500000);
    }
}

