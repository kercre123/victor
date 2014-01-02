///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     usbCoreApiDefines
///              
/// This is the USB API definitions

#ifndef _USB_CORE_API_DEFINES_H_
#define _USB_CORE_API_DEFINES_H_

// 1: Defines
// ----------------------------------------------------------------------------

/// @brief   error codes
#define USB_CORE_OK            0x00
#define USB_CORE_INIT          0x01
#define USB_CORE_BUSY          0x02
#define USB_CORE_INVALID       0xFF


// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// handle set, get of streaming
/// @brief   parameter data structure
typedef struct 
{
	unsigned int address;
	unsigned int nbytes;
} usb_stream_param_t;

// generick enumeration of driver exposed streaming 
typedef enum
{
	USB_CORE_STREAM0 = 0,
	USB_CORE_STREAM1 = 1,
	USB_CORE_STREAM2 = 2,
	USB_CORE_STREAM3 = 3,
} usb_stream_idx_e;

// generick enumeration of sink selection
typedef enum
{
	USB_CORE_PRIMARY_SINK = 0,
	USB_CORE_SECONDARY_SINK = 1,
} usb_stream_sink_e;



// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------




#endif /* _USB_CORE_API_DEFINES_H_ */
