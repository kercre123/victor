#ifndef _SPI_3WIRE_
#define _SPI_3WIRE_

#include <stdint.h>
#include "spi.h"
#include "gpio.h"

/**
 ****************************************************************************************
 *
 * @file spi_3wire.h
 *
 * @brief 3-wire SPI low level driver.
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
 
typedef struct
{
    GPIO_PORT port;
    GPIO_PIN pin;
}SPI_Pin;
 
struct SPI_Config
{
    SPI_Word_Mode_t SPI_Word_Mode;
    SPI_Role_t SPI_Role;
    SPI_Polarity_Mode_t SPI_Polarity_Mode;
    SPI_PHA_Mode_t SPI_PHA_Mode;
    SPI_MINT_Mode_t SPI_MINT_Mode;
    SPI_XTAL_Freq_t SPI_XTAL_Freq;
    SPI_Pin cs;
    SPI_Pin sdio;
    SPI_Pin clk;
};
 
uint8_t read_from_3wire_SPI_register(uint8_t registerIndex, uint8_t is_last_transaction);
void initialize_3wire_spi(struct SPI_Config *cfg, bool enable_interrupt,  uint16_t TsradCounterToSet);
void deactivate_3wire_spi(void);
void write_to_3wire_SPI_register(uint8_t registerIndex, uint8_t valueToWrite);
void burst_write_to_3wire_SPI_register(uint8_t registerIndex);
uint8_t burst_read_from_3wire_SPI_register(void);

void spi3wire_cs_high (void);
void spi3wire_cs_low (void);
//uint8_t burst_read_from_3wire_SPI_register(uint8_t registerIndex);

#endif // _SPI_3WIRE_
