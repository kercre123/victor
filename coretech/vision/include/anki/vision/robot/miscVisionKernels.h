/**
File: miscVisionKernels.h
Author: Peter Barnum
Created: 2013

Various vision kernels. Should probably be refactored.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_H_
#define _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_H_

#include "anki/vision/robot/connectedComponents.h"
#include "anki/vision/robot/fiducialMarkers.h"

namespace Anki
{
  namespace Embedded
  {
    typedef enum {
      BOUNDARY_UNKNOWN = -1,
      BOUNDARY_N  = 0,
      BOUNDARY_NE = 1,
      BOUNDARY_E  = 2,
      BOUNDARY_SE = 3,
      BOUNDARY_S  = 4,
      BOUNDARY_SW = 5,
      BOUNDARY_W  = 6,
      BOUNDARY_NW = 7
    } BoundaryDirection;

    const s32 MAX_BOUNDARY_LENGTH = 2000;

    typedef enum {
      CHARACTERISTIC_SCALE_ORIGINAL = 0,
      CHARACTERISTIC_SCALE_MEDIUM_MEMORY = 1,
      CHARACTERISTIC_SCALE_LOW_MEMORY = 2
    } CharacteristicScaleAlgorithm;

    // Replaces the matlab code for the first three steps of SimpleDetector
    Result SimpleDetector_Steps123(
      const Array<u8> &image,
      const CharacteristicScaleAlgorithm scaleAlgorithm,
      const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier,
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
      const CharacteristicScaleAlgorithm scaleAlgorithm,
      const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier,
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
      const CharacteristicScaleAlgorithm scaleAlgorithm,
      const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      const s32 component_minimumNumPixels, const s32 component_maximumNumPixels,
      const s32 component_sparseMultiplyThreshold, const s32 component_solidMultiplyThreshold,
      const s32 component_percentHorizontal, const s32 component_percentVertical,
      const s32 quads_minQuadArea, const s32 quads_quadSymmetryThreshold, const s32 quads_minDistanceFromImageEdge,
      const f32 decode_minContrastRatio,
      MemoryStack scratch1,
      MemoryStack scratch2);

    Result SimpleDetector_Steps12345_lowMemory(
      const Array<u8> &image,
      FixedLengthList<BlockMarker> &markers,
      FixedLengthList<Array<f64> > &homographies,
      const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      const s32 component_minimumNumPixels, const s32 component_maximumNumPixels,
      const s32 component_sparseMultiplyThreshold, const s32 component_solidMultiplyThreshold,
      const s32 component_percentHorizontal, const s32 component_percentVertical,
      const s32 quads_minQuadArea, const s32 quads_quadSymmetryThreshold, const s32 quads_minDistanceFromImageEdge,
      const f32 decode_minContrastRatio,
      const s32 maxConnectedComponentSegments,
      const s32 maxExtractedQuads,
      MemoryStack scratch1,
      MemoryStack scratch2);

    //Result DetectFiducialMarkers(const Array<u8> &image, FixedLengthList<BlockMarker> &markers, MemoryStack scratch);

    // Compute the characteristic scale using the binomial filter and a LOT of scratch memory
    Result ComputeCharacteristicScaleImage(const Array<u8> &image, const s32 numPyramidLevels, FixedPointArray<u32> &scaleImage, MemoryStack scratch);

    // Compute the characteristic scale using the box filter and a little scratch memory
    // thresholdMultiplier is SQ15.16
    Result ComputeCharacteristicScaleImageAndBinarize(const Array<u8> &image, const s32 numPyramidLevels, Array<u8> &binaryImage, const s32 thresholdMultiplier, MemoryStack scratch);

    Result ComputeCharacteristicScaleImageAndBinarizeAndExtractComponents(
      const Array<u8> &image,
      const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      ConnectedComponents &components,
      MemoryStack scratch);

    Result BinarizeScaleImage(const Array<u8> &originalImage, const FixedPointArray<u32> &scaleImage, Array<u8> &binaryImage);

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

    // Extract the best Laplacian peaks from boundary, up to peaks.get_size() The top
    // peaks.get_size() peaks are returned in the order of their original index, which preserves
    // their original clockwise or counter-clockwise ordering.
    //
    // Requires ??? bytes of scratch
    Result ExtractLaplacianPeaks(const FixedLengthList<Point<s16> > &boundary, FixedLengthList<Point<s16> > &peaks, MemoryStack scratch);

    // Compute the homography from the input quad. The input quad point should be ordered in the non-rotated, corner-opposite format
    Result ComputeHomographyFromQuad(const Quadrilateral<s16> &quad, Array<f64> &homography, MemoryStack scratch);

    // Calculate local extrema (edges) in an image. Returns four list for the four directions of change.
    Result DetectBlurredEdge(const Array<u8> &image, const u8 grayvalueThreshold, const s32 minComponentWidth, FixedLengthList<Point<s16> > &xDecreasing, FixedLengthList<Point<s16> > &xIncreasing, FixedLengthList<Point<s16> > &yDecreasing, FixedLengthList<Point<s16> > &yIncreasing);

    // With region of interest
    Result DetectBlurredEdge(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, FixedLengthList<Point<s16> > &xDecreasing, FixedLengthList<Point<s16> > &xIncreasing, FixedLengthList<Point<s16> > &yDecreasing, FixedLengthList<Point<s16> > &yIncreasing);
  } // namespace Embedded
} // namespace Anki

#endif //_ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_H_
