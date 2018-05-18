/**
 * File: engineMessagingClient.cpp
 *
 * Author: shawnb
 * Created: 3/08/2018
 *
 * Description: Communication point for message coming from / 
 *              going to the engine process. Currently this is
 *              using a tcp connection where engine acts as the
 *              server, and this is the client.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include <stdio.h>
#include <chrono>
#include <thread>
#include "anki-ble/common/log.h"
#include "switchboardd/engineMessagingClient.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "switchboardd/log.h"

using GMessageTag = Anki::Cozmo::ExternalInterface::MessageGameToEngineTag;
using GMessage = Anki::Cozmo::ExternalInterface::MessageGameToEngine;
using EMessageTag = Anki::Cozmo::ExternalInterface::MessageEngineToGameTag;
using EMessage = Anki::Cozmo::ExternalInterface::MessageEngineToGame;
namespace Anki {
namespace Switchboard {

uint8_t EngineMessagingClient::sMessageData[2048];

EngineMessagingClient::EngineMessagingClient(struct ev_loop* evloop)
: loop_(evloop)
{}

bool EngineMessagingClient::Init() {
  ev_timer_init(&_handleEngineMessageTimer.timer, 
                &EngineMessagingClient::sEvEngineMessageHandler, 
                kEngineMessageFrequency_s, 
                kEngineMessageFrequency_s);
  _handleEngineMessageTimer.client = &_client;
  _handleEngineMessageTimer.signal = &_pairingStatusSignal;
  _handleEngineMessageTimer.websocketSignal = &_engineMessageSignal;
  return true;
}

bool Anki::Switchboard::EngineMessagingClient::Connect() {
  bool connected = _client.Connect(Anki::Victor::ENGINE_SWITCH_CLIENT_PATH, Anki::Victor::ENGINE_SWITCH_SERVER_PATH);

  if(connected) {
    ev_timer_start(loop_, &_handleEngineMessageTimer.timer);
  }

  // Send connection message
  static uint8_t connectionByte = 0;
  _client.Send((char*)(&connectionByte), sizeof(connectionByte));

  return connected;
}

bool EngineMessagingClient::Disconnect() {
  bool disconnected = true;
  ev_timer_stop(loop_, &_handleEngineMessageTimer.timer);
  if (_client.IsConnected()) {
    disconnected = _client.Disconnect();
  }
  return disconnected;
}

void EngineMessagingClient::sEvEngineMessageHandler(struct ev_loop* loop, struct ev_timer* w, int revents) {
  struct ev_EngineMessageTimerStruct *wData = (struct ev_EngineMessageTimerStruct*)w;

  int recvSize;
  
  while((recvSize = wData->client->Recv((char*)sMessageData, sizeof(sMessageData))) > kMessageHeaderLength) {
    // Get message tag, and adjust for header size
    const uint8_t* msgPayload = (const uint8_t*)&sMessageData[kMessageHeaderLength];

    const EMessageTag messageTag = (EMessageTag)*msgPayload;

    EMessage message;
    uint16_t msgSize = *(uint16_t*)sMessageData;
    size_t unpackedSize = message.Unpack(msgPayload, msgSize);

    if(unpackedSize != (size_t)msgSize) {
      Log::Error("Received message from engine but had mismatch size when unpacked.");
      continue;
    } 

    if (messageTag == EMessageTag::EnterPairing || messageTag == EMessageTag::ExitPairing) {
      // Emit signal for message
      wData->signal->emit(message);
    } else {
      wData->websocketSignal->emit(message);
    }
  }
}

void EngineMessagingClient::SendMessage(const GMessage& message) {
  uint16_t message_size = message.Size();
  uint8_t buffer[message_size + kMessageHeaderLength];
  message.Pack(buffer + kMessageHeaderLength, message_size);
  memcpy(buffer, &message_size, kMessageHeaderLength);

  _client.Send((char*)buffer, sizeof(buffer));
}

void EngineMessagingClient::SetPairingPin(std::string pin) {
  Anki::Cozmo::SwitchboardInterface::SetBLEPin sbp;
  sbp.pin = std::stoul(pin);
  SendMessage(GMessage::CreateSetBLEPin(std::move(sbp)));
}

void EngineMessagingClient::ShowPairingStatus(Anki::Cozmo::SwitchboardInterface::ConnectionStatus status) {
  Anki::Cozmo::SwitchboardInterface::SetConnectionStatus scs;
  scs.status = status;

  SendMessage(GMessage::CreateSetConnectionStatus(std::move(scs)));
}

} // Switchboard
} // Anki
