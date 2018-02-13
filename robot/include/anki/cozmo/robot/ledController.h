#ifndef ANKI_COZMO_ROBOT_LED_CONTROLLER_H
#define ANKI_COZMO_ROBOT_LED_CONTROLLER_H

#include "coretech/common/shared/types.h"
#include "clad/types/ledTypes.h"

namespace Anki {
  namespace Cozmo {
  
    // Number of milliseconds corresponding to one frame
    const u32 kDefaultMsPerFrame = 30;
    
    // Based on the current time and the current state of an LED (as indicated by
    // its LEDParams), returns the new RGBA color in newColor. Returns whether the
    // color of the LED actually changed.
    bool GetCurrentLEDcolor(const LightState& ledParams, const TimeStamp_t currentTime, TimeStamp_t& phaseTime,
                            u32& newColor, const u32 msPerFrame = kDefaultMsPerFrame);

  } // namespace Cozmo
} // namespace Anki


#endif // ANKI_COZMO_ROBOT_LED_CONTROLLER_H
