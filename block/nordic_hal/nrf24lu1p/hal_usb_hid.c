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
 * $LastChangedRevision: 2409 $
 */

/** @file
 * @brief Function for handling HID device requests
 */

#include "hal_usb_hid.h"
#include "nrf24lu1p.h"
#include "nordic_common.h"

static xdata uint8_t tmp_usb_buf[8];
static uint8_t tmp_usb_hid_protocol;

// More than 3 parameters passed to this function => need to make it reentrant
bool hal_usb_hid_device_req_proc(hal_usb_device_req* req, uint8_t** data_ptr, uint8_t* size, hal_usb_dev_req_resp_t* resp) large reentrant
{
    bool retval = false;    
    *resp = STALL;

    if(req->bRequest == USB_REQ_GET_DESCRIPTOR )
    {
        switch( req->wValueMsb )
        {
            case  USB_CLASS_DESCRIPTOR_REPORT:
                retval = true;
                *data_ptr = g_usb_hid_hids[LSB(req->wIndex)].hid_report_desc;
                *size = MIN(g_usb_hid_hids[LSB(req->wIndex)].hid_report_desc_size, LSB(req->wLength));
                *resp = DATA;
                break;
            case USB_CLASS_DESCRIPTOR_HID:
                retval = true;
                *data_ptr = (uint8_t*)g_usb_hid_hids[LSB(req->wIndex)].hid_desc;
                *size = MIN(sizeof(hal_usb_hid_desc_t), LSB(req->wLength));
                *resp = DATA;
                break;
            default:
                break;
        }
    } 
    else if( ( req->bmRequestType & 0x20 ) == 0x20 ) // This is a class specific request D5..6: Type Class(value 1)
    { 
        switch( req->bRequest )
        {
            case 0x01: // Get_Report
                retval = true;
                // For now we just send an "empty" report. No mousemoves, no mouse-key pressed.
                // TODO: This breaks the generic nature of usb.c. 
                // Have to create some global "default" reports in the template later.
                tmp_usb_buf[0] = tmp_usb_buf[1] = 0x00;
                *data_ptr = &tmp_usb_buf[0];
                *size = 0x03;
                *resp = DATA;
                break;
            case 0x02: // Get_Idle
                retval = true;
                *resp = STALL;
                break;
            case 0x0a: // Set_Idle
                retval = true;
                //idle_value = (req->wValueMsb);
                *resp = DATA;
                break;
            case 0x03: // Get_Protocol
                retval = true;
                tmp_usb_buf[0] = ( tmp_usb_hid_protocol & (1 << LSB(req->wIndex)) ) ? 0x01 : 0x00;
                *data_ptr = &tmp_usb_buf[0];
                *size = 0x01;
                *resp = DATA;
                break;
            case 0x0b: // Set_Protocol
                retval = true;
#if 1 // Right now we do not support setting of protocol in a intelligent way
                if( req->wValueLsb == 0x01 )
                {
                    tmp_usb_hid_protocol |= 1 << LSB(req->wIndex);
                }
                else
                {
                    tmp_usb_hid_protocol &= ~(1 << LSB(req->wIndex));
                }

                *resp = EMPTY_RESPONSE;
#else
                *resp = EMPTY_RESPONSE;
#endif
                break;
            case 0x09: // Set_Report
            {
                if( req->wValueMsb == 0x03 ) // Feature report
                {
                    retval = true;
                    *resp = EMPTY_RESPONSE;
                }
                else if ( req->wValueMsb == 0x02 ) // Output report
                {
                    // For now we just assume that the OUT packet is a keyboard report.
                    *resp = EMPTY_RESPONSE;
                    retval = true;
                }
            }
                break;
            default:
                break;
            }
    }

    return retval;
}
