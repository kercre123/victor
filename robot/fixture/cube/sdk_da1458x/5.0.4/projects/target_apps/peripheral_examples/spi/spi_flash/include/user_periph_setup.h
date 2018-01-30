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

// Select UART settings
#define UART2_BAUDRATE     UART_BAUDRATE_115K2       // Baudrate in bits/s: {9K6, 14K4, 19K2, 28K8, 38K4, 57K6, 115K2}
#define UART2_DATALENGTH   UART_CHARFORMAT_8         // Datalength in bits: {5, 6, 7, 8}
#define UART2_PARITY       UART_PARITY_NONE          // Parity: {UART_PARITY_NONE, UART_PARITY_EVEN, UART_PARITY_ODD}
#define UART2_STOPBITS     UART_STOPBITS_1           // Stop bits: {UART_STOPBITS_1, UART_STOPBITS_2} 
#define UART2_FLOWCONTROL  UART_FLOWCONTROL_DISABLED // Flow control: {UART_FLOWCONTROL_DISABLED, UART_FLOWCONTROL_ENABLED}

#define UART2_GPIO_PORT    GPIO_PORT_0
#define UART2_TX_PIN       GPIO_PIN_4
#define UART2_RX_PIN       GPIO_PIN_7

#if (defined(__DA14583__))
    #define SPI_GPIO_PORT  	GPIO_PORT_2
    #define SPI_CLK_PIN    	GPIO_PIN_0                
    #define SPI_CS_PIN     	GPIO_PIN_3                           
    #define SPI_DI_PIN     	GPIO_PIN_4  
    #define SPI_DO_PIN     	GPIO_PIN_9               
#else 
    #define SPI_GPIO_PORT  GPIO_PORT_0 
    #define SPI_CLK_PIN    GPIO_PIN_0
    #define SPI_CS_PIN     GPIO_PIN_3
    #define SPI_DI_PIN     GPIO_PIN_5
    #define SPI_DO_PIN     GPIO_PIN_6
#endif

#define UART_ENABLED
#define SPI_ENABLED
#define CFG_PRINTF
// SPI Flash options
#define SPI_FLASH_SIZE 131072              // SPI Flash memory size in bytes
#define SPI_FLASH_PAGE 256                 // SPI Flash memory page size in bytes

//SPI initialization parameters
#define SPI_WORD_MODE  SPI_8BIT_MODE       // Select SPI bit mode
#define SPI_SMN_MODE   SPI_MASTER_MODE     // {SPI_MASTER_MODE, SPI_SLAVE_MODE}
#define SPI_POL_MODE   SPI_CLK_INIT_HIGH   // {SPI_CLK_INIT_LOW, SPI_CLK_INIT_HIGH}
#define SPI_PHA_MODE   SPI_PHASE_1         // {SPI_PHA_MODE_0, SPI_PHA_MODE_1}
#define SPI_MINT_EN    SPI_NO_MINT         // {SPI_MINT_DISABLE, SPI_MINT_ENABLE}
#define SPI_CLK_DIV    SPI_XTAL_DIV_2      // Select SPI clock divider between 8, 4, 2 and 14

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
