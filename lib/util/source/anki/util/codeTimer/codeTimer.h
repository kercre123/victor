/**
 *  File: codeTimer
 *  RAMS
 *
 *  Description: Profile the time spent in blocks of code
 *
 *  Author: Stuart Eichert
 *  Created: 8/16/2015
 *
 *  Copyright (c) 2015 Anki. All rights reserved.
 **/

#ifndef __util_codeTimer_codeTimer_H__
#define __util_codeTimer_codeTimer_H__

#include <chrono>

namespace Anki {
namespace Util {

class CodeTimer
{
public:
  using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
  using TimeAccumulator = std::chrono::nanoseconds;
  using MillisecondDuration = std::chrono::duration<int,std::milli>;
  static inline TimePoint Start() { return std::chrono::steady_clock::now();}
  static inline int MillisecondsElapsed(const TimePoint start) {
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    return std::chrono::duration_cast<MillisecondDuration>(diff).count();
  }
  static inline TimeAccumulator InitTimeAccumulator() {
    return TimeAccumulator::zero();
  }
  static inline void Accumulate(const TimePoint start, TimeAccumulator& accumulator) {
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    accumulator += diff;
  }
  static inline int MillisecondsElapsed(const TimeAccumulator accumulator) {
    return std::chrono::duration_cast<MillisecondDuration>(accumulator).count();
  }
};

} // namespace Util
} // namespace Anki

#endif // __util_codeTimer_codeTimer_H__
