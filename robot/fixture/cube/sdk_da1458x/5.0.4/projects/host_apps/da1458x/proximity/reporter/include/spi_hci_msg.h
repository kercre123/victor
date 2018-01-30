/**
 ****************************************************************************************
 *
 * @file spi_hci_msg.h
 *
 * @brief hci over spi message exchange header file.
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

#ifndef _SPI_HCI_
#define _SPI_HCI_ 
 
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "user_periph_setup.h"

/*
 * DEFINES
 *****************************************************************************************
 */
#define SPI_DATA_REG        (GPIO_BASE + (SPI_GPIO_PORT << 5) )
#define SPI_SET_DATA_REG    (SPI_DATA_REG + 2)
#define SPI_RESET_DATA_REG  (SPI_DATA_REG + 4)
#define SPI_DREADY_IRQ      (SPI_GPIO_PORT*7 + SPI_DREADY_PIN + 1)

/*
 * ENUMERATION DEFINITIONS
 *****************************************************************************************
 */

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
* @brief Enables interrupt for DREADY GPIO
****************************************************************************************
*/
void dready_irq_enable(void);

/**
****************************************************************************************
* @brief Receive an HCI message over the SPI
* @param[in] *msg_ptr: 	pointer to the position the received data will be stored
* @return Message type
****************************************************************************************
*/
uint16_t spi_receive_hci_msg(uint8_t * msg_ptr);

/**
****************************************************************************************
* @brief Send an HCI message over the SPI
* @param[in] size: 			size of data to send in bytes
* @param[in] *msg_ptr: 	pointer to the first byte to be sent
****************************************************************************************
*/
void spi_send_hci_msg(uint16_t size, uint8_t *msg_ptr);

/**
****************************************************************************************
* @brief DREADY interrupt service routine
****************************************************************************************
*/
void spi_dready_isr(void);

// end _SPI_HCI_MSG_
#endif
