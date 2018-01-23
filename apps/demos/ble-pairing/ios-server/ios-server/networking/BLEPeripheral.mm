//
//  BLECentral.m
//  ios-server
//
//  Created by Paul Aluri on 1/16/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import "BLEPeripheral.h"

@implementation BLEPeripheral

-(BLEPeripheral*) init {
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
  
  _SubscribedWrite = false;
  _SubscribedRead = false;
  _SubscribedPing = false;
  
  return [super init];
}

-(void) setConnectedSignal: (Signal::Signal<void (Anki::Networking::BLENetworkStream*)> *)connectedSignal {
  _ConnectedSignal = connectedSignal;
}

-(void) advertiseWithService: (const char*)name withService:(const char*)service {
  _LocalName = [NSString stringWithUTF8String:name];
  _ServiceUUID = [NSString stringWithUTF8String:service];
  
  _PeripheralManager = [[CBPeripheralManager alloc] init];
  _PeripheralManager.delegate = self;
  
  _BLEStream = new Anki::Networking::BLENetworkStream(_PeripheralManager, CH_WRITE);
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral central:(CBCentral *)central didSubscribeToCharacteristic:(CBCharacteristic *)characteristic {
  if(characteristic.UUID.UUIDString == CH_PING.UUID.UUIDString) {
    _SubscribedPing = true;
  } else if(characteristic.UUID.UUIDString == CH_READ.UUID.UUIDString) {
    _SubscribedRead = true;
  } else if(characteristic.UUID.UUIDString == CH_WRITE.UUID.UUIDString) {
    _SubscribedWrite = true;
  }
  
  if(_SubscribedPing && _SubscribedRead && _SubscribedWrite) {
    // call connection signal
    _ConnectedSignal->emit(_BLEStream);
  }
}

- (void)peripheralManagerDidUpdateState:(nonnull CBPeripheralManager *)peripheral {
  if(peripheral.state == CBManagerStatePoweredOn) {
    NSDictionary* adData = [[NSMutableDictionary alloc] init];
    CBUUID *uuid = [CBUUID UUIDWithString:_ServiceUUID];
    
    [adData setValue:_LocalName forKey:CBAdvertisementDataLocalNameKey];
    [adData setValue:@[uuid] forKey:CBAdvertisementDataServiceUUIDsKey];
    
    [peripheral addService:SERV_PING];
    [peripheral addService:SERV_INTERFACE];
    
    [_PeripheralManager startAdvertising:adData];
  }
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral
  didReceiveWriteRequests:(NSArray<CBATTRequest *> *)requests {
  
  for(int i = 0; i < requests.count; i++) {
    if(requests[i].characteristic.UUID.UUIDString == CH_READ.UUID.UUIDString) {
      // We are receiving input to read stream
      ((Anki::Networking::INetworkStream*)_BLEStream)->Receive((char*)requests[i].value.bytes, (int)requests[i].value.length);
      [peripheral respondToRequest:requests[i] withResult:CBATTErrorSuccess];
    }
  }
}

@end
