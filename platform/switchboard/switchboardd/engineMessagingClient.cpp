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
#include "anki-ble/log.h"
#include "switchboardd/engineMessagingClient.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"


using GMessageTag = Anki::Cozmo::ExternalInterface::MessageGameToEngineTag;
using GMessage = Anki::Cozmo::ExternalInterface::MessageGameToEngine;
using EMessageTag = Anki::Cozmo::ExternalInterface::MessageEngineToGameTag;
using EMessage = Anki::Cozmo::ExternalInterface::MessageEngineToGame;
namespace Anki {
namespace Switchboard {

const char* EngineMessagingClient::kEngineAddress = "127.0.0.1";
uint8_t EngineMessagingClient::sMessageData[2048];


EngineMessagingClient::EngineMessagingClient(struct ev_loop* evloop)
: loop_(evloop)
{

}

bool EngineMessagingClient::Init() {
  ev_timer_init(&_handleEngineMessageTimer.timer, 
                &EngineMessagingClient::sEvEngineMessageHandler, 
                kEngineMessageFrequency_s, 
                kEngineMessageFrequency_s);
  _handleEngineMessageTimer.client = &client;
  _handleEngineMessageTimer.signal = &_pairingStatusSignal;
  return true;
}

bool Anki::Switchboard::EngineMessagingClient::Connect() {
  bool connected = client.Connect(kEngineAddress, kEnginePort);

  if(connected) {
    ev_timer_start(loop_, &_handleEngineMessageTimer.timer);
  }

  return connected;
}

bool EngineMessagingClient::Disconnect() {
  bool disconnected = true;
  ev_timer_stop(loop_, &_handleEngineMessageTimer.timer);
  if (client.IsConnected()) {
    disconnected = client.Disconnect();
  }
  return disconnected;
}

void EngineMessagingClient::sEvEngineMessageHandler(struct ev_loop* loop, struct ev_timer* w, int revents) {
  struct ev_EngineMessageTimerStruct *wData = (struct ev_EngineMessageTimerStruct*)w;

  uint8_t bytes_recvd;
  do {
    bytes_recvd = wData->client->Recv((char*)sMessageData, sizeof(sMessageData));
    uint8_t bytes_remaining = bytes_recvd;
    // Read all the messages that were returned
    size_t index = 0;
    while (bytes_recvd >= index + kMessageHeaderLength) {
      uint16_t message_size = *(uint16_t*)sMessageData;
      index += kMessageHeaderLength;
      if (bytes_recvd - index < message_size) {
        logw("Received partial message from engine");
      } else {
        const EMessageTag messageTag = *(EMessageTag*)(sMessageData + index);
        if (messageTag == EMessageTag::EnterPairing || messageTag == EMessageTag::ExitPairing) {
          EMessage message;
          message.Unpack(sMessageData + index, bytes_recvd - index);
          wData->signal->emit(message);
        }
      }
      index += message_size;
    }
  } while (bytes_recvd > 0);
}

void EngineMessagingClient::SendMessage(const GMessage& message) {
  uint16_t message_size = message.Size();
  uint8_t buffer[message_size + kMessageHeaderLength];
  message.Pack(buffer + kMessageHeaderLength, sizeof(buffer) - kMessageHeaderLength);
  memcpy(buffer, &message_size, kMessageHeaderLength);

  client.Send((char*)buffer, sizeof(buffer));
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
