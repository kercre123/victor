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

#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/cozmo/basestation/basestation.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoEngine.h"
#include "anki/cozmo/basestation/multiClientComms.h" // TODO: Remove?
#include "anki/cozmo/basestation/visionProcessingThread.h"
#include "anki/cozmo/basestation/signals/cozmoEngineSignals.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include "anki/messaging/basestation/advertisementService.h"

#include "anki/common/basestation/jsonTools.h"


#include "robotMessageHandler.h"

#define ASYNCHRONOUS_DEVICE_VISION 0

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
    
    virtual Result Init(const Json::Value& config);
    
    // Hook this up to whatever is ticking the game "heartbeat"
    Result Update(const BaseStationTime_t currTime_ns);
    
    // Provide an image from the device's camera for processing with the engine's
    // DeviceVisionProcessor
    void ProcessDeviceImage(const Vision::Image& image);
    
    using AdvertisingRobot    = CozmoEngine::AdvertisingRobot;
    //using AdvertisingUiDevice = CozmoEngine::AdvertisingUiDevice;
    
    //void GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots);
    
    virtual bool ConnectToRobot(AdvertisingRobot whichRobot);
    
    
  protected:
  
    // Derived classes must implement any special initialization in this method,
    // which is called by Init().
    virtual Result InitInternal() = 0;
    
    // Derived classes must implement any per-tic updating they need done in this method.
    // Public Update() calls this automatically.
    virtual Result UpdateInternal(const BaseStationTime_t currTime_ns) = 0;
    
    bool                      _isInitialized;
    
    int                       _engineID;
    
    Json::Value               _config;
    
    MultiClientComms          _robotComms;
    
    // Each engine can potetnailly talk to multiple physical robots.
    // Package up the stuff req'd to deal with one robot and store a map
    // of them keyed by robot ID.
    struct RobotContainer {
      VisionProcessingThread    visionThread;
      RobotMessageHandler       visionMsgHandler;
    };
    std::map<AdvertisingRobot, RobotContainer> _connectedRobots;
    
    VisionProcessingThread    _deviceVisionThread;
    
  }; // class CozmoEngineImpl

  
  CozmoEngineImpl::CozmoEngineImpl()
  : _isInitialized(false)
  {

  }
  
  CozmoEngineImpl::~CozmoEngineImpl()
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
    
    Vision::CameraCalibration deviceCamCalib;
    if(!_config.isMember(AnkiUtil::kP_DEVICE_CAMERA_CALIBRATION)) {
      PRINT_NAMED_WARNING("CozmoEngine.Init",
                          "No DeviceCameraCalibration defined in Json config. Using bogus settings.\n");
    } else {
      deviceCamCalib.Set(_config[AnkiUtil::kP_DEVICE_CAMERA_CALIBRATION]);
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
    
#   if ASYNCHRONOUS_DEVICE_VISION
    // TODO: Only start when needed?
    _deviceVisionThread.Start(deviceCamCalib);
#   else 
    _deviceVisionThread.SetCameraCalibration(deviceCamCalib);
#   endif
    
    _isInitialized = true;
    
    return RESULT_OK;
  } // Init()
  
  /*
  void CozmoEngineImpl::GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots)
  {
    _robotComms.Update();
    _robotComms.GetAdvertisingDeviceIDs(advertisingRobots);
  }
   */
  
  
  bool CozmoEngineImpl::ConnectToRobot(AdvertisingRobot whichRobot)
  {
    const bool success = _robotComms.ConnectToDeviceByID(whichRobot);
    if(success) {
      _connectedRobots[whichRobot];
      //_connectedRobots[whichRobot].visionThread.Start();
      //_connectedRobots[whichRobot].visionMsgHandler.Init(<#Comms::IComms *comms#>, <#Anki::Cozmo::RobotManager *robotMgr#>)
    }
    CozmoEngineSignals::RobotConnectedSignal().emit(whichRobot, success);
    
    return success;
  }
  

  
  Result CozmoEngineImpl::Update(const BaseStationTime_t currTime_ns)
  {
    if(!_isInitialized) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "Cannot update CozmoEngine before it is initialized.\n");
      return RESULT_FAIL;
    }
    
    // Notify any listeners that robots are advertising
    std::vector<int> advertisingRobots;
    _robotComms.GetAdvertisingDeviceIDs(advertisingRobots);
    for(auto & robot : advertisingRobots) {
      CozmoEngineSignals::RobotAvailableSignal().emit(robot);
    }
  
    // TODO: Handle images coming from connected robots
    for(auto & robotKeyPair : _connectedRobots) {
      //robotKeyPair.second.visionMsgHandler.ProcessMessages();
    }
    
    Result lastResult = UpdateInternal(currTime_ns);
    
    return lastResult;
  } // Update()
  
  void CozmoEngineImpl::ProcessDeviceImage(const Vision::Image &image)
  {
    // Process image within the detection rectangle with vision processing thread:
    static const Cozmo::MessageRobotState bogusState; // req'd by API, but not really necessary for marker detection
    
#   if ASYNCHRONOUS_DEVICE_VISION
    _deviceVisionThread.SetNextImage(image, bogusState);
#   else
    _deviceVisionThread.Update(image, bogusState);
    
    MessageVisionMarker msg;
    while(_deviceVisionThread.CheckMailbox(msg)) {
      // Pass marker detections along to UI/game for use
      CozmoEngineSignals::DeviceDetectedVisionMarkerSignal().emit(_engineID, msg.markerType,
                                                                     msg.x_imgUpperLeft,  msg.y_imgUpperLeft,
                                                                     msg.x_imgLowerLeft,  msg.y_imgLowerLeft,
                                                                     msg.x_imgUpperRight, msg.y_imgUpperRight,
                                                                     msg.x_imgLowerRight, msg.y_imgLowerRight);
    }
    
#   endif
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
  
  Result CozmoEngine::Init(const Json::Value& config) {
    return _impl->Init(config);
  }
  
  Result CozmoEngine::Update(const float currTime_sec) {
    return _impl->Update(SEC_TO_NANOS(currTime_sec));
  }
  
  /*
  void CozmoEngine::GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots) {
    _impl->GetAdvertisingRobots(advertisingRobots);
  }
   */
  
  bool CozmoEngine::ConnectToRobot(AdvertisingRobot whichRobot) {
    return _impl->ConnectToRobot(whichRobot);
  }

  
  void CozmoEngine::ProcessDeviceImage(const Vision::Image &image) {
    _impl->ProcessDeviceImage(image);
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
    
    void ForceAddRobot(AdvertisingRobot robotID,
                       const char*      robotIP,
                       bool             robotIsSimulated);
    
    void ListenForRobotConnections(bool listen);
    
    int    GetNumRobots() const;
    Robot* GetRobotByID(const RobotID_t robotID); // returns nullptr for invalid ID
    
    // TODO: Remove once we don't have to specially handle forced adds
    virtual bool ConnectToRobot(AdvertisingRobot whichRobot) override;
    
    // TODO: Remove these in favor of it being handled via messages instead of direct API polling
    bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime);
    
  protected:
    
    virtual Result InitInternal() override;
    virtual Result UpdateInternal(const BaseStationTime_t currTime_ns) override;
    
    BasestationMain              _basestation;
    bool                         _isBasestationStarted;
    bool                         _isListeningForRobots;
    
    Comms::AdvertisementService  _robotAdvertisementService;
    
    std::map<AdvertisingRobot, bool> _forceAddedRobots;
    
  }; // class CozmoEngineHostImpl
  
  
  CozmoEngineHostImpl::CozmoEngineHostImpl()
  : _isBasestationStarted(false)
  , _isListeningForRobots(false)
  , _robotAdvertisementService("RobotAdvertisementService")
  {
    
    PRINT_NAMED_INFO("CozmoEngineHostImpl.Constructor",
                     "Starting RobotAdvertisementService, reg port %d, ad port %d\n",
                     ROBOT_ADVERTISEMENT_REGISTRATION_PORT, ROBOT_ADVERTISING_PORT);

    _robotAdvertisementService.StartService(ROBOT_ADVERTISEMENT_REGISTRATION_PORT,
                                            ROBOT_ADVERTISING_PORT);

  }
  
  Result CozmoEngineHostImpl::StartBasestation()
  {
    if(!_isBasestationStarted)
    {
      BasestationStatus status = _basestation.Init(&_robotComms, _config);
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
    return RESULT_OK;
  }
  
  void CozmoEngineHostImpl::ForceAddRobot(AdvertisingRobot robotID,
                                          const char*      robotIP,
                                          bool             robotIsSimulated)
  {
    if(_isBasestationStarted) {
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
    } else {
      PRINT_NAMED_ERROR("CozmoEngineHostImpl.ForceAddRobot",
                        "You cannot force-add a robot until the engine is started.\n");
    }
  }
  
  int CozmoEngineHostImpl::GetNumRobots() const
  {
    return _basestation.GetNumRobots();
  }
  
  Robot* CozmoEngineHostImpl::GetRobotByID(const RobotID_t robotID)
  {
    return _basestation.GetRobotByID(robotID);
  }
  
  bool CozmoEngineHostImpl::ConnectToRobot(AdvertisingRobot whichRobot)
  {
    // Check if already connected
    Robot* robot = CozmoEngineHostImpl::GetRobotByID(whichRobot);
    if (robot != nullptr) {
      PRINT_NAMED_INFO("CozmoEngineHost.ConnectToRobot.AlreadyConnected", "Robot %d already connected", whichRobot);
      return true;
    }
    
    // Connection is the same as normal except that we have to remove forcefully-added
    // robots from the advertising service manually (if they could do this, they also
    // could have registered itself)
    bool result = CozmoEngineImpl::ConnectToRobot(whichRobot);
    if(_forceAddedRobots.count(whichRobot) > 0) {
      PRINT_NAMED_INFO("CozmoEngineHostImpl.ConnectToRobot",
                       "Manually deregistering force-added robot %d from advertising service.\n", whichRobot);
      _robotAdvertisementService.DeregisterAllAdvertisers();
    }
    
    // Another exception for hosts: have to tell the basestation to add the robot as well
    _basestation.AddRobot(whichRobot);
    
    return result;
  }
  
  void CozmoEngineHostImpl::ListenForRobotConnections(bool listen)
  {
    _isListeningForRobots = listen;
  }
  
  Result CozmoEngineHostImpl::UpdateInternal(const BaseStationTime_t currTime_ns)
  {
     
    // Update robot comms
    if(_robotComms.IsInitialized()) {
      _robotComms.Update();
    }
    
    if(_isListeningForRobots) {
      _robotAdvertisementService.Update();
    }
    
    if(!_isBasestationStarted) {
      StartBasestation();
    }
    
    _basestation.Update(currTime_ns);
    
    /*
    if(_isBasestationStarted) {
      // TODO: Handle different basestation return codes
      _basestation.Update(currTime_ns);
    } else if(_robotComms.GetNumConnectedDevices() >= _config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR].asInt()) {
      // We have connected to enough robots and devices:
      StartBasestation();
    } else {
      // Tics advertisement service
      _robotAdvertisementService.Update();
    }
     */
    
    return RESULT_OK;
  } // UpdateInternal()
  
  bool CozmoEngineHostImpl::GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime)
  {
    return _basestation.GetCurrentRobotImage(robotId, img, newerThanTime);
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
  
  void CozmoEngineHost::ListenForRobotConnections(bool listen)
  {
    _hostImpl->ListenForRobotConnections(listen);
  }
  
  bool CozmoEngineHost::GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime)
  {
    return _hostImpl->GetCurrentRobotImage(robotId, img, newerThanTime);
  }
  
  bool CozmoEngineHost::ConnectToRobot(AdvertisingRobot whichRobot)
  {
    return _hostImpl->ConnectToRobot(whichRobot);
  }
  
  int    CozmoEngineHost::GetNumRobots() const {
    return _hostImpl->GetNumRobots();
  }
  
  Robot* CozmoEngineHost::GetRobotByID(const RobotID_t robotID) {
    return _hostImpl->GetRobotByID(robotID);
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
  
  
} // namespace Cozmo
} // namespace Anki