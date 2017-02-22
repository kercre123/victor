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

#include "anki/cozmo/cozmoAPI.h"
#include "anki/cozmo/basestation/cozmoEngine.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/cozmo/game/comms/gameMessagePort.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "clad/externalInterface/messageShared.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include <chrono>

namespace Anki {
namespace Cozmo {

#pragma mark --- CozmoAPI Methods ---

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
  
  ANKI_CPU_TICK("CozmoEngineWebots", kMaxDesiredEngineDuration, Util::CpuThreadProfiler::kLogFrequencyNever);
  
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

void CozmoAPI::ExecuteBackgroundTransfers()
{
  CozmoEngine* engine = (_cozmoRunner != nullptr) ? _cozmoRunner->GetEngine() : nullptr;
  if (engine == nullptr) {
    return;
  }
  engine->ExecuteBackgroundTransfers();
}

size_t CozmoAPI::SendVizMessages(uint8_t* buffer, size_t bufferSize)
{
  GameMessagePort* messagePipe = (_cozmoRunner != nullptr) ? _cozmoRunner->GetVizMessagePort() : nullptr;
  if (messagePipe == nullptr) {
    return 0;
  }

  return messagePipe->PullToGameMessages(buffer, bufferSize);
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
  }
  
  _cozmoRunner.reset();
}

#pragma mark --- CozmoInstanceRunner Methods ---

CozmoAPI::CozmoInstanceRunner::CozmoInstanceRunner(Util::Data::DataPlatform* dataPlatform,
                                                   const Json::Value& config, bool& initResult)
: _gameMessagePort(new GameMessagePort(ExternalInterface::kDirectCommsBufferSize,
                                       !config.get("standalone", false).asBool()))
, _vizMessagePort(CreateVizMessagePort())
, _cozmoInstance(new CozmoEngine(dataPlatform, _gameMessagePort.get(), _vizMessagePort.get()))
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
  using TimeClock = std::chrono::steady_clock;
  auto runStart = TimeClock::now();

  while(_isRunning)
  {
    ANKI_CPU_TICK("CozmoEngine", kMaxDesiredEngineDuration, Util::CpuThreadProfiler::kLogFrequencyNever);
    
    auto tickStart = TimeClock::now();

    std::chrono::duration<double> timeSeconds = tickStart - runStart;
    double timeNanoseconds = Util::SecToNanoSec(timeSeconds.count());

    // If we fail to update properly stop running
    if (!Update(Util::numeric_cast<BaseStationTime_t>(timeNanoseconds)))
    {
      Stop();
    }
    
    const auto minimumSleepTimeMs = std::chrono::milliseconds( (long)(BS_TIME_STEP * 0.2) ); // 60 * 0.2 = 12 milliseconds!?
    
    auto tickNow = TimeClock::now();
    auto ms_left = std::chrono::milliseconds(BS_TIME_STEP) - std::chrono::duration_cast<std::chrono::milliseconds>(tickNow - tickStart);
    if (ms_left < std::chrono::milliseconds(0))
    {
      // Don't sleep if we're overtime, but only complain if we're more than 10ms overtime
      if (ms_left < std::chrono::milliseconds(-10))
      {
        PRINT_NAMED_WARNING("CozmoAPI.CozmoInstanceRunner.overtime", "Update() (%dms max) ran over by %lldms", BS_TIME_STEP, (-ms_left).count());
      }
    }
    else
    {
      ANKI_CPU_PROFILE("CozmoApi.Runner.Sleep");
      
      if (ms_left < minimumSleepTimeMs)
      {
        ms_left = minimumSleepTimeMs;
      }
      std::this_thread::sleep_for(ms_left);
    }
  }
}

bool CozmoAPI::CozmoInstanceRunner::Update(const BaseStationTime_t currentTime_nanosec)
{
  Result updateResult = _cozmoInstance->Update(currentTime_nanosec);
  if (updateResult != RESULT_OK) {
    PRINT_NAMED_ERROR("CozmoAPI.CozmoInstanceRunner.Update", "Cozmo update failed with error %d", updateResult);
  }
  return updateResult == RESULT_OK;
}

GameMessagePort* CozmoAPI::CozmoInstanceRunner::CreateVizMessagePort()
{
  #if VIZ_ON_DEVICE && ANKI_DEV_CHEATS
  return new GameMessagePort(ExternalInterface::kVizCommsBufferSize, false);
  #else
  return nullptr;
  #endif
}

} // namespace Cozmo
} // namespace Anki
