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
#include "spi.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define UART_GPIO_PORT  GPIO_PORT_0
#define UART_TX_PIN     GPIO_PIN_4
#define UART_RX_PIN     GPIO_PIN_5
#undef  UART_ENABLED

#define SPI_GPIO_PORT   GPIO_PORT_0
#define SPI_CLK_PIN     GPIO_PIN_0
#define SPI_CS_PIN      GPIO_PIN_1
#define SPI_DI_PIN      GPIO_PIN_2
#define SPI_DO_PIN      GPIO_PIN_3
#define SPI_DREADY_PIN  GPIO_PIN_7
/// define SPI settings
#define SPI_WORD_MODE   SPI_MODE_8BIT
#define SPI_ROLE        SPI_ROLE_MASTER
#define SPI_POL_MODE    SPI_CLK_IDLE_POL_LOW
#define SPI_PHA_MODE    SPI_PHA_MODE_0
#define SPI_MINT_MODE   SPI_MINT_ENABLE
#define SPI_FREQ_MODE   SPI_XTAL_DIV_8
#define SPI_ENABLED

#endif // _USER_PERIPH_SETUP_H_
