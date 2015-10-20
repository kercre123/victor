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
 * @brief This file define USB HID related structures
 */

#ifndef HAL_USB_HID_DESC_H__
#define HAL_USB_HID_DESC_H__

#include "hal_usb_desc.h"

/**
 * Structure containing the HID descriptor and the associated HID report descriptor
 */

typedef struct {
    hal_usb_hid_desc_t* hid_desc;
    uint8_t* hid_report_desc;
    uint8_t hid_report_desc_size;
} hal_usb_hid_t;

#endif // HAL_USB_HID_DESC_H__
