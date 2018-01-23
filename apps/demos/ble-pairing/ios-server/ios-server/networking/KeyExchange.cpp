//
//  KeyExchange.cpp
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#include "KeyExchange.h"

uint8_t* Anki::Networking::KeyExchange::GenerateKeys() {
  crypto_kx_keypair(_PublicKey, _SecretKey);
  return _PublicKey;
};

void Anki::Networking::KeyExchange::Reset() {
  // set all keys to 0
  memset(_SecretKey, 0, crypto_kx_SECRETKEYBYTES);
  memset(_DecryptKey, 0, crypto_kx_SESSIONKEYBYTES);
  memset(_EncryptKey, 0, crypto_kx_SESSIONKEYBYTES);
  memset(_RemotePublicKey, 0, crypto_kx_PUBLICKEYBYTES);
  memset(_PublicKey, 0, crypto_kx_PUBLICKEYBYTES);
}

void Anki::Networking::KeyExchange::SetRemotePublicKey(uint8_t* pubKey) {
  // Copy in public key
  memcpy(_RemotePublicKey, pubKey, crypto_kx_PUBLICKEYBYTES);
}

bool Anki::Networking::KeyExchange::CalculateSharedKeys(uint8_t* pin) {
  //
  // Messages from the robot will be encrypted with a hash that incorporates
  // a random pin
  // server_tx (encryptKey) needs to be sha-256'ed
  // client_rx (client's decrypt key) needs to be sha-256'ed
  //
  bool success = crypto_kx_server_session_keys(_DecryptKey, _EncryptKey, _PublicKey, _SecretKey, _RemotePublicKey) == 0;
  
  // Save tmp version of encryptKey
  uint8_t tmpEncryptKey[crypto_kx_SESSIONKEYBYTES];
  memcpy(tmpEncryptKey, _EncryptKey, crypto_kx_SESSIONKEYBYTES);
  
  // Hash mix of pin and encryptKey to form new encryptKey
  crypto_generichash(_EncryptKey, crypto_kx_SESSIONKEYBYTES, tmpEncryptKey, crypto_kx_SESSIONKEYBYTES, pin, _NumPinDigits);
  
  return success;
}
