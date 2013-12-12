/**
File: transformations.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/miscVisionKernels.h"
#include "anki/common/robot/opencvLight.h"

namespace Anki
{
  namespace Embedded
  {
    static Result lastResult;

    Result ComputeHomographyFromQuad(const Quadrilateral<s16> &quad, Array<f64> &homography, MemoryStack scratch)
    {
      AnkiConditionalErrorAndReturnValue(homography.IsValid(),
        RESULT_FAIL_INVALID_ARRAY, "ComputeHomographyFromQuad", "homography is not valid");

      AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
        RESULT_FAIL_INVALID_ARRAY, "ComputeHomographyFromQuad", "scratch is not valid");

      FixedLengthList<Point<f64> > originalPoints(4, scratch);
      FixedLengthList<Point<f64> > transformedPoints(4, scratch);

      originalPoints.PushBack(Point<f64>(0,0));
      originalPoints.PushBack(Point<f64>(0,1));
      originalPoints.PushBack(Point<f64>(1,0));
      originalPoints.PushBack(Point<f64>(1,1));

      transformedPoints.PushBack(Point<f64>(quad[0].x, quad[0].y));
      transformedPoints.PushBack(Point<f64>(quad[1].x, quad[1].y));
      transformedPoints.PushBack(Point<f64>(quad[2].x, quad[2].y));
      transformedPoints.PushBack(Point<f64>(quad[3].x, quad[3].y));

      if((lastResult = EstimateHomography(originalPoints, transformedPoints, homography, scratch)) != RESULT_OK)
        return lastResult;

      return RESULT_OK;
    } // Result ComputeHomographyFromQuad(FixedLengthList<Quadrilateral<s16> > quads, FixedLengthList<Array<f64> > &homographies, MemoryStack scratch)
  } // namespace Embedded
} // namespace Anki