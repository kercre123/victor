  //
//  CozmoOperator.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/11/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//
// This Clas abstracts all the messages to drive and operate Cozmo's

#import "AppDelegate.h"

#import "CozmoOperator.h"
#import "SoundCoordinator.h"

#import <anki/cozmo/game/comms/gameComms.h>
#import <anki/cozmo/game/comms/gameMessageHandler.h>
#import <anki/cozmo/game/comms/messaging/uiMessages.h>
#import <anki/cozmo/shared/cozmoConfig.h>

using namespace Anki;

@interface CozmoOperator ()

@property (strong, nonatomic) NSMutableArray* robotConnectedListeners;
@property (strong, nonatomic) NSMutableArray* uiDeviceConnectedListeners;

@property (strong, nonatomic) NSString* forceAddRobotIP;

@end

@implementation CozmoOperator
{
  enum UIState {
    UI_WAITING_FOR_GAME,
    UI_RUNNING
  };
  
  UIState _uiState;
  
  Cozmo::U2G_Ping _pingMsg;
  
  Cozmo::GameMessageHandler* _gameMsgHandler;
  Cozmo::GameComms* _gameComms;
}

/*
+ (instancetype)operatorWithAdvertisingtHostIPAddress:(NSString*)address
                                         withDeviceID:(int)deviceID
{
  CozmoOperator* instance = [CozmoOperator new];
  [instance registerToAvertisingServiceWithIPAddress:address
                                        withDeviceID:deviceID];

  return instance;
}
 */

- (instancetype)init
{
  if (!(self = [super init]))
    return self;

  [self commonInit];

  return self;
}

- (void) cozmoEngineWrapperHeartbeat:(CozmoEngineWrapper *)cozmoEngine
{
  [self update];
}

+ (instancetype)defaultOperator
{
  return ((AppDelegate*)[[UIApplication sharedApplication] delegate]).cozmoOperator;
}

#pragma mark - Game Message Listeners

- (void)addRobotConnectedListener:(id<CozmoRobotConnectedListener>)listener
{
  [self.robotConnectedListeners addObject:listener];
}

- (void)removeRobotConnectedListener:(id<CozmoRobotConnectedListener>)listener
{
  [self.robotConnectedListeners removeObject:listener];
}

- (void)addUiDeviceConnectedListener:(id<CozmoUiDeviceConnectedListener>)listener
{
  [self.uiDeviceConnectedListeners addObject:listener];
}

- (void)removeUiDeviceConnectedListener:(id<CozmoUiDeviceConnectedListener>)listener
{
  [self.uiDeviceConnectedListeners removeObject:listener];
}

#pragma mark - Game Message Callbacks

- (void) handleRobotAvailable:(const Cozmo::G2U_RobotAvailable&) msg
{
  // For now, just tell the game to connect to all robots that advertise
  // TODO: Probably want to display available robots in the UI somewhere and only connect once selected
  Cozmo::U2G_ConnectToRobot msgConnect;
  msgConnect.robotID = msg.robotID;
  
  Cozmo::U2G_Message msgSend;
  msgSend.Set_ConnectToRobot(msgConnect);
  
  [self sendMessage:msgSend];
}

- (void) handleUiDeviceAvailable:(const Cozmo::G2U_UiDeviceAvailable&) msg
{
  // For now, just tell the game to connect to all UI devices that advertise
  // TODO: Probably want to display available devices in the UI somewhere and only connect once selected
  Cozmo::U2G_ConnectToUiDevice msgConnect;
  msgConnect.deviceID = msg.deviceID;
  
  Cozmo::U2G_Message msgSend;
  msgSend.Set_ConnectToUiDevice(msgConnect);

  [self sendMessage:msgSend];
}

- (void) handlePlaySound:(const Cozmo::G2U_PlaySound&) msg
{
  std::string str(std::begin(msg.soundFilename), std::end(msg.soundFilename));
  NSString* filename = [NSString stringWithUTF8String:str.c_str()];
  [[SoundCoordinator defaultCoordinator] playSoundWithFilename:filename];
}

- (void) handleStopSound:(const Cozmo::G2U_StopSound&) msg
{
  [[SoundCoordinator defaultCoordinator] stop];
}

- (void)commonInit
{
  self.robotConnectedListeners = [[NSMutableArray alloc] init];
  self.uiDeviceConnectedListeners = [[NSMutableArray alloc] init];
  
  _pingMsg.counter = 0;
  
  _uiState = UI_WAITING_FOR_GAME;
}

- (void)dealloc
{
  if (_gameComms) {
    delete _gameComms;
    _gameComms = nil;
  }
  if (_gameMsgHandler) {
    delete _gameMsgHandler;
    _gameMsgHandler = nil;
  }
  
  [self.robotConnectedListeners removeAllObjects];
}

- (BOOL)isConnected
{
  if (_gameComms) {
    return _gameComms->HasClient();
  }
  return NO;
}


#pragma mark - Connection & Message Methds

- (void)registerToAvertisingServiceWithIPAddress:(NSString*)ipAddress
                                    withDeviceID:(int)deviceID
{
  /*
  if (!_uiClient->IsConnected()) {

    if (_uiClient->Connect([ipAddress UTF8String], Cozmo::UI_MESSAGE_SERVER_LISTEN_PORT)) {
      // Successfully connected
      self.isConnected = YES;
      NSLog(@"Cozmo iOS UI connected to basestation!\n");
    }
    else {
      self.isConnected = NO;
      NSLog(@"Failed to connect to UI message server listen port\n");
    }
  }
   */
  
  if (!_gameComms) {
    _gameComms = new Cozmo::GameComms(deviceID, Cozmo::UI_MESSAGE_SERVER_LISTEN_PORT,
                                      [ipAddress UTF8String], Cozmo::UI_ADVERTISEMENT_REGISTRATION_PORT);
    _gameMsgHandler = new Cozmo::GameMessageHandler();
  }
  
}

- (void)initMessageHandler
{
  _gameMsgHandler->Init(_gameComms);
  
#define REGISTER_CALLBACK(__MSG_TYPE__) \
case Cozmo::G2U_Message::Type::__MSG_TYPE__: \
[self handle##__MSG_TYPE__:msg.Get_##__MSG_TYPE__()]; \
break;

  _gameMsgHandler->RegisterCallbackForMessage([self](const Cozmo::G2U_Message& msg) {
      switch (msg.GetType()) {
        case Cozmo::G2U_Message::Type::INVALID:
          break;
          
          // Simple wrappers:
          
          REGISTER_CALLBACK(RobotAvailable)
          REGISTER_CALLBACK(PlaySound)
          REGISTER_CALLBACK(UiDeviceAvailable)
          REGISTER_CALLBACK(StopSound)
          
          // Special cases:
        case Cozmo::G2U_Message::Type::RobotConnected:
        {
          NSNumber* robotID = [NSNumber numberWithInt:msg.Get_RobotConnected().robotID];
          [self.robotConnectedListeners makeObjectsPerformSelector:@selector(robotConnectedWithID:) withObject:robotID];
          break;
        }
          
        case Cozmo::G2U_Message::Type::UiDeviceConnected:
        {
          NSNumber* deviceID = [NSNumber numberWithInt:msg.Get_UiDeviceConnected().deviceID];
          [self.uiDeviceConnectedListeners makeObjectsPerformSelector:@selector(uiDeviceConnectedWithID:) withObject:deviceID];
          break;
        }
          
        case Cozmo::G2U_Message::Type::RobotObservedObject:
          if(self.handleRobotObservedObject) {
            CozmoObsObjectBBox* observation = [[CozmoObsObjectBBox alloc] init];
            
            const Cozmo::G2U_RobotObservedObject& obsObjMsg= msg.Get_RobotObservedObject();
            observation.objectID = obsObjMsg.objectID;
            observation.boundingBox = CGRectMake(obsObjMsg.img_topLeft_x,
                                                 obsObjMsg.img_topLeft_y,
                                                 obsObjMsg.img_width,
                                                 obsObjMsg.img_height);
            
            self.handleRobotObservedObject(observation);
          }
          break;

        case Cozmo::G2U_Message::Type::DeviceDetectedVisionMarker:
          if(self.handleDeviceDetectedVisionMarker) {
            CozmoVisionMarkerBBox* marker = [[CozmoVisionMarkerBBox alloc] init];
            
            const Cozmo::G2U_DeviceDetectedVisionMarker& markerMsg = msg.Get_DeviceDetectedVisionMarker();
            marker.markerType = markerMsg.markerType;
            [marker setBoundingBoxFromCornersWithXupperLeft:markerMsg.x_upperLeft  YupperLeft: markerMsg.y_upperLeft
                                                 XlowerLeft:markerMsg.x_lowerLeft  YlowerLeft: markerMsg.y_lowerLeft
                                                XupperRight:markerMsg.x_upperRight YupperRight:markerMsg.y_upperRight
                                                XlowerRight:markerMsg.x_lowerRight YlowerRight:markerMsg.y_lowerRight];
            
            self.handleDeviceDetectedVisionMarker(marker);
          }
          break;
      }
    });
#undef REGISTER_CALLBACK

}

- (void)disconnectFromEngine
{
  if (_gameComms->HasClient()) {
    _gameComms->DisconnectClient();
    NSLog(@"Successfully disconnected from CozmoeEngine");
  }
}

- (void)sendMessage:(Cozmo::U2G_Message&)message
{
  if (self.isConnected) {
    
    UserDeviceID_t devID = 1; // TODO: Where does this come from?
    _gameMsgHandler->SendMessage(devID, message);
    
  } // if(_isConnected)
}



- (void)update
{
  _gameComms->Update();
  
    // Wait for _gameComms to initialize before trying to process game messages
  if(!_gameMsgHandler->IsInitialized()) {
    if(_gameComms->IsInitialized()) {
      [self initMessageHandler];
    }
  } else {
    
    switch(_uiState) {
      case UI_WAITING_FOR_GAME:
      {
        if (!_gameComms->HasClient()) {
          return;
        } else {
          // Once gameComms has a client, tell the engine to start, force-add
          // robot if necessary, and switch states in the UI
          
          // TODO: Do this from UI. Currently CozmoEngineWrapper calls StartEngine directly
          /*
           NSLog(@"Sending StartEngine message.\n");
           
           U2G_StartEngine msg;
           msg.asHost = 1; // TODO: Support running as client?
           std::string vizIpStr = "127.0.0.1";
           std::fill(msg.vizHostIP.begin(), msg.vizHostIP.end(), '\0'); // ensure null termination
           std::copy(vizIpStr.begin(), vizIpStr.end(), msg.vizHostIP.begin());
           msgHandler_.SendMessage(1, msg); // TODO: don't hardcode ID here
           */
          
          if(false && _forceAddRobotIP != nil) {
            NSLog(@"Sending message to force-add robot at IP %@.\n", _forceAddRobotIP);
            Cozmo::U2G_ForceAddRobot msg;
            std::string tempStr = [_forceAddRobotIP UTF8String];
            
            std::fill(msg.ipAddress.begin(), msg.ipAddress.end(), '\0');
            std::copy(tempStr.begin(), tempStr.end(), msg.ipAddress.begin());
            
            msg.isSimulated = false;
            msg.robotID = 1;
            
            Cozmo::U2G_Message msgWrapper;
            msgWrapper.Set_ForceAddRobot(msg);
            [self sendMessage:msgWrapper];
          }
          
          _uiState = UI_RUNNING;
        }
        break;
      }
        
      case UI_RUNNING:
      {
        // Send ping to engine
        //[self sendMessage:_pingMsg];
        //++_pingMsg.counter;
        
        _gameMsgHandler->ProcessMessages();
        
        break;
      }
    } // switch(uiState)
  } // if/else
  
}

#pragma MARK - Communications

- (void)forceAddRobot:(NSString *)ipAddress
{
  _forceAddRobotIP = ipAddress;
}

#pragma mark - Drive Cozmo

- (void)sendWheelCommandWithAngleInDegrees:(float)angle magnitude:(float)magnitude
{
  const f32 ANALOG_INPUT_DEAD_ZONE_THRESH = .1f; //000.f / f32(s16_MAX);
  const f32 ANALOG_INPUT_MAX_DRIVE_SPEED  = 100; // mm/s
  const f32 MAX_ANALOG_RADIUS             = 300;

//#if(DEBUG_GAMEPAD)
//  printf("AnalogLeft X %.2f  Y %.2f\n", point.x, point.y);
//#endif

  f32 _leftWheelSpeed_mmps = 0;
  f32 _rightWheelSpeed_mmps = 0;

  // Stop wheels if magnitude of input is low
  if (magnitude > ANALOG_INPUT_DEAD_ZONE_THRESH) {

    magnitude -= ANALOG_INPUT_DEAD_ZONE_THRESH;
    magnitude /= 1.f - ANALOG_INPUT_DEAD_ZONE_THRESH;

    f32 angleTranslation = angle;

    // Driving forward?
    f32 fwd = (angle > 0) ? 1 : -1;

    // Curving right?
    f32 right = (angle > -90.0 && angle < 90.0) ? 1 : -1;

    if (angle > 90.0) {
      angleTranslation -= 180.0;
    }
    else if (angle < -90.0) {
      angleTranslation += 180.0;
    }
    angleTranslation = ABS(angleTranslation);


    // Base wheel speed based on magnitude of input and whether or not robot is driving forward
    f32 baseWheelSpeed = ANALOG_INPUT_MAX_DRIVE_SPEED * magnitude * fwd;

    // Convert to Rads
    f32 xyAngle = DEG_TO_RAD_F32(angleTranslation) * right;

    // Compute radius of curvature
    f32 roc = (xyAngle / PIDIV2_F) * MAX_ANALOG_RADIUS;

    // Compute individual wheel speeds
    if (fabsf(xyAngle) > PIDIV2_F - 0.1f) {
      // Straight fwd/back
      _leftWheelSpeed_mmps  = baseWheelSpeed;
      _rightWheelSpeed_mmps = baseWheelSpeed;
    } else if (fabsf(xyAngle) < 0.1f) {
      // Turn in place
      _leftWheelSpeed_mmps  = right * magnitude * ANALOG_INPUT_MAX_DRIVE_SPEED;
      _rightWheelSpeed_mmps = -right * magnitude * ANALOG_INPUT_MAX_DRIVE_SPEED;
    } else {

      _leftWheelSpeed_mmps = baseWheelSpeed * (roc + (right * Anki::Cozmo::WHEEL_DIST_HALF_MM)) / roc;
      _rightWheelSpeed_mmps = baseWheelSpeed * (roc - (right * Anki::Cozmo::WHEEL_DIST_HALF_MM)) / roc;

      // Swap wheel speeds
      if (fwd * right < 0) {
        f32 temp = _leftWheelSpeed_mmps;
        _leftWheelSpeed_mmps = _rightWheelSpeed_mmps;
        _rightWheelSpeed_mmps = temp;
      }
    }

    // Cap wheel speed at max
    if (fabsf(_leftWheelSpeed_mmps) > ANALOG_INPUT_MAX_DRIVE_SPEED) {
      const f32 correction = _leftWheelSpeed_mmps - (ANALOG_INPUT_MAX_DRIVE_SPEED * (_leftWheelSpeed_mmps >= 0 ? 1 : -1));
      const f32 correctionFactor = 1.f - fabsf(correction / _leftWheelSpeed_mmps);
      _leftWheelSpeed_mmps *= correctionFactor;
      _rightWheelSpeed_mmps *= correctionFactor;
      //printf("lcorrectionFactor: %f\n", correctionFactor);
    }
    if (fabsf(_rightWheelSpeed_mmps) > ANALOG_INPUT_MAX_DRIVE_SPEED) {
      const f32 correction = _rightWheelSpeed_mmps - (ANALOG_INPUT_MAX_DRIVE_SPEED * (_rightWheelSpeed_mmps >= 0 ? 1 : -1));
      const f32 correctionFactor = 1.f - fabsf(correction / _rightWheelSpeed_mmps);
      _leftWheelSpeed_mmps *= correctionFactor;
      _rightWheelSpeed_mmps *= correctionFactor;
      //printf("rcorrectionFactor: %f\n", correctionFactor);
    }

#if(DEBUG_GAMEPAD)
    printf("AnalogLeft: xyMag %f, xyAngle %f, radius %f, fwd %f, right %f, lwheel %f, rwheel %f\n", magnitude, xyAngle, roc, fwd, right, _leftWheelSpeed_mmps, _rightWheelSpeed_mmps );
#endif
  }

  Cozmo::U2G_DriveWheels message;
  message.lwheel_speed_mmps = _leftWheelSpeed_mmps;
  message.rwheel_speed_mmps = _rightWheelSpeed_mmps;
  
  Cozmo::U2G_Message messageWrapper;
  messageWrapper.Set_DriveWheels(message);
  [self sendMessage:messageWrapper];
}

- (void)sendWheelCommandWithLeftSpeed:(float)left right:(float)right
{
  Cozmo::U2G_DriveWheels message;
  message.lwheel_speed_mmps = left;
  message.rwheel_speed_mmps = right;

  Cozmo::U2G_Message messageWrapper;
  messageWrapper.Set_DriveWheels(message);
  [self sendMessage:messageWrapper];
}

- (void)sendHeadCommandWithAngleRatio:(float)angle accelerationSpeedRatio:(float)acceleration maxSpeedRatio:(float)maxSpeed
{
  const float headAcceleration = 2.0;
  const float headMaxSpeed = 5.0;

  Cozmo::U2G_SetHeadAngle message;
  //message.angle_rad = ABS((f32)angle) * ((f32)angle < 0 ? Cozmo::MIN_HEAD_ANGLE  : Cozmo::MAX_HEAD_ANGLE);
  message.angle_rad = (f32)angle * (Cozmo::MAX_HEAD_ANGLE - Cozmo::MIN_HEAD_ANGLE) + Cozmo::MIN_HEAD_ANGLE;
  message.accel_rad_per_sec2 = (f32)headAcceleration;
  message.max_speed_rad_per_sec = (f32)headMaxSpeed;

  Cozmo::U2G_Message messageWrapper;
  messageWrapper.Set_SetHeadAngle(message);
  [self sendMessage:messageWrapper];
}

- (void)sendHeadCommandWithAngleRatio:(float)angle
{
  // TODO: Fix acceleration & max speed
  [self sendHeadCommandWithAngleRatio:angle accelerationSpeedRatio:0 maxSpeedRatio:0];
}

- (void)sendLiftCommandWithHeightRatio:(float)height accelerationSpeedRatio:(float)acceleration maxSpeedRatio:(float)maxSpeed
{
  const float liftAcceleration = 2.0;
  const float liftMaxSpeed = 5.0;

  Cozmo::U2G_SetLiftHeight message;
  message.height_mm = (f32)height * (Cozmo::LIFT_HEIGHT_CARRY - Cozmo::LIFT_HEIGHT_LOWDOCK) + Cozmo::LIFT_HEIGHT_LOWDOCK;
  message.accel_rad_per_sec2 = (f32)liftAcceleration;
  message.max_speed_rad_per_sec = (f32)liftMaxSpeed;

  Cozmo::U2G_Message messageWrapper;
  messageWrapper.Set_SetLiftHeight(message);
  [self sendMessage:messageWrapper];
}

- (void)sendLiftCommandWithHeightRatio:(float)height
{
  // TODO: Fix acceleration & max speed
  [self sendLiftCommandWithHeightRatio:height accelerationSpeedRatio:2 maxSpeedRatio:5];
}

- (void)sendMoveLiftCommandWithSpeed:(float)speed
{
  NSLog(@"Commanding lift with speed = %f\n", speed);
  Cozmo::U2G_MoveLift message;
  message.speed_rad_per_sec = speed;
  
  Cozmo::U2G_Message messageWrapper;
  messageWrapper.Set_MoveLift(message);
  [self sendMessage:messageWrapper];
  
}

- (void)sendMoveHeadCommandWithSpeed:(float)speed
{
  NSLog(@"Commanding head with speed = %f\n", speed);
  Cozmo::U2G_MoveHead message;
  message.speed_rad_per_sec = speed;
  Cozmo::U2G_Message messageWrapper;
  messageWrapper.Set_MoveHead(message);
  [self sendMessage:messageWrapper];
}

- (void)sendStopAllMotorsCommand
{
  Cozmo::U2G_StopAllMotors message;
  Cozmo::U2G_Message messageWrapper;
  messageWrapper.Set_StopAllMotors(message);
  [self sendMessage:messageWrapper];
}

-(void)sendPickOrPlaceObject:(NSNumber*)objectID
{
  if(objectID) {
    Cozmo::U2G_PickAndPlaceObject message;
    message.objectID = (int)objectID.integerValue;
    message.usePreDockPose = false;
    message.useManualSpeed = false;
    
    Cozmo::U2G_Message messageWrapper;
    messageWrapper.Set_PickAndPlaceObject(message);
    [self sendMessage:messageWrapper];
  }
}

-(void)sendPlaceObjectOnGroundHere
{
  Cozmo::U2G_PlaceObjectOnGroundHere message;
  Cozmo::U2G_Message messageWrapper;
  messageWrapper.Set_PlaceObjectOnGroundHere(message);
  [self sendMessage:messageWrapper];
}

-(void)sendEnableFaceTracking:(BOOL)enable
{
  if(enable) {
    Cozmo::U2G_StartFaceTracking message;
    Cozmo::U2G_Message messageWrapper;
    messageWrapper.Set_StartFaceTracking(message);
    [self sendMessage:messageWrapper];
  } else {
    Cozmo::U2G_StopFaceTracking message;
    Cozmo::U2G_Message messageWrapper;
    messageWrapper.Set_StopFaceTracking(message);
    [self sendMessage:messageWrapper];
    
    {
      // For now, we have to re-enable looking for markers because enabling
      // face tracking turned it off
      Cozmo::U2G_StartLookingForMarkers message;
      Cozmo::U2G_Message messageWrapper;
      messageWrapper.Set_StartLookingForMarkers(message);
      [self sendMessage:messageWrapper];
    }
  }
}


- (void)sendAnimationWithName:(NSString*)animName
{
  Cozmo::U2G_PlayAnimation message;
  
  // TODO: This is icky. Get actual ID from somewhere and use that
  strncpy(&(message.animationName[0]), [animName UTF8String], 32);
  message.numLoops = 1;
  
  Cozmo::U2G_Message messageWrapper;
  messageWrapper.Set_PlayAnimation(message);
  [self sendMessage:messageWrapper];

}


- (void)sendCameraResolution:(BOOL)isHigh
{
  Cozmo::U2G_SetRobotImageSendMode message;
  // HACK
  message.mode = 1;
  message.resolution = isHigh ? Vision::CAMERA_RES_QVGA : Vision::CAMERA_RES_QQQVGA;

  Cozmo::U2G_Message messageWrapper;
  messageWrapper.Set_SetRobotImageSendMode(message);
  [self sendMessage:messageWrapper];
}

- (void)sendEnableRobotImageStreaming:(BOOL)enable
{
  Cozmo::U2G_SetRobotImageSendMode message;
  // HACK
  message.mode = (enable ? 1 : 0); // 1 is ISM_STREAM, 0 is ISM_OFF
  message.resolution = Vision::CAMERA_RES_QVGA;
  
  Cozmo::U2G_Message messageWrapper;
  messageWrapper.Set_SetRobotImageSendMode(message);
  [self sendMessage:messageWrapper];
}

@end
















