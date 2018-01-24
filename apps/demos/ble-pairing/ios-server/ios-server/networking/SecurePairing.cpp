//
//  SecurePairing.m
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#include "SecurePairing.h"

Anki::Networking::SecurePairing::SecurePairing(INetworkStream* stream) {
  _Stream = stream;
  _Stream->OnReceivedEvent().SubscribeForever(std::bind(&SecurePairing::HandleMessageReceive, this, std::placeholders::_1, std::placeholders::_2));
  _KeyExchange = new KeyExchange(NUM_PIN_DIGITS);
  
  Init();
  
  // Send public key
  SharePublicKey();
}

void Anki::Networking::SecurePairing::SharePublicKey() {
  _State = PairingState::AwaitingPublicKey;
  uint8_t* publicKey = (uint8_t*)_KeyExchange->GenerateKeys();
  
  PairingMessage msg;
  msg.bufferSize = 32;
  msg.type = PairingMessageType::PublicKey;
  msg.buffer = publicKey;
  
  _Stream->SendPlainText(msg.GetBuffer(), msg.GetSize());
}

void Anki::Networking::SecurePairing::Init() {
  _State = PairingState::Initial;

  uint32_t mult = pow(10, NUM_PIN_DIGITS-1);
  uint32_t pinNum = arc4random() % (9 * mult) + (1 * mult);
  std::string pinString = std::to_string(pinNum).c_str();
  std::strcpy((char*)_Pin, pinString.c_str());
}

void Anki::Networking::SecurePairing::Reset() {
  memset(_Pin, 0, NUM_PIN_DIGITS);
  
  _KeyExchange->Reset();
  _KeyMatchAttempts = 0;

  Init();
}

void Anki::Networking::SecurePairing::HandleMessageReceive(uint8_t* bytes, uint32_t length) {
  switch(bytes[4]) {
    case Anki::Networking::PairingMessageType::PublicKey:
      _KeyExchange->SetRemotePublicKey(bytes + 5);
      _KeyExchange->CalculateSharedKeys(_Pin);
      
      _Stream->SetCryptoKeys(_KeyExchange->GetEncryptKey(), _KeyExchange->GetDecryptKey());
      _State = PairingState::AwaitingKeyMatchAck;
      
      break;
    case Anki::Networking::PairingMessageType::HashedKey:
    {
      bool isEqual = memcmp(((unsigned char*)(bytes+5)), _KeyExchange->GetVerificationHash(), 32) == 0;
      
      // if not, increment some attack counter
      if(isEqual) {
        PairingMessage msg;
        msg.bufferSize = 32;
        msg.type = PairingMessageType::HashedKey;
        msg.buffer = (uint8_t*)_KeyExchange->GetVerificationHash();
        _Stream->SendPlainText(msg.GetBuffer(), msg.GetSize());
      } else {
        // Increment our attack counter, and if at or above max attempts
        // reset.
        if(++_KeyMatchAttempts >= MAX_MATCH_ATTEMPTS) {
          Reset();
        }
      }
      
      break;
    }
    default:
      int size = *((int*)bytes);
      
      printf("Size: %d Encrypted message: %s ", size, bytes+5);
      break;
  }
}
