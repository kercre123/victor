//
//  BLECozmoManager.m
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 4/13/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#import "BLECozmoManager.h"
#import "BLECentralMultiplexer.h"
#import "BLECentralServiceDescription.h"
#import "BLECozmoConnection.h"
#import "BLELog.h"

#import <mach/mach_time.h>

#include "BLECozmoMessage.h"

UInt64 BLE_TimeInNanoseconds() {
  static mach_timebase_info_data_t machTimebase;
  if (machTimebase.denom == 0) {
    mach_timebase_info(&machTimebase);
  }
  uint64_t absTime = mach_absolute_time();
  return absTime * machTimebase.numer / machTimebase.denom;
}


#pragma mark -
#pragma mark BLECozmoManager

@interface BLECozmoManager () <BLECentralServiceDelegate> {
  BLECentralMultiplexer* _centralMultiplexer;
  BLECentralServiceDescription* _serviceDescription;
}
@property (nonatomic, strong) NSMutableDictionary *connectionsByPeripheralId;
@property (nonatomic, strong) NSMutableDictionary *connectionsByMfgId;

@end


@implementation BLECozmoManager

#pragma mark - Property Synthesizers
@synthesize vehicleConnections;
@synthesize pendingPeripheralConnections;
@synthesize connectionsByMfgId = _connectionsByMfgId;
@synthesize connectionsByPeripheralId = _connectionsByPeripheralId;

#pragma mark - Initialization

#define ANKI_STR_SERVICE_UUID @"763dbeef-5df1-405e-8aac-51572be5bab3"
#define ANKI_STR_CHR_TOPHONE_UUID @"763dbee0-5df1-405e-8aac-51572be5bab3"
#define ANKI_STR_CHR_TOCAR_UUID @"763dbee1-5df1-405e-8aac-51572be5bab3"
#define BLECozmoManagerServiceName @"BLECozmoService"


+(CBUUID*)inboundCharacteristicID {
  static CBUUID* characteristicID = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    characteristicID = [CBUUID UUIDWithString:ANKI_STR_CHR_TOPHONE_UUID];
  });
  return characteristicID;
}

+(CBUUID*)outboundCharacteristicID {
  static CBUUID* characteristicID = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    characteristicID = [CBUUID UUIDWithString:ANKI_STR_CHR_TOCAR_UUID];
  });
  return characteristicID;
}

-(BLECentralServiceDescription*)serviceDescription {
  if(!_serviceDescription) {
    _serviceDescription = [[BLECentralServiceDescription alloc] init];
    _serviceDescription.serviceID = [CBUUID UUIDWithString:ANKI_STR_SERVICE_UUID];
    _serviceDescription.characteristicIDs = @[[BLECozmoManager inboundCharacteristicID], [BLECozmoManager outboundCharacteristicID]];
    _serviceDescription.delegate = self;
    _serviceDescription.name = BLECozmoManagerServiceName;
  }
  return _serviceDescription;
}

-(id)init {
  [NSException raise:@"com.anki.BLECozmoManager.init" format:@"Initialization of BLEManager requires a BLECentralMultiplexer!"];
  return nil;
}

-(id)initWithCentralMultiplexer:(BLECentralMultiplexer*)centralMultiplexer
                          queue:(dispatch_queue_t)queue
{
  NSAssert(centralMultiplexer != nil, @"BLECozmoManager requires a BLECentralMultiplexer!");
  if((self = [super init])) {
    _centralMultiplexer = centralMultiplexer;
    [_centralMultiplexer registerService:[self serviceDescription]];
    
    self.queue = queue;
    
    self.vehicleConnections = [NSMutableArray array];
    self.connectionsByMfgId = [NSMutableDictionary dictionaryWithCapacity:256];
    self.connectionsByPeripheralId = [NSMutableDictionary dictionaryWithCapacity:256];
  }
  return self;
}

-(void)dealloc {
  [_centralMultiplexer cancelService:[self serviceDescription]];
}

#pragma mark - Connection Lookup

-(BLECozmoConnection *)connectionForPeripheral:(CBPeripheral *)peripheral {
  if ( !peripheral )
    return nil;
  
  return self.connectionsByPeripheralId[peripheral.identifier];
}

-(BLECozmoConnection *)connectionForMfgID:(UInt64)mfgID {
  return self.connectionsByMfgId[@(mfgID)];
}

#pragma mark - Add / Remove Vehicles

- (void)addVehicleConnection:(BLECozmoConnection *)connection {
  BLECozmoConnection *existingConnection = [self connectionForPeripheral:connection.carPeripheral];
  if ( existingConnection ) {
    BLELogError("BLECozmoManager.addVehicle.duplicateVehicle", "0x%llx", connection.mfgID);
  }
  
  BLELogDebug("BLECozmoManager.addVehicle", "0x%llx", connection.mfgID);
  
  [self willChangeValueForKey:@"vehicleConnections"];
  [vehicleConnections addObject:connection];
  self.connectionsByMfgId[@(connection.mfgID)] = connection;
  self.connectionsByPeripheralId[connection.carPeripheral.identifier] = connection;
  [self didChangeValueForKey:@"vehicleConnections"];
}

- (void)removeVehicleConnection:(BLECozmoConnection *)connection {
  [connection clearAdvertisementState];
  if (connection && connection.connectionState <= kDisconnected) {
    BLELogDebug("BLECozmoManager.removeVehicle", "0x%llx", connection.mfgID);
    connection.connectionState = kUnavailable;
    
    [self willChangeValueForKey:@"vehicleConnections"];
    [vehicleConnections removeObject:connection];
    [self.connectionsByPeripheralId removeObjectForKey:connection.carPeripheral.identifier];
    [self.connectionsByMfgId removeObjectForKey:@(connection.mfgID)];
    [self didChangeValueForKey:@"vehicleConnections"];
  }
}

- (void)removeVehicleConnectionForPeripheral:(CBPeripheral *)peripheral {
  BLECozmoConnection *connection = [self connectionForPeripheral:peripheral];
  if ( !connection ) {
    BLELogWarn("BLECozmoManager.removeVehicle.noConnectionForPeripheral", "%s(%p)", peripheral.name.UTF8String, peripheral);
    return;
  }
  
  [self removeVehicleConnection:connection];
}

#pragma mark - Vehicle Discovery
- (void)discoverVehicles {
  [_centralMultiplexer registerService:[self serviceDescription]];
  
  // Only start scanning if we're not not scanning for vehicles
  if (![_centralMultiplexer isScanningForService:[self serviceDescription]]) {
    [_centralMultiplexer startScanningForService:[self serviceDescription]];
  }
}

- (void)stopDiscoveringVehicles {
  if ([_centralMultiplexer isScanningForService:[self serviceDescription]]) {
    [_centralMultiplexer stopScanningForService:[self serviceDescription]];
    [self clearVehicleAdvertisements];
  }
}

// Returns YES if we are already scanning for vehicles, otherwise NO.
// Note that a YES return value only reflects that `discoverVehicles` was called
// and `stopDiscoveringVehicles` has not been called. This method does not account
// for whether the multiplexer maybe have paused scanning.
- (BOOL)isDiscoveringVehicles {
  return [_centralMultiplexer isScanningForService:[self serviceDescription]];
}

#pragma mark - BLECozmoServiceDelegate notifications

-(void)notifyVehicleDidAppear:(BLECozmoConnection *)connection {
  [self.serviceDelegate vehicleManager:self vehicleDidAppear:connection];
}

-(void)notifyVehicleDidDisappear:(BLECozmoConnection *)connection {
  [self.serviceDelegate vehicleManager:self vehicleDidDisappear:connection];
}

-(void)notifyVehicleDidConnect:(BLECozmoConnection *)connection {
  [self.serviceDelegate vehicleManager:self vehicleDidConnect:connection];
}

-(void)notifyVehicleDidDisconnect:(BLECozmoConnection *)connection {
  [self.serviceDelegate vehicleManager:self vehicleDidDisconnect:connection];
}

-(void)notifyVehicleDidUpdateAdvertisement:(BLECozmoConnection *)connection {
  [self.serviceDelegate vehicleManager:self vehicleDidUpdateAdvertisement:connection];
}

-(void)notifyVehicleDidUpdateProximity:(BLECozmoConnection *)connection {
  [self.serviceDelegate vehicleManager:self vehicleDidUpdateProximity:connection];
}

-(void)notfiyVehicle:(BLECozmoConnection *)connection didReceiveMessage:(NSData *)message {
  [self.serviceDelegate vehicleManager:self vehicle:connection didReceiveMessage:message];
}

#pragma mark - BLECentralServiceDelegate callbacks

-(void)service:(BLECentralServiceDescription *)service didDiscoverPeripheral:(CBPeripheral *)peripheral withRSSI:(NSNumber *)rssiVal advertisementData:(NSDictionary *)advertisementData {
  BLECozmoConnection* vehicleConn = [self connectionForPeripheral:peripheral];
  if(!vehicleConn) {
    vehicleConn = [[BLECozmoConnection alloc] initWithPeripheral:peripheral vehicleManager:self];
    // vehicleID is decided when we add the vehicle.
    vehicleConn.connectionState = kUnavailable;
    vehicleConn.mfgData = advertisementData[CBAdvertisementDataManufacturerDataKey];
    [self addVehicleConnection:vehicleConn];
    BLELogDebug("BLECozmoManager.initialRSSI", "0x%llx = %ld", vehicleConn.mfgID, (long)rssiVal.integerValue);
  }
  
  vehicleConn.scannedRSSI = rssiVal.integerValue;
  [vehicleConn updateWithAdvertisementData:advertisementData];
  
  if (vehicleConn.connectionState == kUnavailable) {
    vehicleConn.connectionState = kDisconnected;
    [self notifyVehicleDidAppear:vehicleConn];
  } else if (vehicleConn.connectionState == kDisconnected) {
    BLELogDebug("BLECozmoManager.didDiscoverPeripheral.updatingAdvertisingData", "%s", vehicleConn.description.UTF8String);
    BLELogDebug("BLECozmoManager.advertisedRSSI", "0x%llx = %ld", vehicleConn.mfgID, (long)rssiVal.integerValue);
    [self notifyVehicleDidUpdateAdvertisement:vehicleConn];
  } else {
    // This can happen during if we are scanning and request to disconnect a vehicle.
    // We will receive this callback before disconnecting.
    BLELogDebug("BLECozmoManager.didDiscoverPeripheral", "0x%llx received advertisement in connecting state [%d]", vehicleConn.mfgID, vehicleConn.connectionState);
  }
}

-(void)service:(BLECentralServiceDescription *)service didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
  BLECozmoConnection* connection = [self connectionForPeripheral:peripheral];
  if (connection) {
    
    // record the connection state before we change it
    BLEConnectionState connectedState = connection.connectionState;
    
    // When we disconnect, we move through 2 states: disconnected -> unavailable.
    // This is because we expire advertisement upon connection, so after disconnection,
    // we need to report that the vehicle is "unavailable" until we update it from a future scan.
    [self setVehicleConnectionState:peripheral connectionState:kDisconnected];
    
    [self setVehicleConnectionState:peripheral connectionState:kUnavailable];
    
    [self notifyVehicleDidDisconnect:connection];
    
    if (connectedState != kConnectedPipeNotReady) {
      // If we receive this callback, but the vehicle was not previously connected,
      // it means we likely failed due to a connection timeout.
      // TODO: consider creating specific error messages to indicate the disconnection
      //       reason in BLECentralMultiplexer
      BLELogEvent("BLECozmoManager.didDisconnectPeripheral.forgetVehicleConnection",
                  "0x%llx - received disconnect from non-connected state", connection.mfgID);
      [self removeVehicleConnection:connection];
    }
  }
}

-(void)service:(BLECentralServiceDescription *)service discoveredCharacteristic:(CBCharacteristic *)characteristic forPeripheral:(CBPeripheral *)peripheral {
  BLECozmoConnection *vehicleConn = [self connectionForPeripheral:peripheral];
  if ( !vehicleConn )
    return;
  
  if([[characteristic UUID] isEqual:[BLECozmoManager outboundCharacteristicID]]) {
    vehicleConn.toCarCharacteristic = characteristic;
    CBCharacteristic *toPhoneCharacteristic = vehicleConn.toPhoneCharacteristic;
    if(toPhoneCharacteristic && toPhoneCharacteristic.isNotifying) {
      BLELogDebug("BLECozmoManager.discoveredCharacteristic", "toCar && toPhone.isNotifying");
    } else {
      BLELogDebug("BLECozmoManager.discoveredCharacteristic", "toCar");
    }
  }
  else if([[characteristic UUID] isEqual:[BLECozmoManager inboundCharacteristicID]]) {
    BLELogDebug("BLECozmoManager.discoveredCharacteristic", "toPhone");
    if (!characteristic.isNotifying) {
      [peripheral setNotifyValue:YES forCharacteristic:characteristic];
    }
  }
  
  if ([self isVehicleConnectionReady:vehicleConn]) {
    [self setVehicleConnectionState:peripheral connectionState:kConnectedPipeReady];
    // Upon connection, the vehicle stops advertisement, so we clear the advertisement state.
    [vehicleConn clearAdvertisementState];
  } else {
    [self setVehicleConnectionState:peripheral connectionState:kConnectedPipeNotReady];
  }
}

- (BOOL)isVehicleConnectionReady:(BLECozmoConnection *)connection {
  return (connection.toCarCharacteristic && connection.toPhoneCharacteristic && connection.toPhoneCharacteristic.isNotifying);
}

-(void)service:(BLECentralServiceDescription *)service didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic forPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
  BLECozmoConnection *vehicleConn = [self connectionForPeripheral:peripheral];
  if (!vehicleConn)
    return;
  if([[characteristic UUID] isEqual:[BLECozmoManager inboundCharacteristicID]]) {
    vehicleConn.toPhoneCharacteristic = characteristic;
    if(vehicleConn.toCarCharacteristic) {
      BLELogDebug("BLECozmoManager.didUpdateNotificationState", "toCar && toPhone.isNotifying");
    }
  }
  
  if ([self isVehicleConnectionReady:vehicleConn]) {
    [self setVehicleConnectionState:peripheral connectionState:kConnectedPipeReady];
    
    // Upon connection, the vehicle stops advertisement, so we clear the advertisement state.
    [vehicleConn clearAdvertisementState];
  }
}

-(void)service:(BLECentralServiceDescription *)service incomingMessage:(NSData *)message onCharacteristic:(CBCharacteristic*)characteristic forPeripheral:(CBPeripheral *)peripheral {
  BLECozmoConnection* vehicleConnection = [self connectionForPeripheral:peripheral];
  if ( !vehicleConnection )
    return;
  
  static Anki::Cozmo::BLECozmoMessage cozmoMessage{};
  BLEAssert(!cozmoMessage.IsMessageComplete());
  BLEAssert(Anki::Cozmo::BLECozmoMessage::kMessageExactMessageLength == message.length);
  if (!cozmoMessage.AppendChunk(static_cast<const uint8_t*>(message.bytes), (uint32_t)message.length))
  {
    BLEAssert(false);
  }
  
  if (cozmoMessage.IsMessageComplete())
  {
    uint8_t completeMessage[Anki::Cozmo::BLECozmoMessage::kMessageMaxTotalSize];
    uint8_t numBytes = cozmoMessage.DechunkifyMessage(completeMessage, Anki::Cozmo::BLECozmoMessage::kMessageMaxTotalSize);
    
    // TODO:(lc) do something with incoming messages besides printing them and clearing
    NSMutableString *string = [NSMutableString stringWithCapacity:numBytes*2];
    for (NSInteger idx = 0; idx < numBytes; ++idx) {
      [string appendFormat:@"%02x", completeMessage[idx]];
    }
    BLELogDebug("BLECozmoManager.incomingMessage","Received: 0x%s", [string UTF8String]);
    
    [self notfiyVehicle:vehicleConnection didReceiveMessage:[[NSData alloc] initWithBytes:completeMessage length:numBytes]];
    cozmoMessage.Clear();
  }
}

-(void)service:(BLECentralServiceDescription *)service peripheral:(CBPeripheral *)peripheral didUpdateRSSI:(NSNumber *)rssiVal {
  BLECozmoConnection *vehicleConn = [self connectionForPeripheral:peripheral];
  if (!vehicleConn)
    return;
  
  vehicleConn.scannedRSSI = rssiVal.integerValue;
  BLELogDebug("BLECozmoManager.advertisedRSSI", "0x%llx = %ld", vehicleConn.mfgID, (long)rssiVal.integerValue);
  [self notifyVehicleDidUpdateProximity:vehicleConn];
}

-(void)service:(BLECentralServiceDescription*)service peripheral:(CBPeripheral*)peripheral didUpdateAdvertisementData:(NSDictionary*)advertisementData {
  BLECozmoConnection *vehicleConn = [self connectionForPeripheral:peripheral];
  if (!vehicleConn)
    return;
  
  [vehicleConn updateWithAdvertisementData:advertisementData];
  [self notifyVehicleDidUpdateAdvertisement:vehicleConn];
}

-(void)service:(BLECentralServiceDescription*)service peripheral:(CBPeripheral*)peripheral didWriteValueForCharacteristic:(CBCharacteristic *)characteristic
         error:(NSError *)error {
  // We only care about this if it's an error..
  if ( !error )
    return;
  
  BLECozmoConnection *vehicleConn = [self connectionForPeripheral:peripheral];
  if (vehicleConn) {
    // We hit an error, scream and tear this connection down.
    BLELogError("BLECozmoManager.didWriteValueForCharacteristic.error", "vehicle=%s error=%s", vehicleConn.description.UTF8String, error.description.UTF8String);
    [_centralMultiplexer disconnectPeripheral:peripheral withService:service];
  }
}

-(void)service:(BLECentralServiceDescription *)service peripheralDidDisappear:(CBPeripheral *)peripheral {
  BLELogDebug("BLECozmoManager.peripheralDidDisappear", "peripheral=%s", [[peripheral.identifier UUIDString] UTF8String]);
  BLECozmoConnection *connection = [self connectionForPeripheral:peripheral];
  if (connection && connection.connectionState <= kDisconnected) {
    [self setVehicleConnectionState:peripheral connectionState:kUnavailable];
    [self notifyVehicleDidDisappear:connection];
    [connection clearAdvertisementState];
  }
}

#pragma mark - Connect / Disconnect Vehicles

- (void)connectPeripheralForVehicleConnection:(BLECozmoConnection *)connection {
  BLEAssert(connection != nil);
  
  // only connect to devices that aren't already in some phase of connection
  if (connection.connectionState > kDisconnected) {
    return;
  }
  
  [self setVehicleConnectionState:connection.carPeripheral connectionState:kPending];
  [_centralMultiplexer connectPeripheral:connection.carPeripheral withService:[self serviceDescription]];
}

- (BOOL) connectToVehicleByMfgID:(UInt64)mfgID {
  BLECozmoConnection *vehicleConn = [self connectionForMfgID:mfgID];
  if ( !vehicleConn ) {
    BLELogError("BLECozmoManager.connectToVehicleByMFGID.unknownID", "0x%llx", mfgID);
  } else {
    [self connectPeripheralForVehicleConnection:vehicleConn];
  }
  return (nil != vehicleConn);
}

- (BOOL)disconnectVehicleWithMfgId:(UInt64)mfgID
            sendDisconnectMessages:(BOOL)sendDisconnectMessages;
{
  BLECozmoConnection *vehicleConn = [self connectionForMfgID:mfgID];
  if (!vehicleConn) {
    return NO;
  }
  
  [self disconnectConnection:vehicleConn];
  
  return YES;
}

- (void)disconnectConnection:(BLECozmoConnection *)connection;
{
  BLELogEvent("BLECozmoManager.disconnectConnection",
              "0x%llx connectionState: %d",
              connection.mfgID, connection.connectionState);
  
  if (connection.connectionState > kDisconnected ) {
    [_centralMultiplexer disconnectPeripheral:connection.carPeripheral withService:[self serviceDescription]];
  }
}

#pragma mark - Vehicle I/O

- (void)setVehicleConnectionState:(CBPeripheral * )peripheral connectionState:(BLEConnectionState)connectionState
{
  BLECozmoConnection *vehicleConn = [self connectionForPeripheral:peripheral];
  if ( !vehicleConn ) {
    BLELogError("BLECozmoManager.setVehicleConnectionState.unknownPeripheral", "%p", peripheral);
    return;
  }
  if (vehicleConn.connectionState > kDisconnected) {
    BLELogInfo("BLECozmoManager.setVehicleConnectionState", "vehicle=0x%llx state=%d", vehicleConn.mfgID, connectionState);
  }
  vehicleConn.connectionState = connectionState;
  if (vehicleConn.connectionState == kConnectedPipeReady) {
    [self notifyVehicleDidConnect:vehicleConn];
  }
}

# pragma mark - Vehicle Connections cleanup utilities

- (void)clearVehicleAdvertisements {
  [_connectionsByMfgId enumerateKeysAndObjectsUsingBlock:^(NSNumber *mfgId, BLECozmoConnection *connection, BOOL *stop) {
    [connection clearAdvertisementState];
  }];
}

- (void)clearDisconnectedVehicles {
  NSMutableArray* vehiclesToClear = [NSMutableArray array];
  for (BLECozmoConnection* vehicleConn in self.vehicleConnections) {
    if(vehicleConn.connectionState <= kDisconnected) {
      [vehiclesToClear addObject:vehicleConn];
    }
  }
  for (BLECozmoConnection* conn in vehiclesToClear) {
    [self removeVehicleConnection:conn];
  }
}

- (void)clearAllVehicles:(BOOL)sendDisconnectMessages
{
  [pendingPeripheralConnections removeAllObjects];
  
  for (BLECozmoConnection *vehicleConnection in vehicleConnections.copy) {
    [self removeVehicleConnection:vehicleConnection];
  }
}

@end

