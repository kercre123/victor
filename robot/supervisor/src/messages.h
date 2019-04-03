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
 * Description: This files uses the information and macros in the
 *              MessageDefinitions.h to create the typedef'd message
 *              structs passed via USB / BTLE and between main and
 *              long execution "threads".  It also creates the enumerated
 *              list of message IDs.  Everything in this file is independent
 *              of specific message definitions -- those are defined in
 *              MessageDefinitions.h.
 *
 * Major overhaul to use CLAD generated messages and function definitions and to split between the Espressif and K02
 * Author: Daniel Casner
 * 10/22/2015
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef COZMO_MESSAGE_ROBOT_H
#define COZMO_MESSAGE_ROBOT_H

#include "coretech/common/shared/types.h"
#include "clad/types/motorTypes.h"
#include <stdarg.h>
#include <stddef.h>
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"

#ifdef USES_CLAD_CPPLITE
#define CLAD(ns) CppLite::Anki::Vector::ns
#else
#define CLAD(ns) ns
#endif

namespace Anki {
  namespace Vector {
    namespace Messages {
      // Create all the dispatch function prototypes (all implemented
      // manually in messages.cpp).
      #include "clad/robotInterface/messageEngineToRobot_declarations.def"
    }
  }
}

namespace Anki {
  namespace Vector {
    namespace Messages {
  
      // Return a const reference to the current robot state message
      CLAD(RobotState) const& GetRobotStateMsg();
  
      void ProcessBadTag_EngineToRobot(const CLAD(RobotInterface)::EngineToRobot::Tag tag);
  
      Anki::Result Init();
      extern "C" void ProcessMessage(u8* buffer, u16 bufferSize);
  
      void Update();
  
      void ProcessMessage(CLAD(RobotInterface)::EngineToRobot& msg);
  
      void UpdateRobotStateMsg();
  
      Anki::Result SendRobotStateMsg();
  
      Anki::Result SendMotorCalibrationMsg(CLAD(MotorID) motor, bool calibStarted, bool autoStarted = false);
      
      Anki::Result SendMotorAutoEnabledMsg(CLAD(MotorID) motor, bool calibStarted);
  
      Anki::Result SendMicDataMsgs();
  
      // Returns whether or not init message was received from basestation
      bool ReceivedInit();
  
      // Resets the receipt of init message
      void ResetInit();
  
    } // namespace Messages
  } // namespace Vector
} // namespace Anki

#undef CLAD

#endif  // #ifndef COZMO_MESSAGE_ROBOT_H
