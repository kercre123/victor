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
 * @brief USB register layout and interrupts
 *
 * This file contain:
 * - the USB-controller register layout
 * - the USB-controller interrupts towards the MCU
 *
 * The usb_map_t structure is set to point at xdata address 0x0000
 */
#ifndef USB_MAP_H__
#define USB_MAP_H__

#ifdef ALLOCATE_USB_MAP
#define EXTERN
#define _AT_ _at_
#else
#define EXTERN extern
#define _AT_ ;/ ## /
#endif

#define USB_EP_DEFAULT_BUF_SIZE 0x20 // (32)

__no_init EXTERN xdata volatile uint8_t out5buf[USB_EP_DEFAULT_BUF_SIZE] _AT_ 0xC440;
__no_init EXTERN xdata volatile uint8_t in5buf[USB_EP_DEFAULT_BUF_SIZE]  _AT_ 0xC480;
__no_init EXTERN xdata volatile uint8_t out4buf[USB_EP_DEFAULT_BUF_SIZE] _AT_ 0xC4C0;
__no_init EXTERN xdata volatile uint8_t in4buf[USB_EP_DEFAULT_BUF_SIZE]  _AT_ 0xC500;
__no_init EXTERN xdata volatile uint8_t out3buf[USB_EP_DEFAULT_BUF_SIZE] _AT_ 0xC540;
__no_init EXTERN xdata volatile uint8_t in3buf[USB_EP_DEFAULT_BUF_SIZE]  _AT_ 0xC580;
__no_init EXTERN xdata volatile uint8_t out2buf[USB_EP_DEFAULT_BUF_SIZE] _AT_ 0xC5C0;
__no_init EXTERN xdata volatile uint8_t in2buf[USB_EP_DEFAULT_BUF_SIZE]  _AT_ 0xC600;
__no_init EXTERN xdata volatile uint8_t out1buf[USB_EP_DEFAULT_BUF_SIZE] _AT_ 0xC640;
__no_init EXTERN xdata volatile uint8_t in1buf[USB_EP_DEFAULT_BUF_SIZE]  _AT_ 0xC680;
__no_init EXTERN xdata volatile uint8_t out0buf[USB_EP_DEFAULT_BUF_SIZE] _AT_ 0xC6C0;
__no_init EXTERN xdata volatile uint8_t in0buf[USB_EP_DEFAULT_BUF_SIZE]  _AT_ 0xC700;
__no_init EXTERN xdata volatile uint8_t out8data                         _AT_ 0xC760;
__no_init EXTERN xdata volatile uint8_t in8data                          _AT_ 0xC768;
__no_init EXTERN xdata volatile uint8_t out8bch                          _AT_ 0xC770;
__no_init EXTERN xdata volatile uint8_t out8bcl                          _AT_ 0xC771;
__no_init EXTERN xdata volatile uint8_t bout1addr                        _AT_ 0xC781;
__no_init EXTERN xdata volatile uint8_t bout2addr                        _AT_ 0xC782;
__no_init EXTERN xdata volatile uint8_t bout3addr                        _AT_ 0xC783;
__no_init EXTERN xdata volatile uint8_t bout4addr                        _AT_ 0xC784;
__no_init EXTERN xdata volatile uint8_t bout5addr                        _AT_ 0xC785;
__no_init EXTERN xdata volatile uint8_t binstaddr                        _AT_ 0xC788;
__no_init EXTERN xdata volatile uint8_t bin1addr                         _AT_ 0xC789;
__no_init EXTERN xdata volatile uint8_t bin2addr                         _AT_ 0xC78A;
__no_init EXTERN xdata volatile uint8_t bin3addr                         _AT_ 0xC78B;
__no_init EXTERN xdata volatile uint8_t bin4addr                         _AT_ 0xC78C;
__no_init EXTERN xdata volatile uint8_t bin5addr                         _AT_ 0xC78D;
__no_init EXTERN xdata volatile uint8_t isoerr                           _AT_ 0xC7A0;
__no_init EXTERN xdata volatile uint8_t zbcout                           _AT_ 0xC7A2;
__no_init EXTERN xdata volatile uint8_t ivec                             _AT_ 0xC7A8;
__no_init EXTERN xdata volatile uint8_t in_irq                           _AT_ 0xC7A9;
__no_init EXTERN xdata volatile uint8_t out_irq                          _AT_ 0xC7AA;
__no_init EXTERN xdata volatile uint8_t usbirq                           _AT_ 0xC7AB;
__no_init EXTERN xdata volatile uint8_t in_ien                           _AT_ 0xC7AC;
__no_init EXTERN xdata volatile uint8_t out_ien                          _AT_ 0xC7AD;
__no_init EXTERN xdata volatile uint8_t usbien                           _AT_ 0xC7AE;
__no_init EXTERN xdata volatile uint8_t usbbav                           _AT_ 0xC7AF;
__no_init EXTERN xdata volatile uint8_t ep0cs                            _AT_ 0xC7B4;
__no_init EXTERN xdata volatile uint8_t in0bc                            _AT_ 0xC7B5;
__no_init EXTERN xdata volatile uint8_t in1cs                            _AT_ 0xC7B6;
__no_init EXTERN xdata volatile uint8_t in1bc                            _AT_ 0xC7B7;
__no_init EXTERN xdata volatile uint8_t in2cs                            _AT_ 0xC7B8;
__no_init EXTERN xdata volatile uint8_t in2bc                            _AT_ 0xC7B9;
__no_init EXTERN xdata volatile uint8_t in3cs                            _AT_ 0xC7BA;
__no_init EXTERN xdata volatile uint8_t in3bc                            _AT_ 0xC7BB;
__no_init EXTERN xdata volatile uint8_t in4cs                            _AT_ 0xC7BC;
__no_init EXTERN xdata volatile uint8_t in4bc                            _AT_ 0xC7BD;
__no_init EXTERN xdata volatile uint8_t in5cs                            _AT_ 0xC7BE;
__no_init EXTERN xdata volatile uint8_t in5bc                            _AT_ 0xC7BF;
__no_init EXTERN xdata volatile uint8_t out0bc                           _AT_ 0xC7C5;
__no_init EXTERN xdata volatile uint8_t out1cs                           _AT_ 0xC7C6;
__no_init EXTERN xdata volatile uint8_t out1bc                           _AT_ 0xC7C7;
__no_init EXTERN xdata volatile uint8_t out2cs                           _AT_ 0xC7C8;
__no_init EXTERN xdata volatile uint8_t out2bc                           _AT_ 0xC7C9;
__no_init EXTERN xdata volatile uint8_t out3cs                           _AT_ 0xC7CA;
__no_init EXTERN xdata volatile uint8_t out3bc                           _AT_ 0xC7CB;
__no_init EXTERN xdata volatile uint8_t out4cs                           _AT_ 0xC7CC;
__no_init EXTERN xdata volatile uint8_t out4bc                           _AT_ 0xC7CD;
__no_init EXTERN xdata volatile uint8_t out5cs                           _AT_ 0xC7CE;
__no_init EXTERN xdata volatile uint8_t out5bc                           _AT_ 0xC7CF;
__no_init EXTERN xdata volatile uint8_t usbcs                            _AT_ 0xC7D6;
__no_init EXTERN xdata volatile uint8_t togctl                           _AT_ 0xC7D7;
__no_init EXTERN xdata volatile uint8_t usbfrml                          _AT_ 0xC7D8;
__no_init EXTERN xdata volatile uint8_t usbfrmh                          _AT_ 0xC7D9;
__no_init EXTERN xdata volatile uint8_t fnaddr                           _AT_ 0xC7DB;
__no_init EXTERN xdata volatile uint8_t usbpair                          _AT_ 0xC7DD;
__no_init EXTERN xdata volatile uint8_t inbulkval                        _AT_ 0xC7DE;
__no_init EXTERN xdata volatile uint8_t outbulkval                       _AT_ 0xC7DF;
__no_init EXTERN xdata volatile uint8_t inisoval                         _AT_ 0xC7E0;
__no_init EXTERN xdata volatile uint8_t outisoval                        _AT_ 0xC7E1;
__no_init EXTERN xdata volatile uint8_t isostaddr                        _AT_ 0xC7E2;
__no_init EXTERN xdata volatile uint8_t isosize                          _AT_ 0xC7E3;
__no_init EXTERN xdata volatile uint8_t setupbuf[8]                      _AT_ 0xC7E8;
__no_init EXTERN xdata volatile uint8_t out8addr                         _AT_ 0xC7F0;
__no_init EXTERN xdata volatile uint8_t in8addr                          _AT_ 0xC7F8;

#endif
