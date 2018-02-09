//
//  SecurePairing.h
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef SecurePairing_h
#define SecurePairing_h

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "INetworkStream.h"
#include "KeyExchange.h"
#include "log.h"
#include "../libev/ev.h"
#include "../libev/ev++.h"
#include "PairingMessages.h"

namespace Anki {
  namespace Networking {
    #define NUM_PIN_DIGITS 6
    #define MIN_MSG_LENGTH 2
    
    enum PairingState {
      Initial,
      AwaitingPartnerReady,
      AwaitingPublicKey,
      AwaitingNonceAck,
      AwaitingChallengeResponse,
      ConfirmedSharedSecret
    };
    
    enum PairingMessageType : uint8_t {
      Nop = 0,
      Ack = 1,
      PublicKey = 2,
      HashedKey = 3,
      Verified = 4,
      Nonce = 5,
    };
    
    struct PairingMessage {
      uint32_t bufferSize;
      PairingMessageType type;
      uint8_t* buffer;
      
    public:
      uint8_t* GetBuffer() {
        uint8_t* bufferCpy = (uint8_t*)malloc(sizeof(uint32_t) + sizeof(PairingMessageType) + bufferSize);
        
        memcpy(bufferCpy, &bufferSize, sizeof(uint32_t));
        memcpy(bufferCpy + sizeof(uint32_t), &type, sizeof(PairingMessageType));
        memcpy(bufferCpy + sizeof(uint32_t) + sizeof(PairingMessageType), buffer, bufferSize);
        
        return bufferCpy;
      }
      
      uint32_t GetSize() {
        return sizeof(uint32_t) + sizeof(PairingMessageType) + bufferSize;
      }
    };
    
    class SecurePairing {
    public:
      SecurePairing(INetworkStream* stream, struct ev_loop* evloop);
      ~SecurePairing();
      
      void BeginPairing();
      void StopPairing();
      
      std::string GetPin() { return _Pin; }
      
      using ReceivedWifiCredentialsSignal = Signal::Signal<void (std::string, std::string)>;
      using UpdatedPinSignal = Signal::Signal<void (std::string)>;
      
      // WiFi Receive Event
      ReceivedWifiCredentialsSignal& OnReceivedWifiCredentialsEvent() {
        return _ReceivedWifiCredentialSignal;
      }
      
      // PIN Update Event
      UpdatedPinSignal& OnUpdatedPinEvent() {
        return _UpdatedPinSignal;
      }
      
    private:
      void Init();
      void Reset(bool forced=false);
      
      template <class T>
      typename std::enable_if<std::is_base_of<Anki::Networking::Message, T>::value, void>::type
      SendPlainText(const T& message);
      
      template <class T>
      typename std::enable_if<std::is_base_of<Anki::Networking::Message, T>::value, void>::type
      SendEncrypted(const T& message);
      
      void HandleMessageReceived(uint8_t* bytes, uint32_t length);
      void HandleEncryptedMessageReceived(uint8_t* bytes, uint32_t length);
      void HandleDecryptionFailed();
      void HandleInitialPair(uint8_t* bytes, uint32_t length);
      void HandleRestoreConnection();
      void HandleCancelSetup();
      void HandleNonceAck();
      void HandleTimeout();
      void HandlePingResponse(uint8_t* bytes, uint32_t length);
      
      void SendPublicKey();
      void SendNonce();
      void SendChallenge();
      void SendCancelPairing();
      void SendChallengeSuccess();
      
      std::string GeneratePin();
      
      void IncrementAbnormalityCount();
      void IncrementChallengeCount();
      
      const uint8_t MAX_MATCH_ATTEMPTS = 5;
      const uint8_t MAX_PAIRING_ATTEMPTS = 3;
      const uint32_t MAX_ABNORMALITY_COUNT = 5;
      const uint16_t PAIRING_TIMEOUT_SECONDS = 10;
      
      std::string _Pin;
      uint8_t _ChallengeAttempts = 0;
      uint8_t _TotalPairingAttempts;
      uint8_t _NumPinDigits;
      uint32_t _PingChallenge;
      uint32_t _AbnormalityCount = 0;
      
      INetworkStream* _Stream;
      //KeyExchange* _KeyExchange;
      std::unique_ptr<KeyExchange> _KeyExchange;
      PairingState _State = PairingState::Initial;
      
      //
      //
      //
      Signal::SmartHandle _OnReceivePlainTextHandle;
      Signal::SmartHandle _OnReceiveEncryptedHandle;
      Signal::SmartHandle _OnFailedDecryptionHandle;
      //
      using PairingTimeoutSignal = Signal::Signal<void ()>;
      PairingTimeoutSignal _PairingTimeoutSignal;
      
      static long long _TimeStarted;
      struct ev_loop* _Loop;
      ev_timer _Timer;
      static void sEvTimerHandler(struct ev_loop* loop, struct ev_timer* w, int revents);
      
      struct ev_TimerStruct {
        ev_timer timer;
        PairingTimeoutSignal* signal;
      };
      ev_TimerStruct _HandleTimeoutTimer;
      //
      //
      //
      
      UpdatedPinSignal _UpdatedPinSignal;
      ReceivedWifiCredentialsSignal _ReceivedWifiCredentialSignal;
    };
  }
}

#endif /* SecurePairing_h */
