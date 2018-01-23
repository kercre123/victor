//
//  AnkiKeyExchange.h
//
//  Created by Paul Aluri on 1/10/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef AnkiKeyExchange_h
#define AnkiKeyExchange_h

#include "../sodium/sodium.h"
//#include "../networking/INetworkStream.h"

enum AnkiKXServerState {
  Initial,
  AwaitingPartnerReady,
  AwaitingPublicKey,
  AwaitingKeyAck,
  SecureConnectionEstablished
};

enum AnkiKXMessageType : unsigned char {
  Void                = 0,
  ReadyForKeyExchange = 1,
  PublicKey           = 2,
  HashedPrivateKey    = 3,
  SharedKeyAck        = 4,
  Reset               = 5,
  Inquiry            =  6
};

class AnkiKXMessage {
  AnkiKXMessageType MessageType;
  unsigned char BufferSize; // max 255 bytes
  unsigned char* Buffer;
  
public:
  uint16_t getMessageLength() {
    return BufferSize + 2;
  }
  
  void getMessageBytes(unsigned char* outBuffer) {
    outBuffer[0] = (unsigned char)MessageType;
    outBuffer[1] = BufferSize;
    
    // copy Buffer to outBuffer offset by 2 bytes
    memcpy((outBuffer + 2), Buffer, BufferSize);
  }
};

class AnkiKeyExchange {
private:
  unsigned char secretKey [crypto_kx_SECRETKEYBYTES];
  unsigned char decryptKey [crypto_kx_SESSIONKEYBYTES]; // rx
  unsigned char encryptKey [crypto_kx_SESSIONKEYBYTES]; // tx
  unsigned char foreignKey [crypto_kx_PUBLICKEYBYTES];
  unsigned char publicKey [crypto_kx_PUBLICKEYBYTES];
  unsigned char hashedKey [crypto_kx_SESSIONKEYBYTES];
  unsigned char keyMatchAttempts = 0;
  const uint8_t PIN_LENGTH = 6;
  const uint8_t PIN_MAX_ATTEMPTS = 5;
  
public:
  //
  // Method:      generateKeys
  // Description: Generates public and secret key
  // returns:     public key
  //
  unsigned char* generateKeys() {
    crypto_kx_keypair(publicKey, secretKey);
    return publicKey;
  };
  
  void reset() {
    // set all keys to 0
    memset(secretKey, 0, crypto_kx_SECRETKEYBYTES);
    memset(decryptKey, 0, crypto_kx_SESSIONKEYBYTES);
    memset(encryptKey, 0, crypto_kx_SESSIONKEYBYTES);
    memset(foreignKey, 0, crypto_kx_PUBLICKEYBYTES);
    memset(publicKey, 0, crypto_kx_PUBLICKEYBYTES);
    
    // reset key match counter
    keyMatchAttempts = 0;
  }
  
  bool attemptKeyMatch(unsigned char* partnerHashedKey) {
    // Increment keyMatchAttempts, and if we go over our
    // max allowed, then fail and reset the key exchange
    // object. This prevents brute force PIN matching attacks.
    if(++keyMatchAttempts > PIN_MAX_ATTEMPTS) {
      reset();
      return false;
    }
    
    // If parterHashedKey matches our hashedKey, then we both
    // have same keys, and can then move future communication
    // to encrypted channel.
    return memcmp(partnerHashedKey, hashedKey, crypto_kx_SESSIONKEYBYTES);
  }
  
  unsigned char* getPublicKey() {
    // Return public key
    return publicKey;
  }
  
  void setForeignKey(unsigned char* pubKey) {
    // Copy in public key
    memcpy(foreignKey, pubKey, crypto_kx_PUBLICKEYBYTES);
  }
  
  bool calculateSharedKeys(unsigned char* pin) {
    //
    // Messages from the robot will be encrypted with a hash that incorporates
    // a random pin (6 digit)
    // server_tx (encryptKey) needs to be sha-256'ed
    // client_rx (client's decrypt key) needs to be sha-256'ed
    //
    bool success = crypto_kx_server_session_keys(decryptKey, encryptKey, publicKey, secretKey, foreignKey) == 0;
    
    // Save tmp version of encryptKey
    unsigned char tmpEncryptKey[crypto_kx_SESSIONKEYBYTES];
    memcpy(tmpEncryptKey, encryptKey, crypto_kx_SESSIONKEYBYTES);
    
    // Hash mix of pin and encryptKey to form new encryptKey
    //
    // todo: USE PIN to encrypt both directions? does it really matter?
    crypto_generichash(encryptKey, crypto_kx_SESSIONKEYBYTES, tmpEncryptKey, crypto_kx_SESSIONKEYBYTES, pin, PIN_LENGTH);
    
    return success;
  }
  
  AnkiKXServerState State = AnkiKXServerState::Initial;
};

#endif /* AnkiKeyExchange_h */
