/**
 * File: util/logging/DAS.cpp
 *
 * Description: DAS extensions for Util::Logging macros
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "DAS.h"

#include <time.h>

namespace Anki {
namespace Util {
namespace DAS {

//
// If boot clock is available, use it, else fall back to monotonic clock
//
#if defined(CLOCK_BOOTTIME)
#define CLOCK CLOCK_BOOTTIME
#else
#define CLOCK CLOCK_MONOTONIC
#endif

uint64_t UptimeMS()
{
  struct timespec ts{0};
  clock_gettime(CLOCK, &ts);
  return (ts.tv_sec * 1000) + (ts.tv_nsec/1000000);
}

} // end namespace DAS
} // end namespace Util
} // end namespace Anki
