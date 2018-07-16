/**
 * File: tokenClient.h
 *
 * Author: paluri
 * Created: 7/16/2018
 *
 * Description: Unix domain socket client connection to vic-cloud process.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once

#include <functional>
#include <vector>
#include <queue>
#include "signals/simpleSignal.hpp"
#include "ev++.h"
#include "engine/clad/cloud/token.h"
#include "coretech/messaging/shared/LocalUdpClient.h"

namespace Anki {
namespace Switchboard {
  
class TokenClient {
  using TokenMessageSignal = Signal::Signal<void (Anki::Cozmo::TokenResponse)>;

public:
  explicit TokenClient(struct ev_loop* loop);
  bool Init();
  bool Connect();
  void SendAuthRequest(std::string sessionToken);
  void SendMessage(const Anki::Cozmo::TokenRequest& message);
  void SessionTokenRequest(std::vector<uint8_t> sessionToken, 
    std::function<void(int, std::vector<uint8_t>)> callback);

private:
  const char* kDomainSocketServer = "/dev/token_server";
  const char* kDomainSocketClient = "/dev/token_client_switchboard";

  static uint8_t sMessageData[2048];
  const float kMessageFrequency_s = 0.1;

  static void sEvTokenMessageHandler(struct ev_loop* loop, struct ev_timer* w, int revents);

  TokenMessageSignal _tokenMessageSignal;

  struct ev_loop* _loop;
  struct ev_TokenMessageTimerStruct {
    ev_timer timer;
    LocalUdpClient* client;
    TokenMessageSignal* signal;
  } _handleTokenMessageTimer;

  std::queue<std::function<void(int, std::vector<uint8_t>)>> _callbacks;
  std::function<void(int, std::vector<uint8_t>)> _callback;
  LocalUdpClient _client;
};

} // Anki
} // Switchboard