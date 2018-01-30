/**
 ****************************************************************************************
 *
 * @file uart2_read_test.c
 *
 * @brief UART2 asynchronous read example.
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
#include "uart2_print_string.h"

const int READ_CHAR_COUNT = 5;

static char buffer[READ_CHAR_COUNT + 1];
static uint8_t uart2_read_in_progress = 0;

static void uart2_read_completion_cb(uint8_t status)
{
    uart2_read_in_progress = 0;
}

/**
 ****************************************************************************************
 * @brief UART2 asynchronous read example.
 *
 ****************************************************************************************
 */
void uart2_read_test(void)
{
    uart2_print_string("Type 5 characters.\r\n");

    // Indicate that the asynchronous read operation is in progress.
    uart2_read_in_progress = 1;
    
    // Start the asynchronous read of READ_CHAR_COUNT = 5 characters from UART2.
    // The uart2_read_completion_cb callback shall be called from interrupt context
    // when the requested number of characters have been read.
    uart2_read((uint8_t *)buffer, READ_CHAR_COUNT, uart2_read_completion_cb);
    
    // Wait until the callback signals that the asynchronous read operation has been completed.
    while (uart2_read_in_progress)
        ;
    
    buffer[READ_CHAR_COUNT] = 0; // make it a null terminated string
    
    uart2_print_string("You typed the following characters:\r\n");
    uart2_print_string(buffer);
    uart2_print_string("\r\n");
}
