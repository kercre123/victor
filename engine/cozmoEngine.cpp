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

#include "engine/cozmoEngine.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/events/ankiEvent.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/ankiEventUtil.h"
#include "engine/ble/BLESystem.h"
#include "engine/debug/cladLoggerProvider.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/components/visionComponent.h"
#include "engine/deviceData/deviceDataManager.h"
#include "engine/perfMetric.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/utils/parsingConstants/parsingConstants.h"
#include "engine/viz/vizManager.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotManager.h"
#include "engine/util/transferQueue/transferQueueMgr.h"
#include "engine/utils/cozmoExperiments.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/voiceCommands/voiceCommandComponent.h"
#include "engine/cozmoAPI/comms/uiMessageHandler.h"
// #include "webServerProcess/src/webService.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "osState/osState.h"

#include "clad/externalInterface/messageGameToEngine.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/global/globalDefinitions.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/logging/multiLoggerProvider.h"
#include "util/time/universalTime.h"
#include "util/environment/locale.h"
#include "util/transport/connectionStats.h"

#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>

#if USE_DAS
#include <DAS/DAS.h>
#include <DAS/DASPlatform.h>
#endif

#include "engine/animations/animationTransfer.h"

#define LOG_CHANNEL "CozmoEngine"

#if ANKI_PROFILING_ENABLED
  #define ENABLE_CE_SLEEP_TIME_DIAGNOSTICS 0
  #define ENABLE_CE_RUN_TIME_DIAGNOSTICS 1
#else
  #define ENABLE_CE_SLEEP_TIME_DIAGNOSTICS 0
  #define ENABLE_CE_RUN_TIME_DIAGNOSTICS 0
#endif

#define MIN_NUM_FACTORY_TEST_LOGS_FOR_ARCHIVING 100

namespace Anki {
namespace Cozmo {

CozmoEngine::CozmoEngine(Util::Data::DataPlatform* dataPlatform, GameMessagePort* messagePipe)
  : _uiMsgHandler(new UiMessageHandler(1, messagePipe))
  , _context(new CozmoContext(dataPlatform, _uiMsgHandler.get()))
  , _deviceDataManager(new DeviceDataManager(_uiMsgHandler.get()))
  // TODO:(lc) Once the BLESystem state machine has been implemented, create it here
  //, _bleSystem(new BLESystem())
  ,_animationTransferHandler(new AnimationTransfer(_uiMsgHandler.get(),dataPlatform))
{
  DEV_ASSERT(_context->GetExternalInterface() != nullptr, "Cozmo.Engine.ExternalInterface.nullptr");
  if (Anki::Util::gTickTimeProvider == nullptr) {
    Anki::Util::gTickTimeProvider = BaseStationTimer::getInstance();
  }

  // The "main" thread is meant to be the one Update is run on. However, on some systems, some messaging
  // happens during Init which is on one thread, then later, a different thread runs the updates. This will
  // trigger asserts because more than one thread is sending messages. To work around this, we consider the
  // "main thread" to be whatever thread Init is called from, until the first call of Update, at which point
  // we switch our notion of "main thread" to the updating thread
  _context->SetMainThread();

  // log additional das info
  LOG_EVENT("device.language_locale", "%s", _context->GetLocale()->GetLocaleString().c_str());
  std::time_t t = std::time(nullptr);
  std::tm tm = *std::localtime(&t);
  std::stringstream timeZoneString;
  timeZoneString << std::put_time(&tm, "%Z");
  std::stringstream timeZoneOffset;
  timeZoneOffset << std::put_time(&tm, "%z");
  Util::sEventF("device.timezone", {{DDATA, timeZoneOffset.str().c_str()}}, "%s", timeZoneString.str().c_str());

  auto helper = MakeAnkiEventUtil(*_context->GetExternalInterface(), *this, _signalHandles);

  using namespace ExternalInterface;
  helper.SubscribeGameToEngine<MessageGameToEngineTag::ImageRequest>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::ReadFaceAnimationDir>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::RedirectViz>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::ResetFirmware>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestFeatureToggles>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SetFeatureToggle>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SetGameBeingPaused>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::SetRobotImageSendMode>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::StartTestMode>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::UpdateFirmware>();
  helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestLocale>();

  auto handler = [this] (const std::vector<Util::AnkiLab::AssignmentDef>& assignments) {
    _context->GetExperiments()->UpdateLabAssignments(assignments);
  };
  _signalHandles.emplace_back(_context->GetExperiments()->GetAnkiLab()
                              .ActiveAssignmentsUpdatedSignal().ScopedSubscribe(handler));

  _debugConsoleManager.Init(_context->GetExternalInterface(), _context->GetRobotManager()->GetMsgHandler());
  _dasToSdkHandler.Init(_context->GetExternalInterface());
  InitUnityLogger();
}

CozmoEngine::~CozmoEngine()
{
  if (Anki::Util::gTickTimeProvider == BaseStationTimer::getInstance()) {
    Anki::Util::gTickTimeProvider = nullptr;
  }
  BaseStationTimer::removeInstance();

  _context->GetVizManager()->Disconnect();
  _context->Shutdown();
}

Result CozmoEngine::Init(const Json::Value& config) {

  if (_isInitialized) {
    LOG_INFO("CozmoEngine.Init.ReInit", "Reinitializing already-initialized CozmoEngineImpl with new config.");
  }

  _isInitialized = false;

  // Engine currently has no reason to know about CPU
  // freq or temperature so we set the update period to 0
  // avoid time-wasting file access
  OSState::getInstance()->SetUpdatePeriod(0);

  _config = config;

  if(!_config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP)) {
    PRINT_NAMED_ERROR("CozmoEngine.Init", "No AdvertisingHostIP defined in Json config.");
    return RESULT_FAIL;
  }

  if(!_config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
    PRINT_NAMED_ERROR("CozmoEngine.Init", "No UiAdvertisingPort defined in Json config.");
    return RESULT_FAIL;
  }

  Result lastResult = _uiMsgHandler->Init(_context.get(), _config);
  if (RESULT_OK != lastResult)
  {
    PRINT_NAMED_ERROR("CozmoEngine.Init","Error initializing UIMessageHandler");
    return lastResult;
  }

  // Disable Viz entirely on shipping builds
  if(ANKI_DEV_CHEATS)
  {
    if (nullptr != _context->GetExternalInterface())
    {
      // Have VizManager subscribe to the events it should care about
      _context->GetVizManager()->SubscribeToEngineEvents(*_context->GetExternalInterface());
    }
  }

  lastResult = InitInternal();
  if(lastResult != RESULT_OK) {
    PRINT_NAMED_ERROR("CozmoEngine.Init", "Failed calling internal init.");
    return lastResult;
  }

  _context->GetDataLoader()->LoadRobotConfigs();

  _context->GetExperiments()->InitExperiments();

  _context->GetRobotManager()->Init(_config);

  // TODO: Specify random seed from config?
  //       Setting to non-zero value for now for repeatable testing.
  _context->SetRandomSeed(1);

  // VIC-722: Set this automatically using location services
  //          and/or set from UI/SDK?
  _context->SetLocale("en-US");

  _context->GetPerfMetric()->Init();

  LOG_INFO("CozmoEngine.Init.Version", "2");

  SetEngineState(EngineState::LoadingData);

  // _context->GetWebService()->Start(_context->GetDataPlatform(),
  //                                  _context->GetDataLoader()->GetWebServerEngineConfig());

  // DAS Event: "cozmo_engine.init.build_configuration"
  // s_val: Build configuration
  // data: Unused
  Anki::Util::sEvent("cozmo_engine.init.build_configuration", {},
#if defined(DEBUG)
                     "DEBUG");
#elif defined(RELEASE)
                     "RELEASE");
#elif defined(PROFILE)
                     "PROFILE");
#elif defined(SHIPPING)
                     "SHIPPING");
#else
                     "UNKNOWN");
#endif

  _isInitialized = true;

  return RESULT_OK;
}


template<>
void CozmoEngine::HandleMessage(const ExternalInterface::UpdateFirmware& msg)
{
  if (EngineState::UpdatingFirmware == _engineState)
  {
    PRINT_NAMED_WARNING("CozmoEngine.HandleMessage.UpdateFirmware.AlreadyStarted", "");
    return;
  }

  // TODO: figure out how to update firmware for Victor
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
void CozmoEngine::HandleMessage(const ExternalInterface::ResetFirmware& msg)
{
  PRINT_NAMED_WARNING("CozmoEngine.HandleMessage.ResetFirmwareUndefined", "What does this mean for Victor?");
}

Result CozmoEngine::Update(const BaseStationTime_t currTime_nanosec)
{
  ANKI_CPU_PROFILE("CozmoEngine::Update");

  if(!_isInitialized) {
    PRINT_NAMED_ERROR("CozmoEngine.Update", "Cannot update CozmoEngine before it is initialized.");
    return RESULT_FAIL;
  }

  // This is a bit of a hack, but on some systems the thread that calls Update is different from the thread
  // that does all of the setup. This flag assures that we set the "main" thread to be the one that's going to
  // be doing the updating
  if( !_hasRunFirstUpdate ) {
    _context->SetMainThread();
    _hasRunFirstUpdate = true;
  }

  DEV_ASSERT( _context->IsMainThread(), "CozmoEngine.EngineNotOnMainThread" );

#if ENABLE_CE_SLEEP_TIME_DIAGNOSTICS || ENABLE_CE_RUN_TIME_DIAGNOSTICS
  const double startUpdateTimeMs = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
#endif
#if ENABLE_CE_SLEEP_TIME_DIAGNOSTICS
  {
    static bool firstUpdate = true;
    static double lastUpdateTimeMs = 0.0;
    //const double currentTimeMs = (double)currTime_nanosec / 1e+6;
    if (! firstUpdate)
    {
      const double timeSinceLastUpdate = startUpdateTimeMs - lastUpdateTimeMs;
      const double maxLatency = BS_TIME_STEP_MS + 15.;
      if (timeSinceLastUpdate > maxLatency)
      {
        Anki::Util::sEventF("cozmo_engine.update.sleep.slow", {{DDATA,TO_DDATA_STR(BS_TIME_STEP_MS)}}, "%.2f", timeSinceLastUpdate);
      }
    }
    lastUpdateTimeMs = startUpdateTimeMs;
    firstUpdate = false;
  }
#endif // ENABLE_CE_SLEEP_TIME_DIAGNOSTICS

  _uiMsgHandler->ResetMessageCounts();
  _context->GetRobotManager()->GetMsgHandler()->ResetMessageCounts();
  _context->GetVizManager()->ResetMessageCount();
  
  _context->GetVoiceCommandComponent()->Update();

  // _context->GetWebService()->Update();

  // Handle UI
  if (!_uiWasConnected && _uiMsgHandler->HasDesiredNumUiDevices()) {
    LOG_INFO("CozmoEngine.Update.UIConnected", "UI has connected");
    SendSupportInfo();

    if (_engineState == EngineState::Running) {
      _context->GetExternalInterface()->BroadcastToGame<ExternalInterface::EngineLoadingDataStatus>(1.f);
    }

    _uiWasConnected = true;
  } else if (_uiWasConnected && !_uiMsgHandler->HasDesiredNumUiDevices()) {
    LOG_INFO("CozmoEngine.Update.UIDisconnected", "UI has disconnected");
    _uiWasConnected = false;
  }

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
    case EngineState::LoadingData:
    {
      float currentLoadingDone = 0.0f;
      if (_context->GetDataLoader()->DoNonConfigDataLoading(currentLoadingDone))
      {
        SetEngineState(EngineState::ConnectingToRobot);
      }
      LOG_INFO("CozmoEngine.Update.LoadingRatio", "%f", currentLoadingDone);
      _context->GetExternalInterface()->BroadcastToGame<ExternalInterface::EngineLoadingDataStatus>(currentLoadingDone);
      break;
    }
    case EngineState::ConnectingToRobot:
    {
      // Wait for robot process to start up and become ready
      Result result = ConnectToRobotProcess();
      if (RESULT_OK != result) {
        LOG_WARNING("CozmoEngine.Update.ConnectingToRobot", "Unable to connect to robot (result %d)", result);
        break;
      }
      LOG_INFO("CozmoEngine.Update.ConnectingToRobot", "Now connected to robot");
      SetEngineState(EngineState::Running);
      break;
    }
    case EngineState::UpdatingFirmware:
    {
      // TODO: VictorFirmwareUpdate
      SetEngineState(EngineState::Running);

      // deliberate fallthrough because we
      // do not handle updating firmware in
      // victor yet
    }
    case EngineState::Running:
    {
      // Update time
      BaseStationTimer::getInstance()->UpdateTime(currTime_nanosec);

      // Update OSState
      OSState::getInstance()->Update();

      Result result = _context->GetRobotManager()->UpdateRobotConnection();
      if (RESULT_OK != result) {
        LOG_ERROR("CozmoEngine.Update.Running", "Unable to update robot connection (result %d)", result);
        return result;
      }

      // Let the robot manager do whatever it's gotta do to update the
      // robots in the world.
      result = _context->GetRobotManager()->UpdateRobot();
      if(result != RESULT_OK)
      {
        LOG_WARNING("CozmoEngine.Update.UpdateRobotFailed", "Update robot failed with %d", result);
        return result;
      }
      
      UpdateLatencyInfo();
      break;
    }
    default:
      PRINT_NAMED_ERROR("CozmoEngine.Update.UnexpectedState","Running Update in an unexpected state!");
  }

#if ENABLE_CE_RUN_TIME_DIAGNOSTICS
  {
    const double endUpdateTimeMs = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
    const double updateLengthMs = endUpdateTimeMs - startUpdateTimeMs;
    const double maxUpdateDuration = BS_TIME_STEP_MS;
    if (updateLengthMs > maxUpdateDuration)
    {
      static const std::string targetMs = std::to_string(BS_TIME_STEP_MS);
      Anki::Util::sEventF("cozmo_engine.update.run.slow",
                          {{DDATA, targetMs.c_str()}},
                          "%.2f", updateLengthMs);
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
    static const Util::Stats::StatsAccumulator nullStats;
    const Robot* robot = GetRobot();
    const bool useRobotStats = robot != nullptr;
    const Util::Stats::StatsAccumulator& imageStats = useRobotStats ? robot->GetImageStats() : nullStats;
    double currentImageDelay = useRobotStats ? robot->GetCurrentImageDelay() : 0.0;

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

  // clear the first update flag
  _hasRunFirstUpdate = false;

  return RESULT_OK;
}

Result CozmoEngine::ConnectToRobotProcess()
{
  const RobotID_t robotID = OSState::getInstance()->GetRobotID();

  auto * robotManager = _context->GetRobotManager();
  if (!robotManager->DoesRobotExist(robotID)) {
    robotManager->AddRobot(robotID);
  }

  auto * msgHandler = robotManager->GetMsgHandler();
  if (!msgHandler->IsConnected(robotID)) {
    Result result = msgHandler->AddRobotConnection(robotID);
    if (RESULT_OK != result) {
      LOG_WARNING("CozmoEngine.ConnectToRobotProcess", "Unable to connect to robot %d (result %d)", robotID, result);
      return result;
    }
  }

  return RESULT_OK;
}

Robot* CozmoEngine::GetRobot() {
  return _context->GetRobotManager()->GetRobot();
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::ReadFaceAnimationDir& msg)
{
  // TODO: Tell animation process to read the anim dir?
  PRINT_NAMED_WARNING("CozmoEngine.HandleMessage.ReadFaceAnimationDir.NotHookedUp", "");
  //_context->GetRobotManager()->ReadFaceAnimationDir();
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::SetRobotImageSendMode& msg)
{
  const ImageSendMode newMode = msg.mode;
  Robot* robot = GetRobot();

  if(robot != nullptr) {
    robot->SetImageSendMode(newMode);
    // TODO: Can get rid of one of ExternalInterface::SetRobotImageSendMode or
    // ExternalInterfaceImageRequest since they seem to do the same thing now.
  }
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::ImageRequest& msg)
{
  Robot* robot = GetRobot();
  if(robot != nullptr) {
    return robot->SetImageSendMode(msg.mode);
  }
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::StartTestMode& msg)
{
  Robot* robot = GetRobot();
  if(robot != nullptr) {
    robot->SendRobotMessage<StartControllerTestMode>(msg.p1, msg.p2, msg.p3, msg.mode);
  }
}

void CozmoEngine::InitUnityLogger()
{
#if ANKI_DEV_CHEATS
  if(Anki::Util::gLoggerProvider != nullptr) {
    Anki::Util::MultiLoggerProvider* multiLoggerProvider = dynamic_cast<Anki::Util::MultiLoggerProvider*>(Anki::Util::gLoggerProvider);
    if (multiLoggerProvider != nullptr) {
      const std::vector<Anki::Util::ILoggerProvider*>& loggers = multiLoggerProvider->GetProviders();
      for(int i = 0; i < loggers.size(); ++i) {
        CLADLoggerProvider* unityLoggerProvider = dynamic_cast<CLADLoggerProvider*>(loggers[i]);
        if (unityLoggerProvider != nullptr) {
          unityLoggerProvider->SetExternalInterface(_context->GetExternalInterface());
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
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::RequestLocale& msg)
{
  _context->GetExternalInterface()->BroadcastToGame<ExternalInterface::ResponseLocale>(
                                                    _context->GetLocale()->GetLocaleStringLowerCase());
}

template<>
void CozmoEngine::HandleMessage(const ExternalInterface::RequestDataCollectionOption& msg)
{
#if USE_DAS
  if( msg.CollectionEnabled )
  {
    DASEnableNetwork(DASDisableNetworkReason_UserOptOut);
  }
  else
  {
    DASDisableNetwork(DASDisableNetworkReason_UserOptOut);
  }
#endif
}


template<>
void CozmoEngine::HandleMessage(const ExternalInterface::RedirectViz& msg)
{
  // Disable viz in shipping
  if(ANKI_DEV_CHEATS) {
    const uint8_t* ipBytes = (const uint8_t*)&msg.ipAddr;
    std::ostringstream ss;
    ss << (int)ipBytes[0] << "." << (int)ipBytes[1] << "." << (int)ipBytes[2] << "." << (int)ipBytes[3];
    std::string ipAddr = ss.str();
    PRINT_NAMED_INFO("CozmoEngine.RedirectViz.ipAddr", "%s", ipAddr.c_str());

    _context->GetVizManager()->Disconnect();
    _context->GetVizManager()->Connect(ipAddr.c_str(),
                                      (uint16_t)VizConstants::VIZ_SERVER_PORT,
                                      ipAddr.c_str(),
                                      (uint16_t)VizConstants::UNITY_VIZ_SERVER_PORT);
    _context->GetVizManager()->EnableImageSend(true);

    // Erase anything that's still being visualized in case there were leftovers from
    // a previous run?? (We should really be cleaning up after ourselves when
    // we tear down, but it seems like Webots restarts aren't always allowing
    // the cleanup to happen)
    _context->GetVizManager()->EraseAllVizObjects();
  }
}


//void CozmoEngine::ExecuteBackgroundTransfers()
//{
//  _context->GetTransferQueue()->ExecuteTransfers();
//}

Util::AnkiLab::AssignmentStatus CozmoEngine::ActivateExperiment(
  const Util::AnkiLab::ActivateExperimentRequest& request, std::string& outVariationKey)
{
  return _context->GetExperiments()->ActivateExperiment(request, outVariationKey);
}

void CozmoEngine::RegisterEngineTickPerformance(const float tickDuration_ms,
                                                const float tickFrequency_ms,
                                                const float sleepDurationIntended_ms,
                                                const float sleepDurationActual_ms) const
{
  // Send two of these stats to the game for on-screen display
  ExternalInterface::MessageEngineToGame perfEngMsg(ExternalInterface::DebugPerformanceTick(
                                                    "Engine", tickDuration_ms));
  _context->GetExternalInterface()->Broadcast(std::move(perfEngMsg));

  ExternalInterface::MessageEngineToGame perfEngFreqMsg(ExternalInterface::DebugPerformanceTick(
                                                    "EngineFreq", tickFrequency_ms));
   _context->GetExternalInterface()->Broadcast(std::move(perfEngFreqMsg));

  // Update the PerfMetric system for end of tick
  _context->GetPerfMetric()->Update(tickDuration_ms, tickFrequency_ms,
                                    sleepDurationIntended_ms, sleepDurationActual_ms);
}

} // namespace Cozmo
} // namespace Anki
