/**
File: lucasKanade_General.cpp
Author: Peter Barnum
Created: 2014-03-18

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

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
      f32 UpdatePreviousCorners(const Transformations::PlanarTransformation_f32 &transformation,
                                const u16 baseImageWidth, const u16 baseImageHeight,
                                FixedLengthList<Quadrilateral<f32> > &previousCorners,
                                MemoryStack scratch)
      {
        const f32 baseImageHalfWidth = static_cast<f32>(baseImageWidth) / 2.0f;
        const f32 baseImageHalfHeight = static_cast<f32>(baseImageHeight) / 2.0f;

        Quadrilateral<f32> in(
          Point<f32>(-baseImageHalfWidth,-baseImageHalfHeight),
          Point<f32>(baseImageHalfWidth,-baseImageHalfHeight),
          Point<f32>(baseImageHalfWidth,baseImageHalfHeight),
          Point<f32>(-baseImageHalfWidth,baseImageHalfHeight));

        Quadrilateral<f32> newCorners = transformation.Transform(in, scratch, 1.0f);

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

        for(s32 iPrevious=0; iPrevious<(NUM_PREVIOUS_QUADS_TO_COMPARE-1); iPrevious++) {
          previousCorners[iPrevious] = previousCorners[iPrevious+1];
        }
        previousCorners[NUM_PREVIOUS_QUADS_TO_COMPARE-1] = newCorners;

        return minChange;
      } // f32 UpdatePreviousCorners()

      LucasKanadeTracker_Generic::LucasKanadeTracker_Generic(const Transformations::TransformType maxSupportedTransformType)
        : maxSupportedTransformType(maxSupportedTransformType), isValid(false)
      {
      }
      
      LucasKanadeTracker_Generic::LucasKanadeTracker_Generic(const Transformations::TransformType maxSupportedTransformType,
                                                             const Array<u8> &templateImage,
                                                             const Quadrilateral<f32> &templateQuad,
                                                             const f32 scaleTemplateRegionPercent,
                                                             const s32 numPyramidLevels,
                                                             const Transformations::TransformType transformType,
                                                             MemoryStack &memory)
      : baseImageWidth(templateImage.get_size(1))
      , baseImageHeight(templateImage.get_size(0))
      , maxSupportedTransformType(maxSupportedTransformType)
      , numPyramidLevels(numPyramidLevels)
      , templateImageHeight(templateImage.get_size(0))
      , templateImageWidth(templateImage.get_size(1))
      , isValid(false)
      {
        BeginBenchmark("LucasKanadeTracker_Generic");

        AnkiConditionalErrorAndReturn(templateImageHeight > 0 && templateImageWidth > 0,
          "LucasKanadeTracker_Generic::LucasKanadeTracker_Generic", "template widths and heights must be greater than zero, and multiples of %d", ANKI_VISION_IMAGE_WIDTH_MULTIPLE);

        AnkiConditionalErrorAndReturn(numPyramidLevels > 0,
          "LucasKanadeTracker_Generic::LucasKanadeTracker_Generic", "numPyramidLevels must be greater than zero");

        AnkiConditionalErrorAndReturn(transformType <= maxSupportedTransformType,
          "LucasKanadeTracker_Generic::LucasKanadeTracker_Generic", "Transform type %d not supported", transformType);

        const s32 initialImageScaleS32 = baseImageWidth / templateImage.get_size(1);
        const s32 initialImagePowerS32 = Log2u32(static_cast<u32>(initialImageScaleS32));
        initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);

        AnkiConditionalErrorAndReturn(((1<<initialImagePowerS32)*templateImage.get_size(1)) == baseImageWidth,
          "LucasKanadeTracker_Generic::LucasKanadeTracker_Generic", "The templateImage must be a power of two smaller than baseImageWidth (%d)", baseImageWidth);

        templateRegion = templateQuad.ComputeBoundingRectangle<f32>().ComputeScaledRectangle<f32>(scaleTemplateRegionPercent);
 
        templateRegion.left /= initialImageScaleF32;
        templateRegion.right /= initialImageScaleF32;
        templateRegion.top /= initialImageScaleF32;
        templateRegion.bottom /= initialImageScaleF32;

        // All pyramid width except the last one must be divisible by two
        for(s32 i=0; i<(numPyramidLevels-1); i++) {
          const s32 curTemplateHeight = templateImageHeight >> i;
          const s32 curTemplateWidth = templateImageWidth >> i;

          AnkiConditionalErrorAndReturn(!IsOdd(curTemplateHeight) && !IsOdd(curTemplateWidth),
            "LucasKanadeTracker_Generic::LucasKanadeTracker_Generic", "Template widths and height must divisible by 2^numPyramidLevels");
        }

        this->templateRegionHeight = templateRegion.bottom - templateRegion.top + 1.0f;
        this->templateRegionWidth = templateRegion.right - templateRegion.left + 1.0f;

        this->transformation = Transformations::PlanarTransformation_f32(transformType, templateQuad, memory);

        //this->isValid = true;

        EndBenchmark("LucasKanadeTracker_Generic");
      }

      bool LucasKanadeTracker_Generic::IsValid() const
      {
        if(!this->isValid)
          return false;

        if(!this->transformation.IsValid())
          return false;

        return true;
      }

      Result LucasKanadeTracker_Generic::UpdateTransformation(const Array<f32> &update, const f32 scale, MemoryStack scratch, Transformations::TransformType updateType)
      {
        return this->transformation.Update(update, scale, scratch, updateType);
      }

      s32 LucasKanadeTracker_Generic::get_numPyramidLevels() const
      {
        return numPyramidLevels;
      }

      Result LucasKanadeTracker_Generic::set_transformation(const Transformations::PlanarTransformation_f32 &transformation)
      {
        Result lastResult;

        const Transformations::TransformType originalType = this->transformation.get_transformType();

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

      Transformations::PlanarTransformation_f32 LucasKanadeTracker_Generic::get_transformation() const
      {
        return transformation;
      }
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki
