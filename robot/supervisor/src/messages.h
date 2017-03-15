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

#include "anki/types.h"
#include "clad/types/motorTypes.h"
#include <stdarg.h>
#include <stddef.h>
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"

namespace Anki {
  namespace Cozmo {
    namespace Messages {
#ifndef TARGET_ESPRESSIF
      // Return a const reference to the current robot state message
      RobotState const& GetRobotStateMsg();

      // Create all the dispatch function prototypes (all implemented
      // manually in messages.cpp).
      #include "clad/robotInterface/messageEngineToRobot_declarations.def"

      void ProcessBadTag_EngineToRobot(const RobotInterface::EngineToRobot::Tag tag);
#endif
      Result Init();
      extern "C" void ProcessMessage(u8* buffer, u16 bufferSize);

      void Update();

      void ProcessMessage(RobotInterface::EngineToRobot& msg);

      void Process_anim(const RobotInterface::EngineToRobot& msg);

      // Start looking for a particular message ID
      void LookForID(const RobotInterface::EngineToRobot::Tag msgID);

      // Did we see the message ID we last set? (Or perhaps we timed out)
      bool StillLookingForID(void);

      // Used by visionSystem for prediction during tracking
      void UpdateRobotStateMsg();

      // Sends robot state message, either the one passed in or the one
      // stored internally that is updated by UpdateRobotStateMsg().
      Result SendRobotStateMsg(const RobotState* msg = NULL);
      
      void SendTestStateMsg();

      Result SendMotorCalibrationMsg(MotorID motor, bool calibStarted, bool autoStarted = false);
      
      Result SendMotorAutoEnabledMsg(MotorID motor, bool calibStarted);
      
      void ResetMissedLogCount();
      
#ifdef SIMULATOR
      // For sending text message to basestation
      int SendText(const char *format, ...);

      // va_list version
      int SendText(const char *format, va_list vaList);

      // va_list version with level
      int SendText(const RobotInterface::LogLevel level, const char *format, va_list vaList);
#endif

      // Returns whether or not init message was received from basestation
      bool ReceivedInit();

      // Resets the receipt of init message
      void ResetInit();

#     ifdef SIMULATOR
      // Send out a chunked up JPEG-compressed image
      Result CompressAndSendImage(const u8* img, const s32 captureHeight, const s32 captureWidth, const TimeStamp_t captureTime);
#     endif

    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki


#endif  // #ifndef COZMO_MESSAGE_ROBOT_H
