/**
File: lucaseKanade.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/lucasKanade.h"
#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/interpolate.h"
#include "anki/common/robot/arrayPatterns.h"
#include "anki/common/robot/find.h"

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
      PlanarTransformation_f32::PlanarTransformation_f32(TransformType transformType, MemoryStack &memory)
        : transformType(transformType)
      {
        AnkiConditionalErrorAndReturn(transformType==TRANSFORM_TRANSLATION || transformType==TRANSFORM_PROJECTIVE,
          "PlanarTransformation_f32::PlanarTransformation_f32", "Invalid transformType %d", transformType);

        homography = Eye<f32>(3, 3, memory);
      }

      Result PlanarTransformation_f32::TransformPoints(
        const Array<f32> &xIn, const Array<f32> &yIn,
        const f32 scale,
        const Point<f32> &centerOffset,
        Array<f32> &xOut, Array<f32> &yOut)
      {
        AnkiConditionalErrorAndReturnValue(homography.IsValid(),
          RESULT_FAIL, "PlanarTransformation_f32::TransformPoints", "homography is not valid");

        AnkiConditionalErrorAndReturnValue(xIn.IsValid() && yIn.IsValid() && xOut.IsValid() && yOut.IsValid(),
          RESULT_FAIL, "PlanarTransformation_f32::TransformPoints", "All inputs and outputs must be allocated and valid");

        AnkiConditionalErrorAndReturnValue(xIn.get_rawDataPointer() != xOut.get_rawDataPointer() && yIn.get_rawDataPointer() != yOut.get_rawDataPointer(),
          RESULT_FAIL, "PlanarTransformation_f32::TransformPoints", "In and Out arrays must be in different memory locations");

        AnkiConditionalErrorAndReturnValue(
          xIn.get_size(0) == yIn.get_size(0) && xIn.get_size(0) == xOut.get_size(0) && xIn.get_size(0) == yOut.get_size(0) &&
          xIn.get_size(1) == yIn.get_size(1) && xIn.get_size(1) == xOut.get_size(1) && xIn.get_size(1) == yOut.get_size(1),
          RESULT_FAIL, "PlanarTransformation_f32::TransformPoints", "All inputs and outputs must be the same size");

        const s32 numPointsY = xIn.get_size(0);
        const s32 numPointsX = xIn.get_size(1);

        if(transformType == TRANSFORM_TRANSLATION) {
          const f32 dx = homography[0][2];
          const f32 dy = homography[1][2];

          for(s32 y=0; y<numPointsY; y++) {
            const f32 * restrict pXIn = xIn.Pointer(y,0);
            const f32 * restrict pYIn = yIn.Pointer(y,0);
            f32 * restrict pXOut = xOut.Pointer(y,0);
            f32 * restrict pYOut = yOut.Pointer(y,0);

            for(s32 x=0; x<numPointsX; x++) {
              pXOut[x] = pXIn[x] + dx + centerOffset.x;
              pYOut[x] = pYIn[x] + dy + centerOffset.y;
            }
          }
        } else if(transformType == TRANSFORM_PROJECTIVE) {
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
          const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

          assert(FLT_NEAR(homography[2][2], 1.0f));

          for(s32 y=0; y<numPointsY; y++) {
            const f32 * restrict pXIn = xIn.Pointer(y,0);
            const f32 * restrict pYIn = yIn.Pointer(y,0);
            f32 * restrict pXOut = xOut.Pointer(y,0);
            f32 * restrict pYOut = yOut.Pointer(y,0);

            for(s32 x=0; x<numPointsX; x++) {
              const f32 wpi = 1.0f / (h20*pXIn[x] + h21*pYIn[x] + h22);

              const f32 xp = (h00*pXIn[x] + h01*pYIn[x] + h02) * wpi;
              const f32 yp = (h10*pXIn[x] + h11*pYIn[x] + h12) * wpi;

              pXOut[x] = xp + centerOffset.x;
              pYOut[x] = yp + centerOffset.y;
            }
          }
        } else {
          // Should be checked earlier
          assert(false);
          return RESULT_FAIL;
        }

        return RESULT_OK;
      }

      Result PlanarTransformation_f32::Update(const Array<f32> &update, MemoryStack scratch, TransformType updateType)
      {
        AnkiConditionalErrorAndReturnValue(update.IsValid(),
          RESULT_FAIL, "PlanarTransformation_f32::Update", "update is not valid");

        AnkiConditionalErrorAndReturnValue(update.get_size(0) == 1,
          RESULT_FAIL, "PlanarTransformation_f32::Update", "update is the incorrect size");

        if(updateType == TRANSFORM_UNKNOWN) {
          updateType = this->transformType;
        }

        if(this->transformType == TRANSFORM_TRANSLATION) {
          AnkiConditionalErrorAndReturnValue(updateType == TRANSFORM_TRANSLATION,
            RESULT_FAIL, "PlanarTransformation_f32::Update", "cannot update this transform with the update type %d", updateType);
        } else if(this->transformType == TRANSFORM_PROJECTIVE) {
          AnkiConditionalErrorAndReturnValue(updateType == TRANSFORM_TRANSLATION || updateType == TRANSFORM_PROJECTIVE,
            RESULT_FAIL, "PlanarTransformation_f32::Update", "cannot update this transform with the update type %d", updateType);
        } else {
          assert(false);
        }

        const f32 * pUpdate = update[0];

        if(updateType == TRANSFORM_TRANSLATION) {
          AnkiConditionalErrorAndReturnValue(update.get_size(1) == 2,
            RESULT_FAIL, "PlanarTransformation_f32::Update", "update is the incorrect size");

          // this.tform(1:2,3) = this.tform(1:2,3) - update;
          homography[0][2] -= pUpdate[0];
          homography[1][2] -= pUpdate[1];
        } else { // if(updateType == TRANSFORM_TRANSLATION)
          Array<f32> updateArray(3,3,scratch);

          if(updateType == TRANSFORM_PROJECTIVE) {
            AnkiConditionalErrorAndReturnValue(update.get_size(1) == 8,
              RESULT_FAIL, "PlanarTransformation_f32::Update", "update is the incorrect size");

            // tformUpdate = eye(3) + [update(1:3)'; update(4:6)'; update(7:8)' 0];
            updateArray[0][0] = 1.0f + pUpdate[0]; updateArray[0][1] = pUpdate[1];        updateArray[0][2] = pUpdate[2];
            updateArray[1][0] = pUpdate[3];        updateArray[1][1] = 1.0f + pUpdate[4]; updateArray[1][2] = pUpdate[5];
            updateArray[2][0] = pUpdate[6];        updateArray[2][1] = pUpdate[7];        updateArray[2][2] = 1.0f;
          } else {
            AnkiError("PlanarTransformation_f32::Update", "Unknown transformation type %d", updateType);
            return RESULT_FAIL;
          }

          // this.tform = this.tform*inv(tformUpdate);
          Invert3x3(
            updateArray[0][0], updateArray[0][1], updateArray[2][2],
            updateArray[1][0], updateArray[1][1], updateArray[1][2],
            updateArray[2][0], updateArray[2][1], updateArray[0][2]);

          Array<f32> newHomography(3,3,scratch);

          Matrix::Multiply(this->homography, updateArray, newHomography);

          if(!FLT_NEAR(newHomography[2][2], 1.0f)) {
            Matrix::DotDivide<f32,f32,f32>(newHomography, newHomography[2][2], newHomography);
          }

          this->homography.Set(newHomography);
        } // if(updateType == TRANSFORM_TRANSLATION) ... else

        return RESULT_OK;
      }

      Result PlanarTransformation_f32::Print(const char * const variableName)
      {
        return this->homography.Print(variableName);
      }

      Result PlanarTransformation_f32::set_transformType(const TransformType transformType)
      {
        if(transformType == TRANSFORM_TRANSLATION || transformType == TRANSFORM_PROJECTIVE) {
          this->transformType = transformType;
        } else {
          AnkiError("PlanarTransformation_f32::set_transformType", "Unknown transformation type %d", transformType);
          return RESULT_FAIL;
        }

        return RESULT_OK;
      }

      TransformType PlanarTransformation_f32::get_transformType() const
      {
        return transformType;
      }

      Result PlanarTransformation_f32::set_homography(const Array<f32>& in)
      {
        if(this->homography.Set(in) != 9)
          return RESULT_FAIL;

        assert(FLT_NEAR(in[2][2], 1.0f));

        return RESULT_OK;
      }

      const Array<f32>& PlanarTransformation_f32::get_homography() const
      {
        return this->homography;
      }

      LucasKanadeTracker_f32::LucasKanadeTracker_f32(const s32 templateImageHeight, const s32 templateImageWidth, const s32 numPyramidLevels, const TransformType transformType, const f32 ridgeWeight, MemoryStack &memory)
        : isValid(false), templateImageHeight(templateImageHeight), templateImageWidth(templateImageWidth), numPyramidLevels(numPyramidLevels), transformation(PlanarTransformation_f32(transformType,memory)), ridgeWeight(ridgeWeight), isInitialized(false)
      {
        AnkiConditionalErrorAndReturn(templateImageHeight > 0 && templateImageWidth > 0,
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "template widths and heights must be greater than zero, and multiples of %d", ANKI_VISION_IMAGE_WIDTH_MULTIPLE);

        AnkiConditionalErrorAndReturn(numPyramidLevels > 0,
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "numPyramidLevels must be greater than zero");

        AnkiConditionalErrorAndReturn(transformType==TRANSFORM_TRANSLATION || transformType==TRANSFORM_PROJECTIVE,
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "Only TRANSFORM_TRANSLATION and TRANSFORM_PROJECTIVE are supported");

        AnkiConditionalErrorAndReturn(ridgeWeight >= 0.0f,
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "ridgeWeight must be greater or equal to zero");

        for(s32 i=1; i<(numPyramidLevels-1); i++) {
          const s32 curTemplateHeight = templateImageHeight >> i;
          const s32 curTemplateWidth = templateImageWidth >> i;

          AnkiConditionalErrorAndReturn(!IsOdd(curTemplateHeight) && !IsOdd(curTemplateWidth),
            "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "Template widths and height must divisible by 2^numPyramidLevels");
        }

        A_full = FixedLengthList<Array<f32>>(numPyramidLevels, memory);

        AnkiConditionalErrorAndReturn(A_full.IsValid(),
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "Could not allocate A_full");

        A_full.set_size(numPyramidLevels);

        templateCoordinates = FixedLengthList<Meshgrid<f32>>(numPyramidLevels, memory);

        AnkiConditionalErrorAndReturn(templateCoordinates.IsValid(),
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "Could not allocate templateCoordinates");

        templateCoordinates.set_size(numPyramidLevels);

        templateImagePyramid = FixedLengthList<Array<u8>>(numPyramidLevels, memory);

        AnkiConditionalErrorAndReturn(templateImagePyramid.IsValid(),
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "Could not allocate templateImagePyramid");

        templateImagePyramid.set_size(numPyramidLevels);

        templateWeights = FixedLengthList<Array<f32>>(numPyramidLevels, memory);

        AnkiConditionalErrorAndReturn(templateWeights.IsValid(),
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "Could not allocate templateWeights");

        templateWeights.set_size(numPyramidLevels);

        this->isValid = true;
      }

      Result LucasKanadeTracker_f32::InitializeTemplate(const Array<u8> &templateImage, const Rectangle<f32> templateRegion, MemoryStack &memory)
      {
        const bool isOutColumnMajor = true; // TODO: change to false, which will probably be faster

        AnkiConditionalErrorAndReturnValue(this->isValid,
          RESULT_FAIL, "LucasKanadeTracker_f32::InitializeTemplate", "This object's constructor failed, so it cannot be initialized");

        AnkiConditionalErrorAndReturnValue(this->isInitialized == false,
          RESULT_FAIL, "LucasKanadeTracker_f32::InitializeTemplate", "This object has already been initialized");

        AnkiConditionalErrorAndReturnValue(templateImageHeight == templateImage.get_size(0) && templateImageWidth == templateImage.get_size(1),
          RESULT_FAIL, "LucasKanadeTracker_f32::InitializeTemplate", "template size doesn't match constructor");

        // We do this early, before any memory is allocated This way, an early return won't be able
        // to leak memory with multiple calls to this object
        this->isInitialized = true;
        this->isValid = false;

        s32 numTransformationParameters;
        if(transformation.get_transformType() == TRANSFORM_TRANSLATION) {
          numTransformationParameters = 2;
        } else if(transformation.get_transformType() == TRANSFORM_PROJECTIVE) {
          numTransformationParameters = 8;
        }

        this->templateRegionHeight = templateRegion.bottom - templateRegion.top + 1;
        this->templateRegionWidth = templateRegion.right - templateRegion.left + 1;

        this->templateWeightsSigma = sqrtf(this->templateRegionWidth*this->templateRegionWidth + this->templateRegionHeight*this->templateRegionHeight) / 2.0f;

        this->center.y = (templateRegion.bottom + templateRegion.top) / 2;
        this->center.x = (templateRegion.right + templateRegion.left) / 2;

        // Allocate all permanent memory
        for(s32 iScale=0; iScale<this->numPyramidLevels; iScale++) {
          const f32 scale = static_cast<f32>(1 << iScale);

          templateCoordinates[iScale] = Meshgrid<f32>(
            Linspace(-this->templateRegionWidth/2.0f, this->templateRegionWidth/2.0f, static_cast<s32>(floorf(this->templateRegionWidth/scale))),
            Linspace(-this->templateRegionHeight/2.0f, this->templateRegionHeight/2.0f, static_cast<s32>(floorf(this->templateRegionHeight/scale))));

          const s32 numValidPoints = templateCoordinates[iScale].get_numElements();

          this->A_full[iScale] = Array<f32>(numTransformationParameters, numValidPoints, memory);
          AnkiConditionalErrorAndReturnValue(this->A_full[iScale].IsValid(),
            RESULT_FAIL, "LucasKanadeTracker_f32::InitializeTemplate", "Could not allocate A_full[iScale]");

          const s32 numPointsY = templateCoordinates[iScale].get_yGridVector().get_size();
          const s32 numPointsX = templateCoordinates[iScale].get_xGridVector().get_size();

          this->templateImagePyramid[iScale] = Array<u8>(numPointsY, numPointsX, memory);

          AnkiConditionalErrorAndReturnValue(this->templateImagePyramid[iScale].IsValid(),
            RESULT_FAIL, "LucasKanadeTracker_f32::InitializeTemplate", "Could not allocate templateImagePyramid[i]");

          this->templateWeights[iScale] = Array<f32>(1, numPointsY*numPointsX, memory);

          AnkiConditionalErrorAndReturnValue(this->templateWeights[iScale].IsValid(),
            RESULT_FAIL, "LucasKanadeTracker_f32::InitializeTemplate", "Could not allocate templateWeights[i]");

          //templateImage.Show("templateImage", true);
        }

        // Everything below here is temporary
        {
          PUSH_MEMORY_STACK(memory);
          Array<f32> templateMask = Array<f32>(templateImageHeight, templateImageWidth, memory);
          templateMask.SetZero();
          templateMask(
            static_cast<s32>(Roundf(templateRegion.top)),
            static_cast<s32>(Roundf(templateRegion.bottom)),
            static_cast<s32>(Roundf(templateRegion.left)),
            static_cast<s32>(Roundf(templateRegion.right))).Set(1.0f);

          for(s32 iScale=0; iScale<this->numPyramidLevels; iScale++) {
            PUSH_MEMORY_STACK(memory);

            const s32 numPointsY = templateCoordinates[iScale].get_yGridVector().get_size();
            const s32 numPointsX = templateCoordinates[iScale].get_xGridVector().get_size();

            const f32 scale = static_cast<f32>(1 << iScale);

            Array<f32> xIn = templateCoordinates[iScale].EvaluateX2(memory);
            Array<f32> yIn = templateCoordinates[iScale].EvaluateY2(memory);

            assert(xIn.get_size(0) == yIn.get_size(0));
            assert(xIn.get_size(1) == yIn.get_size(1));

            Array<f32> xTransformed(numPointsY, numPointsX, memory);
            Array<f32> yTransformed(numPointsY, numPointsX, memory);

            // Compute the warped coordinates (for later)
            if(transformation.TransformPoints(xIn, yIn, scale, this->center, xTransformed, yTransformed) != RESULT_OK)
              return RESULT_FAIL;

            Array<f32> templateDerivativeX(numPointsY, numPointsX, memory);
            Array<f32> templateDerivativeY(numPointsY, numPointsX, memory);

            if(Interp2(templateImage, xTransformed, yTransformed, this->templateImagePyramid[iScale], INTERPOLATE_LINEAR) != RESULT_OK)
              return RESULT_FAIL;

            // Ix = (image_right(targetBlur) - image_left(targetBlur))/2 * spacing;
            // Iy = (image_down(targetBlur) - image_up(targetBlur))/2 * spacing;
            Matrix::Subtract<u8,f32,f32>(templateImagePyramid[iScale](1,-2,2,-1), templateImagePyramid[iScale](1,-2,0,-3), templateDerivativeX(1,-2,1,-2));
            Matrix::DotMultiply<f32,f32,f32>(templateDerivativeX, scale / 2.0f, templateDerivativeX);

            Matrix::Subtract<u8,f32,f32>(templateImagePyramid[iScale](2,-1,1,-2), templateImagePyramid[iScale](0,-3,1,-2), templateDerivativeY(1,-2,1,-2));
            Matrix::DotMultiply<f32,f32,f32>(templateDerivativeY, scale / 2.0f, templateDerivativeY);

            // Create the A matrix
            if(transformation.get_transformType() == TRANSFORM_TRANSLATION) {
              // this.A_trans{i_scale} = [Ix(:) Iy(:)];

              Array<f32> tmp(1, numPointsY*numPointsX, memory);

              Matrix::Vectorize(isOutColumnMajor, templateDerivativeX, tmp);
              this->A_full[iScale](0,0,0,-1).Set(tmp);

              Matrix::Vectorize(isOutColumnMajor, templateDerivativeY, tmp);
              this->A_full[iScale](1,1,0,-1).Set(tmp);
            } else if(transformation.get_transformType() == TRANSFORM_PROJECTIVE) {
              // X = this.xgrid{i_scale}(:);
              // Y = this.ygrid{i_scale}(:);
              Array<f32> xInV(1, numPointsY*numPointsX, memory);
              Array<f32> yInV(1, numPointsY*numPointsX, memory);
              Matrix::Vectorize(isOutColumnMajor, xIn, xInV);
              Matrix::Vectorize(isOutColumnMajor, yIn, yInV);

              // this.A_full{i_scale} = [ ...
              // X.*Ix(:)
              // Y.*Ix(:)
              // Ix(:)
              // X.*Iy(:)
              // Y.*Iy(:)
              // Iy(:)
              // -X.^2.*Ix(:)-X.*Y.*Iy(:)
              // -X.*Y.*Ix(:)-Y.^2.*Iy(:) ];

              Array<f32> tmp1(1, numPointsY*numPointsX, memory);
              Array<f32> tmp2(1, numPointsY*numPointsX, memory);

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

              //{
              //  Matlab matlab(false);
              //  matlab.PutArray(this->A_full[iScale], "A_full_iScale");
              //  matlab.PutArray(templateDerivativeX, "templateDerivativeX");
              //  matlab.PutArray(templateDerivativeY, "templateDerivativeY");
              //  matlab.PutArray(templateImage, "templateImage");
              //  //matlab.PutArray(, "");
              //}
            }

            {
              PUSH_MEMORY_STACK(memory);

              Array<f32> GaussianTmp(numPointsY, numPointsX, memory);

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

              Array<f32> templateWeightsTmp(numPointsY, numPointsX, memory);

              // W_mask = interp2(double(targetMask), xi, yi, 'linear', 0);
              if(Interp2(templateMask, xTransformed, yTransformed, templateWeightsTmp, INTERPOLATE_LINEAR) != RESULT_OK)
                return RESULT_FAIL;

              // W_ = W_mask .* GaussianTmp;
              Matrix::DotMultiply<f32,f32,f32>(templateWeightsTmp, GaussianTmp, templateWeightsTmp);

              // Set the boundaries to zero, since these won't be correctly estimated
              templateWeightsTmp(0,0,0,0).Set(0);
              templateWeightsTmp(-1,-1,0,0).Set(0);
              templateWeightsTmp(0,0,-1,-1).Set(0);
              templateWeightsTmp(-1,-1,-1,-1).Set(0);

              Matrix::Vectorize(true, templateWeightsTmp, templateWeights[iScale]);
            } // PUSH_MEMORY_STACK(memory);
          } // for(s32 iScale=0; iScale<this->numPyramidLevels; iScale++, fScale++)
        } // PUSH_MEMORY_STACK(memory);

        this->isValid = true;

        return RESULT_OK;
      }

      Result LucasKanadeTracker_f32::UpdateTrack(const Array<u8> &nextImage, const s32 maxIterations, const f32 convergenceTolerance, MemoryStack memory)
      {
        for(s32 iScale=numPyramidLevels-1; iScale>=0; iScale--) {
          bool converged = false;

          if(IterativelyRefineTrack(nextImage, maxIterations, iScale, convergenceTolerance, TRANSFORM_TRANSLATION, converged, memory) != RESULT_OK)
            return RESULT_FAIL;

          this->get_transformation().Print("Translation");

          if(this->transformation.get_transformType() != TRANSFORM_TRANSLATION) {
            if(IterativelyRefineTrack(nextImage, maxIterations, iScale, convergenceTolerance, this->transformation.get_transformType(), converged, memory) != RESULT_OK)
              return RESULT_FAIL;

            this->get_transformation().Print("Other");
          }
        } // for(s32 iScale=numPyramidLevels; iScale>=0; iScale--)

        return RESULT_OK;
      }

      Result LucasKanadeTracker_f32::IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const TransformType curTransformType, bool &converged, MemoryStack memory)
      {
        AnkiConditionalErrorAndReturnValue(this->isInitialized == true,
          RESULT_FAIL, "LucasKanadeTracker_f32::InitializeTemplate", "This object is not initialized");

        AnkiConditionalErrorAndReturnValue(nextImage.IsValid(),
          RESULT_FAIL, "LucasKanadeTracker_f32::IterativelyRefineTrack", "nextImage is not valid");

        AnkiConditionalErrorAndReturnValue(maxIterations > 0 && maxIterations < 1000,
          RESULT_FAIL, "LucasKanadeTracker_f32::IterativelyRefineTrack", "maxIterations must be greater than zero and less than 1000");

        AnkiConditionalErrorAndReturnValue(whichScale >= 0 && whichScale < this->numPyramidLevels,
          RESULT_FAIL, "LucasKanadeTracker_f32::IterativelyRefineTrack", "whichScale is invalid");

        AnkiConditionalErrorAndReturnValue(convergenceTolerance > 0.0f,
          RESULT_FAIL, "LucasKanadeTracker_f32::IterativelyRefineTrack", "convergenceTolerance must be greater than zero");

        const s32 numPointsY = templateCoordinates[whichScale].get_yGridVector().get_size();
        const s32 numPointsX = templateCoordinates[whichScale].get_xGridVector().get_size();

        Array<f32> A_part;

        s32 numSystemParameters = -1;
        if(curTransformType == TRANSFORM_TRANSLATION) {
          numSystemParameters = 2;

          // Translation-only can be performed by grabbing a few rows of the A_full matrix
          if(this->get_transformation().get_transformType() == TRANSFORM_PROJECTIVE) {
            A_part = Array<f32>(2, this->A_full[whichScale].get_size(1), memory);
            A_part(0,-1,0,-1).Set(this->A_full[whichScale](2,3,5,0,1,-1)); // grab the 2nd and 5th rows
          } else if(this->get_transformation().get_transformType() == TRANSFORM_TRANSLATION) {
            A_part = this->A_full[whichScale];
          } else {
            assert(false);
          }
        } else if(curTransformType == TRANSFORM_PROJECTIVE) {
          numSystemParameters = 8;

          A_part = this->A_full[whichScale];
        } else {
          assert(false);
        }

        Array<f32> xPrevious(1, numPointsY*numPointsX, memory);
        Array<f32> yPrevious(1, numPointsY*numPointsX, memory);

        Array<f32> xIn(1, numPointsY*numPointsX, memory);
        Array<f32> yIn(1, numPointsY*numPointsX, memory);

        {
          PUSH_MEMORY_STACK(memory);

          Array<f32> xIn2d = templateCoordinates[whichScale].EvaluateX2(memory);
          Array<f32> yIn2d = templateCoordinates[whichScale].EvaluateY2(memory);

          Matrix::Vectorize(true, xIn2d, xIn);
          Matrix::Vectorize(true, yIn2d, yIn);
        } // PUSH_MEMORY_STACK(memory);

        converged = false;

        for(s32 iteration=0; iteration<maxIterations; iteration++) {
          PUSH_MEMORY_STACK(memory);

          const f32 scale = static_cast<f32>(1 << whichScale);

          // [xi, yi] = this.getImagePoints(i_scale);
          Array<f32> xTransformed(1, numPointsY*numPointsX, memory);
          Array<f32> yTransformed(1, numPointsY*numPointsX, memory);

          if(transformation.TransformPoints(xIn, yIn, scale, this->center, xTransformed, yTransformed) != RESULT_OK)
            return RESULT_FAIL;

          {
            PUSH_MEMORY_STACK(memory);

            Array<f32> tmp1(1, numPointsY*numPointsX, memory);
            Array<f32> tmp2(1, numPointsY*numPointsX, memory);

            // change = sqrt(mean((xPrev(:)-xi(:)).^2 + (yPrev(:)-yi(:)).^2));
            Matrix::Subtract<f32,f32,f32>(xPrevious, xTransformed, tmp1);
            Matrix::DotMultiply<f32,f32,f32>(tmp1, tmp1, tmp1);

            Matrix::Subtract<f32,f32,f32>(yPrevious, yTransformed, tmp2);
            Matrix::DotMultiply<f32,f32,f32>(tmp2, tmp2, tmp2);

            Matrix::Add<f32,f32,f32>(tmp1, tmp2, tmp1);

            const f32 change = sqrtf(Matrix::Mean<f32,f32>(tmp1));

            if(change < convergenceTolerance*scale) {
              converged = true;
              return RESULT_OK;
            }
          } // PUSH_MEMORY_STACK(memory);

          Array<f32> nextImageTransformed(1, numPointsY*numPointsX, memory);

          // imgi = interp2(img, xi(:), yi(:), 'linear');
          {
            PUSH_MEMORY_STACK(memory);

            Array<f32> nextImageTransformed2d(1, numPointsY*numPointsX, memory);

            if(Interp2<u8,f32>(nextImage, xTransformed, yTransformed, nextImageTransformed2d, INTERPOLATE_LINEAR, -1.0f) != RESULT_OK)
              return RESULT_FAIL;

            Matrix::Vectorize<f32,f32>(true, nextImageTransformed2d, nextImageTransformed);
          } // PUSH_MEMORY_STACK(memory);

          // inBounds = ~isnan(imgi);
          // Warning: this is also treating real zeros as invalid, but this should not be a big problem
          Find<f32, Comparison::GreaterThanOrEqual<f32,f32>, f32> inBounds(nextImageTransformed, 0.0f);
          const s32 numInBounds = inBounds.get_numMatches();

          if(numInBounds < 16) {
            //AnkiWarn('Template drifted too far out of image.');
            break;
          }

          Array<f32> templateImage(1, numPointsY*numPointsX, memory);
          Matrix::Vectorize(true, this->templateImagePyramid[whichScale], templateImage);

          // It = imgi - this.target{i_scale}(:);
          Array<f32> templateDerivativeT(1, numInBounds, memory);
          {
            PUSH_MEMORY_STACK(memory);
            Array<f32> templateDerivativeT_allPoints(1, numPointsY*numPointsX, memory);
            Matrix::Subtract<f32,f32,f32>(nextImageTransformed, templateImage, templateDerivativeT_allPoints);
            inBounds.SetArray(templateDerivativeT, templateDerivativeT_allPoints, 1);
          }

          Array<f32> AWAt(numSystemParameters, numSystemParameters, memory);

          //  AtW = (A(inBounds,:).*this.W{i_scale}(inBounds,ones(1,size(A,2))))';

          Array<f32> A = inBounds.SetArray(A_part, 1, memory);

          Array<f32> AW(A.get_size(0), A.get_size(1), memory);
          AW(0,-1,0,-1).Set(A);

          {
            PUSH_MEMORY_STACK(memory);
            Array<f32> validTemplateWeights = inBounds.SetArray(templateWeights[whichScale], 1, memory);

            for(s32 y=0; y<numSystemParameters; y++) {
              Matrix::DotMultiply<f32,f32,f32>(AW(y,y,0,-1), validTemplateWeights, AW(y,y,0,-1));
            }
          } // PUSH_MEMORY_STACK(memory);

          //  AtWA = AtW*A(inBounds,:) + diag(this.ridgeWeight*ones(1,size(A,2)));
          Matrix::MultiplyTranspose(A, AW, AWAt);

          Array<f32> ridgeWeightMatrix = Eye<f32>(numSystemParameters, numSystemParameters, memory);
          Matrix::DotMultiply<f32,f32,f32>(ridgeWeightMatrix, ridgeWeight, ridgeWeightMatrix);

          Matrix::Add<f32,f32,f32>(AWAt, ridgeWeightMatrix, AWAt);

          //  b = AtW*It(inBounds);
          Array<f32> b(1,numSystemParameters,memory);
          Matrix::MultiplyTranspose(templateDerivativeT, AW, b);

          // update = AtWA\b;
          Array<f32> update(1,numSystemParameters,memory);

          if(Matrix::SolveLeastSquares_f32(AWAt, b, update, memory) != RESULT_OK)
            return RESULT_FAIL;

          {
            Matlab matlab(false);

            matlab.PutArray(A, "A");
            matlab.PutArray(AW, "AW");
            matlab.PutArray(AWAt, "AWAt");
            matlab.PutArray(b, "b");
            matlab.PutArray(update, "update");
          }

          this->transformation.Update(update, memory, curTransformType);
        } // for(s32 iteration=0; iteration<maxIterations; iteration++)

        return RESULT_OK;
      } // Result LucasKanadeTracker_f32::IterativelyRefineTrack()

      bool LucasKanadeTracker_f32::IsValid() const
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

      Result LucasKanadeTracker_f32::set_transformation(const PlanarTransformation_f32 &transformation)
      {
        const TransformType originalType = this->transformation.get_transformType();

        if(this->transformation.set_transformType(transformation.get_transformType()) != RESULT_OK) {
          this->transformation.set_transformType(originalType);
          return RESULT_FAIL;
        }

        if(this->transformation.set_homography(transformation.get_homography()) != RESULT_OK) {
          this->transformation.set_transformType(originalType);
          return RESULT_FAIL;
        }

        return RESULT_OK;
      }

      PlanarTransformation_f32 LucasKanadeTracker_f32::get_transformation() const
      {
        return transformation;
      }
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki