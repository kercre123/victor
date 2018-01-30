#ifndef _REG_SPI_
#define _REG_SPI_

/**
 ****************************************************************************************
 *
 * @file reg_spi.h
 *
 * @brief SPI register access inline functions. 
 *
 * $Rev: 2 $
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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "gpio.h"
#include "user_periph_setup.h"

/*
 * DEFINES
 *****************************************************************************************
 */
#define SPI_DATA_REG        (GPIO_BASE + (SPI_GPIO_PORT << 5) )
#define SPI_SET_DATA_REG    (SPI_DATA_REG + 2)
#define SPI_RESET_DATA_REG  (SPI_DATA_REG + 4)

/**
 ****************************************************************************************
 * @brief Reads a byte from SPI after a transaction has been made. 
 *
 * @return   Received byte.
 ****************************************************************************************
 */
__INLINE uint8_t spi_read_byte(void)
{
    uint8_t rd_byte;
    rd_byte = 0xFF&GetWord16(SPI_RX_TX_REG0);     // Read byte from SPI
    SetWord16(SPI_CLEAR_INT_REG, 0x01);           // Clear pending flag
    return rd_byte;                               // Return received data
}

/**
 ****************************************************************************************
 * @brief Check for received data in SPI Rx FIFO. 
 *
 * @return SPI_INT_BIT value: 1 if there are received data available, 0 otherwise.
 ****************************************************************************************
 */
__INLINE uint8_t spi_data_rdy_getf(void)
{
    uint8_t localVal = 0xFF&GetBits16(SPI_CTRL_REG,SPI_INT_BIT);
    return localVal;
}

/**
 ****************************************************************************************
 * @brief Poll CS pin. 
 *
 * @return CS pin level.
 ****************************************************************************************
 */
__INLINE uint8_t spi_cs_getf(void)
{
    uint8_t localVal = GetBits16(SPI_DATA_REG, 1 << SPI_CS_PIN);
    return localVal;
}

/**
 ****************************************************************************************
 * @brief Asserts DREADY signal. 
 ****************************************************************************************
 */
__INLINE void spi_dready_high(void)
{
    SetWord16(SPI_SET_DATA_REG, 1 << SPI_DREADY_PIN);
}

/**
 ****************************************************************************************
 * @brief De-asserts DREADY signal. 
 ****************************************************************************************
 */
__INLINE void spi_dready_low(void)
{
    SetWord16(SPI_RESET_DATA_REG, 1 << SPI_DREADY_PIN);
}

/**
 ****************************************************************************************
 * @brief Polls SPI_INT_BIT till the next SPI transaction. 
 ****************************************************************************************
 */
__INLINE void spi_wait_for_transfer(void)
{
    while(GetBits16(SPI_CTRL_REG,SPI_INT_BIT)==0);
}

/**
 ****************************************************************************************
 * @brief Reads SPI_BUSY bit to determine whether SPI module is busy.
 *
 * @return SPI_BUSY bit value.
 ****************************************************************************************
 */
__INLINE uint8_t spi_busy_getf(void)
{
    uint8_t localVal = GetBits16(SPI_CTRL_REG1,SPI_BUSY);
    return localVal;
}

/**
 ****************************************************************************************
 * @brief Reads SPI_TXH bit to determine whether SPI Tx FIFO is full.
 *
 * @return SPI_TXH bit value.
 ****************************************************************************************
 */
__INLINE uint8_t spi_txfifo_full_getf(void)
{
    uint8_t localVal = GetBits16(SPI_CTRL_REG,SPI_TXH);
    return localVal;
}

/**
 ****************************************************************************************
 * @brief Writes a byte to SPI Rx/Tx register.
 *
 * @param[in]   wr_byte:    Byte to be written. 
 ****************************************************************************************
 */
__INLINE void spi_rxtxreg_write(uint8_t wr_byte)
{
    SetWord16(SPI_RX_TX_REG0,wr_byte);
}

/**
 ****************************************************************************************
 * @brief Reads a byte from SPI Rx/Tx register.
 *
 * @return Read byte.
 ****************************************************************************************
 */
__INLINE uint8_t spi_rxtxreg_read(void)
{
    uint8_t rd_byte = 0xFF&GetWord16(SPI_RX_TX_REG0);
    return rd_byte;
}

/**
 ****************************************************************************************
 * @brief Clears SPI pending flag.
 ****************************************************************************************
 */
__INLINE void spi_int_bit_clear(void)
{
    SetWord16(SPI_CLEAR_INT_REG, 0x01);
}

/**
 ****************************************************************************************
 * @brief Write a byte to SPI.
 *
 * @param[in] wr_byte: Byte to be transmitted.
 ****************************************************************************************
 */
__INLINE void spi_write_byte(uint8_t wr_byte)
{
    while (GetBits16(SPI_CTRL_REG,SPI_TXH)==1);    // Wait if SPI Tx FIFO is full
    SetWord16(SPI_RX_TX_REG0, 0xFF&wr_byte);       // Send byte

    while(spi_data_rdy_getf())  // Still data in the SPI RX FIFO
    {
        spi_rxtxreg_read();     // Get byte from SPI RX FIFO
        spi_int_bit_clear();    // Clear pending flag
    }
}

#endif /// _REG_SPI_
