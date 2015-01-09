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
#include "anki/cozmo/basestation/events/BaseStationEvent.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include "anki/messaging/basestation/advertisementService.h"

#include "anki/common/basestation/jsonTools.h"

#include "messageHandler.h"
#include "uiMessageHandler.h"

namespace Anki {
namespace Cozmo {
  
#if 0
#pragma mark -
#pragma mark Base Class Implementations
#endif
  class CozmoEngineImpl
  {
  public:
    
    CozmoEngineImpl();
    virtual ~CozmoEngineImpl();
    
    virtual Result Init(const Json::Value& config,
                        const Vision::CameraCalibration& deviceCamCalib);
    
    // Hook this up to whatever is ticking the game "heartbeat"
    Result Update(const BaseStationTime_t currTime_ns);
    
    // Provide an image from the device's camera for processing with the engine's
    // DeviceVisionProcessor
    void ProcessDeviceImage(const Vision::Image& image);
    
    bool WasLastDeviceImageProcessed();
    
    using AdvertisingRobot    = CozmoEngine::AdvertisingRobot;
    using AdvertisingUiDevice = CozmoEngine::AdvertisingUiDevice;
    
    void GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots);
    
    virtual bool ConnectToRobot(AdvertisingRobot whichRobot);
    
    // TODO: Remove this in favor of it being handled via messages instead of direct API polling
    bool CheckDeviceVisionProcessingMailbox(MessageVisionMarker& msg);
    
  protected:
    
    // Derived classes must implement any special initialization in this method,
    // which is called by Init().
    virtual Result InitInternal() = 0;
    
    // Derived classes must implement any per-tic updating they need done in this method.
    // Public Update() calls this automatically.
    virtual Result UpdateInternal(const BaseStationTime_t currTime_ns) = 0;
    
    bool _isInitialized;
    
    Json::Value               _config;
    
    VisionProcessingThread    _robotVisionThread;
    VisionProcessingThread    _deviceVisionThread;
    
    MultiClientComms          _robotComms;
    
    MessageHandler            _robotVisionMsgHandler;
    UiMessageHandler          _deviceVisionMsgHandler;
    
    std::vector<AdvertisingRobot>    _connectedRobots;
    std::vector<AdvertisingUiDevice> _connectedUiDevices;
    
  }; // class CozmoEngine

  
  CozmoEngineImpl::CozmoEngineImpl()
  : _isInitialized(false)
  {
    
  }
  
  CozmoEngineImpl::~CozmoEngineImpl()
  {
    
  }
  
  Result CozmoEngineImpl::Init(const Json::Value& config,
                               const Vision::CameraCalibration& deviceCamCalib)
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
    
    lastResult = InitInternal();
    if(lastResult != RESULT_OK) {
      PRINT_NAMED_ERROR("CozomEngine.Init", "Failed calling internal init.\n");
      return lastResult;
    }
    
    //_deviceVisionThread.SetCameraCalibration(deviceCamCalib);
    
    // TODO: Only start when needed?
    _deviceVisionThread.Start(deviceCamCalib);
    
    _isInitialized = true;
    
    return RESULT_OK;
  } // Init()
  
  void CozmoEngineImpl::GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots)
  {
    _robotComms.Update();
    _robotComms.GetAdvertisingDeviceIDs(advertisingRobots);
  }
  
  bool CozmoEngineImpl::ConnectToRobot(AdvertisingRobot whichRobot)
  {
    const bool success = _robotComms.ConnectToDeviceByID(whichRobot);
    if(success) {
      _connectedRobots.push_back(whichRobot);
    }
    BSE_RobotConnect::RaiseEvent(whichRobot, success);
    return success;
  }
  

  
  Result CozmoEngineImpl::Update(const BaseStationTime_t currTime_ns)
  {
    if(!_isInitialized) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "Cannot update CozmoEngine before it is initialized.\n");
      return RESULT_FAIL;
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
  
  bool CozmoEngineImpl::WasLastDeviceImageProcessed()
  {
    return _deviceVisionThread.WasLastImageProcessed();
  }
  
  bool CozmoEngineImpl::CheckDeviceVisionProcessingMailbox(MessageVisionMarker& msg)
  {
    return _deviceVisionThread.CheckMailbox(msg);
  }
  
#if 0
#pragma mark -
#pragma mark Derived Host Class Impl Wrappers
#endif
  
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
  
  Result CozmoEngine::Init(const Json::Value& config,
                           const Vision::CameraCalibration& deviceCamCalib) {
    return _impl->Init(config, deviceCamCalib);
  }
  
  Result CozmoEngine::Update(const Time currTime_sec) {
    return _impl->Update(SEC_TO_NANOS(currTime_sec));
  }
  
  void CozmoEngine::GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots) {
    _impl->GetAdvertisingRobots(advertisingRobots);
  }
  
  bool CozmoEngine::ConnectToRobot(AdvertisingRobot whichRobot) {
    return _impl->ConnectToRobot(whichRobot);
  }

  
  void CozmoEngine::ProcessDeviceImage(const Vision::Image &image) {
    _impl->ProcessDeviceImage(image);
  }
  
  bool CozmoEngine::WasLastDeviceImageProcessed() {
    return _impl->WasLastDeviceImageProcessed();
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
 
    Result StartBasestation();
    
    void GetAdvertisingUiDevices(std::vector<AdvertisingUiDevice>& advertisingUiDevices);
    bool ConnectToUiDevice(AdvertisingUiDevice whichDevice);

    void ForceAddRobot(AdvertisingRobot robotID,
                       const char*      robotIP,
                       bool             robotIsSimulated);
    
    // TODO: Remove once we don't have to specially handle forced adds
    virtual bool ConnectToRobot(AdvertisingRobot whichRobot) override;
    
    // TODO: Remove these in favor of it being handled via messages instead of direct API polling
    bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime);
    bool GetCurrentVisionMarkers(RobotID_t robotId, std::vector<Cozmo::BasestationMain::ObservedObjectBoundingBox>& observations);
    
  protected:
    
    virtual Result InitInternal() override;
    virtual Result UpdateInternal(const BaseStationTime_t currTime_ns) override;
    
    BasestationMain              _basestation;
    bool                         _isBasestationStarted;
    
    Comms::AdvertisementService  _robotAdvertisementService;
    Comms::AdvertisementService  _uiAdvertisementService;
    
    MultiClientComms             _uiComms;
    
    std::map<AdvertisingRobot, bool> _forceAddedRobots;
    
  }; // class CozmoEngineHostImpl
  
  
  CozmoEngineHostImpl::CozmoEngineHostImpl()
  : _isBasestationStarted(false)
  , _robotAdvertisementService("RobotAdvertisementService")
  , _uiAdvertisementService("UIAdvertisementService")
  {
    
    PRINT_NAMED_INFO("CozmoEngineHostImpl.Constructor",
                     "Starting RobotAdvertisementService, reg port %d, ad port %d\n",
                     ROBOT_ADVERTISEMENT_REGISTRATION_PORT, ROBOT_ADVERTISING_PORT);

    _robotAdvertisementService.StartService(ROBOT_ADVERTISEMENT_REGISTRATION_PORT,
                                            ROBOT_ADVERTISING_PORT);
    
    PRINT_NAMED_INFO("CozmoEngineHostImpl.Constructor",
                     "Starting UIAdvertisementService, reg port %d, ad port %d\n",
                     UI_ADVERTISEMENT_REGISTRATION_PORT, UI_ADVERTISING_PORT);
    
    _uiAdvertisementService.StartService(UI_ADVERTISEMENT_REGISTRATION_PORT,
                                         UI_ADVERTISING_PORT);

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
  
  Result CozmoEngineHostImpl::InitInternal()
  {
    if(!_config.isMember(AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR)) {
      PRINT_NAMED_WARNING("CozmoEngine.Init", "No NumRobotsToWaitFor defined in Json config, defaulting to 1.\n");
      _config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR] = 1;
    }
    
    if(!_config.isMember(AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR)) {
      PRINT_NAMED_WARNING("CozmoEngine.Init", "No NumUiDevicesToWaitFor defined in Json config, defaulting to 1.\n");
      _config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR] = 1;
    }
    
    Result lastResult = _uiComms.Init(_config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString(),
                                      _config[AnkiUtil::kP_UI_ADVERTISING_PORT].asInt());
    if(lastResult != RESULT_OK) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "Failed to initialize host uiComms.\n");
      return lastResult;
    }
    
    return RESULT_OK;
  }
  
  void CozmoEngineHostImpl::GetAdvertisingUiDevices(std::vector<AdvertisingUiDevice>& advertisingUiDevices)
  {
    _uiComms.Update();
    _uiComms.GetAdvertisingDeviceIDs(advertisingUiDevices);
  }
  
  bool CozmoEngineHostImpl::ConnectToUiDevice(AdvertisingUiDevice whichDevice)
  {
    const bool success = _uiComms.ConnectToDeviceByID(whichDevice);
    if(success) {
      _connectedUiDevices.push_back(whichDevice);
    }
    
    return success;
  }
  
  void CozmoEngineHostImpl::ForceAddRobot(AdvertisingRobot robotID,
                                          const char*      robotIP,
                                          bool             robotIsSimulated)
  {
    // Force add physical robot since it's not registering by itself yet.
    Anki::Comms::AdvertisementRegistrationMsg forcedRegistrationMsg;
    forcedRegistrationMsg.id = robotID;
    forcedRegistrationMsg.port = Anki::Cozmo::ROBOT_RADIO_BASE_PORT + (robotIsSimulated ? robotID : 0);
    forcedRegistrationMsg.protocol = USE_UDP_ROBOT_COMMS == 1 ? Anki::Comms::UDP : Anki::Comms::TCP;
    forcedRegistrationMsg.enableAdvertisement = 1;
    snprintf((char*)forcedRegistrationMsg.ip, sizeof(forcedRegistrationMsg.ip), "%s", robotIP);
    
    _robotAdvertisementService.ProcessRegistrationMsg(forcedRegistrationMsg);
    
    // Mark this robot as force-added so we can deregister it from the advertising
    // service manually once we connect to it.
    _forceAddedRobots[robotID] = true;
  }
  
  bool CozmoEngineHostImpl::ConnectToRobot(AdvertisingRobot whichRobot)
  {
    // Connection is the same as normal except that we have to remove forcefully-added
    // robots from the advertising service manually (if they could do this, they also
    // could have registered itself)
    bool result = CozmoEngineImpl::ConnectToRobot(whichRobot);
    if(_forceAddedRobots.count(whichRobot) > 0) {
      PRINT_NAMED_INFO("CozmoEngineHostImpl.ConnectToRobot",
                       "Manually deregistering force-added robot %d from advertising service.\n", whichRobot);
      _robotAdvertisementService.DeregisterAllAdvertisers();
    }
    return result;
  }
  
  Result CozmoEngineHostImpl::UpdateInternal(const BaseStationTime_t currTime_ns)
  {
    // Update ui comms
    if(_uiComms.IsInitialized()) {
      _uiComms.Update();
    }

    // Update robot comms
    if(_robotComms.IsInitialized()) {
      _robotComms.Update();
    }
    
    if(_isBasestationStarted) {
      // TODO: Handle different basestation return codes
      _basestation.Update(currTime_ns);
    } else if(_robotComms.GetNumConnectedDevices() >= _config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR].asInt() &&
              _uiComms.GetNumConnectedDevices()    >= _config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR].asInt()) {
      // We have connected to enough robots and devices:
      StartBasestation();
    } else {
      // Tics advertisement services
      _robotAdvertisementService.Update();
      _uiAdvertisementService.Update();
    }
    
    return RESULT_OK;
  } // UpdateInternal()
  
  bool CozmoEngineHostImpl::GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime)
  {
    return _basestation.GetCurrentRobotImage(robotId, img, newerThanTime);
  }
  
  bool CozmoEngineHostImpl::GetCurrentVisionMarkers(RobotID_t robotId, std::vector<Cozmo::BasestationMain::ObservedObjectBoundingBox>& observations)
  {
    return _basestation.GetCurrentVisionMarkers(robotId, observations);
  }
  
#if 0
#pragma mark -
#pragma mark Derived Host Class Impl Wrappers
#endif
  
  CozmoEngineHost::CozmoEngineHost()
  {
    _hostImpl = new CozmoEngineHostImpl();
    assert(_hostImpl != nullptr);
    _impl = _hostImpl;
  }
  
  void CozmoEngineHost::ForceAddRobot(AdvertisingRobot robotID,
                                      const char*      robotIP,
                                      bool             robotIsSimulated)
  {
    return _hostImpl->ForceAddRobot(robotID, robotIP, robotIsSimulated);
  }
  
  bool CozmoEngineHost::GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime)
  {
    return _hostImpl->GetCurrentRobotImage(robotId, img, newerThanTime);
  }
  
  bool CozmoEngineHost::GetCurrentVisionMarkers(RobotID_t robotId, std::vector<Cozmo::BasestationMain::ObservedObjectBoundingBox>& observations)
  {
    return _hostImpl->GetCurrentVisionMarkers(robotId, observations);
  }
  
  bool CozmoEngineHost::ConnectToRobot(AdvertisingRobot whichRobot)
  {
    return _hostImpl->ConnectToRobot(whichRobot);
  }
  
  void CozmoEngineHost::GetAdvertisingUiDevices(std::vector<AdvertisingUiDevice>& advertisingUiDevices) {
    _hostImpl->GetAdvertisingUiDevices(advertisingUiDevices);
  }
  
  bool CozmoEngineHost::ConnectToUiDevice(AdvertisingUiDevice whichDevice) {
    return _hostImpl->ConnectToUiDevice(whichDevice);
  }
  
#if 0
#pragma mark -
#pragma mark Derived Client Class Implementations
#endif
  
  class CozmoEngineClientImpl : public CozmoEngineImpl
  {
  public:
    CozmoEngineClientImpl();
    
  protected:
    
    virtual Result InitInternal() override;
    virtual Result UpdateInternal(const BaseStationTime_t currTime_ns) override;
    
  }; // class CozmoEngineClientImpl

  CozmoEngineClientImpl::CozmoEngineClientImpl()
  {
    
  }
  
  Result CozmoEngineClientImpl::InitInternal()
  {
    // TODO: Do client-specific init here
    
    return RESULT_OK;
  }
  
  Result CozmoEngineClientImpl::UpdateInternal(const BaseStationTime_t currTime_ns)
  {
    // TODO: Do client-specific update stuff here
    
    return RESULT_OK;
  } // UpdateInternal()
  
  
  
#if 0
#pragma mark -
#pragma mark Derived Client Class Impl Wrappers
#endif
  
  CozmoEngineClient::CozmoEngineClient()
  {
    _clientImpl = new CozmoEngineClientImpl();
    assert(_clientImpl != nullptr);
    _impl = _clientImpl;
  }
  
  bool CozmoEngineClient::GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime)
  {
    PRINT_NAMED_WARNING("CozmoEngineClient.GetCurrentRobotImage", "Cannot yet request an image from robot on client.\n");
    return false;
  }
  
  bool CozmoEngineClient::GetCurrentVisionMarkers(RobotID_t robotId, std::vector<Cozmo::BasestationMain::ObservedObjectBoundingBox>& observations)
  {
    PRINT_NAMED_WARNING("CozmoEngineClient.GetCurrentVisionMarkers", "Cannot yet request vision markers from robot on client.\n");
    return false;
  }
  
} // namespace Cozmo
} // namespace Anki