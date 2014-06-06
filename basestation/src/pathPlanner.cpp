/**
 * File: pathPlanner.cpp
 *
 * Author: Kevin Yoon
 * Date:   2/24/2014
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "pathPlanner.h"
#include "anki/common/basestation/general.h"
//#include "anki/cozmo/robot/cozmoConfig.h"

#define DUBINS_TARGET_SPEED_MMPS 50
#define DUBINS_ACCEL_MMPS2 200
#define DUBINS_DECEL_MMPS2 200

#define DUBINS_START_RADIUS_MM 50.f
#define DUBINS_END_RADIUS_MM 50.f

namespace Anki {
  namespace Cozmo {
    
    PathPlanner::PathPlanner() {}

    Result PathPlanner::GetPlan(Planning::Path &path, const Pose3d &startPose, const Pose3d &targetPose)
    {
      Vec3f startPt = startPose.get_translation();
      f32 startAngle = startPose.get_rotationAngle().ToFloat(); // Assuming robot is not tilted
      
      // Currently, we can only deal with rotations around (0,0,1) or (0,0,-1).
      // If it's something else, then quit.
      // TODO: Something smarter?
      Vec3f rotAxis;
      Radians rotAngle;
      startPose.get_rotationVector().get_angleAndAxis(rotAngle, rotAxis);
      float dotProduct = DotProduct(rotAxis, Z_AXIS_3D);
      const float dotProductThreshold = 0.0152f; // 1.f - std::cos(DEG_TO_RAD(10)); // within 10 degrees
      if(!NEAR(rotAngle.ToFloat(), 0, DEG_TO_RAD(10)) && !NEAR(std::abs(dotProduct), 1.f, dotProductThreshold)) {
        PRINT_NAMED_ERROR("PathPlanner.GetPlan.NonZAxisRot_start",
                          "GetPlan() does not support rotations around anything other than z-axis (%f %f %f)\n",
                          rotAxis.x(), rotAxis.y(), rotAxis.z());
        return RESULT_FAIL;
      }
      if (FLT_NEAR(rotAxis.z(), -1.f)) {
        startAngle *= -1;
      }
      
      
      Vec3f targetPt = targetPose.get_translation();
      f32 targetAngle = targetPose.get_rotationAngle().ToFloat(); // Assuming robot is not tilted

      // Currently, we can only deal with rotations around (0,0,1) or (0,0,-1).
      // If it's something else, then quit.
      // TODO: Something smarter?
      targetPose.get_rotationVector().get_angleAndAxis(rotAngle, rotAxis);
      dotProduct = DotProduct(rotAxis, Z_AXIS_3D);
      if(!NEAR(rotAngle.ToFloat(), 0, DEG_TO_RAD(10)) && !NEAR(std::abs(dotProduct), 1.f, dotProductThreshold)) {
        PRINT_NAMED_ERROR("PathPlanner.GetPlan.NonZAxisRot_target",
                          "GetPlan() does not support rotations around anything other than z-axis (%f %f %f)\n",
                          rotAxis.x(), rotAxis.y(), rotAxis.z());
        return RESULT_FAIL;
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
        PRINT_NAMED_INFO("GetPlan.NoPathFound", "Could not generate Dubins path (startPose %f %f %f, targetPose %f %f %f)\n", startPt.x(), startPt.y(), startAngle, targetPt.x(), targetPt.y(), targetAngle);
        return RESULT_FAIL;
      }

      return RESULT_OK;
    }
    
    
  } // namespace Cozmo
} // namespace Anki