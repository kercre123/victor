/**
File: detectFiducialMarkers.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/benchmarking.h"

#include "coretech/vision/robot/fiducialMarkers.h"
#include "coretech/vision/robot/fiducialDetection.h"
#include "coretech/vision/robot/draw_vision.h"
#include "coretech/vision/robot/transformations.h"

#include "coretech/common/robot/matlabInterface.h"

#if RECOGNITION_METHOD == RECOGNITION_METHOD_NEAREST_NEIGHBOR
#  define NEAREST_NEIGHBOR_DISTANCE_THRESHOLD 50 // TODO: Make this a VisionParameter and pass it in dynamically
#endif

//#define SHOW_DRAWN_COMPONENTS // Requires vision system to be in synchronous mode
//#define SEND_COMPONENTS_TO_MATLAB

namespace Anki
{
  namespace Embedded
  {
    
    static Result IlluminationNormalization(const Quadrilateral<f32>& corners,
                                            const Point<f32>& fiducialThicknessFraction,
                                            cv::Mat_<u8>& cvImage, // filters this in place
                                            cv::Mat_<u8>& cvImageROI, // returns the ROI for the corners
                                            cv::Mat_<u8>& cvImageROI_orig)  // stores a copy of the original data inside the ROI
    {
      // Get a slightly-dilated bounding box of the current marker's corners,
      // and store as an OpenCV rectangle
      Rectangle<s32> roi = corners.ComputeBoundingRectangle<s32>();
      cv::Rect cvRoi;
      
      roi.left   = MAX(0, roi.left-5);
      roi.top    = MAX(0, roi.top-5);
      roi.bottom = MIN(cvImage.rows-1, roi.bottom+5);
      roi.right  = MIN(cvImage.cols-1, roi.right+5);
      
      cvRoi.x = roi.left;
      cvRoi.y = roi.top;
      cvRoi.width = roi.get_width();
      cvRoi.height = roi.get_height();
      
      // Extract the ROI for processing
      cvImageROI = cvImage(cvRoi);
      AnkiConditionalErrorAndReturnValue(cvImageROI.empty() == false, RESULT_FAIL_INVALID_SIZE,
                                         "IlluminationNormalization", "Got empty ROI for given corners");
      
      // Hang on to a copy of the original pixel values so we can restore them
      // after filtering, to avoid screwing up the supposedly-const input image
      // for later processing
      cvImageROI.copyTo(cvImageROI_orig);
      
      // Filter with a kernel sized for this ROI:
      // (The kernel is a bunch of -1's surrounding a single positive value
      //  at the center equal to the sum of all the -1's)
      // NOTE: it's a tad slower to use s16, but we seem to get better performance with it
      cv::Mat_<s16> cvImageROI_filtered;
      
      /*
      // Kernel size should be half the size of the image inside the fiducial
      // since this filtering should also match the illumination normalization
      // done to the nearest neighbor library.
      //  So we take the average diagonal of the quad (0.5 times the sum of the
      //  two diagonals), divide that by sqrt(2), and then
      //  use 0.6 of that because that's the size of the marker image, and then we
      //  use half of that. I.e., 0.5*sqrt(2)*0.6*0.5 = .1061
      const s32 kernelSize = 0.1061f*((corners[Quadrilateral<f32>::TopLeft] - corners[Quadrilateral<f32>::BottomRight]).Length() +
                                      (corners[Quadrilateral<f32>::TopRight] - corners[Quadrilateral<f32>::BottomLeft]).Length());
      */

      // Kernel size should be twice the thickness of the fiducial. We use the average
      // diagonal/sqrt(2) to estimate the fiducial width, and since the thickness is
      // 10% of the width, we use 0.2/sqrt(2)=0.14 as the multiplier.
      // This kernel size assumes we RE-filter the extracted probe values inside the nearest
      // neighbor code.
      const f32 avgThicknessFraction = 0.5f*(fiducialThicknessFraction.x + fiducialThicknessFraction.y);
      const f32 kernelSize = std::round(1.4142f*avgThicknessFraction *
                                        ((corners[Quadrilateral<f32>::TopLeft] - corners[Quadrilateral<f32>::BottomRight]).Length() +
                                         (corners[Quadrilateral<f32>::TopRight] - corners[Quadrilateral<f32>::BottomLeft]).Length()));
      
      cv::boxFilter(cvImageROI, cvImageROI_filtered, cvImageROI_filtered.depth(), cv::Size(kernelSize, kernelSize));
      
      // Works better to maintain s16 "precision" here and then only to lose it when we
      // normalize below
      cv::subtract(cvImageROI, cvImageROI_filtered, cvImageROI_filtered);
      
      // Normalize to be between 0 and 255
      cv::normalize(cvImageROI_filtered, cvImageROI, 255, 0, CV_MINMAX);
      
      //cv::imshow("Normalized Filtered ROI", cvImageROI);
      
      return RESULT_OK;
    } // IlluminationNormalization()
    

    
    Result DetectFiducialMarkers(const Array<u8> &image,
                                 FixedLengthList<VisionMarker> &markers,
                                 const FiducialDetectionParameters &params,
                                 const bool isDarkOnLight,
                                 MemoryStack scratchCcm,
                                 MemoryStack scratchOnchip,
                                 MemoryStack scratchOffChip)
    {
      AnkiAssert(params.fiducialThicknessFraction.x > 0 && params.fiducialThicknessFraction.y > 0);
      static const f32 maxProjectiveTermValue = 8.0f;

      Result lastResult;

      BeginBenchmark("DetectFiducialMarkers");

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      AnkiConditionalErrorAndReturnValue(image.IsValid() && markers.IsValid() && scratchOffChip.IsValid() && scratchOnchip.IsValid() && scratchCcm.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "DetectFiducialMarkers", "Some input is invalid");

      // On the robot, we don't have enough memory for resolutions over QVGA
      if(scratchOffChip.get_totalBytes() < 1000000 && scratchOnchip.get_totalBytes() < 1000000 && scratchCcm.get_totalBytes() < 1000000) {
        AnkiConditionalErrorAndReturnValue(imageHeight <= 240 && imageWidth <= 320,
          RESULT_FAIL_INVALID_SIZE, "DetectFiducialMarkers", "The image is too large to test");

        AnkiConditionalErrorAndReturnValue(params.scaleImage_numPyramidLevels <= 3,
          RESULT_FAIL_INVALID_SIZE, "DetectFiducialMarkers", "Only 3 pyramid levels are supported");
      }

      BeginBenchmark("ExtractComponentsViaCharacteristicScale");

      ConnectedComponents extractedComponents;
      if(params.maxConnectedComponentSegments <= std::numeric_limits<u16>::max()) {
        extractedComponents = ConnectedComponents(params.maxConnectedComponentSegments, imageWidth, true, scratchOffChip);
      } else {
        extractedComponents = ConnectedComponents(params.maxConnectedComponentSegments, imageWidth, false, scratchOffChip);
      }

      AnkiConditionalErrorAndReturnValue(extractedComponents.IsValid(),
        RESULT_FAIL_OUT_OF_MEMORY, "DetectFiducialMarkers", "extractedComponents could not be allocated");

      if(params.useIntegralImageFiltering) {
        FixedLengthList<s32> filterHalfWidths(params.scaleImage_numPyramidLevels+2, scratchOnchip, Flags::Buffer(false, false, true));

        AnkiConditionalErrorAndReturnValue(filterHalfWidths.IsValid(),
          RESULT_FAIL_OUT_OF_MEMORY, "DetectFiducialMarkers", "filterHalfWidths could not be allocated");

        for(s32 i=0; i<(params.scaleImage_numPyramidLevels+2); i++) {
          filterHalfWidths[i] = params.imagePyramid_baseScale << (i);
        }

        //const s32 halfWidthData[] = {1,2,3,4,6,8,12,16};
        //for(s32 i=0; i<8; i++) {
        //  filterHalfWidths[i] = halfWidthData[i];
        //}

        // 1. Compute the Scale image
        // 2. Binarize the Scale image
        // 3. Compute connected components from the binary image
        if((lastResult = ExtractComponentsViaCharacteristicScale(
          image,
          filterHalfWidths, params.scaleImage_thresholdMultiplier,
          params.component1d_minComponentWidth, params.component1d_maxSkipDistance, isDarkOnLight,
          extractedComponents,
          scratchCcm, scratchOnchip, scratchOffChip)) != RESULT_OK)
        {
          /* // DEBUG: drop a display of extracted components into matlab
          Embedded::Matlab matlab(false);
          matlab.PutArray(image, "image");
          Array<u8> empty(image.get_size(0), image.get_size(1), scratchOnchip);
          Embedded::DrawComponents<u8>(empty, extractedComponents, 64, 255);
          matlab.PutArray(empty, "empty");
          matlab.EvalStringEcho("desktop; keyboard");
          */
          return lastResult;
        }
      } else { // if(useIntegralImageFiltering)
        if((lastResult = ExtractComponentsViaCharacteristicScale_binomial(
          image,
          params.scaleImage_numPyramidLevels, params.scaleImage_thresholdMultiplier,
          params.component1d_minComponentWidth, params.component1d_maxSkipDistance,
          extractedComponents,
          scratchCcm, scratchOnchip, scratchOffChip)) != RESULT_OK)
        {
          /* // DEBUG: drop a display of extracted components into matlab
          Embedded::Matlab matlab(false);
          matlab.PutArray(image, "image");
          Array<u8> empty(image.get_size(0), image.get_size(1), scratchOnchip);
          Embedded::DrawComponents<u8>(empty, extractedComponents, 64, 255);
          matlab.PutArray(empty, "empty");
          matlab.EvalStringEcho("desktop; keyboard");
          */
          return lastResult;
        }
      } // if(useIntegralImageFiltering) ... else

#ifdef SHOW_DRAWN_COMPONENTS
      {
        const s32 bigScratchSize = 1024 + image.get_size(0) * RoundUp<s32>(image.get_size(1), MEMORY_ALIGNMENT);
        MemoryStack bigScratch(malloc(bigScratchSize), bigScratchSize);
        Array<u8> empty(image.get_size(0), image.get_size(1), bigScratch);
        Embedded::DrawComponents<u8>(empty, extractedComponents, 64, 255);
        image.Show("image", false, false, true);
        empty.Show("components orig", false, false, true);
        free(bigScratch.get_buffer());
      }
#endif

      EndBenchmark("ExtractComponentsViaCharacteristicScale");

      { // 3b. Remove poor components
        BeginBenchmark("CompressConnectedComponentSegmentIds1");
        extractedComponents.CompressConnectedComponentSegmentIds(scratchOnchip);
        EndBenchmark("CompressConnectedComponentSegmentIds1");

        BeginBenchmark("InvalidateSmallOrLargeComponents");
        if((lastResult = extractedComponents.InvalidateSmallOrLargeComponents(params.component_minimumNumPixels, params.component_maximumNumPixels, scratchOnchip)) != RESULT_OK)
          return lastResult;
        EndBenchmark("InvalidateSmallOrLargeComponents");

        BeginBenchmark("CompressConnectedComponentSegmentIds2");
        extractedComponents.CompressConnectedComponentSegmentIds(scratchOnchip);
        EndBenchmark("CompressConnectedComponentSegmentIds2");

        BeginBenchmark("InvalidateSolidOrSparseComponents");
        if((lastResult = extractedComponents.InvalidateSolidOrSparseComponents(params.component_sparseMultiplyThreshold, params.component_solidMultiplyThreshold, scratchOnchip)) != RESULT_OK)
          return lastResult;
        EndBenchmark("InvalidateSolidOrSparseComponents");

        BeginBenchmark("CompressConnectedComponentSegmentIds3");
        extractedComponents.CompressConnectedComponentSegmentIds(scratchOnchip);
        EndBenchmark("CompressConnectedComponentSegmentIds3");

        BeginBenchmark("InvalidateFilledCenterComponents_hollowRows");
        if((lastResult = extractedComponents.InvalidateFilledCenterComponents_hollowRows(params.component_minHollowRatio, scratchOnchip)) != RESULT_OK)
          return lastResult;
        EndBenchmark("InvalidateFilledCenterComponents_hollowRows");

        BeginBenchmark("CompressConnectedComponentSegmentIds4");
        extractedComponents.CompressConnectedComponentSegmentIds(scratchOnchip);
        EndBenchmark("CompressConnectedComponentSegmentIds4");

        BeginBenchmark("SortConnectedComponentSegmentsById");
        extractedComponents.SortConnectedComponentSegmentsById(scratchOnchip);
        EndBenchmark("SortConnectedComponentSegmentsById");
      } // 3b. Remove poor components

#ifdef SHOW_DRAWN_COMPONENTS
      {
        //CoreTechPrint("Components\n");
        const s32 bigScratchSize = 1024 + image.get_size(0) * RoundUp<s32>(image.get_size(1), MEMORY_ALIGNMENT);
        MemoryStack bigScratch(malloc(bigScratchSize), bigScratchSize);
        Array<u8> empty(image.get_size(0), image.get_size(1), bigScratch);
        Embedded::DrawComponents<u8>(empty, extractedComponents, 64, 255);
        empty.Show("components good", false, false, true);
        free(bigScratch.get_buffer());
      }
#endif

#ifdef SEND_COMPONENTS_TO_MATLAB
      {
        Anki::Embedded::Matlab matlab(false);

        const s32 numComponents = extractedComponents.get_size();

        std::shared_ptr<s16> xStart(new s16[numComponents]);
        std::shared_ptr<s16> xEnd(new s16[numComponents]);
        std::shared_ptr<s16> y(new s16[numComponents]);

        s16 * restrict pXStart = xStart.get();
        s16 * restrict pXEnd = xEnd.get();
        s16 * restrict pY = y.get();

        if(maxConnectedComponentSegments <= std::numeric_limits<u16>::max()) {
          const ConnectedComponentSegment<u16> * restrict pComponents = extractedComponents.get_componentsU16()->Pointer(0);

          std::shared_ptr<u16> id(new u16[numComponents]);
          
          u16 * restrict pId = id.get();

          for(s32 i=0; i<numComponents; i++) {
            pXStart[i] = pComponents[i].xStart;
            pXEnd[i] = pComponents[i].xEnd;
            pY[i] = pComponents[i].y;
            pId[i] = pComponents[i].id;
          }

          matlab.Put<s16>(pXStart, numComponents, "xStart");
          matlab.Put<s16>(pXEnd, numComponents, "xEnd");
          matlab.Put<s16>(pY, numComponents, "y");
          matlab.Put<u16>(pId, numComponents, "id");
        } else {
          //TODO: implement
        }      
      }
#endif

      // 4. Compute candidate quadrilaterals from the connected components
      {
        BeginBenchmark("ComputeQuadrilateralsFromConnectedComponents");
        FixedLengthList<Quadrilateral<s16> > extractedQuads(params.maxExtractedQuads, scratchOnchip);

        if((lastResult = ComputeQuadrilateralsFromConnectedComponents(extractedComponents, params.quads_minQuadArea, params.quads_quadSymmetryThreshold, params.quads_minDistanceFromImageEdge, params.minLaplacianPeakRatio, imageHeight, imageWidth, params.cornerMethod, extractedQuads, scratchOnchip)) != RESULT_OK)
          return lastResult;

        markers.set_size(extractedQuads.get_size());

        //printf("quads %d\n", extractedQuads.get_size());

        EndBenchmark("ComputeQuadrilateralsFromConnectedComponents");

        // 4b. Compute a homography for each extracted quadrilateral
        BeginBenchmark("ComputeHomographyFromQuad");
        for(s32 iQuad=0; iQuad<extractedQuads.get_size(); iQuad++) {
          Array<f32> &currentHomography = markers[iQuad].homography;
          VisionMarker &currentMarker = markers[iQuad];

          bool numericalFailure;
          if((lastResult = Transformations::ComputeHomographyFromQuad(extractedQuads[iQuad], currentHomography, numericalFailure, scratchOnchip)) != RESULT_OK) {
            return lastResult;
          }

          currentMarker.validity = VisionMarker::UNKNOWN;
          currentMarker.corners.SetCast(extractedQuads[iQuad]);

          if(numericalFailure) {
            currentMarker.validity = VisionMarker::NUMERICAL_FAILURE;
          } else {
            if(currentHomography[2][0] > maxProjectiveTermValue || currentHomography[2][1] > maxProjectiveTermValue) {
              AnkiWarn("DetectFiducialMarkers", "Homography projective terms are unreasonably large");
              currentMarker.validity = VisionMarker::NUMERICAL_FAILURE;
            }
          }

          //currentHomography.Print("currentHomography");
        } // for(iQuad=0; iQuad<; iQuad++)
        EndBenchmark("ComputeHomographyFromQuad");
      }

      // 5. Decode fiducial markers from the candidate quadrilaterals

      BeginBenchmark("ExtractVisionMarker");

      // refinedHomography and meanGrayvalueThreshold are computed by currentMarker.RefineCorners(), then used by currentMarker.Extract()
      Array<f32> refinedHomography(3, 3, scratchOnchip);
      u8 meanGrayvalueThreshold;

      // Wrap a cv::Mat around the image data
      // NOTE: This will permit us to modify the image, despite the input
      //  Array2d obect being a const reference!!!
      cv::Mat_<u8> cvImage;
      ArrayToCvMat(image, &cvImage);
      
      for(s32 iMarker=0; iMarker<markers.get_size(); iMarker++)
      {
        VisionMarker &currentMarker = markers[iMarker];
        
        cv::Mat_<u8> cvImageROI, cvImageROI_orig;
        if(params.useIlluminationNormalization)
        {
          lastResult = IlluminationNormalization(currentMarker.corners, params.fiducialThicknessFraction, cvImage, cvImageROI, cvImageROI_orig);
          if(lastResult != RESULT_OK) {
            AnkiWarn("DetectFiducialMarkers", "Illumination normalization failed, skipping marker %d", iMarker);
            continue;
          }
        }
        
        if(currentMarker.validity == VisionMarker::UNKNOWN) {
          // If refine_quadRefinementIterations > 0, then make this marker's corners more accurate
          lastResult = currentMarker.RefineCorners(image,
                                                   params.decode_minContrastRatio,
                                                   params.refine_quadRefinementIterations,
                                                   params.refine_numRefinementSamples,
                                                   params.refine_quadRefinementMaxCornerChange,
                                                   params.refine_quadRefinementMinCornerChange,
                                                   params.quads_minQuadArea,
                                                   params.quads_quadSymmetryThreshold,
                                                   params.quads_minDistanceFromImageEdge,
                                                   params.fiducialThicknessFraction,
                                                   params.roundedCornersFraction,
                                                   isDarkOnLight,
                                                   meanGrayvalueThreshold,
                                                   scratchOnchip);

          if(params.useIlluminationNormalization)
          {
            // Put back the original (non-illumination-normalized) pixel values within the ROI
            cvImageROI_orig.copyTo(cvImageROI);
          }
          
          if(lastResult == RESULT_OK) {
            // Refinement succeeded...
            if(currentMarker.validity == VisionMarker::LOW_CONTRAST) {
              // ... but there wasn't enough contrast to use the marker
              currentMarker.markerType = Anki::Vision::MARKER_UNKNOWN;
            } else if(params.doCodeExtraction) {
              // ... and there was enough contrast, so proceed with with decoding
              // the marker
              lastResult = currentMarker.Extract(image,
#if RECOGNITION_METHOD == RECOGNITION_METHOD_NEAREST_NEIGHBOR
                                                 NEAREST_NEIGHBOR_DISTANCE_THRESHOLD,
#                                                else
                                                 meanGrayvalueThreshold,
#                                                endif
                                                 params.decode_minContrastRatio,
                                                 scratchOnchip);
              
              AnkiConditionalWarn(lastResult == RESULT_OK, "DetectFiducialMarkers",
                                  "Marker extraction for quad %d of %d failed",
                                  iMarker, markers.get_size());

              if (currentMarker.markerType == Vision::MARKER_INVALID && !params.returnInvalidMarkers) {
                currentMarker.validity = VisionMarker::UNKNOWN;
              }
              
            } else {
              currentMarker.markerType = Vision::MARKER_UNKNOWN;
              currentMarker.validity = VisionMarker::VALID_BUT_NOT_DECODED;
            }
          } else {
            currentMarker.validity = VisionMarker::REFINEMENT_FAILURE;
            currentMarker.markerType = Anki::Vision::MARKER_UNKNOWN;
          }
        } else { // if(currentMarker.validity == VisionMarker::UNKNOWN)
          // Put back the original (non-illumination-normalized) pixel values within the ROI
          cvImageROI_orig.copyTo(cvImageROI);
        }

        
      } // for(s32 iMarker=0; iMarker<markers.get_size(); iMarker++)

      // Remove invalid markers from the list
      if(!params.returnInvalidMarkers) {
        for(s32 iMarker=0; iMarker<markers.get_size(); iMarker++)
        {
          if(markers[iMarker].validity != VisionMarker::VALID &&
             markers[iMarker].validity != VisionMarker::VALID_BUT_NOT_DECODED)
          {
            for(s32 jQuad=iMarker; jQuad<markers.get_size(); jQuad++) {
              markers[jQuad] = markers[jQuad+1];
            }
            //extractedQuads.set_size(extractedQuads.get_size()-1);
            markers.set_size(markers.get_size()-1);
            iMarker--;
          }
        }
      } // if(!returnInvalidMarkers)
      
      EndBenchmark("ExtractVisionMarker");

      EndBenchmark("DetectFiducialMarkers");

      return RESULT_OK;
    } // DetectFiducialMarkers()
  } // namespace Embedded
} // namespace Anki
