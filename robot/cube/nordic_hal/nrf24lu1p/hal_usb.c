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
 * $LastChangedRevision: 2378 $
 */

/** @file
* @brief Implementaion of the USB HAL
 */
#include "nrf24lu1p.h"
#include <stdint.h>
#ifdef __ICC8051__
#include <intrinsics.h>
#endif
#ifdef __C51__
#include <intrins.h>
#endif

#include "nordic_common.h"
#include "hal_usb_desc.h"
#include "usb.h"
#include "hal_delay.h"


#define ALLOCATE_USB_MAP
#include "usb_map.h"

// Define formulas for jumping in the usb registry map based upon the endpoint number

// Calculate control and status register location in USB-controller
#define CALCULATE_CS_IN_PTR(ep) (uint8_t xdata*)(&in1cs + 2 * ((ep & 0x7f) - 1 ))
#define CALCULATE_CS_OUT_PTR(ep) (uint8_t xdata*)(&out1cs + 2 * ( (ep & 0x7f) - 1 ))

// Calculate byte count register location in USB-controller
#define CALCULATE_BC_OUT_PTR(ep) (uint8_t xdata *)(&out0bc + (ep * 2 ))
#define CALCULATE_BC_IN_PTR(ep) (uint8_t xdata *)(&in0bc + ((ep & 0x7f ) * 2))

// Calculate buffer location in USB-controller
#define CALCULATE_BUF_IN_PTR(ep) (uint8_t xdata *)(in0buf - (( ep & 0x7f) * 128))
#define CALCULATE_BUF_OUT_PTR(ep) (uint8_t xdata *)(out0buf - (ep * 128 ))

static packetizer_t i_packetizer;
static hal_usb_cb_endpoint_t i_endpoint_in_isr[USB_ENDPOINT_IN_COUNT];
static hal_usb_cb_endpoint_t i_endpoint_out_isr[USB_ENDPOINT_OUT_COUNT];

static hal_usb_device_req req;
hal_usb_t volatile g_hal_usb;
static uint8_t stall_data_size0;

void hal_usb_init(bool usb_disconnect, hal_usb_cb_device_req_t device_req, hal_usb_cb_reset_t reset, hal_usb_cb_resume_t resume, hal_usb_cb_suspend_t suspend)
{
  // Setup descriptors
  g_hal_usb.descs.dev = &g_usb_dev_desc;
  g_hal_usb.descs.conf = &g_usb_conf_desc;
  g_hal_usb.descs.string = &g_usb_string_desc;

  // This is for setting language American English (String descriptor 0 is an array of supported languages)
  g_hal_usb.descs.string_zero[0] = 0x04;
  g_hal_usb.descs.string_zero[1] = 0x03;
  g_hal_usb.descs.string_zero[2] = 0x09;
  g_hal_usb.descs.string_zero[3] = 0x04;

  // Setup state information
  g_hal_usb.state = DEFAULT;
  g_hal_usb.bm_state = 0;
  stall_data_size0 = 0;

  // Setconfig configuration information
  g_hal_usb.current_config = 0;
  g_hal_usb.current_alt_interface = 0;

  // Setup callbacks
  g_hal_usb.device_req = device_req;
  g_hal_usb.reset = reset;
  g_hal_usb.resume = resume;
  g_hal_usb.suspend = suspend;

  // Disconnect from USB-bus if we are in this routine from a power on and not a soft reset
  if(usb_disconnect)
  {
    usbcs |= 0x08; // disconnect
    delay_ms(50);
    usbcs &= ~(0x08); // connect
  }

  // Setup interrupts
  USBWU = 1; // USBWU is mapped to IEN1.3
  USB = 1; // USBIRQ is mapped to IEN1.4

  usbien = 0x1d; // ibnie -5 4 - uresir 3 - suspir, 0 - sudavir

  in_ien = 0x01;
  in_irq = 0x1f;
  out_ien = 0x01;
  out_irq = 0x1f;

  // Setup the USB RAM with some OK default values. Note that isochronos is not set up yet.
  bout1addr = 16;
  bout2addr = 32;
  bout3addr = 48;
  bout4addr = 64;
  bout5addr = 80;

  binstaddr = 0xc0;
  bin1addr = 16;
  bin2addr = 32;
  bin3addr = 48;
  bin4addr = 64;
  bin5addr = 80;

  // Set all endpoints to not valid (except EP0IN and EP0OUT)
  inbulkval = 0x01;
  outbulkval = 0x01;
  inisoval = 0x00;
  outisoval = 0x00;
}

void hal_usb_endpoint_stall(uint8_t ep_num, bool stall)
{
  uint8_t temp;
  uint8_t xdata *cs_ptr;

  temp = 2 * ((ep_num & 0x7f) - 1);

  // Calculate register address
  if((ep_num & 0x80 ) == 0x80) // IN endpoints
  {
    // Calculate control and status register for IN endpoint
    cs_ptr = (uint8_t xdata*)(&in1cs + temp);
  }
  else // OUT endpoints
  {
    // Calculate control and status register for OUT endpoint
    cs_ptr = (uint8_t xdata*)(&out1cs + temp);
  }

  if(stall == true)
  {
    // Set the stall bit
    *cs_ptr = 0x01;
  }
  else
  {
    // Clear the stall bit
    *cs_ptr = 0x00;
  }
}

uint8_t hal_usb_get_address()
{
  return fnaddr;
}

void hal_usb_endpoint_config(uint8_t ep_num, uint8_t ep_size, hal_usb_cb_endpoint_t endpoint_isr)
{
  uint8_t xdata *bc_ptr;
  uint8_t temp = (ep_num & 0x7f) - 1;
  uint8_t stemp = 1 << (ep_num & 0x7f);

  // Dummy use of variable to get rid of warning
  ep_size = 0;

  if((ep_num & 0x80 ) == 0x80) // MSB set indicates IN endpoint
  {
    i_endpoint_in_isr[temp] = endpoint_isr;
    if(endpoint_isr != NULL)
    {
      // Add the callback, enable the interrupt and validate the endpoint
      in_ien |= stemp; 
      inbulkval |= stemp;
    }
    else
    {
      // Remove the callback, disable the interrupt and invalidate the endpoint
      in_ien &= ~stemp;
      inbulkval &= ~stemp;
    }
  }
  else // OUT endpoint
  {
    i_endpoint_out_isr[temp] = endpoint_isr;
    if(endpoint_isr != NULL)
    {
      // Add the callback, enable the interrupt and validate the endpoint
      out_ien |= stemp;
      outbulkval |= stemp;

      // Have to write a dummy value to the OUTxBC register to get interrupts
      bc_ptr = CALCULATE_BC_OUT_PTR(ep_num);
      *bc_ptr = 0xff;
    }
    else
    {
      // Remove the callback, disable the interrupt and invalidate the endpoint
      out_ien &= ~stemp;
      outbulkval &= ~stemp;
    }
  }
}

void hal_usb_wakeup()
{
  // We can only issue a wakeup if the host has allowed us to do so
  if((g_hal_usb.bm_state & USB_BM_STATE_ALLOW_REMOTE_WAKEUP) == USB_BM_STATE_ALLOW_REMOTE_WAKEUP)
  {
    USBCON = 0x40;  // Wakeup the USB controller via remote pin
    delay_ms(1);    // Wait until the USB clock starts
    USBCON = 0x00;
  }
}

void hal_usb_reset()
{
  SWRST = 1;  // Perform a hardware reset of the USB controller
}

#pragma disable
hal_usb_state_t hal_usb_get_state()
{
  return g_hal_usb.state;
}

void hal_usb_send_data(uint8_t ep_num, uint8_t* array, uint8_t count)
{
  uint8_t i;

  uint8_t xdata *buf_ptr;
  uint8_t xdata *bc_ptr;

  // Calculate the buffer pointer and byte count pointer
  buf_ptr = CALCULATE_BUF_IN_PTR(ep_num);
  bc_ptr = CALCULATE_BC_IN_PTR(ep_num);

  // Copy the data into the USB controller
  for( i = 0; i < count; i++ )
  {
    buf_ptr[i] = array[i];
  }

  // Set the number of bytes we want to send to USB-host. This also trigger sending of data to USB-host.
  *bc_ptr = count;
}

void hal_usb_bus_disconnect()
{
  usbcs |= 0x08; // disconnect
}

void hal_usb_bus_connect()
{
  usbcs &= ~(0x08); // connect
}

void hal_usb_sleep()
{
  USBSLP = 1;
}
 
static void packetize(uint8_t *data_ptr, uint8_t data_size)
{
  i_packetizer.data_ptr = data_ptr;
  i_packetizer.data_size = data_size;
  i_packetizer.pkt_size = g_hal_usb.descs.dev->bMaxPacketSize0;
}

// This routine is called by functions that shall send their first packet and when the EP0IN interrupt is set
static void packetizer_isr_ep0_in(void) 
{
  uint8_t size, i;

  // We are getting a ep0in interupt when the host send ACK and do not have any more data to send
  if(i_packetizer.data_size == 0)
  {        
    if (stall_data_size0 == 1)
    {
        USB_EP0_DSTALL();
    }
    else
    {
        stall_data_size0 = 1;
        in0bc = 0;
        USB_EP0_HSNAK();
    }       
    return;
  }

  size = MIN(i_packetizer.data_size, i_packetizer.pkt_size);

  // Copy data to the USB-controller buffer
  for(i = 0; i < size; i++)
  {
    in0buf[i] = i_packetizer.data_ptr[i];
  }

  if (size < i_packetizer.pkt_size)
    stall_data_size0 = 1;
  // Tell the USB-controller how many bytes to send
  // If a IN is received from host after this the USB-controller will send the data
  in0bc = size;

  // Update the packetizer data
  i_packetizer.data_ptr += size;
  i_packetizer.data_size -= size;

  return;
}

/** This function processes the response from the callback */
static void usb_process_dev_req_cb_response(void)
{
  uint8_t *data_ptr;
  uint8_t data_size;
  hal_usb_dev_req_resp_t ret = g_hal_usb.device_req(&req, &data_ptr, &data_size);

  switch(ret)
  {
    case DATA:
      packetize((uint8_t *)data_ptr, MIN(req.wLength, data_size));
      packetizer_isr_ep0_in();
      break;
    case NO_RESPONSE:
      break;
    case EMPTY_RESPONSE:
    case NAK:
      USB_EP0_HSNAK();
      break;
    case ACK:
      out0bc = 0xff;
      break;
    case STALL:
    default:
      USB_EP0_STALL();
      break;
    }
}

static void usb_process_get_status(void)
{
  uint8_t xdata *ptr;

  if(g_hal_usb.state == ADDRESSED)
  {
    if(req.wIndex != 0x00)
    {
        USB_EP0_STALL();
    }
    else
    {
      in0buf[0] = in0buf[1] = 
        ((g_hal_usb.descs.conf->conf.bmAttributes & 0x40 ) >> 6); // D0 - 0: bus powered, 1: self powered
      in0bc = 0x02;
    }
  }
  else if(g_hal_usb.state == CONFIGURED)
  {
    in0buf[1] = 0x00;
    switch(req.bmRequestType)
    {
      case 0x80: // Device
        if((g_hal_usb.bm_state & USB_BM_STATE_ALLOW_REMOTE_WAKEUP ) == USB_BM_STATE_ALLOW_REMOTE_WAKEUP)
        {
            in0buf[0] = 0x02;
        }
        else
        {
            in0buf[0] = 0x00;
        }

        in0buf[0] |= ((g_hal_usb.descs.conf->conf.bmAttributes & 0x40 ) >> 6); // D0 - 0: bus powered, 1: self powered
        in0bc = 0x02;
        break;
      case 0x81: // Interface
        in0buf[0] = 0x00;
        in0bc = 0x02;
        break;
      case 0x82: // Endpoint
        if((req.wIndex & 0x80) == 0x80) // IN endpoints
        {
          ptr = CALCULATE_CS_IN_PTR(req.wIndex);
        }
        else
        {
          ptr = CALCULATE_CS_OUT_PTR(req.wIndex);
        }

        in0buf[0] = *ptr & 0x01;
        in0bc = 0x02;
        break;
      default:
        USB_EP0_STALL();
        break;
    } // switch(req.bmRequestType) --end--
  }
  else
  {
    // We should not be in this state
    USB_EP0_STALL();
  }
}

static void usb_process_get_descriptor(void)
{
  // Switch on descriptor type
  switch(req.wValueMsb)
  {
    case USB_DESC_DEVICE:
      packetize((uint8_t *)g_hal_usb.descs.dev,
      MIN(req.wLength, sizeof(hal_usb_dev_desc_t)));
      packetizer_isr_ep0_in();
      break;
    case USB_DESC_CONFIGURATION:
      // For now we just support one configuration. The asked configuration is stored in LSB(wValue).
      packetize((uint8_t *)g_hal_usb.descs.conf,
      MIN(req.wLength, sizeof(usb_conf_desc_templ_t)));
      packetizer_isr_ep0_in();
      break;
    case USB_DESC_STRING:
      // For now we just support english as string descriptor language.
      if(req.wValueLsb == 0x00)
      {
        packetize(g_hal_usb.descs.string_zero, MIN(req.wLength, sizeof(g_hal_usb.descs.string_zero)));
        packetizer_isr_ep0_in();
      }
      else
      {
        if((req.wValueLsb - 1 ) < USB_STRING_DESC_COUNT)
        {
          packetize((uint8_t *)(g_hal_usb.descs.string->idx[req.wValueLsb-1]),
          MIN(req.wLength, g_hal_usb.descs.string->idx[req.wValueLsb-1][0]));
          packetizer_isr_ep0_in();
        }
        else
        {
          USB_EP0_STALL();
        }
      }
      break;
    case USB_DESC_INTERFACE:
    case USB_DESC_ENDPOINT:
    case USB_DESC_DEVICE_QUAL:
    case USB_DESC_OTHER_SPEED_CONF:
    case USB_DESC_INTERFACE_POWER:
      USB_EP0_STALL();
      break;
    default:
      usb_process_dev_req_cb_response();
      break;
  }
}

static void isr_sudav(void)
{
  // Parsing the request into request structure
  req.bmRequestType = setupbuf[0];
  req.bRequest = setupbuf[1];
  req.wValueLsb = setupbuf[2];
  req.wValueMsb = setupbuf[3];
  req.wIndex = setupbuf[4];
  req.wLength = setupbuf[6];
  if (setupbuf[7] > 0)
  {
    req.wLength = 0xff; // We truncate packets requests longer then 255 bytes
  }

  // bmRequestType = 0 00 xxxxx : Data transfer direction: Host-to-device Type: Standard
  if((req.bmRequestType & 0x60) == 0x00)
  {
    switch(req.bRequest)
    {
      case USB_REQ_GET_DESCRIPTOR:
        usb_process_get_descriptor();
        break;
      case USB_REQ_GET_STATUS:
        usb_process_get_status();
        break;           
      case USB_REQ_CLEAR_FEATURE:
      case USB_REQ_SET_FEATURE: 
        switch(req.bmRequestType)
        {
          case 0x00: // Device
            if(req.wValueLsb == USB_DEVICE_REMOTE_WAKEUP)
            {
              if (req.bRequest == USB_REQ_CLEAR_FEATURE) 
                g_hal_usb.bm_state &= ~(USB_BM_STATE_ALLOW_REMOTE_WAKEUP);
              else
                g_hal_usb.bm_state |= USB_BM_STATE_ALLOW_REMOTE_WAKEUP;
              USB_EP0_HSNAK();
            }
            else
            {
              USB_EP0_STALL();
            }
            break;

          case 0x02: // Endpoint
            if(req.wValueLsb == USB_ENDPOINT_HALT)
            {
              if (req.bRequest == USB_REQ_CLEAR_FEATURE) 
                hal_usb_endpoint_stall(req.wIndex, false);
              else
                hal_usb_endpoint_stall(req.wIndex, true);
              USB_EP0_HSNAK();
            }
            else 
            {
                USB_EP0_STALL();
            }
            break;
          case 0x01: // Interface
          default:
            USB_EP0_STALL();
            break;
        }
        break;

      case USB_REQ_SET_ADDRESS:
        g_hal_usb.state = ADDRESSED;
        g_hal_usb.current_config = 0x00;
        break;
      case USB_REQ_GET_CONFIGURATION:
        switch(g_hal_usb.state)
        {
          case ADDRESSED:
            in0buf[0] = 0x00;
            in0bc = 0x01;
            break;
          case CONFIGURED:
            in0buf[0] = g_hal_usb.current_config;
            in0bc = 0x01;
            break;
          default:
            USB_EP0_STALL();
            break;
        }
        break;
      case USB_REQ_SET_CONFIGURATION:
        switch(req.wValueLsb)
        {
          case 0x00:
            g_hal_usb.state = ADDRESSED;
            g_hal_usb.current_config = 0x00;
            USB_EP0_HSNAK();
            break;
          case 0x01:
            g_hal_usb.state = CONFIGURED;
            g_hal_usb.bm_state |= USB_BM_STATE_CONFIGURED;
            g_hal_usb.current_config = 0x01;
            USB_EP0_HSNAK();
            break;
          default:
            USB_EP0_STALL();
            break;
        }
        break;
      case USB_REQ_GET_INTERFACE: // GET_INTERFACE
        in0buf[0] = g_hal_usb.current_alt_interface;
        in0bc = 0x01;
        break;
      case USB_REQ_SET_DESCRIPTOR:
      case USB_REQ_SET_INTERFACE: // SET_INTERFACE (We do not support this)
      case USB_REQ_SYNCH_FRAME:   // SYNCH_FRAME (We do not support this)
      default:
       USB_EP0_STALL();
       break;
    };
  } 
  // bmRequestType = 0 01 xxxxx : Data transfer direction: Host-to-device, Type: Class
  else if((req.bmRequestType & 0x60 ) == 0x20)  // Class request
  {
    if(req.wLength != 0 && ((req.bmRequestType & 0x80) == 0x00))
    {
      // If there is a OUT-transaction associated with the Control-Transfer-Write we call the callback
      // when the OUT-transaction is finished. Note that this function do not handle several out transactions.
      out0bc = 0xff;
    }
    else
    {
      usb_process_dev_req_cb_response();
    }
    // Call the callback function. Data to be sent back to the host is store by the callback in data_ptr and the size in data_size.
  } 
  else  // Unknown request type
  {
    USB_EP0_STALL();
  }
}

static void isr_suspend(void)
{
  uint8_t allow_remote_wu = 0;
  g_hal_usb.bm_state &= ~(USB_BM_STATE_HOST_WU); // We clear the flag that indicates that the host awoke the MCU via USB here

  if( g_hal_usb.state == CONFIGURED )
  {
    if( ( g_hal_usb.bm_state & USB_BM_STATE_ALLOW_REMOTE_WAKEUP ) == USB_BM_STATE_ALLOW_REMOTE_WAKEUP )
    {
      allow_remote_wu = 1;
    }
  }

  g_hal_usb.state = SUSPENDED;

  if( g_hal_usb.suspend != NULL )
  {
    g_hal_usb.suspend(allow_remote_wu);
  }
}

static void isr_usbreset(void)
{
  g_hal_usb.state = DEFAULT;
  g_hal_usb.current_config = 0;
  g_hal_usb.current_alt_interface = 0;
  g_hal_usb.bm_state = 0;
  if( g_hal_usb.reset != NULL ) g_hal_usb.reset();
}


USB_WU_ISR() // address: 0x005b
{
#define ICH4
  #ifdef ICH4
  uint8_t t;
  #endif

  // Check if the wakeup source is the pin to the USB controller
  // If it is by the pin to the USB controller we want to start
  // a remote wakeup
  if( ( usbcs & 0x80 ) == 0x80 )
  {
    // Reset the wakesrc indicator
    usbcs = 0x80;

    // If we are allowed to perform a remote wakeup do that
    if( ( g_hal_usb.bm_state & USB_BM_STATE_ALLOW_REMOTE_WAKEUP ) == USB_BM_STATE_ALLOW_REMOTE_WAKEUP )
    {
  #ifdef ICH4
      // Force the J state on the USB lines
      usbcs |= 0x02;

      // Typical 5.4us delay
      _nop_();
      _nop_();

      t = usbcs;

      // Stop J state on the USB lines
      t &= ~0x02;

      // Signal remote resume
      t |= 0x01;

      // We have to set this register in one operation to avoid
      // idle state is restored between the forced J and resume state
      usbcs = t;
#else
      usbcs |= 0x01;  // Turn on the resume signal on the USB bus
#endif
      delay_ms(7); //.1.7.7 Resume: The remote wakeup device must hold the resume signaling for at 
                    // least 1 ms but for no more than 15ms

      usbcs &= ~0x01; // Turn off the resume signal on the USB bus
    }
  }
  else 
  {
    // We are awoken by the bus
    g_hal_usb.bm_state |= USB_BM_STATE_HOST_WU;
  }

  if((g_hal_usb.bm_state & USB_BM_STATE_CONFIGURED ) == USB_BM_STATE_CONFIGURED)
  {
    g_hal_usb.state = CONFIGURED;
  }
  else
  {
    g_hal_usb.state = DEFAULT;
  }

  // Call resume callback
  g_hal_usb.resume();
}

// This function processes the response from the EP callback
static void usb_process_ep_response(uint8_t ret, uint8_t xdata* cs_ptr, uint8_t xdata* bc_ptr)
{
  if( ret == 0xff ) // Clear the OUTx busy flag enabling reception of the next OUT from USB-host
  {
    *bc_ptr = 0xff;
  }
  else if( ( ret & 0x80 ) == 0x80 )  // STALL
  {
    *cs_ptr = 0x01;
  }
  else if( ( ret & 0x60 ) == 0x60 ) // NAK
  {
    *cs_ptr = 0x02;
  }
  else if( ret == 0 ) // Zero length data
  {
    *bc_ptr = 0;
  }
  else
  {
    *bc_ptr = ret;
  }
}

USB_ISR()
{
  uint8_t ep;
  uint8_t ret;
  uint8_t xdata *cs_ptr;
  uint8_t xdata *buf_ptr;
  uint8_t xdata *bc_ptr;

  switch(ivec)
  {
    case INT_SUDAV:
      usbirq = 0x01;
      isr_sudav();
      break;
    case INT_SOF:
      usbirq = 0x02;
      break;
    case INT_SUTOK:
      usbirq = 0x04;
      i_packetizer.data_ptr = NULL;
      i_packetizer.data_size = 0;
      i_packetizer.pkt_size = 0;
      stall_data_size0 = 0;
      break;
    case INT_SUSPEND:
      usbirq = 0x08;
      isr_suspend();
      break;
    case INT_USBRESET:
      usbirq = 0x10;
      isr_usbreset();
      break;
    case INT_EP0IN:
      in_irq = 0x01;
      packetizer_isr_ep0_in();
      break;
    case INT_EP0OUT:
      out_irq = 0x01;
      i_packetizer.data_size = 0;
      usb_process_dev_req_cb_response();
      break;
      case INT_EP1IN:
      case INT_EP2IN:
      case INT_EP3IN:
      case INT_EP4IN:
      case INT_EP5IN:
        // Calculate IN endpoint number
        ep = (ivec - INT_EP0IN ) >> 3;// INT_EP2IN - INT_EP1IN == 8 ;   
        // Clear interrupt 
        in_irq = ( 1 << ep );

        cs_ptr = CALCULATE_CS_IN_PTR(ep);
        buf_ptr = CALCULATE_BUF_IN_PTR(ep);
        bc_ptr = CALCULATE_BC_IN_PTR(ep);
    
        // Call registered callback
        ret = i_endpoint_in_isr[ep - 1](buf_ptr, bc_ptr);
        usb_process_ep_response(ret, cs_ptr, bc_ptr);
        break;
      case INT_EP1OUT:
      case INT_EP2OUT:
      case INT_EP3OUT:
      case INT_EP4OUT:
      case INT_EP5OUT:
        // Calculate OUT endpoint number
        ep = (ivec - INT_EP0OUT) >> 3;          // INT_EP2OUT - INT_EP1OUT == 8

        // Clear interrupt
        out_irq = ( 1 << ep );
        
        cs_ptr = CALCULATE_CS_OUT_PTR(ep);
        buf_ptr = CALCULATE_BUF_OUT_PTR(ep);
        bc_ptr = CALCULATE_BC_OUT_PTR(ep);

        // Call registered callback
        ret = (i_endpoint_out_isr[ep - 1])(buf_ptr, bc_ptr);
        usb_process_ep_response(ret, cs_ptr, bc_ptr);
        break;
    default:
      break;
  };
}
