#include "anki/embeddedVision/miscVisionKernels.h"

namespace Anki
{
  namespace Embedded
  {
    Result ComputeHomographyFromQuad(const Quadrilateral<s16> &quad, Array<f64> &homography, MemoryStack scratch)
    {
      AnkiConditionalErrorAndReturnValue(homography.IsValid(),
        RESULT_FAIL, "ComputeHomographyFromQuad", "homography is not valid");

      AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
        RESULT_FAIL, "ComputeHomographyFromQuad", "scratch is not valid");

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

      if(EstimateHomography(originalPoints, transformedPoints, homography, scratch) != RESULT_OK)
        return RESULT_FAIL;

      return RESULT_OK;
    } // Result ComputeHomographyFromQuad(FixedLengthList<Quadrilateral<s16> > quads, FixedLengthList<Array<f64> > &homographies, MemoryStack scratch)
  } // namespace Embedded
} // namespace Anki