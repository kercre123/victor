/**
 * File: securePairing.cpp
 *
 * Author: paluri
 * Created: 1/16/2018
 *
 * Description: Secure Pairing controller for ankiswitchboardd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include <sstream>
#include "anki-wifi/wifi.h"
#include "switchboardd/securePairing.h"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Constructors
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
long long Anki::Switchboard::SecurePairing::sTimeStarted;

Anki::Switchboard::SecurePairing::SecurePairing(INetworkStream* stream, struct ev_loop* evloop) :
_pin(""),
_challengeAttempts(0),
_totalPairingAttempts(0),
_numPinDigits(0),
_pingChallenge(0),
_abnormalityCount(0),
_inetTimerCount(0),
_commsState(CommsState::Raw),
_stream(stream),
_loop(evloop)
{
  sTimeStarted = std::time(0);
  
  // Register with stream events
  _onReceivePlainTextHandle = _stream->OnReceivedPlainTextEvent().ScopedSubscribe(
    std::bind(&SecurePairing::HandleMessageReceived,
              this, std::placeholders::_1, std::placeholders::_2));
  
  _onReceiveEncryptedHandle = _stream->OnReceivedEncryptedEvent().ScopedSubscribe(
    std::bind(&SecurePairing::HandleMessageReceived,
              this, std::placeholders::_1, std::placeholders::_2));
  
  _onFailedDecryptionHandle = _stream->OnFailedDecryptionEvent().ScopedSubscribe(
    std::bind(&SecurePairing::HandleDecryptionFailed, this));
  
  // Register with private events
  _pairingTimeoutSignal.SubscribeForever(std::bind(&SecurePairing::HandleTimeout, this));
  _internetTimerSignal.SubscribeForever(std::bind(&SecurePairing::HandleInternetTimerTick, this));
  
  // Initialize the key exchange object
  _keyExchange = std::make_unique<KeyExchange>(kNumPinDigits);

  // Initialize the message handler
  _cladHandler = std::make_unique<ExternalCommsCladHandler>();
  SubscribeToCladMessages();
  
  // Initialize the task executor
  _taskExecutor = std::make_unique<TaskExecutor>(_loop);
  
  // Initialize ev timer
  Log::Write("[timer] init");
  ev_timer_init(&_handleTimeoutTimer.timer, &SecurePairing::sEvTimerHandler, kPairingTimeout_s, kPairingTimeout_s);
  _handleTimeoutTimer.signal = &_pairingTimeoutSignal;

  ev_timer_init(&_handleInternet.timer, &SecurePairing::sEvTimerHandler, kInternetInterval_s, kInternetInterval_s);
  _handleInternet.signal = &_internetTimerSignal;
  
  Log::Write("SecurePairing starting up.");
}

Anki::Switchboard::SecurePairing::~SecurePairing() {
  _onReceivePlainTextHandle = nullptr;
  _onReceiveEncryptedHandle = nullptr;
  _onFailedDecryptionHandle = nullptr;
  
  Log::Write("Destroying SecurePairing object.");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Initialization/Reset methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Anki::Switchboard::SecurePairing::BeginPairing() {
  Log::Write("Beginning secure pairing process.");
  
  _totalPairingAttempts = 0;
  
  // Initialize object
  Init();
  
  Log::Write("[timer] again");
  ev_timer_start(_loop, &_handleTimeoutTimer.timer);
}

void Anki::Switchboard::SecurePairing::StopPairing() {
  _totalPairingAttempts = kMaxPairingAttempts;
  
  Reset(true);
}

void Anki::Switchboard::SecurePairing::Init() {
  // Clear field values
  _challengeAttempts = 0;
  _abnormalityCount = 0;
  _inetTimerCount = 0;
  
  // Generate a random number with kNumPinDigits digits
  _pin = _keyExchange->GeneratePin();
  _updatedPinSignal.emit(_pin);
  
  // Update our state
  _state = PairingState::Initial;
  
  // Send public key
  Log::Write("Sending public key to client.");
  
  ev_timer_again(_loop, &_handleTimeoutTimer.timer);
  
  SendHandshake();
  
  _state = PairingState::AwaitingHandshake;
}

void Anki::Switchboard::SecurePairing::SubscribeToCladMessages() {
  _rtsConnResponseHandle = _cladHandler->OnReceiveRtsConnResponse().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsConnResponse, this, std::placeholders::_1));
  _rtsChallengeMessageHandle = _cladHandler->OnReceiveRtsChallengeMessage().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsChallengeMessage, this, std::placeholders::_1));
  _rtsWifiConnectRequestHandle = _cladHandler->OnReceiveRtsWifiConnectRequest().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsWifiConnectRequest, this, std::placeholders::_1));
  _rtsWifiIpRequestHandle = _cladHandler->OnReceiveRtsWifiIpRequest().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsWifiIpRequest, this, std::placeholders::_1));
  _rtsRtsStatusRequestHandle = _cladHandler->OnReceiveRtsStatusRequest().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsStatusRequest, this, std::placeholders::_1));
  _rtsWifiScanRequestHandle = _cladHandler->OnReceiveRtsWifiScanRequest().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsWifiScanRequest, this, std::placeholders::_1));
  _rtsOtaUpdateRequestHandle = _cladHandler->OnReceiveRtsOtaUpdateRequest().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsOtaUpdateRequest, this, std::placeholders::_1));
  _rtsCancelPairingHandle = _cladHandler->OnReceiveCancelPairingRequest().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsCancelPairing, this, std::placeholders::_1));
  _rtsAckHandle = _cladHandler->OnReceiveRtsAck().ScopedSubscribe(std::bind(&SecurePairing::HandleRtsAck, this, std::placeholders::_1));
}

void Anki::Switchboard::SecurePairing::Reset(bool forced) {
  // Tell the stream that we can no longer send over encrypted channel
  _stream->SetEncryptedChannelEstablished(false);
  _commsState = CommsState::Raw;
  
  // Tell key exchange to reset
  _keyExchange->Reset();
  
  // Clear pin
  _pin = "000000";
  
  // Send cancel message
  SendCancelPairing();
  
  // Put us back in initial state
  if(forced) {
    Log::Write("Client disconnected. Stopping pairing.");
    ev_timer_stop(_loop, &_handleTimeoutTimer.timer);
  } else if(++_totalPairingAttempts < kMaxPairingAttempts) {
    Init();
    Log::Write("SecurePairing restarting.");
  } else {
    Log::Write("SecurePairing ending due to multiple failures. Requires external restart.");
    
    Log::Write("[timer] stop");
    ev_timer_stop(_loop, &_handleTimeoutTimer.timer);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Send data methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

using namespace Anki::Victor::ExternalComms;

void Anki::Switchboard::SecurePairing::SendHandshake() {
  if(!AssertState(CommsState::Raw)) {
    return;
  }
  // Send versioning handshake
  // ************************************************************
  // Handshake Message (first message)
  // This message is fixed. Cannot change. Ever. 
  // If you are thinking about changing the code in this message,
  // DON'T. All Victor's for all time must send this message.
  // ************************************************************
  const uint8_t kHandshakeMessageLength = 5;
  uint8_t handshakeMessage[kHandshakeMessageLength];
  handshakeMessage[0] = SetupMessage::MSG_HANDSHAKE;
  *(uint32_t*)(&handshakeMessage[1]) = PairingProtocolVersion::CURRENT;

  _stream->SendPlainText(handshakeMessage, sizeof(handshakeMessage));
}

void Anki::Switchboard::SecurePairing::SendPublicKey() {
  if(!AssertState(CommsState::Raw)) {
    return;
  }

  // Generate public, private key
  uint8_t* publicKey = (uint8_t*)_keyExchange->GenerateKeys();

  _commsState = CommsState::Clad;
  std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> publicKeyArray;
  memcpy(std::begin(publicKeyArray), publicKey, crypto_kx_PUBLICKEYBYTES);
  SendRtsMessage<RtsConnRequest>(publicKeyArray);
}

void Anki::Switchboard::SecurePairing::SendNonce() {
  if(!AssertState(CommsState::Clad)) {
    return;
  }

  // Send nonce
  const uint8_t NONCE_BYTES = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
  
  // Generate a nonce
  uint8_t* nonce = _keyExchange->GetNonce();
  randombytes_buf(nonce, NONCE_BYTES);
  
  // Give our nonce to the network stream
  _stream->SetNonce(nonce);
  
  std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES> nonceArray;
  memcpy(std::begin(nonceArray), nonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  SendRtsMessage<RtsNonceMessage>(nonceArray);
}

void Anki::Switchboard::SecurePairing::SendChallenge() {
  if(!AssertState(CommsState::Clad)) {
    return;
  }

  // Tell the stream that we can now send over encrypted channel
  _stream->SetEncryptedChannelEstablished(true);
  // Update state to secureClad
  _commsState = CommsState::SecureClad;
  _state = PairingState::AwaitingChallengeResponse;
  
  // Create random challenge value
  randombytes_buf(&_pingChallenge, sizeof(uint32_t));
  
  SendRtsMessage<RtsChallengeMessage>(_pingChallenge);
}

void Anki::Switchboard::SecurePairing::SendChallengeSuccess() {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  // Send challenge and update state
  SendRtsMessage<RtsChallengeSuccessMessage>();
}

void Anki::Switchboard::SecurePairing::SendWifiScanResult() {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  // TODO: will replace with CLAD message format
  std::vector<Anki::WiFiScanResult> wifiResults = Anki::ScanForWiFiAccessPoints();

  const uint8_t statusCode = wifiResults.size() > 0? 0 : 1;

  std::vector<Anki::Victor::ExternalComms::RtsWifiScanResult> wifiScanResults;

  for(int i = 0; i < wifiResults.size(); i++) {
    Anki::Victor::ExternalComms::RtsWifiScanResult result = Anki::Victor::ExternalComms::RtsWifiScanResult(wifiResults[i].auth,
      wifiResults[i].signal_level,
      wifiResults[i].ssid);

      wifiScanResults.push_back(result);
  }

  Log::Write("Sending wifi scan results.");
  SendRtsMessage<RtsWifiScanResponse>(statusCode, wifiScanResults);
}

void Anki::Switchboard::SecurePairing::SendWifiConnectResult(bool success) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  // Send challenge and update state
  SendRtsMessage<RtsWifiConnectResponse>(success? 1 : 0);
}

void Anki::Switchboard::SecurePairing::SendCancelPairing() {
  // Send challenge and update state
  SendRtsMessage<RtsCancelPairing>();
  Log::Write("Canceling pairing.");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Event handling methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Anki::Switchboard::SecurePairing::HandleRtsConnResponse(const Anki::Victor::ExternalComms::RtsConnection& msg) {
  if(!AssertState(CommsState::Clad)) {
    return;
  }

  if(_state == PairingState::AwaitingPublicKey) {
    Anki::Victor::ExternalComms::RtsConnResponse connResponse = msg.Get_RtsConnResponse();

    if(connResponse.connectionType == Anki::Victor::ExternalComms::RtsConnType::FirstTimePair) {    
      HandleInitialPair((uint8_t*)connResponse.publicKey.data(), crypto_kx_PUBLICKEYBYTES);
    } else {
      SendNonce();
      Log::Write("Received renew connection request.");
    }
    _state = PairingState::AwaitingNonceAck;
  } else {
    // ignore msg
    IncrementAbnormalityCount();
    Log::Write("Received initial pair request in wrong state.");
  }
}

void Anki::Switchboard::SecurePairing::HandleRtsChallengeMessage(const Victor::ExternalComms::RtsConnection& msg) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  if(_state == PairingState::AwaitingChallengeResponse) {
    Anki::Victor::ExternalComms::RtsChallengeMessage challengeMessage = msg.Get_RtsChallengeMessage();

    HandleChallengeResponse((uint8_t*)&challengeMessage.number, sizeof(challengeMessage.number));
  } else {
    // ignore msg
    IncrementAbnormalityCount();
    Log::Write("Received challenge response in wrong state.");
  }
}

void Anki::Switchboard::SecurePairing::HandleRtsWifiConnectRequest(const Victor::ExternalComms::RtsConnection& msg) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  if(_state == PairingState::ConfirmedSharedSecret) {
    Anki::Victor::ExternalComms::RtsWifiConnectRequest challengeMessage = msg.Get_RtsWifiConnectRequest();

    bool connected = Anki::ConnectWiFiBySsid(challengeMessage.ssid, 
      challengeMessage.password,
      nullptr,
      nullptr);

    if(Anki::HasInternet()) {
      SendWifiConnectResult(connected);
    } else {
      ev_timer_again(_loop, &_handleInternet.timer);
    }

    if(connected) {
      Log::Write("Connected to wifi.");
    } else {
      Log::Write("Could not connect to wifi.");
    }
  } else {
    Log::Write("Received wifi credentials in wrong state.");
  }
}

void Anki::Switchboard::SecurePairing::HandleRtsWifiIpRequest(const Victor::ExternalComms::RtsConnection& msg) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }
}

void Anki::Switchboard::SecurePairing::HandleRtsStatusRequest(const Victor::ExternalComms::RtsConnection& msg) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }
}

void Anki::Switchboard::SecurePairing::HandleRtsWifiScanRequest(const Victor::ExternalComms::RtsConnection& msg) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }

  if(_state == PairingState::ConfirmedSharedSecret) {
    SendWifiScanResult();
  } else {
    Log::Write("Received wifi scan request in wrong state.");
  }
}

void Anki::Switchboard::SecurePairing::HandleRtsOtaUpdateRequest(const Victor::ExternalComms::RtsConnection& msg) {
  if(!AssertState(CommsState::SecureClad)) {
    return;
  }
  
  Log::Write("Todo: Execute Rts Ota Update request.");
}

void Anki::Switchboard::SecurePairing::HandleRtsCancelPairing(const Victor::ExternalComms::RtsConnection& msg) {
  Log::Write("Stopping pairing due to client request.");
  StopPairing();
}

void Anki::Switchboard::SecurePairing::HandleRtsAck(const Victor::ExternalComms::RtsConnection& msg) {
  Anki::Victor::ExternalComms::RtsAck ack = msg.Get_RtsAck();
  if(_state == PairingState::AwaitingNonceAck && 
    ack.rtsConnectionTag == (uint8_t)Anki::Victor::ExternalComms::RtsConnectionTag::RtsNonceMessage) {
    HandleNonceAck();
  } else {
    // ignore msg
    IncrementAbnormalityCount();
    
    std::ostringstream ss;
    ss << "Received nonce ack in wrong state '" << _state << "'.";
    Log::Write(ss.str().c_str());
  }
}

bool Anki::Switchboard::SecurePairing::HandleHandshake(uint16_t version) {
  // Todo: in the future, when there are more versions,
  // this method will need to handle telling this object
  // to properly switch states to adjust to proper version.
  if(version == PairingProtocolVersion::CURRENT) {
    return true;
  }
  else if(version == PairingProtocolVersion::INVALID) {
    // Client should never send us this message.
    Log::Write("Client reported incompatible version.");
    return false;
  }
  
  return false;
}

void Anki::Switchboard::SecurePairing::HandleInitialPair(uint8_t* publicKey, uint32_t publicKeyLength) {
  // Handle initial pair request from client
  
  // Input client's public key and calculate shared keys
  _keyExchange->SetRemotePublicKey(publicKey);
  _keyExchange->CalculateSharedKeys((unsigned char*)_pin.c_str());
  
  // Give our shared keys to the network stream
  _stream->SetCryptoKeys(
    _keyExchange->GetEncryptKey(),
    _keyExchange->GetDecryptKey());
  
  // Send nonce
  SendNonce();
  
  Log::Write("Received initial pair request, sending nonce.");
}

void Anki::Switchboard::SecurePairing::HandleTimeout() {
  if(_state != PairingState::ConfirmedSharedSecret) {
    Log::Write("Pairing timeout. Client took too long.");
    Reset();
  }
}

void Anki::Switchboard::SecurePairing::HandleRestoreConnection() {
  // todo: implement
  Reset();
}

void Anki::Switchboard::SecurePairing::HandleDecryptionFailed() {
  // todo: implement
  Log::Write("Decryption failed...");
  Reset();
}

void Anki::Switchboard::SecurePairing::HandleNonceAck() {
  // Send challenge to user
  SendChallenge();
  
  Log::Write("Client acked nonce, sending challenge.");
}

inline bool isChallengeSuccess(uint32_t challenge, uint32_t answer) {
  const bool isSuccess = answer == challenge + 1;
  return isSuccess;
}

void Anki::Switchboard::SecurePairing::HandleChallengeResponse(uint8_t* pingChallengeAnswer, uint32_t length) {
  bool success = false;
  
  if(length < sizeof(uint32_t)) {
    success = false;
  } else {
    uint32_t answer = *((uint32_t*)pingChallengeAnswer);
    success = isChallengeSuccess(_pingChallenge, answer);
  }
  
  if(success) {
    // Inform client that we are good to go and
    // update our state
    SendChallengeSuccess();
    _state = PairingState::ConfirmedSharedSecret;
    Log::Write("Challenge answer was accepted. Encrypted channel established.");
  } else {
    // Increment our abnormality and attack counter, and
    // if at or above max attempts reset.
    IncrementAbnormalityCount();
    IncrementChallengeCount();
    Log::Write("Received faulty challenge response.");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Receive messages method
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Anki::Switchboard::SecurePairing::HandleMessageReceived(uint8_t* bytes, uint32_t length) {
  _taskExecutor->WakeSync([this, bytes, length]() {
    Log::Write("Handling plain text message received.");
    if(length < kMinMessageSize) {
      Log::Write("Length is less than kMinMessageSize.");
      return;
    }

    if(_commsState == CommsState::Raw) {
      if(_state == PairingState::Initial ||
        _state == PairingState::AwaitingHandshake) {
        // ************************************************************
        // Handshake Message (first message)
        // This message is fixed. Cannot change. Ever.
        // ************************************************************
        if((Anki::Switchboard::SetupMessage)bytes[0] == Anki::Switchboard::SetupMessage::MSG_HANDSHAKE) {
          uint32_t clientVersion = *(uint32_t*)(bytes + 1);
          bool handleHandshake = HandleHandshake(clientVersion);
          
          if(handleHandshake) {
            SendPublicKey();
            _state = PairingState::AwaitingPublicKey;
          } else {
            // If we can't handle handshake, must cancel
            // THIS SHOULD NEVER HAPPEN
            Log::Write("Unable to process handshake. Something very bad happened.");
            StopPairing();
          }
        } else {
          // ignore msg
        IncrementAbnormalityCount();
        Log::Write("Received raw message that is not handshake.");
        }
      } else{
        // ignore msg
        IncrementAbnormalityCount();
        Log::Write("Internal state machine error. Assuming raw message, but state is not initial.");
      }
    } else {
      Log::Write("CommsState is Clad or SecureClad.");
      _cladHandler->ReceiveExternalCommsMsg(bytes, length);
    }
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Anki::Switchboard::SecurePairing::IncrementChallengeCount() {
  // Increment challenge count
  _challengeAttempts++;
  
  if(_challengeAttempts >= kMaxMatchAttempts) {
    Reset();
  }
  
  Log::Write("Client answered challenge.");
}

void Anki::Switchboard::SecurePairing::IncrementAbnormalityCount() {
  // Increment abnormality count
  _abnormalityCount++;
  
  if(_abnormalityCount >= kMaxAbnormalityCount) {
    Reset();
  }
  
  Log::Write("Abnormality recorded.");
}

void Anki::Switchboard::SecurePairing::HandleInternetTimerTick() {
  _inetTimerCount++;

  if(Anki::HasInternet()) {
    ev_timer_stop(_loop, &_handleInternet.timer);
    SendWifiConnectResult(true);
  } else if(_inetTimerCount > kInternetTimerTimeout_s) {
    ev_timer_stop(_loop, &_handleInternet.timer);
    SendWifiConnectResult(false);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Send messages method
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<typename T, typename... Args>
int Anki::Switchboard::SecurePairing::SendRtsMessage(Args&&... args) {
  Anki::Victor::ExternalComms::ExternalComms msg = Anki::Victor::ExternalComms::ExternalComms(Anki::Victor::ExternalComms::RtsConnection(T(std::forward<Args>(args)...)));
  std::vector<uint8_t> messageData(msg.Size());
  const size_t packedSize = msg.Pack(messageData.data(), msg.Size());

  if(_commsState == CommsState::Clad) {
    return _stream->SendPlainText(messageData.data(), packedSize);
  } else if(_commsState == CommsState::SecureClad) {
    return _stream->SendEncrypted(messageData.data(), packedSize);
  }

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Static methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Anki::Switchboard::SecurePairing::sEvTimerHandler(struct ev_loop* loop, struct ev_timer* w, int revents)
{
  std::ostringstream ss;
  ss << "[timer] " << (time(0) - sTimeStarted) << "s since beginning.";
  Log::Write(ss.str().c_str());
  
  struct ev_TimerStruct *wData = (struct ev_TimerStruct*)w;
  wData->signal->emit();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// EOF
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
