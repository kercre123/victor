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
      static Result ComputeDockingErrorSignal_Affine(const Array<f32> &homography, const Quadrilateral<f32> &templateRegion, const s32 horizontalTrackingResolution, const f32 blockMarkerWidthInMM, const f32 horizontalFocalLengthInMM, const f32 cozmoLiftDistanceInMM, f32 &rel_x, f32 &rel_y, f32 &rel_rad, MemoryStack scratch);

      //currentDistance = BlockMarker3D.ReferenceWidth * this.calibration.fc(1) / L;

      Result ComputeDockingErrorSignal(const TemplateTracker::PlanarTransformation_f32 &transform, const s32 horizontalTrackingResolution, const f32 blockMarkerWidthInMM, const f32 horizontalFocalLengthInMM, const f32 cozmoLiftDistanceInMM, f32 &rel_x, f32 &rel_y, f32 &rel_rad, MemoryStack scratch)
      {
        // Set these now, so if there is an error, the robot will start driving in a circle
        rel_x = -1.0;
        rel_y = -1.0f;
        rel_rad = -1.0f;

        if(transform.get_transformType() == TemplateTracker::TRANSFORM_AFFINE) {
          return ComputeDockingErrorSignal_Affine(transform.get_homography(), transform.get_transformedCorners(scratch), horizontalTrackingResolution, blockMarkerWidthInMM, horizontalFocalLengthInMM, cozmoLiftDistanceInMM, rel_x, rel_y, rel_rad, scratch);
        }

        AnkiAssert(false);

        return RESULT_FAIL_INVALID_PARAMETERS;
      }

      static Result ComputeDockingErrorSignal_Affine(const Array<f32> &homography, const Quadrilateral<f32> &templateRegion, const s32 horizontalTrackingResolution, const f32 blockMarkerWidthInMM, const f32 horizontalFocalLengthInMM, const f32 cozmoLiftDistanceInMM, f32 &rel_x, f32 &rel_y, f32 &rel_rad, MemoryStack scratch)
      {
        // Block may be rotated with top side of marker not facing up, so reorient to make sure we
        // got top corners
        //[th,~] = cart2pol(corners(:,1)-mean(corners(:,1)), corners(:,2)-mean(corners(:,2)));
        //[~,sortIndex] = sort(th);

        Array<f32> thetas(1,4,scratch);
        Array<s32> indexes(1,4,scratch);
        Point<f32> center = templateRegion.ComputeCenter();

        for(s32 i=0; i<4; i++) {
          f32 rho = 0.0f;

          Cart2Pol<f32>(
            templateRegion[i].x - center.x,
            templateRegion[i].y - center.y,
            rho, thetas[0][i]);
        }

        Matrix::Sort(thetas, indexes, 1);

        //upperLeft = corners(sortIndex(1),:);
        //upperRight = corners(sortIndex(2),:);
        const Point<f32> upperLeft = templateRegion[indexes[0][0]];
        const Point<f32> upperRight = templateRegion[indexes[0][1]];

        //AnkiAssert(upperRight(1) > upperLeft(1), ...%if upperRight(1) < upperLeft(1)
        //  ['UpperRight corner should be to the right ' ...
        //  'of the UpperLeft corner.']);

        AnkiAssert(upperRight.x > upperLeft.x);

        // Get the angle from vertical of the top bar of the marker we're tracking

        //L = sqrt(sum( (upperRight-upperLeft).^2) );
        const f32 lineDx = upperRight.x - upperLeft.x;
        const f32 lineDy = upperRight.y - upperLeft.y;
        const f32 lineLength = sqrtf(lineDx*lineDx + lineDy*lineDy);

        //angleError = -asin( (upperRight(2)-upperLeft(2)) / L);
        const f32 angleError = -asinf( (upperRight.y-upperLeft.y) / lineLength);

        //currentDistance = BlockMarker3D.ReferenceWidth * this.calibration.fc(1) / L;
        const f32 currentDistance = blockMarkerWidthInMM * horizontalFocalLengthInMM / lineLength;

        //distError = currentDistance - CozmoDocker.LIFT_DISTANCE;
        const f32 distanceError = currentDistance - cozmoLiftDistanceInMM;

        // TODO: should I be comparing to ncols/2 or calibration center?

        //midPointErr = -( (upperRight(1)+upperLeft(1))/2 - this.trackingResolution(1)/2 );
        f32 midpointError = -( (upperRight.x+upperLeft.x)/2 - horizontalTrackingResolution/2 );

        //midPointErr = midPointErr * currentDistance / this.calibration.fc(1);
        midpointError *= currentDistance / horizontalFocalLengthInMM;

        // Convert the naming schemes
        rel_x = distanceError;
        rel_y = midpointError;
        rel_rad = angleError;

        return RESULT_OK;
      }
    } // namespace Docking
  } // namespace Embedded
} // namespace Anki