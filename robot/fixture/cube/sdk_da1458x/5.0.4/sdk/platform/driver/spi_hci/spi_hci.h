#ifndef _HCI_SPI_
#define _HCI_SPI_

/**
 ****************************************************************************************
 *
 * @file spi_hci.h
 *
 * @brief Header file for SPI Driver for HCI over SPI operation.
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
#include <stdbool.h>

/*
 * ENUMERATION DEFINITIONS
 *****************************************************************************************
 */

/// SPI status values
enum
{
    /// status ok
    SPI_STATUS_OK,
    /// status not ok
    SPI_STATUS_ERROR
};

/// Control bytes for 5-wire SPI protocol
enum
{
    /// Flow on byte from slave
    FLOW_ON_BYTE  = 0x06,
    /// Flow off byte from slave
    FLOW_OFF_BYTE = 0x07,
    /// DREADY acknowledgement byte from master
    DREADY_ACK    = 0x08,
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initializes the SPI module as slave to default values.
 *****************************************************************************************
 */
void spi_hci_slave_init(void);

#ifndef CFG_ROM
/**
 ****************************************************************************************
 * @brief Enable SPI flow.
 *****************************************************************************************
 */
void spi_hci_flow_on_func(void);

/**
 ****************************************************************************************
 * @brief Disable SPI flow.
 *****************************************************************************************
 */
bool spi_hci_flow_off_func(void);
#endif //CFG_ROM

/**
 ****************************************************************************************
 * @brief Starts a data reception.
 *
 * As soon as the end of the data transfer or a buffer overflow is detected,
 * the hci_uart_rx_done function is executed.
 *
 * @param[in,out]  bufptr Pointer to the RX buffer
 * @param[in]      size   Size of the expected reception
 *****************************************************************************************
 */
void spi_hci_read_func(uint8_t *bufptr, uint32_t size, void (*callback) (uint8_t));

/**
 ****************************************************************************************
 * @brief Starts a data transmission.
 *
 * As soon as the end of the data transfer is detected, the hci_uart_tx_done function is
 * executed.
 *
 * @param[in]  bufptr Pointer to the TX buffer
 * @param[in]  size   Size of the transmission
 *****************************************************************************************
 */
void spi_hci_write_func(uint8_t *bufptr, uint32_t size, void (*callback) (uint8_t));


/// @} HCI_SPI
#endif // _HCI_SPI_
