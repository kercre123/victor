//
//  BLENetworkStream.m
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#include "BLENetworkStream.h"

int Anki::Switchboard::BLENetworkStream::SendPlainText(uint8_t* bytes, int length) {
  NSData* data = [NSData dataWithBytes:bytes length:length];
  bool couldSend = [_peripheralManager updateValue:data forCharacteristic:_writeCharacteristic onSubscribedCentrals:nil];
  
  return (int)(couldSend? NetworkResult::MsgSuccess : NetworkResult::MsgFailure);
}

int Anki::Switchboard::BLENetworkStream::SendEncrypted(uint8_t* bytes, int length) {
  uint8_t* bytesWithExtension = (uint8_t*)malloc(length + crypto_aead_xchacha20poly1305_ietf_ABYTES);
  
  uint64_t encryptedLength = 0;
  
  int encryptResult = Encrypt(bytes, length, bytesWithExtension, &encryptedLength);
  
  if(encryptResult != ENCRYPTION_SUCCESS) {
    return NetworkResult::MsgFailure;
  }

  NSData* data = [NSData dataWithBytes:bytesWithExtension length:encryptedLength];
  
  free(bytesWithExtension);
  
  bool couldSend = [_peripheralManager updateValue:data forCharacteristic:_encryptedWriteCharacteristic onSubscribedCentrals:nil];
  
  return (int)(couldSend? NetworkResult::MsgSuccess : NetworkResult::MsgFailure);
}
