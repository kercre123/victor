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
#include "anki/cozmo/basestation/events/BaseStationEvent.h"

#include "anki/common/basestation/utils/logging/logging.h"

#include "anki/cozmo/basestation/uiMessageHandler.h"

namespace Anki {
namespace Cozmo {

#pragma mark - CozmoGameHost Implementation
  
  class CozmoGameHostImpl : public IBaseStationEventListener
  {
  public:
    enum PlaybackMode {
      LIVE_SESSION_NO_RECORD = 0,
      RECORD_SESSION,
      PLAYBACK_SESSION
    };
    
    CozmoGameHostImpl();
    ~CozmoGameHostImpl();
    
    void Init(const Json::Value& config);
    
    // Tick with game heartbeat:
    void Update(const float currentTime_sec);
    
  private:
    
    // Process raised events from CozmoEngine.
    // Req'd by IBaseStationEventListener
    virtual void OnEventRaised( const IBaseStationEventInterface* event ) override;
    
    CozmoEngineHost *_cozmoEngine;
    UiMessageHandler _uiMsgHandler;
    
  }; // class CozmoGameHostImpl
  
  
  CozmoGameHostImpl::CozmoGameHostImpl()
  : _cozmoEngine(nullptr)
  {
    // Register for all events of interest here:
    BSE_RobotConnect::Register( this );
    BSE_DeviceDetectedVisionMarker::Register( this );
  }

  CozmoGameHostImpl::~CozmoGameHostImpl()
  {
    // Unregister for all events
    BSE_RobotConnect::Unregister( this );
    BSE_DeviceDetectedVisionMarker::Unregister( this );
    
    if(_cozmoEngine) {
      delete _cozmoEngine;
    }
  }
  
  void CozmoGameHostImpl::Init(const Json::Value& config) {
    
    if (_cozmoEngine)
      return;
    
    _cozmoEngine = new CozmoEngineHost();
    _cozmoEngine->Init(config);
  }
  
  void CozmoGameHostImpl::Update(const float currentTime_sec) {
    // Connect to any robots we see:
    std::vector<CozmoEngine::AdvertisingRobot> advertisingRobots;
    _cozmoEngine->GetAdvertisingRobots(advertisingRobots);
    for(auto robot : advertisingRobots) {
      _cozmoEngine->ConnectToRobot(robot);
    }
    
    // Connect to any UI devices we see:
    std::vector<CozmoEngine::AdvertisingUiDevice> advertisingUiDevices;
    _cozmoEngine->GetAdvertisingUiDevices(advertisingUiDevices);
    for(auto device : advertisingUiDevices) {
      _cozmoEngine->ConnectToUiDevice(device);
    }
    
    Result status = _cozmoEngine->Update(SEC_TO_NANOS(currentTime_sec));
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
  void CozmoGameHostImpl::OnEventRaised( const IBaseStationEventInterface* event )
  {
    switch( event->GetEventType() ) {
        
      case BSETYPE_RobotConnect:
      {
        BSE_RobotConnect *rcEvent = (BSE_RobotConnect*)event;
        printf("CozmoGame: Robot %d connect: %s !!!!!!\n", rcEvent->robotID_, rcEvent->successful_ ? "SUCCESS" : "FAILED");
        break;
      }
        
      case BSETYPE_DeviceDetectedVisionMarker:
      {
        // Send a message out to UI that the device found a vision marker

        break;
      }
      default:
        printf("CozmoGame: Received unknown event %d\n", event->GetEventType());
        break;
    }
  }
  
  
#pragma mark - CozmoGameHost Wrappers
  
  CozmoGameHost::CozmoGameHost()
  {
    _impl = new CozmoGameHostImpl();
  }
  
  CozmoGameHost::~CozmoGameHost()
  {
    delete _impl;
  }
  
  void CozmoGameHost::Init(const Json::Value &config)
  {
    _impl->Init(config);
  }
  
  void CozmoGameHost::Update(const float currentTime_sec)
  {
    _impl->Update(currentTime_sec);
  }
  
} // namespace Cozmo
} // namespace Anki


