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
    PrintAverageTiming();
  }
  
# if !SHOW_TIMING
  
  void Profiler::SetProfileGroupName(const char* name) {}
  
  void Profiler::Tic(const char*) { }
  
  double Profiler::Toc(const char*) { return 0.; }
  
  double Profiler::AverageToc(const char* timerName) { return 0.; }
  
  void Profiler::PrintAverageTiming() const { }

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
      const auto currentTime = std::chrono::system_clock::now();
      const auto timeDiff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - timer.lastPrintTime);
      if (_timeBetweenPrints_ms >= 0 &&
          timeDiff_ms.count() > _timeBetweenPrints_ms)
      {
        PrintTimerData(timerName, timer);
        timer.lastPrintTime = currentTime;
      }
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
  
  void Profiler::SetProfileGroupName(const char *name)
  {
    _eventName = "Profiler.";
    _eventName += name;
  }
  
  void Profiler::PrintAverageTiming()
  {
    for(auto & timerPair : _timers)
    {
      PrintTimerData(timerPair.first, timerPair.second);
    }
  }
  
  void Profiler::PrintTimerData(const char* name, Timer& timer)
  {
    const auto timeSinceLastPrint = timer.totalTime.count() - timer.totalTimeAtLastPrint.count();
    const auto countSinceLastPrint = timer.count - timer.countAtLastPrint;
    
    const double avgOverAllTime = (timer.count > 0 ? (double)timer.totalTime.count() / (double)timer.count : 0);
    const double avgSinceLastPrint = (countSinceLastPrint > 0 ? (double)timeSinceLastPrint / (double)countSinceLastPrint : 0);
    
    PRINT_NAMED_INFO(_eventName.c_str(), "%s averaged %.2fms over %d calls (%.2fms over %d calls since last print)",
                     name,
                     avgOverAllTime,
                     timer.count,
                     avgSinceLastPrint,
                     countSinceLastPrint);
    
    timer.totalTimeAtLastPrint = timer.totalTime;
    timer.countAtLastPrint = timer.count;
  }
  
# endif // SHOW_TIMING
  
} // namespace Vision
} // namespace Anki

