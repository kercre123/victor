//
//  BLEVehicleConnection.m
//  BLEManager
//
//  Created by Mark Pauley on 4/8/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#import "BLEManager/Vehicle/BLEVehicleConnection.h"
#import "BLEManager/Vehicle/BLEVehicleMessage.h"
#import <IOBluetooth/IOBluetooth.h>
#import "BLELog.h"

#define BLEGameFlashMaxLength  12 // name = 12
#define TestBit(value, pos) (!!((value) & (1<<(pos))))

static BLEVehicleAdvertisedPacket kDefaultAdvertisedPacket = {
  .state = 0,
  .firmwareVersion = 0x0001,
  // The Upgrades in advertisement packet is sent using UTBShort format, with the default 0 (no upgrade) value being 0x2020
  .upgrades = 0x20202020,
  .name = "Drive",
};

static BOOL BLEVehicleConnectionIsAdvertisingBootloader(NSString *packetString)
{
  // BRC: We need to test for 'Anki Drive', advertised by old versions of the bootloader
  const char *s = (const char *)[packetString UTF8String];
  size_t strLen = strlen(s);
  
  if (strLen >= 10 && (strncmp(s, "Anki Drive", 10) == 0) )
    return YES;
  
  // Alternatively, for newer FEP cars, the first 2 bytes will be: 0x01 0x01
  static const char BootloaderBytes[2] = { 0x01, 0x01 };
  if (strLen >= 2 && (memcmp(s, BootloaderBytes, 2)) == 0 )
    return YES;
  
  return NO;
}

static BLEVehicleAdvertisedPacket BLEVehicleConnectionParseAdvertisingPacket(NSString* packetString)
{
  BLEVehicleAdvertisedPacket d = kDefaultAdvertisedPacket;
  
  if(packetString == nil) return d;
  
  BLEAssert(packetString && packetString.length >= 1);
  
  if ( BLEVehicleConnectionIsAdvertisingBootloader(packetString) )
    return d;
  
  const uint8_t *packetData = (const uint8_t *)[packetString UTF8String];
    
#define VehicleStateOnTrack 3
#define VehicleStateFullBattery 4
#define VehicleStateLowBattery 5
#define VehicleStateOnCharger 6
  uint8_t state = (uint8_t)packetData[0] & 0xff;
  d.state.flags.onTrack = TestBit(state, VehicleStateOnTrack);
  d.state.flags.lowBattery = TestBit(state, VehicleStateLowBattery);
  d.state.flags.onCharger = TestBit(state, VehicleStateOnCharger);
  d.state.flags.fullBattery = TestBit(state, VehicleStateFullBattery);
  
  // byte 1 => firmware version LSB
  d.firmwareVersion = (packetData[1] & 0xff);
  
  if ( packetString.length > 2 ) {
    // byte 2 => firmware version MSB
    d.firmwareVersion |= ((packetData[2] & 0xff) << 8);
  }
  
  if ( packetString.length > 6 ) {
    uint16_t upgrade0 = (packetData[4] & 0xff) | ((packetData[5] & 0xff) << 8);
    uint16_t upgrade1 = (packetData[6] & 0xff) | ((packetData[7] & 0xff) << 8);
    d.upgrades = upgrade0 | (upgrade1 << 16);
  }
  
  if ( packetString.length > 8 ) {
    bzero(d.name, (sizeof d.name));
    NSString *nameString = [packetString substringFromIndex:8];
    NSUInteger maxLength = (sizeof d.name) - 1;
    NSUInteger length = 0;
    [nameString getBytes:d.name maxLength:maxLength usedLength:&length encoding:NSUTF8StringEncoding options:NSStringEncodingConversionAllowLossy range:NSMakeRange(0, nameString.length) remainingRange:NULL];
  }
  
  return d;
}

BOOL BLEVehicleAdvertisedPacketEqualToPacket(BLEVehicleAdvertisedPacket a, BLEVehicleAdvertisedPacket b) {
  return  (a.state.value == b.state.value) &&
          (a.firmwareVersion == b.firmwareVersion) &&
          (a.upgrades == b.upgrades) &&
          (memcmp(a.name, b.name, sizeof(a.name)) == 0);
}

static BOOL BLEVehicleGameFlashIsValid(const char *bytes) {
  
  NSString *s = [NSString stringWithCString:bytes encoding:NSUTF8StringEncoding];
  
  // length of bytes
  NSUInteger length = [s lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
  
  BOOL isValid = NO;
  
  if ( length > BLEGameFlashMaxLength ) {
    isValid = NO;
  } else if ( length < BLEGameFlashMaxLength ){
    isValid = YES;
  } else if ( length == BLEGameFlashMaxLength ) {
    // If the length is exactly the same length, then only allow
    // this string if the last character is not multi-byte
    NSRange lastCharacterRange = NSMakeRange(s.length-1, 1);
    NSString *lastCharacter = [s substringWithRange:lastCharacterRange];
    NSUInteger characterLength = [lastCharacter lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    isValid = (characterLength == 1) ? YES : NO;
  }
  
  // NSLog(@"NameEntryViewController.textIsValid: [%@] '%@' (%@) (%u bytes)", isValid ? @"YES" : @" NO", text, NSStringFromRange(NSMakeRange(0, text.length)), length);
  
  return isValid;
}

#define VEHICLE_FIRMWARE_INVALID_VERSION 0x0001

@interface BLEVehicleConnectionResponder : NSObject
@property (nonatomic,weak) id<NSObject> responder;
@property (nonatomic,readwrite) SEL selector;
@end

@implementation BLEVehicleConnectionResponder
@synthesize responder;
@synthesize selector;
@end

@interface BLEVehicleConnection ()
@property (nonatomic, assign) BLEVehicleAdvertisedPacket advertisedPacket;
@property (nonatomic, strong) dispatch_block_t canWriteBlock;
@end

@implementation BLEVehicleConnection {
  NSMutableDictionary* _advertisementData;
  BLEVehicleConnectionResponder* respondersByMessageType[BLEVehicleMessageTypeMax]; // uint8_max
}

@synthesize vehicleID;
@synthesize scannedRSSI;
@synthesize connectionState;
@synthesize carPeripheral;
@synthesize toCarCharacteristic;
@synthesize toPhoneCharacteristic;
@synthesize mfgData;
@synthesize advertisementData = _advertisementData;
@synthesize advertisedPacket = _advertisedPacket;
@synthesize enqueuedMessages = _enqueuedMessages;
@synthesize canWrite = _canWrite;
@synthesize throttlingDisabled = _throttlingDisabled;
@synthesize canWriteBlock = _canWriteBlock;


+ (BOOL)automaticallyNotifiesObserversOfAdvertisedPacket {
  return NO;
}

+ (BOOL)automaticallyNotifiesObserversOfName {
  return NO;
}

- (id) init
{
  [[NSException exceptionWithName:@"com.anki.BLEVehicleConnection.invalidInit"
                           reason:@"BLEVehicleConnection requires peripheral" userInfo:nil] raise];
  return nil;
}

- (id) initWithPeripheral:(CBPeripheral *)peripheral
{
  self = [super init];
  if (self) {
    self.carPeripheral = peripheral;
    self.advertisedPacket = kDefaultAdvertisedPacket;
    _advertisementData = [NSMutableDictionary dictionary];
    _enqueuedMessages = [NSMutableArray array];
    _canWrite = YES;
    _throttlingDisabled = NO;
    self.throttlingQueue = dispatch_get_main_queue();
	}
  
  return self;
}

-(void)updateWithAdvertisementData:(NSDictionary *)newAdvertisementData {
  NSString *oldPacketString = self.advertisementData[CBAdvertisementDataLocalNameKey];
  
  [_advertisementData addEntriesFromDictionary:newAdvertisementData];
  
  NSString* packetString = self.advertisementData[CBAdvertisementDataLocalNameKey];
  if ( ![packetString isEqualToString:oldPacketString] ) {
    self.advertisedPacket = BLEVehicleConnectionParseAdvertisingPacket(packetString);
  }
}

- (void)clearAdvertisementState {
  BLEVehicleAdvertisedPacket p = _advertisedPacket;
  p.state.value = 0;
  self.advertisedPacket = p;
}

- (void)setAdvertisedPacket:(BLEVehicleAdvertisedPacket)advertisedPacket {
  if ( BLEVehicleAdvertisedPacketEqualToPacket(_advertisedPacket, advertisedPacket) )
    return;
  
  BOOL namedChanged = (memcmp(advertisedPacket.name, _advertisedPacket.name, sizeof(_advertisedPacket.name)) != 0);
  
  if ( namedChanged ) [self willChangeValueForKey:@"name"];
  [self willChangeValueForKey:@"advertisedPacket"];
  _advertisedPacket = advertisedPacket;
  [self didChangeValueForKey:@"advertisedPacket"];
  if ( namedChanged ) [self didChangeValueForKey:@"name"];
}

- (UInt64)mfgID {
  BLEAssert(mfgData && mfgData.length == 8);
  if(mfgData && mfgData.bytes) {
    return CFSwapInt64BigToHost(*((UInt64*)mfgData.bytes));
  }
  return 0x00;
}

- (BOOL)isOnTrack
{
  return self.advertisedPacket.state.flags.onTrack;
}

- (BOOL)isBatteryFull
{
  return self.advertisedPacket.state.flags.fullBattery;
}

- (BOOL)isBatteryLow
{
  return self.advertisedPacket.state.flags.lowBattery;
}

- (BOOL)isOnCharger
{
  return self.advertisedPacket.state.flags.onCharger;
}

-(NSString*)name {
  const char *bytes = (const char *)self.advertisedPacket.name;
  NSString *name = [NSString stringWithUTF8String:bytes];
  
  return name.length > 0 ? name : @"Drive";
}

-(uint16_t)buildVersion {
  return self.advertisedPacket.firmwareVersion;
}

// Private:
// used to update build version after flash update,
// before a new advertising packet is received
- (void)setBuildVersion:(uint16_t)buildVersion {
  _advertisedPacket.firmwareVersion = buildVersion;
}

-(BOOL)needsFirmware {
  
  BLEVehicleAdvertisedPacket advertisedPacket = [self advertisedPacket];
  
  return advertisedPacket.firmwareVersion == VEHICLE_FIRMWARE_INVALID_VERSION;
}

-(unsigned int)upgrades {
  BLEVehicleAdvertisedPacket advertisedPacket = [self advertisedPacket];
  return advertisedPacket.upgrades;
}

-(NSString*)description {
  return [NSString stringWithFormat:@"<BLEVehicleConnection: %p name=%@ mfgID=0x%llx vehicleID=%ld rssi=%ld connectionState=%d peripheral=%p characteristic=%p>",
          self, self.name, self.mfgID, (long)self.vehicleID, (long)self.scannedRSSI, self.connectionState, self.carPeripheral, self.toCarCharacteristic];
}

- (void)readConnectedRSSI {
  if (self.carPeripheral.state == CBPeripheralStateConnected) {
    [self.carPeripheral readRSSI];
  }
}

#pragma mark - user-updates
/*
- (BOOL)sendUpdateWithName:(NSString *)name error:(NSError *__autoreleasing *)error
{
  // The name field in the advertised packet includes space for the NULL terminator
  size_t nameBufferLength = sizeof( ((BLEVehicleAdvertisedPacket *)0)->name );
  char bytes[nameBufferLength];
  bzero(bytes, nameBufferLength * sizeof(char));
  
  // Max number of bytes to retreive from name param
  // (getBytes: does not return NULL-terminated bytes)
  size_t maxLen = nameBufferLength - 1;
  NSUInteger nameLen = 0;
  [name getBytes:bytes maxLength:maxLen usedLength:&nameLen encoding:NSUTF8StringEncoding options:0 range:NSMakeRange(0, name.length) remainingRange:NULL];

  // create packetData
  unsigned char packetData[GAMEFLASH_PACKET_SIZE] = {0xff}; // initialize to 0xff (flash default)
  memset(packetData, 0, nameBufferLength); // write 0 for length of name buffer, to ensure NULL-termination
  memcpy(packetData, bytes, MIN(nameLen, GAMEFLASH_PACKET_SIZE));  // copy name
  
  if ( !BLEVehicleGameFlashIsValid((const char *)packetData) ) {
    NSError *e = [NSError errorWithDomain:@"com.anki.drive.BLEVehicleConnection.error" code:BLEVehicleConnectionErrorCodeGameFlashInvalid userInfo:@{NSLocalizedDescriptionKey: NSLocalizedString(@"GameFlash data is not valid.", nil)}];
    if (error != NULL)
      *error = e;
    
    // If the data is invalid here, then there is likely an upstream bug.
    BLELogWarn("BLEVehicleConnection.sendUpdate.error", "%s", packetData);
    
    return NO;
  }
  
  // create message packet
  CarMsg_WriteFlashSinglePacket packet;
  packet.size = SIZE_MSG_B2V_CORE_WRITE_FLASH_SINGLE_PACKET - SIZE_MSG_BASE_SIZE;
  packet.msgID = MSG_B2V_CORE_WRITE_FLASH_SINGLE_PACKET;
  packet.offset = 0x04; // The offset for the name field of flash data
  packet.dataSize = GAMEFLASH_PACKET_SIZE;
  strncpy((char *)packet.data, (const char *)packetData, GAMEFLASH_PACKET_SIZE);
  
  BLELogEvent("BLEVehicleConnection.sendUpdate.name", "name = %s, $phys = 0x%llx", bytes, self.mfgID);
 
  NSData *msgData = [NSData dataWithBytes:&packet length:(sizeof packet)];
  BOOL success = [self writeMessageData:msgData error:error];
 
 
  if ( success ) {
    // update internal data
    BLEVehicleAdvertisedPacket packet = _advertisedPacket;
    bzero(packet.name, nameBufferLength);
    strlcpy((char *)packet.name, bytes, nameBufferLength);
    [self setAdvertisedPacket:packet];
  }
  
  return success;
}
*/

#pragma mark - unbuffered I/O

- (void)writeDataToPeripheral:(NSData *)data;
{
  if ( !data )
    return;
  
  // Send this buffer to the car
  [self.carPeripheral writeValue:data forCharacteristic:self.toCarCharacteristic type:CBCharacteristicWriteWithoutResponse];
  self.bytesSent += (UInt32)data.length;
  self.messagesSent++;
}

// Send a message to the car to tell it to initiate a disconnect. If we tell
// CoreBluetooth to tear down connection directly, it doesn't always happen
// immediately on iOS 6 so we have the car tear it down instead.
- (void)disconnectFromPeripheral
{
  if ( self.connectionState == kConnectedPipeReady ) {
    // TODO: Send disconnect message to Cozmo
    /*
    unsigned char buffer[2];
    buffer[0] = 1;
    buffer[1] = MSG_B2V_BTLE_DISCONNECT;
    [self writeDataToPeripheral:[NSData dataWithBytes:buffer length:2]];
     */
  }
}

// Send disconnect message using our buffer write method
// This ensure that previous messages are flushed before disconnect
- (BOOL)disconnect:(NSError *__autoreleasing *)error
{
  // TODO: Send disconnect message to Cozmo
  /*
  unsigned char buffer[2];
  buffer[0] = 1;
  buffer[1] = MSG_B2V_BTLE_DISCONNECT;
  NSData *message = [NSData dataWithBytes:buffer length:2];
  
  return [self writeData:message error:error];
   */
  return true;
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
  [self.enqueuedMessages removeAllObjects];
  self.canWrite = YES;
}

- (void)writeBufferedData
{
  if ( self.connectionState != kConnectedPipeReady ) {
    BLELogInfo("BLEVehicleConnection.writeBufferedData.disconnected.dropBufferedMessages", "mfgId=0x%llx, count=%lu", self.mfgID, (unsigned long)self.enqueuedMessages.count);
    [self clearQueuedMessages];
    return;
  }
  
  if (self.enqueuedMessages.count == 0) {
    self.canWrite = YES;
    if(self.canWriteBlock)dispatch_async(self.throttlingQueue, self.canWriteBlock);
  } else {
    int messagesLeftInBatch = BLEVehicleConnectionMessageBatchLength;
    while(messagesLeftInBatch > 0 && self.enqueuedMessages.count > 0) {
      messagesLeftInBatch--;
      NSData* nextData = self.enqueuedMessages[0];
      [self.enqueuedMessages removeObjectAtIndex:0];
      [self writeDataToPeripheral:nextData];
    }
    
    // set the throttling timer again.
    [self writeDataDelayed];
  }
}

- (void)writeDataDelayed
{
  dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(BLEVehicleConnectionMessageBatchDelay_MSEC * NSEC_PER_MSEC));
  __weak __typeof(self) weakSelf = self;
  dispatch_after(popTime, self.throttlingQueue, ^(void){
    [weakSelf writeBufferedData];
  });
}

- (BOOL)writeMessageData:(NSData *)data error:(NSError *__autoreleasing *)error
{
  const uint8_t* messageBytes = data.bytes;
  if ( self.needsFirmware && messageBytes[1] >= 0x20 ) {
    // Vehicle is advertising from Bootloader and cannot respond to CORE messages
    NSError *e = [NSError errorWithDomain:@"com.anki.drive.BLEVehicleConnection.error" code:BLEVehicleConnectionErrorCodeNoFirmware userInfo:@{NSLocalizedDescriptionKey: NSLocalizedString(@"Vehicle has no firmware, and the bootloader cannot respond to this message.", nil), NSLocalizedRecoverySuggestionErrorKey: NSLocalizedString(@"This vehicle requires an update.", @"Error message for attempting to send a CORE message to a vehicle with no firmware")}];
    if (error != NULL)
      *error = e;
    
    return NO;
  }
  
  return [self writeData:data error:error];
}

- (BOOL)writeData:(NSData *)data error:(NSError *__autoreleasing *)error
{
  if ( self.connectionState != kConnectedPipeReady ) {
    NSError *e = [NSError errorWithDomain:@"com.anki.drive.BLEVehicleConnection.error" code:BLEVehicleConnectionErrorCodeNotConnected userInfo:@{NSLocalizedDescriptionKey: NSLocalizedString(@"Vehicle is not connected.", nil)}];
    if (error != NULL)
      *error = e;
    
    return NO;
  }
  
  if ( data.length > BLEVehicleMessageMaxMessageLength ) {
    NSError *e = [NSError errorWithDomain:@"com.anki.drive.BLEVehicleConnection.error" code:BLEVehicleConnectionErrorCodeMessageTooLong userInfo:@{NSLocalizedDescriptionKey: NSLocalizedString(@"Vehicle message exceeds maximum allowed message length", nil)}];
    if (error != NULL)
      *error = e;
    
    return NO;
  }

  if ( self.isThrottlingDisabled ) {
    [self writeDataToPeripheral:data];
    return YES;
  }
  
  if ( !self.canWrite ) {
    [self.enqueuedMessages addObject:data];
  } else {
    self.canWrite = NO;
    [self.enqueuedMessages addObject:data];
    __weak typeof(self) weakSelf = self;
    dispatch_async(self.throttlingQueue, ^{
      [weakSelf writeBufferedData];
    });    
  }
  
  return YES;
}

#pragma mark - Inbound Message Responder

#define BLEAssertResponderMethodIsValid(responder, selector) _BLEAssertResponderMethodIsValid(responder, selector)
static void _BLEAssertResponderMethodIsValid(id responder, SEL selector) {
  // Assert that the responder method returns a BOOL
  NSMethodSignature *responderSignature = [(id)responder methodSignatureForSelector:selector];
  
  // assert return type
  static const char * sTypeEncodingBOOL = @encode(BOOL);
  size_t typeSize = strlen(sTypeEncodingBOOL);
  const char * returnType __attribute__((unused)) = [responderSignature methodReturnType];
  BLEAssert((strlen(returnType) == typeSize) && (strncmp(sTypeEncodingBOOL, returnType, typeSize) == 0));
  
  // assert argument types
  static const char * sTypeEncodingObject = @encode(id);
  typeSize = strlen(sTypeEncodingObject);
  NSUInteger argCount = [responderSignature numberOfArguments];
  BLEAssert(argCount == 4); // self, _cmd, message, connection
  for ( NSUInteger i = 2; i < argCount; ++i ) {
    const char *argType __attribute__((unused)) = [responderSignature getArgumentTypeAtIndex:i];
    BLEAssert( (strlen(argType) == typeSize) && (strncmp(sTypeEncodingObject, argType, typeSize) == 0) );
  }
}

// FIXME: I use this pattern in like 3 places, it might be a good idea to condense it!
// this selector should look like: -(BOOL)respondToMessage:forConnection:
-(void)setResponder:(id<NSObject>)responder selector:(SEL)selector forMessageType:(BLEVehicleMessageType)messageType {
  BLEAssert(messageType < BLEVehicleMessageTypeMax);
  BLEAssert([responder respondsToSelector:selector]);
  BLEAssertResponderMethodIsValid(responder, selector);
  
  BLEVehicleConnectionResponder* responderWrapper = [[BLEVehicleConnectionResponder alloc] init];
  responderWrapper.responder = responder;
  responderWrapper.selector = selector;
  
  respondersByMessageType[messageType] = responderWrapper;
}

-(void)removeResponder:(id<NSObject>)responder forMessageType:(BLEVehicleMessageType)type {
  BLEAssert(type < BLEVehicleMessageTypeMax);
  if(respondersByMessageType[type].responder == responder) {
    respondersByMessageType[type] = nil;
  }
}

-(void)removeResponderForAllMessageTypes:(id<NSObject>)responder {
  for (int i = 0; i < BLEVehicleMessageTypeMax; i++) {
    if(respondersByMessageType[i].responder == responder) {
      respondersByMessageType[i] = nil;
    }
  }
}

-(BOOL)sendMessageToResponder:(BLEVehicleMessage*)message handled:(BOOL *)handled {
  BOOL success = NO;
  BLEAssert(message.messageType < BLEVehicleMessageTypeMax);
  BLEVehicleConnectionResponder* responderWrapper = respondersByMessageType[message.messageType];
  if(responderWrapper) {
    if(responderWrapper.responder) {
      BLELogDebug("BLEVehicleConnection.sendMessageToResponder", "%s (%s)",
               NSStringFromClass([responderWrapper.responder class]).UTF8String,
               NSStringFromSelector(responderWrapper.selector).UTF8String);

      // -[NSObject performSelector:withObject:withObject:] warns about an unknown selector
      // However, we've already validated this selector.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
      BOOL responderHandledMessage = (BOOL)[responderWrapper.responder performSelector:responderWrapper.selector withObject:message withObject:self];
#pragma clang diagnostic pop
      
      if ( handled != NULL ) {
        *handled = responderHandledMessage;
      }
      success = YES;
    }
    else {
      // clean the responder wrapper, since the responder is gone now.
      // this can happen behind our backs because of the weak reference.
      respondersByMessageType[message.messageType] = nil;
    }
  }
  return success;
}


@end
