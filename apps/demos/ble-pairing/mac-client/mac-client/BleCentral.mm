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
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <ifaddrs.h>
#include <arpa/inet.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "signal.h"

@implementation BleCentral

BleCentral* centralContext;

- (void)printHelp {
  printf("# vic shell commands #\n");
  printf("  wifi-scan                                    Scan for wifi networks in range.\n");
  printf("  wifi-connect <ssid> <password>               Connect Vector to a wifi network.\n");
  printf("  wifi-ip                                      Get Victor's IP address\n");
  printf("  wifi-ap <bool>                               Enable/Disable Access Point mode on Vector ('true' or 'false')\n");
  printf("  ota-start <url>                              Start Ota update with provided URL string argument.\n");
  printf("  ota-progress                                 Get current Ota download progress.\n");
  printf("  ota-cancel                                   Cancel an on-going Ota download.\n");
  printf("  status                                       Get Vector's general status.\n");
  printf("  ssh-send [filename]                          Generates/Sends a public SSH key to Victor.\n");
  printf("  ssh-start                                    Tries to start an SSH session with Victor.\n");
  printf("  logs [directory]                             Downloads logs tar from Victor, with optional supplied directory path.\n");
}

- (void)showProgress: (float)current expected:(float)expected {
  int size = 100;
  int progress = (int)(((float)current/(float)expected) * size);
  std::string bar = "";
  
  for(int i = 0; i < size; i++) {
    if(i <= progress) bar += "▓";
    else bar += "_";
  }
  
  printf("%100s [%d%%] [%llu/%llu] \r", bar.c_str(), progress, (uint64_t)current, (uint64_t)expected);
  fflush(stdout);
}

- (void)setVerbose:(bool)enabled {
  _verbose = enabled;
}

- (void)setDownload:(bool)enabled {
  _download = enabled;
}

- (void)devDownloadOta {
  NSFileManager* fileManager = [NSFileManager defaultManager];
  
  NSArray* pathParts = [NSArray arrayWithObjects:NSHomeDirectory(), @"Downloads", @"mac-client-ota", nil];
  NSString* otaName = @"latest.ota";
  NSString* dirPath = [NSString pathWithComponents:pathParts];
  NSString* filePath = [NSString pathWithComponents:@[dirPath, otaName]];
  
  (void)[fileManager createDirectoryAtPath:dirPath withIntermediateDirectories:true attributes:nil error:nil];
  
  std::string cmdCurl = "curl http://ota-cdn.anki.com/vic/dev/werih2382df23ij/full/latest.ota --output " + std::string(filePath.UTF8String) + "; ";
  
  if(!_download) {
    cmdCurl = "";
  }

  NSString *s = [NSString stringWithFormat: @"tell application \"Terminal\" to do script \"%spushd %@; python -m SimpleHTTPServer; popd\"", cmdCurl.c_str(), dirPath];

  NSAppleScript *as = [[NSAppleScript alloc] initWithSource: s];
  [as executeAndReturnError:nil];
  
  // Move to wifi-ap mode
  Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsWifiAccessPointRequest>(self, _commVersion, true);
}

- (id)init {
  self = [super init];
  
  if(self) {
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
    
    if(_verbose) NSLog(@"Init bleCentral");
    
    _victorsDiscovered = [[NSMutableDictionary alloc] init];
    _reconnection = false;
    _filter = @"";
    _readyForNextCommand = true;
    _commandQueue = dispatch_queue_create("commands", NULL);
    _requestQueue = dispatch_queue_create("requests", NULL);
    _commVersion = 0;
    
    _colorArray = @[ @32, @33, @34, @35, @36 ];
    
    _otaProgress = 0;
    _otaStatusCode = 0;
    _otaExpected = 0;
    
    _isPairing = false;
  }
  
  return self;
}

void CancelCommand(int s) {
  [centralContext interrupt];
}

- (void)interrupt {
  _readyForNextCommand = true;
  _currentCommand = "";
  
  std::cout << "\b\b  \b\b";
}

- (std::string)hexStr:(char*)data length:(int)len {
  std::stringstream ss;
  for(int i(0);i<len;++i)
    ss << std::setfill('0') << std::setw(2) << std::hex << (int)data[i];
  return ss.str();
}

- (std::string)asciiStr:(char*)data length:(int)size {
  if(size % 2 != 0) {
    return "! Odd size";
  }
  
  std::string ascii = "";
  
  for(int i = 0; i < size; i += 2) {
    uint8_t a = [self nibbleToNumber:data[i]];
    uint8_t b = [self nibbleToNumber:data[i+1]];
    
    if(a == 255 || b == 255) {
      return "! Non-HEX character in string";
    }
    
    ascii += (a << 4) | b;
  }
  
  return ascii;
}

- (uint8_t)nibbleToNumber:(uint8_t)nibble {
  if(nibble >= '0' && nibble <= '9') {
    return nibble - '0';
  } else if(nibble >= 'A' && nibble <= 'F') {
    return nibble - 'A' + 10;
  } else if(nibble >= 'a' && nibble <= 'f') {
    return nibble - 'a' + 10;
  } else {
    return 255;
  }
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
  [self StartScanning:@""];
}

- (void) StartScanning:(NSString*)nameFilter {
  _filter = nameFilter;
  _centralManager = [[CBCentralManager alloc] initWithDelegate:self queue:dispatch_get_main_queue()];
}

- (void)StopScanning {
  [_centralManager stopScan];
}

- (void)printSuccess:(const char *)txt {
  if(_verbose) {
    printf("\033[0;32m");
    NSLog(@"%s", txt);
    printf("\033[0m");
  }
}

// ----------------------------------------------------------------------------------------------------------------
- (void)peripheral:(CBPeripheral *)peripheral
didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic
             error:(NSError *)error {
  if([characteristic.UUID.UUIDString isEqualToString:_writeUuid.UUIDString]) {
    // Victor made write to UUID
    //NSLog(@"Receive %@", characteristic.value);
    if(_verbose) {
      printf("Received Raw [");
      for(int i = 0; i < characteristic.value.length; i++) {
        printf("%x ", ((uint8_t*)characteristic.value.bytes)[i]);
      } printf("]\n");
    }
    _bleMessageProtocol->ReceiveRawBuffer((uint8_t*)characteristic.value.bytes, (size_t)characteristic.value.length);
  } else if([characteristic.UUID.UUIDString isEqualToString:_writeSecureUuid.UUIDString]) {
    // Victor made write to secure UUID
    _bleMessageProtocol->ReceiveRawBuffer((uint8_t*)characteristic.value.bytes, (size_t)characteristic.value.length);
  }
}

- (void) handleReceive:(const void*)bytes length:(int)n {
  if(_verbose) {
    printf("Received [");
    for(int i = 0; i < n; i++) {
      printf("%x ", ((uint8_t*)bytes)[i]);
    } printf("]\n");
  }
  
  switch(_rtsState) {
    case Raw:
      [self HandleReceiveHandshake:bytes length:n];
      break;
    case Clad: {
      Anki::Cozmo::ExternalComms::ExternalComms extComms;
      extComms.Unpack((uint8_t*)bytes, n);
        
      if(_commVersion == 1) {
        if(extComms.GetTag() == Anki::Cozmo::ExternalComms::ExternalCommsTag::RtsConnection_1) {
          Anki::Cozmo::ExternalComms::RtsConnection_1 rtsMsg = extComms.Get_RtsConnection_1();
          
          switch(rtsMsg.GetTag()) {
            case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::Error:
              //
              break;
            case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsConnRequest: {
              Anki::Cozmo::ExternalComms::RtsConnRequest req = rtsMsg.Get_RtsConnRequest();
              [self HandleReceivePublicKey:req];
              break;
            }
            case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsNonceMessage: {
              Anki::Cozmo::ExternalComms::RtsNonceMessage msg = rtsMsg.Get_RtsNonceMessage();
              [self HandleReceiveNonce:msg];
              break;
            }
            case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsCancelPairing: {
              //
              _rtsState = Raw;
              break;
            }
            case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsAck: {
              //
              break;
            }
            default:
              break;
          }
        }
      } else if(_commVersion == 2){
        if(extComms.GetTag() == Anki::Cozmo::ExternalComms::ExternalCommsTag::RtsConnection) {
          Anki::Cozmo::ExternalComms::RtsConnection rtsMsg = extComms.Get_RtsConnection();
          Anki::Cozmo::ExternalComms::RtsConnection_2 rts2Msg = rtsMsg.Get_RtsConnection_2();
          
          switch(rts2Msg.GetTag()) {
            case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::Error:
              //
              break;
            case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsConnRequest: {
              Anki::Cozmo::ExternalComms::RtsConnRequest req = rts2Msg.Get_RtsConnRequest();
              [self HandleReceivePublicKey:req];
              break;
            }
            case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsNonceMessage: {
              Anki::Cozmo::ExternalComms::RtsNonceMessage msg = rts2Msg.Get_RtsNonceMessage();
              [self HandleReceiveNonce:msg];
              break;
            }
            case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsCancelPairing: {
              //
              _rtsState = Raw;
              break;
            }
            case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsAck: {
              //
              break;
            }
            default:
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
      if(_verbose) NSLog(@"wtf");
      break;
  }
}

- (void) handleSend:(const void*)bytes length:(int)n {
  CBCharacteristic* cb = [_characteristics objectForKey:_readUuid.UUIDString];
  NSData* data = [NSData dataWithBytes:bytes length:n];
   if(_verbose) NSLog(@"Send %@", data);
  [_peripheral writeValue:data forCharacteristic:cb type:CBCharacteristicWriteWithResponse];
}

- (void) handleReceiveSecure:(const void*)bytes length:(int)n {
  uint8_t* msgBuffer = (uint8_t*)malloc(n);
  uint64_t size;
  
  int result = crypto_aead_xchacha20poly1305_ietf_decrypt(msgBuffer, &size, nullptr, (uint8_t*)bytes, n, nullptr, 0, _nonceIn, _decryptKey);
  if(result == 0) {
    sodium_increment(_nonceIn, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  } else {
    if(_verbose) NSLog(@"Error in decrypting challenge. Our key must be bad. :(");
    return;
  }
  
  Anki::Cozmo::ExternalComms::ExternalComms extComms;
  extComms.Unpack(msgBuffer, size);
  
  free(msgBuffer);
  
  if(_commVersion == 1) {
    if(extComms.GetTag() == Anki::Cozmo::ExternalComms::ExternalCommsTag::RtsConnection_1) {
      Anki::Cozmo::ExternalComms::RtsConnection_1 rtsMsg = extComms.Get_RtsConnection_1();
      
      switch(rtsMsg.GetTag()) {
        case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::Error:
          //
          break;
        case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsChallengeMessage: {
          Anki::Cozmo::ExternalComms::RtsChallengeMessage msg = rtsMsg.Get_RtsChallengeMessage();
          [self HandleChallengeMessage:msg];
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsChallengeSuccessMessage: {
          Anki::Cozmo::ExternalComms::RtsChallengeSuccessMessage msg = rtsMsg.Get_RtsChallengeSuccessMessage();
          [self HandleChallengeSuccessMessage:msg];
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsWifiIpResponse: {
          //
          Anki::Cozmo::ExternalComms::RtsWifiIpResponse msg = rtsMsg.Get_RtsWifiIpResponse();
          
          if(_currentCommand == "wifi-ip" && !_readyForNextCommand) {
            if(msg.hasIpV4) {
              char ipv4String[INET_ADDRSTRLEN] = {0};
              inet_ntop(AF_INET, msg.ipV4.data(), ipv4String, INET_ADDRSTRLEN);
              printf("IPv4: %s\n", ipv4String);
            }
            
            if(msg.hasIpV6) {
              char ipv6String[INET6_ADDRSTRLEN] = {0};
              inet_ntop(AF_INET6, msg.ipV6.data(), ipv6String, INET6_ADDRSTRLEN);
              printf("IPv6: %s\n", ipv6String);
            }
            
            _readyForNextCommand = true;
          } else if(_currentCommand == "ssh-start" && !_readyForNextCommand) {
            NSString* sshArg = [NSString stringWithFormat:@"root@%d.%d.%d.%d", msg.ipV4[0], msg.ipV4[1], msg.ipV4[2], msg.ipV4[3]];
            
            NSString *s = [NSString stringWithFormat:
                           @"tell application \"Terminal\" to do script \"ssh %@\"", sshArg];
            
            NSAppleScript *as = [[NSAppleScript alloc] initWithSource: s];
            [as executeAndReturnError:nil];
            
            _readyForNextCommand = true;
          }
          
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsWifiConnectResponse: {
          Anki::Cozmo::ExternalComms::RtsWifiConnectResponse msg = rtsMsg.Get_RtsWifiConnectResponse();
          
          if(_currentCommand == "wifi-connect") {
            switch(msg.wifiState) {
              case 1:
                printf("Vector is connected to the internet.\n");
                break;
              case 0:
                printf("Unknown connection status.\n");
                break;
              case 2:
                printf("Vector is connected without internet.\n");
                break;
              case 3:
                printf("Vector is not connected to a network.\n");
                break;
              default:
                break;
            }
          }
          
          if(_currentCommand == "wifi-connect" && !_readyForNextCommand) {
            _readyForNextCommand = true;
          }
          
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsStatusResponse: {
          //
          Anki::Cozmo::ExternalComms::RtsStatusResponse msg = rtsMsg.Get_RtsStatusResponse();
          
          std::string state = "";
          switch(msg.wifiState) {
            case 1:
              state = "ONLINE";
              break;
            case 0:
              state = "UNKNOWN";
              break;
            case 2:
              state = "CONNECTED / NO INTERNET";
              break;
            case 3:
              state = "DISCONNECTED";
              break;
            default:
              break;
          }

          printf("             ssid = %s\n connection_state = %s\n     access_point = %s\n", [self asciiStr:(char*)msg.wifiSsidHex.c_str() length:(int)msg.wifiSsidHex.length()].c_str(), state.c_str(), msg.accessPoint? "true" : "false");
          if(_currentCommand == "status" && !_readyForNextCommand) {
            _readyForNextCommand = true;
          }
          
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsWifiScanResponse: {
          Anki::Cozmo::ExternalComms::RtsWifiScanResponse msg = rtsMsg.Get_RtsWifiScanResponse();
          [self HandleWifiScanResponse:msg];
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsWifiAccessPointResponse: {
          Anki::Cozmo::ExternalComms::RtsWifiAccessPointResponse msg = rtsMsg.Get_RtsWifiAccessPointResponse();
          [self HandleReceiveAccessPointResponse:msg];
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsOtaUpdateResponse: {
          Anki::Cozmo::ExternalComms::RtsOtaUpdateResponse msg = rtsMsg.Get_RtsOtaUpdateResponse();
          _otaStatusCode = msg.status;
          
          if(_otaStatusCode == 2) {
            _otaProgress = msg.current == 0? _otaProgress : msg.current;
          } else {
            _otaProgress = msg.current;
          }
          _otaExpected = msg.expected;
          
          /*
           * Commenting out for visibility because in next pass, going
           * to use this code again to show OTA progress bar.
           */
          if(_otaStatusCode != 2 && _otaStatusCode != 1) {
            printf("ota status code: %d\n", _otaStatusCode);
          }
          
          if(_currentCommand == "ota-progress" && !_readyForNextCommand) {
            if(_otaStatusCode != 2) {
              _readyForNextCommand = true;
              _currentCommand = "";
            }
            
            int size = 100;
            int progress = (int)(((float)_otaProgress/(float)_otaExpected) * size);
            std::string bar = "";
            
            for(int i = 0; i < size; i++) {
              if(i <= progress) bar += "▓";
              else bar += "_";
            }
            
            printf("%100s [%d%%] [%llu/%llu] \r", bar.c_str(), progress, msg.current, msg.expected);
            fflush(stdout);
          }
          
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsCancelPairing: {
          _rtsState = Raw;
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsAck: {
          //
          break;
        }
        default:
          break;
      }
    }
  }
  else if(_commVersion == 2) {
    if(extComms.GetTag() == Anki::Cozmo::ExternalComms::ExternalCommsTag::RtsConnection) {
      Anki::Cozmo::ExternalComms::RtsConnection rtsContainer = extComms.Get_RtsConnection();
      Anki::Cozmo::ExternalComms::RtsConnection_2 rtsMsg = rtsContainer.Get_RtsConnection_2();
      
      // Handle requests
      [self handleRequest:rtsMsg];
      
      switch(rtsMsg.GetTag()) {
        case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::Error:
          //
          break;
        case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsChallengeMessage: {
          Anki::Cozmo::ExternalComms::RtsChallengeMessage msg = rtsMsg.Get_RtsChallengeMessage();
          [self HandleChallengeMessage:msg];
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsChallengeSuccessMessage: {
          Anki::Cozmo::ExternalComms::RtsChallengeSuccessMessage msg = rtsMsg.Get_RtsChallengeSuccessMessage();
          [self HandleChallengeSuccessMessage:msg];
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsWifiConnectResponse: {
          Anki::Cozmo::ExternalComms::RtsWifiConnectResponse msg = rtsMsg.Get_RtsWifiConnectResponse();
          switch(msg.wifiState) {
            case 1:
              printf("Vector is connected to the internet.\n");
              break;
            case 0:
              printf("Unknown connection status.\n");
              break;
            case 2:
              printf("Vector is connected without internet.\n");
              break;
            case 3:
              printf("Vector is not connected to a network.\n");
              break;
            default:
              break;
          }
          
          if(_currentCommand == "wifi-connect" && !_readyForNextCommand) {
            _readyForNextCommand = true;
          }
          
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsWifiIpResponse: {
          Anki::Cozmo::ExternalComms::RtsWifiIpResponse msg = rtsMsg.Get_RtsWifiIpResponse();
          
          if(_currentCommand == "wifi-ip" && !_readyForNextCommand) {
            if(msg.hasIpV4) {
              char ipv4String[INET_ADDRSTRLEN] = {0};
              inet_ntop(AF_INET, msg.ipV4.data(), ipv4String, INET_ADDRSTRLEN);
              printf("IPv4: %s\n", ipv4String);
            }
            
            if(msg.hasIpV6) {
              char ipv6String[INET6_ADDRSTRLEN] = {0};
              inet_ntop(AF_INET6, msg.ipV6.data(), ipv6String, INET6_ADDRSTRLEN);
              printf("IPv6: %s\n", ipv6String);
            }
            
            _readyForNextCommand = true;
          } else if(_currentCommand == "ssh-start" && !_readyForNextCommand) {
            NSString* sshArg = [NSString stringWithFormat:@"root@%d.%d.%d.%d", msg.ipV4[0], msg.ipV4[1], msg.ipV4[2], msg.ipV4[3]];
            
            NSString *s = [NSString stringWithFormat:
                           @"tell application \"Terminal\" to do script \"ssh %@\"", sshArg];
            
            NSAppleScript *as = [[NSAppleScript alloc] initWithSource: s];
            [as executeAndReturnError:nil];
            
            _readyForNextCommand = true;
          }
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsStatusResponse_2: {
          //
          if(_currentCommand == "status" && !_readyForNextCommand) {
            Anki::Cozmo::ExternalComms::RtsStatusResponse_2 msg = rtsMsg.Get_RtsStatusResponse_2();
            
            std::string state = "";
            switch(msg.wifiState) {
              case 1:
                state = "ONLINE";
                break;
              case 0:
                state = "UNKNOWN";
                break;
              case 2:
                state = "CONNECTED / NO INTERNET";
                break;
              case 3:
                state = "DISCONNECTED";
                break;
              default:
                break;
            }
            
            printf("             ssid = %s\n connection_state = %s\n     access_point = %s\n          version = %s\n  ota_in_progress = %s\n", [self asciiStr:(char*)msg.wifiSsidHex.c_str() length:(int)msg.wifiSsidHex.length()].c_str(), state.c_str(), msg.accessPoint? "true" : "false", msg.version.c_str(), msg.otaInProgress? "true" : "false");
            _readyForNextCommand = true;
          }
          
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsWifiScanResponse_2: {
          Anki::Cozmo::ExternalComms::RtsWifiScanResponse_2 msg = rtsMsg.Get_RtsWifiScanResponse_2();
          [self HandleWifiScanResponse_2:msg];
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsWifiAccessPointResponse: {
          Anki::Cozmo::ExternalComms::RtsWifiAccessPointResponse msg = rtsMsg.Get_RtsWifiAccessPointResponse();
          [self HandleReceiveAccessPointResponse:msg];
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsOtaUpdateResponse: {
          Anki::Cozmo::ExternalComms::RtsOtaUpdateResponse msg = rtsMsg.Get_RtsOtaUpdateResponse();
          _otaStatusCode = msg.status;
          
          if(_otaStatusCode != 2 && _otaStatusCode != 1) {
            printf("\nota status code: %d\n", _otaStatusCode);
          }
          
          if(_otaStatusCode == 2) {
            _otaProgress = msg.current == 0? _otaProgress : msg.current;
          } else {
            _otaProgress = msg.current;
          }
          _otaExpected = msg.expected;
          
          /*
           * Commenting out for visibility because in next pass, going
           * to use this code again to show OTA progress bar.
           */
          if(_currentCommand == "ota-progress" && !_readyForNextCommand) {
            if(_otaStatusCode != 2) {
              _readyForNextCommand = true;
              _currentCommand = "";
            }
            
            int size = 100;
            int progress = (int)(((float)_otaProgress/(float)_otaExpected) * size);
            std::string bar = "";
           
            for(int i = 0; i < size; i++) {
              if(i <= progress) bar += "▓";
              else bar += "_";
            }
           
            printf("%100s [%d%%] [%llu/%llu] \r", bar.c_str(), progress, msg.current, msg.expected);
            fflush(stdout);
          }
          
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsLogResponse: {
          // Handle receive RtsLogResponse message
          Anki::Cozmo::ExternalComms::RtsLogResponse msg = rtsMsg.Get_RtsLogResponse();
          
          _currentFileId = msg.fileId;
          _currentFileBuffer.clear();
          
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsFileDownload: {
          // Handle receive RtsLogResponse message
          Anki::Cozmo::ExternalComms::RtsFileDownload msg = rtsMsg.Get_RtsFileDownload();
          
          if(msg.fileId != _currentFileId) {
            break;
          }
          
          // Add to buffer
          _currentFileBuffer.insert(_currentFileBuffer.end(), msg.fileChunk.begin(), msg.fileChunk.end());
          
          [self showProgress:(float)msg.packetNumber expected:(float)msg.packetTotal];
          
          if(msg.packetNumber == msg.packetTotal) {
            NSError *error = nil;
            
            
            NSData* data = [NSData dataWithBytes:_currentFileBuffer.data() length:_currentFileBuffer.size()];
            NSFileManager* fileManager = [NSFileManager defaultManager];
            
            NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
            [dateFormatter setDateFormat:@"yyyy-MM-dd-HH-mm-ss"];
            NSString* fileName = [NSString stringWithFormat:@"vic-logs-%@.tar.bz2", [dateFormatter stringFromDate:[NSDate date]]];
            
            NSArray* pathParts = [NSArray arrayWithObjects:_downloadFilePath, fileName, nil];
            NSString* dirPath = _downloadFilePath;
            NSString* logPath = [NSString pathWithComponents:pathParts];
            
            bool createdDirSuccess = [fileManager createDirectoryAtPath:dirPath withIntermediateDirectories:true attributes:nil error:nil];
            bool success = [data writeToFile:logPath options:NSDataWritingAtomic error:&error];
            
            if(!success || !createdDirSuccess) {
              printf("IO error while trying to write logs.\n");
            }
            
            if(_currentCommand == "logs" && !_readyForNextCommand) {
              printf("\nDownloaded logs to %s\n", [logPath UTF8String]);
              
              _readyForNextCommand = true;
            }
          }
          
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsCancelPairing: {
          _rtsState = Raw;
          break;
        }
        case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsAck: {
          //
          break;
        }
        default:
          break;
      }
    }
  }
}

- (void) handleRequest:(Anki::Cozmo::ExternalComms::RtsConnection_2)msg {
  dispatch_async(_requestQueue, ^(){
    RequestId rid = kUnknown;
    
    switch(msg.GetTag()) {
      case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsStatusResponse_2:
        rid = kStatus;
        break;
      case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsWifiScanResponse_2:
        rid = kWifiScan;
        break;
      case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsWifiConnectResponse:
        rid = kWifiConnect;
        break;
      case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsWifiIpResponse:
        rid = kWifiIp;
        break;
      case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsWifiAccessPointResponse:
        rid = kWifiAp;
        break;
      case Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsOtaUpdateResponse:
        rid = kOta;
        break;
      default:
        break;
    }
    
    if(_delegate != nullptr) {
      [_delegate handleResponse:rid message:msg];
    }
  });
}

- (void) SendSshPublicKey:(std::string)filename {
  NSFileManager* fileManager = [NSFileManager defaultManager];
  NSString* fn = [NSString stringWithUTF8String:filename.c_str()];
  
  if(![fn containsString:@".pub"]) {
    printf("WARNING! Supplied key does not look like a public key. Are you sure you want to send it to Vector? yes/no\n");
    char answer[3];
    scanf("%3s",answer);
    
    if(!(strncmp(answer, "yes", 3) == 0)) {
      return;
    }
  }
  
  if(![fileManager fileExistsAtPath:fn]) {
    // Generate Vector keys
    printf("Supplied public key does not exist.\n");
    return;
  }
  
  NSString* pubKey = [NSString stringWithContentsOfFile:fn encoding:NSUTF8StringEncoding error:nil];
  std::string contents = std::string([pubKey UTF8String], pubKey.length);
  
  std::vector<std::string> keyParts;
  for(uint32_t i = 0; i < contents.length(); i += 255) {
    keyParts.push_back(contents.substr(i, 255));
  }
  
  Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsSshRequest>(self, _commVersion, keyParts);
}

- (void) send:(const void*)bytes length:(int)n {
  if(_rtsState == CladSecure) {
    if(_verbose) NSLog(@"Sending ENCRYPTED message...");
    [self sendSecure:bytes length:n];
  } else {
    if(_verbose) NSLog(@"Sending message...");
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
  if(_verbose) NSLog(@"Received handshake");
  uint8_t* msg = (uint8_t*)bytes;
  
  if(n != 5) {
    return;
  }
  
  if(msg[0] != 1) {
    // Not Handshake message
    return;
  }
  
  uint32_t version = *(uint32_t*)(msg + 1);
  const uint32_t maxVersion = 2;
  
  if(version != 1 &&
     version != 2) {
    // Not Version 1 or Version 2
    printf("Error: Connected Vector speaks version %d, while our max version is %d!\n", version, maxVersion);
    return;
  } else {
    printf("* Speaking to Vector with protocol version [%d].\n", version);
  }
  
  // Set version
  _commVersion = version;
  
  [self send:bytes length:n];
  
  // Update state
  _rtsState = Clad;
}

- (void) HandleReceivePublicKey:(const Anki::Cozmo::ExternalComms::RtsConnRequest&)msg {
  if(_verbose) NSLog(@"Received public key from Victor");
  const void* bytes = (const void*)msg.publicKey.data();
  int n = (int)msg.publicKey.size();
  NSData* remoteKeyData = [NSData dataWithBytes:bytes length:n];
  NSMutableString* remoteKey = [[NSMutableString alloc] init];
  
  for(int i = 0; i < n; i++) {
    [remoteKey appendString:[NSString stringWithFormat:@"%x", ((uint8_t*)bytes)[i]]];
  }
  //[[NSString alloc] initWithData:remoteKeyData encoding:NSUTF8StringEncoding];
  if(_verbose) NSLog(@"Remote key data: %@", remoteKeyData);
  if(_verbose) NSLog(@"Remote key: %@", remoteKey);
  
  if([self HasSavedSession:remoteKey] && !([[_victorsDiscovered objectForKey:_peripheral.name] boolValue])) {
    std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> publicKeyArray;
    memcpy(std::begin(publicKeyArray), [[self GetPublicKey] bytes], crypto_kx_PUBLICKEYBYTES);
    
    NSArray* arr = [self GetSession:remoteKey];
    NSData* encKey = (NSData*)arr[0];
    NSData* decKey = (NSData*)arr[1];
    memcpy(_decryptKey, [decKey bytes], crypto_kx_SESSIONKEYBYTES);
    memcpy(_encryptKey, [encKey bytes], crypto_kx_SESSIONKEYBYTES);
    if(_verbose) NSLog(@"Trying to renew connection");
    _reconnection = true;
    Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsConnResponse>(self, _commVersion, Anki::Cozmo::ExternalComms::RtsConnType::Reconnection,
                                                                       publicKeyArray);
  } else {
    crypto_kx_keypair(_publicKey, _secretKey);
    
    memcpy(_remotePublicKey, msg.publicKey.data(), sizeof(_remotePublicKey));
  
    std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> publicKeyArray;
    memcpy(std::begin(publicKeyArray), _publicKey, crypto_kx_PUBLICKEYBYTES);
    
    int suc = crypto_kx_client_session_keys(_decryptKey, _encryptKey, _publicKey, _secretKey, _remotePublicKey);
    
    if(suc != 0) {
      if(_verbose) NSLog(@"Problem generated session keys. Try running mac-client.");
    }
    
    uint8_t tmpDecryptKey[crypto_kx_SESSIONKEYBYTES];
    memcpy(tmpDecryptKey, _decryptKey, crypto_kx_SESSIONKEYBYTES);
    uint8_t tmpEncryptKey[crypto_kx_SESSIONKEYBYTES];
    memcpy(tmpEncryptKey, _encryptKey, crypto_kx_SESSIONKEYBYTES);
  
    // Hash mix of pin and decryptKey to form new decryptKey
    
    if(!_isPairing) {
      printf("* Double tap backpack button to put Vector in pairing mode and restart mac-client.\n");
      exit(0);
    }
    
    Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsConnResponse>(self, _commVersion, Anki::Cozmo::ExternalComms::RtsConnType::FirstTimePair,
                                                                       publicKeyArray);
    
    char pin[6];
    char garbage[1];
    printf("> Enter pin:\n");
    scanf("%6s",pin);
    scanf("%c", garbage);
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
    if(_verbose) NSLog(@"Theoretically saving stuff.");
  }
}

- (void) HandleReceiveNonce:(const Anki::Cozmo::ExternalComms::RtsNonceMessage &)msg {
  if(_verbose) NSLog(@"Received nonce from Victor");
  memcpy(_nonceIn, msg.toDeviceNonce.data(), crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  memcpy(_nonceOut, msg.toRobotNonce.data(), crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
  
  if(_verbose) NSLog(@"Sending ack");
  
  uint8_t nonceTag = 0;
  
  if(_commVersion == 1) {
    nonceTag = (uint8_t)Anki::Cozmo::ExternalComms::RtsConnection_1Tag::RtsNonceMessage;
  } else {
    nonceTag = (uint8_t)Anki::Cozmo::ExternalComms::RtsConnection_2Tag::RtsNonceMessage;
  }
  
  Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsAck>(self, _commVersion, nonceTag);
  // Move to encrypted comms
  if(_verbose) NSLog(@"Setting mode to ENCRYPTED");
  _rtsState = CladSecure;
}

- (void) HandleChallengeMessage:(const Anki::Cozmo::ExternalComms::RtsChallengeMessage &)msg {
  if(_verbose) NSLog(@"Received challenge message from Victor: %d", msg.number);
  uint32_t challenge = msg.number;
  Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsChallengeMessage>(self, _commVersion, challenge + 1);
}

- (std::vector<std::string>) GetWordsFromLine: (std::string)line {
  std::vector<std::string> words;
  
  std::string w = "";
  
  for(int i = 0; i < line.length(); i++) {
    char c = line.c_str()[i];
    
    if(i == line.length() - 1) {
      w += line.c_str()[i];
      c = ' ';
    }
    
    if((c == ' ') && w != "") {
      words.push_back(w);
      w = "";
    } else {
      w += line.c_str()[i];
    }
  }
  
  return words;
}

- (void) async_WifiScanRequest {
  Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsWifiScanRequest>(self, _commVersion);
}

- (void) async_StatusRequest {
  Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsStatusRequest>(self, _commVersion);
}

- (void) async_WifiConnectRequest:(std::string)ssid password:(std::string)pw hidden:(bool)hid auth:(uint8_t)authType {
  uint8_t requestTimeout_s = 15;
  Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsWifiConnectRequest>(self, _commVersion,
                                                                           [self hexStr:(char*)ssid.c_str() length:(int)ssid.length()], pw, requestTimeout_s, authType, hid);
}

- (void) async_WifiIpRequest {
  Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsWifiIpRequest>(self, _commVersion);
}

- (void) async_WifiApRequest:(bool)enabled {
  Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsWifiAccessPointRequest>(self, _commVersion, enabled);
}

- (void) async_otaStart:(std::string)url {
  Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsOtaUpdateRequest>(self, _commVersion, url);
}

- (void) async_otaCancel {
  Clad::SendRtsMessage_2<Anki::Cozmo::ExternalComms::RtsOtaCancelRequest>(self, _commVersion);
}

- (void) async_otaProgress {
  //
}

- (void) HandleChallengeSuccessMessage:(const Anki::Cozmo::ExternalComms::RtsChallengeSuccessMessage&)msg {
  if(_verbose) [self printSuccess:"### Successfully Created Encrypted Channel ###"];
  
  if(_syncdelegate != nullptr) {
    [_syncdelegate performSelector:@selector(onFullyConnected)];
    return;
  }
  
  dispatch_async(_commandQueue, [self]{
    char* input;
    
    NSString* shellName = @"vector-????";
    if(_peripheral.name.length >= 4) {
      shellName = [_peripheral.name substringFromIndex:(_peripheral.name.length - 4)];
    }
    int vColor = [_colorArray[(_commVersion - 1) % _colorArray.count] intValue];
    
    char prompt[128];
    sprintf(prompt, "\033[0;%dmvector-%s#\033[0m ", vColor, [shellName UTF8String]);
    
    // Start shell
    while(true) {
      if(!_readyForNextCommand) {
        continue;
      }
      
      input = readline((const char*)prompt);
      
      if(strlen(input) > 0) {
        add_history(input);
      }
      
      std::vector<std::string> words = [self GetWordsFromLine:input];
      
      if(words.size() == 0) {
        continue;
      }
      
      _readyForNextCommand = false;
      _currentCommand = words[0];
      
      if(strcmp(words[0].c_str(), "wifi-scan") == 0) {
        [self async_WifiScanRequest];
      } else if(strcmp(words[0].c_str(), "wifi-connect") == 0) {
        if(words.size() < 3) {
          continue;
        }
        
        NSString* ssidS = [NSString stringWithUTF8String:words[1].c_str()];
        
        bool hidden = false;
        WiFiAuth auth = AUTH_WPA_PSK;
        
        NSString* ssidHex = [NSString stringWithUTF8String:[self hexStr:(char*)words[1].c_str() length:(int)ssidS.length].c_str()];
      
        if(![_wifiHidden containsObject:ssidHex] && [_wifiAuth valueForKey:ssidS] != nullptr) {
          auth = (WiFiAuth)[[_wifiAuth objectForKey:ssidS] intValue];
        } else {
          printf("[Hidden network]\n");
          hidden = true;
        }
        
        printf("Connecting to %s\n", words[1].c_str());
        
        uint8_t requestTimeout_s = 15;
        Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsWifiConnectRequest>(self, _commVersion,
                                                                                 [self hexStr:(char*)words[1].c_str() length:(int)words[1].length()], words[2], requestTimeout_s, auth, hidden);
        
      } else if(strcmp(words[0].c_str(), "wifi-ap") == 0) {
        bool enable = (words[1]=="true")?true:false;
        Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsWifiAccessPointRequest>(self, _commVersion, enable);
      } else if(strcmp(words[0].c_str(), "wifi-ip") == 0) {
        Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsWifiIpRequest>(self, _commVersion);
      } else if(strcmp(words[0].c_str(), "ota-start") == 0) {
        std::string url = "http://ota-cdn.anki.com/vic/dev/werih2382df23ij/full/latest.ota";
        if(words.size() > 1) {
          url = words[1];
        }
        Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsOtaUpdateRequest>(self, _commVersion, url);
        _readyForNextCommand = true;
        _currentCommand = "";
      } else if(strcmp(words[0].c_str(), "ota-cancel") == 0) {
        if(_commVersion < 2) {
          printf("ota-cancel not supported in version %d\n", _commVersion);
        } else {
          Clad::SendRtsMessage_2<Anki::Cozmo::ExternalComms::RtsOtaCancelRequest>(self, _commVersion);
        }
        _readyForNextCommand = true;
        _currentCommand = "";
      } else if(strcmp(words[0].c_str(), "ota-progress") == 0) {
        _readyForNextCommand = false;
        _currentCommand = "ota-progress";
      } else if(strcmp(words[0].c_str(), "status") == 0) {
        [self async_StatusRequest];
      } else if(strcmp(words[0].c_str(), "logs") == 0) {
        if(_commVersion < 2) {
          printf("logs not supported in version %d\n", _commVersion);
        } else {
          _readyForNextCommand = false;
          _currentCommand = "logs";
          
          NSArray* pathParts = [NSArray arrayWithObjects:NSHomeDirectory(), @"Downloads", @"mac-client-logs", nil];
          _downloadFilePath = [NSString pathWithComponents:pathParts];
          
          if(words.size() > 1) {
            _downloadFilePath = [NSString stringWithUTF8String:words[1].c_str()];
          }
          
          printf("Downloading log dump over BLE. This may take a couple minutes.\n");
          std::vector<std::string> filter;
          Clad::SendRtsMessage_2<Anki::Cozmo::ExternalComms::RtsLogRequest>(self, _commVersion, 0, filter);
        }
      } else if(strcmp(words[0].c_str(), "ssh-send") == 0) {
        NSArray* pathParts = [NSArray arrayWithObjects:NSHomeDirectory(), @".ssh", @"id_rsa_vic_dev.pub", nil];
        NSString* keyPath = [NSString pathWithComponents:pathParts];
        std::string filename = std::string(keyPath.UTF8String);
        
        if(words.size() > 1) {
          filename = words[1];
        }
        
        [self SendSshPublicKey:filename];
        _readyForNextCommand = true;
        _currentCommand = "";
      } else if(strcmp(words[0].c_str(), "ssh-start") == 0) {
        _readyForNextCommand = false;
        _currentCommand = "ssh-start";
        Clad::SendRtsMessage<Anki::Cozmo::ExternalComms::RtsWifiIpRequest>(self, _commVersion);
      } else if(strcmp(words[0].c_str(), "help") == 0) {
        _readyForNextCommand = true;
        _currentCommand = "";
        [self printHelp];
      } else if(!strcmp(words[0].c_str(), "exit")) {
        exit(0);
      } else {
        printf("Unrecognized command\n");
        _readyForNextCommand = true;
        _currentCommand = "";
        [self printHelp];
        // no command
      }
    }
  });
}

- (void) HandleWifiScanResponse:(const Anki::Cozmo::ExternalComms::RtsWifiScanResponse&)msg {
  printf("Wifi scan results...\n");
  
  if(msg.statusCode != 0) {
    printf("Error code: %d\n", msg.statusCode);
  }
  
  printf("Signal      Security      SSID\n");
  _wifiAuth = [[NSMutableDictionary alloc] init];
  _wifiHidden = [[NSMutableSet alloc] init];
  
  for(int i = 0; i < msg.scanResult.size(); i++) {
    std::string sec = "none";
    
    switch(msg.scanResult[i].authType) {
      case 0:
        sec = "none";
        break;
      case 1:
        sec = "WEP ";
        break;
      case 2:
        sec = "WEPS";
        break;
      case 3:
        sec = "IEEE";
        break;
      case 4:
        sec = "PSKo";
        break;
      case 5:
        sec = "EAPo";
        break;
      case 6:
        sec = "PSK ";
        break;
      case 7:
        sec = "EAP";
        break;
      default:
        break;
    }
    
    std::string ssidAscii = [self asciiStr:(char*)msg.scanResult[i].wifiSsidHex.c_str() length:(int)msg.scanResult[i].wifiSsidHex.length()];
    
    printf("%d           %s          %s\n", msg.scanResult[i].signalStrength, sec.c_str(), ssidAscii.c_str());
    
    NSString* ssidStr = [NSString stringWithUTF8String:ssidAscii.c_str()];
    [_wifiAuth setValue:[NSNumber numberWithInt:msg.scanResult[i].authType] forKey:ssidStr];
  }
  
  if(_currentCommand == "wifi-scan" && !_readyForNextCommand) {
    _readyForNextCommand = true;
  }
}

- (void) HandleWifiScanResponse_2:(const Anki::Cozmo::ExternalComms::RtsWifiScanResponse_2&)msg {
  if(_currentCommand == "wifi-scan") {
    printf("Wifi scan results...\n");
    
    if(msg.statusCode != 0) {
      printf("Error code: %d\n", msg.statusCode);
    }
    
    printf("Signal      Security      SSID\n");
    _wifiAuth = [[NSMutableDictionary alloc] init];
    _wifiHidden = [[NSMutableSet alloc] init];
    
    for(int i = 0; i < msg.scanResult.size(); i++) {
      std::string sec = "none";
      
      switch(msg.scanResult[i].authType) {
        case 0:
          sec = "none";
          break;
        case 1:
          sec = "WEP ";
          break;
        case 2:
          sec = "WEPS";
          break;
        case 3:
          sec = "IEEE";
          break;
        case 4:
          sec = "PSKo";
          break;
        case 5:
          sec = "EAPo";
          break;
        case 6:
          sec = "PSK ";
          break;
        case 7:
          sec = "EAP";
          break;
        default:
          break;
      }
      
      std::string ssidAscii = [self asciiStr:(char*)msg.scanResult[i].wifiSsidHex.c_str() length:(int)msg.scanResult[i].wifiSsidHex.length()];
      
      printf("%d           %s          %s %s\n", msg.scanResult[i].signalStrength, sec.c_str(), ssidAscii.c_str(), msg.scanResult[i].hidden? "H" : "_");
      
      NSString* ssidStr = [NSString stringWithUTF8String:ssidAscii.c_str()];
      [_wifiAuth setValue:[NSNumber numberWithInt:msg.scanResult[i].authType] forKey:ssidStr];
      
      if(msg.scanResult[i].hidden) {
        NSString* ssid = [NSString stringWithUTF8String:msg.scanResult[i].wifiSsidHex.c_str()];
        [_wifiHidden addObject:ssid];
      }
    }
  }
  
  if(_currentCommand == "wifi-scan" && !_readyForNextCommand) {
    _readyForNextCommand = true;
  }
}

- (void) HandleReceiveAccessPointResponse:(const Anki::Cozmo::ExternalComms::RtsWifiAccessPointResponse&)msg {
  
  if(_currentCommand == "wifi-ap" && !_readyForNextCommand) {
    NSLog(@"Access point enabled with SSID: [%s] PW: [%s]", msg.ssid.c_str(), msg.password.c_str());
    _readyForNextCommand = true;
  }
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
      if(_verbose) NSLog(@"Am I trying to subscribe to something?");
      [peripheral setNotifyValue:true forCharacteristic:characteristic];
    } else if ([characteristic.UUID.UUIDString isEqualToString:_writeSecureUuid.UUIDString]) {
      if(_verbose) NSLog(@"Am I trying to subscribe to something?");
      [peripheral setNotifyValue:true forCharacteristic:characteristic];
    }
    
    if(_verbose) NSLog(@"Did discover CHAR: %@.", characteristic.UUID.UUIDString);
  }
  
  if(_verbose) NSLog(@"Did discover characteristics.");
}

- (void)peripheral:(CBPeripheral *)peripheral
didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic
             error:(NSError *)error {
  if(error == nil) {
    if(_verbose) NSLog(@"We think we subscribed correctly");
  } else {
    if(_verbose) NSLog(@"error subbing");
  }
}

// ----------------------------------------------------------------------------------------------------------------

- (void)centralManager:(CBCentralManager *)central
 didDiscoverPeripheral:(CBPeripheral *)peripheral
     advertisementData:(NSDictionary<NSString *,id> *)advertisementData
                  RSSI:(NSNumber *)RSSI {
  NSData* data = advertisementData[CBAdvertisementDataManufacturerDataKey];
  
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
  
  _isPairing = isPairing;
  
  //NSLog(@"[%@] isPairing:%d knownName:%d isAnki:%d", peripheral.name, isPairing, knownName, isAnki);
  
  if(![_filter isEqualToString:@""] && ![peripheral.name containsString:_filter]) {
    return;
  }
  
  if((isAnki && isVictor && (isPairing || knownName)) && !_connecting) {
    printf("* Connecting to %s\n", peripheral.name.UTF8String);
    [_centralManager stopScan];
    [[NSUserDefaults standardUserDefaults] setValue:peripheral.name forKey:@"victorName"];
    _peripheral = peripheral;
    peripheral.delegate = self;
    [_centralManager connectPeripheral:peripheral options:nullptr];
    _connecting = true;
  } else if(!_connecting) {
    if(_verbose) NSLog(@"Ignoring %@", peripheral.name);
  }
}

- (void)centralManager:(CBCentralManager *)central
didFailToConnectPeripheral:(CBPeripheral *)peripheral
                 error:(NSError *)error {
  _connecting = false;
  NSLog(@"Failed to connect.");
}

- (void)centralManager:(CBCentralManager *)central
  didConnectPeripheral:(CBPeripheral *)peripheral {
  if(_verbose) NSLog(@"Connected to peripheral");
  [self StopScanning];
  [peripheral discoverServices:@[_victorService]];
}

- (void)centralManager:(CBCentralManager *)central
didDisconnectPeripheral:(CBPeripheral *)peripheral
                 error:(NSError *)error {
  printf("\n* Disconnected\n");
  exit(0);
}

- (void)centralManagerDidUpdateState:(nonnull CBCentralManager *)central {
  switch(central.state) {
    case CBCentralManagerStatePoweredOn:
      if(_verbose) NSLog(@"Powered On BleCentral");
      [_centralManager
       scanForPeripheralsWithServices:@[_victorService]
       options:@{ CBCentralManagerScanOptionAllowDuplicatesKey: @true }];
    default:
      break;
  }
}

@end
