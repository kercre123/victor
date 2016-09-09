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

      enum BackpackLightLayer {
        BPL_USER,    // User-defined lights from engine via SetBackpackLights
        BPL_ANIM,    // Animation-defined lights from Espressif via AnimKeyFrame::BackpackLights
        BPL_NUM_LAYERS
      };
      
      Result Init();
      Result Update();

      // Enable/Disable the controller.
      // Mostly only useful for test mode or special simluation modes.
      void Enable();
      void Disable();

      void TurnOffAll();

      // Specify which layer should be active
      // If forceUpdate == false, then an updated light command is sent to body
      // only if the current layer is not the given layer.
      // If forceUpdate == true, then an updated light command is sent to body no matter what.
      void EnableLayer(const BackpackLightLayer layer, bool forceUpdate = false);
      
      // Set the parameters of an LED for a specified layer.
      // EnableLayer() must be called to actually apply changes.
      void SetParams(const BackpackLightLayer layer, const LEDId whichLED, const u16 onColor, const u16 offColor,
                     const u8 onFrames, const u8 offFrames, const u8 transitionOnFrames, const u8 transitionOffFrames,
                     const s16 offset);

    } // namespace BackpackLightController
  } // namespace Anki
} // namespace Cozmo
