//
//  SecurePairing.m
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#include "SecurePairing.h"
#include "SecurePairingMessages.h"

Anki::Networking::SecurePairing::SecurePairing(INetworkStream* stream) {  
  _Stream = stream;
  
  _Stream->OnReceivedPlainTextEvent().SubscribeForever(
    std::bind(&SecurePairing::HandleMessageReceived,
              this, std::placeholders::_1, std::placeholders::_2));
  
  _Stream->OnReceivedEncryptedEvent().SubscribeForever(
    std::bind(&SecurePairing::HandleEncryptedMessageReceived,
              this, std::placeholders::_1, std::placeholders::_2));
  
  _Stream->OnFailedDecryptionEvent().SubscribeForever(
    std::bind(&SecurePairing::HandleDecryptionFailed, this));
  
  // Initialize the key exchange object
  _KeyExchange = new KeyExchange(NUM_PIN_DIGITS);
  
  // Initialize object
  Init();
}

void Anki::Networking::SecurePairing::BeginPairing() {
  // Send public key
  printf("Sending public key...\n");
  SendPublicKey();
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
  //
  // if pin is 6 digits, mult is a number like '100000'
  // to form the pin, mult is multiplied by 9 and then
  // added to (mult-1). (100000 * 9) + 99999 = 999999
  //
  const uint8_t NUM_BASE = 10;
  uint32_t mult = pow(NUM_BASE, NUM_PIN_DIGITS-1);
  uint32_t pinNum = arc4random() % ((NUM_BASE-1) * mult) + (1 * mult);
  
  // Convert pin to string
  std::string pinString = std::to_string(pinNum).c_str();
  std::strcpy((char*)_Pin, pinString.c_str());
  
  // Update our state
  _State = PairingState::Initial;
}

void Anki::Networking::SecurePairing::Reset() {
  // Clear pin
  memset(_Pin, 0, NUM_PIN_DIGITS);
  
  // Tell key exchange to reset
  _KeyExchange->Reset();
  
  // Send cancel message
  SendCancelPairing();
  
  // Put us back in initial state
  Init();
}

void Anki::Networking::SecurePairing::HandleInitialPair(uint8_t* publicKey, uint32_t publicKeyLength) {
  // Handle initial pair request from client
  
  // Input client's public key and calculate shared keys
  _KeyExchange->SetRemotePublicKey(publicKey);
  _KeyExchange->CalculateSharedKeys(_Pin);
  
  // Give our shared keys to the network stream
  _Stream->SetCryptoKeys(
    _KeyExchange->GetEncryptKey(),
    _KeyExchange->GetDecryptKey());
  
  // Send nonce
  printf("Receiving public key/ Sending nonce...\n");
  SendNonce();
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
}

void Anki::Networking::SecurePairing::HandleRestoreConnection() {
  // todo: implement
}

void Anki::Networking::SecurePairing::HandleDecryptionFailed() {
  // todo: implement
}

void Anki::Networking::SecurePairing::HandleNonceAck() {
  // Send challenge to user
  SendChallenge();
}

void Anki::Networking::SecurePairing::HandlePingResponse(uint8_t* pingChallengeAnswer, uint32_t length) {
  bool success = false;
  
  if(length < sizeof(uint32_t)) {
    success = false;
  } else {
    uint32_t answer = *((uint32_t*)pingChallengeAnswer);
    success = (answer == _PingChallenge + 1);
  }
  
  if(success) {
    // Inform client that we are good to go and
    // update our state
    SendChallengeSuccess();
  } else {
    // Increment our abnormality and attack counter, and
    // if at or above max attempts reset.
    IncrementAbnormalityCount();
    IncrementChallengeCount();
  }
}

void Anki::Networking::SecurePairing::HandleMessageReceived(uint8_t* bytes, uint32_t length) {
  if(length < 2) {
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
      }
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
        }
      }
      
      break;
    }
    default:
      IncrementAbnormalityCount();
      break;
  }
}

void Anki::Networking::SecurePairing::HandleEncryptedMessageReceived(uint8_t* bytes, uint32_t length) {
  // first byte has type
  Anki::Networking::SecureMessage messageType = (Anki::Networking::SecureMessage)bytes[0];
  
  switch(messageType) {
    case Anki::Networking::SecureMessage::CRYPTO_PING:
      if(_State == PairingState::AwaitingChallengeResponse) {
        HandlePingResponse(bytes+1, length-1);
      } else {
        // ignore msg
        IncrementAbnormalityCount();
      }
      break;
    case Anki::Networking::SecureMessage::CRYPTO_WIFI_CRED:
      if(_State == PairingState::ConfirmedSharedSecret) {
        char* ssidPtr = (char*)bytes + 2;
        std::string ssid(ssidPtr, bytes[1]);
        std::string pw(ssidPtr + bytes[1] + 1, bytes[2 + bytes[1]]);

        _ReceivedWifiCredentialSignal.emit(ssid, pw);
      }
    default:
      IncrementAbnormalityCount();
      break;
  }
}

void Anki::Networking::SecurePairing::IncrementChallengeCount() {
  // Increment challenge count
  _ChallengeAttempts++;
  
  if(_ChallengeAttempts >= MAX_MATCH_ATTEMPTS) {
    Reset();
  }
}

void Anki::Networking::SecurePairing::IncrementAbnormalityCount() {
  // Increment abnormality count
  _AbnormalityCount++;
  
  if(_AbnormalityCount >= MAX_ABNORMALITY_COUNT) {
    Reset();
  }
}
