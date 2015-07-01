/**
 * File: faceTrackingController.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 9/29/2014
 *
 * Description:
 *
 *   Controller for "tracking" a face, meaning keeping the robot's attention 
 *   focused on the face, by turning its body and tilting its head to keep the
 *   face centered in its field of view. In other words, this is a pan-tilt
 *   controller where the face detections coming from the vision system are used
 *   as the control signal.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef COZMO_FACE_TRACKING_CONTROLLER_H_
#define COZMO_FACE_TRACKING_CONTROLLER_H_

#include "anki/common/types.h"

#include "anki/common/robot/geometry_declarations.h"

#include "messages.h"


namespace Anki {
  namespace Cozmo {
    // Forward declaration:
    namespace VisionSystem {
      struct FaceDetectionParameters;
    }
    
    namespace FaceTrackingController {
      
      enum FaceSelectionMethod {
        LARGEST,
        CENTERED
      };
      
      Result Init(const int faceDetectionWidth, const int faceDetectionHeight);

      Result StartTracking(FaceSelectionMethod method, u32 timeout_sec);
      
      Result Update();
      
      Result Reset();
      
      // Returns true if tracker is currently looking for faces, whether or not
      // it is actively tracking. If tracking times out, this will return false.
      bool IsBusy();

      // Sets the latest object position message coming from engine
      void SetObjectPositionMessage(const Messages::FaceDetection& msg);
      
    } // namespace FaceTrackingController
  } // namespace Cozmo
} // namespace Anki

#endif // COZMO_FACE_TRACKING_CONTROLLER_H_
