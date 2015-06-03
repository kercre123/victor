//
//  IChannel.h
//  Cozmo
//
//  Created by Greg Nagel on 5/26/2015.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//


#ifndef __IChannel__
#define __IChannel__

#include "anki/common/types.h"
#include "anki/common/basestation/exceptions.h"

#include "anki/util/logging/logging.h"
#include "anki/util/transport/transportAddress.h"

namespace Anki {
  namespace Comms {

    // The max size in one BLE packet
    #define MAX_BLE_MSG_SIZE        20
    #define MIN_BLE_MSG_SIZE        2
    
    typedef s32 ConnectionId;
    const ConnectionId DEFAULT_CONNECTION_ID = -1;
    
    struct PacketBase {
      PacketBase() { }
      PacketBase(const uint8_t *bufferData, unsigned int bufferDataSize)
      {
        if (bufferDataSize > MAX_SIZE) {
          PRINT_STREAM_WARNING("PacketBase",
                               "Truncating packet size because packet is larger than buffer.");
          bufferDataSize = MAX_SIZE;
        }
        
        std::memcpy(this->buffer, bufferData, bufferDataSize);
        this->bufferSize = bufferDataSize;
      }
      
      // The outgoing buffer.
      static constexpr unsigned int MAX_SIZE = 2000;
      uint8_t buffer[MAX_SIZE];
      unsigned int bufferSize = 0;
    };
    
    // A packet on its way out.
    struct OutgoingPacket : PacketBase {
      OutgoingPacket()
      {
      }
      OutgoingPacket(const uint8_t *bufferData, unsigned int bufferDataSize, ConnectionId destId, bool reliable = true, bool hot = false)
      : PacketBase(bufferData, bufferDataSize), destId(destId), reliable(reliable), hot(hot)
      {
      }
      
      // The destination connection ID.
      ConnectionId destId = DEFAULT_CONNECTION_ID;
      
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
      
      using TransportAddress = Anki::Util::TransportAddress;
      
      IncomingPacket()
      {
      }
      IncomingPacket(Tag tag, const TransportAddress& sourceAddress)
      : tag(tag)
      , sourceAddress(sourceAddress)
      {
      }
      IncomingPacket(Tag tag, const uint8_t *bufferData, unsigned int bufferDataSize, ConnectionId sourceId, const TransportAddress& sourceAddress)
      : PacketBase(bufferData, bufferDataSize)
      , tag(tag)
      , sourceId(sourceId)
      , sourceAddress(sourceAddress)
      {
      }
      
      // The type of this packet. Only NormalMessage actually uses the buffer.
      Tag tag;
      
      // The source connection ID.
      ConnectionId sourceId = DEFAULT_CONNECTION_ID;
      
      // The TransportAddress of the connection.
      TransportAddress sourceAddress;
    };
    
    class IChannel {
    public:
      
      // local definitions for convenience
      using ConnectionId = Anki::Comms::ConnectionId;
      const ConnectionId DEFAULT_CONNECTION_ID = Anki::Comms::DEFAULT_CONNECTION_ID;
      using OutgoingPacket = Anki::Comms::OutgoingPacket;
      using IncomingPacket = Anki::Comms::IncomingPacket;
      using TransportAddress = Anki::Util::TransportAddress;
      
      virtual ~IChannel() {};
      
      virtual void Update() = 0;
      
      // Sends a packet to the packet's destId.
      // Returns false if it cannot send for some reason.
      // A true result does not ensure delivery.
      virtual bool Send(const OutgoingPacket& packet) = 0;
      
      // Pops the next incoming packet from the queue.
      // Returns true and sets the packet if there is one, discarding it from the queue.
      virtual bool PopIncomingPacket(IncomingPacket& packet) = 0;
      
      // Returns true if there is a connection with the specified id.
      // May not actually be connected, eg while connecting or for connectionless protocols.
      // On internal disconnect, will not change state until Disconnection packet is seen.
      virtual bool IsConnectionActive(ConnectionId connectionId) const = 0;
      
      virtual int32_t CountActiveConnections() const = 0;
      
      // Manually adds a connection to a specified address.
      // Should warn, but allow, overriding an old connection.
      // Use IsConnectionActive/GetConnectionId if you want to keep a
      // ConnectionId or TransportAddress mapping up to date.
      // Some IChannel implementations may have other ways of adding connections or auto-determining ids. This is sometimes just used for force-adding.
      virtual void AddConnection(ConnectionId connectionId, const TransportAddress& address) = 0;
      
      // Responds to the initial handshake, allowing the connection.
      // Only send in response to ConnectionRequest packets.
      // The alternative is RefuseIncomingConnection.
      // Not always implemented. On connectionless channels, this is a no-op.
      virtual bool AcceptIncomingConnection(ConnectionId connectionId, const TransportAddress& address) = 0;
      
      // Responds to the initial handshake, disallowing the connection.
      // Only send in response to ConnectionRequest packets.
      // The alternative is AcceptIncomingConnection.
      // Not always implemented. On connectionless channels, this is a no-op.
      virtual void RefuseIncomingConnection(const TransportAddress& address) = 0;
      
      // Manually removes a connection, completely cleaning it up, including disconnecting.
      // Will already be removed if you get a disconnection notification.
      // It is OK to remove a non-existent connection. (Makes it easier to deal with disconnects.)
      // Removing the connection will discard all incoming packets for the specified connectionId.
      virtual void RemoveConnection(ConnectionId connectionId) = 0;
      // Manually remove all connections, completely cleaning them up, including disconnecting.
      // Clears all incoming packets as well, even those not associated with a connectionId.
      virtual void RemoveAllConnections() = 0;
      
      // Figures out the ConnectionId from a given TransportAddress.
      virtual bool GetConnectionId(ConnectionId& connectionId, const TransportAddress& address) const = 0;
      
      // Figures out the TransportAddress from a given ConnectionId.
      virtual bool GetAddress(TransportAddress& address, ConnectionId connectionId) const = 0;
      
      // The maximum number of bytes you can send.
      // May return 0, meaning limit unknown.
      virtual uint32_t GetMaxTotalBytesPerMessage() const = 0;
    };
  }
}

#endif /* defined(__IChannel__) */
