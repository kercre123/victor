///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Calculation of affine matrix of 2d rotation.
///
#ifndef __IMGWARP_H__
#define __IMGWARP_H__

#include "mvcv.h"

namespace mvcv
{

///Calculates the affine matrix of 2d rotation.
///@param center Center of the rotation in the source image
///@param angle  The rotation angle in degrees. Positive values mean counter-clockwise rotation (the coordinate origin is assumed to be the top-left corner)
///@param scale  Isotropic scale factor
///@return the resulted matrix
Mat getRotationMatrix2D(Point2f center, float angle, float scale);

//!{
	/// WarpAffineTransform kernel
	/// @param[in] in         - address of pointer to input frame
	/// @param[in] out        - address of pointer to input frame
	/// @param[in] in_width   - width of input frame
	/// @param[in] in_height  - height of input frame
	/// @param[in] out_width  - width of output frame
	/// @param[in] out_height - height of output frame
	/// @param[in] in_stride  - stride of input frame
	/// @param[in] out_stride - stride of output frame
	/// @param[in] fill_color - pointer to filled color value (if NULL no fill color)
	/// @param[in] mat        - inverted matrix from warp transformation

#ifdef __MOVICOMPILE__

    extern "C"
    void WarpAffineTransformBilinear_asm(u8** in, u8** out, u32 in_width, u32 in_height, u32 out_width, u32 out_height, u32 in_stride, u32 out_stride, u8* fill_color, half mat[2][3]);
    extern "C"
    void WarpAffineTransformNN_asm(u8** in, u8** out, u32 in_width, u32 in_height, u32 out_width, u32 out_height, u32 in_stride, u32 out_stride, u8* fill_color, half mat[2][3]);

#endif

    void WarpAffineTransformBilinear(u8** in, u8** out, u32 in_width, u32 in_height, u32 out_width, u32 out_height, u32 in_stride, u32 out_stride, u8* fill_color, float imat[2][3]);
    void WarpAffineTransformNN(u8** in, u8** out, u32 in_width, u32 in_height, u32 out_width, u32 out_height, u32 in_stride, u32 out_stride, u8* fill_color, float imat[2][3]);

}

#endif

//!}