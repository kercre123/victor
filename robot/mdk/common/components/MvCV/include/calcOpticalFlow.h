///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     Optical Flow kernel Api
///
///
#ifndef __CALC_OPTICAL_FLOW_H__
#define __CALC_OPTICAL_FLOW_H__

#include <mvstl_types.h>
#include "mvcv_types.h"

namespace mvcv
{

/// Builds pyramids for 2 images
/// @param[in] imgA         Pointer to the first input image
/// @param[in] imgB			Pointer to the second input image which should have the same size and 
///							step as the first one
/// @param[in] imgStep		Image step which means number of bytes from the beginning of line i to 
///							the beginning of line i + 1
/// @param[in] imgSize		Image width and height
/// @param[out] pyrA		Pointer to a preallocated buffer which needs to accomodate all pyramid levels
/// @param[out] pyrB		Pointer to a preallocated buffer which needs to accomodate all pyramid levels
/// @param[in] level		Number of pyramid levels to build
/// @param[in] criteria		Termination criteria for the Optical Flow algorithm, which gets adjusted here
/// @param[in] flags		Flags which can signal if a pyramid is already built
/// @param[out] imgI		Pointer to a vector of pointers which point to every pyramid level of the first image
/// @param[out] imgJ		Pointer to a vector of pointers which point to every pyramid level of the second image
/// @param[out] step		Vector of steps for each pyramid level
/// @param[out] size		Vector of sizes for each pyramid level
/// @param[out] scale		Vector of scales for each pyramid level
/// @param[in] buffer		Preallocated buffer which must accomodate the imgI, imgJ, step, size and scale buffers
/// @return					Nothing
void icvInitPyramidalAlgorithm(u8* imgA, u8* imgB, 
	int imgStep, CvSize imgSize,   
	u8 * pyrA, u8 * pyrB,   
	int level,   
	CvTermCriteria * criteria,   
	int flags,   
	u8 *** imgI, u8 *** imgJ,   
	int **step, CvSize** size,   
	float **scale, u8** buffer);

/// Optical Flow algorithm - golden implementation extracted from OpenCV
/// @param[in] imgA			First input image
/// @param[in] imgB			Second input image
/// @param[in] size			Size of the input image
/// @param[out] pyrA		Pointer to a preallocated buffer which needs to accomodate all pyramid levels
/// @param[out] pyrB		Pointer to a preallocated buffer which needs to accomodate all pyramid levels
/// @param[in] featuresA	Input list of features
/// @param[out] featuresB	Output list of tracked features
/// @param[out] status		Found/not found status of each input feature
/// @param[out] error		Error value for each tracked feature
/// @param[in] winSize		Size of the search window used for tracking
/// @param[in] level		Number of pyramid levels to generate
/// @param[in] criteria		Termination criteria for the tracking algorithm (minimum error or maximum iterations reached)
/// @param[in] flags		Flags specifying various runtime options
/// @param[in] nb_points	Number of input features
/// @return					Returns 0 if it has finished succesfully, non-0 for error
s32 calcOpticalFlowPyrLK(u8* imgA, u8* imgB, CvSize size, u8* pyrA, u8* pyrB, 
	CvPoint2D32f* featuresA, CvPoint2D32f* featuresB, u8 *status, fp32 *error, 
	CvSize winSize, u32 level, CvTermCriteria criteria, u32 flags, u32 nb_points);

/// Builds the next pyramid level by applying the Gaussian 5x5 kernel on each second pixel of the image
/// in both directions
/// @param[in] src			Input image
/// @param[out] dst			Output image which has half the size of the input image
/// @return					Nothing
void mvcvPyrDown(Mat* src, Mat* dst);

/// Builds the whole image pyramid
/// @param[out] pyr			Pyramid images including the initial image on index 0
/// @param[in] levels		Number of pyramid levels including the initial image
/// @param[in] pyrBufs		Pointers to preallocated memory that holds the pyramid levels
/// @return					Nothing
void mvcvBuildPyramid(Mat pyr[], int levels, u8* const pyrBufs[]);

/// Optical Flow algorithm - optimized implementation for Myriad
/// @param[in] pyrA			Vector containing the first image pyramid including the initial image on index 0
/// @param[in] pyrB			Vector containing the second image pyramid including the initial image on index 0
/// @param[in] featuresA	Input list of features
/// @param[in] featuresB	Output list of tracked features
/// @param[out] status		Found/not found status of each input feature
/// @param[out] error		Error value for each tracked feature
/// @param[in] _winSize		Size of the search window used for tracking
/// @param[in] levels		Number of pyramid levels to generate
/// @param[in] criteria		Termination criteria for the tracking algorithm (minimum error or maximum iterations reached)
/// @param[in] flags		Flags specifying various runtime options
/// @param[in] count		Number of input features
/// @return					Returns 0 if it has finished succesfully, non-0 for error
s32 mvcvCalcOpticalFlowPyrLK_tuned(Mat pyrA[], Mat pyrB[], CvPoint2D32f* featuresA, 
	CvPoint2D32f* featuresB, u8 *status, fp32 *error, CvSize _winSize, u32 levels, 
	CvTermCriteria criteria, u32 flags, u32 count);

}

#endif
