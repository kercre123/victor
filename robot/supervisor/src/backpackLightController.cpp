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
#ifdef SIMULATOR
#include "anki/cozmo/robot/ledController.h"
#endif
#include <string.h>
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"


#define MS_TO_LED_FRAMES(ms) ((ms+29)/30)

namespace Anki {
namespace Cozmo {
namespace BackpackLightController {

  namespace {

    // User-defined light params
    RobotInterface::BackpackLights  _ledParams[BPL_NUM_LAYERS];
    
    // Currently animated layer
    BackpackLightLayer _layer;
    
#ifdef SIMULATOR
    TimeStamp_t _ledPhases[NUM_BACKPACK_LEDS];
    
    // The current LED params being played
    const RobotInterface::BackpackLights* _currParams = &_ledParams[BPL_USER];
#endif
    
    bool       _enable;
  };

  void SetParams(const RobotInterface::BackpackLights& params)
  {
#ifdef SIMULATOR
    _currParams = &params;
    memset(_ledPhases, 0, sizeof(_ledPhases));
#else
    RobotInterface::SendMessage(params);
#endif
  }

  void EnableLayer(const BackpackLightLayer layer, bool forceUpdate)
  {
    if (layer >= BPL_NUM_LAYERS) {
      AnkiWarn( 194, "BackpackLightController.EnableLayer.InvalidLayer", 498, "Layer %d is invalid", 1, layer);
      return;
    }

    if (forceUpdate || (_layer != layer)) {
      _layer = layer;
      SetParams(_ledParams[_layer]);
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
    memset(&_ledParams[BPL_USER], 0, sizeof(_ledParams[BPL_USER]));
    _enable = true;

    return RESULT_OK;
  }

  Result Update()
  {
    if (!_enable) {
      return RESULT_OK;
    }

#ifdef SIMULATOR
    TimeStamp_t currentTime = HAL::GetTimeStamp();
    for(int i=0; i<NUM_BACKPACK_LEDS; ++i)
    {
      u16 newColor;
      const bool colorUpdated = GetCurrentLEDcolor(_currParams->lights[i], currentTime, _ledPhases[i], newColor);
      if(colorUpdated) {
        HAL::SetLED((LEDId)i, newColor);
      }
    } // for each LED
#endif

    return RESULT_OK;
  }

  void SetParams(const BackpackLightLayer layer, const LEDId whichLED, const u16 onColor, const u16 offColor,
                 const u8 onFrames, const u8 offFrames, const u8 transitionOnFrames, const u8 transitionOffFrames)
  {
    if (layer >= BPL_NUM_LAYERS) {
      AnkiWarn( 195, "BackpackLightController.SetParams.InvalidLayer", 498, "Layer %d is invalid", 1, layer);
      return;
    }
    
    LightState& params = _ledParams[layer].lights[whichLED];

    params.onColor = onColor;
    params.offColor = offColor;
    params.onFrames = onFrames;
    params.offFrames = offFrames;
    params.transitionOnFrames = transitionOnFrames;
    params.transitionOffFrames = transitionOffFrames;
#ifdef SIMULATOR
    _ledPhases[whichLED] = HAL::GetTimeStamp();
#endif
  }

  void TurnOffAll() {
#ifdef SIMULATOR
    for (u8 i = 0; i <NUM_BACKPACK_LEDS; ++i) {
      HAL::SetLED((LEDId)i,0);
    }
#else
    for(int i=0; i<NUM_BACKPACK_LEDS; ++i) {
      SetParams(BPL_USER, i, 0, 0, 0, 0, 0, 0);
    }
    EnableLayer(BPL_USER, true);
#endif
  }

} // namespace BackpackLightController
} // namespace Anki
} // namespace Cozmo
