//
//  CommsBase.h
//  Cozmo
//
//  Created by Greg Nagel on 5/26/2015.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#ifndef __CommsBase__
#define __CommsBase__

#include "anki/messaging/basestation/IComms.h"

#include "anki/util/transport/iNetTransportDataReceiver.h"

#include <deque>
#include <map>
#include <unordered_map>

namespace Anki {
  namespace Comms {

    // Standard IComms base class that implements some basic connection features.
    // Provides a protected implementation of INetTransportDataReceiver so implementers
    // can pass "this" to various callback setters.
    class CommsBase: public IComms, protected Anki::Util::INetTransportDataReceiver {
    public:
      
      CommsBase();
      virtual ~CommsBase() override;
      
      virtual bool PopIncomingPacket(IncomingPacket& packet) override;
      
      // adds both entries to the bidirectional mapping, removing any duplicates
      // assumes a consistent state
      virtual void AddConnection(s32 connectionId, const TransportAddress& address) override;

      // NOTE: Base class implementation does not actually do the disconnection part.
      virtual void RemoveConnection(s32 connectionId) override;

      // NOTE: Base class implementation does not actually do the disconnection part.
      virtual void RemoveAllConnections() override;

      virtual bool GetConnectionId(s32& connectionId, const TransportAddress& address) const override;
      
      virtual bool GetAddress(TransportAddress& address, s32 connectionId) const override;

    protected:
      // no equality operator defined, so use less-than
      static bool AddressEquals(const TransportAddress& a, const TransportAddress& b);

      void PushIncomingPacket(const IncomingPacket& packet);

      void EmplaceIncomingPacket(const IncomingPacket&& packet);

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
      virtual void RemoveConnectionForOverwrite(s32 connectionId);

    private:
      // Removes the connection, but doesn't mess with packets
      bool RemoveConnectionInternal(TransportAddress& address, s32 connectionId);

      std::map<TransportAddress, s32> _addressLookup;
      std::unordered_map<s32, TransportAddress> _connectionIdLookup;
      std::deque<IncomingPacket> _incomingPacketQueue;
    };
  }
}

#endif /* defined(__CommsBase__) */
