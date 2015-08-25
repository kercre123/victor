//
//  ChannelBase.h
//  Cozmo
//
//  Created by Greg Nagel on 5/26/2015.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#ifndef __CommsBase__
#define __CommsBase__

#include "anki/messaging/basestation/IChannel.h"

#include "util/transport/iNetTransportDataReceiver.h"

#include <deque>
#include <map>
#include <unordered_map>

namespace Anki {
  namespace Comms {

    // Standard IChannel base class that implements some basic connection features.
    // Provides a protected implementation of INetTransportDataReceiver so implementers
    // can pass "this" to various callback setters.
    class ChannelBase: public IChannel, protected Anki::Util::INetTransportDataReceiver {
    public:
      
      ChannelBase();
      virtual ~ChannelBase() override;
      
      virtual bool PopIncomingPacket(IncomingPacket& packet) override;
      
      virtual bool IsConnectionActive(ConnectionId connectionId) const override;
      
      virtual int32_t CountActiveConnections() const override;
      
      // adds both entries to the bidirectional mapping, removing any duplicates
      // assumes a consistent state
      virtual void AddConnection(ConnectionId connectionId, const TransportAddress& address) override;

      // Default implementation just returns false.
      virtual bool AcceptIncomingConnection(ConnectionId connectionId, const TransportAddress& address) override;
      
      // Default implementation is a no-op.
      virtual void RefuseIncomingConnection(const TransportAddress& address) override;
      
      // NOTE: Base class implementation does not actually do the disconnection part.
      virtual void RemoveConnection(ConnectionId connectionId) override;

      // NOTE: Base class implementation does not actually do the disconnection part.
      virtual void RemoveAllConnections() override;

      virtual bool GetConnectionId(ConnectionId& connectionId, const TransportAddress& address) const override;
      
      virtual bool GetAddress(TransportAddress& address, ConnectionId connectionId) const override;

    protected:
      void PushIncomingPacket(const IncomingPacket& packet);

      void EmplaceIncomingPacket(IncomingPacket&& packet);

      // You should override this if you need any special handling for say, disconnects.
      virtual void ReceiveData(const uint8_t *buffer, unsigned int bufferSize, const TransportAddress& sourceAddress) override;

      void ClearPacketsForAddress(const TransportAddress& address);

      // Clears packets in the queue until just after the last disconnect
      // This is so we can queue an incoming connection
      void ClearPacketsForAddressUntilNewestConnection(const TransportAddress& address);

      void ClearPacketsForAddressAfterEarliestDisconnect(const TransportAddress& address);

      // Equivalent semantics to RemoveConnection,
      // except that it wipes out all packets that belong to the specified address.
      // This version is only called internally, when the connection is about to be reused.
      virtual void RemoveConnectionForOverwrite(ConnectionId connectionId);

    private:
      // Removes the connection, but doesn't mess with packets
      bool RemoveConnectionInternal(TransportAddress& address, ConnectionId connectionId);

      std::unordered_map<TransportAddress, ConnectionId> _addressLookup;
      std::unordered_map<ConnectionId, TransportAddress> _connectionIdLookup;
      std::deque<IncomingPacket> _incomingPacketQueue;
    };
  }
}

#endif /* defined(__CommsBase__) */
