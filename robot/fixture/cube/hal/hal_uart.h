#ifndef HAL_UART_H_
#define HAL_UART_H_

#include <stdbool.h>
#include <stdint.h>

//-----------------------------------------------------------
//        Defines
//-----------------------------------------------------------
//empty

//-----------------------------------------------------------
//        Interface
//-----------------------------------------------------------

void hal_uart_init(void);

void hal_uart_rx_flush(void); //discard all rx dat
int  hal_uart_getchar(void); //no data -1

void hal_uart_tx_flush(void); //spin until all buffered data is sent
int  hal_uart_putchar(int c);
void hal_uart_write(const char *s);
int  hal_uart_printf(const char * format, ... ); //@return num bytes written

#endif //HAL_UART_H_

