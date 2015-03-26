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
    void HandleRobotCompletedAction(uint8_t robotID, uint8_t success);
    //
    // U2G Message Handling
    //
    //   NOTE: Implemented in cozmoGame_U2G_callbacks.cpp
    
    void RegisterCallbacksU2G();
#define REGISTER_CALLBACK(__MSG_TYPE__) void ProcessMessage(const U2G_##__MSG_TYPE__& msg);
    REGISTER_CALLBACK(Ping)
    REGISTER_CALLBACK(ConnectToRobot)
    REGISTER_CALLBACK(ConnectToUiDevice)
    REGISTER_CALLBACK(DisconnectFromUiDevice)
    REGISTER_CALLBACK(ForceAddRobot)
    REGISTER_CALLBACK(StartEngine)
    REGISTER_CALLBACK(DriveWheels)
    REGISTER_CALLBACK(TurnInPlace)
    REGISTER_CALLBACK(MoveHead)
    REGISTER_CALLBACK(MoveLift)
    REGISTER_CALLBACK(SetLiftHeight)
    REGISTER_CALLBACK(SetHeadAngle)
    REGISTER_CALLBACK(TrackHeadToObject)
    REGISTER_CALLBACK(StopAllMotors)
    REGISTER_CALLBACK(ImageRequest)
    REGISTER_CALLBACK(SetRobotImageSendMode)
    REGISTER_CALLBACK(SaveImages)
    REGISTER_CALLBACK(SaveRobotState)
    REGISTER_CALLBACK(EnableDisplay)
    REGISTER_CALLBACK(SetHeadlights)
    REGISTER_CALLBACK(GotoPose)
    REGISTER_CALLBACK(PlaceObjectOnGround)
    REGISTER_CALLBACK(PlaceObjectOnGroundHere)
    REGISTER_CALLBACK(ExecuteTestPlan)
    REGISTER_CALLBACK(SelectNextObject)
    REGISTER_CALLBACK(PickAndPlaceObject)
    REGISTER_CALLBACK(TraverseObject)
    REGISTER_CALLBACK(SetRobotCarryingObject)
    REGISTER_CALLBACK(ClearAllBlocks)
    REGISTER_CALLBACK(VisionWhileMoving)
    REGISTER_CALLBACK(ExecuteBehavior)
    REGISTER_CALLBACK(SetBehaviorState)
    REGISTER_CALLBACK(AbortPath)
    REGISTER_CALLBACK(AbortAll)
    REGISTER_CALLBACK(DrawPoseMarker)
    REGISTER_CALLBACK(ErasePoseMarker)
    REGISTER_CALLBACK(SetHeadControllerGains)
    REGISTER_CALLBACK(SetLiftControllerGains)
    REGISTER_CALLBACK(SelectNextSoundScheme)
    REGISTER_CALLBACK(StartTestMode)
    REGISTER_CALLBACK(IMURequest)
    REGISTER_CALLBACK(PlayAnimation)
    REGISTER_CALLBACK(ReadAnimationFile)
    REGISTER_CALLBACK(StartFaceTracking)
    REGISTER_CALLBACK(StopFaceTracking)
    REGISTER_CALLBACK(StartLookingForMarkers)
    REGISTER_CALLBACK(StopLookingForMarkers)
    REGISTER_CALLBACK(SetVisionSystemParams)
    REGISTER_CALLBACK(SetFaceDetectParams)
    REGISTER_CALLBACK(SetActiveObjectLEDs)
    REGISTER_CALLBACK(SetAllActiveObjectLEDs)
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
