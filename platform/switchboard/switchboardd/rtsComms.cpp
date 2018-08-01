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
#include "anki-wifi/wifi.h"

namespace Anki {
namespace Switchboard {

RtsComms::RtsComms(INetworkStream* stream, 
    struct ev_loop* evloop,
    std::shared_ptr<EngineMessagingClient> engineClient,
    bool isPairing,
    bool isOtaUpdating,
    Anki::Wifi::Wifi *wifi) :
_wifi(wifi),
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
  if(_rtsHandler != nullptr) {
    delete _rtsHandler;
    _rtsHandler = nullptr;
  }

  ev_timer_stop(_loop, &_handleTimeoutTimer.timer);
}

void RtsComms::BeginPairing() {  
  // Clear field values
  _totalPairingAttempts = 0;

  Init();
}

void RtsComms::Init() {  
  // Update our state
  _state = RtsPairingPhase::Initial;

  if(_rtsHandler != nullptr) {
    delete _rtsHandler;
    _rtsHandler = nullptr;

    ev_timer_stop(_loop, &_handleTimeoutTimer.timer);
  }

  // Send Handshake
  ev_timer_again(_loop, &_handleTimeoutTimer.timer);
  Log::Write("Sending Handshake to Client.");
  SendHandshake();
  _state = RtsPairingPhase::AwaitingHandshake;
}

void RtsComms::StopPairing() {
  if(_rtsHandler) {
    _rtsHandler->StopPairing();
  }
}

void RtsComms::SetIsPairing(bool pairing) { 
  _isPairing = pairing;

  if(_rtsHandler) {
    _rtsHandler->SetIsPairing(_isPairing);
  }
}

void RtsComms::SetOtaUpdating(bool updating) { 
  _isOtaUpdating = updating; 

  if(_rtsHandler) {
    _rtsHandler->SetOtaUpdating(_isOtaUpdating);
  }
}

void RtsComms::SendHandshake() {
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
  // Send Ota Progress
  if(_rtsHandler) {
    _rtsHandler->SendOtaProgress(status, progress, expectedTotal);
  }
}

void RtsComms::UpdateFace(Anki::Vector::SwitchboardInterface::ConnectionStatus state) {
  if(_engineClient == nullptr) {
    // no engine client -- probably testing
    return;
  }

  if(!_isOtaUpdating) {
    _engineClient->ShowPairingStatus(state);
  } else {
    _engineClient->ShowPairingStatus(Anki::Vector::SwitchboardInterface::ConnectionStatus::UPDATING_OS);
  }
}

void RtsComms::HandleReset(bool forced) {
  // [see: VIC-4306]
  // This "Wake" was added because deleting `IRtsHandler` object 
  // in scope of its own execution was causing a memory corruption, 
  // although the root cause was never discovered.
  // VIC-4306 is a ticket to track the actual discovering and 
  // fixing of the root cause.
  _taskExecutor->Wake([this, forced]() {    
    _state = RtsPairingPhase::Initial;

    // Put us back in initial state
    if(forced) {
      Log::Write("Client disconnected. Stopping pairing.");
      ev_timer_stop(_loop, &_handleTimeoutTimer.timer);
      UpdateFace(Anki::Vector::SwitchboardInterface::ConnectionStatus::END_PAIRING);
    } else if(++_totalPairingAttempts < kMaxPairingAttempts) {
      Init();
      Log::Write("SecurePairing restarting.");
      UpdateFace(Anki::Vector::SwitchboardInterface::ConnectionStatus::START_PAIRING);
    } else {
      Log::Write("SecurePairing ending due to multiple failures. Requires external restart.");
      ev_timer_stop(_loop, &_handleTimeoutTimer.timer);
      _stopPairingSignal.emit();
      UpdateFace(Anki::Vector::SwitchboardInterface::ConnectionStatus::END_PAIRING);
    }
  });
}

void RtsComms::HandleTimeout() {
  if(_rtsHandler) {
    _rtsHandler->HandleTimeout();
  } else {
    // if we aren't beyond handshake, mark as strike
    HandleReset(false);
  }
}

void RtsComms::HandleMessageReceived(uint8_t* bytes, uint32_t length) {
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
          _rtsVersion = clientVersion;
        }

        if(handleHandshake) {
          Log::Write("Starting RtsHandler");
          switch(_rtsVersion) {
            case PairingProtocolVersion::CURRENT: {
              _rtsHandler = (IRtsHandler*)new RtsHandlerV3(_stream, 
                              _loop,
                              _engineClient,
                              _isPairing,
                              _isOtaUpdating,
                              _wifi
                );

              RtsHandlerV3* _v3 = static_cast<RtsHandlerV3*>(_rtsHandler);
              _pinHandle = _v3->OnUpdatedPinEvent().ScopedSubscribe([this](std::string s){
                _pin = s;
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
            case PairingProtocolVersion::FACTORY: {
              _rtsHandler = (IRtsHandler*)new RtsHandlerV2(_stream, 
                              _loop,
                              _engineClient,
                              _isPairing,
                              _isOtaUpdating,
                              _wifi
                );

              RtsHandlerV2* _v2 = static_cast<RtsHandlerV2*>(_rtsHandler);
              _pinHandle = _v2->OnUpdatedPinEvent().ScopedSubscribe([this](std::string s){
                _pin = s;
                this->OnUpdatedPinEvent().emit(s);
              });
              _otaHandle = _v2->OnOtaUpdateRequestEvent().ScopedSubscribe([this](std::string s){
                this->OnOtaUpdateRequestEvent().emit(s);
              });
              _endHandle = _v2->OnStopPairingEvent().ScopedSubscribe([this](){
                this->OnStopPairingEvent().emit();
              });
              _completedPairingHandle = _v2->OnCompletedPairingEvent().ScopedSubscribe([this](){
                this->OnCompletedPairingEvent().emit();
              });
              _resetHandle = _v2->OnResetEvent().ScopedSubscribe(
                std::bind(&RtsComms::HandleReset, this, std::placeholders::_1));
              break;
            }
            default: {
              // this case should never happen,
              // because handleHandshake is true
              Log::Write("Error: handleHandshake is true, but version is not handled.");
              StopPairing();
              return;
            }
          }

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