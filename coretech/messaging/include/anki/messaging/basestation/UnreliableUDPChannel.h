//
//  UnreliableUDPChannel.h
//  Cozmo
//
//  Created by Greg Nagel on 5/26/2015.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#ifndef __UnreliableUDPChannel__
#define __UnreliableUDPChannel__

#include "anki/messaging/basestation/CommsBase.h"

#include "anki/util/transport/udpTransport.h"

namespace Anki {
  namespace Comms {
    
    // Unreliable communications channel.
    // Unsolicited connections will be assigned id -1.
    class UnreliableUDPChannel: public CommsBase {
    public:
      
      UnreliableUDPChannel();
      virtual ~UnreliableUDPChannel() override;

      // Determines whether one of the Start methods has been called.
      virtual bool IsStarted() const;

      // Determines whether the channel has been started as a host.
      virtual bool IsHost() const;

      virtual void StartClient();
      
      virtual void StartHost(const TransportAddress& bindAddress);

      virtual void Stop();

      virtual void Update() override;

      virtual bool Send(const Anki::Comms::OutgoingPacket& packet) override;

      virtual uint32_t MaxTotalBytesPerMessage() const override;
      
    protected:
      bool isStarted = false;
      bool isHost = false;
      Anki::Util::UDPTransport unreliableTransport;
    };

  }
}

#endif /* defined(__UnreliableUDPChannel__) */
