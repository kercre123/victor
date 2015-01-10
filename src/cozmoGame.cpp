/*
 * File:          cozmoEngine.cpp
 * Date:          12/23/2014
 *
 * Description:   (See header file.)
 *
 * Author: Andrew Stein / Kevin Yoon
 *
 * Modifications:
 */

#include "anki/cozmo/game/cozmoGame.h"

#include "anki/cozmo/basestation/cozmoEngine.h"

#include "anki/common/basestation/utils/logging/logging.h"

namespace Anki {
namespace Cozmo {

  CozmoGameHost::CozmoGameHost() :
  cozmoEngine_(nullptr)
  {
    // Register for all events of interest here:
    BSE_RobotConnect::Register( this );
  }

  CozmoGameHost::~CozmoGameHost()
  {
    // Unregister for all events
    BSE_RobotConnect::Unregister( this );
    
    if(cozmoEngine_) {
      delete cozmoEngine_;
    }
  }
  
  void CozmoGameHost::Init(const Json::Value& config) {
    
    if (cozmoEngine_)
      return;
    
    // There is no "device" (e.g. phone) with a camera so we can just use a bogus camera calibration
    Vision::CameraCalibration bogusDeviceCamCalib;
    
    cozmoEngine_ = new CozmoEngineHost();
    cozmoEngine_->Init(config, bogusDeviceCamCalib);
  }
  
  void CozmoGameHost::Update(const float currentTime_sec) {
    // Connect to any robots we see:
    std::vector<CozmoEngine::AdvertisingRobot> advertisingRobots;
    cozmoEngine_->GetAdvertisingRobots(advertisingRobots);
    for(auto robot : advertisingRobots) {
      cozmoEngine_->ConnectToRobot(robot);
    }
    
    // Connect to any UI devices we see:
    std::vector<CozmoEngine::AdvertisingUiDevice> advertisingUiDevices;
    cozmoEngine_->GetAdvertisingUiDevices(advertisingUiDevices);
    for(auto device : advertisingUiDevices) {
      cozmoEngine_->ConnectToUiDevice(device);
    }
    
    Result status = cozmoEngine_->Update(SEC_TO_NANOS(currentTime_sec));
    if (status != RESULT_OK) {
      PRINT_NAMED_WARNING("CozmoEngine.Update.NotOK","Status %d\n", status);
    }
    
    /*
     std::vector<BasestationMain::ObservedObjectBoundingBox> boundingQuads;
     if(true == bs.GetCurrentVisionMarkers(1, boundingQuads) ) {
     // TODO: stuff?
     }
     */
  }
  
  // Process raised events from CozmoEngine
  void CozmoGameHost::OnEventRaised( const IBaseStationEventInterface* event )
  {
    switch( event->GetEventType() ) {
        
      case BSETYPE_RobotConnect:
      {
        BSE_RobotConnect *rcEvent = (BSE_RobotConnect*)event;
        printf("CozmoGame: Robot %d connect: %s !!!!!!\n", rcEvent->robotID_, rcEvent->successful_ ? "SUCCESS" : "FAILED");
        break;
      }
      default:
        printf("CozmoGame: Received unknown event %d\n", event->GetEventType());
        break;
    }
  }
  
} // namespace Cozmo
} // namespace Anki


