///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief
/// 

#ifndef _IMAGE_KERNEL_PROCESSING_API_H_
#define _IMAGE_KERNEL_PROCESSING_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "ImageKernelProcessingApiDefines.h"
#include <swcFrameTypes.h>


// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------


/// Start effect on shave
/// @param[in] out address
/// @param[in] in address
/// @return void

#ifdef __cplusplus
extern "C"
#endif

void ImageKernelProcessingStart(frameBuffer* outFrBuff, frameBuffer* inFrBuff);

#endif //_IMAGE_KERNEL_PROCESSING_API_H_
