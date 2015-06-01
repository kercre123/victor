//
//  ChannelBase.cpp
//  Cozmo
//
//  Created by Greg Nagel on 5/26/2015.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#include "anki/messaging/basestation/ChannelBase.h"

#include "anki/util/logging/logging.h"

using namespace Anki::Comms;

ChannelBase::ChannelBase()
{
}

ChannelBase::~ChannelBase()
{
  RemoveAllConnections();
}

bool ChannelBase::IsConnectionActive(ConnectionId connectionId) const
{
  return (_connectionIdLookup.count(connectionId) != 0);
}

int32_t ChannelBase::CountActiveConnections() const
{
  return static_cast<int32_t>(_connectionIdLookup.size());
}

bool ChannelBase::PopIncomingPacket(IncomingPacket& packet)
{
  if (!_incomingPacketQueue.empty())
  {
    packet = _incomingPacketQueue.front();
    _incomingPacketQueue.pop_front();
    
    // assign address if there is one
    // may return false and still leave it as default
    GetConnectionId(packet.sourceId, packet.sourceAddress);
    
    return true;
  }
  return false;
}

void ChannelBase::AddConnection(ConnectionId connectionId, const TransportAddress& address)
{
  PRINT_STREAM_DEBUG("ChannelBase.AddConnection",
                       "ADDING CONNECTION ID " << connectionId << " = " << address);
  
  auto foundAddress = _addressLookup.find(address);
  if (foundAddress != _addressLookup.end()) {
    // already added; ignore
    if (foundAddress->second == connectionId) {
      return;
    }
    
    PRINT_STREAM_WARNING("ChannelBase.AddConnection",
                         "Already registered address " << address <<
                         " with existing id " << foundAddress->second <<
                         "; will disconnect existing id and overwrite with id " << connectionId << ".");
    
    // call that should invoke the correct disconnection methods
    RemoveConnectionForOverwrite(foundAddress->second);
  }

  auto foundConnectionId = _connectionIdLookup.find(connectionId);
  if (foundConnectionId != _connectionIdLookup.end()) {
    PRINT_STREAM_WARNING("ChannelBase.AddConnection",
                         "Already registered connection id " << connectionId <<
                         " with existing address " << foundConnectionId->second.ToString() <<
                         "; will disconnect existing address and overwrite with address " << address << ".");
    
    // call that should invoke the correct disconnection methods
    RemoveConnectionForOverwrite(connectionId);
  }

  _addressLookup.emplace(address, connectionId);
  _connectionIdLookup.emplace(connectionId, address);
}

bool ChannelBase::AcceptIncomingConnection(ConnectionId connectionId, const TransportAddress& address)
{
  return false;
}

void ChannelBase::RefuseIncomingConnection(const TransportAddress& address)
{
  
}

void ChannelBase::RemoveConnection(ConnectionId connectionId)
{
  TransportAddress address;
  if (RemoveConnectionInternal(address, connectionId)) {
    ClearPacketsForAddressUntilNewestConnection(address);
  }
}

void ChannelBase::RemoveAllConnections()
{
  _addressLookup.clear();
  _connectionIdLookup.clear();
  _incomingPacketQueue.clear();
}

bool ChannelBase::GetConnectionId(ConnectionId& connectionId, const TransportAddress& address) const
{
  auto found = _addressLookup.find(address);
  if (found != _addressLookup.end()) {
    connectionId = found->second;
    return true;
  }
  return false;
}

bool ChannelBase::GetAddress(TransportAddress& address, ConnectionId connectionId) const
{
  auto found = _connectionIdLookup.find(connectionId);
  if (found != _connectionIdLookup.end()) {
    address = found->second;
    return true;
  }
  return false;
}

void ChannelBase::PushIncomingPacket(const IncomingPacket& packet)
{
  _incomingPacketQueue.push_back(packet);
}

void ChannelBase::EmplaceIncomingPacket(const IncomingPacket&& packet)
{
  _incomingPacketQueue.emplace_back(packet);
}

void ChannelBase::ReceiveData(const uint8_t *buffer, unsigned int bufferSize, const TransportAddress& sourceAddress)
{
  //f32 timestamp = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
  //PRINT_STREAM_WARNING("UnreliableUDPChannel.ReceiveData",
  //                     "RECEIVING DATA SIZE " << bufferSize << " FROM " << sourceAddress);

  // will set sourceId on peek
  EmplaceIncomingPacket(IncomingPacket(IncomingPacket::Tag::NormalMessage, buffer, bufferSize, DEFAULT_CONNECTION_ID, sourceAddress));
}

void ChannelBase::ClearPacketsForAddress(const TransportAddress& address)
{
  auto begin = _incomingPacketQueue.begin();
  auto end = _incomingPacketQueue.end();
  auto remove_from = std::remove_if(begin, end,
                 [address](const IncomingPacket& packet) -> bool
                 {
                   return (packet.sourceAddress == address);
                 });
  _incomingPacketQueue.erase(remove_from, end);
}

void ChannelBase::ClearPacketsForAddressUntilNewestConnection(const TransportAddress& address)
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
    if (current->tag == IncomingPacket::Tag::Disconnected && current->sourceAddress == address) {
      break;
    }
    
    past_last_disconnect = current;
  } while (past_last_disconnect != begin);
  
  // until past_last_disconnect, shift entries over, stomping over anything with specified address
  // ends with a gap between gap_start_location and past_last_disconnect of garbage data
  auto gap_start_location = std::remove_if(begin, past_last_disconnect,
                                           [address](const IncomingPacket& packet) -> bool
                                           {
                                             return (packet.sourceAddress ==  address);
                                           });
  
  // throw away the gap
  _incomingPacketQueue.erase(gap_start_location, past_last_disconnect);
}

void ChannelBase::ClearPacketsForAddressAfterEarliestDisconnect(const TransportAddress& address)
{
  auto begin = _incomingPacketQueue.begin();
  auto end = _incomingPacketQueue.end();
  
  // look for the iterator pointing to the first disconnect packet
  auto first_disconnect = std::find_if(begin, end,
               [address](const IncomingPacket& packet)
               {
                 return (packet.tag == IncomingPacket::Tag::Disconnected && packet.sourceAddress == address);
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
                                             return (packet.sourceAddress ==  address);
                                           });
  
  // throw away the gap
  _incomingPacketQueue.erase(gap_start_location, end);
}

void ChannelBase::RemoveConnectionForOverwrite(ConnectionId connectionId)
{
  TransportAddress address;
  if (RemoveConnectionInternal(address, connectionId)) {
    ClearPacketsForAddress(address);
  }
}

bool ChannelBase::RemoveConnectionInternal(TransportAddress& address, ConnectionId connectionId)
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
