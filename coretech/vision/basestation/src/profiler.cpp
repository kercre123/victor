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
  
  Profiler::Profiler(const char* name)
  : _eventName(name)
  {
    
  }
                    
  Profiler::~Profiler()
  {
    PrintAverageTiming();
  }
  
# if !SHOW_TIMING
  
  void Profiler::SetProfileGroupName(const char* name) {}
  
  void Profiler::Tic(const char*) { }
  
  double Profiler::Toc(const char*) { return 0.; }
  
  double Profiler::AverageToc(const char* timerName) { return 0.; }
  
  void Profiler::PrintAverageTiming() { }

# else
  
  void Profiler::Tic(const char* timerName)
  {
    // Note will construct timer if matching name doesn't already exist
    _timers[timerName].startTime = ClockType::now();
  }
  
  void Profiler::Timer::Update()
  {
    currentTime = std::chrono::duration_cast<Resolution>(ClockType::now() - startTime);
    totalTime += currentTime;
    ++count;
  }
  
  double Profiler::Toc(const char* timerName)
  {
    auto timerIter = _timers.find(timerName);
    if(timerIter != _timers.end()) {
      Timer& timer = timerIter->second;
      timer.Update();
      const auto currentTime = ClockType::now();
      const auto timeDiff_ms = std::chrono::duration_cast<Resolution>(currentTime - timer.lastPrintTime);
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
    _eventName = name;
  }
  
  void Profiler::PrintAverageTiming()
  {
    for(auto & timerPair : _timers)
    {
      PrintTimerData(timerPair.first, timerPair.second);
    }
  }
  
  template<class Resolution>
  const char * GetAbbreviation();
  
  template<>
  inline const char* GetAbbreviation<std::chrono::milliseconds>() {
    return "ms";
  }
  
  template<>
  inline const char* GetAbbreviation<std::chrono::microseconds>() {
    return "µs";
  }
  
  template<>
  inline const char* GetAbbreviation<std::chrono::nanoseconds>() {
    return "ns";
  }
  
  void Profiler::PrintTimerData(const char* name, Timer& timer)
  {
    const auto timeSinceLastPrint = timer.totalTime.count() - timer.totalTimeAtLastPrint.count();
    const auto countSinceLastPrint = timer.count - timer.countAtLastPrint;
    
    const double avgOverAllTime = (timer.count > 0 ? (double)timer.totalTime.count() / (double)timer.count : 0);
    const double avgSinceLastPrint = (countSinceLastPrint > 0 ? (double)timeSinceLastPrint / (double)countSinceLastPrint : 0);
    
    PRINT_CH_INFO(_printChannelName.c_str(), _eventName.c_str(), "%s averaged %.4f%s over %d calls (%.4f%s over %d calls since last print)",
                  name,
                  avgOverAllTime,
                  GetAbbreviation<Resolution>(),
                  timer.count,
                  avgSinceLastPrint,
                  GetAbbreviation<Resolution>(),
                  countSinceLastPrint);
    
    timer.totalTimeAtLastPrint = timer.totalTime;
    timer.countAtLastPrint = timer.count;
  }
  
  Profiler::TicTocObject::TicTocObject(Profiler& profiler, const char* timerName)
  : _profiler(profiler)
  , _timerName(timerName)
  {
    _profiler.Tic(timerName);
  }
  
  Profiler::TicTocObject::~TicTocObject()
  {
    _profiler.Toc(_timerName);
  }

# endif // SHOW_TIMING
  
} // namespace Vision
} // namespace Anki

