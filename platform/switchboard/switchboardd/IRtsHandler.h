/**
 * File: IRtsHandler.h
 *
 * Author: paluri
 * Created: 7/9/2018
 *
 * Description: Interface for different version of RTS protocol
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once

#include <stdint.h>
#include "switchboardd/keyExchange.h"
#include "switchboardd/savedSessionManager.h"
#include "switchboardd/pairingMessages.h"
#include "switchboardd/tokenClient.h"
#include "switchboardd/log.h"

namespace Anki {
namespace Switchboard {

enum RtsPairingPhase : uint8_t {
  Initial,
  AwaitingHandshake,
  AwaitingPublicKey,
  AwaitingNonceAck,
  AwaitingChallengeResponse,
  ConfirmedSharedSecret
};

enum RtsCommsType : uint8_t {
  Handshake,
  Unencrypted,
  Encrypted
};

class IRtsHandler {
public:
  virtual ~IRtsHandler() {
  }
  
  virtual bool StartRts() = 0;
  virtual void StopPairing() = 0;
  virtual void SendOtaProgress(int status, uint64_t progress, uint64_t expectedTotal) = 0;
  virtual void HandleTimeout() = 0;
  virtual void ForceDisconnect() = 0;
  
  void SetIsPairing(bool pairing) { _isPairing = pairing; }
  void SetOtaUpdating(bool updating) { _isOtaUpdating = updating; }
  void SetHasOwner(bool hasOwner) { _hasOwner = hasOwner; }


protected:
  IRtsHandler(const bool pairing, const bool updating, bool hasOwner, std::shared_ptr<TokenClient> tokenClient)
  : _isPairing(pairing)
  , _isOtaUpdating(updating)
  , _tokenClient(tokenClient)
  , _hasOwner(hasOwner) {}

  inline bool AssertState(RtsCommsType state) {
    return state == _type;
  } 

  bool LoadKeys();

  void SaveKeys() {
    SavedSessionManager::SaveRtsKeys(_rtsKeys);
  }

  std::unique_ptr<KeyExchange> _keyExchange;

  RtsPairingPhase _state = RtsPairingPhase::AwaitingHandshake;
  RtsCommsType _type = RtsCommsType::Unencrypted;
  RtsKeys _rtsKeys;
  bool _isPairing;
  bool _isOtaUpdating;
  std::shared_ptr<TokenClient> _tokenClient;
  bool _hasOwner;
};

} // Switchboard
} // Anki