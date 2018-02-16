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

#include "anki/cozmo/robot/logging.h"

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

  u32 GetCurrentLEDcolor(const LightState& ledParams,
                         const TimeStamp_t currentTime,
                         const TimeStamp_t phaseTime,
                         const u32 msPerFrame)
  {
    AnkiAssert(currentTime >= phaseTime, "LedController.GetCurrentLEDcolor.InvalidPhaseTime");
    
    u32 newColor = 0;
    
    // Check for constant color
    if (ledParams.onFrames == 255 || (ledParams.onColor == ledParams.offColor)) {
      return ledParams.onColor;
    }
    
    const u16 totalFrames = ledParams.transitionOnFrames + ledParams.onFrames + ledParams.transitionOffFrames + ledParams.offFrames;
    
    s32 phaseFrame = (currentTime - phaseTime) / msPerFrame;

    // Apply the offset
    phaseFrame -= ledParams.offset;

    // If it's not time to play yet (because of the offset),
    // just return the off color.
    if (phaseFrame < 0) {
      return ledParams.offColor;
    }
    
    // Modulo to keep phaseFrames in [0, totalFrames)
    phaseFrame %= totalFrames;
    
    if (phaseFrame < ledParams.transitionOnFrames) {
      // In the "on transition" period
      newColor = AlphaBlend(ledParams.onColor, ledParams.offColor, float(phaseFrame)/float(ledParams.transitionOnFrames));
    } else if (phaseFrame < (ledParams.transitionOnFrames + ledParams.onFrames)) {
      // In the "on" period
      newColor = ledParams.onColor;
    } else if (phaseFrame < (ledParams.transitionOnFrames + ledParams.onFrames + ledParams.transitionOffFrames)) {
      // In the "off transition" period
      const u16 offPhase = phaseFrame - (ledParams.transitionOnFrames + ledParams.onFrames);
      newColor = AlphaBlend(ledParams.offColor, ledParams.onColor, float(offPhase)/float(ledParams.transitionOffFrames));
    } else {
      // In the "off" period
      newColor = ledParams.offColor;
    }
    
    return newColor;
    
  } // GetCurrentLEDcolor()

} // namespace Anki
} // namespace Cozmo
