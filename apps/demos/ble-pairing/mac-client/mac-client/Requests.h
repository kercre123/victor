//
//  NSObject+Requests.h
//  mac-client
//
//  Created by Paul Aluri on 5/10/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "BleCentral.h"
#include "messageExternalComms.h"
#include "RequestsDelegate.h"

#ifndef Requests_h
#define Requests_h

typedef NS_ENUM(uint8_t, RequestStatus) {
  kStatusUnknown,
  kSuccess,
  kFailure,
  kTimeout,
};

@interface Requests : NSObject<RequestDelegate> {
  BleCentral* _central;
  RequestId _currentRequest;
  dispatch_semaphore_t _responseSemaphore;
  Anki::Victor::ExternalComms::RtsConnection_2 _currentMessage;
}

@property (nonatomic) RequestStatus success;

-(id) initWithCentral:(BleCentral*)central;

- (void)showProgress: (float)current expected:(float)expected;
-(dispatch_time_t) getTimeout;
-(Anki::Victor::ExternalComms::RtsStatusResponse_2) getStatus;
-(Anki::Victor::ExternalComms::RtsWifiScanResponse_2) getWifiScan;
-(Anki::Victor::ExternalComms::RtsWifiConnectResponse) doWifiConnect:(std::string)ssid password:(std::string)pw hidden:(bool)hidden auth:(uint8_t)auth;
-(Anki::Victor::ExternalComms::RtsWifiIpResponse) getWifiIp;
-(Anki::Victor::ExternalComms::RtsWifiAccessPointResponse) doWifiAp:(bool)enabled;
-(Anki::Victor::ExternalComms::RtsOtaUpdateResponse) otaStart:(std::string)url;
-(Anki::Victor::ExternalComms::RtsOtaUpdateResponse) otaCancel;
-(Anki::Victor::ExternalComms::RtsOtaUpdateResponse) otaProgress;

@end

#endif
