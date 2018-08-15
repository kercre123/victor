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

#include "coretech/common/shared/types.h"
#include "clad/types/ledTypes.h"

namespace Anki {
  namespace Vector {
    
    namespace RobotInterface {
      struct SetBackpackLights;
      struct SetSystemLight;
    }
    
    namespace BackpackLightController {
      
      Result Init();
      Result Update();

      // Enable/Disable the controller.
      // Mostly only useful for test mode or special simluation modes.
      void Enable();
      void Disable();

      // Specify which layer should be active
      // If forceUpdate == false, then an updated light command is sent to body
      // only if the current layer is not the given layer.
      // If forceUpdate == true, then an updated light command is sent to body no matter what.
      void EnableLayer(const BackpackLightLayer layer, bool forceUpdate = false);
      
      // Set the parameters of an LED for a specified layer.
      // EnableLayer() must be called to actually apply changes if this is for the non-active layer.
      void SetParams(const RobotInterface::SetBackpackLights& params);
      void SetParams(const RobotInterface::SetSystemLight& params);

      // Set all lights on all layers to off
      void TurnOffAll();
      
    } // namespace BackpackLightController
  } // namespace Anki
} // namespace Vector
