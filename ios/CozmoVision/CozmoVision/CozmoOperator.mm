  //
//  CozmoOperator.m
//  CozmoVision
//
//  Created by Jordan Rivas on 12/11/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//
// This Clas abstracts all the messages to drive and operate Cozmo's

#import "CozmoOperator.h"
#import "SoundCoordinator.h"

#import <anki/cozmo/basestation/game/gameComms.h>
#import <anki/cozmo/basestation/game/gameMessageHandler.h>
#import <anki/cozmo/basestation/ui/messaging/uiMessages.h>
#import <anki/cozmo/shared/cozmoConfig.h>

using namespace Anki;

@interface CozmoOperator ()

@end

@implementation CozmoOperator
{
  Cozmo::GameMessageHandler* _gameMsgHandler;
  Cozmo::GameComms* _gameComms;
}

+ (instancetype)operatorWithAdvertisingtHostIPAddress:(NSString*)address
                                         withDeviceID:(int)deviceID
{
  CozmoOperator* instance = [CozmoOperator new];
  [instance registerToAvertisingServiceWithIPAddress:address
                                        withDeviceID:deviceID];

  return instance;
}

- (instancetype)init
{
  if (!(self = [super init]))
    return self;

  [self commonInit];

  return self;
}


#pragma mark - Game Message Callbacks

- (void) handleRobotAvailable:(const Cozmo::MessageG2U_RobotAvailable&) msg
{
  // For now, just tell the game to connect to all robots that advertise
  // TODO: Probably want to display available robots in the UI somewhere and only connect once selected
  Cozmo::MessageU2G_ConnectToRobot msgOut;
  msgOut.robotID = msg.robotID;
  [self sendMessage:msgOut];
}

- (void) handleUiDeviceAvailable:(const Cozmo::MessageG2U_UiDeviceAvailable&) msg
{
  // For now, just tell the game to connect to all UI devices that advertise
  // TODO: Probably want to display available devices in the UI somewhere and only connect once selected
  Cozmo::MessageU2G_ConnectToUiDevice msgOut;
  msgOut.deviceID = msg.deviceID;
  [self sendMessage:msgOut];
}

- (void) handlePlayRobotSound:(const Cozmo::MessageG2U_PlaySound&) msg
{
  std::string str(std::begin(msg.soundFilename), std::end(msg.soundFilename));
  NSString* filename = [NSString stringWithUTF8String:str.c_str()];
  [[SoundCoordinator defaultCoordinator] playSoundWithFilename:filename];
}

- (void) handleStopRobotSound:(const Cozmo::MessageG2U_StopSound&) msg
{
  [[SoundCoordinator defaultCoordinator] stop];
}

- (void) drawObservedVisionMarker:(const Cozmo::MessageG2U_ObjectVisionMarker&) msg
{
  // TODO: Fill in
}

- (void)commonInit
{
#ifdef __cplusplus
  //_uiClient = new TcpClient();
  
#endif

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
  
  auto robotAvailableCallbackLambda = [self](RobotID_t, const Cozmo::MessageG2U_RobotAvailable& msg) {
    [self handleRobotAvailable:msg];
  };
  
  _gameMsgHandler->RegisterCallbackForMessageG2U_RobotAvailable(robotAvailableCallbackLambda);
  
  auto playRobotSoundCallbackLambda = [self](RobotID_t, const Cozmo::MessageG2U_PlaySound& msg) {
    [self handlePlayRobotSound:msg];
  };
  
  _gameMsgHandler->RegisterCallbackForMessageG2U_PlaySound(playRobotSoundCallbackLambda);
  
  auto uiDeviceAvailableCallbackLambda = [self](RobotID_t, const Cozmo::MessageG2U_UiDeviceAvailable& msg) {
    [self handleUiDeviceAvailable:msg];
  };
  
  _gameMsgHandler->RegisterCallbackForMessageG2U_UiDeviceAvailable(uiDeviceAvailableCallbackLambda);
  
  auto stopRobotSoundCallbackLambda = [self](RobotID_t, const Cozmo::MessageG2U_StopSound& msg) {
    [self handleStopRobotSound:msg];
  };
  
  _gameMsgHandler->RegisterCallbackForMessageG2U_StopSound(stopRobotSoundCallbackLambda);
  
}

- (void)disconnectFromEngine
{
  if (_gameComms->HasClient()) {
    _gameComms->DisconnectClient();
    NSLog(@"Successfully disconnected from CozmoeEngine");
  }
}

- (void)sendMessage:(Cozmo::UiMessage&)message
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
    _gameMsgHandler->ProcessMessages();
  }
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

  Cozmo::MessageU2G_DriveWheels message;
  message.lwheel_speed_mmps = _leftWheelSpeed_mmps;
  message.rwheel_speed_mmps = _rightWheelSpeed_mmps;
  [self sendMessage:message];
}

- (void)sendWheelCommandWithLeftSpeed:(float)left right:(float)right
{
  Cozmo::MessageU2G_DriveWheels message;
  message.lwheel_speed_mmps = left;
  message.rwheel_speed_mmps = right;
  [self sendMessage:message];
}

- (void)sendHeadCommandWithAngleRatio:(float)angle accelerationSpeedRatio:(float)acceleration maxSpeedRatio:(float)maxSpeed
{
  const float headAcceleration = 2.0;
  const float headMaxSpeed = 5.0;

  Cozmo::MessageU2G_SetHeadAngle message;
  //message.angle_rad = ABS((f32)angle) * ((f32)angle < 0 ? Cozmo::MIN_HEAD_ANGLE  : Cozmo::MAX_HEAD_ANGLE);
  message.angle_rad = (f32)angle * (Cozmo::MAX_HEAD_ANGLE - Cozmo::MIN_HEAD_ANGLE) + Cozmo::MIN_HEAD_ANGLE;
  message.accel_rad_per_sec2 = (f32)headAcceleration;
  message.max_speed_rad_per_sec = (f32)headMaxSpeed;

  [self sendMessage:message];
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

  Cozmo::MessageU2G_SetLiftHeight message;
  message.height_mm = (f32)height * (Cozmo::LIFT_HEIGHT_CARRY - Cozmo::LIFT_HEIGHT_LOWDOCK) + Cozmo::LIFT_HEIGHT_LOWDOCK;
  message.accel_rad_per_sec2 = (f32)liftAcceleration;
  message.max_speed_rad_per_sec = (f32)liftMaxSpeed;

  [self sendMessage:message];
}

- (void)sendLiftCommandWithHeightRatio:(float)height
{
  // TODO: Fix acceleration & max speed
  [self sendLiftCommandWithHeightRatio:height accelerationSpeedRatio:0 maxSpeedRatio:0];
}

- (void)sendStopAllMotorsCommand
{
  Cozmo::MessageU2G_StopAllMotors message;
  [self sendMessage:message];
}

-(void)sendPickOrPlaceObject:(NSNumber*)objectID
{
  if(objectID) {
    Cozmo::MessageU2G_PickAndPlaceObject message;
    message.objectID = (int)objectID.integerValue;
    message.usePreDockPose = false;
    [self sendMessage:message];
  }
}

-(void)sendPlaceObjectOnGroundHere
{
  Cozmo::MessageU2G_PlaceObjectOnGroundHere message;
  [self sendMessage:message];
}

-(void)sendEnableFaceTracking:(BOOL)enable
{
  if(enable) {
    Cozmo::MessageU2G_StartFaceTracking message;
    [self sendMessage:message];
  } else {
    Cozmo::MessageU2G_StopFaceTracking message;
    [self sendMessage:message];
    
    // For now, we have to re-enable looking for markers because enabling
    // face tracking turned it off
    Cozmo::MessageU2G_StartLookingForMarkers tempMessage;
    [self sendMessage:tempMessage];
  }
}


- (void)sendAnimationWithName:(NSString*)animName
{
  Cozmo::MessageU2G_PlayAnimation message;
  
  // TODO: This is icky. Get actual ID from somewhere and use that
  strncpy(&(message.animationName[0]), [animName UTF8String], 32);
  message.numLoops = 1;
  
  [self sendMessage:message];

}


- (void)sendCameraResolution:(BOOL)isHigh
{
  Cozmo::MessageU2G_ImageRequest message;
  // HACK
  message.mode = 1;
  message.resolution = isHigh ? Vision::CAMERA_RES_QVGA : Vision::CAMERA_RES_QQQVGA;

  [self sendMessage:message];
}

@end
















