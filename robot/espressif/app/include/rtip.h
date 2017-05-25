/** @file Header file for C++ interface to RTIP on the Espressif
 * @author Daniel Casner <daniel@anki.com>
 */
#ifndef __RTIP_h
#define __RTIP_h

extern "C"
{
  #include "anki/cozmo/robot/drop.h"
}

#include "clad/robotInterface/messageEngineToRobot.h"

/** Makes a buffer smaller than a full EngineToRobot message bug big enough for anything to send to RTIP on the stack
 * and creates a variable of type EngineToRobot referencing it.
 */
#define MAKE_RTIP_MSG(msg) \
uint32_t msg__buffer[(DROP_TO_RTIP_MAX_VAR_PAYLOAD/sizeof(uint32_t)) + 1]; \
Anki::Cozmo::RobotInterface::EngineToRobot& msg = *((Anki::Cozmo::RobotInterface::EngineToRobot*)msg__buffer)

// Timeout and trigger watchdog after 50 seconds
#define RTIP_MESSAGE_TIMEOUT (50000000)

namespace Anki {
  namespace Cozmo {
    namespace RTIP {
      // Initalize the RTIP module
      bool Init();
      
      // Send a message to the RTIP
      bool SendMessage(const u8* buffer, const u16 bufferSize, const u8 tag=RobotInterface::GLOBAL_INVALID_TAG);
      
      // Send a message to the RTIP
      bool SendMessage(RobotInterface::EngineToRobot& msg);
      
      // Periodic update function, polls messages received over I2SPI and dispatches messages.
      void Update();
      
      /** Check the RTIP queue estimate to see if there is room
       * If there is room, the estimate will automatically be incremented.
       * @param bytes The number of bytes we want to send
       * @return 0 if the is no room or > 0 if there is room
       */
      extern "C" int rtipPoke(const int bytes);
    }
  }
}

#endif
