/**
 * File: tokenClient.cpp
 *
 * Author: paluri
 * Created: 7/16/2018
 *
 * Description: Unix domain socket client connection to vic-cloud process.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "log.h"
#include "tokenClient.h"

namespace Anki {
namespace Switchboard {

uint8_t TokenClient::sMessageData[2048];

TokenClient::TokenClient(struct ev_loop* evloop)
: _loop(evloop)
{}

bool TokenClient::Init() {
  ev_timer_init(&_handleTokenMessageTimer.timer, 
                &TokenClient::sEvTokenMessageHandler, 
                kMessageFrequency_s, 
                kMessageFrequency_s);
  _handleTokenMessageTimer.client = &_client;
  _handleTokenMessageTimer.signal = &_tokenMessageSignal;
  return true;
}

bool TokenClient::Connect() {
  bool connected = _client.Connect(kDomainSocketClient, kDomainSocketServer);

  if(connected) {
    ev_timer_start(_loop, &_handleTokenMessageTimer.timer);
  }

  // Send connection message
  static uint8_t connectionByte = 0;
  _client.Send((char*)(&connectionByte), sizeof(connectionByte));

  return connected;
}

void TokenClient::SendAuthRequest(std::string sessionToken) {
  Anki::Cozmo::TokenRequest tokenRequest = 
    Anki::Cozmo::TokenRequest(Anki::Cozmo::AuthRequest(sessionToken));

  SendMessage(tokenRequest);
}

void TokenClient::SendMessage(const Anki::Cozmo::TokenRequest& message) {
  uint16_t message_size = message.Size();
  uint8_t buffer[message_size];
  message.Pack(buffer, message_size);

  _client.Send((char*)buffer, sizeof(buffer));
}

void TokenClient::sEvTokenMessageHandler(struct ev_loop* loop, struct ev_timer* w, int revents) {
  struct ev_TokenMessageTimerStruct *wData = (struct ev_TokenMessageTimerStruct*)w;

  int recvSize;
  
  while((recvSize = wData->client->Recv((char*)sMessageData, sizeof(sMessageData))) > 0) {
    Log::Write("Received message from token_server: %d", recvSize);

    // Get message tag, and adjust for header size
    const uint8_t* msgPayload = (const uint8_t*)&sMessageData;

    printf("Msg:: ");
    for(int i = 0; i < recvSize; i++) {
      printf("%d ", msgPayload[i]);
    } printf("\n");

    //const Anki::Cozmo::TokenResponseTag messageTag = (Anki::Cozmo::TokenResponseTag)*msgPayload;

    Anki::Cozmo::TokenResponse message;
    size_t unpackedSize = message.Unpack(msgPayload, recvSize);

    if(unpackedSize != (size_t)recvSize) {
      Log::Error("Received message from token server but had mismatch size when unpacked.");
      continue;
    } 

    switch(message.GetTag()) {
      case Anki::Cozmo::TokenResponseTag::auth: {
        Anki::Cozmo::AuthResponse msg = message.Get_auth();
        std::string a = msg.appToken;
        std::string b = msg.jwtToken;
        uint8_t c = (uint8_t)msg.error;

        Log::Write("error[%d] app[%s] jwt[%s]", c, a.c_str(), b.c_str());
      }
      break;
      case Anki::Cozmo::TokenResponseTag::jwt: {
        Anki::Cozmo::JwtResponse msg = message.Get_jwt();
      }
      break;
      default:
        Log::Error("Received unknown message type from TokenServer");
        break;
    }

    // if (messageTag == EMessageTag::EnterPairing || messageTag == EMessageTag::ExitPairing) {
    //   //Emit signal for message
    //   wData->signal->emit(message);
    // }
  }
}

} // Anki
} // Switchboard