/**
 * File: ledController.cpp
 *
 * Author: Andrew Stein
 * Date:   4/20/15
 *
 * Description: Implements low level (simulated) LED control for color blending,
 *              blinking, and transitions.
 *
 * Copyright: Anki, Inc. 2015
 **/


#include "anki/cozmo/shared/ledTypes.h"

#include <webots/Supervisor.hpp>
#include <cassert>

namespace Anki {
namespace Cozmo {

  // Alpha blending (w/ black) without using floating point:
  //  See also: http://stackoverflow.com/questions/12011081/alpha-blending-2-rgba-colors-in-c
  inline u32 AlphaBlend(const u32 onColor, const u32 offColor, const u8 alpha)
  {
    u32 result;
    
    const u8* fg = (const u8*)(&onColor);
    u8* result_u8 = (u8*)(&result);
    
    const u32 alpha_u32 = alpha + 1;
    
    result_u8[3] = (unsigned char)((alpha_u32 * fg[3]) >> 8);
    result_u8[2] = (unsigned char)((alpha_u32 * fg[2]) >> 8);
    result_u8[1] = (unsigned char)((alpha_u32 * fg[1]) >> 8);
    result_u8[0] = 0xff;
    
    return result;
  }

  bool GetCurrentLEDcolor(LEDParams_t& ledParams, TimeStamp_t currentTime,
                          u32& newColor)
  {
    bool colorUpdated = false;
    
    newColor = ledParams.onColor;

    if(currentTime > ledParams.nextSwitchTime)
    {
      u32 newColor = 0;
      
      switch(ledParams.state)
      {
        case LEDState_t::LED_STATE_ON:
          // Time to start turning off
          ledParams.nextSwitchTime = currentTime + ledParams.transitionOffPeriod_ms;
          
          // Check for the special case that LED is just "on" and if so,
          // just stay in this state.
          if(ledParams.offPeriod_ms > 0 ||
             ledParams.transitionOffPeriod_ms > 0 ||
             ledParams.transitionOnPeriod_ms > 0)
          {
            newColor = ledParams.onColor;
            ledParams.state = LEDState_t::LED_STATE_TURNING_OFF;
            colorUpdated = true;
          }
          break;
          
        case LEDState_t::LED_STATE_OFF:
          // Time to start turning on
          newColor = ledParams.offColor;
          ledParams.nextSwitchTime = currentTime + ledParams.transitionOnPeriod_ms;
          ledParams.state = LEDState_t::LED_STATE_TURNING_ON;
          colorUpdated = true;
          break;
          
        case LEDState_t::LED_STATE_TURNING_ON:
          // Time to be fully on:
          newColor = ledParams.onColor;
          ledParams.nextSwitchTime = currentTime + ledParams.onPeriod_ms;
          ledParams.state = LEDState_t::LED_STATE_ON;
          colorUpdated = true;
          break;
          
        case LEDState_t::LED_STATE_TURNING_OFF:
          // Time to be fully off
          newColor = ledParams.offColor;
          ledParams.nextSwitchTime = currentTime + ledParams.offPeriod_ms;
          ledParams.state = LEDState_t::LED_STATE_OFF;
          colorUpdated = true;
          break;
          
        default:
          // Should never get here
          assert(0);
      }
      
    } else if(ledParams.state == LEDState_t::LED_STATE_TURNING_OFF) {
      // Compute alpha b/w 0 and 255 w/ no floating point:
      const u8 alpha = ((ledParams.nextSwitchTime - currentTime)<<8) / ledParams.transitionOffPeriod_ms;
      newColor = AlphaBlend(ledParams.onColor, ledParams.offColor, alpha);
      colorUpdated = true;
    } else if(ledParams.state == LEDState_t::LED_STATE_TURNING_ON) {
      // Compute alpha b/w 0 and 255 w/ no floating point:
      const u8 alpha = 255 - ((ledParams.nextSwitchTime - currentTime)<<8) / ledParams.transitionOnPeriod_ms;
      newColor = AlphaBlend(ledParams.onColor, ledParams.offColor, alpha);
      colorUpdated = true;
    } // if(currentTime > nextSwitchTime)

    return colorUpdated;
    
  } // GetCurrentLEDcolor()

} // namespace Anki
} // namespace Cozmo
