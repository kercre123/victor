///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup biResizeApi Bilinear resize component
/// @{
/// @brief     Bilinear resize API
///
/// This is the API

#ifndef _BI_RESIZE_API_H_
#define _BI_RESIZE_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "biResizeApiDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------


// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Initializes the allocated shave, by powering it up, reseting and loading the mbin.
/// @param   svuNr	allocated shave id
/// @return  error code
///
unsigned int biResizeInit(unsigned int svuNr);

/// Deinitializes a previously allocated shave, by reseting and powering it down.
/// @return  error code
///
unsigned int biResizeDeinit(void);

/// Get the currently stored control parameters
/// @param   param	pointer to parameters
/// @return  error code
///
unsigned int biResizeGet(biResize_param_t* param);

/// Set the currently stored control parameters, if shave is free it also kicks of processing.
/// Setting parameters cyclically is the way to assign new frames to be resized.
/// No interrupt support to report resize completion, so this function return code signals completion.
/// @param   param	pointer to parameters
/// @return  error code
///
unsigned int biResizeSet(biResize_param_t* param);

// 4: inline function implementations
// ----------------------------------------------------------------------------

// 5:  Component Usage
// --------------------------------------------------------------------------------
//  1. User is supposed to dynamically allocate a shave for processing, by calling biResizeInit()
//  2. User is supposed to apply the resize by calling biResizeSet() function, passing the configuration as parameter
//  3. User is supposed to call biResizeDeinit() once this component is no longer required to free up the shave
//
/// @}
#endif //_BI_RESIZE_API_H_
