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
#import <anki/cozmo/game/comms/messaging/uiMessages.h>
#import <anki/cozmo/shared/cozmoConfig.h>
#import <anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h>
#import <anki/cozmo/basestation/cozmoEngine.h>
#import <anki/cozmo/game/cozmoGame.h>


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
const bool FORCED_ROBOT_IS_SIM = false;
const u8 forcedRobotId = 1;
//const char* forcedRobotIP = "192.168.3.34";   // cozmo2
const char* forcedRobotIP = "172.31.1.1";     // cozmo3

using namespace Anki;

@interface CozmoEngineWrapper ()

@property (assign, nonatomic) CozmoEngineRunState runState;

// Config
@property (strong, nonatomic) NSString* _hostAdvertisingIP;
@property (strong, nonatomic) NSString* _vizIP;
@property (assign, nonatomic) BOOL _doForceAdd;
@property (strong, nonatomic) NSString* _forceAddIP;
@property (assign, nonatomic) NSTimeInterval _heartbeatRate;
@property (assign, nonatomic) NSTimeInterval _heartbeatPeriod;
@property (assign, nonatomic) int _numRobotsToWaitFor;
@property (assign, nonatomic) int _numUiDevicesToWaitFor;

@property (strong, nonatomic) NSTimer* _heartbeatTimer;
// Listeners
// TODO: Make this a SET with weak references
@property (strong, nonatomic) NSMutableArray* _heartbeatListeners;
@property (strong, nonatomic) NSMutableArray* _robotConnectedListeners;


@property (strong, nonatomic) NSDictionary *_connectedRobots;

@end


@implementation CozmoEngineWrapper {

  Json::Value    _config;
  CFAbsoluteTime _firstHeartbeatTimestamp;
  CFAbsoluteTime _lastHeartbeatTimestamp;

  Cozmo::CozmoGame*         _cozmoGame;
  
  // Timestamp of last image we asked for from the robot
  // TODO: need one per robot?
  TimeStamp_t               _lastImageTimestamp;
  
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
  
  self.runState = CozmoEngineRunStateNone;

  self._heartbeatListeners = [NSMutableArray new];
  self._connectedRobots = [NSMutableDictionary new]; // { robotId: NSNumber, robot: ??}

  self._hostAdvertisingIP = [NSString stringWithUTF8String:DEFAULT_ADVERTISING_HOST_IP];
  self._vizIP = [NSString stringWithUTF8String:DEFAULT_VIZ_HOST_IP];
  [self setHeartbeatRate:DEFAULT_HEARTBEAT_RATE];
  
  self._numRobotsToWaitFor = 1;
  self._numUiDevicesToWaitFor = 1;
  
  self._doForceAdd = NO;
  self._forceAddIP = nil;
  
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

- (NSString*)forceAddIP
{
  return self._forceAddIP;
}

- (BOOL)setForceAddIP:(NSString *)ip
{
  if (self.runState == CozmoEngineRunStateNone && ip.length > 0) {
    self._forceAddIP = ip;
    return YES;
  }
  return NO;
}

- (BOOL)doForceAdd
{
  return self._doForceAdd;
}

- (BOOL)setDoForceAdd:(BOOL)forceAdd
{
  if (self.runState == CozmoEngineRunStateNone) {
    self._doForceAdd = forceAdd;
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
  
  if(_cozmoGame != nullptr) {
    delete _cozmoGame;
    _cozmoGame = nullptr;
  }
  
  _cozmoGame = new Cozmo::CozmoGame();
  _cozmoGame->Init(_config);
  
  _config[AnkiUtil::kP_AS_HOST] = asHost;
  _cozmoGame->StartEngine(_config);
  
  
  // Force add a robot
  if(FORCE_ADD_ROBOT) {
    _cozmoGame->ForceAddRobot(forcedRobotId, forcedRobotIP, FORCED_ROBOT_IS_SIM);
  } else {
    if (self._doForceAdd) {
      if(self._forceAddIP == nil) {
        NSLog(@"Force add set to true, but IP is NIL!\n");
      } else {
        _cozmoGame->ForceAddRobot(1, [self._forceAddIP UTF8String], false);
      }
    }
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
  
  // TODO: Set device camera calibration parameters in the config file
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
    //DASWarn("RHLocalGame.heartBeatMissedByMS_f", "%lf", thisHeartbeatDelta * 1000.0);
  }
  _lastHeartbeatTimestamp = thisHeartbeatTimestamp;

  // For now connect to anything that's available (until desired number of robots/devices reached)
  // Eventually, display available robots/devices in the UI and use a button
  // press to select ones to connect to.
  
  {
    using namespace Anki::Cozmo;

    _cozmoGame->Update(thisHeartbeatTimestamp);
    
    // Call all listeners (including operator):
    [self._heartbeatListeners makeObjectsPerformSelector:@selector(cozmoEngineWrapperHeartbeat:) withObject:self];

  }
}


#pragma mark - Listeners

- (void)addHeartbeatListener:(id<CozmoEngineHeartbeatListener>)listener
{
  [self._heartbeatListeners addObject:listener];
}

- (void)removeHeartbeatListener:(id<CozmoEngineHeartbeatListener>)listener
{
  [self._heartbeatListeners removeObject:listener];
}



#pragma mark - Public methods

- (UIImage*)imageFrameWithRobotId:(uint8_t)robotId
{
  using namespace Anki;
  
  UIImage *imageFrame = nil;
  Vision::Image img;
  
  if(true == _cozmoGame->GetCurrentRobotImage(robotId, img, _lastImageTimestamp))
  {
    // TODO: Somehow get rid of this copy, but still be threadsafe
    //cv::Mat_<u8> cvMatImg(nrows, ncols, const_cast<unsigned char*>(imageDataPtr));
    imageFrame = [self UIImageFromCVMat:img.get_CvMat_()];
    _lastImageTimestamp = img.GetTimestamp();
  }

  return imageFrame;
}

/*
- (BOOL)checkDeviceVisionMailbox:(CGRect*)markerBBox :(int*)markerType
{
  Anki::Cozmo::MessageVisionMarker msg;
  if(true == _cozmoGame->CheckDeviceVisionProcessingMailbox(msg)) {
    
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
 */

- (void) processDeviceImage:(Anki::Vision::Image&)image
{
  _cozmoGame->ProcessDeviceImage(image);
}

-(NSArray*)getVisionMarkersDetectedByDevice
{
  const std::vector<Anki::Cozmo::MessageG2U_DeviceDetectedVisionMarker>& markers = _cozmoGame->GetVisionMarkersDetectedByDevice();
  
  if(!markers.empty()) {
    
    NSMutableArray* mutableArray = [[NSMutableArray alloc] init];
    
    for(auto & marker : markers) {
      CozmoVisionMarkerBBox* visMarkerBBox = [[CozmoVisionMarkerBBox alloc] init];
      visMarkerBBox.markerType = marker.markerType;
      [visMarkerBBox setBoundingBoxFromCornersWithXupperLeft:marker.x_upperLeft YupperLeft:marker.y_upperLeft
                                                  XlowerLeft:marker.x_lowerLeft YlowerLeft:marker.y_lowerLeft
                                                 XupperRight:marker.x_upperRight YupperRight:marker.y_upperRight
                                                 XlowerRight:marker.x_lowerRight YlowerRight:marker.y_lowerRight];
      [mutableArray addObject:visMarkerBBox];
    }
    
    return mutableArray;
  } else {
    return nil;
  }
  
}

/*
- (BOOL) wasLastDeviceImageProcessed
{
  if(true == _cozmoGame->WasLastDeviceImageProcessed()) {
    return YES;
  } else {
    return NO;
  }
}
 */

@end
