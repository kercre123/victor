//
//  BLENetworkStream.m
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#include "BLENetworkStream.h"

void Anki::Networking::BLENetworkStream::Send(uint8_t* bytes, int length) {
  NSData* data = [NSData dataWithBytes:bytes length:length];
  bool couldSend = [_peripheralManager updateValue:data forCharacteristic:_writeCharacteristic onSubscribedCentrals:nil];
}

void Anki::Networking::BLENetworkStream::SendEncrypted(uint8_t* bytes, int length) {
  NSData* data = [NSData dataWithBytes:bytes length:length];
  bool couldSend = [_peripheralManager updateValue:data forCharacteristic:_encryptedWriteCharacteristic onSubscribedCentrals:nil];
}
