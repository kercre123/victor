//
//  BLENetworkClient.m
//  BLEManager
//
//  Created by Mark Pauley on 3/27/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "BLEManager/Networking/Client/BLENetworkClient.h"
#import <IOBluetooth/IOBluetooth.h>
#import "BLEManager/Logging/BLELog.h"
#import "BLEManager/Networking/BLENetworkingInterfacePrivate.h"

@interface BLENetworkClient () <CBPeripheralManagerDelegate>

@end

@implementation BLENetworkClient {
  CBPeripheralManager* _peripheralManager;
  CBMutableService* _clientService;
  NSString* _hostID;
  CBCentral* _connectedCentral;
  CBCentral* _subscribedCentral;
  CBMutableCharacteristic* _toServerCharacteristic;
  CBMutableCharacteristic* _fromServerCharacteristic;
  CBMutableCharacteristic* _disconnectCharacteristic;
    
  CBCharacteristic* _subscribedToServerCharacteristic;
  CBCharacteristic* _subscribedDisconnectCharacteristic;
  
  BOOL _isConnected;
  BOOL _isDisconnecting;
  BOOL _canSendData;
  NSMutableArray* _sendBuffer;
  
  NSUInteger _numHandshakeTimeouts;
}

@synthesize clientID  = _clientID;
@synthesize available = _available;
@synthesize delegate  = _delegate;
@synthesize peripheralManager = _peripheralManager;

+(CBPeripheralManager*)sharedPeripheralManager {
  static CBPeripheralManager* _peripheralManager = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _peripheralManager = [[CBPeripheralManager alloc] initWithDelegate:nil queue:nil];
  });
  return _peripheralManager;
}

-(id)initWithClientID:(NSString *)clientID peripheralManager:(CBPeripheralManager*)peripheralManager {
  if((self = [super init])) {
    _clientID = clientID;
    _sendBuffer = [[NSMutableArray alloc] init];
    self.peripheralManager = peripheralManager;
  }
  return self;
}

-(id)initWithClientID:(NSString *)clientID {
  if((self = [self initWithClientID:clientID peripheralManager:[[self class] sharedPeripheralManager]])) {
    
  }
  return self;
}

-(void)dealloc {
  [self stop];
}

-(void)stop {
  if(_hostID) {
    [self disconnectFromEndpointID:_hostID];
  }
  [self removeServiceAndCharacteristics];
  self.available = NO;
  self.peripheralManager = nil;
}

-(void)setPeripheralManager:(CBPeripheralManager *)peripheralManager {
  [self removeServiceAndCharacteristics];
  _peripheralManager.delegate = nil;
  
  _peripheralManager = peripheralManager;
  _peripheralManager.delegate = self;
}

-(NSArray*)advertisedEndpoints {
  return @[]; // the server sees us, not the other way around.
}

-(NSArray*)connectingEndpoints {
  return @[];
}

-(NSArray*)connectedEndpoints {
  if(_hostID) {
    return @[_hostID];
  }
  else {
    return @[];
  }
}

-(CBUUID*) inboundCharacteristicUUID {
  return [CBUUID UUIDWithString:@BLENetworkInterfaceCharacteristic_S2C];
}

-(CBUUID*) outboundCharacteristicUUID {
  return [CBUUID UUIDWithString:@BLENetworkInterfaceCharacteristic_C2S];
}

-(CBUUID*) disconnectCharacteristicUUID {
  return [CBUUID UUIDWithString:@BLENetworkInterfaceCharacteristic_Disconnect];
}

-(NSString*)endpointID {
  return _hostID;
}

-(void)addServiceAndCharacteristics {
  if(!_clientService) {
    // The server (central) is able to write to this characteristic
    _fromServerCharacteristic = [[CBMutableCharacteristic alloc] initWithType:[self inboundCharacteristicUUID]
                                                                 properties:CBCharacteristicPropertyWrite|CBCharacteristicPropertyWriteWithoutResponse
                                                                      value:nil
                                                                permissions:CBAttributePermissionsWriteable];
    
    // The server (central) is able to read from this characteristic
    _toServerCharacteristic = [[CBMutableCharacteristic alloc] initWithType:[self outboundCharacteristicUUID]
                                                                   properties:CBCharacteristicPropertyNotify
                                                                        value:nil
                                                                  permissions:CBAttributePermissionsReadable];

    // We use this characteristic to tell the server (central) to disconnect from us
    _disconnectCharacteristic = [[CBMutableCharacteristic alloc] initWithType:[self disconnectCharacteristicUUID]
                                                                   properties:CBCharacteristicPropertyNotify
                                                                        value:nil
                                                                  permissions:CBAttributePermissionsReadable];
    
    _clientService = [[CBMutableService alloc] initWithType:[CBUUID UUIDWithString:@BLENetworkInterfaceService] primary:YES];
    _clientService.characteristics = @[_fromServerCharacteristic,_toServerCharacteristic,_disconnectCharacteristic];
    [self.peripheralManager addService:_clientService];
  }
}

-(void)removeServiceAndCharacteristics {
  if(_clientService) {
    [_peripheralManager removeService:_clientService];
    _clientService = nil;
    _fromServerCharacteristic = nil;
    _toServerCharacteristic = nil;
    _disconnectCharacteristic = nil;
  }
}

-(void)setAvailable:(BOOL)available {
  if(_available && !available) {
    [self stopAdvertising];
  }
  else if(!_available && available) {
    [self startAdvertising];
  }
  _available = available;
}

-(void)startAdvertising {
  if(!_hostID && !_connectedCentral) { // we don't advertise when we're already connected. We will only respond to one central at a time.
    if(_peripheralManager.state == CBPeripheralManagerStatePoweredOn) {
      [self addServiceAndCharacteristics];
      BLELogDebug("BLENetworkClient.startAdvertising", "");
      [_peripheralManager startAdvertising:@{ CBAdvertisementDataServiceUUIDsKey : @[[CBUUID UUIDWithString:@BLENetworkInterfaceService]],
                                                 CBAdvertisementDataLocalNameKey : self.clientID,
      }];
    }
  }
}

-(void)stopAdvertising {
  BLELogDebug("BLENetworkClient.stopAdvertising", "");
  [_peripheralManager stopAdvertising];
}


-(void)connectToEndpointID:(NSString*)peerID {
  // servers connect to clients in BTLE, not the other way around...
  BLEAssert(!"BLENetworkClient.connectToEndpointID");
}

-(void)disconnectFromEndpointID:(NSString *)peerID {
  BLELogDebug("BLENetworkClient.disconnectFromEndpoint", "%s", peerID.UTF8String);
  if([peerID isEqualToString:_hostID]) {
    [_sendBuffer removeAllObjects];
    NSError* error = nil;
    if(![self sendMessage:_BLENetworkingInterfaceDisconnectMessage() toEndpoint:_hostID error:&error] || error) {
      BLELogError("BLENetworkClient.disconnectFromEndpoint.error", "%s", error.description.UTF8String);
    }
    _isDisconnecting = YES;
  }
  else {
    BLELogWarn("BLENetworkClient.disconnectFromEndpoint.unknown", "%s", peerID.UTF8String);
  }
}

-(NSString*)advertisedNameForEndpointID:(NSString*)endpointID {
  // FIXME:
  // This is going to have to come after connect happens
  return @"BLENetwork_Host_Endpoint";
}

-(void)hostConnected:(CBCentral*)connectedCentral {
  NSString* hostUUID = [[NSUUID UUID] UUIDString];
  BLELogDebug("BLENetworkClient.hostConnected", "%s", hostUUID.UTF8String);
  BLELogDebug("BLENetworkClient.central.maximumUpdateValueLength", "%lu", (unsigned long)connectedCentral.maximumUpdateValueLength);
  _connectedCentral = connectedCentral;
  _hostID = hostUUID;
  _canSendData = YES;
  _isConnected = YES;
  [self.delegate BLENetworkInterface:self endpointConnected:_hostID];
  [self stopAdvertising];
}

-(void)hostDisconnected {
  BLELogDebug("BLENetworkClient.hostDisconnected", "%s", _hostID.UTF8String);
  NSString* hostUUID = _hostID;
  _canSendData = NO;
  _hostID = nil;
  _subscribedCentral = nil;
  _connectedCentral = nil;
  _subscribedToServerCharacteristic = nil;
  _subscribedDisconnectCharacteristic = nil;
  _sendBuffer = [[NSMutableArray alloc] init];
  _isConnected = NO;
  _isDisconnecting = NO;
  
  [self.delegate BLENetworkInterface:self endpointDisconnected:hostUUID];
  if(_available) {
    // We throttle advertising after becoming disconnected to keep from flogging the CBPeripheralManager with connect/disconnects.
    double delayInSeconds = 2.0;
    dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSeconds * NSEC_PER_SEC));
    dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
      [self startAdvertising];
    });
  }
}

-(BOOL)sendMessage:(NSData*)message toEndpoint:(NSString*)endpointAddress error:(NSError**)error {
  BLELogDebug("BLENetworkClient.sendMessage.central", "%s", _connectedCentral.description.UTF8String);
  if(!_isConnected) {
    // If we're not connected, drop the message on the floor.
    return NO;
  }
  if(!endpointAddress || ![endpointAddress isEqualToString:_hostID]) {
    // We can't send to someone that's not the host
    BLELogWarn("BLENetworkClient.sendMessage.wrongHostID", "%s", endpointAddress.UTF8String);
    if(error) {
      *error = [NSError errorWithDomain:BLENetworkClientErrorDomain code:BLENetworkClientErrorNoSuchEndpoint userInfo:nil];
    }
    return NO;
  }
  if(_isDisconnecting) {
    BLELogInfo("BLENetworkClient.sendMessage.duringDisconnect", "%s", endpointAddress.UTF8String);
    // Since we could easily get here because the server decided to disconnect,
    //  just drop the message and pretend it worked.
    return YES;
  }
  if(message.length > BLENetworkInterfaceMaxMessageLength) {
    BLELogError("BLENetworkClient.sendMessage.messageTooBig", "%lu", (unsigned long)message.length);
    if(error) {
      *error = [NSError errorWithDomain:BLENetworkClientErrorDomain code:BLENetworkClientErrorPacketTooBig userInfo:nil];
    }
    return NO;
  }
  
  BOOL success = NO;
  if(_canSendData && _sendBuffer.count == 0) {
    BLELogDebug("BLENetworkClient.sendMessage.immediately", "");
    success = [_peripheralManager updateValue:message forCharacteristic:_toServerCharacteristic onSubscribedCentrals:@[_connectedCentral]];
  }
  
  if(!success) {
    BLELogDebug("BLENetworkClient.sendMessage.buffering", "");
    _canSendData = NO;
    [self appendMessageToSendBuffer:message];
    success = YES;
  }
  return success;
}

-(void)appendMessageToSendBuffer:(NSData*)message {
  [_sendBuffer addObject:message];
}

// All messages are reliable going back to the server (thanks to the sendBufferedMessages behavior).
-(BOOL)sendReliableMessage:(NSData*)message toEndpoint:(NSString*)endpointID error:(NSError**)error {
  return [self sendMessage:message toEndpoint:endpointID error:error];
}

-(void)sendBufferedMessages {
  BLELogDebug("BLENetworkClient.sendBufferedMessages", "bufferSize=%lu", (unsigned long)_sendBuffer.count);
  while (_canSendData && _sendBuffer.count > 0) {
    NSData* curMessage = _sendBuffer[0];
    _canSendData = [_peripheralManager updateValue:curMessage forCharacteristic:_toServerCharacteristic onSubscribedCentrals:@[_connectedCentral]];
    if(_canSendData) {
      [_sendBuffer removeObjectAtIndex:0];
      BLELogDebug("BLENetworkClient.sendBufferedMessages.success", "bufferSize=%lu", (unsigned long)_sendBuffer.count);
    }
    else {
      BLELogDebug("BLENetworkClient.sendBufferedMessages.throttle", "bufferSize=%lu", (unsigned long)_sendBuffer.count);
    }
  }
}

#pragma mark - IOS 6 CONNECTION TIMEOUT BUG
-(void)receivedHandshakeTimeout {
  _numHandshakeTimeouts++;
  BLELogWarn("BLENetworkClient.receivedHandshakeTimeout_i", "%lu", (unsigned long)_numHandshakeTimeouts);
  if(_numHandshakeTimeouts >= 3) {
    BLELogError("BLENetworkClient.BLENetworkInterfaceIsHosed", "");
    [[NSNotificationCenter defaultCenter] postNotificationName:BLENetworkInterfaceIsHosedNotification object:self];
  }
}

#pragma mark - CBPeripheralManagerDelegate
- (void)peripheralManagerDidUpdateState:(CBPeripheralManager *)peripheral {
  BLELogInfo("BLENetworkClient.peripheralManagerDidUpdateState", "%d", (int)peripheral.state);
  switch (peripheral.state) {
    case CBPeripheralManagerStateUnknown:
    case CBPeripheralManagerStateUnsupported:
    case CBPeripheralManagerStateUnauthorized:
      BLELogError("BLENetworkClient.peripheralManagerDidUpdateState.unsupported", "%d", (int)peripheral.state);
      break;
      
    case CBPeripheralManagerStateResetting:
    case CBPeripheralManagerStatePoweredOff:
      BLELogWarn("BLENetworkClient.peripheralManagerDidUpdateState.resetting", "%d", (int)peripheral.state);
      [self hostDisconnected];
      [self removeServiceAndCharacteristics];
      break;
      
    case CBPeripheralManagerStatePoweredOn:
      if(_available) {
        [self startAdvertising];
      }
      break;
      
    default:
      break;
  }
}

// someone connected to us
- (void)peripheralManager:(CBPeripheralManager *)peripheral central:(CBCentral *)central didSubscribeToCharacteristic:(CBCharacteristic *)characteristic {
  if ( _subscribedCentral && !(_subscribedCentral == central || [_subscribedCentral.identifier isEqual:central.identifier]) ) {
    // We only support connecting to 1 central
    // Ignore subscriptions from other centrals if someone has already connected to us.
    BLELogDebug("BLENetworkClient.centralSubscribed.ignoreCentral", "%s", central.description.UTF8String);
    return;
  }

  // If we subscribe to a characteristic:
  // store the central and use it to verify that both characteristics are from the same central.
  if([[characteristic UUID] isEqual:[self outboundCharacteristicUUID]]) {
    BLELogDebug("BLENetworkClient.centralSubsubscribed", "outbound");
    _subscribedToServerCharacteristic = characteristic;
    _subscribedCentral = central;
  }
  else if([[characteristic UUID] isEqual:[self disconnectCharacteristicUUID]]) {
    BLELogDebug("BLENetworkClient.centralSubsubscribed", "disconnect");
    _subscribedDisconnectCharacteristic = characteristic;
    _subscribedCentral = central;
  }
  
  if(_subscribedToServerCharacteristic && _subscribedDisconnectCharacteristic) {
    [self hostConnected:_subscribedCentral];
  }
}

// someone disconnected from us
- (void)peripheralManager:(CBPeripheralManager *)peripheral central:(CBCentral *)central didUnsubscribeFromCharacteristic:(CBCharacteristic *)characteristic {
  BLELogDebug("BLENetworkClient.centralUnsubscribed", "%s", central.description.UTF8String);
  
  if([characteristic.UUID isEqual:[self outboundCharacteristicUUID]]) {
    BLELogDebug("BLENetworkClient.centralUnsubscribed.outboundCharacteristic", "");
  }
  else if([characteristic.UUID isEqual:[self inboundCharacteristicUUID]]) {
    BLELogDebug("BLENetworkClient.centralUnsubscribed.inboundCharacteristic", "");
  }
  else if([characteristic.UUID isEqual:[self disconnectCharacteristicUUID]]) {
    BLELogDebug("BLENetworkClient.centralUnsubscribed.disconnectCharacteristic", "");
  }
  else {
    BLELogDebug("BLENetworkClient.centralUnsubscribed.unknownCharacteristic", "");
  }
  
  if(_connectedCentral && ((_connectedCentral == central) || [central.identifier isEqual:_connectedCentral.identifier])) {
    [self hostDisconnected];
  }
}

// someone is writing to us
- (void)peripheralManager:(CBPeripheralManager *)peripheral didReceiveWriteRequests:(NSArray *)requests {
  for (CBATTRequest* request in requests) {
    BLELogDebug("BLENetworkClient.receivedWriteRequest", "%s", request.description.UTF8String);
    if (!_connectedCentral
        || ((_connectedCentral != [request central]) && ![[request central].identifier isEqual:_connectedCentral.identifier]) ) {
      // Stranger Danger!  We got a write request from someone who is not the host.
      BLELogDebug("BLENetworkClient.receivedWriteRequest.unknownHost", "%s", request.central.description.UTF8String);
      [peripheral respondToRequest:request withResult:CBATTErrorWriteNotPermitted];
      return;
    }
    [peripheral respondToRequest:request withResult:CBATTErrorSuccess];
    if([request.value isEqualToData:_BLENetworkingInterfaceDisconnectMessage()]) {
      // We got a disconnect message, start the disconnect process.
      [self disconnectFromEndpointID:_hostID];
    }
    else if(!_isDisconnecting) {
      // If we're disconnecting, sending messages to the client could cause massive confusion.
      [self.delegate BLENetworkInterface:self receivedData:request.value fromEndpoint:_hostID];
    }
  }
}

-(void)peripheralManager:(CBPeripheralManager *)peripheral didReceiveReadRequest:(CBATTRequest *)request {
  BLELogWarn("BLENetworkClient.receivedReadRequest", "%s", request.description.UTF8String);
  // Someone is doing a polling read?
  // This should never happen.
  NSData* initialData = [NSData data];
  request.value = initialData;
  [peripheral respondToRequest:request withResult:CBATTErrorReadNotPermitted];
}

-(void)peripheralManagerIsReadyToUpdateSubscribers:(CBPeripheralManager *)peripheral {
  BLELogDebug("BLENetworkClient.peripheralManagerIsReadyToUpdateSubscribers", "");
  _canSendData = YES;
  [self sendBufferedMessages];
}

@end
