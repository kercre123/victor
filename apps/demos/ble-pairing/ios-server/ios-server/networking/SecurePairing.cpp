//
//  SecurePairing.m
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#include <random>
#include "SecurePairing.h"
#include "SecurePairingMessages.h"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Constructors
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Anki::Networking::SecurePairing::SecurePairing(INetworkStream* stream) {
  _Stream = stream;
  _TotalPairingAttempts = 0;
  
  // Register with stream events
  _Stream->OnReceivedPlainTextEvent().SubscribeForever(
    std::bind(&SecurePairing::HandleMessageReceived,
              this, std::placeholders::_1, std::placeholders::_2));
  
  _Stream->OnReceivedEncryptedEvent().SubscribeForever(
    std::bind(&SecurePairing::HandleEncryptedMessageReceived,
              this, std::placeholders::_1, std::placeholders::_2));
  
  _Stream->OnFailedDecryptionEvent().SubscribeForever(
    std::bind(&SecurePairing::HandleDecryptionFailed, this));
  
  // Register with private events
  _PairingTimeoutSignal.SubscribeForever(std::bind(&SecurePairing::HandleTimeout, this));
  
  // Initialize the key exchange object
  _KeyExchange = new KeyExchange(NUM_PIN_DIGITS);
  
  Log::Write("SecurePairing starting up.");
}

Anki::Networking::SecurePairing::~SecurePairing() {
  delete _KeyExchange;
  
  Log::Write("Destroying SecurePairing object.");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Instance methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Anki::Networking::SecurePairing::BeginPairing() {
  Log::Write("Beginning secure pairing process.");
  
  // Initialize object
  Init();
  
  _HandleTimeoutTimer.signal = &_PairingTimeoutSignal;
  
  ev_timer_init(&_HandleTimeoutTimer.timer, &SecurePairing::sEvTimerHandler, PAIRING_TIMEOUT_SECONDS, PAIRING_TIMEOUT_SECONDS);
  _Loop = ev_default_loop(0);
  ev_timer_start(_Loop, &_HandleTimeoutTimer.timer);
  ev_loop(_Loop, 0);
}

void Anki::Networking::SecurePairing::HandleTimeout() {
  if(_State != PairingState::ConfirmedSharedSecret) {
    Log::Write("Pairing timeout. Client took too long.");
    Reset();
  }
}

void Anki::Networking::SecurePairing::SendPublicKey() {
  const uint8_t KEY_BYTES = crypto_kx_PUBLICKEYBYTES;
  
  // Generate public, private key
  uint8_t* publicKey = (uint8_t*)_KeyExchange->GenerateKeys();
  
  // Initialize buffer for public key, set type byte and fill buffer
  uint8_t* keyBuffer = (uint8_t*)malloc(KEY_BYTES + 1);
  keyBuffer[0] = SetupMessage::MSG_PUBLIC_KEY;
  memcpy(keyBuffer+1, publicKey, KEY_BYTES);
  
  // Send message and update our state
  _Stream->SendPlainText(keyBuffer, KEY_BYTES + 1);
  _State = PairingState::AwaitingPublicKey;
  
  // Release the key buffer
  free(keyBuffer);
}

void Anki::Networking::SecurePairing::Init() {
  // Clear field values
  _ChallengeAttempts = 0;
  _AbnormalityCount = 0;
  
  // Generate a random number with NUM_PIN_DIGITS digits
  _Pin = GeneratePin();
  _UpdatedPinSignal.emit(_Pin);
  
  // Update our state
  _State = PairingState::Initial;
  
  // Send public key
  Log::Write("Sending public key to client.");
  SendPublicKey();
}

std::string Anki::Networking::SecurePairing::GeneratePin() {
  // @seichert
  std::random_device rd;
  std::mt19937 gen(rd());
  
  std::string pinStr;
  
  // The first digit cannot be 0, must be between 1 and 9
  std::uniform_int_distribution<> dis('1', '9');
  pinStr.push_back((char) dis(gen));
  
  // Subsequent digits can be between 0 and 9
  dis.param(std::uniform_int_distribution<>::param_type('0','9'));
  for (int i = 1 ; i < NUM_PIN_DIGITS; i++) {
    pinStr.push_back((char) dis(gen));
  }
  return pinStr;
}

void Anki::Networking::SecurePairing::Reset() {
  // Tell key exchange to reset
  _KeyExchange->Reset();
  
  // Clear pin
  _Pin = "000000";
  
  // Send cancel message
  SendCancelPairing();
  
  // Put us back in initial state
  if(++_TotalPairingAttempts < MAX_PAIRING_ATTEMPTS) {
    Init();
    Log::Write("SecurePairing restarting.");
  } else {
    Log::Write("SecurePairing ending due to multiple failures. Requires external restart.");
    ev_timer_stop(_Loop, &_HandleTimeoutTimer.timer);
    ev_unloop(_Loop, EVBREAK_ALL);
  }
}

void Anki::Networking::SecurePairing::HandleInitialPair(uint8_t* publicKey, uint32_t publicKeyLength) {
  // Handle initial pair request from client
  
  // Input client's public key and calculate shared keys
  _KeyExchange->SetRemotePublicKey(publicKey);
  _KeyExchange->CalculateSharedKeys((unsigned char*)_Pin.c_str());
  
  // Give our shared keys to the network stream
  _Stream->SetCryptoKeys(
    _KeyExchange->GetEncryptKey(),
    _KeyExchange->GetDecryptKey());
  
  // Send nonce
  SendNonce();
  
  Log::Write("Received initial pair request, sending nonce.");
}

void Anki::Networking::SecurePairing::SendNonce() {
  // Send nonce
  const uint8_t NONCE_BYTES = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
  
  // Generate a nonce
  randombytes_buf(_KeyExchange->GetNonce(), NONCE_BYTES);
  
  // Copy nonce bytes into message buffer
  uint8_t* nonceBuffer = (uint8_t*)malloc(NONCE_BYTES + 1);
  nonceBuffer[0] = SetupMessage::MSG_NONCE;
  memcpy(nonceBuffer+1, _KeyExchange->GetNonce(), NONCE_BYTES);
  
  // Give our nonce to the network stream
  _Stream->SetNonce(_KeyExchange->GetNonce());
  
  // Send nonce (in the clear) to client, update our state
  _Stream->SendPlainText(nonceBuffer, NONCE_BYTES + 1);
  _State = PairingState::AwaitingNonceAck;
  
  // Free message buffer
  free(nonceBuffer);
}

void Anki::Networking::SecurePairing::SendChallenge() {
  // Tell the stream that we can now send over encrypted channel
  _Stream->SetEncryptedChannelEstablished(true);
  
  // Create random challenge value
  randombytes_buf(&_PingChallenge, sizeof(uint32_t));
  
  // Send challenge to client
  uint8_t* challengeBuffer = (uint8_t*)malloc(sizeof(uint32_t) + 1);
  challengeBuffer[0] = SecureMessage::CRYPTO_PING;
  memcpy(challengeBuffer+1, &_PingChallenge, sizeof(uint32_t));
  
  // Send challenge and update state
  _Stream->SendEncrypted(challengeBuffer, sizeof(uint32_t) + 1);
  _State = PairingState::AwaitingChallengeResponse;
  
  // Free message buffer
  free(challengeBuffer);
}

void Anki::Networking::SecurePairing::SendChallengeSuccess() {
  // Send challenge success to client
  uint16_t buffer = 0;
  uint8_t* challengeBuffer = (uint8_t*)&buffer;
  challengeBuffer[0] = SecureMessage::CRYPTO_ACCEPTED;
  
  // Send challenge and update state
  _Stream->SendEncrypted(challengeBuffer, sizeof(uint16_t));
  _State = PairingState::ConfirmedSharedSecret;
}

void Anki::Networking::SecurePairing::SendCancelPairing() {
  // Send challenge success to client
  uint16_t buffer = 0;
  uint8_t* cancelBuffer = (uint8_t*)&buffer;
  cancelBuffer[0] = SetupMessage::MSG_CANCEL_SETUP;
  
  // Send challenge and update state
  _Stream->SendPlainText(cancelBuffer, sizeof(uint16_t));
  
  Log::Write("Canceling pairing.");
}

void Anki::Networking::SecurePairing::HandleRestoreConnection() {
  // todo: implement
  Reset();
}

void Anki::Networking::SecurePairing::HandleDecryptionFailed() {
  // todo: implement
  Reset();
}

void Anki::Networking::SecurePairing::HandleNonceAck() {
  // Send challenge to user
  SendChallenge();
  
  Log::Write("Client acked nonce, sending challenge.");
}

inline bool isChallengeSuccess(uint32_t challenge, uint32_t answer) {
  return answer == challenge + 1;
}

void Anki::Networking::SecurePairing::HandlePingResponse(uint8_t* pingChallengeAnswer, uint32_t length) {
  bool success = false;
  
  if(length < sizeof(uint32_t)) {
    success = false;
  } else {
    uint32_t answer = *((uint32_t*)pingChallengeAnswer);
    success = isChallengeSuccess(_PingChallenge, answer);
  }
  
  if(success) {
    // Inform client that we are good to go and
    // update our state
    SendChallengeSuccess();
    Log::Write("Challenge answer was accepted. Encrypted channel established.");
  } else {
    // Increment our abnormality and attack counter, and
    // if at or above max attempts reset.
    IncrementAbnormalityCount();
    IncrementChallengeCount();
    Log::Write("Received faulty challenge response.");
  }
}

void Anki::Networking::SecurePairing::HandleMessageReceived(uint8_t* bytes, uint32_t length) {
  if(length < MIN_MSG_LENGTH) {
    return;
  }
  
  // First byte has message type
  Anki::Networking::SetupMessage messageType = (Anki::Networking::SetupMessage)bytes[0];
  
  switch(messageType) {
    case Anki::Networking::SetupMessage::MSG_REQUEST_INITIAL_PAIR: {
      if(_State == PairingState::Initial ||
         _State == PairingState::AwaitingPublicKey) {
        HandleInitialPair(bytes+1, length-1);
      } else {
        // ignore msg
        IncrementAbnormalityCount();
        Log::Write("Received initial pair request in wrong state.");
      }
      break;
    }
    case Anki::Networking::SetupMessage::MSG_REQUEST_RENEW: {
      SendNonce();
      Log::Write("Received renew connection request.");
      break;
    }
    case Anki::Networking::SetupMessage::MSG_ACK:
    {
      if(bytes[1] == Anki::Networking::SetupMessage::MSG_NONCE) {
        if(_State == PairingState::AwaitingNonceAck) {
          HandleNonceAck();
        } else {
          // ignore msg
          IncrementAbnormalityCount();
          Log::Write("Received nonce ack in wrong state.");
        }
      }
      
      break;
    }
    default:
      IncrementAbnormalityCount();
      Log::Write("Unknown plain text message type.");
      break;
  }
}

void Anki::Networking::SecurePairing::HandleEncryptedMessageReceived(uint8_t* bytes, uint32_t length) {
  if(length < MIN_MSG_LENGTH) {
    return;
  }
  
  // first byte has type
  Anki::Networking::SecureMessage messageType = (Anki::Networking::SecureMessage)bytes[0];
  
  switch(messageType) {
    case Anki::Networking::SecureMessage::CRYPTO_PING:
      if(_State == PairingState::AwaitingChallengeResponse) {
        HandlePingResponse(bytes+1, length-1);
      } else {
        // ignore msg
        IncrementAbnormalityCount();
        Log::Write("Received challenge response in wrong state.");
      }
      break;
    case Anki::Networking::SecureMessage::CRYPTO_WIFI_CREDENTIALS:
      if(_State == PairingState::ConfirmedSharedSecret) {
        char* ssidPtr = (char*)bytes + 2;
        std::string ssid(ssidPtr, bytes[1]);
        std::string pw(ssidPtr + bytes[1] + 1, bytes[2 + bytes[1]]);
        
        _ReceivedWifiCredentialSignal.emit(ssid, pw);
      } else {
        Log::Write("Received wifi credentials in wrong state.");
      }
    default:
      IncrementAbnormalityCount();
      Log::Write("Unknown encrypted message type.");
      break;
  }
}

void Anki::Networking::SecurePairing::IncrementChallengeCount() {
  // Increment challenge count
  _ChallengeAttempts++;
  
  if(_ChallengeAttempts >= MAX_MATCH_ATTEMPTS) {
    Reset();
  }
  
  Log::Write("Client answered challenge.");
}

void Anki::Networking::SecurePairing::IncrementAbnormalityCount() {
  // Increment abnormality count
  _AbnormalityCount++;
  
  if(_AbnormalityCount >= MAX_ABNORMALITY_COUNT) {
    Reset();
  }
  
  Log::Write("Abnormality recorded.");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Static methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Anki::Networking::SecurePairing::sEvTimerHandler(struct ev_loop* loop, struct ev_timer* w, int revents)
{
  struct ev_TimerStruct *wData = (struct ev_TimerStruct*)w;
  wData->signal->emit();
}
