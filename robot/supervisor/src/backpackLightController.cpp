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

#ifdef USES_CPPLITE
#define CLAD_VECTOR(ns) CppLite::Anki::Vector::ns
#else
#define CLAD_VECTOR(ns) ns
#endif

namespace Anki {
namespace Vector {
namespace BackpackLightController {

  namespace {

    // Light parameters for each layer
    RobotInterface::SetBackpackLights _ledParams[(int)CLAD_VECTOR(BackpackLightLayer)::BPL_NUM_LAYERS];

    RobotInterface::SetSystemLight _sysLedParams;
    
    CLAD_VECTOR(BackpackLightLayer) _layer;                                    // Currently animated layer
    TimeStamp_t        _ledPhases[(int)CLAD_VECTOR(LEDId)::NUM_BACKPACK_LEDS]; // Time phase of current animation
    TimeStamp_t        _sysLedPhase;
    bool               _enable = true;                            // Whether or not backpack lights are active
  };

  inline void ResetPhases()
  {
    const TimeStamp_t currentTime = HAL::GetTimeStamp();
    for(int i=0; i<(int)CLAD_VECTOR(LEDId)::NUM_BACKPACK_LEDS; ++i)
    {
      _ledPhases[i] = currentTime;
    }
  }

  void SetParams(const RobotInterface::SetBackpackLights& params)
  {
    if (params.layer >= (int)CLAD_VECTOR(BackpackLightLayer)::BPL_NUM_LAYERS) {
      AnkiWarn( "BackpackLightController.SetParams.InvalidLayer", "Layer %d is invalid", (int)params.layer);
      return;
    }
    
    memcpy(&(_ledParams[params.layer]), &params, sizeof(params));
    // Reset phases if this is the active layer
    if (_layer == (CLAD_VECTOR(BackpackLightLayer))params.layer) {
      ResetPhases();
    }
  }

  void SetParams(const RobotInterface::SetSystemLight& params)
  {
    memcpy(&_sysLedParams, &params, sizeof(params));
    _sysLedPhase = HAL::GetTimeStamp();
  }

  void TurnOffAll()
  {
    memset(_ledParams, 0, sizeof(_ledParams));
    memset(&_sysLedParams, 0, sizeof(_sysLedParams));
    ResetPhases();
    // Note: this is not done in ResetPhases() otherwise
    // the system light phase could be interrupted when setting
    // the other backpack lights
    _sysLedPhase = HAL::GetTimeStamp();
  }

  void EnableLayer(const CLAD_VECTOR(BackpackLightLayer) layer, bool forceUpdate)
  {
    if (layer >= CLAD_VECTOR(BackpackLightLayer)::BPL_NUM_LAYERS) {
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
    memset(&_ledParams[(int)CLAD_VECTOR(BackpackLightLayer)::BPL_USER], 0, sizeof(_ledParams[(int)CLAD_VECTOR(BackpackLightLayer)::BPL_USER]));

    // Climbing white lights on init (will be stopped by engine once connected)
    const u16 kTimeDiff_ms = 600;
    for(u8 i = 0; i < (u8)CLAD_VECTOR(LEDId)::NUM_BACKPACK_LEDS; i++)
    {
      _ledParams[(int)CLAD_VECTOR(BackpackLightLayer)::BPL_USER].lights[i] = {
        .onColor = 0x80808000,
        .offColor = 0,
        .onPeriod_ms = static_cast<u16>(kTimeDiff_ms * (i+1)),
        .offPeriod_ms = static_cast<u16>((kTimeDiff_ms * ((u8)CLAD_VECTOR(LEDId)::NUM_BACKPACK_LEDS - 1 - i))),
        .transitionOnPeriod_ms = 300,
        .transitionOffPeriod_ms = 300,
        .offset_ms = static_cast<s16>((kTimeDiff_ms * ((u8)CLAD_VECTOR(LEDId)::NUM_BACKPACK_LEDS - 1 - i)))
      };
    }

    EnableLayer(CLAD_VECTOR(BackpackLightLayer)::BPL_USER, true);
    
    return RESULT_OK;
  }

  Result Update()
  {
    if (!_enable) {
      return RESULT_OK;
    }

    TimeStamp_t currentTime = HAL::GetTimeStamp();
    for(int i=0; i<(int)CLAD_VECTOR(LEDId)::NUM_BACKPACK_LEDS; ++i)
    {
      const u32 newColor = GetCurrentLEDcolor(_ledParams[(int)_layer].lights[i], currentTime, _ledPhases[i]);
      HAL::SetLED((HAL::LEDId)i, newColor);
    } // for each LED

    const u32 newColor = GetCurrentLEDcolor(_sysLedParams.light, currentTime, _sysLedPhase);
    HAL::SetSystemLED(newColor);

    return RESULT_OK;
  }

} // namespace BackpackLightController
} // namespace Anki
} // namespace Vector
