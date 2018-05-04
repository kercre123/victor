/*
 * File:          cozmoAnim/animEngine.cpp
 * Date:          6/26/2017
 *
 * Description:   A platform-independent container for spinning up all the pieces
 *                required to run Cozmo Animation Process.
 *
 * Author: Kevin Yoon
 *
 * Modifications:
 */

#include "cozmoAnim/animEngine.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/animProcessMessages.h"

#include "cozmoAnim/audio/engineRobotAudioInput.h"
#include "cozmoAnim/animation/animationStreamer.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenManager.h"
#include "cozmoAnim/robotDataLoader.h"
#include "cozmoAnim/textToSpeech/textToSpeechComponent.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "audioEngine/multiplexer/audioMultiplexer.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "webServerProcess/src/webService.h"

#include "osState/osState.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"

#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>


#if USE_DAS
#include <DAS/DAS.h>
#include <DAS/DASPlatform.h>
#endif

#define LOG_CHANNEL    "AnimEngine"

#if ANKI_PROFILING_ENABLED && !defined(SIMULATOR)
  #define ENABLE_CE_SLEEP_TIME_DIAGNOSTICS 0
  #define ENABLE_CE_RUN_TIME_DIAGNOSTICS 1
#else
  #define ENABLE_CE_SLEEP_TIME_DIAGNOSTICS 0
  #define ENABLE_CE_RUN_TIME_DIAGNOSTICS 0
#endif

namespace Anki {
namespace Cozmo {

AnimEngine::AnimEngine(Util::Data::DataPlatform* dataPlatform)
  : _isInitialized(false)
  , _context(std::make_unique<AnimContext>(dataPlatform))
  , _animationStreamer(std::make_unique<AnimationStreamer>(_context.get()))
{
#if ANKI_CPU_PROFILER_ENABLED
  // Initialize CPU profiler early and put tracing file at known location with no dependencies on other systems
  Anki::Util::CpuProfiler::GetInstance();
  Anki::Util::CpuThreadProfiler::SetChromeTracingFile(
      dataPlatform->pathToResource(Util::Data::Scope::Cache, "vic-anim-tracing.json").c_str());
  Anki::Util::CpuThreadProfiler::SendToWebVizCallback([&](const Json::Value& json) { _context->GetWebService()->SendToWebViz("cpuprofile", json); });
#endif

  if (Anki::Util::gTickTimeProvider == nullptr) {
    Anki::Util::gTickTimeProvider = BaseStationTimer::getInstance();
  }
}

AnimEngine::~AnimEngine()
{
  if (Anki::Util::gTickTimeProvider == BaseStationTimer::getInstance()) {
    Anki::Util::gTickTimeProvider = nullptr;
  }
  BaseStationTimer::removeInstance();
}

Result AnimEngine::Init()
{
  if (_isInitialized) {
    LOG_INFO("AnimEngine.Init.ReInit", "Reinitializing already-initialized CozmoEngineImpl with new config.");
  }

  OSState::getInstance()->SetUpdatePeriod(1000);

  RobotDataLoader * dataLoader = _context->GetDataLoader();
  dataLoader->LoadConfigData();
  dataLoader->LoadNonConfigData();

  _ttsComponent = std::make_unique<TextToSpeechComponent>(_context.get());

  // animation streamer must be initialized after loading non config data (otherwise there are no animations loaded)
  _animationStreamer->Init();

  // Create and set up EngineRobotAudioInput to receive Engine->Robot messages and broadcast Robot->Engine
  auto* audioMux = _context->GetAudioMultiplexer();
  auto regId = audioMux->RegisterInput( new Audio::EngineRobotAudioInput() );

  // Set up message handler
  auto * audioInput = static_cast<Audio::EngineRobotAudioInput*>(audioMux->GetInput(regId));
  AnimProcessMessages::Init(this, _animationStreamer.get(), audioInput, _context.get());

  _context->GetWebService()->Start(_context->GetDataPlatform(),
                                   _context->GetDataLoader()->GetWebServerAnimConfig());
  FaceInfoScreenManager::getInstance()->Init(_context.get(), _animationStreamer.get());

  LOG_INFO("AnimEngine.Init.Success","Success");
  _isInitialized = true;

  return RESULT_OK;
}

Result AnimEngine::Update(BaseStationTime_t currTime_nanosec)
{
  //ANKI_CPU_PROFILE("AnimEngine::Update");
  if (!_isInitialized) {
    LOG_ERROR("AnimEngine.Update", "Cannot update AnimEngine before it is initialized.");
    return RESULT_FAIL;
  }

  // Declare some invariants
  DEV_ASSERT(_context, "AnimEngine.Update.InvalidContext");
  DEV_ASSERT(_ttsComponent, "AnimEngine.Update.InvalidTTSComponent");
  DEV_ASSERT(_animationStreamer, "AnimEngine.Update.InvalidAnimationStreamer");

#if ENABLE_CE_SLEEP_TIME_DIAGNOSTICS || ENABLE_CE_RUN_TIME_DIAGNOSTICS
  const double startUpdateTimeMs = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
#endif
#if ENABLE_CE_SLEEP_TIME_DIAGNOSTICS
  {
    static bool firstUpdate = true;
    static double lastUpdateTimeMs = 0.0;
    //const double currentTimeMs = (double)currTime_nanosec / 1e+6;
    if (!firstUpdate)
    {
      const double timeSinceLastUpdate = startUpdateTimeMs - lastUpdateTimeMs;
      const double maxLatency = ANIM_TIME_STEP_MS + ANIM_OVERTIME_WARNING_THRESH_MS;
      if (timeSinceLastUpdate > maxLatency)
      {
        DAS_MSG(slow, "cozmo_anim.update.sleep.slow", "This will be show on the slow updates and as such should not be seen very often")
        FILL_ITEM(s1, timeSinceLastUpdate, "timeSinceLastUpdate as a float")
        SEND_DAS_MSG_EVENT()
      }
    }
    lastUpdateTimeMs = startUpdateTimeMs;
    firstUpdate = false;
  }
#endif // ENABLE_CE_SLEEP_TIME_DIAGNOSTICS

  BaseStationTimer::getInstance()->UpdateTime(currTime_nanosec);

  _context->GetWebService()->Update();

  Result result = AnimProcessMessages::Update(currTime_nanosec);
  if (RESULT_OK != result) {
    LOG_WARNING("AnimEngine.Update", "Unable to process messages (result %d)", result);
    return result;
  }

  OSState::getInstance()->Update();

  _ttsComponent->Update();

  _animationStreamer->Update();

#if ENABLE_CE_RUN_TIME_DIAGNOSTICS
  {
    const double endUpdateTimeMs = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
    const double updateLengthMs = endUpdateTimeMs - startUpdateTimeMs;
    const double maxUpdateDuration = ANIM_TIME_STEP_MS;
    if (updateLengthMs > maxUpdateDuration)
    {
      Anki::Util::sInfoF("cozmo_anim.update.run.slow",
                         {{DDATA,std::to_string(ANIM_TIME_STEP_MS).c_str()}},
                         "%.2f", updateLengthMs);
    }
  }
#endif // ENABLE_CE_RUN_TIME_DIAGNOSTICS

  return RESULT_OK;
}

void AnimEngine::HandleMessage(const RobotInterface::TextToSpeechPrepare & msg)
{
  DEV_ASSERT(_ttsComponent, "AnimEngine.TextToSpeechPrepare.InvalidTTSComponent");
  _ttsComponent->HandleMessage(msg);
}

void AnimEngine::HandleMessage(const RobotInterface::TextToSpeechDeliver & msg)
{
  DEV_ASSERT(_ttsComponent, "AnimEngine.TextToSpeechDeliver.InvalidTTSComponent");
  _ttsComponent->HandleMessage(msg);
}

void AnimEngine::HandleMessage(const RobotInterface::TextToSpeechCancel & msg)
{
  DEV_ASSERT(_ttsComponent, "AnimEngine.TextToSpeechCancel.InvalidTTSComponent");
  _ttsComponent->HandleMessage(msg);
}


} // namespace Cozmo
} // namespace Anki
