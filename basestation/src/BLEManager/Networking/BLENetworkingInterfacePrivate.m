//
//  BLENetworkingInterfacePrivate.c
//  BLEManager
//
//  Created by Mark Pauley on 8/30/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#import <stdio.h>
#import "BLEManager/Networking/BLENetworkingInterfacePrivate.h"

#define BLENetworkDisconnectMessageLength 1
const char BLENetworkDisconnectMessageBytes[BLENetworkDisconnectMessageLength] = {0xff};



NSData* _BLENetworkingInterfaceDisconnectMessage() {
  static NSData* disconnectData = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    disconnectData = [NSData dataWithBytes:BLENetworkDisconnectMessageBytes length:BLENetworkDisconnectMessageLength];
  });
  return disconnectData;
}
