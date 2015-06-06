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
#include "anki/cozmo/shared/cozmoEngineConfig.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoEngine.h"
#include "anki/cozmo/basestation/multiClientChannel.h" // TODO: Remove?
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/visionProcessingThread.h"
#include "anki/cozmo/basestation/signals/cozmoEngineSignals.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include "anki/messaging/basestation/advertisementService.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/util/logging/printfLoggerProvider.h"

#include "robotMessageHandler.h"
#include "recording/playback.h"

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
    
    void DisconnectFromRobot(RobotID_t whichRobot);
    
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
    
    MultiClientChannel        _robotChannel;
    
    /*
    // TODO: Merge this into RobotManager
    // Each engine can potetnailly talk to multiple physical robots.
    // Package up the stuff req'd to deal with one robot and store a map
    // of them keyed by robot ID.
    struct RobotContainer {
      VisionProcessingThread    visionThread;
      RobotMessageHandler       visionMsgHandler;
    };
    std::map<AdvertisingRobot, RobotContainer> _connectedRobots;
    */
    
    VisionProcessingThread    _deviceVisionThread;
    
    std::vector<Signal::SmartHandle> _signalHandles;
    
  }; // class CozmoEngineImpl

  
  CozmoEngineImpl::CozmoEngineImpl()
  : _isInitialized(false)
  {
    if (Anki::Util::gTickTimeProvider == nullptr) {
      Anki::Util::gTickTimeProvider = BaseStationTimer::getInstance();
    }
    
    // Handle robot disconnection:
    auto cbRobotDisconnected = [this](RobotID_t robotID) {
      PRINT_NAMED_INFO("CozmoEngineImpl.Constructor.cbRobotDisconnected", "Disconnecting from robot %d, haven't received message in too long.", robotID);
      this->DisconnectFromRobot(robotID);
    };
    _signalHandles.emplace_back( CozmoEngineSignals::RobotDisconnectedSignal().ScopedSubscribe(cbRobotDisconnected));
  }
  
  CozmoEngineImpl::~CozmoEngineImpl()
  {
    _robotChannel.Stop();
    
    if (Anki::Util::gTickTimeProvider == BaseStationTimer::getInstance()) {
      Anki::Util::gTickTimeProvider = nullptr;
    }
    
    BaseStationTimer::removeInstance();
  }
  
  Result CozmoEngineImpl::Init(const Json::Value& config)
  {
    if(_isInitialized) {
      PRINT_NAMED_INFO("CozmoEngineImpl.Init.ReInit", "Reinitializing already-initialized CozmoEngineImpl with new config.");
    }
    
    _config = config;
    
    if(!_config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP)) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "No AdvertisingHostIP defined in Json config.");
      return RESULT_FAIL;
    }
    
    if(!_config.isMember(AnkiUtil::kP_ROBOT_ADVERTISING_PORT)) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "No RobotAdvertisingPort defined in Json config.");
      return RESULT_FAIL;
    }
    
    if(!_config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "No UiAdvertisingPort defined in Json config.");
      return RESULT_FAIL;
    }
    
    Vision::CameraCalibration deviceCamCalib;
    if(!_config.isMember(AnkiUtil::kP_DEVICE_CAMERA_CALIBRATION)) {
      PRINT_NAMED_WARNING("CozmoEngine.Init",
                          "No DeviceCameraCalibration defined in Json config. Using bogus settings.");
    } else {
      deviceCamCalib.Set(_config[AnkiUtil::kP_DEVICE_CAMERA_CALIBRATION]);
    }
    
    Result lastResult = RESULT_OK;
    const char *ipString = _config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString();
    int port =_config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT].asInt();
    if (port < 0 || port >= 0x10000) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "Failed to initialize RobotComms; bad port.");
      return RESULT_FAIL;
    }
    Anki::Util::TransportAddress address(ipString, static_cast<uint16_t>(port));
    PRINT_STREAM_DEBUG("CozmoEngine.Init", "Initializing on address: " << address << ": " << address.GetIPAddress() << ":" << address.GetIPPort() << "; originals: " << ipString << ":" << port);
    
    _robotChannel.Start(address);
    if(!_robotChannel.IsStarted()) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "Failed to initialize RobotComms.");
      return RESULT_FAIL;
    }
    
    if(!_config.isMember(AnkiUtil::kP_VIZ_HOST_IP)) {
      PRINT_NAMED_WARNING("CozmoEngineInit.NoVizHostIP",
                          "No VizHostIP member in JSON config file. Not initializing VizManager.");
    } else if(!_config[AnkiUtil::kP_VIZ_HOST_IP].asString().empty()){
      VizManager::getInstance()->Connect(_config[AnkiUtil::kP_VIZ_HOST_IP].asCString(), VIZ_SERVER_PORT);
      
      // Erase anything that's still being visualized in case there were leftovers from
      // a previous run?? (We should really be cleaning up after ourselves when
      // we tear down, but it seems like Webots restarts aren't always allowing
      // the cleanup to happen)
      VizManager::getInstance()->EraseAllVizObjects();
      
      // Only send images if the viz host is the same as the robot advertisement service
      // (so we don't waste bandwidth sending (uncompressed) viz data over the network
      //  to be displayed on another machine)
      if(_config[AnkiUtil::kP_VIZ_HOST_IP] == _config[AnkiUtil::kP_ADVERTISING_HOST_IP]) {
        VizManager::getInstance()->EnableImageSend(true);
      }
    }
    
    lastResult = InitInternal();
    if(lastResult != RESULT_OK) {
      PRINT_NAMED_ERROR("CozomEngine.Init", "Failed calling internal init.");
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
    _robotChannel.Update();
    _robotChannel.GetAdvertisingDeviceIDs(advertisingRobots);
  }
   */
  
  
  bool CozmoEngineImpl::ConnectToRobot(AdvertisingRobot whichRobot)
  {
    Anki::Comms::ConnectionId id = static_cast<Anki::Comms::ConnectionId>(whichRobot);
    bool success = _robotChannel.AcceptAdvertisingConnection(id);
    if (success) {
      //_connectedRobots[whichRobot];
      //_connectedRobots[whichRobot].visionThread.Start();
      //_connectedRobots[whichRobot].visionMsgHandler.Init(<#Comms::IComms *comms#>, <#Anki::Cozmo::RobotManager *robotMgr#>)
    }
    CozmoEngineSignals::RobotConnectedSignal().emit(whichRobot, success);
    
    return success;
  }
  
  void CozmoEngineImpl::DisconnectFromRobot(RobotID_t whichRobot) {
    Anki::Comms::ConnectionId id = static_cast<Anki::Comms::ConnectionId>(whichRobot);
    _robotChannel.RemoveConnection(id);
    /*
    auto connectedRobotIter = _connectedRobots.find(whichRobot);
    if(connectedRobotIter != _connectedRobots.end()) {
      _connectedRobots.erase(connectedRobotIter);
    }
     */
  }
  
  Result CozmoEngineImpl::Update(const BaseStationTime_t currTime_ns)
  {
    if(!_isInitialized) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "Cannot update CozmoEngine before it is initialized.\n");
      return RESULT_FAIL;
    }
    
    // Check if engine tic is exceeding expected time
    static TimeStamp_t prevTime = 0;
    TimeStamp_t currTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    if (prevTime != 0) {
      TimeStamp_t timeSinceLastTic = currTime - prevTime;
      // Print warning if this tic was executed more than the expected amount of time
      // (with some margin) after the last tic.
      if (timeSinceLastTic > BS_TIME_STEP + 10) {
        PRINT_NAMED_WARNING("CozmoEngine.Update.LateTic",
                            "currTime %dms, timeSinceLastTic %dms (should be ~%dms)\n",
                            currTime, timeSinceLastTic, BS_TIME_STEP);
      }
    }
    prevTime = currTime;
    
    
    // Notify any listeners that robots are advertising
    std::vector<Comms::ConnectionId> advertisingRobots;
    _robotChannel.GetAdvertisingConnections(advertisingRobots);
    for(auto robot : advertisingRobots) {
      CozmoEngineSignals::RobotAvailableSignal().emit(robot);
    }
  
    // TODO: Handle images coming from connected robots
    /*
    for(auto & robotKeyPair : _connectedRobots) {
      robotKeyPair.second.visionMsgHandler.ProcessMessages();
    }
     */
    
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
  , _loggerProvider()
  {
    _loggerProvider.SetMinLogLevel(0);
    if (Anki::Util::gLoggerProvider == nullptr) {
      Anki::Util::gLoggerProvider = &_loggerProvider;
    }
  }
  
  CozmoEngine::~CozmoEngine()
  {
    if (Anki::Util::gLoggerProvider == &_loggerProvider) {
      Anki::Util::gLoggerProvider = nullptr;
    }
    
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

  void CozmoEngine::DisconnectFromRobot(RobotID_t whichRobot) {
    _impl->DisconnectFromRobot(whichRobot);
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
    std::vector<RobotID_t> const& GetRobotIDList() const;
    
    // TODO: Remove once we don't have to specially handle forced adds
    virtual bool ConnectToRobot(AdvertisingRobot whichRobot) override;
      
    // TODO: Remove these in favor of it being handled via messages instead of direct API polling
    bool GetCurrentRobotImage(RobotID_t robotId, Vision::Image& img, TimeStamp_t newerThanTime);
    
  protected:
    
    virtual Result InitInternal() override;
    virtual Result UpdateInternal(const BaseStationTime_t currTime_ns) override;
    
    void InitPlaybackAndRecording();
    
    
    Result AddRobot(RobotID_t robotID);
    
    bool                         _isListeningForRobots;
    Comms::AdvertisementService  _robotAdvertisementService;
    RobotManager                 _robotMgr;
    RobotMessageHandler          _robotMsgHandler;
    
    std::map<AdvertisingRobot, bool> _forceAddedRobots;
    
#if COZMO_RECORDING_PLAYBACK
    // TODO: Make use of these for playback/recording
    IRecordingPlaybackModule *recordingPlaybackModule_;
    IRecordingPlaybackModule *uiRecordingPlaybackModule_;
#endif
    
  }; // class CozmoEngineHostImpl
  
  
  CozmoEngineHostImpl::CozmoEngineHostImpl()
  : _isListeningForRobots(false)
  , _robotAdvertisementService("RobotAdvertisementService")
  {
    
    PRINT_NAMED_INFO("CozmoEngineHostImpl.Constructor",
                     "Starting RobotAdvertisementService, reg port %d, ad port %d\n",
                     ROBOT_ADVERTISEMENT_REGISTRATION_PORT, ROBOT_ADVERTISING_PORT);

    _robotAdvertisementService.StartService(ROBOT_ADVERTISEMENT_REGISTRATION_PORT,
                                            ROBOT_ADVERTISING_PORT);

  }
  
  Result CozmoEngineHostImpl::InitInternal()
  {
    Result lastResult = _robotMsgHandler.Init(&_robotChannel, &_robotMgr);
    
    return lastResult;
  }
  
  void CozmoEngineHostImpl::ForceAddRobot(AdvertisingRobot robotID,
                                          const char*      robotIP,
                                          bool             robotIsSimulated)
  {
    if(_isInitialized) {
      PRINT_NAMED_INFO("CozmoEngineHostImpl.ForceAddRobot", "Force-adding %s robot with ID %d and IP %s\n",
                       (robotIsSimulated ? "simulated" : "real"), robotID, robotIP);
      
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
                        "You cannot force-add a robot until the engine is initialized.\n");
    }
  }
  
  void CozmoEngineHostImpl::InitPlaybackAndRecording()
  {
    // TODO: get playback/recording working again
    
    /*
     // Get basestation mode out of the config
     int modeInt;
     if(JsonTools::GetValueOptional(config, AnkiUtil::kP_BASESTATION_MODE, modeInt)) {
     mode_ = static_cast<BasestationMode>(modeInt);
     assert(mode_ <= BM_PLAYBACK_SESSION);
     } else {
     mode_ = BM_DEFAULT;
     }
     
     PRINT_INFO("Starting basestation mode %d\n", mode_);
     switch(mode_)
     {
     case BM_RECORD_SESSION:
     {
     // Create folder for all recorded logs
     std::string rootLogFolderName = AnkiUtil::kP_GAME_LOG_ROOT_DIR;
     if (!DirExists(rootLogFolderName.c_str())) {
     if(!MakeDir(rootLogFolderName.c_str())) {
     PRINT_NAMED_WARNING("Basestation.Init.RootLogDirCreateFailed", "Failed to create folder %s\n", rootLogFolderName.c_str());
     return BS_END_INIT_ERROR;
     }
     
     }
     
     // Create folder for log
     std::string logFolderName = rootLogFolderName + "/" + GetCurrentDateTime() + "/";
     if(!MakeDir(logFolderName.c_str())) {
     PRINT_NAMED_WARNING("Basestation.Init.LogDirCreateFailed", "Failed to create folder %s\n", logFolderName.c_str());
     return BS_END_INIT_ERROR;
     }
     
     // Save config to log folder
     Json::StyledStreamWriter writer;
     std::ofstream jsonFile(logFolderName + AnkiUtil::kP_CONFIG_JSON_FILE);
     writer.write(jsonFile, config);
     jsonFile.close();
     
     
     // Setup recording modules
     Comms::IComms *replacementComms = NULL;
     recordingPlaybackModule_ = new Recording();
     status = ConvertStatus(recordingPlaybackModule_->Init(robot_comms, &replacementComms, &config_, logFolderName + AnkiUtil::kP_ROBOT_COMMS_LOG_FILE));
     robot_comms = replacementComms;
     
     uiRecordingPlaybackModule_ = new Recording();
     status = ConvertStatus(uiRecordingPlaybackModule_->Init(ui_comms, &replacementComms, &config_, logFolderName + AnkiUtil::kP_UI_COMMS_LOG_FILE));
     ui_comms = replacementComms;
     break;
     }
     
     case BM_PLAYBACK_SESSION:
     {
     // Get log folder from config
     std::string logFolderName;
     if (!JsonTools::GetValueOptional(config, AnkiUtil::kP_PLAYBACK_LOG_FOLDER, logFolderName)) {
     PRINT_NAMED_ERROR("Basestation.Init.PlaybackDirNotSpecified", "\n");
     return BS_END_INIT_ERROR;
     }
     logFolderName = AnkiUtil::kP_GAME_LOG_ROOT_DIR + string("/") + logFolderName + "/";
     
     
     // Check if folder exists
     if (!DirExists(logFolderName.c_str())) {
     PRINT_NAMED_ERROR("Basestation.Init.PlaybackDirNotFound", "%s\n", logFolderName.c_str());
     return BS_END_INIT_ERROR;
     }
     
     // Load configuration json from playback log folder
     Json::Reader reader;
     std::ifstream jsonFile(logFolderName + AnkiUtil::kP_CONFIG_JSON_FILE);
     reader.parse(jsonFile, config_);
     jsonFile.close();
     
     
     // Setup playback modules
     Comms::IComms *replacementComms = NULL;
     recordingPlaybackModule_ = new Playback();
     status = ConvertStatus(recordingPlaybackModule_->Init(robot_comms, &replacementComms, &config_, logFolderName + AnkiUtil::kP_ROBOT_COMMS_LOG_FILE));
     robot_comms = replacementComms;
     
     uiRecordingPlaybackModule_ = new Playback();
     status = ConvertStatus(uiRecordingPlaybackModule_->Init(ui_comms, &replacementComms, &config_, logFolderName + AnkiUtil::kP_UI_COMMS_LOG_FILE));
     ui_comms = replacementComms;
     break;
     }
     
     case BM_DEFAULT:
     break;
     }
     */
  }
  
  Result CozmoEngineHostImpl::AddRobot(RobotID_t robotID)
  {
    Result lastResult = RESULT_OK;
    
    _robotMgr.AddRobot(robotID, &_robotMsgHandler);
    Robot* robot = _robotMgr.GetRobotByID(robotID);
    if(nullptr == robot) {
      PRINT_NAMED_ERROR("CozmoEngineHostImpl.AddRobot", "Failed to add robot ID=%d (nullptr returned).\n", robotID);
      lastResult = RESULT_FAIL;
    } else {
      lastResult = robot->SyncTime();
    }
    
    return lastResult;
  }
  
  
  int CozmoEngineHostImpl::GetNumRobots() const
  {
    const size_t N = _robotMgr.GetNumRobots();
    assert(N < INT_MAX);
    return static_cast<int>(N);
  }
  
  Robot* CozmoEngineHostImpl::GetRobotByID(const RobotID_t robotID)
  {
    return _robotMgr.GetRobotByID(robotID);
  }
  
  std::vector<RobotID_t> const& CozmoEngineHostImpl::GetRobotIDList() const
  {
    return _robotMgr.GetRobotIDList();
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
    AddRobot(whichRobot);
    
    return result;
  }
  
  void CozmoEngineHostImpl::ListenForRobotConnections(bool listen)
  {
    _isListeningForRobots = listen;
  }
  
  Result CozmoEngineHostImpl::UpdateInternal(const BaseStationTime_t currTime_ns)
  {
     
    // Update robot comms
    _robotChannel.Update();
    
    if(_isListeningForRobots) {
      _robotAdvertisementService.Update();
    }
    
    // Update time
    BaseStationTimer::getInstance()->UpdateTime(currTime_ns);
    
    _robotMsgHandler.ProcessMessages();
    
    // Let the robot manager do whatever it's gotta do to update the
    // robots in the world.
    _robotMgr.UpdateAllRobots();
    
    return RESULT_OK;
  } // UpdateInternal()
  
  bool CozmoEngineHostImpl::GetCurrentRobotImage(RobotID_t robotID, Vision::Image& img, TimeStamp_t newerThanTime)
  {
    Robot* robot = _robotMgr.GetRobotByID(robotID);
    
    if(robot != nullptr) {
      return robot->GetCurrentImage(img, newerThanTime);
    } else {
      PRINT_NAMED_ERROR("BasestationMainImpl.GetCurrentRobotImage.InvalidRobotID",
                        "Image requested for invalid robot ID = %d.\n", robotID);
      return false;
    }
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
  
  std::vector<RobotID_t> const& CozmoEngineHost::GetRobotIDList() const {
    return _hostImpl->GetRobotIDList();
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