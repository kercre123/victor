//
//  BLENetworkServer.h
//  BLEManager
//
//  Created by Mark Pauley on 3/28/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "BLEManager/Netowrking/BLENetworkInterface.h"

#define BLENetworkServerErrorDomain @"BLENetworkServerErrorDomain"

typedef enum _BLENetworkServerError {
  BLENetworkServerErrorEndpointNotConnected = -3,
  BLENetworkServerErrorMessageTooBig = -2,
  BLENetworkServerErrorUnknownEndpoint = -1,
  BLENetworkServerErrorNone = 0
} BLENetworkServerError;

@class BLECentralMultiplexer;
@class BLECentralServiceDescription;
@interface BLENetworkServer : NSObject <BLENetworkInterface>

@property (nonatomic,strong) BLECentralMultiplexer* centralMultiplexer;
@property (nonatomic,readonly) BLECentralServiceDescription* serviceDescription;

@end
