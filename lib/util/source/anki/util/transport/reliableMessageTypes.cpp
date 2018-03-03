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

#ifdef TARGET_ESPRESSIF
#include "c_types.h"
#endif

#include "util/transport/reliableMessageTypes.h"

#ifdef __cplusplus
namespace Anki {
namespace Util {
#endif

bool IsMessageTypeAlwaysSentUnreliably(EReliableMessageType messageType)
{
  switch(messageType)
  {
    case eRMT_SingleUnreliableMessage:
    case eRMT_MultipleReliableMessages:
    case eRMT_MultipleUnreliableMessages:
    case eRMT_MultipleMixedMessages:
    case eRMT_ACK:
    case eRMT_Ping:
      return true;
    default:
      return false;
  }
}


bool IsMutlipleMessagesType(EReliableMessageType messageType)
{
  switch(messageType)
  {
    case eRMT_MultipleReliableMessages:
    case eRMT_MultipleUnreliableMessages:
    case eRMT_MultipleMixedMessages:
      return true;
    default:
      return false;
  }
}
  
  
const char* ReliableMessageTypeToString(EReliableMessageType messageType)
{
#ifdef NDEBUG
  return "";
#else // NDEBUG
  switch(messageType)
  {
    case eRMT_Invalid:                      break;
    case eRMT_ConnectionRequest:            return "ConnRequest";
    case eRMT_ConnectionResponse:           return "ConnResponse";
    case eRMT_DisconnectRequest:            return "DisconnRequest";
    case eRMT_SingleReliableMessage:        return "SingleReliable";
    case eRMT_SingleUnreliableMessage:      return "SingleUnreliable";
    case eRMT_MultiPartMessage:             return "MultiPartMessage";
    case eRMT_MultipleReliableMessages:     return "MultipleReliableMessages";
    case eRMT_MultipleUnreliableMessages:   return "MultipleUnreliableMessage";
    case eRMT_MultipleMixedMessages:        return "MultipleMixedMessages";
    case eRMT_ACK:                          return "ACK";
    case eRMT_Ping:                         return "Ping";
    case eRMT_Count:                        break;
  }
  
  return "Invalid";
#endif // NDEBUG
}
  
bool IsValidMessageType(EReliableMessageType messageType)
{
  return messageType > eRMT_Invalid && messageType < eRMT_Count;
}


#ifdef __cplusplus
} // end namespace Util
} // end namespace Anki
#endif
