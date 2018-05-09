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
#include "engine/components/batteryComponent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/events/ankiEvent.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/ankiEventUtil.h"
#include "engine/debug/cladLoggerProvider.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/deviceData/deviceDataManager.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/mics/micDirectionHistory.h"
#include "engine/perfMetric.h"
#include "engine/utils/parsingConstants/parsingConstants.h"
#include "engine/viz/vizManager.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotManager.h"
#include "engine/util/transferQueue/transferQueueMgr.h"
#include "engine/utils/cozmoExperiments.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "engine/factory/factoryTestLogger.h"
#include "engine/cozmoAPI/comms/uiMessageHandler.h"
#include "webServerProcess/src/webService.h"

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

// Local state variables
namespace {

  // How often do we attempt connection to robot/anim process?
  constexpr auto connectInterval = std::chrono::seconds(1);

  // When did we last try connecting?
  auto lastConnectAttempt = std::chrono::steady_clock::time_point();

}

namespace Anki {
namespace Cozmo {

int GetEngineStatsWebServerHandler(struct mg_connection *conn, void *cbdata)
{
  // We ignore the query string because overhead is minimal
  auto* cozmoEngine = static_cast<CozmoEngine*>(cbdata);
  if (cozmoEngine->GetEngineState() != EngineState::Running)
  {
    LOG_INFO("CozmoEngine.GetEngineStatsWebServerHandler.NotReady", "GetEngineStatsWebServerHandler called but engine not running");
    return 0;
  }
  const auto& robot = cozmoEngine->GetRobot();

  mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: "
            "close\r\n\r\n");

  const auto& batteryComponent = robot->GetBatteryComponent();
  std::stringstream ss;
  ss << std::fixed << std::setprecision(3) << batteryComponent.GetBatteryVolts();
  const auto& stat_batteryVoltsFiltered = ss.str();
  std::stringstream ss2;
  ss2 << std::fixed << std::setprecision(3) << batteryComponent.GetBatteryVoltsRaw();
  const auto& stat_batteryVoltsRaw = ss2.str();
  const std::string& stat_batteryLevel = EnumToString(batteryComponent.GetBatteryLevel());
  const std::string& stat_batteryIsCharging = batteryComponent.IsCharging() ? "true" : "false";
  const std::string& stat_batteryIsOnChargerContacts = batteryComponent.IsOnChargerContacts() ? "true" : "false";
  const std::string& stat_batteryIsOnChargerPlatform = batteryComponent.IsOnChargerPlatform() ? "true" : "false";
  const auto& stat_batteryFullyChargedTime_s = std::to_string(static_cast<int>(batteryComponent.GetFullyChargedTimeSec()));
  const auto& stat_batteryLowBatteryTime_s = std::to_string(static_cast<int>(batteryComponent.GetLowBatteryTimeSec()));

  mg_printf(conn, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
            stat_batteryVoltsFiltered.c_str(), stat_batteryVoltsRaw.c_str(),
            stat_batteryLevel.c_str(), stat_batteryIsCharging.c_str(),
            stat_batteryIsOnChargerContacts.c_str(), stat_batteryIsOnChargerPlatform.c_str(),
            stat_batteryFullyChargedTime_s.c_str(), stat_batteryLowBatteryTime_s.c_str());

  const auto& robotState = robot->GetRobotState();

  std::stringstream ss3;
  ss3 << std::fixed << std::setprecision(1) << RAD_TO_DEG(robotState.poseAngle_rad);
  const auto& stat_poseAngle_rad = ss3.str();
  std::stringstream ss4;
  ss4 << std::fixed << std::setprecision(1) << RAD_TO_DEG(robotState.posePitch_rad);
  const auto& stat_posePitch_rad = ss4.str();
  std::stringstream ss5;
  ss5 << std::fixed << std::setprecision(1) << RAD_TO_DEG(robotState.headAngle_rad);
  const auto& stat_headAngle_rad = ss5.str();
  std::stringstream ss6;
  ss6 << std::fixed << std::setprecision(3) << robotState.liftHeight_mm;
  const auto& stat_liftHeight_mm = ss6.str();
  std::stringstream ss7;
  ss7 << std::fixed << std::setprecision(3) << robotState.leftWheelSpeed_mmps;
  const auto& stat_LWheelSpeed_mmps = ss7.str();
  std::stringstream ss8;
  ss8 << std::fixed << std::setprecision(3) << robotState.rightWheelSpeed_mmps;
  const auto& stat_RWheelSpeed_mmps = ss8.str();
  std::stringstream ss9;
  ss9 << std::fixed << std::setprecision(3) << robotState.accel.x;
  const auto& stat_accelX = ss9.str();
  std::stringstream ss10;
  ss10 << std::fixed << std::setprecision(3) << robotState.accel.y;
  const auto& stat_accelY = ss10.str();
  std::stringstream ss11;
  ss11 << std::fixed << std::setprecision(3) << robotState.accel.z;
  const auto& stat_accelZ = ss11.str();
  std::stringstream ss12;
  ss12 << std::fixed << std::setprecision(3) << robotState.gyro.x;
  const auto& stat_gyroX = ss12.str();
  std::stringstream ss13;
  ss13 << std::fixed << std::setprecision(3) << robotState.gyro.y;
  const auto& stat_gyroY = ss13.str();
  std::stringstream ss14;
  ss14 << std::fixed << std::setprecision(3) << robotState.gyro.z;
  const auto& stat_gyroZ = ss14.str();

  mg_printf(conn, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
            stat_poseAngle_rad.c_str(), stat_posePitch_rad.c_str(),
            stat_headAngle_rad.c_str(), stat_liftHeight_mm.c_str(),
            stat_LWheelSpeed_mmps.c_str(), stat_RWheelSpeed_mmps.c_str(),
            stat_accelX.c_str(), stat_accelY.c_str(), stat_accelZ.c_str(),
            stat_gyroX.c_str(), stat_gyroY.c_str(), stat_gyroZ.c_str());

  const auto& touchSensorComponent = robot->GetTouchSensorComponent();
  const std::string& stat_touchDataRaw = std::to_string(touchSensorComponent.GetLatestRawTouchValue());

  const auto& cliffSensorComponent = robot->GetCliffSensorComponent();
  const auto& cliffDataRaw = cliffSensorComponent.GetCliffDataRaw();
  const auto& stat_cliff0 = std::to_string(cliffDataRaw[0]);
  const auto& stat_cliff1 = std::to_string(cliffDataRaw[1]);
  const auto& stat_cliff2 = std::to_string(cliffDataRaw[2]);
  const auto& stat_cliff3 = std::to_string(cliffDataRaw[3]);

  const auto& proxSensorComponent = robot->GetProxSensorComponent();
  const auto& proxDataRaw = proxSensorComponent.GetLatestProxDataRaw();
  std::stringstream ss15;
  ss15 << std::fixed << std::setprecision(3) << proxDataRaw.signalIntensity;
  const auto& stat_proxSignalIntensity = ss15.str();
  std::stringstream ss16;
  ss16 << std::fixed << std::setprecision(3) << proxDataRaw.ambientIntensity;
  const auto& stat_proxAmbientIntensity = ss16.str();
  std::stringstream ss17;
  ss17 << std::fixed << std::setprecision(3) << proxDataRaw.spadCount;
  const auto& stat_proxSpadCount = ss17.str();
  const auto& stat_proxDistance_mm = std::to_string(proxDataRaw.distance_mm);
  const auto& stat_proxRangeStatus = std::to_string(proxDataRaw.rangeStatus);

  mg_printf(conn, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
            stat_touchDataRaw.c_str(), stat_cliff0.c_str(),
            stat_cliff1.c_str(), stat_cliff2.c_str(),
            stat_cliff3.c_str(), stat_proxSignalIntensity.c_str(),
            stat_proxAmbientIntensity.c_str(), stat_proxSpadCount.c_str(),
            stat_proxDistance_mm.c_str(), stat_proxRangeStatus.c_str());

  const auto& stat_carryingObjectID = std::to_string(robotState.carryingObjectID);
  const auto& stat_carryingObjectOnTopID = std::to_string(robotState.carryingObjectOnTopID);
  const auto& stat_headTrackingObjectID = std::to_string(robotState.headTrackingObjectID);
  const auto& stat_localizedToObjectID = std::to_string(robotState.localizedToObjectID);
  const auto& stat_status = std::to_string(robotState.status);
  mg_printf(conn, "%s\n%s\n%s\n%s\n%s\n",
            stat_carryingObjectID.c_str(), stat_carryingObjectOnTopID.c_str(),
            stat_headTrackingObjectID.c_str(), stat_localizedToObjectID.c_str(),
            stat_status.c_str());

  const auto& micDirectionHistory = robot->GetMicComponent().GetMicDirectionHistory();
  const auto& stat_micRecentDirection = std::to_string(micDirectionHistory.GetRecentDirection());
  const auto& stat_micSelectedDirection = std::to_string(micDirectionHistory.GetSelectedDirection());
  mg_printf(conn, "%s\n%s\n",
            stat_micRecentDirection.c_str(), stat_micSelectedDirection.c_str());

  return 1;
}


CozmoEngine::CozmoEngine(Util::Data::DataPlatform* dataPlatform, GameMessagePort* messagePipe)
  : _uiMsgHandler(new UiMessageHandler(1, messagePipe))
  , _context(new CozmoContext(dataPlatform, _uiMsgHandler.get()))
  , _deviceDataManager(new DeviceDataManager(_uiMsgHandler.get()))
  ,_animationTransferHandler(new AnimationTransfer(_uiMsgHandler.get(),dataPlatform))
{
#if ANKI_CPU_PROFILER_ENABLED
  // Initialize CPU profiler early and put tracing file at known location with no dependencies on other systems
  Anki::Util::CpuProfiler::GetInstance();
  Anki::Util::CpuThreadProfiler::SetChromeTracingFile(
      dataPlatform->pathToResource(Util::Data::Scope::Cache, "vic-engine-tracing.json").c_str());
  Anki::Util::CpuThreadProfiler::SendToWebVizCallback([&](const Json::Value& json) { _context->GetWebService()->SendToWebViz("cpuprofile", json); });
#endif

  DEV_ASSERT(_context->GetExternalInterface() != nullptr, "Cozmo.Engine.ExternalInterface.nullptr");
  if (Anki::Util::gTickTimeProvider == nullptr) {
    Anki::Util::gTickTimeProvider = BaseStationTimer::getInstance();
  }

  //
  // The "engine thread" is meant to be the one Update is run on. However, on some systems, some messaging
  // happens during Init which is on one thread, then later, a different thread runs the updates. This will
  // trigger asserts because more than one thread is sending messages. To work around this, we consider the
  // "engine thread" to be whatever thread Init is called from, until the first call of Update, at which point
  // we switch our notion of "engine thread" to the updating thread.
  //
  // During shutdown, the "engine thread" may switch again so messages can be sent by the thread performing shutdown.
  // This happens AFTER stopping the update thread, so we can still guarantee that no other threads are allowed
  // to send messages.
  //
  _context->SetEngineThread();

  DASMSG(device_language_locale, "device.language_locale", "Prints out the language locale of the robot");
  DASMSG_SET(s1, _context->GetLocale()->GetLocaleString().c_str(), "Locale on start up");
  DASMSG_SEND();

  std::time_t t = std::time(nullptr);
  std::tm tm = *std::localtime(&t);
  std::stringstream timeZoneString;
  timeZoneString << std::put_time(&tm, "%Z");
  std::stringstream timeZoneOffset;
  timeZoneOffset << std::put_time(&tm, "%z");
  Util::sInfoF("device.timezone", {{DDATA, timeZoneOffset.str().c_str()}}, "%s", timeZoneString.str().c_str());

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

  const auto& webService = _context->GetWebService();
  webService->Start(_context->GetDataPlatform(),
                    _context->GetDataLoader()->GetWebServerEngineConfig());
  webService->RegisterRequestHandler("/getenginestats", GetEngineStatsWebServerHandler, this);

  // DAS Event: "cozmo_engine.init.build_configuration"
  // s_val: Build configuration
  // data: Unused
  Anki::Util::sInfo("cozmo_engine.init.build_configuration", {},
#if defined(NDEBUG)
                     "RELEASE");
#else
                     "DEBUG");
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
  // be doing the updating.
  if( !_hasRunFirstUpdate ) {
    _context->SetEngineThread();
    _hasRunFirstUpdate = true;
  }

  DEV_ASSERT(_context->IsEngineThread(), "CozmoEngine.UpdateOnWrongThread" );

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

  _context->GetWebService()->Update();

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
      // Is is time to try connecting?
      const auto now = std::chrono::steady_clock::now();
      const auto elapsed = (now - lastConnectAttempt);
      if (elapsed < connectInterval) {
        // Too soon to try connecting
        break;
      }
      lastConnectAttempt = now;

      // Attempt to conect
      Result result = ConnectToRobotProcess();
      if (RESULT_OK != result) {
        //LOG_WARNING("CozmoEngine.Update.ConnectingToRobot", "Unable to connect to robot (result %d)", result);
        break;
      }

      // Now connected
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
      _context->GetRobotManager()->UpdateRobot();

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
      Anki::Util::sInfoF("cozmo_engine.update.run.slow",
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

  Anki::Util::sInfoF("app.engine.state", {{DDATA,EngineStateToString(newState)}}, "%s", EngineStateToString(oldState));
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
      //LOG_WARNING("CozmoEngine.ConnectToRobotProcess", "Unable to connect to robot %d (result %d)", robotID, result);
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

void CozmoEngine::SetEngineThread()
{
  // Context is valid for lifetime of engine
  DEV_ASSERT(_context, "CozmoEngine.SetEngineThread.InvalidContext");
  _context->SetEngineThread();
}

} // namespace Cozmo
} // namespace Anki
