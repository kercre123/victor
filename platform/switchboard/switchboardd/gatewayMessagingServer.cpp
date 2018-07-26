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
#include "switchboardd/log.h"

namespace Anki {
namespace Switchboard {

uint8_t GatewayMessagingServer::sMessageData[2048];

GatewayMessagingServer::GatewayMessagingServer(struct ev_loop* evloop)
: loop_(evloop)
{}

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
  _handleGatewayMessageTimer.signal = &_gatewayMessageSignal;

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
      case SwitchboardRequestTag::JwtRequest:
      {
        wData->signal->emit(message);
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
