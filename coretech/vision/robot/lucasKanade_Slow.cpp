/**
File: lucasKanade_Slow.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
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
      LucasKanadeTracker_Slow::LucasKanadeTracker_Slow()
        : isValid(false), isInitialized(false)
      {
      }

      LucasKanadeTracker_Slow::LucasKanadeTracker_Slow(const Array<u8> &templateImage, const Quadrilateral<f32> &templateQuad, const f32 scaleTemplateRegionPercent, const s32 numPyramidLevels, const Transformations::TransformType transformType, const f32 ridgeWeight, MemoryStack &memory)
        : baseImageWidth(templateImage.get_size(1)), baseImageHeight(templateImage.get_size(0)), numPyramidLevels(numPyramidLevels), templateImageHeight(templateImage.get_size(0)), templateImageWidth(templateImage.get_size(1)), ridgeWeight(ridgeWeight), isValid(false), isInitialized(false)
      {
        BeginBenchmark("LucasKanadeTracker_Slow");

        AnkiConditionalErrorAndReturn(templateImageHeight > 0 && templateImageWidth > 0,
          "LucasKanadeTracker_Slow::LucasKanadeTracker_Slow", "template widths and heights must be greater than zero, and multiples of %d", ANKI_VISION_IMAGE_WIDTH_MULTIPLE);

        AnkiConditionalErrorAndReturn(numPyramidLevels > 0,
          "LucasKanadeTracker_Slow::LucasKanadeTracker_Slow", "numPyramidLevels must be greater than zero");

        AnkiConditionalErrorAndReturn(transformType==Transformations::TRANSFORM_TRANSLATION || transformType == Transformations::TRANSFORM_AFFINE || transformType==Transformations::TRANSFORM_PROJECTIVE,
          "LucasKanadeTracker_Slow::LucasKanadeTracker_Slow", "Only Transformations::TRANSFORM_TRANSLATION, Transformations::TRANSFORM_AFFINE, and Transformations::TRANSFORM_PROJECTIVE are supported");

        AnkiConditionalErrorAndReturn(ridgeWeight >= 0.0f,
          "LucasKanadeTracker_Slow::LucasKanadeTracker_Slow", "ridgeWeight must be greater or equal to zero");

        const s32 initialImageScaleS32 = baseImageWidth / templateImage.get_size(1);
        const s32 initialImagePowerS32 = Log2u32(static_cast<u32>(initialImageScaleS32));
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);

        AnkiConditionalErrorAndReturn(((1<<initialImagePowerS32)*templateImage.get_size(1)) == baseImageWidth,
          "LucasKanadeTracker_Slow::LucasKanadeTracker_Slow", "The templateImage must be a power of two smaller than baseImageWidth (%d)", baseImageWidth);

        templateRegion = templateQuad.ComputeBoundingRectangle<f32>().ComputeScaledRectangle<f32>(scaleTemplateRegionPercent);

        templateRegion.left /= initialImageScaleF32;
        templateRegion.right /= initialImageScaleF32;
        templateRegion.top /= initialImageScaleF32;
        templateRegion.bottom /= initialImageScaleF32;

        for(s32 i=1; i<(numPyramidLevels-1); i++) {
          const s32 curTemplateHeight = templateImageHeight >> i;
          const s32 curTemplateWidth = templateImageWidth >> i;

          AnkiConditionalErrorAndReturn(!IsOdd(curTemplateHeight) && !IsOdd(curTemplateWidth),
            "LucasKanadeTracker_Slow::LucasKanadeTracker_Slow", "Template widths and height must divisible by 2^numPyramidLevels");
        }

        A_full = FixedLengthList<Array<f32> >(numPyramidLevels, memory);

        AnkiConditionalErrorAndReturn(A_full.IsValid(),
          "LucasKanadeTracker_Slow::LucasKanadeTracker_Slow", "Could not allocate A_full");

        A_full.set_size(numPyramidLevels);

        templateCoordinates = FixedLengthList<Meshgrid<f32> >(numPyramidLevels, memory);

        AnkiConditionalErrorAndReturn(templateCoordinates.IsValid(),
          "LucasKanadeTracker_Slow::LucasKanadeTracker_Slow", "Could not allocate templateCoordinates");

        templateCoordinates.set_size(numPyramidLevels);

        templateImagePyramid = FixedLengthList<Array<u8> >(numPyramidLevels, memory);

        AnkiConditionalErrorAndReturn(templateImagePyramid.IsValid(),
          "LucasKanadeTracker_Slow::LucasKanadeTracker_Slow", "Could not allocate templateImagePyramid");

        templateImagePyramid.set_size(numPyramidLevels);

        templateWeights = FixedLengthList<Array<f32> >(numPyramidLevels, memory);

        AnkiConditionalErrorAndReturn(templateWeights.IsValid(),
          "LucasKanadeTracker_Slow::LucasKanadeTracker_Slow", "Could not allocate templateWeights");

        templateWeights.set_size(numPyramidLevels);

        this->transformation = Transformations::PlanarTransformation_f32(transformType, templateQuad, memory);

        this->isValid = true;

        BeginBenchmark("InitializeTemplate");
        if(LucasKanadeTracker_Slow::InitializeTemplate(templateImage, memory) != RESULT_OK) {
          this->isValid = false;
        }
        EndBenchmark("InitializeTemplate");

        EndBenchmark("LucasKanadeTracker_Slow");
      }

      Result LucasKanadeTracker_Slow::InitializeTemplate(const Array<u8> &templateImage, MemoryStack &memory)
      {
        const bool isOutColumnMajor = false; // TODO: change to false, which will probably be faster

        Result lastResult;

        AnkiConditionalErrorAndReturnValue(this->isValid,
          RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTracker_Slow::InitializeTemplate", "This object's constructor failed, so it cannot be initialized");

        AnkiConditionalErrorAndReturnValue(this->isInitialized == false,
          RESULT_FAIL, "LucasKanadeTracker_Slow::InitializeTemplate", "This object has already been initialized");

        AnkiConditionalErrorAndReturnValue(templateImageHeight == templateImage.get_size(0) && templateImageWidth == templateImage.get_size(1),
          RESULT_FAIL_INVALID_SIZE, "LucasKanadeTracker_Slow::InitializeTemplate", "template size doesn't match constructor");

        AnkiConditionalErrorAndReturnValue(templateRegion.left < templateRegion.right && templateRegion.left >=0 && templateRegion.right < templateImage.get_size(1) &&
          templateRegion.top < templateRegion.bottom && templateRegion.top >=0 && templateRegion.bottom < templateImage.get_size(0),
          RESULT_FAIL, "LucasKanadeTracker_Slow::InitializeTemplate", "template rectangle is invalid or out of bounds");

        // We do this early, before any memory is allocated This way, an early return won't be able
        // to leak memory with multiple calls to this object
        this->isInitialized = true;
        this->isValid = false;

        const s32 initialImageScaleS32 = baseImageWidth / templateImage.get_size(1) ;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32); // TODO: check that this is integer

        const s32 numTransformationParameters = transformation.get_transformType() >> 8;

        this->templateRegionHeight = templateRegion.bottom - templateRegion.top  + 1;
        this->templateRegionWidth  = templateRegion.right  - templateRegion.left + 1;

        this->templateWeightsSigma = sqrtf(this->templateRegionWidth*this->templateRegionWidth + this->templateRegionHeight*this->templateRegionHeight) / 2.0f;

        // Allocate all permanent memory
        BeginBenchmark("InitializeTemplate.allocate");
        for(s32 iScale=0; iScale<this->numPyramidLevels; iScale++) {
          const f32 scale = static_cast<f32>(1 << iScale);

          templateCoordinates[iScale] = Meshgrid<f32>(
            Linspace(-this->templateRegionWidth/2.0f, this->templateRegionWidth/2.0f, static_cast<s32>(floorf(this->templateRegionWidth/scale))),
            Linspace(-this->templateRegionHeight/2.0f, this->templateRegionHeight/2.0f, static_cast<s32>(floorf(this->templateRegionHeight/scale))));

          const s32 numValidPoints = templateCoordinates[iScale].get_numElements();

          this->A_full[iScale] = Array<f32>(numTransformationParameters, numValidPoints, memory);
          AnkiConditionalErrorAndReturnValue(this->A_full[iScale].IsValid(),
            RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTracker_Slow::InitializeTemplate", "Could not allocate A_full[iScale]");

          const s32 numPointsY = templateCoordinates[iScale].get_yGridVector().get_size();
          const s32 numPointsX = templateCoordinates[iScale].get_xGridVector().get_size();

          this->templateImagePyramid[iScale] = Array<u8>(numPointsY, numPointsX, memory);

          AnkiConditionalErrorAndReturnValue(this->templateImagePyramid[iScale].IsValid(),
            RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTracker_Slow::InitializeTemplate", "Could not allocate templateImagePyramid[i]");

          this->templateWeights[iScale] = Array<f32>(1, numPointsY*numPointsX, memory);

          AnkiConditionalErrorAndReturnValue(this->templateWeights[iScale].IsValid(),
            RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTracker_Slow::InitializeTemplate", "Could not allocate templateWeights[i]");

          //templateImage.Show("templateImage", true);
        }
        EndBenchmark("InitializeTemplate.allocate");

        BeginBenchmark("InitializeTemplate.initA");
        // Everything below here is temporary
        {
          PUSH_MEMORY_STACK(memory);

          BeginBenchmark("InitializeTemplate.setTemplateMask");
          Array<f32> templateMask = Array<f32>(templateImageHeight, templateImageWidth, memory);
          templateMask.SetZero();
          templateMask(
            Round<s32>(templateRegion.top),
            Round<s32>(templateRegion.bottom),
            Round<s32>(templateRegion.left),
            Round<s32>(templateRegion.right)).Set(1.0f);
          EndBenchmark("InitializeTemplate.setTemplateMask");

          for(s32 iScale=0; iScale<this->numPyramidLevels; iScale++) {
            PUSH_MEMORY_STACK(memory);

            const s32 numPointsY = templateCoordinates[iScale].get_yGridVector().get_size();
            const s32 numPointsX = templateCoordinates[iScale].get_xGridVector().get_size();

            const f32 scale = static_cast<f32>(1 << iScale);

            BeginBenchmark("InitializeTemplate.evaluateMeshgrid");
            Array<f32> xIn = templateCoordinates[iScale].EvaluateX2(memory);
            Array<f32> yIn = templateCoordinates[iScale].EvaluateY2(memory);
            EndBenchmark("InitializeTemplate.evaluateMeshgrid");

            AnkiAssert(AreEqualSize(xIn, yIn));

            Array<f32> xTransformed(numPointsY, numPointsX, memory);
            Array<f32> yTransformed(numPointsY, numPointsX, memory);

            BeginBenchmark("InitializeTemplate.transformPoints");
            // Compute the warped coordinates (for later)
            //if((lastResult = transformation.TransformPoints(xIn, yIn, scale*initialImageScaleF32, xTransformed, yTransformed)) != RESULT_OK)
            if((lastResult = transformation.TransformPoints(xIn, yIn, initialImageScaleF32, true, false, xTransformed, yTransformed)) != RESULT_OK)
              return lastResult;
            EndBenchmark("InitializeTemplate.transformPoints");

            Array<f32> templateDerivativeX(numPointsY, numPointsX, memory);
            Array<f32> templateDerivativeY(numPointsY, numPointsX, memory);

            BeginBenchmark("InitializeTemplate.image.interp2");
            if((lastResult = Interp2<u8,u8>(templateImage, xTransformed, yTransformed, this->templateImagePyramid[iScale], INTERPOLATE_LINEAR)) != RESULT_OK)
              return lastResult;
            EndBenchmark("InitializeTemplate.image.interp2");

            BeginBenchmark("InitializeTemplate.ComputeImageGradients");
            // Ix = (image_right(targetBlur) - image_left(targetBlur))/2 * spacing;
            // Iy = (image_down(targetBlur) - image_up(targetBlur))/2 * spacing;
            Matrix::Subtract<u8,f32,f32>(templateImagePyramid[iScale](1,-2,2,-1), templateImagePyramid[iScale](1,-2,0,-3), templateDerivativeX(1,-2,1,-2));
            //Matrix::DotMultiply<f32,f32,f32>(templateDerivativeX, scale / 2.0f, templateDerivativeX);
            Matrix::DotMultiply<f32,f32,f32>(templateDerivativeX, scale / (2.0f*255.0f), templateDerivativeX);

            Matrix::Subtract<u8,f32,f32>(templateImagePyramid[iScale](2,-1,1,-2), templateImagePyramid[iScale](0,-3,1,-2), templateDerivativeY(1,-2,1,-2));
            //Matrix::DotMultiply<f32,f32,f32>(templateDerivativeY, scale / 2.0f, templateDerivativeY);
            Matrix::DotMultiply<f32,f32,f32>(templateDerivativeY, scale / (2.0f*255.0f), templateDerivativeY);
            EndBenchmark("InitializeTemplate.ComputeImageGradients");

            // Create the A matrix
            BeginBenchmark("InitializeTemplate.ComputeA");
            if(transformation.get_transformType() == Transformations::TRANSFORM_TRANSLATION) {
              Array<f32> tmp(1, numPointsY*numPointsX, memory);

              Matrix::Vectorize(isOutColumnMajor, templateDerivativeX, tmp);
              this->A_full[iScale](0,0,0,-1).Set(tmp);

              Matrix::Vectorize(isOutColumnMajor, templateDerivativeY, tmp);
              this->A_full[iScale](1,1,0,-1).Set(tmp);
            } else if(transformation.get_transformType() == Transformations::TRANSFORM_AFFINE || transformation.get_transformType() == Transformations::TRANSFORM_PROJECTIVE) {
              // The first six terms of affine and projective are the same

              Array<f32> xInV(1, numPointsY*numPointsX, memory);
              Array<f32> yInV(1, numPointsY*numPointsX, memory);
              Matrix::Vectorize(isOutColumnMajor, xIn, xInV);
              Matrix::Vectorize(isOutColumnMajor, yIn, yInV);

              Array<f32> tmp1(1, numPointsY*numPointsX, memory);

              //xInV.Print("xInV");

              // X.*Ix(:)
              Matrix::Vectorize(isOutColumnMajor, templateDerivativeX, tmp1);
              Matrix::DotMultiply<f32,f32,f32>(tmp1, xInV, tmp1);
              this->A_full[iScale](0,0,0,-1).Set(tmp1);

              // Y.*Ix(:)
              Matrix::Vectorize(isOutColumnMajor, templateDerivativeX, tmp1);
              Matrix::DotMultiply<f32,f32,f32>(tmp1, yInV, tmp1);
              this->A_full[iScale](1,1,0,-1).Set(tmp1);

              // Ix(:)
              Matrix::Vectorize(isOutColumnMajor, templateDerivativeX, tmp1);
              this->A_full[iScale](2,2,0,-1).Set(tmp1);

              // X.*Iy(:)
              Matrix::Vectorize(isOutColumnMajor, templateDerivativeY, tmp1);
              Matrix::DotMultiply<f32,f32,f32>(tmp1, xInV, tmp1);
              this->A_full[iScale](3,3,0,-1).Set(tmp1);

              // Y.*Iy(:)
              Matrix::Vectorize(isOutColumnMajor, templateDerivativeY, tmp1);
              Matrix::DotMultiply<f32,f32,f32>(tmp1, yInV, tmp1);
              this->A_full[iScale](4,4,0,-1).Set(tmp1);

              // Iy(:)
              Matrix::Vectorize(isOutColumnMajor, templateDerivativeY, tmp1);
              this->A_full[iScale](5,5,0,-1).Set(tmp1);

              if(transformation.get_transformType() == Transformations::TRANSFORM_PROJECTIVE) {
                //The seventh and eights terms are for projective only, not for affine

                Array<f32> tmp2(1, numPointsY*numPointsX, memory);

                // -X.^2.*Ix(:) - X.*Y.*Iy(:)
                Matrix::Vectorize(isOutColumnMajor, templateDerivativeX, tmp1); // Ix(:)
                Matrix::DotMultiply<f32,f32,f32>(tmp1, xInV, tmp1); // Ix(:).*X
                Matrix::DotMultiply<f32,f32,f32>(tmp1, xInV, tmp1); // Ix(:).*X.^2
                Matrix::Subtract<f32,f32,f32>(0.0f, tmp1, tmp1); // -Ix(:).*X.^2

                Matrix::Vectorize(isOutColumnMajor, templateDerivativeY, tmp2); // Iy(:)
                Matrix::DotMultiply<f32,f32,f32>(tmp2, xInV, tmp2); // Iy(:).*X
                Matrix::DotMultiply<f32,f32,f32>(tmp2, yInV, tmp2); // Iy(:).*X.*Y

                Matrix::Subtract<f32,f32,f32>(tmp1,tmp2,tmp1);
                this->A_full[iScale](6,6,0,-1).Set(tmp1);

                // -X.*Y.*Ix(:) - Y.^2.*Iy(:)
                Matrix::Vectorize(isOutColumnMajor, templateDerivativeX, tmp1); // Ix(:)
                Matrix::DotMultiply<f32,f32,f32>(tmp1, xInV, tmp1); // Ix(:).*X
                Matrix::DotMultiply<f32,f32,f32>(tmp1, yInV, tmp1); // Ix(:).*X.*Y
                Matrix::Subtract<f32,f32,f32>(0.0f, tmp1, tmp1); // -Ix(:).*X.*Y

                Matrix::Vectorize(isOutColumnMajor, templateDerivativeY, tmp2); // Iy(:)
                Matrix::DotMultiply<f32,f32,f32>(tmp2, yInV, tmp2); // Iy(:).*Y
                Matrix::DotMultiply<f32,f32,f32>(tmp2, yInV, tmp2); // Iy(:).*Y.^2

                Matrix::Subtract<f32,f32,f32>(tmp1,tmp2,tmp1);
                this->A_full[iScale](7,7,0,-1).Set(tmp1);
              } // if(transformation.get_transformType() == Transformations::TRANSFORM_PROJECTIVE)
            } // else if(transformation.get_transformType() == Transformations::TRANSFORM_AFFINE || transformation.get_transformType() == Transformations::TRANSFORM_PROJECTIVE)
            EndBenchmark("InitializeTemplate.ComputeA");

            {
              PUSH_MEMORY_STACK(memory);

              Array<f32> GaussianTmp(numPointsY, numPointsX, memory);

              BeginBenchmark("InitializeTemplate.weights.compute");
              // GaussianTmp = exp(-((this.xgrid{i_scale}).^2 + (this.ygrid{i_scale}).^2) / (2*(W_sigma)^2));
              {
                PUSH_MEMORY_STACK(memory);

                Array<f32> tmp(numPointsY, numPointsX, memory);

                Matrix::DotMultiply<f32,f32,f32>(xIn, xIn, GaussianTmp);
                Matrix::DotMultiply<f32,f32,f32>(yIn, yIn, tmp);
                Matrix::Add<f32,f32,f32>(GaussianTmp, tmp, GaussianTmp);
                Matrix::Subtract<f32,f32,f32>(0.0f, GaussianTmp, GaussianTmp);
                Matrix::DotMultiply<f32,f32,f32>(GaussianTmp, 1.0f/(2.0f*templateWeightsSigma*templateWeightsSigma), GaussianTmp);
                Matrix::Exp<f32,f32,f32>(GaussianTmp, GaussianTmp);
              }
              EndBenchmark("InitializeTemplate.weights.compute");

              Array<f32> templateWeightsTmp(numPointsY, numPointsX, memory);

              BeginBenchmark("InitializeTemplate.weights.interp2");
              // W_mask = interp2(double(targetMask), xi, yi, 'linear', 0);
              if((lastResult = Interp2<f32,f32>(templateMask, xTransformed, yTransformed, templateWeightsTmp, INTERPOLATE_LINEAR)) != RESULT_OK)
                return lastResult;
              EndBenchmark("InitializeTemplate.weights.interp2");

              BeginBenchmark("InitializeTemplate.weights.vectorize");
              // W_ = W_mask .* GaussianTmp;
              Matrix::DotMultiply<f32,f32,f32>(templateWeightsTmp, GaussianTmp, templateWeightsTmp);

              // Set the boundaries to zero, since these won't be correctly estimated
              templateWeightsTmp(0,0,0,0).Set(0);
              templateWeightsTmp(-1,-1,0,0).Set(0);
              templateWeightsTmp(0,0,-1,-1).Set(0);
              templateWeightsTmp(-1,-1,-1,-1).Set(0);

              Matrix::Vectorize(isOutColumnMajor, templateWeightsTmp, templateWeights[iScale]);
              EndBenchmark("InitializeTemplate.weights.vectorize");
            } // PUSH_MEMORY_STACK(memory);
          } // for(s32 iScale=0; iScale<this->numPyramidLevels; iScale++, fScale++)
        } // PUSH_MEMORY_STACK(memory);
        EndBenchmark("InitializeTemplate.initA");

        this->isValid = true;

        return RESULT_OK;
      }

      Result LucasKanadeTracker_Slow::UpdateTrack(const Array<u8> &nextImage, const s32 maxIterations, const f32 convergenceTolerance, const bool useWeights, bool &verify_converged, MemoryStack scratch)
      {
        Result lastResult;

        for(s32 iScale=numPyramidLevels-1; iScale>=0; iScale--) {
          // TODO: remove
          //for(s32 iScale=0; iScale>=0; iScale--) {
          verify_converged = false;

          BeginBenchmark("UpdateTrack.refineTranslation");
          if((lastResult = IterativelyRefineTrack(nextImage, maxIterations, iScale, convergenceTolerance, Transformations::TRANSFORM_TRANSLATION, useWeights, verify_converged, scratch)) != RESULT_OK)
            return lastResult;
          EndBenchmark("UpdateTrack.refineTranslation");

          //this->get_transformation().Print("Translation");

          if(this->transformation.get_transformType() != Transformations::TRANSFORM_TRANSLATION) {
            BeginBenchmark("UpdateTrack.refineOther");
            if((lastResult = IterativelyRefineTrack(nextImage, maxIterations, iScale, convergenceTolerance, this->transformation.get_transformType(), useWeights, verify_converged, scratch)) != RESULT_OK)
              return lastResult;
            EndBenchmark("UpdateTrack.refineOther");

            //this->get_transformation().Print("Other");
          }
        } // for(s32 iScale=numPyramidLevels; iScale>=0; iScale--)

        return RESULT_OK;
      }

      Result LucasKanadeTracker_Slow::IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const Transformations::TransformType curTransformType, const bool useWeights, bool &verify_converged, MemoryStack scratch)
      {
        const bool isOutColumnMajor = false; // TODO: change to false, which will probably be faster

        Result lastResult;

        AnkiConditionalErrorAndReturnValue(this->isInitialized == true,
          RESULT_FAIL, "LucasKanadeTracker_Slow::IterativelyRefineTrack", "This object is not initialized");

        AnkiConditionalErrorAndReturnValue(nextImage.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTracker_Slow::IterativelyRefineTrack", "nextImage is not valid");

        AnkiConditionalErrorAndReturnValue(maxIterations > 0 && maxIterations < 1000,
          RESULT_FAIL_INVALID_PARAMETER, "LucasKanadeTracker_Slow::IterativelyRefineTrack", "maxIterations must be greater than zero and less than 1000");

        AnkiConditionalErrorAndReturnValue(whichScale >= 0 && whichScale < this->numPyramidLevels,
          RESULT_FAIL_INVALID_PARAMETER, "LucasKanadeTracker_Slow::IterativelyRefineTrack", "whichScale is invalid");

        AnkiConditionalErrorAndReturnValue(convergenceTolerance > 0.0f,
          RESULT_FAIL_INVALID_PARAMETER, "LucasKanadeTracker_Slow::IterativelyRefineTrack", "convergenceTolerance must be greater than zero");

        const s32 initialImageScaleS32 = baseImageWidth / nextImage.get_size(1) ;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32); // TODO: check that this is integer

        const s32 numPointsY = templateCoordinates[whichScale].get_yGridVector().get_size();
        const s32 numPointsX = templateCoordinates[whichScale].get_xGridVector().get_size();

        Array<f32> A_part;

        BeginBenchmark("IterativelyRefineTrack.extractAPart");
        const s32 numSystemParameters = curTransformType >> 8;
        if(curTransformType == Transformations::TRANSFORM_TRANSLATION) {
          // Translation-only can be performed by grabbing a few rows of the A_full matrix
          if(this->get_transformation().get_transformType() == Transformations::TRANSFORM_AFFINE ||
            this->get_transformation().get_transformType() == Transformations::TRANSFORM_PROJECTIVE) {
              A_part = Array<f32>(2, this->A_full[whichScale].get_size(1), scratch);
              A_part(0,-1,0,-1).Set(this->A_full[whichScale](2,3,5,0,1,-1)); // grab the 2nd and 5th rows
          } else if(this->get_transformation().get_transformType() == Transformations::TRANSFORM_TRANSLATION) {
            A_part = this->A_full[whichScale];
          } else {
            AnkiAssert(false);
          }
        } else if(curTransformType == Transformations::TRANSFORM_AFFINE || curTransformType == Transformations::TRANSFORM_PROJECTIVE) {
          A_part = this->A_full[whichScale];
        } else {
          AnkiAssert(false);
        }
        EndBenchmark("IterativelyRefineTrack.extractAPart");

        //Array<f32> xPrevious(1, numPointsY*numPointsX, scratch);
        //Array<f32> yPrevious(1, numPointsY*numPointsX, scratch);

        // Initialize with some very extreme coordinates
        FixedLengthList<Quadrilateral<f32> > previousCorners(NUM_PREVIOUS_QUADS_TO_COMPARE, scratch);

        for(s32 i=0; i<NUM_PREVIOUS_QUADS_TO_COMPARE; i++) {
          previousCorners[i] = Quadrilateral<f32>(Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f));
        }

        Array<f32> xIn(1, numPointsY*numPointsX, scratch);
        Array<f32> yIn(1, numPointsY*numPointsX, scratch);

        BeginBenchmark("IterativelyRefineTrack.vectorizeXin");
        {
          PUSH_MEMORY_STACK(scratch);

          Array<f32> xIn2d = templateCoordinates[whichScale].EvaluateX2(scratch);
          Array<f32> yIn2d = templateCoordinates[whichScale].EvaluateY2(scratch);

          Matrix::Vectorize(isOutColumnMajor, xIn2d, xIn);
          Matrix::Vectorize(isOutColumnMajor, yIn2d, yIn);
        } // PUSH_MEMORY_STACK(scratch);
        EndBenchmark("IterativelyRefineTrack.vectorizeXin");

        verify_converged = false;

        for(s32 iteration=0; iteration<maxIterations; iteration++) {
          PUSH_MEMORY_STACK(scratch);

          //const f32 scale = static_cast<f32>(1 << whichScale);

          // [xi, yi] = this.getImagePoints(i_scale);
          Array<f32> xTransformed(1, numPointsY*numPointsX, scratch);
          Array<f32> yTransformed(1, numPointsY*numPointsX, scratch);

          BeginBenchmark("IterativelyRefineTrack.transformPoints");
          //if((lastResult = transformation.TransformPoints(xIn, yIn, scale*initialImageScaleF32, xTransformed, yTransformed)) != RESULT_OK)
          if((lastResult = transformation.TransformPoints(xIn, yIn, initialImageScaleF32, true, false, xTransformed, yTransformed)) != RESULT_OK)
            return lastResult;
          EndBenchmark("IterativelyRefineTrack.transformPoints");

          Array<f32> nextImageTransformed(1, numPointsY*numPointsX, scratch);

          BeginBenchmark("IterativelyRefineTrack.interpTransformedCoords");
          // imgi = interp2(img, xi(:), yi(:), 'linear');
          {
            PUSH_MEMORY_STACK(scratch);

            Array<f32> nextImageTransformed2d(1, numPointsY*numPointsX, scratch);

            if((lastResult = Interp2<u8,f32>(nextImage, xTransformed, yTransformed, nextImageTransformed2d, INTERPOLATE_LINEAR, -1.0f)) != RESULT_OK)
              return lastResult;

            Matrix::Vectorize<f32,f32>(isOutColumnMajor, nextImageTransformed2d, nextImageTransformed);
          } // PUSH_MEMORY_STACK(scratch);
          EndBenchmark("IterativelyRefineTrack.interpTransformedCoords");

          BeginBenchmark("IterativelyRefineTrack.getNumMatches");
          // inBounds = ~isnan(imgi);
          // WARNING: this is also treating real zeros as invalid, but this should not be a big problem
          Find<f32, Comparison::GreaterThanOrEqual<f32,f32>, f32> inBounds(nextImageTransformed, 0.0f);
          const s32 numInBounds = inBounds.get_numMatches();

          if(numInBounds < 16) {
            AnkiWarn("LucasKanadeTracker_Slow::IterativelyRefineTrack", "Template drifted too far out of image.");
            break;
          }
          EndBenchmark("IterativelyRefineTrack.getNumMatches");

          Array<f32> templateImage(1, numPointsY*numPointsX, scratch);
          Matrix::Vectorize(isOutColumnMajor, this->templateImagePyramid[whichScale], templateImage);

          BeginBenchmark("IterativelyRefineTrack.templateDerivative");
          // It = imgi - this.target{i_scale}(:);
          Array<f32> templateDerivativeT(1, numInBounds, scratch);
          {
            PUSH_MEMORY_STACK(scratch);
            Array<f32> templateDerivativeT_allPoints(1, numPointsY*numPointsX, scratch);
            Matrix::Subtract<f32,f32,f32>(nextImageTransformed, templateImage, templateDerivativeT_allPoints);
            inBounds.SetArray(templateDerivativeT, templateDerivativeT_allPoints, 1);
            Matrix::DotMultiply<f32,f32,f32>(templateDerivativeT, 1.0f/255.0f, templateDerivativeT);
          }
          EndBenchmark("IterativelyRefineTrack.templateDerivative");

          Array<f32> AWAt(numSystemParameters, numSystemParameters, scratch);

          // AtW = (A(inBounds,:).*this.W{i_scale}(inBounds,ones(1,size(A,2))))';

          BeginBenchmark("IterativelyRefineTrack.extractApartRows");
          Array<f32> A = inBounds.SetArray(A_part, 1, scratch);
          EndBenchmark("IterativelyRefineTrack.extractApartRows");

          BeginBenchmark("IterativelyRefineTrack.setAw");
          Array<f32> AW(A.get_size(0), A.get_size(1), scratch);
          AW.Set(A);
          EndBenchmark("IterativelyRefineTrack.setAw");

          BeginBenchmark("IterativelyRefineTrack.dotMultiplyWeights");
          if(useWeights) {
            PUSH_MEMORY_STACK(scratch);
            Array<f32> validTemplateWeights = inBounds.SetArray(templateWeights[whichScale], 1, scratch);

            for(s32 y=0; y<numSystemParameters; y++) {
              Matrix::DotMultiply<f32,f32,f32>(AW(y,y,0,-1), validTemplateWeights, AW(y,y,0,-1));
            }
          } // if(useWeights)
          EndBenchmark("IterativelyRefineTrack.dotMultiplyWeights");

          BeginBenchmark("IterativelyRefineTrack.computeAWAt");
          // AtWA = AtW*A(inBounds,:) + diag(this.ridgeWeight*ones(1,size(A,2)));
          Matrix::MultiplyTranspose(A, AW, AWAt);
          EndBenchmark("IterativelyRefineTrack.computeAWAt");

          //if(curTransformType == Transformations::TRANSFORM_AFFINE) {
          //  Matlab matlab(false);

          //  matlab.PutArray(A, "A_tmp");
          //  matlab.PutArray(AW, "AW_tmp");
          //  matlab.PutArray(AWAt, "AWAt_tmp");
          //  matlab.EvalStringEcho("if ~exist('A', 'var') A=cell(0); AW=cell(0); AWAt=cell(0); end;");
          //  matlab.EvalStringEcho("A{end+1}=A_tmp; AW{end+1}=AW_tmp; AWAt{end+1}=AWAt_tmp;");
          //  CoreTechPrint("");
          //}

          Array<f32> ridgeWeightMatrix = Eye<f32>(numSystemParameters, numSystemParameters, scratch);
          Matrix::DotMultiply<f32,f32,f32>(ridgeWeightMatrix, ridgeWeight, ridgeWeightMatrix);

          Matrix::Add<f32,f32,f32>(AWAt, ridgeWeightMatrix, AWAt);

          BeginBenchmark("IterativelyRefineTrack.computeb");
          // b = AtW*It(inBounds);
          Array<f32> b(1,numSystemParameters,scratch);
          Matrix::MultiplyTranspose(templateDerivativeT, AW, b);
          EndBenchmark("IterativelyRefineTrack.computeb");

          //if(curTransformType == Transformations::TRANSFORM_AFFINE) {
          //  Matlab matlab(false);

          //  matlab.PutArray(b, "b_tmp");
          //  //matlab.PutArray(templateDerivativeT, "templateDerivativeT");
          //  //matlab.PutArray(ridgeWeightMatrix, "ridgeWeightMatrix");
          //  matlab.EvalStringEcho("if ~exist('b', 'var') b=cell(0); end;");
          //  matlab.EvalStringEcho("b{end+1}=b_tmp;");
          //  CoreTechPrint("");
          //}

          // update = AtWA\b;
          //AW.Print("AW");
          //AWAt.Print("Orig AWAt");
          //b.Print("Orig b");

          BeginBenchmark("IterativelyRefineTrack.solveForUpdate");
          bool numericalFailure;
          if((lastResult = Matrix::SolveLeastSquaresWithCholesky(AWAt, b, false, numericalFailure)) != RESULT_OK)
            return lastResult;

          if(numericalFailure){
            AnkiWarn("LucasKanadeTracker_Slow::IterativelyRefineTrack", "numericalFailure");
            return RESULT_OK;
          }
          EndBenchmark("IterativelyRefineTrack.solveForUpdate");

          //b.Print("Orig update");

          BeginBenchmark("IterativelyRefineTrack.updateTransformation");
          //this->transformation.Print("t1");
          //b.Print("b");
          this->transformation.Update(b, initialImageScaleF32, scratch, curTransformType);
          //this->transformation.Print("t2");
          EndBenchmark("IterativelyRefineTrack.updateTransformation");

          //if(curTransformType == Transformations::TRANSFORM_AFFINE) {
          //  Matlab matlab(false);

          //  matlab.PutArray(b, "update_tmp");
          //  matlab.PutArray(this->transformation.get_homography(), "homography_tmp");

          //  matlab.EvalStringEcho("if ~exist('update', 'var') update=cell(0); homography=cell(0); end;");
          //  matlab.EvalStringEcho("update{end+1}=update_tmp; homography{end+1}=homography_tmp;");
          //  CoreTechPrint("");
          //}

          BeginBenchmark("IterativelyRefineTrack.checkForCompletion");

          // Check if we're done with iterations
          const f32 minChange = UpdatePreviousCorners(transformation, baseImageWidth, baseImageHeight, previousCorners, scratch);

          if(minChange < convergenceTolerance) {
            verify_converged = true;
            EndBenchmark("IterativelyRefineTrack.checkForCompletion");
            return RESULT_OK;
          }

          EndBenchmark("IterativelyRefineTrack.checkForCompletion");
        } // for(s32 iteration=0; iteration<maxIterations; iteration++)

        return RESULT_OK;
      } // Result LucasKanadeTracker_Slow::IterativelyRefineTrack()

      bool LucasKanadeTracker_Slow::IsValid() const
      {
        if(!this->isValid)
          return false;

        if(!A_full.IsValid())
          return false;

        if(!templateImagePyramid.IsValid())
          return false;

        if(!templateCoordinates.IsValid())
          return false;

        if(!templateWeights.IsValid())
          return false;

        if(this->isInitialized) {
          for(s32 i=0; i<numPyramidLevels; i++) {
            if(!A_full[i].IsValid())
              return false;

            if(!templateImagePyramid[i].IsValid())
              return false;

            if(!templateWeights[i].IsValid())
              return false;
          }
        }

        return true;
      }

      Result LucasKanadeTracker_Slow::UpdateTransformation(const Array<f32> &update, const f32 scale, MemoryStack scratch, Transformations::TransformType updateType)
      {
        return this->transformation.Update(update, scale, scratch, updateType);
      }

      Result LucasKanadeTracker_Slow::set_transformation(const Transformations::PlanarTransformation_f32 &transformation)
      {
        return this->transformation.Set(transformation);
      }

      Transformations::PlanarTransformation_f32 LucasKanadeTracker_Slow::get_transformation() const
      {
        return transformation;
      }
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki
