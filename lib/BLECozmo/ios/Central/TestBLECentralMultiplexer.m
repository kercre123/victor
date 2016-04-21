//
//  TestBLECentralMultiplexer.m
//  BLEManager
//
//  Created by Mark Pauley on 3/28/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#import "TestBLECentralMultiplexer.h"
#import "BLECentralMultiplexer.h"
#import "BLECentralServiceDescription.h"
#import <OCMock/OCMock.h>

#define TestServiceID "74784502-54CE-439B-B275-3B21ACC81AA0"
#define TestInboundCharacteristic "B64B9B75-FD16-40B9-828B-4A9123B05D09"
#define TestOutboundCharacteristic "910BF9CD-BA13-4D58-98AD-E6419D895FB4"

@interface BLECentralMultiplexer (Test) <CBCentralManagerDelegate,CBPeripheralDelegate>

-(void)expireAdvertisementsOlderThan:(CFAbsoluteTime)time;

@end

static CBUUID* testServiceID;
static CBUUID* testInboundCharacteristic;
static CBUUID* testOutboundCharacteristic;

@implementation TestBLECentralMultiplexer {
  BLECentralMultiplexer* testMultiplexer;
  id mockCentralManager;
  id mockServiceDelegate;
}

-(CBUUID*)serviceID {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    testServiceID = [CBUUID UUIDWithString:@TestServiceID];
  });
  return testServiceID;
}

-(CBUUID*)inboundCharacteristic {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    testInboundCharacteristic = [CBUUID UUIDWithString:@TestInboundCharacteristic];
  });
  return testInboundCharacteristic;
}

-(CBUUID*)outboundCharacteristic {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    testOutboundCharacteristic = [CBUUID UUIDWithString:@TestOutboundCharacteristic];
  });
  return testOutboundCharacteristic;
}

-(BLECentralServiceDescription*) serviceDescription {
  BLECentralServiceDescription* serviceDescription = [[BLECentralServiceDescription alloc] init];
  serviceDescription.serviceID = [self serviceID];
  serviceDescription.characteristicIDs = @[[self inboundCharacteristic], [self outboundCharacteristic]];
  serviceDescription.delegate = mockServiceDelegate;
  serviceDescription.name = @"Test Service";
  return serviceDescription;
}

-(void)turnCentralOn {
  CBCentralManagerState onState = CBCentralManagerStatePoweredOn;
  [[[mockCentralManager stub] andReturnValue:OCMOCK_VALUE(onState)] state];
}

-(void)setUp {
  mockCentralManager = [OCMockObject mockForClass:[CBCentralManager class]];
  [[mockCentralManager expect] setDelegate:OCMOCK_ANY];
  [[mockCentralManager expect] state];
  testMultiplexer = [[BLECentralMultiplexer alloc] initWithCentralManager:mockCentralManager];
  mockServiceDelegate = [OCMockObject mockForProtocol:@protocol(BLECentralServiceDelegate)];
  [self turnCentralOn];
}

-(void)tearDown {
  [mockCentralManager verify];
  [mockServiceDelegate verify];
}

-(void)testInit {
  XCTAssertNotNil(testMultiplexer, @"Should have been able to create a new central multiplexer.");
  XCTAssertTrue([testMultiplexer conformsToProtocol:@protocol(CBCentralManagerDelegate)], @"Multiplexer should conform to the CBCentralManagerDelegate protocol");
}

-(void)testRegisterService {
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  XCTAssertEqualObjects([testMultiplexer serviceForUUID:[self serviceID]], serviceDescription, @"Should have successfully registered the service");
}

-(void)testCancelService {
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  [testMultiplexer cancelService:serviceDescription];
  XCTAssertNil([testMultiplexer serviceForUUID:[self serviceID]], @"Should have successfully cancelled the service");
}

-(void)testStartScanning {
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  [[mockCentralManager expect] scanForPeripheralsWithServices:@[[self serviceID]] options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
  [testMultiplexer startScanningForService:serviceDescription];
  XCTAssertTrue(testMultiplexer.isScanning, @"Multiplexer should now be scanning");
}

-(void)testStopScanning {
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  [[mockCentralManager stub] scanForPeripheralsWithServices:@[[self serviceID]] options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
  [testMultiplexer startScanningForService:serviceDescription];
  
  [[mockCentralManager expect] stopScan];
  [testMultiplexer stopScanningForService:serviceDescription];
  XCTAssertFalse(testMultiplexer.isScanning, @"Multiplexer should no longer be scanning");
}

// TODO: write test for stopping scan of only one of two services.


-(id)newMockPeripheral {
  id result = [OCMockObject mockForClass:[CBPeripheral class]];
  [[[result stub] andReturn:[NSUUID UUID]] identifier];
  
  return result;
}

-(void)testPeripheralDiscovery {
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  [[mockCentralManager stub] scanForPeripheralsWithServices:@[[self serviceID]] options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
  [testMultiplexer startScanningForService:serviceDescription];

  UInt64 mfgID = 0x01;
  NSData* mfgData = [NSData dataWithBytes:&mfgID length:sizeof(mfgID)];
  NSDictionary* advertisementData = @{CBAdvertisementDataServiceUUIDsKey: @[[self serviceID]],
                                      CBAdvertisementDataLocalNameKey: @"Fake Name",
                                      CBAdvertisementDataManufacturerDataKey: mfgData};
  id mockPeripheral = [self newMockPeripheral];
  
  [[mockServiceDelegate expect] service:[self serviceDescription] didDiscoverPeripheral:mockPeripheral withRSSI:OCMOCK_ANY advertisementData:advertisementData];
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@0];
}

-(void)testPeripheralDiscovery_cancelledService {
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  [[mockCentralManager stub] scanForPeripheralsWithServices:@[[self serviceID]] options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
  [testMultiplexer startScanningForService:serviceDescription];
  
  UInt64 mfgID = 0x01;
  NSData* mfgData = [NSData dataWithBytes:&mfgID length:sizeof(mfgID)];
  NSDictionary* advertisementData = @{CBAdvertisementDataServiceUUIDsKey: @[[self serviceID]],
                                      CBAdvertisementDataLocalNameKey: @"Fake Name",
                                      CBAdvertisementDataManufacturerDataKey: mfgData};
  id mockPeripheral = [self newMockPeripheral];
  
  [testMultiplexer cancelService:[self serviceDescription]];
  [[mockServiceDelegate reject] service:OCMOCK_ANY didDiscoverPeripheral:OCMOCK_ANY withRSSI:OCMOCK_ANY advertisementData:OCMOCK_ANY];
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@0];
}

-(void)testPeripheralDisappeared {
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  [[mockCentralManager stub] scanForPeripheralsWithServices:@[[self serviceID]] options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
  [testMultiplexer startScanningForService:serviceDescription];
  
  UInt64 mfgID = 0x01;
  NSData* mfgData = [NSData dataWithBytes:&mfgID length:sizeof(mfgID)];
  NSDictionary* advertisementData = @{CBAdvertisementDataServiceUUIDsKey: @[[self serviceID]],
                                      CBAdvertisementDataLocalNameKey: @"Fake Name",
                                      CBAdvertisementDataManufacturerDataKey: mfgData};
  id mockPeripheral = [self newMockPeripheral];
  
  [[mockServiceDelegate stub] service:[self serviceDescription] didDiscoverPeripheral:mockPeripheral withRSSI:OCMOCK_ANY advertisementData:OCMOCK_ANY];
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@0];
  
  [[mockServiceDelegate expect] service:[self serviceDescription] peripheralDidDisappear:mockPeripheral];
  [testMultiplexer expireAdvertisementsOlderThan:CFAbsoluteTimeGetCurrent() + 2.0]; // pretend it's two seconds from now.
}

-(void)testConnectPeripheral {
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  [[mockCentralManager stub] scanForPeripheralsWithServices:@[[self serviceID]] options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
  [testMultiplexer startScanningForService:serviceDescription];
  
  UInt64 mfgID = 0x01;
  NSData* mfgData = [NSData dataWithBytes:&mfgID length:sizeof(mfgID)];
  NSDictionary* advertisementData = @{CBAdvertisementDataServiceUUIDsKey: @[[self serviceID]],
                                      CBAdvertisementDataLocalNameKey: @"Fake Name",
                                      CBAdvertisementDataManufacturerDataKey: mfgData};
  id mockPeripheral = [self newMockPeripheral];
  
  [[mockServiceDelegate stub] service:[self serviceDescription] didDiscoverPeripheral:mockPeripheral withRSSI:OCMOCK_ANY advertisementData:OCMOCK_ANY];
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@0];
  
  BOOL isConnected = NO;
  [[[mockPeripheral stub] andReturnValue:OCMOCK_VALUE(isConnected)] isConnected];
  [[mockPeripheral expect] setDelegate:testMultiplexer];
  [[[mockPeripheral stub] andReturn:@"Fake Name"] name];
  [[mockCentralManager expect] connectPeripheral:mockPeripheral options:@{CBConnectPeripheralOptionNotifyOnDisconnectionKey:@YES}];
  [testMultiplexer connectPeripheral:mockPeripheral withService:serviceDescription];
}

-(void)testPeripheralDidConnect {
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  [[mockCentralManager stub] scanForPeripheralsWithServices:@[[self serviceID]] options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
  [testMultiplexer startScanningForService:serviceDescription];
  
  UInt64 mfgID = 0x01;
  NSData* mfgData = [NSData dataWithBytes:&mfgID length:sizeof(mfgID)];
  NSDictionary* advertisementData = @{CBAdvertisementDataServiceUUIDsKey: @[[self serviceID]],
                                      CBAdvertisementDataLocalNameKey: @"Fake Name",
                                      CBAdvertisementDataManufacturerDataKey: mfgData};
  id mockPeripheral = [self newMockPeripheral];
  
  [[mockServiceDelegate stub] service:[self serviceDescription] didDiscoverPeripheral:mockPeripheral withRSSI:OCMOCK_ANY advertisementData:OCMOCK_ANY];
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@0];
  
  BOOL isConnected = NO;
  [[[mockPeripheral stub] andReturnValue:OCMOCK_VALUE(isConnected)] isConnected];
  [[mockPeripheral expect] setDelegate:testMultiplexer];
  [[[mockPeripheral stub] andReturn:@"Fake Name"] name];
  [[mockCentralManager stub] connectPeripheral:mockPeripheral options:@{CBConnectPeripheralOptionNotifyOnDisconnectionKey:@YES}];
  [testMultiplexer connectPeripheral:mockPeripheral withService:serviceDescription];
  
  [[mockPeripheral expect] discoverServices:@[[self serviceID]]];
  [testMultiplexer centralManager:mockCentralManager didConnectPeripheral:mockPeripheral];
}

-(void)testPeripheralDidDiscoverServices {
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  [[mockCentralManager stub] scanForPeripheralsWithServices:@[[self serviceID]] options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
  [testMultiplexer startScanningForService:serviceDescription];
  
  UInt64 mfgID = 0x01;
  NSData* mfgData = [NSData dataWithBytes:&mfgID length:sizeof(mfgID)];
  NSDictionary* advertisementData = @{CBAdvertisementDataServiceUUIDsKey: @[[self serviceID]],
                                      CBAdvertisementDataLocalNameKey: @"Fake Name",
                                      CBAdvertisementDataManufacturerDataKey: mfgData};
  id mockPeripheral = [self newMockPeripheral];
  
  [[mockServiceDelegate stub] service:[self serviceDescription] didDiscoverPeripheral:mockPeripheral withRSSI:OCMOCK_ANY advertisementData:OCMOCK_ANY];
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@0];
  
  BOOL isConnected = NO;
  [[[mockPeripheral stub] andReturnValue:OCMOCK_VALUE(isConnected)] isConnected];
  [[mockPeripheral expect] setDelegate:testMultiplexer];
  [[[mockPeripheral stub] andReturn:@"Fake Name"] name];
  [[mockCentralManager stub] connectPeripheral:mockPeripheral options:@{CBConnectPeripheralOptionNotifyOnDisconnectionKey:@YES}];
  [testMultiplexer connectPeripheral:mockPeripheral withService:serviceDescription];
  
  [[mockPeripheral stub] discoverServices:@[[self serviceID]]];
  [testMultiplexer centralManager:mockCentralManager didConnectPeripheral:mockPeripheral];
  
  BLECentralServiceDescription* serviceDesc = [self serviceDescription];
  id mockService = [OCMockObject mockForClass:[CBService class]];
  [[[mockService stub] andReturn:serviceDesc.serviceID] UUID];
  NSMutableArray* characteristicsOnMockService = [NSMutableArray array];
  [[[mockService stub] andReturn:characteristicsOnMockService] characteristics];
  [[[mockPeripheral stub] andReturn:@[mockService]] services];
  [[mockPeripheral expect] discoverCharacteristics:serviceDesc.characteristicIDs forService:mockService];
  [testMultiplexer peripheral:mockPeripheral didDiscoverServices:nil];
}

-(void)testPeripheralDidDiscoverCharacteristics {
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  [[mockCentralManager stub] scanForPeripheralsWithServices:@[[self serviceID]] options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
  [testMultiplexer startScanningForService:serviceDescription];
  
  UInt64 mfgID = 0x01;
  NSData* mfgData = [NSData dataWithBytes:&mfgID length:sizeof(mfgID)];
  NSDictionary* advertisementData = @{CBAdvertisementDataServiceUUIDsKey: @[[self serviceID]],
                                      CBAdvertisementDataLocalNameKey: @"Fake Name",
                                      CBAdvertisementDataManufacturerDataKey: mfgData};
  id mockPeripheral = [self newMockPeripheral];
  
  [[mockServiceDelegate stub] service:[self serviceDescription] didDiscoverPeripheral:mockPeripheral withRSSI:OCMOCK_ANY advertisementData:OCMOCK_ANY];
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@0];
  
  BOOL isConnected = NO;
  [[[mockPeripheral stub] andReturnValue:OCMOCK_VALUE(isConnected)] isConnected];
  [[mockPeripheral expect] setDelegate:testMultiplexer];
  [[[mockPeripheral stub] andReturn:@"Fake Name"] name];
  [[mockCentralManager stub] connectPeripheral:mockPeripheral options:@{CBConnectPeripheralOptionNotifyOnDisconnectionKey:@YES}];
  [testMultiplexer connectPeripheral:mockPeripheral withService:serviceDescription];
  
  [[mockPeripheral stub] discoverServices:@[[self serviceID]]];
  [testMultiplexer centralManager:mockCentralManager didConnectPeripheral:mockPeripheral];
  
  BLECentralServiceDescription* serviceDesc = [self serviceDescription];
  id mockService = [OCMockObject mockForClass:[CBService class]];
  [[[mockService stub] andReturn:serviceDesc.serviceID] UUID];
  [[[mockService stub] andReturn:serviceDesc.serviceID.description] description];
  NSMutableArray* characteristicsOnMockService = [NSMutableArray array];
  [[[mockService stub] andReturn:characteristicsOnMockService] characteristics];
  [[[mockPeripheral stub] andReturn:@[mockService]] services];
  [[mockPeripheral stub] discoverCharacteristics:serviceDesc.characteristicIDs forService:mockService];
  [testMultiplexer peripheral:mockPeripheral didDiscoverServices:nil];
  
  id mockInboundCharacteristic = [OCMockObject mockForClass:[CBCharacteristic class]];
  id mockOutboundCharacteristic = [OCMockObject mockForClass:[CBCharacteristic class]];
  [characteristicsOnMockService addObjectsFromArray:@[mockInboundCharacteristic, mockOutboundCharacteristic]];
  [[mockServiceDelegate expect] service:serviceDesc discoveredCharacteristic:mockInboundCharacteristic forPeripheral:mockPeripheral];
  [[mockServiceDelegate expect] service:serviceDesc discoveredCharacteristic:mockOutboundCharacteristic forPeripheral:mockPeripheral];
  [testMultiplexer peripheral:mockPeripheral didDiscoverCharacteristicsForService:mockService error:nil];
}

-(void)testPeripheralDidUpdateNotificationState {
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  [[mockCentralManager stub] scanForPeripheralsWithServices:@[[self serviceID]] options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
  [testMultiplexer startScanningForService:serviceDescription];
  
  UInt64 mfgID = 0x01;
  NSData* mfgData = [NSData dataWithBytes:&mfgID length:sizeof(mfgID)];
  NSDictionary* advertisementData = @{CBAdvertisementDataServiceUUIDsKey: @[[self serviceID]],
                                      CBAdvertisementDataLocalNameKey: @"Fake Name",
                                      CBAdvertisementDataManufacturerDataKey: mfgData};
  id mockPeripheral = [self newMockPeripheral];
  
  [[mockServiceDelegate stub] service:[self serviceDescription] didDiscoverPeripheral:mockPeripheral withRSSI:OCMOCK_ANY advertisementData:OCMOCK_ANY];
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@0];
  
  BOOL isConnected = NO;
  [[[mockPeripheral stub] andReturnValue:OCMOCK_VALUE(isConnected)] isConnected];
  [[mockPeripheral expect] setDelegate:testMultiplexer];
  [[[mockPeripheral stub] andReturn:@"Fake Name"] name];
  [[mockCentralManager stub] connectPeripheral:mockPeripheral options:@{CBConnectPeripheralOptionNotifyOnDisconnectionKey:@YES}];
  [testMultiplexer connectPeripheral:mockPeripheral withService:serviceDescription];
  
  [[mockPeripheral stub] discoverServices:@[[self serviceID]]];
  [testMultiplexer centralManager:mockCentralManager didConnectPeripheral:mockPeripheral];
  
  BLECentralServiceDescription* serviceDesc = [self serviceDescription];
  id mockService = [OCMockObject mockForClass:[CBService class]];
  [[[mockService stub] andReturn:serviceDesc.serviceID] UUID];
  [[[mockPeripheral stub] andReturn:@[mockService]] services];
  NSMutableArray* characteristicsOnMockService = [NSMutableArray array];
  [[[mockService stub] andReturn:characteristicsOnMockService] characteristics];
  [[mockPeripheral stub] discoverCharacteristics:serviceDesc.characteristicIDs forService:mockService];
  [testMultiplexer peripheral:mockPeripheral didDiscoverServices:nil];
  
  id mockInboundCharacteristic = [OCMockObject mockForClass:[CBCharacteristic class]];
  id mockOutboundCharacteristic = [OCMockObject mockForClass:[CBCharacteristic class]];
  [[[mockInboundCharacteristic stub] andReturn:serviceDesc.characteristicIDs[0]] UUID];
  [[[mockOutboundCharacteristic stub] andReturn:serviceDesc.characteristicIDs[1]] UUID];
  [characteristicsOnMockService addObjectsFromArray:@[mockInboundCharacteristic, mockOutboundCharacteristic]];
  [[mockServiceDelegate stub] service:serviceDesc discoveredCharacteristic:mockInboundCharacteristic forPeripheral:mockPeripheral];
  [[mockServiceDelegate stub] service:serviceDesc discoveredCharacteristic:mockOutboundCharacteristic forPeripheral:mockPeripheral];
  [testMultiplexer peripheral:mockPeripheral didDiscoverCharacteristicsForService:mockService error:nil];
  
  [[[mockInboundCharacteristic stub] andReturn:mockService] service];
  BOOL isNotifying = YES;
  [[[mockInboundCharacteristic stub] andReturnValue:OCMOCK_VALUE(isNotifying)] isNotifying];
  [[mockServiceDelegate expect] service:serviceDesc didUpdateNotificationStateForCharacteristic:mockInboundCharacteristic forPeripheral:mockPeripheral error:nil];
  [testMultiplexer peripheral:mockPeripheral didUpdateNotificationStateForCharacteristic:mockInboundCharacteristic error:nil];
}

- (void)testUpdateAdvertisement
{
  
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  [[mockCentralManager stub] scanForPeripheralsWithServices:@[[self serviceID]] options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
  [testMultiplexer startScanningForService:serviceDescription];
  
  UInt64 mfgID = 0x01;
  NSData* mfgData = [NSData dataWithBytes:&mfgID length:sizeof(mfgID)];
  NSDictionary* advertisementData = @{CBAdvertisementDataServiceUUIDsKey: @[[self serviceID]],
                                      CBAdvertisementDataLocalNameKey: @"Fake Name",
                                      CBAdvertisementDataManufacturerDataKey: mfgData};
  id mockPeripheral = [self newMockPeripheral];
  
  [[mockServiceDelegate expect] service:[self serviceDescription] didDiscoverPeripheral:mockPeripheral withRSSI:OCMOCK_ANY advertisementData:advertisementData];
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@0];
  
  // We see the peripheral
  // When Advertisement changes see update
  advertisementData = @{CBAdvertisementDataServiceUUIDsKey: @[[self serviceID]],
                        CBAdvertisementDataLocalNameKey: @"Other Fake Name",
                        CBAdvertisementDataManufacturerDataKey: mfgData};
  
  [[mockServiceDelegate expect] service:[self serviceDescription] peripheral:mockPeripheral didUpdateAdvertisementData:advertisementData];
  
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@0];
  
}

- (void)testUpdateAdvertisementNoChange
{
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  [[mockCentralManager stub] scanForPeripheralsWithServices:@[[self serviceID]] options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
  [testMultiplexer startScanningForService:serviceDescription];
  
  UInt64 mfgID = 0x01;
  NSData* mfgData = [NSData dataWithBytes:&mfgID length:sizeof(mfgID)];
  NSDictionary* advertisementData = @{CBAdvertisementDataServiceUUIDsKey: @[[self serviceID]],
                                      CBAdvertisementDataLocalNameKey: @"Fake Name",
                                      CBAdvertisementDataManufacturerDataKey: mfgData};
  id mockPeripheral = [self newMockPeripheral];
  
  [[mockServiceDelegate expect] service:[self serviceDescription] didDiscoverPeripheral:mockPeripheral withRSSI:OCMOCK_ANY advertisementData:advertisementData];
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@0];
  
  // We see the peripheral
  // When Advertisement do NOT change we don't get updates
  advertisementData = @{CBAdvertisementDataServiceUUIDsKey: @[[self serviceID]],
                        CBAdvertisementDataLocalNameKey: @"Fake Name",
                        CBAdvertisementDataManufacturerDataKey: mfgData};
  
  [[mockServiceDelegate reject] service:[self serviceDescription] peripheral:mockPeripheral didUpdateAdvertisementData:advertisementData];
  
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@0];

}

- (void)testRSSIUpdate
{
  BLECentralServiceDescription* serviceDescription = [self serviceDescription];
  [testMultiplexer registerService:serviceDescription];
  [[mockCentralManager stub] scanForPeripheralsWithServices:@[[self serviceID]] options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
  [testMultiplexer startScanningForService:serviceDescription];
  
  UInt64 mfgID = 0x01;
  NSData* mfgData = [NSData dataWithBytes:&mfgID length:sizeof(mfgID)];
  NSDictionary* advertisementData = @{CBAdvertisementDataServiceUUIDsKey: @[[self serviceID]],
                                      CBAdvertisementDataLocalNameKey: @"Fake Name",
                                      CBAdvertisementDataManufacturerDataKey: mfgData};
  id mockPeripheral = [self newMockPeripheral];
  
  [[mockServiceDelegate expect] service:[self serviceDescription] didDiscoverPeripheral:mockPeripheral withRSSI:OCMOCK_ANY advertisementData:advertisementData];
  
  // Check init value
  float initialRSSIValue = -56.0f;
  
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@(initialRSSIValue)];
  // Sweep timer causes RSSI update
  [[mockServiceDelegate expect] service:[self serviceDescription] peripheral:mockPeripheral didUpdateRSSI:@(initialRSSIValue)];
  [testMultiplexer expireAdvertisementsOlderThan:CFAbsoluteTimeGetCurrent() - 1.0];

  // Check RSSI Upward update
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@-35];
  [[mockServiceDelegate expect] service:[self serviceDescription] peripheral:mockPeripheral didUpdateRSSI:@(initialRSSIValue + 1.5)];
  [testMultiplexer expireAdvertisementsOlderThan:CFAbsoluteTimeGetCurrent() - 1.0];

  // Check RSSI Downward update
  [testMultiplexer centralManager:mockCentralManager didDiscoverPeripheral:mockPeripheral advertisementData:advertisementData RSSI:@-75];
  [[mockServiceDelegate expect] service:[self serviceDescription] peripheral:mockPeripheral didUpdateRSSI:@(initialRSSIValue)];
  [testMultiplexer expireAdvertisementsOlderThan:CFAbsoluteTimeGetCurrent() - 1.0];
}

// TODO: write tests for unregistering a service when there are connected peripherals on that service
// (all of the peripherals should get disconnected, and all of the clients should get disconnect callbacks).


@end
