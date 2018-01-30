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
 * DEFINES
 ****************************************************************************************
 */

void periph_init(void);

// Select UART settings
#define UART2_BAUDRATE     UART_BAUDRATE_115K2       // Baudrate in bits/s: {9K6, 14K4, 19K2, 28K8, 38K4, 57K6, 115K2}
#define UART2_DATALENGTH   UART_CHARFORMAT_8         // Datalength in bits: {5, 6, 7, 8}
#define UART2_PARITY       UART_PARITY_NONE          // Parity: {UART_PARITY_NONE, UART_PARITY_EVEN, UART_PARITY_ODD}
#define UART2_STOPBITS     UART_STOPBITS_1           // Stop bits: {UART_STOPBITS_1, UART_STOPBITS_2} 
#define UART2_FLOWCONTROL  UART_FLOWCONTROL_DISABLED // Flow control: {UART_FLOWCONTROL_DISABLED, UART_FLOWCONTROL_ENABLED}

#define UART2_GPIO_PORT    GPIO_PORT_0
#define UART2_TX_PIN       GPIO_PIN_4
#define UART2_RX_PIN       GPIO_PIN_5

#define UART_ENABLED

// Select EEPROM characteristics
#define I2C_EEPROM_SIZE   0x20000         // EEPROM size in bytes
#define I2C_EEPROM_PAGE   256             // EEPROM's page size in bytes
#define I2C_SLAVE_ADDRESS 0x50            // Set slave device address
#define I2C_SPEED_MODE    I2C_FAST        // Speed mode: I2C_STANDARD (100 kbits/s), I2C_FAST (400 kbits/s)
#define I2C_ADDRESS_MODE  I2C_7BIT_ADDR   // Addressing mode: {I2C_7BIT_ADDR, I2C_10BIT_ADDR}
#define I2C_ADDRESS_SIZE  I2C_2BYTES_ADDR // Address width: {I2C_1BYTE_ADDR, I2C_2BYTES_ADDR, I2C_3BYTES_ADDR}

// Select I2C settings
#define I2C_GPIO_PORT     GPIO_PORT_0
#define I2C_SCL_PIN       GPIO_PIN_2
#define I2C_SDA_PIN       GPIO_PIN_3

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
