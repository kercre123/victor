//
//  CozmoBasestation.m
//  CozmoVision
//
//  Created by Andrew Stein on 12/5/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>

#import "CozmoBasestation.h"

// Cozmo includes
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/basestation/uiTcpComms.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

// Coretech-Common includes
#include "anki/common/basestation/utils/logging/DAS/DAS.h"
#include "anki/common/basestation/jsonTools.h"

// Coretech-Vision includes
#include "anki/vision/basestation/image_impl.h"

// External includes
#include "json/json.h"
#include <opencv2/opencv.hpp>

#include <fstream>

// IP Address of wherever the robot's advertising service is running
//#define DEFAULT_ADVERTISING_HOST_IP "192.168.42.68"
#define DEFAULT_ADVERTISING_HOST_IP "192.168.19.238"

#define USE_BLE_ROBOT_COMMS 0

@interface CozmoBasestation ()

@end

@implementation CozmoBasestation {

  Json::Value    _config;
  CFAbsoluteTime _firstHeartbeatTimestamp;
  CFAbsoluteTime _lastHeartbeatTimestamp;
  BOOL           _areRobotsConnected;
  
  
}

@synthesize _imageViewer;

-(id)init
{
  _areRobotsConnected = NO;
  _imageViewer = nullptr;
  
# ifdef __cplusplus
  ///// Basestation and Comms Init //////
  using namespace Anki;
  
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
    _config["AdvertisingHostIP"] = DEFAULT_ADVERTISING_HOST_IP;
  }
  
  // Connect to robot.
  // UI-layer should handle this (whether the connection is TCP or BTLE).
  // Start basestation only when connections have been established.
  _uiComms = new Cozmo::UiTCPComms();
  
#if (USE_BLE_ROBOT_COMMS)
  BLEComms robotComms;
  BLERobotManager robotBLEManager;
#else
  _robotComms = new Cozmo::RobotComms(_config["AdvertisingHostIP"].asCString());
#endif
  
  
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

  // Once done initializing, this will start the basestation timer loop
  _firstHeartbeatTimestamp = _lastHeartbeatTimestamp = CFAbsoluteTimeGetCurrent();
  _heartbeatTimer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / FRAME_RATE)
                                                     target:self
                                                   selector:@selector(gameHeartbeat:)
                                                   userInfo:nil
                                                    repeats:YES];

  
# endif // #ifdef __cplusplus
  
  return self;
  
} // init


-(void)dealloc
{
#ifdef __cplusplus
  if(_uiComms) {
    delete _uiComms;
    _uiComms = nullptr;
  }
  
  if(_robotComms) {
    delete _robotComms;
    _robotComms = nullptr;
  }
  
  if(_basestation) {
    delete _basestation;
    _basestation = nullptr;
  }
  
#endif
}


-(UIImage *)UIImageFromCVMat:(cv::Mat)cvMat
{
  NSData *data = [NSData dataWithBytes:cvMat.data length:cvMat.elemSize()*cvMat.total()];
  CGColorSpaceRef colorSpace;
  
  if (cvMat.elemSize() == 1) {
    colorSpace = CGColorSpaceCreateDeviceGray();
  } else {
    colorSpace = CGColorSpaceCreateDeviceRGB();
  }
  
  CGDataProviderRef provider = CGDataProviderCreateWithCFData((__bridge CFDataRef)data);
  
  // Creating CGImage from cv::Mat
  CGImageRef imageRef = CGImageCreate(cvMat.cols,                                 //width
                                      cvMat.rows,                                 //height
                                      8,                                          //bits per component
                                      8 * cvMat.elemSize(),                       //bits per pixel
                                      cvMat.step[0],                            //bytesPerRow
                                      colorSpace,                                 //colorspace
                                      kCGImageAlphaNone|kCGBitmapByteOrderDefault,// bitmap info
                                      provider,                                   //CGDataProviderRef
                                      NULL,                                       //decode
                                      false,                                      //should interpolate
                                      kCGRenderingIntentDefault                   //intent
                                      );
  
  
  // Getting UIImage from CGImage
  UIImage *finalImage = [UIImage imageWithCGImage:imageRef];
  CGImageRelease(imageRef);
  CGDataProviderRelease(provider);
  CGColorSpaceRelease(colorSpace);
  
  return finalImage;
}

#pragma mark - Main Game Update Loop
// Main Game Update Loop
- (void)gameHeartbeat:(NSTimer*)timer
{
  CFAbsoluteTime thisHeartbeatTimestamp = CFAbsoluteTimeGetCurrent();
  CFTimeInterval thisHeartbeatDelta = thisHeartbeatTimestamp - _lastHeartbeatTimestamp;
  //DASDebug("RHLocalGame.heartBeatTime", "%lf", thisHeartbeatDelta);
  if(thisHeartbeatDelta > (PERIOD + 0.003)) {
    DASWarn("RHLocalGame.heartBeatMissedByMS_f", "%lf", thisHeartbeatDelta * 1000.0);
  }
  _lastHeartbeatTimestamp = thisHeartbeatTimestamp;
  
  if(_areRobotsConnected) {
    DASDebug("Basestation.gameHeartBeat()", "Basestation heartbeat at time %f", thisHeartbeatTimestamp);

    // Read all UI messages
    _uiComms->Update();
    
    // Read messages from all robots
#if (USE_BLE_ROBOT_COMMS)
    robotBLEManager.Update();  // Necessary?
#else
    _robotComms->Update();
#endif
    
    // Update the basestation itself (should the basestation tell the comms to update?)
    _basestation->Update(thisHeartbeatTimestamp);
    
    
    if(_imageViewer) {
      Anki::Vision::Image img;
      if(true == _basestation->GetCurrentRobotImage(1, img))
      {
        // TODO: Somehow get rid of this copy, but still be threadsafe
        //cv::Mat_<u8> cvMatImg(nrows, ncols, const_cast<unsigned char*>(imageDataPtr));
        _imageViewer.image = [self UIImageFromCVMat:img.get_CvMat_()];
      }
    }
    
  }
  else {
    DASDebug("Basestation.gameHeartBeat()", "Waiting for robots to connect at time %f", thisHeartbeatTimestamp);
    // ====== TCP ======
    _robotComms->Update();
    
    // If not already connected to a robot, connect to the
    // first one that becomes available.
    // TODO: Once we have a UI, we can select the one we want to connect to in a more reasonable way.
    if (_robotComms->GetNumConnectedRobots() == 0) {
      std::vector<int> advertisingRobotIDs;
      if (_robotComms->GetAdvertisingRobotIDs(advertisingRobotIDs) > 0) {
        for(auto robotID : advertisingRobotIDs) {
          printf("RobotComms connecting to robot %d.\n", robotID);
          if (_robotComms->ConnectToRobotByID(robotID)) {
            printf("Connected to robot %d\n", robotID);
            
            // Add connected_robot ID to config
            _config[AnkiUtil::kP_CONNECTED_ROBOTS].append(robotID);
            
            break;
          } else {
            printf("Failed to connect to robot %d\n", robotID);
          }
        }
      }
    }
    
    if (_robotComms->GetNumConnectedRobots() == 0) {
      _areRobotsConnected = NO;
    } else {
      using namespace Anki;
      
      _areRobotsConnected = YES;
      
      // We have connected robots. Init the basestation!
      _basestation = new Cozmo::BasestationMain();
      Cozmo::BasestationStatus status = _basestation->Init(_robotComms, _uiComms, _config, Cozmo::BM_DEFAULT);
      if (status != Cozmo::BS_OK) {
        NSLog(@"Basestation.Init.Failed with status %d\n", status);
      }
      
    }
  } // if/else(_areRobotsConnected)
  
} // gameHeartBeat()



@end