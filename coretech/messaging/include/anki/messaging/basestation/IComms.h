//
//  IComms.h
//  RushHour
//
//  Created by Mark Pauley on 8/17/12.
//  Copyright (c) 2012 Anki, Inc. All rights reserved.
//

#ifndef __IComms__
#define __IComms__

#include "anki/common/types.h"
#include "anki/common/basestation/exceptions.h"

#include "anki/util/logging/logging.h"
#include "anki/util/transport/iNetTransportDataReceiver.h"
#include "anki/util/transport/transportAddress.h"

#include "anki/util/transport/iNetTransportDataReceiver.h"
#include "anki/util/transport/srcBufferSet.h"
#include "anki/util/transport/transportAddress.h"
#include "anki/util/transport/reliableTransport.h"
#include "anki/util/transport/udpTransport.h"

#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace Anki {
  namespace Comms {

    // The max size in one BLE packet
    #define MAX_BLE_MSG_SIZE        20
    #define MIN_BLE_MSG_SIZE        2
    
    struct PacketBase {
      PacketBase() { }
      PacketBase(const uint8_t *bufferData, unsigned int bufferDataSize)
      {
        if (bufferDataSize > MAX_SIZE) {
          PRINT_STREAM_WARNING("PacketBase",
                               "Truncating packet size because packet is larger than buffer.");
        }
        
        this->bufferSize = std::min(MAX_SIZE, bufferDataSize);
        std::memcpy(this->buffer, bufferData, bufferDataSize);
      }
      
      // The outgoing buffer.
      static const unsigned int MAX_SIZE = 2000;
      uint8_t buffer[MAX_SIZE];
      unsigned int bufferSize = 0;
    };
    
    // A packet on its way out.
    struct OutgoingPacket : PacketBase {
      OutgoingPacket()
      {
      }
      OutgoingPacket(const uint8_t *bufferData, unsigned int bufferDataSize, s32 destId, bool reliable = true, bool hot = false)
      : PacketBase(bufferData, bufferDataSize), destId(destId), reliable(reliable), hot(hot)
      {
      }
      
      // The destination connection ID.
      s32 destId = -1;
      
      // Whether to send this reliable or not.
      bool reliable = true;
      
      // Whether to send this as a "hot" packet that should never be buffered on send.
      bool hot = false;
    };
    
    // A packet on its way in.
    struct IncomingPacket : PacketBase {
      enum class Tag {
        // A normal message, with buffer.
        NormalMessage,
        // A disconnection triggered internally or by the other side.
        Disconnected,
        
        // A request for a new connection. Upon receipt, you should always call AcceptIncomingConnection or RefuseIncomingConnection!
        ConnectionRequest,
        // A new connection confirmed by the other side.
        Connected,
      };
      
      IncomingPacket()
      {
      }
      IncomingPacket(Tag tag, const Anki::Util::TransportAddress& sourceAddress)
      : tag(tag)
      , sourceAddress(sourceAddress)
      {
      }
      IncomingPacket(Tag tag, const uint8_t *bufferData, unsigned int bufferDataSize, s32 sourceId, const Anki::Util::TransportAddress& sourceAddress)
      : PacketBase(bufferData, bufferDataSize)
      , tag(tag)
      , sourceId(sourceId)
      , sourceAddress(sourceAddress)
      {
      }
      
      // The type of this packet. Only NormalMessage actually uses the buffer.
      Tag tag;
      
      // The source connection ID.
      s32 sourceId = -1;
      
      // The TransportAddress of the connection.
      Anki::Util::TransportAddress sourceAddress;
    };
    
    class IComms {
    public:
      
      virtual ~IComms() {};
      
      virtual void Update() = 0;
      
      // Sends a packet to the packet's destId.
      // Returns false if it cannot send for some reason.
      // A true result does not ensure delivery.
      virtual bool Send(const OutgoingPacket& packet) = 0;
      
      // Checks to see if there is an incoming packet without popping it from the queue.
      // Returns true and sets the packet if there is one.
      virtual bool PeekIncomingPacket(IncomingPacket& packet) = 0;
      
      // Pops the next incoming packet from the queue.
      // Returns true and sets the packet if there is one, discarding it from the queue.
      virtual bool PopIncomingPacket(IncomingPacket& packet) = 0;
      
      // Returns true if there is a connection with the specified id.
      // May not actually be connected, eg while connecting or for connectionless protocols.
      // On internal disconnect, will not change state until Disconnection packet is seen.
      virtual bool IsConnectionActive(s32 connectionId) const = 0;
      
      // Manually adds a connection to a specified address.
      // Should warn, but allow, overriding an old connection.
      // Some IComms implementations may have other ways of adding connections or auto-determining ids. This is sometimes just used for force-adding.
      virtual void AddConnection(s32 connectionId, const Anki::Util::TransportAddress& address) = 0;
      // Manually removes a connection, completely cleaning it up, including disconnecting.
      // Will already be removed if you get a disconnection notification.
      // It is OK to remove a non-existent connection. (Makes it easier to deal with disconnects.)
      // Removing the connection will discard all incoming packets for the specified connectionId.
      virtual void RemoveConnection(s32 connectionId) = 0;
      // Manually remove all connections, completely cleaning them up, including disconnecting.
      // Clears all incoming packets as well, even those not associated with a connectionId.
      virtual void RemoveAllConnections() = 0;
      
      // Figures out the ConnectionId from a given TransportAddress.
      virtual bool GetConnectionId(s32& connectionId, const Anki::Util::TransportAddress& address) const = 0;
      
      // Figures out the TransportAddress from a given ConnectionId.
      virtual bool GetAddress(Anki::Util::TransportAddress& address, s32 connectionId) const = 0;
      
      // The maximum number of bytes you can send.
      // May return 0, meaning limit unknown.
      virtual uint32_t MaxTotalBytesPerMessage() const = 0;
    };
    
    // Standard IComms base class that implements some basic connection features.
    // Provides a protected implementation of INetTransportDataReceiver so implementers
    // can pass "this" to various callback setters.
    class CommsBase: public IComms, protected Anki::Util::INetTransportDataReceiver {
    public:
      
      virtual ~CommsBase() override { }
      
      virtual bool PeekIncomingPacket(IncomingPacket& packet) override
      {
        if (!_incomingPacketQueue.empty())
        {
          packet = _incomingPacketQueue.front();
          
          // assign address if there is one
          // may return false and still leave it as -1
          s32 sourceId = -1;
          GetConnectionId(sourceId, packet.sourceAddress);
          packet.sourceId = sourceId;
          
          return true;
        }
        return false;
      }
      
      virtual bool PopIncomingPacket(IncomingPacket& packet) override
      {
        bool result = PeekIncomingPacket(packet);
        if (result) {
          _incomingPacketQueue.pop_front();
        }
        return result;
      }
      
      // adds both entries to the bidirectional mapping, removing any duplicates
      // assumes a consistent state
      virtual void AddConnection(s32 connectionId, const Anki::Util::TransportAddress& address) override
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
          
          // virtual call that should invoke the correct disconnection methods
          RemoveConnectionForOverwrite(foundAddress->second);
        }

        auto foundConnectionId = _connectionIdLookup.find(connectionId);
        if (foundConnectionId != _connectionIdLookup.end()) {
          PRINT_STREAM_WARNING("CommsBase.AddConnection",
                               "Already registered connection id " << connectionId <<
                               " with existing address " << foundConnectionId->second.ToString() <<
                               "; will disconnect existing address and overwrite with address " << address.ToString() << ".");
          
          // virtual call that should invoke the correct disconnection methods
          RemoveConnectionForOverwrite(connectionId);
        }

        _addressLookup.emplace(address, connectionId);
        _connectionIdLookup.emplace(connectionId, address);
      }

      // NOTE: Base class implementation does not actually do the disconnection part.
      virtual void RemoveConnection(s32 connectionId) override
      {
        auto found = _connectionIdLookup.find(connectionId);
        if (found == _connectionIdLookup.end()) {
          return;
        }
        Anki::Util::TransportAddress address = found->second;
        _addressLookup.erase(address);
        _connectionIdLookup.erase(found);

        ClearPacketsUntilNextDisconnect(address);
      }

      // NOTE: Base class implementation does not actually do the disconnection part.
      virtual void RemoveAllConnections() override
      {
        _addressLookup.clear();
        _connectionIdLookup.clear();
        _incomingPacketQueue.clear();
      }

      virtual bool GetConnectionId(s32& connectionId, const Anki::Util::TransportAddress& address) const override
      {
        auto found = _addressLookup.find(address);
        if (found != _addressLookup.end()) {
          connectionId = found->second;
          return true;
        }
        return false;
      }
      
      virtual bool GetAddress(Anki::Util::TransportAddress& address, s32 connectionId) const override
      {
        auto found = _connectionIdLookup.find(connectionId);
        if (found != _connectionIdLookup.end()) {
          address = found->second;
          return true;
        }
        return false;
      }

    protected:
      void PushIncomingPacket(const IncomingPacket& packet)
      {
        _incomingPacketQueue.push_back(packet);
      }

      void EmplaceIncomingPacket(const IncomingPacket&& packet)
      {
        _incomingPacketQueue.emplace_back(packet);
      }

      // You should override this if you need any special handling for say, disconnects.
      virtual void ReceiveData(const uint8_t *buffer, unsigned int bufferSize, const Anki::Util::TransportAddress& sourceAddress) override
      {
        //f32 timestamp = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();

        // will set sourceId on peek
        s32 sourceId = -1;
        EmplaceIncomingPacket(IncomingPacket(IncomingPacket::Tag::NormalMessage, buffer, bufferSize, sourceId, sourceAddress));
      }
      
      void ClearPacketsUntilNextDisconnect(const Anki::Util::TransportAddress& address)
      {
        // Get rid of packets up to and including the next disconnect
        auto move_location = _incomingPacketQueue.begin();
        auto end = _incomingPacketQueue.end();
        auto iterator = move_location;
        while (iterator != end) {
          // no equality operator defined, so use less-than
          // cannot use sourceId, since it's not yet set
          if (iterator->sourceAddress < address || address < iterator->sourceAddress) {
            *move_location = std::move(*iterator);
            ++move_location;
          }
          IncomingPacket::Tag tag = iterator->tag;
          ++iterator;
          
          // stop at first disconnection packet, because after this it's a whole new connection
          if (tag == IncomingPacket::Tag::Disconnected) {
            break;
          }
        }
        // remove the gap
        _incomingPacketQueue.erase(move_location, iterator);
      }

      // Equivalent semantics to RemoveConnection, which it calls by default.
      // This version is only called internally, when it is about to be reused.
      virtual void RemoveConnectionForOverwrite(s32 connectionId)
      {
        RemoveConnection(connectionId);
      }

    private:
      std::map<Anki::Util::TransportAddress, s32> _addressLookup;
      std::unordered_map<s32, Anki::Util::TransportAddress> _connectionIdLookup;
      std::deque<IncomingPacket> _incomingPacketQueue;
    };

    // Unreliable communications channel.
    // Unsolicited connections will be assigned id -1.
    class UnreliableUDPChannel: public Anki::Comms::CommsBase {
    public:
      
      UnreliableUDPChannel()
      {
        // TODO ANDROID: SET IP RETRIEVER
        //unreliableTransport.SetIpRetriever(nullptr);
        unreliableTransport.SetDataReceiver(this);
      }
      virtual ~UnreliableUDPChannel() override { }

      // Determines whether one of the Start methods has been called.
      virtual bool IsStarted() const
      {
        return isStarted;
      }

      // Determines whether the channel has been started as a host.
      virtual bool IsHost() const
      {
        return isHost;
      }

      virtual void StartClient()
      {
        Stop();
        
        unreliableTransport.StartClient();
        isHost = false;
        isStarted = true;
      }

      virtual void StartHost(const Anki::Util::TransportAddress& bindAddress)
      {
        Stop();
        
        // TODO: This check shouldn't be necessary
        // but it's needed because StopClient doesn't actually do anything
        if (!unreliableTransport.IsConnected()) {
          // TODO: Bind to specific IP address
          //unreliableTransport.SetAddress(bindAddress.GetIPAddress());
          unreliableTransport.SetPort(bindAddress.GetIPPort());
        }
        
        unreliableTransport.StartHost();
        isHost = true;
        isStarted = true;
      }

      virtual void Stop()
      {
        if (isStarted) {
          RemoveAllConnections();
          
          bool wasHost = isHost;
          isStarted = false;
          isHost = false;
          
          if (wasHost) {
            unreliableTransport.StopHost();
          }
          else {
            unreliableTransport.StopClient();
          }
        }
      }

      virtual void Update() override
      {
        unreliableTransport.Update();
      }

      virtual bool Send(const Anki::Comms::OutgoingPacket& packet) override
      {
        if (!isStarted) {
          PRINT_STREAM_WARNING("UnreliableUDPChannel.Send",
                               "Channel is not started.");
          return false;
        }

        Anki::Util::TransportAddress address;
        if (!GetAddress(address, packet.destId)) {
          PRINT_STREAM_WARNING("UnreliableUDPChannel.Send",
                               "Cannot determine address for connection id " << packet.destId << ".");
          return false;
        }

        Anki::Util::SrcBufferSet srcBuffers;
        srcBuffers.AddBuffer(Anki::Util::SizedSrcBuffer(packet.buffer, packet.bufferSize));
        unreliableTransport.SendData(address, srcBuffers);
        return true;
      }

      virtual uint32_t MaxTotalBytesPerMessage() const override
      {
        return unreliableTransport.MaxTotalBytesPerMessage();
      }

    protected:
      bool isStarted = false;
      bool isHost = false;
      Anki::Util::UDPTransport unreliableTransport;
    };

  }
}

namespace Anki {
  namespace Comms {

    // Reliable communications channel.
    // All IDs that are passed back will definitely be valid, except for ConnectionRequest.
    class ReliableUDPChannel: public UnreliableUDPChannel {
    public:
      
      ReliableUDPChannel()
      : UnreliableUDPChannel()
      , reliableTransport(&unreliableTransport, this)
      {
      }
      virtual ~ReliableUDPChannel() override
      {
      }
      
      virtual void StartClient() override
      {
        std::lock_guard<std::mutex> guard(mutex);
        
        Stop_Internal();
        reliableTransport.StartClient();
        isStarted = true;
        isHost = true;
      }
      
      virtual void StartHost(const Anki::Util::TransportAddress& bindAddress) override
      {
        std::lock_guard<std::mutex> guard(mutex);
        
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
      
      virtual void Stop() override
      {
        std::lock_guard<std::mutex> guard(mutex);
        
        Stop_Internal();
      }
      
      virtual void Update() override
      {
        // Nothing to do; everything is done in threads.
      }
      
      virtual bool Send(const Anki::Comms::OutgoingPacket& packet) override
      {
        std::lock_guard<std::mutex> guard(mutex);
        
        Anki::Util::TransportAddress address;
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
        
        if (data->state != ConnectionData::State::Connected && data->state == ConnectionData::State::WaitingForConnectionResponse) {
          // disconnected should be impossible, so expecting to send
          PRINT_STREAM_WARNING("ReliableUDPChannel.Send",
                               "Cannot send data to connection id " << packet.destId << " when expecting to send a connection response.");
          return false;
        }
        else if (data->numberOfDisconnectsQueued != 0) {
          return false;
        }
        else if (data->state == ConnectionData::State::Connected) {
          return SendDirect(packet);
        }
        else {
          data->connectionPackets.push_back(packet);
          return true;
        }
      }
      
      virtual bool PeekIncomingPacket(IncomingPacket& packet) override
      {
        std::lock_guard<std::mutex> guard(mutex);
        
        bool result = UnreliableUDPChannel::PeekIncomingPacket(packet);
        if (result) {
          PostProcessIncomingPacket(packet);
          _processNextPacket = false;
        }
        return result;
      }
      
      virtual bool PopIncomingPacket(IncomingPacket& packet) override
      {
        std::lock_guard<std::mutex> guard(mutex);
        
        bool result = UnreliableUDPChannel::PopIncomingPacket(packet);
        if (result) {
          PostProcessIncomingPacket(packet);
          _processNextPacket = true;
        }
        return result;
      }
      
      virtual bool IsConnectionActive(s32 connectionId) const override
      {
        std::lock_guard<std::mutex> guard(mutex);
        return UnreliableUDPChannel::IsConnectionActive(connectionId);
      }
      
      virtual void AddConnection(s32 connectionId, const Anki::Util::TransportAddress& address) override
      {
        std::lock_guard<std::mutex> guard(mutex);
        
        // AddConnection should remove any old uses of connectionId and address
        // and send disconnects
        UnreliableUDPChannel::AddConnection(connectionId, address);
        _connectionDataMapping.emplace(address, ConnectionData(ConnectionData::State::WaitingForConnectionResponse));
        
        reliableTransport.Connect(address);
      }
      
      // Responds to the initial handshake, allowing the connection.
      // Only send in response to ConnectionRequest packets.
      // The alternative is RefuseIncomingConnection.
      virtual void AcceptIncomingConnection(s32 connectionId, const Anki::Util::TransportAddress& address)
      {
        std::lock_guard<std::mutex> guard(mutex);
        
        ConnectionData *data = GetConnectionData(address);
        if (data == nullptr) {
          PRINT_STREAM_WARNING("ReliableUDPChannel.AcceptIncomingConnection",
                               "Cannot accept connection; cannot determine connection state for address " << address.ToString() << ".");
          
          return;
        }
        
        if (data->state != ConnectionData::State::MustSendConnectionResponse) {
          PRINT_STREAM_WARNING("ReliableUDPChannel.AcceptIncomingConnection",
                               "Cannot accept connection; not in correct state to accept address " << address.ToString() << ".");
          return;
        }
        
        // Will disconnect old uses of connectionId
        UnreliableUDPChannel::AddConnection(connectionId, address);
        
        data->state = ConnectionData::State::Connected;
        if (data->numberOfDisconnectsQueued == 0 && data->actuallyActive) {
          reliableTransport.FinishConnection(address);
        }
      }
      
      // Responds to the initial handshake, disallowing the connection.
      // Only send in response to ConnectionRequest packets.
      // The alternative is AcceptIncomingConnection.
      virtual void RefuseIncomingConnection(const Anki::Util::TransportAddress& address)
      {
        std::lock_guard<std::mutex> guard(mutex);
        
        ConnectionData *data = GetConnectionData(address);
        if (data == nullptr) {
          PRINT_STREAM_WARNING("ReliableUDPChannel.AcceptIncomingConnection",
                               "Cannot refuse connection; cannot determine connection state for address " << address.ToString() << ".");
          
          return;
        }
        
        if (data->state != ConnectionData::State::MustSendConnectionResponse) {
          PRINT_STREAM_WARNING("ReliableUDPChannel.AcceptIncomingConnection",
                               "Cannot refuse connection; not in correct state to refuse address " << address.ToString() << ".");
          return;
        }
        
        if (data->numberOfDisconnectsQueued == 0) {
          // (actuallyActive is implied by numberOfDisconnectsQueued == 0)
          assert(data->actuallyActive);
          _connectionDataMapping.erase(address);
          reliableTransport.Disconnect(address);
        }
        else {
          data->state = ConnectionData::State::Disconnected;
          data->numberOfDisconnectsQueued -= 1;
          // should be unnecessary, though harmless, here
          data->connectionPackets.clear();
          
          // not calling RemoveConnection, so we need to clean packets here
          ClearPacketsUntilNextDisconnect(address);
        }
      }
      
      virtual void RemoveConnection(s32 connectionId) override
      {
        std::lock_guard<std::mutex> guard(mutex);
        
        Anki::Util::TransportAddress address;
        // OK to attempt to remove connection ids that don't actually exist
        if (!UnreliableUDPChannel::GetAddress(address, connectionId)) {
          return;
        }
        
        UnreliableUDPChannel::RemoveConnection(connectionId);
        
        ConnectionData *data = GetConnectionData(address);
        if (data == nullptr) {
          return;
        }
        
        // should be impossible to be in these states
        assert(data->state != ConnectionData::State::Disconnected);
        assert(data->state != ConnectionData::State::MustSendConnectionResponse);
        
        if (data->numberOfDisconnectsQueued == 0) {
          // (actuallyActive is implied by numberOfDisconnectsQueued == 0)
          assert(data->actuallyActive);
          _connectionDataMapping.erase(address);
          reliableTransport.Disconnect(address);
        }
        else {
          data->state = ConnectionData::State::Disconnected;
          data->numberOfDisconnectsQueued -= 1;
          data->connectionPackets.clear();
        }
      }
      
      virtual void RemoveAllConnections() override
      {
        std::lock_guard<std::mutex> guard(mutex);
        
        UnreliableUDPChannel::RemoveAllConnections();
        std::map<Anki::Util::TransportAddress, ConnectionData> dataMapping;
        std::swap(dataMapping, _connectionDataMapping);
        for (auto entry : dataMapping) {
          if (entry.second.actuallyActive) {
            reliableTransport.Disconnect(entry.first);
          }
        }
      }
      
      virtual bool GetConnectionId(s32& connectionId, const Anki::Util::TransportAddress& address) const override
      {
        std::lock_guard<std::mutex> guard(mutex);
        
        return UnreliableUDPChannel::GetConnectionId(connectionId, address);
      }
      
      virtual bool GetAddress(Anki::Util::TransportAddress& address, s32 connectionId) const override
      {
        std::lock_guard<std::mutex> guard(mutex);
        
        return UnreliableUDPChannel::GetAddress(address, connectionId);
      }
      
      virtual uint32_t MaxTotalBytesPerMessage() const override
      {
        std::lock_guard<std::mutex> guard(mutex);
        
        return reliableTransport.MaxTotalBytesPerMessage();
      }
      
    protected:
      
      virtual void ReceiveData(const uint8_t *buffer, unsigned int bufferSize, const Anki::Util::TransportAddress& sourceAddress) override
      {
        std::lock_guard<std::mutex> guard(mutex);
        
        auto result = _connectionDataMapping.emplace(sourceAddress, ConnectionData());
        ConnectionData& data = result.first->second;
        
        if (buffer == Anki::Util::INetTransportDataReceiver::OnConnectRequest) {
          if (data.actuallyActive) {
            PRINT_STREAM_WARNING("ReliableUDPChannel.ReceiveData",
                                 "Connection request to active address " << sourceAddress.ToString() <<
                                 ". Disconnecting. (This shouldn't happen).");
            
            reliableTransport.Disconnect(sourceAddress);
          }
          else {
            data.actuallyActive = true;
            EmplaceIncomingPacket(IncomingPacket(IncomingPacket::Tag::ConnectionRequest, sourceAddress));
          }
        }
        else if (buffer == Anki::Util::INetTransportDataReceiver::OnConnected) {
          HandleSuccessfulConnection(sourceAddress);
        }
        else if (buffer == Anki::Util::INetTransportDataReceiver::OnDisconnected) {
          HandleDisconnection(sourceAddress);
        }
        else {
          UnreliableUDPChannel::ReceiveData(buffer, bufferSize, sourceAddress);
        }
      }
      
      virtual void HandleConnectionRequest(const Anki::Util::TransportAddress& sourceAddress)
      {
        ConnectionData data;
        if (GetConnectionData(data, sourceAddress)) {
          data.actuallyActive = true;
        }
        
        EmplaceIncomingPacket(IncomingPacket(IncomingPacket::Tag::ConnectionRequest, sourceAddress));
        
        
        // assign address if there is one
        // may return false and still leave it as -1
        s32 oldSourceId = -1;
        if (!GetConnectionId(oldSourceId, sourceAddress)) {
          PRINT_STREAM_WARNING("ReliableUDPChannel.HandleConnectionRequest",
                               "Connection request to active address " << sourceAddress.ToString() <<
                               ", which is currently assigned to connection id " << oldSourceId <<
                               ". Disconnecting. (This should be incredibly rare).");
          
          s32 sourceId;
          if (GetConnectionId(sourceId, sourceAddress)) {
            RemoveConnnection(sourceId);
          }
          else {
            reliableTransport.Disconnect(sourceAddress);
          }
          return;
        }
        
        EmplaceIncomingPacket(IncomingPacket(IncomingPacket::Tag::ConnectionRequest));
        
        
        
        
        
        s32 newSourceId = -1;
        if (incomingConnectionCondition && incomingConnectionCondition(newSourceId, sourceAddress)) {
          
          // if it already exists, we need to stomp it
          Anki::Util::TransportAddress oldAddress;
          if (GetAddress(oldAddress, sourceId)) {
            PRINT_STREAM_WARNING("ReliableUDPChannel.HandleConnectionRequest",
                                 "Already registered connection id " << newSourceId <<
                                 " with existing address " << oldAddress.ToString() <<
                                 "; will disconnect existing address and overwrite with incoming address " << sourceAddress.ToString() << ".");
            RemoveConnection(newSourceId);
          }
          
          // call default version that doesn't actually connect
          CommsBase::AddConnection(newSourceId, sourceAddress);
          _connectionDataMapping.emplace(newSourceId, ConnectionData(ConnectionData::State::Connected));
          
          reliableTransport.FinishConnection(sourceAddress);
        }
        else {
          PRINT_STREAM_DEBUG("ReliableUDPChannel.HandleConnectionRequest",
                             "Refusing incoming connection from " << sourceAddress.ToString() << ".");
          
          reliableTransport.Disconnect(sourceAddress);
        }
      }
      
      virtual void HandleSuccessfulConnection(const Anki::Util::TransportAddress& sourceAddress)
      {
        
        
        
        // if we ever disconnected, we should have also cleaned this data up
        s32 sourceId;
        if (!GetConnectionId(sourceId, sourceAddress)) {
          PRINT_STREAM_DEBUG("ReliableUDPChannel.HandleConnectionRequest",
                             "Got unexpected successful connection from " << sourceAddress.ToString() <<
                             ". Disconnecting. This indicates programmer error");
          
          reliableTransport.Disconnect(sourceAddress);
          return;
        }
        
        auto foundData = _connectionDataMapping.find(sourceId);
        if (foundData == _connectionDataMapping.end()) {
          PRINT_STREAM_DEBUG("ReliableUDPChannel.HandleConnectionRequest",
                             "No connection state on connection id " << sourceId <<
                             " address " << sourceAddress.ToString() <<
                             ". Disconnecting. This indicates programmer error.");
          
          RemoveConnection(sourceId);
          return;
        }
        
        foundData->second.state = ConnectionData::State::Connected;
        std::vector<OutgoingPacket> packets;
        std::swap(packets, foundData->second.connectionPackets);
        for (const OutgoingPacket& packet: packets) {
          SendDirect(packet);
        }
        
        if (successfulConnectionHandler) {
          successfulConnectionHandler(sourceId, sourceAddress);
        }
      }
      
      virtual void HandleDisconnection(const Anki::Util::TransportAddress& sourceAddress)
      {
        // we should have data on this connection if we get a disconnect call
        s32 sourceId;
        if (!GetConnectionId(sourceId, sourceAddress)) {
          PRINT_STREAM_DEBUG("ReliableUDPChannel.HandleConnectionRequest",
                             "Got unexpected disconnection from " << sourceAddress.ToString() <<
                             ". Ignoring. This indicates programmer error.");
          return;
        }
        
        // call default version that doesn't actually disconnect
        CommsBase::RemoveConnection(sourceId);
        _connectionDataMapping.erase(sourceId);
        
        if (disconnectionHandler) {
          disconnectionHandler(sourceId, sourceAddress);
        }
      }
      
      virtual void HandleNormalMessage(const uint8_t* buffer, unsigned int bufferSize, const Anki::Util::TransportAddress& sourceAddress)
      {
        // assign address if there is one
        // may return false and still leave it as -1
        s32 sourceId = -1;
        if (!GetConnectionId(sourceId, sourceAddress)) {
          
        }
        
        if (packetHandler) {
          //f32 timestamp = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
          packetHandler(IncomingPacket(buffer, bufferSize, sourceId, sourceAddress));
        }
      }
      
      std::function<void(s32, const Anki::Util::TransportAddress&)> successfulConnectionHandler;
      std::function<bool(s32&, const Anki::Util::TransportAddress&)> incomingConnectionCondition;
      Anki::Util::ReliableTransport reliableTransport;
      // need recursive for RemoveConnection to not be a pain
      mutable std::mutex mutex;
      
    private:
      struct ConnectionData {
      public:
        enum class State {
          WaitingForConnectionResponse,
          MustSendConnectionResponse,
          Connected,
          Disconnected,
        };
        
        ConnectionData()
        : state(State::Disconnected)
        , numberOfDisconnectsQueued(0)
        , actuallyActive(false)
        {
        }
        
        ConnectionData(State state)
        : state(state)
        , numberOfDisconnectsQueued(0)
        , actuallyActive(true)
        {
        }
        
        // The state as known to the user
        State state;
        // Number of disconnects coming up in the incoming queue.
        u32 numberOfDisconnectsQueued;
        // Whether the reliableTransport thinks the connection is alive
        bool actuallyActive;
        std::vector<OutgoingPacket> connectionPackets;
      };
      
      bool SendDirect(const OutgoingPacket& packet)
      {
        // TODO: Do not hold lock.
        
        Anki::Util::TransportAddress destAddress;
        if (!UnreliableUdpChannel::GetAddress(destAddress, packet.destId)) {
          PRINT_STREAM_WARNING("ReliableUDPChannel.SendDirect",
                               "Cannot determine address for connection id " << packet.destId << ".");
          return false;
        }
        
        reliableTransport.SendData(packet.reliable, destAddress, packet.buffer, packet.bufferSize);
        return true;
      }
      
      void Stop_Internal()
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
      
      ConnectionData *GetConnectionData(const Anki::Util::TransportAddress& address) const
      {
        auto found = _connectionDataMapping.find(address);
        if (found != _connectionDataMapping.end()) {
          return &found->second;
        }
        return nullptr;
      }
      
      void PostProcessIncomingPacket(const IncomingPacket& packet)
      {
        if (_processNextPacket)
        {
          ConnectionData data;
          if (!GetConnectionData(data, packet.sourceAddress)) {
            
          }
          
          
          if (packet.tag == IncomingPacket::Tag::ConnectionRequest) {
            
          }
          else {
            s32 sourceId;
            GetConnectionId(sourceId, sourceAddress);
            
            if (packet.tag == IncomingPacket::Tag::Connected) {
              HandleSuccessfulConnection(sourceAddress);
            }
            else if (packet.tag == IncomingPacket::Tag::Disconnected) {
              HandleDisconnection(sourceAddress);
            }
            else {
              packet.sourceId
            }
          }
        }
      }
      
      // Removes the connection without locking.
      virtual void RemoveConnectionForOverwrite(s32 connectionId) override
      {
        RemoveConnection(connectionId);
      }
      
      bool _processNextPacket = true;
      std::map<Anki::Util::TransportAddress, ConnectionData> _connectionDataMapping;
    };
  }
}

#endif /* defined(__IComms__) */
