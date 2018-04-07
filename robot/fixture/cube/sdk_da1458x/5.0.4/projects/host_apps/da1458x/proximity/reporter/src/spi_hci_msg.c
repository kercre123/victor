/**
 ****************************************************************************************
 *
 * @file spi_hci.c
 *
 * @brief functions for exchange  of hci messages over spi. V2_0
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
 ****************************************************************************************
 * Include files
 ****************************************************************************************
 */
#include <stdint.h>
#include "app.h"
#include "global_io.h"
#include "gpio.h"
#include "spi.h"
#include "spi_hci_msg.h"
#include "queue.h"
#include <core_cm0.h>
#include "user_periph_setup.h"

/*
 ****************************************************************************************
 * Global variables
 ****************************************************************************************
 */
 
// Receive buffer
extern uint8_t rd_data[256];

/*
 ****************************************************************************************
 * Function definitions
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Enables interrupt for DREADY GPIO
 ****************************************************************************************
 */
void dready_irq_enable(void)
{
    // set isr callback
    GPIO_RegisterCallback(GPIO0_IRQn, spi_dready_isr);
    
    // enable GPIO interrupt for dready pin
    GPIO_EnableIRQ(SPI_GPIO_PORT, SPI_DREADY_PIN, GPIO0_IRQn, false, false, 0);
}
/**
 ****************************************************************************************
 * @brief Receive an HCI message over the SPI
 * @param[in] *msg_ptr:  pointer to the position the received data will be stored
 * @return Message type
 ****************************************************************************************
 */
uint16_t spi_receive_hci_msg(uint8_t *msg_ptr)
{
    uint16_t i, size, msg_type;
    
    uint8_t * rd_ptr;
    unsigned char *msg;
    
    rd_ptr = msg_ptr+2;         // Discard 0x0500;

    spi_cs_high();              // Close CS 
    spi_cs_low();               // Open CS
    
    spi_access(DREADY_ACK);    // Write DREADY acknowledge
    
    *msg_ptr++ = spi_access(0x00);
    
    if (*(msg_ptr-1) == 0x05)                   // HCI Message
    {
        msg_ptr++;                              // Align 16-bit
        for (i=0;i<6;i++)
        {
            *msg_ptr++ = spi_access(0x00);
        }
        *msg_ptr = spi_access(0x00);
        size = *msg_ptr++;
        *msg_ptr = spi_access(0x00);
        size += *msg_ptr++<<8;
        for (i=0; i<size; i++)
        {
            *msg_ptr++ = spi_access(0x00);
        }
        
        msg = malloc(size+8);
        memcpy(msg,rd_ptr,size+8);
        EnQueue(&SPIRxQueue, msg);
        app_env.size_rx_queue++;
        msg_type = 1;                               // GTL Message
    }
    else if (*(msg_ptr-1) == FLOW_ON_BYTE)          // Flow ON
    {
        app_env.slave_on_sleep = SLAVE_AVAILABLE;
        msg_type = 2;                               // Flow Message
    }
    else if (*(msg_ptr-1) == FLOW_OFF_BYTE)         // Flow OFF
    {
        app_env.slave_on_sleep = SLAVE_UNAVAILABLE;
        msg_type = 3;                               // Flow Message
    }
    else
    {
        msg_type = 0;   // Error    
    }
        
    spi_cs_high();      // Close CS

    return msg_type;
}


/**
 ****************************************************************************************
 * @brief Send an HCI message over the SPI
 * @param[in] size:      size of data to send in bytes
 * @param[in] *msg_ptr:  pointer to the first byte to be sent
 ****************************************************************************************
 */
void spi_send_hci_msg(uint16_t size, uint8_t *msg_ptr)
{
    uint16_t i;
    
    // disable dready interrupt
    NVIC_DisableIRQ(GPIO0_IRQn);
    
    // Polling DREADY to detect if data is being received
    while(GPIO_GetPinStatus(SPI_GPIO_PORT, SPI_DREADY_PIN));
    
    spi_cs_high();  // Close CS 
    spi_cs_low();   // Open CS
    
    spi_access(0x05);

    for (i=0; i<size; i++)
    {
        // Polling dready to detect if slave needs time for message allocation
        while(GetBits16(SPI_DATA_REG,1<<SPI_DREADY_PIN)==1);
        // send byte over spi
        spi_access(*msg_ptr++);
    }
    
    spi_cs_high();  // Close CS
    
    // clear pending dready interrupts while transmitting
    NVIC_ClearPendingIRQ(GPIO0_IRQn);
    // enable dready interrupt
    NVIC_EnableIRQ(GPIO0_IRQn); 
}

/**
 ****************************************************************************************
 * @brief DREADY interrupt service routine
 ****************************************************************************************
 */
void spi_dready_isr(void)
{
    // if dready is not asserted, this is probably a dummy interrupt
    if (GetBits16(SPI_DATA_REG,1<<SPI_DREADY_PIN)==0)
    {
        // do nothing
        return;
    }
    
    // disable dready interrupt
    NVIC_DisableIRQ(GPIO0_IRQn);
    
    // call GTL message receive function
    spi_receive_hci_msg(rd_data);
    
    NVIC_ClearPendingIRQ(GPIO0_IRQn);   // Clear interrupt requests while disabled
    NVIC_EnableIRQ(GPIO0_IRQn);         // Enable this interrupt
} 
