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

#define DUBINS_START_RADIUS_MM 50
#define DUBINS_END_RADIUS_MM 50

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
      Vec3f rotAxis = startPose.get_rotationAxis();
      if (!FLT_NEAR(rotAxis.x(), 0.f) || !FLT_NEAR(rotAxis.y(), 0.f) || !FLT_NEAR(ABS(rotAxis.z()), 1.f)) {
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
      rotAxis = targetPose.get_rotationAxis();
      if (!FLT_NEAR(rotAxis.x(), 0.f) || !FLT_NEAR(rotAxis.y(), 0.f) || !FLT_NEAR(ABS(rotAxis.z()), 1.f)) {
        PRINT_NAMED_ERROR("PathPlanner.GetPlan.NonZAxisRot_target",
                          "GetPlan() does not support rotations around anything other than z-axis (%f %f %f)\n",
                          rotAxis.x(), rotAxis.y(), rotAxis.z());
        return RESULT_FAIL;
      }
      if (FLT_NEAR(rotAxis.z(), -1.f)) {
        targetAngle *= -1;
      }


      if (Planning::GenerateDubinsPath(path,
                                       startPt.x(), startPt.y(), startAngle,
                                       targetPt.x(), targetPt.y(), targetAngle,
                                       DUBINS_START_RADIUS_MM, DUBINS_END_RADIUS_MM,
                                       DUBINS_TARGET_SPEED_MMPS, DUBINS_ACCEL_MMPS2, DUBINS_DECEL_MMPS2) == 0) {
        PRINT_NAMED_INFO("GetPlan.NoPathFound", "Could not generate Dubins path (startPose %f %f %f, targetPose %f %f %f)\n", startPt.x(), startPt.y(), startAngle, targetPt.x(), targetPt.y(), targetAngle);
        return RESULT_FAIL;
      }

      return RESULT_OK;
    }
    
    
  } // namespace Cozmo
} // namespace Anki