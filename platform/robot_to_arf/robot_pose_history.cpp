#include "robot_pose_history.h"

#include "anki/cozmo/shared/cozmoConfig.h"

namespace Anki {

namespace {
constexpr Anki::RobotTimeStamp_t kTimeBuffer = 5000; // ms
// For tool code reading
// Camera looking straight:
// const RotationMatrix3d Robot::_kDefaultHeadCamRotation = RotationMatrix3d({
//   0,     0,   1.f,
//  -1.f,   0,   0,
//   0,    -1.f, 0,
//});
// 4-degree look down:
const RotationMatrix3d kDefaultHeadCamRotation = RotationMatrix3d({
    0,
    -0.0698f,
    0.9976f,
    -1.0000f,
    0,
    0,
    0,
    -0.9976f,
    -0.0698f,
});

} // namespace

RobotPoseHistory::RobotPoseHistory
    : canonical_neck_pose_(0.f, Y_AXIS_3D(),
                           {NECK_JOINT_POSITION[0], NECK_JOINT_POSITION[1],
                            NECK_JOINT_POSITION[2]},
                           Pose3d(), "RobotNeck"),
      canonical_cam_pose_(kDefaultHeadCamRotation,
                          {HEAD_CAM_POSITION[0], HEAD_CAM_POSITION[1],
                           HEAD_CAM_POSITION[2]},
                          _nominal_neck_pose_, "RobotHeadCam"),
      canonical_cam_pose_(0.f, Y_AXIS_3D(),
                          {LIFT_BASE_POSITION[0], LIFT_BASE_POSITION[1],
                           LIFT_BASE_POSITION[2]},
                          Pose3d(), "RobotLiftBase"),
      raw_poses_(kTimeBuffer),
      localized_poses_(kTimeBuffer) {}

bool RobotPoseHistory::UpdateFromRobotState(const Anki::RobotState &state) {
  if (!pose_origin_list_.ContainsOriginID(state.pose_origin_id)) {
    return false;
  }
  const Anki::Pose3d &origin =
      pose_origin_list_.GetOriginByID(state.pose_origin_id);
  Anki::Pose3d new_pose(state.pose.angle, Z_AXIS_3D(),
                        {state.pose.x, state.pose.y, state.pose.z}, origin);
  raw_poses_.Insert(state.timestamp, new_pose);
  static_state_.Insert(state.timestamp, StaticRobotState(state));
}

} // namespace Anki