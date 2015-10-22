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
* @brief This file contain structures and constants defined in Chapter 9 of the USB 2.0 standard
 */

#ifndef HAL_USB_DESC_H__
#define HAL_USB_DESC_H__

#include <stdint.h>

// Standard request codes
#define USB_REQ_GET_STATUS         0x00
#define USB_REQ_CLEAR_FEATURE      0x01
#define USB_REQ_RESERVED_1         0x02
#define USB_REQ_SET_FEATURE        0x03
#define USB_REQ_RESERVED_2         0x04
#define USB_REQ_SET_ADDRESS        0x05
#define USB_REQ_GET_DESCRIPTOR     0x06
#define USB_REQ_SET_DESCRIPTOR     0x07
#define USB_REQ_GET_CONFIGURATION  0x08
#define USB_REQ_SET_CONFIGURATION  0x09
#define USB_REQ_GET_INTERFACE      0x0a
#define USB_REQ_SET_INTERFACE      0x0b
#define USB_REQ_SYNCH_FRAME        0x0c

// Descriptor types
#define USB_DESC_DEVICE           0x01
#define USB_DESC_CONFIGURATION    0x02
#define USB_DESC_STRING           0x03
#define USB_DESC_INTERFACE        0x04
#define USB_DESC_ENDPOINT         0x05
#define USB_DESC_DEVICE_QUAL      0x06
#define USB_DESC_OTHER_SPEED_CONF 0x07
#define USB_DESC_INTERFACE_POWER  0x08
#define USB_DESC_OTG              0x09
#define USB_DESC_DEBUG            0x0A
#define USB_DESC_INTERFACE_ASSOC  0x0B

#define USB_ENDPOINT_TYPE_CONTROL           0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS       0x01
#define USB_ENDPOINT_TYPE_BULK              0x02
#define USB_ENDPOINT_TYPE_INTERRUPT         0x03

// USB device classes
#define USB_DEVICE_CLASS_RESERVED               0x00
#define USB_DEVICE_CLASS_AUDIO	                0x01
#define USB_DEVICE_CLASS_COMMUNICATIONS         0x02
#define USB_DEVICE_CLASS_HUMAN_INTERFACE        0x03
#define USB_DEVICE_CLASS_MONITOR                0x04
#define USB_DEVICE_CLASS_PHYSICAL_INTERFACE     0x05
#define USB_DEVICE_CLASS_POWER                  0x06
#define USB_DEVICE_CLASS_PRINTER                0x07
#define USB_DEVICE_CLASS_STORAGE                0x08
#define USB_DEVICE_CLASS_HUB                    0x09
#define USB_DEVICE_CLASS_APPLICATION_SPECIFIC   0xFE
#define USB_DEVICE_CLASS_VENDOR_SPECIFIC        0xFF


#define USB_CLASS_DESCRIPTOR_HID    0x21
#define USB_CLASS_DESCRIPTOR_REPORT 0x22
#define USB_CLASS_DESCRIPTOR_PHYSICAL_DESCRIPTOR 0x23

#define USB_DEVICE_REMOTE_WAKEUP    0x01
#define USB_ENDPOINT_HALT           0x00
#define USB_TEST_MODE               0x02

typedef struct {
     volatile uint8_t bLength;
     volatile uint8_t bDescriptorType;
     volatile uint16_t bcdUSB;
     volatile uint8_t bDeviceClass;
     volatile uint8_t bDeviceSubClass;
     volatile uint8_t bDeviceProtocol;
     volatile uint8_t bMaxPacketSize0;
     volatile uint16_t idVendor;
     volatile uint16_t idProduct;
     volatile uint16_t bcdDevice;
     volatile uint8_t iManufacturer;
     volatile uint8_t iProduct;
     volatile uint8_t iSerialNumber;
     volatile uint8_t bNumConfigurations;
} hal_usb_dev_desc_t;

typedef struct {
     volatile uint8_t bLength;
     volatile uint8_t bDescriptorType;
     volatile uint16_t wTotalLength;
     volatile uint8_t bNumInterfaces;
     volatile uint8_t bConfigurationValue;
     volatile uint8_t iConfiguration;
     volatile uint8_t bmAttributes;
     volatile uint8_t bMaxPower;
} hal_usb_conf_desc_t;

typedef struct {
     volatile uint8_t bLength;
     volatile uint8_t bDescriptorType;
     volatile uint8_t bInterfaceNumber;
     volatile uint8_t bAlternateSetting;
     volatile uint8_t bNumEndpoints;
     volatile uint8_t bInterfaceClass;
     volatile uint8_t bInterfaceSubClass;
     volatile uint8_t bInterfaceProtocol;
     volatile uint8_t iInterface;
} hal_usb_if_desc_t;

typedef struct {
     volatile uint8_t bLength;
     volatile uint8_t bDescriptorType;
     volatile uint8_t bEndpointAddress;
     volatile uint8_t bmAttributes;
     volatile uint16_t wMaxPacketSize;
     volatile uint8_t bInterval;
} hal_usb_ep_desc_t;

typedef struct {
    volatile uint8_t bLength;
    volatile uint8_t bDescriptorType;
    volatile uint16_t bcdHID;
    volatile uint8_t bCountryCode;
    volatile uint8_t bNumDescriptors;
    volatile uint8_t bDescriptorType2;
    volatile uint16_t wDescriptorLength;
} hal_usb_hid_desc_t;

typedef struct {
     volatile uint8_t* desc;
} hal_usb_string_desc_t;

typedef struct {
     volatile uint8_t bLength;
     volatile uint8_t bDescriptorType;
} hal_usb_common_desc_t;


#endif // HAL_USB_DESC_H__
