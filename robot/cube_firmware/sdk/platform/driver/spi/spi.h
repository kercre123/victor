/**
 ****************************************************************************************
 *
 * @file spi.h
 *
 * @brief spi interface driver header file.
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

#ifndef _SPI_H_
#define _SPI_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gpio.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define SPI_DRIVER_VERSION       (2)
#define SPI_DRIVER_SUBVERSION    (0)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef enum SPI_WORD_MODES{
	SPI_MODE_8BIT,
	SPI_MODE_16BIT,
	SPI_MODE_32BIT,
	SPI_MODE_9BIT,
}SPI_Word_Mode_t;

typedef enum SPI_ROLES{
	SPI_ROLE_MASTER,
	SPI_ROLE_SLAVE,
}SPI_Role_t;

typedef enum SPI_POL_MODES{
	SPI_CLK_IDLE_POL_LOW,
	SPI_CLK_IDLE_POL_HIGH,
}SPI_Polarity_Mode_t;

typedef enum SPI_PHA_MODES{
	SPI_PHA_MODE_0,
	SPI_PHA_MODE_1,
}SPI_PHA_Mode_t;

typedef enum SPI_MINT_MODES{
	SPI_MINT_DISABLE,
	SPI_MINT_ENABLE,
}SPI_MINT_Mode_t;

typedef enum SPI_FREQ_MODES{
  SPI_XTAL_DIV_8,
  SPI_XTAL_DIV_4,
	SPI_XTAL_DIV_2,
	SPI_XTAL_DIV_14,
}SPI_XTAL_Freq_t;

typedef struct
{
	GPIO_PORT port;
	GPIO_PIN pin;
}SPI_Pad_t;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief SPI spi_cs_low, Start SPI transaction.
 * @return void
 ****************************************************************************************
 */
void spi_cs_low(void);

/**
 ****************************************************************************************
 * @brief SPI spi_cs_low, finish SPI transaction.
 * @return void
 ****************************************************************************************
 */
void spi_cs_high(void);

/**
 ****************************************************************************************
 * @brief SPI initialization.
 * @param[in] cs_pad_param Port/pin to be assigned to the SPI CS signal
 * @param[in] bitmode      SPI_MODE_8BIT,	SPI_MODE_16BIT,	SPI_MODE_32BIT,	SPI_MODE_9BIT
 * @param[in] role         SPI_ROLE_MASTER, SPI_ROLE_SLAVE
 * @param[in] clk_pol      SPI_CLK_IDLE_POL_LOW, SPI_CLK_IDLE_POL_HIGH
 * @param[in] pha_mode     SPI_PHA_MODE_0, SPI_PHA_MODE_1 : spi sampling edge selection (refer to datasheet p.53-54)
 * @param[in] irq          SPI_MINT_DISABLE: disable SPI_INT_BIT to NVIC, SPI_MINT_ENABLE: enable SPI_INT_BIT to NVIC. Note that the SPI_INT interrupt is shared with AD_INT interrupt
 * @param[in] freq         SPI_XTAL_DIV_8, SPI_XTAL_DIV_4, SPI_XTAL_DIV_2, SPI_XTAL_DIV_14 - Select SPI_CLK clock frequency in master mode: (XTAL)/ (CLK_PER_REG * DIV) where div: 8,4,2,14
 * @return void
 ****************************************************************************************
 */
void spi_init(SPI_Pad_t *cs_pad_param, SPI_Word_Mode_t bitmode, SPI_Role_t role, SPI_Polarity_Mode_t clk_pol, SPI_PHA_Mode_t pha_mode, SPI_MINT_Mode_t irq, SPI_XTAL_Freq_t freq);

/**
 ****************************************************************************************
 * @brief Selects the SPI bitmode.
 * @param[in] spiBitMode The desired SPI bitmode
 * @return void
 ****************************************************************************************
 */
void spi_set_bitmode(SPI_Word_Mode_t spiBitMode);

/**
 ****************************************************************************************
 * @brief Releases SPI.
 * @return void
 ****************************************************************************************
 */
void spi_release(void);

/**
 ****************************************************************************************
 * @brief Disables SPI block.
 * @return void
 ****************************************************************************************
 */
void spi_disable(void);

/**
 ****************************************************************************************
 * @brief SPI access to slave without acting on CS (works in all bit modes).
 * @param[in] dataToSend Data to send
 * @return data read
 ****************************************************************************************
 */
uint32_t spi_access(uint32_t dataToSend);

/**
 ****************************************************************************************
 * @brief SPI transaction to slave with CS handling (works in all bit modes).
 * @param[in] dataToSend Data to send
 * @return data read
 ****************************************************************************************
 */
uint32_t spi_transaction(uint32_t dataToSend);

#endif // _SPI_H_
