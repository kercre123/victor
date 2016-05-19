/** @file Upgrade / flash controller
 * This module is responsible for reflashing the Espressif and other chips in Cozmo
 * @author Daniel Casner <daniel@anki.com>
 */
#ifndef __upgrade_controller_h
#define __upgrade_controller_h

#include "clad/robotInterface/otaMessages.h"

namespace Anki {
  namespace Cozmo {
    namespace UpgradeController
    {
      /// Initalize the upgrade controller and enable its operation
      bool Init();
      
      /// Periodic update function run in background trhead
      void Update();
      
      /// Command writing data into flash on the Espressif or RTIP or Body
      void Write(RobotInterface::OTA::Write& msg);
    }
  }
}



#endif
