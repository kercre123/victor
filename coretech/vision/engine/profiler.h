/**
 * File: profiler.h
 *
 * Author: Andrew Stein
 * Date:   11/30/2015
 *
 * Description: Simple tic/toc style profiler which can track and log averages, as well as report to DAS.
 *              See also: ScopedTicToc for even more basic timing.
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
  
  // Move to coretech common VIC-5101
  class Profiler
  {
  public:
    
    // TODO: Create templated Profiler_ class, with Resolution as template argument. COZMO-3246
    // Then alias Profiler = Profiler_<milliseconds> and HighResProfiler = Profiler_<microseconds>.
    using Resolution = std::chrono::milliseconds;
    using ClockType  = std::chrono::high_resolution_clock;
    
    static_assert(ClockType::is_steady, "ClockType should be steady");
    
    // Name is purely used for creating a more specific event name for printing timing
    // messages: "Profiler.<name>".
    Profiler(const char* name = "Profiler");
    
    // Calls Print() on destruction
    ~Profiler();
    
    // For setting name after construction
    void SetProfileGroupName(const char* name);
    // name of the channel to print the info to
    inline void SetPrintChannelName(const char* name) { _printChannelName = name; }
    
    // Start named timer (create if first call, un-pause otherwise)
    void Tic(const char* timerName);
    
    // Pause named timer and return time since last tic in milliseconds
    double Toc(const char* timerName);
    
    // Pause named timer and return average time in milliseconds for all tic/toc
    // pairs up to now
    double AverageToc(const char* timerName);
    
    // Print average time per tic/toc pair for each registered timer
    void PrintAverageTiming();
    
    // Set the minimum time between automatic average timing prints in milliseconds
    void SetPrintFrequency(int64_t printFrequency_ms) { _timeBetweenPrints_ms = printFrequency_ms; }
    
    // Set the minimum time between logging timing info to DAS, in milliseconds
    void SetDasLogFrequency(int64_t dasFrequency_ms) { _timeBetweenDasLogging_ms = dasFrequency_ms; }
    
    // For scoped Tic/Toc pair. Use TicToc() method below. This is just defining the object.
    // This is similar to the ScopedTicToc object but provides the additional stat tracking
    // and logging offered by the Profiler class.
    class TicTocObject
    {
    public:
      TicTocObject(Profiler& profiler, const char* timerName);
      ~TicTocObject();
    private:
      Profiler&   _profiler;
      const char* _timerName = nullptr;
    };
    
    // Create a TicToc object to do Tic on instantiation and Toc when going out of scope
    //  For example:
    //   {
    //     auto ticToc = profiler.TicToc("Foo");
    //     // Do stuff
    //   }
    TicTocObject TicToc(const char* timerName);
    
  private:
    
    struct Timer
    {
      std::chrono::time_point<ClockType> startTime;
      std::chrono::time_point<ClockType> lastPrintTime  = ClockType::now();
      std::chrono::time_point<ClockType> lastDasLogTime = ClockType::now();
      
      Resolution currentTime;
      Resolution totalTime             = Resolution(0);
      Resolution totalTimeAtLastPrint  = Resolution(0);
      Resolution totalTimeAtLastDasLog = Resolution(0);
      
      int count             = 0;
      int countAtLastPrint  = 0;
      int countAtLastDasLog = 0;
      
      void Update();
    };
    
    using TimerContainer = std::unordered_map<const char*, Profiler::Timer>;
    
    TimerContainer _timers;
    
    std::string _eventName;
    std::string _printChannelName = "Profiler";
    
    int64_t _timeBetweenPrints_ms = -1;
    int64_t _timeBetweenDasLogging_ms = -1;
    
    void PrintTimerData(const char* name, Timer& timer);
    void LogTimerDataToDAS(const char* name, Timer& timer);
    
  }; // class Profiler
  
  inline Profiler::TicTocObject Profiler::TicToc(const char* timerName) {
    return TicTocObject(*this, timerName);
  }

} // namespace Vision
} // namespace Anki

#endif // #ifndef __Anki_Vision_Profiler_H__

