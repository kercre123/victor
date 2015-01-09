//
//  CozmoEngineWrapper.mm
//  CozmoVision
//
//  Created by Andrew Stein on 12/5/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import "AppDelegate.h"

#import "CozmoEngineWrapper+ImageProcessing.h"

#import "CozmoEngineWrapper.h"
#import "CozmoOperator.h"

#import "CozmoObsObjectBBox.h"


// Cozmo includes
#import <anki/cozmo/basestation/basestation.h>
#import <anki/cozmo/shared/cozmoConfig.h>
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


@interface CozmoEngineWrapper ()

@property (assign, nonatomic) CozmoEngineRunState runState;

// Config
@property (strong, nonatomic) NSString* _hostAdvertisingIP;
@property (strong, nonatomic) NSString* _vizIP;
@property (assign, nonatomic) NSTimeInterval _heartbeatRate;
@property (assign, nonatomic) NSTimeInterval _heartbeatPeriod;
@property (assign, nonatomic) int _numRobotsToWaitFor;
@property (assign, nonatomic) int _numUiDevicesToWaitFor;

@property (strong, nonatomic) NSTimer* _heartbeatTimer;
// Listeners
// TODO: Make this a SET with weak references
@property (strong, nonatomic) NSMutableArray* _heartbeatListeners;


@property (strong, nonatomic) NSDictionary *_connectedRobots;

@property (strong, nonatomic) CozmoOperator *cozmoOperator;

@end


@implementation CozmoEngineWrapper {

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



+ (instancetype)defaultEngine
{
  return ((AppDelegate*)[[UIApplication sharedApplication] delegate]).cozmoEngineWrapper;
}

- (instancetype)init
{
  if (!(self = [super init]))
    return self;
  
  _cozmoEngine = nullptr;

  self.runState = CozmoEngineRunStateNone;

  self._heartbeatListeners = [NSMutableArray new];
  self._connectedRobots = [NSMutableDictionary new]; // { robotId: NSNumber, robot: ??}

  self._hostAdvertisingIP = [NSString stringWithUTF8String:DEFAULT_ADVERTISING_HOST_IP];
  self._vizIP = [NSString stringWithUTF8String:DEFAULT_VIZ_HOST_IP];
  [self setHeartbeatRate:DEFAULT_HEARTBEAT_RATE];
  
  self._numRobotsToWaitFor = 1;
  self._numUiDevicesToWaitFor = 1;
  
  return self;
}

- (void)dealloc
{
  [self stop];
}


#pragma mark - Config Parameters

// Can only set config properties when the runState == stopped
- (NSString*)hostAdvertisingIP
{
  return self._hostAdvertisingIP;
}

- (BOOL)setHostAdvertisingIP:(NSString*)advertisingIP
{
  if (self.runState == CozmoEngineRunStateNone && advertisingIP.length > 0) {
    self._hostAdvertisingIP = advertisingIP;
    return YES;
  }
  return NO;
}

- (NSString*)vizIP
{
  return self._vizIP;
}

- (BOOL)setVizIP:(NSString *)vizIP
{
  if (self.runState == CozmoEngineRunStateNone && vizIP.length > 0) {
    self._vizIP = vizIP;
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
  if (self.runState == CozmoEngineRunStateNone) {
    self._heartbeatRate = rate;
    self._heartbeatPeriod = PERIOD(self._heartbeatRate);
    return YES;
  }
  return NO;
}

-(int)numRobotsToWaitFor
{
  return self._numRobotsToWaitFor;
}

-(BOOL)setNumRobotsToWaitFor:(int)N
{
  if (self.runState == CozmoEngineRunStateNone) {
    self._numRobotsToWaitFor = N;
    return YES;
  }
  return NO;
}

-(int)numUiDevicesToWaitFor
{
  return self._numUiDevicesToWaitFor;
}

-(BOOL)setNumUiDevicesToWaitFor:(int)N
{
  if (self.runState == CozmoEngineRunStateNone) {
    self._numUiDevicesToWaitFor = N;
    return YES;
  }
  return NO;
}

#pragma mark - Engine state methods

- (BOOL) start:(BOOL)asHost
{
  // Put all the settings in the Json config file
  [self setupConfig];
  
  self.cozmoOperator = [CozmoOperator operatorWithAdvertisingtHostIPAddress:self._hostAdvertisingIP];
  
  if(_cozmoEngine != nullptr) {
    delete _cozmoEngine;
    _cozmoEngine = nullptr;
  }
  
  // TODO: Get device camera calibration from somewhere. For now this is bogus.
  Anki::Vision::CameraCalibration deviceCamCalib;
  
  if(asHost) {
    Anki::Cozmo::CozmoEngineHost* cozmoEngineHost = new Anki::Cozmo::CozmoEngineHost();
    cozmoEngineHost->Init(_config, deviceCamCalib);
    
    // Force add a robot
    if (FORCE_ADD_ROBOT) {
      cozmoEngineHost->ForceAddRobot(forcedRobotId, forcedRobotIP, FORCED_ROBOT_IS_SIM);
    }
  
    _cozmoEngine = cozmoEngineHost;
    
  } else { // as Client
    Anki::Cozmo::CozmoEngineClient* cozmoEngineClient = new Anki::Cozmo::CozmoEngineClient();
    cozmoEngineClient->Init(_config, deviceCamCalib);
    _cozmoEngine = cozmoEngineClient;
  }
  
  [self resetHeartbeatTimer];
  
  self.runState = CozmoEngineRunStateRunning;
  
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
  
  _config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR]     = self._numRobotsToWaitFor;
  _config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR] = self._numUiDevicesToWaitFor;
  
  // TODO: Provide ability to set these from app?
  _config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT] = Anki::Cozmo::ROBOT_ADVERTISING_PORT;
  _config[AnkiUtil::kP_UI_ADVERTISING_PORT]    = Anki::Cozmo::UI_ADVERTISING_PORT;
}


#pragma mark - Change engine run state

- (void)resetHeartbeatTimer
{
  if (self._heartbeatTimer) {
    [self._heartbeatTimer invalidate];
    self._heartbeatTimer = nil;
  }

  // Once done initializing, this will start the engine timer loop (heartbeat)
  _firstHeartbeatTimestamp = _lastHeartbeatTimestamp = CFAbsoluteTimeGetCurrent();
  self._heartbeatTimer = [NSTimer scheduledTimerWithTimeInterval:self._heartbeatPeriod
                                                          target:self
                                                        selector:@selector(engineHeartbeat:)
                                                        userInfo:nil
                                                         repeats:YES];
}

- (void)stop
{
  if(_cozmoEngine != nullptr) {
    delete _cozmoEngine;
    _cozmoEngine = nullptr;
  }
  
  self.cozmoOperator = nil;
  
  self.runState = CozmoEngineRunStateNone;
  
  [self._heartbeatTimer invalidate];
  self._heartbeatTimer = nil;
}



#pragma mark - Main Engine Update Loop
// Main Engine Update Loop
- (void)engineHeartbeat:(NSTimer*)timer
{
  CFTimeInterval thisHeartbeatTimestamp = CFAbsoluteTimeGetCurrent() - _firstHeartbeatTimestamp;
  CFTimeInterval thisHeartbeatDelta = thisHeartbeatTimestamp - _lastHeartbeatTimestamp;
  if(thisHeartbeatDelta > (self._heartbeatPeriod + 0.003)) {
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

    // Call all listeners:
    [self._heartbeatListeners makeObjectsPerformSelector:@selector(cozmoEngineWrapperHeartbeat:) withObject:self];

  }
}


#pragma mark - Listeners

- (void)addListener:(id<CozmoEngineHeartbeatListener>)listener
{
  [self._heartbeatListeners addObject:listener];
}

- (void)removeListener:(id<CozmoEngineHeartbeatListener>)listener
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

- (BOOL)checkDeviceVisionMailbox:(CGRect*)markerBBox :(int*)markerType
{
  Anki::Cozmo::MessageVisionMarker msg;
  if(true == _cozmoEngine->CheckDeviceVisionProcessingMailbox(msg)) {
    
    Float32 xMin = fmin(fmin(msg.x_imgLowerLeft, msg.x_imgLowerRight),
                        fmin(msg.x_imgUpperLeft, msg.x_imgUpperRight));
    Float32 yMin = fmin(fmin(msg.y_imgLowerLeft, msg.y_imgLowerRight),
                        fmin(msg.y_imgUpperLeft, msg.y_imgUpperRight));
    
    Float32 xMax = fmax(fmax(msg.x_imgLowerLeft, msg.x_imgLowerRight),
                        fmax(msg.x_imgUpperLeft, msg.x_imgUpperRight));
    Float32 yMax = fmax(fmax(msg.y_imgLowerLeft, msg.y_imgLowerRight),
                        fmax(msg.y_imgUpperLeft, msg.y_imgUpperRight));
    
    *markerBBox = CGRectMake(xMin, yMin, xMax-xMin, yMax-yMin);
    
    *markerType = msg.markerType;
    
    return YES;
  } else {
    return NO;
  }
}

- (void) processDeviceImage:(Anki::Vision::Image&)image
{
  _cozmoEngine->ProcessDeviceImage(image);
}

- (BOOL) wasLastDeviceImageProcessed
{
  if(true == _cozmoEngine->WasLastDeviceImageProcessed()) {
    return YES;
  } else {
    return NO;
  }
}

@end
