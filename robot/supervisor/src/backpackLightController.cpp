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
#include "anki/cozmo/robot/ledController.h"

namespace Anki {
namespace Cozmo {
namespace BackpackLightController {
  
  namespace {
    
    // User-defined light params
    LightState  _ledParams[NUM_BACKPACK_LEDS];
    
    // Light params when charging
    LightState  _chargingParams[NUM_BACKPACK_LEDS];

    // Light when charged
    LightState  _chargedParams[NUM_BACKPACK_LEDS];

    // Voltage at which it is considered charged
    // TODO: Should there be a HAL::IsFastCharging()?
    const u8 CHARGED_BATT_VOLTAGE_10x = 46;
    
    // Number of cycles it must be detected as charged or not charged before it switches over
    const s32 LIGHT_TRANSITION_CYCLE_COUNT_THRESHOLD = 200;
    s32 _isChargedCount = 0;
    
    // The current LED params being played
    LightState* _currParams = _ledParams;
    
    bool       _enable;
  };
  
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
    for(s32 i=0; i<NUM_BACKPACK_LEDS; ++i) {
      _ledParams[i].onColor = 0;
      _ledParams[i].offColor = 0;
      _ledParams[i].nextSwitchTime = u32_MAX;
    }
    _enable = true;
    
    
    // Set charging light params
    LightState *p = &_chargingParams[LED_BACKPACK_BACK];
    p->onColor = 0x00ff0000;
    p->offColor = 0x0;
    p->onPeriod_ms = 1000;
    p->offPeriod_ms = 200;
    p->transitionOnPeriod_ms = 200;
    p->transitionOffPeriod_ms = 200;
    p->state = LED_STATE_OFF;
    
    _chargingParams[LED_BACKPACK_MIDDLE] = _chargingParams[LED_BACKPACK_BACK];
    p = &_chargingParams[LED_BACKPACK_MIDDLE];
    p->onPeriod_ms = 600;
    p->offPeriod_ms = 200;
    p->transitionOnPeriod_ms = 600;

    _chargingParams[LED_BACKPACK_FRONT] = _chargingParams[LED_BACKPACK_BACK];
    p = &_chargingParams[LED_BACKPACK_FRONT];
    p->onPeriod_ms = 200;
    p->offPeriod_ms = 200;
    p->transitionOnPeriod_ms = 1000;

    
    // Set charged light params
    p = &_chargedParams[LED_BACKPACK_BACK];
    p->onColor = 0x00ff0000;
    p->offColor = 0x00660000;
    p->onPeriod_ms = 1000;
    p->offPeriod_ms = 10;
    p->transitionOnPeriod_ms = 1000;
    p->transitionOffPeriod_ms = 1000;
    p->state = LED_STATE_OFF;
    
    _chargedParams[LED_BACKPACK_MIDDLE] = _chargedParams[LED_BACKPACK_BACK];
    _chargedParams[LED_BACKPACK_FRONT] = _chargedParams[LED_BACKPACK_BACK];
    
    return RESULT_OK;
  }
  
  
  // Assumes p is a pointer to a set of backpack LightStates
  // and turns them all off.
  void SetAllLightsOff(LightState* p)
  {
    for (u8 i = 0; i < NUM_BACKPACK_LEDS; ++i) {
      p[i].state = LED_STATE_OFF;
    }
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
      if ((_currParams != _chargingParams) && _isChargedCount <= 0)
      {
        // Reset current lights and switch to charging lights
        SetAllLightsOff(_currParams);
        _currParams = _chargingParams;
      }
      else if ((_currParams != _chargedParams) && _isChargedCount > 0)
      {
        // Reset current lights and switch to charged lights
        SetAllLightsOff(_currParams);
        _currParams = _chargedParams;
      }
    }
    else if (!HAL::BatteryIsOnCharger() && _currParams != _ledParams)
    {
      // Reset current lights and switch to user-defined lights
      SetAllLightsOff(_currParams);
      _currParams = _ledParams;
    }
  }
  
  Result Update()
  {
    if (!_enable) {
      return RESULT_OK;
    }
    
    SetCurrParams();
    
    TimeStamp_t currentTime = HAL::GetTimeStamp();
    
    for(int i=0; i<NUM_BACKPACK_LEDS; ++i)
    {
      u32 newColor;
      const bool colorUpdated = GetCurrentLEDcolor(_currParams[i], currentTime, newColor);
      if(colorUpdated) {
        HAL::SetLED((LEDId)i, (newColor>>8)); // Shift color to remove alpha bits
      }
    } // for each LED

    
    return RESULT_OK;
  }
  
  void SetParams(LEDId whichLED, u32 onColor, u32 offColor,
                 u32 onPeriod_ms, u32 offPeriod_ms,
                 u32 transitionOnPeriod_ms, u32 transitionOffPeriod_ms)
  {
    LightState& params = _ledParams[whichLED];
    
    params.onColor = onColor;
    params.offColor = offColor;
    params.onPeriod_ms = onPeriod_ms;
    params.offPeriod_ms = offPeriod_ms;
    params.transitionOnPeriod_ms = transitionOnPeriod_ms;
    params.transitionOffPeriod_ms = transitionOffPeriod_ms;
    params.state = LED_STATE_OFF;
    params.nextSwitchTime = 0;
  }
  
  void TurnOffAll() {
    for (u8 i = 0; i <NUM_BACKPACK_LEDS; ++i) {
      HAL::SetLED((LEDId)i,0);
    }
  }
  
} // namespace BackpackLightController
} // namespace Anki
} // namespace Cozmo
