/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic 
 * Semiconductor ASA.Terms and conditions of usage are described in detail 
 * in NORDIC SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT. 
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRENTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *              
 * $LastChangedRevision: 133 $
 */

/** @file
 * @brief Implementation of hal_flash for nRF24LU1+
 */

#include "nrf24lu1p.h"
#include "hal_flash.h"

void hal_flash_page_erase(uint8_t pn)
{
    uint8_t ckcon_val;

    //Backup CKCON value and force CKCON to 0x01. nRF24LU1p PAN 011 #2
    ckcon_val = CKCON;
    CKCON = 0x01;
    // Save interrupt enable state and disable interrupts:
    F0 = EA;
    EA = 0;
    // Enable flash write operation:
    FCR = 0xAA;
    FCR = 0x55;
    WEN = 1;
    //
    // Write the page address to FCR to start the page erase operation. This
    // operation is "self timed" when executing from the flash; the CPU will
    // halt until the operation is finished:
    FCR = pn;
    //
    // When running from XDATA RAM we need to wait for the operation to finish:
    while(RDYN == 1)
        ;
    WEN = 0;
    EA = F0; // Restore interrupt enable state
    CKCON = ckcon_val; //Restore CKCON state
}

void hal_flash_byte_write(uint16_t a, uint8_t b)
{
    uint8_t xdata * pb;
    uint8_t ckcon_val;

    //Backup CKCON value and force CKCON to 0x01. nRF24LU1p PAN 011 #2
    ckcon_val = CKCON;
    CKCON = 0x01;
    // Save interrupt enable state and disable interrupts:
    F0 = EA;
    EA = 0;
    // Enable flash write operation:
    FCR = 0xAA;
    FCR = 0x55;
    WEN = 1;
    //
    // Write the byte directly to the flash. This operation is "self timed" when
    // executing from the flash; the CPU will halt until the operation is
    // finished:
    pb = (uint8_t xdata *)a;
    *pb = b;
    //
    // When running from XDATA RAM we need to wait for the operation to finish:
    while(RDYN == 1)
        ;
    WEN = 0;
    EA = F0; // Restore interrupt enable state
    CKCON = ckcon_val; //Restore CKCON state
}

void hal_flash_bytes_write(uint16_t a, const uint8_t *p, uint16_t n)
{
    uint8_t xdata * pb;
    uint8_t ckcon_val;

    //Backup CKCON value and force CKCON to 0x01. nRF24LU1p PAN 011 #2
    ckcon_val = CKCON;
    CKCON = 0x01;
    // Save interrupt enable state and disable interrupts:
    F0 = EA;
    EA = 0;
    // Enable flash write operation:
    FCR = 0xAA;
    FCR = 0x55;
    WEN = 1;
    //
    // Write the bytes directly to the flash. This operation is
    // "self timed"; the CPU will halt until the operation is
    // finished:
    pb = (uint8_t xdata *)a;
    while(n--)
    {
        //lint --e{613} Suppress possible use of null pointer warning:
        *pb++ = *p++;
        //
        // When running from XDATA RAM we need to wait for the operation to
        // finish:
        while(RDYN == 1) {}
    }
    WEN = 0;
    EA = F0; // Restore interrupt enable state
    CKCON = ckcon_val; //Restore CKCON state
}

uint8_t hal_flash_byte_read(uint16_t a)
{
    //lint --e{613} Suppress possible use of null pointer warning:
    uint8_t xdata *pb = (uint8_t xdata *)a;
    return *pb;
}

void hal_flash_bytes_read(uint16_t a, uint8_t *p, uint16_t n)
{  
    uint8_t xdata *pb = (uint8_t xdata *)a;
    while(n--)
    {
        //lint --e{613} Suppress possible use of null pointer warning:
        *p = *pb;
        pb++;
        p++;
    }
}