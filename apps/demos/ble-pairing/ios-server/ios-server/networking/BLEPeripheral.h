//
//  BLECentral.h
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef BLECentral_h
#define BLECentral_h

#import <Foundation/Foundation.h>
#include <queue>
#include "BLENetworkStream.h"
#include <CoreBluetooth/CoreBluetooth.h>

struct SendContainer {
  NSData* data;
  CBMutableCharacteristic* characteristic;
};

@interface BLEPeripheral : NSObject <CBPeripheralManagerDelegate> {
  CBUUID* CHAR_WRITE;
  CBUUID* CHAR_READ;
  CBUUID* CHAR_SECURE_WRITE;
  CBUUID* CHAR_SECURE_READ;
  CBUUID* CHAR_PING;
  
  CBMutableCharacteristic* CH_WRITE;
  CBMutableCharacteristic* CH_READ;
  CBMutableCharacteristic* CH_PING;
  CBMutableCharacteristic* CH_SECURE_WRITE;
  CBMutableCharacteristic* CH_SECURE_READ;
  
  CBMutableService* SERV_PING;
  CBMutableService* SERV_INTERFACE;
  CBMutableService* SERV_SECURE_INTERFACE;
  
  CBUUID* UUID_PING;
  CBUUID* UUID_INTERFACE;
  CBUUID* UUID_SECURE_INTERFACE;
  
  NSString* _LocalName;
  NSString* _ServiceUUID;
  
  CBPeripheralManager* _PeripheralManager;
  
  NSObject* _Context;
  
  Anki::Switchboard::BLENetworkStream* _BLEStream;
  
  Signal::Signal<void (Anki::Switchboard::BLENetworkStream*)>* _ConnectedSignal;
  Signal::Signal<void (Anki::Switchboard::BLENetworkStream*)>* _DisconnectedSignal;
  
  bool _SubscribedPing;
  bool _SubscribedRead;
  bool _SubscribedWrite;
  bool _SubscribedSecureRead;
  bool _SubscribedSecureWrite;
  
  std::queue<SendContainer> _SendQueue;
}

- (bool)send:(NSData*) data characteristic:(CBMutableCharacteristic*) characteristic;

-(void) setConnectedSignal: (Signal::Signal<void (Anki::Switchboard::BLENetworkStream*)> *)connectedSignal;
-(void) setDisconnectedSignal: (Signal::Signal<void (Anki::Switchboard::BLENetworkStream*)> *)disconnectedSignal;

-(void) advertiseWithService: (const char*)name withService:(const char*)service;
-(void) stopAdvertising;

-(void) tick;

@end

#endif /* BLECentral_h */
