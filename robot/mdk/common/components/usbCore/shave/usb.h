///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @addtogroup usbCore
/// @{
/// @brief     usbCoreApiDefines
///              
/// USB low level driver defines and data-structures
/// 

#ifndef _USB_H_
#define _USB_H_

// sneak user defines to be visible to system manager

;// miscellaneous defines
#define USB_SIGNAL_NR             60
#define USB_CODE_SECTION_SIZE     0x10000
#define USB_STACKTOP_OFFSET       0x04000
;// HW register address
#define USB_HWI_BASE_ADDR         0xD0300000
#define USB_AXI_BASE_ADDR         0xA0000000
#define USB_MXI_BASE_ADDR         0x10000000
#define USB_SLC_BASE_ADDR         0x1C000000


typedef struct
{
    unsigned int Endpoint;
    unsigned int Type;
    unsigned int BufferAddr;
    unsigned int BufferLength;
    unsigned int PriAddr;
    unsigned int PriLength;
    unsigned int SecAddr;
    unsigned int SecLength;
    unsigned int PriAddrShadow;
    unsigned int PriLengthShadow;
    unsigned int SecAddrShadow;
    unsigned int SecLengthShadow;
    unsigned int MuxPacket;
    unsigned int Packet;
    unsigned int (*MuxCallback)(unsigned int);
    unsigned int (*Callback)(unsigned int);
} usb_stream_t;

// USB API
typedef struct
{
    // read only for device status
    unsigned int   deviceStat;
    unsigned int   deviceClock;
    unsigned int   interfaceAlter03;
    unsigned int   interfaceAlter47;
    // read/write interrupt signals here
    unsigned int   interruptSignal[USB_SIGNAL_NR];
    // expose streaming interfaces
    usb_stream_t   stream[4];
} usb_ctrl_t;
/// API visible to system manager, initialize structure and signal data base
extern volatile usb_ctrl_t __attribute__((aligned(64))) usb_ctrl;
///Global init function
extern void USB_Main(void);
/// @}
#endif
