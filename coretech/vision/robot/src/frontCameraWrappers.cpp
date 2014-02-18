/**
File: frontCameraWrappers.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/config.h"
#include "anki/common/robot/benchmarking_c.h"

#include "anki/vision/robot/fiducialMarkers.h"
#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/draw_vision.h"
#include "anki/vision/robot/transformations.h"

#include "anki/common/robot/matlabInterface.h"

//#define SEND_DRAWN_COMPONENTS
//#define SEND_COMPONENT_USAGE
//#define PRINTF_INTERMEDIATES

namespace Anki
{
  namespace Embedded
  {
    Result DetectFiducialMarkers(
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
      MemoryStack scratch2)
    {
      Result lastResult;

      BeginBenchmark("DetectFiducialMarkers");

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      BeginBenchmark("ExtractComponentsViaCharacteristicScale");
      ConnectedComponents extractedComponents = ConnectedComponents(maxConnectedComponentSegments, imageWidth, scratch2);
      {
        PUSH_MEMORY_STACK(scratch1); // Push the current state of the scratch buffer onto the system stack

        // 1. Compute the Scale image (use local scratch1)
        // 2. Binarize the Scale image (store in outer scratch2)
        // 3. Compute connected components from the binary image (use local scratch2, store in outer scratch1)
        if((lastResult = ExtractComponentsViaCharacteristicScale(
          image,
          scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
          component1d_minComponentWidth, component1d_maxSkipDistance,
          extractedComponents,
          scratch1)) != RESULT_OK)
        {
          /* DEBUG: drop a display of extracted components into matlab
          Embedded::Matlab matlab(false);
          matlab.PutArray(image, "image");
          Array<u8> empty(image.get_size(0), image.get_size(1), scratch1);
          Embedded::DrawComponents<u8>(empty, extractedComponents, 64, 255);
          matlab.PutArray(empty, "empty");
          matlab.EvalStringEcho("desktop; keyboard");
          */
          return lastResult;
        }
      }
      EndBenchmark("ExtractComponentsViaCharacteristicScale");

      // 3b. Remove poor components
      {
        PUSH_MEMORY_STACK(scratch1); // Push the current state of the scratch buffer onto the system stack

        BeginBenchmark("CompressConnectedComponentSegmentIds1");
        extractedComponents.CompressConnectedComponentSegmentIds(scratch1);
        EndBenchmark("CompressConnectedComponentSegmentIds1");

        BeginBenchmark("InvalidateSmallOrLargeComponents");
        if((lastResult = extractedComponents.InvalidateSmallOrLargeComponents(component_minimumNumPixels, component_maximumNumPixels, scratch1)) != RESULT_OK)
          return lastResult;
        EndBenchmark("InvalidateSmallOrLargeComponents");

        BeginBenchmark("CompressConnectedComponentSegmentIds2");
        extractedComponents.CompressConnectedComponentSegmentIds(scratch1);
        EndBenchmark("CompressConnectedComponentSegmentIds2");

        BeginBenchmark("InvalidateSolidOrSparseComponents");
        if((lastResult = extractedComponents.InvalidateSolidOrSparseComponents(component_sparseMultiplyThreshold, component_solidMultiplyThreshold, scratch1)) != RESULT_OK)
          return lastResult;
        EndBenchmark("InvalidateSolidOrSparseComponents");

        BeginBenchmark("CompressConnectedComponentSegmentIds3");
        extractedComponents.CompressConnectedComponentSegmentIds(scratch1);
        EndBenchmark("CompressConnectedComponentSegmentIds3");

        BeginBenchmark("InvalidateFilledCenterComponents");
        if((lastResult = extractedComponents.InvalidateFilledCenterComponents(component_percentHorizontal, component_percentVertical, scratch1)) != RESULT_OK)
          return lastResult;
        EndBenchmark("InvalidateFilledCenterComponents");

        BeginBenchmark("CompressConnectedComponentSegmentIds4");
        extractedComponents.CompressConnectedComponentSegmentIds(scratch1);
        EndBenchmark("CompressConnectedComponentSegmentIds4");

        BeginBenchmark("SortConnectedComponentSegmentsById");
        extractedComponents.SortConnectedComponentSegmentsById(scratch1);
        EndBenchmark("SortConnectedComponentSegmentsById");
      } // PUSH_MEMORY_STACK(scratch1);

      // 4. Compute candidate quadrilaterals from the connected components
      BeginBenchmark("ComputeQuadrilateralsFromConnectedComponents");
      FixedLengthList<Quadrilateral<s16> > extractedQuads(maxExtractedQuads, scratch1);

      if((lastResult = ComputeQuadrilateralsFromConnectedComponents(extractedComponents, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, imageHeight, imageWidth, extractedQuads, scratch1)) != RESULT_OK)
        return lastResult;

      EndBenchmark("ComputeQuadrilateralsFromConnectedComponents");

      // 4b. Compute a homography for each extracted quadrilateral
      BeginBenchmark("ComputeHomographyFromQuad");
      markers.set_size(extractedQuads.get_size());
      for(s32 iQuad=0; iQuad<extractedQuads.get_size(); iQuad++) {
        Array<f64> &currentHomography = homographies[iQuad];

        if((lastResult = Transformations::ComputeHomographyFromQuad(extractedQuads[iQuad], currentHomography, scratch1)) != RESULT_OK)
          return lastResult;

        //currentHomography.Print("currentHomography");
      } // for(iQuad=0; iQuad<; iQuad++)
      EndBenchmark("ComputeHomographyFromQuad");

      // 5. Decode fiducial markers from the candidate quadrilaterals
      const FiducialMarkerParser parser = FiducialMarkerParser(scratch2);

      BeginBenchmark("ExtractBlockMarker");
      for(s32 iQuad=0; iQuad<extractedQuads.get_size(); iQuad++) {
        const Array<f64> &currentHomography = homographies[iQuad];
        const Quadrilateral<s16> &currentQuad = extractedQuads[iQuad];
        BlockMarker &currentMarker = markers[iQuad];

        if((lastResult = parser.ExtractBlockMarker(image, currentQuad, currentHomography, decode_minContrastRatio, currentMarker, scratch1)) != RESULT_OK)
          return lastResult;
      }

      // Remove invalid markers from the list
      for(s32 iQuad=0; iQuad<extractedQuads.get_size(); iQuad++) {
        if(markers[iQuad].blockType == -1) {
          for(s32 jQuad=iQuad; jQuad<extractedQuads.get_size(); jQuad++) {
            markers[jQuad] = markers[jQuad+1];
            homographies[jQuad] = homographies[jQuad+1];
          }
          extractedQuads.set_size(extractedQuads.get_size()-1);
          markers.set_size(markers.get_size()-1);
          homographies.set_size(homographies.get_size()-1);
          iQuad--;
        }
      }
      EndBenchmark("ExtractBlockMarker");

      EndBenchmark("DetectFiducialMarkers");

      return RESULT_OK;
    } //  DetectFiducialMarkers()
  } // namespace Embedded
} // namespace Anki
