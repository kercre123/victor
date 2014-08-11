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
      
      // Clears internal pose history and sets pose frame ID to 0
      void ResetPoseFrame();
      
      void Update();
      
      // Sets whether robot is on a ramp or not (the actual sloped portion),
      // and notifies the basestation when there is a change in ramp state.
      Result SetOnRamp(bool onRamp);
      bool GetOnRamp();
      
      // Uses the keyframe ("ground truth") pose at some past time as given
      // by the basestation after it has processed a mat marker to
      // update the current pose by transforming the given keyframe pose
      // by the pose-diff between the historical at time t and the current pose.
      // Also updates the current pose frame ID.
      Result UpdatePoseWithKeyframe(PoseFrameID_t frameID, TimeStamp_t t, const f32 x, const f32 y, const f32 angle);

      // Enables/disables the OnRamp state of the robot.
      // This should only be called from the controller that executes ramp traversal actions
      Result SetOnRamp(bool onRamp);
      
      // Returns true if robot is detected to be on a ramp.
      // This only works if the ramp is traversed using the controller.
      // i.e. Can't detect when manually driving up ramp.
      bool IsOnRamp();
      
      // Returns distance between the current pose and the given xy coordinates
      f32 GetDistTo(const f32 x, const f32 y);
      
    } // Localization
  } // Cozmo
} // Anki
#endif

