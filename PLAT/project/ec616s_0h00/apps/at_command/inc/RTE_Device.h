#ifndef __RTE_DEVICE_H
#define __RTE_DEVICE_H

#include "ec616s.h"

/*  Peripheral IO Mode Select, Must Configure First !!!
    Note, when receiver works in DMA_MODE, interrupt is also enabled to transfer tailing bytes.
*/

#define POLLING_MODE            0x1
#define DMA_MODE                0x2
#define IRQ_MODE                0x3
#define UNILOG_MODE             0x4

#define RTE_UART0_TX_IO_MODE    UNILOG_MODE
#define RTE_UART0_RX_IO_MODE    IRQ_MODE
#define USART0_RX_TRIG_LVL      (30)

#define RTE_UART1_TX_IO_MODE    DMA_MODE
#define RTE_UART1_RX_IO_MODE    DMA_MODE

#define RTE_UART2_TX_IO_MODE    POLLING_MODE
#define RTE_UART2_RX_IO_MODE    IRQ_MODE//DMA_MODE

#define RTE_SPI0_IO_MODE          POLLING_MODE

#define RTE_SPI1_IO_MODE          POLLING_MODE

#define I2C0_INIT_MODE          POLLING_MODE
#define I2C1_INIT_MODE          POLLING_MODE


// I2C0 (Inter-integrated Circuit Interface) [Driver_I2C0]
// Configuration settings for Driver_I2C0 in component ::Drivers:I2C
#define RTE_I2C0                        1

// { PAD_PIN24},  // 0 : gpio13 / 2 : I2C0 SCL
// { PAD_PIN23},  // 0 : gpio12 / 2 : I2C0 SDA
#define RTE_I2C0_SCL_BIT                24
#define RTE_I2C0_SCL_FUNC               PAD_MuxAlt2

#define RTE_I2C0_SDA_BIT                23
#define RTE_I2C0_SDA_FUNC               PAD_MuxAlt2

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_I2C0_DMA_TX_EN              0
#define RTE_I2C0_DMA_TX_REQID           DMA_RequestI2C0TX
//   Rx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_I2C0_DMA_RX_EN              0
#define RTE_I2C0_DMA_RX_REQID           DMA_RequestI2C0RX

// I2C1 (Inter-integrated Circuit Interface) [Driver_I2C1]
// Configuration settings for Driver_I2C1 in component ::Drivers:I2C
#define RTE_I2C1                        1

// { PAD_PIN22},  // 0 : gpio11 / 2 : I2C1 SCL
// { PAD_PIN21},  // 0 : gpio10 / 2 : I2C1 SDA
#define RTE_I2C1_SCL_BIT                22
#define RTE_I2C1_SCL_FUNC               PAD_MuxAlt2

#define RTE_I2C1_SDA_BIT                21
#define RTE_I2C1_SDA_FUNC               PAD_MuxAlt2

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_I2C1_DMA_TX_EN              1
#define RTE_I2C1_DMA_TX_REQID           DMA_RequestI2C1TX
//   Rx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_I2C1_DMA_RX_EN              1
#define RTE_I2C1_DMA_RX_REQID           DMA_RequestI2C1RX


// UART0 (Universal asynchronous receiver transmitter) [Driver_USART0]
// Configuration settings for Driver_USART0 in component ::Drivers:USART
#define RTE_UART0                       1
#define RTE_UART0_CTS_PIN_EN            1
#define RTE_UART0_RTS_PIN_EN            0

// { PAD_PIN17},  // 0 : gpio6 / 1 : UART0 RTSn
// { PAD_PIN18},  // 0 : gpio7 / 1 : UART0 CTSn
// { PAD_PIN19},  // 0 : gpio8 / 1 : UART0 RXD
// { PAD_PIN20},  // 0 : gpio9 / 1 : UART0 TXD
#define RTE_UART0_RTS_BIT               17
#define RTE_UART0_RTS_FUNC              PAD_MuxAlt1

#define RTE_UART0_CTS_BIT               18
#define RTE_UART0_CTS_FUNC              PAD_MuxAlt1

#define RTE_UART0_RX_BIT                19
#define RTE_UART0_RX_FUNC               PAD_MuxAlt1

#define RTE_UART0_TX_BIT                20
#define RTE_UART0_TX_FUNC               PAD_MuxAlt1

// DMA
//  Tx
//    Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART0_DMA_TX_REQID          DMA_RequestUSART0TX
//  Rx
//    Channel    <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART0_DMA_RX_REQID          DMA_RequestUSART0RX

// UART1 (Universal asynchronous receiver transmitter) [Driver_USART1]
// Configuration settings for Driver_USART1 in component ::Drivers:USART
#define RTE_UART1                       1
#define RTE_UART1_CTS_PIN_EN            0
#define RTE_UART1_RTS_PIN_EN            0
// { PAD_PIN21},  // 0 : gpio10   / 1 : UART1 RTS
// { PAD_PIN22},  // 0 : gpio11   / 1 : UART1 CTS
// { PAD_PIN25},  // 0 : gpio14   / 1 : UART1 RXD
// { PAD_PIN26},  // 0 : gpio15   / 1 : UART1 TXD
#define RTE_UART1_RTS_BIT               21
#define RTE_UART1_RTS_FUNC              PAD_MuxAlt1

#define RTE_UART1_CTS_BIT               22
#define RTE_UART1_CTS_FUNC              PAD_MuxAlt1

#define RTE_UART1_RX_BIT                25
#define RTE_UART1_RX_FUNC               PAD_MuxAlt1

#define RTE_UART1_TX_BIT                26
#define RTE_UART1_TX_FUNC               PAD_MuxAlt1

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART1_DMA_TX_REQID          DMA_RequestUSART1TX
//   Rx
//     Channel    <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART1_DMA_RX_REQID          DMA_RequestUSART1RX

// UART2 (Universal asynchronous receiver transmitter) [Driver_USART2]
// Configuration settings for Driver_USART2 in component ::Drivers:USART
#define RTE_UART2                       1
#define RTE_UART2_CTS_PIN_EN            0
#define RTE_UART2_RTS_PIN_EN            0

// { PAD_PIN15},  // 0 : gpio4 / 3 : UART2 RXD
// { PAD_PIN16},  // 0 : gpio5 / 3 : UART2 TXD
//#define RTE_UART2_RX_BIT                15
//#define RTE_UART2_RX_FUNC               PAD_MuxAlt3

//#define RTE_UART2_TX_BIT                16
//#define RTE_UART2_TX_FUNC               PAD_MuxAlt3

#define RTE_UART2_RX_BIT                9
#define RTE_UART2_RX_FUNC               PAD_MuxAlt3

#define RTE_UART2_TX_BIT                10
#define RTE_UART2_TX_FUNC               PAD_MuxAlt3

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART2_DMA_TX_REQID          DMA_RequestUSART2TX
//   Rx
//     Channel    <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART2_DMA_RX_REQID          DMA_RequestUSART2RX

// SPI0 (Serial Peripheral Interface) [Driver_SPI0]
// Configuration settings for Driver_SPI0 in component ::Drivers:SPI
#define RTE_SPI0                        1

// { PAD_PIN14},  // 0 : gpio3 / 4 : SPI0 SSn
// { PAD_PIN15},  // 0 : gpio4 / 4 : SPI0 MOSI
// { PAD_PIN16},  // 0 : gpio5 / 4 : SPI0 MISO
// { PAD_PIN17},  // 0 : gpio6 / 4 : SPI0 SCLK
#define RTE_SPI0_SSN_BIT                14
#define RTE_SPI0_SSN_FUNC               PAD_MuxAlt4

#define RTE_SPI0_MOSI_BIT               15
#define RTE_SPI0_MOSI_FUNC              PAD_MuxAlt4

#define RTE_SPI0_MISO_BIT               16
#define RTE_SPI0_MISO_FUNC              PAD_MuxAlt4

#define RTE_SPI0_SCLK_BIT               17
#define RTE_SPI0_SCLK_FUNC              PAD_MuxAlt4

#define RTE_SPI0_SSN_GPIO_INSTANCE      0
#define RTE_SPI0_SSN_GPIO_INDEX         3

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_SPI0_DMA_TX_REQID           DMA_RequestSPI0TX

//   Rx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_SPI0_DMA_RX_REQID           DMA_RequestSPI0RX

// SPI1 (Serial Peripheral Interface) [Driver_SPI1]
// Configuration settings for Driver_SPI1 in component ::Drivers:SPI
#define RTE_SPI1                        0

// { PAD_PIN24},  // 0 : gpio13 / 3 : SPI1 SSn
// { PAD_PIN22},  // 0 : gpio11 / 3 : SPI1 MOSI
// { PAD_PIN23},  // 0 : gpio12 / 3 : SPI1 MISO
// { PAD_PIN21},  // 0 : gpio10 / 3 : SPI1 SCLK
#define RTE_SPI1_SSN_BIT                24
#define RTE_SPI1_SSN_FUNC               PAD_MuxAlt4

#define RTE_SPI1_MOSI_BIT               22
#define RTE_SPI1_MOSI_FUNC              PAD_MuxAlt4

#define RTE_SPI1_MISO_BIT               23
#define RTE_SPI1_MISO_FUNC              PAD_MuxAlt4

#define RTE_SPI1_SCLK_BIT               21
#define RTE_SPI1_SCLK_FUNC              PAD_MuxAlt4

#define RTE_SPI1_SSN_GPIO_INSTANCE      0
#define RTE_SPI1_SSN_GPIO_INDEX         13

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_SPI1_DMA_TX_REQID           DMA_RequestSPI1TX

//   Rx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_SPI1_DMA_RX_REQID           DMA_RequestSPI1RX

// PWM0 Controller [Driver_PWM0]
// Configuration settings for Driver_PWM0 in component ::Drivers:PWM
#define RTE_PWM                         1

// Netlight
#define NETLIGHT_PAD_INDEX              17
#define NETLIGHT_PAD_ALT_FUNC           PAD_MuxAlt5
#define NETLIGHT_PWM_INSTANCE           4

#define EFUSE_INIT_MODE POLLING_MODE
#define L2CTLS_INIT_MODE POLLING_MODE

#define FLASH_BARE_RW_MODE 1

#endif  /* __RTE_DEVICE_H */
