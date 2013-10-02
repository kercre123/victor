#ifndef _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_H_
#define _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_H_

#include "anki/embeddedVision/config.h"

#include "anki/embeddedCommon.h"

#include "anki/embeddedVision/connectedComponents.h"
#include "anki/embeddedVision/fiducialMarkers.h"

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

    Result SimpleDetector_Steps1234(
      const Array<u8> &image,
      FixedLengthList<BlockMarker> &markers,
      FixedLengthList<Array<f64> > &homographies,
      const s32 scaleImage_numPyramidLevels,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      const s32 component_minimumNumPixels, const s32 component_maximumNumPixels,
      const s32 component_sparseMultiplyThreshold, const s32 component_solidMultiplyThreshold,
      const s32 component_percentHorizontal, const s32 component_percentVertical,
      const s32 quads_minQuadArea, const s32 quads_quadSymmetryThreshold, const s32 quads_minDistanceFromImageEdge,
      MemoryStack scratch1,
      MemoryStack scratch2);

    Result SimpleDetector_Steps12345(
      const Array<u8> &image,
      FixedLengthList<BlockMarker> &markers,
      FixedLengthList<Array<f64> > &homographies,
      const s32 scaleImage_numPyramidLevels,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      const s32 component_minimumNumPixels, const s32 component_maximumNumPixels,
      const s32 component_sparseMultiplyThreshold, const s32 component_solidMultiplyThreshold,
      const s32 component_percentHorizontal, const s32 component_percentVertical,
      const s32 quads_minQuadArea, const s32 quads_quadSymmetryThreshold, const s32 quads_minDistanceFromImageEdge,
      const f32 decode_minContrastRatio,
      MemoryStack scratch1,
      MemoryStack scratch2);

    //Result DetectFiducialMarkers(const Array<u8> &image, FixedLengthList<BlockMarker> &markers, MemoryStack scratch);

    Result BinomialFilter(const Array<u8> &image, Array<u8> &imageFiltered, MemoryStack scratch);

    Result DownsampleByFactor(const Array<u8> &image, const s32 downsampleFactor, Array<u8> &imageDownsampled);

    Result ComputeCharacteristicScaleImage(const Array<u8> &image, const s32 numPyramidLevels, FixedPointArray<u32> &scaleImage, MemoryStack scratch);

    Result ThresholdScaleImage(const Array<u8> &originalImage, const FixedPointArray<u32> &scaleImage, Array<u8> &binaryImage);

    Result TraceInteriorBoundary(const Array<u8> &binaryImage, const Point<s16> &startPoint, BoundaryDirection initialDirection, FixedLengthList<Point<s16> > &boundary);

    Result ComputeQuadrilateralsFromConnectedComponents(const ConnectedComponents &components, const s32 minQuadArea, const s32 quadSymmetryThreshold, const s32 minDistanceFromImageEdge, const s32 imageHeight, const s32 imageWidth, FixedLengthList<Quadrilateral<s16> > &extractedQuads, MemoryStack scratch);

    // Starting a components.Pointer(startComponentIndex), trace the exterior boundary for the
    // component starting at startComponentIndex. extractedBoundary must be at at least
    // "3*componentWidth + 3*componentHeight" (If you don't know the size of the component, you can
    // just make it "3*imageWidth + 3*imageHeight" ). It's possible that a component could be
    // arbitrarily large, so if you have the space, use as much as you have.
    //
    // endComponentIndex is the last index of the component starting at startComponentIndex. The
    // next component is therefore startComponentIndex+1 .
    //
    // Requires sizeof(s16)*(2*componentWidth + 2*componentHeight) bytes of scratch
    Result TraceNextExteriorBoundary(const ConnectedComponents &components, const s32 startComponentIndex, FixedLengthList<Point<s16> > &extractedBoundary, s32 &endComponentIndex, MemoryStack scratch);

    FixedPointArray<s32> Get1dGaussianKernel(const s32 sigma, const s32 numSigmaFractionalBits, const s32 numStandardDeviations, MemoryStack &scratch);

    // Note: uses a 32-bit accumulator, so be careful of overflows
    Result Correlate1d(const FixedPointArray<s32> &in1, const FixedPointArray<s32> &in2, FixedPointArray<s32> &out);

    // Note: uses a 32-bit accumulator, so be careful of overflows
    Result Correlate1dCircularAndSameSizeOutput(const FixedPointArray<s32> &image, const FixedPointArray<s32> &filter, FixedPointArray<s32> &out);

    // Extract the best Laplacian peaks from boundary, up to peaks.get_size() The top
    // peaks.get_size() peaks are returned in the order of their original index, which preserves
    // their original clockwise or counter-clockwise ordering.
    //
    // Requires ??? bytes of scratch
    Result ExtractLaplacianPeaks(const FixedLengthList<Point<s16> > &boundary, FixedLengthList<Point<s16> > &peaks, MemoryStack scratch);

    // Compute the homography from the input quad. The input quad point should be ordered in the non-rotated, corner-opposite format
    Result ComputeHomographyFromQuad(const Quadrilateral<s16> &quad, Array<f64> &homography, MemoryStack scratch);

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
