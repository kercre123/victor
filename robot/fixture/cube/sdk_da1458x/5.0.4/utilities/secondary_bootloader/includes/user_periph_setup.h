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
#include "gpio.h"
#include "i2c_eeprom.h"
#include "spi.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// Secondary bootloader does not use GPIO interrupts
#define GPIO_DRV_IRQ_HANDLING_DISABLED

// Select EEPROM characteristics
#define I2C_EEPROM_SIZE   0x20000         // EEPROM size in bytes
#define I2C_EEPROM_PAGE   256             // EEPROM's page size in bytes
#define I2C_SLAVE_ADDRESS 0x50            // Set slave device address
#define I2C_SPEED_MODE    I2C_FAST        // 1: standard mode (100 kbits/s), 2: fast mode (400 kbits/s)
#define I2C_ADDRESS_MODE  I2C_7BIT_ADDR   // 0: 7-bit addressing, 1: 10-bit addressing
#define I2C_ADDRESS_SIZE  I2C_2BYTES_ADDR // 0: 8-bit memory address, 1: 16-bit memory address, 3: 24-bit memory address

// SPI Flash settings
// SPI Flash Manufacturer and ID
#define W25X10CL_MANF_DEV_ID (0xEF10)     // W25X10CL Manufacturer and ID
#define W25X20CL_MANF_DEV_ID (0xEF11)     // W25X10CL Manufacturer and ID

// SPI Flash options
#define W25X10CL_SIZE 131072              // SPI Flash memory size in bytes
#define W25X20CL_SIZE 262144              // SPI Flash memory size in bytes
#define W25X10CL_PAGE 256                 // SPI Flash memory page size in bytes
#define W25X20CL_PAGE 256                 // SPI Flash memory page size in bytes

#define SPI_FLASH_DEFAULT_SIZE 131072     // SPI Flash memory size in bytes
#define SPI_FLASH_DEFAULT_PAGE 256        // SPI Flash memory page size in bytes

//SPI initialization parameters
#define SPI_WORD_MODE  SPI_8BIT_MODE
#define SPI_SMN_MODE   SPI_MASTER_MODE
#define SPI_POL_MODE   SPI_CLK_INIT_HIGH
#define SPI_PHA_MODE   SPI_PHASE_1
#define SPI_MINT_EN    SPI_NO_MINT
#define SPI_CLK_DIV    SPI_XTAL_DIV_2
// UART GPIOs assignment
#define UART_GPIO_PORT  GPIO_PORT_0
#define UART_TX_PIN     GPIO_PIN_4
#define UART_RX_PIN     GPIO_PIN_5
#define UART_BAUDRATE    baudrate_57K6

// SPI GPIO assignment
#if defined(__DA14583__)

    #define SPI_GPIO_PORT   GPIO_PORT_2
    #define SPI_CS_PIN      GPIO_PIN_3
    #define SPI_CLK_PIN     GPIO_PIN_0
    #define SPI_DO_PIN      GPIO_PIN_9
    #define SPI_DI_PIN      GPIO_PIN_4

#else

    #define SPI_GPIO_PORT   GPIO_PORT_0
    #define SPI_CS_PIN      GPIO_PIN_3
    #define SPI_CLK_PIN     GPIO_PIN_0
    #define SPI_DO_PIN      GPIO_PIN_6
    #define SPI_DI_PIN      GPIO_PIN_5

#endif

// EEPROM GPIO assignment
#define I2C_GPIO_PORT   GPIO_PORT_0
#define I2C_SCL_PIN     GPIO_PIN_2
#define I2C_SDA_PIN     GPIO_PIN_3

#endif // _USER_PERIPH_SETUP_H_

