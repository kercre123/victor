///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup H264EncoderApi H264 Encoder API.
/// @{
/// @brief     H264 Encoder API.
///
/// This is the API

#ifndef _H264_ENCODER_API_H_
#define _H264_ENCODER_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "H264EncoderApiDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Initializes the allocated shaves, resets them and loads appropriate mbin, based on cmdCfg field.
/// Also H264 encoding parameters are passed along at this step.
/// The number of encoder shaves must be between 2 and 4.
/// @param   param	pointer to control data structure
/// @return  error code
///
extern unsigned int H264EncodeInit(h264_encoder_t *param);

/// Deinitializes previously allocated shaves, by reseting and powering it down.
/// @return  error code
///
extern unsigned int H264EncodeDeinit(void);

/// Checks if a NALU buffer is filled and clears availability flags.
/// The buffer parameters are written to the passed parameter data structure.
/// @param   param	pointer to parameter data structure
/// @return  error code
///
extern unsigned int H264EncodeGetBuffer(h264_encode_param_t * param);

/// Tries to pass an empty NALU buffer if availability flags permit.
/// The buffer parameters are read from the passed parameter data structure.
/// @param   param	pointer to parameter data structure
/// @return  error code
///
extern unsigned int H264EncodeSetBuffer(h264_encode_param_t * param);

/// Returns the currently processed frame's parameters.
/// The buffer parameters are written to the passed parameter data structure.
/// @param   param	pointer to parameter data structure
/// @return  error code
///
extern unsigned int H264EncodeGetFrame(h264_encode_param_t *param);

/// Tries to pass a new frame buffer if availability flags permit.
/// The buffer parameters are read from the passed parameter data structure.
/// @param   param	pointer to parameter data structure
/// @return  error code
///
extern unsigned int H264EncodeSetFrame(h264_encode_param_t *param);

// 4: inline function implementations
// ----------------------------------------------------------------------------

/// @}
#endif //_H264_ENCODER_API_H_
