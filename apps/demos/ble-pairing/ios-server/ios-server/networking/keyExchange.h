/**
 * File: keyExchange.h
 *
 * Author: paluri
 * Created: 1/16/2018
 *
 * Description: Class for interfacing with libsodium for ankiswitchboardd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef KeyExchange_h
#define KeyExchange_h

#include <string>
#include "../sodium/sodium.h"

namespace Anki {
namespace Switchboard {
  class KeyExchange {
  public:
    KeyExchange(uint8_t numPinDigits) :
    _numPinDigits(numPinDigits)
    {
    }
    
    // Getters
    uint8_t* GetEncryptKey() {
      return _encryptKey;
    }
    
    uint8_t* GetDecryptKey() {
      return _decryptKey;
    }
    
    uint8_t* GetPublicKey() {
      return _publicKey;
    }
    
    uint8_t* GetPinLengthPtr() {
      return &_numPinDigits;
    }
    
    uint8_t* GetNonce() {
      return _initialNonce;
    }
    
    uint8_t* GetVerificationHash() {
      crypto_generichash(_hashedKey, crypto_kx_SESSIONKEYBYTES, _encryptKey, crypto_kx_SESSIONKEYBYTES, nullptr, 0);
      
      return _hashedKey;
    }
    
    // Method Declarations
    uint8_t* GenerateKeys();
    void Reset();
    void SetRemotePublicKey(const uint8_t* pubKey);
    bool CalculateSharedKeys(const uint8_t* pin);
    
  private:
    // Variables
    uint8_t _secretKey [crypto_kx_SECRETKEYBYTES];   // our secret key
    uint8_t _decryptKey [crypto_kx_SESSIONKEYBYTES]; // rx
    uint8_t _encryptKey [crypto_kx_SESSIONKEYBYTES]; // tx
    uint8_t _remotePublicKey [crypto_kx_PUBLICKEYBYTES];  // partner's public key
    uint8_t _publicKey [crypto_kx_PUBLICKEYBYTES];   // our public key
    uint8_t _hashedKey [crypto_kx_SESSIONKEYBYTES];  // size of code
    uint8_t _initialNonce [crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
    uint8_t _numPinDigits = 6;
  };
} // Switchboard
} // Anki

#endif /* KeyExchange_h */
