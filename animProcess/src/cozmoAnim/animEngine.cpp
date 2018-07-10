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

#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cozmoAnim/audio/microphoneAudioClient.h"
#include "cozmoAnim/audio/engineRobotAudioInput.h"
#include "cozmoAnim/animation/animationStreamer.h"
#include "cozmoAnim/animation/streamingAnimationModifier.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenManager.h"
#include "cozmoAnim/micData/micDataSystem.h"
#include "cozmoAnim/robotDataLoader.h"
#include "cozmoAnim/textToSpeech/textToSpeechComponent.h"

#include "coretech/common/engine/opencvThreading.h"
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

#define LOG_CHANNEL    "AnimEngine"

#if ANKI_PROFILING_ENABLED && !defined(SIMULATOR)
  #define ENABLE_CE_SLEEP_TIME_DIAGNOSTICS 0
  #define ENABLE_CE_RUN_TIME_DIAGNOSTICS 1
#else
  #define ENABLE_CE_SLEEP_TIME_DIAGNOSTICS 0
  #define ENABLE_CE_RUN_TIME_DIAGNOSTICS 0
#endif
#define NUM_ANIM_OPENCV_THREADS 0

namespace Anki {
namespace Cozmo {

#if ANKI_CPU_PROFILER_ENABLED
  CONSOLE_VAR_RANGED(float, kAnimEngine_TimeMax_ms,     ANKI_CPU_CONSOLEVARGROUP, 2, 2, 32);
  CONSOLE_VAR_ENUM(u8,      kAnimEngine_TimeLogging,    ANKI_CPU_CONSOLEVARGROUP, 0, Util::CpuProfiler::CpuProfilerLogging());
#endif

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

  _microphoneAudioClient.reset(new Audio::MicrophoneAudioClient(_context->GetAudioController()));
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

  uint32_t seed = 0; // will choose random seed
# ifdef ANKI_PLATFORM_OSX
  {
    seed = 1; // Setting to non-zero value for now for repeatable testing.
  }
# endif
  _context->SetRandomSeed(seed);

  OSState::getInstance()->SetUpdatePeriod(1000);

  RobotDataLoader * dataLoader = _context->GetDataLoader();
  dataLoader->LoadConfigData();
  dataLoader->LoadNonConfigData();

  _ttsComponent = std::make_unique<TextToSpeechComponent>(_context.get());
  _context->GetMicDataSystem()->Init(*dataLoader);

  // animation streamer must be initialized after loading non config data (otherwise there are no animations loaded)
  _animationStreamer->Init();

  // Create and set up EngineRobotAudioInput to receive Engine->Robot messages and broadcast Robot->Engine
  auto* audioMux = _context->GetAudioMultiplexer();
  auto regId = audioMux->RegisterInput( new Audio::EngineRobotAudioInput() );
  // Easy access to Audio Controller
  _audioControllerPtr = _context->GetAudioController();

  // Set up message handler
  auto * audioInput = static_cast<Audio::EngineRobotAudioInput*>(audioMux->GetInput(regId));
  _streamingAnimationModifier = std::make_unique<StreamingAnimationModifier>(_animationStreamer.get(), audioInput);

  AnimProcessMessages::Init(this, _animationStreamer.get(), _streamingAnimationModifier.get(), audioInput, _context.get());

  _context->GetWebService()->Start(_context->GetDataPlatform(),
                                   _context->GetDataLoader()->GetWebServerAnimConfig());
  FaceInfoScreenManager::getInstance()->Init(_context.get(), _animationStreamer.get());

  // Make sure OpenCV isn't threading
  Result cvResult = SetNumOpencvThreads( NUM_ANIM_OPENCV_THREADS, "AnimEngine.Init" );
  if( RESULT_OK != cvResult )
  {
    return cvResult;
  }

  LOG_INFO("AnimEngine.Init.Success","Success");
  _isInitialized = true;

  return RESULT_OK;
}

Result AnimEngine::Update(BaseStationTime_t currTime_nanosec)
{
  ANKI_CPU_TICK("AnimEngine::Update", kAnimEngine_TimeMax_ms, Util::CpuProfiler::CpuProfilerLoggingTime(kAnimEngine_TimeLogging));
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
        DASMSG(cozmo_anim_update_sleep_slow,
               "cozmo_anim.update.sleep.slow",
               "This will be shown on the slow updates and as such should not be seen very often")
        DASMSG_SET(s1, timeSinceLastUpdate, "timeSinceLastUpdate as a float")
        DASMSG_SEND()
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

  // Clear out sprites that have passed their cache time
  _context->GetDataLoader()->GetSpriteCache()->Update(currTime_nanosec);

  if(_streamingAnimationModifier != nullptr){
    _streamingAnimationModifier->ApplyAlterationsBeforeUpdate(_animationStreamer.get());
  }
  _animationStreamer->Update();
  if(_streamingAnimationModifier != nullptr){
    _streamingAnimationModifier->ApplyAlterationsAfterUpdate(_animationStreamer.get());
  }

  if (_audioControllerPtr != nullptr) {
    // Update mic info in Audio Engine
    const auto& micDirectionMsg = _context->GetMicDataSystem()->GetLatestMicDirectionMsg();
    _microphoneAudioClient->ProcessMessage(micDirectionMsg);
    // Tick the Audio Engine at the end of each anim frame
    _audioControllerPtr->Update();
  }

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

void AnimEngine::HandleMessage(const RobotInterface::TextToSpeechPlay & msg)
{
  DEV_ASSERT(_ttsComponent, "AnimEngine.TextToSpeechPlay.InvalidTTSComponent");
  _ttsComponent->HandleMessage(msg);
}

void AnimEngine::HandleMessage(const RobotInterface::TextToSpeechCancel & msg)
{
  DEV_ASSERT(_ttsComponent, "AnimEngine.TextToSpeechCancel.InvalidTTSComponent");
  _ttsComponent->HandleMessage(msg);
}

void AnimEngine::HandleMessage(const RobotInterface::SetLocale & msg)
{
  const std::string locale(msg.locale, msg.locale_length);

  LOG_INFO("AnimEngine.SetLocale", "Set locale to %s", locale.c_str());

  if (_context != nullptr) {
    _context->SetLocale(locale);
  }

  if (_ttsComponent != nullptr) {
    _ttsComponent->SetLocale(locale);
  }

}
} // namespace Cozmo
} // namespace Anki
