//
//  RequestsDelegate.h
//  mac-client
//
//  Created by Paul Aluri on 5/10/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef RequestsDelegate_h
#define RequestsDelegate_h

typedef NS_ENUM(NSUInteger, RequestId) {
  kUnknown,
  kStatus,
  kWifiScan,
  kWifiConnect,
  kWifiIp,
  kWifiAp,
  kOta,
  kOtaStart,
  kOtaCancel,
  kOtaProgress
};

@protocol RequestDelegate

-(void) handleResponse:(RequestId)requestId message:(Anki::Cozmo::ExternalComms::RtsConnection_2)msg;

@end

#endif /* RequestsDelegate_h */
