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
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/audio/audioController.h"
#include "anki/cozmo/basestation/audio/audioEngineMessageHandler.h"
#include "anki/cozmo/basestation/audio/audioEngineClientConnection.h"
#include "anki/cozmo/basestation/ble/BLESystem.h"
#include "anki/cozmo/basestation/audio/audioServer.h"
#include "anki/cozmo/basestation/audio/audioUnityClientConnection.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/deviceData/deviceDataManager.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/util/transferQueue/transferQueueMgr.h"
#include "anki/cozmo/basestation/utils/cozmoFeatureGate.h"
#include "anki/cozmo/basestation/factory/factoryTestLogger.h"
#include "anki/cozmo/game/comms/uiMessageHandler.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/sosLoggerProvider.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/logging/multiLoggerProvider.h"
#include "util/time/universalTime.h"
#include "util/transport/connectionStats.h"
#include <cstdlib>

#if USE_DAS
#include <DAS/DAS.h>
#include <DAS/DASPlatform.h>
#endif

#if ANKI_DEV_CHEATS
#include "anki/cozmo/basestation/debug/usbTunnelEndServer_ios.h"
#endif

#define ENABLE_CE_SLEEP_TIME_DIAGNOSTICS 0
#define ENABLE_CE_RUN_TIME_DIAGNOSTICS 1

#define MIN_NUM_FACTORY_TEST_LOGS_FOR_ARCHIVING 100

namespace Anki {
namespace Cozmo {

CozmoEngine::CozmoEngine(Util::Data::DataPlatform* dataPlatform, GameMessagePort* messagePipe, GameMessagePort* vizMessagePort)
  : _uiMsgHandler(new UiMessageHandler(1, messagePipe))
  , _context(new CozmoContext(dataPlatform, _uiMsgHandler.get()))
  , _deviceDataManager(new DeviceDataManager(_uiMsgHandler.get()))
  // TODO:(lc) Once the BLESystem state machine has been implemented, create it here
  //, _bleSystem(new BLESystem())
#if ANKI_DEV_CHEATS && !defined(ANDROID)
  , _usbTunnelServerDebug(new USBTunnelServer(_uiMsgHandler.get(),dataPlatform))
#endif
{
  ASSERT_NAMED(_context->GetExternalInterface() != nullptr, "Cozmo.Engine.ExternalInterface.nullptr");
  if (Anki::Util::gTickTimeProvider == nullptr) {
    Anki::Util::gTickTimeProvider = BaseStationTimer::getInstance();
  }
  
  auto helper = MakeAnkiEventUtil(*_context->GetExternalInterface(), *this, _signalHandles);
  
  using namespace ExternalInterface;
  helper.SubscribeGameToEngine<MessageGameToEngineTag::ConnectToRobot>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::DisconnectFromRobot>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::ImageRequest>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::ReadAnimationFile>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::ResetFirmware>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestFeatureToggles>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SetEnableSOSLogging>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SetFeatureToggle>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SetGameBeingPaused>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SetRobotImageSendMode>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::StartEngine>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::StartTestMode>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::UpdateFirmware>();

  _debugConsoleManager.Init(_context->GetExternalInterface());

  #if VIZ_ON_DEVICE
  _context->GetVizManager()->SetMessagePort(vizMessagePort);
  #endif
}

CozmoEngine::~CozmoEngine()
{
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

  _context->GetDataLoader()->LoadData();
  _context->GetRobotManager()->Init(_config);

  return RESULT_OK;
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::StartEngine& msg)
{
  if (EngineState::Running == _engineState) {
    PRINT_NAMED_ERROR("CozmoEngine.HandleMessage.StartEngine.AlreadyStarted", "");
    return;
  }
  
  SetEngineState(EngineState::WaitingForUIDevices);
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::UpdateFirmware& msg)
{
  if (EngineState::UpdatingFirmware == _engineState)
  {
    PRINT_NAMED_WARNING("CozmoEngine.HandleMessage.UpdateFirmware.AlreadyStarted", "");
    return;
  }
  
  if (_context->GetRobotManager()->InitUpdateFirmware(msg.fwType, msg.version))
  {
    SetEngineState(EngineState::UpdatingFirmware);
  }
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::RequestFeatureToggles& msg)
{
  // collect feature list and send to UI
  ExternalInterface::FeatureToggles toggles;
  auto featureList = _context->GetFeatureGate()->GetFeatures();
  for (const auto& featurePair : featureList) {
    toggles.features.emplace_back(featurePair.first, featurePair.second);
  }
  
  _context->GetExternalInterface()->BroadcastToGame<ExternalInterface::FeatureToggles>(std::move(toggles));
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::SetFeatureToggle& message)
{
  _context->GetFeatureGate()->SetFeatureEnabled(message.feature, message.value);
}
  
template<>
void CozmoEngine::HandleMessage(const ExternalInterface::ConnectToRobot& connectMsg)
{
  if( CozmoEngine::HasRobotWithID(connectMsg.robotID)) {
    PRINT_NAMED_INFO("CozmoEngine.HandleMessage.ConnectToRobot.AlreadyConnected", "Robot %d already connected", connectMsg.robotID);
    return;
  }
  
  _context->GetRobotManager()->GetMsgHandler()->AddRobotConnection(connectMsg);
  
  // Another exception for hosts: have to tell the basestation to add the robot as well
  if(AddRobot(connectMsg.robotID) == RESULT_OK) {
    PRINT_NAMED_INFO("CozmoEngine.HandleMessage.ConnectToRobot.Success", "Connected to robot %d!", connectMsg.robotID);
  } else {
    PRINT_NAMED_ERROR("CozmoEngine.HandleMessage.ConnectToRobot.Fail", "Failed to connect to robot %d!", connectMsg.robotID);
  }
}
  
template<>
void CozmoEngine::HandleMessage(const ExternalInterface::ResetFirmware& msg)
{
  for (RobotID_t robotId : GetRobotIDList())
  {
    PRINT_NAMED_INFO("CozmoEngine.HandleMessage.ResetFirmware", "Sending KillBodyCode to Robot %d", robotId);
    _context->GetRobotManager()->GetMsgHandler()->SendMessage(robotId, RobotInterface::EngineToRobot(KillBodyCode()));
  }
}
  
template<>
void CozmoEngine::HandleMessage(const ExternalInterface::DisconnectFromRobot& disconnectMsg)
{
  if( !CozmoEngine::HasRobotWithID(disconnectMsg.robotID)) {
    PRINT_NAMED_INFO("CozmoEngine.HandleMessage.DisconnectFromRobot.AlreadyDisconnected", "Robot %d already disconnected", disconnectMsg.robotID);
    return;
  }
  
  _context->GetRobotManager()->GetMsgHandler()->Disconnect();
  PRINT_NAMED_INFO("CozmoEngine.HandleMessage.DisconnectFromRobot", "Disconnected from robot %d!", disconnectMsg.robotID);
}

Result CozmoEngine::Update(const float currTime_sec)
{
  ANKI_CPU_PROFILE("CozmoEngine::Update");
  
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
        SendSupportInfo();
        SetEngineState(EngineState::Running);
      }
      break;
    }
    case EngineState::Running:
    {
      // Update time
      BaseStationTimer::getInstance()->UpdateTime(SEC_TO_NANOS(currTime_sec));
      
      _context->GetRobotManager()->UpdateRobotConnection();
      
      // Let the robot manager do whatever it's gotta do to update the
      // robots in the world.
      _context->GetRobotManager()->UpdateAllRobots();
      
      UpdateLatencyInfo();

      break;
    }
    case EngineState::UpdatingFirmware:
    {
      // Update comms and messages from robot
      _context->GetRobotManager()->UpdateRobotConnection();
      
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
  
  // Tick Audio Controller after all messages have been processed
  const auto audioServer = _context->GetAudioServer();
  if (audioServer != nullptr) {
    audioServer->UpdateAudioController();
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
  
#if REMOTE_CONSOLE_ENABLED
void PrintTimingInfoStats(const ExternalInterface::TimingInfo& timingInfo, const char* name)
{
  PRINT_CH_INFO("UiComms", "CozmoEngine.LatencyStats", "%s: = %f (%f..%f)", name, timingInfo.avgTime_ms, timingInfo.minTime_ms, timingInfo.maxTime_ms);
}
  
void PrintTimingInfoStats(const ExternalInterface::CurrentTimingInfo& timingInfo, const char* name)
{
  PRINT_CH_INFO("UiComms", "CozmoEngine.LatencyStats", "%s: = %f (%f..%f) (curr: %f)", name, timingInfo.avgTime_ms, timingInfo.minTime_ms, timingInfo.maxTime_ms, timingInfo.currentTime_ms);
}
CONSOLE_VAR(bool, kLogMessageLatencyOnce, "Network.Stats", false);
#endif // REMOTE_CONSOLE_ENABLED


void CozmoEngine::UpdateLatencyInfo()
{
  if (Util::kNetConnStatsUpdate)
  {
    // If the game is paused, don't bother sending latency info
    if (_isGamePaused)
    {
      return;
    }
    
    // We only want to send latency info every N ticks
    constexpr int kTickSendFrequency = 10;
    static int currentTickCount = kTickSendFrequency;
    if (0 != currentTickCount)
    {
      currentTickCount--;
      return;
    }
    currentTickCount = kTickSendFrequency;
    
    ExternalInterface::TimingInfo wifiLatency(Util::gNetStat2LatencyAvg, Util::gNetStat4LatencyMin, Util::gNetStat5LatencyMax);
    ExternalInterface::TimingInfo extSendQueueTime(Util::gNetStat7ExtQueuedAvg_ms, Util::gNetStat8ExtQueuedMin_ms, Util::gNetStat9ExtQueuedMax_ms);
    ExternalInterface::TimingInfo sendQueueTime(Util::gNetStatAQueuedAvg_ms, Util::gNetStatBQueuedMin_ms, Util::gNetStatCQueuedMax_ms);
    const Util::Stats::StatsAccumulator& queuedTimes_ms = _context->GetRobotManager()->GetMsgHandler()->GetQueuedTimes_ms();
    ExternalInterface::TimingInfo recvQueueTime(queuedTimes_ms.GetMean(), queuedTimes_ms.GetMin(), queuedTimes_ms.GetMax());

    // pull image stats from robot if available
    Util::Stats::StatsAccumulator nullStats;
    const Robot* firstRobot = GetFirstRobot();
    const bool useRobotStats = firstRobot != nullptr;
    const Util::Stats::StatsAccumulator& imageStats = useRobotStats ? firstRobot->GetImageStats() : nullStats;
    double currentImageDelay = useRobotStats ? firstRobot->GetCurrentImageDelay() : 0.0;
    
    const Util::Stats::StatsAccumulator& unityLatency = _uiMsgHandler->GetLatencyStats(UiConnectionType::UI);
    const Util::Stats::StatsAccumulator& sdk1Latency  = _uiMsgHandler->GetLatencyStats(UiConnectionType::SdkOverUdp);
    const Util::Stats::StatsAccumulator& sdk2Latency  = _uiMsgHandler->GetLatencyStats(UiConnectionType::SdkOverTcp);
    const Util::Stats::StatsAccumulator& sdkLatency = (sdk1Latency.GetNumDbl() > sdk2Latency.GetNumDbl()) ? sdk1Latency : sdk2Latency;
    ExternalInterface::TimingInfo unityEngineLatency(unityLatency.GetMean(), unityLatency.GetMin(), unityLatency.GetMax());
    ExternalInterface::TimingInfo sdkEngineLatency(sdkLatency.GetMean(), sdkLatency.GetMin(), sdkLatency.GetMax());
    ExternalInterface::CurrentTimingInfo imageLatency(imageStats.GetMean(), imageStats.GetMin(), imageStats.GetMax(), currentImageDelay * 1000.0);
    
    ExternalInterface::MessageEngineToGame debugLatencyMessage(ExternalInterface::LatencyMessage(
                                                                                     std::move(wifiLatency),
                                                                                     std::move(extSendQueueTime),
                                                                                     std::move(sendQueueTime),
                                                                                     std::move(recvQueueTime),
                                                                                     std::move(unityEngineLatency),
                                                                                     std::move(sdkEngineLatency),
                                                                                     std::move(imageLatency) ));

    #if REMOTE_CONSOLE_ENABLED
    if (kLogMessageLatencyOnce)
    {
      PrintTimingInfoStats(wifiLatency,      "wifi");
      PrintTimingInfoStats(extSendQueueTime, "extSendQueue");
      PrintTimingInfoStats(sendQueueTime,    "sendQueue");
      PrintTimingInfoStats(recvQueueTime,    "recvQueue");
      if (unityLatency.GetNumDbl() > 0.0)
      {
        PrintTimingInfoStats(unityEngineLatency, "unity");
      }
      if (sdkLatency.GetNumDbl() > 0.0)
      {
        PrintTimingInfoStats(sdkEngineLatency, "sdk");
      }
      
      PrintTimingInfoStats(imageLatency, "image");
      
      kLogMessageLatencyOnce = false;
    }
    #endif // REMOTE_CONSOLE_ENABLED
    
    _context->GetExternalInterface()->Broadcast( std::move(debugLatencyMessage) );
  }
}

void CozmoEngine::SendSupportInfo() const
{
  #if USE_DAS
  const DAS::IDASPlatform* platform = DASGetPlatform();
  if (platform != nullptr) {
    _context->GetExternalInterface()->BroadcastToGame<ExternalInterface::SupportInfo>(platform->GetDeviceId());
  }
  #endif
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
  
  Anki::Util::sEventF("app.engine.state", {{DDATA,EngineStateToString(newState)}}, "%s", EngineStateToString(oldState));
}
  
Result CozmoEngine::InitInternal()
{
  // Setup Audio Controller
  using namespace Audio;
  
  // Setup Unity Audio Client Connections
  AudioUnityClientConnection *unityConnection = new AudioUnityClientConnection( *_context->GetExternalInterface() );
  _context->GetAudioServer()->RegisterClientConnection( unityConnection );
  
  // Archive factory test logs
  FactoryTestLogger factoryTestLogger;
  u32 numLogs = factoryTestLogger.GetNumLogs(_context->GetDataPlatform());
  if (numLogs >= MIN_NUM_FACTORY_TEST_LOGS_FOR_ARCHIVING) {
    if (factoryTestLogger.ArchiveLogs(_context->GetDataPlatform())) {
      PRINT_NAMED_INFO("CozmoEngine.InitInternal.ArchivedFactoryLogs", "%d logs archived", numLogs);
    } else {
      PRINT_NAMED_WARNING("CozmoEngine.InitInternal.ArchivedFactoryLogsFailed", "");
    }
  }
  
  return RESULT_OK;
}
  
  
void CozmoEngine::ReadCameraCalibration(Robot* robot)
{
  NVStorageComponent::NVStorageReadCallback readCamCalibCallback = [this,robot](u8* data, size_t size, NVStorage::NVResult res) {
    
    if (res == NVStorage::NVResult::NV_OKAY) {
      CameraCalibration payload;
      
      if (size != NVStorageComponent::MakeWordAligned(payload.Size())) {
        PRINT_NAMED_WARNING("CozmoEngine.ReadCameraCalibration.SizeMismatch",
                            "Expected %zu, got %zu",
                            NVStorageComponent::MakeWordAligned(payload.Size()), size);
      } else {
        
        payload.Unpack(data, size);
        PRINT_NAMED_INFO("CozmoEngine.ReadCameraCalibration.Recvd",
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
                                        payload.skew,
                                        payload.distCoeffs);
        
        robot->GetVisionComponent().SetCameraCalibration(calib);
      }
    } else {
      PRINT_NAMED_WARNING("CozmoEngine.ReadCameraCalibration.Failed", "");
    }
    
    robot->GetVisionComponent().Enable(true);
  };
  
  robot->GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_CameraCalib, readCamCalibCallback);
  
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
    
    // Requesting camera calibration
    ReadCameraCalibration(robot);
    
    // Setup Audio Server with Robot Audio Connection & Client
    using namespace Audio;
    AudioEngineMessageHandler* engineMessageHandler = new AudioEngineMessageHandler();
    AudioEngineClientConnection* engineConnection = new AudioEngineClientConnection( engineMessageHandler );
    // Transfer ownership of connection to Audio Server
    _context->GetAudioServer()->RegisterClientConnection( engineConnection );
    
    // Set Robot Audio Client Message Handler to link to Connection and Robot Audio Buffer ( Audio played on Robot )
    robot->GetRobotAudioClient()->SetMessageHandler( engineConnection->GetMessageHandler() );
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

std::vector<RobotID_t> const& CozmoEngine::GetRobotIDList() const {
  return _context->GetRobotManager()->GetRobotIDList();
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::ReadAnimationFile& msg)
{
  _context->GetRobotManager()->ReadAnimationDir();
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::SetRobotImageSendMode& msg)
{
  const RobotID_t robotID = msg.robotID;
  const ImageSendMode newMode = msg.mode;
  const ImageResolution resolution = msg.resolution;
  Robot* robot = GetRobotByID(robotID);
  
  if(robot != nullptr) {
    
    if (newMode == ImageSendMode::Off) {
      robot->GetBlockWorld().EnableDraw(false);
    } else if (newMode == ImageSendMode::Stream) {
      robot->GetBlockWorld().EnableDraw(true);
    }
    robot->SetImageSendMode(newMode);
    robot->SendRobotMessage<RobotInterface::ImageRequest>(newMode, resolution);
  }
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::ImageRequest& msg)
{
  Robot* robot = _context->GetRobotManager()->GetRobotByID(msg.robotID);
  if(robot != nullptr) {
    return robot->SetImageSendMode(msg.mode);
  }
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::StartTestMode& msg)
{
  Robot* robot = GetRobotByID(msg.robotID);
  if(robot != nullptr) {
    robot->SendRobotMessage<StartControllerTestMode>(msg.p1, msg.p2, msg.p3, msg.mode);
  }
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::SetEnableSOSLogging& msg)
{
#if ANKI_DEV_CHEATS
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
#endif //ANKI_DEV_CHEATS
}
  
template<>
void CozmoEngine::HandleMessage(const ExternalInterface::SetGameBeingPaused& msg)
{
  _isGamePaused = msg.isPaused;
  
  // Update Audio
  if (nullptr != _context->GetAudioServer()) {
    _context->GetAudioServer()->GetAudioController()->AppIsInFocus(!_isGamePaused);
  }
}

void CozmoEngine::ExecuteBackgroundTransfers()
{
  _context->GetTransferQueue()->ExecuteTransfers();
}

} // namespace Cozmo
} // namespace Anki
