#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "anki/common/shared/radians.h"
#include "anki/common/types.h"


namespace Anki {
  namespace Cozmo {
    namespace Localization {

      Result Init();
      
      //Embedded::Pose2d GetCurrMatPose();
      
      void GetCurrentMatPose(f32& x, f32& y, Radians& angle);
      void SetCurrentMatPose(f32  x, f32  y, Radians  angle);

      // Get orientation of robot on current mat
      Radians GetCurrentMatOrientation();

      // Get the current pose frame ID
      PoseFrameID_t GetPoseFrameId();
      
      void Update();

      
      // Uses the keyframe ("ground truth") pose at some past time as given
      // by the basestation after it has processed a mat marker to
      // update the current pose by transforming the given keyframe pose
      // by the pose-diff between the historical at time t and the current pose.
      // Also updates the current pose frame ID.
      Result UpdatePoseWithKeyframe(PoseFrameID_t frameID, TimeStamp_t t, const f32 x, const f32 y, const f32 angle);
      
    } // Localization
  } // Cozmo
} // Anki
#endif

