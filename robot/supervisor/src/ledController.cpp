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

#define GET_RED(color) ((color & LED_ENC_RED) >> LED_ENC_RED_SHIFT)
#define GET_GRN(color) ((color & LED_ENC_GRN) >> LED_ENC_GRN_SHIFT)
#define GET_BLU(color) ((color & LED_ENC_BLU) >> LED_ENC_BLU_SHIFT)


namespace Anki {
namespace Cozmo {
  inline u16 AlphaBlend(const u16 onColor, const u16 offColor, const float alpha)
  {
    const float onRed  = GET_RED(onColor);
    const float onGrn  = GET_GRN(onColor);
    const float onBlu  = GET_BLU(onColor);
    const float offRed = GET_RED(offColor);
    const float offGrn = GET_GRN(offColor);
    const float offBlu = GET_BLU(offColor);
    const float invAlpha = 1.0f - alpha;

    return ((u16)int(onRed * alpha + offRed * invAlpha)) << LED_ENC_RED_SHIFT |
           ((u16)int(onGrn * alpha + offGrn * invAlpha)) << LED_ENC_GRN_SHIFT |
           ((u16)int(onBlu * alpha + offBlu * invAlpha)) << LED_ENC_BLU_SHIFT |
           (alpha >= 0.5f ? onColor & LED_ENC_IR : offColor & LED_ENC_IR);
  }

  bool GetCurrentLEDcolor(const LightState& ledParams, const TimeStamp_t currentTime, TimeStamp_t& phaseTime,
                          u16& newColor)
  {
    // Check for constant color
    if (ledParams.onFrames == 255 || (ledParams.onColor == ledParams.offColor)) {
      newColor = ledParams.onColor;
      return true;
    }
    
    u16 phaseFrame = TIMESTAMP_TO_FRAMES(currentTime - phaseTime);
    if (phaseFrame > 1024)
    {
      phaseTime = currentTime; // Someone changed currentTime under us or something else went wrong so reset
      phaseFrame = 0;
    }
    
    if(phaseFrame <= ledParams.onOffset)
    {
      return false;
    }
    else if (phaseFrame <= ledParams.transitionOnFrames + ledParams.onOffset) // Still turning on
    {
      newColor = AlphaBlend(ledParams.onColor, ledParams.offColor, float(phaseFrame - ledParams.onOffset)/float(ledParams.transitionOnFrames));
      return true;
    }
    else if (phaseFrame <= (ledParams.transitionOnFrames + ledParams.onFrames + ledParams.onOffset))
    {
      newColor = ledParams.onColor;
      
      // Return true for the first frame in the onColor to make sure it's set
      return phaseFrame <= ledParams.transitionOnFrames + ledParams.onOffset + 1;
    }
    else if (phaseFrame <= (ledParams.transitionOnFrames + ledParams.onFrames + ledParams.transitionOffFrames + ledParams.onOffset))
    {
      const u16 offPhase = phaseFrame - (ledParams.transitionOnFrames + ledParams.onFrames + ledParams.onOffset);
      newColor = AlphaBlend(ledParams.offColor, ledParams.onColor, float(offPhase)/float(ledParams.transitionOffFrames));
      return true;
    }
    else if (phaseFrame <= (ledParams.transitionOnFrames + ledParams.onFrames + ledParams.transitionOffFrames + ledParams.offFrames + ledParams.onOffset + ledParams.offOffset))
    {
      newColor = ledParams.offColor;
      
      // Return true for the first frame in the offColor to make sure it's set
      return phaseFrame <= (ledParams.transitionOnFrames + ledParams.onFrames + ledParams.transitionOffFrames + ledParams.onOffset) + 1;
    }

    newColor = ledParams.offColor;
    phaseTime = currentTime;
    return true;

  } // GetCurrentLEDcolor()

} // namespace Anki
} // namespace Cozmo
