#ifndef COZMO_CONFIG_H
#define COZMO_CONFIG_H

#include "anki/common/constantsAndMacros.h"
#include "anki/common/types.h"

#include "anki/cozmo/robot/vehicleMath.h"

namespace Anki {
  namespace Cozmo {

    // Cozmo control loop is 100Hz
    const s32 TIME_STEP = 10;
    
    const f32 CONTROL_DT = TIME_STEP*0.001;
    const f32 ONE_OVER_CONTROL_DT = 1.0/CONTROL_DT;
    
    const f32 WHEEL_DIAMETER_MM = 30;  // This should be in sync with the CozmoBot proto file
    const f32 HALF_WHEEL_CIRCUM = WHEEL_DIAMETER_MM * M_PI_2;
    const f32 WHEEL_RAD_TO_MM = WHEEL_DIAMETER_MM / 2;  // or HALF_WHEEL_CIRCUM / PI;

    
  } // namespace Cozmo
} // namespace Cozmo


#endif // COZMO_CONFIG_H