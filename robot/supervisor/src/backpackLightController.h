/**
 * File: backpackLightController.h
 *
 * Author: Andrew Stein
 * Created: 4/20/2015
 *
 * Description:
 *
 *   Controller for the LEDs on the backpack (i.e. the "health" or "directional" indicator).
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/shared/ledTypes.h"

namespace Anki {
  namespace Cozmo {
    namespace BackpackLightController {
      
      Result Init();
      Result Update();
      
      void TurnOffAll();
      
      void SetParams(LEDId whichLED, u32 onColor, u32 offColor,
                     u32 onPeriod_ms, u32 offPeriod_ms,
                     u32 transitionOnPeriod_ms, u32 transitionOffPeriod_ms);
      
    } // namespace BackpackLightController
  } // namespace Anki
} // namespace Cozmo
