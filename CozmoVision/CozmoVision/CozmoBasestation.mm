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
#import "CozmoOperator.h"

#import "CozmoObsObjectBBox.h"


// Cozmo includes
#import <anki/cozmo/basestation/basestation.h>
#import <anki/cozmo/robot/cozmoConfig.h>
#import <anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h>
#import <anki/cozmo/basestation/cozmoEngine.h>

// Comms
//#import <anki/cozmo/basestation/multiClientComms.h>
//#import <anki/messaging/basestation/advertisementService.h>

// Coretech-Common includes
#import <anki/common/basestation/utils/logging/DAS/DAS.h>
#import <anki/common/basestation/jsonTools.h>
#import <anki/common/basestation/math/rect_impl.h>

// Coretech-Vision includes
#import <anki/vision/basestation/image_impl.h>

// External includes
#import "json/json.h"
#import <opencv2/opencv.hpp>

#import <fstream>



#define USE_BLE_ROBOT_COMMS 0

#define PERIOD(frameRate) ( 1.0 / frameRate )

// For connecting to physical robot
// TODO: Expose these to UI
const bool FORCE_ADD_ROBOT = false;
const bool FORCED_ROBOT_IS_SIM = true;
const u8 forcedRobotId = 1;
const char* forcedRobotIP = "192.168.19.238";


@interface CozmoBasestation ()

//@property (assign, nonatomic) CozmoBasestationRunState runState;
@property (assign, nonatomic) BOOL engineRunning;
@property (assign, nonatomic) BOOL robotConnected;
@property (assign, nonatomic) BOOL uiConnected;

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

@property (strong, nonatomic) CozmoOperator *cozmoOperator;

@end


@implementation CozmoBasestation {

  Json::Value    _config;
  CFAbsoluteTime _firstHeartbeatTimestamp;
  CFAbsoluteTime _lastHeartbeatTimestamp;

  Anki::Cozmo::CozmoEngine*       _cozmoEngine;
  
  //Anki::Cozmo::BasestationMain*   _basestation;

  // Timestamp of last image we asked for from the robot
  // TODO: need one per robot?
  Anki::TimeStamp_t               _lastImageTimestamp;
  
  // Comms
  //Anki::Cozmo::MultiClientComms*        _robotComms;
  //Anki::Cozmo::MultiClientComms*        _uiComms;
 
  //Anki::Comms::AdvertisementService*    _robotAdService;
  //Anki::Comms::AdvertisementService*    _uiAdService;
}



+ (instancetype)defaultBasestation
{
  return ((AppDelegate*)[[UIApplication sharedApplication] delegate]).basestation;
}

- (instancetype)init
{
  if (!(self = [super init]))
    return self;
  
  _cozmoEngine = nullptr;
  
  //self.runState = CozmoBasestationRunStateNone;
  self.engineRunning  = NO;
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
  [self stopCommsAndBasestation];
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

- (BOOL) start:(BOOL)asHost
{
  // Put all the settings in the Json config file
  [self setupConfig];
  
  self.cozmoOperator = [CozmoOperator operatorWithAdvertisingtHostIPAddress:self._hostAdvertisingIP];
  
  if(_cozmoEngine != nullptr) {
    delete _cozmoEngine;
    _cozmoEngine = nullptr;
  }
  
  if(asHost) {
    Anki::Cozmo::CozmoEngineHost* cozmoEngineHost = new Anki::Cozmo::CozmoEngineHost();
    cozmoEngineHost->Init(_config);
    
    // Force add a robot
    if (FORCE_ADD_ROBOT) {
      cozmoEngineHost->ForceAddRobot(forcedRobotId, forcedRobotIP, FORCED_ROBOT_IS_SIM);
    }
  
    _cozmoEngine = cozmoEngineHost;
    
  } else { // as Client
    Anki::Cozmo::CozmoEngineClient* cozmoEngineClient = new Anki::Cozmo::CozmoEngineClient();
    cozmoEngineClient->Init(_config);
    _cozmoEngine = cozmoEngineClient;
  }
  
  [self resetHeartbeatTimer];
  
  self.engineRunning = YES;
  
  return YES;
}


- (void)setupConfig
{
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

  // Put all settings into the config file for initializing the CozmoEngine
  _config[AnkiUtil::kP_ADVERTISING_HOST_IP] = self._hostAdvertisingIP.UTF8String;
  _config[AnkiUtil::kP_VIZ_HOST_IP] = self._hostAdvertisingIP.UTF8String;
  
  // TODO: Provide ability to set these from app?
  _config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT] = Anki::Cozmo::ROBOT_ADVERTISING_PORT;
  _config[AnkiUtil::kP_UI_ADVERTISING_PORT]    = Anki::Cozmo::UI_ADVERTISING_PORT;
  
  _config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR]     = 1;
  _config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR] = 1;
  
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

- (void)stopCommsAndBasestation
{
  if(_cozmoEngine != nullptr) {
    delete _cozmoEngine;
    _cozmoEngine = nullptr;
  }
  
  self.cozmoOperator = nil;
  
  //self.runState = CozmoBasestationRunStateNone;
  self.engineRunning = NO;
  
  [self._heartbeatTimer invalidate];
  self._heartbeatTimer = nil;
}



#pragma mark - Main Game Update Loop
// Main Game Update Loop
- (void)gameHeartbeat:(NSTimer*)timer
{
  CFTimeInterval thisHeartbeatTimestamp = CFAbsoluteTimeGetCurrent() - _firstHeartbeatTimestamp;
  CFTimeInterval thisHeartbeatDelta = thisHeartbeatTimestamp - _lastHeartbeatTimestamp;
  if(thisHeartbeatDelta > (__heartbeatPeriod + 0.003)) {
    DASWarn("RHLocalGame.heartBeatMissedByMS_f", "%lf", thisHeartbeatDelta * 1000.0);
  }
  _lastHeartbeatTimestamp = thisHeartbeatTimestamp;

  // For now connect to anything that's available (until desired number of robots/devices reached)
  // Eventually, display available robots/devices in the UI and use a button
  // press to select ones to connect to.
  
  {
    using namespace Anki::Cozmo;
    
    // Connect to any robots we see:
    static bool haveRobot = false;
    if(!haveRobot) {
      std::vector<CozmoEngine::AdvertisingRobot> advertisingRobots;
      _cozmoEngine->GetAdvertisingRobots(advertisingRobots);
      for(auto robot : advertisingRobots) {
        if(_cozmoEngine->ConnectToRobot(robot)) {
          NSLog(@"Connected to robot %d\n", robot);
          haveRobot = true;
        }
      }
    }
    
    // Connect to any UI devices we see:
    std::vector<CozmoEngine::AdvertisingUiDevice> advertisingUiDevices;
    _cozmoEngine->GetAdvertisingUiDevices(advertisingUiDevices);
    for(auto device : advertisingUiDevices) {
      if(_cozmoEngine->ConnectToUiDevice(device)) {
        NSLog(@"Connected to UI device %d\n", device);
      }
    }
    
    [self.cozmoOperator update];
    
    Anki::Result status = _cozmoEngine->Update(thisHeartbeatTimestamp);
    if (status != Anki::RESULT_OK) {
      DASWarn("CozmoEngine.Update.NotOK","Status %d\n", status);
    }
  }
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


#pragma mark - Public methods

- (UIImage*)imageFrameWtihRobotId:(uint8_t)robotId
{
  using namespace Anki;
  
  UIImage *imageFrame = nil;
  Vision::Image img;
  if (true == _cozmoEngine->GetCurrentRobotImage(robotId, img, _lastImageTimestamp))
  {
    // TODO: Somehow get rid of this copy, but still be threadsafe
    //cv::Mat_<u8> cvMatImg(nrows, ncols, const_cast<unsigned char*>(imageDataPtr));
    imageFrame = [self UIImageFromCVMat:img.get_CvMat_()];
    _lastImageTimestamp = img.GetTimestamp();
  }

  return imageFrame;
}


- (NSArray*)boundingBoxesObservedByRobotId:(uint8_t)robotId
{
  using namespace Anki;
  
  NSMutableArray* boundingBoxes = nil;
  
  std::vector<Cozmo::BasestationMain::ObservedObjectBoundingBox> observations;
  if( true == _cozmoEngine->GetCurrentVisionMarkers(robotId, observations))
  {
    if(!observations.empty())
    {
      boundingBoxes = [[NSMutableArray alloc] init];
      
      // Turn each Basestation observed object into an Objective-C "CozmoObsObjectBBox" object
      for(auto & observation : observations) {
        CozmoObsObjectBBox* output = [[CozmoObsObjectBBox alloc] init];
        
        output.objectID = observation.objectID;
        output.boundingBox = CGRectMake(observation.boundingBox.GetX(),
                                        observation.boundingBox.GetY(),
                                        observation.boundingBox.GetWidth(),
                                        observation.boundingBox.GetHeight());
        
        [boundingBoxes addObject:output];
      }
    } // if(!observations.empty())
  }
  
  return boundingBoxes;
}

@end
