///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Core kernels definitions
///
/// This is the API to the MvCV kernels library
///

#ifndef __CORE_H__
#define __CORE_H__

#include <mv_types.h>
#include <mvcv_types.h>

#ifdef __cplusplus
namespace mvcv
{
#endif

//!@{
/// minMax kernel - computes the minimum and the maximum value of a given input image
/// @param[in] in         - array of pointers to input lines
/// @param[in] width      - line`s width(length)
/// @param[in] height     - height of image (defaulted to one line)
/// @param[in] minVal     - stores the minimum value on the line
/// @param[in] maxVal     - stores the maximum value on the line
/// @param[in] maskAddr   - mask filled with 1s and 0s which determines the image area to compute minimum and maximum
/// @return               - Nothing
#ifdef __PC__

    #define minMaxKernel_asm minMaxKernel
#endif

#ifdef __MOVICOMPILE__
#ifdef __cplusplus
    extern "C"
#endif
    void minMaxKernel_asm(u8** in, u32 width, u32 height, u8* minVal, u8* maxVal, u8* maskAddr);
#endif

#ifdef __cplusplus
    extern "C"
#endif
    void minMaxKernel(u8** in, u32 width, u32 height, u8* minVal, u8* maxVal, u8* maskAddr);
//!@}
	
//!@{	
/// pixel Position kernel      - returns the position of a given pixel value
/// @param[in] srcAddr         - array of pointers to input lines
/// @param[in] maskAddr        - mask filled with 1s and 0s which determines the image area to find position
/// @param[in] width           - line`s width(length)
/// @param[in] pixelValue      - stores the pixel value to be searched
/// @param[out] pixelPosition  - stores the position occupied by the searched value within line
/// @param[out] status         - stores 0x11 if pixel value found, else 0x00
/// @return               - Nothing
#ifdef __PC__

    #define pixelPos_asm  pixelPos
#endif

#ifdef __MOVICOMPILE__
#ifdef __cplusplus
    extern "C"
#endif
    void pixelPos_asm(u8** srcAddr, u8* maskAddr, u32 width, u8 pixelValue, u32* pixelPosition, u8* status);
#endif

#ifdef __cplusplus
    extern "C"
#endif
    void pixelPos(u8** srcAddr, u8* maskAddr, u32 width, u8 pixelValue, u32* pixelPosition, u8* status);
//!@}


//!@{
/// minMaxpOS kernel     - computes the minimum and the maximum value of a given input line and their position
/// @param[in] in        - input line
/// @param[in] width     - line`s width(length)
/// @param[in] height    - height of image (defaulted to one line)
/// @param[in] minVal    - stores the minimum value on the line
/// @param[in] maxVal    - stores the maximum value on the line
/// @param[out] minPos   - stores the position occupied by the MIN value within line
/// @param[out] maxPos   - stores the position occupied by the MAX value within line
/// @param[in] maskAddr  - mask filled with 1s and 0s which determines the image area to compute minimum and maximum
/// @return              - Nothing
#ifdef __PC__

    #define minMaxPos_asm minMaxPos
#endif

#ifdef __MOVICOMPILE__
#ifdef __cplusplus
    extern "C"
#endif
    void minMaxPos_asm(u8** in, u32 width, u8* minVal, u8* maxVal, u32* minPos, u32* maxPos, u8* maskAddr);
#endif

#ifdef __cplusplus
    extern "C"
#endif
    void minMaxPos(u8** in, u32 width, u8* minVal, u8* maxVal, u32* minPos, u32* maxPos, u8* maskAddr);
//!@}
	
//!@{
///Calculates mean and standard deviation of an array of elements
///@param[in]  in     - pointer to input line
///@param[out] mean   - computed mean value
///@param[out] stddev - computed standard deviation
///@param[out] width  - width of line
#ifdef __PC__
    #define meanstddev_asm meanstddev
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void meanstddev_asm(u8** in, float *mean, float *stddev, u32 width);
#endif
    void meanstddev(u8** in, float *mean, float *stddev, u32 width);
//!@}

#ifdef __cplusplus
}
#endif


#endif /* __CORE_H__ */
