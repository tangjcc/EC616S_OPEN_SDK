/****************************************************************************
 *
 * Copy right:   2020-, Copyrigths of EigenComm Ltd.
 * File name:    hal_uart_atcmd.h
 * Description:  EC616 uart at command header file
 * History:      Rev1.0   2020-02-24
 *
 ****************************************************************************/
/**
  \brief Register AT Channel
  \return
 */
void atCmdRegisterMultiAtChannel(void);

/**
  \brief Init uart handler for AT CMD
  \return
 */
void atCmdInitUartHandler(ARM_DRIVER_USART * uartDriverHandler, uint32_t baudRate, uint32_t frameFormat);

/**
  \brief Init uart handler for AT CMD 2
  \return
 */
void atCmd2InitUartHandler(ARM_DRIVER_USART * uartDriverHandler, uint32_t baudRate);

