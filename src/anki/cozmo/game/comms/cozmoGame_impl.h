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
#include "anki/cozmo/basestation/multiClientComms.h"
#include "anki/cozmo/basestation/signals/cozmoEngineSignals.h"

namespace Anki {
namespace Cozmo {

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
    
    void ForceAddRobot(int              robotID,
                       const char*      robotIP,
                       bool             robotIsSimulated);
    
    bool ConnectToUiDevice(AdvertisingUiDevice whichDevice);
    bool ConnectToRobot(AdvertisingRobot whichRobot);
    
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
    
    const std::vector<Cozmo::G2U::DeviceDetectedVisionMarker>& GetVisionMarkersDetectedByDevice() const;
    
    void SetImageSendMode(RobotID_t forRobotID, Cozmo::ImageSendMode_t newMode);
    
  protected:
    
    Robot* GetRobotByID(const RobotID_t robotID);
    
    Result UpdateAsHost(const float currentTime_sec);
    Result UpdateAsClient(const float currentTime_sec);
    
    bool SendRobotImage(RobotID_t robotID);
    
    //
    // Signals
    //
    //   NOTE: Signal handler implementations are in cozmoGame_signal_handlers.cpp
    
    void SetupSignalHandlers();
    
    void HandleRobotAvailableSignal(RobotID_t robotID);
    void HandleUiDeviceAvailableSignal(UserDeviceID_t deviceID);
    void HandleRobotConnectedSignal(RobotID_t robotID, bool successful);
    void HandleRobotDisconnectedSignal(RobotID_t robotID);
    void HandleUiDeviceConnectedSignal(UserDeviceID_t deviceID, bool successful);
    void HandlePlaySoundForRobotSignal(RobotID_t robotID, u32 soundID, u8 numLoops, u8 volume);
    void HandleStopSoundForRobotSignal(RobotID_t robotID);
    void HandleRobotObservedObjectSignal(uint8_t robotID, uint32_t objectFamily,
                                         uint32_t objectType, uint32_t objectID,
                                         uint8_t markersVisible,
                                         float img_x_upperLeft,  float img_y_upperLeft,
                                         float img_width,  float img_height,
                                         float world_x,
                                         float world_y,
                                         float world_z,
                                         float q0, float q1, float q2, float q3,
                                         float topMarkerOrientation_rad,
                                         bool isActive);
    void HandleRobotObservedNothingSignal(uint8_t robotID);
    void HandleRobotDeletedObjectSignal(uint8_t robotID, uint32_t objectID);
    void HandleConnectToRobotSignal(RobotID_t robotID);
    void HandleConnectToUiDeviceSignal(UserDeviceID_t deviceID);
    void HandleRobotImageAvailable(RobotID_t robotID);
    void HandleRobotImageChunkAvailable(RobotID_t robotID, const void *chunkMsg);
    void HandleDeviceDetectedVisionMarkerSignal(uint8_t engineID, uint32_t markerType,
                                                float x_upperLeft,  float y_upperLeft,
                                                float x_lowerLeft,  float y_lowerLeft,
                                                float x_upperRight, float y_upperRight,
                                                float x_lowerRight, float y_lowerRight);
    void HandleRobotCompletedAction(uint8_t robotID, RobotActionType actionType,
                                    ActionResult result,
                                    std::array<int32_t,5> objectIDs, uint8_t numObjects);
    void HandleActiveObjectMoved(uint8_t robotID, uint32_t objectID,
                                 float xAccel, float yAccel, float zAccel,
                                 uint8_t upAxis);
    void HandleActiveObjectStoppedMoving(uint8_t robotID, uint32_t objectID, uint8_t upAxis, bool rolled);
    void HandleActiveObjectTapped(uint8_t robotID, uint32_t objectID, uint8_t numTaps);
    
    //
    // U2G Message Handling
    //
    //   NOTE: Implemented in cozmoGame_U2G_callbacks.cpp
    
    void RegisterCallbacksU2G();
    void ProcessBadTag_Message(U2G::Message::Tag tag);
#include "anki/cozmo/messageBuffers/game/UiMessagesU2G_declarations.def"
    
    //
    // Member Variables
    //
    
    Cozmo::G2U::Ping                  _pingToUI;
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
    u32                              _hostUiDeviceID;
    
    std::map<RobotID_t, ImageSendMode_t> _imageSendMode;
    
    std::vector<Cozmo::G2U::DeviceDetectedVisionMarker> _visionMarkersDetectedByDevice;
    
    std::vector<Signal::SmartHandle> _signalHandles;

    std::vector<AdvertisingUiDevice> _connectedUiDevices;
    
  }; // CozmoGameImpl

  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_GAME_IMPL_H
