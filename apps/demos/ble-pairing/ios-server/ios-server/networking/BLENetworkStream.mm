//
//  BLENetworkStream.m
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#include "BLENetworkStream.h"

const uint8_t Anki::Switchboard::BLENetworkStream::kMaxPacketSize;

void Anki::Switchboard::BLENetworkStream::HandleSendRawPlainText(uint8_t* buffer, size_t size){
  _sendSignal.emit(buffer, (int)size, false);
}

void Anki::Switchboard::BLENetworkStream::HandleSendRawEncrypted(uint8_t* buffer, size_t size){
  _sendSignal.emit(buffer, (int)size, true);
}

void Anki::Switchboard::BLENetworkStream::HandleReceivePlainText(uint8_t* buffer, size_t size) {
  INetworkStream::ReceivePlainText(buffer, (int)size);
}

void Anki::Switchboard::BLENetworkStream::HandleReceiveEncrypted(uint8_t* buffer, size_t size) {
  INetworkStream::ReceiveEncrypted(buffer, (int)size);
}

int Anki::Switchboard::BLENetworkStream::SendPlainText(uint8_t* bytes, int length) {
  _bleMessageProtocolPlainText->SendMessage(bytes, (size_t)length);
  return 0;
}

int Anki::Switchboard::BLENetworkStream::SendEncrypted(uint8_t* bytes, int length) {
  uint8_t* bytesWithExtension = (uint8_t*)malloc(length + crypto_aead_xchacha20poly1305_ietf_ABYTES);
  
  uint64_t encryptedLength = 0;
  
  int encryptResult = Encrypt(bytes, length, bytesWithExtension, &encryptedLength);
  
  if(encryptResult != ENCRYPTION_SUCCESS) {
    return NetworkResult::MsgFailure;
  }
  
  _bleMessageProtocolEncrypted->SendMessage(bytesWithExtension, (size_t)encryptedLength);
  
  free(bytesWithExtension);
  return 0;
}

void Anki::Switchboard::BLENetworkStream::ReceivePlainText(uint8_t* bytes, int length) {
  _bleMessageProtocolPlainText->ReceiveRawBuffer(bytes, (size_t)length);
}

void Anki::Switchboard::BLENetworkStream::ReceiveEncrypted(uint8_t* bytes, int length) {
  _bleMessageProtocolEncrypted->ReceiveRawBuffer(bytes, (size_t)length);
}
