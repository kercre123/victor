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

#ifndef COZMO_MSG_PROTOCOL_H
#define COZMO_MSG_PROTOCOL_H

#include "anki/common/types.h"


// Basestation defines
#define MSG_FIRST_DATA_BYTE_IDX_BASESTATION 2

// Expected message receive latency
// It is assumed that this value does not fluctuate greatly.
// The more inaccurate this value is, the more invalid our
// handling of messages will be.
#define MSG_RECEIVE_LATENCY_SEC 0.03

// The effective latency of vehicle messages for basestation modelling purposes
// This is twice the MSG_RECEIVE_LATENCY_SEC so that the basestation maintains a model
// of the system one message cycle latency in the future. This way, commanded actions are applied
// at the time they are expected in the physical world.
#define BASESTATION_MODEL_LATENCY_SEC (2*MSG_RECEIVE_LATENCY_SEC)


////////// Simulator comms //////////

// TODO: are all these still used and should they be defined elsewhere?

// Channel number used by CozmoWorldComm Webots receiver
// for robot messages bound for the basestation.
#define BASESTATION_SIM_COMM_CHANNEL 100

// Port on which CozmoWorldComms is listening for a connection from basestation.
#define COZMO_WORLD_LISTEN_PORT "5555"

////////// End Simulator comms //////////

namespace Anki {
  namespace Cozmo {
    namespace Messages {

      // Packet headers/footers:
      
      // TODO: Do we need this?  Only used in simulation I think? (Add #ifdef SIMULATOR?)
      const u8 RADIO_PACKET_HEADER[2] = {0xBE, 0xEF};

      const u8 USB_PACKET_HEADER[4] = {0xBE, 0xEF, 0xF0, 0xFF}; // BEEFF0FF
      const u8 USB_PACKET_FOOTER[4] = {0xFF, 0x0F, 0xFE, 0xEB}; // FF0FFEEB
      
      
      // 1. Initial include just defines the definition modes for use below
#include "anki/cozmo/MessageDefinitions.h"
      
      // 2. Define all the message structs:
#define MESSAGE_DEFINITION_MODE MESSAGE_STRUCT_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"
      
      // 3. Create the enumerated message IDs:
      typedef enum {
        NO_MESSAGE_ID = 0,
#undef MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_ENUM_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"
        NUM_MSG_IDS // Final entry without comma at end
      } ID;
      
      // Return the size of a message, given its ID
      u8 GetSize(const ID msgID);
      
      void ProcessBTLEMessages();
      void ProcessUARTMessages();
      
      void ProcessMessage(const ID msgID, const u8* buffer);
      
      // Start looking for a particular message ID
      void LookForID(const ID msgID);
      
      // Did we see the message ID we last set? (Or perhaps we timed out)
      bool StillLookingForID(void);
      
      // These return true if a mailbox messages was available, and they copy
      // that message into the passed-in message struct.
      bool CheckMailbox(BlockMarkerObserved& msg);
      bool CheckMailbox(MatMarkerObserved&   msg);
      bool CheckMailbox(DockingErrorSignal&  msg);
      
    } // namespace Messages
  } // namespace Cozmo
} // namespace Anki


#endif  // #ifndef COZMO_MSG_PROTOCOL

