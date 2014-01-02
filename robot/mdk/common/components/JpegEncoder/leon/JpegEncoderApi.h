///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup JpegEncoderApi JPEG Encoder API.
/// @{
/// @brief     JPEG Encoder Functions API.
///
/// This is the API to a simple JPEG library implementing a jpeg encoding process.
///

#ifndef _JPEG_ENCODER_API_H_
#define _JPEG_ENCODER_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "JpegEncoderApiDefines.h"


// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------
extern unsigned char jpeg_buff[1024 * 1024];
extern int outbytes;


// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

// 4: inline function implementations
// ----------------------------------------------------------------------------

/// The JPEG encoding algorithm
/// @param[in] jFmSpec        Pointer to jpegFrameSpec structure
/// @param[in] quality_coef   Generic, quality coefficients are between 20-30 to max 90
/// @return                   void
///
void JPEG_encode(jpegFrameSpec *jFmSpec, unsigned int quality_coef);

// 5:  Component Usage
// --------------------------------------------------------------------------------
//  In order to use the component the following steps are ought to be done:
//  1. User should declare variables for the 3 image planes:
//           extern unsigned char img_Y;
//           extern unsigned char img_U;
//           extern unsigned char img_V;
//  2. User should configure the image, by using a variable of jpegFrameSpec type,
//           which is a structure whose fields are the 3 image planes, and information
//           about image width and height
//  4. User should start applying the encode algorithm on the image, by calling JPEG_encode function
//  5. User can verify the processing result, by making a memory dump from "jpeg_buff",
//           having the size equal to "outbytes"
//
/// @}
#endif
