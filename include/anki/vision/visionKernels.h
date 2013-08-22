#ifndef _ANKICORETECH_VISION_VISIONKERNELS_H_
#define _ANKICORETECH_VISION_VISIONKERNELS_H_

#include "anki/vision/config.h"
#include "anki/common.h"

namespace Anki
{
  enum BoundaryDirection {
    BOUNDARY_UNKNOWN = -1,
    BOUNDARY_N  = 0,
    BOUNDARY_NE = 1,
    BOUNDARY_E  = 2,
    BOUNDARY_SE = 3,
    BOUNDARY_S  = 4,
    BOUNDARY_SW = 5,
    BOUNDARY_W  = 6,
    BOUNDARY_NW = 7
  };

  const s32 MAX_BOUNDARY_LENGTH = 5000;

  Result BinomialFilter(const Array2dUnmanaged<u8> &img, Array2dUnmanaged<u8> &imgFiltered, MemoryStack scratch);

  Result DownsampleByFactor(const Array2dUnmanaged<u8> &img, s32 downsampleFactor, Array2dUnmanaged<u8> &imgDownsampled);

  Result ComputeCharacteristicScaleImage(const Array2dUnmanaged<u8> &img, s32 numLevels, Array2dUnmanagedFixedPoint<u32> &scaleImage, MemoryStack scratch);

  Result TraceBoundary(const Array2dUnmanaged<u8> &binaryImg, const Point2<s16> &startPoint, BoundaryDirection initialDirection, FixedLengthList<Point2<s16>> &boundary);

  template<typename T> inline T Interpolate2d(T pixel00, T pixel01, T pixel10, T pixel11, T alphaY, T alphaYinverse, T alphaX, T alphaXinverse)
  {
    const T interpolatedTop = alphaXinverse*pixel00 + alphaX*pixel01;
    const T interpolatedBottom = alphaXinverse*pixel10 + alphaX*pixel11;
    const T interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;

    return interpolatedPixel;
  }
}

#endif //_ANKICORETECH_VISION_VISIONKERNELS_H_
