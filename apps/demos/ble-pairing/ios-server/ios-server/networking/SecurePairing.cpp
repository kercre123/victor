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
  char* publicKey = (char*)_KeyExchange->GenerateKeys();
  
  PairingMessage msg;
  msg.bufferSize = 32;
  msg.type = PairingMessageType::PublicKey;
  msg.buffer = publicKey;
  
  _Stream->Send(msg.GetBuffer(), msg.GetSize());
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

  Init();
}

void Anki::Networking::SecurePairing::HandleMessageReceive(char* bytes, int length) {
  switch(bytes[4]) {
    case Anki::Networking::PairingMessageType::PublicKey:
      _KeyExchange->SetForeignKey((unsigned char*)(bytes + 5));
      _KeyExchange->CalculateSharedKeys(_Pin);
      
      printf("\nEncryptKey:\n");
      for(int i = 0; i < 32; i++) {
        printf("%x ", _KeyExchange->GetEncryptKey()[i]);
      }
      printf("\n");
      printf("\n\nDecryptKey:\n");
      for(int i = 0; i < 32; i++) {
        printf("%x ", _KeyExchange->GetDecryptKey()[i]);
      }
      printf("\n");
      
      _Stream->SetCryptoKeys((char*)_KeyExchange->GetEncryptKey(), (char*)_KeyExchange->GetDecryptKey());
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
        msg.buffer = (char*)_KeyExchange->GetVerificationHash();
        _Stream->Send(msg.GetBuffer(), msg.GetSize());
      } else {
        printf("Sadness!\n");
      }
      
      break;
    }
    default:
      int size = *((int*)bytes);
      
      printf("Size: %d Encrypted message: %s ", size, bytes+5);
      break;
  }
}
