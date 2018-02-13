//
//  BLENetworkStream.m
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#include "BLENetworkStream.h"

int Anki::Switchboard::BLENetworkStream::SendPlainText(uint8_t* bytes, int length) {
  _sendSignal.emit(bytes, length, false);
  return 0;
}

int Anki::Switchboard::BLENetworkStream::SendEncrypted(uint8_t* bytes, int length) {
  uint8_t* bytesWithExtension = (uint8_t*)malloc(length + crypto_aead_xchacha20poly1305_ietf_ABYTES);
  
  uint64_t encryptedLength = 0;
  
  int encryptResult = Encrypt(bytes, length, bytesWithExtension, &encryptedLength);
  
  if(encryptResult != ENCRYPTION_SUCCESS) {
    return NetworkResult::MsgFailure;
  }
  
  _sendSignal.emit(bytesWithExtension, (int)encryptedLength, true);
  free(bytesWithExtension);
  return 0;
}
