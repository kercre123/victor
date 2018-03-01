//
//  BleClient.h
//  mac-client
//
//  Created by Paul Aluri on 2/28/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef BleClient_h
#define BleClient_h

#include <CoreBluetooth/CoreBluetooth.h>

@interface BleCentral : NSObject <CBCentralManagerDelegate, CBPeripheralDelegate> {
  NSString* _localName;
  
  CBCentralManager* _centralManager;
  CBUUID* _victorService;
  CBUUID* _readUuid;
  CBUUID* _writeUuid;
  CBUUID* _readSecureUuid;
  CBUUID* _writeSecureUuid;
  
  CBPeripheral* _peripheral;
  
  NSMutableDictionary* _characteristics;
  
  bool _connecting;
}

- (void) handleReceive:(const void*)bytes length:(int)n;
- (void) handleReceiveSecure:(const void*)bytes length:(int)n;

- (void) send:(const void*)bytes length:(int)n;
- (void) sendSecure:(const void*)bytes length:(int)n;

- (void) StartScanning;
- (void) StopScanning;

@end

#endif /* BleClient_h */
