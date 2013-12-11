/**
 * File: cozmoMsgProtocol.h
 * 
 * Author: Kevin Yoon 
 * Created: 9/24/2013 
 *
 * Description: The file contains all the low level structs that
 *              define the messaging protocol. This is only a
 *              header file that is shared between the robot and
 *              the basestation.
 *
 * Copyright: Anki, Inc. 2013
 **/

#ifndef COZMO_MSG_PROTOCOL_H
#define COZMO_MSG_PROTOCOL_H

#include "anki/common/types.h"
    
// Base message size (just size byte)
#define SIZE_MSG_BASE_SIZE 1

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

// Channel number used by CozmoWorldComm Webots receiver
// for robot messages bound for the basestation.
#define BASESTATION_SIM_COMM_CHANNEL 100

// Port on which CozmoWorldComms is listening for a connection from basestation.
#define COZMO_WORLD_LISTEN_PORT "5555"

// First two bytes of message which prefixes normal messages
#define COZMO_WORLD_MSG_HEADER_BYTE_1 (0xbe)
#define COZMO_WORLD_MSG_HEADER_BYTE_2 (0xef)

// Size of CozmoWorldMessage header.
// Includes COZMO_WORLD_MSG_HEADER_BYTE_1 and COZMO_WORLD_MSG_HEADER_BYTE_2 and 1 byte for robotID.
#define COZMO_WORLD_MSG_HEADER_SIZE 3

const u32 BASESTATION_RECV_BUFFER_SIZE = 2048;

////////// End Simulator comms //////////

// 1. Initial include just defines the definition modes for use below
#include "anki/cozmo/MessageDefinitions.h"

// 2. Define all the message structs:
#define MESSAGE_DEFINITION_MODE MESSAGE_STRUCT_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"

// 3. Create the enumerated message IDs:
typedef enum {
#undef MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_ENUM_DEFINITION_MODE
#include "anki/cozmo/MessageDefinitions.h"
  NUM_MSG_IDS // Final entry without comman at end
} CozmoMessageID;

// 4. Fill in the message information lookup table:
// <This happens in messageProtocol.cpp>
typedef struct {
  u8 size;
  u8 priority;
  void (*dispatchFcn)(const u8* buffer);
} MessageTableEntry;

extern MessageTableEntry MessageTable[256];

/* I don't think i need this
 
// From a message buffer, determines the message ID (and makes sure it is
// within expected range), determines the message size (and makes sure it
// matches expected size for this message ID), and returns a pointer to
// the start of the actual message contents (no header) within the original
// buffer.
inline ReturnCode ParseMessageBuffer(const u8* fullBuffer,
                                     u8& size, CozmoMessageID& msgID,
                                     const u8* &strippedBuffer)
{
  ReturnCode retVal = EXIT_SUCCESS;
  
  size  = fullBuffer[0];
  msgID = static_cast<CozmoMessageID>(fullBuffer[1]);

  if(msgID >= NUM_MSG_IDS) {
    PRINT("Invalid message ID in message buffer.\n");
    retVal = EXIT_FAILURE;
  }
  else {
    if(size != MessageTable[msgID].size) {
      PRINT("Unexpected message size for specified message ID.\n");
      retVal = EXIT_FAILURE;
    }
    else {
      // Just move up two bytes to get past the header
      strippedBuffer = fullBuffer + 2;
    }
  }
  return retVal;
}
*/



#endif  // #ifndef COZMO_MSG_PROTOCOL

