#ifndef __HARRIS_H__
#define __HARRIS_H__

#include "ac_defs.h"
#include "mvcv_types.h"

namespace mvcv
{

//#define MAX_FEATURES	300 //obsolete... probably
///Find interest points (corners) in an image using Harris corner detector and returns their count
///@param input_img     Image to find corners in
///@param integral_img  Integral image buffer 
///@param responses_img Filter responses buffer
///@param size          Size of input image
///@param corners       Found corners vector
///@param thres         Corner response threshold
///@return number of interest points(corners)
int harris(MVIplImage* input_img,
	MVIplImage* integral_img,
	MVIplImage* responses_img,		
	CvSize size,
	CvPoint2D32f** corners,
	float    thres);

///Crop an image patch by splitting it in height DMA transfers, each having width length
///@param[in] src        pointer to source image
///@param[in] src_stride source stride
///@param[out] dst       pointer to output image
///@param[in] dst_stride destination stride
///@param[in] width      width of input image
///@param[in] height     height of input image
void stride_copy(unsigned char *src,
	int           src_stride,
	unsigned char *dst,
	int           dst_stride,
	int           width,
	int           height);

}

#endif