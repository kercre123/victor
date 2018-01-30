/**
 ****************************************************************************************
 *
 * @file uart2_print_string.c
 *
 * @brief UART2 print string helper function.
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

#include <string.h>
#include "uart.h"


static uint8_t uart2_write_in_progress = 0;

static void uart2_write_completion_cb(uint8_t status)
{
    uart2_write_in_progress = 0;
}

void uart2_print_string(const char *str)
{
    uart2_write_in_progress = 1;
    
    uart2_write((uint8_t *)str, strlen(str), uart2_write_completion_cb);
    
    while (uart2_write_in_progress)
        ;
}
