//
//  KeyExchange.h
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef KeyExchange_h
#define KeyExchange_h

#include <string>
#include "../sodium/sodium.h"

namespace Anki {
  namespace Networking {
    class KeyExchange {
    public:
      KeyExchange(uint8_t numPinDigits) {
        _NumPinDigits = numPinDigits;
      }
      
      void Reset();
      unsigned char* GenerateKeys();
      bool AttemptKeyMatch(unsigned char* partnerHashedKey);
      void SetForeignKey(unsigned char* pubKey);
      bool CalculateSharedKeys(unsigned char* pin);
      
      unsigned char* GetEncryptKey() {
        return _EncryptKey;
      }
      
      unsigned char* GetDecryptKey() {
        return _DecryptKey;
      }
      
      unsigned char* GetPublicKey() {
        // Return public key
        return _PublicKey;
      }
      
      unsigned char* GetVerificationHash() {
        crypto_generichash(_HashedKey, crypto_kx_SESSIONKEYBYTES, _EncryptKey, crypto_kx_SESSIONKEYBYTES, nullptr, 0);
        
        return _HashedKey;
      }
      
    private:
      unsigned char _SecretKey [crypto_kx_SECRETKEYBYTES];
      unsigned char _DecryptKey [crypto_kx_SESSIONKEYBYTES]; // rx
      unsigned char _EncryptKey [crypto_kx_SESSIONKEYBYTES]; // tx
      unsigned char _ForeignKey [crypto_kx_PUBLICKEYBYTES];
      unsigned char _PublicKey [crypto_kx_PUBLICKEYBYTES];
      unsigned char _HashedKey [crypto_kx_SESSIONKEYBYTES];
      unsigned char _KeyMatchAttempts = 0;
      const unsigned char MAX_MATCH_ATTEMPTS = 5;
      uint8_t _NumPinDigits = 6;
    };
  }
}

#endif /* KeyExchange_h */
