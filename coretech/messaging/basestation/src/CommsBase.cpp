//
//  CommsBase.cpp
//  Cozmo
//
//  Created by Greg Nagel on 5/26/2015.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#include "anki/messaging/basestation/CommsBase.h"

#include "anki/util/logging/logging.h"

using namespace Anki::Comms;

CommsBase::CommsBase()
{
}

CommsBase::~CommsBase()
{
}

bool CommsBase::PopIncomingPacket(IncomingPacket& packet)
{
  if (!_incomingPacketQueue.empty())
  {
    packet = _incomingPacketQueue.front();
    _incomingPacketQueue.pop_front();
    
    // assign address if there is one
    // may return false and still leave it as -1
    s32 sourceId = -1;
    GetConnectionId(sourceId, packet.sourceAddress);
    packet.sourceId = sourceId;
    
    return true;
  }
  return false;
}

void CommsBase::AddConnection(s32 connectionId, const TransportAddress& address)
{
  auto foundAddress = _addressLookup.find(address);
  if (foundAddress != _addressLookup.end()) {
    // already added; ignore
    if (foundAddress->second == connectionId) {
      return;
    }
    
    PRINT_STREAM_WARNING("CommsBase.AddConnection",
                         "Already registered address " << address.ToString() <<
                         " with existing id " << foundAddress->second <<
                         "; will disconnect existing id and overwrite with id " << connectionId << ".");
    
    // call that should invoke the correct disconnection methods
    RemoveConnectionForOverwrite(foundAddress->second);
  }

  auto foundConnectionId = _connectionIdLookup.find(connectionId);
  if (foundConnectionId != _connectionIdLookup.end()) {
    PRINT_STREAM_WARNING("CommsBase.AddConnection",
                         "Already registered connection id " << connectionId <<
                         " with existing address " << foundConnectionId->second.ToString() <<
                         "; will disconnect existing address and overwrite with address " << address.ToString() << ".");
    
    // call that should invoke the correct disconnection methods
    RemoveConnectionForOverwrite(connectionId);
  }

  _addressLookup.emplace(address, connectionId);
  _connectionIdLookup.emplace(connectionId, address);
}

void CommsBase::RemoveConnection(s32 connectionId)
{
  TransportAddress address;
  if (RemoveConnectionInternal(address, connectionId)) {
    ClearPacketsForAddressUntilNewestConnection(address);
  }
}

void CommsBase::RemoveAllConnections()
{
  _addressLookup.clear();
  _connectionIdLookup.clear();
  _incomingPacketQueue.clear();
}

bool CommsBase::GetConnectionId(s32& connectionId, const TransportAddress& address) const
{
  auto found = _addressLookup.find(address);
  if (found != _addressLookup.end()) {
    connectionId = found->second;
    return true;
  }
  return false;
}

bool CommsBase::GetAddress(TransportAddress& address, s32 connectionId) const
{
  auto found = _connectionIdLookup.find(connectionId);
  if (found != _connectionIdLookup.end()) {
    address = found->second;
    return true;
  }
  return false;
}

bool CommsBase::AddressEquals(const TransportAddress& a, const TransportAddress& b)
{
  return !(a < b || b < a);
}

void CommsBase::PushIncomingPacket(const IncomingPacket& packet)
{
  _incomingPacketQueue.push_back(packet);
}

void CommsBase::EmplaceIncomingPacket(const IncomingPacket&& packet)
{
  _incomingPacketQueue.emplace_back(packet);
}

void CommsBase::ReceiveData(const uint8_t *buffer, unsigned int bufferSize, const TransportAddress& sourceAddress)
{
  //f32 timestamp = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();

  // will set sourceId on peek
  const s32 PLACEHOLDER_SOURCE_ID = -1;
  EmplaceIncomingPacket(IncomingPacket(IncomingPacket::Tag::NormalMessage, buffer, bufferSize, PLACEHOLDER_SOURCE_ID, sourceAddress));
}

void CommsBase::ClearPacketsForAddress(const TransportAddress& address)
{
  auto begin = _incomingPacketQueue.begin();
  auto end = _incomingPacketQueue.end();
  auto remove_from = std::remove_if(begin, end,
                 [address](const IncomingPacket& packet) -> bool
                 {
                   return AddressEquals(address, packet.sourceAddress);
                 });
  _incomingPacketQueue.erase(remove_from, end);
}

void CommsBase::ClearPacketsForAddressUntilNewestConnection(const TransportAddress& address)
{
  auto begin = _incomingPacketQueue.begin();
  auto end = _incomingPacketQueue.end();
  // no contents, nothing to remove
  if (end == begin) {
    return;
  }
  
  // look for the iterator just past the last disconnect packet
  // we need the forward iterator for later
  auto past_last_disconnect = end;
  do {
    auto current = past_last_disconnect;
    // already checked against begin
    --current;
    
    // only looks at disconnect packets
    if (current->tag == IncomingPacket::Tag::Disconnected &&
        AddressEquals(current->sourceAddress, address)) {
      break;
    }
    
    past_last_disconnect = current;
  } while (past_last_disconnect != begin);
  
  // until past_last_disconnect, shift entries over, stomping over anything with specified address
  // ends with a gap between gap_start_location and past_last_disconnect of garbage data
  auto gap_start_location = std::remove_if(begin, past_last_disconnect,
                                           [address](const IncomingPacket& packet) -> bool
                                           {
                                             return AddressEquals(packet.sourceAddress, address);
                                           });
  
  // throw away the gap
  _incomingPacketQueue.erase(gap_start_location, past_last_disconnect);
}

void CommsBase::ClearPacketsForAddressAfterEarliestDisconnect(const TransportAddress& address)
{
  auto begin = _incomingPacketQueue.begin();
  auto end = _incomingPacketQueue.end();
  
  // look for the iterator pointing to the first disconnect packet
  auto first_disconnect = std::find_if(begin, end,
               [address](const IncomingPacket& packet)
               {
                 return (packet.tag == IncomingPacket::Tag::Disconnected &&
                         AddressEquals(packet.sourceAddress, address));
               });
  
  // early out
  if (first_disconnect == end) {
    return;
  }
  // point just past the first disconnect
  ++first_disconnect;
  
  // from first_disconnect to end, shift entries toward begin, stomping over anything with specified address
  // ends with a gap between gap_start_location and end of garbage data
  auto gap_start_location = std::remove_if(first_disconnect, end,
                                           [address](const IncomingPacket& packet) -> bool
                                           {
                                             return AddressEquals(packet.sourceAddress, address);
                                           });
  
  // throw away the gap
  _incomingPacketQueue.erase(gap_start_location, end);
}

void CommsBase::RemoveConnectionForOverwrite(s32 connectionId)
{
  TransportAddress address;
  if (RemoveConnectionInternal(address, connectionId)) {
    ClearPacketsForAddress(address);
  }
}

bool CommsBase::RemoveConnectionInternal(TransportAddress& address, s32 connectionId)
{
  auto found = _connectionIdLookup.find(connectionId);
  if (found == _connectionIdLookup.end()) {
    address = found->second;
    _addressLookup.erase(address);
    _connectionIdLookup.erase(found);
    return true;
  }
  return false;
}
