//
//  UnreliableUDPChannel.cpp
//  Cozmo
//
//  Created by Greg Nagel on 5/26/2015.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#include "anki/messaging/basestation/UnreliableUDPChannel.h"

#include "anki/util/logging/logging.h"

#include "anki/util/transport/srcBufferSet.h"

using namespace Anki::Comms;

UnreliableUDPChannel::UnreliableUDPChannel()
{
  // TODO ANDROID: SET IP RETRIEVER
  //unreliableTransport.SetIpRetriever(nullptr);
  unreliableTransport.SetDataReceiver(this);
}

UnreliableUDPChannel::~UnreliableUDPChannel()
{
}

bool UnreliableUDPChannel::IsStarted() const
{
  return isStarted;
}

bool UnreliableUDPChannel::IsHost() const
{
  return isHost;
}

void UnreliableUDPChannel::StartClient()
{
  Stop();
  
  unreliableTransport.StartClient();
  isHost = false;
  isStarted = true;
}

void UnreliableUDPChannel::StartHost(const TransportAddress& bindAddress)
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

void UnreliableUDPChannel::Stop()
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

void UnreliableUDPChannel::Update()
{
  unreliableTransport.Update();
}

bool UnreliableUDPChannel::Send(const Anki::Comms::OutgoingPacket& packet)
{
  if (!isStarted) {
    PRINT_STREAM_WARNING("UnreliableUDPChannel.Send",
                         "Channel is not started.");
    return false;
  }

  TransportAddress address;
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

uint32_t UnreliableUDPChannel::MaxTotalBytesPerMessage() const
{
  return unreliableTransport.MaxTotalBytesPerMessage();
}
