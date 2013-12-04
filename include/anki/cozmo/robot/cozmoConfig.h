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

    
#ifdef SIMULATOR
    // Cozmo control loop is 100Hz on simulator. (That's as fast as it goes!)
    const s32 TIME_STEP = 10;
    
    // TODO: Currently set to Webots specs
    const f32 LIFT_JOINT_HEIGHT = 27.f + (0.5f*WHEEL_DIAMETER_MM);
    const f32 LIFT_LENGTH = 97.5f;
    const f32 LIFT_HEIGHT_LOWDOCK  = 5.f;
    const f32 LIFT_HEIGHT_HIGHDOCK = 90.f;
    const f32 LIFT_HEIGHT_CARRY    = 95.f;
#else
    // Control loop on robot is 500Hz.
    const s32 TIME_STEP = 2;
    
    const f32 LIFT_JOINT_HEIGHT = 45.f;
    const f32 LIFT_LENGTH = 91.f;
    const f32 LIFT_HEIGHT_LOWDOCK  = 22.f;
    const f32 LIFT_HEIGHT_HIGHDOCK = 65.f;
    const f32 LIFT_HEIGHT_CARRY    = 70.f;
#endif
    
    const f32 CONTROL_DT = TIME_STEP*0.001f;
    const f32 ONE_OVER_CONTROL_DT = 1.0f/CONTROL_DT;
    
    
    // TODO: convert to using these in degree form?
    const f32 MIN_HEAD_ANGLE = DEG_TO_RAD(-25.f);
    const f32 MAX_HEAD_ANGLE = DEG_TO_RAD( 30.f);
    
    // TODO: make these 3-element f32 arrays. Currently, can't instantiate with those.
#define NECK_JOINT_POSITION {{-15.5f, 0.f, 25.f}} // relative to robot origin
#define HEAD_CAM_POSITION   {{ 25.0f, 0.f, 15.f}} // relative to neck joint
#define LIFT_BASE_POSITION  {{-60.0f, 0.f, 27.f}} // relative to robot origin
#define MAT_CAM_POSITION    {{-25.0f, 0.f, -3.f}} // relative to robot origin
    
#define BLOCKMARKER3D_USE_OUTSIDE_SQUARE false
    
  } // namespace Cozmo
} // namespace Anki


#endif // COZMO_CONFIG_H