/**
File: binaryTracker.cpp
Author: Peter Barnum
Created: 2014-02-21

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/binaryTracker.h"
#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/interpolate.h"
#include "anki/common/robot/arrayPatterns.h"
#include "anki/common/robot/find.h"
#include "anki/common/robot/benchmarking_c.h"
#include "anki/common/robot/draw.h"
#include "anki/common/robot/comparisons.h"

#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/imageProcessing.h"
#include "anki/vision/robot/transformations.h"

//#define SEND_BINARY_IMAGES_TO_MATLAB

#define USE_ARM_ACCELERATION

#if defined(__EDG__)
#ifndef USE_ARM_ACCELERATION
#warning not using USE_ARM_ACCELERATION
#endif
#else
#undef USE_ARM_ACCELERATION
#endif

#if defined(USE_ARM_ACCELERATION)
#include <ARMCM4.h>
//#include <core_cmInstr.h>
//#include <core_cmFunc.h>
//#include <core_cm4_simd.h>
#endif

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
      BinaryTracker::BinaryTracker()
        : isValid(false)
      {
      }

      BinaryTracker::BinaryTracker(
        const Array<u8> &templateImage, const Quadrilateral<f32> &templateQuad,
        const u8 edgeDetection_grayvalueThreshold, const s32 edgeDetection_minComponentWidth, const s32 edgeDetection_maxDetectionsPerType,
        MemoryStack &memory)
        : isValid(false)
      {
        const s32 templateImageHeight = templateImage.get_size(0);
        const s32 templateImageWidth = templateImage.get_size(1);

        AnkiConditionalErrorAndReturn(templateImageHeight > 0 && templateImageWidth > 0,
          "BinaryTracker::BinaryTracker", "template widths and heights must be greater than zero");

        AnkiConditionalErrorAndReturn(templateImage.IsValid(),
          "BinaryTracker::BinaryTracker", "templateImage is not valid");

        Point<f32> centerOffset((templateImage.get_size(1)-1) / 2.0f, (templateImage.get_size(0)-1) / 2.0f);
        this->transformation = Transformations::PlanarTransformation_f32(Transformations::TRANSFORM_PROJECTIVE, templateQuad, centerOffset, memory);

        //this->templateImage = Array<u8>(templateImageHeight, templateImageWidth, memory);
        this->templateQuad = templateQuad;

        this->templateImageHeight = templateImage.get_size(0);
        this->templateImageWidth = templateImage.get_size(1);

        this->templateEdges.xDecreasing = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, memory);
        this->templateEdges.xIncreasing = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, memory);
        this->templateEdges.yDecreasing = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, memory);
        this->templateEdges.yIncreasing = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, memory);

        AnkiConditionalErrorAndReturn(
          this->templateEdges.xDecreasing.IsValid() && this->templateEdges.xIncreasing.IsValid() &&
          this->templateEdges.yDecreasing.IsValid() && this->templateEdges.yIncreasing.IsValid(),
          "BinaryTracker::BinaryTracker", "Could not allocate local memory");

        const Rectangle<f32> templateRectRaw = templateQuad.ComputeBoundingRectangle();
        const Rectangle<s32> templateRect(static_cast<s32>(templateRectRaw.left), static_cast<s32>(templateRectRaw.right), static_cast<s32>(templateRectRaw.top), static_cast<s32>(templateRectRaw.bottom));

        const Result result = DetectBlurredEdges(templateImage, templateRect, edgeDetection_grayvalueThreshold, edgeDetection_minComponentWidth, this->templateEdges);

        AnkiConditionalErrorAndReturn(result == RESULT_OK,
          "BinaryTracker::BinaryTracker", "DetectBlurredEdge failed");

#ifdef SEND_BINARY_IMAGES_TO_MATLAB
        {
          MemoryStack matlabScratch(malloc(1000000),1000000);

          cv::Mat drawn = DrawIndexes(this->templateImageHeight, this->templateImageWidth, this->templateEdges.xDecreasing, this->templateEdges.xIncreasing, this->templateEdges.yDecreasing, this->templateEdges.yIncreasing);

          cv::imshow("binary template", drawn);
          cv::waitKey();

          Matlab matlab(false);

          Array<u8> rendered(templateImageHeight, templateImageWidth, matlabScratch);

          rendered.SetZero();
          DrawPoints(this->templateEdges.xDecreasing, 1, rendered);
          matlab.PutArray(rendered, "templateEdges.xDecreasing");

          rendered.SetZero();
          DrawPoints(this->templateEdges.xIncreasing, 1, rendered);
          matlab.PutArray(rendered, "templateEdges.xIncreasing");

          rendered.SetZero();
          DrawPoints(this->templateEdges.yDecreasing, 1, rendered);
          matlab.PutArray(rendered, "templateEdges.yDecreasing");

          rendered.SetZero();
          DrawPoints(this->templateEdges.yIncreasing, 1, rendered);
          matlab.PutArray(rendered, "templateEdges.yIncreasing");

          free(matlabScratch.get_buffer());
        }
#endif // #ifdef SEND_BINARY_IMAGES_TO_MATLAB

        this->homographyOffsetX = (static_cast<f32>(templateImageWidth)-1.0f) / 2.0f;
        this->homographyOffsetY = (static_cast<f32>(templateImageHeight)-1.0f) / 2.0f;

        this->grid = Meshgrid<f32>(
          Linspace(-homographyOffsetX, homographyOffsetX, static_cast<s32>(FLT_FLOOR(templateImageWidth))),
          Linspace(-homographyOffsetY, homographyOffsetY, static_cast<s32>(FLT_FLOOR(templateImageHeight))));

        this->xGrid = this->grid.get_xGridVector().Evaluate(memory);
        this->yGrid = this->grid.get_yGridVector().Evaluate(memory);

        this->isValid = true;
      }

      Result BinaryTracker::ShowTemplate(const bool waitForKeypress, const bool fitImageToWindow) const
      {
#ifndef ANKICORETECH_EMBEDDED_USE_OPENCV
        return RESULT_FAIL;
#else
        if(!this->IsValid())
          return RESULT_FAIL;

        const char * windowName = "BinaryTracker Template";

        cv::Mat toShow = BinaryTracker::DrawIndexes(
          templateImageHeight, templateImageWidth,
          templateEdges.xDecreasing, templateEdges.xIncreasing, templateEdges.yDecreasing, templateEdges.yIncreasing);

        if(fitImageToWindow) {
          cv::namedWindow(windowName, CV_WINDOW_NORMAL);
        } else {
          cv::namedWindow(windowName, CV_WINDOW_AUTOSIZE);
        }

        cv::imshow(windowName, toShow);

        if(waitForKeypress)
          cv::waitKey();

        return RESULT_OK;
#endif
      }

#ifdef ANKICORETECH_EMBEDDED_USE_OPENCV
      // Allocates the returned cv::Mat on the heap
      cv::Mat BinaryTracker::DrawIndexes(
        const s32 imageHeight, const s32 imageWidth,
        const FixedLengthList<Point<s16> > &indexPoints1,
        const FixedLengthList<Point<s16> > &indexPoints2,
        const FixedLengthList<Point<s16> > &indexPoints3,
        const FixedLengthList<Point<s16> > &indexPoints4)
      {
        const u8 colors[4][3] = {
          {128,0,0},
          {0,128,0},
          {0,0,128},
          {128,128,0}};

        const s32 scratchSize = 10000000;
        MemoryStack scratch(malloc(scratchSize), scratchSize);

        Array<u8> image1(imageHeight, imageWidth, scratch);
        Array<u8> image2(imageHeight, imageWidth, scratch);
        Array<u8> image3(imageHeight, imageWidth, scratch);
        Array<u8> image4(imageHeight, imageWidth, scratch);

        DrawPoints(indexPoints1, 1, image1);
        DrawPoints(indexPoints2, 2, image2);
        DrawPoints(indexPoints3, 3, image3);
        DrawPoints(indexPoints4, 4, image4);

        cv::Mat totalImage(imageHeight, imageWidth, CV_8UC3);
        totalImage.setTo(0);

        for(s32 y=0; y<imageHeight; y++) {
          for(s32 x=0; x<imageWidth; x++) {
            u8* pTotalImage = totalImage.ptr<u8>(y,x);

            if(image1[y][x] != 0) {
              for(s32 c=0; c<3; c++) {
                pTotalImage[c] += colors[0][c];
              }
            }

            if(image2[y][x] != 0) {
              for(s32 c=0; c<3; c++) {
                pTotalImage[c] += colors[1][c];
              }
            }

            if(image3[y][x] != 0) {
              for(s32 c=0; c<3; c++) {
                pTotalImage[c] += colors[2][c];
              }
            }

            if(image4[y][x] != 0) {
              for(s32 c=0; c<3; c++) {
                pTotalImage[c] += colors[3][c];
              }
            }
          }
        }

        free(scratch.get_buffer());

        return totalImage;
      }

#endif

      bool BinaryTracker::IsValid() const
      {
        if(!this->isValid)
          return false;

        if(!templateEdges.xDecreasing.IsValid())
          return false;

        if(!templateEdges.xIncreasing.IsValid())
          return false;

        if(!templateEdges.yDecreasing.IsValid())
          return false;

        if(!templateEdges.yIncreasing.IsValid())
          return false;

        return true;
      }

      Result BinaryTracker::set_transformation(const Transformations::PlanarTransformation_f32 &transformation)
      {
        return this->transformation.Set(transformation);
      }

      Transformations::PlanarTransformation_f32 BinaryTracker::get_transformation() const
      {
        return transformation;
      }

      Result BinaryTracker::ComputeAllIndexLimits(const EdgeLists &imageEdges, AllIndexLimits &allLimits, MemoryStack &memory)
      {
        allLimits.xDecreasing_yStartIndexes = Array<s32>(1, imageEdges.imageWidth+1, memory);
        allLimits.xIncreasing_yStartIndexes = Array<s32>(1, imageEdges.imageWidth+1, memory);
        allLimits.yDecreasing_xStartIndexes = Array<s32>(1, imageEdges.imageHeight+1, memory);
        allLimits.yIncreasing_xStartIndexes = Array<s32>(1, imageEdges.imageHeight+1, memory);

        AnkiConditionalErrorAndReturnValue(allLimits.yIncreasing_xStartIndexes.IsValid(),
          RESULT_FAIL_OUT_OF_MEMORY, "BinaryTracker::ComputeAllIndexLimits", "Could not allocate local memory");

        ComputeIndexLimitsVertical(imageEdges.xDecreasing, allLimits.xDecreasing_yStartIndexes);

        ComputeIndexLimitsVertical(imageEdges.xIncreasing, allLimits.xIncreasing_yStartIndexes);

        ComputeIndexLimitsHorizontal(imageEdges.yDecreasing, allLimits.yDecreasing_xStartIndexes);

        ComputeIndexLimitsHorizontal(imageEdges.yIncreasing, allLimits.yIncreasing_xStartIndexes);

        return RESULT_OK;
      }

      Result BinaryTracker::UpdateTrack(
        const Array<u8> &nextImage,
        const u8 edgeDetection_grayvalueThreshold, const s32 edgeDetection_minComponentWidth, const s32 edgeDetection_maxDetectionsPerType,
        const s32 matching_maxDistance, const s32 matching_maxCorrespondences,
        const bool useFixedPoint_projective,
        MemoryStack scratch)
      {
        Result lastResult;

        EdgeLists nextImageEdges;

        nextImageEdges.xDecreasing = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, scratch);
        nextImageEdges.xIncreasing = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, scratch);
        nextImageEdges.yDecreasing = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, scratch);
        nextImageEdges.yIncreasing = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, scratch);

        AnkiConditionalErrorAndReturnValue(nextImageEdges.xDecreasing.IsValid() && nextImageEdges.xIncreasing.IsValid() && nextImageEdges.yDecreasing.IsValid() && nextImageEdges.yIncreasing.IsValid(),
          RESULT_FAIL_OUT_OF_MEMORY, "BinaryTracker::UpdateTrack", "Could not allocate local scratch");

#ifdef SEND_BINARY_IMAGES_TO_MATLAB
        {
          MemoryStack matlabScratch(malloc(1000000),1000000);

          Matlab matlab(false);

          cv::Mat drawn = DrawIndexes(this->templateImageHeight, this->templateImageWidth, nextImageEdges.xDecreasing, nextImageEdges.xIncreasing, nextImageEdges.yDecreasing, nextImageEdges.yIncreasing);

          cv::imshow("binary next", drawn);
          cv::waitKey();

          Array<u8> rendered(nextImageEdges.imageHeight, nextImageEdges.imageWidth, matlabScratch);

          rendered.SetZero();
          DrawPoints(nextImageEdges.xDecreasing, 1, rendered);
          matlab.PutArray(rendered, "nextImageEdges.xDecreasing");

          rendered.SetZero();
          DrawPoints(nextImageEdges.xIncreasing, 1, rendered);
          matlab.PutArray(rendered, "nextImageEdges.xIncreasing");

          rendered.SetZero();
          DrawPoints(nextImageEdges.yDecreasing, 1, rendered);
          matlab.PutArray(rendered, "nextImageEdges.yDecreasing");

          rendered.SetZero();
          DrawPoints(nextImageEdges.yIncreasing, 1, rendered);
          matlab.PutArray(rendered, "nextImageEdges.yIncreasing");

          matlab.EvalStringEcho("");

          free(matlabScratch.get_buffer());
        }
#endif // #ifdef SEND_BINARY_IMAGES_TO_MATLAB

        BeginBenchmark("ut_DetectEdges");

        lastResult = DetectBlurredEdges(nextImage, edgeDetection_grayvalueThreshold, edgeDetection_minComponentWidth, nextImageEdges);

        EndBenchmark("ut_DetectEdges");

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::UpdateTrack", "DetectBlurredEdge failed");

        // First, to speed up the correspondence search, find the min and max of x or y points
        AllIndexLimits allLimits;

        BeginBenchmark("ut_IndexLimits");

        if((lastResult = BinaryTracker::ComputeAllIndexLimits(nextImageEdges, allLimits, scratch)) != RESULT_OK)
          return lastResult;

        EndBenchmark("ut_IndexLimits");

        // Second, compute the actual correspondence and refine the homography

        BeginBenchmark("ut_translation");

        lastResult = IterativelyRefineTrack_Translation(nextImageEdges, allLimits, matching_maxDistance, matching_maxCorrespondences, scratch);

        EndBenchmark("ut_translation");

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::UpdateTrack", "Transformations::TRANSFORM_TRANSLATION failed");

        BeginBenchmark("ut_projective");

        lastResult = IterativelyRefineTrack_Projective(nextImageEdges, allLimits, matching_maxDistance, matching_maxCorrespondences, useFixedPoint_projective, scratch);

        EndBenchmark("ut_projective");

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::UpdateTrack", "Transformations::TRANSFORM_PROJECTIVE failed");

        return RESULT_OK;
      }

      Result BinaryTracker::ComputeIndexLimitsVertical(const FixedLengthList<Point<s16> > &points, Array<s32> &yStartIndexes)
      {
        const Point<s16> * restrict pPoints = points.Pointer(0);
        s32 * restrict pIndexes = yStartIndexes.Pointer(0,0);

        const s32 maxY = yStartIndexes.get_size(1) - 1;
        const s32 numPoints = points.get_size();

        s32 iPoint = 0;
        s32 lastY = -1;

        while(iPoint < numPoints) {
          while((iPoint < numPoints) && (pPoints[iPoint].y == lastY)) {
            iPoint++;
          }

          for(s32 y=lastY+1; y<=pPoints[iPoint].y; y++) {
            pIndexes[y] = iPoint;
          }

          lastY = pPoints[iPoint].y;
        }

        lastY = pPoints[numPoints-1].y;

        for(s32 y=lastY+1; y<=maxY; y++) {
          pIndexes[y] = numPoints;
        }

        return RESULT_OK;
      }

      Result BinaryTracker::ComputeIndexLimitsHorizontal(const FixedLengthList<Point<s16> > &points, Array<s32> &xStartIndexes)
      {
        const Point<s16> * restrict pPoints = points.Pointer(0);
        s32 * restrict pIndexes = xStartIndexes.Pointer(0,0);

        const s32 maxX = xStartIndexes.get_size(1) - 1;
        const s32 numPoints = points.get_size();

        s32 iPoint = 0;
        s32 lastX = -1;

        while(iPoint < numPoints) {
          while((iPoint < numPoints) && (pPoints[iPoint].x == lastX)) {
            iPoint++;
          }

          for(s32 x=lastX+1; x<=pPoints[iPoint].x; x++) {
            pIndexes[x] = iPoint;
          }

          lastX = pPoints[iPoint].x;
        }

        lastX = pPoints[numPoints-1].x;

        for(s32 x=lastX+1; x<=maxX; x++) {
          pIndexes[x] = numPoints;
        }

        return RESULT_OK;
      }

      /*      Result BinaryTracker::FindVerticalCorrespondences_Translation(
      const s32 maxMatchingDistance,
      const Transformations::PlanarTransformation_f32 &transformation,
      const FixedLengthList<Point<s16> > &templatePoints,
      const FixedLengthList<Point<s16> > &newPoints,
      const s32 imageHeight,
      const s32 imageWidth,
      const Array<s32> &xStartIndexes,
      f32 &sumY,
      s32 &numCorrespondences,
      MemoryStack scratch)
      {
      const s32 numTemplatePoints = templatePoints.get_size();

      const Array<f32> &homography = transformation.get_homography();
      const Point<f32> &centerOffset = transformation.get_centerOffset();

      const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
      const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
      const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

      AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

      sumY = 0.0f;
      numCorrespondences = 0;

      const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
      const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
      const s32 * restrict pXStartIndexes = xStartIndexes.Pointer(0,0);

      for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
      const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
      const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

      //
      // Warp x and y based on the current homography
      //

      // Subtract the center offset
      const f32 xc = xr - centerOffset.x;
      const f32 yc = yr - centerOffset.y;

      // Projective warp
      const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
      const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
      const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

      // TODO: verify the -0.5f is correct
      // TODO: can this be done faster on the M4?
      const s32 warpedXrounded = static_cast<s32>(Round(warpedX + centerOffset.x - 0.5f));
      const s32 warpedYrounded = static_cast<s32>(Round(warpedY + centerOffset.y - 0.5f));

      if(warpedYrounded >= maxMatchingDistance && warpedYrounded < (imageHeight-maxMatchingDistance)) {
      s32 minOffset = -maxMatchingDistance;
      s32 maxOffset = maxMatchingDistance + 1;

      // TODO: manually verify these conditions are correct
      if(warpedXrounded < maxMatchingDistance) {
      minOffset += (maxMatchingDistance - warpedXrounded);
      }

      if(warpedXrounded > (imageWidth - maxMatchingDistance - 2)) {
      maxOffset += (imageWidth - warpedXrounded - maxMatchingDistance - 2);
      }

      for(s32 offset=minOffset; offset<maxOffset; offset++) {
      const s32 xpRounded = warpedXrounded;
      const s32 ypRounded = warpedYrounded + offset;

      // TODO: make a binary search?
      for(s32 iMatch=pXStartIndexes[xpRounded]; iMatch<pXStartIndexes[xpRounded+1]; iMatch++) {
      if(ypRounded == pNewPoints[iMatch].y) {
      //const f32 matchedY = warpedY + static_cast<f32>(offset);
      sumY += offset;
      numCorrespondences++;
      }
      }
      } // for(s32 iOffset=-maxMatchingDistance; iOffset<=maxMatchingDistance; iOffset++)
      } // if(warpedYrounded >= maxMatchingDistance && warpedYrounded < (imageHeight-maxMatchingDistance))
      } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

      return RESULT_OK;
      } // Result BinaryTracker::FindVerticalCorrespondences_Translation()*/

      Result BinaryTracker::FindVerticalCorrespondences_Translation_FixedPoint(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &xStartIndexes,
        s32 &sumY,
        s32 &numCorrespondences,
        MemoryStack scratch)
      {
        const s32 numTemplatePoints = templatePoints.get_size();

        const Array<f32> &homography = transformation.get_homography();
        const Point<f32> &centerOffset = transformation.get_centerOffset();

        // TODO: if the homography is just translation, we can do this faster
        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
        const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

        AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

        sumY = 0;
        numCorrespondences = 0;

        const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
        const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
        const s32 * restrict pXStartIndexes = xStartIndexes.Pointer(0,0);

        for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
          const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
          const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

          //
          // Warp x and y based on the current homography
          //

          // Subtract the center offset
          const f32 xc = xr - centerOffset.x;
          const f32 yc = yr - centerOffset.y;

          //const s32 xc_s32 = static_cast<s32>(Round(xc));
          //const s32 yc_s32 = static_cast<s32>(Round(yc));

          // Projective warp
          const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
          const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
          const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

          //TODO: if these are always .5, then we should floor here and add to sumY at the end
          //const s32 warpedX_s32 = static_cast<s32>(Round(warpedX));
          //const s32 warpedY_s32 = static_cast<s32>(Round(warpedY));

          // TODO: verify the -0.5f is correct
          // TODO: can this be done faster on the M4?
          const s32 warpedXrounded = static_cast<s32>(Round(warpedX + centerOffset.x - 0.5f));
          const s32 warpedYrounded = static_cast<s32>(Round(warpedY + centerOffset.y - 0.5f));

          if(warpedYrounded >= maxMatchingDistance && warpedYrounded < (imageHeight-maxMatchingDistance)) {
            s32 minOffset = -maxMatchingDistance;
            s32 maxOffset = maxMatchingDistance + 1;

            // TODO: manually verify these conditions are correct
            if(warpedXrounded < maxMatchingDistance) {
              minOffset += (maxMatchingDistance - warpedXrounded);
            }

            if(warpedXrounded > (imageWidth - maxMatchingDistance - 2)) {
              maxOffset += (imageWidth - warpedXrounded - maxMatchingDistance - 2);
            }

            for(s32 offset=minOffset; offset<maxOffset; offset++) {
              const s32 xpRounded = warpedXrounded;
              const s32 ypRounded = warpedYrounded + offset;

              // TODO: make a binary search?
              for(s32 iMatch=pXStartIndexes[xpRounded]; iMatch<pXStartIndexes[xpRounded+1]; iMatch++) {
                if(ypRounded == pNewPoints[iMatch].y) {
                  sumY += offset;
                  numCorrespondences++;
                }
              }
            } // for(s32 iOffset=-maxMatchingDistance; iOffset<=maxMatchingDistance; iOffset++)
          } // if(warpedYrounded >= maxMatchingDistance && warpedYrounded < (imageHeight-maxMatchingDistance))
        } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

        return RESULT_OK;
      } // Result BinaryTracker::FindVerticalCorrespondences_Translation_FixedPoint()

      /*      Result BinaryTracker::FindHorizontalCorrespondences_Translation(
      const s32 maxMatchingDistance,
      const Transformations::PlanarTransformation_f32 &transformation,
      const FixedLengthList<Point<s16> > &templatePoints,
      const FixedLengthList<Point<s16> > &newPoints,
      const s32 imageHeight,
      const s32 imageWidth,
      const Array<s32> &yStartIndexes,
      f32 &sumX,
      s32 &numCorrespondences,
      MemoryStack scratch)
      {
      const s32 numTemplatePoints = templatePoints.get_size();

      const Array<f32> &homography = transformation.get_homography();
      const Point<f32> &centerOffset = transformation.get_centerOffset();

      const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
      const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
      const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

      AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

      sumX = 0.0f;
      numCorrespondences = 0;

      const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
      const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
      const s32 * restrict pYStartIndexes = yStartIndexes.Pointer(0,0);

      for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
      const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
      const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

      //
      // Warp x and y based on the current homography
      //

      // Subtract the center offset
      const f32 xc = xr - centerOffset.x;
      const f32 yc = yr - centerOffset.y;

      // Projective warp
      const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
      const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
      const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

      // TODO: verify the -0.5f is correct
      // TODO: can this be done faster on the M4?
      const s32 warpedXrounded = static_cast<s32>(Round(warpedX + centerOffset.x - 0.5f));
      const s32 warpedYrounded = static_cast<s32>(Round(warpedY + centerOffset.y - 0.5f));

      if(warpedXrounded >= maxMatchingDistance && warpedXrounded < (imageWidth-maxMatchingDistance)) {
      s32 minOffset = -maxMatchingDistance;
      s32 maxOffset = maxMatchingDistance + 1;

      // TODO: manually verify these conditions are correct
      if(warpedYrounded < maxMatchingDistance) {
      minOffset += (maxMatchingDistance - warpedYrounded);
      }

      if(warpedYrounded > (imageHeight - maxMatchingDistance - 2)) {
      maxOffset += (imageHeight - warpedYrounded - maxMatchingDistance - 2);
      }

      for(s32 offset=minOffset; offset<maxOffset; offset++) {
      const s32 xpRounded = warpedXrounded + offset;
      const s32 ypRounded = warpedYrounded;

      // TODO: make a binary search?
      for(s32 iMatch=pYStartIndexes[ypRounded]; iMatch<pYStartIndexes[ypRounded+1]; iMatch++) {
      if(xpRounded == pNewPoints[iMatch].x) {
      //const f32 matchedX = warpedX + static_cast<f32>(offset);
      sumX += offset;
      numCorrespondences++;
      }
      }
      } // for(s32 iOffset=-maxMatchingDistance; iOffset<=maxMatchingDistance; iOffset++)
      } // if(warpedYrounded >= maxMatchingDistance && warpedYrounded < (imageHeight-maxMatchingDistance))
      } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

      return RESULT_OK;
      } // Result BinaryTracker::FindHorizontalCorrespondences_Translation()*/

      Result BinaryTracker::FindHorizontalCorrespondences_Translation_FixedPoint(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &yStartIndexes,
        s32 &sumX,
        s32 &numCorrespondences,
        MemoryStack scratch)
      {
        const s32 numTemplatePoints = templatePoints.get_size();

        const Array<f32> &homography = transformation.get_homography();
        const Point<f32> &centerOffset = transformation.get_centerOffset();

        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
        const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

        AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

        sumX = 0;
        numCorrespondences = 0;

        const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
        const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
        const s32 * restrict pYStartIndexes = yStartIndexes.Pointer(0,0);

        for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
          const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
          const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

          //
          // Warp x and y based on the current homography
          //

          // Subtract the center offset
          const f32 xc = xr - centerOffset.x;
          const f32 yc = yr - centerOffset.y;

          //const s32 xc_s32 = static_cast<s32>(Round(xc));
          //const s32 yc_s32 = static_cast<s32>(Round(yc));

          // Projective warp
          const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
          const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
          const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

          //TODO: if these are always .5, then we should floor here and add to sumY at the end
          //const s32 warpedX_s32 = static_cast<s32>(Round(warpedX));
          //const s32 warpedY_s32 = static_cast<s32>(Round(warpedY));

          // TODO: verify the -0.5f is correct
          // TODO: can this be done faster on the M4?
          const s32 warpedXrounded = static_cast<s32>(Round(warpedX + centerOffset.x - 0.5f));
          const s32 warpedYrounded = static_cast<s32>(Round(warpedY + centerOffset.y - 0.5f));

          if(warpedXrounded >= maxMatchingDistance && warpedXrounded < (imageWidth-maxMatchingDistance)) {
            s32 minOffset = -maxMatchingDistance;
            s32 maxOffset = maxMatchingDistance + 1;

            // TODO: manually verify these conditions are correct
            if(warpedYrounded < maxMatchingDistance) {
              minOffset += (maxMatchingDistance - warpedYrounded);
            }

            if(warpedYrounded > (imageHeight - maxMatchingDistance - 2)) {
              maxOffset += (imageHeight - warpedYrounded - maxMatchingDistance - 2);
            }

            for(s32 offset=minOffset; offset<maxOffset; offset++) {
              const s32 xpRounded = warpedXrounded + offset;
              const s32 ypRounded = warpedYrounded;

              // TODO: make a binary search?
              for(s32 iMatch=pYStartIndexes[ypRounded]; iMatch<pYStartIndexes[ypRounded+1]; iMatch++) {
                if(xpRounded == pNewPoints[iMatch].x) {
                  //const s32 matchedX_s32 = warpedX_s32 + offset;
                  sumX += offset;
                  numCorrespondences++;
                }
              }
            } // for(s32 iOffset=-maxMatchingDistance; iOffset<=maxMatchingDistance; iOffset++)
          } // if(warpedYrounded >= maxMatchingDistance && warpedYrounded < (imageHeight-maxMatchingDistance))
        } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

        return RESULT_OK;
      } // Result BinaryTracker::FindHorizontalCorrespondences_Translation_FixedPoint()

      NO_INLINE Result BinaryTracker::FindVerticalCorrespondences_Projective(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &xStartIndexes,
        Array<f32> &AtA,
        Array<f32> &Atb_t,
        MemoryStack scratch)
      {
        const s32 numTemplatePoints = templatePoints.get_size();

        const Array<f32> &homography = transformation.get_homography();
        const Point<f32> &centerOffset = transformation.get_centerOffset();

        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
        const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

        AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

        // These addresses should be known at compile time, so should be faster
#if !defined(USE_ARM_ACCELERATION) // natural C
        f32 AtA_raw[8][8];
        f32 Atb_t_raw[8];

        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=0; ja<8; ja++) {
            AtA_raw[ia][ja] = 0;
          }
          Atb_t_raw[ia] = 0;
        }
#else // ARM optimized
        f32 AtA_raw33 = 0, AtA_raw34 = 0, AtA_raw35 = 0, AtA_raw36 = 0, AtA_raw37 = 0;
        f32 AtA_raw44 = 0, AtA_raw45 = 0, AtA_raw46 = 0, AtA_raw47 = 0;
        f32 AtA_raw55 = 0, AtA_raw56 = 0, AtA_raw57 = 0;
        f32 AtA_raw66 = 0, AtA_raw67 = 0;
        f32 AtA_raw77 = 0;

        f32 Atb_t_raw3 = 0, Atb_t_raw4 = 0, Atb_t_raw5 = 0, Atb_t_raw6 = 0, Atb_t_raw7 = 0;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

        const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
        const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
        const s32 * restrict pXStartIndexes = xStartIndexes.Pointer(0,0);

        for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
          const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
          const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

          //
          // Warp x and y based on the current homography
          //

          // Subtract the center offset
          const f32 xc = xr - centerOffset.x;
          const f32 yc = yr - centerOffset.y;

          // Projective warp
          const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
          const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
          const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

          // TODO: verify the -0.5f is correct
          // TODO: can this be done faster on the M4?
          const s32 warpedXrounded = static_cast<s32>(Round(warpedX + centerOffset.x - 0.5f));
          const s32 warpedYrounded = static_cast<s32>(Round(warpedY + centerOffset.y - 0.5f));

          if(warpedYrounded >= maxMatchingDistance && warpedYrounded < (imageHeight-maxMatchingDistance)) {
            s32 minOffset = -maxMatchingDistance;
            s32 maxOffset = maxMatchingDistance + 1;

            // TODO: manually verify these conditions are correct
            if(warpedXrounded < maxMatchingDistance) {
              minOffset += (maxMatchingDistance - warpedXrounded);
            }

            if(warpedXrounded > (imageWidth - maxMatchingDistance - 2)) {
              maxOffset += (imageWidth - warpedXrounded - maxMatchingDistance - 2);
            }

            for(s32 offset=minOffset; offset<maxOffset; offset++) {
              const s32 xpRounded = warpedXrounded;
              const s32 ypRounded = warpedYrounded + offset;

              // TODO: make a binary search?
              for(s32 iMatch=pXStartIndexes[xpRounded]; iMatch<pXStartIndexes[xpRounded+1]; iMatch++) {
                if(ypRounded == pNewPoints[iMatch].y) {
                  const f32 yp = warpedY + static_cast<f32>(offset);

#if !defined(USE_ARM_ACCELERATION) // natural C
                  const f32 aValues[8] = {0, 0, 0, -xc, -yc, -1, xc*yp, yc*yp};

                  const f32 bValue = -yp;

                  for(s32 ia=0; ia<8; ia++) {
                    for(s32 ja=ia; ja<8; ja++) {
                      AtA_raw[ia][ja] += aValues[ia] * aValues[ja];
                    }
                    Atb_t_raw[ia] += bValue * aValues[ia];
                  }
#else // ARM optimized
                  const f32 aValues6 = xc*yp;
                  const f32 aValues7 = yc*yp;

                  AtA_raw33 += xc * xc;
                  AtA_raw34 += xc * yc;
                  AtA_raw35 += xc;
                  AtA_raw36 -= xc * aValues6;
                  AtA_raw37 -= xc * aValues7;

                  AtA_raw44 += yc * yc;
                  AtA_raw45 += yc;
                  AtA_raw46 -= yc * aValues6;
                  AtA_raw47 -= yc * aValues7;

                  AtA_raw55 += 1;
                  AtA_raw56 -= aValues6;
                  AtA_raw57 -= aValues7;

                  AtA_raw66 += aValues6 * aValues6;
                  AtA_raw67 += aValues6 * aValues7;

                  AtA_raw77 += aValues7 * aValues7;

                  Atb_t_raw3 += yp * xc;
                  Atb_t_raw4 += yp * yc;
                  Atb_t_raw5 += yp;
                  Atb_t_raw6 -= yp * aValues6;
                  Atb_t_raw7 -= yp * aValues7;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

                  //const f32 aValues[8] = {0, 0, 0, -xc, -yc, -1, xc*yp, yc*yp};

                  //const f32 bValue = -yp;

                  /*for(s32 ia=3; ia<8; ia++) {
                  for(s32 ja=ia; ja<8; ja++) {
                  AtA_raw[ia][ja] += aValues[ia] * aValues[ja];
                  }
                  }*/

                  /*for(s32 ja=3; ja<8; ja++) {
                  AtA_raw[3][ja] += aValues[3] * aValues[ja];
                  }

                  for(s32 ja=4; ja<8; ja++) {
                  AtA_raw[4][ja] += aValues[4] * aValues[ja];
                  }

                  for(s32 ja=5; ja<8; ja++) {
                  AtA_raw[5][ja] += aValues[5] * aValues[ja];
                  }

                  for(s32 ja=6; ja<8; ja++) {
                  AtA_raw[6][ja] += aValues[6] * aValues[ja];
                  }

                  for(s32 ja=7; ja<8; ja++) {
                  AtA_raw[7][ja] += aValues[7] * aValues[ja];
                  }

                  Atb_t_raw[3] -= yp * aValues[3];
                  Atb_t_raw[4] -= yp * aValues[4];
                  Atb_t_raw[5] -= yp * aValues[5];
                  Atb_t_raw[6] -= yp * aValues[6];
                  Atb_t_raw[7] -= yp * aValues[7];*/

                  /*const f32 aValues[8] = {0, 0, 0, -xc, -yc, -1, xc*yp, yc*yp};

                  AtA_raw[3][3] += aValues[3] * aValues[3];
                  AtA_raw[3][4] += aValues[3] * aValues[4];
                  AtA_raw[3][5] += aValues[3] * aValues[5];
                  AtA_raw[3][6] += aValues[3] * aValues[6];
                  AtA_raw[3][7] += aValues[3] * aValues[7];

                  AtA_raw[4][4] += aValues[4] * aValues[4];
                  AtA_raw[4][5] += aValues[4] * aValues[5];
                  AtA_raw[4][6] += aValues[4] * aValues[6];
                  AtA_raw[4][7] += aValues[4] * aValues[7];

                  AtA_raw[5][5] += aValues[5] * aValues[5];
                  AtA_raw[5][6] += aValues[5] * aValues[6];
                  AtA_raw[5][7] += aValues[5] * aValues[7];

                  AtA_raw[6][6] += aValues[6] * aValues[6];
                  AtA_raw[6][7] += aValues[6] * aValues[7];

                  AtA_raw[7][7] += aValues[7] * aValues[7];

                  Atb_t_raw[3] -= yp * aValues[3];
                  Atb_t_raw[4] -= yp * aValues[4];
                  Atb_t_raw[5] -= yp * aValues[5];
                  Atb_t_raw[6] -= yp * aValues[6];
                  Atb_t_raw[7] -= yp * aValues[7];*/

                  /*const f32 aValues6 = xc*yp;
                  const f32 aValues7 = yc*yp;

                  AtA_raw[3][3] += -xc * -xc;
                  AtA_raw[3][4] += -xc * -yc;
                  AtA_raw[3][5] += -xc * -1;
                  AtA_raw[3][6] += -xc * aValues6;
                  AtA_raw[3][7] += -xc * aValues7;

                  AtA_raw[4][4] += -yc * -yc;
                  AtA_raw[4][5] += -yc * -1;
                  AtA_raw[4][6] += -yc * aValues6;
                  AtA_raw[4][7] += -yc * aValues7;

                  AtA_raw[5][5] += -1 * -1;
                  AtA_raw[5][6] += -1 * aValues6;
                  AtA_raw[5][7] += -1 * aValues7;

                  AtA_raw[6][6] += aValues6 * aValues6;
                  AtA_raw[6][7] += aValues6 * aValues7;

                  AtA_raw[7][7] += aValues7 * aValues7;

                  Atb_t_raw[3] -= yp * -xc;
                  Atb_t_raw[4] -= yp * -yc;
                  Atb_t_raw[5] -= yp * -1;
                  Atb_t_raw[6] -= yp * aValues6;
                  Atb_t_raw[7] -= yp * aValues7;*/

                  /*const f32 aValues6 = xc*yp;
                  const f32 aValues7 = yc*yp;

                  AtA_raw[3][3] += xc * xc;
                  AtA_raw[3][4] += xc * yc;
                  AtA_raw[3][5] += xc;
                  AtA_raw[3][6] -= xc * aValues6;
                  AtA_raw[3][7] -= xc * aValues7;

                  AtA_raw[4][4] += yc * yc;
                  AtA_raw[4][5] += yc;
                  AtA_raw[4][6] -= yc * aValues6;
                  AtA_raw[4][7] -= yc * aValues7;

                  AtA_raw[5][5] += 1;
                  AtA_raw[5][6] -= aValues6;
                  AtA_raw[5][7] -= aValues7;

                  AtA_raw[6][6] += aValues6 * aValues6;
                  AtA_raw[6][7] += aValues6 * aValues7;

                  AtA_raw[7][7] += aValues7 * aValues7;

                  Atb_t_raw[3] += yp * xc;
                  Atb_t_raw[4] += yp * yc;
                  Atb_t_raw[5] += yp;
                  Atb_t_raw[6] -= yp * aValues6;
                  Atb_t_raw[7] -= yp * aValues7;*/
                }
              } // if(ypRounded == pNewPoints[iMatch].y)
            } // for(s32 iOffset=-maxMatchingDistance; iOffset<=maxMatchingDistance; iOffset++)
          } // if(warpedYrounded >= maxMatchingDistance && warpedYrounded < (imageHeight-maxMatchingDistance))
        } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

#if !defined(USE_ARM_ACCELERATION) // natural C
        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=ia; ja<8; ja++) {
            AtA[ia][ja] = AtA_raw[ia][ja];
          }
          Atb_t[0][ia] = Atb_t_raw[ia];
        }
#else // ARM optimized
        AtA[3][3] = AtA_raw33; AtA[3][4] = AtA_raw34; AtA[3][5] = AtA_raw35; AtA[3][6] = AtA_raw36; AtA[3][7] = AtA_raw37;
        AtA[4][4] = AtA_raw44; AtA[4][5] = AtA_raw45; AtA[4][6] = AtA_raw46; AtA[4][7] = AtA_raw47;
        AtA[5][5] = AtA_raw55; AtA[5][6] = AtA_raw56; AtA[5][7] = AtA_raw57;
        AtA[6][6] = AtA_raw66; AtA[6][7] = AtA_raw67;
        AtA[7][7] = AtA_raw77;

        Atb_t[0][3] = Atb_t_raw3; Atb_t[0][4] = Atb_t_raw4; Atb_t[0][5] = Atb_t_raw5; Atb_t[0][6] = Atb_t_raw6; Atb_t[0][7] = Atb_t_raw7;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

        return RESULT_OK;
      } // NO_INLINE Result BinaryTracker::FindVerticalCorrespondences_Projective()

      NO_INLINE Result BinaryTracker::FindVerticalCorrespondences_Projective_FixedPoint(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &xStartIndexes,
        Array<f32> &AtA,
        Array<f32> &Atb_t,
        MemoryStack scratch)
      {
        //const s32 numFractionalBits = 2;

        const s32 numTemplatePoints = templatePoints.get_size();

        const Array<f32> &homography = transformation.get_homography();
        const Point<f32> &centerOffset = transformation.get_centerOffset();

        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
        const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

        AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

        // These addresses should be known at compile time, so should be faster
#if !defined(USE_ARM_ACCELERATION) // natural C
        s64 AtA_raw[8][8];
        s64 Atb_t_raw[8];

        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=0; ja<8; ja++) {
            AtA_raw[ia][ja] = 0;
          }
          Atb_t_raw[ia] = 0;
        }
#else // ARM optimized
        s32 AtA_raw33 = 0, AtA_raw34 = 0, AtA_raw35 = 0, AtA_raw36 = 0, AtA_raw37 = 0;
        s32 AtA_raw44 = 0, AtA_raw45 = 0, AtA_raw46 = 0, AtA_raw47 = 0;
        s32 AtA_raw55 = 0, AtA_raw56 = 0, AtA_raw57 = 0;
        s32 AtA_raw66 = 0, AtA_raw67 = 0;
        s32 AtA_raw77 = 0;

        s32 Atb_t_raw3 = 0, Atb_t_raw4 = 0, Atb_t_raw5 = 0, Atb_t_raw6 = 0, Atb_t_raw7 = 0;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

        const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
        const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
        const s32 * restrict pXStartIndexes = xStartIndexes.Pointer(0,0);

        for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
          const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
          const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

          //
          // Warp x and y based on the current homography
          //

          // Subtract the center offset
          const f32 xc = xr - centerOffset.x;
          const f32 yc = yr - centerOffset.y;

          //const s32 xc_SQ27p1 = static_cast<s32>(Round(xc * static_cast<f32>(1<<numFractionalBits)));
          //const s32 yc_SQ27p1 = static_cast<s32>(Round(yc * static_cast<f32>(1<<numFractionalBits)));
          const s32 xc_s32 = static_cast<s32>(Round(xc));
          const s32 yc_s32 = static_cast<s32>(Round(yc));

          // Projective warp
          const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
          const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
          const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

          //const s32 warpedX_SQ27p1 = static_cast<s32>(Round(warpedX * static_cast<f32>(1<<numFractionalBits)));
          //const s32 warpedY_SQ27p1 = static_cast<s32>(Round(warpedY * static_cast<f32>(1<<numFractionalBits)));
          const s32 warpedX_s32 = static_cast<s32>(Round(warpedX));
          const s32 warpedY_s32 = static_cast<s32>(Round(warpedY));

          // TODO: verify the -0.5f is correct
          // TODO: can this be done faster on the M4?
          const s32 warpedXrounded = static_cast<s32>(Round(warpedX + centerOffset.x - 0.5f));
          const s32 warpedYrounded = static_cast<s32>(Round(warpedY + centerOffset.y - 0.5f));

          if(warpedYrounded >= maxMatchingDistance && warpedYrounded < (imageHeight-maxMatchingDistance)) {
            s32 minOffset = -maxMatchingDistance;
            s32 maxOffset = maxMatchingDistance + 1;

            // TODO: manually verify these conditions are correct
            if(warpedXrounded < maxMatchingDistance) {
              minOffset += (maxMatchingDistance - warpedXrounded);
            }

            if(warpedXrounded > (imageWidth - maxMatchingDistance - 2)) {
              maxOffset += (imageWidth - warpedXrounded - maxMatchingDistance - 2);
            }

            for(s32 offset=minOffset; offset<maxOffset; offset++) {
              const s32 xpRounded = warpedXrounded;
              const s32 ypRounded = warpedYrounded + offset;

              // TODO: make a binary search?
              for(s32 iMatch=pXStartIndexes[xpRounded]; iMatch<pXStartIndexes[xpRounded+1]; iMatch++) {
                if(ypRounded == pNewPoints[iMatch].y) {
                  const s32 yp_s32 = warpedY_s32 + offset;

#if !defined(USE_ARM_ACCELERATION) // natural C
                  const s32 aValues[8] = {
                    0, 0, 0,
                    -xc_s32,
                    -yc_s32,
                    -1, //(-1) << numFractionalBits,
                    xc_s32 * yp_s32,
                    yc_s32 * yp_s32};

                  const s32 bValue = -yp_s32;

                  for(s32 ia=0; ia<8; ia++) {
                    for(s32 ja=ia; ja<8; ja++) {
                      AtA_raw[ia][ja] += aValues[ia] * aValues[ja];
                    }

                    Atb_t_raw[ia] += bValue * aValues[ia];
                  }
#else // ARM optimized
                  //const s32 aValues[8] = {
                  //  0, 0, 0,
                  //  -xc_s32,
                  //  -yc_s32,
                  //  -1, //(-1) << numFractionalBits,
                  //  xc_s32 * yp_s32,
                  //  yc_s32 * yp_s32};

                  //const s32 bValue = -yp_s32;

                  //for(s32 ia=0; ia<8; ia++) {
                  //  for(s32 ja=ia; ja<8; ja++) {
                  //    AtA_raw[ia][ja] = __SMLALD(aValues[ia], aValues[ja], AtA_raw[ia][ja]);
                  //  }

                  //  Atb_t_raw[ia] = __SMLALD(bValue, aValues[ia], Atb_t_raw[ia]);
                  //}

                  //const s32 aValues[8] = {
                  //  0, 0, 0,
                  //  -xc_s32,
                  //  -yc_s32,
                  //  -1, //(-1) << numFractionalBits,
                  //  xc_s32 * yp_s32,
                  //  yc_s32 * yp_s32};

                  //const s32 bValue = -yp_s32;

                  //for(s32 ja=3; ja<8; ja++) {
                  //  AtA_raw[3][ja] = __SMLALD(aValues[3], aValues[ja], AtA_raw[3][ja]);
                  //}

                  //for(s32 ja=4; ja<8; ja++) {
                  //  AtA_raw[4][ja] = __SMLALD(aValues[4], aValues[ja], AtA_raw[4][ja]);
                  //}

                  //for(s32 ja=5; ja<8; ja++) {
                  //  AtA_raw[5][ja] = __SMLALD(aValues[5], aValues[ja], AtA_raw[5][ja]);
                  //}

                  //for(s32 ja=6; ja<8; ja++) {
                  //  AtA_raw[6][ja] = __SMLALD(aValues[6], aValues[ja], AtA_raw[6][ja]);
                  //}

                  //for(s32 ja=7; ja<8; ja++) {
                  //  AtA_raw[7][ja] = __SMLALD(aValues[7], aValues[ja], AtA_raw[7][ja]);
                  //}

                  //Atb_t_raw[3] = __SMLALD(bValue, aValues[3], Atb_t_raw[3]);
                  //Atb_t_raw[4] = __SMLALD(bValue, aValues[4], Atb_t_raw[4]);
                  //Atb_t_raw[5] = __SMLALD(bValue, aValues[5], Atb_t_raw[5]);
                  //Atb_t_raw[6] = __SMLALD(bValue, aValues[6], Atb_t_raw[6]);
                  //Atb_t_raw[7] = __SMLALD(bValue, aValues[7], Atb_t_raw[7]);

                  const s32 aValues3 = -xc_s32;
                  const s32 aValues4 = -yc_s32;
                  //const s32 aValues5 =  -1;
                  const s32 aValues6 = xc_s32 * yp_s32;
                  const s32 aValues7 = yc_s32 * yp_s32;

                  const s32 bValue = -yp_s32;

                  AtA_raw33 = __SMMLA(aValues3, aValues3, AtA_raw33);
                  AtA_raw34 = __SMMLA(aValues3, aValues4, AtA_raw34);
                  AtA_raw35 -= aValues3;
                  AtA_raw36 = __SMMLA(aValues3, aValues6, AtA_raw36);
                  AtA_raw37 = __SMMLA(aValues3, aValues7, AtA_raw37);

                  AtA_raw44 = __SMMLA(aValues4, aValues4, AtA_raw44);
                  AtA_raw45 -= aValues4;
                  AtA_raw46 = __SMMLA(aValues4, aValues6, AtA_raw46);
                  AtA_raw47 = __SMMLA(aValues4, aValues7, AtA_raw47);

                  AtA_raw55 += 1;
                  AtA_raw56 -= aValues6;
                  AtA_raw57 -= aValues7;

                  AtA_raw66 = __SMMLA(aValues6, aValues6, AtA_raw66);
                  AtA_raw67 = __SMMLA(aValues6, aValues7, AtA_raw67);

                  AtA_raw77 = __SMMLA(aValues7, aValues7, AtA_raw77);

                  Atb_t_raw3 = __SMMLA(bValue, aValues3, Atb_t_raw3);
                  Atb_t_raw4 = __SMMLA(bValue, aValues4, Atb_t_raw4);
                  Atb_t_raw5 -= bValue;
                  Atb_t_raw6 = __SMMLA(bValue, aValues6, Atb_t_raw6);
                  Atb_t_raw7 = __SMMLA(bValue, aValues7, Atb_t_raw7);

#endif // #if !defined(USE_ARM_ACCELERATION) ... #else
                }
              } // if(ypRounded == pNewPoints[iMatch].y)
            } // for(s32 iOffset=-maxMatchingDistance; iOffset<=maxMatchingDistance; iOffset++)
          } // if(warpedYrounded >= maxMatchingDistance && warpedYrounded < (imageHeight-maxMatchingDistance))
        } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

#if !defined(USE_ARM_ACCELERATION) // natural C
        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=ia; ja<8; ja++) {
            AtA[ia][ja] = static_cast<f32>(AtA_raw[ia][ja]);
          }

          Atb_t[0][ia] = static_cast<f32>(Atb_t_raw[ia]);
        }
#else // ARM optimized
        AtA[3][3] = static_cast<f32>(AtA_raw33); AtA[3][4] = static_cast<f32>(AtA_raw34); AtA[3][5] = static_cast<f32>(AtA_raw35); AtA[3][6] = static_cast<f32>(AtA_raw36); AtA[3][7] = static_cast<f32>(AtA_raw37);
        AtA[4][4] = static_cast<f32>(AtA_raw44); AtA[4][5] = static_cast<f32>(AtA_raw45); AtA[4][6] = static_cast<f32>(AtA_raw46); AtA[4][7] = static_cast<f32>(AtA_raw47);
        AtA[5][5] = static_cast<f32>(AtA_raw55); AtA[5][6] = static_cast<f32>(AtA_raw56); AtA[5][7] = static_cast<f32>(AtA_raw57);
        AtA[6][6] = static_cast<f32>(AtA_raw66); AtA[6][7] = static_cast<f32>(AtA_raw67);
        AtA[7][7] = static_cast<f32>(AtA_raw77);

        Atb_t[0][3] = static_cast<f32>(Atb_t_raw3); Atb_t[0][4] = static_cast<f32>(Atb_t_raw4); Atb_t[0][5] = static_cast<f32>(Atb_t_raw5); Atb_t[0][6] = static_cast<f32>(Atb_t_raw6); Atb_t[0][7] = static_cast<f32>(Atb_t_raw7);
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

        return RESULT_OK;
      } // NO_INLINE Result BinaryTracker::FindVerticalCorrespondences_Projective_FixedPoint()

      NO_INLINE Result BinaryTracker::FindHorizontalCorrespondences_Projective(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &yStartIndexes,
        Array<f32> &AtA,
        Array<f32> &Atb_t,
        MemoryStack scratch)
      {
        const s32 numTemplatePoints = templatePoints.get_size();

        const Array<f32> &homography = transformation.get_homography();
        const Point<f32> &centerOffset = transformation.get_centerOffset();

        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
        const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

        AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

        // These addresses should be known at compile time, so should be faster
#if !defined(USE_ARM_ACCELERATION) // natural C
        f32 AtA_raw[8][8];
        f32 Atb_t_raw[8];

        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=0; ja<8; ja++) {
            AtA_raw[ia][ja] = 0;
          }
          Atb_t_raw[ia] = 0;
        }
#else // ARM optimized
        f32 AtA_raw00 = 0, AtA_raw01 = 0, AtA_raw02 = 0, AtA_raw06 = 0, AtA_raw07 = 0;
        f32 AtA_raw11 = 0, AtA_raw12 = 0, AtA_raw16 = 0, AtA_raw17 = 0;
        f32 AtA_raw22 = 0, AtA_raw26 = 0, AtA_raw27 = 0;
        f32 AtA_raw66 = 0, AtA_raw67 = 0;
        f32 AtA_raw77 = 0;

        f32 Atb_t_raw0 = 0, Atb_t_raw1 = 0, Atb_t_raw2 = 0, Atb_t_raw6 = 0, Atb_t_raw7 = 0;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

        const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
        const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
        const s32 * restrict pYStartIndexes = yStartIndexes.Pointer(0,0);

        for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
          const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
          const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

          //
          // Warp x and y based on the current homography
          //

          // Subtract the center offset
          const f32 xc = xr - centerOffset.x;
          const f32 yc = yr - centerOffset.y;

          // Projective warp
          const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
          const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
          const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

          // TODO: verify the -0.5f is correct
          // TODO: can this be done faster on the M4?
          const s32 warpedXrounded = static_cast<s32>(Round(warpedX + centerOffset.x - 0.5f));
          const s32 warpedYrounded = static_cast<s32>(Round(warpedY + centerOffset.y - 0.5f));

          if(warpedXrounded >= maxMatchingDistance && warpedXrounded < (imageWidth-maxMatchingDistance)) {
            s32 minOffset = -maxMatchingDistance;
            s32 maxOffset = maxMatchingDistance + 1;

            // TODO: manually verify these conditions are correct
            if(warpedYrounded < maxMatchingDistance) {
              minOffset += (maxMatchingDistance - warpedYrounded);
            }

            if(warpedYrounded > (imageHeight - maxMatchingDistance - 2)) {
              maxOffset += (imageHeight - warpedYrounded - maxMatchingDistance - 2);
            }

            for(s32 offset=minOffset; offset<maxOffset; offset++) {
              const s32 xpRounded = warpedXrounded + offset;
              const s32 ypRounded = warpedYrounded;

              // TODO: make a binary search?
              for(s32 iMatch=pYStartIndexes[ypRounded]; iMatch<pYStartIndexes[ypRounded+1]; iMatch++) {
                if(xpRounded == pNewPoints[iMatch].x) {
                  const f32 xp = warpedX + static_cast<f32>(offset);

#if !defined(USE_ARM_ACCELERATION) // natural C
                  const f32 aValues[8] = {xc, yc, 1, 0, 0, 0, -xc*xp, -yc*xp};

                  const f32 bValue = xp;

                  for(s32 ia=0; ia<8; ia++) {
                    for(s32 ja=ia; ja<8; ja++) {
                      AtA_raw[ia][ja] += aValues[ia] * aValues[ja];
                    }

                    Atb_t_raw[ia] += aValues[ia] * bValue;
                  }
#else // ARM optimized
                  const f32 aValues6 = -xc*xp;
                  const f32 aValues7 = -yc*xp;

                  AtA_raw00 += xc * xc;
                  AtA_raw01 += xc * yc;
                  AtA_raw02 += xc;
                  AtA_raw06 += xc * aValues6;
                  AtA_raw07 += xc * aValues7;

                  AtA_raw11 += yc * yc;
                  AtA_raw12 += yc;
                  AtA_raw16 += yc * aValues6;
                  AtA_raw17 += yc * aValues7;

                  AtA_raw22 += 1;
                  AtA_raw26 += aValues6;
                  AtA_raw27 += aValues7;

                  AtA_raw66 += aValues6 * aValues6;
                  AtA_raw67 += aValues6 * aValues7;

                  AtA_raw77 += aValues7 * aValues7;

                  Atb_t_raw0 += xc * xp;
                  Atb_t_raw1 += yc * xp;
                  Atb_t_raw2 += xp;
                  Atb_t_raw6 += aValues6 * xp;
                  Atb_t_raw7 += aValues7 * xp;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else
                }
              }
            } // for(s32 iOffset=-maxMatchingDistance; iOffset<=maxMatchingDistance; iOffset++)
          } // if(warpedYrounded >= maxMatchingDistance && warpedYrounded < (imageHeight-maxMatchingDistance))
        } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

#if !defined(USE_ARM_ACCELERATION) // natural C
        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=ia; ja<8; ja++) {
            AtA[ia][ja] = AtA_raw[ia][ja];
          }
          Atb_t[0][ia] = Atb_t_raw[ia];
        }
#else // ARM optimized
        AtA[0][0] = AtA_raw00; AtA[0][1] = AtA_raw01; AtA[0][2] = AtA_raw02; AtA[0][6] = AtA_raw06; AtA[0][7] = AtA_raw07;
        AtA[1][1] = AtA_raw11; AtA[1][2] = AtA_raw12; AtA[1][6] = AtA_raw16; AtA[1][7] = AtA_raw17;
        AtA[2][2] = AtA_raw22; AtA[2][6] = AtA_raw26; AtA[2][7] = AtA_raw27;
        AtA[6][6] = AtA_raw66; AtA[6][7] = AtA_raw67;
        AtA[7][7] = AtA_raw77;

        Atb_t[0][0] = Atb_t_raw0; Atb_t[0][1] = Atb_t_raw1; Atb_t[0][2] = Atb_t_raw2; Atb_t[0][6] = Atb_t_raw6; Atb_t[0][7] = Atb_t_raw7;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

        return RESULT_OK;
      } // NO_INLINE Result BinaryTracker::FindHorizontalCorrespondences_Projective()

      NO_INLINE Result BinaryTracker::FindHorizontalCorrespondences_Projective_FixedPoint(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &yStartIndexes,
        Array<f32> &AtA,
        Array<f32> &Atb_t,
        MemoryStack scratch)
      {
        const s32 numTemplatePoints = templatePoints.get_size();

        const Array<f32> &homography = transformation.get_homography();
        const Point<f32> &centerOffset = transformation.get_centerOffset();

        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
        const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

        AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

        // These addresses should be known at compile time, so should be faster
        s64 AtA_raw[8][8];
        s64 Atb_t_raw[8];

        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=0; ja<8; ja++) {
            AtA_raw[ia][ja] = 0;
          }
          Atb_t_raw[ia] = 0;
        }

        const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
        const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
        const s32 * restrict pYStartIndexes = yStartIndexes.Pointer(0,0);

        for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
          const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
          const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

          //
          // Warp x and y based on the current homography
          //

          // Subtract the center offset
          const f32 xc = xr - centerOffset.x;
          const f32 yc = yr - centerOffset.y;

          const s32 xc_s32 = static_cast<s32>(Round(xc));
          const s32 yc_s32 = static_cast<s32>(Round(yc));

          // Projective warp
          const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
          const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
          const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

          const s32 warpedX_s32 = static_cast<s32>(Round(warpedX));
          const s32 warpedY_s32 = static_cast<s32>(Round(warpedY));

          // TODO: verify the -0.5f is correct
          // TODO: can this be done faster on the M4?
          const s32 warpedXrounded = static_cast<s32>(Round(warpedX + centerOffset.x - 0.5f));
          const s32 warpedYrounded = static_cast<s32>(Round(warpedY + centerOffset.y - 0.5f));

          if(warpedXrounded >= maxMatchingDistance && warpedXrounded < (imageWidth-maxMatchingDistance)) {
            s32 minOffset = -maxMatchingDistance;
            s32 maxOffset = maxMatchingDistance + 1;

            // TODO: manually verify these conditions are correct
            if(warpedYrounded < maxMatchingDistance) {
              minOffset += (maxMatchingDistance - warpedYrounded);
            }

            if(warpedYrounded > (imageHeight - maxMatchingDistance - 2)) {
              maxOffset += (imageHeight - warpedYrounded - maxMatchingDistance - 2);
            }

            for(s32 offset=minOffset; offset<maxOffset; offset++) {
              const s32 xpRounded = warpedXrounded + offset;
              const s32 ypRounded = warpedYrounded;

              // TODO: make a binary search?
              for(s32 iMatch=pYStartIndexes[ypRounded]; iMatch<pYStartIndexes[ypRounded+1]; iMatch++) {
                if(xpRounded == pNewPoints[iMatch].x) {
                  //const f32 xp = warpedX + static_cast<f32>(offset);
                  const s32 xp_s32 = warpedX_s32 + offset;

                  const s32 aValues[8] = {xc_s32, yc_s32, 1, 0, 0, 0, -xc_s32*xp_s32, -yc_s32*xp_s32};

                  const s32 bValue = xp_s32;

                  for(s32 ia=0; ia<8; ia++) {
                    for(s32 ja=ia; ja<8; ja++) {
                      // 32x32 -> 64 MAC
                      AtA_raw[ia][ja] += aValues[ia] * aValues[ja];
                    }

                    // 32x32 -> 64 MAC
                    Atb_t_raw[ia] += aValues[ia] * bValue;
                  }
                }
              }
            } // for(s32 iOffset=-maxMatchingDistance; iOffset<=maxMatchingDistance; iOffset++)
          } // if(warpedYrounded >= maxMatchingDistance && warpedYrounded < (imageHeight-maxMatchingDistance))
        } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=ia; ja<8; ja++) {
            AtA[ia][ja] = static_cast<f32>(AtA_raw[ia][ja]);
          }
          Atb_t[0][ia] = static_cast<f32>(Atb_t_raw[ia]);
        }

        return RESULT_OK;
      } // NO_INLINE Result BinaryTracker::FindHorizontalCorrespondences_Projective_FixedPoint()

      Result BinaryTracker::IterativelyRefineTrack_Translation(
        const EdgeLists &nextImageEdges,
        const AllIndexLimits &allLimits,
        const s32 matching_maxDistance, const s32 matching_maxCorrespondences,
        MemoryStack scratch)
      {
        Result lastResult;

        s32 sumX_xDecreasing;
        s32 sumX_xIncreasing;
        s32 sumY_yDecreasing;
        s32 sumY_yIncreasing;

        s32 numX_xDecreasing;
        s32 numX_xIncreasing;
        s32 numY_yDecreasing;
        s32 numY_yIncreasing;

        lastResult = BinaryTracker::FindHorizontalCorrespondences_Translation_FixedPoint(
          matching_maxDistance,
          this->transformation,
          this->templateEdges.xDecreasing,
          nextImageEdges.xDecreasing,
          nextImageEdges.imageHeight,
          nextImageEdges.imageWidth,
          allLimits.xDecreasing_yStartIndexes,
          sumX_xDecreasing, numX_xDecreasing,
          scratch);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack", "FindHorizontalCorrespondences 1 failed");

        lastResult = BinaryTracker::FindHorizontalCorrespondences_Translation_FixedPoint(
          matching_maxDistance,
          this->transformation,
          this->templateEdges.xIncreasing,
          nextImageEdges.xIncreasing,
          nextImageEdges.imageHeight,
          nextImageEdges.imageWidth,
          allLimits.xIncreasing_yStartIndexes,
          sumX_xIncreasing, numX_xIncreasing,
          scratch);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack", "FindHorizontalCorrespondences 2 failed");

        lastResult = BinaryTracker::FindVerticalCorrespondences_Translation_FixedPoint(
          matching_maxDistance,
          this->transformation,
          this->templateEdges.yDecreasing,
          nextImageEdges.yDecreasing,
          nextImageEdges.imageHeight,
          nextImageEdges.imageWidth,
          allLimits.yDecreasing_xStartIndexes,
          sumY_yDecreasing, numY_yDecreasing,
          scratch);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack", "FindVerticalCorrespondences 1 failed");

        lastResult = BinaryTracker::FindVerticalCorrespondences_Translation_FixedPoint(
          matching_maxDistance,
          this->transformation,
          this->templateEdges.yIncreasing,
          nextImageEdges.yIncreasing,
          nextImageEdges.imageHeight,
          nextImageEdges.imageWidth,
          allLimits.yIncreasing_xStartIndexes,
          sumY_yIncreasing, numY_yIncreasing,
          scratch);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack", "FindVerticalCorrespondences 2 failed");

        // Update the transformation
        {
          Array<f32> update(1,2,scratch);

          const s32 sumX = sumX_xDecreasing + sumX_xIncreasing;
          const s32 numX = numX_xDecreasing + numX_xIncreasing;

          const s32 sumY = sumY_yDecreasing + sumY_yIncreasing;
          const s32 numY = numY_yDecreasing + numY_yIncreasing;

          if(numX < 1 || numY < 1)
            return RESULT_OK;

          update[0][0] = f32(-sumX) / f32(numX);
          update[0][1] = f32(-sumY) / f32(numY);

          //update.Print("update");

          this->transformation.Update(update, scratch, Transformations::TRANSFORM_TRANSLATION);
        }

        return RESULT_OK;
      } // Result BinaryTracker::IterativelyRefineTrack_Translation()

      Result BinaryTracker::IterativelyRefineTrack_Projective(
        const EdgeLists &nextImageEdges,
        const AllIndexLimits &allLimits,
        const s32 matching_maxDistance, const s32 matching_maxCorrespondences,
        const bool useFixedPoint,
        MemoryStack scratch)
      {
        Result lastResult;

        Array<f32> AtA_xDecreasing(8,8,scratch);
        Array<f32> AtA_xIncreasing(8,8,scratch);
        Array<f32> AtA_yDecreasing(8,8,scratch);
        Array<f32> AtA_yIncreasing(8,8,scratch);

        Array<f32> Atb_t_xDecreasing(1,8,scratch);
        Array<f32> Atb_t_xIncreasing(1,8,scratch);
        Array<f32> Atb_t_yDecreasing(1,8,scratch);
        Array<f32> Atb_t_yIncreasing(1,8,scratch);

        if(useFixedPoint) {
          lastResult = BinaryTracker::FindHorizontalCorrespondences_Projective_FixedPoint(
            matching_maxDistance, this->transformation,
            this->templateEdges.xDecreasing, nextImageEdges.xDecreasing,
            nextImageEdges.imageHeight, nextImageEdges.imageWidth,
            allLimits.xDecreasing_yStartIndexes, AtA_xDecreasing, Atb_t_xDecreasing, scratch);
        } else { // if(useFixedPoint)
          lastResult = BinaryTracker::FindHorizontalCorrespondences_Projective(
            matching_maxDistance, this->transformation,
            this->templateEdges.xDecreasing, nextImageEdges.xDecreasing,
            nextImageEdges.imageHeight, nextImageEdges.imageWidth,
            allLimits.xDecreasing_yStartIndexes, AtA_xDecreasing, Atb_t_xDecreasing, scratch);
        } // if(useFixedPoint) ... else

        //AtA_xDecreasing.Print("AtA_xDecreasing");
        //Atb_t_xDecreasing.Print("Atb_t_xDecreasing");

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack", "FindHorizontalCorrespondences 1 failed");

        if(useFixedPoint) {
          lastResult = BinaryTracker::FindHorizontalCorrespondences_Projective_FixedPoint(
            matching_maxDistance, this->transformation,
            this->templateEdges.xIncreasing, nextImageEdges.xIncreasing,
            nextImageEdges.imageHeight, nextImageEdges.imageWidth,
            allLimits.xIncreasing_yStartIndexes, AtA_xIncreasing, Atb_t_xIncreasing, scratch);
        } else { // if(useFixedPoint)
          lastResult = BinaryTracker::FindHorizontalCorrespondences_Projective(
            matching_maxDistance, this->transformation,
            this->templateEdges.xIncreasing, nextImageEdges.xIncreasing,
            nextImageEdges.imageHeight, nextImageEdges.imageWidth,
            allLimits.xIncreasing_yStartIndexes, AtA_xIncreasing, Atb_t_xIncreasing, scratch);
        } // if(useFixedPoint) ... else

        //AtA_xIncreasing.Print("AtA_xIncreasing");
        //Atb_t_xIncreasing.Print("Atb_t_xIncreasing");

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack", "FindHorizontalCorrespondences 2 failed");

        if(useFixedPoint) {
          lastResult = BinaryTracker::FindVerticalCorrespondences_Projective_FixedPoint(
            matching_maxDistance, this->transformation,
            this->templateEdges.yDecreasing, nextImageEdges.yDecreasing,
            nextImageEdges.imageHeight, nextImageEdges.imageWidth,
            allLimits.yDecreasing_xStartIndexes, AtA_yDecreasing, Atb_t_yDecreasing, scratch);
        } else { // if(useFixedPoint)
          lastResult = BinaryTracker::FindVerticalCorrespondences_Projective(
            matching_maxDistance, this->transformation,
            this->templateEdges.yDecreasing, nextImageEdges.yDecreasing,
            nextImageEdges.imageHeight, nextImageEdges.imageWidth,
            allLimits.yDecreasing_xStartIndexes, AtA_yDecreasing, Atb_t_yDecreasing, scratch);
        } // if(useFixedPoint) ... else

        //AtA_yDecreasing.Print("AtA_yDecreasing");
        //Atb_t_yDecreasing.Print("Atb_t_yDecreasing");

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack", "FindVerticalCorrespondences 1 failed");

        if(useFixedPoint) {
          lastResult = BinaryTracker::FindVerticalCorrespondences_Projective_FixedPoint(
            matching_maxDistance, this->transformation,
            this->templateEdges.yIncreasing, nextImageEdges.yIncreasing,
            nextImageEdges.imageHeight, nextImageEdges.imageWidth,
            allLimits.yIncreasing_xStartIndexes, AtA_yIncreasing, Atb_t_yIncreasing, scratch);
        } else { // if(useFixedPoint)
          lastResult = BinaryTracker::FindVerticalCorrespondences_Projective(
            matching_maxDistance, this->transformation,
            this->templateEdges.yIncreasing, nextImageEdges.yIncreasing,
            nextImageEdges.imageHeight, nextImageEdges.imageWidth,
            allLimits.yIncreasing_xStartIndexes, AtA_yIncreasing, Atb_t_yIncreasing, scratch);
        } // if(useFixedPoint) ... else

        //AtA_yIncreasing.Print("AtA_yIncreasing");
        //Atb_t_yIncreasing.Print("Atb_t_yIncreasing");

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack", "FindVerticalCorrespondences 2 failed");

        // Update the transformation
        {
          Array<f32> newHomography(3, 3, scratch);

          Array<f32> AtA(8,8,scratch);
          Array<f32> Atb_t(1,8,scratch);

          // The total AtA and Atb matrices are just the elementwise sums of their partial versions
          for(s32 y=0; y<8; y++) {
            for(s32 x=0; x<8; x++) {
              AtA[y][x] = AtA_xDecreasing[y][x] + AtA_xIncreasing[y][x] + AtA_yDecreasing[y][x] + AtA_yIncreasing[y][x];
            }

            Atb_t[0][y] = Atb_t_xDecreasing[0][y] + Atb_t_xIncreasing[0][y] + Atb_t_yDecreasing[0][y] + Atb_t_yIncreasing[0][y];
          }

          Matrix::MakeSymmetric(AtA, false);

          //AtA.Print("AtA");
          //Atb_t.Print("Atb_t");

          lastResult = Matrix::SolveLeastSquaresWithCholesky<f32>(AtA, Atb_t, false);

          //Atb_t.Print("result");

          AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
            lastResult, "BinaryTracker::UpdateTransformation", "SolveLeastSquaresWithCholesky failed");

          const f32 * restrict pAtb_t = Atb_t.Pointer(0,0);

          newHomography[0][0] = pAtb_t[0]; newHomography[0][1] = pAtb_t[1]; newHomography[0][2] = pAtb_t[2];
          newHomography[1][0] = pAtb_t[3]; newHomography[1][1] = pAtb_t[4]; newHomography[1][2] = pAtb_t[5];
          newHomography[2][0] = pAtb_t[6]; newHomography[2][1] = pAtb_t[7]; newHomography[2][2] = 1.0f;

          //newHomography.Print("newHomography");

          this->transformation.set_homography(newHomography);
        }

        return RESULT_OK;
      } // Result BinaryTracker::IterativelyRefineTrack_Projective()
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki
