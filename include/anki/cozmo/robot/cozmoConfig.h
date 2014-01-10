#ifndef COZMO_CONFIG_H
#define COZMO_CONFIG_H

#include "anki/common/constantsAndMacros.h"
#include "anki/common/types.h"
#include "anki/cozmo/robot/debug.h"

namespace Anki {
  namespace Cozmo {

    const f32 WHEEL_DIAMETER_MM = 40.f;  // This should be in sync with the CozmoBot proto file
    const f32 HALF_WHEEL_CIRCUM = WHEEL_DIAMETER_MM * M_PI_2;
    const f32 WHEEL_RAD_TO_MM = WHEEL_DIAMETER_MM / 2.f;  // or HALF_WHEEL_CIRCUM / PI;
    const f32 WHEEL_BASE = 50.f; // distance b/w the axles
    
    const f32 MAT_CAM_HEIGHT_FROM_GROUND_MM = (0.5f*WHEEL_DIAMETER_MM) - 3.f;

    const u8 NUM_RADIAL_DISTORTION_COEFFS = 4;

    // Cozmo control loop is 500Hz.
    const s32 TIME_STEP = 2;
    
#ifdef SIMULATOR
    // Should be less than the timestep of offboard_vision_processing,
    // and roughly reflect image capture time.  If it's larger than or equal
    // to offboard_vision_processing timestep then we may send the same image
    // multiple times.
    const s32 VISION_TIME_STEP = 20;
    
    // Height of main lift joint where the arm attaches to robot body
    const f32 LIFT_JOINT_HEIGHT = 45.f;
    
    // Distance between main lift joint and the joint where arm attaches to fork assembly
    const f32 LIFT_ARM_LENGTH = 95.f;
    
    // Height of the actual fork relative to the joint where the lift arm attaches to the fork assembly
    const f32 LIFT_FORK_HEIGHT_REL_TO_ARM_END = 0.f;
    
    // The height of the fork at various configurations
    const f32 LIFT_HEIGHT_LOWDOCK  = 25.42f;  // Actual limit in proto is closer to 20.4mm, but there is a weird
                                              // issue with moving the lift when it is at a limit. The lift arm
                                              // flies off of the robot and comes back! So for now, we just don't
                                              // drive the lift down that far. We also skip calibration in sim.
    const f32 LIFT_HEIGHT_HIGHDOCK = 100.f;
    const f32 LIFT_HEIGHT_CARRY    = 105.f;
#else

    // Height of main lift joint where the arm attaches to robot body
    const f32 LIFT_JOINT_HEIGHT = 45.f;
    
    // Distance between main lift joint and the joint where arm attaches to fork assembly
    const f32 LIFT_ARM_LENGTH = 91.f;
    
    // Height of the actual fork relative to the joint where the lift arm attaches to the fork assembly
    const f32 LIFT_FORK_HEIGHT_REL_TO_ARM_END = -15.f;
    
    // The height of the fork at various configurations
    const f32 LIFT_HEIGHT_LOWDOCK  = 8.f;
    const f32 LIFT_HEIGHT_HIGHDOCK = 65.f;
    const f32 LIFT_HEIGHT_CARRY    = 70.f;
    
    // TODO: Get these from calibration somehow
    const u16 HEAD_CAM_CALIB_WIDTH  = 320;
    const u16 HEAD_CAM_CALIB_HEIGHT = 240;
    const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 315.6995f;
    const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 316.8701f;
    const f32 HEAD_CAM_CALIB_CENTER_X = 169.6225f;
    const f32 HEAD_CAM_CALIB_CENTER_Y = 119.5692f;
    const f32 HEAD_CAM_CALIB_DISTORTION[NUM_RADIAL_DISTORTION_COEFFS] =
    {0.0265995f, -0.1683574f, -0.0009116f, 0.0061439f};
    
    // TODO: Get real mat camera calibration params: (currently just copies of head cam's)
    const u16 MAT_CAM_CALIB_WIDTH  = 320;
    const u16 MAT_CAM_CALIB_HEIGHT = 240;
    const f32 MAT_CAM_CALIB_FOCAL_LENGTH_X = 315.6995f;
    const f32 MAT_CAM_CALIB_FOCAL_LENGTH_Y = 316.8701f;
    const f32 MAT_CAM_CALIB_CENTER_X = 169.6225f;
    const f32 MAT_CAM_CALIB_CENTER_Y = 119.5692f;
    const f32 MAT_CAM_CALIB_DISTORTION[NUM_RADIAL_DISTORTION_COEFFS] = {0.f, 0.f, 0.f, 0.f};

#endif
    
    const f32 CONTROL_DT = TIME_STEP*0.001f;
    const f32 ONE_OVER_CONTROL_DT = 1.0f/CONTROL_DT;
    
    
    // TODO: convert to using these in degree form?
    const f32 MIN_HEAD_ANGLE = DEG_TO_RAD(-25.f);
    const f32 MAX_HEAD_ANGLE = DEG_TO_RAD( 30.f);
    
    const f32 NECK_JOINT_POSITION[3] = {-15.5f, 0.f, 25.f}; // relative to robot origin
    const f32 HEAD_CAM_POSITION[3]   = { 25.0f, 0.f, 15.f}; // relative to neck joint
    const f32 LIFT_BASE_POSITION[3]  = {-60.0f, 0.f, 27.f}; // relative to robot origin
    const f32 MAT_CAM_POSITION[3]   =  {-25.0f, 0.f, -3.f}; // relative to robot origin
    
    // This is the width of the *outside* of the square fiducial!
    // Note that these don't affect Matlab, meaning offboard vision!
    #define BLOCKMARKER3D_USE_OUTSIDE_SQUARE true
    const f32 BLOCK_MARKER_WIDTH_MM = 32.f;
    
  } // namespace Cozmo
} // namespace Anki


#endif // COZMO_CONFIG_H