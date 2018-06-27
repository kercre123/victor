/**
 * File: securePairing.h
 *
 * Author: paluri
 * Created: 1/16/2018
 *
 * Description: Secure Pairing controller for ankiswitchboardd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef SecurePairing_h
#define SecurePairing_h

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "switchboardd/log.h"
#include "ev++.h"
#include "anki-wifi/wifi.h"
#include "switchboardd/IRtsHandler.h"
#include "switchboardd/rtsHandlerV2.h"
#include "switchboardd/rtsHandlerV3.h"
#include "switchboardd/INetworkStream.h"
#include "switchboardd/keyExchange.h"
#include "switchboardd/pairingMessages.h"
#include "switchboardd/taskExecutor.h"
#include "switchboardd/externalCommsCladHandler.h"
#include "switchboardd/engineMessagingClient.h"
#include "switchboardd/savedSessionManager.h"

namespace Anki {
namespace Switchboard {
  enum CommsState : uint8_t {
    Raw,
    Clad,
    SecureClad
  };
  
  class SecurePairing {
  public:
    // Types
    using ReceivedWifiCredentialsSignal = Signal::Signal<void (std::string, std::string)>;
    using UpdatedPinSignal = Signal::Signal<void (std::string)>;
    using OtaUpdateSignal = Signal::Signal<void (std::string)>;
    using PairingSignal = Signal::Signal<void ()>;
    
    // Constructors
    SecurePairing(INetworkStream* stream, struct ev_loop* evloop, std::shared_ptr<EngineMessagingClient> engineClient, bool isPairing, bool isOtaUpdating);
    ~SecurePairing();
    
    // Methods
    void BeginPairing();
    void StopPairing();
    void SendOtaProgress(int status, uint64_t progress, uint64_t expectedTotal);
    
    std::string GetPin() {
      return _pin;
    }

    void SetOtaUpdating(bool updating) {
      _isOtaUpdating = updating;
    }

    void SetIsPairing(bool pairing) {
      Log::Write("Set isPairing:%s", pairing?"true":"false");
      _isPairing = pairing;
    }
    
    // WiFi Receive Event
    ReceivedWifiCredentialsSignal& OnReceivedWifiCredentialsEvent() {
      return _receivedWifiCredentialSignal;
    }
    
    // PIN Update Event
    UpdatedPinSignal& OnUpdatedPinEvent() {
      return _updatedPinSignal;
    }

    PairingSignal& OnStopPairingEvent() {
      return _stopPairingSignal;
    }

    PairingSignal& OnCompletedPairingEvent() {
      return _completedPairingSignal;
    }

    OtaUpdateSignal& OnOtaUpdateRequestEvent() {
      return _otaUpdateRequestSignal;
    }
    
  private:
    // Statics
    static long long sTimeStarted;
    static void sEvTimerHandler(struct ev_loop* loop, struct ev_timer* w, int revents);
    
    // Types
    using PairingTimeoutSignal = Signal::Signal<void ()>;
    
    // Template Methods
    template<typename T, typename... Args>
    int SendRtsMessage(Args&&... args);
    
    // Methods
    void Init();
    void Reset(bool forced=false);
    bool LoadKeys();
    void SaveKeys();
    
    void HandleMessageReceived(uint8_t* bytes, uint32_t length);
    void HandleDecryptionFailed();
    bool HandleHandshake(uint16_t version);
    void HandleInitialPair(uint8_t* bytes, uint32_t length);
    void HandleCancelSetup();
    void HandleNonceAck();
    void HandleTimeout();
    void HandleInternetTimerTick();
    void HandleIdleTimeout();
    void HandleOtaRequest();
    void HandleChallengeResponse(uint8_t* bytes, uint32_t length);

    void SubscribeToCladMessages();
    void UpdateFace(Anki::Cozmo::SwitchboardInterface::ConnectionStatus state);
    
    inline bool AssertState(CommsState state) {
      return state == _commsState;
    }
    
    void SendHandshake();
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
    
    void IncrementAbnormalityCount();
    void IncrementChallengeCount();

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

    Signal::SmartHandle _rtsSshHandle;
    void HandleRtsSsh(const Cozmo::ExternalComms::RtsConnection_3& msg);
    
    // Variables
    const uint8_t kMaxMatchAttempts = 5;
    const uint8_t kMaxPairingAttempts = 3;
    const uint32_t kMaxAbnormalityCount = 5;
    const uint16_t kPairingTimeout_s = 60;
    const uint8_t kNumPinDigits = 6;
    const uint8_t kMinMessageSize = 2;
    const uint8_t kWifiApPasswordSize = 8;
    const uint8_t kWifiConnectMinTimeout_s = 1;
    const uint8_t kWifiConnectInterval_s = 1;
    const uint8_t kBleIdleConnectionTimeout_s = 5;
    
    std::string _pin;
    uint8_t _challengeAttempts;
    uint8_t _totalPairingAttempts;
    uint8_t _numPinDigits;
    uint32_t _pingChallenge;
    uint32_t _abnormalityCount;
    uint8_t _inetTimerCount;
    uint8_t _wifiConnectTimeout_s;
    
    CommsState _commsState;
    INetworkStream* _stream;
    RtsPairingPhase _state = RtsPairingPhase::Initial;
    RtsKeys _rtsKeys;
    
    std::unique_ptr<KeyExchange> _keyExchange;
    std::unique_ptr<TaskExecutor> _taskExecutor;
    std::unique_ptr<ExternalCommsCladHandler> _cladHandler;
    IRtsHandler* _rtsHandler;
    
    Signal::SmartHandle _onReceivePlainTextHandle;
    Signal::SmartHandle _onReceiveEncryptedHandle;
    Signal::SmartHandle _onFailedDecryptionHandle;

    PairingTimeoutSignal _pairingTimeoutSignal;
    PairingTimeoutSignal _internetTimerSignal;
    PairingTimeoutSignal _idleConnectionSignal;
    
    struct ev_loop* _loop;
    ev_timer _timer;
    
    struct ev_TimerStruct {
      ev_timer timer;
      PairingTimeoutSignal* signal;
    } _handleTimeoutTimer;

    struct ev_TimerStruct _handleInternet;

    struct ev_TimerStruct _idleConnectionTimer;
    
    UpdatedPinSignal _updatedPinSignal;
    ReceivedWifiCredentialsSignal _receivedWifiCredentialSignal;
    std::shared_ptr<EngineMessagingClient> _engineClient;
    bool _isPairing = false;
    bool _isOtaUpdating = false;
    uint32_t _rtsVersion = 0;
    OtaUpdateSignal _otaUpdateRequestSignal;
    PairingSignal _completedPairingSignal;
    PairingSignal _stopPairingSignal;
  };
} // Switchboard
} // Anki

#endif /* SecurePairing_h */
