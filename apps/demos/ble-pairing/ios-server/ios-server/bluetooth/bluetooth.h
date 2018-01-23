//
//  bluetooth.h
//  mac-server
//
//  Created by Paul Aluri on 1/9/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef bluetooth_h
#define bluetooth_h

#include "AnkiKeyExchange.h"
#include <CoreBluetooth/CoreBluetooth.h>

@interface BluetoothManager : NSObject <CBPeripheralManagerDelegate> {
  CBUUID* CHAR_WRITE;
  CBUUID* CHAR_READ;
  CBUUID* CHAR_PING;
  
  CBCharacteristic* CH_WRITE;
  CBCharacteristic* CH_READ;
  CBCharacteristic* CH_PING;
  
  CBMutableService* SERV_PING;
  CBMutableService* SERV_INTERFACE;
  
  CBUUID* UUID_PING;
  CBUUID* UUID_INTERFACE;
  
  //
  AnkiKeyExchange* _AnkiKeyExchange;
}

@property (strong, nonatomic) CBPeripheralManager* cb;

- (void) StartAdvertising;

- (void) setCharacteristics;

@end

#endif /* bluetooth_h */
