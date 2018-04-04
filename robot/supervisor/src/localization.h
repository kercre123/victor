#ifndef LOCALIZATION_H
#define LOCALIZATION_H
#include "localization_geometry.h"
#include "coretech/common/shared/radians.h"
#include "coretech/common/shared/types.h"

namespace Anki {
  namespace Cozmo {
    namespace Localization {

      Result Init();

      f32 GetCurrPose_x();
      f32 GetCurrPose_y();
      const Radians& GetCurrPose_angle();
      Embedded::Pose2d GetCurrPose();

      void GetCurrPose(f32& x, f32& y, Radians& angle);
      void SetCurrPose(const f32 &x, const f32 &y, const Radians &angle);

      void GetDriveCenterPose(f32& x, f32& y, Radians& angle);
      void SetDriveCenterPose(const f32 &x, const f32 &y, const Radians &angle);

      f32 GetDriveCenterOffset();

      // Given a robotOriginPose, returns the pose of the drive center
      void ConvertToDriveCenterPose(const Anki::Embedded::Pose2d &robotOriginPose,
                                    Anki::Embedded::Pose2d &driveCenterPose);

      // Given a drive center pose, returns the robot origin pose
      void ConvertToOriginPose(const Anki::Embedded::Pose2d &driveCenterPose,
                               Anki::Embedded::Pose2d &robotOriginPose);

      // Get the current pose frame ID
      PoseFrameID_t GetPoseFrameId();
      
      // Get the current pose origin ID
      PoseOriginID_t GetPoseOriginId();

      // Clears internal pose history and sets pose frame ID to 0
      void ResetPoseFrame();

      void Update();

      // Uses the keyframe ("ground truth") pose at some past time as given
      // by the basestation after it has processed a mat marker to
      // update the current pose by transforming the given keyframe pose
      // by the pose-diff between the historical pose at time t and the current pose.
      // Also updates the current pose frame ID.
      // If t==0, updates the current pose. i.e. Pretty much calls SetCurrPose()
      Result UpdatePoseWithKeyframe(PoseOriginID_t originID,
                                    PoseFrameID_t frameID,
                                    TimeStamp_t t,
                                    const f32 x,
                                    const f32 y,
                                    const f32 angle);

      // Retrieves the closest historical pose at time t.
      // Returns OK if t is between the oldest and newest timestamps in history.
      // Otherwise returns FAIL, but p will still be set to the closest pose
      // in history at time t. It just may not be very close at all...
      Result GetHistPoseAtTime(TimeStamp_t t, Anki::Embedded::Pose2d& p);

      // Sets whether robot is on a ramp or not (the actual sloped portion),
      // and notifies the basestation when there is a change in ramp state.
      Result SetOnRamp(bool onRamp);

      // Sets whether robot is on a bridge or not, and notifies the
      // basestation when there is a change in bridge state.
      Result SetOnBridge(bool onBridge);

      // Returns true if robot is detected to be on a ramp.
      // This only works if the ramp is traversed using the controller.
      // i.e. Can't detect when manually driving up ramp.
      bool IsOnRamp();

      // Returns true if robot is detected to be on a bridge.
      // This only works if the bridge is traversed using the controller.
      // i.e. Can't detect when manually driving over a bridge.
      bool IsOnBridge();

      // Returns distance between the current pose and the given xy coordinates
      f32 GetDistTo(const f32 x, const f32 y);

      // Set motion model parameters
      void SetMotionModelParams(f32 slipFactor);

    } // Localization
  } // Cozmo
} // Anki
#endif
