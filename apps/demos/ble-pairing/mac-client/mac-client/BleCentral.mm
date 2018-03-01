//
//  BleCentral.m
//  mac-client
//
//  Created by Paul Aluri on 2/28/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "BleCentral.h"

@implementation BleCentral

- (id)init {
  _victorService =
    [CBUUID UUIDWithString:@"D55E356B-59CC-4265-9D5F-3C61E9DFD70F"];
  _readUuid =
    [CBUUID UUIDWithString:@"7D2A4BDA-D29B-4152-B725-2491478C5CD7"];
  _writeUuid =
    [CBUUID UUIDWithString:@"30619F2D-0F54-41BD-A65A-7588D8C85B45"];
  _readSecureUuid =
    [CBUUID UUIDWithString:@"045C8155-3D7B-41BC-9DA0-0ED27D0C8A61"];
  _writeSecureUuid =
    [CBUUID UUIDWithString:@"28C35E4C-B218-43CB-9718-3D7EDE9B5316"];
  
  _localName = @"VICTOR_1dd99b69";
  _connecting = false;
  
  _characteristics = [NSMutableDictionary dictionaryWithCapacity:4];
  
  NSLog(@"Init bleCentral");
  
  return [super init];
}

- (void)StartScanning {
  _centralManager = [[CBCentralManager alloc] initWithDelegate:self queue:dispatch_get_main_queue()];
}

- (void)StopScanning {
  [_centralManager stopScan];
}

// ----------------------------------------------------------------------------------------------------------------
- (void)peripheral:(CBPeripheral *)peripheral
didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic
             error:(NSError *)error {
  if([characteristic.UUID.UUIDString isEqualToString:_writeUuid.UUIDString]) {
    // Victor made write to UUID
    NSLog(@"%@", characteristic.value);
    [self handleReceive:characteristic.value.bytes length:(int)characteristic.value.length];
  } else if([characteristic.UUID.UUIDString isEqualToString:_writeSecureUuid.UUIDString]) {
    // Victor made write to secure UUID
    [self handleReceiveSecure:characteristic.value.bytes length:(int)characteristic.value.length];
  }
}

- (void) handleReceive:(const void*)bytes length:(int)n {
}

- (void) handleReceiveSecure:(const void*)bytes length:(int)n {
  
}

- (void) send:(const void*)bytes length:(int)n {
  
}

- (void) sendSecure:(const void*)bytes length:(int)n {
  
}

- (void)peripheral:(CBPeripheral *)peripheral
didDiscoverServices:(NSError *)error {
  [peripheral discoverCharacteristics:@[_readUuid, _writeUuid, _readSecureUuid, _writeSecureUuid] forService:peripheral.services[0]];
}

- (void)peripheral:(CBPeripheral *)peripheral
didDiscoverCharacteristicsForService:(CBService *)service
             error:(NSError *)error {
  for(int i = 0; i < service.characteristics.count; i++) {
    CBCharacteristic* characteristic = service.characteristics[i];
    [_characteristics setObject:characteristic forKey:characteristic.UUID.UUIDString];
    [peripheral setNotifyValue:true forCharacteristic:characteristic];
  }
}

// ----------------------------------------------------------------------------------------------------------------

- (void)centralManager:(CBCentralManager *)central
 didDiscoverPeripheral:(CBPeripheral *)peripheral
     advertisementData:(NSDictionary<NSString *,id> *)advertisementData
                  RSSI:(NSNumber *)RSSI {
  if([peripheral.name isEqualToString:_localName] && !_connecting) {
     NSLog(@"Connecting to %@", peripheral.name);
    _peripheral = peripheral;
    peripheral.delegate = self;
    [_centralManager connectPeripheral:peripheral options:nullptr];
    _connecting = true;
  } else {
    NSLog(@"Ignoring %@", peripheral.name);
  }
}

- (void)centralManager:(CBCentralManager *)central
didFailToConnectPeripheral:(CBPeripheral *)peripheral
                 error:(NSError *)error {
  _connecting = false;
  NSLog(@"Failed to connect");
}

- (void)centralManager:(CBCentralManager *)central
  didConnectPeripheral:(CBPeripheral *)peripheral {
  NSLog(@"Connected to peripheral");
  [peripheral discoverServices:@[_victorService]];
}

- (void)centralManagerDidUpdateState:(nonnull CBCentralManager *)central {
  switch(central.state) {
    case CBCentralManagerStatePoweredOn:
      NSLog(@"Powered On BleCentral");
      [_centralManager
       scanForPeripheralsWithServices:@[_victorService]
       options:@{ CBCentralManagerScanOptionAllowDuplicatesKey: @true }];
    default:
      break;
  }
}

@end
