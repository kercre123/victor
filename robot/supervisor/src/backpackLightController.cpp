/**
 * File: backpackLightController.cpp
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


#include "backpackLightController.h"
#include "anki/cozmo/robot/logging.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/ledController.h"

#include <string.h>
#include "clad/robotInterface/messageEngineToRobot.h"

namespace Anki {
namespace Cozmo {
namespace BackpackLightController {

  namespace {

    // Light parameters for each layer
    RobotInterface::SetBackpackLights _ledParams[(int)BackpackLightLayer::BPL_NUM_LAYERS];
    
    BackpackLightLayer _layer;                                    // Currently animated layer
    TimeStamp_t        _ledPhases[(int)LEDId::NUM_BACKPACK_LEDS]; // Time phase of current animation
    bool               _enable = true;                            // Whether or not backpack lights are active
  };

  inline void ResetPhases()
  {
    memset(_ledPhases, 0, sizeof(_ledPhases));
  }

  void SetParams(const RobotInterface::SetBackpackLights& params)
  {
    if (params.layer >= (int)BackpackLightLayer::BPL_NUM_LAYERS) {
      AnkiWarn( "BackpackLightController.SetParams.InvalidLayer", "Layer %d is invalid", (int)params.layer);
      return;
    }
    
    memcpy(&(_ledParams[params.layer]), &params, sizeof(params));
    // Reset phases if this is the active layer
    if (_layer == (BackpackLightLayer)params.layer) {
      ResetPhases();
    }
  }

  void TurnOffAll()
  {
    memset(_ledParams, 0, sizeof(_ledParams));
    ResetPhases();
  }

  void EnableLayer(const BackpackLightLayer layer, bool forceUpdate)
  {
    if (layer >= BackpackLightLayer::BPL_NUM_LAYERS) {
      AnkiWarn( "BackpackLightController.EnableLayer.InvalidLayer", "Layer %d is invalid", (int)layer);
      return;
    }

    if (forceUpdate || (_layer != layer)) {
      _layer = layer;
      SetParams(_ledParams[(int)_layer]);
      ResetPhases();
    }
  }
  
  void Enable()
  {
    _enable = true;
  }

  void Disable()
  {
    _enable = false;
  }

  Result Init()
  {
    memset(&_ledParams[(int)BackpackLightLayer::BPL_USER], 0, sizeof(_ledParams[(int)BackpackLightLayer::BPL_USER]));
    return RESULT_OK;
  }

  Result Update()
  {
    if (!_enable) {
      return RESULT_OK;
    }

    TimeStamp_t currentTime = HAL::GetTimeStamp();
    for(int i=0; i<(int)LEDId::NUM_BACKPACK_LEDS; ++i)
    {
      u32 newColor;
      bool colorUpdated = false;
      colorUpdated = GetCurrentLEDcolor(_ledParams[(int)_layer].lights[i], currentTime, _ledPhases[i], newColor);

      if(colorUpdated) {
        HAL::SetLED((HAL::LEDId)i, newColor);
      }
    } // for each LED

    return RESULT_OK;
  }

} // namespace BackpackLightController
} // namespace Anki
} // namespace Cozmo
