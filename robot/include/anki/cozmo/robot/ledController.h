#ifndef ANKI_COZMO_ROBOT_LED_CONTROLLER_H
#define ANKI_COZMO_ROBOT_LED_CONTROLLER_H

#include "coretech/common/shared/types.h"
#include "clad/types/ledTypes.h"

#ifdef USES_CPPLITE
#define CLAD(ns) CppLite::Anki::Vector::ns
#else
#define CLAD(ns) ns
#endif

namespace Anki {
  namespace Vector {
  
    // Number of milliseconds corresponding to one frame
    const u32 kDefaultMsPerFrame = 30;
    
    // Based on the current time and the current state of an LED (as indicated by
    // its LEDParams), returns the new RGBA color.
    u32 GetCurrentLEDcolor(const CLAD(LightState)& ledParams,     // The ledParams struct for this LED
                           const TimeStamp_t currentTime,   // current timestamp
                           const TimeStamp_t phaseTime,     // timestamp indicating the 'beginning' of the light animation
                           const u32 msPerFrame = kDefaultMsPerFrame);

  } // namespace Vector
} // namespace Anki

#undef CLAD

#endif // ANKI_COZMO_ROBOT_LED_CONTROLLER_H
