    /**
 ****************************************************************************************
 *
 * @file uart.h
 *
 * @brief eeprom driver over i2c interface header file.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _UART_H_
#define _UART_H_

#define UART_BAUD_9600   0
#define UART_BAUD_19200  1
#define UART_BAUD_57600  2
#define UART_BAUD_115200 3
#define UART_BAUD_1M     4

void uart_irq_enable(void);
void uart_irq_disable(void);
void uart_send (unsigned char txdata);
unsigned char uart_recv (void);
void uart_wait_end_of_tx(void);
void uart_wait_end_of_rx(void);
void uart_pads (unsigned char sel);
void uart_init (unsigned char pad_sel, unsigned char baud_rate);

void uart_putc(unsigned char letter);
unsigned char uart_getc(void);
void uart_print(char *buf);

int uart_rx_poll(void);

void uart_print_char_hex(unsigned char c);
void uart_print_hex(unsigned char *c, int len);
void uart_print_int(unsigned int word) ;
#endif
