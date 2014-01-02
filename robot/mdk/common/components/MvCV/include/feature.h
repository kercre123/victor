///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     ImgProc kernels definitions
///
/// This is the API to the MvCV kernels library
///

#ifndef __FEATURE_H__
#define __FEATURE_H__

#include "mat.h"

#ifdef __cplusplus
namespace mvcv
{
#endif

//!@{
/// cannyEdge filter - The function finds edges in the input image image and marks them in the output map edges using the Canny algorithm(9x9 kernel size).
/// The smallest value between threshold1 and threshold2 is used for edge linking. The largest value is used to find initial segments of strong edges.
/// @param[in]  srcAddr    - array of pointers to input lines      
/// @param[out] dstAddr    - pointers for output line    
/// @param[in]  threshold1 - lower threshold
/// @param[in]  threshold2 - upper threshold
/// @param[in]  width      - width of input line
//if the user wants to have at output width pixels computed correctly , he must provide 4+width+4 pixels, at output the image will have still 4+width+4 pixels, but the first 4 and the last 4 will be 0
#ifdef __PC__
    #define canny_asm canny
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void canny_asm(u8** srcAddr, u8** dstAddr,  float threshold1, float threshold2, u32 width);
#endif
#ifdef __cplusplus
    extern "C"
#endif
    void canny(u8** srcAddr, u8** dstAddr,  float threshold1, float threshold2, u32 width);
//!@}	

//!@{
/// CornerMinEigenVal_patched filter - Calculate MinEigenVal - kernel size is 5x5 
/// @param[in]  in_lines       - array of pointers to input lines
/// @param[in]  posy           - position on line
/// @param[out] out_pix        - pointer to output pixel    
/// @param[in]  width          - width of line
#ifdef __PC__
    #define CornerMinEigenVal_patched_asm CornerMinEigenVal_patched
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void CornerMinEigenVal_patched_asm(u8 **in_lines, u32 posy, u8 *out_pix, u32 width);
#endif
#ifdef __cplusplus
    extern "C"
#endif
    void CornerMinEigenVal_patched(u8 **in_lines, u32 posy, u8 *out_pix, u32 width);
//!@}	

//!@{
/// CornerMinEigenVal filter   - Calculate MinEigenVal - kernel size is 5x5 
/// @param[in]   input_lines   - array of pointers to input lines
/// @param[out]  output_line   - pointer to output line
/// @param[in]   width         - width of line

#ifdef __PC__
    #define CornerMinEigenVal_asm CornerMinEigenVal
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void CornerMinEigenVal_asm(u8 **in_lines, u8 **out_line, u32 width);
#endif
#ifdef __cplusplus
    extern "C"
#endif
    void CornerMinEigenVal(u8 **in_lines, u8 **out_line, u32 width);
//!@}	

//!@{	
/// Fast9 - corner feature detection
/// @param[in]  in_lines   - array of pointers to input lines
/// @param[out] score      - pointer to corner score buffer
/// @param[out] base       - pointer to corner candidates buffer ; first element is the number of candidates, the rest are the position of coordinates
/// @param[in]  posy       - y position
/// @param[in]  thresh     - threshold
/// @param[in]  width      - number of pixels to process
#ifdef __PC__
    #define fast9_asm fast9
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void fast9_asm(u8** in_lines, u8* score,  unsigned int *base, u32 posy, u32 thresh, u32 width);
#endif
#ifdef __cplusplus
    extern "C"
#endif
    void fast9(u8** in_lines, u8* score,  unsigned int *base, u32 posy, u32 thresh, u32 width);
//!@}	

#ifdef __cplusplus
}
#endif
#endif











