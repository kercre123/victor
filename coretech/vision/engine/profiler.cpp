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

#include "coretech/vision/engine/profiler.h"
#include "util/logging/logging.h"

#define SHOW_TIMING 1

namespace Anki {
namespace Vision {

  static const std::string kDasEventNamePrefix("robot.vision.profiler.");
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

  inline static bool IsTimeToPrintOrLog(const std::chrono::time_point<Profiler::ClockType>& currentTime,
                                        const int64_t timeBetween,
                                        std::chrono::time_point<Profiler::ClockType>& lastTime)
  {
    // How long since last time
    const auto lastPrintTimeDiff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime);

    // Has it been long enough?
    const bool isTimeToPrint = (timeBetween >= 0 && lastPrintTimeDiff_ms.count() > timeBetween);

    if (isTimeToPrint)
    {
      // Update last time for next check
      lastTime = currentTime;
      return true;
    }

    return false;
  }

  double Profiler::Toc(const char* timerName)
  {
    auto timerIter = _timers.find(timerName);
    if(timerIter != _timers.end())
    {
      Timer& timer = timerIter->second;
      timer.Update();
      const auto currentTime = ClockType::now();

      // Print to log if it's time
      if(IsTimeToPrintOrLog(currentTime, _timeBetweenPrints_ms, timer.lastPrintTime))
      {
        PrintTimerData(timerName, timer);
      }

      // Log to DAS if it's time
      if(IsTimeToPrintOrLog(currentTime, _timeBetweenDasLogging_ms, timer.lastDasLogTime))
      {
        LogTimerDataToDAS(timerName, timer);
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
    return "us";
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

    const char* units = GetAbbreviation<Resolution>();
    PRINT_CH_INFO(_printChannelName.c_str(), _eventName.c_str(),
                  "%s averaged %.4f%s over %d calls (%.4f%s over %d calls since last print)",
                  name, avgOverAllTime, units,
                  timer.count, avgSinceLastPrint, units,
                  countSinceLastPrint);

    timer.totalTimeAtLastPrint = timer.totalTime;
    timer.countAtLastPrint = timer.count;
  }

  void Profiler::LogTimerDataToDAS(const char* name, Timer& timer)
  {
    const auto timeSinceLastDasLog = timer.totalTime.count() - timer.totalTimeAtLastDasLog.count();
    const auto countSinceLastDasLog = timer.count - timer.countAtLastDasLog;

    const char* units = GetAbbreviation<Resolution>();

    // Log overall and recent times to DAS, coupled with number of calls in the DDATA field,
    // so we can compute averages / rates later from the logs
    // NOTE: using camel case here because event and timer names are already
    //       being specified in camel case
    const std::string fullPrefix(kDasEventNamePrefix + _eventName + "." + name);
    Util::sInfoF((fullPrefix + ".OverallTime_" + units).c_str(),
                 {{DDATA, std::to_string(timer.count).c_str()}},
                 "%lld", timer.totalTime.count());
    Util::sInfoF((fullPrefix + ".RecentTime_" + units).c_str(),
                 {{DDATA, std::to_string(countSinceLastDasLog).c_str()}},
                 "%lld", timeSinceLastDasLog);

    timer.totalTimeAtLastDasLog = timer.totalTime;
    timer.countAtLastDasLog = timer.count;
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
