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
 * @brief Interface for the USB device controller.
 *
 * The header file must define the following type-defined structs:
 *  - hal_usb_conf_desc_templ_t
 *  - hal_usb_string_desc_templ_t
 *  - hal_usb_dev_desc_templ_t
 *  .
 * And the following global variables:
 *  - g_usb_string_desc
 *  - g_usb_conf_desc
 *  - g_usb_dev_desc
 *
 * @defgroup hal_nrf24lu1p_hal_usb Universal Serial Bus (hal_usb)
 * @{
 * @ingroup hal_nrf24lu1p
 *
 * <h1>Control Transfer Functionality</h1>
 *
 * <h2>Descriptor Requests</h2>
 * The module handles the following <b>descriptor requests</b>:
 * - Device
 * - Configuration
 * - String
 *
 * It does <b>not</b> support (replies with STALL to USB host controller):
 * - Interface
 * - DeviceQual
 * - OtherSpeedConf
 * - InterfacePower
 * 
 * All other descriptor requests (HID, ...) are made available to the application code through the hal_usb_cb_device_req_t callback.
 *
 * The module does <b>not</b> support SetDescriptor requests
 *
 * <h2>Feature Requests</h2>
 *
 * The module handles the following feature requests:
 * -  DeviceRemoteWakeup
 * -  EndpointHalt
 * 
 * It does <b>not</b> support (replies with STALL to USB host controller) any other feature requests yet
 *
 * <h2>Configuration Requests</h2>
 *
 * The module do only support setting configuration 0 (sets the adressed state) and 1. Other configurations replies with STALL. Several configurations for a device is not implemented.
 * 
 * <h2>Alternative Interface Requests</h2>
 *
 * Setting and getting alternative interfaces are not supported (replies with STALL to USB host controller)
 * 
 * <h2>Sync Frame Requests</h2>
 * Not supported
 * 
 * <h2>Class Specific Requests</h2>
 * All class specific requests are made available to the application code through the hal_usb_cb_device_req_t callback. 
 *
 * <h1>Endpoint Functionality</h1>
 * All available IN and OUT endpoints are avialable to the application through the hal_usb_send_packet, hal_usb_receive_packet functions and the callbacks registered for the endpoint. The callback functions are called when the host replies with an ACK for having received a packet or when the host has sent an OUT packet. 
 * 
 * Isochronos endpoints are not supported in this version of USB HAL.
 */

#ifndef HAL_USB_H__
#define HAL_USB_H__

#include <stdint.h>
#include <stdbool.h>

#include "hal_usb_desc.h"
#include "config.h" 

#ifndef USB_DESC_TEMPLATE
#error "USB_DESC_TEMPLATE not defined. Please include a file with g_usb_string_desc, g_usb_conf_desc and g_usb_dev_desc defined" 
#endif

#define USB_BM_STATE_CONFIGURED 0x01
#define USB_BM_STATE_ALLOW_REMOTE_WAKEUP 0x02
#define USB_BM_STATE_HOST_WU     0x04

/** An enum describing the USB state
 * 
 *  The states described in this enum are found in Chapter 9 of the USB 2.0 specification
 */

typedef enum  { 
    ATTACHED,   /**< Device is attached to the USB, but is not powered */
    POWERED,    /**< Device is attached to the USB and powered */
    DEFAULT,    /**< Device is attached to the USB and powered and has been reset, but has not been assigned a unique address */
    ADDRESSED,  /**< Device is attached to the USB, powered, has been reset, and a unique device address has been assigned. Device is not configured */
    CONFIGURED, /**< Device is attached to the USB, powered, has been reset, has a unique address, is configured and is not suspended */
    SUSPENDED   /**< Device is, at a minimum, attached to the USB and is powered and has not seen bus activity for 3ms. It may also have a unique address and be configured for use. However, because the device is suspended, the host may not use the device configuration */
} hal_usb_state_t;

/** Structure containing the USB standard request
 *  See Chapter 9 USB Device Framework in the USB 2.0 specification.
 */

typedef struct {
    uint8_t  bmRequestType; /**< Bitmapped field identifying the characteristics of the request.
                        - D7: Data transfer direction 
                        - 0 = Host-to-device
                        - 1 = Device-to-host

                        - D6..5: Type
                        - 0 = Standard
                        - 1 = Class
                        - 2 = Vendor
                        - 3 = Reserved

                        - D4..0: Recipient
                        - 0 = Device
                        - 1 = Interface
                        - 2 = Endpoint
                        - 3 = Other
                        - 4..31 = Reserved
                          */
    uint8_t  bRequest;       /**< Field specifying request. bmRequestType(Type) modifies the meaning of this field. */

    uint8_t wValueMsb;      /**< Field used to pass a parameter to the device, specific to the request. MSB*/
    uint8_t wValueLsb;      /**< Field used to pass a parameter to the device, specific to the request. LSB*/
    uint8_t wIndex;         /**< Field used to pass a parameter to the device, specific to the request. */
    uint8_t wLength;        /**< Field used to specify length of the data transferred during the second phase of the control transfer. Direction of data transfer given by bmRequestType(Direction). If field is zero there is no data transfer phase. */
//    void* misc_data;
} hal_usb_device_req;

/** An enum describing which reply to send to the control request
 */

typedef enum {
    STALL,         /**< Respond with STALL */
    NAK,           /**< Respond with NAK   */
    ACK,           /**< Respond with ACK (if this is an OUT request) */
    NO_RESPONSE,   /**< Do not respond */
    DATA,          /**< Data is available */
    EMPTY_RESPONSE /**< Send an empty response */
} hal_usb_dev_req_resp_t;

/** Callback function that is called when a class request is received.
 *  The type of class request is determined by the interface the request is for. If interface 1 is a HID interface the request is a HID class request.
 *  @param std_req The complete request. 
 *  @param data_ptr Pointer to pointer to data(descriptor struct) the function wants to send back to USB-host
 *  @param size Size of data the function wants to send back to USB-host
 */

typedef hal_usb_dev_req_resp_t (*hal_usb_cb_device_req_t)(hal_usb_device_req* device_req, uint8_t ** data_ptr, uint8_t* size) large reentrant;

/** Callback function that is called when an endpoint interrupt occurs 
 *  @param adr_ptr IN endpoint: Pointer to address containing data to send. OUT endpoint: Pointer to address containg data received.
 *  @param size IN endpoint: Number of bytes to send. OUT endpoint: number of bytes received.
 *  @retval Bit 7 Set: STALL request
 *  @retval Bit 6 Set: NACK request
 *  @retval Bit 5..0 : Bytes to send
 */
typedef uint8_t (*hal_usb_cb_endpoint_t)(uint8_t* adr_ptr, uint8_t* size) large reentrant;

/** Callback function that is called when a resume is signalled on the bus.
 *  A resume is signalled by the Data K state. For full-speed devices this is Differential "0". Differential "0": D-> Voh(main) and D+ < Vol(max).
 */

typedef void (*hal_usb_cb_resume_t)() large reentrant;

/** Callback function that is called when a suspend condition occurs.
 *  @param can_resume Set to 1 if the device is allowed to wake up the host controller
 *  @see hal_usb_state_t
 */

typedef void (*hal_usb_cb_suspend_t)(uint8_t allow_remote_wu) large reentrant;

/** Callback function that is called when a reset condition occurs.
 *  
 */
typedef void (*hal_usb_cb_reset_t)() large reentrant;

/** A struct containing variables related to the USB HAL layer 
 *  
 */

typedef struct {
    usb_descs_templ_t descs;     /**< Structure containing device, string and configuration descriptors for a specific application */
    uint8_t  bm_state;             /**< Bitmask containing USB state information: bitmask: 0 - is_hw_reset, 1 - can signal remote wakeup, 2 - usb awake */
    uint8_t current_config;        /**< Currently set configuration. If current_config is zero the device is not configured */
    uint8_t current_alt_interface; /**< Currently alternative configuration. If an alternative configuration is chosen the index of the alternative configuration is stored here. */
    hal_usb_state_t state;       /**< Enum containing USB state information as described in Chapter 9 of the USB 2.0 specification.  */

    hal_usb_cb_device_req_t device_req;
    hal_usb_cb_reset_t      reset;
    hal_usb_cb_resume_t     resume;
    hal_usb_cb_suspend_t    suspend;
} hal_usb_t;

/** Function for setting up the USB controller and registering the callbacks 
 *  @param usb_disconnect Set to true to perform physical disconnect and reconnect. Make sure to enable interrupts as soon as possible after
 *                        the call to this function.
 *  @param device_req Pointer to function to call when a class specific request occur
 *  @param reset Pointer to function to call when USB controller detects a reset
 *  @param resume Pointer to function to call when USB controller detects a resume
 *  @param suspend Pointer to function to call when USB controller detects a suspend
 */
void hal_usb_init(
    bool usb_disconnect,
    hal_usb_cb_device_req_t device_req,
    hal_usb_cb_reset_t      reset,
    hal_usb_cb_resume_t     resume,
    hal_usb_cb_suspend_t    suspend);

/** Function to send a packet to host (IN endpoint)
 *  @param ep_in_num IN endpointer number
 *  @param buffer Pointer to buffer containing data to send
 *  @param bytes_to_send Number of bytes to send.
 */
void hal_usb_send_data(uint8_t ep_in_num, uint8_t* buffer, uint8_t bytes_to_send);

/** Function to register callbacks for given endpoints 
 *  To register a callback one have to have a function with an argument list equal to usb_endpoint_cb_t.
 *  @param ep_num Endpoint number. If MSB is set it indicates this is an IN endpoint.
 *  @param ep_size The maximum size of one packet for this endpoint
 *  @param endpoint_isr Pointer to function that is called when host issues a request on the given endpoint. Set to 0 to unregister function.
 */
void hal_usb_endpoint_config(uint8_t ep_num, uint8_t ep_size, hal_usb_cb_endpoint_t endpoint_isr);

/** Function to stall or unstall an endpoint
 *  @param ep_num Endpoint number. If MSB is set it indicates this is an IN endpoint.
 *  @param stall 1 - stall the endpoint. 0 - unstall the endpoint.
 */
void hal_usb_endpoint_stall(uint8_t ep_num, bool stall);

/** Function returning the current state of the USB controller
 *  @see usb_stat_t
 */
hal_usb_state_t hal_usb_get_state();

/** Function returning the assigned address for the device */
uint8_t hal_usb_get_address();

/** Function to initiate a remote wakeup of the USB host */
void hal_usb_wakeup();

/** Function to initiate a <b>hardware reset</b> of the USB-controller */
void hal_usb_reset();

/** Function disconnecting the USB-controller from the USB bus  */
void hal_usb_bus_disconnect();

/** Function connecting the USB-controller to the USB bus */
void hal_usb_bus_connect();

/** Function stopping the clock to the usb controller or by other means powers it down */
void hal_usb_sleep();

extern hal_usb_t volatile g_hal_usb;

#endif //  HAL_USB_H__
/** @} */
