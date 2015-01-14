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
#include "anki/cozmo/basestation/soundManager.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include "anki/common/basestation/utils/logging/logging.h"

#include "anki/cozmo/basestation/uiMessageHandler.h"
#include "anki/cozmo/basestation/multiClientComms.h"
#include "anki/cozmo/basestation/signals/cozmoEngineSignals.h"
#include "anki/cozmo/game/signals/cozmoGameSignals.h"

namespace Anki {
namespace Cozmo {
  
#pragma mark - CozmoGame Implementation
  class CozmoGameImpl : public IBaseStationEventListener
  {
  public:
    using RunState = CozmoGame::RunState;
    
    CozmoGameImpl();
    virtual ~CozmoGameImpl();
    
    RunState GetRunState() const;
    
    //
    // Vision API
    //
    bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime);
    
    void ProcessDeviceImage(const Vision::Image& image);
    
    bool WasLastDeviceImageProcessed();
    
    bool CheckDeviceVisionProcessingMailbox(MessageVisionMarker& msg);
    
  protected:

    // Derived classes must be able to provide a pointer to a CozmoEngine
    virtual CozmoEngine* GetCozmoEngine() = 0;
    
    RunState         _runState;
  };
  
  CozmoGameImpl::CozmoGameImpl()
  : _runState(CozmoGame::STOPPED)
  {
    
  }
  
  CozmoGameImpl::~CozmoGameImpl()
  {
    // NOTE: Do not delete _cozmoEngine pointer here. It's just a pointer to an
    // engine living in the derived class, and its memory should be handled there.
  }
  
  CozmoGameImpl::RunState CozmoGameImpl::GetRunState() const
  {
    return _runState;
  }
  
  bool CozmoGameImpl::GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime)
  {
    return GetCozmoEngine()->GetCurrentRobotImage(robotId, img, newerThanTime);
  }
  
  void CozmoGameImpl::ProcessDeviceImage(const Vision::Image& image)
  {
    GetCozmoEngine()->ProcessDeviceImage(image);
  }
  
  bool CozmoGameImpl::WasLastDeviceImageProcessed()
  {
    return GetCozmoEngine()->WasLastDeviceImageProcessed();
  }
  
  bool CozmoGameImpl::CheckDeviceVisionProcessingMailbox(MessageVisionMarker& msg)
  {
    return GetCozmoEngine()->CheckDeviceVisionProcessingMailbox(msg);
  }
  
#pragma mark - CozmoGame Wrappers
  
  CozmoGame::CozmoGame()
  : _impl(nullptr)
  {
    // NOTE: _impl should be set by the derived classes' constructors
  }
  
  CozmoGame::~CozmoGame()
  {
    // Let it be derived class's problem to delete their implementations
    _impl = nullptr;
  }
  
  bool CozmoGame::GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime)
  {
    assert(_impl != nullptr);
    return _impl->GetCurrentRobotImage(robotId, img, newerThanTime);
  }
  
  void CozmoGame::ProcessDeviceImage(const Vision::Image& image)
  {
    _impl->ProcessDeviceImage(image);
  }
  
  bool CozmoGame::WasLastDeviceImageProcessed()
  {
    return _impl->WasLastDeviceImageProcessed();
  }
  
  CozmoGame::RunState CozmoGame::GetRunState() const
  {
    assert(_impl != nullptr);
    return _impl->GetRunState();
  }
  
  bool CozmoGame::CheckDeviceVisionProcessingMailbox(MessageVisionMarker& msg)
  {
    return _impl->CheckDeviceVisionProcessingMailbox(msg);
  }
  
#pragma mark - CozmoGameHost Implementation
  
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
    
    // Tick with game heartbeat:
    void Update(const float currentTime_sec);
    
  protected:
    
    // Process raised events from CozmoEngine.
    // Req'd by IBaseStationEventListener
    //virtual void OnEventRaised( const IBaseStationEventInterface* event ) override;
    
    // Signal handlers
    void HandleRobotConnectSignal(RobotID_t robotID, bool successful);
    void HandleRobotAvailableSignal(RobotID_t robotID);
    void HandleUiDeviceAvailableSignal(UserDeviceID_t deviceID);
    void HandleDeviceDetectedVisionMarkerSignal(uint8_t engineID, uint32_t markerType,
                                                float x_upperLeft,  float y_upperLeft,
                                                float x_lowerLeft,  float y_lowerLeft,
                                                float x_upperRight, float y_upperRight,
                                                float x_lowerRight, float y_lowerRight);
    void HandleRobotObservedObjectSignal(uint8_t robotID, uint32_t objectID,
                                         float x_upperLeft,  float y_upperLeft,
                                         float width,  float height);
    
    void HandleConnectToRobotSignal(RobotID_t robotID);
    void HandleConnectToUiDeviceSignal(UserDeviceID_t deviceID);
    
    
    enum RunState {
      STOPPED = 0
      ,WAITING_FOR_UI_DEVICES
      ,WAITING_FOR_ROBOTS
      ,ENGINE_RUNNING
    };
    
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
    
    // Signal handler handles
    std::vector<Signal::SmartHandle> _signalHandles;
    
  }; // class CozmoGameHostImpl
  
  
  CozmoGameHostImpl::CozmoGameHostImpl()
  : _uiAdvertisementService("UIAdvertisementService")
  , _hostUiDeviceID(1)
  {
    // Register for signals of interest
/*
    _signalHandles.emplace_back( CozmoEngineSignals::GetRobotConnectSignal().ScopedSubscribe(
      std::bind(&CozmoGameHostImpl::HandleRobotConnectSignal, this,
                std::placeholders::_1,
                std::placeholders::_2)));
 
    _signalHandles.emplace_back( CozmoEngineSignals::GetRobotAvailableSignal().ScopedSubscribe(
      std::bind(&CozmoGameHostImpl::HandleRobotAvailableSignal, this,
                std::placeholders::_1)));

    
    _signalHandles.emplace_back( CozmoEngineSignals::GetUiDeviceAvailableSignal().ScopedSubscribe(
      std::bind(&CozmoGameHostImpl::HandleUiDeviceAvailableSignal, this,
                std::placeholders::_1)));
    
    _signalHandles.emplace_back( CozmoEngineSignals::GetDeviceDetectedVisionMarkerSignal().ScopedSubscribe(
      std::bind(&CozmoGameHostImpl::HandleDeviceDetectedVisionMarkerSignal, this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                std::placeholders::_4,
                std::placeholders::_5,
                std::placeholders::_6,
                std::placeholders::_7,
                std::placeholders::_8,
                std::placeholders::_9,
                std::placeholders::_10)));
    
    _signalHandles.emplace_back( CozmoEngineSignals::GetRobotObservedObjectSignal().ScopedSubscribe(
      std::bind(&CozmoGameHostImpl::HandleRobotObservedObjectSignal, this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3,
                std::placeholders::_4,
                std::placeholders::_5,
                std::placeholders::_6)));

    
    _signalHandles.emplace_back( CozmoGameSignals::GetConnectToRobotSignal().ScopedSubscribe(
      std::bind(&CozmoGameHostImpl::HandleConnectToRobotSignal, this,
                std::placeholders::_1)));
    
    _signalHandles.emplace_back( CozmoGameSignals::GetConnectToUiDeviceSignal().ScopedSubscribe(
      std::bind(&CozmoGameHostImpl::HandleConnectToUiDeviceSignal, this,
                std::placeholders::_1)));
 */
    
    auto cbRobotConnectSignal = [this](RobotID_t robotID, bool successful) {
      this->HandleRobotConnectSignal(robotID, successful);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::GetRobotConnectSignal().ScopedSubscribe(cbRobotConnectSignal));
    
    auto cbRobotAvailableSignal = [this](RobotID_t robotID) {
      this->HandleRobotAvailableSignal(robotID);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::GetRobotAvailableSignal().ScopedSubscribe(cbRobotAvailableSignal));
    
    auto cbUiDeviceAvailableSignal = [this](UserDeviceID_t deviceID) {
      this->HandleUiDeviceAvailableSignal(deviceID);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::GetUiDeviceAvailableSignal().ScopedSubscribe(cbUiDeviceAvailableSignal));
    
    auto cbDeviceDetectedVisionMarkerSignal = [this](uint8_t engineID, uint32_t markerType,
                                                     float x_upperLeft,  float y_upperLeft,
                                                     float x_lowerLeft,  float y_lowerLeft,
                                                     float x_upperRight, float y_upperRight,
                                                     float x_lowerRight, float y_lowerRight) {
      this->HandleDeviceDetectedVisionMarkerSignal(engineID, markerType,
                                                   x_upperLeft,  y_upperLeft,
                                                   x_lowerLeft,  y_lowerLeft,
                                                   x_upperRight, y_upperRight,
                                                   x_lowerRight, y_lowerRight);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::GetDeviceDetectedVisionMarkerSignal().ScopedSubscribe(cbDeviceDetectedVisionMarkerSignal));
    
    auto cbRobotObservedObjectSignal = [this](uint8_t robotID, uint32_t objectID,
                                              float x_upperLeft,  float y_upperLeft,
                                              float width,  float height) {
      this->HandleRobotObservedObjectSignal(robotID, objectID, x_upperLeft, y_upperLeft, width, height);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::GetRobotObservedObjectSignal().ScopedSubscribe(cbRobotObservedObjectSignal));
    

    auto cbConnectToRobotSignal = [this](RobotID_t robotID) {
      this->HandleConnectToRobotSignal(robotID);
    };
    _signalHandles.emplace_back( CozmoGameSignals::GetConnectToRobotSignal().ScopedSubscribe(cbConnectToRobotSignal));

    auto cbConnectToUiDeviceSignal = [this](UserDeviceID_t deviceID) {
      this->HandleConnectToUiDeviceSignal(deviceID);
    };
    _signalHandles.emplace_back( CozmoGameSignals::GetConnectToUiDeviceSignal().ScopedSubscribe(cbConnectToUiDeviceSignal));

    
    
    PRINT_NAMED_INFO("CozmoEngineHostImpl.Constructor",
                     "Starting UIAdvertisementService, reg port %d, ad port %d\n",
                     UI_ADVERTISEMENT_REGISTRATION_PORT, UI_ADVERTISING_PORT);
    
    _uiAdvertisementService.StartService(UI_ADVERTISEMENT_REGISTRATION_PORT,
                                         UI_ADVERTISING_PORT);
    
  }

  CozmoGameHostImpl::~CozmoGameHostImpl()
  {
    // Other tear-down:
    SoundManager::removeInstance();
  }
  
  Result CozmoGameHostImpl::Init(const Json::Value& config)
  {
    /*
    // Force add any advertising UI device on this same device:
    Comms::AdvertisementRegistrationMsg localUiRegMsg;
    localUiRegMsg.id = 1;
    localUiRegMsg.port = UI_ADVERTISEMENT_REGISTRATION_PORT;
    localUiRegMsg.protocol = Comms::UDP;
    localUiRegMsg.enableAdvertisement = 1;
    _uiAdvertisementService.ProcessRegistrationMsg(localUiRegMsg);
    */
    
    _cozmoEngine.Init(config);
    
    if(!config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP) ||
       !config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
     
      PRINT_NAMED_ERROR("CozmoGameHostImpl.Init", "Missing advertising hosdt / UI advertising port in Json config file.\n");
      return RESULT_FAIL;
    }
      
    Result lastResult = _uiComms.Init(config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString(),
                                      config[AnkiUtil::kP_UI_ADVERTISING_PORT].asInt());

    if(lastResult != RESULT_OK) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.Init", "Failed to initialize host uiComms.\n");
      return lastResult;
    }
    
    _uiMsgHandler.Init(&_uiComms, &_cozmoEngine);
    
    if(!config.isMember(AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR)) {
      PRINT_NAMED_WARNING("CozmoGameHostImpl.Init", "No NumRobotsToWaitFor defined in Json config, defaulting to 1.\n");
      _desiredNumRobots = 1;
    } else {
      _desiredNumRobots    = config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR].asInt();
    }
    
    if(!config.isMember(AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR)) {
      PRINT_NAMED_WARNING("CozmoGameHostImpl.Init", "No NumUiDevicesToWaitFor defined in Json config, defaulting to 1.\n");
      _desiredNumUiDevices = 1;
    } else {
      _desiredNumUiDevices = config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR].asInt();
    }
    
    _runState = CozmoGameHost::WAITING_FOR_UI_DEVICES;

      return RESULT_OK;
  }
  
  void CozmoGameHostImpl::ForceAddRobot(int              robotID,
                                        const char*      robotIP,
                                        bool             robotIsSimulated)
  {
    _cozmoEngine.ForceAddRobot(robotID, robotIP, robotIsSimulated);
  }
  
  
  bool CozmoGameHostImpl::ConnectToUiDevice(AdvertisingUiDevice whichDevice)
  {
    const bool success = _uiComms.ConnectToDeviceByID(whichDevice);
    if(success) {
      _connectedUiDevices.push_back(whichDevice);
    }
    
    return success;
  }

  bool CozmoGameHostImpl::ConnectToRobot(AdvertisingRobot whichRobot)
  {
    return _cozmoEngine.ConnectToRobot(whichRobot);
  }
  
  void CozmoGameHostImpl::Update(const float currentTime_sec) {
    /*
    // Connect to any robots we see:
    std::vector<CozmoEngine::AdvertisingRobot> advertisingRobots;
    _cozmoEngine.GetAdvertisingRobots(advertisingRobots);
    for(auto robot : advertisingRobots) {
      _cozmoEngine.ConnectToRobot(robot);
    }
    
    // Connect to any UI devices we see:
    std::vector<CozmoEngine::AdvertisingUiDevice> advertisingUiDevices;
    _cozmoEngine.GetAdvertisingUiDevices(advertisingUiDevices);
    for(auto device : advertisingUiDevices) {
      _cozmoEngine.ConnectToUiDevice(device);
    }
     */
    
    // Update ui comms
    if(_uiComms.IsInitialized()) {
      _uiComms.Update();
    }
    
    // Handle UI messages
    _uiMsgHandler.ProcessMessages();
    
    switch(_runState)
    {
      case CozmoGameHost::STOPPED:
        // Nothing to do
        break;
        
      case CozmoGameHost::WAITING_FOR_UI_DEVICES:
      {
        _uiAdvertisementService.Update();

        // TODO: Do we want to do this all the time in case UI devices want to join later?
        // Notify the UI that there are advertising devices
        std::vector<int> advertisingUiDevices;
        _uiComms.GetAdvertisingDeviceIDs(advertisingUiDevices);
        for(auto & device : advertisingUiDevices) {
          if(device == _hostUiDeviceID) {
            // Force connection to first (local) UI device
            if(true == ConnectToUiDevice(device)) {
              PRINT_NAMED_INFO("CozmoGameHostImpl.Update", "Automatically connected to local UI device %d!\n", device);
            }
          } else {
            CozmoEngineSignals::GetUiDeviceAvailableSignal().emit(device);
          }
          /*
          MessageG2U_UiDeviceAvailable msg;
          msg.deviceID = device;
          _uiMsgHandler.SendMessage(1, msg);
           */
        }

        if(_uiComms.GetNumConnectedDevices() >= _desiredNumUiDevices) {
          PRINT_NAMED_INFO("CozmoGameHostImpl.Update",
                           "Enough UI devices connected (%d), will wait for %d robots.\n",
                           _desiredNumUiDevices, _desiredNumRobots);
          _cozmoEngine.ListenForRobotConnections(true);
          _runState = CozmoGameHost::WAITING_FOR_ROBOTS;
        }
        break;
      }
    
      case CozmoGameHost::WAITING_FOR_ROBOTS:
      {
        Result status = _cozmoEngine.Update(SEC_TO_NANOS(currentTime_sec));
        if (status != RESULT_OK) {
          PRINT_NAMED_WARNING("CozmoGameHostImpl.Update","Bad engine update: status = %d\n", status);
        }
        
        // Tell the engine to keep listening for robots until it reports that
        // it has connections to enough
        if(_cozmoEngine.GetNumRobots() >= _desiredNumRobots) {
          PRINT_NAMED_INFO("CozmoGameHostImpl.Update",
                           "Enough robots connected (%d), will run engine.\n",
                           _desiredNumRobots);
          // TODO: We could keep listening for others to join mid-game...
          _cozmoEngine.ListenForRobotConnections(false);
          _runState = CozmoGameHost::ENGINE_RUNNING;
        }
        break;
      }
        
      case CozmoGameHost::ENGINE_RUNNING:
      {
        Result status = _cozmoEngine.Update(SEC_TO_NANOS(currentTime_sec));
        if (status != RESULT_OK) {
          PRINT_NAMED_WARNING("CozmoGameHostImpl.Update","Bad engine update: status = %d\n", status);
        }
        break;
      }
        
      default:
        PRINT_NAMED_ERROR("CozmoHostImpl.Update", "Reached unknown RunState %d.\n", _runState);
        
    }
    
    /*
     std::vector<BasestationMain::ObservedObjectBoundingBox> boundingQuads;
     if(true == bs.GetCurrentVisionMarkers(1, boundingQuads) ) {
     // TODO: stuff?
     }
     */
  } // Update()
  
  ///////////////////////////////////////////////
  // Signal handlers
  ///////////////////////////////////////////////
  void CozmoGameHostImpl::HandleRobotConnectSignal(RobotID_t robotID, bool successful)
  {
    // TODO
    {
      case BSETYPE_RobotAvailable:
      {
        // Notify UI that robot is available and let it issue message to connect
        MessageG2U_RobotAvailable msg;
        msg.robotID = reinterpret_cast<const BSE_RobotAvailable*>(event)->robotID_;
        _uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
        break;
      }
        
      case BSETYPE_UiDeviceAvailable:
      {
        // Notify UI that a UI device is available and let it issue message to connect
        MessageG2U_UiDeviceAvailable msg;
        msg.deviceID = reinterpret_cast<const BSE_UiDeviceAvailable*>(event)->deviceID_;
        _uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
        break;
      }
        
      case BSETYPE_ConnectToRobot:
      {
        const RobotID_t robotID = reinterpret_cast<const BSE_ConnectToRobot*>(event)->robotID_;
        const bool success = ConnectToRobot(robotID);
        if(success) {
          PRINT_NAMED_INFO("CozmoGameHost.OnEventRaised", "Connected to robot %d!\n", robotID);
        } else {
          PRINT_NAMED_ERROR("CozmoGameHost.OnEventRaised", "Failed to connected to robot %d!\n", robotID);
        }
      }
        
      case BSETYPE_ConnectToUiDevice:
      {
        const u32 deviceID = reinterpret_cast<const BSE_ConnectToUiDevice*>(event)->deviceID_;
        const bool success = ConnectToUiDevice(deviceID);
        if(success) {
          PRINT_NAMED_INFO("CozmoGameHost.OnEventRaised", "Connected to UI device %d!\n", deviceID);
        } else {
          PRINT_NAMED_ERROR("CozmoGameHost.OnEventRaised", "Failed to connected to UI device %d!\n", deviceID);
        }
      }
        
      case BSETYPE_PlaySoundForRobot:
      {
        const BSE_PlaySoundForRobot* playEvent = reinterpret_cast<const BSE_PlaySoundForRobot*>(event);
        
#       if ANKI_IOS_BUILD
        // Tell the host UI device to play a sound:
        MessageG2U_PlaySound msg;
        msg.numLoops = playEvent->numLoops_;
        msg.volume   = playEvent->volume_;
        const std::string& filename = SoundManager::getInstance()->GetSoundFile((SoundID_t)playEvent->soundID_);
        strncpy(&(msg.soundFilename[0]), filename.c_str(), msg.soundFilename.size());
        
        bool success = RESULT_OK == _uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
        
#       else
        // Use SoundManager:
        bool success = SoundManager::getInstance()->Play((SoundID_t)playEvent->soundID_,
                                                         playEvent->numLoops_,
                                                         playEvent->volume_);
#       endif
        
        if(!success) {
          PRINT_NAMED_ERROR("CozmoGameHost.OnEventRaise.PlaySoundForRobot",
                            "SoundManager failed to play sound ID %d.\n", playEvent->soundID_);
        }
        
        break;
      } // BSETYPE_PlaySoundForRobot
        
      case BSETYPE_StopSoundForRobot:
      {
#       if ANKI_IOS_BUILD
        // Tell the host UI device to stop the sound
        // TODO: somehow use the robot ID?
        MessageG2U_StopSound msg;
        _uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
#       else
        // Use SoundManager:
        SoundManager::getInstance()->Stop();
#       endif
        
        break;
      }
        
        /*
      case BSETYPE_DeviceDetectedVisionMarker:
      {
        // Send a message out to UI that the device found a vision marker
        const BSE_DeviceDetectedVisionMarker *vmEvent = reinterpret_cast<const BSE_DeviceDetectedVisionMarker*>(event);
        assert(vmEvent != nullptr);
        
        MessageG2U_ObjectVisionMarker msg;
        msg.objectID = vmEvent->markerType_;
        
        break;
      }
       */
      case BSETYPE_RobotObservedObject:
      {
        // Send a message out to UI that the device found a vision marker
        const BSE_RobotObservedObject *obsObjEvent = reinterpret_cast<const BSE_RobotObservedObject*>(event);
        assert(obsObjEvent != nullptr);
        
        MessageG2U_ObjectVisionMarker msg;
        msg.robotID   = obsObjEvent->robotID_;
        msg.objectID  = obsObjEvent->objectID_;
        msg.topLeft_x = obsObjEvent->x_upperLeft_;
        msg.topLeft_y = obsObjEvent->y_upperLeft_;
        msg.width     = obsObjEvent->width_;
        msg.height    = obsObjEvent->height_;
        
        // TODO: Look up which UI device to notify based on the robotID that saw the object
        _uiMsgHandler.SendMessage(1, msg);
        
        break;
      }
        
      default:
        printf("CozmoGame: Received unknown event %d\n", event->GetEventType());
        break;
    }
  }
  
  void CozmoGameHostImpl::HandleRobotAvailableSignal(RobotID_t robotID) {
    MessageG2U_RobotAvailable msg;
    msg.robotID = robotID;
    
    // TODO: Notify UI that robot is availale and let it issue message to connect
    //_uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
    
    // For now, just connect any robot we see
    CozmoGameSignals::GetConnectToRobotSignal().emit(msg.robotID);
  }

  void CozmoGameHostImpl::HandleUiDeviceAvailableSignal(UserDeviceID_t deviceID) {
    MessageG2U_UiDeviceAvailable msg;
    msg.deviceID = deviceID;
    
    // TODO: Notify UI that a UI device is availale and let it issue message to connect
    //_uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
    
    // For now, just connect to any UI device we see:
    CozmoGameSignals::GetConnectToUiDeviceSignal().emit(msg.deviceID);
  }

  void CozmoGameHostImpl::HandleDeviceDetectedVisionMarkerSignal(uint8_t engineID, uint32_t markerType,
                                                                 float x_upperLeft,  float y_upperLeft,
                                                                 float x_lowerLeft,  float y_lowerLeft,
                                                                 float x_upperRight, float y_upperRight,
                                                                 float x_lowerRight, float y_lowerRight)
  {
    // TODO
    PRINT_NAMED_WARNING("CozmoGameHost.EmptyHandler.DeviceDetectedVisionMarkerSignal","");
    
    /*
    // Send a message out to UI that the device found a vision marker
    const BSE_DeviceDetectedVisionMarker *vmEvent = reinterpret_cast<const BSE_DeviceDetectedVisionMarker*>(event);
    assert(vmEvent != nullptr);
    
    MessageG2U_ObjectVisionMarker msg;
    msg.objectID = vmEvent->markerType_;
    */
  }
  
  void CozmoGameHost::ForceAddRobot(int              robotID,
                                    const char*      robotIP,
                                    bool             robotIsSimulated)
  {
    _hostImpl->ForceAddRobot(robotID, robotIP, robotIsSimulated);
  }
  
  /*
  bool CozmoGameHost::ConnectToRobot(AdvertisingRobot whichRobot)
  {
    return _hostImpl->ConnectToRobot(whichRobot);
    // Send a message out to UI that the device found a vision marker
    MessageG2U_ObjectVisionMarker msg;
    msg.robotID   = robotID;
    msg.objectID  = objectID;
    msg.topLeft_x = x_upperLeft;
    msg.topLeft_y = y_upperLeft;
    msg.width     = width;
    msg.height    = height;
    
    // TODO: Look up which UI device to notify based on the robotID that saw the object
    _uiMsgHandler.SendMessage(1, msg);
  
  bool CozmoGameHost::ConnectToUiDevice(AdvertisingUiDevice whichDevice)
  {
    return _hostImpl->ConnectToUiDevice(whichDevice);
  }
   */
  
  void CozmoGameHostImpl::HandleConnectToRobotSignal(RobotID_t robotID)
  {
    const bool success = ConnectToRobot(robotID);
    if(success) {
      PRINT_NAMED_INFO("CozmoGameHost.OnEventRaised", "Connected to robot %d!\n", robotID);
    } else {
      PRINT_NAMED_ERROR("CozmoGameHost.OnEventRaised", "Failed to connected to robot %d!\n", robotID);
    }
  }
  
  
#pragma mark - CozmoGameClient Implementation
  
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
    // Req'd by IBaseStationEventListener
    virtual void OnEventRaised( const IBaseStationEventInterface* event ) override;

    // Req'd by CozmoGameImpl
    virtual CozmoEngine* GetCozmoEngine() { return &_cozmoEngine; }
    
    RunState           _runState;
    CozmoEngineClient  _cozmoEngine;
    
  }; // class CozmoGameClientImpl
  
  CozmoGameClientImpl::CozmoGameClientImpl()
  : _runState(CozmoGame::STOPPED)
  {
    
    if(success) {
      PRINT_NAMED_INFO("CozmoGameHost.OnEventRaised", "Connected to UI device %d!\n", deviceID);
    } else {
      PRINT_NAMED_ERROR("CozmoGameHost.OnEventRaised", "Failed to connected to UI device %d!\n", deviceID);
    }
  }
  
  CozmoGameClientImpl::~CozmoGameClientImpl()
  {

  }
  
  Result CozmoGameClientImpl::Init(const Json::Value& config)
  {
    
    return RESULT_OK;
  }
  
  void CozmoGameClientImpl::Update(const float currentTime_sec)
  {
    _cozmoEngine.Update(currentTime_sec);
  }
  
  CozmoGameClientImpl::RunState CozmoGameClientImpl::GetRunState() const
  {
    return _runState;
  }
  
  void CozmoGameClientImpl::OnEventRaised( const IBaseStationEventInterface* event )
  {
    // TODO: Implement
  }
  
#pragma mark - CozmoGameClient Wrappers
  
  CozmoGameClient::CozmoGameClient()
  {
    _clientImpl = new CozmoGameClientImpl();
    
    // Set base class's impl pointer
    _impl = _clientImpl;
  }
  
  CozmoGameClient::~CozmoGameClient()
  {
    delete _impl;
  }
  
  bool CozmoGameClient::Init(const Json::Value& config)
  {
    return RESULT_OK == _clientImpl->Init(config);
  }
  
  void CozmoGameClient::Update(const float currentTime_sec)
  {
    _clientImpl->Update(currentTime_sec);
  }

  
  
} // namespace Cozmo
} // namespace Anki


