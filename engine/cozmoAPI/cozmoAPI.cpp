/**
 * File: cozmoAPI.cpp
 *
 * Author: Lee Crippen
 * Created: 08/19/15
 *
 * Description: Point of entry for anything needing to interact with Cozmo.
 *
 * Copyright: Anki, Inc. 2015
 *
 * COZMO_PUBLIC_HEADER
 **/

#include "engine/cozmoAPI/cozmoAPI.h"
#include "engine/cozmoEngine.h"
#include "engine/viz/vizManager.h"
#include "engine/cozmoAPI/comms/gameMessagePort.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "clad/externalInterface/messageShared.h"
#include "util/ankiLab/ankiLabDef.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include "util/threading/threadPriority.h"
#include <chrono>

namespace Anki {
namespace Vector {

#pragma mark --- CozmoAPI Methods ---

#if ANKI_CPU_PROFILER_ENABLED
  CONSOLE_VAR_ENUM(u8, kCozmoEngineWebots_Logging, ANKI_CPU_CONSOLEVARGROUP, 0, Util::CpuProfiler::CpuProfilerLogging());
  CONSOLE_VAR_ENUM(u8, kCozmoEngine_Logging,       ANKI_CPU_CONSOLEVARGROUP, 0, Util::CpuProfiler::CpuProfilerLogging());
#endif

bool CozmoAPI::StartRun(Util::Data::DataPlatform* dataPlatform, const Json::Value& config)
{
  // If there's already a thread running, we'll kill and restart
  if (_cozmoRunnerThread.joinable())
  {
    Clear();
  }
  else if (_cozmoRunner)
  {
    PRINT_NAMED_ERROR("CozmoAPI.StartRun", "Non-threaded Cozmo already created!");
    return Result::RESULT_FAIL;
  }

  // Init the InstanceRunner
  bool gameInitResult = false;
  _cozmoRunner.reset(new CozmoInstanceRunner(dataPlatform, config, gameInitResult));

  if (!gameInitResult)
  {
    PRINT_NAMED_ERROR("CozmoAPI.StartRun", "Error initializing new api instance!");
    return Result::RESULT_FAIL;
  }

  // Start the thread
  _cozmoRunnerThread = std::thread(&CozmoInstanceRunner::Run, _cozmoRunner.get());

  return gameInitResult;
}

bool CozmoAPI::IsRunning() const
{
  if (_cozmoRunner) {
    return _cozmoRunner->IsRunning();
  }
  return false;
}

bool CozmoAPI::Start(Util::Data::DataPlatform* dataPlatform, const Json::Value& config)
{
  // If we have a joinable thread already, we can't start
  if (_cozmoRunnerThread.joinable())
  {
    PRINT_NAMED_ERROR("CozmoAPI.Start", "Cozmo already running in thread!");
    return Result::RESULT_FAIL;
  }

  // Game init happens in CozmoInstanceRunner construction, so we get the result
  // If we already had an instance, kill it and start again
  bool gameInitResult = false;
  _cozmoRunner.reset();
  _cozmoRunner.reset(new CozmoInstanceRunner(dataPlatform, config, gameInitResult));

  return gameInitResult;
}


ANKI_CPU_PROFILER_ENABLED_ONLY(const float kMaxDesiredEngineDuration = 60.0f); // Above this warn etc.


bool CozmoAPI::Update(const BaseStationTime_t currentTime_nanosec)
{
  // If we have a joinable thread already, shouldn't be updating
  if (_cozmoRunnerThread.joinable())
  {
    PRINT_NAMED_ERROR("CozmoAPI.Update", "Cozmo running in thread - can not be externally updated!");
    return false;
  }

  if (!_cozmoRunner)
  {
    PRINT_NAMED_ERROR("CozmoAPI.Update", "Cozmo has not been started!");
    return false;
  }

  // Replace Util::CpuThreadProfiler::kLogFrequencyNever with a small value to output logging,
  // can be used with Chrome Tracing format
  ANKI_CPU_TICK("CozmoEngineWebots", kMaxDesiredEngineDuration, Util::CpuProfiler::CpuProfilerLoggingTime(kCozmoEngineWebots_Logging));

  return _cozmoRunner->Update(currentTime_nanosec);
}

size_t CozmoAPI::SendMessages(uint8_t* buffer, size_t bufferSize)
{
  GameMessagePort* messagePipe = (_cozmoRunner != nullptr) ? _cozmoRunner->GetGameMessagePort() : nullptr;
  if (messagePipe == nullptr) {
    return 0;
  }

  return messagePipe->PullToGameMessages(buffer, bufferSize);
}

void CozmoAPI::ReceiveMessages(const uint8_t* buffer, size_t size)
{
  GameMessagePort* messagePipe = (_cozmoRunner != nullptr) ? _cozmoRunner->GetGameMessagePort() : nullptr;
  if (messagePipe == nullptr) {
    return;
  }

  messagePipe->PushFromGameMessages(buffer, size);
}

//void CozmoAPI::ExecuteBackgroundTransfers()
//{
//  CozmoEngine* engine = (_cozmoRunner != nullptr) ? _cozmoRunner->GetEngine() : nullptr;
//  if (engine == nullptr) {
//    return;
//  }
//  engine->ExecuteBackgroundTransfers();
//}

uint32_t CozmoAPI::ActivateExperiment(const uint8_t* requestBuffer, size_t requestLen,
                                      uint8_t* responseBuffer, size_t responseLen)
{
  using namespace Anki::Util::AnkiLab;

  // Response buffer will be filled in by ActivateExperiment. Set default values here.
  ActivateExperimentResponse res{AssignmentStatus::Invalid, ""};
  const size_t minResponseBufferLen = res.Size();

  // Assert that parameters are valid
  ASSERT_NAMED((nullptr != requestBuffer) && (requestLen > 0),
               "Must provide a valid requestBuffer/requestBufferLen to activate experiment");
  ASSERT_NAMED((nullptr != responseBuffer) && (responseLen >= minResponseBufferLen),
               "Must provide a valid responseBuffer/responseBufferLen to activate experiment");

  if (!_cozmoRunner) {
    return 0;
  }

  _cozmoRunner->SyncWithEngineUpdate([this, &res, requestBuffer, requestLen] {

    auto* engine = _cozmoRunner->GetEngine();
    if (engine == nullptr) {
      return;
    }

    // Unpack request buffer
    ActivateExperimentRequest req{requestBuffer, requestLen};

    res.status = engine->ActivateExperiment(req, res.variation_key);
  });

  const size_t bytesPacked = res.Pack(responseBuffer, responseLen);
  return Anki::Util::numeric_cast<uint32_t>(bytesPacked);
}

void CozmoAPI::RegisterEngineTickPerformance(const float tickDuration_ms,
                                             const float tickFrequency_ms,
                                             const float sleepDurationIntended_ms,
                                             const float sleepDurationActual_ms) const
{
  _cozmoRunner->GetEngine()->RegisterEngineTickPerformance(tickDuration_ms,
                                                           tickFrequency_ms,
                                                           sleepDurationIntended_ms,
                                                           sleepDurationActual_ms);
}

CozmoAPI::~CozmoAPI()
{
  Clear();
}

void CozmoAPI::Clear()
{
  // If there is a thread running, kill it first
  if (_cozmoRunnerThread.joinable())
  {
    if (_cozmoRunner)
    {
      _cozmoRunner->Stop();
    }
    else
    {
      PRINT_NAMED_ERROR("CozmoAPI.Clear", "Running thread has null object... what?");
    }
    _cozmoRunnerThread.join();
    _cozmoRunnerThread = std::thread();

    // We are now "owning thread" for engine updates
    if (_cozmoRunner)
    {
      _cozmoRunner->SetEngineThread();
    }
  }

  _cozmoRunner.reset();
}

#pragma mark --- CozmoInstanceRunner Methods ---

CozmoAPI::CozmoInstanceRunner::CozmoInstanceRunner(Util::Data::DataPlatform* dataPlatform,
                                                   const Json::Value& config, bool& initResult)
: _gameMessagePort(new GameMessagePort(ExternalInterface::kDirectCommsBufferSize,
                                       !config.get("standalone", false).asBool()))
, _cozmoInstance(new CozmoEngine(dataPlatform, _gameMessagePort.get()))
, _isRunning(true)
{
  Result initResultReturn = _cozmoInstance->Init(config);
  if (initResultReturn != RESULT_OK) {
    PRINT_NAMED_ERROR("CozmoAPI.CozmoInstanceRunner", "cozmo init failed with error %d", initResultReturn);
  }
  initResult = initResultReturn == RESULT_OK;
}

// Destructor must exist in cpp (even though it's empty) in order for CozmoGame unique_ptr to be defined and deletable
CozmoAPI::CozmoInstanceRunner::~CozmoInstanceRunner()
{

}

void CozmoAPI::CozmoInstanceRunner::Run()
{
  using namespace std::chrono;
  using TimeClock = steady_clock;

  const auto runStart = TimeClock::now();
  auto prevTickStart  = runStart;
  auto tickStart      = runStart;

  // Set the target time for the end of the first frame
  auto targetEndFrameTime = runStart + (microseconds)(BS_TIME_STEP_MICROSECONDS);
  Anki::Util::SetThreadName(pthread_self(), "CozmoRunner");

  while(_isRunning)
  {
    ANKI_CPU_TICK("CozmoEngine", kMaxDesiredEngineDuration, Util::CpuProfiler::CpuProfilerLoggingTime(kCozmoEngine_Logging));

    const duration<double> curTimeSeconds = tickStart - runStart;
    const double curTimeNanoseconds = Util::SecToNanoSec(curTimeSeconds.count());

    const bool tickSuccess = Update(Util::numeric_cast<BaseStationTime_t>(curTimeNanoseconds));
    if (!tickSuccess)
    {
      // If we fail to update properly, stop running
      Stop();
    }

    const auto tickAfterEngineExecution = TimeClock::now();
    const auto remaining_us = duration_cast<microseconds>(targetEndFrameTime - tickAfterEngineExecution);
    const auto tickDuration_us = duration_cast<microseconds>(tickAfterEngineExecution - tickStart);
//    PRINT_NAMED_INFO("CozmoAPI.CozmoInstanceRunner", "targetEndFrameTime:%8lld, tickDuration_us:%8lld, remaining_us:%8lld",
//                     TimeClock::time_point(targetEndFrameTime).time_since_epoch().count(), tickDuration_us.count(), remaining_us.count());

    // Only complain if we're more than 10ms behind
    if (remaining_us < microseconds(-10000))
    {
      PRINT_NAMED_WARNING("CozmoAPI.CozmoInstanceRunner.overtime", "Update() (%dms max) is behind by %.3fms",
                          BS_TIME_STEP_MS, (float)(-remaining_us).count() * 0.001f);
    }

    // Now we ALWAYS sleep, but if we're overtime, we 'sleep zero' which still
    // allows other threads to run
    static const auto minimumSleepTime_us = microseconds((long)0);
    const auto sleepTime_us = std::max(minimumSleepTime_us, remaining_us);
    {
      ANKI_CPU_PROFILE("CozmoApi.Runner.Sleep");

      std::this_thread::sleep_for(sleepTime_us);
    }

    // Set the target end time for the next frame
    targetEndFrameTime += (microseconds)(BS_TIME_STEP_MICROSECONDS);

    // See if we've fallen very far behind (this happens e.g. after a 5-second blocking
    // load operation); if so, compensate by catching the target frame end time up somewhat.
    // This is so that we don't spend the next SEVERAL frames catching up.
    const auto timeBehind_us = -remaining_us;
    static const auto kusPerFrame = ((microseconds)(BS_TIME_STEP_MICROSECONDS)).count();
    static const int kTooFarBehindFramesThreshold = 4;
    static const auto kTooFarBehindThreshold = (microseconds)(kTooFarBehindFramesThreshold * kusPerFrame);
    if (timeBehind_us >= kTooFarBehindThreshold)
    {
      const int framesBehind = (int)(timeBehind_us.count() / kusPerFrame);
      const auto forwardJumpDuration = kusPerFrame * framesBehind;
      targetEndFrameTime += (microseconds)forwardJumpDuration;
      PRINT_NAMED_WARNING("CozmoAPI.CozmoInstanceRunner.catchup",
                          "Update was too far behind so moving target end frame time forward by an additional %.3fms",
                          (float)(forwardJumpDuration * 0.001f));
    }

    tickStart = TimeClock::now();

    const auto timeSinceLastTick_us = duration_cast<microseconds>(tickStart - prevTickStart);
    prevTickStart = tickStart;

    const auto sleepTimeActual_us = duration_cast<microseconds>(tickStart - tickAfterEngineExecution);
    GetEngine()->RegisterEngineTickPerformance(tickDuration_us.count() * 0.001f,
                                               timeSinceLastTick_us.count() * 0.001f,
                                               sleepTime_us.count() * 0.001f,
                                               sleepTimeActual_us.count() * 0.001f);
  }
}

bool CozmoAPI::CozmoInstanceRunner::Update(const BaseStationTime_t currentTime_nanosec)
{
  Result updateResult;
  {
    std::lock_guard<std::mutex> lock{_updateMutex};
    updateResult = _cozmoInstance->Update(currentTime_nanosec);
  }
  if (updateResult != RESULT_OK) {
    PRINT_NAMED_ERROR("CozmoAPI.CozmoInstanceRunner.Update", "Cozmo update failed with error %d", updateResult);
  }
  return updateResult == RESULT_OK;
}

void CozmoAPI::CozmoInstanceRunner::SyncWithEngineUpdate(const std::function<void ()>& func) const
{
  std::lock_guard<std::mutex> lock{_updateMutex};
  func();
}

void CozmoAPI::CozmoInstanceRunner::SetEngineThread()
{
  // Instance is valid for lifetime of instance runner
  DEV_ASSERT(_cozmoInstance, "CozmoAPI.CozmoInstanceRunner.InvalidCozmoInstance");
  std::lock_guard<std::mutex> lock{_updateMutex};
  _cozmoInstance->SetEngineThread();
}

} // namespace Vector
} // namespace Anki
