/**
 * File: util/transport/netTimeStamp.cpp
 *
 * Author: Mark Wesley
 * Created: 01/06/16
 *
 * Description: TimeStamp for messages and connection info (milliseconds since 1st timestamp)
 *              Only relevant on local device - each device will have a completely different start
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "util/transport/netTimeStamp.h"
#include "util/logging/logging.h"
#include <chrono>


namespace Anki {
namespace Util {

  using NetTimeStampClock = std::chrono::steady_clock;
  
  NetTimeStamp GetCurrentNetTimeStamp() // Milliseconds since 1st timestamp
  {
    using namespace std::chrono;

    // Initialized *first* time function is called
    static const NetTimeStampClock::time_point sEpochStartTime = NetTimeStampClock::now();

    // Initialized *each* time function is called
    const NetTimeStampClock::time_point now = NetTimeStampClock::now();
    
    // Calculate difference between time points
    const microseconds numMicroSecondsSinceStart = duration_cast<microseconds>(now - sEpochStartTime);
    const NetTimeStamp numMilliSecondsSinceStart = 0.001 * numMicroSecondsSinceStart.count();
    
    DEV_ASSERT_MSG(numMilliSecondsSinceStart >= 0.0, "NetTimeStamp.NegativeTimeStamp",
                   "Unexpected Timestamp = %f", numMilliSecondsSinceStart);
    
    return numMilliSecondsSinceStart;
  }

  // Force sEpochStartTime to initialize in main app static init to ensure thread safety
  static NetTimeStamp kFirstTimeStamp = GetCurrentNetTimeStamp();
 

} // end namespace Util
} // end namespace Anki
