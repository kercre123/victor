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
  void HandleMotorControl(Anki::Cozmo::ExternalComms::MotorControl unionInstance);
  void HandleMotorControl_DriveWheels(Anki::Cozmo::ExternalComms::DriveWheels sdkMessage);
  void HandleMotorControl_DriveArc(Anki::Cozmo::ExternalComms::DriveArc sdkMessage);
  void HandleMotorControl_MoveHead(Anki::Cozmo::ExternalComms::MoveHead sdkMessage);
  void HandleMotorControl_MoveLift(Anki::Cozmo::ExternalComms::MoveLift sdkMessage);

  Signal::SmartHandle _onEngineMessageHandle;
  SendToWebsocketSignal _sendSignal;
  std::shared_ptr<EngineMessagingClient> _engineMessaging;
};
} // Switchboard
} // Anki
