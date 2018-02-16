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
#include <random>
#include "securePairing.h"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Constructors
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
long long Anki::Switchboard::SecurePairing::sTimeStarted;

Anki::Switchboard::SecurePairing::SecurePairing(INetworkStream* stream, struct ev_loop* evloop) :
_stream(stream),
_loop(evloop),
_totalPairingAttempts(0),
_numPinDigits(0),
_pingChallenge(0),
_abnormalityCount(0),
_challengeAttempts(0),
_pin("")
{
  sTimeStarted = std::time(0);
  
  // Register with stream events
  _onReceivePlainTextHandle = _stream->OnReceivedPlainTextEvent().ScopedSubscribe(
    std::bind(&SecurePairing::HandleMessageReceived,
              this, std::placeholders::_1, std::placeholders::_2));
  
  _onReceiveEncryptedHandle = _stream->OnReceivedEncryptedEvent().ScopedSubscribe(
    std::bind(&SecurePairing::HandleEncryptedMessageReceived,
              this, std::placeholders::_1, std::placeholders::_2));
  
  _onFailedDecryptionHandle = _stream->OnFailedDecryptionEvent().ScopedSubscribe(
    std::bind(&SecurePairing::HandleDecryptionFailed, this));
  
  // Register with private events
  _pairingTimeoutSignal.SubscribeForever(std::bind(&SecurePairing::HandleTimeout, this));
  
  // Initialize the key exchange object
  _keyExchange = std::make_unique<KeyExchange>(kNumPinDigits);
  
  // Initialize the task executor
  _taskExecutor = std::make_unique<TaskExecutor>(_loop);
  
  // Initialize ev timer
  Log::Write("[timer] init");
  ev_timer_init(&_handleTimeoutTimer.timer, &SecurePairing::sEvTimerHandler, kPairingTimeout_s, kPairingTimeout_s);
  _handleTimeoutTimer.signal = &_pairingTimeoutSignal;
  
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
  
  // Generate a random number with kNumPinDigits digits
  _pin = GeneratePin();
  _updatedPinSignal.emit(_pin);
  
  // Update our state
  _state = PairingState::Initial;
  
  // Send public key
  Log::Write("Sending public key to client.");
  
  ev_timer_again(_loop, &_handleTimeoutTimer.timer);
  
  SendHandshake();
  
  _state = PairingState::AwaitingHandshake;
}

void Anki::Switchboard::SecurePairing::Reset(bool forced) {
  // Tell the stream that we can no longer send over encrypted channel
  _stream->SetEncryptedChannelEstablished(false);
  
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

void Anki::Switchboard::SecurePairing::SendHandshake() {
  // Send versioning handshake
  SendPlainText(HandshakeMessage(PairingProtocolVersion::CURRENT));
}

void Anki::Switchboard::SecurePairing::SendPublicKey() {
  // Generate public, private key
  uint8_t* publicKey = (uint8_t*)_keyExchange->GenerateKeys();
  
  // Send message and update our state
  SendPlainText(PublicKeyMessage((char*)publicKey));
}

void Anki::Switchboard::SecurePairing::SendNonce() {
  // Send nonce
  const uint8_t NONCE_BYTES = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
  
  // Generate a nonce
  uint8_t* nonce = _keyExchange->GetNonce();
  randombytes_buf(nonce, NONCE_BYTES);
  
  // Give our nonce to the network stream
  _stream->SetNonce(nonce);
  
  // Send nonce (in the clear) to client, update our state
  SendPlainText(NonceMessage((char*)nonce));
}

void Anki::Switchboard::SecurePairing::SendChallenge() {
  // Tell the stream that we can now send over encrypted channel
  _stream->SetEncryptedChannelEstablished(true);
  
  // Create random challenge value
  randombytes_buf(&_pingChallenge, sizeof(uint32_t));
  
  // Send challenge and update state
  SendEncrypted(ChallengeMessage_crypto(_pingChallenge));
}

void Anki::Switchboard::SecurePairing::SendChallengeSuccess() {
  // Send challenge and update state
  SendEncrypted(ChallengeAcceptedMessage_crypto());
}

void Anki::Switchboard::SecurePairing::SendCancelPairing() {
  // Send challenge and update state
  SendPlainText(CancelMessage());
  Log::Write("Canceling pairing.");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Event handling methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

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

void Anki::Switchboard::SecurePairing::HandleMessageReceived(uint8_t* bytes, uint32_t length) {
  _taskExecutor->WakeSync([this, bytes, length]() {
    Log::Write("Handling plain text message received.");
    if(length < kMinMessageSize) {
      return;
    }
    
    // First byte has message type
    Anki::Switchboard::SetupMessage messageType = (Anki::Switchboard::SetupMessage)bytes[0];
    
    switch(messageType) {
      case Anki::Switchboard::SetupMessage::MSG_HANDSHAKE: {
        if(_state == PairingState::Initial ||
           _state == PairingState::AwaitingHandshake) {
          
          HandshakeMessage* handshakeMessage = Message::CastFromBuffer<HandshakeMessage>((char*)bytes);
          bool handleHandshake = HandleHandshake(handshakeMessage->version);
          
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
          Log::Write("Received handshake request in wrong state.");
        }
        
        break;
      }
      case Anki::Switchboard::SetupMessage::MSG_REQUEST_INITIAL_PAIR: {
        if(_state == PairingState::AwaitingPublicKey) {
          PublicKeyMessage* pKeyMsg = Message::CastFromBuffer<PublicKeyMessage>((char*)bytes);
          
          HandleInitialPair((uint8_t*)pKeyMsg->publicKey, crypto_kx_PUBLICKEYBYTES);
          _state = PairingState::AwaitingNonceAck;
        } else {
          // ignore msg
          IncrementAbnormalityCount();
          Log::Write("Received initial pair request in wrong state.");
        }
        break;
      }
      case Anki::Switchboard::SetupMessage::MSG_REQUEST_RENEW: {
        SendNonce();
        Log::Write("Received renew connection request.");
        break;
      }
      case Anki::Switchboard::SetupMessage::MSG_ACK:
      {
        if(bytes[1] == Anki::Switchboard::SetupMessage::MSG_NONCE) {
          if(_state == PairingState::AwaitingNonceAck) {
            HandleNonceAck();
            _state = PairingState::AwaitingChallengeResponse;
          } else {
            // ignore msg
            IncrementAbnormalityCount();
            
            std::ostringstream ss;
            ss << "Received nonce ack in wrong state '" << _state << "'.";
            Log::Write(ss.str().c_str());
          }
        }
        
        break;
      }
      default:
        IncrementAbnormalityCount();
        Log::Write("Unknown plain text message type.");
        break;
    }
  });
}

void Anki::Switchboard::SecurePairing::HandleEncryptedMessageReceived(uint8_t* bytes, uint32_t length) {
  _taskExecutor->WakeSync([this, bytes, length]() {
    Log::Write("Handling encrypted message received.");
    if(length < kMinMessageSize) {
      return;
    }
    
    // first byte has type
    Anki::Switchboard::SecureMessage messageType = (Anki::Switchboard::SecureMessage)bytes[0];
    
    switch(messageType) {
      case Anki::Switchboard::SecureMessage::CRYPTO_CHALLENGE_RESPONSE:
        if(_state == PairingState::AwaitingChallengeResponse) {
          HandleChallengeResponse(bytes+1, length-1);
        } else {
          // ignore msg
          IncrementAbnormalityCount();
          Log::Write("Received challenge response in wrong state.");
        }
        break;
      case Anki::Switchboard::SecureMessage::CRYPTO_WIFI_CREDENTIALS:
        if(_state == PairingState::ConfirmedSharedSecret) {
          char* ssidPtr = (char*)bytes + 2;
          std::string ssid(ssidPtr, bytes[1]);
          std::string pw(ssidPtr + bytes[1] + 1, bytes[2 + bytes[1]]);
          
          _receivedWifiCredentialSignal.emit(ssid, pw);
        } else {
          Log::Write("Received wifi credentials in wrong state.");
        }
      default:
        IncrementAbnormalityCount();
        Log::Write("Unknown encrypted message type.");
        break;
    }
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

std::string Anki::Switchboard::SecurePairing::GeneratePin() {
  // @seichert
  std::random_device rd;
  std::mt19937 gen(rd());
  
  std::string pinStr;
  
  // The first digit cannot be 0, must be between 1 and 9
  std::uniform_int_distribution<> dis('1', '9');
  pinStr.push_back((char) dis(gen));
  
  // Subsequent digits can be between 0 and 9
  dis.param(std::uniform_int_distribution<>::param_type('0','9'));
  for (int i = 1 ; i < kNumPinDigits; i++) {
    pinStr.push_back((char) dis(gen));
  }
  return pinStr;
}


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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Send messages methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<class T>
typename std::enable_if<std::is_base_of<Anki::Switchboard::Message, T>::value, int>::type
Anki::Switchboard::SecurePairing::SendPlainText(const T& message) {
  uint32_t length;
  uint8_t* buffer = (uint8_t*)Anki::Switchboard::Message::CastToBuffer<T>((T*)&message, &length);
  
  return _stream->SendPlainText(buffer, length);
}

template<class T>
typename std::enable_if<std::is_base_of<Anki::Switchboard::Message, T>::value, int>::type
Anki::Switchboard::SecurePairing::SendEncrypted(const T& message) {
  uint32_t length;
  uint8_t* buffer = (uint8_t*)Anki::Switchboard::Message::CastToBuffer<T>((T*)&message, &length);
  
  return _stream->SendEncrypted(buffer, length);
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
