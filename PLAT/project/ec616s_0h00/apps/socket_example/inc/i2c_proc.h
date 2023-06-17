
#include <stdio.h>

/**
  \fn          uint16_t OPT3001_ReadReg(uint8_t addr)
  \param[in]   addr    OPT3001 register addr
  \brief       read OPT3001 register
  \return      register value of OPT3001
*/
void OPT3001_WriteReg(uint8_t addr, uint16_t value);


/**
  \fn          uint16_t OPT3001_ReadReg(uint8_t addr)
  \param[in]   addr    OPT3001 register addr
  \brief       read OPT3001 register
  \return      register value of OPT3001
*/
uint16_t OPT3001_ReadReg(uint8_t addr);

/**
  \fn          int32_t OPT3001_Enable (bool enable)
  \brief       Enable/Disable opt3001
  \return
*/
void OPT3001_Enable(bool enable);

void OPT3001_Config(uint16_t configValue);

void I2C_init(void);
void I2C_CheckDevId(void);
void I2C_ConfigEnable(void);
