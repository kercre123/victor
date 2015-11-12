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
 * $LastChangedRevision: 2182 $
 */

/** @file
* @brief This file contain functions to handle USB HID related requests
 */

#ifndef HAL_USB_HID_H__
#define HAL_USB_HID_H__

#include <stdint.h>
#include <stdbool.h>

#include "hal_usb.h"

/**
 * Function that process a HID device request
 *
 * This function is usually called from an application that use the USB HID specification
 *
 * @param req The request from host
 * @param data_ptr  Address where this function can put data into which is sent to USB-host
 * @param size  Size of the data to send
 * @param resp The response this function send back to the USB-host
 */

bool hal_usb_hid_device_req_proc(hal_usb_device_req* req, uint8_t** data_ptr, uint8_t* size, hal_usb_dev_req_resp_t* resp) large reentrant;

#endif // HAL_USB_HID_H__
