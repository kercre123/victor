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
 * @brief This file contain definitions related to the USB-controller and internal structures
 */

#ifndef USB_H__
#define USB_H__

#include <stdint.h>

#include "hal_usb.h"

#define USB_ENDPOINT_IN_COUNT 5
#define USB_ENDPOINT_OUT_COUNT 5

#define USB_EP0_HSNAK() do { ep0cs = 0x02; } while(0)
#define USB_EP0_STALL() do { ep0cs = 0x11; } while(0) // Set both DSTALL and STALL when we want to stall a request during a SETUP transaction
#define USB_EP0_DSTALL() do { ep0cs |= 0x10; } while(0)

#define INT_SUDAV    0x00
#define INT_SOF      0x04
#define INT_SUTOK    0x08
#define INT_SUSPEND  0x0C
#define INT_USBRESET 0x10
#define INT_EP0IN    0x18
#define INT_EP0OUT   0x1C
#define INT_EP1IN    0x20
#define INT_EP1OUT   0x24
#define INT_EP2IN    0x28
#define INT_EP2OUT   0x2C
#define INT_EP3IN    0x30
#define INT_EP3OUT   0x34
#define INT_EP4IN    0x38
#define INT_EP4OUT   0x3C
#define INT_EP5IN    0x40
#define INT_EP5OUT   0x44

#define BM_REQUEST_TYPE  0
#define B_REQUEST        1
#define W_VALUE          2
#define W_INDEX          4
#define W_LENGTH         6

typedef struct
{
    uint8_t *data_ptr;
    uint8_t data_size;
    uint8_t pkt_size;
} packetizer_t;

#endif
