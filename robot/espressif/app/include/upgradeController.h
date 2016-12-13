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
      
      /// Maximum possible size of firmware version meta data
      extern const u32 VERSION_INFO_MAX_LENGTH;
      
      /// Retreive pointer to firmware version meta data in flash
      u32* GetVersionInfo();
      
      /// Retrieves the first character of the firmware build type
      char GetBuildType();
      
      extern "C" {
        /// Retrieve numerical firmware version
        s32 GetFirmwareVersion();
        
        /// Retrieve the numerical (epoch) build timestamp
        u32 GetBuildTime();
        
        /// One time (per firmware upgrade) write of firmware note
        bool SetFirmwareNote(const u32 offset, u32 note);
        
        /// Return the one time write note stored at the end of firmware
        u32 GetFirmwareNote(const u32 offset);
      }
    }
  }
}



#endif
