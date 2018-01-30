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
#define UART2_BAUDRATE     UART_BAUDRATE_115K2       // Baudrate in bits/s: {9K6, 14K4, 19K2, 28K8, 38K4, 57K6, 115K2}
#define UART2_DATALENGTH   UART_CHARFORMAT_8         // Datalength in bits: {5, 6, 7, 8}
#define UART2_PARITY       UART_PARITY_NONE          // Parity: {UART_PARITY_NONE, UART_PARITY_EVEN, UART_PARITY_ODD}
#define UART2_STOPBITS     UART_STOPBITS_1           // Stop bits: {UART_STOPBITS_1, UART_STOPBITS_2}
#define UART2_FLOWCONTROL  UART_FLOWCONTROL_DISABLED // Flow control: {UART_FLOWCONTROL_DISABLED, UART_FLOWCONTROL_ENABLED}

#define UART2_GPIO_PORT   GPIO_PORT_0
#define UART2_TX_PIN      GPIO_PIN_4
#define UART2_RX_PIN      GPIO_PIN_5
#define UART_ENABLED

// pwm pin selection
#define PWM2_PORT         GPIO_PORT_1
#define PWM2_PIN          GPIO_PIN_2
#define PWM3_PORT         GPIO_PORT_0
#define PWM3_PIN          GPIO_PIN_7
#define PWM4_PORT         GPIO_PORT_1
#define PWM4_PIN          GPIO_PIN_0

// pwm settings
#define PWM_FREQUENCY     0x1000
#define BIT_MASK          0xFFF
#define PWM2_DUTY_STEP    0x200
#define PWM34_DUTY_STEP   0x100
#define PWM3_MAX_DUTY     0x1000

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
