/**
File: fiducialDetection.h
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
    // The primary wrapper function for detecting fiducial markers in an image
    Result DetectFiducialMarkers(
      const Array<u8> &image,
      //FixedLengthList<BlockMarker> &markers,
      FixedLengthList<VisionMarker> &markers,
      FixedLengthList<Array<f32> > &homographies,
      const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      const s32 component_minimumNumPixels, const s32 component_maximumNumPixels,
      const s32 component_sparseMultiplyThreshold, const s32 component_solidMultiplyThreshold,
      const f32 component_minHollowRatio,
      const s32 quads_minQuadArea, const s32 quads_quadSymmetryThreshold, const s32 quads_minDistanceFromImageEdge,
      const f32 decode_minContrastRatio,
      const s32 maxConnectedComponentSegments,
      const s32 maxExtractedQuads,
      const bool returnInvalidMarkers,
      MemoryStack scratchCcm,
      MemoryStack scratchOnchip,
      MemoryStack scratchOffChip);

    // Used by DetectFiducialMarkers
    //
    // Compute characteristic scale, binary image, and extract connected components
    // Warning: fastScratch and slowScratch cannot be the same object pointing to the same memory
    Result ExtractComponentsViaCharacteristicScale(
      const Array<u8> &image,
      const s32 scaleImage_numPyramidLevels, const s32 scaleImage_thresholdMultiplier,
      const s16 component1d_minComponentWidth, const s16 component1d_maxSkipDistance,
      ConnectedComponents &components,
      MemoryStack fastScratch,
      MemoryStack slowScratch);

    // Used by DetectFiducialMarkers
    //
    // Extracts quadrilaterals from a list of connected component segments
    Result ComputeQuadrilateralsFromConnectedComponents(const ConnectedComponents &components, const s32 minQuadArea, const s32 quadSymmetryThreshold, const s32 minDistanceFromImageEdge, const s32 imageHeight, const s32 imageWidth, FixedLengthList<Quadrilateral<s16> > &extractedQuads, MemoryStack scratch);

    // Used by DetectFiducialMarkers
    //
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
  } // namespace Embedded
} // namespace Anki

#endif //_ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_H_
