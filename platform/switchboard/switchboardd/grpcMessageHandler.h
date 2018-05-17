/**
 * File: grpcMessageHandler.h
 *
 * Author: shawnblakesley
 * Created: 4/05/2018
 *
 * Description: WiFi message protocol for ankiswitchboardd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once

#include <cstdio>
#include <stdlib.h>
#include "signals/simpleSignal.hpp"
#include "switchboardd/engineMessagingClient.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

namespace Anki {
namespace Switchboard {
class GrpcMessageHandler {
public:
  // Constructor
  explicit GrpcMessageHandler(std::shared_ptr<EngineMessagingClient> emc);
  
  // Signals
  using SendToGrpcSignal = Signal::Signal<bool (uint8_t*, size_t)>;
  
  void Receive(uint8_t* buffer, size_t size);
  bool Send(const Anki::Cozmo::ExternalInterface::MessageEngineToGame& msg);
  
  SendToGrpcSignal& OnSendEvent() {
    return _sendSignal;
  }
  
private:
  Signal::SmartHandle _onEngineMessageHandle;
  SendToGrpcSignal _sendSignal;
  std::shared_ptr<EngineMessagingClient> _engineMessaging;
};
} // Switchboard
} // Anki
