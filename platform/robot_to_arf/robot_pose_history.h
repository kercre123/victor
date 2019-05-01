#include <mutex>

#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/robotTimeStamp.h"

class RobotStateHistory {

  RobotStateHistory();

  bool UpdateFromRobotState(const Anki::RobotState &state);

  // Current best estimate of the robot pose.
  Pose3d GetCurrentPose() const {}

  void RegisterCurrentPoseUpdatedCallback();

  HistoricalRobotState GetHistoricalRobotState(RobotTimestamp_t timestamp);

  Pose3D GetHistoricalCameraPose(RobotTimestamp_t timestamp);

  bool AddLocalizedPose(const Anki::Pose3d &pose);

private:
  std::mutex mutex_;
  const Pose3d canonical_neck_pose_;
  const Pose3d canonical_head_cam_pose_;
  const Pose3d canonical_lift_base_pose_;
  PoseOriginList pose_origin_list_;
  TemporalTimeLimitedBuffer<StaticRobotState, RobotTimeStamp_t> static_state_;
  TemporalTimeLimitedBuffer<ProxSensorData, RobotTimestamp_t> prox_data_;
  TemporalTimeLimitedBuffer<Anki::Pose3d, RobotTimeStamp_t> raw_poses_;
  TemporalTimeLimitedBuffer<Anki::Pose3d, RobotTimeStamp_t> localized_poses_;
  TemporalTimeLimitedBuffer<Anki::Pose3d, RobotTimeStamp_t> computed_poses_;
}