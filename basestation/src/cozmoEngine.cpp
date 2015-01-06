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

#include "anki/cozmo/robot/cozmoConfig.h"

#include "anki/cozmo/basestation/basestation.h"
#include "anki/cozmo/basestation/cozmoEngine.h"
#include "anki/cozmo/basestation/multiClientComms.h"
#include "anki/cozmo/basestation/visionProcessingThread.h"

#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include "anki/messaging/basestation/advertisementService.h"

#include "anki/common/basestation/jsonTools.h"

#include "messageHandler.h"
#include "uiMessageHandler.h"

namespace Anki {
namespace Cozmo {
  
  // Helper macro to only free non-null pointers and then set them to nullptr:
#define FREE_HELPER(__ptr__) if(__ptr__ != nullptr) { delete __ptr__; __ptr__ = nullptr; }

  
#if 0
#pragma mark -
#pragma mark Base Class Implementations
#endif
  class CozmoEngineImpl
  {
  public:
    
    CozmoEngineImpl();
    virtual ~CozmoEngineImpl();
    
    virtual Result Init(const Json::Value& config);
    
    // Hook this up to whatever is ticking the game "heartbeat"
    Result Update(const BaseStationTime_t currTime_ns);
    
    // Provide an image from the device's camera for processing with the engine's
    // DeviceVisionProcessor
    void ProcessDeviceImage(const Vision::Image& image);
    
    using AdvertisingRobot    = CozmoEngine::AdvertisingRobot;
    using AdvertisingUiDevice = CozmoEngine::AdvertisingUiDevice;
    
    void GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots);
    void GetAdvertisingUiDevices(std::vector<AdvertisingUiDevice>& advertisingUiDevices);
    
    bool ConnectToRobot(AdvertisingRobot whichRobot);
    bool ConnectToUiDevice(AdvertisingUiDevice whichDevice);
    
    // TODO: Remove this in favor of it being handled via messages instead of direct API polling
    bool CheckDeviceVisionProcessingMailbox(MessageVisionMarker& msg);
    
  protected:
    
    // Derived classes must implement any per-tic updating they need done in this method.
    // Public Update() calls this automatically.
    virtual Result UpdateInternal(const BaseStationTime_t currTime_ns) = 0;
    
    bool _isInitialized;
    
    Json::Value               _config;
    
    VisionProcessingThread    _robotVisionThread;
    VisionProcessingThread    _deviceVisionThread;
    
    MultiClientComms          _robotComms;
    MultiClientComms          _uiComms;
    
    MessageHandler            _robotVisionMsgHandler;
    UiMessageHandler          _deviceVisionMsgHandler;
    
    std::vector<AdvertisingRobot>    _connectedRobots;
    std::vector<AdvertisingUiDevice> _connectedUiDevices;
    
  }; // class CozmoEngine

  
  CozmoEngineImpl::CozmoEngineImpl()
  : _isInitialized(false)
  {
    
  }
  
  Result CozmoEngineImpl::Init(const Json::Value& config)
  {
    if(_isInitialized) {
      PRINT_NAMED_INFO("CozmoEngineImpl.Init.ReInit", "Reinitializing already-initialized CozmoEngineImpl with new config.\n");
    }
    
    _config = config;
    
    if(!_config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP)) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "No AdvertisingHostIP defined in Json config.\n");
      return RESULT_FAIL;
    }
    
    if(!_config.isMember(AnkiUtil::kP_ROBOT_ADVERTISING_PORT)) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "No RobotAdvertisingPort defined in Json config.\n");
      return RESULT_FAIL;
    }
    
    if(!_config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "No UiAdvertisingPort defined in Json config.\n");
      return RESULT_FAIL;
    }
    
    Result lastResult = RESULT_OK;
    lastResult = _robotComms.Init(_config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString(),
                                  _config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT].asInt());
    if(lastResult != RESULT_OK) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "Failed to initialize RobotComms.\n");
      return lastResult;
    }
    
    lastResult = _uiComms.Init(_config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString(),
                                _config[AnkiUtil::kP_UI_ADVERTISING_PORT].asInt());
    if(lastResult != RESULT_OK) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "Failed to initialize GameComms.\n");
      return lastResult;
    }
    
    _isInitialized = true;
    
    return RESULT_OK;
  } // Init()
  
  void CozmoEngineImpl::GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots)
  {
    _robotComms.GetAdvertisingDeviceIDs(advertisingRobots);
  }
  
  void CozmoEngineImpl::GetAdvertisingUiDevices(std::vector<AdvertisingUiDevice>& advertisingUiDevices)
  {
    _uiComms.GetAdvertisingDeviceIDs(advertisingUiDevices);
  }
  
  bool CozmoEngineImpl::ConnectToRobot(AdvertisingRobot whichRobot)
  {
    const bool success = _robotComms.ConnectToDeviceByID(whichRobot);
    if(success) {
      _connectedRobots.push_back(whichRobot);
    }
    return success;
  }
  
  bool CozmoEngineImpl::ConnectToUiDevice(AdvertisingUiDevice whichDevice)
  {
    const bool success = _uiComms.ConnectToDeviceByID(whichDevice);
    if(success) {
      _connectedUiDevices.push_back(whichDevice);
    }

    return success;
  }
  
  Result CozmoEngineImpl::Update(const BaseStationTime_t currTime_ns)
  {
    if(!_isInitialized) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "Cannot update CozmoEngine before it is initialized.\n");
      return RESULT_FAIL;
    }
    
    // Update robot comms
    if(_robotComms.IsInitialized()) {
      _robotComms.Update();
    }
    
    // Update ui comms
    if(_uiComms.IsInitialized()) {
      _uiComms.Update();
    }
    
    // TODO: Handle images coming from a connected robot
    //_robotVisionMsgHandler.ProcessMessages();
    
    // TODO: Handle anything produced by device image processing
    /*
    MessageVisionMarker msg;
    while(_deviceVisionThread.CheckMailbox(msg)) {
      // Pass marker detections along to UI/game for use
      _deviceVisionMsgHandler.SendMessage(<#const UserDeviceID_t devID#>, msg);
    }
     */
    
    Result lastResult = UpdateInternal(currTime_ns);
    
    return lastResult;
  } // Update()
  
  void CozmoEngineImpl::ProcessDeviceImage(const Vision::Image &image)
  {
    // Process image within the detection rectangle with vision processing thread:
    static const Cozmo::MessageRobotState bogusState; // req'd by API, but not really necessary for marker detection
    _deviceVisionThread.SetNextImage(image, bogusState);
  }
  
  bool CozmoEngineImpl::CheckDeviceVisionProcessingMailbox(MessageVisionMarker& msg)
  {
    return _deviceVisionThread.CheckMailbox(msg);
  }
  
  //
  // Non-Impl Wrappers:
  //
  
  CozmoEngine::CozmoEngine()
  : _impl(nullptr)
  {
    
  }
  
  CozmoEngine::~CozmoEngine() {
    if(_impl != nullptr) {
      delete _impl;
      _impl = nullptr;
    }
  }
  
  Result CozmoEngine::Update(const Time currTime_sec)
  {
    return _impl->Update(SEC_TO_NANOS(currTime_sec));
  }
  
  void CozmoEngine::GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots) {
    _impl->GetAdvertisingRobots(advertisingRobots);
  }
  
  void CozmoEngine::GetAdvertisingUiDevices(std::vector<AdvertisingUiDevice>& advertisingUiDevices) {
    _impl->GetAdvertisingUiDevices(advertisingUiDevices);
  }
  
  bool CozmoEngine::ConnectToRobot(AdvertisingRobot whichRobot) {
    return _impl->ConnectToRobot(whichRobot);
  }
  
  bool CozmoEngine::ConnectToUiDevice(AdvertisingUiDevice whichDevice) {
    return _impl->ConnectToUiDevice(whichDevice);
  }
  
  void CozmoEngine::ProcessDeviceImage(const Vision::Image &image) {
    _impl->ProcessDeviceImage(image);
  }
  
  bool CozmoEngine::CheckDeviceVisionProcessingMailbox(MessageVisionMarker& msg) {
    return _impl->CheckDeviceVisionProcessingMailbox(msg);
  }
  
#if 0
#pragma mark -
#pragma mark Derived Host Class Implementations
#endif
  
  
  class CozmoEngineHostImpl : public CozmoEngineImpl
  {
  public:
    CozmoEngineHostImpl();
    ~CozmoEngineHostImpl();
 
    Result StartBasestation();
    
  protected:
    
    virtual Result UpdateInternal(const BaseStationTime_t currTime_ns) override;
    
    BasestationMain              _basestation;
    bool                         _isBasestationStarted;
    
    Comms::AdvertisementService  _robotAdvertisementService;
    Comms::AdvertisementService  _uiAdvertisementService;
    
  }; // class CozmoEngineHostImpl
  
  
  CozmoEngineHostImpl::CozmoEngineHostImpl()
  : _isBasestationStarted(false)
  , _robotAdvertisementService("RobotAdvertisementService")
  , _uiAdvertisementService("UIAdvertisementService")
  {

    _robotAdvertisementService.StartService(ROBOT_ADVERTISEMENT_REGISTRATION_PORT,
                                            ROBOT_ADVERTISING_PORT);
    
    _uiAdvertisementService.StartService(UI_ADVERTISEMENT_REGISTRATION_PORT,
                                         UI_ADVERTISING_PORT);

  }
  
  CozmoEngineHostImpl::~CozmoEngineHostImpl()
  {

  }
  
  Result CozmoEngineHostImpl::StartBasestation()
  {
    if(!_isBasestationStarted)
    {
      // Add connected_robots' IDs to config for basestation to use
      _config[AnkiUtil::kP_CONNECTED_ROBOTS].clear();
      for(auto robotID : _connectedRobots) {
        _config[AnkiUtil::kP_CONNECTED_ROBOTS].append(robotID);
      }
      
      BasestationStatus status = _basestation.Init(&_robotComms, &_uiComms, _config);
      if (status != BS_OK) {
        PRINT_NAMED_ERROR("Basestation.Init.Fail","Status = %d\n", status);
        return RESULT_FAIL;
      }
      
      _isBasestationStarted = true;
    }
    
    return RESULT_OK;
  }
  
  
  Result CozmoEngineHostImpl::UpdateInternal(const BaseStationTime_t currTime_ns)
  {
    
    if(_isBasestationStarted) {
      // TODO: Handle different basestation return codes
      _basestation.Update(currTime_ns);
    } else {
      // Tics advertisement services
      _robotAdvertisementService.Update();
      _uiAdvertisementService.Update();
    }
    
    return RESULT_OK;
  } // UpdateInternal()
  
  //
  // Non-Impl Wrappers:
  //
  CozmoEngineHost::CozmoEngineHost()
  {
    _impl = new CozmoEngineHostImpl();
    assert(_impl != nullptr);
  }
  
#if 0
#pragma mark -
#pragma mark Derived Client Class Implementations
#endif
  
  class CozmoEngineClientImpl : public CozmoEngineImpl
  {
  public:
    CozmoEngineClientImpl();
    ~CozmoEngineClientImpl();
    
  protected:
    
    virtual Result UpdateInternal(const BaseStationTime_t currTime_ns) override;
    
  }; // class CozmoEngineClientImpl

  
  
  Result CozmoEngineClientImpl::UpdateInternal(const BaseStationTime_t currTime_ns)
  {
    // TODO: Do client-specific update stuff here
    
    return RESULT_OK;
  } // UpdateInternal()
  
  
  //
  // Non-Impl Wrappers:
  //
  
  CozmoEngineClient::CozmoEngineClient()
  {
    _impl = new CozmoEngineClientImpl();
    assert(_impl != nullptr);
  }
  
  
  
#undef FREE_HELPER
  
} // namespace Cozmo
} // namespace Anki