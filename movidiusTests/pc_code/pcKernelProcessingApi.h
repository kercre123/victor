///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief


#ifndef _PC_KERNEL_PROCESSING_API_H_
#define _PC_KERNEL_PROCESSING_API_H_

// 1: Includes
// ----------------------------------------------------------------------------

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------


/// Start effect on PC
/// @param[in] in file name
/// @param[in] resolution width
/// @param[in] resolution  height
/// @param[in] input number frames
/// @param[in] out file name
/// @return void
void pcKernelProcessing(char * inFileName, int width, int height, int nrFrames, char * outFileName);

#endif //_PC_KERNEL_PROCESSING_API_H_



