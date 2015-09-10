#ifndef COZMO_CONFIG_H
#define COZMO_CONFIG_H

#include "anki/common/constantsAndMacros.h"
#include "anki/common/types.h"

namespace Anki {
namespace Cozmo {

#include "anki/cozmo/shared/cozmoConfig_common.h"
  
#ifdef COZMO_ROBOT
  // Robot
#ifdef SIMULATOR
#include "anki/cozmo/shared/cozmoConfig_sim.h"
#else
#include "anki/cozmo/shared/cozmoConfig_v4.1.h"
#endif
  
#else // COZMO_BASESTATION
  // Engine / App

  // If there are config values that need to differ between sim and physical robot
  // they can be referenced via namespace
  namespace RobotConfigSim {
    #include "anki/cozmo/shared/cozmoConfig_sim.h"
  }
  namespace RobotConfigPhys {
    #include "anki/cozmo/shared/cozmoConfig_v4.1.h"
  }

#endif

} // namespace Cozmo
} // namespace Anki

#endif // COZMO_CONFIG_H
