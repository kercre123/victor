#ifndef _ANKICORETECH_VISION_VISIONKERNELS_H_
#define _ANKICORETECH_VISION_VISIONKERNELS_H_

#include "anki/vision/config.h"
#include "anki/common.h"

namespace Anki
{
  Result BinomialFilter(const Matrix<u8> &img, Matrix<u8> &imgFiltered, MemoryStack scratch);
  Result DownsampleByFactor(const Matrix<u8> &img, u32 downsampleFactor, Matrix<u8> &imgDownsampled);

  Result ComputeCharacteristicScaleImage(const Matrix<u8> &img, u32 numLevels, FixedPointMatrix<u32> &scaleImage, MemoryStack scratch);

  template<typename T> inline T Interpolate2d(T pixel00, T pixel01, T pixel10, T pixel11, T alphaY, T alphaYinverse, T alphaX, T alphaXinverse)
  {
    const T interpolatedTop = alphaXinverse*pixel00 + alphaX*pixel01;
    const T interpolatedBottom = alphaXinverse*pixel10 + alphaX*pixel11;
    const T interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;

    return interpolatedPixel;
  }
}

#endif //_ANKICORETECH_VISION_VISIONKERNELS_H_
