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

#include "cozmoGame_impl.h"
#include "anki/cozmo/basestation/cozmoEngine.h"
#include "anki/cozmo/basestation/cozmoEngineHost.h"
#include "anki/cozmo/basestation/cozmoEngineClient.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/soundManager.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "util/logging/logging.h"
#include "anki/common/basestation/math/rect_impl.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/cozmo/game/comms/uiMessageHandler.h"
#include "anki/cozmo/basestation/multiClientComms.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/gameStatusFlag.h"

namespace Anki {
namespace Cozmo {
  
  const float UI_PING_TIMEOUT_SEC = 5.0f;
  
#pragma mark - CozmoGame Implementation
    
  CozmoGameImpl::CozmoGameImpl(Util::Data::DataPlatform* dataPlatform)
  : _isHost(true)
  , _isEngineStarted(false)
  , _runState(CozmoGame::STOPPED)
  , _cozmoEngine(nullptr)
  , _desiredNumUiDevices(1)
  , _desiredNumRobots(1)
  , _uiAdvertisementService("UIAdvertisementService")
  , _uiMsgHandler(1)
  , _dataPlatform(dataPlatform)
  {
    _pingToUI.counter = 0;
    
    
    PRINT_NAMED_INFO("CozmoEngineHostImpl.Constructor",
                     "Starting UIAdvertisementService, reg port %d, ad port %d",
                     UI_ADVERTISEMENT_REGISTRATION_PORT, UI_ADVERTISING_PORT);
    
    _uiAdvertisementService.StartService(UI_ADVERTISEMENT_REGISTRATION_PORT,
                                         UI_ADVERTISING_PORT);
  }
  
  CozmoGameImpl::~CozmoGameImpl()
  {
    if(_cozmoEngine != nullptr) {
      delete _cozmoEngine;
      _cozmoEngine = nullptr;
    }
  
    VizManager::getInstance()->Disconnect();
    
    // Remove singletons
    SoundManager::removeInstance();
    VizManager::removeInstance();
  }
  
  CozmoGameImpl::RunState CozmoGameImpl::GetRunState() const
  {
    return _runState;
  }
  
  Result CozmoGameImpl::Init(const Json::Value& config)
  {
    _lastPingTimeFromUI_sec = -1.f;
    
    if(_isEngineStarted) {
      // We've already initialzed and started running before, so shut down the
      // already-running engine.
      PRINT_NAMED_INFO("CozmoGameImpl.Init",
                       "Re-initializing, so destroying existing cozmo engine and "
                       "waiting for another StartEngine command.");
      
      delete _cozmoEngine;
      _cozmoEngine = nullptr;
      _isEngineStarted = false;
    }
    else {
      SetupSubscriptions();
    }
    
    if(!config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP) ||
       !config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
      
      PRINT_NAMED_ERROR("CozmoGameImpl.Init", "Missing advertising hosdt / UI advertising port in Json config file.");
      return RESULT_FAIL;
    }
    
    Result lastResult = _uiComms.Init(config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString(),
                                      config[AnkiUtil::kP_UI_ADVERTISING_PORT].asInt());

    if(lastResult != RESULT_OK) {
      PRINT_NAMED_ERROR("CozmoGameImpl.Init", "Failed to initialize host uiComms.");
      return lastResult;
    }
    
    _uiMsgHandler.Init(&_uiComms);
    RegisterCallbacksU2G();
    
    if(!config.isMember(AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR)) {
      PRINT_NAMED_WARNING("CozmoGameImpl.Init", "No NumRobotsToWaitFor defined in Json config, defaulting to 1.");
      _desiredNumRobots = 1;
    } else {
      _desiredNumRobots    = config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR].asInt();
    }
    
    if(!config.isMember(AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR)) {
      PRINT_NAMED_WARNING("CozmoGameImpl.Init", "No NumUiDevicesToWaitFor defined in Json config, defaulting to 1.");
      _desiredNumUiDevices = 1;
    } else {
      _desiredNumUiDevices = config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR].asInt();
    }

    _config = config;
    
    _runState = CozmoGame::STOPPED;
        
    return lastResult;
  }
  
  void CozmoGameImpl::SetupSubscriptions()
  {
    // We'll use this callback for simple events we care about
    auto commonCallback = std::bind(&CozmoGameImpl::HandleEvents, this, std::placeholders::_1);
    
    // Subscribe to desired events
    _signalHandles.push_back(_uiMsgHandler.Subscribe(ExternalInterface::MessageGameToEngineTag::ConnectToUiDevice, commonCallback));
    _signalHandles.push_back(_uiMsgHandler.Subscribe(ExternalInterface::MessageGameToEngineTag::DisconnectFromUiDevice, commonCallback));
    
    // Use a separate callback for StartEngine
    auto startEngineCallback = std::bind(&CozmoGameImpl::HandleStartEngine, this, std::placeholders::_1);
    _signalHandles.push_back(_uiMsgHandler.Subscribe(ExternalInterface::MessageGameToEngineTag::StartEngine, startEngineCallback));
  }
  
  Result CozmoGameImpl::StartEngine(Json::Value config)
  {
    Result lastResult = RESULT_FAIL;
    
    if(!config.isMember("asHost")) {
      
      PRINT_NAMED_ERROR("CozmoGameImpl.StartEngine",
                        "Missing 'asHost' field in configuration.");
      return RESULT_FAIL;
    }
    
    // Pass the game's advertising IP/port info along to the engine:
    config[AnkiUtil::kP_ADVERTISING_HOST_IP]    = _config[AnkiUtil::kP_ADVERTISING_HOST_IP];
    config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT] = _config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT];
    config[AnkiUtil::kP_UI_ADVERTISING_PORT]    = _config[AnkiUtil::kP_UI_ADVERTISING_PORT];
    
    _isHost = config["asHost"].asBool();
    
    //if(_runState == CozmoGame::WAITING_FOR_UI_DEVICES) {
      
      if(_isEngineStarted) {
        delete _cozmoEngine;
      }
      
      if(_isHost) {
        PRINT_NAMED_INFO("CozmoGameImpl.StartEngine", "Creating HOST engine.");
        CozmoEngineHost* engineHost = new CozmoEngineHost(&_uiMsgHandler, _dataPlatform);
        engineHost->ListenForRobotConnections(true);
        _cozmoEngine = engineHost;
      } else {
        PRINT_NAMED_INFO("CozmoGameImpl.StartEngine", "Creating CLIENT engine.");
        _cozmoEngine = new CozmoEngineClient(&_uiMsgHandler, _dataPlatform);
      }
      
      // Init the engine with the given configuration info:
      lastResult = _cozmoEngine->Init(config);
      
      if(lastResult == RESULT_OK) {
        _isEngineStarted = true;
      } else {
        PRINT_NAMED_ERROR("CozmoGameImpl.StartEngine",
                          "Failed to initialize the engine.");
      }
    /*
    } else {
      PRINT_NAMED_ERROR("CozmoGameImpl.StartEngine",
                        "Engine already running, must start from stopped state.");
    }
     */
    
    _runState = CozmoGame::WAITING_FOR_UI_DEVICES;
    
    return lastResult;
  }

  bool CozmoGameImpl::GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime)
  {
    return _cozmoEngine->GetCurrentRobotImage(robotId, img, newerThanTime);
  }
  
  void CozmoGameImpl::ProcessDeviceImage(const Vision::Image& image)
  {
    _visionMarkersDetectedByDevice.clear();
    
    _cozmoEngine->ProcessDeviceImage(image);
  }

  const std::vector<ExternalInterface::DeviceDetectedVisionMarker>& CozmoGameImpl::GetVisionMarkersDetectedByDevice() const
  {
    return _visionMarkersDetectedByDevice;
  }
  
  bool CozmoGameImpl::ConnectToUiDevice(AdvertisingUiDevice whichDevice)
  {
    const bool success = _uiComms.ConnectToDeviceByID(whichDevice);
    if(success) {
      _connectedUiDevices.push_back(whichDevice);
    }
    _uiMsgHandler.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::UiDeviceConnected(whichDevice, success)));
    return success;
  }
  
  int CozmoGameImpl::GetNumRobots() const
  {
    if(_isHost) {
      CozmoEngineHost* cozmoEngineHost = reinterpret_cast<CozmoEngineHost*>(_cozmoEngine);
      assert(cozmoEngineHost != nullptr);
      return cozmoEngineHost->GetNumRobots();
    } else {
      PRINT_NAMED_ERROR("CozmoGameImpl.GetNumRobots",
                        "Cannot request number of robots from game running as client.");
      return -1;
    }
  }

  Result CozmoGameImpl::Update(const float currentTime_sec)
  {
    Result lastResult = RESULT_OK;
  
    if(_lastPingTimeFromUI_sec > 0.f) {
      const f32 timeSinceLastUiPing = currentTime_sec - _lastPingTimeFromUI_sec;
      
      if(timeSinceLastUiPing > UI_PING_TIMEOUT_SEC) {
        /*
        PRINT_NAMED_ERROR("CozmoGameImpl.Update",
                          "Lost connection to UI (no ping in %.2f seconds). Resetting.",
                          timeSinceLastUiPing);
        
        Init(_config);
        return lastResult;
         */
        
        PRINT_NAMED_WARNING("CozmoGameImpl.Update",
                            "No ping from UI in %.2f seconds, but NOT ressetting.",
                            timeSinceLastUiPing);
        _lastPingTimeFromUI_sec = -1.f;
      }
    }
    
    // Update UI comms
    if(_uiComms.IsInitialized()) {
      _uiComms.Update();
      
      if(_uiComms.GetNumConnectedDevices() > 0) {
        // Ping the UI to let them know we're still here
        ExternalInterface::MessageEngineToGame message;
        message.Set_Ping(_pingToUI);
        _uiMsgHandler.Broadcast(message);
        ++_pingToUI.counter;
      }
    }
    
    // Handle UI messages
    _uiMsgHandler.ProcessMessages();
    
    if(!_isEngineStarted || _runState == CozmoGame::WAITING_FOR_UI_DEVICES) {
      // If we are still waiting on the engine to start, or even if it is started
      // but we have not connected to enough UI devices, then keep ticking the
      // UI advertisement service and connect to anything advertising until we
      // have enough devices and can switch to looking for robots.
      
      _uiAdvertisementService.Update();
      
      // TODO: Do we want to do this all the time in case UI devices want to join later?
      // Notify the UI that there are advertising devices
      std::vector<int> advertisingUiDevices;
      _uiComms.GetAdvertisingDeviceIDs(advertisingUiDevices);
      for(auto & device : advertisingUiDevices) {
        if(device == _uiMsgHandler.GetHostUiDeviceID()) {
          // Force connection to first (local) UI device
          if(true == ConnectToUiDevice(device)) {
            PRINT_NAMED_INFO("CozmoGameImpl.Update",
                             "Automatically connected to local UI device %d!", device);
          }
        } else {
          _uiMsgHandler.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::UiDeviceAvailable(device)));
        }
      }
      
      if(_uiComms.GetNumConnectedDevices() >= _desiredNumUiDevices) {
        PRINT_NAMED_INFO("CozmoGameImpl.UpdateAsHost",
                         "Enough UI devices connected (%d), will wait for %d robots.",
                         _desiredNumUiDevices, _desiredNumRobots);
        _runState = CozmoGame::WAITING_FOR_ROBOTS;
      }
      
    } else {
      if(_isHost) {
        lastResult = UpdateAsHost(currentTime_sec);
      } else {
        lastResult = UpdateAsClient(currentTime_sec);
      }
    }
    
    return lastResult;
    
  } // Update()
  
  Result CozmoGameImpl::UpdateAsHost(const float currentTime_sec)
  {
    Result lastResult = RESULT_OK;
    
    CozmoEngineHost* cozmoEngineHost = reinterpret_cast<CozmoEngineHost*>(_cozmoEngine);
    assert(cozmoEngineHost != nullptr);
    
    switch(_runState)
    {
      case CozmoGame::STOPPED:
        // Nothing to do
        break;
        
      case CozmoGame::WAITING_FOR_UI_DEVICES:
      {
        /*
        _uiAdvertisementService.Update();
        
        // TODO: Do we want to do this all the time in case UI devices want to join later?
        // Notify the UI that there are advertising devices
        std::vector<int> advertisingUiDevices;
        _uiComms.GetAdvertisingDeviceIDs(advertisingUiDevices);
        for(auto & device : advertisingUiDevices) {
          if(device == _hostUiDeviceID) {
            // Force connection to first (local) UI device
            if(true == ConnectToUiDevice(device)) {
              PRINT_NAMED_INFO("CozmoGameImpl.Update",
                               "Automatically connected to local UI device %d!", device);
            }
          } else {
            CozmoGameSignals::UiDeviceAvailableSignal().emit(device);
          }
        }
        
        if(_uiComms.GetNumConnectedDevices() >= _desiredNumUiDevices) {
          PRINT_NAMED_INFO("CozmoGameImpl.UpdateAsHost",
                           "Enough UI devices connected (%d), will wait for %d robots.",
                           _desiredNumUiDevices, _desiredNumRobots);
          cozmoEngineHost->ListenForRobotConnections(true);
          _runState = CozmoGame::WAITING_FOR_ROBOTS;
        }
         */
        break;
      }
        
      case CozmoGame::WAITING_FOR_ROBOTS:
      {
        lastResult = cozmoEngineHost->Update(currentTime_sec);
        if (lastResult != RESULT_OK) {
          PRINT_NAMED_WARNING("CozmoGameImpl.UpdateAsHost",
                              "Bad engine update: status = %d", lastResult);
        }
        
        // Tell the engine to keep listening for robots until it reports that
        // it has connections to enough
        if(cozmoEngineHost->GetNumRobots() >= _desiredNumRobots) {
          PRINT_NAMED_INFO("CozmoGameImpl.UpdateAsHost",
                           "Enough robots connected (%d), will run engine.",
                           _desiredNumRobots);
          // TODO: We could keep listening for others to join mid-game...
          //cozmoEngineHost->ListenForRobotConnections(false);
          _runState = CozmoGame::ENGINE_RUNNING;
        }
        break;
      }
        
      case CozmoGame::ENGINE_RUNNING:
      {
        lastResult = cozmoEngineHost->Update(currentTime_sec);
        
        if (lastResult != RESULT_OK) {
          PRINT_NAMED_WARNING("CozmoGameImpl.UpdateAsHost",
                              "Bad engine update: status = %d", lastResult);
        } else {
          // Send out robot state information for each robot:
          auto robotIDs = cozmoEngineHost->GetRobotIDList();
          for(auto & robotID : robotIDs) {
            Robot* robot = cozmoEngineHost->GetRobotByID(robotID);
            if(robot == nullptr) {
              PRINT_NAMED_ERROR("CozmoGameImpl.UpdateAsHost", "Null robot returned for ID=%d!", robotID);
              lastResult = RESULT_FAIL;
            } else {
              if(robot->HasReceivedRobotState()) {
                ExternalInterface::RobotState msg;
                
                msg.robotID = robotID;
                
                msg.pose_x = robot->GetPose().GetTranslation().x();
                msg.pose_y = robot->GetPose().GetTranslation().y();
                msg.pose_z = robot->GetPose().GetTranslation().z();
                
                msg.poseAngle_rad = robot->GetPose().GetRotationAngle<'Z'>().ToFloat();
                const UnitQuaternion<float>& q = robot->GetPose().GetRotation().GetQuaternion();
                msg.pose_quaternion0 = q.w();
                msg.pose_quaternion1 = q.x();
                msg.pose_quaternion2 = q.y();
                msg.pose_quaternion3 = q.z();
                
                msg.leftWheelSpeed_mmps  = robot->GetLeftWheelSpeed();
                msg.rightWheelSpeed_mmps = robot->GetRigthWheelSpeed();
                
                msg.headAngle_rad = robot->GetHeadAngle();
                msg.liftHeight_mm = robot->GetLiftHeight();
                
                msg.status = 0;
                if(robot->IsMoving())           { msg.status |= (uint32_t)RobotStatusFlag::IS_MOVING; }
                if(robot->IsPickingOrPlacing()) { msg.status |= (uint32_t)RobotStatusFlag::IS_PICKING_OR_PLACING; }
                if(robot->IsPickedUp())         { msg.status |= (uint32_t)RobotStatusFlag::IS_PICKED_UP; }
                if(robot->IsAnimating())        { msg.status |= (uint32_t)RobotStatusFlag::IS_ANIMATING; }
                if(robot->IsIdleAnimating())    { msg.status |= (uint32_t)RobotStatusFlag::IS_ANIMATING_IDLE; }
                if(robot->IsCarryingObject())   {
                  msg.status |= (uint32_t)RobotStatusFlag::IS_CARRYING_BLOCK;
                  msg.carryingObjectID = robot->GetCarryingObject();
                  msg.carryingObjectOnTopID = robot->GetCarryingObjectOnTop();
                } else {
                  msg.carryingObjectID = -1;
                }
                if(!robot->GetActionList().IsEmpty()) {
                  msg.status |= (uint32_t)RobotStatusFlag::IS_PATHING;
                }
                
                msg.gameStatus = 0;
                if (robot->IsLocalized() && !robot->IsPickedUp()) { msg.gameStatus |= (uint8_t)GameStatusFlag::IsLocalized; }
                
                msg.headTrackingObjectID = robot->GetTrackToObject();
                
                // TODO: Add proximity sensor data to state message
                
                msg.batteryVoltage = robot->GetBatteryVoltage();
                
                ExternalInterface::MessageEngineToGame message;
                message.Set_RobotState(msg);
                _uiMsgHandler.Broadcast(message);
              } else {
                PRINT_NAMED_WARNING("CozmoGameImpl.UpdateAsHost",
                                    "Not sending robot %d state (none available).",
                                    robotID);
              }
            }
          }
        }
        break;
      }
        
      default:
        PRINT_NAMED_ERROR("CozmoGameImpl.UpdateAsHost",
                          "Reached unknown RunState %d.", _runState);
        
    }
    
    return lastResult;
    
  }
  
  Result CozmoGameImpl::UpdateAsClient(const float currentTime_sec)
  {
    Result lastResult = RESULT_OK;
    
    // Don't tick the engine until it has been started
    if(_runState != CozmoGame::STOPPED) {
      lastResult = _cozmoEngine->Update(currentTime_sec);
    }
    
    return lastResult;
  } // UpdateAsClient()
  
  void CozmoGameImpl::HandleEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
  {
    switch (event.GetData().GetTag())
    {
      case ExternalInterface::MessageGameToEngineTag::ConnectToUiDevice:
      {
        const ExternalInterface::ConnectToUiDevice& msg = event.GetData().Get_ConnectToUiDevice();
        const bool success = ConnectToUiDevice(msg.deviceID);
        if(success) {
          PRINT_NAMED_INFO("CozmoGameImpl.HandleEvents", "Connected to UI device %d!", msg.deviceID);
        } else {
          PRINT_NAMED_ERROR("CozmoGameImpl.HandleEvents", "Failed to connect to UI device %d!", msg.deviceID);
        }
        break;
      }
      case ExternalInterface::MessageGameToEngineTag::DisconnectFromUiDevice:
      {
        const ExternalInterface::DisconnectFromUiDevice& msg = event.GetData().Get_DisconnectFromUiDevice();
        _uiComms.DisconnectDeviceByID(msg.deviceID);
        PRINT_NAMED_INFO("CozmoGameImpl.ProcessMessage", "Disconnected from UI device %d!", msg.deviceID);
        
        if(_uiComms.GetNumConnectedDevices() == 0) {
          PRINT_NAMED_INFO("CozmoGameImpl.ProcessMessage",
                           "Last UI device just disconnected: forcing re-initialization.");
          Init(_config);
        }
        break;
      }
      default:
      {
        PRINT_STREAM_ERROR("CozmoGameImpl.HandleEvents",
                           "Subscribed to unhandled event of type "
                           << ExternalInterface::MessageGameToEngineTagToString(event.GetData().GetTag()) << "!");
      }
    }
  }
  
  void CozmoGameImpl::HandleStartEngine(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
  {
    const ExternalInterface::StartEngine& msg = event.GetData().Get_StartEngine();
    if (_isEngineStarted) {
      PRINT_NAMED_INFO("CozmoGameImpl.Process_StartEngine.AlreadyStarted", "");
      return;
    }
    
    // Populate the Json configuration from the message members:
    Json::Value config;
    
    // Viz Host IP:
    char ip[16];
    assert(msg.vizHostIP.size() <= 16);
    std::copy(msg.vizHostIP.begin(), msg.vizHostIP.end(), ip);
    config[AnkiUtil::kP_VIZ_HOST_IP] = ip;
    
    config[AnkiUtil::kP_AS_HOST] = msg.asHost;
    
    // Start the engine with that configuration
    StartEngine(config);
  }

  
#pragma mark - CozmoGame Wrappers
  
  CozmoGame::CozmoGame(Util::Data::DataPlatform* dataPlatform)
  : _impl(nullptr)
  {
    _impl = new CozmoGameImpl(dataPlatform);
  }
  
  CozmoGame::~CozmoGame()
  {
    delete _impl;
  }
  
  Result CozmoGame::Init(const Json::Value &config)
  {
    return _impl->Init(config);
  }
  
  Result CozmoGame::StartEngine(Json::Value config)
  {
    return _impl->StartEngine(config);
  }
  
  Result CozmoGame::Update(const float currentTime_sec)
  {
    return _impl->Update(currentTime_sec);
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
  
  CozmoGame::RunState CozmoGame::GetRunState() const
  {
    assert(_impl != nullptr);
    return _impl->GetRunState();
  }
  
  const std::vector<Cozmo::ExternalInterface::DeviceDetectedVisionMarker>& CozmoGame::GetVisionMarkersDetectedByDevice() const
  {
    return _impl->GetVisionMarkersDetectedByDevice();
  }

  
} // namespace Cozmo
} // namespace Anki


