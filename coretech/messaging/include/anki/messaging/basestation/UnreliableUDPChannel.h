//
//  UnreliableUDPChannel.h
//  Cozmo
//
//  Created by Greg Nagel on 5/26/2015.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#ifndef __UnreliableUDPChannel__
#define __UnreliableUDPChannel__

#include "anki/messaging/basestation/ChannelBase.h"

#include "anki/util/transport/udpTransport.h"

namespace Anki {
  namespace Comms {
    
    // Unreliable communications channel.
    // Unsolicited connections will be assigned id DEFAULT_CONNECTION_ID.
    class UnreliableUDPChannel: public ChannelBase {
    public:
      
      UnreliableUDPChannel();
      virtual ~UnreliableUDPChannel() override;

      // Determines whether one of the Start methods has been called.
      virtual bool IsStarted() const;

      // Determines whether the channel has been started as a host.
      virtual bool IsHost() const;
      
      virtual TransportAddress GetHostingAddress() const;

      virtual void StartClient();
      
      virtual void StartHost(const TransportAddress& bindAddress);

      virtual void Stop();

      virtual void Update() override;

      virtual bool Send(const Anki::Comms::OutgoingPacket& packet) override;

      virtual uint32_t GetMaxTotalBytesPerMessage() const override;
      
    protected:
      bool isStarted = false;
      bool isHost = false;
      TransportAddress hostingAddress;
      Anki::Util::UDPTransport unreliableTransport;
    };

  }
}

#endif /* defined(__UnreliableUDPChannel__) */
