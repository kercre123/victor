/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Shave kernel example
///

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <mv_types.h>

#ifdef __cplusplus
namespace mvcv
{
#endif
//!@{
///Adds the square of the source image to the accumulator.
///@param srcAddr  The input image, 1- or 3-channel, 8-bit or 32-bit floating point
///@param destAddr The accumulator image with the same number of channels as input image, 32-bit or 64-bit floating-point
///@param maskAddr Optional operation mask
///@param width    Width of input image
///@param height   Number of lines of input images (defaulted to one line)
#ifdef __PC__
    #define AccumulateSquare_asm AccumulateSquare
#endif

#ifdef __MOVICOMPILE__
    extern "C"
	void AccumulateSquare_asm(u8** srcAddr, u8** maskAddr, fp32** destAddr,u32 width, u32 height);
#endif
#ifdef __cplusplus
    extern "C"
#endif
    void AccumulateSquare(u8** srcAddr, u8** maskAddr, fp32** destAddr,u32 width, u32 height);
//!@}

//!@{
/// AccumulateWeighted kernel - The function calculates the weighted sum of the input image (srcAddr) and the accumulator (destAddr) so that accumulator becomes a running average of frame sequence
/// @param[in] srcAddr         - array of pointers to input lines      
/// @param[in] maskAddr        - array of pointers to input lines of mask      
/// @param[out] destAddr       - array of pointers for output lines    
/// @param[in] width           - width of input line
/// @param[in] alpha           - Weight of the input image must be a fp32 between 0 and 1
#ifdef __PC__
    #define AccumulateWeighted_asm AccumulateWeighted
#endif

#ifdef __MOVICOMPILE__
    extern "C"
	void AccumulateWeighted_asm(u8** srcAddr, u8** maskAddr, fp32** destAddr,u32 width, fp32 height);
#endif
#ifdef __cplusplus
    extern "C"
#endif
    void AccumulateWeighted(u8** srcAddr, u8** maskAddr, fp32** destAddr,u32 width, fp32 alpha);
//!@}	
	
	
#ifdef __cplusplus
}
#endif


#endif //__VIDEO_H__




