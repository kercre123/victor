/**
 * File: GatewayMessagingServer.cpp
 *
 * Author: shawnb
 * Created: 7/24/2018
 *
 * Description: Communication point for message coming from / 
 *              going to the gateway process. Currently this is
 *              using a tcp connection where gateway acts as the
 *              client, and this is the server.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include <stdio.h>
#include <chrono>
#include <thread>
#include "switchboardd/gatewayMessagingServer.h"
#include "clad/externalInterface/messageExternalComms.h"
#include "switchboardd/log.h"

namespace Anki {
namespace Switchboard {

using namespace Anki::Vector::ExternalComms;

uint8_t GatewayMessagingServer::sMessageData[2048];

GatewayMessagingServer::GatewayMessagingServer(struct ev_loop* evloop, std::shared_ptr<TokenClient> tokenClient)
: _tokenClient(tokenClient)
, loop_(evloop)
{}

GatewayMessagingServer::~GatewayMessagingServer() {
  (void)Disconnect();
}

bool GatewayMessagingServer::Init() {
  if(_server.HasClient()) {
    _server.Disconnect();
  }

  _server.StopListening();
  _server.StartListening(Anki::Victor::SWITCH_GATEWAY_SERVER_PATH);

  ev_timer_init(&_handleGatewayMessageTimer.timer,
                &GatewayMessagingServer::sEvGatewayMessageHandler,
                kGatewayMessageFrequency_s, 
                kGatewayMessageFrequency_s);
  _handleGatewayMessageTimer.server = &_server;
  _handleGatewayMessageTimer.messagingServer = this;

  ev_timer_start(loop_, &_handleGatewayMessageTimer.timer);

  return true;
}

bool GatewayMessagingServer::Disconnect() {
  if(_server.HasClient()) {
    _server.Disconnect();
  }
  ev_timer_stop(loop_, &_handleGatewayMessageTimer.timer);
  return true;
}

void GatewayMessagingServer::HandleAuthRequest(const SwitchboardRequest& message) {
  if(_tokenClient.expired()) {
    return;
  }

  std::shared_ptr<TokenClient> tokenClient = _tokenClient.lock();
  std::string sessionToken = message.Get_AuthRequest().sessionToken;

  tokenClient->SendJwtRequest(
    [this, sessionToken, tokenClient](Anki::Vector::TokenError error, std::string jwtToken) {
      bool isPrimary = false;
      Log::Write("CloudRequest JWT Response Handler");

      switch(error) {
        case Anki::Vector::TokenError::NullToken: {
          // Primary association
          isPrimary = true;
          tokenClient->SendAuthRequest(sessionToken, 
            [this, isPrimary](Anki::Vector::TokenError authError, std::string appToken, std::string authJwtToken) {
            ProcessCloudAuthResponse(isPrimary, authError, appToken, authJwtToken);
          });
        }
        break;
        case Anki::Vector::TokenError::NoError: {
          // Secondary association
          isPrimary = false;
          tokenClient->SendSecondaryAuthRequest(sessionToken, "", "",
            [this, isPrimary](Anki::Vector::TokenError authError, std::string appToken, std::string authJwtToken) {
            Log::Write("CloudRequest Auth Response Handler");
            ProcessCloudAuthResponse(isPrimary, authError, appToken, authJwtToken);
          });
        }
        break;
        case Anki::Vector::TokenError::InvalidToken: {
          // We received an invalid token
          Log::Error("Received invalid token for JwtRequest, try reassociation");
          isPrimary = false;
          tokenClient->SendReassociateAuthRequest(sessionToken, "", "",
            [this, isPrimary](Anki::Vector::TokenError authError, std::string appToken, std::string authJwtToken) {
            Log::Write("CloudRequest Auth Response Handler");
            ProcessCloudAuthResponse(isPrimary, authError, appToken, authJwtToken);
          });
        }
        break;
        case Anki::Vector::TokenError::Connection:
        default: {
          // Could not connect/authorize to server
          Log::Error("Received connection error msg for JwtRequest");
          SendMessage(SwitchboardResponse(Anki::Vector::AuthResponse("", "", error)));
        }
        break;
      }
  });
}

void GatewayMessagingServer::ProcessCloudAuthResponse(bool isPrimary, Anki::Vector::TokenError authError, std::string appToken, std::string authJwtToken) {
  // Send SwitchboardResponse
  SwitchboardResponse res = SwitchboardResponse(Anki::Vector::AuthResponse(appToken, "", authError));
  SendMessage(res);
}

void GatewayMessagingServer::sEvGatewayMessageHandler(struct ev_loop* loop, struct ev_timer* w, int revents) {
  struct ev_GatewayMessageTimerStruct *wData = (struct ev_GatewayMessageTimerStruct*)w;

  int recvSize;

  while((recvSize = wData->server->Recv((char*)sMessageData, sizeof(sMessageData))) > kMessageHeaderLength) {
    // Get message tag, and adjust for header size
    const uint8_t* msgPayload = (const uint8_t*)&sMessageData[kMessageHeaderLength];

    const SwitchboardRequestTag messageTag = (SwitchboardRequestTag)*msgPayload;

    SwitchboardRequest message;
    uint16_t msgSize = *(uint16_t*)sMessageData;
    size_t unpackedSize = message.Unpack(msgPayload, msgSize);

    if(unpackedSize != (size_t)msgSize) {
      continue;
    }

    switch(messageTag) {
      case SwitchboardRequestTag::AuthRequest:
      {
        GatewayMessagingServer* self = wData->messagingServer;
        self->HandleAuthRequest(message);
      }
        break;
      default:
        break;
    }
  }
}

bool GatewayMessagingServer::SendMessage(const SwitchboardResponse& message) {
  if (_server.HasClient()) {
    uint16_t message_size = message.Size();
    uint8_t buffer[message_size + kMessageHeaderLength];
    message.Pack(buffer + kMessageHeaderLength, message_size);
    memcpy(buffer, &message_size, kMessageHeaderLength);

    const ssize_t res = _server.Send((const char*)buffer, sizeof(buffer));
    if (res < 0) {
      _server.Disconnect();
      return false;
    }
    return true;
  }
  return false;
}

} // Switchboard
} // Anki
