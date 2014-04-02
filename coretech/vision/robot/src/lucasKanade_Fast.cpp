/**
File: lucasKanade_Fast.cpp
Author: Peter Barnum
Created: 2014-03-18

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/lucasKanade.h"
#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/interpolate.h"
#include "anki/common/robot/arrayPatterns.h"
#include "anki/common/robot/find.h"
#include "anki/common/robot/benchmarking_c.h"
#include "anki/common/robot/draw.h"
#include "anki/common/robot/comparisons.h"
#include "anki/common/robot/errorHandling.h"

#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/imageProcessing.h"
#include "anki/vision/robot/transformations.h"

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
      LucasKanadeTracker_Fast::LucasKanadeTracker_Fast(const Transformations::TransformType maxSupportedTransformType)
        : LucasKanadeTracker_Generic(maxSupportedTransformType)
      {
      }

      LucasKanadeTracker_Fast::LucasKanadeTracker_Fast(
        const Transformations::TransformType maxSupportedTransformType,
        const Array<u8> &templateImage,
        const Quadrilateral<f32> &templateQuad,
        const f32 scaleTemplateRegionPercent,
        const s32 numPyramidLevels,
        const Transformations::TransformType transformType,
        MemoryStack &memory)
        : LucasKanadeTracker_Generic(maxSupportedTransformType, templateImage, templateQuad, scaleTemplateRegionPercent, numPyramidLevels, transformType, memory)
      {
        Result lastResult;

        BeginBenchmark("LucasKanadeTracker_Fast");

        // Allocate the memory for the pyramid lists
        templateCoordinates = FixedLengthList<Meshgrid<f32> >(numPyramidLevels, memory);
        templateImagePyramid = FixedLengthList<Array<u8> >(numPyramidLevels, memory);
        templateImageXGradientPyramid = FixedLengthList<Array<s16> >(numPyramidLevels, memory);
        templateImageYGradientPyramid = FixedLengthList<Array<s16> >(numPyramidLevels, memory);

        templateCoordinates.set_size(numPyramidLevels);
        templateImagePyramid.set_size(numPyramidLevels);
        templateImageXGradientPyramid.set_size(numPyramidLevels);
        templateImageYGradientPyramid.set_size(numPyramidLevels);

        AnkiConditionalErrorAndReturn(templateImagePyramid.IsValid() && templateImageXGradientPyramid.IsValid() && templateImageYGradientPyramid.IsValid() && templateCoordinates.IsValid(),
          "LucasKanadeTracker_Fast::LucasKanadeTracker_Fast", "Could not allocate pyramid lists");

        // Allocate the memory for all the images
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

          templateImagePyramid[iScale] = Array<u8>(numPointsY, numPointsX, memory);
          templateImageXGradientPyramid[iScale] = Array<s16>(numPointsY, numPointsX, memory);
          templateImageYGradientPyramid[iScale] = Array<s16>(numPointsY, numPointsX, memory);

          AnkiConditionalErrorAndReturn(templateImagePyramid[iScale].IsValid() && templateImageXGradientPyramid[iScale].IsValid() && templateImageYGradientPyramid[iScale].IsValid(),
            "LucasKanadeTracker_Fast::LucasKanadeTracker_Fast", "Could not allocate pyramid images");
        }

        // Sample all levels of the pyramid images
        for(s32 iScale=0; iScale<numPyramidLevels; iScale++) {
          if((lastResult = Interp2_Affine<u8,u8>(templateImage, templateCoordinates[iScale], transformation.get_homography(), this->transformation.get_centerOffset(initialImageScaleF32), this->templateImagePyramid[iScale], INTERPOLATE_LINEAR)) != RESULT_OK) {
            AnkiError("LucasKanadeTracker_Fast::LucasKanadeTracker_Fast", "Interp2_Affine failed with code 0x%x", lastResult);
            return;
          }
        }

        // Compute the spatial derivatives
        // TODO: compute without borders?
        for(s32 i=0; i<numPyramidLevels; i++) {
          if((lastResult = ImageProcessing::ComputeXGradient<u8,s16,s16>(templateImagePyramid[i], templateImageXGradientPyramid[i])) != RESULT_OK) {
            AnkiError("LucasKanadeTracker_Fast::LucasKanadeTracker_Fast", "ComputeXGradient failed with code 0x%x", lastResult);
            return;
          }

          if((lastResult = ImageProcessing::ComputeYGradient<u8,s16,s16>(templateImagePyramid[i], templateImageYGradientPyramid[i])) != RESULT_OK) {
            AnkiError("LucasKanadeTracker_Fast::LucasKanadeTracker_Fast", "ComputeYGradient failed with code 0x%x", lastResult);
            return;
          }
        }

        //this->isValid = true;

        EndBenchmark("LucasKanadeTracker_Fast");
      }

      bool LucasKanadeTracker_Fast::IsValid() const
      {
        if(!LucasKanadeTracker_Generic::IsValid())
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

      s32 LucasKanadeTracker_Fast::get_numTemplatePixels() const
      {
        return RoundS32(templateRegionHeight * templateRegionWidth);
      }
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki
