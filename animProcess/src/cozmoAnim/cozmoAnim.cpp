/*
 * File:          cozmoAnim.cpp
 * Date:          6/26/2017
 *
 * Description:   A platform-independent container for spinning up all the pieces
 *                required to run Cozmo Animation Process.
 *
 * Author: Kevin Yoon
 *
 * Modifications:
 */

#include "cozmoAnim/cozmoAnim.h"
#include "cozmoAnim/engineMessages.h"
#include "cozmoAnim/cozmoAnimContext.h"
#include "cozmoAnim/audio/engineRobotAudioInput.h"
#include "cozmoAnim/animation/animationStreamer.h"
#include "cozmoAnim/robotDataLoader.h"
#include "cozmoAnim/textToSpeech/textToSpeechComponent.h"

#include "anki/common/basestation/utils/timer.h"
#include "audioEngine/multiplexer/audioMultiplexer.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "osState/osState.h"

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


#if ANKI_PROFILING_ENABLED && !defined(SIMULATOR)
  #define ENABLE_CE_SLEEP_TIME_DIAGNOSTICS 0
  #define ENABLE_CE_RUN_TIME_DIAGNOSTICS 1
#else
  #define ENABLE_CE_SLEEP_TIME_DIAGNOSTICS 0
  #define ENABLE_CE_RUN_TIME_DIAGNOSTICS 0
#endif

namespace Anki {
namespace Cozmo {

CozmoAnimEngine::CozmoAnimEngine(Util::Data::DataPlatform* dataPlatform)
  : _isInitialized(false)
  , _context(std::make_unique<CozmoAnimContext>(dataPlatform))
  , _animationStreamer(std::make_unique<AnimationStreamer>(_context.get()))
{
  
  if (Anki::Util::gTickTimeProvider == nullptr) {
    Anki::Util::gTickTimeProvider = BaseStationTimer::getInstance();
  }
  
}

CozmoAnimEngine::~CozmoAnimEngine()
{
  if (Anki::Util::gTickTimeProvider == BaseStationTimer::getInstance()) {
    Anki::Util::gTickTimeProvider = nullptr;
  }
  BaseStationTimer::removeInstance();
}

Result CozmoAnimEngine::Init() {

  if(_isInitialized) {
    PRINT_NAMED_INFO("CozmoEngine.Init.ReInit", "Reinitializing already-initialized CozmoEngineImpl with new config.");
  }

  OSState::getInstance()->SetUpdatePeriod(1000);

  RobotDataLoader * dataLoader = _context->GetDataLoader();
  dataLoader->LoadConfigData();
  dataLoader->LoadNonConfigData();
  
  _ttsComponent = std::make_unique<TextToSpeechComponent>(_context.get());

  // animation streamer must be initialized after loading non config data (otherwise there are no animations loaded)
  _animationStreamer->Init();
  
  // Create and setup EngineRobotAudioInput to receive Engine->Robot messages and broadcast Robot->Engine
  auto* audioMux = _context->GetAudioMultiplexer();
  auto regId = audioMux->RegisterInput( new Audio::EngineRobotAudioInput() );
  
  // Setup Engine Message
  Messages::Init( _animationStreamer.get(),
                  static_cast<Audio::EngineRobotAudioInput*>(audioMux->GetInput( regId )),
                  _context.get() );
  
  
  
  PRINT_NAMED_INFO("CozmoAnimEngine.Init.Success","");
  _isInitialized = true;

  return RESULT_OK;
}

Result CozmoAnimEngine::Update(const BaseStationTime_t currTime_nanosec)
{
  //ANKI_CPU_PROFILE("CozmoAnimEngine::Update");
  
  if(!_isInitialized) {
    PRINT_NAMED_ERROR("CozmoAnimEngine.Update", "Cannot update CozmoEngine before it is initialized.");
    return RESULT_FAIL;
  }

  
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
      const double maxLatency = ANIM_TIME_STEP_MS + ANIM_OVERTIME_WARNING_THRESH_MS;
      if (timeSinceLastUpdate > maxLatency)
      {
        Anki::Util::sEventF("cozmo_anim.update.sleep.slow", {{DDATA,TO_DDATA_STR(ANIM_TIME_STEP_MS)}}, "%.2f", timeSinceLastUpdate);
      }
    }
    lastUpdateTimeMs = startUpdateTimeMs;
    firstUpdate = false;
  }
#endif // ENABLE_CE_SLEEP_TIME_DIAGNOSTICS
  
  BaseStationTimer::getInstance()->UpdateTime(currTime_nanosec);
  Messages::Update();
  
  OSState::getInstance()->Update();
  _animationStreamer->Update();
  
#if ENABLE_CE_RUN_TIME_DIAGNOSTICS
  {
    const double endUpdateTimeMs = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
    const double updateLengthMs = endUpdateTimeMs - startUpdateTimeMs;
    const double maxUpdateDuration = ANIM_TIME_STEP_MS;
    if (updateLengthMs > maxUpdateDuration)
    {
      Anki::Util::sEventF("cozmo_anim.update.run.slow",
                          {{DDATA,std::to_string(ANIM_TIME_STEP_MS).c_str()}},
                          "%.2f", updateLengthMs);
    }
  }
#endif // ENABLE_CE_RUN_TIME_DIAGNOSTICS

  return RESULT_OK;
}


} // namespace Cozmo
} // namespace Anki
