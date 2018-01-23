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
#include "BLENetworkStream.h"
#include <CoreBluetooth/CoreBluetooth.h>

@interface BLEPeripheral : NSObject <CBPeripheralManagerDelegate> {
  CBUUID* CHAR_WRITE;
  CBUUID* CHAR_READ;
  CBUUID* CHAR_PING;
  
  CBMutableCharacteristic* CH_WRITE;
  CBMutableCharacteristic* CH_READ;
  CBMutableCharacteristic* CH_PING;
  
  CBMutableService* SERV_PING;
  CBMutableService* SERV_INTERFACE;
  
  CBUUID* UUID_PING;
  CBUUID* UUID_INTERFACE;
  
  NSString* _LocalName;
  NSString* _ServiceUUID;
  
  CBPeripheralManager* _PeripheralManager;
  
  NSObject* _Context;
  
  Anki::Networking::BLENetworkStream* _BLEStream;
  
  Signal::Signal<void (Anki::Networking::BLENetworkStream*)>* _ConnectedSignal;
  
  bool _SubscribedPing;
  bool _SubscribedRead;
  bool _SubscribedWrite;
}

-(void) setConnectedSignal: (Signal::Signal<void (Anki::Networking::BLENetworkStream*)> *)connectedSignal;

-(void) advertiseWithService: (const char*)name withService:(const char*)service;

@end

#endif /* BLECentral_h */
