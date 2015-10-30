/**
 * File: messages.h  (was messageProtocol.h)
 *
 * Author: Kevin Yoon
 * Created: 9/24/2013
 *
 * Major overhaul to use macros for generating message defintions
 * Author: Andrew Stein
 * Date:   10/13/2013
 *
 * Major overhaul to use CLAD generated messages and function definitions and to split between the Espressif and K02
 * Author: Daniel Casner
 * 10/22/2015
 *
 * Copyright: Anki, Inc. 2013
 **/

#ifndef COZMO_MESSAGE_ROBOT_H
#define COZMO_MESSAGE_ROBOT_H

#include "anki/types.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"

#include <stdarg.h>

namespace Anki {
  namespace Cozmo {
    namespace Messages {

      Result Init();

      extern "C" void ProcessMessage(u8* buffer, u16 bufferSize);
      
      // For sending text message to basestation
      int SendText(const char *format, ...);

      // va_list version
      int SendText(const char *format, va_list vaList);
      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki


#endif  // #ifndef COZMO_MESSAGE_ROBOT_H
