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

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
      PlaneTransformation_f32::PlaneTransformation_f32(TransformType transformType)
        : transformType(transformType)
      {
        homography[0][0] = 1.0f; homography[0][1] = 0.0f; homography[0][2] = 0.0f;
        homography[1][0] = 0.0f; homography[1][1] = 1.0f; homography[1][2] = 0.0f;
        homography[2][0] = 0.0f; homography[2][1] = 0.0f; homography[2][2] = 1.0f;
      }

      Result PlaneTransformation_f32::TransformPoints(
        const Array<f32> &xIn, const Array<f32> &yIn,
        const f32 scale,
        const Point<f32> &centerOffset,
        Array<f32> &xOut, Array<f32> &yOut)
      {
        //const s32 numPoints = xIn.get_size(1);

        AnkiConditionalErrorAndReturnValue(xIn.IsValid() && yIn.IsValid() && xOut.IsValid() && yOut.IsValid(),
          RESULT_FAIL, "PlaneTransformation_f32::TransformPoints", "All inputs and outputs must be allocated and valid");

        AnkiConditionalErrorAndReturnValue(xIn.get_rawDataPointer() != xOut.get_rawDataPointer() && yIn.get_rawDataPointer() != yOut.get_rawDataPointer(),
          RESULT_FAIL, "PlaneTransformation_f32::TransformPoints", "In and Out arrays must be in different memory locations");

        AnkiConditionalErrorAndReturnValue(
          xIn.get_size(0) == yIn.get_size(0) && xIn.get_size(0) == xOut.get_size(0) && xIn.get_size(0) == yOut.get_size(0) &&
          xIn.get_size(1) == yIn.get_size(1) && xIn.get_size(1) == xOut.get_size(1) && xIn.get_size(1) == yOut.get_size(1),
          RESULT_FAIL, "PlaneTransformation_f32::TransformPoints", "All inputs and outputs must be the same size");

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
        } else {
          // Should be checked earlier
          assert(false);
          return RESULT_FAIL;
        }

        return RESULT_OK;
      }

      LucasKanadeTracker_f32::LucasKanadeTracker_f32(const s32 templateImageHeight, const s32 templateImageWidth, const s32 numPyramidLevels, const TransformType transformType, const f32 ridgeWeight, MemoryStack &memory)
        : isValid(false), templateImageHeight(templateImageHeight), templateImageWidth(templateImageWidth), numPyramidLevels(numPyramidLevels), transformation(PlaneTransformation_f32(transformType)), ridgeWeight(ridgeWeight), isInitialized(false)
      {
        AnkiConditionalErrorAndReturn(templateImageHeight > 0 && templateImageWidth > 0,
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "template widths and heights must be greater than zero, and multiples of %d", ANKI_VISION_IMAGE_WIDTH_MULTIPLE);

        AnkiConditionalErrorAndReturn(numPyramidLevels > 0,
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "numPyramidLevels must be greater than zero");

        AnkiConditionalErrorAndReturn(transformType==TRANSFORM_TRANSLATION,
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "Only TRANSFORM_TRANSLATION is supported");

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

        AnkiConditionalErrorAndReturnValue(this->isInitialized == false,
          RESULT_FAIL, "LucasKanadeTracker_f32::InitializeTemplate", "This object has already been initialized");

        AnkiConditionalErrorAndReturnValue(templateImageHeight == templateImage.get_size(0) && templateImageWidth == templateImage.get_size(1),
          RESULT_FAIL, "LucasKanadeTracker_f32::InitializeTemplate", "template size doesn't match constructor");

        // We do this early, before any memory is allocated This way, an early return won't be able
        // to leak memory with multiple calls to this object
        this->isInitialized = true;

        this->templateRegionHeight = templateRegion.bottom - templateRegion.top + 1;
        this->templateRegionWidth = templateRegion.right - templateRegion.left + 1;

        this->templateWeightsSigma = sqrtf(this->templateRegionWidth*this->templateRegionWidth + this->templateRegionHeight*this->templateRegionHeight) / 2.0f;

        this->center.y = (templateRegion.bottom + templateRegion.top) / 2;
        this->center.x = (templateRegion.right + templateRegion.left) / 2;

        // Allocate all permanent memory
        this->templateImagePyramid[0] = Array<u8>(templateImageHeight, templateImageWidth, memory);

        f32 fScale = 0.0f;
        for(s32 iScale=0; iScale<this->numPyramidLevels; iScale++, fScale++) {
          const f32 scale = powf(2.0f, fScale);

          templateCoordinates[iScale] = Meshgrid<f32>(
            Linspace(-this->templateRegionWidth/2.0f, this->templateRegionWidth/2.0f, static_cast<s32>(Roundf(this->templateRegionWidth/scale))),
            Linspace(-this->templateRegionHeight/2.0f, this->templateRegionHeight/2.0f, static_cast<s32>(Roundf(this->templateRegionHeight/scale))));

          const s32 numValidPoints = templateCoordinates[iScale].get_numElements();

          this->A_full[iScale] = Array<f32>(8, numValidPoints, memory);
          AnkiConditionalErrorAndReturnValue(this->A_full[iScale].IsValid(),
            RESULT_FAIL, "LucasKanadeTracker_f32::InitializeTemplate", "Could not allocate A_full[iScale]");

          const s32 numPointsY = templateCoordinates[iScale].get_yGridVector().get_size();
          const s32 numPointsX = templateCoordinates[iScale].get_xGridVector().get_size();

          this->templateImagePyramid[iScale] = Array<u8>(numPointsY, numPointsX, memory);

          AnkiConditionalErrorAndReturnValue(this->templateImagePyramid[iScale].IsValid(),
            RESULT_FAIL, "LucasKanadeTracker_f32::InitializeTemplate", "Could not allocate templateImagePyramid[i]");

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

          fScale = 0.0f;
          for(s32 iScale=0; iScale<this->numPyramidLevels; iScale++, fScale++) {
            PUSH_MEMORY_STACK(memory);

            const s32 numPointsY = templateCoordinates[iScale].get_yGridVector().get_size();
            const s32 numPointsX = templateCoordinates[iScale].get_xGridVector().get_size();

            const f32 scale = powf(2.0f, fScale);

            Array<f32> xTransformed(numPointsY, numPointsX, memory);
            Array<f32> yTransformed(numPointsY, numPointsX, memory);

            {
              PUSH_MEMORY_STACK(memory);

              Array<f32> xIn = templateCoordinates[iScale].EvaluateX2(memory);
              Array<f32> yIn = templateCoordinates[iScale].EvaluateY2(memory);

              assert(xIn.get_size(0) == yIn.get_size(0));
              assert(xIn.get_size(1) == yIn.get_size(1));

              if(transformation.TransformPoints(xIn, yIn, scale, this->center, xTransformed, yTransformed) != RESULT_OK)
                return RESULT_FAIL;
            } // PUSH_MEMORY_STACK(memory);

            if(Interp2(templateImage, xTransformed, yTransformed, this->templateImagePyramid[iScale], INTERPOLATE_BILINEAR) != RESULT_OK)
              return RESULT_FAIL;

            //{
            //  Matlab matlab(false);
            //  matlab.PutArray(xIn, "xIn");
            //  matlab.PutArray(yIn, "yIn");
            //  matlab.PutArray(xTransformed, "xTransformed");
            //  matlab.PutArray(yTransformed, "yTransformed");
            //  matlab.PutArray(this->templateImagePyramid[iScale], "templateImagePyramid0");
            //  matlab.PutArray(templateImage, "templateImage");
            //}

            Array<f32> templateDerivativeX(numPointsY, numPointsX, memory);
            Array<f32> templateDerivativeY(numPointsY, numPointsX, memory);

            // Ix = (image_right(targetBlur) - image_left(targetBlur))/2 * spacing;
            // Iy = (image_down(targetBlur) - image_up(targetBlur))/2 * spacing;
            Matrix::Subtract<u8,f32,f32>(templateImagePyramid[iScale](1,-2,2,-1), templateImagePyramid[iScale](1,-2,0,-3), templateDerivativeX(1,-2,1,-2));
            Matrix::DotMultiply<f32,f32,f32>(templateDerivativeX, scale / 2.0f, templateDerivativeX);

            Matrix::Subtract<u8,f32,f32>(templateImagePyramid[iScale](2,-1,1,-2), templateImagePyramid[iScale](0,-3,1,-2), templateDerivativeY(1,-2,1,-2));
            Matrix::DotMultiply<f32,f32,f32>(templateDerivativeY, scale / 2.0f, templateDerivativeY);

            {
              PUSH_MEMORY_STACK(memory);

              Array<f32> GaussianTmp(numPointsY, numPointsX, memory);

              // GaussianTmp = exp(-((this.xgrid{i_scale}).^2 + (this.ygrid{i_scale}).^2) / (2*(W_sigma)^2));
              {
                PUSH_MEMORY_STACK(memory);

                // TODO: if recomputing this is slow, keep the old version
                Array<f32> xIn = templateCoordinates[iScale].EvaluateX2(memory);
                Array<f32> yIn = templateCoordinates[iScale].EvaluateY2(memory);

                Array<f32> tmp(numPointsY, numPointsX, memory);

                Matrix::DotMultiply<f32,f32,f32>(xIn, xIn, GaussianTmp);
                Matrix::DotMultiply<f32,f32,f32>(yIn, yIn, tmp);
                Matrix::Add<f32,f32,f32>(GaussianTmp, tmp, GaussianTmp);
                Matrix::Subtract<f32,f32,f32>(0.0f, GaussianTmp, GaussianTmp);
                Matrix::DotMultiply<f32,f32,f32>(GaussianTmp, 1.0f/(2.0f*templateWeightsSigma*templateWeightsSigma), GaussianTmp);
                Matrix::Exp<f32,f32,f32>(GaussianTmp, GaussianTmp);
              }

              // W_mask = interp2(double(targetMask), xi, yi, 'linear', 0);

              Array<f32> templateWeightsTmp(numPointsY, numPointsX, memory);

              if(Interp2(templateMask, xTransformed, yTransformed, templateWeightsTmp, INTERPOLATE_BILINEAR) != RESULT_OK)
                return RESULT_FAIL;

              // W_ = W_mask .* GaussianTmp;
              Matrix::DotMultiply<f32,f32,f32>(templateWeightsTmp, GaussianTmp, templateWeightsTmp);

              {
                Matlab matlab(false);
                matlab.PutArray(this->templateImagePyramid[iScale], "templateImagePyramid0");
                matlab.PutArray(templateDerivativeX, "templateDerivativeX");
                matlab.PutArray(templateDerivativeY, "templateDerivativeY");
                matlab.PutArray(GaussianTmp, "GaussianTmp");
                matlab.PutArray(templateWeightsTmp, "templateWeightsTmp");
              }
            } // PUSH_MEMORY_STACK(memory);

            const s32 numValidPoints = templateCoordinates[iScale].get_numElements();
          } // for(s32 iScale=0; iScale<this->numPyramidLevels; iScale++, fScale++)
        } // PUSH_MEMORY_STACK(memory);

        return RESULT_OK;
      }

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
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki