#ifndef _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_H_
#define _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_H_

#include "anki/embeddedVision/config.h"
#include "anki/embeddedCommon.h"

namespace Anki
{
  namespace Embedded
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

    const s32 MAX_BOUNDARY_LENGTH = 2000;

    Result BinomialFilter(const Array_u8 &image, Array_u8 &imageFiltered, MemoryStack scratch);

    Result DownsampleByFactor(const Array_u8 &image, s32 downsampleFactor, Array_u8 &imageDownsampled);

    Result ComputeCharacteristicScaleImage(const Array_u8 &image, s32 numLevels, Array_u32 &scaleImage, MemoryStack scratch);

    Result TraceBoundary(const Array_u8 &binaryImage, const Point_s16 &startPoint, BoundaryDirection initialDirection, FixedLengthList_Point_s16 &boundary);

    template<typename T> inline T Interpolate2d(T pixel00, T pixel01, T pixel10, T pixel11, T alphaY, T alphaYinverse, T alphaX, T alphaXinverse)
    {
      const T interpolatedTop = alphaXinverse*pixel00 + alphaX*pixel01;
      const T interpolatedBottom = alphaXinverse*pixel10 + alphaX*pixel11;
      const T interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;

      return interpolatedPixel;
    }

    Result extract1dComponents(const u8 * restrict binaryImageRow, const s16 binaryImageWidth, const s16 minComponentWidth, FixedLengthList_Component1d &extractedComponents);
  } // namespace Embedded
} // namespace Anki

#endif //_ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_H_
