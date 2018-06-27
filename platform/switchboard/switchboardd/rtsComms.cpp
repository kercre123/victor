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
_rtsHandler(nullptr),
_rtsVersion(0)
{
  // Register with stream events
  _onReceivePlainTextHandle = _stream->OnReceivedPlainTextEvent().ScopedSubscribe(
    std::bind(&RtsComms::HandleMessageReceived,
    this, std::placeholders::_1, std::placeholders::_2));

  // Initialize the task executor
  _taskExecutor = std::make_unique<TaskExecutor>(_loop);
}

RtsComms::~RtsComms() {
  // cleanup
  _onReceivePlainTextHandle = nullptr;

  if(_rtsHandler != nullptr) {
    delete _rtsHandler;
  }

  Log::Write("Destroying RtsComms object");
}

void RtsComms::BeginPairing() {
   // Update our state
  _state = RtsPairingPhase::Initial;

  // Send Handshake
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
  // todo: forward on to IRtsHandler
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

          switch(clientVersion) {
            case PairingProtocolVersion::CURRENT: {
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
          _rtsHandler->StartRts();
          _onReceivePlainTextHandle = nullptr;
        } else {
          // If we can't handle handshake, must cancel
          // THIS SHOULD NEVER HAPPEN
          Log::Write("Unable to process handshake. Something very bad happened.");
          StopPairing();
        }
      } else {
        // ignore msg
        // todo: disconnect
        Log::Write("Received raw message that is not handshake.");
      }
    } else{
      // ignore msg
      // todo: disconnect
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