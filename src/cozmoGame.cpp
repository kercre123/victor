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
  : _runState(CozmoGame::STOPPED)
  {
    SetupSignalHandlers();
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
  
  
  Result CozmoGameImpl::StartEngine(const Json::Value& config)
  {
    Result lastResult = RESULT_FAIL;
    
    if(_runState == CozmoGameHost::STOPPED) {
      
      // Init the engine with the given configuration info:
      lastResult = GetCozmoEngine()->Init(config);
      
      if(lastResult == RESULT_OK) {
        _runState = CozmoGameHost::WAITING_FOR_UI_DEVICES;
      } else {
        PRINT_NAMED_ERROR("CozmoGameHostImpl.StartEngine",
                          "Failed to initialize the engine.\n");
      }
      
    } else {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.StartEngine",
                        "Engine already running, must start from stopped state.\n");
    }
    return lastResult;
  }
  
  void CozmoGameImpl::SetImageSendMode(RobotID_t forRobotID, Cozmo::ImageSendMode_t newMode)
  {
    _imageSendMode[forRobotID] = newMode;
  }
  
  bool CozmoGameImpl::GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime)
  {
    return GetCozmoEngine()->GetCurrentRobotImage(robotId, img, newerThanTime);
  }
  
  void CozmoGameImpl::ProcessDeviceImage(const Vision::Image& image)
  {
    _visionMarkersDetectedByDevice.clear();
    
    GetCozmoEngine()->ProcessDeviceImage(image);
  }

  const std::vector<Cozmo::MessageG2U_DeviceDetectedVisionMarker>& CozmoGameImpl::GetVisionMarkersDetectedByDevice() const
  {
    return _visionMarkersDetectedByDevice;
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
  
  Result CozmoGame::StartEngine(const Json::Value& config)
  {
    return _impl->StartEngine(config);
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
  
#pragma mark - CozmoGameHost Implementation
  
  
  CozmoGameHostImpl::CozmoGameHostImpl()
  : _uiAdvertisementService("UIAdvertisementService")
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
    
    _runState = CozmoGameHost::STOPPED;

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
    CozmoGameSignals::UiDeviceConnectedSignal().emit(whichDevice, success);
    return success;
  }

  bool CozmoGameHostImpl::ConnectToRobot(AdvertisingRobot whichRobot)
  {
    return _cozmoEngine.ConnectToRobot(whichRobot);
  }
  
  int CozmoGameHostImpl::GetNumRobots() const
  {
    return _cozmoEngine.GetNumRobots();
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
    
    // Update UI comms
    if(_uiComms.IsInitialized()) {
      _uiComms.Update();
    }
    
    // Handle UI messages
    _uiMsgHandler.ProcessMessages();
    
    switch(_runState)
    {
      case CozmoGameHost::STOPPED:
        // Nothing to do (just process UI messages above)
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
            CozmoGameSignals::UiDeviceAvailableSignal().emit(device);
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
        Result status = _cozmoEngine.Update(currentTime_sec);
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
        Result status = _cozmoEngine.Update(currentTime_sec);
        if (status != RESULT_OK) {
          PRINT_NAMED_WARNING("CozmoGameHostImpl.Update","Bad engine update: status = %d\n", status);
        }
        break;
      }
        
      default:
        PRINT_NAMED_ERROR("CozmoHostImpl.Update", "Reached unknown RunState %d.\n", _runState);
        
    }
    
  } // Update()
  
  bool CozmoGameHostImpl::SendRobotImage(RobotID_t robotID)
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
  
  
#pragma mark - CozmoGameHost Wrappers
  
  CozmoGameHost::CozmoGameHost()
  {
    _hostImpl = new CozmoGameHostImpl();
    // Set base class's impl pointer
    _impl = _hostImpl;
  }
  
  CozmoGameHost::~CozmoGameHost()
  {
    delete _hostImpl;
  }
  
  bool CozmoGameHost::Init(const Json::Value &config)
  {
    return _hostImpl->Init(config) == RESULT_OK;
  }
  
  void CozmoGameHost::ForceAddRobot(int              robotID,
                                    const char*      robotIP,
                                    bool             robotIsSimulated)
  {
    _hostImpl->ForceAddRobot(robotID, robotIP, robotIsSimulated);
  }
  
  int CozmoGameHost::GetNumRobots() const
  {
    return _hostImpl->GetNumRobots();
  }
  
  void CozmoGameHost::Update(const float currentTime_sec)
  {
    _hostImpl->Update(currentTime_sec);
  }

  

#pragma mark - CozmoGameClient Implementation
  
  CozmoGameClientImpl::CozmoGameClientImpl()
  : _runState(CozmoGame::STOPPED)
  {
    
  }
  
  CozmoGameClientImpl::~CozmoGameClientImpl()
  {

  }
  
  Result CozmoGameClientImpl::Init(const Json::Value& config)
  {
    // TODO: Any other initialization needed on the client?
    return _cozmoEngine.Init(config);
  }
  
  void CozmoGameClientImpl::Update(const float currentTime_sec)
  {
    // Don't tick the engine until it has been started
    if(_runState != CozmoGame::STOPPED) {
      _cozmoEngine.Update(currentTime_sec);
    }
  }
  
  CozmoGameClientImpl::RunState CozmoGameClientImpl::GetRunState() const
  {
    return _runState;
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


