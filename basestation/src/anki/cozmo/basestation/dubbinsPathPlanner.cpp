/**
 * File: dubbinsPathPlanner.h
 *
 * Author: Andrew / Kevin (reorg by Brad)
 * Created: 2015-09-16
 *
 * Description: Dubbins path planner
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/common/basestation/math/pose.h"
#include "anki/planning/shared/path.h"
#include "dubbinsPathPlanner.h"
#include "util/logging/logging.h"

#define DUBINS_TARGET_SPEED_MMPS 50
#define DUBINS_ACCEL_MMPS2 200
#define DUBINS_DECEL_MMPS2 200

#define DUBINS_START_RADIUS_MM 50.f
#define DUBINS_END_RADIUS_MM 50.f

namespace Anki {
namespace Cozmo {

DubbinsPlanner::DubbinsPlanner() {}

IPathPlanner::EPlanStatus DubbinsPlanner::GetPlan(Planning::Path &path,
                                                  const Pose3d &startPose,
                                                  const Pose3d &targetPose)
{
  Vec3f startPt = startPose.GetTranslation();
  f32 startAngle = startPose.GetRotationAngle<'Z'>().ToFloat(); // Assuming robot is not tilted
      
  // Currently, we can only deal with rotations around (0,0,1) or (0,0,-1).
  // If it's something else, then quit.
  // TODO: Something smarter?
  Vec3f rotAxis;
  Radians rotAngle;
  startPose.GetRotationVector().GetAngleAndAxis(rotAngle, rotAxis);
  float dotProduct = DotProduct(rotAxis, Z_AXIS_3D());
  const float dotProductThreshold = 0.0152f; // 1.f - std::cos(DEG_TO_RAD(10)); // within 10 degrees
  if(!NEAR(rotAngle.ToFloat(), 0, DEG_TO_RAD(10)) && !NEAR(std::abs(dotProduct), 1.f, dotProductThreshold)) {
    PRINT_NAMED_ERROR("PathPlanner.GetPlan.NonZAxisRot_start",
                      "GetPlan() does not support rotations around anything other than z-axis (%f %f %f)\n",
                      rotAxis.x(), rotAxis.y(), rotAxis.z());
    return PLAN_NEEDED_BUT_START_FAILURE;
  }
  if (FLT_NEAR(rotAxis.z(), -1.f)) {
    startAngle *= -1;
  }
      
      
  Vec3f targetPt = targetPose.GetTranslation();
  f32 targetAngle = targetPose.GetRotationAngle<'Z'>().ToFloat(); // Assuming robot is not tilted

  // Currently, we can only deal with rotations around (0,0,1) or (0,0,-1).
  // If it's something else, then quit.
  // TODO: Something smarter?
  targetPose.GetRotationVector().GetAngleAndAxis(rotAngle, rotAxis);
  dotProduct = DotProduct(rotAxis, Z_AXIS_3D());
  if(!NEAR(rotAngle.ToFloat(), 0, DEG_TO_RAD(10)) && !NEAR(std::abs(dotProduct), 1.f, dotProductThreshold)) {
    PRINT_NAMED_ERROR("PathPlanner.GetPlan.NonZAxisRot_target",
                      "GetPlan() does not support rotations around anything other than z-axis (%f %f %f)\n",
                      rotAxis.x(), rotAxis.y(), rotAxis.z());
    return PLAN_NEEDED_BUT_GOAL_FAILURE;
  }
  if (FLT_NEAR(rotAxis.z(), -1.f)) {
    targetAngle *= -1;
  }

  const f32 dubinsRadius = (targetPt - startPt).Length() * 0.25f;
      
  if (Planning::GenerateDubinsPath(path,
                                   startPt.x(), startPt.y(), startAngle,
                                   targetPt.x(), targetPt.y(), targetAngle,
                                   std::min(DUBINS_START_RADIUS_MM, dubinsRadius),
                                   std::min(DUBINS_END_RADIUS_MM, dubinsRadius),
                                   DUBINS_TARGET_SPEED_MMPS, DUBINS_ACCEL_MMPS2, DUBINS_DECEL_MMPS2) == 0) {
    PRINT_NAMED_INFO("GetPlan.NoPathFound",
                     "Could not generate Dubins path (startPose %f %f %f, targetPose %f %f %f)\n",
                     startPt.x(), startPt.y(), startAngle,
                     targetPt.x(), targetPt.y(), targetAngle);
    return PLAN_NEEDED_BUT_PLAN_FAILURE;
  }

  return DID_PLAN;
}

}
}

