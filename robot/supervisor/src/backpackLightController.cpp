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

#include "anki/cozmo/robot/hal.h"
#ifdef SIMULATOR
#include "anki/cozmo/robot/ledController.h"
#endif
#include <string.h>
#include "clad/robotInterface/messageEngineToRobot.h"


#define MS_TO_LED_FRAMES(ms) ((ms+29)/30)

namespace Anki {
namespace Cozmo {
namespace BackpackLightController {

  namespace {

    // User-defined light params
    RobotInterface::BackpackLights  _ledParams;
#ifdef SIMULATOR
    TimeStamp_t _ledPhases[NUM_BACKPACK_LEDS];
#endif
    
    // Light params when off charger
    const RobotInterface::BackpackLights _defaultOffChargerParams = { .lights = {
      {0, 0, 0, 0, 0, 0}, // LED_BACKPACK_LEFT
      {0x03e0, 0x0200, 10, 10, 50, 50}, // LED_BACKPACK_FRONT
      {0, 0, 0, 0, 0, 0}, // LED_BACKPACK_MIDDLE
      {0, 0, 0, 0, 0, 0}, // LED_BACKPACK_BACK
      {0, 0, 0, 0, 0, 0}  // LED_BACKPACK_BACK
    }};

    // Light params when charging
    const RobotInterface::BackpackLights _chargingParams = { .lights = {
      {0, 0, 0, 0, 0, 0}, // LED_BACKPACK_LEFT
      {0x03e0, 0x0180, 20, 6, 20, 6}, // LED_BACKPACK_FRONT
      {0x03e0, 0x0180, 20, 6, 20, 6}, // LED_BACKPACK_MIDDLE
      {0x03e0, 0x0180, 20, 6, 20, 6}, // LED_BACKPACK_BACK
      {0, 0, 0, 0, 0, 0} // LED_BACKPACK_RIGHT
    }};

    // Light when charged
    const RobotInterface::BackpackLights _chargedParams = { .lights = {
      {0, 0, 0, 0, 0, 0}, // LED_BACKPACK_LEFT
      {0x03e0, 0x0180, 33, 1, 33, 33}, // LED_BACKPACK_FRONT
      {0x03e0, 0x0180, 33, 1, 33, 33}, // LED_BACKPACK_MIDDLE
      {0x03e0, 0x0180, 33, 1, 33, 33}, // LED_BACKPACK_BACK
      {0, 0, 0, 0, 0, 0} // LED_BACKPACK_RIGHT
    }};

    // Voltage at which it is considered charged
    // TODO: Should there be a HAL::IsFastCharging()?
    const u8 CHARGED_BATT_VOLTAGE_10x = 46;

    // Number of cycles it must be detected as charged or not charged before it switches over
    const s32 LIGHT_TRANSITION_CYCLE_COUNT_THRESHOLD = 200;
    s32 _isChargedCount = 0;

    // The current LED params being played
    const RobotInterface::BackpackLights* _currParams = &_ledParams;

    bool       _enable;
  };

  void SetParams(const RobotInterface::BackpackLights& params)
  {
    _currParams = &params;
#ifdef SIMULATOR
    memset(_ledPhases, 0, sizeof(_ledPhases));
#else
    RobotInterface::SendMessage(params);
#endif
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
    memcpy(&_ledParams, &_defaultOffChargerParams, sizeof(_ledParams));
    _enable = true;

    return RESULT_OK;
  }

  // Set which light parameters are to be displayed based on onCharger status.
  // NOTE: While on charger, backpack lights can be set, but they won't show
  //       until the robot comes off the charger. This may or may not be what we want.
  void SetCurrParams()
  {
    bool isCharged = HAL::BatteryGetVoltage10x() >= CHARGED_BATT_VOLTAGE_10x;

    // Hysteresis on charging light pattern
    if (HAL::BatteryIsOnCharger()) {
      if (isCharged) {
        if (_isChargedCount < LIGHT_TRANSITION_CYCLE_COUNT_THRESHOLD ) {
          ++_isChargedCount;
        }
      } else {
        if (_isChargedCount > -LIGHT_TRANSITION_CYCLE_COUNT_THRESHOLD) {
          --_isChargedCount;
        }
      }
    } else {
      _isChargedCount = 0;
    }

    if (HAL::BatteryIsOnCharger()) {
      if ((_currParams != &_chargingParams) && _isChargedCount <= 0)
      {
        // Reset current lights and switch to charging lights
        SetParams(_chargingParams);
      }
      else if ((_currParams != &_chargedParams) && _isChargedCount > 0)
      {
        // Reset current lights and switch to charged lights
        SetParams(_chargedParams);
      }
    }
    else if (!HAL::BatteryIsOnCharger() && _currParams != &_ledParams)
    {
      // Reset current lights and switch to user-defined lights
      SetParams(_ledParams);
    }
  }

  Result Update()
  {
    if (!_enable) {
      return RESULT_OK;
    }

    SetCurrParams();

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

  void SetParams(const LEDId whichLED, const u16 onColor, const u16 offColor,
                 const u8 onFrames, const u8 offFrames, const u8 transitionOnFrames, const u8 transitionOffFrames)
  {
    LightState& params = _ledParams.lights[whichLED];

    params.onColor = onColor;
    params.offColor = offColor;
    params.onFrames = onFrames;
    params.offFrames = offFrames;
    params.transitionOnFrames = transitionOnFrames;
    params.transitionOffFrames = transitionOffFrames;
#ifdef SIMULATOR
    _ledPhases[whichLED] = HAL::GetTimeStamp();
#else
    if (_currParams == &_ledParams)
    {
      SetParams(_ledParams);
    }
#endif
  }

  void TurnOffAll() {
#ifdef SIMULATOR
    for (u8 i = 0; i <NUM_BACKPACK_LEDS; ++i) {
      HAL::SetLED((LEDId)i,0);
    }
#else
    memset(&_ledParams, 0, sizeof(_ledParams));
    SetParams(_ledParams);
#endif
  }

} // namespace BackpackLightController
} // namespace Anki
} // namespace Cozmo
