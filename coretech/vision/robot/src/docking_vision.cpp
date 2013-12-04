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
      static Result ComputeDockingErrorSignal_Affine(const Array<f32> &homography, const Quadrilateral<f32> &templateRegion, f32 &rel_x, f32 &rel_y, f32 &rel_rad);

      Result ComputeDockingErrorSignal(const TemplateTracker::PlanarTransformation_f32 &transform, f32 &rel_x, f32 &rel_y, f32 &rel_rad)
      {
        // Set these now, so if there is an error, the robot will start driving in a circle
        rel_x = -1.0;
        rel_y = -1.0f;
        rel_rad = -1.0f;

        if(transform.get_transformType() == TemplateTracker::TRANSFORM_AFFINE) {
          return ComputeDockingErrorSignal_Affine(transform.get_homography(), transform.get_transformedCorners(), rel_x, rel_y, rel_rad);
        }

        assert(false);

        return RESULT_FAIL;
      }

      static Result ComputeDockingErrorSignal_Affine(const Array<f32> &homography, const Quadrilateral<f32> &templateRegion, f32 &rel_x, f32 &rel_y, f32 &rel_rad)
      {
        // Block may be rotated with top side of marker not facing up, so reorient to make sure we
        // got top corners
        //[th,~] = cart2pol(corners(:,1)-mean(corners(:,1)), corners(:,2)-mean(corners(:,2)));
        //[~,sortIndex] = sort(th);

        //upperLeft = corners(sortIndex(1),:);
        //upperRight = corners(sortIndex(2),:);

        //assert(upperRight(1) > upperLeft(1), ...%if upperRight(1) < upperLeft(1)
        //  ['UpperRight corner should be to the right ' ...
        //  'of the UpperLeft corner.']);

        // Get the angle from vertical of the top bar of the marker we're tracking
        //L = sqrt(sum( (upperRight-upperLeft).^2) );
        //angleError = -asin( (upperRight(2)-upperLeft(2)) / L);

        //currentDistance = BlockMarker3D.ReferenceWidth * this.calibration.fc(1) / L;
        //distError = currentDistance - CozmoDocker.LIFT_DISTANCE;

        // TODO: should i be comparing to ncols/2 or calibration center?
        //midPointErr = -( (upperRight(1)+upperLeft(1))/2 - this.trackingResolution(1)/2 );
        //midPointErr = midPointErr * currentDistance / this.calibration.fc(1);

        return RESULT_OK;
      }
    } // namespace Docking
  } // namespace Embedded
} // namespace Anki