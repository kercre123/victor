//
//  bluetooth.m
//  mac-server
//
//  Created by Paul Aluri on 1/9/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreBluetooth/CoreBluetooth.h>
#import "bluetooth.h"
#import "AnkiKeyExchange.h"

@implementation BluetoothManager

- (void) StartAdvertising {
  [self setCharacteristics];
  
  _cb = [[CBPeripheralManager alloc] init];
  _cb.delegate = self;
}

- (void)setCharacteristics {
  CHAR_WRITE = [CBUUID UUIDWithString:@"3333"];
  CHAR_READ = [CBUUID UUIDWithString:@"4444"];
  CHAR_PING = [CBUUID UUIDWithString:@"5555"];
  
  UUID_PING = [CBUUID UUIDWithString:@"ABCD"];
  UUID_INTERFACE = [CBUUID UUIDWithString:@"1234"];
  
  CH_WRITE = [[CBMutableCharacteristic alloc]
    initWithType:CHAR_WRITE
    properties: CBCharacteristicPropertyRead |
                CBCharacteristicPropertyWrite |
                CBCharacteristicPropertyNotify
    value:nil
    permissions:CBAttributePermissionsReadable |
                CBAttributePermissionsWriteable];
  
  CH_READ = [[CBMutableCharacteristic alloc]
              initWithType:CHAR_READ
              properties: CBCharacteristicPropertyRead |
              CBCharacteristicPropertyWrite |
              CBCharacteristicPropertyNotify
              value:nil
              permissions:CBAttributePermissionsReadable |
              CBAttributePermissionsWriteable];
  
  CH_PING = [[CBMutableCharacteristic alloc]
             initWithType:CHAR_PING
             properties: CBCharacteristicPropertyRead |
             CBCharacteristicPropertyWrite |
             CBCharacteristicPropertyNotify
             value:nil
             permissions:CBAttributePermissionsReadable |
             CBAttributePermissionsWriteable];
  
  SERV_PING = [[CBMutableService alloc] initWithType:UUID_PING primary:true];
  SERV_INTERFACE = [[CBMutableService alloc] initWithType:UUID_INTERFACE primary:true];
  
  SERV_PING.characteristics = @[ CH_PING ];
  SERV_INTERFACE.characteristics = @[ CH_READ, CH_WRITE ];
}

- (void)peripheralManagerDidUpdateState:(nonnull CBPeripheralManager *)peripheral {
  if(peripheral.state == CBManagerStatePoweredOn) {
    NSDictionary* adData = [[NSMutableDictionary alloc] init];
    CBUUID *uuid = [CBUUID UUIDWithString:@"00112233-4455-6677-8899-887766554433"];
    
    [adData setValue:@"Victor D0G3" forKey:CBAdvertisementDataLocalNameKey];
    [adData setValue:@[uuid] forKey:CBAdvertisementDataServiceUUIDsKey];
    
    [peripheral addService:SERV_PING];
    [peripheral addService:SERV_INTERFACE];
    
    [_cb startAdvertising:adData];
    
    NSLog(@"Start advertising");
  }
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral
  didReceiveWriteRequests:(NSArray<CBATTRequest *> *)requests {
  
  NSLog(@"Received write request");
  
  for(int i = 0; i < requests.count; i++) {
    if(requests[i].characteristic.UUID.UUIDString == CH_READ.UUID.UUIDString) {
      // We are receiving input to read stream
      NSString* dataString = [[NSString alloc] initWithData:requests[i].value encoding:NSUTF8StringEncoding];
      
      NSLog(@"Received data: %@", requests[i].value);
      NSLog(dataString);
      [peripheral respondToRequest:requests[i] withResult:CBATTErrorSuccess];
    }
  }
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral central:(CBCentral *)central didSubscribeToCharacteristic:(CBCharacteristic *)characteristic {
  if(characteristic.UUID.UUIDString == CH_PING.UUID.UUIDString) {
    // initialize keyex
    _AnkiKeyExchange = new AnkiKeyExchange();
    
    // connection made -> begin kx, share public key
    unsigned char* publicKey = _AnkiKeyExchange->generateKeys();
    NSData* publicKeyData = [[NSData alloc] initWithBytes:publicKey length:crypto_kx_SESSIONKEYBYTES];
    
    // write public key
    NSLog(@"Sending public key: %@", publicKeyData);
    [peripheral updateValue:publicKeyData forCharacteristic:CH_WRITE onSubscribedCentrals:nil];
  }
}

@end
