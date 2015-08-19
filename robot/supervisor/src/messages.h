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

namespace Anki {
  namespace Cozmo {
    namespace Messages {

      // 1. Initial include just defines the definition modes for use below
#     include "anki/cozmo/shared/RobotMessageDefinitions.h"

      // 2. Define all the message structs:
#     define MESSAGE_DEFINITION_MODE MESSAGE_STRUCT_DEFINITION_MODE
#     include "anki/cozmo/shared/RobotMessageDefinitions.h"

      // 3. Create the enumerated message IDs:
#ifdef ROBOT_HARDWARE
      // Keil doesn't support enum inheritance, but it compiles ID to 1-byte anyway.
      // Compile-time check is below.
      typedef enum {
#else
      typedef enum : u8 {
#endif
        NO_MESSAGE_ID = 0,
#       undef MESSAGE_DEFINITION_MODE
#       define MESSAGE_DEFINITION_MODE MESSAGE_ENUM_DEFINITION_MODE
#       include "anki/cozmo/shared/RobotMessageDefinitions.h"
        NUM_MSG_IDS // Final entry without comma at end
      } ID;
        
      // Compile-time check for size of ID
      ct_assert(sizeof(ID) == 1);

      // Return the size of a message, given its ID
      u16 GetSize(const ID msgID);

      // Return a const reference to the current robot state message
      RobotState const& GetRobotStateMsg();

      // Create all the dispatch function prototypes (all implemented
      // manually in messages.cpp).
#     define MESSAGE_DEFINITION_MODE MESSAGE_DISPATCH_DEFINITION_MODE
#     include "anki/cozmo/shared/MessageDefinitionsB2R.def"

      Result Init();

      void ProcessBTLEMessages();
      void ProcessUARTMessages();

      // This message is special because it's not only sent to the basestation, but it's also
      // "received" by the robot from visionSystem. It's defined as a R2B message
      // so its declaration is not automatically generated.
      void ProcessDockingErrorSignalMessage(const DockingErrorSignal& msg);

      void ProcessFaceDetectionMessage(const FaceDetection& msg);

      void ProcessMessage(const ID msgID, const u8* buffer);

      // Start looking for a particular message ID
      void LookForID(const ID msgID);

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

      // These return true if a mailbox messages was available, and they copy
      // that message into the passed-in message struct.
      //bool CheckMailbox(VisionMarker&        msg);
      //bool CheckMailbox(ImageChunk&          msg);
      bool CheckMailbox(DockingErrorSignal&  msg);
      bool CheckMailbox(FaceDetection&       msg);

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
