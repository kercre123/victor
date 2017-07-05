/*
 * File:          cozmoAnim.cpp
 * Date:          6/26/2017
 *
 * Description:   (See header file.)
 *
 * Author: Kevin Yoon
 *
 * Modifications:
 */

#include "cozmoAnim/cozmoAnim.h"
#include "cozmoAnim/engineMessages.h"
//#include "anki/cozmo/basestation/cozmoContext.h"
//#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
//#include "anki/cozmo/basestation/events/ankiEvent.h"
//#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
//#include "anki/cozmo/basestation/ankiEventUtil.h"
//#include "anki/cozmo/basestation/audio/audioEngineMessageHandler.h"
//#include "anki/cozmo/basestation/audio/audioEngineInput.h"
//#include "anki/cozmo/basestation/audio/cozmoAudioController.h"
//#include "anki/cozmo/basestation/audio/robotAudioClient.h"
//#include "anki/cozmo/basestation/debug/cladLoggerProvider.h"
//#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/timer.h"
//#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
//#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/shared/cozmoConfig.h"
//#include "audioEngine/multiplexer/audioMultiplexer.h"
//#include "util/console/consoleInterface.h"
//#include "util/cpuProfiler/cpuProfiler.h"
//#include "util/global/globalDefinitions.h"
//#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
//#include "util/logging/printfLoggerProvider.h"
//#include "util/logging/multiLoggerProvider.h"
#include "util/time/universalTime.h"
//#include "util/environment/locale.h"
//#include "util/transport/connectionStats.h"
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>


#if USE_DAS
#include <DAS/DAS.h>
#include <DAS/DASPlatform.h>
#endif



#if ANKI_PROFILING_ENABLED
  #define ENABLE_CE_SLEEP_TIME_DIAGNOSTICS 0
  #define ENABLE_CE_RUN_TIME_DIAGNOSTICS 1
#else
  #define ENABLE_CE_SLEEP_TIME_DIAGNOSTICS 0
  #define ENABLE_CE_RUN_TIME_DIAGNOSTICS 0
#endif

namespace Anki {
namespace Cozmo {

CozmoAnim::CozmoAnim(Util::Data::DataPlatform* dataPlatform)
{
  
  if (Anki::Util::gTickTimeProvider == nullptr) {
    Anki::Util::gTickTimeProvider = BaseStationTimer::getInstance();
  }
  
}

CozmoAnim::~CozmoAnim()
{
  if (Anki::Util::gTickTimeProvider == BaseStationTimer::getInstance()) {
    Anki::Util::gTickTimeProvider = nullptr;
  }
  BaseStationTimer::removeInstance();
}

Result CozmoAnim::Init(const Json::Value& config) {

  if(_isInitialized) {
    PRINT_NAMED_INFO("CozmoEngine.Init.ReInit", "Reinitializing already-initialized CozmoEngineImpl with new config.");
  }
  
  _isInitialized = false;

  _config = config;
  
//  if(!_config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP)) {
//    PRINT_NAMED_ERROR("CozmoEngine.Init", "No AdvertisingHostIP defined in Json config.");
//    return RESULT_FAIL;
//  }
//  
//  if(!_config.isMember(AnkiUtil::kP_ROBOT_ADVERTISING_PORT)) {
//    PRINT_NAMED_ERROR("CozmoEngine.Init", "No RobotAdvertisingPort defined in Json config.");
//    return RESULT_FAIL;
//  }
//  
//  if(!_config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
//    PRINT_NAMED_ERROR("CozmoEngine.Init", "No UiAdvertisingPort defined in Json config.");
//    return RESULT_FAIL;
//  }
  
//  Result lastResult = _uiMsgHandler->Init(_context.get(), _config);
//  if (RESULT_OK != lastResult)
//  {
//    PRINT_NAMED_ERROR("CozmoEngine.Init","Error initializing UIMessageHandler");
//    return lastResult;
//  }
  
  Messages::Init();
  
  
  Result lastResult = InitInternal();
  if(lastResult != RESULT_OK) {
    PRINT_NAMED_ERROR("CozomEngine.Init", "Failed calling internal init.");
    return lastResult;
  }
  
  PRINT_NAMED_INFO("CozmoAnim.Init.Success","");
  _isInitialized = true;

  return RESULT_OK;
}

//template<>
//void CozmoAnim::HandleMessage(const ExternalInterface::StartEngine& msg)
//{
//  //_context->SetRandomSeed(msg.random_seed);
//  //_context->SetLocale(msg.locale);
//  
//  if (EngineState::Running == _engineState) {
//    PRINT_NAMED_ERROR("CozmoEngine.HandleMessage.StartEngine.AlreadyStarted", "");
//    return;
//  }
//  
//  SetEngineState(EngineState::WaitingForUIDevices);
//}
  

Result CozmoAnim::Update(const BaseStationTime_t currTime_nanosec)
{
  //ANKI_CPU_PROFILE("CozmoAnim::Update");
  
  if(!_isInitialized) {
    PRINT_NAMED_ERROR("CozmoAnim.Update", "Cannot update CozmoEngine before it is initialized.");
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
      const double maxLatency = BS_TIME_STEP + 15.;
      if (timeSinceLastUpdate > maxLatency)
      {
        Anki::Util::sEventF("cozmo_anim.update.sleep.slow", {{DDATA,TO_DDATA_STR(BS_TIME_STEP)}}, "%.2f", timeSinceLastUpdate);
      }
    }
    lastUpdateTimeMs = startUpdateTimeMs;
    firstUpdate = false;
  }
#endif // ENABLE_CE_SLEEP_TIME_DIAGNOSTICS
  
  BaseStationTimer::getInstance()->UpdateTime(currTime_nanosec);
  Messages::Update();
  
  
//  // Handle UI
//  Result lastResult = _uiMsgHandler->Update();
//  if (RESULT_OK != lastResult)
//  {
//    PRINT_NAMED_ERROR("CozmoAnim.Update", "Error updating UIMessageHandler");
//    return lastResult;
//  }
  
//  switch (_engineState)
//  {
//    case EngineState::Stopped:
//    {
//      break;
//    }
//    case EngineState::WaitingForUIDevices:
//    {
//      if (_uiMsgHandler->HasDesiredNumUiDevices()) {
//        SetEngineState(EngineState::LoadingData);
//      }
//      break;
//    }
//    case EngineState::LoadingData:
//    {
//      float currentLoadingDone = 0.0f;
//      if (_context->GetDataLoader()->DoNonConfigDataLoading(currentLoadingDone))
//      {
//        _context->GetRobotManager()->BroadcastAvailableAnimations();
//        SetEngineState(EngineState::Running);
//      }
//      _context->GetExternalInterface()->BroadcastToGame<ExternalInterface::EngineLoadingDataStatus>(currentLoadingDone);
//      break;
//    }
//    case EngineState::Running:
//    {
//      break;
//    }
//    default:
//      PRINT_NAMED_ERROR("CozmoAnim.Update.UnexpectedState","Running Update in an unexpected state!");
//  }
  
  // Tick Audio Controller after all messages have been processed
//  const auto audioMux = _context->GetAudioMultiplexer();
//  if (audioMux != nullptr) {
//    audioMux->UpdateAudioController();
//  }
  
#if ENABLE_CE_RUN_TIME_DIAGNOSTICS
  {
    const double endUpdateTimeMs = Util::Time::UniversalTime::GetCurrentTimeInMilliseconds();
    const double updateLengthMs = endUpdateTimeMs - startUpdateTimeMs;
    const double maxUpdateDuration = ANIM_TIME_STEP;
    if (updateLengthMs > maxUpdateDuration)
    {
      Anki::Util::sEventF("cozmo_anim.update.run.slow",
                          {{DDATA,std::to_string(ANIM_TIME_STEP).c_str()}},
                          "%.2f", updateLengthMs);
    }
  }
#endif // ENABLE_CE_RUN_TIME_DIAGNOSTICS

  return RESULT_OK;
}



//void CozmoAnim::SetEngineState(EngineState newState)
//{
//  EngineState oldState = _engineState;
//  if (oldState == newState)
//  {
//    return;
//  }
//  
//  _engineState = newState;
//  
//  // TODO: Send some kind of AnimProcess state update to engine
////  _context->GetExternalInterface()->BroadcastToGame<ExternalInterface::UpdateEngineState>(oldState, newState);
//  
//  Anki::Util::sEventF("app.engine.state", {{DDATA,EngineStateToString(newState)}}, "%s", EngineStateToString(oldState));
//}
  
Result CozmoAnim::InitInternal()
{
  // Setup Audio Controller
  {
  }
  
  return RESULT_OK;
}
  

//template<>
//void CozmoEngine::HandleMessage(const ExternalInterface::ReadAnimationFile& msg)
//{
//  _context->GetRobotManager()->ReadAnimationDir();
//}
//
//template<>
//void CozmoEngine::HandleMessage(const ExternalInterface::ReadFaceAnimationDir& msg)
//{
//  _context->GetRobotManager()->ReadFaceAnimationDir();
//}


} // namespace Cozmo
} // namespace Anki
