/**
 * File: visionSystem.h
 *
 * Author: Andrew Stein
 * Date:   (various)
 *
 * Description: High-level module that controls the vision system and switches
 *              between fiducial detection and tracking and feeds results to 
 *              main execution thread via message mailboxes.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_ROBOT_VISIONSYSTEM_H
#define ANKI_COZMO_ROBOT_VISIONSYSTEM_H

#include "anki/common/types.h"

#include "anki/common/robot/fixedLengthList.h"
#include "anki/common/robot/geometry_declarations.h"

#include "anki/vision/robot/fiducialMarkers.h"

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/robot/hal.h"
#include "visionParameters.h"

namespace Anki {
  namespace Cozmo {
    namespace VisionSystem {
    
      
      // This is main Update() call to be called in a loop from above.
      //
      // NOTE: It is important the passed-in robot state message be passed by
      //   value and NOT by reference, since the vision system can be interrupted
      //   by main execution (which updates the state).
      Result Update(const Messages::RobotState robotState);
      
        // Compute the 3D pose of a VisionMarker w.r.t. the camera.
      // - If the pose w.r.t. the robot is required, follow this with a call to
      //    the GetWithRespectToRobot() method below.
      // - If ignoreOrientation=true, the orientation of the marker within the
      //    image plane will be ignored (by sorting the marker's corners such
      //    that they always represent an upright marker).
      // NOTE: rotation should already be allocated as a 3x3 array.
      Result GetVisionMarkerPose(const Embedded::VisionMarker& marker,
                                 const bool                    ignoreOrientation,
                                 Embedded::Array<f32>&         rotationWrtCamera,
                                 Embedded::Point3<f32>&        translationWrtCamera);
      
      // Find the VisionMarker with the specified type whose 3D pose is closest
      // to the given 3D position (with respect to *robot*) and also within the
      // specified maxDistance (in mm).  If such a marker is found, the pose is
      // returned and the "markerFound" flag will be true.
      // NOTE: rotation should already be allocated as a 3x3 array.
      Result GetVisionMarkerPoseNearestTo(const Embedded::Point3<f32>&  atPosition,
                                          const Vision::MarkerType&     withType,
                                          const f32                     maxDistance_mm,
                                          Embedded::Array<f32>&         rotationWrtRobot,
                                          Embedded::Point3<f32>&        translationWrtRobot,
                                          bool&                         markerFound);
      
      // Convert a point or pose in camera coordinates to robot coordinates,
      // using the kinematic chain of the neck and head geometry.
      // NOTE: the rotation matrices should already be allocated as 3x3 arrays.
      Result GetWithRespectToRobot(const Embedded::Point3<f32>& pointWrtCamera,
                                   Embedded::Point3<f32>&       pointWrtRobot);
      
      Result GetWithRespectToRobot(const Embedded::Array<f32>&  rotationWrtCamera,
                                   const Embedded::Point3<f32>& translationWrtCamera,
                                   Embedded::Array<f32>&        rotationWrtRobot,
                                   Embedded::Point3<f32>&       translationWrtRobot);
      
      // Send single or continuous images back to basestation at requested resolution.
      // If resolution is not supported, this function does nothing.
      void SetImageSendMode(ImageSendMode_t mode, Vision::CameraResolution res);
      
      // Tell the vision system to grab a snapshot on the next call to Update(),
      // within the specified Region of Interest (roi), subsampled by the
      // given amount (e.g. every subsample=4 pixels) and put it in the given
      // snapshot array.  readyFlag will be set to true once this is done.
      // Calling this multiple times before the snapshot is ready does nothing
      // extra (internally, the system already knows it is waiting on a snapshot).
      Result TakeSnapshot(const Embedded::Rectangle<s32> roi, const s32 subsample,
                          Embedded::Array<u8>& snapshot, bool& readyFlag);

      
      // Tell the vision system to switch to face-detection mode
      Result StartDetectingFaces();
      
      // Returns field of view (radians) of camera
      f32 GetVerticalFOV();
      f32 GetHorizontalFOV();
      
      const FaceDetectionParameters& GetFaceDetectionParams();
      
      void SetParams(const bool autoExposureOn,
                     const f32 exposureTime,
                     const s32 integerCountsIncrement,
                     const f32 minExposureTime,
                     const f32 maxExposureTime,
                     const u8 highValue,
                     const f32 percentileToMakeHigh,
                     const bool limitFramerate);

      void SetFaceDetectParams(const f32 scaleFactor,
                               const s32 minNeighbors,
                               const s32 minObjectHeight,
                               const s32 minObjectWidth,
                               const s32 maxObjectHeight,
                               const s32 maxObjectWidth);
    
    } // namespace VisionSystem
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_VISIONSYSTEM_H
