/**
 ****************************************************************************************
 *
 * @file uart.h
 *
 * @brief uart initialization and print functions header file.
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
 
#include <stdint.h>

#ifndef UART_H_INCLUDED
#define UART_H_INCLUDED

#define baudrate_115K2 9
#define baudrate_57K6 17
#define baudrate_38K4 26
#define baudrate_28K8 35
#define baudrate_19K2 52
#define baudrate_14K4 69
#define baudrate_9K6  104

#define UART_FIFO_EN 1
#define UART_RX_FIFO_RESET 2
#define UART_TX_FIFO_RESET 4

void uart_initialization(void);
void uart_release(void);

char uart_receive_byte(char *ch);
void uart_send_byte(char ch);
 
  
#endif
