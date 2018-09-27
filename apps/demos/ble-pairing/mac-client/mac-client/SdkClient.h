//
//  SdkClient.h
//  mac-client
//
//  Created by Paul Aluri on 9/21/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef SdkClient_h
#define SdkClient_h

@interface SdkClient : NSObject<NSURLSessionDelegate> {
  NSString* _ip;
  NSString* _esn;
  NSString* _clientAppToken;
  dispatch_semaphore_t _responseSemaphore;
}

-(id) initWithEsn:(NSString*)_esn ipAddr:(NSString*)_ipAddr clientAppToken:(NSString*)_clientAppToken;
-(NSData*) getDerDataFromPem:(NSString*)contents;
-(dispatch_time_t) getTimeout;

@end

#endif /* SdkClient_h */
