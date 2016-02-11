/** @file Header file for C++ interface to RTIP on the Espressif
 * @author Daniel Casner <daniel@anki.com>
 */
#ifndef __RTIP_h
#define __RTIP_h

#include "clad/robotInterface/messageEngineToRobot.h"

namespace Anki {
  namespace Cozmo {
    namespace RTIP {
      // Initalize the RTIP module
      bool Init();
      
      // Send a message to the RTIP
      bool SendMessage(RobotInterface::EngineToRobot& msg);
      
      extern u32 Version;
      extern u32 Date;
      #define VERSION_DESCRIPTION_SIZE 14
      extern char VersionDescription[VERSION_DESCRIPTION_SIZE]; 
      
      // Accept a message from the RTIP and relay or process it
      extern "C" bool AcceptRTIPMessage(uint8_t* payload, uint8_t length);
    }
  }
}

#endif
