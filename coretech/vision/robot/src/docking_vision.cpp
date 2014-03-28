/**
File: docking.cpp
Author: Peter Barnum
Created: December 3, 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/docking_vision.h"

#define USE_MATLAB_FOR_PROJECTIVE_ERROR_SIGNAL 0

#if USE_MATLAB_FOR_PROJECTIVE_ERROR_SIGNAL
#include "anki/common/robot/matlabInterface.h"
static Anki::Embedded::Matlab matlab(false);
static bool matlabInit = false;
#endif

namespace Anki
{
  namespace Embedded
  {
    namespace Docking
    {
      /*
      static Result ComputeDockingErrorSignal_Affine(const Array<f32> &homography, const Quadrilateral<f32> &templateRegion, const s32 horizontalTrackingResolution, const f32 blockMarkerWidthInMM, const f32 horizontalFocalLengthInMM, f32 &rel_x, f32 &rel_y, f32 &rel_rad, MemoryStack scratch);
      */
      static Result ComputeDockingErrorSignal_Affine(const Quadrilateral<f32> &templateRegion,
        const s32 horizontalTrackingResolution,
        const f32 blockMarkerWidthInMM,
        const f32 horizontalFocalLengthInMM,
        f32 &rel_x, f32 &rel_y, f32 &rel_rad,
        MemoryStack scratch);
      
      static Result ComputeDockingErrorSignal_Projective(const Quadrilateral<f32> &templateRegion,
                                                         const s32 horizontalTrackingResolution,
                                                         const f32 blockMarkerWidthInMM,
                                                         const f32 horizontalFocalLengthInMM,
                                                         f32 &rel_x, f32 &rel_y, f32 &rel_rad,
                                                         MemoryStack scratch);
      

      //currentDistance = BlockMarker3D.ReferenceWidth * this.calibration.fc(1) / L;

      Result ComputeDockingErrorSignal(const Transformations::PlanarTransformation_f32 &transform,
        const s32 horizontalTrackingResolution,
        const f32 blockMarkerWidthInMM,
        const f32 horizontalFocalLengthInMM,
        f32& rel_x, f32& rel_y, f32& rel_rad,
        MemoryStack scratch, const f32* calib)
      {
        AnkiAssert(blockMarkerWidthInMM > 0.f);

        // Set these now, so if there is an error, the robot will start driving in a circle
        rel_x = -1.0;
        rel_y = -1.0f;
        rel_rad = -1.0f;

        Quadrilateral<f32> transformedQuad = transform.get_transformedCorners(scratch);

		// NOTE: ComputeDockingErrorSignal_Affine will work for affine or projective
        if(transform.get_transformType() == Transformations::TRANSFORM_AFFINE)
        {
          return ComputeDockingErrorSignal_Affine(transformedQuad,
            horizontalTrackingResolution,
            blockMarkerWidthInMM,
            horizontalFocalLengthInMM,
            rel_x, rel_y, rel_rad,
            scratch);
        }
        else if(transform.get_transformType() == Transformations::TRANSFORM_PROJECTIVE)
        {
#if USE_MATLAB_FOR_PROJECTIVE_ERROR_SIGNAL

          /* Matlab code we are duplicating:
           
           this.block.pose = this.headCam.computeExtrinsics(...
           scale*this.LKtracker.corners, this.marker3d, ...
           'initialPose', initialPose);
           
           blockWRTrobot = this.block.pose.getWithRespectTo(this.robotPose);
           distError     = blockWRTrobot.T(1);
           midPointErr   = blockWRTrobot.T(2);
           angleError    = atan2(blockWRTrobot.Rmat(2,1), blockWRTrobot.Rmat(1,1));
           */

          if(!matlabInit) {
            matlab.Put(calib, 4, "calib");
            
            matlab.EvalStringEcho("run('~/Code/products-cozmo/matlab/initCozmoPath'); "
                                  //"robotPose = Pose([0 0 0], [0 0 0]); "
                                  //"neckPose = Pose([0 0 0], [-13 0 33.5+14.2]); "
                                  //"neckPose.parent = robotPose; "
                                  //"HEAD_ANGLE = -10; " // degrees
                                  //"Rpitch = rodrigues(-HEAD_ANGLE*pi/180*[0 1 0]); "
                                  //"headPose = Pose(Rpitch * [0 0 1; -1 0 0; 0 -1 0], "
                                  //"                Rpitch * [11.45 0 -6]'); "
                                  //"headPose.parent = neckPose; "
                                  "camera = Camera('calibration', struct("
                                  "   'fc', calib(1:2), 'cc', calib(3:4), "
                                  "   'kc', zeros(5,1), 'alpha_c', 0)); " //, "
                                  //"   'pose', headPose); "
                                  "marker3d = (%f/2)*[-1 0 1; -1 0 -1; 1 0 1; 1 0 -1];",
                                  blockMarkerWidthInMM);
            
            matlabInit = true;
          }
          
          matlab.PutQuad(transformedQuad, "transformedQuad");
          matlab.PutArray(transform.get_homography(), "trackerTform");
          
          matlab.EvalStringEcho("blockPose = camera.computeExtrinsics(transformedQuad, marker3d); "
                                //"blockWRTrobot = blockPose.getWithRespectTo(robotPose); "
                                //"angleError    = atan2(blockWRTrobot.Rmat(2,1), blockWRTrobot.Rmat(1,1)) + pi/2; "
                                "angleError    = -atan2(blockPose.Rmat(1,1), blockPose.Rmat(3,1)) + pi/2; " // why + pi/2 ??
                                "distError     = blockPose.T(3); "
                                "midPointErr   = -blockPose.T(1); "
                                ); // why +pi/2??
                                //"desktop; keyboard;");
          
          rel_x   = static_cast<f32>(mxGetScalar(matlab.GetArray("distError")));
          rel_y   = static_cast<f32>(mxGetScalar(matlab.GetArray("midPointErr")));
          rel_rad = static_cast<f32>(mxGetScalar(matlab.GetArray("angleError")));
                                
                                
          
          /*
          return ComputeDockingErrorSignal_Projective(transformedQuad,
                                                      horizontalTrackingResolution,
                                                      blockMarkerWidthInMM,
                                                      horizontalFocalLengthInMM,
                                                      rel_x, rel_y, rel_rad,
                                                      scratch);
           */
#else
          
          // Not using Matlab, just call the same affine error signal
          // generator even for projective tracker for now
          
          return ComputeDockingErrorSignal_Affine(transformedQuad,
                                                  horizontalTrackingResolution,
                                                  blockMarkerWidthInMM,
                                                  horizontalFocalLengthInMM,
                                                  rel_x, rel_y, rel_rad,
                                                  scratch);
#endif
          
        }
        else {
          AnkiAssert(false);
        }
        
        return RESULT_FAIL_INVALID_PARAMETERS;
      }

      /*
      Result ComputeDockingErrorSignal(const Transformations::PlanarTransformation_f32 &transform, const s32 horizontalTrackingResolution, const f32 blockMarkerWidthInMM, const f32 horizontalFocalLengthInMM, f32 &rel_x, f32 &rel_y, f32 &rel_rad, MemoryStack scratch)
      {
      // Set these now, so if there is an error, the robot will start driving in a circle
      rel_x = -1.0;
      rel_y = -1.0f;
      rel_rad = -1.0f;

      if(transform.get_transformType() == Transformations::TRANSFORM_AFFINE) {
      Quadrilateral<f32> transformedCorners = transform.get_transformedCorners(scratch);

      return ComputeDockingErrorSignal_Affine(transform.get_homography(), transform.get_transformedCorners(scratch), horizontalTrackingResolution, blockMarkerWidthInMM, horizontalFocalLengthInMM, rel_x, rel_y, rel_rad, scratch);
      }

      AnkiAssert(false);

      return RESULT_FAIL_INVALID_PARAMETERS;
      }
      */

      static Result ComputeDockingErrorSignal_Affine(const Quadrilateral<f32> &templateRegion,
        const s32 horizontalTrackingResolution,
        const f32 blockMarkerWidthInMM,
        const f32 horizontalFocalLengthInMM,
        f32 &rel_x, f32 &rel_y, f32 &rel_rad,
        MemoryStack scratch)
      {
        // Block may be rotated with top side of marker not facing up, so reorient to make sure we
        // got top corners
        //[th,~] = cart2pol(corners(:,1)-mean(corners(:,1)), corners(:,2)-mean(corners(:,2)));
        //[~,sortIndex] = sort(th);

        Quadrilateral<f32> sortedQuad = templateRegion.ComputeClockwiseCorners();
        const Point<f32>& lineLeft  = sortedQuad[3]; // bottomLeft
        const Point<f32>& lineRight = sortedQuad[2]; // bottomRight

        //AnkiAssert(upperRight(1) > upperLeft(1), ...%if upperRight(1) < upperLeft(1)
        //  ['UpperRight corner should be to the right ' ...
        //  'of the UpperLeft corner.']);

        AnkiAssert(lineRight.x > lineLeft.x);

        // Get the angle from vertical of the top bar of the marker we're tracking

        //L = sqrt(sum( (upperRight-upperLeft).^2) );
        const f32 lineDx = lineRight.x - lineLeft.x;
        const f32 lineDy = lineRight.y - lineLeft.y;
        const f32 lineLength = sqrtf(lineDx*lineDx + lineDy*lineDy);

        //angleError = -asin( (upperRight(2)-upperLeft(2)) / L);
        //const f32 angleError = -asinf( (upperRight.y-upperLeft.y) / lineLength);
        const f32 angleError = -asinf( (lineRight.y-lineLeft.y) / lineLength) * 4;  // Multiply by scalar which makes angleError a little more accurate.  TODO: Something smarter than this.

        //currentDistance = BlockMarker3D.ReferenceWidth * this.calibration.fc(1) / L;
        const f32 distanceError = blockMarkerWidthInMM * horizontalFocalLengthInMM / lineLength;

        //ANS: now returning error in terms of camera. mainExecution converts to robot coords
        // //distError = currentDistance - CozmoDocker.LIFT_DISTANCE;
        // const f32 distanceError = currentDistance - cozmoLiftDistanceInMM;

        // TODO: should I be comparing to ncols/2 or calibration center?

        //midPointErr = -( (upperRight(1)+upperLeft(1))/2 - this.trackingResolution(1)/2 );
        f32 midpointError = -( (lineRight.x+lineLeft.x)/2 - horizontalTrackingResolution/2 );

        //midPointErr = midPointErr * currentDistance / this.calibration.fc(1);
        midpointError *= distanceError / horizontalFocalLengthInMM;

        // Convert the naming schemes
        rel_x = distanceError;
        rel_y = midpointError;
        rel_rad = angleError;

        return RESULT_OK;
        
      } // ComputeDockingErrorSignal_Affine()
      

      
      static Result ComputeDockingErrorSignal_Projective(const Quadrilateral<f32> &templateRegion,
                                                         const s32 horizontalTrackingResolution,
                                                         const f32 blockMarkerWidthInMM,
                                                         const f32 horizontalFocalLengthInMM,
                                                         f32 &rel_x, f32 &rel_y, f32 &rel_rad,
                                                         MemoryStack scratch)
      {
        AnkiAssert(false);
        return RESULT_OK;
      } // ComputeDockingErrorSignal_Projective()
      
    } // namespace Docking
  } // namespace Embedded
} // namespace Anki
