/** @file Header file for C++ interface to RTIP on the Espressif
 * @author Daniel Casner <daniel@anki.com>
 */
#ifndef __RTIP_h
#define __RTIP_h

#include "clad/robotInterface/messageEngineToRobot.h"

namespace Anki {
  namespace Cozmo {
    namespace RTIP {
      // Send a message to the RTIP
      bool SendMessage(RobotInterface::EngineToRobot& msg);
    }
  }
}

#endif
