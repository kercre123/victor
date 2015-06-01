/**
 * File: multiClientChannel.h
 *
 * Author: Kevin Yoon
 * Created: 1/22/2014
 *
 * Description: Interface class that creates multiple TCP or UDP clients to connect
 *              and communicate with advertising devices.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef BASESTATION_COMMS_MULTI_CLIENT_CHANNEL_H_
#define BASESTATION_COMMS_MULTI_CLIENT_CHANNEL_H_

#include <unordered_map>
#include <vector>
#include <anki/messaging/basestation/IChannel.h>
#include <anki/messaging/basestation/UnreliableUDPChannel.h>
#include <anki/messaging/basestation/ReliableUDPChannel.h>
#include "anki/messaging/basestation/advertisementService.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/messaging/shared/UdpClient.h"

// Set to 1 to simulate a send/receive latencies
// beyond the actual latency of TCP.
// Note that the resolution of these latencies is currently equal to
// the Basestation frequency since that's what defines how often Update() is called.
//#define DO_SIM_COMMS_LATENCY 1
#define SIM_RECV_LATENCY_SEC 0 // 0.03
#define SIM_SEND_LATENCY_SEC 0 // 0.03



namespace Anki {
namespace Cozmo {

  class MultiClientChannel : public Comms::IChannel {
  public:
    
    MultiClientChannel();
    
    // The destructor will automatically cleans up
    virtual ~MultiClientChannel() override;
    
    // Determines whether the Start method has been called.
    bool IsStarted() const;
    
    TransportAddress GetAdvertisingAddress() const;
    
    void Start(const TransportAddress& advertisingAddress);
    
    void Stop();
    
    
    virtual void Update() override;
    
    virtual bool Send(const Anki::Comms::OutgoingPacket& packet) override;

    virtual bool PopIncomingPacket(IncomingPacket& packet) override;
    
    // true if we have recent advertisement data on the connection id
    bool IsConnectionAdvertising(ConnectionId connectionId) const;
    
    // true if we are either trying to connect or currently sending/receiving with the connection
    virtual bool IsConnectionActive(ConnectionId connectionId) const override;
    
    int32_t CountAdvertisingConnections() const;
    
    virtual int32_t CountActiveConnections() const override;
    
    bool GetAdvertisingConnections(std::vector<ConnectionId>& connectionIds);
    
    bool AcceptAdvertisingConnection(ConnectionId connectionId);
    
    bool AcceptAllAdvertisingConnections();
    
    // Not implemented, as connection is done through advertising. Just returns false.
    virtual bool AcceptIncomingConnection(ConnectionId connectionId, const TransportAddress& transportAddress) override;
    // Not implemented, as connection is done through advertising. No-op.
    virtual void RefuseIncomingConnection(const TransportAddress& transportAddress) override;
    
    
    // Force add an unadvertised connection; connecting directly to that location
    virtual void AddConnection(ConnectionId connectionId, const TransportAddress& address) override;
    
    // Removes the connection so it is not advertising
    void RemoveAdvertisingConnection(ConnectionId connectionId);
    
    // Removes the connection so it is inactive
    // Does not affect advertisement
    virtual void RemoveConnection(ConnectionId connectionId) override;
    
    // Remove just the advertising connections
    void RemoveAllAdvertisingConnections();
    
    // Removes just the active connections
    virtual void RemoveAllConnections() override;
    
    
    virtual bool GetConnectionId(ConnectionId& connectionId, const TransportAddress& address) const override;
    
    virtual bool GetAddress(TransportAddress& address, ConnectionId connectionId) const override;
    
    
    virtual uint32_t GetMaxTotalBytesPerMessage() const override;
  
  private:
    
    struct AdvertisementConnectionInfo {
      Comms::AdvertisementMsg message;
      double lastSeenTime = 0;
      
      AdvertisementConnectionInfo()
      {
      }
      AdvertisementConnectionInfo(const Comms::AdvertisementMsg& message, double lastSeenTime)
      : message(message)
      , lastSeenTime(lastSeenTime)
      {
      }
    };
    
    void SendZeroPacket();
    
    bool AcceptAdvertisingConnectionInternal(ConnectionId connectionId, const AdvertisementConnectionInfo& info);
    
    Anki::Comms::UnreliableUDPChannel _advertisingChannel;
    
    // TODO: Remove this once advertisementService is updated to talk with UnreliableUDPChannel
    UdpClient _advertisingClient;
    
    Anki::Comms::ReliableUDPChannel _reliableChannel;
    
    // Map of advertising robots (key: dev id)
    std::unordered_map<ConnectionId, AdvertisementConnectionInfo> _advertisingInfo;
    
//#if(DO_SIM_COMMS_LATENCY)
//    // The number of messages that have been in recvdMsgPackets for at least
//    // SIM_RECV_LATENCY_SEC and are now available for reading.
//    u32 numRecvRdyMsgs_;
//    
//    // Queue of messages to be sent with the times they should be sent at
//    // (key: dev id)
//    std::map<int, PacketQueue_t> sendMsgPackets_;
//
//    // The actual function that does the sending when we're simulating latency
//    int RealSend(const Comms::MsgPacket &p);
//    
//    // Outgoing bytes sent since last call to Update()
//    int bytesSentThisUpdateCycle_;
//#endif
    
  };

}  // namespace Cozmo
}  // namespace Anki

#endif  // #ifndef BASESTATION_COMMS_MULTI_CLIENT_COMMS_H_

