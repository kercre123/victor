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
    
    const std::vector<Cozmo::G2U_DeviceDetectedVisionMarker>& GetVisionMarkersDetectedByDevice() const;
    
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
    void HandleRobotDisconnectedSignal(RobotID_t robotID, float timeSinceLastMsg_sec);
    void HandleUiDeviceConnectedSignal(UserDeviceID_t deviceID, bool successful);
    void HandlePlaySoundForRobotSignal(RobotID_t robotID, u32 soundID, u8 numLoops, u8 volume);
    void HandleStopSoundForRobotSignal(RobotID_t robotID);
    void HandleRobotObservedObjectSignal(uint8_t robotID, uint32_t objectFamily,
                                         uint32_t objectType, uint32_t objectID,
                                         float img_x_upperLeft,  float img_y_upperLeft,
                                         float img_width,  float img_height,
                                         float world_x,
                                         float world_y,
                                         float world_z);
    void HandleRobotObservedNothingSignal(uint8_t robotID);
    void HandleConnectToRobotSignal(RobotID_t robotID);
    void HandleConnectToUiDeviceSignal(UserDeviceID_t deviceID);
    void HandleRobotImageAvailable(RobotID_t robotID);
    void HandleDeviceDetectedVisionMarkerSignal(uint8_t engineID, uint32_t markerType,
                                                float x_upperLeft,  float y_upperLeft,
                                                float x_lowerLeft,  float y_lowerLeft,
                                                float x_upperRight, float y_upperRight,
                                                float x_lowerRight, float y_lowerRight);
    void HandleRobotCompletedPickAndPlaceAction(uint8_t robotID, uint8_t success);
    void HandleRobotCompletedPlaceObjectOnGroundAction(uint8_t robotID, uint8_t success);
    //
    // U2G Message Handling
    //
    //   NOTE: Implemented in cozmoGame_U2G_callbacks.cpp
    
    void RegisterCallbacksU2G();
#define REGISTER_CALLBACK(__MSG_TYPE__) void ProcessMessage(const __MSG_TYPE__& msg);
    REGISTER_CALLBACK(U2G_Ping)
    REGISTER_CALLBACK(U2G_ConnectToRobot)
    REGISTER_CALLBACK(U2G_ConnectToUiDevice)
    REGISTER_CALLBACK(U2G_DisconnectFromUiDevice)
    REGISTER_CALLBACK(U2G_ForceAddRobot)
    REGISTER_CALLBACK(U2G_StartEngine)
    REGISTER_CALLBACK(U2G_DriveWheels)
    REGISTER_CALLBACK(U2G_TurnInPlace)
    REGISTER_CALLBACK(U2G_MoveHead)
    REGISTER_CALLBACK(U2G_MoveLift)
    REGISTER_CALLBACK(U2G_SetLiftHeight)
    REGISTER_CALLBACK(U2G_SetHeadAngle)
    REGISTER_CALLBACK(U2G_StopAllMotors)
    REGISTER_CALLBACK(U2G_ImageRequest)
    REGISTER_CALLBACK(U2G_SetRobotImageSendMode)
    REGISTER_CALLBACK(U2G_SaveImages)
    REGISTER_CALLBACK(U2G_EnableDisplay)
    REGISTER_CALLBACK(U2G_SetHeadlights)
    REGISTER_CALLBACK(U2G_GotoPose)
    REGISTER_CALLBACK(U2G_PlaceObjectOnGround)
    REGISTER_CALLBACK(U2G_PlaceObjectOnGroundHere)
    REGISTER_CALLBACK(U2G_ExecuteTestPlan)
    REGISTER_CALLBACK(U2G_SelectNextObject)
    REGISTER_CALLBACK(U2G_PickAndPlaceObject)
    REGISTER_CALLBACK(U2G_TraverseObject)
    REGISTER_CALLBACK(U2G_SetRobotCarryingObject)
    REGISTER_CALLBACK(U2G_ClearAllBlocks)
    REGISTER_CALLBACK(U2G_ExecuteBehavior)
    REGISTER_CALLBACK(U2G_SetBehaviorState)
    REGISTER_CALLBACK(U2G_AbortPath)
    REGISTER_CALLBACK(U2G_AbortAll)
    REGISTER_CALLBACK(U2G_DrawPoseMarker)
    REGISTER_CALLBACK(U2G_ErasePoseMarker)
    REGISTER_CALLBACK(U2G_SetHeadControllerGains)
    REGISTER_CALLBACK(U2G_SetLiftControllerGains)
    REGISTER_CALLBACK(U2G_SelectNextSoundScheme)
    REGISTER_CALLBACK(U2G_StartTestMode)
    REGISTER_CALLBACK(U2G_IMURequest)
    REGISTER_CALLBACK(U2G_PlayAnimation)
    REGISTER_CALLBACK(U2G_ReadAnimationFile)
    REGISTER_CALLBACK(U2G_StartFaceTracking)
    REGISTER_CALLBACK(U2G_StopFaceTracking)
    REGISTER_CALLBACK(U2G_StartLookingForMarkers)
    REGISTER_CALLBACK(U2G_StopLookingForMarkers)
    REGISTER_CALLBACK(U2G_SetVisionSystemParams)
    REGISTER_CALLBACK(U2G_SetFaceDetectParams)
#undef REGISTER_CALLBACK
    
    //
    // Member Variables
    //
    
    Cozmo::G2U_Ping                  _pingToUI;
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
    
    std::vector<Cozmo::G2U_DeviceDetectedVisionMarker> _visionMarkersDetectedByDevice;
    
    std::vector<Signal::SmartHandle> _signalHandles;

    std::vector<AdvertisingUiDevice> _connectedUiDevices;
    
  }; // CozmoGameImpl

  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_GAME_IMPL_H
