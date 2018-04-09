/**
 ****************************************************************************************
 *
 * @file uart2_loopback_test.c
 *
 * @brief UART2 loopback example.
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

#include "datasheet.h"
#include "core_cm0.h"
#include "uart.h"

#include "ring_buffer.h"


static uint8_t rxbuf[1];
static uint8_t txbuf[1];
static uint8_t tx_in_progress = 0;

static void uart_read_cb(uint8_t status)
{
    // Put received byte in ring buffer 
    buffer_put_byte(rxbuf[0]);

    // Start the next asynchronous read of 1 character.
    uart2_read(rxbuf, 1, uart_read_cb);
}

static void uart_write_cb(uint8_t status)
{
    tx_in_progress = 0;
}

/**
 ****************************************************************************************
 * @brief UART2 loopback example 
 *
 ****************************************************************************************
 */
void uart2_loopback_test(void)
{
    // Triggers the 1st asynchronous read of 1 character from UART2.
    // When the character is received, the uart_read_cb() callback
    // shall be called from interrupt context.
    uart2_read(rxbuf, 1, uart_read_cb);
    
    while(1)
    {
        int status;

        // Disable UART2 interrupt
        NVIC_DisableIRQ(UART2_IRQn);
        
        // Attemp to get a byte from the ring buffer
        status = buffer_get_byte(&txbuf[0]);
        
        // Enable UART2 interrupt 
        NVIC_EnableIRQ(UART2_IRQn);

        // Check if a character was read successfully from the ring buffer
        if (0 == status)
        {
            // Write the character to UART2 and then wait synchronously for completion.
            tx_in_progress = 1;
            uart2_write( txbuf, 1, uart_write_cb);
            while (tx_in_progress)
                ;
        }
    }
}
