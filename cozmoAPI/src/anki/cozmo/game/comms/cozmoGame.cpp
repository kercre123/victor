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
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/soundManager.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "util/logging/logging.h"
#include "anki/common/basestation/math/rect_impl.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/game/comms/uiMessageHandler.h"
#include "anki/cozmo/basestation/multiClientComms.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
#pragma mark - CozmoGame Implementation
    
  CozmoGameImpl::CozmoGameImpl(Util::Data::DataPlatform* dataPlatform)
  : _isEngineStarted(false)
  , _runState(CozmoGame::STOPPED)
  , _desiredNumRobots(1)
  , _uiMsgHandler(1)
  , _dataPlatform(dataPlatform)
  {
  }
  
  CozmoGameImpl::~CozmoGameImpl()
  {
    // Remove singletons
    SoundManager::removeInstance();
  }
  
  CozmoGameImpl::RunState CozmoGameImpl::GetRunState() const
  {
    return _runState;
  }
  
  Result CozmoGameImpl::Init(const Json::Value& config)
  {
    if(_isEngineStarted) {
      // We've already initialzed and started running before, so shut down the
      // already-running engine.
      PRINT_NAMED_INFO("CozmoGameImpl.Init",
                       "Re-initializing, so destroying existing cozmo engine and "
                       "waiting for another StartEngine command.");
      
      _isEngineStarted = false;
    }
    else {
      SetupSubscriptions();
    }
    
    Result lastResult = _uiMsgHandler.Init(config);
    if (RESULT_OK != lastResult)
    {
      PRINT_NAMED_ERROR("CozmoGameImpl.Init","Error initializing UIMessageHandler");
      return lastResult;
    }
    
    if(!config.isMember(AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR)) {
      PRINT_NAMED_WARNING("CozmoGameImpl.Init", "No NumRobotsToWaitFor defined in Json config, defaulting to 1.");
      _desiredNumRobots = 1;
    } else {
      _desiredNumRobots    = config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR].asInt();
    }

    _config = config;
    
    _runState = CozmoGame::STOPPED;
        
    return lastResult;
  }
  
  void CozmoGameImpl::SetupSubscriptions()
  {
    // Use a separate callback for StartEngine
    auto startEngineCallback = std::bind(&CozmoGameImpl::HandleStartEngine, this, std::placeholders::_1);
    _signalHandles.push_back(_uiMsgHandler.Subscribe(ExternalInterface::MessageGameToEngineTag::StartEngine, startEngineCallback));
  }
  
  Result CozmoGameImpl::StartEngine(Json::Value config)
  {
    Result lastResult = RESULT_FAIL;
    
    // Pass the game's advertising IP/port info along to the engine:
    config[AnkiUtil::kP_ADVERTISING_HOST_IP]    = _config[AnkiUtil::kP_ADVERTISING_HOST_IP];
    config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT] = _config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT];
    config[AnkiUtil::kP_UI_ADVERTISING_PORT]    = _config[AnkiUtil::kP_UI_ADVERTISING_PORT];
      
    PRINT_NAMED_INFO("CozmoGameImpl.StartEngine", "Creating HOST engine.");
    
    // Destroy(if it exists) and create CozmoEngine. We intentionally destory it first, then create the new one,
    // because bad things happen if there happen to be multiple CozmoEngines existing at the same time.
    _cozmoEngine.reset();
    _cozmoEngine.reset(new CozmoEngine(&_uiMsgHandler, _dataPlatform));
    _cozmoEngine->ListenForRobotConnections(true);
  
    // Init the engine with the given configuration info:
    lastResult = _cozmoEngine->Init(config);
    
    if(lastResult == RESULT_OK) {
      _isEngineStarted = true;
    } else {
      PRINT_NAMED_ERROR("CozmoGameImpl.StartEngine",
                        "Failed to initialize the engine.");
    }
    
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
  
  int CozmoGameImpl::GetNumRobots() const
  {
    if(_cozmoEngine) {
      return _cozmoEngine->GetNumRobots();
    } else {
      PRINT_NAMED_ERROR("CozmoGameImpl.GetNumRobots",
                        "Cannot request number of robots from game running as client.");
      return -1;
    }
  }

  Result CozmoGameImpl::Update(const float currentTime_sec)
  {
    Result lastResult = RESULT_OK;
    
    // Handle UI
    lastResult = _uiMsgHandler.Update();
    if (RESULT_OK != lastResult)
    {
      PRINT_NAMED_ERROR("CozmoGameImpl.Update", "Error updating UIMessageHandler");
      return lastResult;
    }
    
    if (_uiMsgHandler.HasDesiredNumUiDevices()) {
      if (_runState == CozmoGame::WAITING_FOR_UI_DEVICES)
      {
        _runState = CozmoGame::WAITING_FOR_ROBOTS;
      }
      lastResult = UpdateAsHost(currentTime_sec);
    }
    
    return lastResult;
    
  } // Update()
  
  Result CozmoGameImpl::UpdateAsHost(const float currentTime_sec)
  {
    Result lastResult = RESULT_OK;
    
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
        lastResult = _cozmoEngine->Update(currentTime_sec);
        if (lastResult != RESULT_OK) {
          PRINT_NAMED_WARNING("CozmoGameImpl.UpdateAsHost",
                              "Bad engine update: status = %d", lastResult);
        }
        
        // Tell the engine to keep listening for robots until it reports that
        // it has connections to enough
        if(_cozmoEngine->GetNumRobots() >= _desiredNumRobots) {
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
        lastResult = _cozmoEngine->Update(currentTime_sec);
        break;
      }
        
      default:
        PRINT_NAMED_ERROR("CozmoGameImpl.UpdateAsHost",
                          "Reached unknown RunState %d.", _runState);
        
    }
    
    return lastResult;
    
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


