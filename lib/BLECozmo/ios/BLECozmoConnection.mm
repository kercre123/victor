//
//  BLECozmoConnection.m
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 4/13/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#import "BLECozmoConnection.h"
#import "BLECozmoManager.h"
#import "BLELog.h"

#import <CoreBluetooth/CoreBluetooth.h>

#include "BLECozmoMessage.h"

#if defined(DEBUG)
#define BLECOZMOCONNECTION_DEBUG 1
#else
#define BLECOZMOCONNECTION_DEBUG 0
#endif

static BLECozmoonnectionThrottlingParams sConnectionThrottlingParams = {
  ._batchLength = kBLECozmoConnectionMessageBatchLength,
  ._batchDelayMillisec = kBLECozmoConnectionMessageBatchDelay_MSEC,
};

@interface BLECozmoConnection ()
- (BOOL)writeData:(NSData *)data error:(NSError *__autoreleasing *)error;
- (void)writeDataToPeripheral:(NSData *)data;

@property (nonatomic, strong) dispatch_block_t canWriteBlock;

@end

@implementation BLECozmoConnection {
  NSMutableDictionary* _advertisementData;
  NSData* _mfgData;
}

@synthesize scannedRSSI;
@synthesize connectionState = _connectionState;
@synthesize carPeripheral;
@synthesize toCarCharacteristic;
@synthesize toPhoneCharacteristic;

- (NSData*) mfgData
{
  return _mfgData;
}

- (void) setMfgData:(NSData *)mfgData
{
  _mfgData = mfgData;
  
  if (BLECOZMOCONNECTION_DEBUG)
  {
    NSUInteger numBytes = mfgData.length;
    NSMutableString *string = [NSMutableString stringWithCapacity:numBytes*2];
    for (NSInteger idx = 0; idx < numBytes; ++idx) {
      [string appendFormat:@"%02x", ((uint8_t*)mfgData.bytes)[idx]];
    }
    BLELogDebug("BLECozmoConnection.mfgID","0x%llx is short for 0x%s", [self mfgID], [string UTF8String]);
  }
}

- (id) init
{
  [[NSException exceptionWithName:@"com.anki.BLECozmoConnection.invalidInit"
                           reason:@"BLECozmoConnection requires peripheral" userInfo:nil] raise];
  return nil;
}

- (id)initWithPeripheral:(CBPeripheral *)peripheral vehicleManager:(BLECozmoManager *)manager;
{
  self = [super init];
  if (self) {
    self.cozmoManager = manager;
    self.carPeripheral = peripheral;
    _advertisementData = [NSMutableDictionary dictionary];
    _enqueuedMessages = [NSMutableArray array];
    _canWrite = YES;
    _latencyTimeout = 3.0;
    _throttlingDisabled = NO;
    _messageBatchLength = sConnectionThrottlingParams._batchLength;
    _messageBatchDelayMilliSec = sConnectionThrottlingParams._batchDelayMillisec;
    self.throttlingQueue = self.cozmoManager.queue;
  }
  
  return self;
}

-(void)updateWithAdvertisementData:(NSDictionary *)newAdvertisementData {
  [_advertisementData addEntriesFromDictionary:newAdvertisementData];
}

- (void)clearAdvertisementState {
  // This is where we might clear any local struct that keeps a cache of state data set up from data
  // in the advertisement packet. For now on Cozmo we don't keep any state like that
}
// Clear any state associated with a CBPeripheral connection
// This method should return the connection to a default disconnected/unavailable state.
// TODO: Consider caching characteristic UUIDs instead, but that would require more supporting changes.
- (void)clearConnectionState {
  self.toCarCharacteristic = nil;
  self.toPhoneCharacteristic = nil;
  [self clearQueuedMessages];
  
  // reset throttling
  _throttlingDisabled = NO;
}

- (UInt64)mfgID {
  // TODO:(lc) Our mfgData is larger than 8 bytes, but it's not clear whether we care about it all or what.
  // Once we know what part of the data is unique and useful, revisit this to get the data properly
  //BLEAssert(mfgData && mfgData.length == 8);
  if(_mfgData && _mfgData.bytes) {
    return CFSwapInt64BigToHost(*((UInt64*)_mfgData.bytes));
  }
  return 0x00;
}

#pragma mark - unbuffered I/O

- (void)writeDataToPeripheral:(NSData *)data;
{
  if (!data)
  {
    return;
  }
  
  // Note we do the write with response, meaning we expect reliable, in-order delivery
  if (data.length == Anki::Cozmo::BLECozmoMessage::kMessageExactMessageLength) {
    [self.carPeripheral writeValue:data forCharacteristic:self.toCarCharacteristic
                              type:CBCharacteristicWriteWithResponse];
    self.bytesSent += (UInt32) data.length;
    self.messagesSent++;
  } else {
    BLELogError("BLECozmoConnection.writeDataToPeripheral", "Message size %lu not supported, message dropped", (unsigned long)data.length);
    BLEAssert(0);
  }
}

- (void)writeDataToPeripheralFromQueue
{
  if (self.enqueuedMessages.count == 0)
  {
    return;
  }
  
  NSData* nextData = self.enqueuedMessages[0];
  [self writeDataToPeripheral: nextData];
  [self.enqueuedMessages removeObjectAtIndex:0];
}


#pragma mark - buffered I/O

- (void)setThrottlingDisabled:(BOOL)throttlingDisabled
{
  _throttlingDisabled = throttlingDisabled;
  
  // Clear queued messaged if the throttling state changes, since
  // we can not be sure that queued messages are valid.
  [self clearQueuedMessages];
}

- (void)clearQueuedMessages
{
  __weak __typeof(self) weakSelf = self;
  dispatch_async(self.throttlingQueue, ^{
    [weakSelf.enqueuedMessages removeAllObjects];
    weakSelf.canWrite = YES;
  });
}

- (void)writeBufferedData
{
  if ( self.connectionState != kConnectedPipeReady ) {
    BLELogInfo("BLECozmoConnection.writeBufferedData.disconnected.dropBufferedMessages", "mfgId=0x%llx, count=%lu", self.mfgID, (unsigned long)self.enqueuedMessages.count);
    [self clearQueuedMessages];
    return;
  }
  
  if (self.enqueuedMessages.count == 0) {
    self.canWrite = YES;
    if (self.canWriteBlock) {
      dispatch_async(self.throttlingQueue, self.canWriteBlock);
    }
  } else {
    int messagesLeftInBatch = (int)self.messageBatchLength;
    while(messagesLeftInBatch > 0 && self.enqueuedMessages.count > 0) {
      messagesLeftInBatch--;
      [self writeDataToPeripheralFromQueue];
    }
    
    // set the throttling timer again for the remaining messages
    int messagesSent = ((int) self.messageBatchLength - messagesLeftInBatch);
    int64_t delay =
    (int64_t)(messagesSent * self.messageBatchDelayMilliSec * NSEC_PER_MSEC);
    [self writeDataDelayed:delay];
  }
}

- (void)writeDataDelayed:(int64_t)delay
{
  dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, delay);
  __weak __typeof(self) weakSelf = self;
  dispatch_after(popTime, self.throttlingQueue, ^(void){
    [weakSelf writeBufferedData];
  });
}

- (BOOL)writeMessageData:(NSData *)data error:(NSError *__autoreleasing *)error encrypted:(BOOL)encrypted
{
  if (!data)
  {
    return NO;
  }
  
  Anki::Cozmo::BLECozmoMessage cozmoMessage;
  uint8_t numChunks = cozmoMessage.ChunkifyMessage(static_cast<const uint8_t*>(data.bytes), (uint32_t)data.length);
  
  if (BLECOZMOCONNECTION_DEBUG)
  {
    uint8_t dechunked[Anki::Cozmo::BLECozmoMessage::kMessageMaxTotalSize];
    cozmoMessage.DechunkifyMessage(dechunked, Anki::Cozmo::BLECozmoMessage::kMessageMaxTotalSize);
    for (int i=0; i<data.length; i++)
    {
      BLEAssert(dechunked[i] == static_cast<const uint8_t*>(data.bytes)[i]);
    }
  }
  
  for (int i=0; i < numChunks; i++)
  {
    if (![self writeData:[[NSData alloc] initWithBytes:cozmoMessage.GetChunkData(i)
                                                length:Anki::Cozmo::BLECozmoMessage::kMessageExactMessageLength]
                   error:error])
    {
      return NO;
    }
  }
  return YES;
}

- (BOOL)writeData:(NSData *)data error:(NSError *__autoreleasing *)error
{
  if ( self.connectionState != kConnectedPipeReady ) {
    NSError *e = [NSError errorWithDomain:@"com.anki.drive.BLECozmoConnection.error" code:BLECozmoConnectionErrorCodeNotConnected userInfo:@{NSLocalizedDescriptionKey: NSLocalizedString(@"Cozmo is not connected.", nil)}];
    if (error != NULL) {
      *error = e;
    }
    
    return NO;
  }
  
  if ( data.length != Anki::Cozmo::BLECozmoMessage::kMessageExactMessageLength ) {
    NSError *e = [NSError errorWithDomain:@"com.anki.drive.BLECozmoConnection.error" code:BLECozmoConnectionErrorCodeMessageSizeIncorrect userInfo:@{NSLocalizedDescriptionKey: NSLocalizedString(@"Cozmo message is not correct message length", nil)}];
    if (error != NULL) {
      *error = e;
    }
    
    return NO;
  }
  
  //BLE_Profiling_Start_Stopwatch(data);
  
  if ( self.isThrottlingDisabled ) {
    [self writeDataToPeripheral:data];
    return YES;
  }
  
  __weak __typeof(self) weakSelf = self;
  dispatch_async(self.throttlingQueue, ^{
    if ( !weakSelf.canWrite ) {
      [weakSelf.enqueuedMessages addObject:data];
    } else {
      weakSelf.canWrite = NO;
      [weakSelf.enqueuedMessages addObject:data];
      dispatch_async(self.throttlingQueue, ^{
        [weakSelf writeBufferedData];
      });
    }
  });
  
  return YES;
}

-(NSString*)description {
  return [NSString stringWithFormat:@"<BLECozmoConnection: %p mfgID=0x%llx rssi=%ld connectionState=%d peripheral=%p characteristic=%p>",
          self, self.mfgID, (long)self.scannedRSSI, self.connectionState, self.carPeripheral, self.toCarCharacteristic];
}

- (void)readConnectedRSSI {
  if (CBPeripheralStateConnected == self.carPeripheral.state) {
    [self.carPeripheral readRSSI];
  }
}

@end