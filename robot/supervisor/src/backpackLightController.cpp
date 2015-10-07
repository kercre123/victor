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
    
    //LEDId       _ledLUT[NUM_LEDS];
    
    LightState _ledParams[NUM_BACKPACK_LEDS];
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
    /*
    _ledLUT[0] = LEDId::LED_BACKPACK_LEFT;
    _ledLUT[1] = LEDId::LED_BACKPACK_RIGHT;
    _ledLUT[2] = LEDId::LED_BACKPACK_BACK;
    _ledLUT[3] = LEDId::LED_BACKPACK_FRONT;
    _ledLUT[4] = LEDId::LED_BACKPACK_MIDDLE;
    */
    
    for(s32 i=0; i<NUM_BACKPACK_LEDS; ++i) {
      _ledParams[i].onColor = 0;
      _ledParams[i].offColor = 0;
      _ledParams[i].nextSwitchTime = u32_MAX;
    }
    _enable = true;
    
    return RESULT_OK;
  }
  
  Result Update()
  {
    if (!_enable) {
      return RESULT_OK;
    }
    
    TimeStamp_t currentTime = HAL::GetTimeStamp();
    
    for(int i=0; i<NUM_BACKPACK_LEDS; ++i)
    {
      u32 newColor;
      const bool colorUpdated = GetCurrentLEDcolor(_ledParams[i], currentTime, newColor);
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
