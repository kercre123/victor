/**
 * File: rtsHandlerV3.h
 *
 * Author: paluri
 * Created: 6/27/2018
 *
 * Description: V3 of BLE Protocol
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once

#include "switchboardd/IRtsHandler.h"
#include "switchboardd/engineMessagingClient.h"
#include "switchboardd/INetworkStream.h"
#include "switchboardd/externalCommsCladHandler.h"
#include "anki-wifi/wifi.h"

namespace Anki {
namespace Switchboard {

class RtsHandlerV3 : public IRtsHandler {
public:
  RtsHandlerV3(INetworkStream* stream, 
    struct ev_loop* evloop,
    std::shared_ptr<EngineMessagingClient> engineClient,
    bool isPairing,
    bool isOtaUpdating);

  bool StartRts();

private:
  void SendPublicKey();
  void SendNonce();
  void SendChallenge();
  void SendCancelPairing();
  void SendChallengeSuccess();
  void SendWifiScanResult();
  void SendWifiConnectResult(ConnectWifiResult result);
  void SendWifiAccessPointResponse(bool success, std::string ssid, std::string pw);
  void SendStatusResponse();
  void SendFile(uint32_t fileId, std::vector<uint8_t> fileBytes);

  INetworkStream* _stream;
  struct ev_loop* _loop;
  std::shared_ptr<EngineMessagingClient> _engineClient;
  bool _isPairing;
  bool _isOtaUpdating;

  const uint8_t kNumPinDigits = 6;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Send messages method
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  template<typename T, typename... Args>
  int SendRtsMessage(Args&&... args) {
    Anki::Cozmo::ExternalComms::ExternalComms msg = Anki::Cozmo::ExternalComms::ExternalComms(
      Anki::Cozmo::ExternalComms::RtsConnection(Anki::Cozmo::ExternalComms::RtsConnection_3(T(std::forward<Args>(args)...))));
    std::vector<uint8_t> messageData(msg.Size());
    const size_t packedSize = msg.Pack(messageData.data(), msg.Size());

    if(_type == RtsCommsType::Unencrypted) {
      return _stream->SendPlainText(messageData.data(), packedSize);
    } else if(_type == RtsCommsType::Encrypted) {
      return _stream->SendEncrypted(messageData.data(), packedSize);
    } else {
      Log::Write("Tried to send clad message when state was already set back to RAW.");
    }

    return -1;
  }
};

} // Switchboard
} // Anki