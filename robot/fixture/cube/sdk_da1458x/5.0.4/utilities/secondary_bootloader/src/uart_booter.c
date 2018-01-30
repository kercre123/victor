/**
 ****************************************************************************************
 *
 * @file uart_booter.c
 *
 * @brief Firmware download through UART
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

#include <stdio.h>
#include "uart_booter.h"
#include "user_periph_setup.h"
#include "gpio.h"
#include "uart.h"


volatile WORD timeout_multi=0;
volatile WORD timeout_occur=0;

/**
****************************************************************************************
* @brief download firmware from UART interface
****************************************************************************************
*/
int FwDownload(void)
{
    int fw_size;
    int i;
    char *code;
    char crc_code;
    char recv_byte;

    uart_send_byte(STX);                                // send start TX char

    if (0 == uart_receive_byte(&recv_byte)) return -1;  // receive SOH
    if (recv_byte != SOH) return -2;

    if (0 == uart_receive_byte(&recv_byte)) return -3;  // receive FW length LSB
    fw_size = recv_byte;

    if (0 == uart_receive_byte(&recv_byte)) return -4;  // receive FW length MSB
    fw_size += (recv_byte << 8);

    if (fw_size > max_code_length)                      // check if the program fits into memory
    {
        uart_send_byte(NAK);                            // if it does not fit send ACK
        return -5;
    }
    else
    {
        uart_send_byte(ACK);                            // if it fits send ACK
    }

    crc_code = 0;
    code = (char*) (SYSRAM_COPY_BASE_ADDRESS);          // set pointer to SYSRAM
    for (i = 0; i < fw_size; i++)                       // copy code from UART to RAM
    {
        if (0 == uart_receive_byte(&recv_byte)) return -6; // receive code byte
        crc_code ^= recv_byte;                          // update CRC
        *code = recv_byte;                              // write to RAM
        code++;                                         // increase RAM pointer
    }
    uart_send_byte(crc_code);                           // send CRC byte

    if (0 == uart_receive_byte(&recv_byte)) return -7;  // receive ACK
    if (recv_byte != ACK) return -8;

    return fw_size;
}


