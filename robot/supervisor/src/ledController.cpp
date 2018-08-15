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
namespace Vector {
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
    if (ledParams.onPeriod_ms == std::numeric_limits<u16>::max() ||
        (ledParams.onColor == ledParams.offColor)) {
      return ledParams.onColor;
    }
    
    const u32 totalTime_ms = ledParams.transitionOnPeriod_ms +
                             ledParams.onPeriod_ms +
                             ledParams.transitionOffPeriod_ms +
                             ledParams.offPeriod_ms;
    
    s32 phaseTime_ms = (currentTime - phaseTime);

    // Apply the offset
    phaseTime_ms -= ledParams.offset_ms;

    // If it's not time to play yet (because of the offset),
    // just return the off color.
    if (phaseTime_ms < 0) {
      return ledParams.offColor;
    }
    
    // Modulo to keep phaseFrames in [0, totalFrames)
    phaseTime_ms %= totalTime_ms;
    
    if (phaseTime_ms < ledParams.transitionOnPeriod_ms) {
      // In the "on transition" period
      newColor = AlphaBlend(ledParams.onColor, ledParams.offColor, float(phaseTime_ms)/float(ledParams.transitionOnPeriod_ms));
    } else if (phaseTime_ms < (ledParams.transitionOnPeriod_ms + ledParams.onPeriod_ms)) {
      // In the "on" period
      newColor = ledParams.onColor;
    } else if (phaseTime_ms < (ledParams.transitionOnPeriod_ms + ledParams.onPeriod_ms + ledParams.transitionOffPeriod_ms)) {
      // In the "off transition" period
      const u16 offPhase = phaseTime_ms - (ledParams.transitionOnPeriod_ms + ledParams.onPeriod_ms);
      newColor = AlphaBlend(ledParams.offColor, ledParams.onColor, float(offPhase)/float(ledParams.transitionOffPeriod_ms));
    } else {
      // In the "off" period
      newColor = ledParams.offColor;
    }
    
    return newColor;
    
  } // GetCurrentLEDcolor()

} // namespace Anki
} // namespace Vector
