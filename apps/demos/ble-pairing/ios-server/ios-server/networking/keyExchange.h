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
      KeyExchange(uint8_t numPinDigits) :
      _NumPinDigits(numPinDigits)
      {
      }
      
      uint8_t* GetEncryptKey() {
        return _EncryptKey;
      }
      
      uint8_t* GetDecryptKey() {
        return _DecryptKey;
      }
      
      uint8_t* GetPublicKey() {
        return _PublicKey;
      }
      
      uint8_t* GetPinLengthPtr() {
        return &_NumPinDigits;
      }
      
      uint8_t* GetNonce() {
        // Return nonce
        return _InitialNonce;
      }
      
      uint8_t* GetVerificationHash() {
        crypto_generichash(_HashedKey, crypto_kx_SESSIONKEYBYTES, _EncryptKey, crypto_kx_SESSIONKEYBYTES, nullptr, 0);
        
        return _HashedKey;
      }
      
      uint8_t* GenerateKeys();
      void Reset();
      void SetRemotePublicKey(const uint8_t* pubKey);
      bool CalculateSharedKeys(const uint8_t* pin);
      
    private:
      uint8_t _SecretKey [crypto_kx_SECRETKEYBYTES];   // our secret key
      uint8_t _DecryptKey [crypto_kx_SESSIONKEYBYTES]; // rx
      uint8_t _EncryptKey [crypto_kx_SESSIONKEYBYTES]; // tx
      uint8_t _RemotePublicKey [crypto_kx_PUBLICKEYBYTES];  // partner's public key
      uint8_t _PublicKey [crypto_kx_PUBLICKEYBYTES];   // our public key
      uint8_t _HashedKey [crypto_kx_SESSIONKEYBYTES];  // size of code
      uint8_t _InitialNonce [crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
      uint8_t _NumPinDigits = 6;
    };
  }
}

#endif /* KeyExchange_h */
