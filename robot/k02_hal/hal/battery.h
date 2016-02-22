#ifndef __BATTERY_H
#define __BATTERY_H

#include "clad/robotInterface/messageEngineToRobot.h"

namespace Anki {
  namespace Cozmo {
    namespace HAL {
      namespace Battery {
        void HandlePowerStateUpdate(PowerState& msg);
      }
    }
  }
}

#endif
