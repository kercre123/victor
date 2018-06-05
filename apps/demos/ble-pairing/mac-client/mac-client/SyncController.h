//
//  SyncController.h
//  mac-client
//
//  Created by Paul Aluri on 5/10/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#include "Requests.h"

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#ifndef SyncController_h
#define SyncController_h

@protocol BleCentralDelegate

-(void) onFullyConnected;

@end

@interface SyncController : NSObject<BleCentralDelegate> {
  dispatch_queue_t _syncQueue;
  std::string _command;
  std::string _arg;
  
  std::string _lastSSID;
  std::string _lastPW;
  std::string _otaUrl;
}

-(void) processCommands:(bool)doExit;
-(void) setCommand:(std::string)s;
-(void) setArg:(std::string)s;
-(void) setOtaUrl:(std::string)url;
-(void) downloadOta:(std::string)url;
-(void) moveOta:(std::string)path;
-(void) macConnectVictorWifi;
-(std::string) getIPAddress;

-(std::vector<std::string>) splitString:(std::string)s delimit:(char)c;
-(std::string)trim:(const std::string&)str;

@property (strong, nonatomic) Requests* requests;
@property (nonatomic) bool useLocal;

@end

#endif /* SyncController_h */
