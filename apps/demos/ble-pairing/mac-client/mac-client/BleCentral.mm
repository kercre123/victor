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
  _rtsState = Raw;
  
  _bleMessageProtocol = std::make_unique<Anki::Switchboard::BleMessageProtocol>(20);
  
  _bleMessageProtocol->OnSendRawBufferEvent().SubscribeForever([self](uint8_t* bytes, size_t size){
    [self handleSend:bytes length:(int)size];
  });
  _bleMessageProtocol->OnReceiveMessageEvent().SubscribeForever([self](uint8_t* bytes, size_t size){
    [self handleReceive:bytes length:(int)size];
  });
  
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

- (void)printSuccess:(const char *)txt {
  NSLog(@"\033[0;32m");
  NSLog(@"%s", txt);
  NSLog(@"\033[0m");
}

// ----------------------------------------------------------------------------------------------------------------
- (void)peripheral:(CBPeripheral *)peripheral
didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic
             error:(NSError *)error {
  if([characteristic.UUID.UUIDString isEqualToString:_writeUuid.UUIDString]) {
    // Victor made write to UUID
    //NSLog(@"Receive %@", characteristic.value);
    _bleMessageProtocol->ReceiveRawBuffer((uint8_t*)characteristic.value.bytes, (size_t)characteristic.value.length);
  } else if([characteristic.UUID.UUIDString isEqualToString:_writeSecureUuid.UUIDString]) {
    // Victor made write to secure UUID
    _bleMessageProtocol->ReceiveRawBuffer((uint8_t*)characteristic.value.bytes, (size_t)characteristic.value.length);
  }
}

- (void) handleReceive:(const void*)bytes length:(int)n {
  switch(_rtsState) {
    case Raw:
      [self HandleReceiveHandshake:bytes length:n];
      break;
    case Clad: {
      Anki::Victor::ExternalComms::ExternalComms extComms;
      const size_t unpackSize = extComms.Unpack((uint8_t*)bytes, n);
      
      if(extComms.GetTag() == Anki::Victor::ExternalComms::ExternalCommsTag::RtsConnection) {
        Anki::Victor::ExternalComms::RtsConnection rtsMsg = extComms.Get_RtsConnection();
        
        switch(rtsMsg.GetTag()) {
          case Anki::Victor::ExternalComms::RtsConnectionTag::Error:
            //
            break;
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsConnRequest: {
            Anki::Victor::ExternalComms::RtsConnRequest req = rtsMsg.Get_RtsConnRequest();
            [self HandleReceivePublicKey:req];
            break;
          }
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsNonceMessage: {
            Anki::Victor::ExternalComms::RtsNonceMessage msg = rtsMsg.Get_RtsNonceMessage();
            [self HandleReceiveNonce:msg];
            break;
          }
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsCancelPairing: {
            //
            break;
          }
          case Anki::Victor::ExternalComms::RtsConnectionTag::RtsAck: {
            //
            break;
          }
        }
      }
      
      break;
    }
    case CladSecure:
      [self handleReceiveSecure:bytes length:n];
      break;
    default:
      NSLog(@"wtf");
      break;
  }
}

- (void) handleSend:(const void*)bytes length:(int)n {
  CBCharacteristic* cb = [_characteristics objectForKey:_readUuid.UUIDString];
  NSData* data = [NSData dataWithBytes:bytes length:n];
   NSLog(@"Send %@", data);
  [_peripheral writeValue:data forCharacteristic:cb type:CBCharacteristicWriteWithResponse];
}

- (void) handleReceiveSecure:(const void*)bytes length:(int)n {
  uint8_t* msgBuffer = (uint8_t*)malloc(n);
  uint64_t size;
  
  int result = crypto_aead_xchacha20poly1305_ietf_decrypt(msgBuffer, &size, nullptr, (uint8_t*)bytes, n, nullptr, 0, _nonceIn, _decryptKey);
  if(result == 0) {
    sodium_increment(_nonceIn, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  }
  
  Anki::Victor::ExternalComms::ExternalComms extComms;
  const size_t unpackSize = extComms.Unpack(msgBuffer, size);
  
  free(msgBuffer);
  
  if(extComms.GetTag() == Anki::Victor::ExternalComms::ExternalCommsTag::RtsConnection) {
    Anki::Victor::ExternalComms::RtsConnection rtsMsg = extComms.Get_RtsConnection();
    
    switch(rtsMsg.GetTag()) {
      case Anki::Victor::ExternalComms::RtsConnectionTag::Error:
        //
        break;
      case Anki::Victor::ExternalComms::RtsConnectionTag::RtsChallengeMessage: {
        Anki::Victor::ExternalComms::RtsChallengeMessage msg = rtsMsg.Get_RtsChallengeMessage();
        [self HandleChallengeMessage:msg];
        break;
      }
      case Anki::Victor::ExternalComms::RtsConnectionTag::RtsChallengeSuccessMessage: {
        Anki::Victor::ExternalComms::RtsChallengeSuccessMessage msg = rtsMsg.Get_RtsChallengeSuccessMessage();
        [self HandleChallengeSuccessMessage:msg];
        break;
      }
      case Anki::Victor::ExternalComms::RtsConnectionTag::RtsWifiConnectResponse: {
        Anki::Victor::ExternalComms::RtsWifiConnectResponse msg = rtsMsg.Get_RtsWifiConnectResponse();
        printf("Is victor connected to the internet? %d\n", msg.statusCode); // 1 = yes, 0 = no
        break;
      }
      case Anki::Victor::ExternalComms::RtsConnectionTag::RtsWifiIpResponse: {
        //
        break;
      }
      case Anki::Victor::ExternalComms::RtsConnectionTag::RtsStatusResponse: {
        //
        break;
      }
      case Anki::Victor::ExternalComms::RtsConnectionTag::RtsWifiScanResponse: {
        Anki::Victor::ExternalComms::RtsWifiScanResponse msg = rtsMsg.Get_RtsWifiScanResponse();
        [self HandleWifiScanResponse:msg];
        break;
      }
      case Anki::Victor::ExternalComms::RtsConnectionTag::RtsOtaUpdateResponse: {
        //
        break;
      }
      case Anki::Victor::ExternalComms::RtsConnectionTag::RtsCancelPairing: {
        _rtsState = Raw;
        break;
      }
      case Anki::Victor::ExternalComms::RtsConnectionTag::RtsAck: {
        //
        break;
      }
    }
  }
}

- (void) send:(const void*)bytes length:(int)n {
  if(_rtsState == CladSecure) {
    NSLog(@"Sending ENCRYPTED message...");
    [self sendSecure:bytes length:n];
  } else {
    NSLog(@"Sending message...");
    _bleMessageProtocol->SendMessage((uint8_t*)bytes, n);
  }
}

- (void) sendSecure:(const void*)bytes length:(int)n {
  uint8_t* cipherText = (uint8_t*)malloc(n + crypto_aead_xchacha20poly1305_ietf_ABYTES);
  uint64_t size;
  
  int result = crypto_aead_xchacha20poly1305_ietf_encrypt(cipherText, &size, (uint8_t*)bytes, n, nullptr, 0, nullptr, _nonceOut, _encryptKey);
  
  if(result == 0) {
    sodium_increment(_nonceOut, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  }
  
  _bleMessageProtocol->SendMessage((uint8_t*)cipherText, size);
  
  free(cipherText);
}

- (void) HandleReceiveHandshake:(const void*)bytes length:(int)n {
  NSLog(@"Received handshake");
  uint8_t* msg = (uint8_t*)bytes;
  
  if(n != 5) {
    return;
  }
  
  if(msg[0] != 1) {
    // Not Handshake message
    return;
  }
  
  uint32_t version = *(uint32_t*)(msg + 1);
  
  if(version != 1) {
    // Not Version 1
    return;
  }
  
  [self send:bytes length:n];
  
  // Update state
  _rtsState = Clad;
}

- (void) HandleReceivePublicKey:(const Anki::Victor::ExternalComms::RtsConnRequest&)msg {
  NSLog(@"Received public key from Victor");
  crypto_kx_keypair(_publicKey, _secretKey);
  memcpy(_remotePublicKey, msg.publicKey.data(), sizeof(_remotePublicKey));
  
  std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> publicKeyArray;
  memcpy(std::begin(publicKeyArray), _publicKey, crypto_kx_PUBLICKEYBYTES);
  
  crypto_kx_client_session_keys(_decryptKey, _encryptKey, _publicKey, _secretKey, _remotePublicKey);
  
  char pin[6];
  NSLog(@"Enter pin:");
  scanf("%6s",pin);
  
  uint8_t tmpDecryptKey[crypto_kx_SESSIONKEYBYTES];
  memcpy(tmpDecryptKey, _decryptKey, crypto_kx_SESSIONKEYBYTES);
  
  // Hash mix of pin and decryptKey to form new decryptKey
  crypto_generichash(_decryptKey, crypto_kx_SESSIONKEYBYTES, tmpDecryptKey, crypto_kx_SESSIONKEYBYTES, (uint8_t*)pin, 6);
  
  Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsConnResponse>(self, Anki::Victor::ExternalComms::RtsConnType::FirstTimePair,
                                                                     publicKeyArray);
}

- (void) HandleReceiveNonce:(const Anki::Victor::ExternalComms::RtsNonceMessage &)msg {
  NSLog(@"Received nonce from Victor");
  memcpy(_nonceIn, msg.nonce.data(), crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  memcpy(_nonceOut, msg.nonce.data(), crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  
  NSLog(@"Sending ack");
  Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsAck>(self, (uint8_t)Anki::Victor::ExternalComms::RtsConnectionTag::RtsNonceMessage);
  // Move to encrypted comms
  NSLog(@"Setting mode to ENCRYPTED");
  _rtsState = CladSecure;
}

- (void) HandleChallengeMessage:(const Anki::Victor::ExternalComms::RtsChallengeMessage &)msg {
  NSLog(@"Received challenge message from Victor");
  uint32_t challenge = msg.number;
  Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsChallengeMessage>(self, challenge + 1);
}

- (void) HandleChallengeSuccessMessage:(const Anki::Victor::ExternalComms::RtsChallengeSuccessMessage&)msg {
  [self printSuccess:"### Successfully Created Encrypted Channel ###"];
  Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsWifiScanRequest>(self);
}

- (void) HandleWifiScanResponse:(const Anki::Victor::ExternalComms::RtsWifiScanResponse&)msg {
  NSLog(@"Wifi scan results:");
  for(int i = 0; i < msg.result.size(); i++) {
    NSLog(@"%d: %d %d %s", i, msg.result[i].signalStrength, msg.result[i].authType, msg.result[i].ssid.c_str());
  }
  
  char ssid[32];
  char pw[64];
  
  NSLog(@"Enter wifi credentials:");
  scanf("%32s %64s", ssid, pw);
  
  Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsWifiConnectRequest>(self, std::string(ssid), std::string(pw));
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
  
  NSLog(@"Did discover characteristics.");
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
  [self StopScanning];
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
