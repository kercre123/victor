/**
 * File: GatewayMessagingServer.h
 *
 * Author: shawnb
 * Created: 7/24/2018
 *
 * Description: Communication point for message coming from / 
 *              going to the gateway process. Currently this is
 *              using a udp connection where gateway acts as the
 *              client, and this is the server.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef PLATFORM_SWITCHBOARD_SWITCHBOARDD_GatewayMessagingServer_H_
#define PLATFORM_SWITCHBOARD_SWITCHBOARDD_GatewayMessagingServer_H_

#include <string>
#include <signals/simpleSignal.hpp>
#include "ev++.h"
#include "coretech/messaging/shared/socketConstants.h"
#include "coretech/messaging/shared/LocalUdpServer.h"
#include "engine/clad/gateway/switchboard.h"

namespace Anki {
namespace Switchboard {

class GatewayMessagingServer {
public:
  using GatewayMessageSignal = Signal::Signal<void (SwitchboardRequest)>;
  explicit GatewayMessagingServer(struct ev_loop* loop);
  bool Init();
  bool Disconnect();
  bool SendMessage(const SwitchboardResponse& message);
  static void sEvGatewayMessageHandler(struct ev_loop* loop, struct ev_timer* w, int revents);


private:

  LocalUdpServer _server;
  GatewayMessageSignal _gatewayMessageSignal;

  struct ev_loop* loop_;
  struct ev_GatewayMessageTimerStruct {
    ev_timer timer;
    LocalUdpServer* server;
    GatewayMessageSignal* signal;
  } _handleGatewayMessageTimer;

  static uint8_t sMessageData[2048];
  static const unsigned int kMessageHeaderLength = 2;
  const float kGatewayMessageFrequency_s = 0.1;
};
} // Switchboard
} // Anki

#endif // PLATFORM_SWITCHBOARD_SWITCHBOARDD_GatewayMessagingServer_H_
