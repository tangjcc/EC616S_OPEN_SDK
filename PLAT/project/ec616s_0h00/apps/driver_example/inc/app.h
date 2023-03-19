/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    app.h
 * Description:  EC616S driver example/demo entry header file
 * History:      Rev1.0   2018-07-12
 *
 ****************************************************************************/

/** \brief List of all examples */
typedef enum
{
    USART_EX,
    SPI_EX,
    I2C_EX,
    GPIO_EX,
    TIMER_EX,
    WDT_EX,
    DMA_EX,
    ADC_EX,
    TLS_EX
} example_id_t;

/**
  \fn          void USART_ExampleEntry(void)
  \brief       USART example entry function
*/
void USART_ExampleEntry(void);

/**
  \fn          void SPI_ExampleEntry(void)
  \brief       SPI example entry function.
  \return
*/
void SPI_ExampleEntry(void);

/**
  \fn          void I2C_ExampleEntry(void)
  \brief       I2C example entry function.
  \return
*/
void I2C_ExampleEntry(void);

/**
  \fn          void GPIO_ExampleEntry(void)
  \brief       GPIO example entry function.
  \return
*/
void GPIO_ExampleEntry(void);

/**
  \fn          void TIMER_ExampleEntry(void)
  \brief       TIMER example entry function.
  \return
*/
void TIMER_ExampleEntry(void);

/**
  \fn          void WDT_ExampleEntry(void)
  \brief       WDT example entry function.
  \return
*/
void WDT_ExampleEntry(void);

/**
  \fn          void DMA_ExampleEntry(void)
  \brief       DMA example entry function.
  \return
*/
void DMA_ExampleEntry(void);

/**
  \fn          void ADC_ExampleEntry(void)
  \brief       ADC example entry function.
  \return
*/
void ADC_ExampleEntry(void);

/**
  \fn          void TLS_ExampleEntry(void)
  \brief       TLS example entry function.
  \return
*/
void TLS_ExampleEntry(void);


