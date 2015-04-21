#ifndef ANKI_COZMO_ROBOT_LED_CONTROLLER_H
#define ANKI_COZMO_ROBOT_LED_CONTROLLER_H

#include "anki/common/types.h"
#include "anki/cozmo/shared/ledTypes.h"

namespace Anki {
  namespace Cozmo {
    
    // Based on the current time and the current state of an LED (as indicated by
    // its LEDParams), returns the new RGBA color in newColor. Returns whether the
    // color of the LED actually changed.
    bool GetCurrentLEDcolor(LEDParams_t& ledParams, TimeStamp_t currentTime,
                            u32& newColor);
    
  } // namespace Cozmo
} // namespace Anki


#endif // ANKI_COZMO_ROBOT_LED_CONTROLLER_H
