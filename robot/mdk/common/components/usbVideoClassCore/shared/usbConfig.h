///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Common UVC USB configurations header
///

#ifndef _USB_CONFIG_H_
#define _USB_CONFIG_H_

#define UVC_INTERRUPT_IDX              0x07

// defines for raw yuv frame rates
#define UVC_FRAMERATE_NR               0x03
#define UVC_FRAMERATE_MAX              25
#define UVC_FRAMERATE1                 25
#define UVC_FRAMERATE2                 20
#define UVC_FRAMERATE3                 15
#define UVC_FRAMERATE_MIN              15
// defines for video control device modeling
#define UVC_MODEL_NONE_ID              0x00
#define UVC_MODEL_CAM_IN_ID            0x01
#define UVC_MODEL_USB_OUT_ID           0x02

#define UVC_EPSTAT                     0x07

#define USB_SETUP03_IDX                0x38
#define USB_SETUP47_IDX                0x39

// PC commands defines
#define USB_TRANSFER_IIC        1
#define USB_TRANSFER_SPI        2
#define USB_WAIT_USEC           3


#endif
