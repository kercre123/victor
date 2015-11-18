/** @file Upgrade / flash controller
 * This module is responsible for reflashing the Espressif and other chips in Cozmo
 * @author Daniel Casner <daniel@anki.com>
 */
#ifndef __upgrade_controller_h
#define __upgrade_controller_h

#include "clad/robotInterface/otaMessages.h"

/// An impossible flash address to invalidate the state
#define INVALID_FLASH_ADDRESS (0xFFFFffff)
/// Lowest address we will allow writing to in order to protect the bootloader.and firmware, assets etc go beyond 1MB
#define FLASH_WRITE_START_ADDRESS (0x100000)

namespace Anki {
  namespace Cozmo {
    namespace UpgradeController
    {
      /// Command to erase flash
      void EraseFlash(RobotInterface::EraseFlash& msg);
      
      /// Command writing data into flash
      void WriteFlash(RobotInterface::WriteFlash& msg);
      
      /// Command upgrade initiation
      void Trigger(RobotInterface::OTAUpgrade& msg);
    }
  }
}



#endif
