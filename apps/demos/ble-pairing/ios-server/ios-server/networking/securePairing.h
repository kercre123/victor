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
#include "log.h"
#include "libev.h"
#include "INetworkStream.h"
#include "keyExchange.h"
#include "pairingMessages.h"
#include "taskExecutor.h"

namespace Anki {
namespace Switchboard {
  enum PairingState : uint8_t {
    Initial,
    AwaitingHandshake,
    AwaitingPublicKey,
    AwaitingNonceAck,
    AwaitingChallengeResponse,
    ConfirmedSharedSecret
  };
  
  class SecurePairing {
  public:
    // Types
    using ReceivedWifiCredentialsSignal = Signal::Signal<void (std::string, std::string)>;
    using UpdatedPinSignal = Signal::Signal<void (std::string)>;
    
    // Constructors
    SecurePairing(INetworkStream* stream, struct ev_loop* evloop);
    ~SecurePairing();
    
    // Methods
    void BeginPairing();
    void StopPairing();
    
    std::string GetPin() {
      return _pin;
    }
    
    // WiFi Receive Event
    ReceivedWifiCredentialsSignal& OnReceivedWifiCredentialsEvent() {
      return _receivedWifiCredentialSignal;
    }
    
    // PIN Update Event
    UpdatedPinSignal& OnUpdatedPinEvent() {
      return _updatedPinSignal;
    }
    
  private:
    // Statics
    static long long sTimeStarted;
    static void sEvTimerHandler(struct ev_loop* loop, struct ev_timer* w, int revents);
    
    // Types
    using PairingTimeoutSignal = Signal::Signal<void ()>;
    
    // Template Methods
    template <class T>
    typename std::enable_if<std::is_base_of<Anki::Switchboard::Message, T>::value, int>::type
    SendPlainText(const T& message);
    
    template <class T>
    typename std::enable_if<std::is_base_of<Anki::Switchboard::Message, T>::value, int>::type
    SendEncrypted(const T& message);
    
    // Methods
    void Init();
    void Reset(bool forced=false);
    
    void HandleMessageReceived(uint8_t* bytes, uint32_t length);
    void HandleEncryptedMessageReceived(uint8_t* bytes, uint32_t length);
    void HandleDecryptionFailed();
    bool HandleHandshake(uint16_t version);
    void HandleInitialPair(uint8_t* bytes, uint32_t length);
    void HandleRestoreConnection();
    void HandleCancelSetup();
    void HandleNonceAck();
    void HandleTimeout();
    void HandleChallengeResponse(uint8_t* bytes, uint32_t length);
    
    void SendHandshake();
    void SendPublicKey();
    void SendNonce();
    void SendChallenge();
    void SendCancelPairing();
    void SendChallengeSuccess();
    
    std::string GeneratePin();
    
    void IncrementAbnormalityCount();
    void IncrementChallengeCount();
    
    // Variables
    const uint8_t kMaxMatchAttempts = 5;
    const uint8_t kMaxPairingAttempts = 10;
    const uint32_t kMaxAbnormalityCount = 5;
    const uint16_t kPairingTimeout_s = 60;
    const uint8_t kNumPinDigits = 6;
    const uint8_t kMinMessageSize = 2;
    
    std::string _pin;
    uint8_t _challengeAttempts;
    uint8_t _totalPairingAttempts;
    uint8_t _numPinDigits;
    uint32_t _pingChallenge;
    uint32_t _abnormalityCount;
    
    INetworkStream* _stream;
    PairingState _state = PairingState::Initial;
    
    std::unique_ptr<KeyExchange> _keyExchange;
    std::unique_ptr<TaskExecutor> _taskExecutor;
    
    Signal::SmartHandle _onReceivePlainTextHandle;
    Signal::SmartHandle _onReceiveEncryptedHandle;
    Signal::SmartHandle _onFailedDecryptionHandle;

    PairingTimeoutSignal _pairingTimeoutSignal;
    
    struct ev_loop* _loop;
    ev_timer _timer;
    
    struct ev_TimerStruct {
      ev_timer timer;
      PairingTimeoutSignal* signal;
    } _handleTimeoutTimer;
    
    UpdatedPinSignal _updatedPinSignal;
    ReceivedWifiCredentialsSignal _receivedWifiCredentialSignal;
  };
} // Switchboard
} // Anki

#endif /* SecurePairing_h */
