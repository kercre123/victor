#ifndef COZMO_ENGINE_CONFIG_H
#define COZMO_ENGINE_CONFIG_H

#include "anki/common/constantsAndMacros.h"
#include "anki/common/types.h"
#include "clad/types/imageTypes.h"

namespace Anki {
  namespace Cozmo {
    
    // Camera pose correction (APPLIES TO PHYSICAL ROBOT ONLY)
    // TODO: The headCamPose is eventually to be calibrated
    //       if it can't be manufactured to sufficient tolerance.
    const f32 HEAD_CAM_YAW_CORR = 0.f;
    const f32 HEAD_CAM_PITCH_CORR = 0.f;
    const f32 HEAD_CAM_ROLL_CORR = 0.f;
    const f32 HEAD_CAM_TRANS_X_CORR = 0.f;

    
    // Resolution of images that are streamed to basestation (dev purposes)
    const ImageResolution IMG_STREAM_RES = ImageResolution::QQQVGA;
    
    /***************************************************************************
     *
     *                          Localization
     *
     **************************************************************************/
    
    // Suppresses marker-based localization (FOR PHYSICAL ROBOT ONLY)
    const bool SKIP_PHYS_ROBOT_LOCALIZATION = true;
    
    // Only localize to / identify active objects within this distance
    const f32 MAX_LOCALIZATION_AND_ID_DISTANCE_MM = 250.f;
    
    /***************************************************************************
     *
     *                          Physical Robot Geometry
     *
     **************************************************************************/
    
    
    // Amount to recede the front boundary of the robot when we don't want to include the lift.
    // TEMP: Applied by default to robot bounding box params below mostly for demo purposes
    //       so we don't prematurely delete blocks, but we may eventually want to apply it
    //       conditionally (e.g. when carrying a block)
    const f32 ROBOT_BOUNDING_X_LIFT  = 10.f;
    
    const f32 ROBOT_BOUNDING_X       = 88.f - ROBOT_BOUNDING_X_LIFT; // including gripper fingers
    const f32 ROBOT_BOUNDING_Y       = 54.2f;
    const f32 ROBOT_BOUNDING_X_FRONT = 32.1f - ROBOT_BOUNDING_X_LIFT; // distance from robot origin to front of bounding box
    const f32 ROBOT_BOUNDING_Z       = 67.7f; // from ground to top of head
    const f32 ROBOT_BOUNDING_RADIUS  = sqrtf((0.25f*ROBOT_BOUNDING_X*ROBOT_BOUNDING_X) +
                                             (0.25f*ROBOT_BOUNDING_Y*ROBOT_BOUNDING_Y));
  
    // Apply a conservative negative padding when checking for collisions with known objects to
    // see if they should be deleted..
    // (A negative value means it _really_ has to intersect in order for the object to be deleted.)
    const f32 ROBOT_BBOX_PADDING_FOR_OBJECT_DELETION = -5.f;
    
    /***************************************************************************
     *
     *                          Timing (non-comms)
     *
     **************************************************************************/
    
    // Basestation control loop
    const s32 BS_TIME_STEP = 60;
    
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
    const f32 DEFAULT_MAX_PLANNER_COMPUTATION_TIME_S = 4.f;

    // A different default used for replanning (while we are already following a path)
    const f32 DEFAULT_MAX_PLANNER_REPLAN_COMPUTATION_TIME_S = 1.f;
    
    // Default distance from marker for predock pose
    const f32 DEFAULT_PREDOCK_POSE_DISTANCE_MM = 120.f;
    
    // Maximum difference along Z-axis between robot and predock pose for it
    // to be able to reach predock pose.
    const f32 REACHABLE_PREDOCK_POSE_Z_THRESH_MM = 2*44;

    // When getting a preaction pose for an offset dock, this is the amount by which the
    // preaction pose is offset relative to the specified docking offset. (0 < val < 1)
    const f32 PREACTION_POSE_OFFSET_SCALAR = 1.0f;
    
  } // namespace Cozmo
} // namespace Anki




#endif // COZMO_ENGINE_CONFIG_H
