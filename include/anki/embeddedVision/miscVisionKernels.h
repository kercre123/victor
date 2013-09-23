#ifndef _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_H_
#define _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_H_

#include "anki/embeddedVision/config.h"
#include "anki/embeddedCommon.h"
#include "anki/embeddedVision/connectedComponents.h"

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

    // Replaces the matlab code for the first three steps of SimpleDetector
    Result SimpleDetector_Steps123(
      const Array<u8> &image,
      const s32 scaleImage_numPyramidLevels,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      const s32 component_minimumNumPixels, const s32 component_maximumNumPixels,
      const s32 component_sparseMultiplyThreshold, const s32 component_solidMultiplyThreshold,
      ConnectedComponents &extractedComponents,
      MemoryStack scratch1,
      MemoryStack scratch2);

    //Result DetectFiducialMarkers(const Array<u8> &image, FixedLengthList<FiducialMarker> &markers, MemoryStack scratch);

    Result BinomialFilter(const Array<u8> &image, Array<u8> &imageFiltered, MemoryStack scratch);

    Result DownsampleByFactor(const Array<u8> &image, const s32 downsampleFactor, Array<u8> &imageDownsampled);

    Result ComputeCharacteristicScaleImage(const Array<u8> &image, const s32 numPyramidLevels, FixedPointArray<u32> &scaleImage, MemoryStack scratch);

    Result ThresholdScaleImage(const Array<u8> &originalImage, const FixedPointArray<u32> &scaleImage, Array<u8> &binaryImage);

    Result TraceBoundary(const Array<u8> &binaryImage, const Point<s16> &startPoint, BoundaryDirection initialDirection, FixedLengthList<Point<s16> > &boundary);

    template<typename T> inline T Interpolate2d(const T pixel00, const T pixel01, const T pixel10, const T pixel11, const T alphaY, const T alphaYinverse, const T alphaX, const T alphaXinverse);

#pragma mark --- Implementations ---

    template<typename T> inline T Interpolate2d(const T pixel00, const T pixel01, const T pixel10, const T pixel11, const T alphaY, const T alphaYinverse, const T alphaX, const T alphaXinverse)
    {
      const T interpolatedTop = alphaXinverse*pixel00 + alphaX*pixel01;
      const T interpolatedBottom = alphaXinverse*pixel10 + alphaX*pixel11;
      const T interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;

      return interpolatedPixel;
    }
  } // namespace Embedded
} // namespace Anki

#endif //_ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_H_
