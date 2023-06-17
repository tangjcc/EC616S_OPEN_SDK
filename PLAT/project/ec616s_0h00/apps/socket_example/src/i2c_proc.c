 /****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    i2c_demo.c
 * Description:  EC616S I2C driver example entry source file
 * History:      Rev1.0   2018-07-12
 *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "bsp.h"

// SC7A20 device addr
#define SC7A20_DEVICE_ADDR           0x18 // 0011xxxb

// SC7A20 register addr
#define SC7A20_REG_RESULT            0x00
#define SC7A20_REG_CONFIGURATION     0x01
#define SC7A20_REG_LOW_LIMIT         0x02
#define SC7A20_REG_HIGH_LIMIT        0x03
#define SC7A20_REG_MANUFACTURE_ID    0x7E
#define SC7A20_REG_DEVICE_ID         0x0F

#define SC7A20_OUT_X_L 0x28
#define SC7A20_OUT_X_H 0x29
#define SC7A20_OUT_Y_L 0x2A
#define SC7A20_OUT_Y_H 0x2B
#define SC7A20_OUT_Z_L 0x2C
#define SC7A20_OUT_Z_H 0x2D

// SC7A20 CONFIG resister bit map
#define CONFIG_RN_Pos         (12)
#define CONFIG_RN_Msk         (0xF << CONFIG_RN_Pos)

#define CONFIG_CT_Pos         (11)
#define CONFIG_CT_Msk         (0x1 << CONFIG_CT_Pos)

#define CONFIG_M_Pos          (9)
#define CONFIG_M_Msk          (0x3 << CONFIG_M_Pos)

#define CONFIG_OVF_Pos        (8)
#define CONFIG_OVF_Msk        (0x1 << CONFIG_OVF_Pos)

#define CONFIG_CRF_Pos        (7)
#define CONFIG_CRF_Msk        (0x1 << CONFIG_CRF_Pos)

#define CONFIG_FH_Pos         (6)
#define CONFIG_FH_Msk         (0x1 << CONFIG_FH_Pos)

#define CONFIG_FL_Pos         (5)
#define CONFIG_FL_Msk         (0x1 << CONFIG_FL_Pos)

#define CONFIG_L_Pos          (4)
#define CONFIG_L_Msk          (0x1 << CONFIG_L_Pos)

#define CONFIG_POL_Pos        (3)
#define CONFIG_POL_Msk        (0x1 << CONFIG_POL_Pos)

#define CONFIG_ME_Pos         (2)
#define CONFIG_ME_Msk         (0x1 << CONFIG_ME_Pos)

#define CONFIG_FC_Pos         (0)
#define CONFIG_FC_Msk         (0x3 << CONFIG_L_Pos)


// SC7A20 CONFIG setting macro
#define CONFIG_CT_100         0x0000                           // conversion time set to 100ms
#define CONFIG_CT_800         CONFIG_CT_Msk                    // conversion time set to 800ms

#define CONFIG_M_CONTI        (0x2 << CONFIG_M_Pos)            // continuous conversions
#define CONFIG_M_SINGLE       (0x1 << CONFIG_M_Pos)            // single-shot
#define CONFIG_M_SHUTDOWN     0x0000                           // shutdown


#define CONFIG_RN_RESET       (0xC << CONFIG_RN_Pos)
#define CONFIG_CT_RESET       CONFIG_CT_800
#define CONFIG_L_RESET        CONFIG_L_Msk
#define CONFIG_DEFAULTS       (CONFIG_RN_RESET | CONFIG_CT_RESET | CONFIG_L_RESET)

#define CONFIG_ENABLE_CONTINUOUS     (CONFIG_M_CONTI | CONFIG_DEFAULTS)
#define CONFIG_ENABLE_SINGLE_SHOT    (CONFIG_M_SINGLE | CONFIG_DEFAULTS)
#define CONFIG_DISABLE    CONFIG_DEFAULTS

#define REGISTER_LENGTH        1

#define RetCheck(cond, v1)                  \
do{                                         \
    if (cond == ARM_DRIVER_OK)              \
    {                                       \
        printf("%s OK\r\n", (v1));            \
    } else {                                \
        printf("%s Failed !!!\r\n", (v1));    \
    }                                       \
}while(FALSE)

extern ARM_DRIVER_I2C Driver_I2C1;

static ARM_DRIVER_I2C   *i2cDrvInstance = &CREATE_SYMBOL(Driver_I2C, 1);

#if (RTE_I2C0_IO_MODE != POLLING_MODE)
#error I2C work mode shall be set to POLLING_MODE in RTE_Device.h for this example!
#endif

/**
  \fn          uint16_t SC7A20_ReadReg(uint8_t addr)
  \param[in]   addr    SC7A20 register addr
  \brief       read SC7A20 register
  \return      register value of SC7A20
*/
void SC7A20_WriteReg(uint8_t addr, uint16_t value)
{
    uint8_t tempBuffer[3];

    tempBuffer[0] = addr;
    tempBuffer[1] = (uint8_t)(value >> 8);
    tempBuffer[2] = (uint8_t)value;

    i2cDrvInstance->MasterTransmit(SC7A20_DEVICE_ADDR, tempBuffer, sizeof(tempBuffer), true);
}



/**
  \fn          uint16_t SC7A20_ReadReg(uint8_t addr)
  \param[in]   addr    SC7A20 register addr
  \brief       read SC7A20 register
  \return      register value of SC7A20
*/
uint8_t SC7A20_ReadReg(uint8_t addr)
{
    uint8_t a;
    a = addr | 0x80;

    uint8_t tempBuffer[REGISTER_LENGTH];

    i2cDrvInstance->MasterTransmit(SC7A20_DEVICE_ADDR, &a, 1, true);

    i2cDrvInstance->MasterReceive(SC7A20_DEVICE_ADDR, tempBuffer, 1, true);

	//printf("7BIT:0x%x,0x%x\r\n", tempBuffer[0], tempBuffer[1]);
    return (uint8_t)(tempBuffer[0]);

}

uint16_t SC7A20_ReadReg_8bit(uint8_t addr)
{
    uint8_t a;
    a = addr;

    uint8_t tempBuffer[2];

    i2cDrvInstance->MasterTransmit(0x30, &a, 1, true);

    i2cDrvInstance->MasterReceive(0x30, tempBuffer, sizeof(tempBuffer), true);

	printf("8BIT:0x%x,0x%x\r\n", tempBuffer[0], tempBuffer[1]);
    return (uint16_t)(tempBuffer[0] << 8 | tempBuffer[1]);

}


/**
  \fn          int32_t SC7A20_Enable (bool enable)
  \brief       Enable/Disable SC7A20
  \return
*/
void SC7A20_Enable(bool enable)
{
    uint16_t val;
    if (enable)
    {
        val = CONFIG_ENABLE_SINGLE_SHOT;
    } else {
        val = CONFIG_DISABLE;
    }
    SC7A20_WriteReg(SC7A20_REG_CONFIGURATION, val);
}

void SC7A20_Config(uint16_t configValue)
{
    SC7A20_WriteReg(SC7A20_REG_CONFIGURATION, configValue);
}


void I2C_ExampleEntry(void)
{
    int32_t ret;
    uint16_t regVal, exponent, fraction;

    // Initialize with callback
    ret = i2cDrvInstance->Initialize(NULL);
    RetCheck(ret, "initialize");

    // Power on
    ret = i2cDrvInstance->PowerControl(ARM_POWER_FULL);
    RetCheck(ret, "Power on");

    // Configure I2C bus
    ret = i2cDrvInstance->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);
    RetCheck(ret, "Config bus");

    ret = i2cDrvInstance->Control(ARM_I2C_BUS_CLEAR, 0);
    RetCheck(ret, "Clear bus");

    // Check device id
    regVal = SC7A20_ReadReg(SC7A20_REG_DEVICE_ID);

    printf("Device ID=%x\r\n", regVal);

    SC7A20_Config(CONFIG_ENABLE_CONTINUOUS);

    while(1)
    {
        // check if sensor data ready
        while(1)
        {
            printf("check sensor data ready\n");
            regVal = SC7A20_ReadReg(SC7A20_REG_CONFIGURATION);
            if(regVal & CONFIG_CRF_Msk)
            {
                printf("sensor data ready\r\n");
                break;
            }
            delay_us(100000);
        }
        // read result out
        regVal = SC7A20_ReadReg(SC7A20_REG_RESULT);

        // convert result to LUX
        fraction = regVal & 0xFFF;
        exponent = 1 << (regVal >> 12);

        printf("=============read lux=%d/100 =================\n", fraction * exponent);

        delay_us(3000000);
    }

}

void I2C_Proc(void);

void I2C_init(void)
{
    int32_t ret;
    uint16_t regVal, exponent, fraction;

    // Initialize with callback
    ret = i2cDrvInstance->Initialize(NULL);
    RetCheck(ret, "initialize");

    // Power on
    ret = i2cDrvInstance->PowerControl(ARM_POWER_FULL);
    RetCheck(ret, "Power on");

    // Configure I2C bus
    ret = i2cDrvInstance->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);
    RetCheck(ret, "Config bus");

    ret = i2cDrvInstance->Control(ARM_I2C_BUS_CLEAR, 0);
    RetCheck(ret, "Clear bus");

    // Check device id
    //regVal = SC7A20_ReadReg(SC7A20_REG_DEVICE_ID);

    //printf("Device ID=%x\r\n", regVal);

    //SC7A20_Config(CONFIG_ENABLE_CONTINUOUS);
	I2C_Proc();
}

void I2C_CheckDevId(void)
{
	printf("I2C_CheckDevId\r\n");
	uint8_t regVal = SC7A20_ReadReg(SC7A20_REG_DEVICE_ID);
	printf("7BIT:Device ID=0x%x\r\n", regVal);
}

void I2C_ReadXYZ(void)
{
	uint8_t regVal0 = SC7A20_ReadReg(SC7A20_OUT_X_L);
	uint8_t regVal1 = SC7A20_ReadReg(SC7A20_OUT_X_H);
	uint16_t xx = (uint16_t)(regVal1 << 8 | regVal0);
	uint8_t regVal2 = SC7A20_ReadReg(SC7A20_OUT_Y_L);
	uint8_t regVal3 = SC7A20_ReadReg(SC7A20_OUT_Y_H);
	uint16_t yy = (uint16_t)(regVal3 << 8 | regVal2);
	uint8_t regVal4 = SC7A20_ReadReg(SC7A20_OUT_Z_L);
	uint8_t regVal5 = SC7A20_ReadReg(SC7A20_OUT_Z_H);
	uint16_t zz = (uint16_t)(regVal5 << 8 | regVal4);
	printf("I2C_ReadXYZ:(0x%x,0x%x,0x%x)\r\n", xx, yy, zz);
}


void I2C_ConfigEnable(void)
{
	SC7A20_Config(CONFIG_ENABLE_CONTINUOUS);
}

static void I2C_TimerExp(void *timerCtx)
{
    I2C_CheckDevId();
	//I2C_ReadXYZ();
}

#define I2C_TIMER_PERIOD    (10*1000)  /* 3min, unit: ms */

void I2C_Proc(void)
{
	osTimerId_t         i2cTimer;
	i2cTimer = osTimerNew((osTimerFunc_t)I2C_TimerExp, osTimerPeriodic, NULL, NULL);
	if (i2cTimer == NULL) {
		printf("i2cTimer failed\r\n");
		return;
	}
	osTimerStart(i2cTimer, I2C_TIMER_PERIOD);
}
