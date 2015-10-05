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


#include "anki/cozmo/robot/ledController.h"

#include <cassert>

namespace Anki {
namespace Cozmo {    
  // Transitions are not allowed to be more than 2 seconds apart
  // This rule (workaround?) unjams the controller when SetTimeStamp changes the time underneath us
  const int MAX_TRANSITION_MS = 2000;
  
  // Alpha blending (w/ black) without using floating point:
  //  See also: http://stackoverflow.com/questions/12011081/alpha-blending-2-rgba-colors-in-c
  inline u32 AlphaBlend(const u32 onColor, const u32 offColor, const u8 alpha)
  {
    u32 result;
    
    const u8* fg = (const u8*)(&onColor);
    const u8* bg = (const u8*)(&offColor);
    u8* result_u8 = (u8*)(&result);
    
    const u32 alpha_u32    = alpha + 1;
    const u32 invAlpha_u32 = 256 - alpha;
    result_u8[3] = (unsigned char)((alpha_u32 * fg[3] + invAlpha_u32 * bg[3]) >> 8);
    result_u8[2] = (unsigned char)((alpha_u32 * fg[2] + invAlpha_u32 * bg[2]) >> 8);
    result_u8[1] = (unsigned char)((alpha_u32 * fg[1] + invAlpha_u32 * bg[1]) >> 8);
    result_u8[0] = 0xff;
    
    return result;
  }

  bool GetCurrentLEDcolor(LEDParams_t& ledParams, TimeStamp_t currentTime,
                          u32& newColor)
  {
    bool colorUpdated = false;

    int timeLeft = ledParams.nextSwitchTime - currentTime;
    if (timeLeft <= 0 || timeLeft >= MAX_TRANSITION_MS)
    {
      switch(ledParams.state)
      {
        case LED_STATE_ON:
          // Check for the special case that LED is just "on" and if so,
          // just stay in this state.
          if(ledParams.offPeriod_ms > 0 ||
             ledParams.transitionOffPeriod_ms > 0 ||
             ledParams.transitionOnPeriod_ms > 0)
          {
            // Time to start turning off
            newColor = ledParams.onColor;
            ledParams.nextSwitchTime = currentTime + ledParams.transitionOffPeriod_ms;
            ledParams.state = LED_STATE_TURNING_OFF;
            colorUpdated = true;
          } else {
            ledParams.nextSwitchTime = currentTime + ledParams.onPeriod_ms;
          }
          break;
          
        case LED_STATE_OFF:
          // Time to start turning on
          newColor = ledParams.offColor;
          ledParams.nextSwitchTime = currentTime + ledParams.transitionOnPeriod_ms;
          ledParams.state = LED_STATE_TURNING_ON;
          colorUpdated = true;
          break;
          
        case LED_STATE_TURNING_ON:
          // Time to be fully on:
          newColor = ledParams.onColor;
          ledParams.nextSwitchTime = currentTime + ledParams.onPeriod_ms;
          ledParams.state = LED_STATE_ON;
          colorUpdated = true;
          break;
          
        case LED_STATE_TURNING_OFF:
          // Time to be fully off
          newColor = ledParams.offColor;
          ledParams.nextSwitchTime = currentTime + ledParams.offPeriod_ms;
          ledParams.state = LED_STATE_OFF;
          colorUpdated = true;
          break;
          
        default:
          // Should never get here
          assert(0);
      }
      
    } else if(ledParams.state == LED_STATE_TURNING_OFF) {
      // Compute alpha b/w 0 and 255 w/ no floating point:
      const u8 alpha = ((ledParams.nextSwitchTime - currentTime)<<8) / ledParams.transitionOffPeriod_ms;
      newColor = AlphaBlend(ledParams.onColor, ledParams.offColor, alpha);
      colorUpdated = true;
    } else if(ledParams.state == LED_STATE_TURNING_ON) {
      // Compute alpha b/w 0 and 255 w/ no floating point:
      const u8 alpha = 255 - ((ledParams.nextSwitchTime - currentTime)<<8) / ledParams.transitionOnPeriod_ms;
      newColor = AlphaBlend(ledParams.onColor, ledParams.offColor, alpha);
      colorUpdated = true;
    } // if(currentTime > nextSwitchTime)

    return colorUpdated;
    
  } // GetCurrentLEDcolor()

} // namespace Anki
} // namespace Cozmo
