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
#include "anki/common/robot/benchmarking_c.h"

#include "anki/vision/robot/miscVisionKernels.h"
#include "anki/vision/robot/imageProcessing.h"

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
      static Result lastResult;

      PlanarTransformation_f32::PlanarTransformation_f32(const TransformType transformType, const Quadrilateral<f32> &initialCorners, const Array<f32> &initialHomography, MemoryStack &memory)
        : transformType(transformType), initialCorners(initialCorners),
        centerOffset(initialCorners.ComputeCenter())
      {
        AnkiConditionalErrorAndReturn(transformType==TRANSFORM_TRANSLATION || transformType==TRANSFORM_AFFINE || transformType==TRANSFORM_PROJECTIVE,
          "PlanarTransformation_f32::PlanarTransformation_f32", "Invalid transformType %d", transformType);

        // Store the initial quad recentered around the centerOffset.
        // get_transformedCorners() will add it back
        for(s32 i_pt=0; i_pt<4; ++i_pt) {
          this->initialCorners[i_pt] -= this->centerOffset;
        }

        this->homography = Eye<f32>(3, 3, memory);

        if(initialHomography.IsValid()) {
          this->homography.Set(initialHomography);
        }
      }

      PlanarTransformation_f32::PlanarTransformation_f32(const TransformType transformType, const Quadrilateral<f32> &initialCorners, MemoryStack &memory)
        : transformType(transformType), initialCorners(initialCorners),
        centerOffset(initialCorners.ComputeCenter())
      {
        AnkiConditionalErrorAndReturn(transformType==TRANSFORM_TRANSLATION || transformType==TRANSFORM_AFFINE || transformType==TRANSFORM_PROJECTIVE,
          "PlanarTransformation_f32::PlanarTransformation_f32", "Invalid transformType %d", transformType);

        // Store the initial quad recentered around the centerOffset.
        // get_transformedCorners() will add it back
        for(s32 i_pt=0; i_pt<4; ++i_pt) {
          this->initialCorners[i_pt] -= this->centerOffset;
        }

        this->homography = Eye<f32>(3, 3, memory);
      }

      PlanarTransformation_f32::PlanarTransformation_f32(const TransformType transformType, MemoryStack &memory)
      {
        AnkiConditionalErrorAndReturn(transformType==TRANSFORM_TRANSLATION || transformType==TRANSFORM_AFFINE || transformType==TRANSFORM_PROJECTIVE,
          "PlanarTransformation_f32::PlanarTransformation_f32", "Invalid transformType %d", transformType);

        this->transformType = transformType;
        initialCorners = Quadrilateral<f32>(Point<f32>(0.0f,0.0f),
          Point<f32>(0.0f,0.0f),
          Point<f32>(0.0f,0.0f),
          Point<f32>(0.0f,0.0f));
        centerOffset = initialCorners.ComputeCenter();

        // Store the initial quad recentered around the centerOffset.
        // get_transformedCorners() will add it back
        for(s32 i_pt=0; i_pt<4; ++i_pt) {
          this->initialCorners[i_pt] -= this->centerOffset;
        }

        this->homography = Eye<f32>(3, 3, memory);
      }

      PlanarTransformation_f32::PlanarTransformation_f32()
      {
        initialCorners = Quadrilateral<f32>(Point<f32>(-1.0f,-1.0f), Point<f32>(-1.0f,-1.0f), Point<f32>(-1.0f,-1.0f), Point<f32>(-1.0f,-1.0f));
        centerOffset = initialCorners.ComputeCenter();
      }

      Result PlanarTransformation_f32::TransformPoints(
        const Array<f32> &xIn, const Array<f32> &yIn,
        const f32 scale,
        Array<f32> &xOut, Array<f32> &yOut) const
      {
        return TransformPointsStatic(xIn, yIn, scale, this->centerOffset, xOut, yOut, this->get_transformType(), this->get_homography());
      }

      Result PlanarTransformation_f32::Update(const Array<f32> &update, MemoryStack scratch, TransformType updateType)
      {
        AnkiConditionalErrorAndReturnValue(update.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "PlanarTransformation_f32::Update", "update is not valid");

        AnkiConditionalErrorAndReturnValue(update.get_size(0) == 1,
          RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::Update", "update is the incorrect size");

        if(updateType == TRANSFORM_UNKNOWN) {
          updateType = this->transformType;
        }

        // An Object of a given transformation type can only be updated with a simpler transformation
        if(this->transformType == TRANSFORM_TRANSLATION) {
          AnkiConditionalErrorAndReturnValue(updateType == TRANSFORM_TRANSLATION,
            RESULT_FAIL_INVALID_PARAMETERS, "PlanarTransformation_f32::Update", "cannot update this transform with the update type %d", updateType);
        } else if(this->transformType == TRANSFORM_AFFINE) {
          AnkiConditionalErrorAndReturnValue(updateType == TRANSFORM_TRANSLATION || updateType == TRANSFORM_AFFINE,
            RESULT_FAIL_INVALID_PARAMETERS, "PlanarTransformation_f32::Update", "cannot update this transform with the update type %d", updateType);
        } else if(this->transformType == TRANSFORM_PROJECTIVE) {
          AnkiConditionalErrorAndReturnValue(updateType == TRANSFORM_TRANSLATION|| updateType == TRANSFORM_AFFINE || updateType == TRANSFORM_PROJECTIVE,
            RESULT_FAIL_INVALID_PARAMETERS, "PlanarTransformation_f32::Update", "cannot update this transform with the update type %d", updateType);
        } else {
          AnkiAssert(false);
        }

        const f32 * pUpdate = update[0];

        if(updateType == TRANSFORM_TRANSLATION) {
          AnkiConditionalErrorAndReturnValue(update.get_size(1) == TRANSFORM_TRANSLATION>>8,
            RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::Update", "update is the incorrect size");

          // this.tform(1:2,3) = this.tform(1:2,3) - update;
          homography[0][2] -= pUpdate[0];
          homography[1][2] -= pUpdate[1];
        } else { // if(updateType == TRANSFORM_TRANSLATION)
          Array<f32> updateArray(3,3,scratch);

          if(updateType == TRANSFORM_AFFINE) {
            AnkiConditionalErrorAndReturnValue(update.get_size(1) == TRANSFORM_AFFINE>>8,
              RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::Update", "update is the incorrect size");

            updateArray[0][0] = 1.0f + pUpdate[0]; updateArray[0][1] = pUpdate[1];        updateArray[0][2] = pUpdate[2];
            updateArray[1][0] = pUpdate[3];        updateArray[1][1] = 1.0f + pUpdate[4]; updateArray[1][2] = pUpdate[5];
            updateArray[2][0] = 0.0f;              updateArray[2][1] = 0.0f;              updateArray[2][2] = 1.0f;
          } else if(updateType == TRANSFORM_PROJECTIVE) {
            AnkiConditionalErrorAndReturnValue(update.get_size(1) == TRANSFORM_PROJECTIVE>>8,
              RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::Update", "update is the incorrect size");

            // tformUpdate = eye(3) + [update(1:3)'; update(4:6)'; update(7:8)' 0];
            updateArray[0][0] = 1.0f + pUpdate[0]; updateArray[0][1] = pUpdate[1];        updateArray[0][2] = pUpdate[2];
            updateArray[1][0] = pUpdate[3];        updateArray[1][1] = 1.0f + pUpdate[4]; updateArray[1][2] = pUpdate[5];
            updateArray[2][0] = pUpdate[6];        updateArray[2][1] = pUpdate[7];        updateArray[2][2] = 1.0f;
          } else {
            AnkiError("PlanarTransformation_f32::Update", "Unknown transformation type %d", updateType);
            return RESULT_FAIL_INVALID_PARAMETERS;
          }

          // this.tform = this.tform*inv(tformUpdate);
          Invert3x3(
            updateArray[0][0], updateArray[0][1], updateArray[0][2],
            updateArray[1][0], updateArray[1][1], updateArray[1][2],
            updateArray[2][0], updateArray[2][1], updateArray[2][2]);

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

      Quadrilateral<f32> PlanarTransformation_f32::TransformQuadrilateral(const Quadrilateral<f32> &in, MemoryStack scratch, const f32 scale) const
      {
        Array<f32> xIn(1,4,scratch);
        Array<f32> yIn(1,4,scratch);
        Array<f32> xOut(1,4,scratch);
        Array<f32> yOut(1,4,scratch);

        for(s32 i=0; i<4; i++) {
          xIn[0][i] = in.corners[i].x;
          yIn[0][i] = in.corners[i].y;
        }

        TransformPoints(xIn, yIn, scale, xOut, yOut);

        Quadrilateral<f32> out;

        for(s32 i=0; i<4; i++) {
          out.corners[i].x = xOut[0][i];
          out.corners[i].y = yOut[0][i];
        }

        return out;
      }

      Result PlanarTransformation_f32::Set(const PlanarTransformation_f32 &newTransformation)
      {
        const TransformType originalType = this->get_transformType();

        if((lastResult = this->set_transformType(newTransformation.get_transformType())) != RESULT_OK) {
          this->set_transformType(originalType);
          return lastResult;
        }

        if((lastResult = this->set_homography(newTransformation.get_homography())) != RESULT_OK) {
          this->set_transformType(originalType);
          return lastResult;
        }

        this->centerOffset = newTransformation.get_centerOffset();

        this->initialCorners = initialCorners;

        return RESULT_OK;
      }

      Result PlanarTransformation_f32::set_transformType(const TransformType transformType)
      {
        if(transformType == TRANSFORM_TRANSLATION || transformType == TRANSFORM_AFFINE || transformType == TRANSFORM_PROJECTIVE) {
          this->transformType = transformType;
        } else {
          AnkiError("PlanarTransformation_f32::set_transformType", "Unknown transformation type %d", transformType);
          return RESULT_FAIL_INVALID_PARAMETERS;
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
          return RESULT_FAIL_INVALID_SIZE;

        AnkiAssert(FLT_NEAR(in[2][2], 1.0f));

        return RESULT_OK;
      }

      const Array<f32>& PlanarTransformation_f32::get_homography() const
      {
        return this->homography;
      }

      Result PlanarTransformation_f32::set_initialCorners(const Quadrilateral<f32> &initialCorners)
      {
        this->initialCorners = initialCorners;

        return RESULT_OK;
      }

      const Quadrilateral<f32>& PlanarTransformation_f32::get_initialCorners() const
      {
        return this->initialCorners;
      }

      const Point<f32>& PlanarTransformation_f32::get_centerOffset() const
      {
        return this->centerOffset;
      }

      Quadrilateral<f32> PlanarTransformation_f32::get_transformedCorners(MemoryStack scratch) const
      {
        return this->TransformQuadrilateral(this->get_initialCorners(), scratch);
      }

      Result PlanarTransformation_f32::TransformPointsStatic(
        const Array<f32> &xIn, const Array<f32> &yIn,
        const f32 scale,
        const Point<f32> &centerOffset,
        Array<f32> &xOut, Array<f32> &yOut,
        const TransformType transformType,
        const Array<f32> &homography)
      {
        AnkiConditionalErrorAndReturnValue(homography.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "PlanarTransformation_f32::TransformPoints", "homography is not valid");

        AnkiConditionalErrorAndReturnValue(xIn.IsValid() && yIn.IsValid() && xOut.IsValid() && yOut.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "PlanarTransformation_f32::TransformPoints", "All inputs and outputs must be allocated and valid");

        AnkiConditionalErrorAndReturnValue(xIn.get_rawDataPointer() != xOut.get_rawDataPointer() && yIn.get_rawDataPointer() != yOut.get_rawDataPointer(),
          RESULT_FAIL_ALIASED_MEMORY, "PlanarTransformation_f32::TransformPoints", "In and Out arrays must be in different memory locations");

        AnkiConditionalErrorAndReturnValue(
          xIn.get_size(0) == yIn.get_size(0) && xIn.get_size(0) == xOut.get_size(0) && xIn.get_size(0) == yOut.get_size(0) &&
          xIn.get_size(1) == yIn.get_size(1) && xIn.get_size(1) == xOut.get_size(1) && xIn.get_size(1) == yOut.get_size(1),
          RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::TransformPoints", "All inputs and outputs must be the same size");

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
        } else if(transformType == TRANSFORM_AFFINE) {
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];

          AnkiAssert(FLT_NEAR(homography[2][0], 0.0f));
          AnkiAssert(FLT_NEAR(homography[2][1], 0.0f));
          AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

          for(s32 y=0; y<numPointsY; y++) {
            const f32 * restrict pXIn = xIn.Pointer(y,0);
            const f32 * restrict pYIn = yIn.Pointer(y,0);
            f32 * restrict pXOut = xOut.Pointer(y,0);
            f32 * restrict pYOut = yOut.Pointer(y,0);

            for(s32 x=0; x<numPointsX; x++) {
              //// Remove center offset
              //const f32 xc = pXIn[x] - centerOffset.x;
              //const f32 yc = pYIn[x] - centerOffset.y;

              const f32 xp = (h00*pXIn[x] + h01*pYIn[x] + h02);
              const f32 yp = (h10*pXIn[x] + h11*pYIn[x] + h12);

              // Restore center offset
              pXOut[x] = xp + centerOffset.x;
              pYOut[x] = yp + centerOffset.y;
            }
          }
        } else if(transformType == TRANSFORM_PROJECTIVE) {
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
          const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

          AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

          for(s32 y=0; y<numPointsY; y++) {
            const f32 * restrict pXIn = xIn.Pointer(y,0);
            const f32 * restrict pYIn = yIn.Pointer(y,0);
            f32 * restrict pXOut = xOut.Pointer(y,0);
            f32 * restrict pYOut = yOut.Pointer(y,0);

            for(s32 x=0; x<numPointsX; x++) {
              const f32 wpi = 1.0f / (h20*pXIn[x] + h21*pYIn[x] + h22);

              //// Remove center offset
              //const f32 xc = pXIn[x] - centerOffset.x;
              //const f32 yc = pYIn[x] - centerOffset.y;

              const f32 xp = (h00*pXIn[x] + h01*pYIn[x] + h02) * wpi;
              const f32 yp = (h10*pXIn[x] + h11*pYIn[x] + h12) * wpi;

              // Restore center offset
              pXOut[x] = xp + centerOffset.x;
              pYOut[x] = yp + centerOffset.y;
            }
          }
        } else {
          // Should be checked earlier
          AnkiAssert(false);
          return RESULT_FAIL;
        }

        return RESULT_OK;
      }

      LucasKanadeTracker_f32::LucasKanadeTracker_f32()
        : isValid(false), isInitialized(false)
      {
      }

      LucasKanadeTracker_f32::LucasKanadeTracker_f32(const Array<u8> &templateImage, const Quadrilateral<f32> &templateQuad, const s32 numPyramidLevels, const TransformType transformType, const f32 ridgeWeight, MemoryStack &memory)
        : numPyramidLevels(numPyramidLevels), templateImageHeight(templateImage.get_size(0)), templateImageWidth(templateImage.get_size(1)), ridgeWeight(ridgeWeight), isValid(false), isInitialized(false)
      {
        BeginBenchmark("LucasKanadeTracker_f32");

        AnkiConditionalErrorAndReturn(templateImageHeight > 0 && templateImageWidth > 0,
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "template widths and heights must be greater than zero, and multiples of %d", ANKI_VISION_IMAGE_WIDTH_MULTIPLE);

        AnkiConditionalErrorAndReturn(numPyramidLevels > 0,
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "numPyramidLevels must be greater than zero");

        AnkiConditionalErrorAndReturn(transformType==TRANSFORM_TRANSLATION || transformType == TRANSFORM_AFFINE || transformType==TRANSFORM_PROJECTIVE,
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "Only TRANSFORM_TRANSLATION, TRANSFORM_AFFINE, and TRANSFORM_PROJECTIVE are supported");

        AnkiConditionalErrorAndReturn(ridgeWeight >= 0.0f,
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "ridgeWeight must be greater or equal to zero");

        templateRegion = templateQuad.ComputeBoundingRectangle();

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

        this->transformation = PlanarTransformation_f32(transformType, templateQuad, memory);

        this->isValid = true;

        BeginBenchmark("InitializeTemplate");
        if(LucasKanadeTracker_f32::InitializeTemplate(templateImage, memory) != RESULT_OK) {
          this->isValid = false;
        }
        EndBenchmark("InitializeTemplate");

        EndBenchmark("LucasKanadeTracker_f32");
      }

      Result LucasKanadeTracker_f32::InitializeTemplate(const Array<u8> &templateImage, MemoryStack &memory)
      {
        const bool isOutColumnMajor = false; // TODO: change to false, which will probably be faster

        AnkiConditionalErrorAndReturnValue(this->isValid,
          RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTracker_f32::InitializeTemplate", "This object's constructor failed, so it cannot be initialized");

        AnkiConditionalErrorAndReturnValue(this->isInitialized == false,
          RESULT_FAIL, "LucasKanadeTracker_f32::InitializeTemplate", "This object has already been initialized");

        AnkiConditionalErrorAndReturnValue(templateImageHeight == templateImage.get_size(0) && templateImageWidth == templateImage.get_size(1),
          RESULT_FAIL_INVALID_SIZE, "LucasKanadeTracker_f32::InitializeTemplate", "template size doesn't match constructor");

        AnkiConditionalErrorAndReturnValue(templateRegion.left < templateRegion.right && templateRegion.left >=0 && templateRegion.right < templateImage.get_size(1) &&
          templateRegion.top < templateRegion.bottom && templateRegion.top >=0 && templateRegion.bottom < templateImage.get_size(0),
          RESULT_FAIL, "LucasKanadeTracker_f32::InitializeTemplate", "template rectangle is invalid or out of bounds");

        // We do this early, before any memory is allocated This way, an early return won't be able
        // to leak memory with multiple calls to this object
        this->isInitialized = true;
        this->isValid = false;

        const s32 numTransformationParameters = transformation.get_transformType() >> 8;

        this->templateRegionHeight = templateRegion.bottom - templateRegion.top  + 1;
        this->templateRegionWidth  = templateRegion.right  - templateRegion.left + 1;

        this->templateWeightsSigma = sqrtf(this->templateRegionWidth*this->templateRegionWidth + this->templateRegionHeight*this->templateRegionHeight) / 2.0f;

        // Allocate all permanent memory
        BeginBenchmark("InitializeTemplate.allocate");
        for(s32 iScale=0; iScale<this->numPyramidLevels; iScale++) {
          const f32 scale = static_cast<f32>(1 << iScale);

          templateCoordinates[iScale] = Meshgrid<f32>(
            Linspace(-this->templateRegionWidth/2.0f, this->templateRegionWidth/2.0f, static_cast<s32>(FLT_FLOOR(this->templateRegionWidth/scale))),
            Linspace(-this->templateRegionHeight/2.0f, this->templateRegionHeight/2.0f, static_cast<s32>(FLT_FLOOR(this->templateRegionHeight/scale))));

          const s32 numValidPoints = templateCoordinates[iScale].get_numElements();

          this->A_full[iScale] = Array<f32>(numTransformationParameters, numValidPoints, memory);
          AnkiConditionalErrorAndReturnValue(this->A_full[iScale].IsValid(),
            RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTracker_f32::InitializeTemplate", "Could not allocate A_full[iScale]");

          const s32 numPointsY = templateCoordinates[iScale].get_yGridVector().get_size();
          const s32 numPointsX = templateCoordinates[iScale].get_xGridVector().get_size();

          this->templateImagePyramid[iScale] = Array<u8>(numPointsY, numPointsX, memory);

          AnkiConditionalErrorAndReturnValue(this->templateImagePyramid[iScale].IsValid(),
            RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTracker_f32::InitializeTemplate", "Could not allocate templateImagePyramid[i]");

          this->templateWeights[iScale] = Array<f32>(1, numPointsY*numPointsX, memory);

          AnkiConditionalErrorAndReturnValue(this->templateWeights[iScale].IsValid(),
            RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTracker_f32::InitializeTemplate", "Could not allocate templateWeights[i]");

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
            static_cast<s32>(Roundf(templateRegion.top)),
            static_cast<s32>(Roundf(templateRegion.bottom)),
            static_cast<s32>(Roundf(templateRegion.left)),
            static_cast<s32>(Roundf(templateRegion.right))).Set(1.0f);
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

            AnkiAssert(xIn.get_size(0) == yIn.get_size(0));
            AnkiAssert(xIn.get_size(1) == yIn.get_size(1));

            Array<f32> xTransformed(numPointsY, numPointsX, memory);
            Array<f32> yTransformed(numPointsY, numPointsX, memory);

            BeginBenchmark("InitializeTemplate.transformPoints");
            // Compute the warped coordinates (for later)
            if((lastResult = transformation.TransformPoints(xIn, yIn, scale, xTransformed, yTransformed)) != RESULT_OK)
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
            if(transformation.get_transformType() == TRANSFORM_TRANSLATION) {
              Array<f32> tmp(1, numPointsY*numPointsX, memory);

              Matrix::Vectorize(isOutColumnMajor, templateDerivativeX, tmp);
              this->A_full[iScale](0,0,0,-1).Set(tmp);

              Matrix::Vectorize(isOutColumnMajor, templateDerivativeY, tmp);
              this->A_full[iScale](1,1,0,-1).Set(tmp);
            } else if(transformation.get_transformType() == TRANSFORM_AFFINE || transformation.get_transformType() == TRANSFORM_PROJECTIVE) {
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

              if(transformation.get_transformType() == TRANSFORM_PROJECTIVE) {
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
              } // if(transformation.get_transformType() == TRANSFORM_PROJECTIVE)
            } // else if(transformation.get_transformType() == TRANSFORM_AFFINE || transformation.get_transformType() == TRANSFORM_PROJECTIVE)
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

      Result LucasKanadeTracker_f32::UpdateTrack(const Array<u8> &nextImage, const s32 maxIterations, const f32 convergenceTolerance, const bool useWeights, bool& converged, MemoryStack scratch)
      {
        for(s32 iScale=numPyramidLevels-1; iScale>=0; iScale--) {
          // TODO: remove
          //for(s32 iScale=0; iScale>=0; iScale--) {
          converged = false;

          BeginBenchmark("UpdateTrack.refineTranslation");
          if((lastResult = IterativelyRefineTrack(nextImage, maxIterations, iScale, convergenceTolerance, TRANSFORM_TRANSLATION, useWeights, converged, scratch)) != RESULT_OK)
            return lastResult;
          EndBenchmark("UpdateTrack.refineTranslation");

          //this->get_transformation().Print("Translation");

          if(this->transformation.get_transformType() != TRANSFORM_TRANSLATION) {
            // TODO: remove
            //Array<f32> newH = Eye<f32>(3,3,memory);
            //newH[0][2] = -0.0490;
            //newH[1][2] = -0.1352;
            //this->transformation.set_homography(newH);

            BeginBenchmark("UpdateTrack.refineOther");
            if((lastResult = IterativelyRefineTrack(nextImage, maxIterations, iScale, convergenceTolerance, this->transformation.get_transformType(), useWeights, converged, scratch)) != RESULT_OK)
              return lastResult;
            EndBenchmark("UpdateTrack.refineOther");

            //this->get_transformation().Print("Other");
          }
        } // for(s32 iScale=numPyramidLevels; iScale>=0; iScale--)

        return RESULT_OK;
      }

      Result LucasKanadeTracker_f32::IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const TransformType curTransformType, const bool useWeights, bool &converged, MemoryStack scratch)
      {
        const bool isOutColumnMajor = false; // TODO: change to false, which will probably be faster

        AnkiConditionalErrorAndReturnValue(this->isInitialized == true,
          RESULT_FAIL, "LucasKanadeTracker_f32::IterativelyRefineTrack", "This object is not initialized");

        AnkiConditionalErrorAndReturnValue(nextImage.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTracker_f32::IterativelyRefineTrack", "nextImage is not valid");

        AnkiConditionalErrorAndReturnValue(maxIterations > 0 && maxIterations < 1000,
          RESULT_FAIL_INVALID_PARAMETERS, "LucasKanadeTracker_f32::IterativelyRefineTrack", "maxIterations must be greater than zero and less than 1000");

        AnkiConditionalErrorAndReturnValue(whichScale >= 0 && whichScale < this->numPyramidLevels,
          RESULT_FAIL_INVALID_PARAMETERS, "LucasKanadeTracker_f32::IterativelyRefineTrack", "whichScale is invalid");

        AnkiConditionalErrorAndReturnValue(convergenceTolerance > 0.0f,
          RESULT_FAIL_INVALID_PARAMETERS, "LucasKanadeTracker_f32::IterativelyRefineTrack", "convergenceTolerance must be greater than zero");

        const s32 numPointsY = templateCoordinates[whichScale].get_yGridVector().get_size();
        const s32 numPointsX = templateCoordinates[whichScale].get_xGridVector().get_size();

        Array<f32> A_part;

        BeginBenchmark("IterativelyRefineTrack.extractAPart");
        const s32 numSystemParameters = curTransformType >> 8;
        if(curTransformType == TRANSFORM_TRANSLATION) {
          // Translation-only can be performed by grabbing a few rows of the A_full matrix
          if(this->get_transformation().get_transformType() == TRANSFORM_AFFINE ||
            this->get_transformation().get_transformType() == TRANSFORM_PROJECTIVE) {
              A_part = Array<f32>(2, this->A_full[whichScale].get_size(1), scratch);
              A_part(0,-1,0,-1).Set(this->A_full[whichScale](2,3,5,0,1,-1)); // grab the 2nd and 5th rows
          } else if(this->get_transformation().get_transformType() == TRANSFORM_TRANSLATION) {
            A_part = this->A_full[whichScale];
          } else {
            AnkiAssert(false);
          }
        } else if(curTransformType == TRANSFORM_AFFINE || curTransformType == TRANSFORM_PROJECTIVE) {
          A_part = this->A_full[whichScale];
        } else {
          AnkiAssert(false);
        }
        EndBenchmark("IterativelyRefineTrack.extractAPart");

        //Array<f32> xPrevious(1, numPointsY*numPointsX, scratch);
        //Array<f32> yPrevious(1, numPointsY*numPointsX, scratch);

        // Initialize with some very extreme coordinates
        FixedLengthList<Quadrilateral<f32>> previousCorners(NUM_PREVIOUS_QUADS_TO_COMPARE, scratch);

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

        converged = false;

        for(s32 iteration=0; iteration<maxIterations; iteration++) {
          PUSH_MEMORY_STACK(scratch);

          const f32 scale = static_cast<f32>(1 << whichScale);

          // [xi, yi] = this.getImagePoints(i_scale);
          Array<f32> xTransformed(1, numPointsY*numPointsX, scratch);
          Array<f32> yTransformed(1, numPointsY*numPointsX, scratch);

          BeginBenchmark("IterativelyRefineTrack.transformPoints");
          if((lastResult = transformation.TransformPoints(xIn, yIn, scale, xTransformed, yTransformed)) != RESULT_OK)
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
            AnkiWarn("LucasKanadeTracker_f32::IterativelyRefineTrack", "Template drifted too far out of image.");
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

          //if(curTransformType == TRANSFORM_AFFINE) {
          //  Matlab matlab(false);

          //  matlab.PutArray(A, "A_tmp");
          //  matlab.PutArray(AW, "AW_tmp");
          //  matlab.PutArray(AWAt, "AWAt_tmp");
          //  matlab.EvalStringEcho("if ~exist('A', 'var') A=cell(0); AW=cell(0); AWAt=cell(0); end;");
          //  matlab.EvalStringEcho("A{end+1}=A_tmp; AW{end+1}=AW_tmp; AWAt{end+1}=AWAt_tmp;");
          //  printf("");
          //}

          Array<f32> ridgeWeightMatrix = Eye<f32>(numSystemParameters, numSystemParameters, scratch);
          Matrix::DotMultiply<f32,f32,f32>(ridgeWeightMatrix, ridgeWeight, ridgeWeightMatrix);

          Matrix::Add<f32,f32,f32>(AWAt, ridgeWeightMatrix, AWAt);

          BeginBenchmark("IterativelyRefineTrack.computeb");
          // b = AtW*It(inBounds);
          Array<f32> b(1,numSystemParameters,scratch);
          Matrix::MultiplyTranspose(templateDerivativeT, AW, b);
          EndBenchmark("IterativelyRefineTrack.computeb");

          //if(curTransformType == TRANSFORM_AFFINE) {
          //  Matlab matlab(false);

          //  matlab.PutArray(b, "b_tmp");
          //  //matlab.PutArray(templateDerivativeT, "templateDerivativeT");
          //  //matlab.PutArray(ridgeWeightMatrix, "ridgeWeightMatrix");
          //  matlab.EvalStringEcho("if ~exist('b', 'var') b=cell(0); end;");
          //  matlab.EvalStringEcho("b{end+1}=b_tmp;");
          //  printf("");
          //}

          // update = AtWA\b;
          //AW.Print("AW");
          //AWAt.Print("Orig AWAt");
          //b.Print("Orig b");

          BeginBenchmark("IterativelyRefineTrack.solveForUpdate");
          if((lastResult = Matrix::SolveLeastSquaresWithCholesky(AWAt, b, false)) != RESULT_OK)
            return lastResult;
          EndBenchmark("IterativelyRefineTrack.solveForUpdate");

          //b.Print("Orig update");

          BeginBenchmark("IterativelyRefineTrack.updateTransformation");
          //this->transformation.Print("t1");
          this->transformation.Update(b, scratch, curTransformType);
          //this->transformation.Print("t2");
          EndBenchmark("IterativelyRefineTrack.updateTransformation");

          //if(curTransformType == TRANSFORM_AFFINE) {
          //  Matlab matlab(false);

          //  matlab.PutArray(b, "update_tmp");
          //  matlab.PutArray(this->transformation.get_homography(), "homography_tmp");

          //  matlab.EvalStringEcho("if ~exist('update', 'var') update=cell(0); homography=cell(0); end;");
          //  matlab.EvalStringEcho("update{end+1}=update_tmp; homography{end+1}=homography_tmp;");
          //  printf("");
          //}

          BeginBenchmark("IterativelyRefineTrack.checkForCompletion");
          // Check if we're done with iterations
          {
            PUSH_MEMORY_STACK(scratch);

            Quadrilateral<f32> in(
              Point<f32>(0.0f,0.0f),
              Point<f32>(static_cast<f32>(nextImage.get_size(1)),0.0f),
              Point<f32>(static_cast<f32>(nextImage.get_size(0)),static_cast<f32>(nextImage.get_size(1))),
              Point<f32>(0.0f,static_cast<f32>(nextImage.get_size(1))));

            Quadrilateral<f32> newCorners = transformation.TransformQuadrilateral(in, scratch, scale);

            //const f32 change = sqrtf(Matrix::Mean<f32,f32>(tmp1));
            f32 minChange = 1e10f;
            for(s32 iPrevious=0; iPrevious<NUM_PREVIOUS_QUADS_TO_COMPARE; iPrevious++) {
              f32 change = 0.0f;
              for(s32 i=0; i<4; i++) {
                const f32 dx = previousCorners[iPrevious][i].x - newCorners[i].x;
                const f32 dy = previousCorners[iPrevious][i].y - newCorners[i].y;
                change += sqrtf(dx*dx + dy*dy);
              }
              change /= 4;

              minChange = MIN(minChange, change);
            }

            //printf("newCorners");
            //newCorners.Print();
            //printf("change: %f\n", change);

            if(minChange < convergenceTolerance*scale) {
              converged = true;
              return RESULT_OK;
            }

            for(s32 iPrevious=0; iPrevious<(NUM_PREVIOUS_QUADS_TO_COMPARE-1); iPrevious++) {
              previousCorners[iPrevious] = previousCorners[iPrevious+1];
            }
            previousCorners[NUM_PREVIOUS_QUADS_TO_COMPARE-1] = newCorners;
          } // PUSH_MEMORY_STACK(scratch);
          EndBenchmark("IterativelyRefineTrack.checkForCompletion");
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
        return this->transformation.Set(transformation);
      }

      PlanarTransformation_f32 LucasKanadeTracker_f32::get_transformation() const
      {
        return transformation;
      }

      LucasKanadeTrackerFast::LucasKanadeTrackerFast()
        : isValid(false)
      {
      }

      LucasKanadeTrackerFast::LucasKanadeTrackerFast(const Array<u8> &templateImage, const Quadrilateral<f32> &templateQuad, const s32 numPyramidLevels, const TransformType transformType, const f32 ridgeWeight, MemoryStack &scratch)
        : numPyramidLevels(numPyramidLevels), templateImageHeight(templateImage.get_size(0)), templateImageWidth(templateImage.get_size(1)), ridgeWeight(ridgeWeight), isValid(false)
      {
        BeginBenchmark("LucasKanadeTrackerFast");

        AnkiConditionalErrorAndReturn(templateImageHeight > 0 && templateImageWidth > 0,
          "LucasKanadeTrackerFast::LucasKanadeTrackerFast", "template widths and heights must be greater than zero, and multiples of %d", ANKI_VISION_IMAGE_WIDTH_MULTIPLE);

        AnkiConditionalErrorAndReturn(numPyramidLevels > 0,
          "LucasKanadeTrackerFast::LucasKanadeTrackerFast", "numPyramidLevels must be greater than zero");

        AnkiConditionalErrorAndReturn(transformType==TRANSFORM_TRANSLATION || transformType == TRANSFORM_AFFINE,
          "LucasKanadeTracker_f32::LucasKanadeTracker_f32", "Only TRANSFORM_TRANSLATION or TRANSFORM_AFFINE are supported");

        AnkiConditionalErrorAndReturn(ridgeWeight >= 0.0f,
          "LucasKanadeTrackerFast::LucasKanadeTrackerFast", "ridgeWeight must be greater or equal to zero");

        templateRegion = templateQuad.ComputeBoundingRectangle();

        // All pyramid width except the last one must be divisible by two
        for(s32 i=0; i<(numPyramidLevels-1); i++) {
          const s32 curTemplateHeight = templateImageHeight >> i;
          const s32 curTemplateWidth = templateImageWidth >> i;

          AnkiConditionalErrorAndReturn(!IsOdd(curTemplateHeight) && !IsOdd(curTemplateWidth),
            "LucasKanadeTrackerFast::LucasKanadeTrackerFast", "Template widths and height must divisible by 2^numPyramidLevels");
        }

        this->templateRegionHeight = templateRegion.bottom - templateRegion.top + 1.0f;
        this->templateRegionWidth = templateRegion.right - templateRegion.left + 1.0f;

        this->transformation = PlanarTransformation_f32(transformType, templateQuad, scratch);

        // Allocate the scratch for the pyramid lists
        templateCoordinates = FixedLengthList<Meshgrid<f32>>(numPyramidLevels, scratch);
        templateImagePyramid = FixedLengthList<Array<u8>>(numPyramidLevels, scratch);
        templateImageXGradientPyramid = FixedLengthList<Array<s16>>(numPyramidLevels, scratch);
        templateImageYGradientPyramid = FixedLengthList<Array<s16>>(numPyramidLevels, scratch);

        templateCoordinates.set_size(numPyramidLevels);
        templateImagePyramid.set_size(numPyramidLevels);
        templateImageXGradientPyramid.set_size(numPyramidLevels);
        templateImageYGradientPyramid.set_size(numPyramidLevels);

        AnkiConditionalErrorAndReturn(templateImagePyramid.IsValid() && templateImageXGradientPyramid.IsValid() && templateImageYGradientPyramid.IsValid() && templateCoordinates.IsValid(),
          "LucasKanadeTrackerFast::LucasKanadeTrackerFast", "Could not allocate pyramid lists");

        // Allocate the scratch for all the images
        for(s32 iScale=0; iScale<numPyramidLevels; iScale++) {
          const f32 scale = static_cast<f32>(1 << iScale);

          // Unused, remove?
          //const s32 curTemplateHeight = templateImageHeight >> iScale;
          //const s32 curTemplateWidth = templateImageWidth >> iScale;

          templateCoordinates[iScale] = Meshgrid<f32>(
            Linspace(-this->templateRegionWidth/2.0f, this->templateRegionWidth/2.0f, static_cast<s32>(FLT_FLOOR(this->templateRegionWidth/scale))),
            Linspace(-this->templateRegionHeight/2.0f, this->templateRegionHeight/2.0f, static_cast<s32>(FLT_FLOOR(this->templateRegionHeight/scale))));

          const s32 numPointsY = templateCoordinates[iScale].get_yGridVector().get_size();
          const s32 numPointsX = templateCoordinates[iScale].get_xGridVector().get_size();

          templateImagePyramid[iScale] = Array<u8>(numPointsY, numPointsX, scratch);
          templateImageXGradientPyramid[iScale] = Array<s16>(numPointsY, numPointsX, scratch);
          templateImageYGradientPyramid[iScale] = Array<s16>(numPointsY, numPointsX, scratch);

          AnkiConditionalErrorAndReturn(templateImagePyramid[iScale].IsValid() && templateImageXGradientPyramid[iScale].IsValid() && templateImageYGradientPyramid[iScale].IsValid(),
            "LucasKanadeTrackerFast::LucasKanadeTrackerFast", "Could not allocate pyramid images");
        }

        // Sample all levels of the pyramid images
        for(s32 iScale=0; iScale<numPyramidLevels; iScale++) {
          if((lastResult = Interp2_Affine<u8,u8>(templateImage, templateCoordinates[iScale], transformation.get_homography(), this->transformation.get_centerOffset(), this->templateImagePyramid[iScale], INTERPOLATE_LINEAR)) != RESULT_OK) {
            AnkiError("LucasKanadeTrackerFast::LucasKanadeTrackerFast", "Interp2_Affine failed with code 0x%x", lastResult);
            return;
          }
        }

        // Compute the spatial derivatives
        // TODO: compute without borders?
        for(s32 i=0; i<numPyramidLevels; i++) {
          if((lastResult = ImageProcessing::ComputeXGradient<u8,s16,s16>(templateImagePyramid[i], templateImageXGradientPyramid[i])) != RESULT_OK) {
            AnkiError("LucasKanadeTrackerFast::LucasKanadeTrackerFast", "ComputeXGradient failed with code 0x%x", lastResult);
            return;
          }

          if((lastResult = ImageProcessing::ComputeYGradient<u8,s16,s16>(templateImagePyramid[i], templateImageYGradientPyramid[i])) != RESULT_OK) {
            AnkiError("LucasKanadeTrackerFast::LucasKanadeTrackerFast", "ComputeYGradient failed with code 0x%x", lastResult);
            return;
          }
        }

        this->isValid = true;

        EndBenchmark("LucasKanadeTrackerFast");
      }

      Result LucasKanadeTrackerFast::UpdateTrack(const Array<u8> &nextImage, const s32 maxIterations, const f32 convergenceTolerance, bool& converged, MemoryStack scratch)
      {
        for(s32 iScale=numPyramidLevels-1; iScale>=0; iScale--) {
          converged = false;

          BeginBenchmark("UpdateTrack.refineTranslation");
          if((lastResult = IterativelyRefineTrack(nextImage, maxIterations, iScale, convergenceTolerance, TRANSFORM_TRANSLATION, converged, scratch)) != RESULT_OK)
            return lastResult;
          EndBenchmark("UpdateTrack.refineTranslation");

          if(this->transformation.get_transformType() != TRANSFORM_TRANSLATION) {
            BeginBenchmark("UpdateTrack.refineOther");
            if((lastResult = IterativelyRefineTrack(nextImage, maxIterations, iScale, convergenceTolerance, this->transformation.get_transformType(), converged, scratch)) != RESULT_OK)
              return lastResult;
            EndBenchmark("UpdateTrack.refineOther");
          }
        } // for(s32 iScale=numPyramidLevels; iScale>=0; iScale--)

        return RESULT_OK;
      }

      Result LucasKanadeTrackerFast::IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const TransformType curTransformType, bool &converged, MemoryStack scratch)
      {
        // Unused, remove?
        //const bool isOutColumnMajor = false; // TODO: change to false, which will probably be faster

        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);

        AnkiConditionalErrorAndReturnValue(this->IsValid() == true,
          RESULT_FAIL, "LucasKanadeTrackerFast::IterativelyRefineTrack", "This object is not initialized");

        AnkiConditionalErrorAndReturnValue(nextImage.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTrackerFast::IterativelyRefineTrack", "nextImage is not valid");

        AnkiConditionalErrorAndReturnValue(maxIterations > 0 && maxIterations < 1000,
          RESULT_FAIL_INVALID_PARAMETERS, "LucasKanadeTrackerFast::IterativelyRefineTrack", "maxIterations must be greater than zero and less than 1000");

        AnkiConditionalErrorAndReturnValue(whichScale >= 0 && whichScale < this->numPyramidLevels,
          RESULT_FAIL_INVALID_PARAMETERS, "LucasKanadeTrackerFast::IterativelyRefineTrack", "whichScale is invalid");

        AnkiConditionalErrorAndReturnValue(convergenceTolerance > 0.0f,
          RESULT_FAIL_INVALID_PARAMETERS, "LucasKanadeTrackerFast::IterativelyRefineTrack", "convergenceTolerance must be greater than zero");

        AnkiConditionalErrorAndReturnValue(nextImageHeight == templateImageHeight && nextImageWidth == templateImageWidth,
          RESULT_FAIL_INVALID_SIZE, "LucasKanadeTrackerFast::IterativelyRefineTrack", "nextImage must be the same size as the template");

        //const Rectangle<s32> curTemplateRegion(
        //  static_cast<s32>(Round(this->templateRegion.left / powf(2.0f,static_cast<f32>(whichScale)))),
        //  static_cast<s32>(Round(this->templateRegion.right / powf(2.0f,static_cast<f32>(whichScale)))),
        //  static_cast<s32>(Round(this->templateRegion.top / powf(2.0f,static_cast<f32>(whichScale)))),
        //  static_cast<s32>(Round(this->templateRegion.bottom / powf(2.0f,static_cast<f32>(whichScale)))));

        if(curTransformType == TRANSFORM_TRANSLATION) {
          return IterativelyRefineTrack_Translation(nextImage, maxIterations, whichScale, convergenceTolerance, converged, scratch);
        } else if(curTransformType == TRANSFORM_AFFINE) {
          return IterativelyRefineTrack_Affine(nextImage, maxIterations, whichScale, convergenceTolerance, converged, scratch);
        }

        return RESULT_FAIL;
      } // Result LucasKanadeTrackerFast::IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const TransformType curTransformType, bool &converged, MemoryStack scratch)

      Result LucasKanadeTrackerFast::IterativelyRefineTrack_Translation(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch)
      {
        // This method is heavily based on Interp2_Affine
        // The call would be like: Interp2_Affine<u8,u8>(nextImage, originalCoordinates, interpolationHomography, centerOffset, nextImageTransformed2d, INTERPOLATE_LINEAR, 0);

        Array<f32> AWAt(2, 2, scratch);
        Array<f32> b(1, 2, scratch);

        f32 &AWAt00 = AWAt[0][0];
        f32 &AWAt01 = AWAt[0][1];
        // Unused, remove?  f32 &AWAt10 = AWAt[1][0];
        f32 &AWAt11 = AWAt[1][1];

        f32 &b0 = b[0][0];
        f32 &b1 = b[0][1];

        converged = false;

        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);

        const f32 scale = static_cast<f32>(1 << whichScale);

        const f32 oneOverTwoFiftyFive = 1.0f / 255.0f;
        const f32 scaleOverFiveTen = scale / (2.0f*255.0f);

        const Point<f32>& centerOffset = this->transformation.get_centerOffset();

        // Initialize with some very extreme coordinates
        FixedLengthList<Quadrilateral<f32>> previousCorners(NUM_PREVIOUS_QUADS_TO_COMPARE, scratch);

        for(s32 i=0; i<NUM_PREVIOUS_QUADS_TO_COMPARE; i++) {
          previousCorners[i] = Quadrilateral<f32>(Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f));
        }

        Meshgrid<f32> originalCoordinates(
          Linspace(-this->templateRegionWidth/2.0f, this->templateRegionWidth/2.0f, static_cast<s32>(FLT_FLOOR(this->templateRegionWidth/scale))),
          Linspace(-this->templateRegionHeight/2.0f, this->templateRegionHeight/2.0f, static_cast<s32>(FLT_FLOOR(this->templateRegionHeight/scale))));

        // Unused, remove?
        //const s32 outHeight = originalCoordinates.get_yGridVector().get_size();
        //const s32 outWidth = originalCoordinates.get_xGridVector().get_size();

        const f32 xyReferenceMin = 0.0f;
        const f32 xReferenceMax = static_cast<f32>(nextImageWidth) - 1.0f;
        const f32 yReferenceMax = static_cast<f32>(nextImageHeight) - 1.0f;

        const LinearSequence<f32> &yGridVector = originalCoordinates.get_yGridVector();
        const LinearSequence<f32> &xGridVector = originalCoordinates.get_xGridVector();

        const f32 yGridStart = yGridVector.get_start();
        const f32 xGridStart = xGridVector.get_start();

        const f32 yGridDelta = yGridVector.get_increment();
        const f32 xGridDelta = xGridVector.get_increment();

        const s32 yIterationMax = yGridVector.get_size();
        const s32 xIterationMax = xGridVector.get_size();

        for(s32 iteration=0; iteration<maxIterations; iteration++) {
          const Array<f32> &homography = this->transformation.get_homography();
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];

          const f32 yTransformedDelta = h10 * yGridDelta;
          const f32 xTransformedDelta = h00 * xGridDelta;

          AWAt.SetZero();
          b.SetZero();

          s32 numInBounds = 0;

          // TODO: make the x and y limits from 1 to end-2

          f32 yOriginal = yGridStart;
          for(s32 y=0; y<yIterationMax; y++) {
            const u8 * restrict pTemplateImage = this->templateImagePyramid[whichScale].Pointer(y, 0);

            const s16 * restrict pTemplateImageXGradient = this->templateImageXGradientPyramid[whichScale].Pointer(y, 0);
            const s16 * restrict pTemplateImageYGradient = this->templateImageYGradientPyramid[whichScale].Pointer(y, 0);

            f32 xOriginal = xGridStart;

            // TODO: This could be strength-reduced further, but it wouldn't be much faster
            f32 xTransformed = h00*xOriginal + h01*yOriginal + h02 + centerOffset.x;
            f32 yTransformed = h10*xOriginal + h11*yOriginal + h12 + centerOffset.y;

            for(s32 x=0; x<xIterationMax; x++) {
              const f32 x0 = FLT_FLOOR(xTransformed);
              const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;

              const f32 y0 = FLT_FLOOR(yTransformed);
              const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;

              // If out of bounds, continue
              if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
                // strength reduction for the affine transformation along this horizontal line
                xTransformed += xTransformedDelta;
                yTransformed += yTransformedDelta;
                xOriginal += xGridDelta;
                continue;
              }

              numInBounds++;

              const f32 alphaX = xTransformed - x0;
              const f32 alphaXinverse = 1 - alphaX;

              const f32 alphaY = yTransformed - y0;
              const f32 alphaYinverse = 1.0f - alphaY;

              const s32 y0S32 = static_cast<s32>(Roundf(y0));
              const s32 y1S32 = static_cast<s32>(Roundf(y1));
              const s32 x0S32 = static_cast<s32>(Roundf(x0));

              const u8 * restrict pReference_y0 = nextImage.Pointer(y0S32, x0S32);
              const u8 * restrict pReference_y1 = nextImage.Pointer(y1S32, x0S32);

              const f32 pixelTL = *pReference_y0;
              const f32 pixelTR = *(pReference_y0+1);
              const f32 pixelBL = *pReference_y1;
              const f32 pixelBR = *(pReference_y1+1);

              const f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);

              //const u8 interpolatedPixel = static_cast<u8>(Roundf(interpolatedPixelF32));

              // This block is the non-interpolation part of the per-sample algorithm
              {
                const f32 templatePixelValue = static_cast<f32>(pTemplateImage[x]);
                const f32 xGradientValue = scaleOverFiveTen * static_cast<f32>(pTemplateImageXGradient[x]);
                const f32 yGradientValue = scaleOverFiveTen * static_cast<f32>(pTemplateImageYGradient[x]);

                const f32 tGradientValue = oneOverTwoFiftyFive * (interpolatedPixelF32 - templatePixelValue);

                //AWAt
                //  b
                AWAt00 += xGradientValue * xGradientValue;
                AWAt01 += xGradientValue * yGradientValue;
                AWAt11 += yGradientValue * yGradientValue;

                b0 += xGradientValue * tGradientValue;
                b1 += yGradientValue * tGradientValue;
              }

              // strength reduction for the affine transformation along this horizontal line
              xTransformed += xTransformedDelta;
              yTransformed += yTransformedDelta;
              xOriginal += xGridDelta;
            } // for(s32 x=0; x<xIterationMax; x++)

            yOriginal += yGridDelta;
          } // for(s32 y=0; y<yIterationMax; y++)

          if(numInBounds < 16) {
            AnkiWarn("LucasKanadeTrackerFast::IterativelyRefineTrack_Translation", "Template drifted too far out of image.");
            return RESULT_OK;
          }

          Matrix::MakeSymmetric(AWAt, false);

          //AWAt.Print("New AWAt");
          //b.Print("New b");

          if((lastResult = Matrix::SolveLeastSquaresWithCholesky(AWAt, b, false)) != RESULT_OK)
            return lastResult;

          //b.Print("New update");

          this->transformation.Update(b, scratch, TRANSFORM_TRANSLATION);

          // Check if we're done with iterations
          {
            PUSH_MEMORY_STACK(scratch);

            Quadrilateral<f32> in(
              Point<f32>(0.0f,0.0f),
              Point<f32>(static_cast<f32>(nextImage.get_size(1)),0.0f),
              Point<f32>(static_cast<f32>(nextImage.get_size(0)),static_cast<f32>(nextImage.get_size(1))),
              Point<f32>(0.0f,static_cast<f32>(nextImage.get_size(1))));

            Quadrilateral<f32> newCorners = transformation.TransformQuadrilateral(in, scratch, scale);

            //const f32 change = sqrtf(Matrix::Mean<f32,f32>(tmp1));
            f32 minChange = 1e10f;
            for(s32 iPrevious=0; iPrevious<NUM_PREVIOUS_QUADS_TO_COMPARE; iPrevious++) {
              f32 change = 0.0f;
              for(s32 i=0; i<4; i++) {
                const f32 dx = previousCorners[iPrevious][i].x - newCorners[i].x;
                const f32 dy = previousCorners[iPrevious][i].y - newCorners[i].y;
                change += sqrtf(dx*dx + dy*dy);
              }
              change /= 4;

              minChange = MIN(minChange, change);
            }

            //printf("newCorners");
            //newCorners.Print();
            //printf("change: %f\n", change);

            if(minChange < convergenceTolerance*scale) {
              converged = true;
              return RESULT_OK;
            }

            for(s32 iPrevious=0; iPrevious<(NUM_PREVIOUS_QUADS_TO_COMPARE-1); iPrevious++) {
              previousCorners[iPrevious] = previousCorners[iPrevious+1];
            }
            previousCorners[NUM_PREVIOUS_QUADS_TO_COMPARE-1] = newCorners;
          } // PUSH_MEMORY_STACK(scratch);
        } // for(s32 iteration=0; iteration<maxIterations; iteration++)

        return RESULT_OK;
      } // Result LucasKanadeTrackerFast::IterativelyRefineTrack_Translation()

      Result LucasKanadeTrackerFast::IterativelyRefineTrack_Affine(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch)
      {
        // This method is heavily based on Interp2_Affine
        // The call would be like: Interp2_Affine<u8,u8>(nextImage, originalCoordinates, interpolationHomography, centerOffset, nextImageTransformed2d, INTERPOLATE_LINEAR, 0);

        Array<f32> AWAt(6, 6, scratch);
        Array<f32> b(1, 6, scratch);

        //f32 * restrict AWAt0 = AWAt[0];
        //f32 * restrict AWAt1 = AWAt[1];
        //f32 * restrict AWAt2 = AWAt[2];
        //f32 * restrict AWAt3 = AWAt[3];
        //f32 * restrict AWAt4 = AWAt[4];
        //f32 * restrict AWAt5 = AWAt[5];

        //f32 * restrict b = b[0];

        // These addresses should be known at compile time, so should be faster
        f32 AWAt_raw[6][6];
        f32 b_raw[6];

        converged = false;

        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);

        const f32 scale = static_cast<f32>(1 << whichScale);

        const f32 oneOverTwoFiftyFive = 1.0f / 255.0f;
        const f32 scaleOverFiveTen = scale / (2.0f*255.0f);

        const Point<f32>& centerOffset = this->transformation.get_centerOffset();

        // Initialize with some very extreme coordinates
        FixedLengthList<Quadrilateral<f32>> previousCorners(NUM_PREVIOUS_QUADS_TO_COMPARE, scratch);

        for(s32 i=0; i<NUM_PREVIOUS_QUADS_TO_COMPARE; i++) {
          previousCorners[i] = Quadrilateral<f32>(Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f));
        }

        Meshgrid<f32> originalCoordinates(
          Linspace(-this->templateRegionWidth/2.0f, this->templateRegionWidth/2.0f, static_cast<s32>(FLT_FLOOR(this->templateRegionWidth/scale))),
          Linspace(-this->templateRegionHeight/2.0f, this->templateRegionHeight/2.0f, static_cast<s32>(FLT_FLOOR(this->templateRegionHeight/scale))));

        // Unused, remove?
        //const s32 outHeight = originalCoordinates.get_yGridVector().get_size();
        //const s32 outWidth = originalCoordinates.get_xGridVector().get_size();

        const f32 xyReferenceMin = 0.0f;
        const f32 xReferenceMax = static_cast<f32>(nextImageWidth) - 1.0f;
        const f32 yReferenceMax = static_cast<f32>(nextImageHeight) - 1.0f;

        const LinearSequence<f32> &yGridVector = originalCoordinates.get_yGridVector();
        const LinearSequence<f32> &xGridVector = originalCoordinates.get_xGridVector();

        const f32 yGridStart = yGridVector.get_start();
        const f32 xGridStart = xGridVector.get_start();

        const f32 yGridDelta = yGridVector.get_increment();
        const f32 xGridDelta = xGridVector.get_increment();

        const s32 yIterationMax = yGridVector.get_size();
        const s32 xIterationMax = xGridVector.get_size();

        for(s32 iteration=0; iteration<maxIterations; iteration++) {
          const Array<f32> &homography = this->transformation.get_homography();
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];

          const f32 yTransformedDelta = h10 * yGridDelta;
          const f32 xTransformedDelta = h00 * xGridDelta;

          //AWAt.SetZero();
          //b.SetZero();

          for(s32 ia=0; ia<6; ia++) {
            for(s32 ja=0; ja<6; ja++) {
              AWAt_raw[ia][ja] = 0;
            }
            b_raw[ia] = 0;
          }

          s32 numInBounds = 0;

          // TODO: make the x and y limits from 1 to end-2

          f32 yOriginal = yGridStart;
          for(s32 y=0; y<yIterationMax; y++) {
            const u8 * restrict pTemplateImage = this->templateImagePyramid[whichScale].Pointer(y, 0);

            const s16 * restrict pTemplateImageXGradient = this->templateImageXGradientPyramid[whichScale].Pointer(y, 0);
            const s16 * restrict pTemplateImageYGradient = this->templateImageYGradientPyramid[whichScale].Pointer(y, 0);

            f32 xOriginal = xGridStart;

            // TODO: This could be strength-reduced further, but it wouldn't be much faster
            f32 xTransformed = h00*xOriginal + h01*yOriginal + h02 + centerOffset.x;
            f32 yTransformed = h10*xOriginal + h11*yOriginal + h12 + centerOffset.y;

            for(s32 x=0; x<xIterationMax; x++) {
              const f32 x0 = FLT_FLOOR(xTransformed);
              const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;

              const f32 y0 = FLT_FLOOR(yTransformed);
              const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;

              // If out of bounds, continue
              if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
                // strength reduction for the affine transformation along this horizontal line
                xTransformed += xTransformedDelta;
                yTransformed += yTransformedDelta;
                xOriginal += xGridDelta;
                continue;
              }

              numInBounds++;

              const f32 alphaX = xTransformed - x0;
              const f32 alphaXinverse = 1 - alphaX;

              const f32 alphaY = yTransformed - y0;
              const f32 alphaYinverse = 1.0f - alphaY;

              const s32 y0S32 = static_cast<s32>(Roundf(y0));
              const s32 y1S32 = static_cast<s32>(Roundf(y1));
              const s32 x0S32 = static_cast<s32>(Roundf(x0));

              const u8 * restrict pReference_y0 = nextImage.Pointer(y0S32, x0S32);
              const u8 * restrict pReference_y1 = nextImage.Pointer(y1S32, x0S32);

              const f32 pixelTL = *pReference_y0;
              const f32 pixelTR = *(pReference_y0+1);
              const f32 pixelBL = *pReference_y1;
              const f32 pixelBR = *(pReference_y1+1);

              const f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);

              //const u8 interpolatedPixel = static_cast<u8>(Roundf(interpolatedPixelF32));

              // This block is the non-interpolation part of the per-sample algorithm
              {
                const f32 templatePixelValue = static_cast<f32>(pTemplateImage[x]);
                const f32 xGradientValue = scaleOverFiveTen * static_cast<f32>(pTemplateImageXGradient[x]);
                const f32 yGradientValue = scaleOverFiveTen * static_cast<f32>(pTemplateImageYGradient[x]);

                const f32 tGradientValue = oneOverTwoFiftyFive * (interpolatedPixelF32 - templatePixelValue);

                //printf("%f ", xOriginal);
                const f32 values[6] = {
                  xOriginal * xGradientValue,
                  yOriginal * xGradientValue,
                  xGradientValue,
                  xOriginal * yGradientValue,
                  yOriginal * yGradientValue,
                  yGradientValue};

                //for(s32 ia=0; ia<6; ia++) {
                //  printf("%f ", values[ia]);
                //}
                //printf("\n");

                //f32 AWAt_raw[6][6];
                //f32 b_raw[6];
                for(s32 ia=0; ia<6; ia++) {
                  for(s32 ja=ia; ja<6; ja++) {
                    AWAt_raw[ia][ja] += values[ia] * values[ja];
                  }
                  b_raw[ia] += values[ia] * tGradientValue;
                }
              }

              // strength reduction for the affine transformation along this horizontal line
              xTransformed += xTransformedDelta;
              yTransformed += yTransformedDelta;
              xOriginal += xGridDelta;
            } // for(s32 x=0; x<xIterationMax; x++)

            yOriginal += yGridDelta;
          } // for(s32 y=0; y<yIterationMax; y++)

          if(numInBounds < 16) {
            AnkiWarn("LucasKanadeTrackerFast::IterativelyRefineTrack_Translation", "Template drifted too far out of image.");
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

          if((lastResult = Matrix::SolveLeastSquaresWithCholesky(AWAt, b, false)) != RESULT_OK)
            return lastResult;

          //b.Print("New update");

          this->transformation.Update(b, scratch, TRANSFORM_AFFINE);

          //this->transformation.get_homography().Print("new transformation");

          // Check if we're done with iterations
          {
            PUSH_MEMORY_STACK(scratch);

            Quadrilateral<f32> in(
              Point<f32>(0.0f,0.0f),
              Point<f32>(static_cast<f32>(nextImage.get_size(1)),0.0f),
              Point<f32>(static_cast<f32>(nextImage.get_size(0)),static_cast<f32>(nextImage.get_size(1))),
              Point<f32>(0.0f,static_cast<f32>(nextImage.get_size(1))));

            Quadrilateral<f32> newCorners = transformation.TransformQuadrilateral(in, scratch, scale);

            //const f32 change = sqrtf(Matrix::Mean<f32,f32>(tmp1));
            f32 minChange = 1e10f;
            for(s32 iPrevious=0; iPrevious<NUM_PREVIOUS_QUADS_TO_COMPARE; iPrevious++) {
              f32 change = 0.0f;
              for(s32 i=0; i<4; i++) {
                const f32 dx = previousCorners[iPrevious][i].x - newCorners[i].x;
                const f32 dy = previousCorners[iPrevious][i].y - newCorners[i].y;
                change += sqrtf(dx*dx + dy*dy);
              }
              change /= 4;

              minChange = MIN(minChange, change);
            }

            //printf("newCorners");
            //newCorners.Print();
            //printf("change: %f\n", change);

            if(minChange < convergenceTolerance*scale) {
              converged = true;
              return RESULT_OK;
            }

            for(s32 iPrevious=0; iPrevious<(NUM_PREVIOUS_QUADS_TO_COMPARE-1); iPrevious++) {
              previousCorners[iPrevious] = previousCorners[iPrevious+1];
            }
            previousCorners[NUM_PREVIOUS_QUADS_TO_COMPARE-1] = newCorners;
          } // PUSH_MEMORY_STACK(scratch);
        } // for(s32 iteration=0; iteration<maxIterations; iteration++)

        return RESULT_OK;
      } // Result LucasKanadeTrackerFast::IterativelyRefineTrack_Affine()

      bool LucasKanadeTrackerFast::IsValid() const
      {
        if(!this->isValid)
          return false;

        if(!templateImagePyramid.IsValid())
          return false;

        if(!templateImageXGradientPyramid.IsValid())
          return false;

        if(!templateImageYGradientPyramid.IsValid())
          return false;

        for(s32 i=0; i<numPyramidLevels; i++) {
          if(!templateImagePyramid[i].IsValid())
            return false;

          if(!templateImageXGradientPyramid[i].IsValid())
            return false;

          if(!templateImageYGradientPyramid[i].IsValid())
            return false;
        }

        return true;
      }

      Result LucasKanadeTrackerFast::set_transformation(const PlanarTransformation_f32 &transformation)
      {
        const TransformType originalType = this->transformation.get_transformType();

        if((lastResult = this->transformation.set_transformType(transformation.get_transformType())) != RESULT_OK) {
          this->transformation.set_transformType(originalType);
          return lastResult;
        }

        if((lastResult = this->transformation.set_homography(transformation.get_homography())) != RESULT_OK) {
          this->transformation.set_transformType(originalType);
          return lastResult;
        }

        return RESULT_OK;
      }

      PlanarTransformation_f32 LucasKanadeTrackerFast::get_transformation() const
      {
        return transformation;
      }

      LucasKanadeTrackerBinary::LucasKanadeTrackerBinary()
        : isValid(false)
      {
      }

      LucasKanadeTrackerBinary::LucasKanadeTrackerBinary(
        const Array<u8> &templateImage, const Quadrilateral<f32> &templateQuad,
        const u8 edgeDetection_grayvalueThreshold, const s32 edgeDetection_minComponentWidth, const s32 edgeDetection_maxDetectionsPerType,
        MemoryStack &memory)
        : isValid(false)
      {
        const s32 templateImageHeight = templateImage.get_size(0);
        const s32 templateImageWidth = templateImage.get_size(1);

        AnkiConditionalErrorAndReturn(templateImageHeight > 0 && templateImageWidth > 0,
          "LucasKanadeTrackerBinary::LucasKanadeTrackerBinary", "template widths and heights must be greater than zero");

        AnkiConditionalErrorAndReturn(templateImage.IsValid(),
          "LucasKanadeTrackerBinary::LucasKanadeTrackerBinary", "templateImage is not valid");

        this->templateImage = Array<u8>(templateImageHeight, templateImageWidth, memory);
        this->templateQuad = templateQuad;

        this->template_xDecreasingIndexes = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, memory);
        this->template_xIncreasingIndexes = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, memory);
        this->template_yDecreasingIndexes = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, memory);
        this->template_yIncreasingIndexes = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, memory);

        AnkiConditionalErrorAndReturn(this->templateImage.IsValid() &&
          this->template_xDecreasingIndexes.IsValid() && this->template_xIncreasingIndexes.IsValid() &&
          this->template_yDecreasingIndexes.IsValid() && this->template_yIncreasingIndexes.IsValid(),
          "LucasKanadeTrackerBinary::LucasKanadeTrackerBinary", "Could not allocate local memory");

        this->templateImage.Set(templateImage);

        const Rectangle<f32> templateRectRaw = templateQuad.ComputeBoundingRectangle();
        const Rectangle<s32> templateRect(static_cast<s32>(templateRectRaw.left), static_cast<s32>(templateRectRaw.right), static_cast<s32>(templateRectRaw.top), static_cast<s32>(templateRectRaw.bottom));

        const Result result = DetectBlurredEdge(this->templateImage, templateRect, edgeDetection_grayvalueThreshold, edgeDetection_minComponentWidth, template_xDecreasingIndexes, template_xIncreasingIndexes, template_yDecreasingIndexes, template_yIncreasingIndexes);

        AnkiConditionalErrorAndReturn(result == RESULT_OK,
          "LucasKanadeTrackerBinary::LucasKanadeTrackerBinary", "DetectBlurredEdge failed");

        this->homographyOffsetX = (static_cast<f32>(templateImageWidth)-1.0f) / 2.0f;
        this->homographyOffsetY = (static_cast<f32>(templateImageHeight)-1.0f) / 2.0f;

        this->grid = Meshgrid<f32>(
          Linspace(-homographyOffsetX, homographyOffsetX, static_cast<s32>(FLT_FLOOR(templateImageWidth))),
          Linspace(-homographyOffsetY, homographyOffsetY, static_cast<s32>(FLT_FLOOR(templateImageHeight))));

        // Update the points in the lists to use the meshgrid coordinates

        this->xGrid = this->grid.get_xGridVector().Evaluate(memory);
        this->yGrid = this->grid.get_yGridVector().Evaluate(memory);

        this->template_xDecreasingGrid = LucasKanadeTrackerBinary::TransformIndexesToGrid(this->template_xDecreasingIndexes, this->xGrid, this->yGrid, memory);
        this->template_xIncreasingGrid = LucasKanadeTrackerBinary::TransformIndexesToGrid(this->template_xIncreasingIndexes, this->xGrid, this->yGrid, memory);
        this->template_yDecreasingGrid = LucasKanadeTrackerBinary::TransformIndexesToGrid(this->template_yDecreasingIndexes, this->xGrid, this->yGrid, memory);
        this->template_yIncreasingGrid = LucasKanadeTrackerBinary::TransformIndexesToGrid(this->template_yIncreasingIndexes, this->xGrid, this->yGrid, memory);

        AnkiConditionalErrorAndReturn(
          this->template_xDecreasingGrid.IsValid() && this->template_xIncreasingGrid.IsValid() &&
          this->template_yDecreasingGrid.IsValid() && this->template_yIncreasingGrid.IsValid(),
          "LucasKanadeTrackerBinary::LucasKanadeTrackerBinary", "Could not allocate local memory");

        this->isValid = true;
      }

      FixedLengthList<Point<f32> > LucasKanadeTrackerBinary::TransformIndexesToGrid(const FixedLengthList<Point<s16> > &pointIndexes, const Array<f32> &xGrid, const Array<f32> &yGrid, MemoryStack &memory)
      {
        const s32 numIndexes = pointIndexes.get_size();

        FixedLengthList<Point<f32> > pointGrid(numIndexes, memory, Flags::Buffer(false, false, true));

        AnkiConditionalErrorAndReturnValue(pointGrid.IsValid(),
          pointGrid, "LucasKanadeTrackerBinary::TransformIndexesToGrid", "Could not allocate local memory");

        const f32 * restrict pXGrid = xGrid.Pointer(0,0);
        const f32 * restrict pYGrid = yGrid.Pointer(0,0);

        const Point<s16> * restrict pPointIndexes = pointIndexes.Pointer(0);
        Point<f32> * restrict pPointGrid = pointGrid.Pointer(0);

        for(s32 i=0; i<numIndexes; i++) {
          pPointGrid[i].x = pXGrid[pPointIndexes[i].x];
          pPointGrid[i].y = pXGrid[pPointIndexes[i].y];
        }

        return pointGrid;
      }

      bool LucasKanadeTrackerBinary::IsValid() const
      {
        if(!this->isValid)
          return false;

        if(!templateImage.IsValid())
          return false;

        if(!template_xDecreasingIndexes.IsValid())
          return false;

        if(!template_xIncreasingIndexes.IsValid())
          return false;

        if(!template_yDecreasingIndexes.IsValid())
          return false;

        if(!template_yIncreasingIndexes.IsValid())
          return false;

        if(!template_xDecreasingGrid.IsValid())
          return false;

        if(!template_xIncreasingGrid.IsValid())
          return false;

        if(!template_yDecreasingGrid.IsValid())
          return false;

        if(!template_yIncreasingGrid.IsValid())
          return false;

        return true;
      }

      Result LucasKanadeTrackerBinary::set_transformation(const PlanarTransformation_f32 &transformation)
      {
        return this->transformation.Set(transformation);
      }

      PlanarTransformation_f32 LucasKanadeTrackerBinary::get_transformation() const
      {
        return transformation;
      }

      Result LucasKanadeTrackerBinary::UpdateTrackOnce(
        const Array<u8> &nextImage,
        const u8 edgeDetection_grayvalueThreshold, const s32 edgeDetection_minComponentWidth, const s32 edgeDetection_maxDetectionsPerType,
        const s32 maxMatchingDistance,
        const TransformType updateType,
        MemoryStack scratch)
      {
        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);

        AnkiConditionalErrorAndReturnValue(updateType==TRANSFORM_TRANSLATION || updateType == TRANSFORM_PROJECTIVE,
          RESULT_FAIL_INVALID_PARAMETERS, "LucasKanadeTrackerBinary::UpdateTrackOnce", "Only TRANSFORM_TRANSLATION or TRANSFORM_PROJECTIVE are supported");

        AnkiConditionalErrorAndReturnValue(templateImage.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTrackerBinary::UpdateTrackOnce", "nextImage is not valid");

        FixedLengthList<Point<s16> > next_xDecreasingIndexes = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, scratch);
        FixedLengthList<Point<s16> > next_xIncreasingIndexes = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, scratch);
        FixedLengthList<Point<s16> > next_yDecreasingIndexes = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, scratch);
        FixedLengthList<Point<s16> > next_yIncreasingIndexes = FixedLengthList<Point<s16> >(edgeDetection_maxDetectionsPerType, scratch);

        AnkiConditionalErrorAndReturnValue(next_xDecreasingIndexes.IsValid() && next_xIncreasingIndexes.IsValid() && next_yDecreasingIndexes.IsValid() && next_yIncreasingIndexes.IsValid() &&
          next_xDecreasingIndexes.IsValid() && next_xIncreasingIndexes.IsValid() && next_yDecreasingIndexes.IsValid() && next_yIncreasingIndexes.IsValid(),
          RESULT_FAIL_OUT_OF_MEMORY, "LucasKanadeTrackerBinary::UpdateTrackOnce", "Could not allocate local scratch");

        const Result result = DetectBlurredEdge(nextImage, edgeDetection_grayvalueThreshold, edgeDetection_minComponentWidth, next_xDecreasingIndexes, next_xIncreasingIndexes, next_yDecreasingIndexes, next_yIncreasingIndexes);

        AnkiConditionalErrorAndReturnValue(result == RESULT_OK,
          result, "LucasKanadeTrackerBinary::UpdateTrackOnce", "DetectBlurredEdge failed");

        FixedLengthList<Point<f32> > next_xDecreasingGrid = LucasKanadeTrackerBinary::TransformIndexesToGrid(next_xDecreasingIndexes, this->xGrid, this->yGrid, scratch);
        FixedLengthList<Point<f32> > next_xIncreasingGrid = LucasKanadeTrackerBinary::TransformIndexesToGrid(next_xIncreasingIndexes, this->xGrid, this->yGrid, scratch);
        FixedLengthList<Point<f32> > next_yDecreasingGrid = LucasKanadeTrackerBinary::TransformIndexesToGrid(next_yDecreasingIndexes, this->xGrid, this->yGrid, scratch);
        FixedLengthList<Point<f32> > next_yIncreasingGrid = LucasKanadeTrackerBinary::TransformIndexesToGrid(next_yIncreasingIndexes, this->xGrid, this->yGrid, scratch);

        AnkiConditionalErrorAndReturnValue(
          next_xDecreasingGrid.IsValid() && next_xIncreasingGrid.IsValid() && next_yDecreasingGrid.IsValid() && next_yIncreasingGrid.IsValid(),
          RESULT_FAIL_OUT_OF_MEMORY, "LucasKanadeTrackerBinary::UpdateTrackOnce", "Could not allocate local memory");

        return RESULT_OK;
      }

      Result LucasKanadeTrackerBinary::ComputeIndexLimitsVertical(const FixedLengthList<Point<s16> > &points, Array<s32> &yStartIndexes)
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

      Result LucasKanadeTrackerBinary::ComputeIndexLimitsHorizontal(const FixedLengthList<Point<s16> > &points, Array<s32> &xStartIndexes)
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
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki