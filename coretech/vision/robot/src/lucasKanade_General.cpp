/**
File: lucasKanade_General.cpp
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
      f32 UpdatePreviousCorners(
        const Transformations::PlanarTransformation_f32 &transformation,
        FixedLengthList<Quadrilateral<f32> > &previousCorners,
        MemoryStack scratch)
      {
        const f32 baseImageHalfWidth = static_cast<f32>(BASE_IMAGE_WIDTH) / 2.0f;
        const f32 baseImageHalfHeight = static_cast<f32>(BASE_IMAGE_HEIGHT) / 2.0f;

        Quadrilateral<f32> in(
          Point<f32>(-baseImageHalfWidth,-baseImageHalfHeight),
          Point<f32>(baseImageHalfWidth,-baseImageHalfHeight),
          Point<f32>(baseImageHalfWidth,baseImageHalfHeight),
          Point<f32>(-baseImageHalfWidth,baseImageHalfHeight));

        Quadrilateral<f32> newCorners = transformation.TransformQuadrilateral(in, scratch, 1.0f);

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
      } // f32 ComputeMinChange()
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki
