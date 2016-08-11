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
    // The ordering of the params follows the ordering of the LED_BACKPACK_* enum
    RobotInterface::BackpackLightsTurnSignals  _ledParamsTurnSignals[BPL_NUM_LAYERS];
    RobotInterface::BackpackLightsMiddle _ledParamsMiddle[BPL_NUM_LAYERS];
    
    // Currently animated layer
    BackpackLightLayer _layer;
    
#ifdef SIMULATOR
    TimeStamp_t _ledPhases[NUM_BACKPACK_LEDS];
    
    // The current LED params being played
    const RobotInterface::BackpackLightsTurnSignals* _currParamsTurnSignals = &_ledParamsTurnSignals[BPL_USER];
    const RobotInterface::BackpackLightsMiddle* _currParamsMiddle = &_ledParamsMiddle[BPL_USER];
#endif
    
    bool       _enable;
  };

  void SetParams(const RobotInterface::BackpackLightsTurnSignals& params)
  {
#ifdef SIMULATOR
    _currParamsTurnSignals = &params;
    memset(_ledPhases, 0, sizeof(_ledPhases));
#else
    RobotInterface::SendMessage(params);
#endif
  }

  void SetParams(const RobotInterface::BackpackLightsMiddle& params)
  {
#ifdef SIMULATOR
    _currParamsMiddle = &params;
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
      SetParams(_ledParamsTurnSignals[_layer]);
      SetParams(_ledParamsMiddle[_layer]);
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
    memset(&_ledParamsMiddle[BPL_USER], 0, sizeof(_ledParamsMiddle[BPL_USER]));
    memset(&_ledParamsTurnSignals[BPL_USER], 0, sizeof(_ledParamsTurnSignals[BPL_USER]));
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
    u8 turnCount = 0;
    u8 middleCount = 0;
    for(int i=0; i<NUM_BACKPACK_LEDS; ++i)
    {
      u16 newColor;
      bool colorUpdated = false;
      if(i == LED_BACKPACK_LEFT || i == LED_BACKPACK_RIGHT)
      {
        colorUpdated = GetCurrentLEDcolor(_currParamsTurnSignals->lights[turnCount++], currentTime, _ledPhases[i], newColor);
      }
      else
      {
        colorUpdated = GetCurrentLEDcolor(_currParamsMiddle->lights[middleCount++], currentTime, _ledPhases[i], newColor);
      }

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
    
    LightState* params;
    if(whichLED == LED_BACKPACK_RIGHT || whichLED == LED_BACKPACK_LEFT)
    {
      // LED_BACKPACK_LEFT is the first param and RIGHT is the second
      const u8 i = (whichLED == LED_BACKPACK_LEFT ? 0 : 1);
      params = &_ledParamsTurnSignals[layer].lights[i];
    }
    else
    {
      // Subtract by 1 to shift the LED_BACKPACK_FRONT/MIDDLE/BACK down to the 0-2 range
      params = &_ledParamsMiddle[layer].lights[whichLED-1];
    }

    params->onColor = onColor;
    params->offColor = offColor;
    params->onFrames = onFrames;
    params->offFrames = offFrames;
    params->transitionOnFrames = transitionOnFrames;
    params->transitionOffFrames = transitionOffFrames;
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
