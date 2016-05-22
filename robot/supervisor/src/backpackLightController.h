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

#include "anki/types.h"
#include "clad/types/ledTypes.h"

namespace Anki {
  namespace Cozmo {
    namespace BackpackLightController {

      Result Init();
      Result Update();

      // Enable/Disable the controller.
      // Mostly only useful for test mode or special simluation modes.
      void Enable();
      void Disable();

      void TurnOffAll();

      void SetParams(const LEDId whichLED, const u16 onColor, const u16 offColor,
                     const u8 onFrames, const u8 offFrames, const u8 transitionOnFrames, const u8 transitionOffFrames);

    } // namespace BackpackLightController
  } // namespace Anki
} // namespace Cozmo
