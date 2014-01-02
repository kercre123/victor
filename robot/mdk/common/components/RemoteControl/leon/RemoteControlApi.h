///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup RemoteControl Remote Control API
/// @{
/// @brief     Configure Remote Control
///


#ifndef _REMOTE_CONTROL_API_H_
#define _REMOTE_CONTROL_API_H_


// 1: Includes
// ----------------------------------------------------------------------------
#include <mv_types.h>
#include "RemoteControlDefinesApi.h"

// 2:  Exported Global Data (generally better to avoid)

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Verify the system clock in order to catch a correct command
/// @param[in] infraRedGpio  the IR GPIO
/// @return    void
///
void NECRemoteControlMain (int infraRedGpio);

/// Initialize the remote control functionality.
/// @param[in] infraRedGpio  the IR GPIO
/// @return    void
///
void NECRemoteControlInit (int infraRedGpio);

/// Gets the current command code.
/// @return    the command code
///
u8  NECRemoteControlGetCurrentKey (void);

/// Returns the button that generated the command.
/// @param[in] detectedCode  the code that has been detected
/// @return    the button that generate the command
///
int NECRemoteControlGetButton (unsigned int detectedCode);


// 4: inline function implementations
// ----------------------------------------------------------------------------

/// @}
#endif //_REMOTE_CONTROL_API_H_

