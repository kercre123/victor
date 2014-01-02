///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @defgroup HorizontalLineResizeApi Horizontal resize API
/// @{
/// @brief     This is the API to a horizontal resize library in CMX.
/// 

#ifndef _HORIZONTAL_LINE_RESIZE_API_H_
#define _HORIZONTAL_LINE_RESIZE_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "HorizontalLineResizeApiDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

// 4: inline function implementations
// ----------------------------------------------------------------------------

/// Function that configures shave status
// @param[in]                  none
/// @return                     void
///
void HRESShaveConf(void);

/// Function that provides access to the input buffers (we use triple buffering)
/// @param[in] no_buf           Which buffer address to return
/// @param[in] shave_no         Number of shaves used
/// @return unsigned char*      Buffer address
unsigned char* HRESGetInBuffer(unsigned int no_buf, unsigned int shave_no);

/// Function that sets the last finished line buffer by CIF
/// @param[in] buf              Pointer to buffer address
/// @param[in] index            Index used to compute current shave used
/// @param[in] line_no          Number of the current line used by CIF
/// @return                     void
void HRESSetFinishedBuffer(unsigned char* buf, unsigned int index, unsigned int line_no);

/// Function that starts HRES shave
/// @param[in] stride           Parameter defined as distance between pixel at (y, x) and (y+1, x)
/// @param[in] in_width         Size of input width
/// @param[in] out_width        Size of output width
/// @param[in] in_out_height    Size of input and output height
/// @return                     void
void HRESStartShave(unsigned int stride, unsigned int in_width, unsigned int out_width, unsigned int in_out_height);

/// Function that stops HRES shaves
// @param[in]                  none
/// @return                     void
void HRESStopShaves(void);

/// Function called when one frame is finished
/// Must be implemented by user!
/// @param[in] unused           Unused for now, should be user defined
/// @return                     void
void HRESASYNCDone(unsigned int unused);

// 5:  Component Usage
// --------------------------------------------------------------------------------
//  In order to use the component the following steps are ought to be done:
//  1. User is supposed to declare the buffers and a variable indicating their position:
//             extern frameBuffer* horizresz_buffers;
//             extern unsigned int horizBufferPos;
//  2. User is supposed to allocate space for the component buffers (horizresz_buffers)
//  3. User is supposed to initialize CIF line buffers, by calling HRESGetInBuffer function
//  4. User is supposed to configure shaves by calling HRESShaveConf function
//  5. User is supposed to implement the HRESASYNCDone function, as the component API
//             requires it, it may be empty, or contains code to be executed immediately after
//             component finishes horizontal resize
//  6. User is supposed to apply horizontal resize effect, by calling HRESStartShave
//  7. User is supposed to call the function that sets the last finished line buffer by CIF,
//             HRESSetFinishedBuffer
//
/// @}
#endif //_HORIZONTAL_LINE_RESIZE_API_H_
