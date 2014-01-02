///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup usbCore usbCore API
/// @{
/// @brief     usbCoreAPI
///              
/// This is the USB API

#ifndef _USB_CORE_API_H_
#define _USB_CORE_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "usbCoreApiDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------


// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
/// Initializes the allocated shave, resets and loads usb driver on it.
/// Starts the allocated shave, activating usb device.
/// @param   svuNr	allocated shave id
/// @return  error	code
///
extern unsigned int usbInit(unsigned int svuNr);

/// Deinitializes a previously allocated shave, by reseting and powering it down.
/// USB communication will no longer be possible.
/// @return  error code
///
extern unsigned int usbDeinit(void);

/// Check if a full USB buffer is available from a stream and associated sink.
/// The buffer parameters are written to the passed parameter data structure.
/// @param   idx	stream index
/// @param   sink	stream sink
/// @param   param	stream parameter
/// @return  error	code
///
extern unsigned int usbGetBuffer(usb_stream_idx_e idx, usb_stream_sink_e sink, usb_stream_param_t* param);

/// Tries to pass an empty  USB buffer to a stream and associated sink.
/// The buffer parameters are read from the passed parameter data structure.
/// @param   idx	stream index
/// @param   sink	stream sink
/// @param   param	stream parameter
/// @return  error	code
///
extern unsigned int usbSetBuffer(usb_stream_idx_e idx, usb_stream_sink_e sink, usb_stream_param_t* param);

/// Tries to pass an empty  USB buffer to a stream and associated sink, but only if previous transfer completed.
/// The buffer parameters are read from the passed parameter data structure.
/// @param   idx	stream index
/// @param   sink	stream sink
/// @param   param	stream parameter
/// @return  error	code
///
extern unsigned int usbSetBufferSequential(usb_stream_idx_e idx, usb_stream_sink_e sink, usb_stream_param_t* param);

/// Read a signal from the USB core.
/// @param   idx	signal index
/// @return  signal value
///
extern unsigned int usbGetSignal(unsigned int idx);

/// Writes a signal to the USB core.
/// @param   idx	signal index
/// @param   value	signal value
/// @return  void
///
extern void         usbSetSignal(unsigned int idx, unsigned int value);

/// Read alternate interface set on a USB core interface.
/// @param   idx	interface index
/// @return  alternate interface value
///
extern unsigned int usbGetAltInterface(unsigned int idx);

// 4: inline function implementations
// ----------------------------------------------------------------------------

// 5:  Component Usage
// --------------------------------------------------------------------------------
//  1. User is supposed to dynamically allocate a shave for processing, by calling usbInit()
//  2. User can take advantage of the particular USB configuration, as determined by the callback function
//     implementations in usbConfig.c. The interface functions permit access to interface and signal configurations,
//     as well as streaming endpoints.
//  3. User is supposed to call usbDeinit() once this component is no longer required to free up the shave
//
/// @}
#endif //_USB_CORE_API_H_
