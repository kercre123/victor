#ifndef COZMO_CONFIG_H
#define COZMO_CONFIG_H

#include "anki/common/constantsAndMacros.h"
#include "anki/common/types.h"

namespace Anki {
namespace Cozmo {

#ifdef SIMULATOR
    // Simulated robot
    #include "anki/cozmo/shared/cozmoConfig_sim.h"
#elif defined(COZMO_ROBOT_V40)
    #include "anki/cozmo/shared/cozmoConfig_v4.0.h"
#else // COZMO_BASESTATION?
    // Engine / App
    // Include the file for whichever robot you want to be controlling with the app
    #include "anki/cozmo/shared/cozmoConfig_v4.0.h"
#endif
    
    #include "anki/cozmo/shared/cozmoConfig_common.h"

} // namespace Cozmo
} // namespace Anki

#endif // COZMO_CONFIG_H
