/**
 * File: rtsComms.h
 *
 * Author: paluri
 * Created: 6/27/2018
 *
 * Description: Class to facilitate versioned comms with client over BLE
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once

#include <stdlib.h>
#include "ev++.h"
#include "switchboardd/pairingMessages.h"
#include "switchboardd/engineMessagingClient.h"
#include "switchboardd/INetworkStream.h"
#include "switchboardd/IRtsHandler.h"
#include "switchboardd/taskExecutor.h"

namespace Anki {
namespace Switchboard {

class RtsComms {
public:
  // Constructors
  RtsComms(INetworkStream* stream, 
    struct ev_loop* evloop,
    std::shared_ptr<EngineMessagingClient> engineClient,
    bool isPairing,
    bool isOtaUpdating);

  ~RtsComms();

  // Types
  using StringSignal = Signal::Signal<void (std::string)>;
  using VoidSignal = Signal::Signal<void ()>;

  //
  // API
  //

  // Methods
  void BeginPairing();
  void StopPairing();
  void SetIsPairing(bool pairing);
  void SetOtaUpdating(bool updating);
  void SendOtaProgress(int32_t status, uint64_t progress, uint64_t expectedTotal);
  std::string GetPin() { return _pin; }

  // Events
  StringSignal& OnUpdatedPinEvent() { return _updatedPinSignal; }
  StringSignal& OnOtaUpdateRequestEvent() { return _otaUpdateRequestSignal; }
  VoidSignal& OnStopPairingEvent() { return _stopPairingSignal; }
  VoidSignal& OnCompletedPairingEvent() { return _completedPairingSignal; }

private:
  // Methods
  void Init();
  void SendHandshake();
  void HandleMessageReceived(uint8_t* bytes, uint32_t length);
  bool HandleHandshake(uint16_t version);
  void HandleReset(bool forced);
  void HandleTimeout();
  void UpdateFace(Anki::Vector::SwitchboardInterface::ConnectionStatus state);

  // Constants
  const uint8_t kMinMessageSize = 2;
  const uint8_t kMaxPairingAttempts = 3;
  const uint16_t kPairingTimeout_s = 60;

  // Fields
  std::string _pin;
  INetworkStream* _stream;
  struct ev_loop* _loop;
  std::shared_ptr<EngineMessagingClient> _engineClient;
  bool _isPairing = false;
  bool _isOtaUpdating = false;
  uint8_t _totalPairingAttempts;

  StringSignal _updatedPinSignal;
  StringSignal _otaUpdateRequestSignal;
  VoidSignal _stopPairingSignal;
  VoidSignal _completedPairingSignal;

  Signal::SmartHandle _pinHandle;
  Signal::SmartHandle _otaHandle;
  Signal::SmartHandle _endHandle;
  Signal::SmartHandle _completedPairingHandle;
  Signal::SmartHandle _resetHandle;

  Signal::SmartHandle _onReceivePlainTextHandle;
  Signal::SmartHandle _onPairingTimeoutReceived;

  struct ev_TimerStruct {
    ev_timer timer;
    VoidSignal* signal;
  } _handleTimeoutTimer;

  VoidSignal _pairingTimeoutSignal;

  IRtsHandler* _rtsHandler;
  uint32_t _rtsVersion;
  RtsPairingPhase _state = RtsPairingPhase::Initial;
  std::unique_ptr<TaskExecutor> _taskExecutor;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Static methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  static void sEvTimerHandler(struct ev_loop* loop, struct ev_timer* w, int revents) {
    struct ev_TimerStruct *wData = (struct ev_TimerStruct*)w;
    wData->signal->emit();
  }
};

} // Switchboard
} // Anki