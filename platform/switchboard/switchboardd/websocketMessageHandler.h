/**
 * File: websocketMessageHandler.h
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
#include "clad/externalInterface/messageExternalComms.h"

namespace Anki {
namespace Switchboard {
class WebsocketMessageHandler {
public:
  // Constructor
  explicit WebsocketMessageHandler(std::shared_ptr<EngineMessagingClient> emc);
  
  // Signals
  using SendToWebsocketSignal = Signal::Signal<bool (uint8_t*, size_t)>;
  
  void Receive(uint8_t* buffer, size_t size);
  bool Send(const Anki::Cozmo::ExternalInterface::MessageEngineToGame& msg);
  
  SendToWebsocketSignal& OnSendEvent() {
    return _sendSignal;
  }
  
private:
  // // TODO: These methods are examples for where the handlers should go.
  // //       they should be removed once there are any real ones implemented.
  // void HandleMeetVictor(Anki::Victor::ExternalComms::MeetVictor meetVictor);
  // void HandleMeetVictor_MeetVictorRequest(float lwheel_speed_mmps, float rwheel_speed_mmps);

  Signal::SmartHandle _onEngineMessageHandle;
  SendToWebsocketSignal _sendSignal;
  std::shared_ptr<EngineMessagingClient> _engineMessaging;
};
} // Switchboard
} // Anki
