/**
 * File: reliableMessageTypes
 *
 * Author: Mark Wesley
 * Created: 05/22/15
 *
 * Description: Reliable Message Types (and helper functions related to them)
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Util_Transport_ReliableMessageTypes_H__
#define __Util_Transport_ReliableMessageTypes_H__

#ifdef __cplusplus
namespace Anki {
namespace Util {
extern "C" {
#else
#include "cBool.h"
#endif

#ifdef __APPLE__
typedef enum __attribute__ ((__packed__))
#else
typedef enum
#endif
{
  eRMT_Invalid = 0,
  eRMT_ConnectionRequest,
  eRMT_ConnectionResponse,
  eRMT_DisconnectRequest,
  eRMT_SingleReliableMessage,
  eRMT_SingleUnreliableMessage, // for unreliable messages stuffed into a multi-message of other messages to minimize packets sent
  eRMT_MultiPartMessage, // big message, in n pieces
  eRMT_MultipleReliableMessages,   // several reliable messages grouped grouped into 1 packet (increase chance of acking all together)
  eRMT_MultipleUnreliableMessages, // several unreliable messages grouped into 1 packet (reduce number of packets sent)
  eRMT_MultipleMixedMessages,      // several (reliable and unreliable) messages grouped into 1 packet (combo of above)
  eRMT_ACK,
  eRMT_Ping,
  eRMT_Count
} EReliableMessageType;


// ReliableTransport layer can still send things unreliably - some message types are always unreliable
bool IsMessageTypeAlwaysSentUnreliably(EReliableMessageType messageType);

bool IsMutlipleMessagesType(EReliableMessageType messageType);
  
const char* ReliableMessageTypeToString(EReliableMessageType messageType);
  
bool IsValidMessageType(EReliableMessageType messageType);

#ifdef __cplusplus
} // end extern "C"
} // end namespace Util
} // end namespace Anki
#endif

#endif // __Util_Transport_ReliableMessageTypes_H__
