#ifndef __KERNELS_H__
#define __KERNELS_H__

#include <mvcv_types.h>

namespace mvcv
{

/// Computes the gradients matrix over a patch of the image as described in the Lucas-Kanade paper.
/// Specific to the Optical Flow algorithm.
/// @param[in] Ix				Template patch containing gradient in horizontal direction
/// @param[in] isz				Patch size
/// @param[in] minI				Top-left corner of the patch
/// @param[in] Iy				Template patch containing gradient in vertical direction
/// @param[in] jsz				Size of the window overlapping the patch - used to compute the gradients only
///								on the common pixels resulted from the overlap of I and J patches
/// @param[in] minJ				Top-left corner of the search window
/// @param[out] G				Gradients matrix: Gxx, Gyy, Gxy, D, where D is the determinant of the gradients matrix
#ifdef __PC__
    #define CalcG_asm						CalcG
#endif

#ifdef __MOVICOMPILE__
    extern "C" 
	void CalcG_asm(const float* Ix, ClSize isz, CvPoint minI,
        const float* Iy, ClSize jsz, CvPoint minJ, Scalar& G);
#endif
	void CalcG(const float* Ix, ClSize isz, CvPoint minI, const float* Iy, ClSize jsz, 
		CvPoint minJ, Scalar& G);

/// Computes a measure of dissimilarity between 2 patches, in x and y directions and the SSD.
/// Specific to the Optical Flow algorithm.
/// @param[in] patchI			Template patch we are searching for, noted as I
/// @param[in] isz				Size of the template patch
/// @param[in] Ix				Template patch containing gradient in horizontal direction
/// @param[in] Iy				Template patch containing gradient in vertical direction
/// @param[in] minI				Top-left corner of the template patch
/// @param[in] patchJ			Patch from the image we are searching in
/// @param[in] jsz				Size of the J patch
/// @param[in] minJ				Top-left corner of the J patch
/// @param[out] error			Error value computed using SSD
/// @return						(Bx, By) pair of values which mean the dissimilarity between
///								patches I and J on x and y directions
#ifdef __PC__
    #define CalcBxBy_asm                    CalcBxBy
#endif

#ifdef __MOVICOMPILE__
    extern "C" 
	CvPoint2D32f CalcBxBy_asm(const u8* patchI, ClSize isz,
        const float* Ix, const float* Iy, CvPoint minI, const u8* patchJ,
        ClSize jsz, CvPoint minJ, float& error);
#endif
	CvPoint2D32f CalcBxBy(const u8* patchI, ClSize isz, const float* Ix, 
		const float* Iy, CvPoint minI, const u8* patchJ, ClSize jsz, CvPoint minJ,
		float& error);

// Bilinear Interpolation Component Usage
// --------------------------------------------------------------------------------
//  In order to use the component, keep track of the following:
//  1. Bilinear Interpolation is just a kernel, therefore data to be processed needs to
//             already be present in CMX
//  2. ClSize and CVPoint2D32f, non-POD types are mapped to OpenCL vector types, because
//             it showed significant performance improvement
//  3. It works on a patch size of 2 * win_size, centered in "center"
//  4. Patch size for input is considered to be NxM and for output is (N-1)x(M-1)
//             because adjacent pixels are always needed to compute 1 pixel
//
/// bilinear Interpolation kernel   used in texture mapping, uses
///                                 nearest pixel values which are located in diagonal directions
/// @param[in] src         Pointer to input source
/// @param[in] src_step    Step (stride) for source
/// @param[in] dst         Pointer to destination
/// @param[in] dst_step    Step (stride) for destination
/// @param[in] win_size    Window size
/// @param[in] center      Center coordinates
/// @return                Nothing
#ifdef __PC__
    #define bilinearInterpolation16u_asm    bilinearInterpolation16u
#endif

#ifdef __MOVICOMPILE__
    extern "C" 
	void bilinearInterpolation16u_asm(const u8* src, s32 src_step,
        u8* dst, s32 dst_step, ClSize win_size, CvPoint2D32f center);
#endif
	void bilinearInterpolation16u(const u8* src, s32 src_step, u8* dst, s32 dst_step, 
		ClSize win_size, CvPoint2D32f center);

/// Generic convolution with a 3x3 separable kernel using fp32 precision
/// @param[in] src			Input image to compute the convolution on
/// @param[in] src_step		Step size of the input image
/// @param[out] dstX		Output buffer containing convolution result in the X direction
/// @param[out] dstY		Output buffer containing convolution result in the Y direction
/// @param[in] dst_step		Step sze of the output buffers
/// @param[in] src_size		Size of the input image
/// @param[in] smooth_k		Coefficients of the convolution kernel
/// @param[in] buffer0		Preallocated scratch buffer for intermediate data
	void conv3x3fp32(const u8* src, int src_step, float* dstX, float* dstY, 
       int dst_step, ClSizeW src_size, const float* smooth_k, float* buffer0);

/// Convolution with a 3x3 separable kernel using fp32 precision for the specific case of a 13x13 patch
/// @param[in] src			Input image to compute the convolution on
/// @param[out] dstX		Output buffer containing convolution result in the X direction
/// @param[out] dstY		Output buffer containing convolution result in the Y direction
/// @param[in] smooth_k		Coefficients of the convolution kernel
#ifdef __PC__
    #define conv3x3fp32_13x13_asm           conv3x3fp32_13x13
#endif
#ifdef __MOVICOMPILE__
	extern "C" 
	void conv3x3fp32_13x13_asm( const u8* src, float* dstX, float* dstY, 
		const float* smooth_k);
#endif
	void conv3x3fp32_13x13(const u8* src, float* dstX, float* dstY, 
		const float* smooth_k);

}

#endif
