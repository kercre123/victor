#ifndef __SAMPLERS_H__
#define __SAMPLERS_H__

#include <mvcv_types.h>

namespace mvcv
{

/// Extracts a rectangle out of the image, using bilinear filtering
/// @param[in] src				Source image buffer
/// @param[in] src_step			Source image step
/// @param[in] src_size			Source image size
/// @param[out] dst				Destination buffer for the extracted patch
/// @param[in] dst_step			Destination patch size
/// @param[in] win_size			Windows size which defines the size of the extracted patch
///								as: win_size.x * 2 x win_size.y * 2 centered in "rect_center"
/// @param[in] rect_center		Center of the extracted patch
/// @param[in] inPlace			Specifies if the source image is already in CMX and doesn't
///								need to be DMA-ed in
int icvGetRectSubPix_8u32f_C1R_tuned(const u8* src, int src_step, ClSizeW src_size,
	u8* dst, int dst_step, ClSizeW win_size, CvPoint2D32fW rect_center, bool inPlace);

}

#endif