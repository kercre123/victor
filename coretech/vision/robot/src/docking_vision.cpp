/**
File: docking.cpp
Author: Peter Barnum
Created: December 3, 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/docking_vision.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Docking
    {
      static Result ComputeDockingErrorSignal_Affine(const Array<f32> &homography, f32 &rel_x, f32 &rel_y, f32 &rel_rad);

      Result ComputeDockingErrorSignal(const TemplateTracker::PlanarTransformation_f32 &transform, f32 &rel_x, f32 &rel_y, f32 &rel_rad)
      {
        // Set these now, so if there is an error, the robot will start driving in a circle
        rel_x = -1.0;
        rel_y = -1.0f;
        rel_rad = -1.0f;

        if(transform.get_transformType() == TemplateTracker::TRANSFORM_AFFINE) {
          return ComputeDockingErrorSignal_Affine(transform.get_homography(), rel_x, rel_y, rel_rad);
        }

        assert(false);

        return RESULT_FAIL;
      }

      static Result ComputeDockingErrorSignal_Affine(const Array<f32> &homography, f32 &rel_x, f32 &rel_y, f32 &rel_rad)
      {
        return RESULT_OK;
      }
    } // namespace Docking
  } // namespace Embedded
} // namespace Anki