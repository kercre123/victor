/**
 * File: cozmoGame_impl.h
 *
 * Author: Andrew Stein
 * Date:   2/10/2015
 *
 * Description: Defines the private implementation of the CozmoGame object.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_GAME_IMPL_H
#define ANKI_COZMO_GAME_IMPL_H

#include "anki/cozmo/game/cozmoGame.h"
#include "anki/cozmo/game/comms/uiMessageHandler.h"
#include "anki/cozmo/basestation/cozmoEngine.h"
#include "anki/cozmo/basestation/cozmoEngineHost.h"
#include "anki/cozmo/basestation/multiClientComms.h"
#include "util/signals/simpleSignal_fwd.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

namespace Anki {
namespace Cozmo {
  
  // Forward Declarations
  namespace ExternalInterface {
  class MessageGameToEngine;
  }
  
  template <typename Type>
  class AnkiEvent;

  class CozmoEngine;
  
  class CozmoGameImpl
  {
  public:
    using RunState     = CozmoGame::RunState;
    using PlaybackMode = CozmoGame::PlaybackMode;
    
    using AdvertisingUiDevice = CozmoGame::AdvertisingUiDevice;
    using AdvertisingRobot    = CozmoGame::AdvertisingRobot;
    
    CozmoGameImpl();
    ~CozmoGameImpl();
    

    //
    // Startup / Initialization / Comms
    //
    
    Result StartEngine(Json::Value config);
    
    Result Init(const Json::Value& config);
    
    bool ConnectToUiDevice(AdvertisingUiDevice whichDevice);
    
    //
    // Running
    //

    // Tick with game heartbeat:
    Result Update(const float currentTime_sec);

    RunState GetRunState() const;
    
    // Only valid when running as host, returns -1 if running as client.
    int GetNumRobots() const;
    
    
    //
    // Vision API
    //
    bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime);
    
    void ProcessDeviceImage(const Vision::Image& image);
    
    const std::vector<ExternalInterface::DeviceDetectedVisionMarker>& GetVisionMarkersDetectedByDevice() const;
    
    void SetImageSendMode(RobotID_t forRobotID, Cozmo::ImageSendMode_t newMode);
    
  protected:
    
    Robot* GetRobotByID(const RobotID_t robotID);
    
    Result UpdateAsHost(const float currentTime_sec);
    Result UpdateAsClient(const float currentTime_sec);
    
    bool SendRobotImage(RobotID_t robotID);

    //
    // U2G Message Handling
    //
    //   NOTE: Implemented in cozmoGame_U2G_callbacks.cpp
    
    void RegisterCallbacksU2G();
    void ProcessBadTag_MessageGameToEngine(ExternalInterface::MessageGameToEngine::Tag tag);
#include "clad/externalInterface/messageGameToEngine_declarations.def"
    
    void SetupSubscriptions();
    void HandleEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
    void HandleStartEngine(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
    
    //
    // Member Variables
    //

    ExternalInterface::Ping                  _pingToUI;
    f32                              _lastPingTimeFromUI_sec;
    u32                              _lastPingCounterFromUI;
    
    bool                             _isHost;
    bool                             _isEngineStarted;
    RunState                         _runState;
    
    Json::Value                      _config;
    
    CozmoEngine*                     _cozmoEngine;
    int                              _desiredNumUiDevices;
    int                              _desiredNumRobots;

    Comms::AdvertisementService      _uiAdvertisementService;
    MultiClientComms                 _uiComms;
    UiMessageHandler                 _uiMsgHandler;

    std::vector<ExternalInterface::DeviceDetectedVisionMarker> _visionMarkersDetectedByDevice;
    
    std::vector<Signal::SmartHandle> _signalHandles;

    std::vector<AdvertisingUiDevice> _connectedUiDevices;
    
  }; // CozmoGameImpl

  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_GAME_IMPL_H
