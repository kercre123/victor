#ifndef COZMO_ENGINE_CONFIG_H
#define COZMO_ENGINE_CONFIG_H

#include "anki/common/constantsAndMacros.h"
#include "anki/common/types.h"
#include "anki/vision/CameraSettings.h"

namespace Anki {
  namespace Cozmo {
   
    // Suppresses marker-based localization (FOR PHYSICAL ROBOT ONLY)
    const bool SKIP_PHYS_ROBOT_LOCALIZATION = false;
    
    
    // Camera pose correction (APPLIES TO PHYSICAL ROBOT ONLY)
    // TODO: The headCamPose is eventually to be calibrated
    //       if it can't be manufactured to sufficient tolerance.
#if(1)
    // Cozmo v4 #1
    const f32 HEAD_CAM_YAW_CORR = DEG_TO_RAD_F32(0.f);
    const f32 HEAD_CAM_PITCH_CORR = DEG_TO_RAD_F32(0.f);
    const f32 HEAD_CAM_ROLL_CORR = DEG_TO_RAD_F32(0.f);
    const f32 HEAD_CAM_TRANS_X_CORR = 0.f;

    
    // ===  Magic numbers ===
    
    // Robot seems to consistently dock to the right
    const f32 DOCKING_LATERAL_OFFSET_HACK = 0.f;
#elif(0)
    // Cozmo v3.2 #1
    const f32 HEAD_CAM_YAW_CORR = DEG_TO_RAD_F32(2.f);
    const f32 HEAD_CAM_PITCH_CORR = DEG_TO_RAD_F32(-1.5f);
    const f32 HEAD_CAM_ROLL_CORR = DEG_TO_RAD_F32(0.f);
    const f32 HEAD_CAM_TRANS_X_CORR = 2.f;

    
    // ===  Magic numbers ===
    
    // Robot seems to consistently dock to the right
    const f32 DOCKING_LATERAL_OFFSET_HACK = 2.f;
#else
    const f32 HEAD_CAM_YAW_CORR = 0.f;
    const f32 HEAD_CAM_PITCH_CORR = 0.f;
    const f32 HEAD_CAM_ROLL_CORR = 0.f;
    const f32 HEAD_CAM_TRANS_X_CORR = 0.f;
#endif

    
    // Resolution of images that are streamed to basestation (dev purposes)
    const Vision::CameraResolution IMG_STREAM_RES = Vision::CAMERA_RES_QQQVGA;
    
    
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
    
    // Default distance from marker for predock pose
    const f32 DEFAULT_PREDOCK_POSE_DISTANCE_MM = 120.f;
    
    // Maximum difference along Z-axis between robot and predock pose for it
    // to be able to reach predock pose.
    const f32 REACHABLE_PREDOCK_POSE_Z_THRESH_MM = 2*44;

    
  } // namespace Cozmo
} // namespace Anki




#endif // COZMO_ENGINE_CONFIG_H
