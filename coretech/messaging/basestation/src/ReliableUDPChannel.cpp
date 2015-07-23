//
//  ReliableUDPChannel.cpp
//  Cozmo
//
//  Created by Mark Pauley on 8/17/12.
//  Copyright (c) 2012 Anki, Inc. All rights reserved.
//

#include "anki/messaging/basestation/ReliableUDPChannel.h"
#include "util/transport/reliableConnection.h"

#include "util/logging/logging.h"

using namespace Anki::Comms;

ReliableUDPChannel::ReliableUDPChannel()
: UnreliableUDPChannel()
, reliableTransport(&unreliableTransport, this)
{
}
ReliableUDPChannel::~ReliableUDPChannel()
{
}

bool ReliableUDPChannel::IsStarted() const
{
  std::lock_guard<std::recursive_mutex> guard(mutex);
  return isStarted;
}

bool ReliableUDPChannel::IsHost() const
{
  std::lock_guard<std::recursive_mutex> guard(mutex);
  return isHost;
}

Anki::Util::TransportAddress ReliableUDPChannel::GetHostingAddress() const
{
  std::lock_guard<std::recursive_mutex> guard(mutex);
  return hostingAddress;
}

void ReliableUDPChannel::StartClient()
{
  std::lock_guard<std::recursive_mutex> guard(mutex);

  ConfigureReliableTransport();

  Stop_Internal();

  if (!unreliableTransport.IsConnected()) {
    unreliableTransport.SetPort(0);
  }
  reliableTransport.StartClient();
  isStarted = true;
  isHost = true;
}

void ReliableUDPChannel::StartHost(const TransportAddress& bindAddress)
{
  std::lock_guard<std::recursive_mutex> guard(mutex);

  ConfigureReliableTransport();

  Stop_Internal();

  // TODO: This check shouldn't be necessary
  // but it's needed because StopClient doesn't actually do anything
  if (!unreliableTransport.IsConnected()) {
    // TODO: Bind to specific IP address
    //unreliableTransport.SetAddress(bindAddress.GetIPAddress());
    unreliableTransport.SetPort(bindAddress.GetIPPort());
  }

  reliableTransport.StartHost();
  isHost = true;
  isStarted = true;
}

void ReliableUDPChannel::Stop()
{
  std::lock_guard<std::recursive_mutex> guard(mutex);

  Stop_Internal();
}

void ReliableUDPChannel::Update()
{
  // Nothing to do; everything is done in threads.
}

bool ReliableUDPChannel::Send(const Anki::Comms::OutgoingPacket& packet)
{
  std::lock_guard<std::recursive_mutex> guard(mutex);

  TransportAddress address;
  if (!UnreliableUDPChannel::GetAddress(address, packet.destId)) {
    PRINT_STREAM_WARNING("ReliableUDPChannel.Send",
                         "No registered connected connection id " << packet.destId << ".");
    return false;
  }

  ConnectionData *data = GetConnectionData(address);
  // shouldn't be null
  if (data == nullptr) {
    PRINT_STREAM_WARNING("ReliableUDPChannel.Send",
                         "Cannot find state for connection id " << packet.destId << ". This indicates programmer error.");
    return false;
  }

  switch (data->state) {
    case ConnectionData::State::Connected:
      // fake sending and throw out packet, since we have a disconnect queued
      if (data->isDisconnectionQueued) {
        return true;
      }
      return SendDirect(packet);
    case ConnectionData::State::Disconnected:
      PRINT_STREAM_WARNING("ReliableUDPChannel.Send",
                           "Cannot send data to connection id " << packet.destId << " when disconnected.");
      return false;
    case ConnectionData::State::WaitingForConnectionResponse:
      // fake sending and throw out packet, since we have a disconnect queued
      if (data->isDisconnectionQueued) {
        return true;
      }
      data->connectionPackets.push_back(packet);
      return true;
    case ConnectionData::State::MustSendConnectionResponse:
      PRINT_STREAM_WARNING("ReliableUDPChannel.Send",
                           "Cannot send data to connection id " << packet.destId << " when expecting to send a connection response.");
      return false;
    default:
      assert(false);
      return false;
  }
}

bool ReliableUDPChannel::PopIncomingPacket(IncomingPacket& packet)
{
  std::lock_guard<std::recursive_mutex> guard(mutex);

  bool result = UnreliableUDPChannel::PopIncomingPacket(packet);
  if (result) {
    PostProcessIncomingPacket(packet);
  }
  return result;
}

bool ReliableUDPChannel::IsConnectionActive(ConnectionId connectionId) const
{
  std::lock_guard<std::recursive_mutex> guard(mutex);
  return UnreliableUDPChannel::IsConnectionActive(connectionId);
}

int32_t ReliableUDPChannel::CountActiveConnections() const
{
  std::lock_guard<std::recursive_mutex> guard(mutex);
  return UnreliableUDPChannel::CountActiveConnections();
}

void ReliableUDPChannel::AddConnection(ConnectionId connectionId, const TransportAddress& address)
{
  std::lock_guard<std::recursive_mutex> guard(mutex);

  // Check if connection already exists
  if (IsConnectionActive(connectionId)) {
    PRINT_STREAM_DEBUG("ReliableUDPChannel.AddConnection.AlreadyConnected",
                       "Already connected to connectionId " << connectionId);
    return;
  }

  // AddConnection should remove any old uses of connectionId and address
  // and send disconnects
  UnreliableUDPChannel::AddConnection(connectionId, address);
  assert(_connectionDataMapping.count(address) == 0);
  _connectionDataMapping.emplace(address, ConnectionData(ConnectionData::State::WaitingForConnectionResponse));

  reliableTransport.Connect(address);
}

// Responds to the initial handshake, allowing the connection.
// Only send in response to ConnectionRequest packets.
// The alternative is RefuseIncomingConnection.
bool ReliableUDPChannel::AcceptIncomingConnection(ConnectionId connectionId, const TransportAddress& address)
{
  std::lock_guard<std::recursive_mutex> guard(mutex);

  ConnectionData *data = GetConnectionData(address);
  if (data == nullptr) {
    PRINT_STREAM_WARNING("ReliableUDPChannel.AcceptIncomingConnection",
                         "Cannot accept connection; cannot determine connection state for address " << address.ToString() << ".");

    return false;
  }

  if (data->state != ConnectionData::State::MustSendConnectionResponse) {
    PRINT_STREAM_WARNING("ReliableUDPChannel.AcceptIncomingConnection",
                         "Cannot accept connection; not in correct state to accept address " << address.ToString() << ".");
    return false;
  }

  // Will disconnect old uses of connectionId and address
  UnreliableUDPChannel::AddConnection(connectionId, address);

  data->state = ConnectionData::State::Connected;
  if (!data->isDisconnectionQueued) {
    assert(data->isRealConnectionActive);
    reliableTransport.FinishConnection(address);
  }
  // fake that it worked if there's a disconnect queued
  return true;
}

// Responds to the initial handshake, disallowing the connection.
// Only send in response to ConnectionRequest packets.
// The alternative is AcceptIncomingConnection.
void ReliableUDPChannel::RefuseIncomingConnection(const TransportAddress& address)
{
  std::lock_guard<std::recursive_mutex> guard(mutex);

  ConnectionData *data = GetConnectionData(address);
  if (data == nullptr) {
    PRINT_STREAM_WARNING("ReliableUDPChannel.RefuseIncomingConnection",
                         "Cannot refuse connection; cannot determine connection state "
                         "for the connection to the specified address " << address.ToString() << ".");

    return;
  }

  if (data->state != ConnectionData::State::MustSendConnectionResponse) {
    PRINT_STREAM_WARNING("ReliableUDPChannel.RefuseIncomingConnection",
                         "Cannot refuse connection; not in correct state to refuse the "
                         "connection to the specified address " << address.ToString() << ".");
    return;
  }

  RemoveConnectionData(address, false);
}

void ReliableUDPChannel::RemoveConnection(ConnectionId connectionId)
{
  std::lock_guard<std::recursive_mutex> guard(mutex);

  TransportAddress address;
  if (!GetAddress(address, connectionId)) {
    return;
  }
  UnreliableUDPChannel::RemoveConnection(connectionId);
  RemoveConnectionData(address, false);
}

void ReliableUDPChannel::RemoveAllConnections()
{
  std::lock_guard<std::recursive_mutex> guard(mutex);

  UnreliableUDPChannel::RemoveAllConnections();

  // swap not actually necessary, but just in case Disconnect triggers unexpected callbacks
  std::unordered_map<TransportAddress, ConnectionData> dataMapping;
  std::swap(dataMapping, _connectionDataMapping);

  for (auto entry : dataMapping) {
    if (entry.second.isRealConnectionActive) {
      reliableTransport.Disconnect(entry.first);
    }
  }
}

bool ReliableUDPChannel::GetConnectionId(ConnectionId& connectionId, const TransportAddress& address) const
{
  std::lock_guard<std::recursive_mutex> guard(mutex);

  return UnreliableUDPChannel::GetConnectionId(connectionId, address);
}

bool ReliableUDPChannel::GetAddress(TransportAddress& address, ConnectionId connectionId) const
{
  std::lock_guard<std::recursive_mutex> guard(mutex);

  return UnreliableUDPChannel::GetAddress(address, connectionId);
}

uint32_t ReliableUDPChannel::GetMaxTotalBytesPerMessage() const
{
  std::lock_guard<std::recursive_mutex> guard(mutex);

  return reliableTransport.MaxTotalBytesPerMessage();
}

// NOTE: This method runs on a separate thread, and thus should never touch ConnectionData::state.
void ReliableUDPChannel::ReceiveData(const uint8_t *buffer, unsigned int bufferSize, const TransportAddress& sourceAddress)
{
  std::lock_guard<std::recursive_mutex> guard(mutex);

  if (buffer == Anki::Util::INetTransportDataReceiver::OnConnectRequest) {
    QueueDisconnectionIfActuallyConnected(sourceAddress, true);
    EmplaceIncomingPacket(IncomingPacket(IncomingPacket::Tag::ConnectionRequest, sourceAddress));
  }
  else if (buffer == Anki::Util::INetTransportDataReceiver::OnConnected) {
    QueueDisconnectionIfActuallyConnected(sourceAddress, true);
    EmplaceIncomingPacket(IncomingPacket(IncomingPacket::Tag::Connected, sourceAddress));

    ConnectionData *data = GetConnectionData(sourceAddress);
    if (data != nullptr) {
      std::vector<OutgoingPacket> packetsToSend;
      std::swap(packetsToSend, data->connectionPackets);

      for (const OutgoingPacket& packet : packetsToSend) {
        SendDirect(packet);
      }
    }
  }
  else if (buffer == Anki::Util::INetTransportDataReceiver::OnDisconnected) {
    QueueDisconnectionIfActuallyConnected(sourceAddress, false);
  }
  else {
    // can only process normal messages if we're supposed to be connected
    ConnectionData *data = GetConnectionData(sourceAddress);
    if (data == nullptr || data->isDisconnectionQueued) {
      PRINT_STREAM_WARNING("ReliableUDPChannel.ReceiveData",
                           "Received unexpected normal messages from address " << sourceAddress.ToString() << ".");
      return;
    }
    assert(data->isRealConnectionActive);

    UnreliableUDPChannel::ReceiveData(buffer, bufferSize, sourceAddress);
  }
}

void ReliableUDPChannel::ConfigureReliableTransport()
{
    // Set parameters for all reliable transport instances
    Util::ReliableTransport::SetSendAckOnReceipt(false);
    Util::ReliableTransport::SetSendUnreliableMessagesImmediately(false);
    Util::ReliableTransport::SetSendPacketsImmediately(false);
    Util::ReliableTransport::SetSendLatestPacketOnSendMessage(true);
    Util::ReliableTransport::SetMaxPacketsToReSendOnAck(1);
    Util::ReliableTransport::SetMaxPacketsToSendOnSendMessage(1);
    // Set parameters for all reliable connections
    Util::ReliableConnection::SetTimeBetweenPingsInMS(33.3); // Heartbeat interval
    Util::ReliableConnection::SetTimeBetweenResendsInMS(133.3); // 4x heartbeat interval
    Util::ReliableConnection::SetConnectionTimeoutInMS(5000.0);
    Util::ReliableConnection::SetMaxPingRoundTripsToTrack(10);
    Util::ReliableConnection::SetSendSeparatePingMessages(false);
    Util::ReliableConnection::SetMaxPacketsToReSendOnUpdate(1);
}

bool ReliableUDPChannel::SendDirect(const OutgoingPacket& packet)
{
  // TODO: Do not hold lock.

  TransportAddress destAddress;
  if (!UnreliableUDPChannel::GetAddress(destAddress, packet.destId)) {
    PRINT_STREAM_WARNING("ReliableUDPChannel.SendDirect",
                         "Cannot determine address for connection id " << packet.destId << ".");
    return false;
  }

  reliableTransport.SendData(packet.reliable, destAddress, packet.buffer, packet.bufferSize, packet.hot);
  return true;
}

void ReliableUDPChannel::Stop_Internal()
{
  if (isStarted) {
    RemoveAllConnections();

    bool wasHost = isHost;
    isStarted = false;
    isHost = false;

    if (wasHost) {
      reliableTransport.StopHost();
    }
    else {
      reliableTransport.StopClient();
    }
  }
}

ReliableUDPChannel::ConnectionData *ReliableUDPChannel::GetConnectionData(const TransportAddress& address)
{
  auto found = _connectionDataMapping.find(address);
  if (found != _connectionDataMapping.end()) {
    return &found->second;
  }
  return nullptr;
}

// mainly sets packet.sourceId
// also used for verification and state changes
void ReliableUDPChannel::PostProcessIncomingPacket(IncomingPacket& packet)
{
  ConnectionData *data = GetConnectionData(packet.sourceAddress);
  // indicates a problem within this class implementation
  if (data == nullptr) {
    PRINT_STREAM_WARNING("ReliableUDPChannel.PostProcessIncomingPacket",
                         "No connection data available for a queued packet from address " << packet.sourceAddress.ToString() <<
                         ". Disconnecting. (This shouldn't happen.)");

    reliableTransport.Disconnect(packet.sourceAddress);
    return;
  }

  // asserts are for state that is queued by this class, so any error is an error within this class
  switch (packet.tag) {
    case IncomingPacket::Tag::ConnectionRequest:
      assert(data->state == ConnectionData::State::Disconnected);
      assert(!data->isDisconnectionQueued);

      data->state = ConnectionData::State::MustSendConnectionResponse;
      break;
    case IncomingPacket::Tag::Connected:
      assert(data->state == ConnectionData::State::Disconnected);
      assert(!data->isDisconnectionQueued);

      data->state = ConnectionData::State::Connected;
      break;
    case IncomingPacket::Tag::Disconnected:
      assert(data->state == ConnectionData::State::Connected ||
             data->state == ConnectionData::State::MustSendConnectionResponse ||
             data->state == ConnectionData::State::WaitingForConnectionResponse);
      assert(data->isDisconnectionQueued);

      data->state = ConnectionData::State::Disconnected;
      if (!data->isRealConnectionActive) {
        _connectionDataMapping.erase(packet.sourceAddress);
      }
      else {
        data->isDisconnectionQueued = false;
      }
      break;
    case IncomingPacket::Tag::NormalMessage:
      //assert(data->state == ConnectionData::State::Connected);
      break;
    default:
      assert(false);
      break;
  }
}

// Removes the connection without locking.
void ReliableUDPChannel::RemoveConnectionForOverwrite(ConnectionId connectionId)
{
  TransportAddress address;
  if (!GetAddress(address, connectionId)) {
    return;
  }

  UnreliableUDPChannel::RemoveConnectionForOverwrite(connectionId);
  RemoveConnectionData(address, true);
}

// Removes the ConnectionData struct.
// if full is set, it always wipes it and the current connection full
// if full is not set, it will try to leave a queued connection in the queue
void ReliableUDPChannel::RemoveConnectionData(const TransportAddress& address, bool full)
{
  auto foundData = _connectionDataMapping.find(address);
  if (foundData == _connectionDataMapping.end()) {
    return;
  }
  ConnectionData *data = &foundData->second;

  // clear any queued packets past the last incoming disconnect
  if (data->isDisconnectionQueued && !full) {
    ClearPacketsForAddressUntilNewestConnection(address);

    if (!data->isRealConnectionActive) {
      _connectionDataMapping.erase(foundData);
    }
    else {
      data->isDisconnectionQueued = false;
      data->connectionPackets.clear();
    }
  }
  else {
    assert(data->isRealConnectionActive || full);
    ClearPacketsForAddress(address);

    // if no disconnect is queued, the connection is current and we need to disconnect
    if (data->isRealConnectionActive) {
      reliableTransport.Disconnect(address);
    }
    _connectionDataMapping.erase(foundData);
  }
}

void ReliableUDPChannel::QueueDisconnectionIfActuallyConnected(const TransportAddress& sourceAddress, bool makeActive)
{
  ConnectionData *data = GetConnectionData(sourceAddress);

  if (data == nullptr) {
    if (makeActive) {
      _connectionDataMapping.emplace(sourceAddress, ConnectionData(ConnectionData::State::Disconnected));
    }
  }
  else if (data->isDisconnectionQueued) {
    ClearPacketsForAddressAfterEarliestDisconnect(sourceAddress);
  }
  else if (data->isRealConnectionActive) {
    EmplaceIncomingPacket(IncomingPacket(IncomingPacket::Tag::Disconnected, sourceAddress));
    data->isDisconnectionQueued = true;
  }
  else {
    // should be true or data should not exist
    assert(data->isDisconnectionQueued || data->isRealConnectionActive);
  }
  data->isRealConnectionActive = makeActive;
  data->connectionPackets.clear();
}
