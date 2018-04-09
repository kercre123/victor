/**
 * File: keyExchange.cpp
 *
 * Author: paluri
 * Created: 1/16/2018
 *
 * Description: Class for interfacing with libsodium for ankiswitchboardd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "keyExchange.h"

uint8_t* Anki::Switchboard::KeyExchange::GenerateKeys() {
  crypto_kx_keypair(_publicKey, _secretKey);
  return _publicKey;
};

void Anki::Switchboard::KeyExchange::Reset() {
  // set all keys to 0
  memset(_secretKey, 0, crypto_kx_SECRETKEYBYTES);
  memset(_decryptKey, 0, crypto_kx_SESSIONKEYBYTES);
  memset(_encryptKey, 0, crypto_kx_SESSIONKEYBYTES);
  memset(_remotePublicKey, 0, crypto_kx_PUBLICKEYBYTES);
  memset(_publicKey, 0, crypto_kx_PUBLICKEYBYTES);
}

void Anki::Switchboard::KeyExchange::SetRemotePublicKey(const uint8_t* pubKey) {
  // Copy in public key
  memcpy(_remotePublicKey, pubKey, crypto_kx_PUBLICKEYBYTES);
}

bool Anki::Switchboard::KeyExchange::CalculateSharedKeys(const uint8_t* pin) {
  //
  // Messages from the robot will be encrypted with a hash that incorporates
  // a random pin
  // server_tx (encryptKey) needs to be sha-256'ed
  // client_rx (client's decrypt key) needs to be sha-256'ed
  //
  bool success = crypto_kx_server_session_keys(_decryptKey, _encryptKey, _publicKey, _secretKey, _remotePublicKey) == 0;
  
  // Save tmp version of encryptKey
  uint8_t tmpEncryptKey[crypto_kx_SESSIONKEYBYTES];
  memcpy(tmpEncryptKey, _encryptKey, crypto_kx_SESSIONKEYBYTES);
  
  // Hash mix of pin and encryptKey to form new encryptKey
  crypto_generichash(_encryptKey, crypto_kx_SESSIONKEYBYTES, tmpEncryptKey, crypto_kx_SESSIONKEYBYTES, pin, _numPinDigits);
  
  return success;
}
