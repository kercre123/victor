/**
 * File: rtsComms.cpp
 *
 * Author: paluri
 * Created: 6/27/2018
 *
 * Description: Class to facilitate versioned comms with client over BLE
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "switchboardd/rtsHandlerV2.h"
#include "switchboardd/rtsHandlerV3.h"
#include "switchboardd/rtsComms.h"

namespace Anki {
namespace Switchboard {

RtsComms::RtsComms(INetworkStream* stream, 
    struct ev_loop* evloop,
    std::shared_ptr<EngineMessagingClient> engineClient,
    bool isPairing,
    bool isOtaUpdating) :
_pin(""),
_stream(stream),
_loop(evloop),
_engineClient(engineClient),
_isPairing(isPairing),
_isOtaUpdating(isOtaUpdating),
_totalPairingAttempts(0),
_rtsHandler(nullptr),
_rtsVersion(0)
{
  Log::Write("RtsComms");
  // Register with stream events
  _onReceivePlainTextHandle = _stream->OnReceivedPlainTextEvent().ScopedSubscribe(
    std::bind(&RtsComms::HandleMessageReceived,
    this, std::placeholders::_1, std::placeholders::_2));

  // pairing timeout
  _onPairingTimeoutReceived = _pairingTimeoutSignal.ScopedSubscribe(std::bind(&RtsComms::HandleTimeout, this));
  _handleTimeoutTimer.signal = &_pairingTimeoutSignal;
  ev_timer_init(&_handleTimeoutTimer.timer, &RtsComms::sEvTimerHandler, kPairingTimeout_s, kPairingTimeout_s);

  // Initialize the task executor
  _taskExecutor = std::make_unique<TaskExecutor>(_loop);
}

RtsComms::~RtsComms() {
  Log::Write("~RtsComms");

  // cleanup
  //if(_onReceivePlainTextHandle) {
  //  _onReceivePlainTextHandle = nullptr;
  //}
  //_onPairingTimeoutReceived = nullptr;

  if(_rtsHandler != nullptr) {
    Log::Write("--> Delete");
    delete _rtsHandler;
    _rtsHandler = nullptr;
  }

  ev_timer_stop(_loop, &_handleTimeoutTimer.timer);

  Log::Write("Destroying RtsComms object");
}

void RtsComms::BeginPairing() {
  Log::Write("BeginPairing");
  
  // Clear field values
  _totalPairingAttempts = 0;

  Init();
}

void RtsComms::Init() {
  Log::Write("Init");
  
  // Update our state
  _state = RtsPairingPhase::Initial;

  if(_rtsHandler != nullptr) {
    ev_timer_stop(_loop, &_handleTimeoutTimer.timer);
    // todo:  something about this delete 
    //        is causing a corrupted double linked list crash
    Log::Write("--> Delete");
    delete _rtsHandler;
    _rtsHandler = nullptr;
  }

  // Send Handshake
  ev_timer_again(_loop, &_handleTimeoutTimer.timer);
  Log::Write("Sending Handshake to Client.");
  SendHandshake();
  _state = RtsPairingPhase::AwaitingHandshake;
}

void RtsComms::StopPairing() {
  Log::Write("StopPairing/RtsComms");

  if(_rtsHandler) {
    Log::Write("???");
    _rtsHandler->StopPairing();
  }
}

void RtsComms::SetIsPairing(bool pairing) { 
  Log::Write("SetIsPairing");

  _isPairing = pairing;

  if(_rtsHandler) {
    Log::Write("SetIsPairing/??");
    _rtsHandler->SetIsPairing(_isPairing);
  }
}

void RtsComms::SetOtaUpdating(bool updating) { 
  Log::Write("SetOtaUpdating");

  _isOtaUpdating = updating; 

  if(_rtsHandler) {
    Log::Write("SetOtaUpdating/??");
    _rtsHandler->SetOtaUpdating(_isOtaUpdating);
  }
}

void RtsComms::SendHandshake() {
  Log::Write("SendHandshake");

  if(_state != RtsPairingPhase::Initial) {
    return;
  }
  // Send versioning handshake
  // ************************************************************
  // Handshake Message (first message)
  // This message is fixed. Cannot change. Ever.
  // If you are thinking about changing the code in this message,
  // DON'T. All Victor's for all time must send this message.
  // ANY victor needs to be able to communicate with
  // ANY version of the client, at least enough to
  // know if they can speak the same language.
  // ************************************************************
  const uint8_t kHandshakeMessageLength = 5;
  uint8_t handshakeMessage[kHandshakeMessageLength];
  handshakeMessage[0] = SetupMessage::MSG_HANDSHAKE;
  *(uint32_t*)(&handshakeMessage[1]) = PairingProtocolVersion::CURRENT;
  int result = _stream->SendPlainText(handshakeMessage, sizeof(handshakeMessage));

  if(result != 0) {
    Log::Write("Unable to send message.");
  }
}

void RtsComms::SendOtaProgress(int status, uint64_t progress, uint64_t expectedTotal) {
  Log::Write("SendOtaProgress");

  // Send Ota Progress
  if(_rtsHandler) {
    Log::Write("SendOtaProgress/??");
    _rtsHandler->SendOtaProgress(status, progress, expectedTotal);
  }
}

void RtsComms::UpdateFace(Anki::Cozmo::SwitchboardInterface::ConnectionStatus state) {
  Log::Write("UpdateFace");
  
  if(_engineClient == nullptr) {
    // no engine client -- probably testing
    return;
  }

  if(!_isOtaUpdating) {
    _engineClient->ShowPairingStatus(state);
  } else {
    _engineClient->ShowPairingStatus(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::UPDATING_OS);
  }
}

void RtsComms::HandleReset(bool forced) {
  Log::Write("HandleReset");

  _state = RtsPairingPhase::Initial;

  // Tell key exchange to reset
  //_keyExchange->Reset();
  
  // Put us back in initial state
  if(forced) {
    Log::Write("Client disconnected. Stopping pairing.");
    ev_timer_stop(_loop, &_handleTimeoutTimer.timer);
    UpdateFace(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::END_PAIRING);
  } else if(++_totalPairingAttempts < kMaxPairingAttempts) {
    Init();
    Log::Write("SecurePairing restarting.");
    UpdateFace(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::START_PAIRING);
  } else {
    Log::Write("SecurePairing ending due to multiple failures. Requires external restart.");
    ev_timer_stop(_loop, &_handleTimeoutTimer.timer);
    _stopPairingSignal.emit();
    UpdateFace(Anki::Cozmo::SwitchboardInterface::ConnectionStatus::END_PAIRING);
  }
}

void RtsComms::HandleTimeout() {
  Log::Write("A");
  if(_rtsHandler) {
    Log::Write("1");
    _rtsHandler->HandleTimeout();
    Log::Write("2");
  } else {
    // if we aren't beyond handshake, mark as strike
    Log::Write("11");
    HandleReset(false);
    Log::Write("22");
  }
  Log::Write("B");
}

void RtsComms::HandleMessageReceived(uint8_t* bytes, uint32_t length) {
  Log::Write("HandleMessageReceived");
  _taskExecutor->WakeSync([this, bytes, length]() {
    if(length < kMinMessageSize) {
      Log::Write("Length is less than kMinMessageSize.");
      return;
    }

    if(_state == RtsPairingPhase::AwaitingHandshake) {
      // ************************************************************
      // Handshake Message (first message)
      // This message is fixed. Cannot change. Ever.
      // ANY victor needs to be able to communicate with
      // ANY version of the client, at least enough to
      // know if they can speak the same language.
      // ************************************************************
      if((SetupMessage)bytes[0] == SetupMessage::MSG_HANDSHAKE) {
        bool handleHandshake = false;

        if(length < sizeof(uint32_t) + 1) {
          Log::Write("Handshake message too short.");
        } else {
          uint32_t clientVersion = *(uint32_t*)(bytes + 1);
          handleHandshake = HandleHandshake(clientVersion);

          Log::Write("Searching for compatible comms version...");
          switch(clientVersion) {
            case PairingProtocolVersion::CURRENT: {
              Log::Write("--> Create");
              _rtsHandler = (IRtsHandler*)new RtsHandlerV3(_stream, 
                              _loop,
                              _engineClient,
                              _isPairing,
                              _isOtaUpdating);

              RtsHandlerV3* _v3 = (RtsHandlerV3*)_rtsHandler;
              _pinHandle = _v3->OnUpdatedPinEvent().ScopedSubscribe([this](std::string s){
                this->OnUpdatedPinEvent().emit(s);
              });
              _otaHandle = _v3->OnOtaUpdateRequestEvent().ScopedSubscribe([this](std::string s){
                this->OnOtaUpdateRequestEvent().emit(s);
              });
              _endHandle = _v3->OnStopPairingEvent().ScopedSubscribe([this](){
                this->OnStopPairingEvent().emit();
              });
              _completedPairingHandle = _v3->OnCompletedPairingEvent().ScopedSubscribe([this](){
                this->OnCompletedPairingEvent().emit();
              });
              _resetHandle = _v3->OnResetEvent().ScopedSubscribe(
                std::bind(&RtsComms::HandleReset, this, std::placeholders::_1));

              break;
            }
            default:
            case PairingProtocolVersion::FACTORY:
              _rtsHandler = (IRtsHandler*)new RtsHandlerV2();
              break;
          }

          _rtsVersion = clientVersion;
        }

        if(handleHandshake) {
          Log::Write("Starting RtsHandler");
          _rtsHandler->StartRts();
          _onReceivePlainTextHandle = nullptr;
          _state = RtsPairingPhase::AwaitingPublicKey;
        } else {
          // If we can't handle handshake, must cancel
          // THIS SHOULD NEVER HAPPEN
          Log::Write("Unable to process handshake. Something very bad happened.");
          StopPairing();
        }
      } else {
        // ignore msg
        StopPairing();
        Log::Write("Received raw message that is not handshake.");
      }
    } else{
      // ignore msg
      StopPairing();
      Log::Write("Internal state machine error. Assuming raw message, but state is not initial [%d].", (int)_state);
    }
  });
}

bool RtsComms::HandleHandshake(uint16_t version) {
  Log::Write("HandleHandshake");
  // our supported versions
  if((version == PairingProtocolVersion::CURRENT) ||
     (version == PairingProtocolVersion::FACTORY)) {
    return true;
  }
  else if(version == PairingProtocolVersion::INVALID) {
    // Client should never send us this message.
    Log::Write("Client reported incompatible version [%d]. Our version is [%d]", version, PairingProtocolVersion::CURRENT);
    return false;
  }

  return false;
}

} // Switchboard
} // Anki