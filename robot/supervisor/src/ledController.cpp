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
#define TIMESTAMP_TO_FRAMES(ts) ((u16)((ts) / 30))


namespace Anki {
namespace Cozmo {
  // Alpha blending (w/ black) without using floating point:
  //  See also: http://stackoverflow.com/questions/12011081/alpha-blending-2-rgba-colors-in-c
  inline u16 AlphaBlend(const u16 onColor, const u16 offColor, const float alpha)
  {
    const float onRed  = GET_RED(onColor);
    const float onGrn  = GET_GRN(onColor);
    const float onBlu  = GET_BLU(onColor);
    const float offRed = GET_RED(offColor);
    const float offGrn = GET_GRN(offColor);
    const float offBlu = GET_BLU(offColor);
    const float invAlpha = 1.0f - alpha;

    return ((u16)(onRed * alpha + offRed * invAlpha)) << LED_ENC_RED_SHIFT |
           ((u16)(onGrn * alpha + offGrn * invAlpha)) << LED_ENC_GRN_SHIFT |
           ((u16)(onBlu * alpha + offBlu * invAlpha)) << LED_ENC_BLU_SHIFT |
           (alpha >= 0.5f ? onColor & LED_ENC_IR : offColor & LED_ENC_IR);
  }

  bool GetCurrentLEDcolor(const LightState& ledParams, const TimeStamp_t currentTime, TimeStamp_t& phaseTime,
                          u16& newColor)
  {
    u16 phaseFrame = TIMESTAMP_TO_FRAMES(currentTime - phaseTime);
    if (phaseFrame > 1024)
    {
      phaseTime = currentTime; // Someone changed currentTime under us or something else went wrong so reset
      phaseFrame = 0;
    }

    if (phaseFrame < ledParams.onFrames)
    {
      if (phaseFrame <= ledParams.transitionOnFrames) // Still turning on
      {
        newColor = AlphaBlend(ledParams.onColor, ledParams.offColor, (float)phaseFrame/(float)ledParams.transitionOnFrames);
        return true;
      }
      else // All the way on, don't need to change the color
      {
        newColor = ledParams.onColor;
        return false;
      }
    }
    else if (phaseFrame < (ledParams.onFrames + ledParams.offFrames))
    {
      const u16 offPhase = phaseFrame - ledParams.onFrames;
      if (offPhase <= ledParams.transitionOffFrames)
      {
        newColor = AlphaBlend(ledParams.onColor, ledParams.offColor, (float)offPhase/(float(ledParams.transitionOffFrames)));
        return true;
      }
      else
      {
        newColor = ledParams.offColor;
        return false;
      }
    }
    else
    {
      phaseTime = currentTime;
      newColor = ledParams.offColor;
      return false;
    }
  } // GetCurrentLEDcolor()

} // namespace Anki
} // namespace Cozmo
