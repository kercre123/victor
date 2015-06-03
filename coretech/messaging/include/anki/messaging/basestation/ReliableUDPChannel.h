//
//  ReliableUDPChannel.h
//  Cozmo
//
//  Created by Mark Pauley on 8/17/12.
//  Copyright (c) 2012 Anki, Inc. All rights reserved.
//

#ifndef __ReliableUDPChannel__
#define __ReliableUDPChannel__

#include "anki/messaging/basestation/UnreliableUDPChannel.h"

#include "anki/util/transport/reliableTransport.h"

#include <map>
#include <mutex>
#include <vector>

namespace Anki {
  namespace Comms {

    // Reliable communications channel.
    // All IDs that are passed back will definitely be valid, except for ConnectionRequest.
    class ReliableUDPChannel: public UnreliableUDPChannel {
    public:

      ReliableUDPChannel();
      virtual ~ReliableUDPChannel() override;

      virtual bool IsStarted() const override;

      virtual bool IsHost() const override;

      virtual TransportAddress GetHostingAddress() const override;

      virtual void StartClient() override;

      virtual void StartHost(const TransportAddress& bindAddress) override;

      virtual void Stop() override;

      virtual void Update() override;

      virtual bool Send(const Anki::Comms::OutgoingPacket& packet) override;

      virtual bool PopIncomingPacket(IncomingPacket& packet) override;

      virtual bool IsConnectionActive(ConnectionId connectionId) const override;

      virtual int32_t CountActiveConnections() const override;

      virtual void AddConnection(ConnectionId connectionId, const TransportAddress& address) override;

      virtual bool AcceptIncomingConnection(ConnectionId connectionId, const TransportAddress& address) override;

      virtual void RefuseIncomingConnection(const TransportAddress& address) override;

      virtual void RemoveConnection(ConnectionId connectionId) override;

      virtual void RemoveAllConnections() override;

      virtual bool GetConnectionId(ConnectionId& connectionId, const TransportAddress& address) const override;

      virtual bool GetAddress(TransportAddress& address, ConnectionId connectionId) const override;

      virtual uint32_t GetMaxTotalBytesPerMessage() const override;

    protected:

      // NOTE: This method runs on a separate thread, and thus should never touch ConnectionData::state.
      virtual void ReceiveData(const uint8_t *buffer, unsigned int bufferSize, const TransportAddress& sourceAddress) override;

      Anki::Util::ReliableTransport reliableTransport;
      // need recursive for RemoveConnection to not be a pain
      mutable std::recursive_mutex mutex;

    private:
      struct ConnectionData {
      public:
        enum class State {
          WaitingForConnectionResponse,
          MustSendConnectionResponse,
          Connected,
          Disconnected,
        };

        ConnectionData(State state)
        : state(state)
        , isRealConnectionActive(true)
        , isDisconnectionQueued(false)
        {
        }

        // The state as known to the user
        State state;
        // Whether the reliableTransport has a connection for this address
        bool isRealConnectionActive;
        // Whether a disconnect is coming down the pipe
        bool isDisconnectionQueued;
        std::vector<OutgoingPacket> connectionPackets;
      };

      void ConfigureReliableTransport();

      bool SendDirect(const OutgoingPacket& packet);

      void Stop_Internal();

      ConnectionData *GetConnectionData(const TransportAddress& address);

      // mainly sets packet.sourceId
      // also used for verification and state changes
      void PostProcessIncomingPacket(IncomingPacket& packet);

      // Removes the connection without locking.
      virtual void RemoveConnectionForOverwrite(ConnectionId connectionId) override;

      // Removes the ConnectionData struct.
      // if full is set, it always wipes it and the current connection full
      // if full is not set, it will try to leave a queued connection in the queue
      void RemoveConnectionData(const TransportAddress& address, bool full);

      void QueueDisconnectionIfActuallyConnected(const TransportAddress& sourceAddress, bool makeActive);

      std::unordered_map<TransportAddress, ConnectionData> _connectionDataMapping;
    };
  }
}

#endif /* defined(__ReliableUDPChannel__) */
