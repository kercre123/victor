/**
 * File: universalTime
 *
 * Author: damjan
 * Created: 4/14/14
 * 
 * Description: Helper functions for measuring time on different platforms
 * 
 *
 * Copyright: Anki, Inc. 2014
 *
 **/
#include "util/time/universalTime.h"
#if defined(__MACH__) && defined(__APPLE__)
#include <mach/mach_time.h>
#elif defined(ANDROID) || defined(LINUX) || defined(VICOS)
#include <time.h>
#endif
#include "util/math/numericCast.h"


namespace Anki{ namespace Util {

namespace Time {

// returns time in nanoseconds
unsigned long long int UniversalTime::GetCurrentTimeInNanoseconds()
{
#if defined(__MACH__) && defined(__APPLE__)
  static mach_timebase_info_data_t s_timebase_info;

  if (s_timebase_info.denom == 0) {
    (void) mach_timebase_info(&s_timebase_info);
  }

  return (unsigned long long int)((mach_absolute_time() * s_timebase_info.numer) /  s_timebase_info.denom);
#elif defined(ANDROID) || defined(LINUX) || defined(VICOS)
  timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (unsigned long long)(t.tv_sec*1000000000ULL) + (unsigned long long)t.tv_nsec;
#else
  return 0;
#endif
}

  // returns time in milliseconds
double UniversalTime::GetCurrentTimeInMilliseconds()
{
  const unsigned long long int timeNano = GetCurrentTimeInNanoseconds();
  const double timeMs = numeric_cast<double>(timeNano) * 1e-6;
  return timeMs;
}

// returns time in milliseconds
double UniversalTime::GetCurrentTimeInSeconds()
{
  const unsigned long long int timeNano = GetCurrentTimeInNanoseconds();
  const double time_s = numeric_cast<double>(timeNano) * 1e-9;
  return time_s;
}

// returns time value
// note this is not in Milli Seconds nor nanoseconds
// use GetCurrentTimeInNanoseconds to get nanoseconds
unsigned long long int UniversalTime::GetCurrentTimeValue()
{
#if defined(__MACH__) && defined(__APPLE__)
  return mach_absolute_time();
#elif defined(ANDROID)  || defined(LINUX) || defined(VICOS)
  timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (unsigned long long)(t.tv_sec*1000000000ULL) + (unsigned long long)t.tv_nsec;
#else
  return 0;
#endif
}

// returns nanoseconds since previous time
unsigned long long int UniversalTime::GetNanosecondsElapsedSince(unsigned long long int previousTime)
{
#if defined(__MACH__) && defined(__APPLE__)
  // Stop the clock.
  uint64_t end = mach_absolute_time();

  uint64_t        elapsed;
  static mach_timebase_info_data_t    sTimebaseInfo;

  // Calculate the duration.
  elapsed = end - previousTime;

  // Convert to nanoseconds.

  // If this is the first time we've run, get the timebase.
  // We can use denom == 0 to indicate that sTimebaseInfo is
  // uninitialised because it makes no sense to have a zero
  // denominator is a fraction.
  if ( sTimebaseInfo.denom == 0 ) {
    (void) mach_timebase_info(&sTimebaseInfo);
  }

  // Do the maths. We hope that the multiplication doesn't
  // overflow; the price you pay for working in fixed point.
  return elapsed * sTimebaseInfo.numer / sTimebaseInfo.denom;
#elif defined(ANDROID) || defined(LINUX) || defined(VICOS)
  timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return ((unsigned long long)(t.tv_sec*1000000000ULL) + (unsigned long long)t.tv_nsec) - previousTime;
#else
  return 0;
#endif
}

} // end namespace Time

} // end namespace Anki
} // namespace