#ifndef _ANKICORETECH_VISION_VISIONKERNELS_H_
#define _ANKICORETECH_VISION_VISIONKERNELS_H_

#include "anki/vision/config.h"
#include "anki/common.h"

namespace Anki
{
  Result BinomialFilter(const Matrix<u8> &img, Matrix<u8> &imgFiltered, MemoryStack scratch);
  Result DownsampleByFactor(const Matrix<u8> &img, u32 downsampleFactor, Matrix<u8> &imgDownsampled);
}

#endif //_ANKICORETECH_VISION_VISIONKERNELS_H_
