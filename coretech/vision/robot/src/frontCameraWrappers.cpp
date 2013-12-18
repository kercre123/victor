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
#include "anki/vision/robot/miscVisionKernels.h"
#include "anki/vision/robot/draw_vision.h"

//#define SEND_DRAWN_COMPONENTS
//#define SEND_COMPONENT_USAGE
//#define PRINTF_INTERMEDIATES

namespace Anki
{
  namespace Embedded
  {
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
      MemoryStack scratch2)
    {
      // TODO: figure out a simpler way to write code that reuses big blocks of memory

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      Array<u8> binaryImage(imageHeight, imageWidth, scratch2);

      // 1. Compute the Scale image (use local scratch1)
      // 2. Binarize the Scale image (store in outer scratch2)
      if(scaleAlgorithm == CHARACTERISTIC_SCALE_ORIGINAL) {
        PUSH_MEMORY_STACK(scratch1); // Push the current state of the scratch buffer onto the system stack

        FixedPointArray<u32> scaleImage(imageHeight, imageWidth, 16, scratch1);

        if(ComputeCharacteristicScaleImage(image, scaleImage_numPyramidLevels, scaleImage, scratch2) != RESULT_OK) {
          return RESULT_FAIL;
        }

        if(BinarizeScaleImage(image, scaleImage, binaryImage) != RESULT_OK) {
          return RESULT_FAIL;
        }

        // image.Show("image", false, false);
        // binaryImage.Show("binaryImage", true, true);
      } else if(scaleAlgorithm == CHARACTERISTIC_SCALE_MEDIUM_MEMORY) {
        if(ComputeCharacteristicScaleImageAndBinarize(image, scaleImage_numPyramidLevels, binaryImage, scaleImage_thresholdMultiplier, scratch1) != RESULT_OK)
          return RESULT_FAIL;
      }

      // 3. Compute connected components from the binary image (use local scratch2, store in outer scratch1)
      //FixedLengthList<ConnectedComponentSegment> extractedComponents(maxConnectedComponentSegments, scratch1);
      {
        PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

        if(extractedComponents.Extract2dComponents_FullImage(binaryImage, component1d_minComponentWidth, component1d_maxSkipDistance) != RESULT_OK) {
          return RESULT_FAIL;
        }

        //{
        //  PUSH_MEMORY_STACK(scratch1);
        //  Array<u8> drawnComponents(imageHeight, imageWidth, scratch1);
        //  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);
        //  drawnComponents.Show("drawnComponents0", false);
        //}

        extractedComponents.CompressConnectedComponentSegmentIds(scratch2);

        //{
        //  PUSH_MEMORY_STACK(scratch1);
        //  Array<u8> drawnComponents(imageHeight, imageWidth, scratch1);
        //  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);
        //  drawnComponents.Show("drawnComponents1", false);
        //}

        if(extractedComponents.InvalidateSmallOrLargeComponents(component_minimumNumPixels, component_maximumNumPixels, scratch2) != RESULT_OK) {
          return RESULT_FAIL;
        }

        //{
        //  PUSH_MEMORY_STACK(scratch1);
        //  Array<u8> drawnComponents(imageHeight, imageWidth, scratch1);
        //  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);
        //  drawnComponents.Show("drawnComponents2", false);
        //}

        extractedComponents.CompressConnectedComponentSegmentIds(scratch2);

        //{
        //  PUSH_MEMORY_STACK(scratch1);
        //  Array<u8> drawnComponents(imageHeight, imageWidth, scratch1);
        //  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);
        //  drawnComponents.Show("drawnComponents3", false);
        //}

        if(extractedComponents.InvalidateSolidOrSparseComponents(component_sparseMultiplyThreshold, component_solidMultiplyThreshold, scratch2) != RESULT_OK) {
          return RESULT_FAIL;
        }

        //{
        //  PUSH_MEMORY_STACK(scratch1);
        //  Array<u8> drawnComponents(imageHeight, imageWidth, scratch1);
        //  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);
        //  drawnComponents.Show("drawnComponents4", false);
        //}

        extractedComponents.CompressConnectedComponentSegmentIds(scratch2);

        //{
        //  PUSH_MEMORY_STACK(scratch1);
        //  Array<u8> drawnComponents(imageHeight, imageWidth, scratch1);
        //  DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);
        //  drawnComponents.Show("drawnComponents4", true);
        //}
      } // PUSH_MEMORY_STACK(scratch2);

      return RESULT_OK;
    } // SimpleDetector_Steps123()

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
      MemoryStack scratch2)
    {
      // TODO: figure out a simpler way to write code that reuses big blocks of memory

      const s32 maxConnectedComponentSegments = u16_MAX;
      const s32 maxCandidateMarkers = 1000;
      const s32 maxExtractedQuads = 100;

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      // Stored in the outermost scratch2
      FixedLengthList<BlockMarker> candidateMarkers(maxCandidateMarkers, scratch2);
      ConnectedComponents extractedComponents; // This isn't allocated until after the scaleImage
      {
        PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack
        Array<u8> binaryImage(imageHeight, imageWidth, scratch2);

        // 1. Compute the Scale image (use local scratch1)
        // 2. Binarize the Scale image (store in outer scratch2)
        if(scaleAlgorithm == CHARACTERISTIC_SCALE_ORIGINAL) {
          PUSH_MEMORY_STACK(scratch1); // Push the current state of the scratch buffer onto the system stack

          FixedPointArray<u32> scaleImage(imageHeight, imageWidth, 16, scratch1);

          if(ComputeCharacteristicScaleImage(image, scaleImage_numPyramidLevels, scaleImage, scratch2) != RESULT_OK) {
            return RESULT_FAIL;
          }

          if(BinarizeScaleImage(image, scaleImage, binaryImage) != RESULT_OK) {
            return RESULT_FAIL;
          }

          // image.Show("image", false, false);
          // binaryImage.Show("binaryImage", true, true);
        } else if(scaleAlgorithm == CHARACTERISTIC_SCALE_MEDIUM_MEMORY) {
          if(ComputeCharacteristicScaleImageAndBinarize(image, scaleImage_numPyramidLevels, binaryImage, scaleImage_thresholdMultiplier, scratch1) != RESULT_OK)
            return RESULT_FAIL;
        }

        // 3. Compute connected components from the binary image (use local scratch2, store in outer scratch1)
        extractedComponents = ConnectedComponents(maxConnectedComponentSegments, imageWidth, scratch1);
        {
          PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

          if(extractedComponents.Extract2dComponents_FullImage(binaryImage, component1d_minComponentWidth, component1d_maxSkipDistance) != RESULT_OK)
            return RESULT_FAIL;

#ifdef SEND_DRAWN_COMPONENTS
          {
            Array<u8> drawnComponents(imageHeight, imageWidth, scratch1);
            DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);

            Matlab matlab(false);
            matlab.PutArray(drawnComponents, "drawnComponents0");
            //drawnComponents.Show("drawnComponents0", true, false);
          }
#endif

          extractedComponents.CompressConnectedComponentSegmentIds(scratch2);

          if(extractedComponents.InvalidateSmallOrLargeComponents(component_minimumNumPixels, component_maximumNumPixels, scratch2) != RESULT_OK)
            return RESULT_FAIL;

          extractedComponents.CompressConnectedComponentSegmentIds(scratch2);

          if(extractedComponents.InvalidateSolidOrSparseComponents(component_sparseMultiplyThreshold, component_solidMultiplyThreshold, scratch2) != RESULT_OK)
            return RESULT_FAIL;

          extractedComponents.CompressConnectedComponentSegmentIds(scratch2);

#ifdef SEND_DRAWN_COMPONENTS
          {
            Array<u8> drawnComponents(imageHeight, imageWidth, scratch1);
            DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);

            Matlab matlab(false);
            matlab.PutArray(drawnComponents, "drawnComponents1");
            //drawnComponents.Show("drawnComponents0", true, false);
          }
#endif

          // TODO: invalidate filled center components
          if(extractedComponents.InvalidateFilledCenterComponents(component_percentHorizontal, component_percentVertical, scratch2) != RESULT_OK)
            return RESULT_FAIL;

          extractedComponents.CompressConnectedComponentSegmentIds(scratch2);

          extractedComponents.SortConnectedComponentSegmentsById(scratch1);
        } // PUSH_MEMORY_STACK(scratch2);
      } // PUSH_MEMORY_STACK(scratch2);

#ifdef SEND_DRAWN_COMPONENTS
      {
        Array<u8> drawnComponents(imageHeight, imageWidth, scratch1);
        DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);

        Matlab matlab(false);
        matlab.PutArray(drawnComponents, "drawnComponents2");
        //drawnComponents.Show("drawnComponents0", true, false);
      }
#endif

      // 4. Compute candidate quadrilaterals from the connected components
      FixedLengthList<Quadrilateral<s16> > extractedQuads(maxExtractedQuads, scratch2);
      {
        PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

        if(ComputeQuadrilateralsFromConnectedComponents(extractedComponents, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, imageHeight, imageWidth, extractedQuads, scratch2) != RESULT_OK)
          return RESULT_FAIL;
      } // PUSH_MEMORY_STACK(scratch2);

      // 4b. Compute a homography for each extracted quadrilateral
      markers.set_size(extractedQuads.get_size());
      for(s32 iQuad=0; iQuad<extractedQuads.get_size(); iQuad++) {
        PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

        //BlockMarker &currentMarker = markers[iQuad];
        Array<f64> &currentHomography = homographies[iQuad];

        if(ComputeHomographyFromQuad(extractedQuads[iQuad], currentHomography, scratch2) != RESULT_OK)
          return RESULT_FAIL;

        markers[iQuad].blockType = -1;
        markers[iQuad].faceType = -1;
        for(s32 iCorner=0; iCorner<4; iCorner++) {
          markers[iQuad].corners[iCorner] = extractedQuads[iQuad].corners[iCorner];
        }
      } // for(iQuad=0; iQuad<; iQuad++)

      // 5. Decode fiducial markers from the candidate quadrilaterals
      // TODO: do decoding

      return RESULT_OK;
    } //  SimpleDetector_Steps1234()

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
      MemoryStack scratch2)
    {
      // BeginBenchmark("SimpleDetector_Steps12345");

      const s32 maxConnectedComponentSegments = 25000; // u16_MAX
      const s32 maxCandidateMarkers = 1000;
      const s32 maxExtractedQuads = 100;

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      // Stored in the outermost scratch2
      FixedLengthList<BlockMarker> candidateMarkers(maxCandidateMarkers, scratch2);
      ConnectedComponents extractedComponents; // This isn't allocated until after the scaleImage
      {
        PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack
        Array<u8> binaryImage(imageHeight, imageWidth, scratch2);

        // 1. Compute the Scale image (use local scratch1)
        // 2. Binarize the Scale image (store in outer scratch2)
        if(scaleAlgorithm == CHARACTERISTIC_SCALE_ORIGINAL) {
          PUSH_MEMORY_STACK(scratch1); // Push the current state of the scratch buffer onto the system stack

          FixedPointArray<u32> scaleImage(imageHeight, imageWidth, 16, scratch1);

          if(ComputeCharacteristicScaleImage(image, scaleImage_numPyramidLevels, scaleImage, scratch2) != RESULT_OK) {
            return RESULT_FAIL;
          }

          if(BinarizeScaleImage(image, scaleImage, binaryImage) != RESULT_OK) {
            return RESULT_FAIL;
          }

          // image.Show("image", false, false);
          // binaryImage.Show("binaryImage", true, true);
        } else if(scaleAlgorithm == CHARACTERISTIC_SCALE_MEDIUM_MEMORY) {
          if(ComputeCharacteristicScaleImageAndBinarize(image, scaleImage_numPyramidLevels, binaryImage, scaleImage_thresholdMultiplier, scratch1) != RESULT_OK)
            return RESULT_FAIL;
        }

        //{
        //  Matlab matlab(false);
        //  matlab.PutArray(binaryImage, "binaryImage");
        //}

        // Print a checksum of the binary image
#ifdef PRINTF_INTERMEDIATES
        const s32 binarySum = Sum(binaryImage);
        printf("Sum(binaryImage) = %d\n", binarySum);
        //binaryImage.Print("binaryImage");
#endif

        // 3. Compute connected components from the binary image (use local scratch2, store in outer scratch1)
        extractedComponents = ConnectedComponents(maxConnectedComponentSegments, imageWidth, scratch1);
        {
          PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

          // BeginBenchmark("Extract2dComponents");
          if(extractedComponents.Extract2dComponents_FullImage(binaryImage, component1d_minComponentWidth, component1d_maxSkipDistance) != RESULT_OK)
            return RESULT_FAIL;
          // EndBenchmark("Extract2dComponents");

#ifdef SEND_COMPONENT_USAGE
          {
            Matlab matlab(false);
            matlab.EvalStringEcho("if ~exist('maxComponents', 'var') maxComponents=[]; end; maxComponents(end+1) = %d;", extractedComponents.get_size());
          }
#endif

#ifdef SEND_DRAWN_COMPONENTS
          {
            Array<u8> drawnComponents(imageHeight, imageWidth, scratch1);
            DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);

            Matlab matlab(false);
            matlab.PutArray(drawnComponents, "drawnComponents0");
            //drawnComponents.Show("drawnComponents0", true, false);
          }
#endif

#ifdef PRINTF_INTERMEDIATES
          printf("MaxId(0) = %d\n", static_cast<s32>(extractedComponents.get_maximumId()));
#endif

          // BeginBenchmark("CompressConnectedComponentSegmentIds1");
          extractedComponents.CompressConnectedComponentSegmentIds(scratch2);
          // EndBenchmark("CompressConnectedComponentSegmentIds1");

#ifdef PRINTF_INTERMEDIATES
          printf("MaxId(1) = %d\n", static_cast<s32>(extractedComponents.get_maximumId()));
#endif

          // BeginBenchmark("InvalidateSmallOrLargeComponents");
          if(extractedComponents.InvalidateSmallOrLargeComponents(component_minimumNumPixels, component_maximumNumPixels, scratch2) != RESULT_OK)
            return RESULT_FAIL;
          // EndBenchmark("InvalidateSmallOrLargeComponents");

#ifdef PRINTF_INTERMEDIATES
          printf("MaxId(2) = %d\n", static_cast<s32>(extractedComponents.get_maximumId()));
#endif

          // BeginBenchmark("CompressConnectedComponentSegmentIds2");
          extractedComponents.CompressConnectedComponentSegmentIds(scratch2);
          // EndBenchmark("CompressConnectedComponentSegmentIds2");

#ifdef PRINTF_INTERMEDIATES
          printf("MaxId(3) = %d\n", static_cast<s32>(extractedComponents.get_maximumId()));
#endif

          // BeginBenchmark("InvalidateSolidOrSparseComponents");
          if(extractedComponents.InvalidateSolidOrSparseComponents(component_sparseMultiplyThreshold, component_solidMultiplyThreshold, scratch2) != RESULT_OK)
            return RESULT_FAIL;
          // EndBenchmark("InvalidateSolidOrSparseComponents");

#ifdef PRINTF_INTERMEDIATES
          printf("MaxId(4) = %d\n", static_cast<s32>(extractedComponents.get_maximumId()));
#endif

          // BeginBenchmark("CompressConnectedComponentSegmentIds3");
          extractedComponents.CompressConnectedComponentSegmentIds(scratch2);
          // EndBenchmark("CompressConnectedComponentSegmentIds3");

#ifdef PRINTF_INTERMEDIATES
          printf("MaxId(5) = %d\n", static_cast<s32>(extractedComponents.get_maximumId()));
#endif

#ifdef SEND_DRAWN_COMPONENTS
          {
            Array<u8> drawnComponents(imageHeight, imageWidth, scratch1);
            DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);

            Matlab matlab(false);
            matlab.PutArray(drawnComponents, "drawnComponents1");
            //drawnComponents.Show("drawnComponents0", true, false);
          }
#endif

          // BeginBenchmark("InvalidateFilledCenterComponents");
          if(extractedComponents.InvalidateFilledCenterComponents(component_percentHorizontal, component_percentVertical, scratch2) != RESULT_OK)
            return RESULT_FAIL;
          // EndBenchmark("InvalidateFilledCenterComponents");

#ifdef PRINTF_INTERMEDIATES
          printf("MaxId(6) = %d\n", static_cast<s32>(extractedComponents.get_maximumId()));
#endif

          // BeginBenchmark("CompressConnectedComponentSegmentIds4");
          extractedComponents.CompressConnectedComponentSegmentIds(scratch2);
          // EndBenchmark("CompressConnectedComponentSegmentIds4");

#ifdef PRINTF_INTERMEDIATES
          printf("MaxId(7) = %d\n", static_cast<s32>(extractedComponents.get_maximumId()));
#endif

          // BeginBenchmark("SortConnectedComponentSegmentsById");
          extractedComponents.SortConnectedComponentSegmentsById(scratch1);
          // EndBenchmark("SortConnectedComponentSegmentsById");
        } // PUSH_MEMORY_STACK(scratch2);
      } // PUSH_MEMORY_STACK(scratch2);

#ifdef SEND_DRAWN_COMPONENTS
      {
        Array<u8> drawnComponents(imageHeight, imageWidth, scratch1);
        DrawComponents<u8>(drawnComponents, extractedComponents, 64, 255);

        Matlab matlab(false);
        matlab.PutArray(drawnComponents, "drawnComponents2");
        //drawnComponents.Show("drawnComponents0", true, false);
      }
#endif

      // 4. Compute candidate quadrilaterals from the connected components
      // BeginBenchmark("ComputeQuadrilateralsFromConnectedComponents");
      FixedLengthList<Quadrilateral<s16> > extractedQuads(maxExtractedQuads, scratch2);
      {
        PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

        if(ComputeQuadrilateralsFromConnectedComponents(extractedComponents, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, imageHeight, imageWidth, extractedQuads, scratch2) != RESULT_OK)
          return RESULT_FAIL;
      } // PUSH_MEMORY_STACK(scratch2);
      // EndBenchmark("ComputeQuadrilateralsFromConnectedComponents");

#ifdef PRINTF_INTERMEDIATES
      extractedQuads.Print("extractedQuads");
#endif

      // 4b. Compute a homography for each extracted quadrilateral
      // BeginBenchmark("ComputeHomographyFromQuad");
      markers.set_size(extractedQuads.get_size());
      for(s32 iQuad=0; iQuad<extractedQuads.get_size(); iQuad++) {
        PUSH_MEMORY_STACK(scratch2); // Push the current state of the scratch buffer onto the system stack

        Array<f64> &currentHomography = homographies[iQuad];

        if(ComputeHomographyFromQuad(extractedQuads[iQuad], currentHomography, scratch2) != RESULT_OK)
          return RESULT_FAIL;

#ifdef PRINTF_INTERMEDIATES
        PrintfOneArray_f64(currentHomography, "currentHomography");
#endif
        //currentHomography.Print("currentHomography");
      } // for(iQuad=0; iQuad<; iQuad++)
      // EndBenchmark("ComputeHomographyFromQuad");

      // 5. Decode fiducial markers from the candidate quadrilaterals
      const FiducialMarkerParser parser = FiducialMarkerParser();

      // BeginBenchmark("ExtractBlockMarker");
      for(s32 iQuad=0; iQuad<extractedQuads.get_size(); iQuad++) {
        const Array<f64> &currentHomography = homographies[iQuad];
        const Quadrilateral<s16> &currentQuad = extractedQuads[iQuad];
        BlockMarker &currentMarker = markers[iQuad];

        if(parser.ExtractBlockMarker(image, currentQuad, currentHomography, decode_minContrastRatio, currentMarker, scratch2) != RESULT_OK)
          return RESULT_FAIL;
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
      // EndBenchmark("ExtractBlockMarker");

      // EndBenchmark("SimpleDetector_Steps12345");

      return RESULT_OK;
    } //  SimpleDetector_Steps12345()

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
      MemoryStack scratch1,
      MemoryStack scratch2)
    {
      BeginBenchmark("SimpleDetector_Steps12345_lowMemory");

      const s32 maxConnectedComponentSegments = 25000; // u16_MAX
      const s32 maxExtractedQuads = 1000;

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      BeginBenchmark("ComputeCharacteristicScaleImageAndBinarizeAndExtractComponents");
      ConnectedComponents extractedComponents = ConnectedComponents(maxConnectedComponentSegments, imageWidth, scratch2);
      {
        PUSH_MEMORY_STACK(scratch1); // Push the current state of the scratch buffer onto the system stack

        // 1. Compute the Scale image (use local scratch1)
        // 2. Binarize the Scale image (store in outer scratch2)
        // 3. Compute connected components from the binary image (use local scratch2, store in outer scratch1)
        if(ComputeCharacteristicScaleImageAndBinarizeAndExtractComponents(
          image,
          scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier,
          component1d_minComponentWidth, component1d_maxSkipDistance,
          extractedComponents,
          scratch1) != RESULT_OK)

          return RESULT_FAIL;
      }
      EndBenchmark("ComputeCharacteristicScaleImageAndBinarizeAndExtractComponents");

      // 3b. Remove poor components
      {
        PUSH_MEMORY_STACK(scratch1); // Push the current state of the scratch buffer onto the system stack

        BeginBenchmark("CompressConnectedComponentSegmentIds1");
        extractedComponents.CompressConnectedComponentSegmentIds(scratch1);
        EndBenchmark("CompressConnectedComponentSegmentIds1");

        BeginBenchmark("InvalidateSmallOrLargeComponents");
        if(extractedComponents.InvalidateSmallOrLargeComponents(component_minimumNumPixels, component_maximumNumPixels, scratch1) != RESULT_OK)
          return RESULT_FAIL;
        EndBenchmark("InvalidateSmallOrLargeComponents");

        BeginBenchmark("CompressConnectedComponentSegmentIds2");
        extractedComponents.CompressConnectedComponentSegmentIds(scratch1);
        EndBenchmark("CompressConnectedComponentSegmentIds2");

        BeginBenchmark("InvalidateSolidOrSparseComponents");
        if(extractedComponents.InvalidateSolidOrSparseComponents(component_sparseMultiplyThreshold, component_solidMultiplyThreshold, scratch1) != RESULT_OK)
          return RESULT_FAIL;
        EndBenchmark("InvalidateSolidOrSparseComponents");

        BeginBenchmark("CompressConnectedComponentSegmentIds3");
        extractedComponents.CompressConnectedComponentSegmentIds(scratch1);
        EndBenchmark("CompressConnectedComponentSegmentIds3");

        BeginBenchmark("InvalidateFilledCenterComponents");
        if(extractedComponents.InvalidateFilledCenterComponents(component_percentHorizontal, component_percentVertical, scratch1) != RESULT_OK)
          return RESULT_FAIL;
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

      if(ComputeQuadrilateralsFromConnectedComponents(extractedComponents, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, imageHeight, imageWidth, extractedQuads, scratch1) != RESULT_OK)
        return RESULT_FAIL;

      EndBenchmark("ComputeQuadrilateralsFromConnectedComponents");

      // 4b. Compute a homography for each extracted quadrilateral
      BeginBenchmark("ComputeHomographyFromQuad");
      markers.set_size(extractedQuads.get_size());
      for(s32 iQuad=0; iQuad<extractedQuads.get_size(); iQuad++) {
        Array<f64> &currentHomography = homographies[iQuad];

        if(ComputeHomographyFromQuad(extractedQuads[iQuad], currentHomography, scratch1) != RESULT_OK)
          return RESULT_FAIL;

        //currentHomography.Print("currentHomography");
      } // for(iQuad=0; iQuad<; iQuad++)
      EndBenchmark("ComputeHomographyFromQuad");

      // 5. Decode fiducial markers from the candidate quadrilaterals
      const FiducialMarkerParser parser = FiducialMarkerParser();

      BeginBenchmark("ExtractBlockMarker");
      for(s32 iQuad=0; iQuad<extractedQuads.get_size(); iQuad++) {
        const Array<f64> &currentHomography = homographies[iQuad];
        const Quadrilateral<s16> &currentQuad = extractedQuads[iQuad];
        BlockMarker &currentMarker = markers[iQuad];

        if(parser.ExtractBlockMarker(image, currentQuad, currentHomography, decode_minContrastRatio, currentMarker, scratch1) != RESULT_OK)
          return RESULT_FAIL;
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

      EndBenchmark("SimpleDetector_Steps12345_lowMemory");

      return RESULT_OK;
    } //  SimpleDetector_Steps12345_lowMemory()
  } // namespace Embedded
} // namespace Anki