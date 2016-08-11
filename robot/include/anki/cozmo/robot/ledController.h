#ifndef ANKI_COZMO_ROBOT_LED_CONTROLLER_H
#define ANKI_COZMO_ROBOT_LED_CONTROLLER_H

#include "anki/types.h"
#include "clad/types/ledTypes.h"

#define TIMESTAMP_TO_FRAMES(ts) ((u16)((ts) / 30))

namespace Anki {
  namespace Cozmo {

    // Based on the current time and the current state of an LED (as indicated by
    // its LEDParams), returns the new RGBA color in newColor. Returns whether the
    // color of the LED actually changed.
    bool GetCurrentLEDcolor(const LightState& ledParams, const TimeStamp_t currentTime, TimeStamp_t& phaseTime,
                            u16& newColor);

  } // namespace Cozmo
} // namespace Anki


#endif // ANKI_COZMO_ROBOT_LED_CONTROLLER_H
