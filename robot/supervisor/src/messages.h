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
 * Copyright: Anki, Inc. 2013
 **/

#ifndef COZMO_MESSAGE_ROBOT_H
#define COZMO_MESSAGE_ROBOT_H

#include "anki/common/types.h"
#include "anki/common/robot/array2d_declarations.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

namespace Anki {
  namespace Cozmo {
    namespace Messages {

      // Return a const reference to the current robot state message
      RobotState const& GetRobotStateMsg();

      // Create all the dispatch function prototypes (all implemented
      // manually in messages.cpp).
      #include "clad/robotInterface/messageEngineToRobot_declarations.def"
    
      Result Init();

      void ProcessBTLEMessages();
      void ProcessUARTMessages();

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

      // For sending text message to basestation
      int SendText(const char *format, ...);

      // va_list version
      int SendText(const char *format, va_list vaList);

      // Returns whether or not init message was received from basestation
      bool ReceivedInit();

      // Resets the receipt of init message
      void ResetInit();

#     ifdef SIMULATOR
      // Send out a chunked up JPEG-compressed image
      Result CompressAndSendImage(const Embedded::Array<u8> &img, const TimeStamp_t captureTime);
#     endif

    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki


#endif  // #ifndef COZMO_MESSAGE_ROBOT_H
