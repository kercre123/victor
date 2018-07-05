//
//  SyncController.m
//  mac-client
//
//  Created by Paul Aluri on 5/10/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "SyncController.h"
#include <CoreWLAN/CoreWLAN.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

@implementation SyncController

-(void) onFullyConnected {
  _syncQueue = dispatch_queue_create("syncQueue", NULL);
  
  dispatch_async(_syncQueue, ^() {
    if(_command == "ota-update") {
      Anki::Victor::ExternalComms::RtsStatusResponse_2 status = [_requests getStatus];
      bool online = status.wifiState == 1 || status.wifiState == 2;
      
      if(!online) {
        Anki::Victor::ExternalComms::RtsWifiScanResponse_2 scan = [_requests getWifiScan];
        Anki::Victor::ExternalComms::RtsWifiConnectResponse connect = [_requests doWifiConnect:"AnkiRobits" password:"KlaatuBaradaNikto!" hidden:false auth:6];
      }
      
      if(_requests.success == kSuccess) {
        (void)[_requests otaStart:_arg];
      }
  
      (void)[_requests otaProgress];
      
      exit(0);
      return;
    } else if(_command == "ota-update-ap") {
      _command = "wifi-ap:true; mac-vec-wifi";
      
      // download and serve
      [self downloadOta:_arg];
      
      [self processCommands:false];
      
      // wait a couple seconds
      sleep(12);
      
      std::string ip = [self getIPAddress];
      
      if(ip == "error") {
        printf("Fatal: Could not connect to Vector's wifi AP\n");
        exit(0);
      }
      
      std::string updateUrl = "http://" + [self getIPAddress] + ":8000/downloaded.ota";
      
      printf("Starting OTA with url: %s\n", updateUrl.c_str());
      if(_requests.success == kSuccess) {
        (void)[_requests otaStart:updateUrl];
      }
      
      (void)[_requests otaProgress];
      
      exit(0);
      return;
    }
  });
  
  dispatch_async(_syncQueue, ^(){
    [self processCommands:true];
  });
}

-(void) macConnectVictorWifi {
  NSString* matchingSsid = [NSString stringWithUTF8String:_lastSSID.c_str()];
  NSString* pw = [NSString stringWithUTF8String:_lastPW.c_str()];
  
  printf("Searching for SSID [%s]...\n", matchingSsid.UTF8String);
  
  CWInterface* cwi = [[CWInterface alloc] initWithInterfaceName:@"en0"];
  
  // give Victor some time to spin up ap mode
  sleep(3);
  
  NSSet *scanResults = [cwi scanForNetworksWithSSID:nil error:nil];
  
  CWNetwork* best = nil;
  int bestRssi = -1000;
  
  for(CWNetwork* network in scanResults) {
    if([network.ssid isEqualToString:matchingSsid] && (int)network.rssiValue > bestRssi) {
      bestRssi = (int)network.rssiValue;
      best = network;
    }
  }
  
  if(best == nil) {
    // Couldn't find network
    _requests.success = kFailure;
    printf("Vector AP not found...\n");
  } else {
    NSError* err;
    
    bool wifiConnect = [cwi associateToNetwork:best password:pw error:&err];
    if(!wifiConnect) {
      NSLog(err.localizedDescription);
    }
    _requests.success = wifiConnect? kSuccess : kFailure;
  }
}

-(std::string) getIPAddress {
  std::string address = "error";
  struct ifaddrs *interfaces = NULL;
  struct ifaddrs *temp_addr = NULL;
  
  if (getifaddrs(&interfaces) == 0) {
    temp_addr = interfaces;
    while(temp_addr != NULL) {
      if(temp_addr->ifa_addr->sa_family == AF_INET) {
        std::string p = std::string(inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr));
        if([[NSString stringWithUTF8String:temp_addr->ifa_name] isEqualToString:@"en0"]) {
          address = std::string(inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr));
        }
      }
      temp_addr = temp_addr->ifa_next;
    }
  }

  freeifaddrs(interfaces);
  return address;
}

-(void) downloadOta:(std::string)url {
  NSFileManager* fileManager = [NSFileManager defaultManager];
  
  NSArray* pathParts = [NSArray arrayWithObjects:NSHomeDirectory(), @"Downloads", @"mac-client-ota", nil];
  NSString* otaName = @"downloaded.ota";
  NSString* dirPath = [NSString pathWithComponents:pathParts];
  NSString* filePath = [NSString pathWithComponents:@[dirPath, otaName]];
  
  (void)[fileManager createDirectoryAtPath:dirPath withIntermediateDirectories:true attributes:nil error:nil];
  
  std::string curlCmd = "curl " + url + " --output " + std::string(filePath.UTF8String);
  system(curlCmd.c_str());
  
  NSString *s = [NSString stringWithFormat: @"tell application \"Terminal\" to do script \"pushd %@; python -m SimpleHTTPServer; popd\"", dirPath];
  
  NSAppleScript *as = [[NSAppleScript alloc] initWithSource: s];
  [as executeAndReturnError:nil];
}

-(void) processCommands:(bool)doExit {
  RequestStatus status = kStatusUnknown;
  bool requiresSuccess = false;
  
  // parse commands
  std::vector<std::string> commands = [self splitString:_command delimit:';'];
  
  for(int i = 0; i < commands.size(); i++) {
    std::vector<std::string> cmdline = [self splitString:commands[i] delimit:':'];
    std::string cmd = [self trim:cmdline[0]];
    
    if(cmd.at(cmd.length() - 1) == '+') {
      requiresSuccess = true;
      cmd = cmd.substr(0, cmd.length() - 1);
    }
    
    if(cmd == "status") {
      Anki::Victor::ExternalComms::RtsStatusResponse_2 status = [_requests getStatus];
      printf("Got status: %s\n", status.version.c_str());
    } else if(cmd == "wifi-scan") {
      Anki::Victor::ExternalComms::RtsWifiScanResponse_2 scan = [_requests getWifiScan];
      printf("Got scan: %lu\n", scan.scanResult.size());
    } else if(cmd == "wifi-ip") {
      Anki::Victor::ExternalComms::RtsWifiIpResponse ip = [_requests getWifiIp];
      if(ip.hasIpV4) {
        char ipv4String[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, ip.ipV4.data(), ipv4String, INET_ADDRSTRLEN);
        printf("IPv4: %s\n", ipv4String);
      }
    } else if(cmd == "@robits") {
      Anki::Victor::ExternalComms::RtsWifiConnectResponse connect = [_requests doWifiConnect:"AnkiRobits" password:"KlaatuBaradaNikto!" hidden:false auth:6];
      printf("Got connect: %s %d\n", connect.wifiSsidHex.c_str(), connect.wifiState);
    }  else if(cmd == "wifi-connect") {
      if(cmdline.size() < 2) {
        continue;
      }
      
      std::vector<std::string> args = [self splitString:cmdline[1] delimit:'|'];
      
      std::string ssid = args[0];
      std::string pw = "";
      bool hid = false;
      uint8_t auth = 0;
      
      if(args.size() > 1) {
        pw = args[1];
      }
      
      if(args.size() > 2) {
        auth = (uint8_t)std::stoi([self trim:args[2]]);
      }
      
      if(args.size() > 3) {
        hid = [self trim:args[3]] == "hidden" || [self trim:args[3]] == "true";
      }
      
      Anki::Victor::ExternalComms::RtsWifiConnectResponse connect = [_requests doWifiConnect:ssid password:pw hidden:hid auth:auth];
      printf("Got connect: %s %d\n", connect.wifiSsidHex.c_str(), connect.wifiState);
    } else if(cmd == "wifi-ap") {
      if(cmdline.size() < 2) {
        continue;
      }
      
      std::vector<std::string> args = [self splitString:cmdline[1] delimit:'|'];
      
      if(args.size() > 1) {
        continue;
      }
      
      if(args[0] == "true") {
        Anki::Victor::ExternalComms::RtsWifiAccessPointResponse ap = [_requests doWifiAp:true];
        printf("Access point enabled: %d\n", ap.enabled);
        _lastSSID = ap.ssid;
        _lastPW = ap.password;
      } else if(args[0] == "false") {
        Anki::Victor::ExternalComms::RtsWifiAccessPointResponse ap = [_requests doWifiAp:false];
        printf("Access point enabled: %d\n", ap.enabled);
      }
    } else if(cmd == "ota-start") {
      if(cmdline.size() < 2) {
        continue;
      }
      
      std::vector<std::string> args = [self splitString:cmdline[1] delimit:'|'];
      
      if(args.size() > 1) {
        continue;
      }
      
      Anki::Victor::ExternalComms::RtsOtaUpdateResponse ota = [_requests otaStart:[self trim:args[0]]];
      printf("Started ota-update with code: %d\n", ota.status);
    } else if(cmd == "ota-cancel") {
      if(cmdline.size() > 1) {
        continue;
      }
      
      Anki::Victor::ExternalComms::RtsOtaUpdateResponse ota = [_requests otaCancel];
      printf("Cancelled ota with code: %d\n", ota.status);
    } else if(cmd == "ota-progress") {
      if(cmdline.size() > 1) {
        continue;
      }
      
      Anki::Victor::ExternalComms::RtsOtaUpdateResponse ota = [_requests otaProgress];
      printf("Finished ota with code: %d\n", ota.status);
    } else if(cmd == "mac-vec-wifi") {
      [self macConnectVictorWifi];
    }
    
    status = _requests.success;
    
    if(requiresSuccess && status != kSuccess) {
      printf("Command failed.\n");
      break;
    }
    
    requiresSuccess = false;
  }
  
  if(doExit) exit(0);
}

-(void) setCommand:(std::string)s {
  _command = s;
}

-(void) setArg:(std::string)s {
  _arg = s;
}

-(void) setOtaUrl:(std::string)url {
  _otaUrl = url;
}

-(std::vector<std::string>) splitString:(std::string)s delimit:(char)c {
  std::vector<std::string> ret;
  std::istringstream f(s);
  std::string w;
  while (std::getline(f, w, c)) {
    ret.push_back([self trim:w]);
  }
  
  return ret;
}

-(std::string)trim:(const std::string&)str {
  size_t first = str.find_first_not_of(' ');
  if (std::string::npos == first) {
    return str;
  }
  size_t last = str.find_last_not_of(' ');
  return str.substr(first, (last - first + 1));
}

@end
