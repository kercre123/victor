//
//  BLENetworkServer.m
//  BLEManager
//
//  Created by Mark Pauley on 3/28/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#import "BLEManager/Networking/Server/BLENetworkServer.h"
#import "BLEManager/Central/BLECentralMultiplexer.h"
#import "BLEManager/Central/BLECentralServiceDescription.h"
#import "BLEManager/Logging/BLELog.h"
#import "BLEManager/Networking/BLENetworkingInterfacePrivate.h"
#import <limits.h>

static CBUUID* NetworkServiceID;
static CBUUID* NetworkClientToServerCharacteristic;
static CBUUID* NetworkServerToClientCharacteristic;
static CBUUID* NetworkDisconnectCharacteristic; // when client wishes to disconnect.

#define kBLEServerCBCentralManagerUnbustedVersion 10.0

#define kBLEServerInflightBatchLength 1 // send batches at a time
static inline int _BLEServerInflightBatchLength() {
//  if([[UIDevice currentDevice] systemVersion].doubleValue < kBLEServerCBCentralManagerUnbustedVersion) {
//    return kBLEServerInflightBatchLength;
//  }
//  return INT_MAX;
  return kBLEServerInflightBatchLength;
}

#define kBLEServerThrottlePeriodMSecs 30 // period for throttling batch
static inline int _BLEServerThrottlePeriodMSecs() {
//  if([[UIDevice currentDevice] systemVersion].doubleValue < kBLEServerCBCentralManagerUnbustedVersion) {
//    return kBLEServerThrottlePeriodMSecs;
//  }
//  return 0;
  return kBLEServerThrottlePeriodMSecs;
}


// Endpoint
@interface BLENetworkServer_Endpoint : NSObject
@property (nonatomic,strong) CBPeripheral* peripheral;
@property (nonatomic,strong) CBCharacteristic* clientToServerCharacteristic;
@property (nonatomic,strong) CBCharacteristic* serverToClientCharacteristic;
@property (nonatomic,strong) CBCharacteristic* disconnectCharacteristic;
@property (nonatomic,strong) NSString* clientID;
@property (nonatomic,strong) NSString* advertisedName;
@property (nonatomic,assign) NSUInteger inflightReliableMessages;
@property (nonatomic,assign) BOOL canWrite;
@property (nonatomic,strong) NSMutableArray* enqueuedReliableMessages;
@property (nonatomic,assign) BLENetworkEndpointState state;
@end

@implementation BLENetworkServer_Endpoint
@synthesize peripheral;
@synthesize clientToServerCharacteristic;
@synthesize serverToClientCharacteristic;
@synthesize disconnectCharacteristic;
@synthesize clientID;
@synthesize advertisedName;
@synthesize inflightReliableMessages;
@synthesize canWrite;
@synthesize enqueuedReliableMessages;
@synthesize state;
@end

// end Endpoint


@interface BLENetworkServer () <BLECentralServiceDelegate>

@end

@implementation BLENetworkServer {
  NSMutableDictionary* _availableEndpointsByClientID;
  NSMutableDictionary* _connectedEndpointsByClientID;
  NSMutableDictionary* _connectingEndpointsByClientID;
  
  BLECentralServiceDescription* _serviceDescription;
}

@synthesize clientID = _clientID;
@synthesize available = _available;
@synthesize centralMultiplexer = _centralMultiplexer;
@synthesize delegate = _delegate;

+(void)initServiceAndCharacteristicIDs {
  static dispatch_once_t NetworkServiceOnce = 0;
  dispatch_once(&NetworkServiceOnce, ^{
    NetworkServiceID = [CBUUID UUIDWithString:@BLENetworkInterfaceService];
    NetworkClientToServerCharacteristic = [CBUUID UUIDWithString:@BLENetworkInterfaceCharacteristic_C2S];
    NetworkServerToClientCharacteristic = [CBUUID UUIDWithString:@BLENetworkInterfaceCharacteristic_S2C];
    NetworkDisconnectCharacteristic     = [CBUUID UUIDWithString:@BLENetworkInterfaceCharacteristic_Disconnect];
  });
}

// this clientID doesn't look like it makes much sense here..
//  the clientID is really a higher level thing I believe.
-(id)initWithClientID:(NSString *)clientID {
  if((self = [super init])) {
    _clientID = clientID;
    _availableEndpointsByClientID  = [NSMutableDictionary dictionary];
    _connectedEndpointsByClientID  = [NSMutableDictionary dictionary];
    _connectingEndpointsByClientID = [NSMutableDictionary dictionary];
    [self createServiceDescription];
  }
  return self;
}

-(void)dealloc {
  [self stop];
}

-(void)stop {
  if(self.centralMultiplexer) {
    [_connectedEndpointsByClientID enumerateKeysAndObjectsUsingBlock:^(NSString* clientID, BLENetworkServer_Endpoint* endpoint, BOOL *stop) {
      [self.centralMultiplexer disconnectPeripheral:endpoint.peripheral withService:_serviceDescription];
    }];
    [_connectingEndpointsByClientID enumerateKeysAndObjectsUsingBlock:^(NSString* clientID, BLENetworkServer_Endpoint* endpoint, BOOL *stop) {
      [self.centralMultiplexer disconnectPeripheral:endpoint.peripheral withService:_serviceDescription];
    }];
    self.centralMultiplexer = nil;
  }
}

-(void)createServiceDescription {
  _serviceDescription = [[BLECentralServiceDescription alloc] init];
  [BLENetworkServer initServiceAndCharacteristicIDs];
  _serviceDescription.serviceID = NetworkServiceID;
  _serviceDescription.characteristicIDs = @[NetworkClientToServerCharacteristic, NetworkServerToClientCharacteristic, NetworkDisconnectCharacteristic];
  _serviceDescription.delegate = self;
  _serviceDescription.name = @BLENetworkInterfaceServiceName;
}

-(BLECentralServiceDescription*)serviceDescription {
  return _serviceDescription;
}

-(void)setCentralMultiplexer:(BLECentralMultiplexer *)centralMultiplexer {
  [_centralMultiplexer cancelService:self.serviceDescription];
  _centralMultiplexer = centralMultiplexer;
  [_centralMultiplexer registerService:self.serviceDescription];
}

-(void)setAvailable:(BOOL)available {
  if(!_available && available) {
    // start scanning
    [_centralMultiplexer startScanningForService:self.serviceDescription];
  }
  else if(_available && !available) {
    // stop scanning (should we only unregister our services?)
    [_centralMultiplexer stopScanningForService:self.serviceDescription];
  }
  _available = available;
}

-(NSArray*)advertisedEndpoints {
  return _availableEndpointsByClientID.allKeys;
}

-(NSArray*)connectedEndpoints {
  return _connectedEndpointsByClientID.allKeys;
}

-(NSArray*)connectingEndpoints {
  return _connectingEndpointsByClientID.allKeys;
}

-(BLENetworkServer_Endpoint*)endpointInDictionary:(NSDictionary*)dictionary forPeripheral:(CBPeripheral*)peripheral {
  for (BLENetworkServer_Endpoint* curEndpoint in dictionary.allValues) {
    if(curEndpoint.peripheral == peripheral) {
      return curEndpoint;
    }
  }
  return nil;
}

-(BLENetworkServer_Endpoint*)availableEndpointForPeripheral:(CBPeripheral*)peripheral {
  return [self endpointInDictionary:_availableEndpointsByClientID forPeripheral:peripheral];
}

-(BLENetworkServer_Endpoint*)connectingEndpointForPeripheral:(CBPeripheral*)peripheral {
  return [self endpointInDictionary:_connectingEndpointsByClientID forPeripheral:peripheral];
}

-(BLENetworkServer_Endpoint*)connectedEndpointForPeripheral:(CBPeripheral*)peripheral {
  return [self endpointInDictionary:_connectedEndpointsByClientID forPeripheral:peripheral];
}

-(void)connectToEndpointID:(NSString*)peerID {
  BLENetworkServer_Endpoint* endpoint = _availableEndpointsByClientID[peerID];
  if(endpoint != nil) {
    endpoint.state = BLENetworkEndpointStateConnecting;
    [_centralMultiplexer connectPeripheral:endpoint.peripheral withService:self.serviceDescription];
    _connectingEndpointsByClientID[peerID] = endpoint;
    // we'll remove this endpoint from the _availableEndpoints when we get the peripheralDidDisappear callback.
  }
}

-(void)disconnectFromEndpointID:(NSString *)peerID {
  BLELogInfo("BLENetworkServer.disconnectFromEndpoint", "%s", peerID.UTF8String);
  BLENetworkServer_Endpoint* endpoint = _connectedEndpointsByClientID[peerID];
  if(endpoint == nil) {
    endpoint = _connectingEndpointsByClientID[peerID];
    if(endpoint) {
      // We're still connecting, just disconnect normally.
      if(endpoint.state != BLENetworkEndpointStateDisconnecting) {
        endpoint.state = BLENetworkEndpointStateDisconnecting;
        [_centralMultiplexer disconnectPeripheral:endpoint.peripheral withService:self.serviceDescription];
      }
    }
  }
  else {
    if(endpoint.state < BLENetworkEndpointStateDisconnecting) {
      // Clear the buffered messages, and send the disconnect message
      [endpoint.enqueuedReliableMessages removeAllObjects];
      NSError* error = nil;
      if(![self sendMessage:_BLENetworkingInterfaceDisconnectMessage() toEndpoint:peerID error:&error] || error) {
        BLELogError("BLENetworkServer.disconnectFromEndpoint.error", "%s", error.description.UTF8String);
      }
      endpoint.state = BLENetworkEndpointStateDisconnecting;
    }
  }
}

-(void)doRealDisconnectFromEndpointID:(NSString*)peerID {
  BLENetworkServer_Endpoint* endpoint = _connectedEndpointsByClientID[peerID];
  if(endpoint.state != BLENetworkEndpointStateWaitingForCBDisconnect) {
    // We set the notify value to NO, and disconnect once the notifications have stopped.
    endpoint.state = BLENetworkEndpointStateWaitingForCBDisconnect;
    [endpoint.peripheral setNotifyValue:NO forCharacteristic:endpoint.clientToServerCharacteristic];
    [endpoint.peripheral setNotifyValue:NO forCharacteristic:endpoint.disconnectCharacteristic];
    [_centralMultiplexer disconnectPeripheral:endpoint.peripheral withService:self.serviceDescription];
  }
}

-(NSString*)advertisedNameForEndpointID:(NSString*)endpointID {
  BLENetworkServer_Endpoint* endpoint = _availableEndpointsByClientID[endpointID];
  if(endpoint == nil) {
    endpoint = _connectedEndpointsByClientID[endpointID];
    if(endpoint == nil) {
      endpoint = _connectingEndpointsByClientID[endpointID];
    }
  }
  if(endpoint) {
    return endpoint.advertisedName;
  }
  return nil;
}

-(BOOL)sendMessage:(NSData *)message toEndpoint:(NSString *)endpointAddress error:(NSError *__autoreleasing *)error {
  BLENetworkServer_Endpoint* endpoint = _connectedEndpointsByClientID[endpointAddress];
  if(endpoint == nil) {
    BLELogError("BLENetworkServer.sendMessage.unknownEndpoint", "%s", endpointAddress.UTF8String);
    if(error) {
      *error = [NSError errorWithDomain:BLENetworkServerErrorDomain code:BLENetworkServerErrorUnknownEndpoint userInfo:nil];
    }
    return NO;
  }
  
  if(message.length > BLENetworkInterfaceMaxMessageLength) {
    BLEAssert(message.length <= BLENetworkInterfaceMaxMessageLength);
    BLELogError("BLENetworkServer.sendMessage.tooBig", "size=%lu bytes", (unsigned long)message.length);
    if(error) {
      *error = [NSError errorWithDomain:BLENetworkServerErrorDomain code:BLENetworkServerErrorMessageTooBig userInfo:nil];
    }
    return NO;
  }
  
  if(endpoint.state != BLENetworkEndpointStateConnected) {
    BLELogWarn("BLENetworkServer.sendMessage.endpointNotConnected", "%s", endpointAddress.UTF8String);
    if(error) {
      *error = [NSError errorWithDomain:BLENetworkServerErrorDomain code:BLENetworkServerErrorEndpointNotConnected userInfo:nil];
    }
    return NO;
  }
  
  // We rate-limit messages.  canWrite is a flag that tells us if we can write immediately.
  //  The first write will be quick, to be conservative we just write one message on the first
  //  connection interval.  We will batch subsequent messages.
  if(!endpoint.canWrite) {
    BLELogDebug("BLENetworkServer.sendMessage.bufferingMessage", "length=%lu", (unsigned long)message.length);
    [endpoint.enqueuedReliableMessages addObject:message];
  }
  else {
    BLELogDebug("BLENetworkServer.sendMessage.immediateSend", "length=%lu", (unsigned long)message.length);
    endpoint.canWrite = NO;
    [endpoint.enqueuedReliableMessages addObject:message];
    dispatch_async(dispatch_get_main_queue(), ^{
      // send when we hit the main runloop again, to give us time
      //  to possibly enqueue several writes.
      [self sendBufferedMessagesToEndpointAddress:endpointAddress];
    });
  }

  return YES;
}

-(BOOL)sendReliableMessage:(NSData*)message toEndpoint:(NSString*)endpointID error:(NSError**)error {
  return [self sendMessage:message toEndpoint:endpointID error:error];
}

-(void)sendBufferedMessagesToEndpointAddress:(NSString*)endpointAddress {
  BLENetworkServer_Endpoint* endpoint = _connectedEndpointsByClientID[endpointAddress];
  if(endpoint != nil) {
    //We may have been waiting for a batch to drain while we received a disconnect.
    if(endpoint.state == BLENetworkEndpointStateDisconnecting) {
      [self sendDisconnectMessageToEndpoint:endpointAddress];
      return;
    }
    // Drop messages on the floor if we're not connected.
    if(endpoint.state != BLENetworkEndpointStateConnected) {
      BLELogWarn("BLENetworkServer.sendBufferedMessages.notConnected", "%s", endpointAddress.UTF8String);
      return;
    }
    // Only clear the canWrite flag (allowing the buffer pump loop to start again)
    //  when the pump runs out of messages.
    if(endpoint.enqueuedReliableMessages.count == 0) {
      endpoint.canWrite = YES;
      BLELogDebug("BLENetworkServer.sendBufferedMessages.bufferEmpty", "%s", endpointAddress.UTF8String);
    }
    else {
      [self sendNextBatchOfMessagesToEndpointAddress:endpointAddress];
    }
  }
  else {
    BLELogWarn("BLENetworkServer.sendBufferedMessages.endpointDisconnected", "%s", endpointAddress.UTF8String);
  }
}

-(void)sendDisconnectMessageToEndpoint:(NSString*)endpointID {
  BLELogDebug("BLENetworkServer.sendDisconnectMessageToEndpoint", "%s", endpointID.UTF8String);
  BLENetworkServer_Endpoint* endpoint = _connectedEndpointsByClientID[endpointID];
  assert(endpoint);
  assert(endpoint.state == BLENetworkEndpointStateDisconnecting);
  assert(endpoint.enqueuedReliableMessages.count == 1);
  // We should only have one message in our outbound queue (the disconnect message).
  NSData* disconnectData = endpoint.enqueuedReliableMessages[0];
  assert([disconnectData isEqualToData:_BLENetworkingInterfaceDisconnectMessage()]);
  [endpoint.peripheral writeValue:disconnectData forCharacteristic:endpoint.serverToClientCharacteristic type:CBCharacteristicWriteWithoutResponse];
  double delayInSeconds = 5.0;
  dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSeconds * NSEC_PER_SEC));
  dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
    // Force the disconnect if we haven't received a response in 5 seconds.
    if(endpoint.state == BLENetworkEndpointStateDisconnecting) {
      [self doRealDisconnectFromEndpointID:endpointID];
    }
  });
}

-(void)sendNextBatchOfMessagesToEndpointAddress:(NSString*)endpointAddress {
  BLENetworkServer_Endpoint* endpoint = _connectedEndpointsByClientID[endpointAddress];
  assert(endpoint);
  assert(endpoint.state == BLENetworkEndpointStateConnected);
  
  // Pump the next batch of messages into CoreBluetooth.
  int batchLength = _BLEServerInflightBatchLength();
  int messagesLeftInBatch = batchLength;
  while(messagesLeftInBatch > 0 && endpoint.enqueuedReliableMessages.count > 0) {
    BLELogDebug("BLENetworkServer.sendBufferedMessages.sendNextBufferedMessage", "inFlight=%u, buffered=%lu", batchLength - messagesLeftInBatch, (unsigned long)endpoint.enqueuedReliableMessages.count);
    messagesLeftInBatch--;
    NSData* nextData = endpoint.enqueuedReliableMessages[0];
    [endpoint.enqueuedReliableMessages removeObjectAtIndex:0];
    [endpoint.peripheral writeValue:nextData forCharacteristic:endpoint.serverToClientCharacteristic type:CBCharacteristicWriteWithoutResponse];
  }
  
  // Set the throttling timer again.
  //  Even if other messages come in, they have to wait until these ones trickle out.
  BLELogDebug("BLENetworkServer.sendBufferedMessages.throttlingMessages", "%s", endpointAddress.UTF8String);
  [self delayedWriteToEndpointAddress:endpointAddress];
}

-(void)delayedWriteToEndpointAddress:(NSString*)endpointAddress {
  BLELogDebug("BLENetworkServer.delayedWriteToEndpointAddress", "%s", endpointAddress.UTF8String);
  BLENetworkServer_Endpoint* endpoint = _connectedEndpointsByClientID[endpointAddress];
  if(endpoint) {
    uint64_t delayInMilliSeconds = _BLEServerThrottlePeriodMSecs();
    dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInMilliSeconds * NSEC_PER_MSEC));
    dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
      [self sendBufferedMessagesToEndpointAddress:endpointAddress];
    });
  }
}

#pragma mark - IOS 6 CONNECTION TIMEOUT BUG
-(void)receivedHandshakeTimeout {
  // The bug is on the client side, don't tell the server to reboot...
}


#pragma mark - BLECentralServiceDelegate methods

-(void)service:(BLECentralServiceDescription *)service didDiscoverPeripheral:(CBPeripheral *)peripheral withRSSI:(NSNumber *)rssiVal advertisementData:(NSDictionary *)advertisementData {
  // Don't go report an already known / connected endpoint.
  BLENetworkServer_Endpoint* existingEndpoint = [self connectedEndpointForPeripheral:peripheral];
  if(!existingEndpoint) {
    existingEndpoint = [self connectingEndpointForPeripheral:peripheral];
  }
  if(existingEndpoint) {
    // happens when we're retrying.
    BLELogDebug("BLENetworkServer.didDiscoverPeripheral.alreadyConnecting", "peripheral=%p, endpoint=%s", peripheral, existingEndpoint.clientID.UTF8String);
    return;
  }
  
  // Make sure this isn't a duplicate!
  existingEndpoint = [self availableEndpointForPeripheral:peripheral];
  if(existingEndpoint) {
    BLELogWarn("BLENetworkServer.didDiscoverPeripheral.duplicate", "peripheral=%p, endpoint=%s", peripheral, existingEndpoint.clientID.UTF8String);
    BLEAssert(!existingEndpoint);
  }
  
  // This is a new endpoint, keep track of it.
  BLENetworkServer_Endpoint* newEndpoint = [[BLENetworkServer_Endpoint alloc] init];
  newEndpoint.peripheral = peripheral;
  newEndpoint.clientID = [[NSUUID UUID] UUIDString];
  newEndpoint.state = BLENetworkEndpointStateAvailable;
  newEndpoint.advertisedName = advertisementData[CBAdvertisementDataLocalNameKey];
  newEndpoint.enqueuedReliableMessages = [NSMutableArray arrayWithCapacity:60];
  
  BLELogDebug("BLENetworkServer.didDiscoverPeripheral.newEndpoint", "endpoint=%s (name=%s)", newEndpoint.clientID.UTF8String, newEndpoint.advertisedName.UTF8String);

  // TODO: KVO here.
  _availableEndpointsByClientID[newEndpoint.clientID] = newEndpoint;
  [self.delegate BLENetworkInterface:self endpointDiscovered:newEndpoint.clientID];
}

-(void)service:(BLECentralServiceDescription *)service peripheralDidDisappear:(CBPeripheral *)peripheral {
  BLENetworkServer_Endpoint* disappearingEndpoint = [self availableEndpointForPeripheral:peripheral];
  
  BLELogDebug("BLENetworkServer.peripheralDidDisappear", "endpoint=%s (name=%s)", disappearingEndpoint.clientID.UTF8String, disappearingEndpoint.advertisedName.UTF8String);
  // The peripheral disappears once it's not advertising anymore.
  // This should happen when the peripheral goes away and also when it becomes connected.
  if(disappearingEndpoint != nil) {
    // TODO: KVO here.
    [_availableEndpointsByClientID removeObjectForKey:disappearingEndpoint.clientID];
    if(disappearingEndpoint.state < BLENetworkEndpointStateConnecting) {
      disappearingEndpoint.state = BLENetworkEndpointStateUnavailable;
      // Note: we dispatch endpointDisappeared if we are connected or connecting.
      [self.delegate BLENetworkInterface:self endpointDisappeared:disappearingEndpoint.clientID];
    }
  }
}

- (BOOL)isEndpointConnected:(BLENetworkServer_Endpoint*)endpoint {
  return endpoint.clientToServerCharacteristic && endpoint.clientToServerCharacteristic.isNotifying
      && endpoint.disconnectCharacteristic && endpoint.disconnectCharacteristic.isNotifying
      && endpoint.serverToClientCharacteristic;
}

- (BOOL)isEndpointDisconnected:(BLENetworkServer_Endpoint*)endpoint {
  return !(endpoint.clientToServerCharacteristic && endpoint.clientToServerCharacteristic.isNotifying)
      && !(endpoint.disconnectCharacteristic && endpoint.disconnectCharacteristic.isNotifying);
}

-(void)service:(BLECentralServiceDescription *)service discoveredCharacteristic:(CBCharacteristic *)characteristic forPeripheral:(CBPeripheral *)peripheral {
  BLELogDebug("BLENetworkServer.discoveredCharacteristic", "uuid=%s peripheral=%p", characteristic.UUID.description.UTF8String, peripheral);

  BLEAssert([service isEqual:[self serviceDescription]]);
  BLENetworkServer_Endpoint* endpoint = [self connectingEndpointForPeripheral:peripheral];
  if(endpoint != nil) {
    if([characteristic.UUID isEqual:NetworkClientToServerCharacteristic]) {
      // TODO: check to make sure this characteristic isn't already notifying.
      BLELogDebug("BLENetworkServer.discoveredCharacteristic.inboundPipeDiscovered", "");
      // set the notification state on this characteristic since we know it's a read-only characteristic.
      // we save this in the notify change callback
      [peripheral setNotifyValue:YES forCharacteristic:characteristic];
    }
    else if([characteristic.UUID isEqual:NetworkServerToClientCharacteristic]){
      BLELogDebug("BLENetworkServer.discoveredCharacteristic.outboundPipeDiscovered", "");
      // this is the write-only characteristic, save it.
      endpoint.serverToClientCharacteristic = characteristic;
      
      if ( [self isEndpointConnected:endpoint] ) {
        [self endpointConnected:endpoint];
      }
    }
    else if([characteristic.UUID isEqual:NetworkDisconnectCharacteristic]) {
      BLELogDebug("BLENetworkServer.discoveredCharacteristic.disconnectPipeDiscovered", "");
      [peripheral setNotifyValue:YES forCharacteristic:characteristic];
    }
  }
}

-(void)endpointConnected:(BLENetworkServer_Endpoint*)endpoint {
  BLELogDebug("BLENetworkServer.endpointConnected", "%s (name=%s)", endpoint.clientID.UTF8String, endpoint.advertisedName.UTF8String);
  _connectedEndpointsByClientID[endpoint.clientID] = endpoint;
  [_connectingEndpointsByClientID removeObjectForKey:endpoint.clientID];
  endpoint.state = BLENetworkEndpointStateConnected;
  endpoint.canWrite = YES;
  [self.delegate BLENetworkInterface:self endpointConnected:endpoint.clientID];
}

-(void)service:(BLECentralServiceDescription *)service didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic forPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
  BLELogDebug("BLENetworkServer.didUpdateNotificationStateForCharacteristic", "peripheral=%p characteristic=%s error=%s", peripheral, characteristic.UUID.description.UTF8String, error.description.UTF8String);

  BLEAssert([service isEqual:[self serviceDescription]]);
  BLENetworkServer_Endpoint* endpoint = [self connectingEndpointForPeripheral:peripheral];
  if(endpoint != nil) {
    if(characteristic.isNotifying) {
      if ([characteristic.UUID isEqual:NetworkClientToServerCharacteristic]) {
        BLELogDebug("BLENetworkServer.connectedInboundPipe", "%s (name=%s)", endpoint.clientID.UTF8String, endpoint.advertisedName.UTF8String);
        endpoint.clientToServerCharacteristic = characteristic;
      }
      else if([characteristic.UUID isEqual:NetworkDisconnectCharacteristic]) {
        BLELogDebug("BLENetworkServer.connectedDisconnectPipe", "%s (name=%s)", endpoint.clientID.UTF8String, endpoint.advertisedName.UTF8String);
        endpoint.disconnectCharacteristic = characteristic;
      }
      
      if ( [self isEndpointConnected:endpoint] ) {
        [self endpointConnected:endpoint];
      }
    }// end isNotifying
     else if ( [self isEndpointDisconnected:endpoint] ) {
       BLELogInfo("BLENetworkServer.disconnectedPipe", "%s (name=%s)", endpoint.clientID.UTF8String, endpoint.advertisedName.UTF8String);
       // Notification went into NO state, can we just re-enable notifications?
       //  We cause this to happen on purpose during the disconnect sequence.  It may happen on accident as well, so we need to watch out.
       [_centralMultiplexer disconnectPeripheral:endpoint.peripheral withService:self.serviceDescription];
     }
  }
}

-(void)service:(BLECentralServiceDescription *)service incomingMessage:(NSData *)message onCharacteristic:(CBCharacteristic *)characteristic forPeripheral:(CBPeripheral *)peripheral {
  if(message == nil || message.length == 0) {
    // Shouldn't happen anymore..
    return;
  }
  
  BLENetworkServer_Endpoint* endpoint = [self connectedEndpointForPeripheral:peripheral];
  if(endpoint != nil) {
    
    
    if([message isEqualToData:_BLENetworkingInterfaceDisconnectMessage()]) {
      BLELogDebug("BLENetworkServer.incomingMessage.disconnectMessageFromEndpoint", "%s", endpoint.clientID.UTF8String);
      [self doRealDisconnectFromEndpointID:endpoint.clientID];
      return;
    }
    
    BLELogDebug("BLENetworkServer.incomingMessage", "%s (name=%s) length=%lu", endpoint.clientID.UTF8String, endpoint.advertisedName.UTF8String, (unsigned long)message.length);
    if(endpoint.state != BLENetworkEndpointStateConnected) {
      BLELogDebug("BLENetworkServer.incomingMessage.disconnected", "%s (name=%s)", endpoint.clientID.UTF8String, endpoint.advertisedName.UTF8String);
    }
    if([characteristic.UUID isEqual:NetworkClientToServerCharacteristic]) {
      // incoming message, pass it up to the interface above.
      [self.delegate BLENetworkInterface:self receivedData:message fromEndpoint:endpoint.clientID];
    }
    else if([characteristic.UUID isEqual:NetworkDisconnectCharacteristic]) {
      // the client wishes to disconnect, we should respect this.
      [self disconnectFromEndpointID:endpoint.clientID];
    }
  
  }
  else {
    BLELogDebug("BLENetworkServer.incomingMessage.unknownPeripheral", "%p", peripheral);
  }
}


-(void)service:(BLECentralServiceDescription *)service didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
  assert([service isEqual:[self serviceDescription]]);
  BLENetworkServer_Endpoint* endpoint = [self connectedEndpointForPeripheral:peripheral];
  if(endpoint == nil) {
    endpoint = [self connectingEndpointForPeripheral:peripheral];
  }
  if(endpoint != nil) {
    BLELogInfo("BLENetworkServer.didDisconnectPeripheral", "%s (name=%s)", endpoint.clientID.UTF8String, endpoint.advertisedName.UTF8String);
    if(endpoint.state >= BLENetworkEndpointStateConnecting) {
      endpoint.state = BLENetworkEndpointStateUnavailable;
    }
    [_connectingEndpointsByClientID removeObjectForKey:endpoint.clientID];
    [_connectedEndpointsByClientID removeObjectForKey:endpoint.clientID];
    [_availableEndpointsByClientID removeObjectForKey:endpoint.clientID];
    [self.delegate BLENetworkInterface:self endpointDisconnected:endpoint.clientID];
  }
}

-(void)service:(BLECentralServiceDescription *)service peripheral:(CBPeripheral *)peripheral didUpdateRSSI:(NSNumber *)rssiVal {
  // update RSSI..
  //  There's currently no way to get RSSI up to the networking layer.  Perhaps we should add this.
}

-(void)service:(BLECentralServiceDescription*)service peripheral:(CBPeripheral*)peripheral didUpdateAdvertisementData:(NSDictionary*)advertisementData {
  // No way to update the advertisement data unless the client stops advertising, right?
}

-(void)service:(BLECentralServiceDescription*)service peripheral:(CBPeripheral*)peripheral didWriteValueForCharacteristic:(CBCharacteristic *)characteristic
         error:(NSError *)error {
  if(error) {
    // If an error occurred, log it and synthesize a disconnect (the connection is bunk at this point).
    BLELogError("BLENetworkServer.didWriteValueForCharacteristic.error", "error=%s", error.description.UTF8String);
    [_centralMultiplexer disconnectPeripheral:peripheral withService:service];
  }
  else {
    // Otherwise note that we got a response for write to the characteristic and possibly send more reliable messages.`
    BLELogDebug("BLENetworkServer.didWriteValueForCharacteristic", "peripheral=%s, characteristic=%s", peripheral.description.UTF8String, characteristic.description.UTF8String);
  }
}

@end
