/**
File: lucasKanade_SampledProjective.cpp
Author: Peter Barnum
Created: 2014-03-25

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/vision/robot/lucasKanade.h"
#include "coretech/common/robot/matlabInterface.h"
#include "coretech/common/robot/interpolate.h"
#include "coretech/common/robot/arrayPatterns.h"
#include "coretech/common/robot/find.h"
#include "coretech/common/robot/benchmarking.h"
#include "coretech/common/robot/draw.h"
#include "coretech/common/robot/comparisons.h"
#include "coretech/common/robot/errorHandling.h"

#include "coretech/vision/robot/fiducialDetection.h"
#include "coretech/vision/robot/imageProcessing.h"
#include "coretech/vision/robot/transformations.h"

//#define SEND_BINARY_IMAGES_TO_MATLAB

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
      LucasKanadeTracker_SampledProjective::LucasKanadeTracker_SampledProjective()
      : LucasKanadeTracker_Generic(Transformations::TransformType::TRANSFORM_PROJECTIVE)
      {
      }

      LucasKanadeTracker_SampledProjective::LucasKanadeTracker_SampledProjective(
        const Array<u8> &templateImage,
        const Quadrilateral<f32> &templateQuad,
        const f32 scaleTemplateRegionPercent,
        const s32 numPyramidLevels,
        const Transformations::TransformType transformType,
        const s32 maxSamplesAtBaseLevel,
        MemoryStack ccmMemory,
        MemoryStack &onchipScratch,
        MemoryStack offchipScratch)
        : LucasKanadeTracker_Generic(Transformations::TRANSFORM_PROJECTIVE, templateImage, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, transformType, onchipScratch)
      {
        const s32 numSelectBins = 20;

        Result lastResult;

        BeginBenchmark("LucasKanadeTracker_SampledProjective");

        this->templateSamplePyramid = FixedLengthList<FixedLengthList<TemplateSample> >(numPyramidLevels, onchipScratch);

        for(s32 iScale=0; iScale<numPyramidLevels; iScale++) {
          const f32 scale = static_cast<f32>(1 << iScale);

          Meshgrid<f32> curTemplateCoordinates = Meshgrid<f32>(
            Linspace(-this->templateRegionWidth/2.0f, this->templateRegionWidth/2.0f, static_cast<s32>(floorf(this->templateRegionWidth/scale))),
            Linspace(-this->templateRegionHeight/2.0f, this->templateRegionHeight/2.0f, static_cast<s32>(floorf(this->templateRegionHeight/scale))));

          // Half the sample at each subsequent level (not a quarter)
          const s32 maxPossibleLocations = curTemplateCoordinates.get_numElements();
          const s32 curMaxSamples = MIN(maxPossibleLocations, maxSamplesAtBaseLevel >> iScale);

          this->templateSamplePyramid[iScale] = FixedLengthList<TemplateSample>(curMaxSamples, onchipScratch);
          this->templateSamplePyramid[iScale].set_size(curMaxSamples);
        }

        //
        // Temporary allocations below this point
        //
        {
          // This section is based off lucasKanade_Fast, except uses f32 in offchip instead of integer types in onchip

          FixedLengthList<Meshgrid<f32> > templateCoordinates = FixedLengthList<Meshgrid<f32> >(numPyramidLevels, onchipScratch);
          FixedLengthList<Array<f32> > templateImagePyramid = FixedLengthList<Array<f32> >(numPyramidLevels, offchipScratch);
          FixedLengthList<Array<f32> > templateImageXGradientPyramid = FixedLengthList<Array<f32> >(numPyramidLevels, offchipScratch);
          FixedLengthList<Array<f32> > templateImageYGradientPyramid = FixedLengthList<Array<f32> >(numPyramidLevels, offchipScratch);
          FixedLengthList<Array<f32> > templateImageSquaredGradientMagnitudePyramid = FixedLengthList<Array<f32> >(numPyramidLevels, offchipScratch);

          templateCoordinates.set_size(numPyramidLevels);
          templateImagePyramid.set_size(numPyramidLevels);
          templateImageXGradientPyramid.set_size(numPyramidLevels);
          templateImageYGradientPyramid.set_size(numPyramidLevels);
          templateImageSquaredGradientMagnitudePyramid.set_size(numPyramidLevels);

          AnkiConditionalErrorAndReturn(templateImagePyramid.IsValid() && templateImageXGradientPyramid.IsValid() && templateImageYGradientPyramid.IsValid() && templateCoordinates.IsValid() && templateImageSquaredGradientMagnitudePyramid.IsValid(),
            "LucasKanadeTracker_SampledProjective::LucasKanadeTracker_SampledProjective", "Could not allocate pyramid lists");

          // Allocate the memory for all the images
          for(s32 iScale=0; iScale<numPyramidLevels; iScale++) {
            const f32 scale = static_cast<f32>(1 << iScale);

            templateCoordinates[iScale] = Meshgrid<f32>(
              Linspace(-this->templateRegionWidth/2.0f, this->templateRegionWidth/2.0f, static_cast<s32>(floorf(this->templateRegionWidth/scale))),
              Linspace(-this->templateRegionHeight/2.0f, this->templateRegionHeight/2.0f, static_cast<s32>(floorf(this->templateRegionHeight/scale))));

            const s32 numPointsY = templateCoordinates[iScale].get_yGridVector().get_size();
            const s32 numPointsX = templateCoordinates[iScale].get_xGridVector().get_size();

            templateImagePyramid[iScale] = Array<f32>(numPointsY, numPointsX, offchipScratch);
            templateImageXGradientPyramid[iScale] = Array<f32>(numPointsY, numPointsX, offchipScratch);
            templateImageYGradientPyramid[iScale] = Array<f32>(numPointsY, numPointsX, offchipScratch);
            templateImageSquaredGradientMagnitudePyramid[iScale] = Array<f32>(numPointsY, numPointsX, offchipScratch);

            AnkiConditionalErrorAndReturn(templateImagePyramid[iScale].IsValid() && templateImageXGradientPyramid[iScale].IsValid() && templateImageYGradientPyramid[iScale].IsValid() && templateImageSquaredGradientMagnitudePyramid[iScale].IsValid(),
              "LucasKanadeTracker_SampledProjective::LucasKanadeTracker_SampledProjective", "Could not allocate pyramid images");
          }

          // Sample all levels of the pyramid images
          for(s32 iScale=0; iScale<numPyramidLevels; iScale++) {
            if((lastResult = Interp2_Affine<u8,f32>(templateImage, templateCoordinates[iScale], transformation.get_homography(), this->transformation.get_centerOffset(initialImageScaleF32), templateImagePyramid[iScale], INTERPOLATE_LINEAR)) != RESULT_OK) {
              AnkiError("LucasKanadeTracker_SampledProjective::LucasKanadeTracker_SampledProjective", "Interp2_Affine failed with code 0x%x", lastResult);
              return;
            }
          }

          // Compute the spatial derivatives
          // TODO: compute without borders?
          for(s32 iScale=0; iScale<numPyramidLevels; iScale++) {
            PUSH_MEMORY_STACK(offchipScratch);
            PUSH_MEMORY_STACK(onchipScratch);

            const s32 numPointsY = templateCoordinates[iScale].get_yGridVector().get_size();
            const s32 numPointsX = templateCoordinates[iScale].get_xGridVector().get_size();

            if((lastResult = ImageProcessing::ComputeXGradient<f32,f32,f32>(templateImagePyramid[iScale], templateImageXGradientPyramid[iScale])) != RESULT_OK) {
              AnkiError("LucasKanadeTracker_SampledProjective::LucasKanadeTracker_SampledProjective", "ComputeXGradient failed with code 0x%x", lastResult);
              return;
            }

            if((lastResult = ImageProcessing::ComputeYGradient<f32,f32,f32>(templateImagePyramid[iScale], templateImageYGradientPyramid[iScale])) != RESULT_OK) {
              AnkiError("LucasKanadeTracker_SampledProjective::LucasKanadeTracker_SampledProjective", "ComputeYGradient failed with code 0x%x", lastResult);
              return;
            }

            // Using the computed gradients, find a set of the max values, and store them

            Array<f32> tmpMagnitude(numPointsY, numPointsX, offchipScratch);

            Matrix::DotMultiply<f32,f32,f32>(templateImageXGradientPyramid[iScale], templateImageXGradientPyramid[iScale], tmpMagnitude);
            Matrix::DotMultiply<f32,f32,f32>(templateImageYGradientPyramid[iScale], templateImageYGradientPyramid[iScale], templateImageSquaredGradientMagnitudePyramid[iScale]);
            Matrix::Add<f32,f32,f32>(tmpMagnitude, templateImageSquaredGradientMagnitudePyramid[iScale], templateImageSquaredGradientMagnitudePyramid[iScale]);

            //Matrix::Sqrt<f32,f32,f32>(templateImageSquaredGradientMagnitudePyramid[iScale], templateImageSquaredGradientMagnitudePyramid[iScale]);

            Array<f32> magnitudeVector = Matrix::Vectorize<f32,f32>(false, templateImageSquaredGradientMagnitudePyramid[iScale], offchipScratch);
            Array<u16> magnitudeIndexes = Array<u16>(1, numPointsY*numPointsX, offchipScratch);

            AnkiConditionalErrorAndReturn(magnitudeVector.IsValid() && magnitudeIndexes.IsValid(),
              "LucasKanadeTracker_SampledProjective::LucasKanadeTracker_SampledProjective", "Out of memory");

            // Really slow
            //const f32 t0 = GetTimeF32();
            //Matrix::Sort<f32>(magnitudeVector, magnitudeIndexes, 1, false);
            //const f32 t1 = GetTimeF32();

            const s32 numSamples = this->templateSamplePyramid[iScale].get_size();
            s32 numSelected;
            LucasKanadeTracker_SampledProjective::ApproximateSelect(magnitudeVector, numSelectBins, numSamples, numSelected, magnitudeIndexes);

            //const f32 t2 = GetTimeF32();
            //CoreTechPrint("%f %f\n", t1-t0, t2-t1);

            if(numSelected == 0) {
              return;
            }

            //{
            //  Matlab matlab(false);
            //  matlab.PutArray(magnitudeVector, "magnitudeVector");
            //  matlab.PutArray(magnitudeIndexes, "magnitudeIndexes");
            //}

            Array<f32> yCoordinatesVector = templateCoordinates[iScale].EvaluateY1(false, offchipScratch);
            Array<f32> xCoordinatesVector = templateCoordinates[iScale].EvaluateX1(false, offchipScratch);

            Array<f32> yGradientVector = Matrix::Vectorize<f32,f32>(false, templateImageYGradientPyramid[iScale], offchipScratch);
            Array<f32> xGradientVector = Matrix::Vectorize<f32,f32>(false, templateImageXGradientPyramid[iScale], offchipScratch);
            Array<f32> grayscaleVector = Matrix::Vectorize<f32,f32>(false, templateImagePyramid[iScale], offchipScratch);

            const f32 * restrict pYCoordinates = yCoordinatesVector.Pointer(0,0);
            const f32 * restrict pXCoordinates = xCoordinatesVector.Pointer(0,0);
            const f32 * restrict pYGradientVector = yGradientVector.Pointer(0,0);
            const f32 * restrict pXGradientVector = xGradientVector.Pointer(0,0);
            const f32 * restrict pGrayscaleVector = grayscaleVector.Pointer(0,0);
            const u16 * restrict pMagnitudeIndexes = magnitudeIndexes.Pointer(0,0);

            TemplateSample * restrict pTemplateSamplePyramid = this->templateSamplePyramid[iScale].Pointer(0);

            for(s32 iSample=0; iSample<numSamples; iSample++){
              const s32 curIndex = pMagnitudeIndexes[iSample];

              TemplateSample curSample;
              curSample.xCoordinate = pXCoordinates[curIndex];
              curSample.yCoordinate = pYCoordinates[curIndex];
              curSample.xGradient = pXGradientVector[curIndex];
              curSample.yGradient = pYGradientVector[curIndex];
              curSample.grayvalue = pGrayscaleVector[curIndex];

              pTemplateSamplePyramid[iSample] = curSample;
            }
          }
        } // PUSH_MEMORY_STACK(fastMemory);

        this->isValid = true;

        EndBenchmark("LucasKanadeTracker_SampledProjective");
      }

      Result LucasKanadeTracker_SampledProjective::ShowTemplate(const char * windowName, const bool waitForKeypress, const bool fitImageToWindow) const
      {
#if !ANKICORETECH_EMBEDDED_USE_OPENCV
        return RESULT_FAIL;
#else
        //if(!this->IsValid())
        //  return RESULT_FAIL;

        const s32 scratchSize = 10000000;
        MemoryStack scratch(malloc(scratchSize), scratchSize);

        Array<u8> image(this->templateImageHeight, this->templateImageWidth, scratch);

        const Point<f32> centerOffset = this->transformation.get_centerOffset(1.0f);

        for(s32 iScale=0; iScale<numPyramidLevels; iScale++) {
          const s32 numSamples = this->templateSamplePyramid[iScale].get_size();

          image.SetZero();

          const TemplateSample * restrict pTemplateSample = this->templateSamplePyramid[iScale].Pointer(0);

          for(s32 iSample=0; iSample<numSamples; iSample++) {
            const TemplateSample curTemplateSample = pTemplateSample[iSample];

            const s32 curY = Round<s32>(curTemplateSample.yCoordinate + centerOffset.y);
            const s32 curX = Round<s32>(curTemplateSample.xCoordinate + centerOffset.x);

            if(curX >= 0 && curY >= 0 && curX < this->templateImageWidth && curY < this->templateImageHeight) {
              image[curY][curX] = 255;
            }
          }

          char windowNameTotal[128];
          snprintf(windowNameTotal, 128, "%s %d", windowName, iScale);

          if(fitImageToWindow) {
            cv::namedWindow(windowNameTotal, CV_WINDOW_NORMAL);
          } else {
            cv::namedWindow(windowNameTotal, CV_WINDOW_AUTOSIZE);
          }

          cv::Mat_<u8> image_cvMat;
          ArrayToCvMat(image, &image_cvMat);
          cv::imshow(windowNameTotal, image_cvMat);
        }

        if(waitForKeypress)
          cv::waitKey();

        return RESULT_OK;
#endif // #if !ANKICORETECH_EMBEDDED_USE_OPENCV ... #else
      }

      bool LucasKanadeTracker_SampledProjective::IsValid() const
      {
        if(!LucasKanadeTracker_Generic::IsValid())
          return false;

        // TODO: add more checks

        return true;
      }

      s32 LucasKanadeTracker_SampledProjective::get_numTemplatePixels(const s32 whichScale) const
      {
        if(whichScale < 0 || whichScale > this->numPyramidLevels)
          return 0;

        return this->templateSamplePyramid[whichScale].get_size();
      }

      Result LucasKanadeTracker_SampledProjective::UpdateTrack(
        const Array<u8> &nextImage,
        const s32 maxIterations,
        const f32 convergenceTolerance,
        const u8 verify_maxPixelDifference,
        bool &verify_converged,
        s32 &verify_meanAbsoluteDifference,
        s32 &verify_numInBounds,
        s32 &verify_numSimilarPixels,
        MemoryStack scratch)
      {
        Result lastResult;

        for(s32 iScale=numPyramidLevels-1; iScale>=0; iScale--) {
          verify_converged = false;

          BeginBenchmark("UpdateTrack.refineTranslation");
          if((lastResult = IterativelyRefineTrack(nextImage, maxIterations, iScale, convergenceTolerance, Transformations::TRANSFORM_TRANSLATION, verify_converged, scratch)) != RESULT_OK)
            return lastResult;
          EndBenchmark("UpdateTrack.refineTranslation");

          if(this->transformation.get_transformType() != Transformations::TRANSFORM_TRANSLATION) {
            BeginBenchmark("UpdateTrack.refineOther");
            if((lastResult = IterativelyRefineTrack(nextImage, maxIterations, iScale, convergenceTolerance, this->transformation.get_transformType(), verify_converged, scratch)) != RESULT_OK)
              return lastResult;
            EndBenchmark("UpdateTrack.refineOther");
          }
        } // for(s32 iScale=numPyramidLevels; iScale>=0; iScale--)

        lastResult = this->VerifyTrack_Projective(nextImage, verify_maxPixelDifference, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratch);

        return lastResult;
      }

      Result LucasKanadeTracker_SampledProjective::IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const Transformations::TransformType curTransformType, bool &verify_converged, MemoryStack scratch)
      {
        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);

        AnkiConditionalErrorAndReturnValue(this->IsValid() == true,
          RESULT_FAIL, "LucasKanadeTracker_SampledProjective::IterativelyRefineTrack", "This object is not initialized");

        AnkiConditionalErrorAndReturnValue(nextImage.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTracker_SampledProjective::IterativelyRefineTrack", "nextImage is not valid");

        AnkiConditionalErrorAndReturnValue(maxIterations > 0 && maxIterations < 1000,
          RESULT_FAIL_INVALID_PARAMETER, "LucasKanadeTracker_SampledProjective::IterativelyRefineTrack", "maxIterations must be greater than zero and less than 1000");

        AnkiConditionalErrorAndReturnValue(whichScale >= 0 && whichScale < this->numPyramidLevels,
          RESULT_FAIL_INVALID_PARAMETER, "LucasKanadeTracker_SampledProjective::IterativelyRefineTrack", "whichScale is invalid");

        AnkiConditionalErrorAndReturnValue(convergenceTolerance > 0.0f,
          RESULT_FAIL_INVALID_PARAMETER, "LucasKanadeTracker_SampledProjective::IterativelyRefineTrack", "convergenceTolerance must be greater than zero");

        AnkiConditionalErrorAndReturnValue(nextImageHeight == templateImageHeight && nextImageWidth == templateImageWidth,
          RESULT_FAIL_INVALID_SIZE, "LucasKanadeTracker_SampledProjective::IterativelyRefineTrack", "nextImage must be the same size as the template");

        const s32 initialImageScaleS32 = baseImageWidth / nextImageWidth;
        const s32 initialImagePowerS32 = Log2u32(static_cast<u32>(initialImageScaleS32));

        AnkiConditionalErrorAndReturnValue(((1<<initialImagePowerS32)*nextImageWidth) == baseImageWidth,
          RESULT_FAIL_INVALID_SIZE, "LucasKanadeTracker_SampledProjective::IterativelyRefineTrack", "The templateImage must be a power of two smaller than baseImageWidth (%d)", baseImageWidth);

        if(curTransformType == Transformations::TRANSFORM_TRANSLATION) {
          return IterativelyRefineTrack_Translation(nextImage, maxIterations, whichScale, convergenceTolerance, verify_converged, scratch);
        } else if(curTransformType == Transformations::TRANSFORM_AFFINE) {
          return IterativelyRefineTrack_Affine(nextImage, maxIterations, whichScale, convergenceTolerance, verify_converged, scratch);
        } else if(curTransformType == Transformations::TRANSFORM_PROJECTIVE) {
          return IterativelyRefineTrack_Projective(nextImage, maxIterations, whichScale, convergenceTolerance, verify_converged, scratch);
        }

        return RESULT_FAIL;
      } // Result LucasKanadeTracker_SampledProjective::IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const TransformType curTransformType, bool &verify_converged, MemoryStack scratch)

      Result LucasKanadeTracker_SampledProjective::IterativelyRefineTrack_Translation(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &verify_converged, MemoryStack scratch)
      {
        // This method is heavily based on Interp2_Projective
        // The call would be like: Interp2_Projective<u8,u8>(nextImage, originalCoordinates, interpolationHomography, centerOffset, nextImageTransformed2d, INTERPOLATE_LINEAR, 0);

        Result lastResult;

        Array<f32> AWAt(2, 2, scratch);
        Array<f32> b(1, 2, scratch);

        f32 &AWAt00 = AWAt[0][0];
        f32 &AWAt01 = AWAt[0][1];
        //f32 &AWAt10 = AWAt[1][0];
        f32 &AWAt11 = AWAt[1][1];

        f32 &b0 = b[0][0];
        f32 &b1 = b[0][1];

        verify_converged = false;

        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);

        const f32 scale = static_cast<f32>(1 << whichScale);

        const s32 initialImageScaleS32 = baseImageWidth / nextImageWidth;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);

        const f32 oneOverTwoFiftyFive = 1.0f / 255.0f;
        const f32 scaleOverFiveTen = scale / (2.0f*255.0f);

        //const Point<f32>& centerOffset = this->transformation.get_centerOffset();
        const Point<f32> centerOffsetScaled = this->transformation.get_centerOffset(initialImageScaleF32);

        // Initialize with some very extreme coordinates
        FixedLengthList<Quadrilateral<f32> > previousCorners(NUM_PREVIOUS_QUADS_TO_COMPARE, scratch);

        for(s32 i=0; i<NUM_PREVIOUS_QUADS_TO_COMPARE; i++) {
          previousCorners[i] = Quadrilateral<f32>(Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f));
        }

        const f32 xyReferenceMin = 0.0f;
        const f32 xReferenceMax = static_cast<f32>(nextImageWidth) - 1.0f;
        const f32 yReferenceMax = static_cast<f32>(nextImageHeight) - 1.0f;

        const TemplateSample * restrict pTemplateSamplePyramid = this->templateSamplePyramid[whichScale].Pointer(0);

        const s32 numTemplateSamples = this->get_numTemplatePixels(whichScale);

        for(s32 iteration=0; iteration<maxIterations; iteration++) {
          const Array<f32> &homography = this->transformation.get_homography();
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2] / initialImageScaleF32;
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2] / initialImageScaleF32;
          const f32 h20 = homography[2][0] * initialImageScaleF32; const f32 h21 = homography[2][1] * initialImageScaleF32; //const f32 h22 = 1.0f;

          AWAt.SetZero();
          b.SetZero();

          s32 numInBounds = 0;

          // TODO: make the x and y limits from 1 to end-2

          for(s32 iSample=0; iSample<numTemplateSamples; iSample++) {
            const TemplateSample curSample = pTemplateSamplePyramid[iSample];
            const f32 yOriginal = curSample.yCoordinate;
            const f32 xOriginal = curSample.xCoordinate;

            // TODO: These two could be strength reduced
            const f32 xTransformedRaw = h00*xOriginal + h01*yOriginal + h02;
            const f32 yTransformedRaw = h10*xOriginal + h11*yOriginal + h12;

            const f32 normalization = h20*xOriginal + h21*yOriginal + 1.0f;

            const f32 xTransformed = (xTransformedRaw / normalization) + centerOffsetScaled.x;
            const f32 yTransformed = (yTransformedRaw / normalization) + centerOffsetScaled.y;

            const f32 x0 = floorf(xTransformed);
            const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;

            const f32 y0 = floorf(yTransformed);
            const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;

            // If out of bounds, continue
            if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
              continue;
            }

            numInBounds++;

            const f32 alphaX = xTransformed - x0;
            const f32 alphaXinverse = 1 - alphaX;

            const f32 alphaY = yTransformed - y0;
            const f32 alphaYinverse = 1.0f - alphaY;

            const s32 y0S32 = Round<s32>(y0);
            const s32 y1S32 = Round<s32>(y1);
            const s32 x0S32 = Round<s32>(x0);

            const u8 * restrict pReference_y0 = nextImage.Pointer(y0S32, x0S32);
            const u8 * restrict pReference_y1 = nextImage.Pointer(y1S32, x0S32);

            const f32 pixelTL = *pReference_y0;
            const f32 pixelTR = *(pReference_y0+1);
            const f32 pixelBL = *pReference_y1;
            const f32 pixelBR = *(pReference_y1+1);

            const f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);

            //const u8 interpolatedPixel = static_cast<u8>(Round(interpolatedPixelF32));

            // This block is the non-interpolation part of the per-sample algorithm
            {
              const f32 templatePixelValue = curSample.grayvalue;
              const f32 xGradientValue = scaleOverFiveTen * curSample.xGradient;
              const f32 yGradientValue = scaleOverFiveTen * curSample.yGradient;

              const f32 tGradientValue = oneOverTwoFiftyFive * (interpolatedPixelF32 - templatePixelValue);

              //AWAt
              //  b
              AWAt00 += xGradientValue * xGradientValue;
              AWAt01 += xGradientValue * yGradientValue;
              AWAt11 += yGradientValue * yGradientValue;

              b0 += xGradientValue * tGradientValue;
              b1 += yGradientValue * tGradientValue;
            }
          } // for(s32 iSample=0; iSample<numTemplateSamples; iSample++)

          if(numInBounds < 16) {
            AnkiWarn("LucasKanadeTracker_SampledProjective::IterativelyRefineTrack_Translation", "Template drifted too far out of image.");
            return RESULT_OK;
          }

          Matrix::MakeSymmetric(AWAt, false);

          //AWAt.Print("New AWAt");
          //b.Print("New b");

          bool numericalFailure;

          if((lastResult = Matrix::SolveLeastSquaresWithCholesky(AWAt, b, false, numericalFailure)) != RESULT_OK)
            return lastResult;

          if(numericalFailure){
            AnkiWarn("LucasKanadeTracker_SampledProjective::IterativelyRefineTrack_Translation", "numericalFailure");
            return RESULT_OK;
          }

          //b.Print("New update");

          this->transformation.Update(b, initialImageScaleF32, scratch, Transformations::TRANSFORM_TRANSLATION);

          // Check if we're done with iterations
          const f32 minChange = UpdatePreviousCorners(transformation, baseImageWidth, baseImageHeight, previousCorners, scratch);

          if(minChange < convergenceTolerance) {
            verify_converged = true;
            return RESULT_OK;
          }
        } // for(s32 iteration=0; iteration<maxIterations; iteration++)

        return RESULT_OK;
      } // Result LucasKanadeTracker_SampledProjective::IterativelyRefineTrack_Translation()

      Result LucasKanadeTracker_SampledProjective::IterativelyRefineTrack_Affine(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &verify_converged, MemoryStack scratch)
      {
        // This method is heavily based on Interp2_Projective
        // The call would be like: Interp2_Projective<u8,u8>(nextImage, originalCoordinates, interpolationHomography, centerOffset, nextImageTransformed2d, INTERPOLATE_LINEAR, 0);

        Result lastResult;

        Array<f32> AWAt(6, 6, scratch);
        Array<f32> b(1, 6, scratch);

        // These addresses should be known at compile time, so should be faster
        f32 AWAt_raw[6][6];
        f32 b_raw[6];

        verify_converged = false;

        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);

        const f32 scale = static_cast<f32>(1 << whichScale);

        const s32 initialImageScaleS32 = baseImageWidth / nextImageWidth;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);

        const f32 oneOverTwoFiftyFive = 1.0f / 255.0f;
        const f32 scaleOverFiveTen = scale / (2.0f*255.0f);

        //const Point<f32>& centerOffset = this->transformation.get_centerOffset();
        const Point<f32> centerOffsetScaled = this->transformation.get_centerOffset(initialImageScaleF32);

        // Initialize with some very extreme coordinates
        FixedLengthList<Quadrilateral<f32> > previousCorners(NUM_PREVIOUS_QUADS_TO_COMPARE, scratch);

        for(s32 i=0; i<NUM_PREVIOUS_QUADS_TO_COMPARE; i++) {
          previousCorners[i] = Quadrilateral<f32>(Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f));
        }

        const f32 xyReferenceMin = 0.0f;
        const f32 xReferenceMax = static_cast<f32>(nextImageWidth) - 1.0f;
        const f32 yReferenceMax = static_cast<f32>(nextImageHeight) - 1.0f;

        const TemplateSample * restrict pTemplateSamplePyramid = this->templateSamplePyramid[whichScale].Pointer(0);

        const s32 numTemplateSamples = this->get_numTemplatePixels(whichScale);

        for(s32 iteration=0; iteration<maxIterations; iteration++) {
          const Array<f32> &homography = this->transformation.get_homography();
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2] / initialImageScaleF32;
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2] / initialImageScaleF32;
          const f32 h20 = homography[2][0] * initialImageScaleF32; const f32 h21 = homography[2][1] * initialImageScaleF32; //const f32 h22 = 1.0f;

          //AWAt.SetZero();
          //b.SetZero();

          for(s32 ia=0; ia<6; ia++) {
            for(s32 ja=0; ja<6; ja++) {
              AWAt_raw[ia][ja] = 0;
            }
            b_raw[ia] = 0;
          }

          s32 numInBounds = 0;

          for(s32 iSample=0; iSample<numTemplateSamples; iSample++) {
            const TemplateSample curSample = pTemplateSamplePyramid[iSample];
            const f32 yOriginal = curSample.yCoordinate;
            const f32 xOriginal = curSample.xCoordinate;

            // TODO: These two could be strength reduced
            const f32 xTransformedRaw = h00*xOriginal + h01*yOriginal + h02;
            const f32 yTransformedRaw = h10*xOriginal + h11*yOriginal + h12;

            const f32 normalization = h20*xOriginal + h21*yOriginal + 1.0f;

            const f32 xTransformed = (xTransformedRaw / normalization) + centerOffsetScaled.x;
            const f32 yTransformed = (yTransformedRaw / normalization) + centerOffsetScaled.y;

            const f32 x0 = floorf(xTransformed);
            const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;

            const f32 y0 = floorf(yTransformed);
            const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;

            // If out of bounds, continue
            if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
              continue;
            }

            numInBounds++;

            const f32 alphaX = xTransformed - x0;
            const f32 alphaXinverse = 1 - alphaX;

            const f32 alphaY = yTransformed - y0;
            const f32 alphaYinverse = 1.0f - alphaY;

            const s32 y0S32 = Round<s32>(y0);
            const s32 y1S32 = Round<s32>(y1);
            const s32 x0S32 = Round<s32>(x0);

            const u8 * restrict pReference_y0 = nextImage.Pointer(y0S32, x0S32);
            const u8 * restrict pReference_y1 = nextImage.Pointer(y1S32, x0S32);

            const f32 pixelTL = *pReference_y0;
            const f32 pixelTR = *(pReference_y0+1);
            const f32 pixelBL = *pReference_y1;
            const f32 pixelBR = *(pReference_y1+1);

            const f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);

            //const u8 interpolatedPixel = static_cast<u8>(Round(interpolatedPixelF32));

            // This block is the non-interpolation part of the per-sample algorithm
            {
              const f32 templatePixelValue = curSample.grayvalue;
              const f32 xGradientValue = scaleOverFiveTen * curSample.xGradient;
              const f32 yGradientValue = scaleOverFiveTen * curSample.yGradient;

              const f32 tGradientValue = oneOverTwoFiftyFive * (interpolatedPixelF32 - templatePixelValue);

              //CoreTechPrint("%f ", xOriginal);
              const f32 values[6] = {
                xOriginal * xGradientValue,
                yOriginal * xGradientValue,
                xGradientValue,
                xOriginal * yGradientValue,
                yOriginal * yGradientValue,
                yGradientValue};

              //for(s32 ia=0; ia<6; ia++) {
              //  CoreTechPrint("%f ", values[ia]);
              //}
              //CoreTechPrint("\n");

              //f32 AWAt_raw[6][6];
              //f32 b_raw[6];
              for(s32 ia=0; ia<6; ia++) {
                for(s32 ja=ia; ja<6; ja++) {
                  AWAt_raw[ia][ja] += values[ia] * values[ja];
                }
                b_raw[ia] += values[ia] * tGradientValue;
              }
            }
          } // for(s32 iSample=0; iSample<numTemplateSamples; iSample++)

          if(numInBounds < 16) {
            AnkiWarn("LucasKanadeTracker_SampledProjective::IterativelyRefineTrack_Affine", "Template drifted too far out of image.");
            return RESULT_OK;
          }

          for(s32 ia=0; ia<6; ia++) {
            for(s32 ja=ia; ja<6; ja++) {
              AWAt[ia][ja] = AWAt_raw[ia][ja];
            }
            b[0][ia] = b_raw[ia];
          }

          Matrix::MakeSymmetric(AWAt, false);

          //AWAt.Print("New AWAt");
          //b.Print("New b");

          bool numericalFailure;

          if((lastResult = Matrix::SolveLeastSquaresWithCholesky(AWAt, b, false, numericalFailure)) != RESULT_OK)
            return lastResult;

          if(numericalFailure){
            AnkiWarn("LucasKanadeTracker_SampledProjective::IterativelyRefineTrack_Affine", "numericalFailure");
            return RESULT_OK;
          }

          //b.Print("New update");

          this->transformation.Update(b, initialImageScaleF32, scratch, Transformations::TRANSFORM_AFFINE);

          //this->transformation.get_homography().Print("new transformation");

          // Check if we're done with iterations
          const f32 minChange = UpdatePreviousCorners(transformation, baseImageWidth, baseImageHeight, previousCorners, scratch);

          if(minChange < convergenceTolerance) {
            verify_converged = true;
            return RESULT_OK;
          }
        } // for(s32 iteration=0; iteration<maxIterations; iteration++)

        return RESULT_OK;
      } // Result LucasKanadeTracker_SampledProjective::IterativelyRefineTrack_Affine()

      Result LucasKanadeTracker_SampledProjective::IterativelyRefineTrack_Projective(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &verify_converged, MemoryStack scratch)
      {
        // This method is heavily based on Interp2_Projective
        // The call would be like: Interp2_Projective<u8,u8>(nextImage, originalCoordinates, interpolationHomography, centerOffset, nextImageTransformed2d, INTERPOLATE_LINEAR, 0);

        Result lastResult;

        Array<f32> AWAt(8, 8, scratch);
        Array<f32> b(1, 8, scratch);

        // These addresses should be known at compile time, so should be faster
        f32 AWAt_raw[8][8];
        f32 b_raw[8];

        verify_converged = false;

        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);

        const f32 scale = static_cast<f32>(1 << whichScale);

        const s32 initialImageScaleS32 = baseImageWidth / nextImageWidth;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);

        const f32 oneOverTwoFiftyFive = 1.0f / 255.0f;
        const f32 scaleOverFiveTen = scale / (2.0f*255.0f);

        //const Point<f32>& centerOffset = this->transformation.get_centerOffset();
        const Point<f32> centerOffsetScaled = this->transformation.get_centerOffset(initialImageScaleF32);

        // Initialize with some very extreme coordinates
        FixedLengthList<Quadrilateral<f32> > previousCorners(NUM_PREVIOUS_QUADS_TO_COMPARE, scratch);

        for(s32 i=0; i<NUM_PREVIOUS_QUADS_TO_COMPARE; i++) {
          previousCorners[i] = Quadrilateral<f32>(Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f));
        }

        const f32 xyReferenceMin = 0.0f;
        const f32 xReferenceMax = static_cast<f32>(nextImageWidth) - 1.0f;
        const f32 yReferenceMax = static_cast<f32>(nextImageHeight) - 1.0f;

        const TemplateSample * restrict pTemplateSamplePyramid = this->templateSamplePyramid[whichScale].Pointer(0);

        const s32 numTemplateSamples = this->get_numTemplatePixels(whichScale);

        for(s32 iteration=0; iteration<maxIterations; iteration++) {
          const Array<f32> &homography = this->transformation.get_homography();
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2] / initialImageScaleF32;
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2] / initialImageScaleF32;
          const f32 h20 = homography[2][0] * initialImageScaleF32; const f32 h21 = homography[2][1] * initialImageScaleF32; //const f32 h22 = 1.0f;

          //AWAt.SetZero();
          //b.SetZero();

          for(s32 ia=0; ia<8; ia++) {
            for(s32 ja=0; ja<8; ja++) {
              AWAt_raw[ia][ja] = 0;
            }
            b_raw[ia] = 0;
          }

          s32 numInBounds = 0;

          // TODO: make the x and y limits from 1 to end-2

          for(s32 iSample=0; iSample<numTemplateSamples; iSample++) {
            const TemplateSample curSample = pTemplateSamplePyramid[iSample];
            const f32 yOriginal = curSample.yCoordinate;
            const f32 xOriginal = curSample.xCoordinate;

            // TODO: These two could be strength reduced
            const f32 xTransformedRaw = h00*xOriginal + h01*yOriginal + h02;
            const f32 yTransformedRaw = h10*xOriginal + h11*yOriginal + h12;

            const f32 normalization = h20*xOriginal + h21*yOriginal + 1.0f;

            const f32 xTransformed = (xTransformedRaw / normalization) + centerOffsetScaled.x;
            const f32 yTransformed = (yTransformedRaw / normalization) + centerOffsetScaled.y;

            const f32 x0 = floorf(xTransformed);
            const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;

            const f32 y0 = floorf(yTransformed);
            const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;

            // If out of bounds, continue
            if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
              continue;
            }

            numInBounds++;

            const f32 alphaX = xTransformed - x0;
            const f32 alphaXinverse = 1 - alphaX;

            const f32 alphaY = yTransformed - y0;
            const f32 alphaYinverse = 1.0f - alphaY;

            const s32 y0S32 = Round<s32>(y0);
            const s32 y1S32 = Round<s32>(y1);
            const s32 x0S32 = Round<s32>(x0);

            const u8 * restrict pReference_y0 = nextImage.Pointer(y0S32, x0S32);
            const u8 * restrict pReference_y1 = nextImage.Pointer(y1S32, x0S32);

            const f32 pixelTL = *pReference_y0;
            const f32 pixelTR = *(pReference_y0+1);
            const f32 pixelBL = *pReference_y1;
            const f32 pixelBR = *(pReference_y1+1);

            const f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);

            //const u8 interpolatedPixel = static_cast<u8>(Round(interpolatedPixelF32));

            // This block is the non-interpolation part of the per-sample algorithm
            {
              const f32 templatePixelValue = curSample.grayvalue;
              const f32 xGradientValue = scaleOverFiveTen * curSample.xGradient;
              const f32 yGradientValue = scaleOverFiveTen * curSample.yGradient;

              const f32 tGradientValue = oneOverTwoFiftyFive * (interpolatedPixelF32 - templatePixelValue);

              //CoreTechPrint("%f ", xOriginal);

              const f32 values[8] = {
                xOriginal * xGradientValue,
                yOriginal * xGradientValue,
                xGradientValue,
                xOriginal * yGradientValue,
                yOriginal * yGradientValue,
                yGradientValue,
                -xOriginal*xOriginal*xGradientValue - xOriginal*yOriginal*yGradientValue,
                -xOriginal*yOriginal*xGradientValue - yOriginal*yOriginal*yGradientValue};

              for(s32 ia=0; ia<8; ia++) {
                for(s32 ja=ia; ja<8; ja++) {
                  AWAt_raw[ia][ja] += values[ia] * values[ja];
                }
                b_raw[ia] += values[ia] * tGradientValue;
              }
            }
          } // for(s32 iSample=0; iSample<numTemplateSamples; iSample++)

          if(numInBounds < 16) {
            AnkiWarn("LucasKanadeTracker_SampledProjective::IterativelyRefineTrack_Projective", "Template drifted too far out of image.");
            return RESULT_OK;
          }

          for(s32 ia=0; ia<8; ia++) {
            for(s32 ja=ia; ja<8; ja++) {
              AWAt[ia][ja] = AWAt_raw[ia][ja];
            }
            b[0][ia] = b_raw[ia];
          }

          Matrix::MakeSymmetric(AWAt, false);

          //AWAt.Print("New AWAt");
          //b.Print("New b");

          bool numericalFailure;

          if((lastResult = Matrix::SolveLeastSquaresWithCholesky(AWAt, b, false, numericalFailure)) != RESULT_OK)
            return lastResult;

          if(numericalFailure){
            AnkiWarn("LucasKanadeTracker_SampledProjective::IterativelyRefineTrack_Projective", "numericalFailure");
            return RESULT_OK;
          }

          //b.Print("New update");

          this->transformation.Update(b, initialImageScaleF32, scratch, Transformations::TRANSFORM_PROJECTIVE);

          //this->transformation.get_homography().Print("new transformation");

          // Check if we're done with iterations
          // Check if we're done with iterations
          const f32 minChange = UpdatePreviousCorners(transformation, baseImageWidth, baseImageHeight, previousCorners, scratch);

          if(minChange < convergenceTolerance) {
            verify_converged = true;
            return RESULT_OK;
          }
        } // for(s32 iteration=0; iteration<maxIterations; iteration++)

        return RESULT_OK;
      } // Result LucasKanadeTracker_SampledProjective::IterativelyRefineTrack_Projective()

      Result LucasKanadeTracker_SampledProjective::VerifyTrack_Projective(
        const Array<u8> &nextImage,
        const u8 verify_maxPixelDifference,
        s32 &verify_meanAbsoluteDifference,
        s32 &verify_numInBounds,
        s32 &verify_numSimilarPixels,
        MemoryStack scratch) const
      {
        // This method is heavily based on Interp2_Projective
        // The call would be like: Interp2_Projective<u8,u8>(nextImage, originalCoordinates, interpolationHomography, centerOffset, nextImageTransformed2d, INTERPOLATE_LINEAR, 0);
        const s32 verify_maxPixelDifferenceS32 = verify_maxPixelDifference;

        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);

        const s32 whichScale = 0;

        const s32 initialImageScaleS32 = baseImageWidth / nextImageWidth;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);

        const Point<f32> centerOffsetScaled = this->transformation.get_centerOffset(initialImageScaleF32);

        // Initialize with some very extreme coordinates
        FixedLengthList<Quadrilateral<f32> > previousCorners(NUM_PREVIOUS_QUADS_TO_COMPARE, scratch);

        for(s32 i=0; i<NUM_PREVIOUS_QUADS_TO_COMPARE; i++) {
          previousCorners[i] = Quadrilateral<f32>(Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f));
        }

        const f32 xyReferenceMin = 0.0f;
        const f32 xReferenceMax = static_cast<f32>(nextImageWidth) - 1.0f;
        const f32 yReferenceMax = static_cast<f32>(nextImageHeight) - 1.0f;

        const TemplateSample * restrict pTemplateSamplePyramid = this->templateSamplePyramid[whichScale].Pointer(0);

        const s32 numTemplateSamples = this->get_numTemplatePixels(whichScale);

        const Array<f32> &homography = this->transformation.get_homography();
        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2] / initialImageScaleF32;
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2] / initialImageScaleF32;
        const f32 h20 = homography[2][0] * initialImageScaleF32; const f32 h21 = homography[2][1] * initialImageScaleF32; //const f32 h22 = 1.0f;

        verify_numInBounds = 0;
        verify_numSimilarPixels = 0;
        s32 totalGrayvalueDifference = 0;

        // TODO: make the x and y limits from 1 to end-2

        for(s32 iSample=0; iSample<numTemplateSamples; iSample++) {
          const TemplateSample curSample = pTemplateSamplePyramid[iSample];
          const f32 yOriginal = curSample.yCoordinate;
          const f32 xOriginal = curSample.xCoordinate;

          // TODO: These two could be strength reduced
          const f32 xTransformedRaw = h00*xOriginal + h01*yOriginal + h02;
          const f32 yTransformedRaw = h10*xOriginal + h11*yOriginal + h12;

          const f32 normalization = h20*xOriginal + h21*yOriginal + 1.0f;

          const f32 xTransformed = (xTransformedRaw / normalization) + centerOffsetScaled.x;
          const f32 yTransformed = (yTransformedRaw / normalization) + centerOffsetScaled.y;

          const f32 x0 = floorf(xTransformed);
          const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;

          const f32 y0 = floorf(yTransformed);
          const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;

          // If out of bounds, continue
          if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
            continue;
          }

          verify_numInBounds++;

          const f32 alphaX = xTransformed - x0;
          const f32 alphaXinverse = 1 - alphaX;

          const f32 alphaY = yTransformed - y0;
          const f32 alphaYinverse = 1.0f - alphaY;

          const s32 y0S32 = Round<s32>(y0);
          const s32 y1S32 = Round<s32>(y1);
          const s32 x0S32 = Round<s32>(x0);

          const u8 * restrict pReference_y0 = nextImage.Pointer(y0S32, x0S32);
          const u8 * restrict pReference_y1 = nextImage.Pointer(y1S32, x0S32);

          const f32 pixelTL = *pReference_y0;
          const f32 pixelTR = *(pReference_y0+1);
          const f32 pixelBL = *pReference_y1;
          const f32 pixelBR = *(pReference_y1+1);

          const s32 interpolatedPixelValue = Round<s32>(InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse));
          const s32 templatePixelValue = Round<s32>(curSample.grayvalue);
          const s32 grayvalueDifference = ABS(interpolatedPixelValue - templatePixelValue);

          totalGrayvalueDifference += grayvalueDifference;

          if(grayvalueDifference <= verify_maxPixelDifferenceS32) {
            verify_numSimilarPixels++;
          }
        } // for(s32 iSample=0; iSample<numTemplateSamples; iSample++)

        verify_meanAbsoluteDifference = totalGrayvalueDifference / verify_numInBounds;

        return RESULT_OK;
      }

      Result LucasKanadeTracker_SampledProjective::ApproximateSelect(const Array<f32> &magnitudeVector, const s32 numBins, const s32 numToSelect, s32 &numSelected, Array<u16> &magnitudeIndexes)
      {
        const f32 maxMagnitude = Matrix::Max<f32>(magnitudeVector);

        const f32 magnitudeIncrement = maxMagnitude / static_cast<f32>(numBins);

        // For each threshold, count the number above the threshold
        // If the number is low enough, copy the appropriate indexes and return

        const s32 numMagnitudes = magnitudeVector.get_size(1);

        const f32 * restrict pMagnitudeVector = magnitudeVector.Pointer(0,0);

        numSelected = 0;

        f32 foundThreshold = -1.0f;
        for(f32 threshold=0; threshold<maxMagnitude; threshold+=magnitudeIncrement) {
          s32 numAbove = 0;
          for(s32 i=0; i<numMagnitudes; i++) {
            if(pMagnitudeVector[i] > threshold) {
              numAbove++;
            }
          }

          if(numAbove <= numToSelect) {
            foundThreshold = threshold;
            break;
          }
        }

        if(foundThreshold < -0.1f) {
          AnkiWarn("LucasKanadeTracker_SampledProjective::ApproximateSelect", "Could not find valid threshold");
          magnitudeIndexes.SetZero();
          return RESULT_OK;
        }

        u16 * restrict pMagnitudeIndexes = magnitudeIndexes.Pointer(0,0);

        for(s32 i=0; i<numMagnitudes; i++) {
          if(pMagnitudeVector[i] > foundThreshold) {
            pMagnitudeIndexes[numSelected] = static_cast<u16>(i);
            numSelected++;
          }
        }

        return RESULT_OK;
      }
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki
