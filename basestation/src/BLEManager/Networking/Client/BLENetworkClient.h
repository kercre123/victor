//
//  BLENetworkClient.h
//  BLEManager
//
//  Created by Mark Pauley on 3/27/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "BLEManager/Networking/BLENetworkInterface.h"

#define BLENetworkClientErrorDomain @"BLENetworkClientErrorDomain"

typedef enum _BLENetworkClientError {
  BLENetworkClientErrorPacketTooBig = -2,
  BLENetworkClientErrorNoSuchEndpoint = -1,
  BLENetworkClientErrorNone = 0
} BLENetworkClientError;

@class CBPeripheralManager;

@interface BLENetworkClient : NSObject <BLENetworkInterface>

@property (nonatomic,strong) CBPeripheralManager* peripheralManager;
@property (nonatomic,readonly) NSString *endpointID;

+(CBPeripheralManager*)sharedPeripheralManager;
-(id)initWithClientID:(NSString *)clientID peripheralManager:(CBPeripheralManager*)peripheralManager;

@end
