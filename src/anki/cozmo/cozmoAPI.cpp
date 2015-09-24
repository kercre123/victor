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
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include <chrono>

namespace Anki {
namespace Cozmo {

#pragma mark --- CozmoAPI Methods ---

Result CozmoAPI::StartRun(Util::Data::DataPlatform* dataPlatform, const Json::Value& config)
{
  // If there's already a thread running, we'll kill and restart
  if (_cozmoRunnerThread.joinable())
  {
    Clear();
  }
  else if (nullptr != _cozmoRunner)
  {
    PRINT_NAMED_ERROR("CozmoAPI.StartRun", "Non-threaded Cozmo already created!");
    return Result::RESULT_FAIL;
  }
  
  // Init the InstanceRunner
  Result gameInitResult;
  _cozmoRunner = new CozmoInstanceRunner(dataPlatform, config, gameInitResult);
  
  // Start the thread
  _cozmoRunnerThread = std::thread(&CozmoInstanceRunner::Run, _cozmoRunner);
  
  return gameInitResult;
}

Result CozmoAPI::Start(Util::Data::DataPlatform* dataPlatform, const Json::Value& config)
{
  // If we have a joinable thread already, we can't start
  if (_cozmoRunnerThread.joinable())
  {
    PRINT_NAMED_ERROR("CozmoAPI.Start", "Cozmo already running in thread!");
    return Result::RESULT_FAIL;
  }
  
  // If we already had an instance, kill it and start again
  if (nullptr != _cozmoRunner)
  {
    delete _cozmoRunner;
    _cozmoRunner = nullptr;
  }
  
  // Game init happens in CozmoInstanceRunner construction, so we get the result
  Result gameInitResult;
  _cozmoRunner = new CozmoInstanceRunner(dataPlatform, config, gameInitResult);
  
  return gameInitResult;
}

Result CozmoAPI::Update(const double currentTime_sec)
{
  // If we have a joinable thread already, shouldn't be updating
  if (_cozmoRunnerThread.joinable())
  {
    PRINT_NAMED_ERROR("CozmoAPI.Update", "Cozmo running in thread - can not be externally updated!");
    return Result::RESULT_FAIL;
  }
  
  if (nullptr == _cozmoRunner)
  {
    PRINT_NAMED_ERROR("CozmoAPI.Update", "Cozmo has not been started!");
    return Result::RESULT_FAIL;
  }
  
  return _cozmoRunner->Update(currentTime_sec);
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
    if (nullptr != _cozmoRunner)
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
  
  Util::SafeDelete(_cozmoRunner);
}

#pragma mark --- CozmoInstanceRunner Methods ---

CozmoAPI::CozmoInstanceRunner::CozmoInstanceRunner(Util::Data::DataPlatform* dataPlatform,
                                                   const Json::Value& config, Result& initResult)
: _cozmoInstance(dataPlatform)
, _isRunning(true)
{
  initResult = _cozmoInstance.Init(config);
}
  
void CozmoAPI::CozmoInstanceRunner::Run()
{
  auto runStart = std::chrono::system_clock::now();
  auto tickStart = runStart;
  
  while(_isRunning)
  {
    std::chrono::duration<double> timeSeconds = tickStart - runStart;
    
    Update(timeSeconds.count());
    
    auto tickNow = std::chrono::system_clock::now();
    auto ms_left = std::chrono::milliseconds(BS_TIME_STEP) - std::chrono::duration_cast<std::chrono::milliseconds>(tickNow - tickStart);
    if (ms_left < std::chrono::milliseconds(0))
    {
      // Don't sleep if we're overtime, but only complain if we're more than 10ms overtime
      if (ms_left < std::chrono::milliseconds(-10))
      {
        PRINT_NAMED_WARNING("CozmoInstanceRunner.overtime", "over by %lld ms", std::chrono::duration_cast<std::chrono::milliseconds>(-ms_left).count());
      }
    }
    else
    {
      std::this_thread::sleep_for(ms_left);
    }
    
    tickStart = tickNow;
  }
}

Result CozmoAPI::CozmoInstanceRunner::Update(const double currentTime_sec)
{
  return _cozmoInstance.Update(static_cast<float>(currentTime_sec));
}
  
} // namespace Cozmo
} // namespace Anki