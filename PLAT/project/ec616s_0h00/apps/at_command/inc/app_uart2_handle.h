#ifndef APP_UART2_HANDLE_H
#define APP_UART2_HANDLE_H

void uart2_set_rxbuff(uint8_t *prxbuff,uint32_t rx_length);
void uart2_tx(uint8_t *ptxbuff,uint32_t tx_length);
uint8_t uart2_get_flag_timeout(void);
void uart2_clear_flag_timeout(void);
uint8_t uart2_get_flag_complete(void);
void uart2_clear_flag_complete(void);
void uart2_init(void);
void app_uart2_handle_init(void);

#endif


