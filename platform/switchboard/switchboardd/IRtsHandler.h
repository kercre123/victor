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
  
  void SetIsPairing(bool pairing) { _isPairing = pairing; }
  void SetOtaUpdating(bool updating) { _isOtaUpdating = updating; }

private:
  bool _isPairing = false;
  bool _isOtaUpdating = false;

protected:
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
};

} // Switchboard
} // Anki