//
//  BleCentral.m
//  mac-client
//
//  Created by Paul Aluri on 2/28/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "BleCentral.h"
#include <stdio.h>
#include <sstream>
#include <iomanip>

#define START_OTA_AFTER_CONNECT 0

@implementation BleCentral

- (id)init {
  _victorService =
    [CBUUID UUIDWithString:@"FEE3"];
  _readUuid =
    [CBUUID UUIDWithString:@"7D2A4BDA-D29B-4152-B725-2491478C5CD7"];
  _writeUuid =
    [CBUUID UUIDWithString:@"30619F2D-0F54-41BD-A65A-7588D8C85B45"];
  _readSecureUuid =
    [CBUUID UUIDWithString:@"045C8155-3D7B-41BC-9DA0-0ED27D0C8A61"];
  _writeSecureUuid =
    [CBUUID UUIDWithString:@"28C35E4C-B218-43CB-9718-3D7EDE9B5316"];
  
  _localName = @"Vector B4H3";
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
  
  NSData* publicKey = [[NSUserDefaults standardUserDefaults] dataForKey:@"publicKey"];
  NSLog(@"data: %@", publicKey);
  
  //[self resetDefaults];
  _victorsDiscovered = [[NSMutableDictionary alloc] init];
  
  const char* str = "HELLO WORLD";
  std::string s = [self hexStr:(uint8_t*)str length:11];
  printf("[%s]\n", s.c_str());
  
  _reconnection = false;
  
  return [super init];
}

- (std::string)hexStr:(uint8_t*)data length:(int)len {
  std::stringstream ss;
  ss<<std::hex;
  for(int i(0);i<len;++i)
    ss<<(int)data[i];
  return ss.str();
}

- (void)resetDefaults {
  NSUserDefaults * defs = [NSUserDefaults standardUserDefaults];
  NSDictionary * dict = [defs dictionaryRepresentation];
  for (id key in dict) {
    [defs removeObjectForKey:key];
  }
  [defs synchronize];
}

- (void)StartScanning {
  _centralManager = [[CBCentralManager alloc] initWithDelegate:self queue:dispatch_get_main_queue()];
}

- (void)StopScanning {
  [_centralManager stopScan];
}

- (void)printSuccess:(const char *)txt {
  printf("\033[0;32m");
  NSLog(@"%s", txt);
  printf("\033[0m");
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
          default:
            break;
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
        
        if(msg.statusCode == 1) {
//          NSString* fp = @"/Users/paul/.ssh/id_rsa.pub";
//          NSString* key = [NSString stringWithContentsOfFile:fp encoding:NSUTF8StringEncoding error:nil];
//
//          printf("key length: %d\n", (int)key.length);
//
//          std::string contents = std::string([key UTF8String], 740);
//
//          printf("%s", contents.c_str());
//
//          std::vector<std::string> parts;
//
//          for (unsigned i = 0; i < contents.length(); i += 255) {
//            parts.push_back(contents.substr(i, 255));
//          }
//
//          Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsSshRequest>(self, parts);
//          Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsWifiIpRequest>(self);
#if START_OTA_AFTER_CONNECT
          Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsOtaUpdateRequest>(self, "http://sai-general.s3.amazonaws.com/build-assets/ota-test.tar");
#endif
        }
        
        break;
      }
      case Anki::Victor::ExternalComms::RtsConnectionTag::RtsWifiIpResponse: {
        //
        Anki::Victor::ExternalComms::RtsWifiIpResponse msg = rtsMsg.Get_RtsWifiIpResponse();
        for(int i = 0; i < 4; i++) {
          printf("%d ", msg.ipV4[i]);
        } printf("\n");
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
      case Anki::Victor::ExternalComms::RtsConnectionTag::RtsWifiAccessPointResponse: {
        Anki::Victor::ExternalComms::RtsWifiAccessPointResponse msg = rtsMsg.Get_RtsWifiAccessPointResponse();
        [self HandleReceiveAccessPointResponse:msg];
        break;
      }
      case Anki::Victor::ExternalComms::RtsConnectionTag::RtsOtaUpdateResponse: {
        Anki::Victor::ExternalComms::RtsOtaUpdateResponse msg = rtsMsg.Get_RtsOtaUpdateResponse();
        uint64_t c = msg.current;
        uint64_t t = msg.expected;
        
        int size = 100;
        int progress = (int)(((float)c/(float)t) * size);
        std::string bar = "";
        
        for(int i = 0; i < size; i++) {
          if(i <= progress) bar += "▓";
          else bar += "_";
        }
        
        printf("%100s [%d%%] [%llu/%llu] \r", bar.c_str(), progress, msg.current, msg.expected);
        fflush(stdout);
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

- (NSData*) GetPublicKey {
  NSData* publicKey =  [[NSUserDefaults standardUserDefaults] dataForKey:@"publicKey"];
  return publicKey;
}
- (NSArray*) GetSession: (NSString*)key {
  NSArray* session = [[NSUserDefaults standardUserDefaults] arrayForKey:key];
  return session;
}
- (bool) HasSavedPublicKey {
  return [[NSUserDefaults standardUserDefaults] dataForKey:@"publicKey"] != nil;
}
- (bool) HasSavedSession: (NSString*)key {
  return [[NSUserDefaults standardUserDefaults] arrayForKey:key] != nil;
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
  const void* bytes = (const void*)msg.publicKey.data();
  int n = (int)msg.publicKey.size();
  NSData* remoteKeyData = [NSData dataWithBytes:bytes length:n];
  NSMutableString* remoteKey = [[NSMutableString alloc] init];
  
  for(int i = 0; i < n; i++) {
    [remoteKey appendString:[NSString stringWithFormat:@"%x", ((uint8_t*)bytes)[i]]];
  }
  //[[NSString alloc] initWithData:remoteKeyData encoding:NSUTF8StringEncoding];
  NSLog(@"Remote key data: %@", remoteKeyData);
  NSLog(@"Remote key: %@", remoteKey);
  
  if([self HasSavedSession:remoteKey] && !([[_victorsDiscovered objectForKey:_peripheral.name] boolValue])) {
    std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> publicKeyArray;
    memcpy(std::begin(publicKeyArray), [[self GetPublicKey] bytes], crypto_kx_PUBLICKEYBYTES);
    
    NSArray* arr = [self GetSession:remoteKey];
    NSData* encKey = (NSData*)arr[0];
    NSData* decKey = (NSData*)arr[1];
    memcpy(_decryptKey, [decKey bytes], crypto_kx_SESSIONKEYBYTES);
    memcpy(_encryptKey, [encKey bytes], crypto_kx_SESSIONKEYBYTES);
    NSLog(@"Trying to renew connection");
    _reconnection = true;
    Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsConnResponse>(self, Anki::Victor::ExternalComms::RtsConnType::Reconnection,
                                                                       publicKeyArray);
  } else {
    crypto_kx_keypair(_publicKey, _secretKey);
    memcpy(_remotePublicKey, msg.publicKey.data(), sizeof(_remotePublicKey));
  
    std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> publicKeyArray;
    memcpy(std::begin(publicKeyArray), _publicKey, crypto_kx_PUBLICKEYBYTES);
    
    crypto_kx_client_session_keys(_decryptKey, _encryptKey, _publicKey, _secretKey, _remotePublicKey);
    
    uint8_t tmpDecryptKey[crypto_kx_SESSIONKEYBYTES];
    memcpy(tmpDecryptKey, _decryptKey, crypto_kx_SESSIONKEYBYTES);
    uint8_t tmpEncryptKey[crypto_kx_SESSIONKEYBYTES];
    memcpy(tmpEncryptKey, _encryptKey, crypto_kx_SESSIONKEYBYTES);
  
    // Hash mix of pin and decryptKey to form new decryptKey
    
    Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsConnResponse>(self, Anki::Victor::ExternalComms::RtsConnType::FirstTimePair,
                                                                       publicKeyArray);
    char pin[6];
    NSLog(@"Enter pin:");
    scanf("%6s",pin);
    crypto_generichash(_decryptKey, crypto_kx_SESSIONKEYBYTES, tmpDecryptKey, crypto_kx_SESSIONKEYBYTES, (uint8_t*)pin, 6);
    crypto_generichash(_encryptKey, crypto_kx_SESSIONKEYBYTES, tmpEncryptKey, crypto_kx_SESSIONKEYBYTES, (uint8_t*)pin, 6);
    
    // save settings
    NSData* publicKeyData = [NSData dataWithBytes:_publicKey length:sizeof(_publicKey)];
    NSData* encData = [NSData dataWithBytes:_encryptKey length:sizeof(_encryptKey)];
    NSData* decData = [NSData dataWithBytes:_decryptKey length:sizeof(_decryptKey)];
    
    [[NSUserDefaults standardUserDefaults] setValue:publicKeyData forKey:@"publicKey"];
    NSMutableArray* arr = [[NSMutableArray alloc] initWithObjects:encData, decData, nil];
    [[NSUserDefaults standardUserDefaults] setValue:arr forKey:remoteKey];
    [[NSUserDefaults standardUserDefaults] synchronize];
    NSLog(@"Theoretically saving stuff.");
  }
}

- (void) HandleReceiveNonce:(const Anki::Victor::ExternalComms::RtsNonceMessage &)msg {
  NSLog(@"Received nonce from Victor");
  memcpy(_nonceIn, msg.toDeviceNonce.data(), crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  memcpy(_nonceOut, msg.toRobotNonce.data(), crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  
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
  
  if(_reconnection == false) {
    Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsWifiScanRequest>(self);
  } else {
    Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsWifiScanRequest>(self);
    //Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsOtaUpdateRequest>(self, "http://sai-general.s3.amazonaws.com/build-assets/ota-test.tar");
  }
  //Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsWifiAccessPointRequest>(self, true);
}

- (void) HandleWifiScanResponse:(const Anki::Victor::ExternalComms::RtsWifiScanResponse&)msg {
  NSLog(@"Wifi scan results:");
  NSMutableDictionary* wifiAuth = [[NSMutableDictionary alloc] init];
  
  for(int i = 0; i < msg.scanResult.size(); i++) {
    NSLog(@"%d: %d %d %s", i, msg.scanResult[i].signalStrength, msg.scanResult[i].authType, msg.scanResult[i].ssid.c_str());
    NSString* ssidStr = [NSString stringWithUTF8String:msg.scanResult[i].ssid.c_str()];
    [wifiAuth setValue:[NSNumber numberWithInt:msg.scanResult[i].authType] forKey:ssidStr];
  }
  
  
  char ssid[32];
  char pw[64];
  
  NSLog(@"Enter wifi credentials:");
  scanf("%32s %64s", ssid, pw);
  
  NSString* ssidS = [NSString stringWithUTF8String:std::string(ssid).c_str()];
  
  bool hidden = false;
  WiFiAuth auth = AUTH_WPA_PSK;
  
  if([wifiAuth valueForKey:ssidS] != nullptr) {
    auth = (WiFiAuth)[[wifiAuth objectForKey:ssidS] intValue];
  } else {
    hidden = true;
  }

  
  Clad::SendRtsMessage<Anki::Victor::ExternalComms::RtsWifiConnectRequest>(self,
    [self hexStr:(uint8_t*)std::string(ssid).c_str() length:std::string(ssid).length()],
    std::string(pw), 15, auth, hidden);
}

- (void) HandleWifiAccessPointResponse:(const Anki::Victor::ExternalComms::RtsWifiAccessPointResponse&)msg {
  NSLog(@"Access point enabled with SSID: [%s] PW: [%s]", msg.ssid.c_str(), msg.pw.c_str());
}

- (void)peripheral:(CBPeripheral *)peripheral
didDiscoverServices:(NSError *)error {
  [peripheral discoverCharacteristics:nil forService:peripheral.services[0]];
  //[peripheral discoverCharacteristics:@[_readUuid, _writeUuid, _readSecureUuid, _writeSecureUuid] forService:peripheral.services[0]];
}

- (void)peripheral:(CBPeripheral *)peripheral
didDiscoverCharacteristicsForService:(CBService *)service
             error:(NSError *)error {
  for(int i = 0; i < service.characteristics.count; i++) {
    CBCharacteristic* characteristic = service.characteristics[i];
    [_characteristics setObject:characteristic forKey:characteristic.UUID.UUIDString];
    if([characteristic.UUID.UUIDString isEqualToString:_writeUuid.UUIDString]) {
      NSLog(@"Am I trying to subscribe to something?");
      [peripheral setNotifyValue:true forCharacteristic:characteristic];
    } else if ([characteristic.UUID.UUIDString isEqualToString:_writeSecureUuid.UUIDString]) {
      NSLog(@"Am I trying to subscribe to something?");
      [peripheral setNotifyValue:true forCharacteristic:characteristic];
    }
    
    NSLog(@"Did discover CHAR: %@.", characteristic.UUID.UUIDString);
  }
  
  NSLog(@"Did discover characteristics.");
}

- (void)peripheral:(CBPeripheral *)peripheral
didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic
             error:(NSError *)error {
  if(error == nil) {
    NSLog(@"We think we subscribed correctly");
  } else {
    NSLog(@"error subbing");
  }
}

// ----------------------------------------------------------------------------------------------------------------

- (void)centralManager:(CBCentralManager *)central
 didDiscoverPeripheral:(CBPeripheral *)peripheral
     advertisementData:(NSDictionary<NSString *,id> *)advertisementData
                  RSSI:(NSNumber *)RSSI {
  NSData* data = advertisementData[CBAdvertisementDataManufacturerDataKey];
  
  NSLog(@"md: %@", data);
  
  if(data == nil || data.length < 3) {
    return;
  }
  
  bool isAnki = ((uint8_t*)data.bytes)[0] == 0xF8 && ((uint8_t*)data.bytes)[1] == 0x05;
  bool isVictor = ((uint8_t*)data.bytes)[2] == 0x76;
  
  bool isPairing = false;
  bool knownName = false;
  
  if(data.length > 3) {
    isPairing = ((uint8_t*)data.bytes)[3] == 0x70;
  }
  
  // set global bool
  [_victorsDiscovered setValue:[NSNumber numberWithBool:isPairing] forKey:peripheral.name];
  
  NSString* savedName = [[NSUserDefaults standardUserDefaults] stringForKey:@"victorName"];
  knownName = [savedName isEqualToString:peripheral.name];
  
  NSLog(@"[%@] isPairing:%d knownName:%d isAnki:%d", peripheral.name, isPairing, knownName, isAnki);
  
  if([peripheral.name containsString:@"T9D3"]) {
    return;
  }
  
  if((isAnki && isVictor && (isPairing || knownName)) && !_connecting) {
    NSLog(@"Connecting to %@", peripheral.name);
    [[NSUserDefaults standardUserDefaults] setValue:peripheral.name forKey:@"victorName"];
    _peripheral = peripheral;
    peripheral.delegate = self;
    [_centralManager connectPeripheral:peripheral options:nullptr];
    _connecting = true;
  } else if(!_connecting) {
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
