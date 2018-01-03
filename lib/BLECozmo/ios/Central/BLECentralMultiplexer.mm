//
//  BLECentralMultiplexer.m
//  BLEManager
//
//  Created by Mark Pauley on 3/28/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#import "BLECentralMultiplexer.h"
#import "BLECentralServiceDescription.h"
#import "BLEAdvertisement.h"
#import "BLEPeripheralConnectionAttempt.h"
#import "BLELog.h"
#import <Availability.h>

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#import <UIKit/UIDevice.h>
#endif

//
// Use new naming (CBManagerState) when available.
// Fall back to old naming (CBCentralManagerState) on older MacOS.
//
#if (TARGET_OS_OSX && __MAC_OS_X_VERSION_MIN_REQUIRED < 101300)
#define CBManagerState CBCentralManagerState
#define CBManagerStateUnknown CBCentralManagerStateUnknown
#define CBManagerStateResetting CBCentralManagerStateResetting
#define CBManagerStateUnsupported CBCentralManagerStateUnsupported
#define CBManagerStateUnauthorized CBCentralManagerStateUnauthorized
#define CBManagerStatePoweredOn CBCentralManagerStatePoweredOn
#define CBManagerStatePoweredOff CBCentralManagerStatePoweredOff
#endif

NSString* const BLEManagerNotificationName = @"BLEManagerNotificationName";
NSString* const BLEManagerKeyPowerIsOn = @"BLEManagerKeyIsPowerOn";
NSString* const BLEManagerKeyBLEUnsupported = @"BLEManagerKeyIsBLEUnsupported";
NSString* const BLEManagerKeyBLEUnauthorized = @"BLEManagerKeyBLEUnauthorized";

NSString* const BLECentralMultiplexerIsWedgedNotification = @"BLECentralMultiplexerIsWedgedNotification";


#define BLE_CONNECT_NUM_RETRIES 10
#define BLE_CONNECT_RETRY_DELAY_MSECS 100
#define BLE_PERIPHERAL_ADVERT_SWEEP_TIMER_SECS 0.5f
#define BLE_PERIPHERAL_ADVERT_SWEEP_TIMER_LEEWAY_MSECS 30
#define BLE_PERIPHERAL_ADVERT_EXPIRATION_SECS_SMALL 1.25f
#define BLE_PERIPHERAL_ADVERT_EXPIRATION_SECS_LARGE 2.5f
#define BLE_PERIPHERAL_CONNECT_TIMEOUT_SECS 5.0f
#define BLE_PERIPHERAL_CONNECT_SWEEP_TIMER_SECS 0.5f
#define BLE_PERIPHERAL_CONNECT_SWEEP_TIMER_LEEWAY_MSECS 30
#define BLE_SECONDS_BEFORE_LONG_EXPIRATION 90.0f

#if TARGET_OS_MAC && !(TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
static const BOOL kOnMacOsX = YES;
#else
static const BOOL kOnMacOsX = NO;
#endif

@interface BLECentralMultiplexer () <CBCentralManagerDelegate,CBPeripheralDelegate>
@property (nonatomic, assign) BOOL debugTimeoutNextConnectionAttempt;
@end

static id<NSCopying> _addressOfPeripheral(CBPeripheral* peripheral) {
  return peripheral.identifier;
}

static const char* ScanningStateName(BLECentralMultiplexerScanningState state) {
  const char* stateName = "Unknown";
  switch (state) {
    case BLECentralMultiplexerScanningStateStopped:
      stateName = "Stopped";
      break;
    case BLECentralMultiplexerScanningStateScanning:
      stateName = "Scanning";
      break;
    case BLECentralMultiplexerScanningStatePaused:
      stateName = "Paused";
      break;
    default:
      break;
  }
  return stateName;
}

#pragma mark - BLECentralMultiplexer Object

@implementation BLECentralMultiplexer {
  CBCentralManager* _centralManager;
  CBManagerState _centralManagerState;
  BOOL _isWedged;
  
  // Service registration
  NSMutableDictionary* _registeredServicesByServiceID;
  
  // Scanning services
  NSMutableSet* _scanningServices;
  BOOL _isScanning;
  NSTimeInterval _pauseTimestamp;

  // Advertisement data
  BOOL _usePeripheralNameIfLocalNameIsAbsent;
  NSMutableDictionary* _advertisementsByPeripheralAddress; // BLEAdvertisement given a peripheral address
  NSMapTable* _servicesForPeripheral; // <CBPeripheral(weak) -> NSMutableSet of BLECentralServiceDescription(strong)> Used to map events back to delegates registered by Service ID.
  NSMutableSet* _peripheralsWaitingForData;
  
  // Advertisement expiry
  dispatch_source_t _advertSweepTimer;
  dispatch_source_t _connectionTimer;
  CFAbsoluteTime _scanningStartTime;
  
  // Connection queueing
  BLEPeripheralConnectionAttempt* _connectionAttemptInProgress;
  NSMutableArray*      _pendingConnections;
  
  // Connected peripherals
  NSMutableSet*        _connectedPeripherals;
  NSMutableSet*        _disconnectingPeripherals;
  
  dispatch_queue_t _dispatchQueue;
  void* IsOnBLEQueue;
}

@synthesize scanningState = _scanningState;

-(BOOL)isOnOurQueue
{
  return (dispatch_get_specific(&IsOnBLEQueue) == (__bridge void *) self);
}

-(void)assertOnOurQueue
{
  assert([self isOnOurQueue]);
}

-(void)runBlockOnOurQueueSync:(dispatch_block_t)block
{
  if ([self isOnOurQueue]) {
    block();
  } else {
    dispatch_sync(_dispatchQueue, block);
  }
}

-(void)runBlockOnOurQueueAsync:(dispatch_block_t)block
{
  if ([self isOnOurQueue]) {
    block();
  } else {
    dispatch_async(_dispatchQueue, block);
  }
}

-(dispatch_source_t)createDispatchTimer:(uint64_t) interval
                                 leeway:(uint64_t) leeway
                                  queue:(dispatch_queue_t) queue
                                  block:(dispatch_block_t) block
{
  dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,
                                                   0, 0, queue);
  if (timer)
  {
    dispatch_source_set_timer(timer, dispatch_time(DISPATCH_TIME_NOW, 0), interval, leeway);
    dispatch_source_set_event_handler(timer, block);
    dispatch_resume(timer);
  }
  return timer;
}

-(id)initWithCentralManager:(CBCentralManager *)centralManager
                      queue:(dispatch_queue_t)queue
{
  if((self = [super init])) {
    _dispatchQueue = queue;
    dispatch_queue_set_specific(_dispatchQueue, &IsOnBLEQueue, (__bridge void *) self, NULL);
    _centralManager = centralManager;
    _centralManager.delegate = self;
    _centralManagerState = (CBManagerState) centralManager.state;
    _registeredServicesByServiceID = [NSMutableDictionary dictionaryWithCapacity:4];
    _scanningServices = [NSMutableSet setWithCapacity:4];
    _usePeripheralNameIfLocalNameIsAbsent = kOnMacOsX;
    _advertisementsByPeripheralAddress = [NSMutableDictionary dictionary];
    _servicesForPeripheral = [NSMapTable mapTableWithKeyOptions:NSMapTableWeakMemory valueOptions:NSMapTableStrongMemory];
    _pendingConnections = [NSMutableArray arrayWithCapacity:20];
    _connectedPeripherals = [NSMutableSet setWithCapacity:20];
    _disconnectingPeripherals = [NSMutableSet setWithCapacity:20];
    _scanningState = BLECentralMultiplexerScanningStateStopped;
    _peripheralsWaitingForData = [NSMutableSet setWithCapacity:20];
  }
  return self;
}

-(void)dealloc {
  _centralManager.delegate = nil;
}

#pragma mark - Service Registry

-(void)registerService:(BLECentralServiceDescription *)serviceDescription {
  BLELogDebug("BLECentralMultiplexer.registeredService", "%s",
              serviceDescription.description.UTF8String);
  [self runBlockOnOurQueueAsync:^{
    _registeredServicesByServiceID[serviceDescription.serviceID] = serviceDescription;
  }];
}

-(void)cancelService:(BLECentralServiceDescription *)serviceDescription {
  BLELogDebug("BLECentralMultiplexer.cancelService", "%s",
              serviceDescription.description.UTF8String);
  [self runBlockOnOurQueueAsync:^{
    // disconnect any peripherals connected for this service description
    for(CBPeripheral* peripheral in [_connectedPeripherals copy]) {
      if([[_servicesForPeripheral objectForKey:peripheral]
          containsObject:serviceDescription.serviceID]) {
        [self disconnectPeripheral:peripheral withService:serviceDescription];
        // We have to notify the client of disconnects now, because this service is going down!
        [self notifyClientsOfDisconnectedPeripheral:peripheral error:nil];
        [_servicesForPeripheral removeObjectForKey:peripheral];
      }
    }
    // cancel any pending connections for this service description
    for(BLEPeripheralConnectionAttempt* connectionAttempt in [_pendingConnections copy]) {
      [self disconnectPeripheral:connectionAttempt.peripheral withService:serviceDescription];
    }
    if([[_servicesForPeripheral objectForKey:_connectionAttemptInProgress.peripheral]
        containsObject:serviceDescription.serviceID]) {
      [self failCurrentConnectionAttempt];
    }
    // expire any advertisements for this service description
    [self expireAllAdvertisementsForService:serviceDescription];

    if([_registeredServicesByServiceID[serviceDescription.serviceID] isEqual:serviceDescription]) {
      [_registeredServicesByServiceID removeObjectForKey:serviceDescription.serviceID];
    }
  }];
}

-(BLECentralServiceDescription*)serviceForUUID:(CBUUID *)serviceID {
  __block BLECentralServiceDescription* service;
  [self runBlockOnOurQueueSync:^{
    service = _registeredServicesByServiceID[serviceID];
  }];
  return service;
}

-(BLECentralServiceDescription*)serviceForPeripheral:(CBPeripheral*)peripheral {
  NSSet* serviceIDs = [_servicesForPeripheral objectForKey:peripheral];
  CBUUID* uuid = serviceIDs.anyObject;
  return [self serviceForUUID:uuid];
}


#pragma mark - Advertisement
-(void)startScanningForService:(BLECentralServiceDescription *)serviceDescription {
  [self runBlockOnOurQueueAsync:^{
    BLEAssert(_registeredServicesByServiceID[serviceDescription.serviceID] != nil);
    if(![_scanningServices containsObject:serviceDescription]) {
      // Add the service if it is not already registered
      [_scanningServices addObject:serviceDescription];
    }

    // Always issue a 'startScanning' command, which ensure that we restart,
    // even from a paused state.
    if (((CBManagerState) _centralManager.state) == CBManagerStatePoweredOn) {
      [self startScanning];
    }
  }];
}

-(void)startScanning {
  if(_scanningServices.count > 0) {
    if(!_isScanning) {
      _isScanning = YES;
      // Keep the time since we started scanning. CoreBluetooth changes its scan window after 110 seconds, likely as a power
      // optimization. We keep track of the time we've been scanning so we can have a responsive UI when a car peripheral stops
      // advertising. In the average case, we shouldn't be on this screen very long - so we can use the shorter experation interval.
      _scanningStartTime = CFAbsoluteTimeGetCurrent();

      _advertSweepTimer =
        [self createDispatchTimer:(BLE_PERIPHERAL_ADVERT_SWEEP_TIMER_SECS * NSEC_PER_SEC)
                           leeway:(BLE_PERIPHERAL_ADVERT_SWEEP_TIMER_LEEWAY_MSECS * NSEC_PER_MSEC)
                            queue:_dispatchQueue
                            block:^{ [self sweepOldPeripherals]; }];
    }
    [self updateScanForPeripheralsWithServices];
  }
}

-(void)updateScanForPeripheralsWithServices {
  NSMutableArray* serviceIDs = [NSMutableArray arrayWithCapacity:_scanningServices.count];
  for(BLECentralServiceDescription* serviceDescription in _scanningServices) {
    [serviceIDs addObject:serviceDescription.serviceID];
  }
  [_centralManager scanForPeripheralsWithServices:serviceIDs
                                          options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
  [self setScanningState:BLECentralMultiplexerScanningStateScanning];
}

-(void)stopScanningForService:(BLECentralServiceDescription *)serviceDescription {
  [self runBlockOnOurQueueAsync:^{
    assert(_registeredServicesByServiceID[serviceDescription.serviceID] != nil);
    if([_scanningServices containsObject:serviceDescription]) {
      [_scanningServices removeObject:serviceDescription];
      if(_scanningServices.count == 0) {
        [self stopScanning];
      }
      else {
        // we need to change the list of services we're scanning for.
        [self updateScanForPeripheralsWithServices];
      }
      [self expireAllAdvertisementsForService:serviceDescription];
    }
  }];
}

-(void)stopScanning {
  if (_isScanning && ((CBManagerState)_centralManager.state) == CBManagerStatePoweredOn) {
    [_centralManager stopScan];
  }
  _isScanning = NO;
  if ([self isScanningForServices]) {
    // If we still have services registered for scanning, then pause scanning.
    [self setScanningState:BLECentralMultiplexerScanningStatePaused];
  } else {
    // Otherwise, no services are registered for scanning, so stop scanning.
    [self setScanningState:BLECentralMultiplexerScanningStateStopped];
  }
  if (_advertSweepTimer) {
    dispatch_source_cancel(_advertSweepTimer);
    _advertSweepTimer = nil;
  }
}

-(void)pauseScanning {
  [self runBlockOnOurQueueAsync:^{
    if (_isScanning) {
      _pauseTimestamp = CFAbsoluteTimeGetCurrent();
      BLELogInfo("BLECentralMultiplexer.pauseScanning", "");
      [self stopScanning];
    }
  }];
}

-(void)resumeScanning {
  [self runBlockOnOurQueueAsync:^{
    if (!_isScanning) {
      BLELogInfo("BLECentralMultiplexer.resumeScanning",
                 "paused for %3.3f sec", CFAbsoluteTimeGetCurrent() - _pauseTimestamp);
      [self refreshAdvertisements];
      [self startScanning];
      _pauseTimestamp = 0.0;
    }
  }];
}

-(BOOL)isScanningForService:(BLECentralServiceDescription *)serviceDescription {
  __block BOOL containsService;
  [self runBlockOnOurQueueSync:^{
    containsService = [_scanningServices containsObject:serviceDescription];
  }];
  return containsService;
}

// Return YES if any services are currently requesting scanning.
// Note that isScanning only reflects the service state and therefore
// may return true even if BLE scanning is paused.
// The exact scanning state can be determined via the scanningState accessor.
-(BOOL)isScanningForServices {
  return (_scanningServices.count > 0);
}

-(void)setScanningState:(BLECentralMultiplexerScanningState)scanningState {
  if (scanningState != _scanningState) {
    (void)ScanningStateName(scanningState);
    BLELogInfo("BLECentralMultiplexer.setScanningState",
               "%s => %s", ScanningStateName(_scanningState), ScanningStateName(scanningState));
    _scanningState = scanningState;
  }
}

-(void)expireAllAdvertisementsForService:(BLECentralServiceDescription*)serviceDescription {
  NSMutableArray* peripheralsToExpire = [NSMutableArray arrayWithCapacity:_advertisementsByPeripheralAddress.count];
  for(BLEAdvertisement* advertisement in [_advertisementsByPeripheralAddress allValues]) {
    if([advertisement.serviceIDs containsObject:serviceDescription.serviceID]) {
      [peripheralsToExpire addObject:advertisement.peripheral];
    }
  }
  for(CBPeripheral* peripheral in peripheralsToExpire) {
    // FIXME: remove the service ID in the service description.
    //  Only expire the advertisement when the service list is empty.
    [_advertisementsByPeripheralAddress removeObjectForKey:_addressOfPeripheral(peripheral)];
    [serviceDescription.delegate service:serviceDescription peripheralDidDisappear:peripheral];
  }
}

#pragma mark Advertisement Expiry
- (void) sweepOldPeripherals
{
  if(!_isScanning) {
    return;
  }
  
  CFAbsoluteTime currentTime = CFAbsoluteTimeGetCurrent();
  double expirationInterval;
  if (currentTime - _scanningStartTime < BLE_SECONDS_BEFORE_LONG_EXPIRATION) {
    expirationInterval = BLE_PERIPHERAL_ADVERT_EXPIRATION_SECS_SMALL;
  } else {
    expirationInterval = BLE_PERIPHERAL_ADVERT_EXPIRATION_SECS_LARGE;
  }
  
  [self expireAdvertisementsOlderThan:(currentTime - expirationInterval)];
}

-(void)expireAdvertisementsOlderThan:(CFAbsoluteTime)time {
  NSMutableArray *advertsToRetire = [NSMutableArray array];
  [_advertisementsByPeripheralAddress enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
    BLEAdvertisement *advert  = obj;
    if (advert.timeStamp <= time) { // then we're a candidate for expiration
      [advertsToRetire addObject:obj];
    }
    else {
      // report RSSI
      for(CBUUID* serviceID in advert.serviceIDs) {
        BLECentralServiceDescription* registeredService = [self serviceForUUID:serviceID];
        [registeredService.delegate service:registeredService peripheral:advert.peripheral didUpdateRSSI:advert.rssi];
      }
    }
  }];
  
  [self expireAdvertisements:advertsToRetire];
}

-(void)expireAdvertisements:(NSArray*)advertisements {
  [self assertOnOurQueue];
  for(BLEAdvertisement* advert in advertisements) {
    [_advertisementsByPeripheralAddress removeObjectForKey:_addressOfPeripheral(advert.peripheral)];
    for(CBUUID* serviceID in advert.serviceIDs) {
      // notify responder of this service
      BLECentralServiceDescription* registeredService = [self serviceForUUID:serviceID];
      [registeredService.delegate service:registeredService peripheralDidDisappear:advert.peripheral];
    }
  }
}

-(void)refreshAdvertisements {
  CFAbsoluteTime nowTimestamp = CFAbsoluteTimeGetCurrent();
  [_advertisementsByPeripheralAddress enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
    BLEAdvertisement *advertisement  = obj;
    advertisement.timeStamp = nowTimestamp;
  }];
}

-(void)failConnectionAttemptIfOlderThan:(CFAbsoluteTime)time {
  if(_connectionAttemptInProgress) {
    if(_connectionAttemptInProgress.timeStamp <= time) {
      BLELogWarn("BLECentralMultiplexer.connectionFailureTimedOut", "Service=%s, Peripheral=%s>",
                 [[self serviceForPeripheral:_connectionAttemptInProgress.peripheral].name UTF8String],
                 [[_connectionAttemptInProgress.peripheral identifier].UUIDString UTF8String]);
      
      BOOL waitingForServiceDiscovery = (_connectionAttemptInProgress.state == BLEPeripheralConnectionStateConnected);
      BOOL isFirstAttempt = (_connectionAttemptInProgress.numAttempts == 1);
      if (waitingForServiceDiscovery && isFirstAttempt) {
       // We believe we're wedged if we were able to connect, but service discovery timed out and this was our first attempt (we hung for a while).
        _isWedged = YES;
      }
      
      // clear debug timeout flag
      self.debugTimeoutNextConnectionAttempt = NO;
      
      // Hard fail this connection if it takes too long (even if we're in the middle of retrying).
      [self failCurrentConnectionAttempt];
    }
  }
}

#pragma mark - Connection / Disconnection

-(void)sweepConnectionAttempts {
  CFAbsoluteTime currentTime = CFAbsoluteTimeGetCurrent();
  [self failConnectionAttemptIfOlderThan:(currentTime - BLE_PERIPHERAL_CONNECT_TIMEOUT_SECS)];
}

-(void)startConnectionTimer {
  if (!_connectionTimer) {
    _connectionTimer =
      [self createDispatchTimer:(BLE_PERIPHERAL_CONNECT_SWEEP_TIMER_SECS * NSEC_PER_SEC)
                         leeway:(BLE_PERIPHERAL_CONNECT_SWEEP_TIMER_LEEWAY_MSECS * NSEC_PER_MSEC)
                          queue:_dispatchQueue
                          block:^{ [self sweepConnectionAttempts]; }];
  }
}

-(void)stopConnectionTimer {
  if (_connectionTimer) {
    dispatch_source_cancel(_connectionTimer);
    _connectionTimer = nil;
  }
}

#pragma mark connection
-(void)connectPeripheral:(CBPeripheral*)peripheral
             withService:(BLECentralServiceDescription*)service {
  [self runBlockOnOurQueueAsync:^{
    if ((CBPeripheralStateConnected != peripheral.state)
        && ![self connectionAttemptForPeripheral:peripheral]) {
      BLEPeripheralConnectionAttempt* connectionAttempt =
        [[BLEPeripheralConnectionAttempt alloc] init];
      connectionAttempt.peripheral = peripheral;
      connectionAttempt.state = BLEPeripheralConnectionStateConnecting;

      [_pendingConnections addObject:connectionAttempt];

      [self maybeStartConnectionAttempt];
    }
    else {
      BLELogWarn("BLECentralMultiplexer.connectingAlreadyConnectedPeripheral", "%s",
                 peripheral.identifier.UUIDString.UTF8String);
      BLEPeripheralConnectionAttempt *attempt = [self connectionAttemptForPeripheral:peripheral];
      attempt.state = BLEPeripheralConnectionStateConnected;
      // just try to discover services..
      peripheral.delegate = self;
      [self discoverServicesForPeripheral:peripheral];
    }
  }];
}

-(BOOL)maybeStartConnectionAttempt {
  if(!_connectionAttemptInProgress) {
    if(_pendingConnections.count > 0) {
      _connectionAttemptInProgress = [_pendingConnections objectAtIndex:0];
      [_pendingConnections removeObjectAtIndex:0];
      
      _connectionAttemptInProgress.peripheral.delegate = self;

      _connectionAttemptInProgress.numAttempts++;
      BLELogDebug("BLECentralMultiplexer.connectToPeripheral","%s (tries=%lu)",
            _connectionAttemptInProgress.peripheral.name.UTF8String,
            (unsigned long)_connectionAttemptInProgress.numAttempts);
      
      // Remember when this attempt started, in order to time out the connection attempt.
      _connectionAttemptInProgress.timeStamp = CFAbsoluteTimeGetCurrent();
      
      // Set the option to get notification on disconnect, it's very likely that this isn't necessary for our needs.
      //  We only remain connected while our app is open and active.
      if((CBPeripheralStateConnected != _connectionAttemptInProgress.peripheral.state)) {
        [self startConnectionTimer];
        [_centralManager connectPeripheral:_connectionAttemptInProgress.peripheral options:@{CBConnectPeripheralOptionNotifyOnDisconnectionKey:@YES}];
      }
      else {
        BLELogWarn("BLECentralMultiplexer.maybeConnectToAlreadyConnected", "%s", _connectionAttemptInProgress.peripheral.identifier.UUIDString.UTF8String);
      }

      return YES;
    }
  }
  return NO;
}

-(BLEPeripheralConnectionAttempt*)connectionAttemptForPeripheral:(CBPeripheral*)peripheral {
  __block BLEPeripheralConnectionAttempt *result = nil;
  if(_connectionAttemptInProgress.peripheral == peripheral) {
    return _connectionAttemptInProgress;
  }
  
  [_pendingConnections enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
    BLEPeripheralConnectionAttempt* candidateAttempt = obj;
    if(candidateAttempt.peripheral == peripheral) {
      result = candidateAttempt;
      *stop = YES;
    }
  }];
  return result;
}

-(void)discoverServicesForPeripheral:(CBPeripheral*)peripheral {
  // Begin the service discovery
  BLELogInfo("BLECentralMultiplexer.discoverServicesForPeripheral", "%s", peripheral.name.UTF8String);
  NSSet* servicesForPeripheral = [_servicesForPeripheral objectForKey:peripheral];
  [peripheral discoverServices:servicesForPeripheral.allObjects];
}

-(BOOL)retryConnectToPeripheral:(CBPeripheral*)peripheral {
  if(_connectionAttemptInProgress.peripheral == peripheral) {
    if(_connectionAttemptInProgress.numAttempts <= BLE_CONNECT_NUM_RETRIES) {
      BLELogInfo("BLECentralMultiplexer.retryingConnection", "%s", peripheral.name.UTF8String);
      _connectionAttemptInProgress.state = BLEPeripheralConnectionStatePending;
      [_pendingConnections insertObject:_connectionAttemptInProgress atIndex:0];
      _connectionAttemptInProgress = nil;
      // The delay in retrying here is completely arbitrary.
      int64_t delayInMSecs = BLE_CONNECT_RETRY_DELAY_MSECS;
      dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, delayInMSecs * NSEC_PER_MSEC);
      dispatch_after(popTime, _dispatchQueue, ^(void){
        [self maybeStartConnectionAttempt];
      });
      return YES;
    }
    else {
      BLELogWarn("BLECentralMultiplexer.failedTooManyTimes", "peripheral=%s services=%s",
                 peripheral.identifier.UUIDString.UTF8String,
                 [[[self serviceForPeripheral:peripheral] description] UTF8String]);
      [self failCurrentConnectionAttempt];
    }
  }
  return NO;
}

-(void)failCurrentConnectionAttempt {
  if(_connectionAttemptInProgress != nil) {
    _connectionAttemptInProgress.state = BLEPeripheralConnectionStateCancelled;
    CBPeripheral* peripheral = _connectionAttemptInProgress.peripheral;
    BLELogInfo("BLECentralMultiplexer.failCurrentConnectionAttempt", "%s", peripheral.name.UTF8String);
    _connectionAttemptInProgress = nil;
    [self failedToConnectPeripheral:peripheral error:nil];
    [self maybeStartConnectionAttempt];
  }
}

-(void)failedToConnectPeripheral:(CBPeripheral*)peripheral error:(NSError*)error {
  BLELogInfo("BLECentralMultiplexer.failedToConnectToPeripheral", "%s error=%s", peripheral.name.UTF8String, error.description.UTF8String);
  [self stopConnectionTimer];
  if(peripheral.state > CBPeripheralStateDisconnected) {
    // If the peripheral is actually connected, tell BLE to disconnect.
    //  we will catch the disconnect callback and notify the client at that point.
    [_centralManager cancelPeripheralConnection:peripheral];
  }
  else {
    // Otherwise, tell the client that we failed to connect now.
    //  And clear the advertisement entry to let the client know that
    //  the peripheral is advertising.
    [self notifyClientsOfDisconnectedPeripheral:peripheral error:error];
    [self expireAdvertisementForPeripheral:peripheral];
  }
}

-(void)notifyClientsOfDisconnectedPeripheral:(CBPeripheral*)peripheral error:(NSError*)error {
  NSSet* servicesForPeripheral = [_servicesForPeripheral objectForKey:peripheral];
  if(servicesForPeripheral) {
    for (CBUUID* serviceID in servicesForPeripheral) { // there may be no services at this point!
      BLECentralServiceDescription* serviceDescription = [self serviceForUUID:serviceID];
      if(serviceDescription) {
        BLELogDebug("BLECentralMultiplexer.notifyClientsOfDisconnect", "peripheral=%s error=%s", peripheral.description.UTF8String, error.description.UTF8String);
        [serviceDescription.delegate service:serviceDescription didDisconnectPeripheral:peripheral error:error];
      }
      else {
        BLELogDebug("BLECentralMultiplexer.notifyClientsOfDisconnect.noService", "peripheral=%s", peripheral.description.UTF8String);
      }
    }
  }
  else {
    BLELogDebug("BLECentralMultiplexer.notifyClientsOfDisconnect.noClients", "peripheral=%s", peripheral.description.UTF8String);
  }
}

-(void)expireAdvertisementForPeripheral:(CBPeripheral*)peripheral {
  BLEAdvertisement* advertisement = _advertisementsByPeripheralAddress[_addressOfPeripheral(peripheral)];
  if(advertisement) {
    [self expireAdvertisements:@[advertisement]];
  }
}

#pragma mark Disconnect Peripheral
-(void)disconnectPeripheral:(CBPeripheral*)peripheral
                withService:(BLECentralServiceDescription*)service {
  [self runBlockOnOurQueueAsync:^{
    // Mark this peripheral as attempting to disconnect, cancel the service.
    //  The disconnect delegate callback will come via the CBCentralManager.
    //  Disconnecting the peripheral delegate will keep us from getting incoming value changes.
    peripheral.delegate = nil;

    [self cancelPendingConnectionAttemptsForPeripheral:peripheral];

    if([_connectedPeripherals containsObject:peripheral]) {
      // If we're already connected, ask the central manager to disconnect.
      // We will get a disconnected event at some point later and clean up there.
      [_disconnectingPeripherals addObject:peripheral];
      [_centralManager cancelPeripheralConnection:peripheral];
    }
    else if(_connectionAttemptInProgress.peripheral == peripheral) {
      // Otherwise if we're currently attempting to disconnect, we cancel the connection.
      [_disconnectingPeripherals addObject:peripheral];
      [self failCurrentConnectionAttempt];
    }
  }];
}

-(void)cancelPendingConnectionAttemptsForPeripheral:(CBPeripheral*)peripheral {
  // Search the pending connections, and stop the future connection (cancel connect).
  for (BLEPeripheralConnectionAttempt* attempt in [_pendingConnections copy]) {
    if (attempt.peripheral == peripheral) {
      [self cancelPendingConnectionAttempt:attempt];
    }
  }
}

-(void)cancelPendingConnectionAttempt:(BLEPeripheralConnectionAttempt*)attempt {
  // Just remove the pending connection attempt from our queue
  //  and let the client know that we're not going to connect.
  [_pendingConnections removeObject:attempt];
  [self notifyClientsOfDisconnectedPeripheral:attempt.peripheral error:nil];
}

-(void)reset {
  [self runBlockOnOurQueueAsync:^{
    
    // Pause scanning so that it will auto-resume after the Central resets
    [self pauseScanning];
    
    // stop scanning for services
    for (BLECentralServiceDescription *serviceDescription in _scanningServices) {
      // disconnect any peripherals connected for this service description
      for(CBPeripheral* peripheral in [_connectedPeripherals copy]) {
        if([[_servicesForPeripheral objectForKey: peripheral] containsObject:serviceDescription.serviceID]) {
          [self disconnectPeripheral:peripheral withService:serviceDescription];
          // We have to notify the client of disconnects now, because this service is going down!
          [self notifyClientsOfDisconnectedPeripheral:peripheral error:nil];
          [_servicesForPeripheral removeObjectForKey:peripheral];
        }
      }
      // cancel any pending connections for this service description
      for(BLEPeripheralConnectionAttempt* connectionAttempt in [_pendingConnections copy]) {
        [self disconnectPeripheral:connectionAttempt.peripheral withService:serviceDescription];
      }
      if([[_servicesForPeripheral objectForKey:_connectionAttemptInProgress.peripheral] containsObject:serviceDescription.serviceID]) {
        [self failCurrentConnectionAttempt];
      }
      // expire any advertisements for this service description
      [self expireAllAdvertisementsForService:serviceDescription];
    }
    
    // Pause scanning so that it will auto-resume after the Central resets
    [self pauseScanning];
  }];
}

-(BOOL)isWedged {
  return _isWedged;
}

#pragma mark Peripheral Disconnect Event
// Main entry point for receiving a peripheral disconnection event.
-(void)peripheralDisconnected:(CBPeripheral*)peripheral error:(NSError*)error {
  if([_disconnectingPeripherals containsObject:peripheral]) {
    // We meant to disconnect.
    [self handleExpectedDisconnectionOfPeripheral:peripheral];
    [self expireAdvertisementForPeripheral:peripheral];
  }
  else {
    // Something bad happened to the connection.
    [self handleUnexpectedDisconnectionOfPeripheral:peripheral error:error];
  }
  [_connectedPeripherals removeObject:peripheral];
}

-(void)handleExpectedDisconnectionOfPeripheral:(CBPeripheral*)peripheral {
  BLELogEvent("BLECentralMultiplexer.peripheralDisconnect.peripheralDisconnect.expected",
              "%s", peripheral.identifier.UUIDString.UTF8String);
  if(_connectionAttemptInProgress.peripheral == peripheral) {
    // Pop this out of the connection queue, try the next one.
    [self stopConnectionTimer];
    _connectionAttemptInProgress = nil;
    [self maybeStartConnectionAttempt];
  }
  [_disconnectingPeripherals removeObject:peripheral];
  [self notifyClientsOfDisconnectedPeripheral:peripheral error:nil];
}

// TODO: add 'expect disconnect' for when we expect the peripheral to drop the connection on it's side.
-(void)handleUnexpectedDisconnectionOfPeripheral:(CBPeripheral*)peripheral error:(NSError*)error {
  if(_connectionAttemptInProgress.peripheral == peripheral) {
    // We were trying to connect to this guy, possibly retry?
    [self retryConnectToPeripheral:peripheral];
  }
  else {
    BLELogEvent("BLECentralMultiplexer.peripheralDisconnect.unexpected",
                "%s error=%s", peripheral.identifier.UUIDString.UTF8String,
                error.description.UTF8String);
    [self notifyClientsOfDisconnectedPeripheral:peripheral error:error];
    [self expireAdvertisementForPeripheral:peripheral];
  }
}

#pragma mark - CBCentralManagerDelegate methods
- (void)centralManagerDidUpdateState:(CBCentralManager *)central {
  [self assertOnOurQueue];
  NSError* error = nil;
  switch (central.state) {
    case CBManagerStateUnknown:
      // This is the normal starting state.
      BLELogWarn("BLECentralMultiplexer.unexpectedCentralState", "%s", "CentralManagerStateUnknown");
      break;
    case CBManagerStateUnsupported:
    case CBManagerStateUnauthorized:
      // Not sure what to do with these guys.  Log them.
#if !TARGET_IPHONE_SIMULATOR
      BLELogError("BLECentralMultiplexer.unhandledCentralState", "%d", (int)central.state);
#endif
      break;
      
    case CBManagerStateResetting:
      // If we enter this state, it could be because we are recovering from
      // a BTServer crash/restart.
      if (_isWedged) {
        BLELogEvent("BLECentralMultiplexer.centralRestting.recoverFromWedge",
                    "%s", "CentralManagerStateResetting");
      }
      _isWedged = NO;
      
      // falling through
    case CBManagerStatePoweredOff:
      BLELogWarn("BLECentralMultiplexer.centralResetting", "state=%d", (int)central.state);
      // Shut everything down, let everyone know that everything is disconnected.
      error = [NSError errorWithDomain:@"BLECentralManagerErrorDomain" code:-1 userInfo:nil];
      for(CBPeripheral* peripheral in [_connectedPeripherals copy]) {
        [self peripheralDisconnected:peripheral error:error];
      }
      for(BLEPeripheralConnectionAttempt* attempt in [_pendingConnections copy]) {
        [self cancelPendingConnectionAttempt:attempt];
      }
      [self failCurrentConnectionAttempt];
      [self expireAdvertisements:[[_advertisementsByPeripheralAddress allValues] copy]];
      break;
      
    case CBManagerStatePoweredOn:
      BLELogInfo("BLECentralMultiplexer.centralPoweredOn", "");
      // CoreBluetooth is ready to go, start scanning if we were asked previously.
      if(_scanningServices.count > 0) {
        [self startScanning];
      }
      break;
      
    default:
      BLELogError("BLECentralMultiplexer.unknownCentralState", "%d", (int)central.state);
      break;
  }
  
  CBManagerState previousState = _centralManagerState;
  _centralManagerState = (CBManagerState) central.state;
  
  // Notify observers that power state changed
  if ( previousState != _centralManagerState ) {
    if (_centralManagerState == CBManagerStatePoweredOn ||
        _centralManagerState == CBManagerStatePoweredOff ||
        _centralManagerState == CBManagerStateResetting ||
        _centralManagerState == CBManagerStateUnsupported ||
        _centralManagerState == CBManagerStateUnauthorized) {
      BOOL isUnauthorized = (_centralManagerState == CBManagerStateUnauthorized);
      BOOL isUnsupported = (_centralManagerState == CBManagerStateUnsupported);
      BOOL isPoweredOn = (_centralManagerState == CBManagerStatePoweredOn);
      __weak __typeof(self) weakSelf = self;
      dispatch_async(dispatch_get_main_queue(), ^{
        __strong __typeof(self) strongSelf = weakSelf;
        [[NSNotificationCenter defaultCenter] postNotificationName:BLEManagerNotificationName
                                                            object:strongSelf
                                                          userInfo:@{BLEManagerKeyPowerIsOn: @(isPoweredOn),
                                                                     BLEManagerKeyBLEUnsupported: @(isUnsupported),
                                                                     BLEManagerKeyBLEUnauthorized: @(isUnauthorized)}];
      });
    }
  }
}

static BOOL hasRequiredAdvertisingData(BLEAdvertisement* advertisement)
{
  return advertisement && advertisement.name && advertisement.serviceIDs && advertisement.serviceIDs.count > 0 && advertisement.mfgData;
}

- (void)centralManager:(CBCentralManager *)central
 didDiscoverPeripheral:(CBPeripheral *)peripheral
     advertisementData:(NSDictionary *)advertisementData
                  RSSI:(NSNumber *)RSSI {
  [self assertOnOurQueue];
  NSMutableDictionary* amendedAdvertisementData =
    [NSMutableDictionary dictionaryWithDictionary:advertisementData];

  if (!amendedAdvertisementData[CBAdvertisementDataLocalNameKey]) {
    if (_usePeripheralNameIfLocalNameIsAbsent && (peripheral.name.length > 0)) {
      BLELogDebug("BLECentralMultiplexer.didDiscoverPeripheral",
        "Received BLE advertisement without LocalName, using peripheral.name = '%s' instead.",
        peripheral.name.UTF8String);
      amendedAdvertisementData[CBAdvertisementDataLocalNameKey] = peripheral.name;
    } else {
      BLELogDebug("BLECentralMultiplexer.didDiscoverPeripheral",
        "Received BLE advertisement without LocalName");
    }
  }

  NSArray* serviceIDs = amendedAdvertisementData[CBAdvertisementDataServiceUUIDsKey];
  NSString* serviceLocalName = amendedAdvertisementData[CBAdvertisementDataLocalNameKey];
  NSData* mfgData = amendedAdvertisementData[CBAdvertisementDataManufacturerDataKey];
  id<NSCopying> peripheralAddress = _addressOfPeripheral(peripheral);
  
  if ( !peripheral || peripheral == (id)[NSNull null] )
    return;
  
  BLEAdvertisement* existingAdvertisement = _advertisementsByPeripheralAddress[peripheralAddress];
  
  if (!existingAdvertisement)
  {
    // New Peripheral discovered
    BLEAdvertisement* newAdvert = [[BLEAdvertisement alloc] init];
    newAdvert.peripheral = peripheral;
    newAdvert.discovered = YES;
    [newAdvert readRSSI:RSSI];
    existingAdvertisement = newAdvert;
    _advertisementsByPeripheralAddress[peripheralAddress] = newAdvert;
    [_peripheralsWaitingForData addObject:peripheralAddress];
  }
  
  if (serviceLocalName)
  {
    existingAdvertisement.name = serviceLocalName;
  }
  
  if (mfgData)
  {
    existingAdvertisement.mfgData = mfgData;
  }
  
  if (serviceIDs) {
    NSMutableSet* servicesForPeripheral = [NSMutableSet setWithSet:[_servicesForPeripheral objectForKey:peripheral]];
    // For each of these service ID's, we must let any delegates know that we've discovered peripherals
    for(CBUUID* serviceID in serviceIDs) {
      [servicesForPeripheral addObject:serviceID];
      [existingAdvertisement addServiceID:serviceID];
    }
    BLEAssert(servicesForPeripheral.count > 0);
    // Uses a weak key to let the mapping drop when nobody cares about the peripheral any longer.
    //  We could add a debug flag to switch this to strong key to check for this being the source of bugs.
    [_servicesForPeripheral setObject:servicesForPeripheral forKey:peripheral];
  }
  
  NSNumber* oldRssi = [existingAdvertisement rssi];
  // update the RSSI
  [existingAdvertisement readRSSI:RSSI];
  // bump the timestamp to prevent expiry.
  existingAdvertisement.timeStamp = CFAbsoluteTimeGetCurrent();
  
  NSDictionary* originalItems = [NSDictionary dictionaryWithDictionary: existingAdvertisement.advertisementData];
  [existingAdvertisement.advertisementData addEntriesFromDictionary:advertisementData];
  
  // If we were waiting for complete advertising data before 'discovering' this periperhal and it now has the data
  // 'discover' it now
  if ([_peripheralsWaitingForData containsObject:peripheralAddress])
  {
    if (hasRequiredAdvertisingData(existingAdvertisement))
    {
      for(CBUUID* serviceID in existingAdvertisement.serviceIDs) {
        BLECentralServiceDescription* registeredService = [self serviceForUUID:serviceID];
        [registeredService.delegate service:registeredService
                      didDiscoverPeripheral:peripheral
                                   withRSSI:existingAdvertisement.rssi
                          advertisementData:existingAdvertisement.advertisementData];
      }
      [_peripheralsWaitingForData removeObject:peripheralAddress];
    }
  }
  else if (![existingAdvertisement.advertisementData isEqualToDictionary:originalItems]
           || [oldRssi doubleValue] != [[existingAdvertisement rssi] doubleValue])
  {
    for(CBUUID* serviceID in [_servicesForPeripheral objectForKey:peripheral]) {
      BLECentralServiceDescription* service = [self serviceForUUID:serviceID];
      [service.delegate service:service peripheral:peripheral didUpdateAdvertisementData:existingAdvertisement.advertisementData];
    }
  }
}

-(void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral {
  [self assertOnOurQueue];
  // Doesn't guarantee connection completed successfully, we have to wait until the service discovery for that.
  BLELogInfo("BLECentralMultiplexer.didConnectPeripheral", "%s", peripheral.name.UTF8String);
  [_connectedPeripherals addObject:peripheral];
  BLEPeripheralConnectionAttempt *attempt = [self connectionAttemptForPeripheral:peripheral];
  attempt.state = BLEPeripheralConnectionStateConnected;
  [self discoverServicesForPeripheral:peripheral];
}

-(void)centralManager:(CBCentralManager *)central didFailToConnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
  [self assertOnOurQueue];
  // Doesn't seem to happen in practice, we should at least fail the connection attempt.
  BLELogError("BLECentralMultiplexer.didFailToConnectToPeripheral", "%s, error=%s", peripheral.identifier.UUIDString.UTF8String, error.description.UTF8String);
  [self peripheralDisconnected:peripheral error:error];
}

-(void)centralManager:(CBCentralManager *)central didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
  [self assertOnOurQueue];
  BLELogInfo("BLECentralMultiplexer.didDisconnectPeripheral", "%s, error=%s", peripheral.name.UTF8String, error.description.UTF8String);
  [self peripheralDisconnected:peripheral error:error];
  
  if (_isWedged) {
    // Send notification that central is wedged.
    BLELogWarn("BLECentralMultiplexer.didDisconnectPeripheral.isWedged", "");
    [[NSNotificationCenter defaultCenter] postNotificationName:BLECentralMultiplexerIsWedgedNotification object:nil];
  }
}

#pragma mark - Peripheral callbacks
// First the services are discovery comes back
-(void)peripheral:(CBPeripheral *)peripheral didDiscoverServices:(NSError *)error {
  [self assertOnOurQueue];
  // we discover all of the characteristics given in the service description
  BLELogInfo("BLECentralMultiplexer.didDiscoverServices", "%s num_services=%lu, error=%s", peripheral.name.UTF8String, (unsigned long)peripheral.services.count, error.description.UTF8String);
  BLEPeripheralConnectionAttempt *attempt = [self connectionAttemptForPeripheral:peripheral];
  
  if (self.debugTimeoutNextConnectionAttempt) {
    attempt.numAttempts = 1;
    return;
  }
  
  attempt.state = BLEPeripheralConnectionStateDiscoveringServices;
  if(error == nil) {
    // Can this happen several times?
    // This code assumes all of the services come back at the same time
    for (CBService* service in peripheral.services) {
      if(service.characteristics && service.characteristics.count > 0) {
        BLELogWarn("BLECentralMultiplexer.discoveredMoreServices_unexpected", "%s", service.characteristics.description.UTF8String);
      }
      BLECentralServiceDescription* serviceDescription = [self serviceForUUID:service.UUID];
      if(serviceDescription != nil) {
        BLEAssert([serviceDescription.serviceID isEqual:service.UUID]);
        [peripheral discoverCharacteristics:serviceDescription.characteristicIDs forService:service];
      }
      else {
        BLELogWarn("BLECentralMultiplexer.didDiscoverServices.unregisteredService", "serviceID=%s", service.UUID.description.UTF8String);
      }
    }
  }
  else {
    // Error discovering services, cancel the connection (and fail the connection attempt).
    BLELogWarn("BLECentralMultiplexer.didDiscoverServices.error", "%s error=%s", peripheral.identifier.UUIDString.UTF8String, error.description.UTF8String);
    [_centralManager cancelPeripheralConnection:peripheral];
  }
  
}

// Second the characteristics come back (we consider this the end of the sequential connection process)
-(void)peripheral:(CBPeripheral *)peripheral didDiscoverCharacteristicsForService:(CBService *)service error:(NSError *)error {
  [self assertOnOurQueue];
  BLELogDebug("BLECentralMultiplexer.didDiscoverCharacteristics", "peripheral=%s service=%s error=%s", peripheral.name.UTF8String, service.UUID.description.UTF8String, error.description.UTF8String);
  // we set the notification state on all of the characteristics we wish to read from
  BLECentralServiceDescription* serviceDescription = [self serviceForUUID:service.UUID];
  BLEPeripheralConnectionAttempt *attempt = [self connectionAttemptForPeripheral:peripheral];
  if(serviceDescription != nil) {
    for(CBCharacteristic* characteristic in service.characteristics) {
      attempt.state = BLEPeripheralConnectionStateDiscoveringCharacteristics;
      [serviceDescription.delegate service:serviceDescription discoveredCharacteristic:characteristic forPeripheral:peripheral];
    }
  }
  else {
    BLELogWarn("BLECentralMultiplexer.didDiscoverCharacteristics.unregisteredService", "uuid=%s", service.UUID.description.UTF8String);
    // should we disconnect from this peripheral?
  }
  
  // We're done establishing the connection at this point and we're good to start with the next one.
  if(_connectionAttemptInProgress.peripheral == peripheral) {
    _connectionAttemptInProgress = nil;
    [self stopConnectionTimer];
    [self maybeStartConnectionAttempt];
  }
}

// Third the notification state is updated (if the client decided to set Notify on one of the characteristics)
-(void)peripheral:(CBPeripheral *)peripheral didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {
  [self assertOnOurQueue];
  BLELogDebug("BLECentralMultiplexer.didUpdateNotificationStateForCharacteristic", "peripheral=%s characteristic=%s error=%s", peripheral.name.UTF8String, characteristic.UUID.description.UTF8String, error.description.UTF8String);
  // The client is responsible for understanding what it means to have a completed connection.
  //  Typically, that happens after the notification state has been updated for inbound characteristics.
  BLECentralServiceDescription* serviceDescription = [self serviceForUUID:characteristic.service.UUID];
  if(serviceDescription != nil) {
    [serviceDescription.delegate service:serviceDescription didUpdateNotificationStateForCharacteristic:characteristic forPeripheral:peripheral error:error];
  }
  else {
    BLELogWarn("BLECentralMultiplexer.didUpdateNotificationStateForCharacteristic.unregisteredService", "uuid=%s", characteristic.service.UUID.description.UTF8String);
  }
}


// If the services changed, we consider the peripheral to be disconnected.
//  Technically, we could scan again to avoid a scan / reconnect.

#if (__IPHONE_OS_VERSION_MAX_ALLOWED < 70000)
// This method has been deprecated as of iOS 7, don't even bother compiling it there.
-(void)peripheralDidInvalidateServices:(CBPeripheral *)peripheral {
  [self assertOnOurQueue];
  BLELogInfo("BLECentralMultiplexer.didInvalidateServices", "peripheral=%s", peripheral.name.UTF8String);
  [self disconnectPeripheral:peripheral withService:nil];
}
#endif

#if (__IPHONE_OS_VERSION_MAX_ALLOWED >= 70000)
// This method is new as of iOS 7, don't bother compiling it if we are not at least using the 7 SDK.
- (void)peripheral:(CBPeripheral *)peripheral didModifyServices:(NSArray *)invalidatedServices {
  [self assertOnOurQueue];
  BLELogInfo("BLECentralMultiplexer.peripheral.didModifyServices", "peripheral=%s,services=%s",
             peripheral.name.UTF8String,
             invalidatedServices.description.UTF8String);
  // We might want to check if we're trying to connect to this handset..
  // It could be that we just need to re-scan the peripheral instead of going nuclear on it.
  [self disconnectPeripheral:peripheral withService:nil];
}
#endif

-(void)peripheral:(CBPeripheral *)peripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {
  [self assertOnOurQueue];
  BLELogDebug("BLECentralMultiplexer.didUpdateValueForCharacteristic", "peripheral=%s characteristic=%s error=%s", peripheral.name.UTF8String, characteristic.UUID.description.UTF8String, error.description.UTF8String);

  // This happens when the other side changes the value for the inbound characteristic ( they send us data ).
  BLECentralServiceDescription* serviceDescription = [self serviceForUUID:characteristic.service.UUID];
  [serviceDescription.delegate service:serviceDescription incomingMessage:characteristic.value onCharacteristic:characteristic forPeripheral:peripheral];
}

-(void)peripheral:(CBPeripheral *)peripheral didWriteValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {
  [self assertOnOurQueue];
  BLELogDebug("BLECentralMultiplexer.didWriteValueForCharacteristic", "peripheral=%s characteristic=%s error=%s", peripheral.name.UTF8String, characteristic.UUID.description.UTF8String, error.description.UTF8String);
  if(error != nil) {
    BLELogError("BLECentralMultiplexer.didWriteValueForCharacteristic.error", "peripheral=%s characteristic=%s error=%s", peripheral.identifier.UUIDString.UTF8String,
                characteristic.UUID.description.UTF8String, error.description.UTF8String);
  }
  // We pipe this back to the clients (in the case of write with response, or an error occurred).
  BLECentralServiceDescription* serviceDescription = [self serviceForUUID:characteristic.service.UUID];
  if(serviceDescription != nil) {
    [serviceDescription.delegate service:serviceDescription peripheral:peripheral didWriteValueForCharacteristic:characteristic error:error];
  }
}

@end

@implementation BLECentralMultiplexer (TestSupport)

- (void)debugSetWedgedState:(BOOL)isWedged
{
  _isWedged = isWedged;
}

@end
