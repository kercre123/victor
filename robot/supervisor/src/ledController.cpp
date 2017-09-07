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

#define GET_RED(color) ((color & EnumToUnderlyingType(LEDColor::LED_RED)) >> EnumToUnderlyingType(LEDColorShift::LED_RED_SHIFT))
#define GET_GRN(color) ((color & EnumToUnderlyingType(LEDColor::LED_GREEN)) >> EnumToUnderlyingType(LEDColorShift::LED_GRN_SHIFT))
#define GET_BLU(color) ((color & EnumToUnderlyingType(LEDColor::LED_BLUE)) >> EnumToUnderlyingType(LEDColorShift::LED_BLU_SHIFT))


namespace Anki {
namespace Cozmo {
  inline u32 AlphaBlend(const u32 onColor, const u32 offColor, const float alpha)
  {
    const float onRed  = GET_RED(onColor);
    const float onGrn  = GET_GRN(onColor);
    const float onBlu  = GET_BLU(onColor);
    const float offRed = GET_RED(offColor);
    const float offGrn = GET_GRN(offColor);
    const float offBlu = GET_BLU(offColor);
    const float invAlpha = 1.0f - alpha;

    return (int(onRed * alpha + offRed * invAlpha)) << EnumToUnderlyingType(LEDColorShift::LED_RED_SHIFT) |
           (int(onGrn * alpha + offGrn * invAlpha)) << EnumToUnderlyingType(LEDColorShift::LED_GRN_SHIFT) |
           (int(onBlu * alpha + offBlu * invAlpha)) << EnumToUnderlyingType(LEDColorShift::LED_BLU_SHIFT);
  }

  bool GetCurrentLEDcolor(const LightState& ledParams, const TimeStamp_t currentTime, TimeStamp_t& phaseTime,
                          u32& newColor)
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
    
    if(phaseFrame <= ledParams.offset)
    {
      return false;
    }
    else if (phaseFrame <= ledParams.transitionOnFrames + ledParams.offset) // Still turning on
    {
      newColor = AlphaBlend(ledParams.onColor, ledParams.offColor, float(phaseFrame - ledParams.offset)/float(ledParams.transitionOnFrames));
      return true;
    }
    else if (phaseFrame <= (ledParams.transitionOnFrames + ledParams.onFrames + ledParams.offset))
    {
      newColor = ledParams.onColor;
      
      // Return true for the first frame in the onColor to make sure it's set
      return phaseFrame <= ledParams.transitionOnFrames + ledParams.offset + 1;
    }
    else if (phaseFrame <= (ledParams.transitionOnFrames + ledParams.onFrames + ledParams.transitionOffFrames + ledParams.offset))
    {
      const u16 offPhase = phaseFrame - (ledParams.transitionOnFrames + ledParams.onFrames + ledParams.offset);
      newColor = AlphaBlend(ledParams.offColor, ledParams.onColor, float(offPhase)/float(ledParams.transitionOffFrames));
      return true;
    }
    else if (phaseFrame <= (ledParams.transitionOnFrames + ledParams.onFrames + ledParams.transitionOffFrames + ledParams.offFrames + ledParams.offset))
    {
      newColor = ledParams.offColor;
      
      // Return true for the first frame in the offColor to make sure it's set
      return phaseFrame <= (ledParams.transitionOnFrames + ledParams.onFrames + ledParams.transitionOffFrames + ledParams.offset) + 1;
    }

    newColor = ledParams.offColor;
    phaseTime = currentTime;
    return true;

  } // GetCurrentLEDcolor()

} // namespace Anki
} // namespace Cozmo
