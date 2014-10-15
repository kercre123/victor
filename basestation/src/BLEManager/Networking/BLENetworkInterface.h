//
//  BLENetworkInterface.h
//  BLEManager
//
//  Created by Mark Pauley on 3/27/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol BLENetworkInterface;

@protocol BLENetworkInterfaceDelegate <NSObject>

-(void)BLENetworkInterface:(id<BLENetworkInterface>)interface
        endpointDiscovered:(NSString*)endpointID;

-(void)BLENetworkInterface:(id<BLENetworkInterface>)interface
       endpointDisappeared:(NSString*)endpointID; // error?

-(void)BLENetworkInterface:(id<BLENetworkInterface>)interface
         endpointConnected:(NSString*)endpointID;

-(void)BLENetworkInterface:(id<BLENetworkInterface>)interface
      endpointDisconnected:(NSString*)endpointID; // error?

-(void)BLENetworkInterface:(id<BLENetworkInterface>)interface
              receivedData:(NSData*)data
              fromEndpoint:(NSString*)endpointID;

// do we need an 'endpoint ready to send receive data' callback?

@end

typedef enum _BLENetworkEndpointState {
  BLENetworkEndpointStateUnavailable = 0,
  BLENetworkEndpointStateAvailable,
  BLENetworkEndpointStateConnecting,
  BLENetworkEndpointStateConnected,
  BLENetworkEndpointStateDisconnecting,
  BLENetworkEndpointStateWaitingForCBDisconnect,
  BLENetworkEndpointStateLength
} BLENetworkEndpointState;


@protocol BLENetworkInterface <NSObject>

// Generally not used directly by clients. (is clientID even used?)
-(id)initWithClientID:(NSString*)clientID;

// Forces disconnect of everything, better to do this on purpose rather than relying on dealloc.
-(void)stop;

// This should be one of the advertised endpoints
-(void)connectToEndpointID:(NSString*)endpointID;

// This should be one of the connected or connecting endpoints
-(void)disconnectFromEndpointID:(NSString*)endpointID;

-(NSString*)advertisedNameForEndpointID:(NSString*)endpointID;

// Send to a connected endpoint
// This is not a streaming API, it will fail and barf if you send more than 20 bytes.
// The return value likely isn't very important (we keep a buffer in the implementations).
-(BOOL)sendMessage:(NSData*)message toEndpoint:(NSString*)endpointID error:(NSError**)error;

-(BOOL)sendReliableMessage:(NSData*)message toEndpoint:(NSString*)endpointID error:(NSError**)error;

// IOS 6 BLE CONNECTION TIMEOUT BUG
//  Call this method when you have received a connectionTimeout.
-(void)receivedHandshakeTimeout;

@property (readonly, nonatomic) NSString* clientID; // advertised name for device.

@property (readonly, nonatomic) NSArray* advertisedEndpoints; // list of Endpoint ID's
@property (readonly, nonatomic) NSArray* connectingEndpoints; 
@property (readonly, nonatomic) NSArray* connectedEndpoints;  

@property (assign, nonatomic, getter = isAvailable) BOOL available;

@property (weak, nonatomic) id<BLENetworkInterfaceDelegate> delegate;

@end


// These are the interface service ID and characteristic ID's
#define BLENetworkInterfaceService            "1F687216-B8BF-4D61-B99A-8FBE8D5E6C98"
#define BLENetworkInterfaceCharacteristic_C2S "30619F2D-0F54-41BD-A65A-7588D8C85B45"
#define BLENetworkInterfaceCharacteristic_S2C "7D2A4BDA-D29B-4152-B725-2491478C5CD7"
#define BLENetworkInterfaceCharacteristic_Disconnect "68F0FD05-B0D4-495A-8FD8-130179A137C0"
#define BLENetworkInterfaceServiceName        "BLENetworkInterface"

#define BLENetworkInterfaceMaxMessageLength 20

// IOS 6 BLE CONNECTION TIMEOUT BUG
// Subscribe to this notification to find out when we believe the client device may require a reboot.
#define BLENetworkInterfaceIsHosedNotification @"BLENetworkInterfaceIsHosedNotification"


