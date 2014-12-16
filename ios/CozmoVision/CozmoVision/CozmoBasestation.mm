//
//  CozmoBasestation.m
//  CozmoVision
//
//  Created by Andrew Stein on 12/5/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import "AppDelegate.h"

#import "CozmoBasestation+ImageProcessing.h"

#import "CozmoBasestation.h"


// Cozmo includes
#import <anki/cozmo/basestation/basestation.h>
#import <anki/cozmo/robot/cozmoConfig.h>
#import <anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h>

// Comms
#import <anki/cozmo/basestation/robotComms.h>
#import <anki/cozmo/basestation/uiTcpComms.h>

// Coretech-Common includes
#import <anki/common/basestation/utils/logging/DAS/DAS.h>
#import <anki/common/basestation/jsonTools.h>

// Coretech-Vision includes
#import <anki/vision/basestation/image_impl.h>

// External includes
#import "json/json.h"
#import <opencv2/opencv.hpp>

#import <fstream>



#define USE_BLE_ROBOT_COMMS 0

#define PERIOD(frameRate) ( 1.0 / frameRate )

@interface CozmoBasestation ()

@property (assign, nonatomic) CozmoBasestationRunState runState;
@property (assign, nonatomic) BOOL robotConnected;

// Config
@property (strong, nonatomic) NSString* _hostAdvertisingIP;
@property (strong, nonatomic) NSString* _basestationIP;
@property (assign, nonatomic) NSTimeInterval _heartbeatRate;
@property (assign, nonatomic) NSTimeInterval _heartbeatPeriod;

@property (strong, nonatomic) NSTimer* _heartbeatTimer;
// Listeners
// TODO: Make this a SET with weak references
@property (strong, nonatomic) NSMutableArray* _heartbeatListeners;


@property (strong, nonatomic) NSDictionary *_connectedRobots;

@end


@implementation CozmoBasestation {

  Json::Value    _config;
  CFAbsoluteTime _firstHeartbeatTimestamp;
  CFAbsoluteTime _lastHeartbeatTimestamp;

  Anki::Cozmo::BasestationMain*   _basestation;

  // Comms
  Anki::Cozmo::RobotComms*        _robotComms;
  Anki::Cozmo::UiTCPComms*        _uiComms;
  
}



+ (instancetype)defaultBasestation
{
  return ((AppDelegate*)[[UIApplication sharedApplication] delegate]).basestation;
}

- (instancetype)init
{
  if (!(self = [super init]))

  self.runState = CozmoBasestationRunStateNone;
  self.robotConnected = NO;

  self._heartbeatListeners = [NSMutableArray new];
  self._connectedRobots = [NSMutableDictionary new]; // { robotId: NSNumber, robot: ??}

  self._hostAdvertisingIP = [NSString stringWithUTF8String:DEFAULT_ADVERTISING_HOST_IP];
  self._basestationIP = [NSString stringWithUTF8String:DEFAULT_BASESTATION_IP];
  [self setHeartbeatRate:DEFAULT_HEARTBEAT_RATE];

  return self;
}

- (void)dealloc
{
#ifdef __cplusplus
  if(_uiComms) {
    delete _uiComms;
    _uiComms = nil;
  }
  
  if(_robotComms) {
    delete _robotComms;
    _robotComms = nil;
  }
  
  if(_basestation) {
    delete _basestation;
    _basestation = nil;
  }
#endif
}


#pragma mark - Config Parameters

// Can only set config properties when the runState == stopped
- (NSString*)hostAdvertisingIP
{
  return self._hostAdvertisingIP;
}

- (BOOL)setHostAdvertisingIP:(NSString*)advertisingIP
{
  if (self.runState == CozmoBasestationRunStateNone && advertisingIP.length > 0) {
    self._hostAdvertisingIP = advertisingIP;
    return YES;
  }
  return NO;
}

- (NSString*)basestationIP
{
  return self._basestationIP;
}

- (BOOL)setBasestationIP:(NSString*)basestationIP
{
  if (self.runState == CozmoBasestationRunStateNone && basestationIP.length > 0) {
    self._basestationIP = basestationIP;
    return YES;
  }
  return NO;
}

// Run loop frequency (Ticks per second)
- (double)heartbeatRate
{
  return self._heartbeatRate;
}

- (BOOL)setHeartbeatRate:(NSTimeInterval)rate
{
  if (self.runState == CozmoBasestationRunStateNone) {
    self._heartbeatRate = rate;
    self._heartbeatPeriod = PERIOD(self._heartbeatRate);
    return YES;
  }
  return NO;
}


#pragma mark - Basestation state methods

- (BOOL)startComms
{
  [self setupConfig];

# ifdef __cplusplus
  ///// Comms Init //////
  using namespace Anki;

#if (USE_BLE_ROBOT_COMMS)
  BLEComms robotComms;
  BLERobotManager robotBLEManager;
#else
  _robotComms = new Cozmo::RobotComms(_config["AdvertisingHostIP"].asCString());

  // Connect to robot.
  // UI-layer should handle this (whether the connection is TCP or BTLE).
  // Start basestation only when connections have been established.
  _uiComms = new Cozmo::UiTCPComms();
#endif

#endif

  self.runState = CozmoBasestationRunStateCommsReady;

  [self resetHeartbeatTimer];

  return YES;
}

- (void)setupConfig
{
  // Get configuration JSON
  Json::Value config;
  /*
   //const std::string subPath(std::string("basestation/config/") + std::string(AnkiUtil::kP_CONFIG_JSON_FILE));
   //const std::string jsonFilename = PREPEND_SCOPED_PATH(Config, subPath);
   const std::string jsonFilename(AnkiUtil::kP_CONFIG_JSON_FILE);
   Json::Reader reader;
   std::ifstream jsonFile(jsonFilename);
   reader.parse(jsonFile, config);
   jsonFile.close();

   // Get basestation mode
   Cozmo::BasestationMode bm;
   Json::Value bmValue = JsonTools::GetValueOptional(config, "basestation_mode", (u8&)bm);
   assert(bm <= Cozmo::BM_PLAYBACK_SESSION);
   */
  _config = config;
  if(!config.isMember("AdvertisingHostIP")) {
    _config["AdvertisingHostIP"] = self._hostAdvertisingIP.UTF8String;
  }
}


#pragma mark - Change Basestation run state

- (void)resetHeartbeatTimer
{
  if (self._heartbeatTimer) {
    [self._heartbeatTimer invalidate];
    self._heartbeatTimer = nil;
  }

  // Once done initializing, this will start the basestation timer loop
  _firstHeartbeatTimestamp = _lastHeartbeatTimestamp = CFAbsoluteTimeGetCurrent();
  self._heartbeatTimer = [NSTimer scheduledTimerWithTimeInterval:self._heartbeatPeriod
                                                          target:self
                                                        selector:@selector(gameHeartbeat:)
                                                        userInfo:nil
                                                         repeats:YES];
}

- (BOOL)startBasestation
{
  // Init basestation & connect
  BOOL didStart = NO;
  if (self.runState == CozmoBasestationRunStateRobotConnected && self.robotConnected) {
    // We have connected robots. Init the basestation!
# ifdef __cplusplus
    using namespace Anki;
    _basestation = new Cozmo::BasestationMain();
#endif

    Cozmo::BasestationStatus status = _basestation->Init(_robotComms, _uiComms, _config, Cozmo::BM_DEFAULT);
    if (status == Cozmo::BS_OK) {
      self.runState = CozmoBasestationRunStateRunning;
      didStart = YES;
    }
    else {
      NSLog(@"Basestation.Init.Failed with status %d\n", status);
    }

#if (USE_BLE_ROBOT_COMMS)
    basestationController.step(BS_TIME_STEP);
    robotBLEManager.DiscoverRobots();
#endif


#if (USE_BLE_ROBOT_COMMS)
    // Wait until robot is connected
    while (basestationController.step(BS_TIME_STEP) != -1) {
      robotBLEManager.Update();
      if (robotBLEManager.GetNumConnectedRobots() > 0) {
        printf("Connected to robot!\n");
        break;
      }
    }
#endif
  }
  return didStart;

}

- (void)stopCommsAndBasestation
{
  // Kill basestation
#ifdef __cplusplus
  if(_uiComms) {
    delete _uiComms;
    _uiComms = nil;
  }

  if(_robotComms) {
    delete _robotComms;
    _robotComms = nil;
  }

  if(_basestation) {
    delete _basestation;
    _basestation = nil;
  }

  self.runState = CozmoBasestationRunStateNone;
  [self._heartbeatTimer invalidate];
  self._heartbeatTimer = nil;
#endif
}


#pragma mark - Connect to Robot

- (BOOL)attemptToConnectToRobot
{
  BOOL didConnect = NO;
  std::vector<int> advertisingRobotIDs;
  if (_robotComms->GetAdvertisingRobotIDs(advertisingRobotIDs) > 0) {

    for(auto robotID : advertisingRobotIDs) {
      printf("RobotComms connecting to robot %d.\n", robotID);

      if (_robotComms->ConnectToRobotByID(robotID)) {
        printf("Connected to robot %d\n", robotID);

        // Add connected_robot ID to config
        _config[AnkiUtil::kP_CONNECTED_ROBOTS].append(robotID);

        self.runState = CozmoBasestationRunStateRobotConnected;
        didConnect = YES;

        break;
      } else {
        printf("Failed to connect to robot %d\n", robotID);
      }
    }
  }

  return didConnect;
}


#pragma mark - Main Game Update Loop
// Main Game Update Loop
- (void)gameHeartbeat:(NSTimer*)timer
{
  CFAbsoluteTime thisHeartbeatTimestamp = CFAbsoluteTimeGetCurrent();
  CFTimeInterval thisHeartbeatDelta = thisHeartbeatTimestamp - _lastHeartbeatTimestamp;
  if(thisHeartbeatDelta > (__heartbeatPeriod + 0.003)) {
    DASWarn("RHLocalGame.heartBeatMissedByMS_f", "%lf", thisHeartbeatDelta * 1000.0);
  }
  _lastHeartbeatTimestamp = thisHeartbeatTimestamp;

  switch (_runState) {
    case CozmoBasestationRunStateNone:
    {
      // Do Nothing
    }
      break;

    case CozmoBasestationRunStateCommsReady:
    {
      [self gameHeartbeatCommsReadyStateWithTimestamp:thisHeartbeatTimestamp];
    }
      break;

    case CozmoBasestationRunStateRobotConnected:
    {
      // Wait for basestation to start
    }
      break;

    case CozmoBasestationRunStateRunning:
    {
      [self gameHeartbeatRunningStateWithTimestamp:thisHeartbeatTimestamp];
    }
      break;

    default:
      break;
  }
}

- (void)gameHeartbeatCommsReadyStateWithTimestamp:(CFTimeInterval)timestamp
{
  DASDebug("Basestation.gameHeartBeat()", "Waiting for robots to connect at time %f", timestamp);
  // ====== TCP ======
  _robotComms->Update();

  if ([self attemptToConnectToRobot]) {
    if (_robotComms->GetNumConnectedRobots() > 0) {
      self.robotConnected = YES;
    }
  }
}

- (void)gameHeartbeatRunningStateWithTimestamp:(CFTimeInterval)timestamp
{
  DASDebug("Basestation.gameHeartBeat()", "Basestation heartbeat at time %f", timestamp);

  // Read all UI messages
  _uiComms->Update();

  // Read messages from all robots
#if (USE_BLE_ROBOT_COMMS)
  robotBLEManager.Update();  // Necessary?
#else
  _robotComms->Update();
#endif

  // Update the basestation itself (should the basestation tell the comms to update?)
  _basestation->Update(timestamp);

  // Call all listeners
  [self._heartbeatListeners makeObjectsPerformSelector:@selector(cozmoBasestationHearbeat:) withObject:self];
}


#pragma mark - Listeners

- (void)addListener:(id<CozmoBasestationHeartbeatListener>)listener
{
  [self._heartbeatListeners addObject:listener];
}

- (void)removeListener:(id<CozmoBasestationHeartbeatListener>)listener
{
  [self._heartbeatListeners removeObject:listener];
}


# pragma mark - Public methods

- (UIImage*)imageFrameWtihRobotId:(uint8_t)robotId
{
  UIImage *imageFrame = nil;
  Anki::Vision::Image img;
  if (true == _basestation->GetCurrentRobotImage(1, img))
  {
    // TODO: Somehow get rid of this copy, but still be threadsafe
    //cv::Mat_<u8> cvMatImg(nrows, ncols, const_cast<unsigned char*>(imageDataPtr));
    imageFrame = [self UIImageFromCVMat:img.get_CvMat_()];
  }

  return imageFrame;
}


@end
