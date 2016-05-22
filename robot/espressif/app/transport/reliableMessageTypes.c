/**
 * File: reliableMessageTypes
 *
 * Author: Mark Wesley
 * Created: 05/22/15
 *
 * Copied to cozmo-engine/robot/espressif to remove external dependancy for firmware build
 *
 * Description: Reliable Message Types (and helper functions related to them)
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "c_types.h"

#include "transport/reliableMessageTypes.h"

bool IsMessageTypeAlwaysSentUnreliably(EReliableMessageType messageType)
{
  switch(messageType)
  {
    case eRMT_SingleUnreliableMessage:
    case eRMT_ACK:
    case eRMT_Ping:
    case eRMT_MultipleReliableMessages:
    case eRMT_MultipleUnreliableMessages:
    case eRMT_MultipleMixedMessages:
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
}
  
bool IsValidMessageType(EReliableMessageType messageType)
{
  return messageType > eRMT_Invalid && messageType < eRMT_Count;
}
