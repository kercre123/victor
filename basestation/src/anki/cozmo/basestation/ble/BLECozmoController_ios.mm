/**
* File: BLECozmoController_ios
*
* Author: Lee Crippen
* Created: 04/26/16
*
* Description:
*
* Copyright: Anki, Inc. 2016
*
**/

#include "anki/cozmo/basestation/ble/BLECozmoController_ios.h"
#include "anki/cozmo/basestation/ble/BLESystem.h"
#include "anki/cozmo/basestation/ble/IBLECozmoResponder.h"

#import <Foundation/Foundation.h>
#import "BLECozmoServiceDelegate.h"
#import "BLECentralMultiplexer.h"
#import "BLECozmoManager.h"


@interface BLECozmoDelegateAdapter : NSObject<BLECozmoServiceDelegate>
@property (readwrite,unsafe_unretained) Anki::Cozmo::IBLECozmoResponder* bleResponder;
@end


namespace Anki {
namespace Cozmo {
  
struct BLECozmoControllerImpl
{
  BLECozmoDelegateAdapter*  _cozmoDelegateAdapter;
  BLECentralMultiplexer*    _centralMultiplexer;
  BLECozmoManager*          _cozmoManager;
  dispatch_queue_t          _bleQueue;
};
  
  
BLECozmoController::BLECozmoController(IBLECozmoResponder* bleResponder)
  : _impl(new BLECozmoControllerImpl{})
{
  _impl->_cozmoDelegateAdapter = [[BLECozmoDelegateAdapter alloc] init];
  _impl->_cozmoDelegateAdapter.bleResponder = bleResponder;
  _impl->_bleQueue = dispatch_queue_create("com.anki.BLECentralMultiplexer", DISPATCH_QUEUE_SERIAL);
  CBCentralManager* centralManager = [[CBCentralManager alloc] initWithDelegate:nil
                                                                          queue:_impl->_bleQueue];
  _impl->_centralMultiplexer = [[BLECentralMultiplexer alloc] initWithCentralManager:centralManager queue:_impl->_bleQueue];
  _impl->_cozmoManager = [[BLECozmoManager alloc] initWithCentralMultiplexer:_impl->_centralMultiplexer
                                                                       queue:_impl->_bleQueue];
  [_impl->_cozmoManager setServiceDelegate:_impl->_cozmoDelegateAdapter];
}
  
BLECozmoController::~BLECozmoController() = default;
  

void BLECozmoController::StartDiscoveringVehicles()
{
  __weak __typeof(_impl->_cozmoManager) weakManager = _impl->_cozmoManager;
  dispatch_async(_impl->_bleQueue, ^{
    [weakManager discoverVehicles];
  });
}

void BLECozmoController::StopDiscoveringVehicles()
{
  __weak __typeof(_impl->_cozmoManager) weakManager = _impl->_cozmoManager;
  dispatch_async(_impl->_bleQueue, ^{
    [weakManager stopDiscoveringVehicles];
  });
}

void BLECozmoController::Connect(UUIDBytes vehicleId)
{
  __weak __typeof(_impl->_cozmoManager) weakManager = _impl->_cozmoManager;
  dispatch_async(_impl->_bleQueue, ^{
    [weakManager connectToVehicleByMfgID:[[NSUUID alloc] initWithUUIDBytes:vehicleId.bytes]];
  });
}

void BLECozmoController::Disconnect(UUIDBytes vehicleId)
{
  __weak __typeof(_impl->_cozmoManager) weakManager = _impl->_cozmoManager;
  dispatch_async(_impl->_bleQueue, ^{
    [weakManager disconnectVehicleWithMfgId:[[NSUUID alloc] initWithUUIDBytes:vehicleId.bytes]];
  });
}

void BLECozmoController::SendMessage(UUIDBytes vehicleId, const uint8_t *message, const size_t size) const
{
  BLECozmoConnection* connection = [_impl->_cozmoManager connectionForMfgID:[[NSUUID alloc] initWithUUIDBytes:vehicleId.bytes]];
  NSData* messageData = [NSData dataWithBytes:message length:size];
  __weak __typeof(connection) weakConnection = connection;
  dispatch_async(_impl->_bleQueue, ^{
    [weakConnection writeData:messageData error:nil];
  });
}
  
} // namespace Cozmo
} // namespace Anki



@implementation BLECozmoDelegateAdapter

-(void)vehicleManager:(BLECozmoManager*)manager vehicleDidAppear:(BLECozmoConnection *)connection
{
  UUIDBytes uuid;
  UUIDBytesFromString(&uuid, [[[connection mfgID] UUIDString] UTF8String]);
  _bleResponder->OnVehicleDiscovered(uuid);
}

-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidDisappear:(BLECozmoConnection *)connection
{
  UUIDBytes uuid;
  UUIDBytesFromString(&uuid, [[[connection mfgID] UUIDString] UTF8String]);
  _bleResponder->OnVehicleDisappeared(uuid);
}

-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidConnect:(BLECozmoConnection *)connection
{
  UUIDBytes uuid;
  UUIDBytesFromString(&uuid, [[[connection mfgID] UUIDString] UTF8String]);
  _bleResponder->OnVehicleConnected(uuid);
}

-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidDisconnect:(BLECozmoConnection *)connection
{
  UUIDBytes uuid;
  UUIDBytesFromString(&uuid, [[[connection mfgID] UUIDString] UTF8String]);
  _bleResponder->OnVehicleDisconnected(uuid);
}

-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidUpdateAdvertisement:(BLECozmoConnection *)connection
{
  // TODO:(lc) if we find a use case for needing the data out of advertisements, hook this up.
}

-(void)vehicleManager:(BLECozmoManager *)manager vehicleDidUpdateProximity:(BLECozmoConnection *)connection
{
  UUIDBytes uuid;
  UUIDBytesFromString(&uuid, [[[connection mfgID] UUIDString] UTF8String]);
  // Note the IsClose variable is only possible on android, so always false on iOS
  _bleResponder->OnVehicleProximityChanged(uuid, (int)connection.scannedRSSI, false);
}

-(void)vehicleManager:(BLECozmoManager *)manager vehicle:(BLECozmoConnection *)connection didReceiveMessage:(NSData *)message
{
  UUIDBytes uuid;
  UUIDBytesFromString(&uuid, [[[connection mfgID] UUIDString] UTF8String]);
  std::vector<uint8_t> messageBytes(message.length);
  [message getBytes:messageBytes.data() length:message.length];
  _bleResponder->OnVehicleMessageReceived(uuid, std::move(messageBytes));
}

@end
