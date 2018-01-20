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

#include "coretech/vision/robot/connectedComponents.h"
#include "coretech/vision/robot/fiducialMarkers.h"
#include "coretech/common/robot/geometry_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    enum CornerMethod {
      CORNER_METHOD_LAPLACIAN_PEAKS = 0,
      CORNER_METHOD_LINE_FITS = 1
    };

    struct FiducialDetectionParameters {
      bool useIntegralImageFiltering;
      bool useIlluminationNormalization;
      s32 scaleImage_numPyramidLevels;
      s32 imagePyramid_baseScale;
      s32 scaleImage_thresholdMultiplier;
      s16 component1d_minComponentWidth;
      s16 component1d_maxSkipDistance;
      s32 component_minimumNumPixels;
      s32 component_maximumNumPixels;
      s32 component_sparseMultiplyThreshold;
      s32 component_solidMultiplyThreshold;
      f32 component_minHollowRatio;
      CornerMethod cornerMethod;
      s32 minLaplacianPeakRatio;
      s32 quads_minQuadArea;
      s32 quads_quadSymmetryThreshold;
      s32 quads_minDistanceFromImageEdge;
      f32 decode_minContrastRatio;
      s32 maxConnectedComponentSegments;
      s32 maxExtractedQuads;
      s32 refine_quadRefinementIterations;
      s32 refine_numRefinementSamples;
      f32 refine_quadRefinementMaxCornerChange;
      f32 refine_quadRefinementMinCornerChange;
      Point<f32> roundedCornersFraction;
      Point<f32> fiducialThicknessFraction;
      bool returnInvalidMarkers;
      bool doCodeExtraction;
    };
    
    // The primary wrapper function for detecting fiducial markers in an image
    Result DetectFiducialMarkers(
      const Array<u8> &image,
      FixedLengthList<VisionMarker> &markers,
      const FiducialDetectionParameters& params,
      const bool isDarkOnLight,
      MemoryStack scratchCcm,
      MemoryStack scratchOnchip,
      MemoryStack scratchOffChip);

    // Used by DetectFiducialMarkers
    //
    // Compute characteristic scale, binary image, and extract connected components
    // Warning: fastScratch and slowScratch cannot be the same object pointing to the same memory
    Result ExtractComponentsViaCharacteristicScale(
      const Array<u8> &image,
      const FixedLengthList<s32> &filterHalfWidths,
      const s32 scaleImage_thresholdMultiplier,
      const s16 component1d_minComponentWidth,
      const s16 component1d_maxSkipDistance,
      const bool isDarkOnLight,
      ConnectedComponents &components,
      MemoryStack fastScratch,
      MemoryStack slowerScratch,
      MemoryStack slowestScratch);

    Result ExtractComponentsViaCharacteristicScale_binomial(
      const Array<u8> &image,
      const s32 numPyramidLevels,
      const s32 scaleImage_thresholdMultiplier,
      const s16 component1d_minComponentWidth,
      const s16 component1d_maxSkipDistance,
      ConnectedComponents &components,
      MemoryStack fastScratch,
      MemoryStack slowerScratch,
      MemoryStack slowestScratch);

    // Used by DetectFiducialMarkers
    //
    // Extracts quadrilaterals from a list of connected component segments
    Result ComputeQuadrilateralsFromConnectedComponents(const ConnectedComponents &components, const s32 minQuadArea, const s32 quadSymmetryThreshold, const s32 minDistanceFromImageEdge, const s32 minLaplacianPeakRatio, const s32 imageHeight, const s32 imageWidth, const CornerMethod cornerMethod, FixedLengthList<Quadrilateral<s16> > &extractedQuads, MemoryStack scratch);

    // Does the input quad (with corners in canonical order) have a reasonable shape?
    //
    // quadSymmetryThreshold is SQ23.8
    //
    // Reasonable values for the parameters
    // minQuadArea = 25;
    // quadSymmetryThreshold = 2 << 8;
    // minDistanceFromImageEdge = 2;
    bool IsQuadrilateralReasonable(const Quadrilateral<s16> &quad, const s32 minQuadArea, const s32 quadSymmetryThreshold, const s32 minDistanceFromImageEdge, const s32 imageHeight, const s32 imageWidth, bool &areCornersDisordered);

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
    // The ratio of the 4th peak to the 5th peak must exceed minPeakRatio or peaks will be empty.
    //
    // Requires ??? bytes of scratch
    Result ExtractLaplacianPeaks(const FixedLengthList<Point<s16> > &boundary, const s32 minPeakRatio, FixedLengthList<Point<s16> > &peaks, MemoryStack scratch);

    // Extract the best peaks, using the line fits method. Works with curved corner fiducials
    Result ExtractLineFitsPeaks(const FixedLengthList<Point<s16> > &boundary, FixedLengthList<Point<s16> > &peaks, const s32 imageHeight, const s32 imageWidth, MemoryStack scratch);

    // Used by DetectFiducialMarkers
    //
    // Uses projective Lucas-Kanade to refine an initial on-pixel quadrilateral
    // to sub-pixel position, using samples along the edges of an implicit model
    // of a black square fiducial on a white background.  If any of the corners
    // changes its position by more than maxCornerChange, the original quad and
    // homography are returned.
    //
    Result RefineQuadrilateral(
      const Quadrilateral<f32>& initialQuad,
      const Array<f32>& initialHomography,
      const Array<u8> &image,
      const Point<f32>& squareSizeFraction,
      const Point<f32>& roundedCornerFraction,
      const s32 maxIterations,
      const f32 darkValue,
      const f32 brightValue,
      const s32 numSamples,
      const f32 maxCornerChange,
      const f32 minCornerChange,
      Quadrilateral<f32>& refinedQuad,
      Array<f32>& refinedHomography,
      MemoryStack scratch);
  } // namespace Embedded
} // namespace Anki

#endif //_ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_H_
