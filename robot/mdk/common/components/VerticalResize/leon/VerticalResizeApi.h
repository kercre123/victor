///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @defgroup VerticalResize Vertical resize API
/// @{
/// @brief      This is the API to a vertical resize library in CMX.
/// 
///@details
///  In order to use the component the following steps are ought to be done:
///  1. User is supposed to declare the buffers and a variable indicating their position:
///             extern frameBuffer* vertresz_buffers;
///             extern unsigned int vertBufferPos;
///  2. User is supposed to allocate space for the component buffers (vertresz_buffers)
///  3. User is supposed to start the shaves used in application with VRESStartShave
///             resize function
///  4. User is supposed to implement the VRESASYNCDone function, as the component API
///             requires it, it may be empty, or contains code to be executed immediately after
///             component finishes vertical resize
///  5. User is supposed to call the VRESShaveDone in order to make sure the
///             component calls VRESASYNCDone and changes to the next vertical buffer position
///
#ifndef _VERTICAL_RESIZE_API_H_
#define _VERTICAL_RESIZE_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "VerticalResizeApiDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

// 4: inline function implementations
// ----------------------------------------------------------------------------

/// Start VRES shave function
/// @param[in] out_addr       Pointer to output address
/// @param[in] in_addr        Pointer to input address
/// @param[in] in_out_width   Size of input and output width
/// @param[in] in_height      Size of input height
/// @param[in] out_height     Size of output height
/// @return                   void
///
void VRESStartShave(unsigned char* out_addr, unsigned char* in_addr,
        unsigned int in_out_width, unsigned int in_height, unsigned int out_height);

/// Stop VRES shave function
/// @param[in] unused        Unused for now, defined by user
/// @return                  void
///
void VRESShaveDone(unsigned int unused);

/// Function to be implemented by user for
/// with instructions of what to do with the newly released frame
/// @param[in] unused        Unused for now, defined by user
/// @return    void
void VRESASYNCDone(unsigned int unused);

// 5:  Component Usage
// --------------------------------------------------------------------------------

/// @}
#endif //_VERTICAL_RESIZE_API_H_
