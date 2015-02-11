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

#include "anki/common/basestation/utils/logging/logging.h"
#include "anki/common/basestation/math/rect_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include "anki/cozmo/game/comms/uiMessageHandler.h"
#include "anki/cozmo/basestation/multiClientComms.h"
#include "anki/cozmo/basestation/signals/cozmoEngineSignals.h"
#include "anki/cozmo/game/signals/cozmoGameSignals.h"

namespace Anki {
namespace Cozmo {
  
#pragma mark - CozmoGame Implementation
    
  CozmoGameImpl::CozmoGameImpl()
  : _isHost(true)
  , _isEngineStarted(false)
  , _runState(CozmoGame::STOPPED)
  , _cozmoEngine(nullptr)
  , _desiredNumUiDevices(1)
  , _desiredNumRobots(1)
  , _uiAdvertisementService("UIAdvertisementService")
  , _hostUiDeviceID(1)
  {
    SetupSignalHandlers();
    
    RegisterCallbacksU2G();
    
    
    PRINT_NAMED_INFO("CozmoEngineHostImpl.Constructor",
                     "Starting UIAdvertisementService, reg port %d, ad port %d\n",
                     UI_ADVERTISEMENT_REGISTRATION_PORT, UI_ADVERTISING_PORT);
    
    _uiAdvertisementService.StartService(UI_ADVERTISEMENT_REGISTRATION_PORT,
                                         UI_ADVERTISING_PORT);
  }
  
  CozmoGameImpl::~CozmoGameImpl()
  {
    delete _cozmoEngine;
    
    SoundManager::removeInstance();
  }
  
  CozmoGameImpl::RunState CozmoGameImpl::GetRunState() const
  {
    return _runState;
  }
  
  Result CozmoGameImpl::Init(const Json::Value& config)
  {
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
    
    _uiMsgHandler.Init(&_uiComms);
    
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

    _config = config;
    
    _runState = CozmoGame::STOPPED;
    
    // Let the UI know this cozmoGame object is here:
    SendAvailabilityMessage();
    
    return lastResult;
  }
  
  Result CozmoGameImpl::StartEngine(Json::Value config)
  {
    Result lastResult = RESULT_FAIL;
    
    if(!config.isMember("asHost")) {
      
      PRINT_NAMED_ERROR("CozmoGameImpl.StartEngine",
                        "Missing 'asHost' field in configuration.\n");
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
        PRINT_NAMED_INFO("CozmoGameImpl.StartEngine", "Creating HOST engine.\n");
        CozmoEngineHost* engineHost = new CozmoEngineHost();
        engineHost->ListenForRobotConnections(true);
        _cozmoEngine = engineHost;
      } else {
        PRINT_NAMED_INFO("CozmoGameImpl.StartEngine", "Creating CLIENT engine.\n");
        _cozmoEngine = new CozmoEngineClient();
      }
      
      // Init the engine with the given configuration info:
      lastResult = _cozmoEngine->Init(config);
      
      if(lastResult == RESULT_OK) {
        _isEngineStarted = true;
      } else {
        PRINT_NAMED_ERROR("CozmoGameHostImpl.StartEngine",
                          "Failed to initialize the engine.\n");
      }
    /*
    } else {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.StartEngine",
                        "Engine already running, must start from stopped state.\n");
    }
     */
    return lastResult;
  }
  
  void CozmoGameImpl::SetImageSendMode(RobotID_t forRobotID, Cozmo::ImageSendMode_t newMode)
  {
    _imageSendMode[forRobotID] = newMode;
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

  const std::vector<Cozmo::MessageG2U_DeviceDetectedVisionMarker>& CozmoGameImpl::GetVisionMarkersDetectedByDevice() const
  {
    return _visionMarkersDetectedByDevice;
  }
  
  void CozmoGameImpl::ForceAddRobot(int              robotID,
                                    const char*      robotIP,
                                    bool             robotIsSimulated)
  {
    if(_isHost) {
      CozmoEngineHost* cozmoEngineHost = reinterpret_cast<CozmoEngineHost*>(_cozmoEngine);
      assert(cozmoEngineHost != nullptr);
      cozmoEngineHost->ForceAddRobot(robotID, robotIP, robotIsSimulated);
    } else {
      PRINT_NAMED_ERROR("CozmoGameImpl.ForceAddRobot",
                        "Cannot force-add a robot to game running as client.\n");
    }
  }
  
  void CozmoGameImpl::SendAvailabilityMessage()
  {
    MessageG2U_EngineAvailable msg;
    msg.isHost = 1;
    msg.runningOnDeviceID = _hostUiDeviceID;
    _uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
  }

  bool CozmoGameImpl::ConnectToUiDevice(AdvertisingUiDevice whichDevice)
  {
    const bool success = _uiComms.ConnectToDeviceByID(whichDevice);
    if(success) {
      _connectedUiDevices.push_back(whichDevice);
    }
    CozmoGameSignals::UiDeviceConnectedSignal().emit(whichDevice, success);
    return success;
  }

  bool CozmoGameImpl::ConnectToRobot(AdvertisingRobot whichRobot)
  {
    return _cozmoEngine->ConnectToRobot(whichRobot);
  }
  
  int CozmoGameImpl::GetNumRobots() const
  {
    if(_isHost) {
      CozmoEngineHost* cozmoEngineHost = reinterpret_cast<CozmoEngineHost*>(_cozmoEngine);
      assert(cozmoEngineHost != nullptr);
      return cozmoEngineHost->GetNumRobots();
    } else {
      PRINT_NAMED_ERROR("CozmoGameImpl.GetNumRobots",
                        "Cannot request number of robots from game running as client.\n");
      return -1;
    }
  }

  Result CozmoGameImpl::Update(const float currentTime_sec)
  {
    Result lastResult = RESULT_OK;
    
    // Update UI comms
    if(_uiComms.IsInitialized()) {
      _uiComms.Update();
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
        if(device == _hostUiDeviceID) {
          // Force connection to first (local) UI device
          if(true == ConnectToUiDevice(device)) {
            PRINT_NAMED_INFO("CozmoGameHostImpl.Update",
                             "Automatically connected to local UI device %d!\n", device);
          }
        } else {
          CozmoGameSignals::UiDeviceAvailableSignal().emit(device);
        }
      }
      
      if(_uiComms.GetNumConnectedDevices() >= _desiredNumUiDevices) {
        PRINT_NAMED_INFO("CozmoGameImpl.UpdateAsHost",
                         "Enough UI devices connected (%d), will wait for %d robots.\n",
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
              PRINT_NAMED_INFO("CozmoGameHostImpl.Update",
                               "Automatically connected to local UI device %d!\n", device);
            }
          } else {
            CozmoGameSignals::UiDeviceAvailableSignal().emit(device);
          }
        }
        
        if(_uiComms.GetNumConnectedDevices() >= _desiredNumUiDevices) {
          PRINT_NAMED_INFO("CozmoGameImpl.UpdateAsHost",
                           "Enough UI devices connected (%d), will wait for %d robots.\n",
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
                              "Bad engine update: status = %d\n", lastResult);
        }
        
        // Tell the engine to keep listening for robots until it reports that
        // it has connections to enough
        if(cozmoEngineHost->GetNumRobots() >= _desiredNumRobots) {
          PRINT_NAMED_INFO("CozmoGameImpl.UpdateAsHost",
                           "Enough robots connected (%d), will run engine.\n",
                           _desiredNumRobots);
          // TODO: We could keep listening for others to join mid-game...
          cozmoEngineHost->ListenForRobotConnections(false);
          _runState = CozmoGame::ENGINE_RUNNING;
        }
        break;
      }
        
      case CozmoGame::ENGINE_RUNNING:
      {
        lastResult = _cozmoEngine->Update(currentTime_sec);
        if (lastResult != RESULT_OK) {
          PRINT_NAMED_WARNING("CozmoGameImpl.UpdateAsHost",
                              "Bad engine update: status = %d\n", lastResult);
        }
        break;
      }
        
      default:
        PRINT_NAMED_ERROR("CozmoGameImpl.UpdateAsHost",
                          "Reached unknown RunState %d.\n", _runState);
        
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
  
  bool CozmoGameImpl::SendRobotImage(RobotID_t robotID)
  {
    // Get the image from the robot
    Vision::Image img;
    // TODO: fill in the timestamp?
    const bool gotImage = GetCurrentRobotImage(robotID, img, 0);
    
    if(gotImage) {
      
      static u32 imgID = 0;
      
      // Downsample and split into image chunk message
      const s32 ncols = img.GetNumCols();
      const s32 nrows = img.GetNumRows();
      
      const u32 numTotalBytes = nrows*ncols;
      
      MessageG2U_ImageChunk m;
      // TODO: pass this in so it corresponds to actual frame capture time instead of send time
      m.frameTimeStamp = img.GetTimestamp();
      m.nrows = nrows;
      m.ncols = ncols;
      m.imageId = ++imgID;
      m.chunkId = 0;
      m.chunkSize = G2U_IMAGE_CHUNK_SIZE;
      m.imageChunkCount = ceilf((f32)numTotalBytes / G2U_IMAGE_CHUNK_SIZE);
      m.imageEncoding = 0;
      
      u32 totalByteCnt = 0;
      u32 chunkByteCnt = 0;
      
      //PRINT("Downsample: from %d x %d  to  %d x %d\n", img.get_size(1), img.get_size(0), xRes, yRes);
      
      for(s32 i=0; i<nrows; ++i) {
        
        const u8* img_i = img.GetRow(i);
        
        for(s32 j=0; j<ncols; ++j) {
          m.data[chunkByteCnt] = img_i[j];
          ++chunkByteCnt;
          ++totalByteCnt;
          
          if(chunkByteCnt == G2U_IMAGE_CHUNK_SIZE) {
            // Filled this chunk
            _uiMsgHandler.SendMessage(_hostUiDeviceID, m);
            ++m.chunkId;
            chunkByteCnt = 0;
          } else if(totalByteCnt == numTotalBytes) {
            // This is the last chunk!
            m.chunkSize = chunkByteCnt;
            _uiMsgHandler.SendMessage(_hostUiDeviceID, m);
          }
        } // for each col
      } // for each row
      
    } // if gotImage
    
    return gotImage;
    
  } // SendImage()
  
  
#pragma mark - CozmoGame Wrappers
  
  CozmoGame::CozmoGame()
  : _impl(nullptr)
  {
    _impl = new CozmoGameImpl();
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
  
  const std::vector<Cozmo::MessageG2U_DeviceDetectedVisionMarker>& CozmoGame::GetVisionMarkersDetectedByDevice() const
  {
    return _impl->GetVisionMarkersDetectedByDevice();
  }

  
} // namespace Cozmo
} // namespace Anki


