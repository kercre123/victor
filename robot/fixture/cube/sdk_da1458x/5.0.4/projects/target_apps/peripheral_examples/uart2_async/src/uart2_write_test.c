/**
 ****************************************************************************************
 *
 * @file uart2_write_test.c
 *
 * @brief UART2 asynchronous write example.
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

#include <uart.h>


static const char OUTPUT_STRING[] = "0123456789 0123456789 0123456789 0123456789 0123456789\r\n"
                                    "0123456789 0123456789 0123456789 0123456789 0123456789\r\n"
                                    "0123456789 0123456789 0123456789 0123456789 0123456789\r\n"
                                    "0123456789 0123456789 0123456789 0123456789 0123456789\r\n"
                                    "0123456789 0123456789 0123456789 0123456789 0123456789\r\n"
                                    ;

static uint8_t uart2_write_in_progress = 0;
static void uart2_write_completion_cb(uint8_t status)
{
    uart2_write_in_progress = 0;
}

/**
 ****************************************************************************************
 * @brief UART2 UART2 asynchronous write example.
 *
 ****************************************************************************************
 */
void uart2_write_test(void)
{
    // Indicate that the asynchronous write operation is in progress.
    uart2_write_in_progress = 1;
    
    // Start the asynchronous write to UART2.
    // The uart2_write_completion_cb() callback shall be called from interrupt context
    // when the requested number of characters have been written in the UART2 FIFO.
    uart2_write((uint8_t *)OUTPUT_STRING, sizeof(OUTPUT_STRING) - 1, uart2_write_completion_cb);
    
    // Wait until the callback signals that the asynchronous write operation has been completed.
    while (uart2_write_in_progress)
        ;
}
