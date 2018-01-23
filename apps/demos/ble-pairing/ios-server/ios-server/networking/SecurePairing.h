//
//  SecurePairing.h
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef SecurePairing_h
#define SecurePairing_h

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "INetworkStream.h"
#include "KeyExchange.h"

namespace Anki {
  namespace Networking {
    #define NUM_PIN_DIGITS 6
    
    enum PairingState {
      Initial,
      AwaitingPartnerReady,
      AwaitingPublicKey,
      AwaitingKeyMatchAck,
      ConfirmedSharedSecret
    };
    
    enum PairingMessageType : char {
      Nop = 0,
      Ack = 1,
      PublicKey = 2,
      HashedKey = 3,
      Verified = 4,
    };
    
    struct PairingMessage {
      uint32_t bufferSize;
      PairingMessageType type;
      char* buffer;
      
    public:
      char* GetBuffer() {
        char* bufferCpy = (char*)malloc(sizeof(uint32_t) + sizeof(PairingMessageType) + bufferSize);
        
        memcpy(bufferCpy, &bufferSize, sizeof(uint32_t));
        memcpy(bufferCpy + sizeof(uint32_t), &type, sizeof(PairingMessageType));
        memcpy(bufferCpy + sizeof(uint32_t) + sizeof(PairingMessageType), buffer, bufferSize);
        
        return bufferCpy;
      }
      
      int GetSize() {
        return sizeof(uint32_t) + sizeof(PairingMessageType) + bufferSize;
      }
    };
    
    class SecurePairing {
    public:
      SecurePairing(INetworkStream* stream);
      
      void SharePublicKey();
      
      unsigned char* GetPin() { return _Pin; }
      
    private:
      unsigned char _Pin[NUM_PIN_DIGITS];
      
      void Init();
      void Reset();
      void HandleMessageReceive(char* bytes, int length);
      
      uint8_t _NumPinDigits;
      INetworkStream* _Stream;
      KeyExchange* _KeyExchange;
      PairingState _State = PairingState::Initial;
    };
  }
}

#endif /* SecurePairing_h */
