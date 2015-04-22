/**
 * File: BLEVehicleManager.mm
 *
 * Author: Mark Palatucci (markmp)
 * Created: 7/11/2012
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description: Objective-C class that implements functions called
 * by the CoreBluetooth delegates, and manages the relationships between
 * peripherals, services, characteristics, and vehicleIDs. This will
 * interface with the C++ object that the basestation uses for messaging.
 *
 * Copyright: Anki, Inc. 2008-2012
 *
 **/

#import "BLEManager/BLEVehicleManager.h"
#import "BLEManager/Central/BLECentralMultiplexer.h"
#import "BLEManager/Central/BLECentralServiceDescription.h"
#import "BLEManager/Logging/BLELog.h"

#import <mach/mach_time.h>

UInt64 BLE_TimeInNanoseconds() {
  static mach_timebase_info_data_t machTimebase;
  if (machTimebase.denom == 0) {
    mach_timebase_info(&machTimebase);
  }
  uint64_t absTime = mach_absolute_time();
  return absTime * machTimebase.numer / machTimebase.denom;
}

#pragma mark -
#pragma mark BLEVehicleManager
/****************************************************************************/
/*								BLEVehicleManager		                  								        */
/****************************************************************************/

@interface BLEVehicleManager () <BLECentralServiceDelegate> {
  BLECentralMultiplexer* _centralMultiplexer;
  BLEVehicleConnection* _connectionsByID[256];
  BLECentralServiceDescription* _serviceDescription;
}
@property (nonatomic, strong) NSMutableDictionary *connectionsByPeripheral;
@property (nonatomic, strong) NSMutableDictionary *connectionsByMfgId;
@end

@interface BLEConnectionNode : NSObject {
  BLEVehicleConnection *connection;
  uint64_t uniqueId;
  
}
@end

#define PeripheralAddress(peripheral) ([NSValue valueWithNonretainedObject:(peripheral)])

@implementation BLEVehicleManager

#pragma mark - Property Synthesizers
@synthesize vehicleConnections;
@synthesize vehicleMessages;
@synthesize basestationMode;
@synthesize pendingPeripheralConnections;
@synthesize availabilityDelegate;
@synthesize connectionsByMfgId = _connectionsByMfgId;
@synthesize connectionsByPeripheral = _connectionsByPeripheral;

#pragma mark - Initialization
// FIXME: consider getting rid of singleton
+ (BLEVehicleManager*) sharedInstance
{
	static BLEVehicleManager	*vehicleMgr	= nil;

  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    vehicleMgr = [[BLEVehicleManager alloc] initWithCentralMultiplexer:[BLECentralMultiplexer sharedInstance]];
  });
  
	return vehicleMgr;
}

#define ANKI_STR_SERVICE_UUID @"BE15BEEF-6186-407E-8381-0BD89C4D8DF4"
#define ANKI_STR_CHR_TOPHONE_UUID @"BE15BEE0-6186-407E-8381-0BD89C4D8DF4"
#define ANKI_STR_CHR_TOCAR_UUID @"BE15BEE1-6186-407E-8381-0BD89C4D8DF4"
#define BLEVehicleManagerServiceName @"BLEVehicleService"

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
    _serviceDescription.characteristicIDs = @[[BLEVehicleManager inboundCharacteristicID], [BLEVehicleManager outboundCharacteristicID]];
    _serviceDescription.delegate = self;
    _serviceDescription.name = BLEVehicleManagerServiceName;
  }
  return _serviceDescription;
}

-(id)init {
  [NSException raise:@"com.anki.BLEVehicleManager.init" format:@"Initialization of BLEManager requires a BLECentralMultiplexer!"];
  return nil;
}

-(id)initWithCentralMultiplexer:(BLECentralMultiplexer*)centralMultiplexer {
  NSAssert(centralMultiplexer != nil, @"BLEVehicleManager requires a BLECentralMultiplexer!");
  if((self = [super init])) {
    _centralMultiplexer = centralMultiplexer;

    self.basestationMode = FALSE;
    self.vehicleConnections = [NSMutableArray array];
    self.connectionsByMfgId = [NSMutableDictionary dictionaryWithCapacity:256];
    self.connectionsByPeripheral = [NSMutableDictionary dictionaryWithCapacity:256];
    
    self.vehicleMessages = [NSMutableArray array];
  }
  return self;
}

-(void)dealloc {
  [_centralMultiplexer cancelService:[self serviceDescription]];
}

#pragma mark - Connection Lookup

-(BLEVehicleConnection *)connectionForPeripheral:(CBPeripheral *)peripheral {
  if ( !peripheral )
    return nil;
  
  return self.connectionsByPeripheral[PeripheralAddress(peripheral)];
}

-(BLEVehicleConnection *)connectionForMfgID:(UInt64)mfgID {
  return self.connectionsByMfgId[@(mfgID)];
}

-(BLEVehicleConnection *)connectionForVehicleID:(NSInteger)vehicleID {
  // We should replace this with a more scalable version that doesn't search
  // through the list of connections each time
  assert(vehicleID >= 0);
  assert(vehicleID < 256);
  
  return _connectionsByID[vehicleID];
}

#pragma mark - Add / Remove Vehicles

- (void)addVehicleConnection:(BLEVehicleConnection *)connection {
  BLEVehicleConnection *existingConnection = [self connectionForPeripheral:connection.carPeripheral];
  if ( existingConnection ) {
    BLELogError("BLEVehicleManager.addVehicle.duplicateVehicle", "0x%llx", connection.mfgID);
  }
  
  // TODO: move this nonsense into the bleComms.mm class.  It is the thing that should manage basestation mappings.
  for(int i = 1; i < 256; i++) { // carComms.h doesn't like us to start with zero.  Zero is a sentinel value.
    if(nil == _connectionsByID[i]) {
      connection.vehicleID = i;
      _connectionsByID[i] = connection;
      break;
    }
  }
  BLEAssert(connection.vehicleID < 255); // this is the last one, after this, we have 255 vehicles and can't talk about them all..
  BLELogDebug("BLEVehicleManager.addVehicle", "0x%llx", connection.mfgID);
  
  [self willChangeValueForKey:@"vehicleConnections"];
  [vehicleConnections addObject:connection];
  self.connectionsByMfgId[@(connection.mfgID)] = connection;
  self.connectionsByPeripheral[PeripheralAddress(connection.carPeripheral)] = connection;
  [self didChangeValueForKey:@"vehicleConnections"];
  [self.availabilityDelegate vehicleManager:self vehicleBecameAvailable:connection];
}

- (void)removeVehicleConnection:(BLEVehicleConnection *)connection {
  [connection clearAdvertisementState];
  if (connection && connection.connectionState <= kDisconnected) {
    BLELogDebug("BLEVehicleManager.removeVehicle", "0x%llx", connection.mfgID);
    connection.connectionState = kUnavailable;
    
    [self willChangeValueForKey:@"vehicleConnections"];
    [vehicleConnections removeObject:connection];
    [self.connectionsByPeripheral removeObjectForKey:PeripheralAddress(connection.carPeripheral)];
    [self.connectionsByMfgId removeObjectForKey:@(connection.mfgID)];
    if(connection.vehicleID <= 255) {
      _connectionsByID[connection.vehicleID] = nil;
    }
    [self didChangeValueForKey:@"vehicleConnections"];
    [self.availabilityDelegate vehicleManager:self vehicleBecameUnavailable:connection];
  }
}

- (void)removeVehicleConnectionForPeripheral:(CBPeripheral *)peripheral {
  BLEVehicleConnection *connection = [self connectionForPeripheral:peripheral];
  if ( !connection ) {
    BLELogWarn("BLEVehicleManager.removeVehicle.noConnectionForPeripheral", "%s(%p)", peripheral.name.UTF8String, peripheral);
    return;
  }
  
  [self removeVehicleConnection:connection];
}

#pragma mark - Vehicle Discovery
- (void) discoverVehicles {
  [_centralMultiplexer registerService:[self serviceDescription]];
  [_centralMultiplexer startScanningForService:[self serviceDescription]];
}

- (void) stopDiscoveringVehicles {
  [_centralMultiplexer stopScanningForService:[self serviceDescription]];
  [self clearDisconnectedVehicles];
}


#pragma mark - BLE Service delegate callbacks

-(void)service:(BLECentralServiceDescription *)service didDiscoverPeripheral:(CBPeripheral *)peripheral withRSSI:(NSNumber *)rssiVal advertisementData:(NSDictionary *)advertisementData {
  BLEVehicleConnection* vehicleConn = [self connectionForPeripheral:peripheral];
  if(!vehicleConn) {
    vehicleConn = [[BLEVehicleConnection alloc] initWithPeripheral:peripheral];
    // vehicleID is decided when we add the vehicle.
    vehicleConn.scannedRSSI = rssiVal.integerValue;
    vehicleConn.connectionState = kDisconnected;
    vehicleConn.mfgData = advertisementData[CBAdvertisementDataManufacturerDataKey];
    [vehicleConn updateWithAdvertisementData:advertisementData];
    [self addVehicleConnection:vehicleConn];
    BLELogDebug("BLEVehicleManager.initialRSSI", "0x%llx = %ld", vehicleConn.mfgID, (long)rssiVal.integerValue);
  }
  else {
    BLELogDebug("BLEVehicleManager.didDiscoverPeripheral.updatingAdvertisingData", "%s", vehicleConn.description.UTF8String);
    vehicleConn.scannedRSSI = rssiVal.integerValue;
    BLELogDebug("BLEVehicleManager.advertisedRSSI", "0x%llx = %ld", vehicleConn.mfgID, (long)rssiVal.integerValue);
    [vehicleConn updateWithAdvertisementData:advertisementData];
    [self.availabilityDelegate vehicleManager:self vehicleStatusChanged:vehicleConn];
  }
  BLELogDebug("BLEVehicleManager.advertisedRSSI", "0x%llx = %ld", vehicleConn.mfgID, (long)rssiVal.integerValue);
}

-(void)service:(BLECentralServiceDescription *)service didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
  [self setVehicleConnectionState:peripheral connectionState:kUnavailable];
  [self removeVehicleConnectionForPeripheral:peripheral];
}

-(void)service:(BLECentralServiceDescription *)service discoveredCharacteristic:(CBCharacteristic *)characteristic forPeripheral:(CBPeripheral *)peripheral {
  BLEVehicleConnection *vehicleConn = [self connectionForPeripheral:peripheral];
  if ( !vehicleConn )
    return;
  
  if([[characteristic UUID] isEqual:[BLEVehicleManager outboundCharacteristicID]]) {
    vehicleConn.toCarCharacteristic = characteristic;
    if(vehicleConn.toPhoneCharacteristic) {
      [self setVehicleConnectionState:peripheral connectionState:kConnectedPipeReady];
    }
  }
  else if([[characteristic UUID] isEqual:[BLEVehicleManager inboundCharacteristicID]]) {
    [self setVehicleConnectionState:peripheral connectionState:kConnectedPipeNotReady];
    [peripheral setNotifyValue:YES forCharacteristic:characteristic];
  }
}

-(void)service:(BLECentralServiceDescription *)service didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic forPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
  BLEVehicleConnection *vehicleConn = [self connectionForPeripheral:peripheral];
  if (!vehicleConn)
    return;
  if([[characteristic UUID] isEqual:[BLEVehicleManager inboundCharacteristicID]]) {
    vehicleConn.toPhoneCharacteristic = characteristic;
    if(vehicleConn.toCarCharacteristic) {
      // Upon connection, the vehicle stops advertisement, so we clear the advertisement state.
      [vehicleConn clearAdvertisementState];
      [self setVehicleConnectionState:peripheral connectionState:kConnectedPipeReady];
    }
  }
}

-(void)service:(BLECentralServiceDescription *)service incomingMessage:(NSData *)message onCharacteristic:(CBCharacteristic*)characteristic forPeripheral:(CBPeripheral *)peripheral {
  BLEVehicleConnection* vehicleConnection = [self connectionForPeripheral:peripheral];
  if ( !vehicleConnection )
    return;
  
  BLEVehicleMessage *vehicleMsg = [[BLEVehicleMessage alloc] initWithData:message];
  vehicleMsg.timestamp = BLE_TimeInNanoseconds();
  vehicleMsg.vehicleID = vehicleConnection.vehicleID;
  
  vehicleConnection.bytesReceived += (UInt32)message.length;
  vehicleConnection.messagesReceived++;
  
  // Set the unique mfg ID on the message
  vehicleMsg.uniqueVehicleID = [vehicleConnection mfgID];
  
  // Handle the incoming message.
  // We allow the vehicleConnection to mark the message as handled or we handle it
  // ourself by adding it to the message queue.
  BOOL handled = NO;
  if ( !([vehicleConnection sendMessageToResponder:vehicleMsg handled:&handled] && handled) ) {
    // The connection either has no responder or the responder indicated not to mark the message
    // as handled.  Handle it ourselves.
    [self handleIncomingVehicleMessage:vehicleMsg];
  }
}

- (void)handleIncomingVehicleMessage:(BLEVehicleMessage *)vehicleMsg {
  // Beware, the BLEComms apparently doesn't set itself up as a delegate so we are just dumping messages
  //  into this queue with no guarantee they'll ever be used.  We're essentially leaking the messages.
  [vehicleMessages addObject:vehicleMsg];
  [self.delegate incomingMessagesAvailable];
}

-(void)service:(BLECentralServiceDescription *)service peripheral:(CBPeripheral *)peripheral didUpdateRSSI:(NSNumber *)rssiVal {
  BLEVehicleConnection *vehicleConn = [self connectionForPeripheral:peripheral];
  if (!vehicleConn)
    return;
  
  vehicleConn.scannedRSSI = rssiVal.integerValue;
  BLELogDebug("BLEVehicleManager.advertisedRSSI", "0x%llx = %ld", vehicleConn.mfgID, (long)rssiVal.integerValue);
  if([self.availabilityDelegate respondsToSelector:@selector(vehicleManager:vehicleRSSIUpdated:)]) {
    [self.availabilityDelegate vehicleManager:self vehicleRSSIUpdated:vehicleConn];
  }
}

-(void)service:(BLECentralServiceDescription*)service peripheral:(CBPeripheral*)peripheral didUpdateAdvertisementData:(NSDictionary*)advertisementData {
  BLEVehicleConnection *vehicleConn = [self connectionForPeripheral:peripheral];
  if (!vehicleConn)
    return;
  
  [vehicleConn updateWithAdvertisementData:advertisementData];
  [self.availabilityDelegate vehicleManager:self vehicleStatusChanged:vehicleConn];
}

-(void)service:(BLECentralServiceDescription*)service peripheral:(CBPeripheral*)peripheral didWriteValueForCharacteristic:(CBCharacteristic *)characteristic
         error:(NSError *)error {
  // We only care about this if it's an error..
  if ( !error )
    return;
  
  BLEVehicleConnection *vehicleConn = [self connectionForPeripheral:peripheral];
  if (vehicleConn) {
    // We hit an error, scream and tear this connection down.
    BLELogError("BLEVehicleManager.didWriteValueForCharacteristic.error", "vehicle=%s error=%s", vehicleConn.description.UTF8String, error.description.UTF8String);
    [_centralMultiplexer disconnectPeripheral:peripheral withService:service];
  }
}

-(void)service:(BLECentralServiceDescription *)service peripheralDidDisappear:(CBPeripheral *)peripheral {
  BLELogDebug("BLEVehicleManager.peripheralDidDisappear", "peripheral=%p", peripheral);
  [self removeVehicleConnectionForPeripheral:peripheral];
}

#pragma mark - Connect / Disconnect Vehicles

- (void)connectPeripheralForVehicleConnection:(BLEVehicleConnection *)connection {
  BLEAssert(connection != nil);
  
  // only connect to devices that aren't already in some phase of connection
  if (connection.connectionState != kDisconnected) {
    return;
  }
  
  [self setVehicleConnectionState:connection.carPeripheral connectionState:kPending];
  [_centralMultiplexer connectPeripheral:connection.carPeripheral withService:[self serviceDescription]];
}

- (void) connectToVehicleByID:(NSInteger)vehicleID {
  BLEVehicleConnection *vehicleConn = [self connectionForVehicleID:vehicleID];
  if ( !vehicleConn ) {
    BLELogError("BLEVehicleManager.connectToVehicleByVehicleID.unknownID", "%ld", (long)vehicleID);
  } else {
    [self connectPeripheralForVehicleConnection:vehicleConn];
  }
}

- (void) connectToVehicleByMfgID:(UInt64)mfgID {
  BLEVehicleConnection *vehicleConn = [self connectionForMfgID:mfgID];
  if ( !vehicleConn ) {
    BLELogError("BLEVehicleManager.connectToVehicleByMFGID.unknownID", "0x%llx", mfgID);
  } else {
    [self connectPeripheralForVehicleConnection:vehicleConn];
  }
}

- (void)disconnectVehicle:(BLEVehicleConnection *)vehicleConn sendDisconnectMessages:(BOOL)sendDisconnectMessages
{
  BLELogEvent("BLEVehicleManager.disconnectVehicle", "0x%llx", vehicleConn.mfgID);
  if (sendDisconnectMessages && (vehicleConn.connectionState == kConnectedPipeReady)) {
    // Send message to car to tear down connection. We do this because the OS doesn't always
    // tear down the connection immediately in iOS 6 if we just call cancelPeriperalConnection
    NSError *error = nil;
    if ( ![vehicleConn disconnect:&error] ) {
      BLELogError("BLEVehicleManager.disconnectVehicle.unknownVehicle", "%s error=%s", vehicleConn.description.UTF8String, error.description.UTF8String);
    }
  }
  
  // If the pipe is not up but we've already called connect, then call the os disconnect cause
  // it's our best option
  if (sendDisconnectMessages && (vehicleConn.connectionState == kConnectedPipeNotReady ||
                                 vehicleConn.connectionState == kConnecting)) {
    [_centralMultiplexer disconnectPeripheral:vehicleConn.carPeripheral withService:[self serviceDescription]];
  }
}

#pragma mark - Vehicle I/O

- (void) writeData:(NSData *)data toVehicleByID:(NSInteger)vehicleID {
  BLEVehicleConnection *connection = [self connectionForVehicleID:vehicleID];
  [connection writeData:data error:NULL];
}

// call readRSSI on the peripheral to get the RSSI updated callback. This lets us read RSSI while
// connected (i.e. there are no advertisements available to provide RSSI)
- (void) readConnectedRSSI:(NSInteger)vehicleID {
  BLEVehicleConnection *vehicleConn = [self connectionForVehicleID:vehicleID];
  [vehicleConn readConnectedRSSI];
}

- (void)setVehicleConnectionState:(CBPeripheral * )peripheral connectionState:(BLEConnectionState)connectionState
{
  BLEVehicleConnection *vehicleConn = [self connectionForPeripheral:peripheral];
  if ( !vehicleConn ) {
    BLELogError("BLEVehicleManager.setVehicleConnectionState.unknownPeripheral", "%p", peripheral);
    return;
  }
  BLELogDebug("BLEVehicleManager.setVehicleConnectionState", "vehicle=0x%llx state=%d", vehicleConn.mfgID, connectionState);
  vehicleConn.connectionState = connectionState;
  [self.availabilityDelegate vehicleManager:self vehicleStatusChanged:vehicleConn];
}

# pragma mark - Vehicle Connections cleanup utilities

- (void) clearDisconnectedVehicles {
  NSMutableArray* vehiclesToClear = [NSMutableArray array];
  for (BLEVehicleConnection* vehicleConn in self.vehicleConnections) {
    if(vehicleConn.connectionState <= kDisconnected) {
      [vehiclesToClear addObject:vehicleConn];
    }
  }
  for (BLEVehicleConnection* conn in vehiclesToClear) {
    [self removeVehicleConnection:conn];
  }
}

- (void)clearAllVehicles:(BOOL)sendDisconnectMessages
{
  [vehicleMessages removeAllObjects];
  [pendingPeripheralConnections removeAllObjects];
  
  for (BLEVehicleConnection *vehicleConnection in vehicleConnections.copy) {
    [vehicleConnection disconnectFromPeripheral];
    [self removeVehicleConnection:vehicleConnection];
  }
}

@end
