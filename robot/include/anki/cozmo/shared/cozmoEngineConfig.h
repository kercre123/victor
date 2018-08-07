#ifndef COZMO_ENGINE_CONFIG_H
#define COZMO_ENGINE_CONFIG_H

#include <math.h>
#include "coretech/common/shared/types.h"
#include "util/math/math.h"
#include "clad/types/imageTypes.h"
#include "clad/types/pathMotionProfile.h"

namespace Anki {
  namespace Vector {
    
    /***************************************************************************
     *
     *                          Vision
     *
     **************************************************************************/
    
    const f32 MAX_MARKER_NORMAL_ANGLE_FOR_SHOULD_BE_VISIBLE_CHECK_RAD = DEG_TO_RAD(45.f);
    const f32 MIN_MARKER_SIZE_FOR_SHOULD_BE_VISIBLE_CHECK_PIX = 40.f;
    
    /***************************************************************************
     *
     *                          Localization
     *
     **************************************************************************/
    
    
    
    /***************************************************************************
     *
     *                          Physical Robot Geometry
     *
     **************************************************************************/
    
    
    // Amount to recede the front boundary of the robot when we don't want to include the lift.
    // TEMP: Applied by default to robot bounding box params below mostly for demo purposes
    //       so we don't prematurely delete blocks, but we may eventually want to apply it
    //       conditionally (e.g. when carrying a block)
    const f32 ROBOT_BOUNDING_X_LIFT  = 19.7f;
    
    const f32 ROBOT_BOUNDING_X       = 100.7f - ROBOT_BOUNDING_X_LIFT; // including gripper fingers
    const f32 ROBOT_BOUNDING_Y       = 60.0f;
    const f32 ROBOT_BOUNDING_X_FRONT = 34.2f - ROBOT_BOUNDING_X_LIFT; // distance from robot origin to front of bounding box
    const f32 ROBOT_BOUNDING_Z       = 69.3f; // from ground to top of head

    // Apply a conservative negative padding when checking for collisions with known objects to
    // see if their pose should be marked as "dirty".
    // (A negative value means it _really_ has to intersect in order for the object to be dirtied.)
    const f32 ROBOT_BBOX_PADDING_FOR_OBJECT_COLLISION = -10.f;

    
    /***************************************************************************
     *
     *                          Tolerances
     *
     **************************************************************************/

    // Angular tolerance on point turns.
    // Mostly used by TurnInPlaceAction, but also used by planner in some places. Maybe planner should keep its own constants?
    const f32 POINT_TURN_ANGLE_TOL = DEG_TO_RAD(2.f);
    
    const f32 READ_TOOL_CODE_LIFT_HEIGHT_TOL_MM = 2.f;
    
    constexpr f32 STACKED_HEIGHT_TOL_MM = 15.f;
    
    constexpr f32 ON_GROUND_HEIGHT_TOL_MM = 10.f; 
    
    /***************************************************************************
     *
     *                          Timing (non-comms)
     *
     **************************************************************************/
    
    // Basestation control loop
    const s32 BS_TIME_STEP_MS = 60;
    const s32 BS_TIME_STEP_MICROSECONDS = (BS_TIME_STEP_MS * 1000);
    
    /***************************************************************************
     *
     *                          Poses and Planner
     *
     **************************************************************************/
    
    // A common distance threshold for pose equality comparison.
    // If two poses are this close to each other, they are considered to be equal
    // (at least in terms of translation).
    const f32 DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM = 10.f;
    
    // A common angle threshold for pose equality comparison
    // If two poses are this close in terms of angle, they are considered equal.
    const f32 DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD = DEG_TO_RAD(10);

    // Default maximum amount of time to let the planner run
    const f32 DEFAULT_MAX_PLANNER_COMPUTATION_TIME_S = 6.f;

    // A different default used for replanning (while we are already following a path)
    const f32 DEFAULT_MAX_PLANNER_REPLAN_COMPUTATION_TIME_S = 1.f;

    // The closest distance to use for docking (in case we are already close to the cube)
    const f32 DEFAULT_MIN_PREDOCK_POSE_DISTANCE_MM = 65.0f;
    
    // Default length of the preDock pose line (the line on which preDock poses can fall)
    const f32 DEFAULT_PREDOCK_POSE_LINE_LENGTH_MM = 75.f;
    
    // Default distance to block for flip preDock pose
    const f32 FLIP_PREDOCK_POSE_DISTAMCE_MM = 80.f;
    
    // Default distances for place relative predock poses
    const f32 PLACE_RELATIVE_MIN_PREDOCK_POSE_DISTANCE_MM = 100.f;
    const f32 PLACE_RELATIVE_PREDOCK_POSE_LINE_LENGTH_MM = 40.f;
    
    // Maximum difference along Z-axis between robot and predock pose for it
    // to be able to reach predock pose.
    const f32 REACHABLE_PREDOCK_POSE_Z_THRESH_MM = 2*44;

    // When getting a preaction pose for an offset dock, this is the amount by which the
    // preaction pose is offset relative to the specified docking offset. (0 < val < 1)
    const f32 PREACTION_POSE_OFFSET_SCALAR = 1.0f;
    
    // Scalar for preAction pose x distance threshold. We really only care if we are aligned in y so
    // the x distance is multiplied by this scalar
    const f32 PREACTION_POSE_X_THRESHOLD_SCALAR = 2.0f;

    // For things like docking, we want to not turn away too much if we can avoid it. This is a threshold in
    // radians. If the starting point is close (in euclidean distance) and also the robot angle is within this
    // threshold of the final goal angle
    const f32 PLANNER_MAINTAIN_ANGLE_THRESHOLD = 0.392699081699f;
  
    // Tolerance on angular alignment with predock pose
    const f32 DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE = DEG_TO_RAD(7.5f);
  
    // Default values in clad
    const PathMotionProfile DEFAULT_PATH_MOTION_PROFILE = PathMotionProfile();
    
  } // namespace Vector
} // namespace Anki




#endif // COZMO_ENGINE_CONFIG_H
