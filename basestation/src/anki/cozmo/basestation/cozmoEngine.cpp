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

#include "anki/cozmo/basestation/cozmoEngine.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/logging/logging.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "util/helpers/templateHelpers.h"
#include "anki/cozmo/basestation/audio/audioController.h"
#include "anki/cozmo/basestation/audio/audioUnityClientConnection.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/cozmo/basestation/audio/audioEngineMessageHandler.h"
#include "anki/cozmo/basestation/audio/audioEngineClientConnection.h"
#include "anki/cozmo/basestation/audio/audioServer.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/multiClientChannel.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/game/comms/uiMessageHandler.h"
#include "util/console/consoleInterface.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/sosLoggerProvider.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/logging/multiLoggerProvider.h"
#include "util/time/universalTime.h"
#include <cstdlib>

#if ANDROID
#include "anki/cozmo/basestation/speechRecognition/keyWordRecognizer_android.h"
#else
#include "anki/cozmo/basestation/speechRecognition/keyWordRecognizer_ios_mac.h"
#endif

#if ANKI_DEV_CHEATS
#include "anki/cozmo/basestation/debug/usbTunnelEndServer_ios.h"
#endif

#define ENABLE_CE_SLEEP_TIME_DIAGNOSTICS 0
#define ENABLE_CE_RUN_TIME_DIAGNOSTICS 1

#if REMOTE_CONSOLE_ENABLED
namespace Anki { namespace Util {
CONSOLE_VAR_EXTERN(float,  gNetStat2LatencyAvg);
CONSOLE_VAR_EXTERN(float,  gNetStat4LatencyMin);
CONSOLE_VAR_EXTERN(float,  gNetStat5LatencyMax);
}}
#endif

namespace Anki {
namespace Cozmo {

CozmoEngine::CozmoEngine(Util::Data::DataPlatform* dataPlatform)
  : _robotChannel(new MultiClientChannel{})
  , _uiMsgHandler(new UiMessageHandler(1))
  , _keywordRecognizer(new SpeechRecognition::KeyWordRecognizer(_uiMsgHandler.get()))
  , _context(new CozmoContext(dataPlatform, _uiMsgHandler.get()))
#if ANKI_DEV_CHEATS && !ANDROID
  , _usbTunnelServerDebug(new USBTunnelServer(_uiMsgHandler.get(),dataPlatform))
#endif
{
  ASSERT_NAMED(_context->GetExternalInterface() != nullptr, "Cozmo.Engine.ExternalInterface.nullptr");
  if (Anki::Util::gTickTimeProvider == nullptr) {
    Anki::Util::gTickTimeProvider = BaseStationTimer::getInstance();
  }
  
  PRINT_NAMED_INFO("CozmoEngine.Constructor",
                   "Starting RobotAdvertisementService, reg port %d, ad port %d",
                   ROBOT_ADVERTISEMENT_REGISTRATION_PORT, ROBOT_ADVERTISING_PORT);
  
  _context->GetRobotAdvertisementService()->StartService(ROBOT_ADVERTISEMENT_REGISTRATION_PORT,
                                          ROBOT_ADVERTISING_PORT);
  
  // Handle robot disconnection:
  _signalHandles.emplace_back( _context->GetRobotManager()->OnRobotDisconnected().ScopedSubscribe(
                                                                               std::bind(&CozmoEngine::DisconnectFromRobot, this, std::placeholders::_1)
                                                                               ));
  // We'll use this callback for simple events we care about
  auto callback = std::bind(&CozmoEngine::HandleGameEvents, this, std::placeholders::_1);
  _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::SetRobotImageSendMode, callback));
  _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::ImageRequest, callback));
  _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::ConnectToRobot, callback));
  _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::ForceAddRobot, callback));
  _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::ReadAnimationFile, callback));
  _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::StartTestMode, callback));
  _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::SetRobotVolume, callback));
  _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::ReliableTransportRunMode, callback));
  _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::SetEnableSOSLogging, callback));

  
  // Use a separate callback for StartEngine
  auto startEngineCallback = std::bind(&CozmoEngine::HandleStartEngine, this, std::placeholders::_1);
  _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::StartEngine, startEngineCallback));
  
  auto updateFirmwareCallback = std::bind(&CozmoEngine::HandleUpdateFirmware, this, std::placeholders::_1);
  _signalHandles.push_back(_context->GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::UpdateFirmware, updateFirmwareCallback));
  
  _debugConsoleManager.Init(_context->GetExternalInterface());
}
  

CozmoEngine::~CozmoEngine()
{
  _robotChannel->Stop();
  
  if (Anki::Util::gTickTimeProvider == BaseStationTimer::getInstance()) {
    Anki::Util::gTickTimeProvider = nullptr;
  }
  
  BaseStationTimer::removeInstance();
  _context->GetVizManager()->Disconnect();
}

Result CozmoEngine::Init(const Json::Value& config) {

  if(_isInitialized) {
    PRINT_NAMED_INFO("CozmoEngine.Init.ReInit", "Reinitializing already-initialized CozmoEngineImpl with new config.");
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
  
  Result lastResult = _uiMsgHandler->Init(_config);
  if (RESULT_OK != lastResult)
  {
    PRINT_NAMED_ERROR("CozmoEngine.Init","Error initializing UIMessageHandler");
    return lastResult;
  }
  
  if(!_config.isMember(AnkiUtil::kP_VIZ_HOST_IP)) {
    PRINT_NAMED_WARNING("CozmoEngineInit.NoVizHostIP",
                        "No VizHostIP member in JSON config file. Not initializing VizManager.");
  } else if(!_config[AnkiUtil::kP_VIZ_HOST_IP].asString().empty()){
    _context->GetVizManager()->Connect(_config[AnkiUtil::kP_VIZ_HOST_IP].asCString(), (uint16_t)VizConstants::VIZ_SERVER_PORT,
                                       _config[AnkiUtil::kP_ADVERTISING_HOST_IP].isString() ? _config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString() : "127.0.0.1", (uint16_t)VizConstants::UNITY_VIZ_SERVER_PORT);
    
    // Erase anything that's still being visualized in case there were leftovers from
    // a previous run?? (We should really be cleaning up after ourselves when
    // we tear down, but it seems like Webots restarts aren't always allowing
    // the cleanup to happen)
    _context->GetVizManager()->EraseAllVizObjects();
    
    // Only send images if the viz host is the same as the robot advertisement service
    // (so we don't waste bandwidth sending (uncompressed) viz data over the network
    //  to be displayed on another machine)
    if(_config[AnkiUtil::kP_VIZ_HOST_IP] == _config[AnkiUtil::kP_ADVERTISING_HOST_IP]) {
      _context->GetVizManager()->EnableImageSend(true);
    }
    
    if (nullptr != _context->GetExternalInterface())
    {
      // Have VizManager subscribe to the events it should care about
      _context->GetVizManager()->SubscribeToEngineEvents(*_context->GetExternalInterface());
    }
  }
  
  lastResult = InitInternal();
  if(lastResult != RESULT_OK) {
    PRINT_NAMED_ERROR("CozomEngine.Init", "Failed calling internal init.");
    return lastResult;
  }
  
  _isInitialized = true;
  
  _context->GetRobotManager()->Init();
  
  return RESULT_OK;
}
  
void CozmoEngine::HandleStartEngine(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  if (EngineState::Running == _engineState) {
    PRINT_NAMED_ERROR("CozmoEngine.HandleStartEngine.AlreadyStarted", "");
    return;
  }
  
  ListenForRobotConnections(true);
  
  const char *ipString = _config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString();
  int port =_config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT].asInt();
  if (port < 0 || port >= 0x10000) {
    PRINT_NAMED_ERROR("CozmoEngine.HandleStartEngine", "Failed to initialize RobotComms; bad port %d", port);
    return;
  }
  Anki::Util::TransportAddress address(ipString, static_cast<uint16_t>(port));
  PRINT_STREAM_DEBUG("CozmoEngine.HandleStartEngine", "Initializing on address: " << address << ": " << address.GetIPAddress() << ":" << address.GetIPPort() << "; originals: " << ipString << ":" << port);
  
  _robotChannel->Start(address);
  if(!_robotChannel->IsStarted()) {
    PRINT_NAMED_ERROR("CozmoEngine.HandleStartEngine", "Failed to initialize RobotComms.");
    return;
  }
  
  _context->GetRobotMsgHandler()->Init(_robotChannel.get(), _context->GetRobotManager());
  
  SetEngineState(EngineState::WaitingForUIDevices);
}
  
void CozmoEngine::HandleUpdateFirmware(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  const ExternalInterface::UpdateFirmware& msg = event.GetData().Get_UpdateFirmware();
  
  if (EngineState::UpdatingFirmware == _engineState)
  {
    PRINT_NAMED_WARNING("CozmoEngine.HandleUpdateFirmware.AlreadyStarted", "");
    return;
  }

  if (_context->GetRobotManager()->InitUpdateFirmware(msg.version))
  {
    SetEngineState(EngineState::UpdatingFirmware);
  }
}
  
bool CozmoEngine::ConnectToRobot(AdvertisingRobot whichRobot)
{
  if( CozmoEngine::HasRobotWithID(whichRobot)) {
    PRINT_NAMED_INFO("CozmoEngine.ConnectToRobot.AlreadyConnected", "Robot %d already connected", whichRobot);
    return true;
  }
  
  // Connection is the same as normal except that we have to remove forcefully-added
  // robots from the advertising service manually (if they could do this, they also
  // could have registered itself)
  Anki::Comms::ConnectionId id = static_cast<Anki::Comms::ConnectionId>(whichRobot);
  bool result = _robotChannel->AcceptAdvertisingConnection(id);
  if(_forceAddedRobots.count(whichRobot) > 0) {
    PRINT_NAMED_INFO("CozmoEngine.ConnectToRobot",
                     "Manually deregistering force-added robot %d from advertising service.", whichRobot);
    _context->GetRobotAdvertisementService()->DeregisterAllAdvertisers();
  }
  
  // Another exception for hosts: have to tell the basestation to add the robot as well
  AddRobot(whichRobot);
  _context->GetExternalInterface()->BroadcastToGame<ExternalInterface::RobotConnected>(whichRobot, result);
  return result;
}
  
void CozmoEngine::DisconnectFromRobot(RobotID_t whichRobot)
{
  Anki::Comms::ConnectionId id = static_cast<Anki::Comms::ConnectionId>(whichRobot);
  _robotChannel->RemoveConnection(id);
}
  
Result CozmoEngine::Update(const float currTime_sec)
{
  if(!_isInitialized) {
    PRINT_NAMED_ERROR("CozmoEngine.Update", "Cannot update CozmoEngine before it is initialized.");
    return RESULT_FAIL;
  }
  
  const double startUpdateTimeMs = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
#if ENABLE_CE_SLEEP_TIME_DIAGNOSTICS
  {
    static bool firstUpdate = true;
    static double lastUpdateTimeMs = 0.0;
    //const double currentTimeMs = (double)currTime_sec * 1e+3;
    if (! firstUpdate)
    {
      const double timeSinceLastUpdate = startUpdateTimeMs - lastUpdateTimeMs;
      const double maxLatency = BS_TIME_STEP + 15.;
      if (timeSinceLastUpdate > maxLatency)
      {
        char dataString[20]{0};
        std::snprintf(dataString, sizeof(dataString), "%d", BS_TIME_STEP);
        Anki::Util::sEventF("cozmo_engine.update.sleep.slow", {{DDATA,dataString}}, "%.2f", timeSinceLastUpdate);
      }
    }
    lastUpdateTimeMs = startUpdateTimeMs;
    firstUpdate = false;
  }
#endif // ENABLE_CE_SLEEP_TIME_DIAGNOSTICS
  
  // Handle UI
  Result lastResult = _uiMsgHandler->Update();
  if (RESULT_OK != lastResult)
  {
    PRINT_NAMED_ERROR("CozmoEngine.Update", "Error updating UIMessageHandler");
    return lastResult;
  }
  
  switch (_engineState)
  {
    case EngineState::Stopped:
    {
      break;
    }
    case EngineState::WaitingForUIDevices:
    {
      if (_uiMsgHandler->HasDesiredNumUiDevices()) {
        _context->GetRobotManager()->BroadcastAvailableAnimations();
        SetEngineState(EngineState::Running);
      }
      break;
    }
    case EngineState::Running:
    {
      // Notify any listeners that robots are advertising
      std::vector<Comms::ConnectionId> advertisingRobots;
      _robotChannel->GetAdvertisingConnections(advertisingRobots);
      for(auto robot : advertisingRobots) {
        _context->GetExternalInterface()->BroadcastToGame<ExternalInterface::RobotAvailable>(robot);
      }
      
      _robotChannel->Update();
      
      if(_isListeningForRobots) {
        _context->GetRobotAdvertisementService()->Update();
      }
      
      // Update time
      BaseStationTimer::getInstance()->UpdateTime(SEC_TO_NANOS(currTime_sec));
      
      _context->GetRobotMsgHandler()->ProcessMessages();
      
      // Let the robot manager do whatever it's gotta do to update the
      // robots in the world.
      _context->GetRobotManager()->UpdateAllRobots();
      
      _keywordRecognizer->Update((uint32_t)(BaseStationTimer::getInstance()->GetTimeSinceLastTickInSeconds() * 1000.0f));
      
#if REMOTE_CONSOLE_ENABLED
      _context->GetExternalInterface()->BroadcastToGame<ExternalInterface::DebugLatencyMessage>(Util::gNetStat2LatencyAvg,
                                                                                                Util::gNetStat4LatencyMin,
                                                                                                Util::gNetStat5LatencyMax);
#endif
      break;
    }
    case EngineState::UpdatingFirmware:
    {
      // Update comms and messages from robot
      
      _robotChannel->Update();
      
      _context->GetRobotMsgHandler()->ProcessMessages();
      
      // Update the firmware updating, returns true when complete (error or success)
      
      if (_context->GetRobotManager()->UpdateFirmware())
      {
        SetEngineState(EngineState::Running);
      }
      
      break;
    }
    default:
      PRINT_NAMED_ERROR("CozmoEngine.Update.UnexpectedState","Running Update in an unexpected state!");
  }
  
#if ENABLE_CE_RUN_TIME_DIAGNOSTICS
  {
    const double endUpdateTimeMs = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
    const double updateLengthMs = endUpdateTimeMs - startUpdateTimeMs;
    const double maxUpdateDuration = BS_TIME_STEP;
    if (updateLengthMs > maxUpdateDuration)
    {
      char dataString[20]{0};
      std::snprintf(dataString, sizeof(dataString), "%d", BS_TIME_STEP);
      Anki::Util::sEventF("cozmo_engine.update.run.slow", {{DDATA,dataString}}, "%.2f", updateLengthMs);
    }
  }
#endif // ENABLE_CE_RUN_TIME_DIAGNOSTICS

  return RESULT_OK;
}
  
void CozmoEngine::SetEngineState(EngineState newState)
{
  EngineState oldState = _engineState;
  if (oldState == newState)
  {
    return;
  }
  
  _engineState = newState;
  
  _context->GetExternalInterface()->BroadcastToGame<ExternalInterface::UpdateEngineState>(oldState, newState);
  
  PRINT_NAMED_EVENT("app.engine.state","%s => %s", EngineStateToString(oldState), EngineStateToString(newState));
}

void CozmoEngine::ReadAnimationsFromDisk()
{
  _context->GetRobotManager()->ReadAnimationDir();
}
  
Result CozmoEngine::InitInternal()
{
  std::string hmmFolder = _context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, "pocketsphinx/en-us");
  std::string keywordFile = _context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, "config/basestation/config/cozmoPhrases.txt");
  std::string dictFile = _context->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, "pocketsphinx/cmudict-en-us.dict");
  _keywordRecognizer->Init(hmmFolder, keywordFile, dictFile);
  
  // Setup Audio Controller
  using namespace Audio;
  
  // Setup Unity Audio Client Connections
  AudioUnityClientConnection *unityConnection = new AudioUnityClientConnection( *_context->GetExternalInterface() );
  _context->GetAudioServer()->RegisterClientConnection( unityConnection );
  
  return RESULT_OK;
}

void CozmoEngine::ForceAddRobot(AdvertisingRobot robotID,
                                        const char*      robotIP,
                                        bool             robotIsSimulated)
{
  if(_isInitialized) {
    
    // Force add physical robot since it's not registering by itself yet.
    AdvertisementRegistrationMsg forcedRegistrationMsg;
    forcedRegistrationMsg.id = robotID;
    forcedRegistrationMsg.toEnginePort = Anki::Cozmo::ROBOT_RADIO_BASE_PORT + (robotIsSimulated ? robotID : 0);
    forcedRegistrationMsg.fromEnginePort = forcedRegistrationMsg.toEnginePort;
    forcedRegistrationMsg.enableAdvertisement = 1;
    forcedRegistrationMsg.ip = robotIP;
    
    PRINT_NAMED_INFO("CozmoEngine.ForceAddRobot", "Force-adding %s robot with ID %d and IP %s:%d",
                     (robotIsSimulated ? "simulated" : "real"), robotID, robotIP, (int)forcedRegistrationMsg.toEnginePort);
    
    _context->GetRobotAdvertisementService()->ProcessRegistrationMsg(forcedRegistrationMsg);
    
    // Mark this robot as force-added so we can deregister it from the advertising
    // service manually once we connect to it.
    _forceAddedRobots[robotID] = true;
  } else {
    PRINT_NAMED_ERROR("CozmoEngine.ForceAddRobot",
                      "You cannot force-add a robot until the engine is initialized.");
  }
}
  
Result CozmoEngine::AddRobot(RobotID_t robotID)
{
  Result lastResult = RESULT_OK;
  
  _context->GetRobotManager()->AddRobot(robotID);
  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  if(nullptr == robot) {
    PRINT_NAMED_ERROR("CozmoEngine.AddRobot", "Failed to add robot ID=%d (nullptr returned).", robotID);
    lastResult = RESULT_FAIL;
  } else {
    PRINT_NAMED_INFO("CozmoEngine.AddRobot", "Sending init to the robot %d.", robotID);
    lastResult = robot->SyncTime();
    
    // Requesting camera calibration
    robot->GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_CameraCalibration,
                                        [robot](u8* data, size_t size, NVStorage::NVResult res) {
                                        
                                        if (res == NVStorage::NVResult::NV_OKAY) {
                                          CameraCalibration payload;
                                          payload.Unpack(data, size);
                                          PRINT_NAMED_INFO("CozmoEngine.ReadCameraCalibration",
                                                           "Received new %dx%d camera calibration from robot. (fx: %f, fy: %f, cx: %f cy: %f)",
                                                           payload.ncols, payload.nrows,
                                                           payload.focalLength_x, payload.focalLength_y,
                                                           payload.center_x, payload.center_y);
                                          
                                          // Convert calibration message into a calibration object to pass to the robot
                                          Vision::CameraCalibration calib(payload.nrows,
                                                                          payload.ncols,
                                                                          payload.focalLength_x,
                                                                          payload.focalLength_y,
                                                                          payload.center_x,
                                                                          payload.center_y,
                                                                          payload.skew);
                                          
                                          robot->GetVisionComponent().SetCameraCalibration(calib);
                                          
                                          
                                          
                                          
                                        } else {
                                          PRINT_NAMED_INFO("CozmoEngine.ReadCameraCalibration.Failed", "");
                                        }
                                        });

    // TEMP: Copy calibration over to NVEntry_CameraCalib, which is in the manufacturing partition.
    //       Eventually we'll delete NVEntry_CameraCalibration.
    robot->GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_CameraCalib,
                                        [robot](u8* data, size_t size, NVStorage::NVResult res) {
                                          CameraCalibration payload;
                                          if (res == NVStorage::NVResult::NV_OKAY) {
                                            payload.Unpack(data, size);
                                            PRINT_NAMED_INFO("CozmoEngine.ReadMfgCameraCalib",
                                                             "Mfg camera calib received from robot (size: %dx%d, fx: %f, fy: %f, cx: %f cy: %f)",
                                                             payload.ncols, payload.nrows,
                                                             payload.focalLength_x, payload.focalLength_y,
                                                             payload.center_x, payload.center_y);
                                          } else {
                                            
                                            if (!robot->GetVisionComponent().IsCameraCalibrationSet()) {
                                              PRINT_NAMED_INFO("CozmoEngine.CopyCamCalibToMfg.FailedBecauseCalibNotSet", "");
                                              return;
                                            }
                                            
                                            // Camera calibration was not found in manufacturing partition
                                            // Write it!
                                            Vision::CameraCalibration calib = robot->GetVisionComponent().GetCameraCalibration();
                                            payload.focalLength_x = calib.GetFocalLength_x();
                                            payload.focalLength_y = calib.GetFocalLength_y();
                                            payload.center_x = calib.GetCenter_x();
                                            payload.center_y = calib.GetCenter_y();
                                            payload.skew = calib.GetSkew();
                                            payload.nrows = calib.GetNrows();
                                            payload.ncols = calib.GetNcols();
                                            
                                            u8 buf[2*sizeof(payload)];
                                            size_t numBytes = payload.Pack(buf, sizeof(buf));
                                            
                                            PRINT_NAMED_INFO("CozmoEngine.CopyCamCalibToMfg.Copying",
                                                             "size: %dx%d, fx: %f, fy: %f, cx: %f cy: %f",
                                                             payload.ncols, payload.nrows,
                                                             payload.focalLength_x, payload.focalLength_y,
                                                             payload.center_x, payload.center_y);
                                            
                                            robot->GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_CameraCalib, buf, numBytes,
                                                                                 [](NVStorage::NVResult res) {
                                                                                   PRINT_NAMED_INFO("CozmoEngine.CopyCamCalibToMfg.Complete", "%s", EnumToString(res));
                                                                                 });
                                          }
                                          
                                        });
    
    
    // Setup Audio Server with Robot Audio Connection & Client
    using namespace Audio;
    AudioEngineMessageHandler* engineMessageHandler = new AudioEngineMessageHandler();
    AudioEngineClientConnection* engineConnection = new AudioEngineClientConnection( engineMessageHandler );
    // Transfer ownership of connection to Audio Server
    _context->GetAudioServer()->RegisterClientConnection( engineConnection );
    
    // Set Robot Audio Client Message Handler to link to Connection and Robot Audio Buffer ( Audio played on Robot )
    RobotAudioClient* audioClient = robot->GetRobotAudioClient();
    audioClient->SetMessageHandler( engineConnection->GetMessageHandler() );
    audioClient->SetAudioController( _context->GetAudioServer()->GetAudioController() );
  }
  
  return lastResult;
}
  
Robot* CozmoEngine::GetFirstRobot() {
  return _context->GetRobotManager()->GetFirstRobot();
}
  
int CozmoEngine::GetNumRobots() const {
  const size_t N = _context->GetRobotManager()->GetNumRobots();
  assert(N < INT_MAX);
  return static_cast<int>(N);
}
  
Robot* CozmoEngine::GetRobotByID(const RobotID_t robotID) {
  return _context->GetRobotManager()->GetRobotByID(robotID);
}

bool  CozmoEngine::HasRobotWithID(const RobotID_t robotID) const
{
  return _context->GetRobotManager()->DoesRobotExist(robotID);
}

void CozmoEngine::ListenForRobotConnections(bool listen)
{
  _isListeningForRobots = listen;
}

bool CozmoEngine::GetCurrentRobotImage(RobotID_t robotID, Vision::Image& img, TimeStamp_t newerThanTime)
{
  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  
  if(robot != nullptr) {
    return robot->GetCurrentImage(img, newerThanTime);
  } else {
    PRINT_NAMED_ERROR("CozmoEngine.GetCurrentRobotImage.InvalidRobotID",
                      "Image requested for invalid robot ID = %d.", robotID);
    return false;
  }
}

std::vector<RobotID_t> const& CozmoEngine::GetRobotIDList() const {
  return _context->GetRobotManager()->GetRobotIDList();
}

void CozmoEngine::SetImageSendMode(RobotID_t robotID, ImageSendMode newMode)
{
  Robot* robot = _context->GetRobotManager()->GetRobotByID(robotID);
  if(robot != nullptr) {
    return robot->SetImageSendMode(newMode);
  }
}
  
void CozmoEngine::SetRobotImageSendMode(RobotID_t robotID, ImageSendMode newMode, ImageResolution resolution)
{
  Robot* robot = GetRobotByID(robotID);
  
  if(robot != nullptr) {
    
    if (newMode == ImageSendMode::Off) {
      robot->GetBlockWorld().EnableDraw(false);
    } else if (newMode == ImageSendMode::Stream) {
      robot->GetBlockWorld().EnableDraw(true);
    }
    
    robot->SendRobotMessage<RobotInterface::ImageRequest>(newMode, resolution);
  }
  
}
  
void CozmoEngine::HandleGameEvents(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  switch (event.GetData().GetTag())
  {
    case ExternalInterface::MessageGameToEngineTag::ForceAddRobot:
    {
      const ExternalInterface::ForceAddRobot& msg = event.GetData().Get_ForceAddRobot();
      char ip[16];
      assert(msg.ipAddress.size() <= 16);
      std::copy(msg.ipAddress.begin(), msg.ipAddress.end(), ip);
      ForceAddRobot(msg.robotID, ip, msg.isSimulated);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::ConnectToRobot:
    {
      const ExternalInterface::ConnectToRobot& msg = event.GetData().Get_ConnectToRobot();
      const bool success = ConnectToRobot(msg.robotID);
      if(success) {
        PRINT_NAMED_INFO("CozmoEngine.HandleEvents", "Connected to robot %d!", msg.robotID);
      } else {
        PRINT_NAMED_ERROR("CozmoEngine.HandleEvents", "Failed to connect to robot %d!", msg.robotID);
      }
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::ReadAnimationFile:
    {
      ReadAnimationsFromDisk();
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::SetRobotImageSendMode:
    {
      const ExternalInterface::SetRobotImageSendMode& msg = event.GetData().Get_SetRobotImageSendMode();
      SetRobotImageSendMode(msg.robotID, msg.mode, msg.resolution);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::ImageRequest:
    {
      const ExternalInterface::ImageRequest& msg = event.GetData().Get_ImageRequest();
      SetImageSendMode(msg.robotID, msg.mode);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::StartTestMode:
    {
      const ExternalInterface::StartTestMode& msg = event.GetData().Get_StartTestMode();
      Robot* robot = GetRobotByID(msg.robotID);
      if(robot != nullptr) {
        robot->SendRobotMessage<StartControllerTestMode>(msg.p1, msg.p2, msg.p3, msg.mode);
      }
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::SetRobotVolume:
    {
      const ExternalInterface::SetRobotVolume& msg = event.GetData().Get_SetRobotVolume();
      Robot* robot = GetRobotByID(msg.robotId);
      if(robot != nullptr) {
        robot->GetRobotAudioClient()->SetRobotVolume(msg.volume);
      }
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::ReliableTransportRunMode:
    {
      const ExternalInterface::ReliableTransportRunMode& msg = event.GetData().Get_ReliableTransportRunMode();
      _robotChannel->SetReliableTransportRunMode(msg.isSync);
      break;
    }
    case ExternalInterface::MessageGameToEngineTag::SetEnableSOSLogging:
    {
      if(Anki::Util::gLoggerProvider != nullptr) {
        Anki::Util::MultiLoggerProvider* multiLoggerProvider = dynamic_cast<Anki::Util::MultiLoggerProvider*>(Anki::Util::gLoggerProvider);
        if (multiLoggerProvider != nullptr) {
          const std::vector<Anki::Util::ILoggerProvider*>& loggers = multiLoggerProvider->GetProviders();
          for(int i = 0; i < loggers.size(); ++i) {
            Anki::Util::SosLoggerProvider* sosLoggerProvider = dynamic_cast<Anki::Util::SosLoggerProvider*>(loggers[i]);
            if (sosLoggerProvider != nullptr) {
              sosLoggerProvider->OpenSocket();
              // disables tags for the SOS program
              sosLoggerProvider->SetSoSTagEncoding(false);
              break;
            }
          }

        }
      }
      break;
    }
    default:
    {
      PRINT_STREAM_ERROR("CozmoEngine.HandleEvents",
                         "Subscribed to unhandled event of type "
                         << ExternalInterface::MessageGameToEngineTagToString(event.GetData().GetTag()) << "!");
    }
  }
}

} // namespace Cozmo
} // namespace Anki
