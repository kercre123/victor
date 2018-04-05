/**
 ****************************************************************************************
 *
 * @file user_periph_setup.h
 *
 * @brief Peripherals setup header file.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _USER_PERIPH_SETUP_H_
#define _USER_PERIPH_SETUP_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "global_io.h"
#include "datasheet.h"
#include <stdint.h>
#include <core_cm0.h>
#include "common_uart.h"
#include "gpio.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// Select UART settings
#define UART_BAUDRATE     UART_BAUDRATE_115K2       // Baudrate in bits/s: {9K6, 14K4, 19K2, 28K8, 38K4, 57K6, 115K2}
#define UART_DATALENGTH   UART_DATALENGTH_8         // Datalength in bits: {5, 6, 7, 8}
#define UART_PARITY       UART_PARITY_NONE          // Parity: {UART_PARITY_NONE, UART_PARITY_EVEN, UART_PARITY_ODD}
#define UART_STOPBITS     UART_STOPBITS_1           // Stop bits: {UART_STOPBITS_1, UART_STOPBITS_2}
#define UART_FLOWCONTROL  UART_FLOWCONTROL_DISABLED // Flow control: {UART_FLOWCONTROL_DISABLED, UART_FLOWCONTROL_ENABLED}

#define UART_GPIO_PORT    GPIO_PORT_0
#define UART_TX_PIN       GPIO_PIN_4
#define UART_RX_PIN       GPIO_PIN_5
#define UART_ENABLED

// LED
#define LED_PORT          GPIO_PORT_1
#define LED_PIN           GPIO_PIN_0

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Enable pad's and peripheral clocks assuming that peripherals' power domain
 * is down. The Uart and SPI clocks are set.
 * @return void
 ****************************************************************************************
 */
void periph_init(void);

#endif // _USER_PERIPH_SETUP_H_
