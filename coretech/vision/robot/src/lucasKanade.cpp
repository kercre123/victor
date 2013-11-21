/**
File: lucaseKanade.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/lucasKanade.h"

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
        const s32 numPoints = xIn.get_size(1);

        AnkiConditionalErrorAndReturnValue(xIn.IsValid() && yIn.IsValid() && xOut.IsValid() && yOut.IsValid(),
          RESULT_FAIL, "PlaneTransformation_f32::TransformPoints", "All inputs and outputs must be allocated and valid");

        AnkiConditionalErrorAndReturnValue(xIn.get_rawDataPointer() != xOut.get_rawDataPointer() && yIn.get_rawDataPointer() != yOut.get_rawDataPointer(),
          RESULT_FAIL, "PlaneTransformation_f32::TransformPoints", "In and Out arrays must be in different memory locations");

        AnkiConditionalErrorAndReturnValue(
          xIn.get_size(0) == 1 && yIn.get_size(0) == 1 && xOut.get_size(0) == 1 && yOut.get_size(0) == 1 &&
          xIn.get_size(1) == numPoints && yIn.get_size(1) == numPoints && xOut.get_size(1) == numPoints && yOut.get_size(1) == numPoints,
          RESULT_FAIL, "PlaneTransformation_f32::TransformPoints", "All inputs and outputs must be 1xN");

        //Array<f32> tmp(1, numPoints, scratch);

        const f32 * restrict pXIn = xIn.Pointer(0,0);
        const f32 * restrict pYIn = yIn.Pointer(0,0);
        f32 * restrict pXOut = xOut.Pointer(0,0);
        f32 * restrict pYOut = yOut.Pointer(0,0);

        if(transformType == TRANSFORM_TRANSLATION) {
          const f32 dx = homography[0][2];
          const f32 dy = homography[1][2];
          for(s32 i=0; i<numPoints; i++) {
            pXOut[i] = pXIn[i] + dx + centerOffset.x;
            pYOut[i] = pYIn[i] + dy + centerOffset.y;
          }
        } else {
          return RESULT_FAIL;
        }

        return RESULT_OK;
      }

      LucasKanadeTracker_f32::LucasKanadeTracker_f32(const s32 templateImageHeight, const s32 templateImageWidth, const s32 numPyramidLevels, const TransformType transformType, const f32 ridgeWeight, MemoryStack &memory)
        : isValid(false), templateImageHeight(templateImageHeight), templateImageWidth(templateImageWidth), numPyramidLevels(numPyramidLevels), transformation(PlaneTransformation_f32(transformType)), ridgeWeight(ridgeWeight), isInitialized(false)
      {
        AnkiConditionalErrorAndReturn(templateImageHeight > 0 && templateImageWidth > 0 && (templateImageHeight%ANKI_VISION_IMAGE_WIDTH_MULTIPLE)==0 && (templateImageWidth%ANKI_VISION_IMAGE_WIDTH_MULTIPLE)==0,
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

        /*        A_full[0] = Array<f32>(8, templateImageHeight*templateImageWidth, memory);
        AnkiConditionalErrorAndReturn(A_full[0].IsValid(),
        "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "Could not allocate A_full[0]");

        for(s32 i=1; i<numPyramidLevels; i++) {
        const s32 curTemplateHeight = templateImageHeight >> i;
        const s32 curTemplateWidth = templateImageWidth >> i;

        A_full[i] = Array<f32>(8, curTemplateHeight*curTemplateWidth, memory);;

        AnkiConditionalErrorAndReturn(A_full[i].IsValid(),
        "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "Could not allocate A_full[i]");
        }

        templateMask = Array<f32>(templateImageHeight, templateImageWidth, memory);
        AnkiConditionalErrorAndReturn(templateMask.IsValid(),
        "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "Could not allocate templateMask");

        templateWeights = Array<f32>(templateImageHeight, templateImageWidth, memory);
        AnkiConditionalErrorAndReturn(templateWeights.IsValid(),
        "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "Could not allocate templateWeights");*/

        this->isValid = true;
      }

      Result LucasKanadeTracker_f32::InitializeTemplate(const Array<u8> &templateImage, const Rectangle<f32> templateRegion, MemoryStack &memory)
      {
        const bool isColumnMajor = true; // TODO: change to false, which will probably be faster

        AnkiConditionalErrorAndReturnValue(this->isInitialized == false,
          RESULT_FAIL, "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "This object has already been initialized");

        AnkiConditionalErrorAndReturnValue(templateImageHeight == templateImage.get_size(0) && templateImageWidth == templateImage.get_size(1),
          RESULT_FAIL, "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "template size doesn't match constructor");

        // We do this early, before any memory is allocated This way, an early return won't be able
        // to leak memory with multiple calls to this object
        this->isInitialized = true;

        templateMask.SetZero();
        templateMask(
          static_cast<s32>(Roundf(templateRegion.top)),
          static_cast<s32>(Roundf(templateRegion.bottom)),
          static_cast<s32>(Roundf(templateRegion.left)),
          static_cast<s32>(Roundf(templateRegion.right))).Set(1.0f);

        this->templateRegionHeight = templateRegion.bottom - templateRegion.top;
        this->templateRegionWidth = templateRegion.right - templateRegion.left;

        this->center.y = this->templateRegionHeight / 2;
        this->center.x = this->templateRegionWidth / 2;

        this->templateWeightsSigma = sqrtf(this->templateRegionWidth*this->templateRegionWidth + this->templateRegionHeight*this->templateRegionHeight) / 2.0f;

        f32 fScale = 0.0f;
        for(s32 iScale=0; iScale<this->numPyramidLevels; iScale++, fScale++) {
          const f32 scale = powf(2.0f, fScale);

          templateCoordinates[iScale] = Meshgrid<f32>(
            Linspace(-this->templateRegionWidth/2.0f, this->templateRegionWidth/2.0f, static_cast<s32>(Roundf(this->templateRegionWidth/scale))),
            Linspace(-this->templateRegionHeight/2.0f, this->templateRegionHeight/2.0f, static_cast<s32>(Roundf(this->templateRegionHeight/scale))));
        }

        fScale = 0.0f;
        for(s32 iScale=0; iScale<this->numPyramidLevels; iScale++, fScale++) {
          PUSH_MEMORY_STACK(memory);

          const f32 scale = powf(2.0f, fScale);

          const s32 numValidPoints = templateCoordinates[iScale].get_numElements();

          Array<f32> xIn = templateCoordinates[iScale].EvaluateX(isColumnMajor, memory);
          Array<f32> yIn = templateCoordinates[iScale].EvaluateY(isColumnMajor, memory);

          assert(xIn.get_size(1) == numValidPoints);
          assert(xIn.get_size(1) == yIn.get_size(1));

          Array<f32> xTransformed(1, numValidPoints, memory);
          Array<f32> yTransformed(1, numValidPoints, memory);

          if(transformation.TransformPoints(xIn, yIn, scale, this->center, xTransformed, yTransformed) != RESULT_OK)
            return RESULT_FAIL;

          A_full[iScale] = Array<f32>(8, numValidPoints, memory);
          AnkiConditionalErrorAndReturnValue(A_full[iScale].IsValid(),
            RESULT_FAIL, "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "Could not allocate A_full[iScale]");
        }

        return RESULT_OK;
      }

      bool LucasKanadeTracker_f32::IsValid() const
      {
        if(!this->isValid)
          return false;

        if(!A_full.IsValid())
          return false;

        for(s32 i=0; i<numPyramidLevels; i++) {
          if(!A_full[i].IsValid())
            return false;
        }

        if(!templateMask.IsValid())
          return false;

        if(!templateWeights.IsValid())
          return false;

        return true;
      }
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki