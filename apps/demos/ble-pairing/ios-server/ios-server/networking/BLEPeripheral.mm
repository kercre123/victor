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
  
  CHAR_SECURE_READ = [CBUUID UUIDWithString:@"6677"];
  CHAR_SECURE_WRITE = [CBUUID UUIDWithString:@"7766"];
  UUID_SECURE_INTERFACE = [CBUUID UUIDWithString:@"6767"];
  
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
  
  CH_SECURE_WRITE = [[CBMutableCharacteristic alloc]
              initWithType:CHAR_SECURE_WRITE
              properties: CBCharacteristicPropertyRead |
              CBCharacteristicPropertyWrite |
              CBCharacteristicPropertyNotify
              value:nil
              permissions:CBAttributePermissionsReadable |
              CBAttributePermissionsWriteable];
  
  CH_SECURE_READ = [[CBMutableCharacteristic alloc]
             initWithType:CHAR_SECURE_READ
             properties: CBCharacteristicPropertyRead |
             CBCharacteristicPropertyWrite |
             CBCharacteristicPropertyNotify
             value:nil
             permissions:CBAttributePermissionsReadable |
             CBAttributePermissionsWriteable];
  
  SERV_PING = [[CBMutableService alloc] initWithType:UUID_PING primary:true];
  SERV_INTERFACE = [[CBMutableService alloc] initWithType:UUID_INTERFACE primary:true];
  SERV_SECURE_INTERFACE = [[CBMutableService alloc] initWithType:UUID_SECURE_INTERFACE primary:true];
  
  SERV_PING.characteristics = @[ CH_PING ];
  SERV_INTERFACE.characteristics = @[ CH_READ, CH_WRITE ];
  SERV_SECURE_INTERFACE.characteristics = @[ CH_SECURE_READ, CH_SECURE_WRITE ];
  
  _SubscribedWrite = false;
  _SubscribedRead = false;
  _SubscribedPing = false;
  _SubscribedSecureRead = false;
  _SubscribedSecureWrite = false;
  
  return [super init];
}

-(void) setConnectedSignal: (Signal::Signal<void (Anki::Networking::BLENetworkStream*)> *)connectedSignal {
  _ConnectedSignal = connectedSignal;
}

-(void) setDisconnectedSignal: (Signal::Signal<void (Anki::Networking::BLENetworkStream*)> *)disconnectedSignal {
  _DisconnectedSignal = disconnectedSignal;
}

-(void) advertiseWithService: (const char*)name withService:(const char*)service {
  _LocalName = [NSString stringWithUTF8String:name];
  _ServiceUUID = [NSString stringWithUTF8String:service];
  
  _PeripheralManager = [[CBPeripheralManager alloc] init];
  _PeripheralManager.delegate = self;
  
  _BLEStream = new Anki::Networking::BLENetworkStream(_PeripheralManager, CH_WRITE, CH_SECURE_WRITE);
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral central:(CBCentral *)central didSubscribeToCharacteristic:(CBCharacteristic *)characteristic {
  if(characteristic.UUID.UUIDString == CH_PING.UUID.UUIDString) {
    _SubscribedPing = true;
  } else if(characteristic.UUID.UUIDString == CH_READ.UUID.UUIDString) {
    _SubscribedRead = true;
  } else if(characteristic.UUID.UUIDString == CH_WRITE.UUID.UUIDString) {
    _SubscribedWrite = true;
  } else if(characteristic.UUID.UUIDString == CH_SECURE_READ.UUID.UUIDString) {
    _SubscribedSecureRead = true;
  } else if(characteristic.UUID.UUIDString == CH_SECURE_WRITE.UUID.UUIDString) {
    _SubscribedSecureWrite = true;
  }
  
  if(_SubscribedPing &&
     _SubscribedRead &&
     _SubscribedWrite &&
     _SubscribedSecureRead &&
     _SubscribedSecureWrite) {
    // call connection signal
    _ConnectedSignal->emit(_BLEStream);
  }
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral
                  central:(CBCentral *)central
didUnsubscribeFromCharacteristic:(CBCharacteristic *)characteristic {
  if(characteristic.UUID.UUIDString == CH_PING.UUID.UUIDString) {
    _SubscribedPing = false;
    _SubscribedRead = false;
    _SubscribedWrite = false;
    _SubscribedSecureRead = false;
    _SubscribedSecureWrite = false;
    
    _DisconnectedSignal->emit(_BLEStream);
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
    [peripheral addService:SERV_SECURE_INTERFACE];
    
    [_PeripheralManager startAdvertising:adData];
  }
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral
  didReceiveWriteRequests:(NSArray<CBATTRequest *> *)requests {
  for(int i = 0; i < requests.count; i++) {
    if(requests[i].characteristic.UUID.UUIDString == CH_READ.UUID.UUIDString) {
      // We are receiving input to read stream
      Log::Write("------> [plain text] Receiving incoming writee to BLE characteristic");
      ((Anki::Networking::INetworkStream*)_BLEStream)->ReceivePlainText((uint8_t*)requests[i].value.bytes, (int)requests[i].value.length);
      [peripheral respondToRequest:requests[i] withResult:CBATTErrorSuccess];
    } else if(requests[i].characteristic.UUID.UUIDString == CH_SECURE_READ.UUID.UUIDString) {
      // We are receiving input to read stream
      Log::Write("------> [##encrypted##] Receiving incoming writee to BLE characteristic");
      ((Anki::Networking::INetworkStream*)_BLEStream)->ReceiveEncrypted((uint8_t*)requests[i].value.bytes, (int)requests[i].value.length);
      [peripheral respondToRequest:requests[i] withResult:CBATTErrorSuccess];
    }
  }
}

@end
