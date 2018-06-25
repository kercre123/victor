
//
//  NSObject+Requests.m
//  mac-client
//
//  Created by Paul Aluri on 5/10/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import "Requests.h"

@implementation Requests

-(id) initWithCentral:(BleCentral*)central {
  _central = central;
  _responseSemaphore = dispatch_semaphore_create(0);
  _currentRequest = kUnknown;
  _success = kStatusUnknown;
  
  return [self init];
}

-(dispatch_time_t) getTimeout {
  return dispatch_time(DISPATCH_TIME_NOW, (int64_t)(30 * NSEC_PER_SEC));
}

-(void) handleResponse:(RequestId)requestId message:(Anki::Cozmo::ExternalComms::RtsConnection_2)msg {
  if(requestId != kUnknown && _currentRequest == requestId) {
    _currentMessage = msg;
    
    // handle incoming message
    // note: this is a switch in case different
    // request need to do something before signaling
    
    switch(requestId) {
      case kStatus:
      case kWifiScan:
      case kWifiConnect:
      case kWifiIp:
      case kWifiAp:
      case kOta:
        dispatch_semaphore_signal(_responseSemaphore);
        break;
      default:
        break;
    }
  }
}

-(Anki::Cozmo::ExternalComms::RtsStatusResponse_2) getStatus {
  _currentRequest = kStatus;
  [_central async_StatusRequest];
  
  Anki::Cozmo::ExternalComms::RtsStatusResponse_2 res;
  if(dispatch_semaphore_wait(_responseSemaphore, [self getTimeout]) == 0) {
    res = _currentMessage.Get_RtsStatusResponse_2();
    _success = kSuccess;
  } else {
    _success = kTimeout;
  }
  
  return res;
}

-(Anki::Cozmo::ExternalComms::RtsWifiScanResponse_2) getWifiScan {
  _currentRequest = kWifiScan;
  [_central async_WifiScanRequest];
  
  Anki::Cozmo::ExternalComms::RtsWifiScanResponse_2 res;
  if(dispatch_semaphore_wait(_responseSemaphore, [self getTimeout]) == 0) {
    res = _currentMessage.Get_RtsWifiScanResponse_2();
    _success = res.statusCode == 0? kSuccess : kFailure;
  } else {
    _success = kTimeout;
  }
  
  return res;
}

-(Anki::Cozmo::ExternalComms::RtsWifiConnectResponse) doWifiConnect:(std::string)ssid password:(std::string)pw hidden:(bool)hidden auth:(uint8_t)auth {
  _currentRequest = kWifiConnect;
  [_central async_WifiConnectRequest:ssid password:pw hidden:hidden auth:auth];
  
  Anki::Cozmo::ExternalComms::RtsWifiConnectResponse res;
  if(dispatch_semaphore_wait(_responseSemaphore, [self getTimeout]) == 0) {
    res = _currentMessage.Get_RtsWifiConnectResponse();
    _success = (res.wifiState == 1 || res.wifiState == 2)? kSuccess : kFailure;
  } else {
    _success = kTimeout;
  }
  
  return res;
}

-(Anki::Cozmo::ExternalComms::RtsWifiIpResponse) getWifiIp {
  _currentRequest = kWifiIp;
  [_central async_WifiIpRequest];
  
  Anki::Cozmo::ExternalComms::RtsWifiIpResponse res;
  if(dispatch_semaphore_wait(_responseSemaphore, [self getTimeout]) == 0) {
    res = _currentMessage.Get_RtsWifiIpResponse();
    _success = res.hasIpV4 || res.hasIpV6? kSuccess : kFailure;
  } else {
    _success = kTimeout;
  }
  
  return res;
}

-(Anki::Cozmo::ExternalComms::RtsWifiAccessPointResponse) doWifiAp:(bool)enabled {
  _currentRequest = kWifiAp;
  [_central async_WifiApRequest:enabled];
  
  Anki::Cozmo::ExternalComms::RtsWifiAccessPointResponse res;
  if(dispatch_semaphore_wait(_responseSemaphore, [self getTimeout]) == 0) {
    res = _currentMessage.Get_RtsWifiAccessPointResponse();
    _success = res.enabled == enabled? kSuccess : kFailure;
  } else {
    _success = kTimeout;
  }
  
  return res;
}

-(Anki::Cozmo::ExternalComms::RtsOtaUpdateResponse) otaStart:(std::string)url {
  _currentRequest = kOta;
  [_central async_otaStart:url];
  
  Anki::Cozmo::ExternalComms::RtsOtaUpdateResponse res;
  if(dispatch_semaphore_wait(_responseSemaphore, [self getTimeout]) == 0) {
    res = _currentMessage.Get_RtsOtaUpdateResponse();
    _success = res.status == 3? kSuccess : kFailure;
  } else {
    _success = kTimeout;
  }
  
  return res;
}

-(Anki::Cozmo::ExternalComms::RtsOtaUpdateResponse) otaCancel {
  _currentRequest = kOta;
  [_central async_otaCancel];
  
  Anki::Cozmo::ExternalComms::RtsOtaUpdateResponse res;
  if(dispatch_semaphore_wait(_responseSemaphore, [self getTimeout]) == 0) {
    res = _currentMessage.Get_RtsOtaUpdateResponse();
    _success = res.status == 5? kSuccess : kFailure;
  } else {
    _success = kTimeout;
  }
  
  return res;
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

-(Anki::Cozmo::ExternalComms::RtsOtaUpdateResponse) otaProgress {
  _currentRequest = kOta;
  [_central async_otaProgress];
  
  Anki::Cozmo::ExternalComms::RtsOtaUpdateResponse res;
  
  long status = 0;
  bool earlyExit = false;
  while(true) {
    status = dispatch_semaphore_wait(_responseSemaphore, [self getTimeout]);
    
    if(status != 0) {
      printf("Got timeout\n");
      exit(-1);
    } else {
      res = _currentMessage.Get_RtsOtaUpdateResponse();
      
      if(res.status == 3) {
        _success = kSuccess;
        break;
      } else if(res.status == 5) {
        _success = kFailure;
        break;
      } else if(res.status == 2) {
        if(res.expected != 0) {
          [self showProgress:(float)res.current expected:(float)res.expected];
        }
      } else if(res.status >= 6) {
        // update-engine exit
        _success = kFailure;
        earlyExit = true;
        break;
      }
    }
  }
  
  if(earlyExit || dispatch_semaphore_wait(_responseSemaphore, [self getTimeout]) == 0) {
    res = _currentMessage.Get_RtsOtaUpdateResponse();
    _success = res.status == 3? kSuccess : kFailure;
  } else {
    _success = kTimeout;
  }
  
  return res;
}

@end
