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
#include "arch.h"
#include "customer_prod.h"

/*
 * VARIABLE DEFINITIONS
 ****************************************************************************************
 */
 
extern uint8_t port_sel;

typedef struct __uart_sel_pins
{
    uint8_t uart_port_tx;
    uint8_t uart_tx_pin;
    uint8_t uart_port_rx;
    uint8_t uart_rx_pin;
}_uart_sel_pins;

extern _uart_sel_pins uart_sel_pins;

/*
 * DEFINES
 ****************************************************************************************
 */

#undef CONFIG_UART_GPIOS

// Only one of the following lines must be uncommented.
#define UART_P04_P05
//#define UART_P00_P01
//#define UART_P14_P15
//#define UART_P04_P13


#define GPIO_PWM0_PORT     GPIO_PORT_1
#define GPIO_PWM1_PORT     GPIO_PORT_1
#define GPIO_PWM0_PIN      GPIO_PIN_0
#define GPIO_PWM1_PIN      GPIO_PIN_1

/****************************************************************************************/ 
/* i2c eeprom configuration                                                             */
/****************************************************************************************/ 
#define DEVICE_ADDR   0x50  // Set slave device address
#define I2C_10BITADDR 0     // 0: 7-bit addressing, 1: 10-bit addressing
#define EEPROM_SIZE   256   // EEPROM size in bytes
#define PAGE_SIZE     8     // EEPROM's page size in bytes
#define SPEED         2     // 1: standard mode (100 kbits/s), 2: fast mode (400 kbits/s)


/****************************************************************************************/ 
/* SPI Flash configuration                                                             */
/****************************************************************************************/  
#define SPI_FLASH_SIZE 131072  // SPI Flash memory size in bytes
#define SPI_FLASH_PAGE 256     // SPI Flash memory page size in bytes
#define SPI_WORD_MODE  0       // 0: 8-bit, 1: 16-bit, 2: 32-bit, 3: 9-bit
#define SPI_SMN_MODE   0       // 0: Master mode, 1: Slave mode
#define SPI_POL_MODE   1       // 0: SPI clk initially low, 1: SPI clk initially high
#define SPI_PHA_MODE   1       // If same with SPI_POL_MODE, data are valid on clk high edge, else on low
#define SPI_MINT_EN    0       // (SPI interrupt to the ICU) 0: Disabled, 1: Enabled
#define SPI_CLK_DIV    2       // (SPI clock divider) 0: 8, 1: 4, 2: 2, 3: 14
#define SPI_CS_PIN     3       // Define Chip Select pin

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

/**
 ****************************************************************************************
 * @brief Each application reserves its own GPIOs here.
 * @return void
 ****************************************************************************************
 */
void GPIO_reservations(void);

/**
 ****************************************************************************************
 * @brief Initialize TXEN and RXEN.
 * @return void
 ****************************************************************************************
 */
void init_TXEN_RXEN_irqs(void);

/**
 ****************************************************************************************
 * @brief Map port pins. The Uart and SPI port pins and GPIO ports are mapped.
 * @return void
 ****************************************************************************************
 */
void set_pad_functions(void);

/**
 ****************************************************************************************
 * @brief Map port pins. The Uart pins are mapped.
 * @return void
 ****************************************************************************************
 */
void set_pad_uart(void);

#endif // _USER_PERIPH_SETUP_H_
