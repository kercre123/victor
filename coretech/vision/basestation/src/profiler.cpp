/**
 * File: profiler.cpp
 *
 * Author: Andrew Stein
 * Date:   11/30/2015
 *
 * Description: Simple tic/toc style profiler
 *
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/vision/basestation/profiler.h"
#include "util/logging/logging.h"

#define SHOW_TIMING 1

namespace Anki {
namespace Vision {
  
  Profiler::~Profiler()
  {
    Print();
  }
  
# if !SHOW_TIMING
  
  void Profiler::Tic(const char*) { }
  
  double Profiler::Toc(const char*) { return 0.; }
  
  double Profiler::AverageToc(const char* timerName) { return 0.; }
  
  void Profiler::Print() const { }

# else
  
  void Profiler::Tic(const char* timerName)
  {
    // Note will construct timer if matching name doesn't already exist
    _timers[timerName].startTime = std::chrono::system_clock::now();
  }
  
  void Profiler::Timer::Update()
  {
    using namespace std::chrono;
    currentTime = duration_cast<milliseconds>(system_clock::now() - startTime);
    totalTime += currentTime;
    ++count;
  }
  
  double Profiler::Toc(const char* timerName)
  {
    auto timerIter = _timers.find(timerName);
    if(timerIter != _timers.end()) {
      Timer& timer = timerIter->second;
      timer.Update();
      return timer.currentTime.count();
    } else {
      return 0;
    }
  }
  
  double Profiler::AverageToc(const char* timerName)
  {
    auto timerIter = _timers.find(timerName);
    if(timerIter != _timers.end()) {
      Timer& timer = timerIter->second;
      timer.Update();
      if(timer.count > 0) {
        return (double)(timer.totalTime.count() / (double)timer.count);
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  }
  
  void Profiler::Print() const
  {
    for(auto & timerPair : _timers)
    {
      const Timer& timer = timerPair.second;
      PRINT_NAMED_INFO("Profiler", "%s averaged %.2fms over %d calls",
                       timerPair.first,
                       (timer.count > 0 ? (double)timer.totalTime.count() / (double)timer.count : 0),
                       timer.count);
    }
  }
  
# endif // SHOW_TIMING
  
} // namespace Vision
} // namespace Anki

