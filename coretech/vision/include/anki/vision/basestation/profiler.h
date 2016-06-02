/**
 * File: profiler.h
 *
 * Author: Andrew Stein
 * Date:   11/30/2015
 *
 * Description: Simple tic/toc style profiler
 *
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Anki_Vision_Profiler_H__
#define __Anki_Vision_Profiler_H__

#include <chrono>
#include <unordered_map>
#include <string>
#include <cstdint>

namespace Anki {
namespace Vision {
  
  class Profiler
  {
  public:
    
    // Calls Print() on destruction
    ~Profiler();
    
    // Purely used for creating a more specific event name for printing timing
    // messages: "Profiler.<name>". If you don't set this, the generic event name
    // "Profiler" will be used.
    void SetProfileGroupName(const char* name);
    
    // Start named timer (create if first call, un-pause otherwise)
    void Tic(const char* timerName);
    
    // Pause named timer and return time since last tic in milliseconds
    double Toc(const char* timerName);
    
    // Pause named timer and return average time in milliseconds for all tic/toc
    // pairs up to now
    double AverageToc(const char* timerName);
    
    // Print average time per tic/toc pair for each registered timer
    void PrintAverageTiming() const;
    
    // Set the minimum time between automatic average timing prints in milliseconds
    void SetPrintFrequency(int64_t printFrequency_ms) { _timeBetweenPrints_ms = printFrequency_ms; }
    
  private:
    
    struct Timer {
      std::chrono::time_point<std::chrono::system_clock> startTime;
      std::chrono::time_point<std::chrono::system_clock> lastPrintTime = std::chrono::system_clock::now();
      std::chrono::milliseconds currentTime;
      std::chrono::milliseconds totalTime = std::chrono::milliseconds(0);
      int count = 0;
      
      void Update();
    };
    
    using TimerContainer = std::unordered_map<const char*, Profiler::Timer>;
    
    TimerContainer _timers;
    
    std::string _eventName = "Profiler";
    
    int64_t _timeBetweenPrints_ms = -1;
    
    void PrintTimerData(const char* name, const Timer& timer) const;
    
  }; // class Profiler
  
} // namespace Vision
} // namespace Anki

#endif // #ifndef __Anki_Vision_Profiler_H__

