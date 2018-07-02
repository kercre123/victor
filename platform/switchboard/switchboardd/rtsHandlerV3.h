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
#include "switchboardd/taskExecutor.h"
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

  ~RtsHandlerV3();

  bool StartRts();
  void StopPairing();
  void SendOtaProgress(int status, uint64_t progress, uint64_t expectedTotal);
  void HandleTimeout();

  // Types
  using StringSignal = Signal::Signal<void (std::string)>;
  using BoolSignal = Signal::Signal<void (bool)>;
  using VoidSignal = Signal::Signal<void ()>;

  // Events
  StringSignal& OnUpdatedPinEvent() { return _updatedPinSignal; }
  StringSignal& OnOtaUpdateRequestEvent() { return _otaUpdateRequestSignal; }
  VoidSignal& OnStopPairingEvent() { return _stopPairingSignal; }
  VoidSignal& OnCompletedPairingEvent() { return _completedPairingSignal; }
  BoolSignal& OnResetEvent() { return _resetSignal; }

private:
  // Statics
  static long long sTimeStarted;
  static void sEvTimerHandler(struct ev_loop* loop, struct ev_timer* w, int revents);

  void Reset(bool forced=false);

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

  void HandleMessageReceived(uint8_t* bytes, uint32_t length);
  void HandleDecryptionFailed();
  void HandleInitialPair(uint8_t* bytes, uint32_t length);
  void HandleCancelSetup();
  void HandleNonceAck();
  void HandleInternetTimerTick();
  void HandleOtaRequest();
  void HandleChallengeResponse(uint8_t* bytes, uint32_t length);

  void IncrementAbnormalityCount();
  void IncrementChallengeCount();

  void SubscribeToCladMessages();
  void UpdateFace(Anki::Cozmo::SwitchboardInterface::ConnectionStatus state);

  INetworkStream* _stream;
  struct ev_loop* _loop;
  std::shared_ptr<EngineMessagingClient> _engineClient;
  std::unique_ptr<TaskExecutor> _taskExecutor;
  std::unique_ptr<ExternalCommsCladHandler> _cladHandler;
  bool _isPairing;
  bool _isOtaUpdating;

  const uint8_t kMaxMatchAttempts = 5;
  const uint8_t kMaxPairingAttempts = 3;
  const uint32_t kMaxAbnormalityCount = 5;
  const uint8_t kWifiApPasswordSize = 8;
  const uint8_t kNumPinDigits = 6;
  const uint8_t kWifiConnectMinTimeout_s = 1;
  const uint8_t kWifiConnectInterval_s = 1;
  const uint8_t kMinMessageSize = 2;
  
  std::string _pin;
  uint8_t _challengeAttempts;
  uint8_t _numPinDigits;
  uint32_t _pingChallenge;
  uint32_t _abnormalityCount;
  uint8_t _inetTimerCount;
  uint8_t _wifiConnectTimeout_s;

  Signal::SmartHandle _onReceivePlainTextHandle;
  Signal::SmartHandle _onReceiveEncryptedHandle;
  Signal::SmartHandle _onFailedDecryptionHandle;

  struct ev_TimerStruct {
    ev_timer timer;
    VoidSignal* signal;
  } _handleInternet;

  StringSignal _updatedPinSignal;
  StringSignal _otaUpdateRequestSignal;
  VoidSignal _stopPairingSignal;
  VoidSignal _completedPairingSignal;
  BoolSignal _resetSignal;

  VoidSignal _internetTimerSignal;

  //
  // V3 Request to Listen for
  //
  Signal::SmartHandle _rtsConnResponseHandle;
  void HandleRtsConnResponse(const Cozmo::ExternalComms::RtsConnection_3& msg);

  Signal::SmartHandle _rtsChallengeMessageHandle;
  void HandleRtsChallengeMessage(const Cozmo::ExternalComms::RtsConnection_3& msg);

  Signal::SmartHandle _rtsWifiConnectRequestHandle;
  void HandleRtsWifiConnectRequest(const Cozmo::ExternalComms::RtsConnection_3& msg);

  Signal::SmartHandle _rtsWifiIpRequestHandle;
  void HandleRtsWifiIpRequest(const Cozmo::ExternalComms::RtsConnection_3& msg);

  Signal::SmartHandle _rtsRtsStatusRequestHandle;
  void HandleRtsStatusRequest(const Cozmo::ExternalComms::RtsConnection_3& msg);

  Signal::SmartHandle _rtsWifiScanRequestHandle;
  void HandleRtsWifiScanRequest(const Cozmo::ExternalComms::RtsConnection_3& msg);

  Signal::SmartHandle _rtsWifiForgetRequestHandle;
  void HandleRtsWifiForgetRequest(const Cozmo::ExternalComms::RtsConnection_3& msg);

  Signal::SmartHandle _rtsOtaUpdateRequestHandle;
  void HandleRtsOtaUpdateRequest(const Cozmo::ExternalComms::RtsConnection_3& msg);

  Signal::SmartHandle _rtsOtaCancelRequestHandle;
  void HandleRtsOtaCancelRequest(const Cozmo::ExternalComms::RtsConnection_3& msg);

  Signal::SmartHandle _rtsWifiAccessPointRequestHandle;
  void HandleRtsWifiAccessPointRequest(const Cozmo::ExternalComms::RtsConnection_3& msg);

  Signal::SmartHandle _rtsCancelPairingHandle;
  void HandleRtsCancelPairing(const Cozmo::ExternalComms::RtsConnection_3& msg);

  Signal::SmartHandle _rtsAckHandle;
  void HandleRtsAck(const Cozmo::ExternalComms::RtsConnection_3& msg);

  Signal::SmartHandle _rtsLogRequestHandle;
  void HandleRtsLogRequest(const Cozmo::ExternalComms::RtsConnection_3& msg);

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