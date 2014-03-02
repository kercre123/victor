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


#define DUBINS_START_RADIUS_MM 50
#define DUBINS_END_RADIUS_MM 50

namespace Anki {
  namespace Cozmo {
    
    PathPlanner::PathPlanner() {}

    ReturnCode PathPlanner::GetPlan(Planning::Path &path, const Pose3d &startPose, const Pose3d &targetPose)
    {
      Vec3f startPt = startPose.get_translation();
      f32 startAngle = startPose.get_rotationAngle().ToFloat(); // Assuming robot is not tilted
      
      Vec3f targetPt = targetPose.get_translation();
      f32 targetAngle = targetPose.get_rotationAngle().ToFloat(); // Assuming robot is not tilted
      

      if (Planning::GenerateDubinsPath(path,
                                       startPt.x(), startPt.y(), startAngle,
                                       targetPt.x(), targetPt.y(), targetAngle,
                                       DUBINS_START_RADIUS_MM, DUBINS_END_RADIUS_MM) == 0) {
        PRINT_NAMED_INFO("No path found", "Could not generate Dubins path\n");
        return EXIT_FAILURE;
      }

      return EXIT_SUCCESS;
    }
    
    
  } // namespace Cozmo
} // namespace Anki