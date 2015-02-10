/**
 * File: cozmoGame_impl.h
 *
 * Author: Kevin Yoon
 * Date:   7/11/2014
 *
 * Description: Defines the private implementation of the base/host/client CozmoGame objects.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_GAME_IMPL_H
#define ANKI_COZMO_GAME_IMPL_H

#include "anki/cozmo/game/cozmoGame.h"
#include "anki/cozmo/game/comms/uiMessageHandler.h"

#include "anki/cozmo/basestation/cozmoEngine.h"
#include "anki/cozmo/basestation/multiClientComms.h"
#include "anki/cozmo/basestation/signals/cozmoEngineSignals.h"

namespace Anki {
namespace Cozmo {

  class CozmoEngine;
  
  class CozmoGameImpl
  {
  public:
    using RunState = CozmoGame::RunState;
    
    CozmoGameImpl();
    virtual ~CozmoGameImpl();
    
    RunState GetRunState() const;
    
    Result StartEngine(const Json::Value& config);
    
    //
    // Vision API
    //
    bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime);
    
    void ProcessDeviceImage(const Vision::Image& image);
    
    const std::vector<Cozmo::MessageG2U_DeviceDetectedVisionMarker>& GetVisionMarkersDetectedByDevice() const;
    
    void SetImageSendMode(RobotID_t forRobotID, Cozmo::ImageSendMode_t newMode);
    
  protected:
    
    // Derived classes must be able to provide a pointer to a CozmoEngine
    virtual CozmoEngine* GetCozmoEngine() = 0;
    
    RunState         _runState;
    
    std::map<RobotID_t, ImageSendMode_t> _imageSendMode;
    
  private:
    
    // NOTE: Signal handler setup is in cozmoGame_signal_handlers.cpp
    
    void SetupSignalHandlers();
    
    // Signal causes the additin of a MessageG2U_DeviceDetectedVisionMarker message to
    // the vector below.
    void HandleDeviceDetectedVisionMarkerSignal(uint8_t engineID, uint32_t markerType,
                                                float x_upperLeft,  float y_upperLeft,
                                                float x_lowerLeft,  float y_lowerLeft,
                                                float x_upperRight, float y_upperRight,
                                                float x_lowerRight, float y_lowerRight);
    
    std::vector<Cozmo::MessageG2U_DeviceDetectedVisionMarker> _visionMarkersDetectedByDevice;
    
    std::vector<Signal::SmartHandle> _signalHandles;
    
  }; // CozmoGameImpl


  class CozmoGameHostImpl : public CozmoGameImpl
  {
  public:
    
    using RunState = CozmoGameHost::RunState;
    using PlaybackMode = CozmoGameHost::PlaybackMode;
    
    using AdvertisingUiDevice = CozmoGameHost::AdvertisingUiDevice;
    using AdvertisingRobot    = CozmoGameHost::AdvertisingRobot;
    
    CozmoGameHostImpl();
    virtual ~CozmoGameHostImpl();
    
    Result Init(const Json::Value& config);
    
    void ForceAddRobot(int              robotID,
                       const char*      robotIP,
                       bool             robotIsSimulated);
    
    bool ConnectToUiDevice(AdvertisingUiDevice whichDevice);
    bool ConnectToRobot(AdvertisingRobot whichRobot);
    
    int GetNumRobots() const;
    
    // Tick with game heartbeat:
    void Update(const float currentTime_sec);
    
  protected:
    
    
    // Req'd by CozmoGameImpl
    virtual CozmoEngine* GetCozmoEngine() { return &_cozmoEngine; }
    
    int                          _desiredNumUiDevices = 1;
    int                          _desiredNumRobots = 1;
    
    CozmoEngineHost              _cozmoEngine;
    
    // UI Comms Stuff
    Comms::AdvertisementService  _uiAdvertisementService;
    MultiClientComms             _uiComms;
    UiMessageHandler             _uiMsgHandler;
    u32                          _hostUiDeviceID;
    
    std::vector<AdvertisingUiDevice> _connectedUiDevices;
    
    
    // TODO: promote these to CozmoGame base class once we have distributed robot vision
    bool SendRobotImage(RobotID_t robotID);
    
  private:
    
    // Process raised events from CozmoEngine.
    // NOTE: These methods are in cozmoGame_signal_handlers.cpp
    std::vector<Signal::SmartHandle> _signalHandles;

    void SetupSignalHandlers();
    
    void HandleRobotAvailableSignal(RobotID_t robotID);
    void HandleUiDeviceAvailableSignal(UserDeviceID_t deviceID);
    void HandleRobotConnectedSignal(RobotID_t robotID, bool successful);
    void HandleUiDeviceConnectedSignal(UserDeviceID_t deviceID, bool successful);
    void HandlePlaySoundForRobotSignal(RobotID_t robotID, u32 soundID, u8 numLoops, u8 volume);
    void HandleStopSoundForRobotSignal(RobotID_t robotID);
    void HandleRobotObservedObjectSignal(uint8_t robotID, uint32_t objectID,
                                         float x_upperLeft,  float y_upperLeft,
                                         float width,  float height);
    void HandleConnectToRobotSignal(RobotID_t robotID);
    void HandleConnectToUiDeviceSignal(UserDeviceID_t deviceID);
    void HandleRobotImageAvailable(RobotID_t robotID);


    // Callbacks for incoming U2G messages
    // NOTE: Implemented in cozmoGame_U2G_callbacks.cpp
    void RegisterCallbacksU2G();
#   define MESSAGE_DEFINITION_MODE MESSAGE_PROCESS_METHODS_NO_WRAPPERS_MODE
#   define MESSAGE_BASECLASS_NAME UiMessage
#   include "anki/cozmo/game/comms/messaging/UiMessageDefinitionsU2G.def"
#   undef MESSAGE_BASECLASS_NAME

    //void ProcessMessage(const MessageU2G_AbortAll& msg);
    
  }; // class CozmoGameHostImpl
  
  
  class CozmoGameClientImpl : public CozmoGameImpl
  {
  public:
    using RunState = CozmoGame::RunState;
    
    CozmoGameClientImpl();
    virtual ~CozmoGameClientImpl();
    
    Result Init(const Json::Value& config);
    void Update(const float currentTime_sec);
    RunState GetRunState() const;
    
  protected:
    
    // Process raised events from CozmoEngine.
    // ...
    
    // Req'd by CozmoGameImpl
    virtual CozmoEngine* GetCozmoEngine() { return &_cozmoEngine; }
    
    RunState           _runState;
    CozmoEngineClient  _cozmoEngine;
    
  }; // class CozmoGameClientImpl
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_GAME_IMPL_H
