#ifndef COZMO_CONFIG_H
#define COZMO_CONFIG_H

#include "anki/common/constantsAndMacros.h"
#include "anki/common/types.h"

#include "anki/cozmo/robot/vehicleMath.h"

namespace Anki {
  namespace Cozmo {

    // Cozmo control loop is 100Hz
    const s32 TIME_STEP = 10;
    
    const f32 CONTROL_DT = TIME_STEP*0.001f;
    const f32 ONE_OVER_CONTROL_DT = 1.0f/CONTROL_DT;
    
    const f32 WHEEL_DIAMETER_MM = 40.f;  // This should be in sync with the CozmoBot proto file
    const f32 HALF_WHEEL_CIRCUM = WHEEL_DIAMETER_MM * M_PI_2;
    const f32 WHEEL_RAD_TO_MM = WHEEL_DIAMETER_MM / 2.f;  // or HALF_WHEEL_CIRCUM / PI;
    const f32 WHEEL_BASE = 50.f; // distance b/w the axles

    const f32 MAT_CAM_HEIGHT_FROM_GROUND_MM = (0.5f*WHEEL_DIAMETER_MM) - 3.f;
    
    // TODO: make these 3-element f32 arrays. Currently, can't instantiate with those.
#define NECK_JOINT_POSITION {{0.f, 9.5f, 25.f}} // relative to robot origin
#define HEAD_CAM_POSITION   {{0.f, 25.f, 15.f}} // relative to neck joint
    
  } // namespace Cozmo
} // namespace Cozmo


#endif // COZMO_CONFIG_H